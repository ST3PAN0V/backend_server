#pragma once
#include "sdk.h"
#include "model.h"

// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <optional>
#include <filesystem>
#include <fstream>

#define BADREQUEST "badRequest"s
#define MAPNOTFOUND "mapNotFound"s

namespace http_server {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
namespace fs = std::filesystem;
using tcp = net::ip::tcp;
using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;
using namespace std::literals;

enum class RequestType {
    GET,
    HEAD,
    OTHER
};

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view APP_JSON = "application/json"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};

inline void ReportError(beast::error_code ec, std::string_view what) {
    std::cerr << what << ": "sv << ec.message() << std::endl;
}

class SessionBase {
public:
    // Запрещаем копирование и присваивание объектов SessionBase и его наследников
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;

    void Run();

protected:
    explicit SessionBase(tcp::socket&& socket);

    using HttpRequest = http::request<http::string_body>;

    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields>&& response) {
        // Запись выполняется асинхронно, поэтому response перемещаем в область кучи
        auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));

        auto self = GetSharedThis();
        http::async_write(stream_, *safe_response,
                            [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                                self->OnWrite(safe_response->need_eof(), ec, bytes_written);
                            });
    }

    void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written);

    ~SessionBase() = default;

private:
    // tcp_stream содержит внутри себя сокет и добавляет поддержку таймаутов
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;

    void Read();

    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);

    void Close();

    // Обработку запроса делегируем подклассу
    virtual void HandleRequest(HttpRequest&& request) = 0;

    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;
};


template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& request_handler)
        : SessionBase(std::move(socket))
        , request_handler_(std::forward<Handler>(request_handler)) {
    }
private:
    RequestHandler request_handler_;

    std::shared_ptr<SessionBase> GetSharedThis() override {
        return this->shared_from_this();
    }  

    void HandleRequest(HttpRequest&& request) override {
        // Захватываем умный указатель на текущий объект Session в лямбде,
        // чтобы продлить время жизни сессии до вызова лямбды.
        // Используется generic-лямбда функция, способная принять response произвольного типа
        request_handler_(std::move(request), [self = this->shared_from_this()](auto&& response) {
            self->Write(std::move(response));
        });
    }
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
        : ioc_(ioc)
        // Обработчики асинхронных операций acceptor_ будут вызываться в своём strand
        , acceptor_(net::make_strand(ioc))
        , request_handler_(std::forward<Handler>(request_handler)) {
        // Открываем acceptor, используя протокол (IPv4 или IPv6), указанный в endpoint
        acceptor_.open(endpoint.protocol());

        // После закрытия TCP-соединения сокет некоторое время может считаться занятым,
        // чтобы компьютеры могли обменяться завершающими пакетами данных.
        // Однако это может помешать повторно открыть сокет в полузакрытом состоянии.
        // Флаг reuse_address разрешает открыть сокет, когда он "наполовину закрыт"
        acceptor_.set_option(net::socket_base::reuse_address(true));
        // Привязываем acceptor к адресу и порту endpoint
        acceptor_.bind(endpoint);
        // Переводим acceptor в состояние, в котором он способен принимать новые соединения
        // Благодаря этому новые подключения будут помещаться в очередь ожидающих соединений
        acceptor_.listen(net::socket_base::max_listen_connections);
    }

    void Run() {
        DoAccept();
    }

private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler request_handler_;

    void DoAccept() {
        acceptor_.async_accept(
            // Передаём последовательный исполнитель, в котором будут вызываться обработчики
            // асинхронных операций сокета
            net::make_strand(ioc_),
            // С помощью bind_front_handler создаём обработчик, привязанный к методу OnAccept
            // текущего объекта.
            // Так как Listener — шаблонный класс, нужно подсказать компилятору, что
            // shared_from_this — метод класса, а не свободная функция.
            // Для этого вызываем его, используя this
            // Этот вызов bind_front_handler аналогичен
            // namespace ph = std::placeholders;
            // std::bind(&Listener::OnAccept, this->shared_from_this(), ph::_1, ph::_2)
            beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }

    // Метод socket::async_accept создаст сокет и передаст его передан в OnAccept
    void OnAccept(sys::error_code ec, tcp::socket socket) {
        using namespace std::literals;

        if (ec) {
            return ReportError(ec, "accept"sv);
        }

        // Асинхронно обрабатываем сессию
        AsyncRunSession(std::move(socket));

        // Принимаем новое соединение
        DoAccept();
    }

    void AsyncRunSession(tcp::socket&& socket) {
        std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();
    }
};

class SyncWriteOStreamAdapter {
public:
    explicit SyncWriteOStreamAdapter(std::ostream& os)
        : os_{os} {
    }

    template <typename ConstBufferSequence>
    size_t write_some(const ConstBufferSequence& cbs, sys::error_code& ec) {
        const size_t total_size = net::buffer_size(cbs);
        if (total_size == 0) {
            ec = {};
            return 0;
        }
        size_t bytes_written = 0;
        for (const auto& cb : cbs) {
            const size_t size = cb.size();
            const char* const data = reinterpret_cast<const char*>(cb.data());
            if (size > 0) {
                if (!os_.write(reinterpret_cast<const char*>(data), size)) {
                    ec = make_error_code(boost::system::errc::io_error);
                    return bytes_written;
                }
                bytes_written += size;
            }
        }
        ec = {};
        return bytes_written;
    }

    template <typename ConstBufferSequence>
    size_t write_some(const ConstBufferSequence& cbs) {
        sys::error_code ec;
        const size_t bytes_written = write_some(cbs, ec);
        if (ec) {
            throw std::runtime_error("Failed to write");
        }
        return bytes_written;
    }

private:
    std::ostream& os_;
}; 

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    // При помощи decay_t исключим ссылки из типа RequestHandler,
    // чтобы Listener хранил RequestHandler по значению
    using MyListener = Listener<std::decay_t<RequestHandler>>;

    std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
}

bool IsSubPath(fs::path path, fs::path base);

void HandleSTATICResponse(StringResponse& response, const StringRequest& req, const model::Game& game);

StringResponse MakeErrorString(std::string errCode, StringRequest&& req);

std::optional<std::string> FoundAndGetStringMap(const std::vector<model::Map>& maps, std::string_view needIdMap);

void HandleGETResponse(StringResponse& response, const StringRequest& req, const model::Game& game);

void HandleAPIResponse(StringResponse& response, const StringRequest& req, const model::Game& game);

std::string GetStringFromMaps(const std::vector<model::Map>& maps);


StringResponse MakeStringResponse(http::status status, RequestType req_type, StringRequest&& req);

StringResponse HandleRequest(StringRequest&& req, const model::Game& game);

}  // namespace http_server

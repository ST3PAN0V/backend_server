#include "http_server.h"

namespace http_server {

void SessionBase::Run() {
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}


SessionBase::SessionBase(tcp::socket&& socket)
    : stream_(std::move(socket)) {
}

void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
    if (ec) {
        return ReportError(ec, "write"sv);
    }

    if (close) {
        // Семантика ответа требует закрыть соединение
        return Close();
    }

    // Считываем следующий запрос
    Read();
}

void SessionBase::Read() {
    using namespace std::literals;
    // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    request_ = {};
    stream_.expires_after(30s);
    // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
    http::async_read(stream_, buffer_, request_,
                        // По окончании операции будет вызван метод OnRead
                        beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    using namespace std::literals;
    if (ec == http::error::end_of_stream) {
        // Нормальная ситуация - клиент закрыл соединение
        return Close();
    }
    if (ec) {
        return ReportError(ec, "read"sv);
    }
    HandleRequest(std::move(request_));
}

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}

void MakeErrorString(http::status errCode, std::string description, const StringRequest& req, StringResponse& response) {
    auto getStrFromCode = [](http::status errCode) {
        switch (errCode) {
            case http::status::bad_request:
                return "badRequest";
                break;
            case http::status::not_found:
                return "mapNotFound";
                break;
            default:
                return "UnknownError";
                break;
        }
    };

    boost::json::object err;
    StringResponse responseErr(errCode, req.version());
    responseErr.set(http::field::content_type, ContentType::APP_JSON);
    err["code"] = getStrFromCode(errCode);
    err["message"] = description;
    responseErr.body() = boost::json::serialize(err);
    
    response = responseErr;
}

void ModifyResponseGET(StringResponse& response, const StringRequest& req, const model::Game& game) {
    response.set(http::field::content_type, ContentType::APP_JSON);
    
    if (req.target() == "/api/v1/maps") {
        response.body() = std::move(json_loader::GetStringFromMaps(game.GetMaps()));
    } else if (req.target().substr(0, 13) == "/api/v1/maps/") {
        std::string needed_map = std::string(req.target().substr(13, req.target().size()));
        auto tmp = std::move(json_loader::FoundAndGetStringMap(game.GetMaps(), needed_map));
        if (tmp.has_value()) {
            response.body() = std::move(*tmp);
        } else {
            MakeErrorString(http::status::not_found, "Map not found", req, response);
        }
    } else if (req.target().substr(0,5) == "/api/") {
        MakeErrorString(http::status::bad_request, "Bad request", req, response);
    }

    response.content_length(response.body().size());
}

StringResponse MakeStringResponse(http::status status, RequestType req_type, StringRequest&& req, const model::Game& game) {
    StringResponse response(status, req.version());
    response.keep_alive(req.keep_alive());

    switch (req_type) {
    case RequestType::GET:
        ModifyResponseGET(response, req, game);
        break;
    default:
        assert(false);
    }
    return response;
}

StringResponse HandleRequest(StringRequest&& req, const model::Game& game) {
    if (req.method_string() == "GET"s) {
        return MakeStringResponse(http::status::ok, RequestType::GET, move(req), game);
    } else {
        return MakeStringResponse(http::status::method_not_allowed, RequestType::OTHER, move(req), game);
    }
}

}  // namespace http_server

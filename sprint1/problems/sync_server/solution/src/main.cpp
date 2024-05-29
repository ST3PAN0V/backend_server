
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <thread>
#include <optional>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;
using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
};

std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer) {
    beast::error_code ec;
    StringRequest req;
    http::read(socket, buffer, req, ec);

    if (ec == http::error::end_of_stream) {
        return std::nullopt;
    }
    if (ec) {
        throw std::runtime_error("Failed to read request: "s.append(ec.message()));
    }
    return req;
} 

StringResponse MakeStringResponseHEAD(StringRequest&& req) {
    StringResponse response(http::status::ok, req.version());
    response.set(http::field::content_type, std::string(ContentType::TEXT_HTML));


    response.content_length(std::string("Hello, " + std::string(req.target()).substr(1)).size());
    return response;
}

StringResponse MakeStringResponseGET(StringRequest&& req) {
    StringResponse response(http::status::ok, req.version());
    response.set(http::field::content_type, std::string(ContentType::TEXT_HTML));

    std::string_view body = std::string("Hello, " + std::string(req.target()).substr(1));
    response.body() = body;

    response.content_length(body.size());
    return response;
}

StringResponse MakeStringResponseOTHER(StringRequest&& req) {
    StringResponse response(http::status::method_not_allowed, req.version());
    response.set(http::field::content_type, std::string(ContentType::TEXT_HTML));
    response.set(http::field::allow, "GET, HEAD");
    response.body() = "Invalid method"sv;

    response.content_length(14);
    return response;
}



StringResponse HandleRequest(StringRequest&& req) {

    if (req.method_string() == "GET"s) {
        return MakeStringResponseGET(move(req));
    } else if (req.method_string() == "HEAD"s) {
        return MakeStringResponseHEAD(move(req));
    } else {
        return MakeStringResponseOTHER(move(req));
    }
} 

void HandleConnection(tcp::socket& socket) {
    try {
        beast::flat_buffer buffer;

        while (auto request = ReadRequest(socket, buffer)) {
            StringResponse response = HandleRequest(*std::move(request));
            http::write(socket, response);
            if (response.need_eof()) {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

int main() {
    net::io_context ioc;
    const auto address = net::ip::make_address("0.0.0.0"sv);
    constexpr unsigned short port = 8080;
    tcp::acceptor acceptor(ioc, {address, port});
    std::cout << "Server has started..." << std::endl;

    while (true) {
        tcp::socket socket(ioc);
        acceptor.accept(socket);

        std::thread t(
            []
            (tcp::socket socket)
            {
                HandleConnection(socket);
            },
            std::move(socket));
        t.detach();
    }
}

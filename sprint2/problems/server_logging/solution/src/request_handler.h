#pragma once

#include "http_server.h"
#include "model.h"
#include "logger.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game) : game_(game) {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Send>
    void operator()(http_server::StringResponse& strResponse, Send&& send) {
        send(strResponse);
    }

    const model::Game& GetGame() const {
        return game_;
    }

private:
    model::Game& game_;
};

template<class SomeRequestHandler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(SomeRequestHandler& basicHandler) : decorated_(basicHandler) {}

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, const std::string& staticFolderPath, tcp::endpoint endpoint) {
        json::object requestData{{"ip"s, endpoint.address().to_string()}, {"URI"s, req.target()}, {"method", std::string(req.method_string())}};
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, requestData) << "request received"sv;

        auto startTime = std::chrono::steady_clock::now();
        http_server::StringResponse strResponse = http_server::HandleRequestSendResponse(std::move(req), decorated_.GetGame(), staticFolderPath);
        
        auto endTime = std::chrono::steady_clock::now();
        auto responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        std::string contentType = "null";
        if (strResponse.find(http::field::content_type) != strResponse.end()) contentType = std::string(strResponse[http::field::content_type]);

        decorated_(strResponse, std::move(send));
        
        json::object responseData{{"response_time"s, responseTime}, {"code"s, strResponse.result_int()}, {"content_type", contentType}};
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, responseData) << "response sent"sv;
    }
private:
    SomeRequestHandler& decorated_;
};

}  // namespace http_handler

#pragma once

#include "./http_server.h"
#include "./../logger/logger.h"
#include "./api_handler.h"

#include <functional>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
using ResponseCallback = std::function<void(StringResponse)>;

std::string DecodeUrl(const std::string& url);

class RequestHandler {
    
public:
    explicit RequestHandler(std::string static_folder, ApiHandler& api_handler)
        : resources_{std::move(static_folder)}
        , api_handler_(api_handler) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        const auto& target = DecodeUrl(std::string{req.target()});
        const auto& body = std::string{req.body()};
        HandleRequest(req.method(), target, body, req.base(), [send](StringResponse response) {
            send(std::move(response));
        });
   }

private:
    std::string GetToken(const http::fields& headers) const;

    using HttpHeaders = std::unordered_map<http::field, std::string>;
    StringResponse HandleResponse(http::status status, const std::string& data, const HttpHeaders& headers) const;
    StringResponse HandleError(http::status status, const std::string& code, const std::string& message, const HttpHeaders& headers) const;
    StringResponse HandleAuthorizationError() const;
    StringResponse HandleAllowMethodError(const std::string& allowed_methods) const;

    void HandleRequest(boost::beast::http::verb method, const std::string& target, const std::string& body,
                        const http::fields& headers, const ResponseCallback& callback) const;
    void HandleGetMaps(const ResponseCallback& callback) const;
    void HandleGetMapById(const std::string& id, const ResponseCallback& callback) const;
    void HandleGetResource(const std::string& target, const ResponseCallback& callback) const;
    void HandleJoinGame(const std::string& body, const ResponseCallback& callback) const;
    void HandleGetPlayers(const std::string& token, const ResponseCallback& callback) const;
    void HandleGetGameState(const std::string& token, const ResponseCallback& callback) const;
    void HandlePlayerAction(const std::string& token, const std::string& body, const ResponseCallback& callback) const;
    void HandleGameTick(const std::string& body, const ResponseCallback& callback) const;

    std::string resources_;
    ApiHandler& api_handler_;
};

}  // namespace http_handler

#include "request_handler.h"
#include "constants.h"

#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <regex>

namespace sys = boost::system;
namespace fs = std::filesystem;

using namespace constants;
using namespace std::literals;

namespace http_handler {

bool IsSubPath(fs::path path, fs::path base) {
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

std::string GetContentType(const std::string& path) {
    std::unordered_map<std::string, std::string> contentTypes = {
        {".htm", "text/html"},
        {".html", "text/html"},
        {".css", "text/css"},
        {".txt", "text/plain"},
        {".js", "text/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpe", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".ico", "image/vnd.microsoft.icon"},
        {".tiff", "image/tiff"},
        {".tif", "image/tiff"},
        {".svg", "image/svg+xml"},
        {".svgz", "image/svg+xml"},
        {".mp3", "audio/mpeg"}
    };

    size_t dot = path.find_last_of('.');
    if (dot != std::string::npos) {
        std::string extension = path.substr(dot);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        auto it = contentTypes.find(extension);
        if (it != contentTypes.end()) {
            return it->second;
        }
    }

    return "application/octet-stream";
}

std::string ReadFileToString(fs::path file_path) {
    std::ifstream file(file_path, std::ios::in | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + file_path.string());
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}


std::string RequestHandler::DecodeUrl(const std::string& url) const {
    std::string result;
    result.reserve(url.size());

    for (std::size_t i = 0; i < url.size(); ++i) {
        if (url[i] == '%') {
            if (i + 2 < url.size()) {
                std::string hex_str = url.substr(i + 1, 2);
                char decoded_char = static_cast<char>(std::strtol(hex_str.c_str(), nullptr, 16));
                result += decoded_char;
                i += 2;
            } else {
                result += '%';
            }
        } else if (url[i] == '+') {
            result += ' ';
        } else {
            result += url[i];
        }
    }

    return result;
}

std::string RequestHandler::GetToken(const http::fields& headers) const {
    if (headers.find(http::field::authorization) != headers.end()) {
        const auto& auth_header = headers[http::field::authorization];
        const auto& bearer_prefix = "Bearer "sv;
        if (auth_header.starts_with(bearer_prefix)) {
            std::string token_str{auth_header.substr(bearer_prefix.size())};

            //проверка валидности
            static const std::regex hex_regex("^[0-9a-zA-Z]{32}$");
            if (std::regex_match(token_str, hex_regex)) {
                return token_str;
            }
        }
    }
    return {};
}

StringResponse RequestHandler::HandleResponse(http::status status, const std::string& data, const HttpHeaders& headers) const {
    StringResponse res;
    res.result(status);
    res.body() = data;
    for (auto const& [field, value] : headers) {
        res.set(field, value);
    }
    res.prepare_payload();
    return res;
}

StringResponse RequestHandler::HandleError(http::status status, const std::string& code, const std::string& message, const HttpHeaders& headers) const {
    json::object obj;
    obj[CODE] = code;
    obj[MESSAGE] = message;

    return HandleResponse(status, json::serialize(obj), headers);
}

StringResponse RequestHandler::HandleAuthorizationError() const {
    return HandleError(http::status::unauthorized,
                       "invalidToken", "Authorization header is missing",
                        {{http::field::content_type, "application/json"},
                         {http::field::cache_control, "no-cache"}});
}

StringResponse RequestHandler::HandleAllowMethodError(const std::string& allowed_methods) const {
    return HandleError(http::status::method_not_allowed,
                       "invalidMethod", "Invalid method",
                       {{http::field::content_type, "application/json"},
                        {http::field::cache_control, "no-cache"},
                        {http::field::allow, allowed_methods}});
}


void RequestHandler::HandleRequest(http::verb method, const std::string& target, const std::string& body,
                                    const http::fields& headers, const ResponseCallback& callback) const {
    //старт игры
    if (target == "/api/v1/game/join") {
        if (method == http::verb::post) {
            HandleJoinGame(body, callback);
            return;
        } else {
            callback(HandleAllowMethodError("POST"));
            return;
        }
    }

    //игровое состояние
    if (target == "/api/v1/game/state") {
        if (method == http::verb::get || method == http::verb::head) {
            if (auto token = GetToken(headers); !token.empty()) {
                HandleGetGameState(token, callback);
                return;
            }
            callback(HandleAuthorizationError());
            return;
        } else {
            callback(HandleAllowMethodError("GET, HEAD"));
            return;
        }
    }

    //управление временем
    if (target == "/api/v1/game/tick") {
        if (method == http::verb::post) {
            HandleGameTick(body, callback);
            return;
        } else {
            callback(HandleAllowMethodError("POST"));
            return;
        }
    }

    //список карт
    if (target == "/api/v1/maps") {
        if (method == http::verb::get || method == http::verb::head) {
            HandleGetMaps(callback);
            return;    
        } else {
            callback(HandleAllowMethodError("GET, HEAD"));
            return;
        }
    }

    //данные карты
    if (target.starts_with("/api/v1/maps/")) {
        if (method == http::verb::get || method == http::verb::head) {
            const auto& id = target.substr(std::string("/api/v1/maps/").size());
            HandleGetMapById(id, callback);
            return;
        } else {
            callback(HandleAllowMethodError("GET, HEAD"));
            return;
        }
    }

    //список игроков
    if (target == "/api/v1/game/players") {
        if (method == http::verb::get || method == http::verb::head) {
            if (auto token = GetToken(headers); !token.empty()) {
                HandleGetPlayers(token, callback);
                return;
            }
            callback(HandleAuthorizationError());
            return;
        } else {
            callback(HandleAllowMethodError("GET, HEAD"));
            return;
        }
    }

    if (target == "/api/v1/game/player/action") {
        if (method == http::verb::post) {
            if (headers.find(http::field::content_type) == headers.end() || headers[http::field::content_type] != "application/json") {
                callback(HandleError(http::status::bad_request,
                                     "invalidArgument", "Invalid content type",
                                     {{http::field::content_type, "application/json"},
                                      {http::field::cache_control, "no-cache"}}));
                return;
            }
            if (auto token = GetToken(headers); !token.empty()) {
                HandlePlayerAction(token, body, callback);
                return;
            }
            callback(HandleAuthorizationError());
            return;
        } else {
            callback(HandleAllowMethodError("POST"));
            return;
        }
    }

    if (!target.starts_with("/api/")) {
        if (method == http::verb::get || method == http::verb::head) {
            HandleGetResource(target.substr(1), callback);//удаляем первый /, чтобы сделать путь относительным
            return;
        }
    }

    callback(HandleError(http::status::bad_request,
                         "badRequest", "Bad request",
                         {{http::field::content_type, "application/json"},
                          {http::field::cache_control, "no-cache"}}));
}

void RequestHandler::HandleGetMaps(const ResponseCallback& callback) const {
    api_handler_.GetMaps([this, callback](const std::string& data){
        callback(HandleResponse(http::status::ok, data,
                                {{http::field::content_type, "application/json"},
                                 {http::field::cache_control, "no-cache"}}));
    });
}

void RequestHandler::HandleGetMapById(const std::string& id, const ResponseCallback& callback) const {
    try {
        api_handler_.GetMapById(id, [this, callback](const std::string& data){
            callback(HandleResponse(http::status::ok, data,
                                    {{http::field::content_type, "application/json"},
                                     {http::field::cache_control, "no-cache"}}));
        });
    } catch (const ApiException& ex) {
        callback(HandleError(ex.status, ex.code, ex.what(),
                             {{http::field::content_type, "application/json"},
                              {http::field::cache_control, "no-cache"}}));
    }
}

void RequestHandler::HandleGetResource(const std::string& target, const ResponseCallback& callback) const {
    fs::path path{target};
    fs::path resources_path{resources_};

    fs::path abs_path = fs::weakly_canonical(resources_path / path);
    if (!IsSubPath(abs_path, resources_path)) {
        callback(HandleError(http::status::bad_request,
                             "BadRequest", "Path is out of resources folder",
                             {{http::field::content_type, "text/plain"}}));
        return;
    }
    if (!fs::exists(abs_path)) {
        callback(HandleError(http::status::not_found,
                             "NotFound", "Resource not found",
                             {{http::field::content_type, "text/plain"}}));
        return;
    }
    if (fs::is_directory(abs_path)) {
        abs_path /= "index.html";
    }

    std::string file_content;
    try {
        file_content = ReadFileToString(abs_path);
        callback(HandleResponse(http::status::ok, std::move(file_content),
                                {{http::field::content_type, GetContentType(abs_path)}}));
    } catch( const std::exception& ex) {
        callback(HandleError(http::status::not_found,
                             "NotFound", "Failed to open file",
                             {{http::field::content_type, "text/plain"}}));
    }
}

void RequestHandler::HandleJoinGame(const std::string& body, const ResponseCallback& callback) const {
    try {
        api_handler_.JoinGame(body, [this, callback](const std::string& data){
            callback(HandleResponse(http::status::ok, data,
                                    {{http::field::content_type, "application/json"},
                                     {http::field::cache_control, "no-cache"}}));
        });
        return;
    } catch (const ApiException& ex) {
        callback(HandleError(ex.status, ex.code, ex.what(),
                             {{http::field::content_type, "application/json"},
                              {http::field::cache_control, "no-cache"}}));
    } catch (...) {
        callback(HandleError(http::status::bad_request, "invalidArgument",
                             "Join game request parse error",
                             {{http::field::content_type, "application/json"},
                              {http::field::cache_control, "no-cache"}}));
    }
}

void RequestHandler::HandleGetPlayers(const std::string& token, const ResponseCallback& callback) const {
    try {
        api_handler_.GetPlayers(token, [this, callback](const std::string& data){
            callback(HandleResponse(http::status::ok, data,
                                    {{http::field::content_type, "application/json"},
                                     {http::field::cache_control, "no-cache"}}));
        });
        return;
    } catch (const ApiException& ex) {
        callback(HandleError(ex.status, ex.code, ex.what(),
                             {{http::field::content_type, "application/json"},
                              {http::field::cache_control, "no-cache"}}));
    }
}

void RequestHandler::HandleGetGameState(const std::string& token, const ResponseCallback& callback) const {
    try {
        api_handler_.GetGameState(token, [this, callback](const std::string& data){
            callback(HandleResponse(http::status::ok, data,
                                    {{http::field::content_type, "application/json"},
                                     {http::field::cache_control, "no-cache"}}));
        });
        return;
    } catch (const ApiException& ex) {
        callback(HandleError(ex.status, ex.code, ex.what(),
                             {{http::field::content_type, "application/json"},
                              {http::field::cache_control, "no-cache"}}));

    }
}

void RequestHandler::HandlePlayerAction(const std::string& token, const std::string& body, const ResponseCallback& callback) const {
    try {
        api_handler_.PlayerAction(token, body, [this, callback](const std::string& data){
            callback(HandleResponse(http::status::ok, data,
                                    {{http::field::content_type, "application/json"},
                                     {http::field::cache_control, "no-cache"}}));
        });
        return;
    } catch (const ApiException& ex) {
        callback(HandleError(ex.status, ex.code, ex.what(),
                             {{http::field::content_type, "application/json"},
                              {http::field::cache_control, "no-cache"}}));
    } catch (...) {
        callback(HandleError(http::status::bad_request, "invalidArgument",
                             "Failed to parse action",
                             {{http::field::content_type, "application/json"},
                              {http::field::cache_control, "no-cache"}}));
    }
}

void RequestHandler::HandleGameTick(const std::string& body, const ResponseCallback& callback) const {
    try {
        api_handler_.GameTick(body, [this, callback](const std::string& data){
            callback(HandleResponse(http::status::ok, data,
                                    {{http::field::content_type, "application/json"},
                                     {http::field::cache_control, "no-cache"}}));
        });
        return;
    } catch (const ApiException& ex) {
        callback(HandleError(ex.status, ex.code, ex.what(),
                             {{http::field::content_type, "application/json"},
                              {http::field::cache_control, "no-cache"}}));
    } catch (...) {
        callback(HandleError(http::status::bad_request, "invalidArgument",
                             "Failed to parse tick request JSON",
                             {{http::field::content_type, "application/json"},
                              {http::field::cache_control, "no-cache"}}));
    }
}

}  // namespace http_handler

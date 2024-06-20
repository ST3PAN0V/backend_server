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

void MakeErrorString(http::status errCode, std::string description, const StringRequest& req, StringResponse& response, std::string_view contentType) {
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
    responseErr.set(http::field::content_type, contentType);
    err["code"] = getStrFromCode(errCode);
    err["message"] = description;
    responseErr.body() = boost::json::serialize(err);
    
    response = responseErr;
}

void HandleAPIResponse(StringResponse& response, const StringRequest& req, const model::Game& game) {
    response.set(http::field::content_type, ContentType::APP_JSON);
    if (req.target() == "/api/v1/maps") {
        response.body() = std::move(json_loader::GetStringFromMaps(game.GetMaps()));
    } else if (req.target().substr(0, 13) == "/api/v1/maps/") {
        std::string needed_map = std::string(req.target().substr(13, req.target().size()));
        auto tmp = std::move(json_loader::FoundAndGetStringMap(game.GetMaps(), needed_map));
        if (tmp.has_value()) {
            response.body() = std::move(*tmp);
        } else {
            MakeErrorString(http::status::not_found, "Map not found", req, response, ContentType::APP_JSON);
        }
    } else {
        MakeErrorString(http::status::bad_request, "Bad request", req, response, ContentType::APP_JSON);
    }
}

bool IsSubPath(fs::path path, fs::path base) {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

std::string read_file(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (file) {
        std::ostringstream contents;
        contents << file.rdbuf();
        return contents.str();
    }
    assert(false); // не удалось открыть файл
}

void HandleGETResponse(StringResponse& response, const StringRequest& req, const model::Game& game, const std::string& staticFolderPath) {
    if (req.target().substr(0,5) == "/api/") HandleAPIResponse(response, req, game);
    else {
        HandleSTATICResponse(response, req, game, staticFolderPath);
    }
    response.content_length(response.body().size());
}

void HandleSTATICResponse(StringResponse& response, const StringRequest& req, const model::Game& game, const std::string& staticFolderPath) {
    std::string modStaticPath = "/" + staticFolderPath;
    std::string requestTarget = std::string(req.target().data(), req.target().size());

    if (IsSubPath(modStaticPath + requestTarget, modStaticPath)) {
        std::string file_path = "./.." + modStaticPath + requestTarget;
        if (requestTarget == "/") {
            file_path = "./.." + modStaticPath + "/index.html";
        }
        std::ifstream file_stream(file_path);
        
        if (file_stream) {
            std::string file_contents((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
            response.body() = file_contents;
        } else {
            MakeErrorString(http::status::not_found, "Not found", req, response, ContentType::TEXT_PLAIN);
        }
        response.prepare_payload();

    } else {
        MakeErrorString(http::status::bad_request, "Bad request", req, response, ContentType::APP_JSON);
    }
}

StringResponse MakeStringResponse(http::status status, RequestType req_type, StringRequest&& req, const model::Game& game, const std::string& staticFolderPath) {
    StringResponse response(status, req.version());
    response.keep_alive(req.keep_alive());

    switch (req_type) {
    case RequestType::GET:
        HandleGETResponse(response, req, game, staticFolderPath);
        break;
    default:
        assert(false);
    }
    return response;
}

StringResponse HandleRequestSendResponse(StringRequest&& req, const model::Game& game, const std::string& staticFolderPath) {
    if (req.method_string() == "GET"s) {
        return MakeStringResponse(http::status::ok, RequestType::GET, move(req), game, staticFolderPath);
    } else {
        return MakeStringResponse(http::status::method_not_allowed, RequestType::OTHER, move(req), game, staticFolderPath);
    }
}

}  // namespace http_server

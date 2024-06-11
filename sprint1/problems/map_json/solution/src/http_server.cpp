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

std::string GetStringFromMaps(const std::vector<model::Map>& maps) {
    std::string returnObject;

    returnObject += "[";
    for (const auto& map : maps) {
        returnObject += "{\"id\": \"" + map.GetId().operator*() + "\", \"name\": \"" + map.GetName() + "\"}, ";
    }
    returnObject = returnObject.substr(0, returnObject.size()-2) + "]";
    return returnObject;
}

std::optional<std::string> FoundAndGetStringMap(const std::vector<model::Map>& maps, const std::string& needIdMap) {
    for (const auto& map : maps) {
        if (map.GetId().operator*() == needIdMap) {
            boost::json::object response;

            response["id"] = map.GetId().operator*();
            response["name"] =  map.GetName();

            // Дороги
            boost::json::array roads;
            for (const auto& road : map.GetRoads()) {
                boost::json::object road_node;
                road_node["x0"] = road.GetStart().x;
                road_node["y0"] = road.GetStart().y;
                if (road.IsHorizontal()) {
                    road_node["x1"] = road.GetEnd().x;
                } else {
                    road_node["y1"] = road.GetEnd().y;
                }
                roads.push_back(road_node);
            }
            response["roads"] = roads;

            // Здания
            boost::json::array buildings;
            for (const auto& building : map.GetBuildings()) {
                boost::json::object building_node;
                building_node["x"] = building.GetBounds().position.x;
                building_node["y"] = building.GetBounds().position.y;
                building_node["w"] = building.GetBounds().size.width;
                building_node["h"] = building.GetBounds().size.height;
                buildings.push_back(building_node);
            }
            response["buildings"] = buildings;

            // оффисы
            boost::json::array offices;
            for (const auto& office : map.GetOffices()) {
                boost::json::object office_node;
                office_node["id"] = office.GetId().operator*();
                office_node["x"] = office.GetPosition().x;
                office_node["y"] = office.GetPosition().y;
                office_node["offsetX"] = office.GetOffset().dx;
                office_node["offsetY"] = office.GetOffset().dy;
                offices.push_back(office_node);
            }
            response["offices"] = offices;
            
            return boost::json::serialize(response);
        }
    }
    return std::nullopt;
}

StringResponse MakeErrorString(std::string errCode, const StringRequest& req) {
    boost::json::object err;

    auto makeError = [&req, &errCode, &err](http::status codeStatus, std::string description){
        StringResponse response(codeStatus, req.version());
        response.set(http::field::content_type, ContentType::APP_JSON);
        err["code"] = errCode;
        err["message"] = description;
        response.body() = boost::json::serialize(err);
        response.content_length(response.body().size());
        return response;
    };

    if (errCode == MAPNOTFOUND) {
        return makeError(http::status::not_found, "Map not found");
    } else if (errCode == BADREQUEST) {
        return makeError(http::status::bad_request, "Bad request");
    }
    
}

void ModifyResponseGET(StringResponse& response, const StringRequest& req, const model::Game& game) {
    response.set(http::field::content_type, ContentType::APP_JSON);
    
    if (req.target() == "/api/v1/maps") {
        response.body() = std::move(GetStringFromMaps(game.GetMaps()));
    } else if (req.target().substr(0, 13) == "/api/v1/maps/") {
        std::string needed_map = std::string(req.target().substr(13, req.target().size()));
        auto tmp = std::move(FoundAndGetStringMap(game.GetMaps(), needed_map));
        if (tmp.has_value()) {
            response.body() = std::move(*tmp);

        } else {
            throw std::invalid_argument(MAPNOTFOUND);
        }
    } else if (req.target().substr(0,5) == "/api/") {
        throw std::invalid_argument(BADREQUEST);
    }
    response.content_length(response.body().size());
}

void ModifyResponseHEAD(StringResponse& response, const StringRequest& req, const model::Game& game) {
    //const std::string_view body = std::string("Hello!");
    //response.body() = "";
    //response.content_length(body.size());
}

StringResponse MakeStringResponse(http::status status, RequestType req_type, StringRequest&& req, const model::Game& game) {
    StringResponse response(status, req.version());
    response.keep_alive(req.keep_alive());

    switch (req_type) {
    case RequestType::GET:
        ModifyResponseGET(response, req, game);
        break;
    case RequestType::HEAD:
        ModifyResponseHEAD(response, req, game);
        break;
    case RequestType::OTHER:
        // response.body() = "Invalid method"sv;
        // response.set(http::field::allow, "GET, HEAD");
        // response.content_length(static_cast<size_t>(14));
        break;
    
    default:
        break;
    }
    return response;
}

StringResponse HandleRequest(StringRequest&& req, const model::Game& game) {
    try {
        if (req.method_string() == "GET"s) {
            return MakeStringResponse(http::status::ok, RequestType::GET, move(req), game);
        } else if (req.method_string() == "HEAD"s) {
            return MakeStringResponse(http::status::ok, RequestType::HEAD, move(req), game);
        } else {
            return MakeStringResponse(http::status::method_not_allowed, RequestType::OTHER, move(req), game);
        }
    } catch (const std::exception& err) {
        if (err.what() == MAPNOTFOUND) {
            return MakeErrorString(MAPNOTFOUND, req);
        } else if (err.what() == BADREQUEST) {
            return MakeErrorString(BADREQUEST, req);
        } else {
            
        }
    }
}

}  // namespace http_server

#include "api_handler.h"
#include "constants.h"

#include <boost/json.hpp>

namespace json = boost::json;

void ApiHandler::GetMaps(const Callback& callback) const {
    auto maps = game_.GetMaps();
    json::array json_maps;

    for (const auto& map : maps) {
        json::object obj;
        obj[constants::ID] = *(map.GetId());
        obj[constants::NAME] = map.GetName();
        json_maps.push_back(obj);
    }

    callback(json::serialize(json_maps));
}

json::array GetRoadsArray(const model::Map& map) {
    json::array json_roads;
    for (const auto& road : map.GetRoads()) {
        json::object road_obj;
        const auto& start = road.GetStart();
        road_obj[constants::X0] = start.x;
        road_obj[constants::Y0] = start.y;
        if (road.IsHorizontal()) {
            road_obj[constants::X1] = road.GetEnd().x;
        }
        if (road.IsVertical()) {
            road_obj[constants::Y1] = road.GetEnd().y;
        }
        json_roads.push_back(road_obj);
    }
    return json_roads;
}

json::array GetBuildingsArray(const model::Map& map) {
    json::array json_buildings;
    for (const auto& building : map.GetBuildings()) {
        json::object building_obj;
        const auto& bounds = building.GetBounds();
        building_obj[constants::X] = bounds.position.x;
        building_obj[constants::Y] = bounds.position.y;
        building_obj[constants::W] = bounds.size.width;
        building_obj[constants::H] = bounds.size.height;
        json_buildings.push_back(building_obj);
    }
    return json_buildings;
}

json::array GetOfficesArray(const model::Map& map) {
    json::array json_offices;
    for (const auto& office : map.GetOffices()) {
        json::object office_obj;
        const auto& position = office.GetPosition();
        const auto& offset = office.GetOffset();
        office_obj[constants::ID] = *(office.GetId());
        office_obj[constants::X] = position.x;
        office_obj[constants::Y] = position.y;
        office_obj[constants::OFFSET_X] = offset.dx;
        office_obj[constants::OFFSET_Y] = offset.dy;
        json_offices.push_back(office_obj);
    }
    return json_offices;
}

json::array GetLootTypes(const std::string& loot_types) {
    return json::parse(loot_types).as_array();
}

void ApiHandler::GetMapById(const std::string& id, const Callback& callback) const {
    auto map_opt = game_.FindMap(model::Map::Id{id});
    if (!map_opt) {
        throw ApiException("Map not found", "mapNotFound", http::status::not_found);
    }

    const auto& map = *map_opt;
    json::object obj;
    obj[constants::ID] = *(map.GetId());
    obj[constants::NAME] = map.GetName();

    obj[constants::ROADS] = GetRoadsArray(map);
    obj[constants::BUILDINGS] = GetBuildingsArray(map);
    obj[constants::OFFICES] = GetOfficesArray(map);
    obj[constants::LOOT_TYPES] = GetLootTypes(loot_data_.GetLootTypesOnMap(*(map.GetId())));

    callback(json::serialize(obj));
}

void ApiHandler::JoinGame(const std::string& body, const Callback& callback) {
    auto obj = json::parse(body).as_object();

    if (!obj.contains("userName") || !obj["userName"].is_string()) {
        throw ApiException("Invalid name", "invalidArgument", http::status::bad_request);
    }

    std::string userName = obj["userName"].as_string().c_str();

    if (userName.empty()) {
        throw ApiException("Invalid name", "invalidArgument", http::status::bad_request);
    }

    if (!obj.contains("mapId") || !obj["mapId"].is_string()) {
        throw ApiException("Invalid mapId", "invalidArgument", http::status::bad_request);
    }

    std::string mapId = obj["mapId"].as_string().c_str();
    auto map_opt = game_.FindMap(model::Map::Id{mapId});
    if (!map_opt) {
        throw ApiException("Map not found", "mapNotFound", http::status::not_found);
    }

    net::post(api_strand_, [this, userName, map_opt, callback]() {
        // Генерируем playerId и authToken
        const auto& [token, id] = app_.AddPlayer(userName, map_opt);

        json::object result;
        result["authToken"] = *token;
        result["playerId"] = id;
        callback(json::serialize(result));
    });
}

void ApiHandler::GetPlayers(const std::string& token_str, const Callback& callback) const {
    Token token(token_str);
    const auto* player = app_.GetPlayer(token);
    if (!player) {
        throw ApiException("Player token has not been found", "unknownToken", http::status::unauthorized);
    }

    net::post(api_strand_, [this, player, callback]() {
        json::object result;
        for(const auto* player_on_map : app_.GetPlayersOnMap(player->GetMap())) {
            result[std::to_string(player_on_map->GetId())] = { {"name", player_on_map->GetName()} };
        }
        callback(json::serialize(result));
    });
}

void ApiHandler::GetGameState(const std::string& token_str, const Callback& callback) const {
    Token token(token_str);
    const auto* player = app_.GetPlayer(token);
    if (!player) {
        throw ApiException("Player token has not been found", "unknownToken", http::status::unauthorized);
    }

    net::post(api_strand_, [this, player, callback]() {
        json::object result;
        for(auto* player_on_map : app_.GetPlayersOnMap(player->GetMap())) {
            auto* dog = player_on_map->GetDog();
            
            json::array pos = {json::value_from(dog->GetPosition().x), json::value_from(dog->GetPosition().y)};
            json::array speed = {json::value_from(dog->GetSpeed().x), json::value_from(dog->GetSpeed().y)};
            char dir = dog->GetDirection();

            json::array bag;
            for (const auto& loot : dog->GetBag()) {
                json::object loot_obj;
                loot_obj["id"] = loot.id;
                loot_obj["type"] = loot.type;
                bag.push_back(loot_obj);
            }
            
            json::object player_data;
            player_data["pos"] = pos;
            player_data["speed"] = speed;
            player_data["dir"] = std::string{dir};
            player_data["bag"] = bag;
            
            result[std::to_string(player_on_map->GetId())] = player_data;
        }

        json::object result_lost_objects;
        for(const auto& lost_object : player->GetMap()->GetLostObjects()) {
            json::array pos = {json::value_from(lost_object.position.x), json::value_from(lost_object.position.y)};

            json::object lost_object_data;
            lost_object_data["type"] = lost_object.type;
            lost_object_data["pos"] = pos;
            
            result_lost_objects[std::to_string(lost_object.id)] = lost_object_data;
        }

        json::object response;
        response["players"] = result;
        response["lostObjects"] = result_lost_objects;
        callback(json::serialize(response));
    });
}

void ApiHandler::PlayerAction(const std::string& token_str, const std::string& body, const Callback& callback) const {
    Token token(token_str);
    auto* player = app_.GetPlayer(token);
    if (!player) {
        throw ApiException("Player token has not been found", "unknownToken", http::status::unauthorized);
    }

    auto obj = json::parse(body).as_object();

    // Проверяем наличие и тип поля userName
    if (!obj.contains("move") || !obj["move"].is_string()) {
        throw ApiException("Failed to parse action", "invalidArgument", http::status::bad_request);
    }

    std::string move = obj["move"].as_string().c_str();
    char direction = move.empty() ? 0 : move.at(0);
    
    if (direction != 'L' && direction != 'R' && direction != 'D' && direction != 'U' && direction != 0 ) {
        throw ApiException("Failed to parse action", "invalidArgument", http::status::bad_request);
    }

    net::post(api_strand_, [this, player, direction, callback]() {
        const auto speed = player->GetMap()->GetDogSpeed();
        player->GetDog()->SetDirection(direction);
        player->GetDog()->SetSpeed(speed);

        json::object result;
        callback(json::serialize(result));
    });
}

void ApiHandler::GameTick(const std::string& body, const Callback& callback) {
    if (tick_period_) {
        throw ApiException("Invalid endpoint", "badRequest", http::status::bad_request);
    }

    auto obj = json::parse(body).as_object();

    // Проверяем наличие и тип поля timeDelta
    if (!obj.contains("timeDelta") || !obj["timeDelta"].is_int64()) {
        throw ApiException("Failed to parse tick request JSON", "invalidArgument", http::status::bad_request);
    }

    const auto time_ms = obj["timeDelta"].as_int64();

    net::post(api_strand_, [this, time_ms, callback]() {
        TickAction(time_ms);

        json::object result;
        callback(json::serialize(result));
    });
}

void ApiHandler::TickAction(int64_t time_ms) {
    // Генерим новые объкты на карте
    for (auto& map : game_.GetMaps()) {
        const auto loot_count = map.GetLostObjects().size();
        const auto players_count  = app_.GetPlayersOnMap(&map).size();
        const auto num_objects = loot_generator_.Generate(loot::Generator::MakeTimeInterval(time_ms), loot_count, players_count);

        const auto loot_types_count = GetLootTypes(loot_data_.GetLootTypesOnMap(*map.GetId())).size();
        for (unsigned int i = 0; i < num_objects; ++i) {
            map.AddLostObject(loot_types_count);
        }
    }

    // Двигаем игроков
    app_.Move(time_ms);
}

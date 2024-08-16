#include <application.h>
#include <boost/url.hpp>

namespace application {

std::optional<std::string> check_token(const std::string& autorization_text) {
    std::string bearer = "Bearer ";
    std::string example_token = "77777777777777777777777777777777";

    if ((autorization_text.substr(0, bearer.size()) != bearer) || 
       (autorization_text.size() < (bearer.size() + example_token.size()))) {
        return std::nullopt;
    }

    auto token = autorization_text.substr(bearer.size(), autorization_text.size());
    return token;
}

std::string SerializeMessageCode(const std::string& code, const std::string& message) {
    boost::json::object response;
    response["code"] = code.data();
    response["message"] = message.data();
    return boost::json::serialize(response);
}

std::string Application::GetMapJson(const std::string& request_target, http::status& response_status) {
    boost::urls::url_view url_api(request_target);
    // get map name 
    std::string map_name;
    auto url_route_segments = url_api.encoded_segments();
    for (auto segment_iterator = url_route_segments.begin(); segment_iterator != url_route_segments.end(); ++segment_iterator) {
        if (*segment_iterator == "maps") {
            auto segment_next = ++segment_iterator;
            if (segment_next != url_route_segments.end()) map_name = *segment_next;
            break;
        }
    }

    gameplay::ModelJsonSerializer model_serializer(game_);
    if (map_name.empty()) {
        response_status = http::status::ok;
        return model_serializer.SerializeMaps();
    }
    else {
        auto map = model_serializer.SerializeMap(map_name);
        if (map)
        {
            response_status = http::status::ok;
            return *map;
        }
        else 
        {
            response_status = http::status::not_found;
            return SerializeMessageCode("mapNotFound", "Map not found");
        }
    }
}

std::string Application::Tick(const std::string& jsonBody, APPLICATION_ERROR& app_error) {

    const auto json_parsing_error = [&](APPLICATION_ERROR& app_error) {
        app_error = APPLICATION_ERROR::INVALID_ARGUMENT;
        return SerializeMessageCode("invalidArgument", "Failed to parse tick request JSON");
    };
        
    // check timeDelta
    // try parsing body to json
    boost::system::error_code ec;
    auto value = boost::json::parse(jsonBody, ec);
    if(ec) {
        return json_parsing_error(app_error);
    }

    // parsing timeDelta from json
    if (!value.as_object().if_contains("timeDelta")) {
        return json_parsing_error(app_error);
    }
    auto time_delta_json = value.at("timeDelta");

    // check timeDelta format
    uint64_t time_delta = 0;
    if (auto n = time_delta_json.if_int64(); n && *n > 0) {
        time_delta = *n;
    } 
    else if (auto n = time_delta_json.if_uint64()) {
        time_delta = *n;           
    }
    else {
        return json_parsing_error(app_error);
    }

    // check time_delta isn't zero    
    if (time_delta == 0) {
        return json_parsing_error(app_error);
    }

    // set timeDelta
    game_.Tick(time_delta);

    app_error = APPLICATION_ERROR::APPLICATION_NO_ERROR;
    return boost::json::serialize(boost::json::object{});
}

std::string Application::ActionPlayer(const std::string& auth_message, APPLICATION_ERROR& app_error, const std::string& jsonBody) {  

    const auto json_parsing_error = [&](APPLICATION_ERROR& app_error) {
        app_error = APPLICATION_ERROR::INVALID_ARGUMENT;
        return SerializeMessageCode("invalidArgument", "Failed to parse action");
    };

    // check valid json     
    boost::system::error_code ec;
    auto value = boost::json::parse(jsonBody, ec);
    if(ec) {
        return json_parsing_error(app_error);
    }

    // parsing move command from json
    if (!value.as_object().if_contains("move")) {
        return json_parsing_error(app_error);
    }
    std::string command = value.at("move").as_string().c_str();

    std::string app_error_msg;
    auto player = GetPlayerFromToken(auth_message, app_error, app_error_msg);

    if (player == nullptr) {
        return app_error_msg;
    }

    // move player (dog)
    player->Move(command);

    app_error = APPLICATION_ERROR::APPLICATION_NO_ERROR;
    return boost::json::serialize(boost::json::object{});     
}

gameplay::Player* Application::GetPlayerFromToken(const std::string& auth_message, APPLICATION_ERROR& app_error, std::string& app_error_msg) {
    // check token format
    auto auth_token = check_token(auth_message);
    if (!auth_token) {
        app_error = APPLICATION_ERROR::INVALID_TOKEN;
        app_error_msg = SerializeMessageCode("invalidToken", "Authorization header is missing");
        return nullptr;
    }

    // authorize token
    gameplay::Token token{ *auth_token };
    auto player = player_tokens_.FindPlayer(token);
    if (player == nullptr) {
        app_error = APPLICATION_ERROR::UNKNOWN_TOKEN;
        app_error_msg = SerializeMessageCode("unknownToken", "Player token has not been found");
    }
    return player;
}

std::string Application::GetState(const std::string& auth_message, APPLICATION_ERROR& app_error)
{
    auto get_json_array = [](const auto &x, const auto &y) {
        boost::json::array json_array;
        json_array.emplace_back(x);
        json_array.emplace_back(y);
        return json_array;
    };

    std::string app_error_msg;
    auto player = GetPlayerFromToken(auth_message, app_error, app_error_msg);

    if (player == nullptr) {
        return app_error_msg;
    }

    boost::json::object state;
    auto session = player->GetSession();
    auto dogs = session->GetDogs();
    for (auto& dog : dogs) {
        boost::json::object dog_param;
        dog_param["pos"] = get_json_array(dog.GetCoordinate().x, dog.GetCoordinate().y);
        dog_param["speed"] = get_json_array(dog.GetSpeed().x, dog.GetSpeed().y);
        dog_param["dir"] = dog.GetDirection();
        state[std::to_string(*dog.GetId())] = dog_param;
    }

    // serialize response
    boost::json::object players;
    players["players"] = state;
    app_error = APPLICATION_ERROR::APPLICATION_NO_ERROR;
    return boost::json::serialize(players);
}

std::string Application::Join(const std::string& jsonBody, APPLICATION_ERROR& join_error) {
    
    auto invalid_argument = SerializeMessageCode("invalidArgument", "Join game error");

    std::string resp;
    std::string userName;
    std::string mapId;

    boost::json::error_code ec;
    auto value = boost::json::parse(boost::json::string{jsonBody}, ec);
    if (ec) {
        join_error = APPLICATION_ERROR::BAD_JSON;
        return invalid_argument;
    }

    try {
        userName = value.at("userName").as_string();
        mapId = value.at("mapId").as_string();
    }
    catch (...) {
        join_error = APPLICATION_ERROR::BAD_JSON;
        return invalid_argument;
    }

    if (userName.empty()) {
        join_error = APPLICATION_ERROR::INVALID_NAME;
        return SerializeMessageCode("invalidArgument", "Invalid name");
    }

    auto idmap = model::Map::Id{mapId.data()};
    auto map = game_.FindMap({ idmap });

    if (map == nullptr) {
        join_error = APPLICATION_ERROR::MAP_NOT_FOUND;
        return SerializeMessageCode("mapNotFound", "Map not found");
    }

    auto player = GetPlayer(userName, mapId);
    auto token = *player_tokens_.AddPlayer(player);
    auto id = *player->GetId();

    boost::json::object response;
    response["authToken"] = token;
    response["playerId"]  = id;

    join_error = APPLICATION_ERROR::APPLICATION_NO_ERROR;
    return boost::json::serialize(response);
}

std::string Application::GetPlayers(const std::string& auth_message, APPLICATION_ERROR& auth_error) {
    std::string app_error_msg;
    auto player = GetPlayerFromToken(auth_message, auth_error, app_error_msg);

    if (player == nullptr) {
        return app_error_msg;
    }

    boost::json::object response;
    for (auto palyer_id = 0; (player = players_.FindPlayer(gameplay::Player::Id{palyer_id})) != nullptr; palyer_id++) {
        boost::json::object name;
        name["name"] = player->GetName();
        response[std::to_string(palyer_id)] = name;
    }

    auth_error = APPLICATION_ERROR::APPLICATION_NO_ERROR;
    return boost::json::serialize(response);
}

gameplay::Player* Application::GetPlayer(const std::string& name, const std::string& mapId)
{
    auto map_id = model::Map::Id{mapId};
    auto player_id = players_.FindPlayerId(name, map_id);
    if (player_id == nullptr) {
        auto session = game_.FindGameSession(map_id);
        if (session == nullptr) {
            session = game_.AddGameSession(map_id);
        }
        auto dog = session->AddDog(name);
        auto player = players_.Add(dog, session);
        return player;
    }
    else {
        return players_.FindPlayer(*player_id, map_id);
    }
    return nullptr;
}

};
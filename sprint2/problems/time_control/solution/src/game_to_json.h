#ifndef GAME_TO_JSON_HPP
#define GAME_TO_JSON_HPP

#include <boost/json.hpp>

#include "model.h"
#include "gamesession.h"

namespace GameToJson
{    
    namespace json = boost::json;
    
    
    json::object MapToJson(const model::Map* map);
    
    json::array MapsToJson(const model::Game::Maps& maps);
    
    json::object GameStateToJson(const model::GameSession::Dogs& dogs);
    
    json::object DogsToJson(const model::GameSession::Dogs& dogs);
}

#endif // GAME_TO_JSON_HPP

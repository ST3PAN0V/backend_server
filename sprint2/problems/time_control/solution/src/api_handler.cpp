#include <regex>
#include <variant>
#include <iostream>

#include <boost/json.hpp>

#include "api_handler.h"
#include "players.h"
#include "game_to_json.h"

namespace json = boost::json;

namespace http_handler 
{
    
    enum class JoinRequestError{Parsing, Name, Map};
    using JoinRequestParam = std::pair<std::string, std::string>;
    using JoinRequest = std::variant<JoinRequestParam, JoinRequestError>;    
    
    enum class PlayerCommand
    {
        None,
        Stop, 
        L, R, U, D 
    };
    
    JoinRequest LoadJoinRequest(const json::value& request)
    {
        //{"userName": "Scooby Doo", "mapId": "map1"} 
        auto const& req_param = request.get_object();
        if( req_param.empty())
        {
            //error
            return JoinRequestError::Parsing;
        }
        
        if(!req_param.if_contains("userName"))        
        {
            //error
            //{"code": "invalidArgument", "message": "Invalid name"} 
            return JoinRequestError::Name;
        }
        
        if(!req_param.if_contains("mapId"))        
        {
            //error
            //{"code": "mapNotFound", "message": "Map not found"} 
            return JoinRequestError::Name;
        }
        
        const std::string userName = req_param.at("userName").as_string().c_str();
        const std::string mapId = req_param.at("mapId").as_string().c_str();
        
        if(userName.empty())
        {
            //error
            //{"code": "invalidArgument", "message": "Invalid name"} 
            return JoinRequestError::Name;
        }
        
        if(mapId.empty())
        {
            //error
            //{"code": "mapNotFound", "message": "Map not found"} 
            return JoinRequestError::Map;
        }
        
        return JoinRequestParam{userName, mapId};
    }       
    
    PlayerCommand PlayerActionRequest(const json::value& request)
    {
        static const std::string Tag{"move"};
        
        auto const& req_param = request.get_object();
        if( req_param.empty())
        {
            //error
            return PlayerCommand::None;
        }
        
        if(!req_param.if_contains(Tag))        
        {
            //error
            return PlayerCommand::None;
        }
        
        const std::string action = req_param.at(Tag).as_string().c_str();
        
        if(action.size() > 1)
        {
            return PlayerCommand::None;
        }
        
        if(action == "L")
        {
            return PlayerCommand::L;
        }
        
        if(action == "R")
        {
            return PlayerCommand::R;
        }
        
        if(action == "U")
        {
            return PlayerCommand::U;
        }
        
        if(action == "D")
        {
            return PlayerCommand::D;
        }
        
        return PlayerCommand::Stop;
    }
    
    std::optional<int64_t> TickRequest(const json::value& request)
    {
        static const std::string Tag{"timeDelta"};
        
        auto const& req_param = request.get_object();
        if( req_param.empty())
        {
            //error
            return {};
        }
        
        if(!req_param.if_contains(Tag))        
        {
            //error
            return {};
        }
        
        //const json::object tag_value{req_param.at(Tag).get_object()};
        
        if(!req_param.at(Tag).is_int64())
        {
            return {};
        }
        
        const int64_t action = req_param.at(Tag).as_int64();       
        
        return action;
    }
    
    ApiHandler::ApiHandler(model::Game& game):
        m_game{game}
    {        
    }
    
    bool ApiHandler::hasApiRequest(const std::string& target) const
    {
        static const std::regex api_v1{R"(^/api/)"};
        
        std::smatch m;
        
        return std::regex_search(target, m, api_v1);
    }
    
    StringResponse ApiHandler::ApiProcessRequest(const StringRequest& req)
    {
        static const std::regex maps{R"(^/api/v1/maps$)"};
        static const std::regex maps_id{R"(^/(api/v1/maps)/([\w]+)$)"};        
        static const std::regex game_join{R"(^/api/v1/game/join$)"};    
        static const std::regex game_players{R"(^/api/v1/game/players$)"};    
        static const std::regex game_state{R"(^/api/v1/game/state$)"}; 
        static const std::regex game_action{R"(^/api/v1/game/player/action$)"}; 
        static const std::regex game_tick{R"(^/api/v1/game/tick$)"}; 
        
        bool is_head_get{(req.method() == http::verb::get) ||
                         (req.method() == http::verb::head)};
        
        std::string target{req.target()};
        
        std::smatch m;                 
        
        //join
        if (std::regex_search(target, m, game_join))
        {   
            //get map
            return JoinGame(req);            
        }           
        
        if(is_head_get)
        {
            //map id /api/v1/maps/map1
            if (std::regex_search(target, m, maps_id))
            {   
                //get map
                return GetMap(m[2], req.version(), req.keep_alive());            
            }        
                          
            //maps /api/v1/maps
            if (std::regex_search(target, m, maps))
            {    
                //list all maps                        
                
                const model::Game::Maps maps{ m_game.GetMaps()};
    
                return MakeStringNoCashResponse(http::status::ok,
                                                serialize( GameToJson::MapsToJson(maps) ),
                                                req.version(),
                                                req.keep_alive(),
                                                ContentType::APP_JSON);
            }                
        }
        
        //players /api/v1/game/players
        if (std::regex_search(target, m, game_players))
        {   
            //get players
            return GetPlayers(req);            
        }    
        
        if(std::regex_search(target, m, game_state))
        {
            return GetGameState(req);           
        }
        
        if(std::regex_search(target, m, game_action))
        {
            return GameAction(req);           
        }
        
        if(std::regex_search(target, m, game_tick))
        {
            return GameTick(req);           
        }
        
        //error   
        //uri invalid 400 bad_request        
        
        return MakeStringResponse(http::status::bad_request,
                                  ErrorToJson("badRequest", "Bad request"), 
                                  req.version(), 
                                  req.keep_alive(),
                                  ContentType::APP_JSON);
    }
    
    StringResponse ApiHandler::GetMap(const std::string& target, 
                                      unsigned http_version, 
                                      bool keep_alive) const
    {
        const model::Map* map{m_game.FindMap(model::Map::Id{target})};
        
        if(nullptr == map)
        {
            //error
            //  map_id is invalid 404 not_found
            
            return MakeStringResponse(http::status::not_found,
                                      ErrorToJson("mapNotFound", "Map not found"), 
                                      http_version, 
                                      keep_alive,
                                      ContentType::APP_JSON);
        }   
        
        return MakeStringNoCashResponse(http::status::ok,
                                        serialize( GameToJson::MapToJson(map) ),
                                        http_version, 
                                        keep_alive, 
                                        ContentType::APP_JSON);
    }
    
    StringResponse ApiHandler::JoinGame(const StringRequest& req) 
    {
        if(req.method() != http::verb::post)
        {
            StringResponse response{MakeStringNoCashResponse(http::status::method_not_allowed,
                                                             ErrorToJson("invalidMethod", 
                                                                         "Only POST method is expected"),
                                                             req.version(), 
                                                             req.keep_alive())};
            response.set(http::field::allow, "POST");
            return response;
        }
        
        std::string body{req.body()};
        json::error_code ec;
        json::value request = json::parse(body, ec);
        
        if(ec)
        {
            //{"code": "invalidArgument", "message": "Join game request parse error"} 
            
            return MakeStringNoCashResponse(http::status::bad_request,
                                            ErrorToJson("invalidArgument", 
                                                        "Join game request parse error"), 
                                            req.version(), 
                                            req.keep_alive());
        }

        JoinRequest req_param{LoadJoinRequest(request)};
        
        if(std::holds_alternative<JoinRequestError>(req_param))
        {
            JoinRequestError errorCode{std::get<JoinRequestError>(req_param)};
            
            switch (errorCode) 
            {
                case JoinRequestError::Parsing:
                {
                    return MakeStringNoCashResponse(http::status::bad_request,
                                                    ErrorToJson("invalidArgument", 
                                                                "Join game request parse error"), 
                                                    req.version(), 
                                                    req.keep_alive());
                }
                break;
                    
                case JoinRequestError::Name:
                {                    
                    return MakeStringNoCashResponse(http::status::bad_request,
                                                    ErrorToJson("invalidArgument", 
                                                                "Invalid name"), 
                                                    req.version(), 
                                                    req.keep_alive());
                }
                break;
                    
                case JoinRequestError::Map:
                {
                    return MakeStringNoCashResponse(http::status::not_found,
                                                    ErrorToJson("mapNotFound", 
                                                                "Map not found"), 
                                                    req.version(), 
                                                    req.keep_alive());
                }
                break;
            }
            
            //error
        }
        
        JoinRequestParam param{std::get<JoinRequestParam>(req_param)};
        //создать в сессии пользователя и собаку для карты
        
        //подобрать сессию для этой карты или создать новую
        model::GameSession* session{m_game.FindSession(model::Map::Id{param.second})};
        
        if(nullptr == session)
        {
            return MakeStringNoCashResponse(http::status::not_found,
                                            ErrorToJson("mapNotFound", 
                                                        "Map not found"), 
                                            req.version(), 
                                            req.keep_alive());
        }
        
        model::Dog* player_dog{session->CreateDog(param.first)};
        
        if(nullptr == player_dog)
        {
            return MakeStringNoCashResponse(http::status::not_found,
                                            ErrorToJson("mapNotFound", 
                                                        "dog not created"), 
                                            req.version(), 
                                            req.keep_alive());
        }
        
        app::Player* player{m_players.Create(player_dog, session)};
        
        app::Token playerToken{m_playerTokens.AddPlayer(player)}; 
        
        return MakeStringNoCashResponse(http::status::ok,
                                        authTokenToJson(playerToken, player->id()), 
                                        req.version(), 
                                        req.keep_alive());
    }
    std::string ApiHandler::authTokenToJson(const app::Token& token, 
                                            const size_t playerId) const
    {
        json::object obj; 
        
        obj["authToken"] = *token;
        obj["playerId"] = playerId;
        
        return serialize( obj );
    }
    
    std::optional<app::Token> ApiHandler::TryExtractToken(const StringRequest& req)
    {
        std::string_view auth{req[http::field::authorization]};        
        
        const size_t tokenPos{auth.find(" ")};
        
        if(std::string::npos == tokenPos)
        {
            return {};            
        }       
        
        const std::string token{std::string(auth.substr(1 + tokenPos))};   
        
        if(token.size() != 32)
        {
            return {};
        }
        

        app::Token t{token};
        return t;
    }
    
    StringResponse ApiHandler::MakeUnauthorizedError(unsigned http_version, 
                                                     bool keep_alive) 
    {
        return MakeStringNoCashResponse(http::status::unauthorized,
                                        ErrorToJson("invalidToken", 
                                                    "Authorization header is missing"), 
                                        http_version, 
                                        keep_alive);
    }
    
    StringResponse ApiHandler::GetPlayers(const StringRequest& request)
    {
        unsigned http_version{request.version()};
        bool keep_alive{request.keep_alive()};
        
        if((request.method() != http::verb::get) &&
           (request.method() != http::verb::head))
        {
            StringResponse response{MakeStringNoCashResponse(http::status::method_not_allowed,
                                                             ErrorToJson("invalidMethod", 
                                                                         "Invalid method"), 
                                                             http_version, 
                                                             keep_alive)};
            
            response.set(http::field::allow, "GET, HEAD");
            
            return response;
        }       
                
        return ExecuteAuthorized(request, 
                                 [http_version, keep_alive, this] (const app::Token& token)
        {
            app::Player* currentPlayer{m_playerTokens.FindPlayerByToken(token)};
            
            if(nullptr == currentPlayer)
            {
                return MakeStringNoCashResponse(http::status::unauthorized,
                                                ErrorToJson("unknownToken", 
                                                            "Player token has not been found"), 
                                                http_version, 
                                                keep_alive);
            }
            
            model::GameSession* session{currentPlayer->session()};
            
            if(nullptr == session)
            {
                return MakeStringNoCashResponse(http::status::unauthorized,
                                                ErrorToJson("unknownToken", 
                                                            "Player token has not been found"), 
                                                http_version, 
                                                keep_alive);
            }
                    
            return MakeStringNoCashResponse(http::status::ok,
                                            serialize( GameToJson::DogsToJson(session->GetDogs())), 
                                            http_version, 
                                            keep_alive);
        });
    }
    
    StringResponse ApiHandler::GetGameState(const StringRequest& request)
    {
        unsigned http_version{request.version()};
        bool keep_alive{request.keep_alive()};
        
        if((request.method() != http::verb::get) &&
           (request.method() != http::verb::head))
        {
            StringResponse response{MakeStringNoCashResponse(http::status::method_not_allowed,
                                                             ErrorToJson("invalidMethod", 
                                                                         "Invalid method"), 
                                                             http_version, 
                                                             keep_alive)};
            
            response.set(http::field::allow, "GET, HEAD");
            
            return response;
        }       
        
        return ExecuteAuthorized(request, 
                                 [http_version, keep_alive, this] (const app::Token& token)
        {
            app::Player* currentPlayer{m_playerTokens.FindPlayerByToken(token)};
            
            if(nullptr == currentPlayer)
            {
                return MakeStringNoCashResponse(http::status::unauthorized,
                                                ErrorToJson("unknownToken", 
                                                            "Player token has not been found"), 
                                                http_version, 
                                                keep_alive);
            }
            
            model::GameSession* session{currentPlayer->session()};
            
            if(nullptr == session)
            {
                return MakeStringNoCashResponse(http::status::unauthorized,
                                                ErrorToJson("unknownToken", 
                                                            "Player token has not been found"), 
                                                http_version, 
                                                keep_alive);
            }
                    
            return MakeStringNoCashResponse(http::status::ok,
                                            serialize( GameToJson::GameStateToJson(session->GetDogs())), 
                                            http_version, 
                                            keep_alive);
        });
    }
    
    StringResponse ApiHandler::GameAction(const StringRequest& request)
    {
        unsigned http_version{request.version()};
        bool keep_alive{request.keep_alive()};
        
        if(request.method() != http::verb::post )
        {
            StringResponse response{MakeStringNoCashResponse(http::status::method_not_allowed,
                                                             ErrorToJson("invalidMethod", 
                                                                         "Invalid method"), 
                                                             http_version, 
                                                             keep_alive)};
            
            response.set(http::field::allow, "POST");
            
            return response;
        }      
        const std::string body{request.body()};
        return ExecuteAuthorized(request, 
                                 [http_version, keep_alive, this, &body]
                                 (const app::Token& token)
        {
            json::error_code ec;
            json::value request = json::parse(body, ec);
            
            if(ec)
            {
                //{"code": "invalidArgument", "message": "Join game request parse error"} 
                
                return MakeStringNoCashResponse(http::status::bad_request,
                                                ErrorToJson("invalidArgument", 
                                                            "Join game request parse error"), 
                                                http_version, 
                                                keep_alive);
            }
            
            
            app::Player* currentPlayer{m_playerTokens.FindPlayerByToken(token)};
            
            if(nullptr == currentPlayer)
            {
                return MakeStringNoCashResponse(http::status::unauthorized,
                                                ErrorToJson("unknownToken", 
                                                            "Player token has not been found"), 
                                                http_version, 
                                                keep_alive);
            }
            
            model::Dog* dog{currentPlayer->dog()};
            
            if(nullptr == dog)
            {
                return MakeStringNoCashResponse(http::status::unauthorized,
                                                ErrorToJson("unknownToken", 
                                                            "Player token has not been found"), 
                                                http_version, 
                                                keep_alive);
            }
               
            
//            std::cout<<__PRETTY_FUNCTION__<<" before action: "<<serialize(
//                           GameToJson::GameStateToJson(currentPlayer->session()->GetDogs()))<<std::endl;
//            std::cout<<__PRETTY_FUNCTION__<<" request: "<<body<<std::endl;
            //std::cout<<__PRETTY_FUNCTION__<<" dog: "<<dog->name()<<std::endl;
            PlayerCommand cmd{PlayerActionRequest(request)};
            
            const double speed{currentPlayer->session()->GetMap()->dogSpeed()};
            
            switch (cmd)
            {  
                case PlayerCommand::L:
                { 
                    dog->setSpeed(-speed, 0.0);
                    dog->setDir(model::Direction::WEST);
                }
                break;
                    
                case PlayerCommand::R:
                {
                    dog->setSpeed(speed, 0.0);
                    dog->setDir(model::Direction::EAST);
                }
                break;
                    
                case PlayerCommand::U:
                {
                    dog->setSpeed(0.0, -speed);
                    dog->setDir(model::Direction::NORTH);
                }
                break;
                    
                case PlayerCommand::D:
                {
                    dog->setSpeed(0.0, speed);
                    dog->setDir(model::Direction::SOUTH);
                }
                break;
                    
                case PlayerCommand::Stop:
                {
                    dog->setSpeed(0.0, 0.0);
                }
                break;
                    
                default:
                {
                    //error
                    return MakeStringNoCashResponse(http::status::bad_request,
                                                    ErrorToJson("invalidArgument", 
                                                                "Failed to parse action"), 
                                                    http_version, 
                                                    keep_alive);
                }
                break;
            }            

            
            return MakeStringNoCashResponse(http::status::ok,
                                            "{}", 
                                            http_version, 
                                            keep_alive);
        });
    }
    
    StringResponse ApiHandler::GameTick(const StringRequest& request)
    {
        unsigned http_version{request.version()};
        bool keep_alive{request.keep_alive()};
        
        if(request.method() != http::verb::post )
        {
            StringResponse response{MakeStringNoCashResponse(http::status::method_not_allowed,
                                                             ErrorToJson("invalidMethod", 
                                                                         "Invalid method"), 
                                                             http_version, 
                                                             keep_alive)};
            
            response.set(http::field::allow, "POST");
            
            return response;
        }      
        const std::string body{request.body()};
        /*return ExecuteAuthorized(request, 
                                 [http_version, keep_alive, this, &body]
                                 (const app::Token& token)*/
        //{
        
        //std::cout<<__PRETTY_FUNCTION__<<" body: "<<body<<std::endl;
        json::error_code ec;
        json::value j_request = json::parse(body, ec);
        
        if(ec)
        {
            //{"code": "invalidArgument", "message": "Join game request parse error"} 
            std::cout<<__PRETTY_FUNCTION__<<" json parsing error"<<std::endl;
            return MakeStringNoCashResponse(http::status::bad_request,
                                            ErrorToJson("invalidArgument", 
                                                        "Join game request parse error"), 
                                            http_version, 
                                            keep_alive);
        }
        
        
        /*app::Player* currentPlayer{m_playerTokens.FindPlayerByToken(token)};
        
        if(nullptr == currentPlayer)
        {
            return MakeStringNoCashResponse(http::status::unauthorized,
                                            ErrorToJson("unknownToken", 
                                                        "Player token has not been found"), 
                                            http_version, 
                                            keep_alive);
        }*/
        
        
        
        
        std::optional<int64_t> delta{TickRequest(j_request)};
        
        if(delta)
        {
            //update GameSession
            //currentPlayer->session()->updateState(*delta);
            m_game.allTick(*delta);
            return MakeStringNoCashResponse(http::status::ok,
                                            "{}", 
                                            http_version, 
                                            keep_alive);
        }
        
        return MakeStringNoCashResponse(http::status::bad_request,
                                        ErrorToJson("invalidArgument", 
                                                    "Join game request parse error"), 
                                        http_version, 
                                        keep_alive);
            
        //});
    }
    
}

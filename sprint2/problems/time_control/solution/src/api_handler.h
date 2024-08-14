#pragma once

#include <optional>

#include <boost/beast/http.hpp>

#include "model.h"
#include "players.h"
#include "player_tokens.h"

#include "common_request_handler.h"


namespace http_handler 
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    
    class ApiHandler
    {
        public:
            ApiHandler(model::Game& game);                       
            
            bool hasApiRequest(const std::string& target) const;
            
            StringResponse ApiProcessRequest(const StringRequest& req);
           
            
        protected:
            model::Game& m_game;
            
            app::Players m_players;
            app::PlayerTokens m_playerTokens;
            
        protected:            
            StringResponse GetMap(const std::string& target,
                                  unsigned http_version,
                                  bool keep_alive) const;
            StringResponse JoinGame(const StringRequest& req);
                        
            std::string authTokenToJson(const app::Token& token, 
                                        const size_t playerId) const;           
            
            template <typename Fn>
            StringResponse ExecuteAuthorized(const StringRequest& request, Fn&& action) 
            {
                if (auto token = TryExtractToken(request)) 
                {
                    return action(*token);
                } 
                else 
                {
                    return MakeUnauthorizedError(request.version(), request.keep_alive());
                }
            }
            std::optional<app::Token> TryExtractToken(const StringRequest& req);
            StringResponse MakeUnauthorizedError(unsigned http_version, 
                                                 bool keep_alive);
            StringResponse GetPlayers(const StringRequest& request);
            StringResponse GetGameState(const StringRequest& request);
            StringResponse GameAction(const StringRequest& request);
            StringResponse GameTick(const StringRequest& request);
    };
}



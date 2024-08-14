#pragma once

#include <unordered_map>

#include "player.h"

namespace app 
{
    class Players
    {
        public:
            Players();
            
            Player* Create(model::Dog* d, model::GameSession* s);
            
        protected:
            std::unordered_map<size_t, Player> m_players;
            std::unordered_map<model::Dog*, size_t> m_dogToPlayerIndex;
            std::unordered_map<model::GameSession*, size_t> m_gameSessionToPlayerIndex;
    };

}

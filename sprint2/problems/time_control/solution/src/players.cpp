#include "players.h"

namespace app 
{
    Players::Players()
    {
        
    }
    
    Player* Players::Create(model::Dog* d, model::GameSession* s)
    {
        const size_t new_player_id{m_players.size()};
        
        //m_players.insert(new_player_id, Player{s, d, new_player_id});
        m_players[new_player_id].setParam(s, d, new_player_id);
        
        Player* player{&m_players[new_player_id]};
        
        return player;
    }
}


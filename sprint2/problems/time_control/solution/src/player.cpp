#include "player.h"


namespace app 
{
    Player::Player()
    {        
    }
    
    Player::Player(model::GameSession* session,
                   model::Dog* dog, 
                   size_t id):
        session_{session}
      , dog_{dog}
      , id_{id}
    {        
    }
    
    size_t Player::id() const
    {
        return id_;
    }
    
    void Player::setParam(model::GameSession* session, model::Dog* dog, size_t id)
    {
        session_ = session;
        dog_ = dog;
        id_ = id;
    }
    
    model::GameSession* Player::session()
    {
        return session_;
    }
    
    model::Dog* app::Player::dog()
    {
        return dog_;
    }
    
    
}

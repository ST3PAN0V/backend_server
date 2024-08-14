#pragma once

#include "dog.h"
#include "gamesession.h"

namespace app 
{
    class Player
    {
        public:
            Player();
            Player(model::GameSession* session, model::Dog* dog, size_t id);
            
            size_t id() const;
            void setParam(model::GameSession* session,
                          model::Dog* dog, 
                          size_t id);
            
            model::GameSession* session();
            model::Dog* dog();
            
        protected:
            model::Dog* dog_;
            model::GameSession* session_;
            size_t id_;
    };

}

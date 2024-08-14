#pragma once



#include <vector>
#include <optional>
#include <deque>

#include "dog.h"

namespace model 
{
    class Map;
    
    class GameSession
    {
        public: 
            using Dogs = std::deque<Dog>;
            
            GameSession() noexcept;           
            
            
            void AddMap(Map* map);
            Dog* CreateDog(const std::string& name);
            
            const Map* GetMap() const noexcept;
            
            const Dogs& GetDogs() const noexcept;
            
            void updateState(const int64_t timeDelta);
            
        protected:            
            Map* m_map;
            //dogs
            std::deque<Dog> dogs_;
            
        protected:
            std::optional<Position> calculateStop(const Position startPos, const Position stopPos) const; 
    };
}


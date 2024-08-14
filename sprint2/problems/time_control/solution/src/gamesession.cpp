#include <random>
#include <iostream>

#include "gamesession.h"

#include "model.h"

namespace model 
{
    
    std::ostream& operator<<(std::ostream& out, 
                             const Position& p)
    {
        out << "p("<<p.x << ", "<<p.y<<")"; 
        return out;    
    }
    
    std::ostream& operator<<(std::ostream& out, 
                             const Road& p)
    {
        out << "r("<<p.lbPos() << ", "<<p.rtPos()<<")"; 
        return out;    
    }
    
    double calculateDistance(const Position lhs, const Position rhs)
    {
        const double dx{rhs.x - lhs.x};
        const double dy{rhs.y - lhs.y};
        
        return std::sqrt(dx*dx + dy*dy);
    }
    
    GameSession::GameSession() noexcept:
        m_map{nullptr}
    {        
    }
    
    void model::GameSession::AddMap(Map* map)
    {
        m_map = map;
    }
    
    Dog* GameSession::CreateDog(const std::string& name)
    {
        const size_t new_dog_id{dogs_.size()};        
        dogs_.emplace_back(name);
        
        //настройка положения
        const Map::Roads& roads{m_map->GetRoads()};
        //выбираем дорогу
        const Road& road{roads.front()};
        dogs_[new_dog_id].setPosition(road.GetStart().x, road.GetStart().y);
        
        /*std::random_device rd;
        std::uniform_int_distribution<size_t> dist(0, roads.size());
        const size_t roadId{dist(rd)};
        const Road& road{roads.at(roadId)};//
        //ищем положение для собаки
        
        if(road.IsHorizontal())
        {
            std::uniform_int_distribution<int> h_dist(road.GetStart().x, road.GetEnd().x);
            
            const int pos{h_dist(rd)};
            
            dogs_[new_dog_id].setPosition(pos, road.GetStart().y);
        }
        else
        {
            std::uniform_int_distribution<int> v_dist(road.GetStart().y, road.GetEnd().y);
            
            const int pos{v_dist(rd)};
            
            dogs_[new_dog_id].setPosition(road.GetStart().x, pos);
        }*/
          
        return &dogs_[new_dog_id];
    }
    
    const Map* GameSession::GetMap() const noexcept
    {
        return m_map;
    }
    
    const GameSession::Dogs& GameSession::GetDogs() const noexcept
    {
        return dogs_;
    }
        
    void model::GameSession::updateState(const int64_t timeDelta)
    {
        const double dT{static_cast<double>(timeDelta)/1000.0};
        
        for(Dog& dog: dogs_)
        {
            //обновить состояние            
            const Position startPos{dog.position()};
            const Speed speed{dog.speed()};            
                        
            const Position stopPos{startPos.x + speed.v_x*dT, 
                                   startPos.y + speed.v_y*dT};
            
            std::optional<Position> correctStopPosition{calculateStop(startPos, stopPos)};
            
            if(correctStopPosition)
            {
                const Position pos{*correctStopPosition};
                dog.setPosition(pos);
                
                if(pos == stopPos)
                {
                    continue;
                }
            }
            
            dog.setSpeed(0.0, 0.0);//врезались            
        }
    }
    
    std::optional<Position> GameSession::calculateStop(const Position startPos, 
                                                       const Position stopPos) const
    {
        //найти дороги где может быть собака
        std::vector<const Road*> roads;
        
        for(const auto& it: m_map->GetRoads())
        {
            if(!it.contain(startPos))
            {                
                continue;
            }
            
            roads.push_back(&it);
        }
                
        //проверить остановку
        if(roads.empty())
        {
            //собака в гиперкосмосе
            //выход в обычное пространство            
            return {};
        }
        
        const size_t size{roads.size()};
               
        //вычисляем точку куда может добраться собака
        const Road* road{roads.front()};
        
        Position tmp{road->calculatePositionOnBorder(stopPos)};
        
        if(size == 1)
        {
            return tmp;
        }
        
        double maxDist{calculateDistance(startPos, tmp)};        
        for(size_t i{1}; i < size; ++i)
        {
            road = roads[i];
            Position currentPos{road->calculatePositionOnBorder(stopPos)};
            double currentDist{calculateDistance(startPos, currentPos)};            
            if(maxDist < currentDist)
            {
                tmp = currentPos;
                maxDist = currentDist;
            }
        }

        return tmp;
    }
    
    
    
}

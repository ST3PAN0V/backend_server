#include "model.h"

#include <stdexcept>

namespace model
{
    using namespace std::literals;
    
    void Map::AddOffice(Office office)
    {
        if (warehouse_id_to_index_.contains(office.GetId())) 
        {
            throw std::invalid_argument("Duplicate warehouse");
        }
    
        const size_t index = offices_.size();
        Office& o = offices_.emplace_back(std::move(office));
        
        try 
        {
            warehouse_id_to_index_.emplace(o.GetId(), index);
        } 
        catch (...) 
        {
            // Удаляем офис из вектора, если не удалось вставить в unordered_map
            offices_.pop_back();
            throw;
        }
    }
    
    void Map::setDogSpeed(const double speed)
    {
        dogSpeed_ = speed;
    }
    
    void Game::AddMap(Map map)
    {
        const size_t index = maps_.size();
        if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) 
        {
            throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
        }
        else 
        {
            try 
            {
                maps_.emplace_back(std::move(map));
            } 
            catch (...) 
            {
                map_id_to_index_.erase(it);
                throw;
            }
        }
    }
    
    const Game::Maps& Game::GetMaps() const noexcept 
    {
        return maps_;
    }
    
    const Map* Game::FindMap(const Map::Id& id) const noexcept 
    {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) 
        {
            return &maps_.at(it->second);
        }
        
        return nullptr;
    }
    
    GameSession* Game::FindSession(const Map::Id& id)
    {
        auto map_it = map_id_to_index_.find(id);
        
        if(map_id_to_index_.end() == map_it)
        {
            //нет такой карты
            return nullptr;
        }
        
        auto session_it = sessions_.find(map_it->second);
        
        if(sessions_.end() == session_it)
        {
            const size_t index{map_it->second};
            sessions_[map_it->second].AddMap(&maps_[index]);//создаем сессию для этой карты. 
            
            return &sessions_[index];
        }
        
        return &session_it->second;
    }
    
    double model::Map::dogSpeed() const
    {
        return dogSpeed_;
    }
    
    void model::Game::allTick(const int64_t delta)
    {
        for(auto& it: sessions_)
        {
            it.second.updateState(delta);
        }
    }
    
    bool Road::contain(const Position p) const
    {
        if(left_bottom_.x > p.x)
        {
            return false;
        }
        
        if(left_bottom_.y > p.y)
        {
            return false;
        }
        
        if(right_top_.x < p.x)
        {
            return false;
        }
        
        if(right_top_.y < p.y)
        {
            return false;
        }
        
        return true;
    }
    
    double calculateBorder(double l, double r, 
                           double value)
    {
        const double lower{std::min(l, r)};
        const double upper{std::max(l, r)};
        
        const double tmp{std::min(upper, value)};
        
        return std::max(lower, tmp);
    }
    
    Position Road::calculatePositionOnBorder(const Position p) const
    {        
        const double nx{calculateBorder(left_bottom_.x, right_top_.x, p.x)};
        const double ny{calculateBorder(left_bottom_.y, right_top_.y, p.y)};
        
        return Position{nx, ny};
    }
    
    Position Road::lbPos() const
    {
        return left_bottom_;
    }
    
    Position Road::rtPos() const
    {
        return right_top_;
    }
    
    void Road::init() noexcept
    {
        left_bottom_.x = static_cast<double>(std::min(start_.x, end_.x)) - width_;
        left_bottom_.y = static_cast<double>(std::min(start_.y, end_.y)) - width_;
        
        right_top_.x = static_cast<double>(std::max(start_.x, end_.x)) + width_;
        right_top_.y = static_cast<double>(std::max(start_.y, end_.y)) + width_;
    }
    
    
    
    
}  // namespace model

#include "game_to_json.h"

namespace GameToJson
{
    template<typename Handler, typename Container>
    json::array DataToJsonArray(const Container& cont,                   
                   Handler hh)
    {
        json::array out;
        
        for(const auto& item: cont)
        {
            out.push_back(hh(item));
        }
        
        return out;
    }
    
    json::object RoadToJson(const model::Road& road)
    {
        json::object out;
        
        out["x0"] = road.GetStart().x;
        out["y0"] = road.GetStart().y;
        
        if(road.IsVertical())
        {
            out["y1"] = road.GetEnd().y;
        }
        else
        {
            out["x1"] = road.GetEnd().x;
        }
        
        return out;
    }
    
    json::object BuildingToJson(const model::Building& building)
    {
        json::object out;                
        
        const model::Rectangle& rect{building.GetBounds()};
        
        out["x"] = rect.position.x;
        out["y"] = rect.position.y;
        
        out["w"] = rect.size.width;
        out["h"] = rect.size.height;
        
        return out;
    }
    
    json::object OfficeToJson(const model::Office& office)
    {
        json::object out;                
        
        out["id"] = *office.GetId();
        
        out["x"] = office.GetPosition().x;
        out["y"] = office.GetPosition().y;
        
        out["offsetX"] = office.GetOffset().dx;
        out["offsetY"] = office.GetOffset().dy;
        
        return out;
    }
    
    
    json::object MapToJson(const model::Map* map) 
    {
        if(nullptr == map)
        {
            return {};
        }
        
        json::object map_obj;
        
        map_obj["id"] = *map->GetId();
        map_obj["name"] = map->GetName();
        map_obj["roads"] = DataToJsonArray(map->GetRoads(), RoadToJson);
        map_obj["buildings"] = DataToJsonArray(map->GetBuildings(), BuildingToJson);
        map_obj["offices"] = DataToJsonArray(map->GetOffices(), OfficeToJson);
        
        return map_obj;
    }
    
    json::array MapsToJson(const model::Game::Maps& maps) 
    {
        json::array out;
        
        for(const auto& it: maps)
        {
            json::object obj_map; 
            
            obj_map["id"] = *it.GetId();
            obj_map["name"] = it.GetName();
            
            out.push_back(obj_map);
        }
        
        return out;       
    }
    
    json::array State(const double f, const double s)
    {
        json::array out;     
        
        out.emplace_back(f);
        out.emplace_back(s);
        
        return out;
    }
    
    json::object DogStateToJson(const model::Dog& dog)
    {
        json::object out;                
        
        out["pos"] = State(dog.position().x, dog.position().y);
        out["speed"] = State(dog.speed().v_x, dog.speed().v_y);
        
        
        switch (dog.dir()) 
        {
            case model::Direction::NORTH:
            {
                out["dir"] = "U";
            }
            break;
                
            case model::Direction::SOUTH:
            {
                out["dir"] = "D";
            }
            break;
                
            case model::Direction::WEST:
            {
                out["dir"] = "L";
            }
            break;
                
            case model::Direction::EAST:
            {
                out["dir"] = "R";
            }
            break;
        }
        
        return out;
    }
    
    json::object GameStateToJson(const model::GameSession::Dogs& dogs)
    {
        json::object out;   
        
        json::object players;  
        for(size_t i{0}; i < dogs.size(); ++i)
        {
            const std::string index{std::to_string(i)};            
            
            players[index] = DogStateToJson(dogs[i]);
        }
        out["players"] = players;
        return out;
    }
    
    json::object DogToJson(const model::Dog& dog)
    {
        json::object out;                
        
        out["name"] = dog.name();
        
        return out;
    }
    
    json::object DogsToJson(const model::GameSession::Dogs& dogs)
    {
        json::object out;   
        
        for(size_t i{0}; i < dogs.size(); ++i)
        {
            const std::string index{std::to_string(i)};
            
            json::object dog;
            
            dog["name"] = DogToJson(dogs[i]);
            
            out[index] = dog;
        }
        
        return out;
    }
    
    
}



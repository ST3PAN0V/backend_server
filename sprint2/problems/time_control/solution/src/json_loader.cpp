#include <iostream>
#include <fstream>
#include <iterator>

#include <boost/json.hpp>

#include "json_loader.h"

namespace json = boost::json;

namespace json_loader 
{
    void LoadRoad(json::value& road, 
                  model::Map& map)
    {
        int x0 = road.at("x0").as_int64();
        int y0 = road.at("y0").as_int64();
        
        if(road.as_object().if_contains("x1"))        
        {
            //hor
            json::value x1 = road.at("x1");
            model::Road out{
                model::Road::HORIZONTAL, 
                {x0, y0}, 
                static_cast<model::Coord>(x1.as_int64()) };
            
            map.AddRoad(out); 
            return;
        }
                
        if(!road.as_object().if_contains("y1"))        
        {
            return;
        }
        
        json::value y1 = road.at("y1");
        model::Road out{
            model::Road::VERTICAL, 
            {x0, y0}, 
            static_cast<model::Coord>(y1.as_int64())};
        
        map.AddRoad(out);         
    }
    
    void LoadBuilding(json::value& building, 
                      model::Map& map)
    {
        int x = building.at("x").as_int64();
        int y = building.at("y").as_int64();
        int w = building.at("w").as_int64();
        int h = building.at("h").as_int64();
        
        model::Rectangle rect{{x, y}, {w, h}};
        
        map.AddBuilding(model::Building{rect});
    }
    
    void LoadOffice(json::value& office, 
                    model::Map& map)
    {
        const std::string id = office.at("id").as_string().c_str();
        
        int x = office.at("x").as_int64();
        int y = office.at("y").as_int64();
        int offsetX = office.at("offsetX").as_int64();
        int offsetY = office.at("offsetY").as_int64();
        
        model::Office o{model::Office::Id{id}, {x, y}, {offsetX, offsetY}};
        
        map.AddOffice(o);
    }
        
    template<typename Handler, typename Container>
    void LoadArray(json::array& array, 
                   Container& cont,                   
                   Handler hh)
    {
        auto it = array.begin();
        
        while(it != array.end())
        {
            hh(*it, cont);                    
            
            std::advance(it, 1);
        }
    }
    
    void LoadMap(json::value& jv,
                 model::Game& game, 
                 const double defDogSpeed)
    {
        static const std::string SpeedTag{"dogSpeed"};
        auto const& map = jv.get_object();
        if( map.empty())
        {
            return;
        }
        
        const std::string id = map.at("id").as_string().c_str();
        const std::string name = map.at("name").as_string().c_str();
                
        model::Map out{model::Map::Id{id}, name};
        
        json::array roads = jv.at("roads").as_array();           
        
        if(!roads.empty())
        {
            LoadArray(roads, out, LoadRoad);
        }
        
        json::array buildings = jv.at("buildings").as_array();           
        
        if(!buildings.empty())
        {
            LoadArray(buildings, out, LoadBuilding);
        }
        
        json::array offices = jv.at("offices").as_array();           
        
        if(!offices.empty())
        {
            LoadArray(offices, out, LoadOffice);
        }
        
        out.setDogSpeed(defDogSpeed);
        
        json::object obj{jv.as_object()};
        if(obj.if_contains(SpeedTag))
        {
            if(obj.at(SpeedTag).is_double())
            {
                out.setDogSpeed(obj.at(SpeedTag).as_double());
            }
        }
        
        game.AddMap(out);
    }
    
    model::Game LoadGame(const std::filesystem::path& json_path) 
    {
        // Загрузить содержимое файла json_path, например, в виде строки
        // Распарсить строку как JSON, используя boost::json::parse
        // Загрузить модель игры из файла
        static const std::string SpeedTag{"defaultDogSpeed"};
        std::ifstream ifs(json_path);
        
        if(!ifs.is_open())
        {
            return model::Game{};
        }
        
        std::string content( (std::istreambuf_iterator<char>(ifs) ),
                             (std::istreambuf_iterator<char>()    ) );    
        if(content.empty())
        {
            return model::Game{};
        }

        json::error_code ec;
        json::value config = json::parse(content, ec);
        
        model::Game game;
        
        if(!ec)
        {            
            json::array maps = config.at("maps").as_array();               
            
            double defaultDogSpeed{1.0};
            json::object obj{config.as_object()};
            
            if(obj.if_contains(SpeedTag))
            {
                if(obj.at(SpeedTag).is_double())
                {
                    defaultDogSpeed = obj.at(SpeedTag).as_double();
                }
            }
            
            if(!maps.empty())
            {
                LoadArray(maps, game, 
                          [defaultDogSpeed](json::value& jv, model::Game& game)
                {
                    LoadMap(jv, game, defaultDogSpeed);
                } );
            }
        }

        return game;
    }
}  // namespace json_loader

#include "json_loader.h"
#include "tagged.h"
#include <fstream>
#include <iostream>
#include <string>
#include <boost/json.hpp>

namespace json_loader 
{
    namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path) 
{
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    model::Game game;

    std::ifstream file(json_path);
    std::ostringstream tmp;

    tmp << file.rdbuf();

    std::string json_str = tmp.str();

    auto value = json::parse(json_str);

    json::array maps = value.as_object().at("maps").as_array();

    for (auto entry : maps)
    {
        json::object map_data = entry.as_object();

        using Id = util::Tagged<std::string, model::Map>;

        Id id{std::string(map_data.at("id").as_string())};
        model::Map map(id, std::string(map_data.at("name").as_string()));

        for (auto road_entry : map_data.at("roads").as_array())
        {
            json::object road_data = road_entry.as_object();

            if (road_data.contains("x1"))
            {
                model::Road road(
                {
                    model::Road::HORIZONTAL,
                    {int(road_data.at("x0").as_int64()), int(road_data.at("y0").as_int64())},
                    int(road_data.at("x1").as_int64())
                });

                map.AddRoad(std::move(road));
            }
            else
            {
                model::Road road(
                {
                    model::Road::VERTICAL,
                    {int(road_data.at("x0").as_int64()), int(road_data.at("y0").as_int64())},
                    int(road_data.at("y1").as_int64())
                });

                map.AddRoad(std::move(road));
            }

        }

        for (auto building_entry : map_data.at("buildings").as_array())
        {
            json::object building_data = building_entry.as_object();

            model::Building building(
            { 
                {int(building_data.at("x").as_int64()), int(building_data.at("y").as_int64())}, 
                {int(building_data.at("w").as_int64()), int(building_data.at("h").as_int64())}
            });

            map.AddBuilding(std::move(building));
        }

        for (auto office_entry : map_data.at("offices").as_array())
        {
            json::object office_data = office_entry.as_object();

            using Id = util::Tagged<std::string, model::Office>;
            Id id{ std::string(office_data.at("id").as_string()) };

            model::Office office(
            {
                id,
                {int(office_data.at("x").as_int64()), int(office_data.at("y").as_int64())},
                {int(office_data.at("offsetX").as_int64()), int(office_data.at("offsetY").as_int64())}
            });

            map.AddOffice(std::move(office));
        }

        game.AddMap(map);
    }

    return game;
}

}  // namespace json_loader

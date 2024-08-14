#include "json_loader.h"

#include <iostream>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace json_loader {

using namespace boost::property_tree;

model::Road LoadRoad(const ptree &road) {
    try {
        model::Point point_start;
        point_start.x = road.get<int>("x0");
        point_start.y = road.get<int>("y0");
        if (road.to_iterator(road.find("y1")) == road.end()) {
            return model::Road(model::Road::HORIZONTAL, point_start, road.get<int>("x1"));
        }
        else {
            return model::Road(model::Road::VERTICAL, point_start, road.get<int>("y1"));
        }
    }
    catch (std::exception& e) {
        throw std::runtime_error("Error loading road " + std::string(e.what()));
    }
}

model::Building LoadBuilding(const ptree& building) {
    try {
        auto rectangle = model::Rectangle(
            {
                .position = {
                    .x = building.get<int>("x"),
                    .y = building.get<int>("y")
                },
                .size = {
                    .width = building.get<int>("w"),
                    .height = building.get<int>("h")
                }
            }
        );

        return model::Building(rectangle);
    }
    catch (std::exception e) {
        throw std::runtime_error("Error loading building " + std::string(e.what()));
    }
}

model::Office LoadOffice(const ptree& office) {
    try {
        auto office_id = model::Office::Id(office.get<std::string>("id"));
        return model::Office(office_id, 
            model::Point{.x = office.get<int>("x"), .y = office.get<int>("y")}, 
            model::Offset{.dx = office.get<int>("offsetX"), .dy = office.get<int>("offsetY")});
    }
    catch (std::exception e) {
        throw std::runtime_error("Error loading office " + std::string(e.what()));
    }
}

model::Map LoadMap(const ptree& map, double defaultDogSpeed) {
    try {
        auto map_id = model::Map::Id(map.get<std::string>("id"));
        auto map_name = map.get<std::string>("name");
        model::Map model_map(map_id, map_name);

        // check dog speed on map
        if (map.to_iterator(map.find("dogSpeed")) != map.end()) {
            model_map.SetDogSpeed(map.get<double>("dogSpeed"));
        }
        else {
            model_map.SetDogSpeed(defaultDogSpeed);
        }

        auto roads = map.get_child("roads");
        for (auto road : roads) {
            model_map.AddRoad(LoadRoad(road.second));
        }
        auto buildings = map.get_child("buildings");
        for (auto building : buildings) {
            model_map.AddBuilding(LoadBuilding(building.second));
        }
        auto offices = map.get_child("offices");
        for (auto office : offices) {
            model_map.AddOffice(LoadOffice(office.second));
        }
        return model_map;
    }
    catch (std::exception e) {
        throw std::runtime_error("Error loading map " + std::string(e.what()));
    }
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла

    model::Game game;

    std::ifstream jsonFile(json_path);
    if (!jsonFile) {
        throw std::runtime_error("Error opening file " + json_path.string());
    }

    try {
        ptree pt;
        json_parser::read_json(jsonFile, pt);

        // check default dog speed gloabal
        auto defaultDogSpeed = pt.get<double>("defaultDogSpeed");
        game.SetDefaultDogSpeed(defaultDogSpeed);

        auto maps = pt.get_child("maps");
        for (auto map : maps) {
            game.AddMap(LoadMap(map.second, defaultDogSpeed));
        }
    } 
    catch (ptree_error &e) {
        throw std::runtime_error("Json file read error: "  + std::string(e.what()));
    }

    return game;
}

}  // namespace json_loader

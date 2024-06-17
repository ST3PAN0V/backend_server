#include "json_loader.h"
#include <fstream>

namespace json_loader {

model::Road MakeObjectRoadFromJson(const boost::json::value& jsroad) {
    try {
        return model::Road(model::Road::HORIZONTAL, {static_cast<int>(jsroad.at("x0").as_int64()),
                static_cast<int>(jsroad.at("y0").as_int64())}, static_cast<int>(jsroad.at("x1").as_int64()));
    } catch (const std::exception& ex) {
        return model::Road(model::Road::VERTICAL, {static_cast<int>(jsroad.at("x0").as_int64()),
                static_cast<int>(jsroad.at("y0").as_int64())}, static_cast<int>(jsroad.at("y1").as_int64()));
    }
}

model::Building MakeObjectBuildingFromJson(const boost::json::value& jsbuilding) {
    auto point = model::Point(static_cast<int>(jsbuilding.at("x").as_int64()), static_cast<int>(jsbuilding.at("y").as_int64()));
    auto size = model::Size(static_cast<int>(jsbuilding.at("w").as_int64()), static_cast<int>(jsbuilding.at("h").as_int64()));
    auto rectangle = model::Rectangle(std::move(point), std::move(size));
    return model::Building(std::move(rectangle));
}

model::Office MakeObjectOfficeFromJson(const boost::json::value& jsoffice) {
    using IdOffice = util::Tagged<std::string, model::Office>;
    auto point = model::Point(static_cast<int>(jsoffice.at("x").as_int64()), static_cast<int>(jsoffice.at("y").as_int64()));
    auto offset = model::Offset(static_cast<int>(jsoffice.at("offsetX").as_int64()), static_cast<int>(jsoffice.at("offsetY").as_int64()));
    IdOffice office_id(jsoffice.at("id").as_string().c_str());
    return model::Office(office_id, std::move(point), std::move(offset));
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    using IdMap = util::Tagged<std::string, model::Map>;
    using IdOffice = util::Tagged<std::string, model::Office>;

    model::Game game;

    std::ifstream file(json_path);
    std::string json_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto json_parsed = json::parse(json_content);

    for (const auto& jsmap : json_parsed.at("maps").as_array()) {
        IdMap map_id(jsmap.at("id").as_string().c_str());
        auto map_name = jsmap.at("name").as_string().c_str();

        model::Map map(map_id, map_name);

        for (const auto& jsroad : jsmap.at("roads").as_array()) {
            map.AddRoad(std::move(MakeObjectRoadFromJson(jsroad)));
        }

        for (const auto& jsbuilding : jsmap.at("buildings").as_array()) {
            map.AddBuilding(std::move(MakeObjectBuildingFromJson(jsbuilding)));
        }

        for (const auto& jsoffice : jsmap.at("offices").as_array()) {
            map.AddOffice(std::move(MakeObjectOfficeFromJson(jsoffice)));
        }
        game.AddMap(std::move(map));
    }
    return game;
}

boost::json::object ParseRoadInJson(const model::Road& road) {
    boost::json::object road_node;
    road_node["x0"] = road.GetStart().x;
    road_node["y0"] = road.GetStart().y;
    if (road.IsHorizontal()) {
        road_node["x1"] = road.GetEnd().x;
    } else {
        road_node["y1"] = road.GetEnd().y;
    }
    return road_node;
}

boost::json::object ParseBuildingInJson(const model::Building& building) {
    boost::json::object building_node;
    building_node["x"] = building.GetBounds().position.x;
    building_node["y"] = building.GetBounds().position.y;
    building_node["w"] = building.GetBounds().size.width;
    building_node["h"] = building.GetBounds().size.height;
    return building_node;
}

boost::json::object ParseOfficeInJson(const model::Office& office) {
    boost::json::object office_node;
    office_node["id"] = office.GetId().operator*();
    office_node["x"] = office.GetPosition().x;
    office_node["y"] = office.GetPosition().y;
    office_node["offsetX"] = office.GetOffset().dx;
    office_node["offsetY"] = office.GetOffset().dy;
    return office_node;
}


std::optional<std::string> FoundAndGetStringMap(const std::vector<model::Map>& maps, const std::string& needIdMap) {
    auto needMap = std::find_if(maps.begin(), maps.end(), [&needIdMap](const model::Map& map){
        return map.GetId().operator*() == needIdMap;
    });


    if (needMap != maps.end()) {
        model::Map map = *needMap;
        boost::json::object response;

        response["id"] = map.GetId().operator*();
        response["name"] =  map.GetName();

        boost::json::array roads;
        for (const auto& road : map.GetRoads()) {
            roads.push_back(std::move(ParseRoadInJson(road)));
        }
        response["roads"] = roads;

        boost::json::array buildings;
        for (const auto& building : map.GetBuildings()) {
            buildings.push_back(std::move(ParseBuildingInJson(building)));
        }
        response["buildings"] = buildings;

        boost::json::array offices;
        for (const auto& office : map.GetOffices()) {
            offices.push_back(std::move(ParseOfficeInJson(office)));
        }
        response["offices"] = offices;
        
        return boost::json::serialize(response);
    } else {
        return std::nullopt;
    }
}

std::string GetStringFromMaps(const std::vector<model::Map>& maps) {
    boost::json::array mapsArray;

    for (const auto& map : maps) {
        boost::json::object mapObject;
        mapObject["id"] = map.GetId().operator*();
        mapObject["name"] = map.GetName();
        mapsArray.push_back(mapObject);
    }

    return boost::json::serialize(mapsArray);
}

}  // namespace json_loader

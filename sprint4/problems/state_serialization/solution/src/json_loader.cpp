#include "json_loader.h"
#include "constants.h"

#include <fstream>

using namespace model;
using namespace constants;

namespace json_loader {

json::object GetRootJsonObject(const std::filesystem::path& json_path) {
    std::ifstream file(json_path);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + json_path.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    json::error_code ec;
    json::value jroot = json::parse(content, ec);
    if (ec) {
        throw std::runtime_error("Failed to parse JSON: " + ec.message());
    }

    return jroot.as_object();
}

int get_int(const json::value& value) {
    return static_cast<int>(value.as_int64());
}

void LoadRoads(const auto& jmap, auto& map) {
    for (const auto& jroad : jmap.at(ROADS).as_array()) {

        Point start{get_int(jroad.at(X0)), get_int(jroad.at(Y0))};

        const auto& obj = jroad.as_object();

        if (obj.if_contains(X1)) {
            Coord end = get_int(obj.at(X1));

            map.AddRoad({Road::HORIZONTAL, start, end});
        }
        else if (obj.if_contains(Y1)) {
            Coord end = get_int(obj.at(Y1));

            map.AddRoad({Road::VERTICAL, start, end});
        }
    }
}

void LoadBuildings(const auto& jmap, auto& map) {
    for (const auto& jbuilding : jmap.at(BUILDINGS).as_array()) {

        Point position{get_int(jbuilding.at(X)), get_int(jbuilding.at(Y))};
        Size size{get_int(jbuilding.at(W)), get_int(jbuilding.at(H))};

        map.AddBuilding(Building{{position, size}});
    }
}

void LoadOffices(const auto& jmap, auto& map) {
    for (const auto& joffice : jmap.at(OFFICES).as_array()) {

        Office::Id id{json::value_to<std::string>(joffice.at(ID))};
        Point position{get_int(joffice.at(X)), get_int(joffice.at(Y))};
        Offset offset{get_int(joffice.at(OFFSET_X)), get_int(joffice.at(OFFSET_Y))};

        map.AddOffice(Office{id, position, offset});
    }
}

void LoadDogSpeed(const auto& jmap, auto& map, double default_speed) {
    if (jmap.as_object().contains("dogSpeed")) {
        double dog_speed = jmap.at("dogSpeed").as_double();
        map.SetDogSpeed(dog_speed);
    } else {
        map.SetDogSpeed(default_speed);
    }
}

void LoadBagCapacity(const auto& jmap, auto& map, int default_capacity) {
    if (jmap.as_object().contains("bagCapacity")) {
        int capacity = get_int(jmap.at("bagCapacity"));
        map.SetBagCapacity(capacity);
    } else {
        map.SetDogSpeed(default_capacity);
    }
}


model::Game LoadGame(const json::object& jroot) {
    model::Game game{};

    double default_dog_speed = 1.0;
    if (jroot.contains("defaultDogSpeed")) {
        default_dog_speed = jroot.at("defaultDogSpeed").as_double();
    }

    int default_bag_capacity = 3;
    if (jroot.contains("defaultBagCapacity")) {
        default_bag_capacity = get_int(jroot.at("defaultBagCapacity"));
    }

    const auto& jmaps = jroot.at(MAPS);
    for (const auto& jmap : jmaps.as_array()) {

        Map::Id id{json::value_to<std::string>(jmap.at(ID))};
        std::string name{json::value_to<std::string>(jmap.at(NAME))};
        Map map{id, std::move(name)};

        LoadRoads(jmap, map);
        LoadBuildings(jmap, map);
        LoadOffices(jmap, map);
        LoadDogSpeed(jmap, map, default_dog_speed);
        LoadBagCapacity(jmap, map, default_bag_capacity);

        game.AddMap(map);
    }

    return game;
}

loot::Generator LoadLootGenerator(const json::object& jroot) {
    double period_s = 0., probability = 0.;
    if (jroot.contains("lootGeneratorConfig")) {
        auto jconfig = jroot.at("lootGeneratorConfig");
        if (jconfig.as_object().contains("period")) {
            period_s = jconfig.at("period").as_double();
        }
        if (jconfig.as_object().contains("probability")) {
            probability = jconfig.at("probability").as_double();
        }
    }
    auto period_ms = loot::Generator::MakeTimeInterval(static_cast<int64_t>(period_s * 1000));

    return {period_ms, probability};
}

loot::Data LoadLootData(const json::object& jroot) {
    auto maps = jroot.at(MAPS).as_array();

    std::unordered_map<std::string, std::string> loot_types_map;

    for (const auto& map : maps) {
        std::string map_id = map.as_object().at(ID).as_string().c_str();

        const auto& loot_types = map.as_object().at(LOOT_TYPES);
        loot_types_map[map_id] = boost::json::serialize(loot_types);
    }
    return loot::Data{std::move(loot_types_map)};
}


}  // namespace json_loader

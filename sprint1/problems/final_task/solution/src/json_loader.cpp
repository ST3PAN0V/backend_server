#include "json_loader.h"

#include <fstream>


namespace json_loader {

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
            try {
                map.AddRoad(model::Road(model::Road::HORIZONTAL, {static_cast<int>(jsroad.at("x0").as_int64()),
                        static_cast<int>(jsroad.at("y0").as_int64())}, static_cast<int>(jsroad.at("x1").as_int64())));
            } catch (std::out_of_range ex){
                map.AddRoad(model::Road(model::Road::VERTICAL, {static_cast<int>(jsroad.at("x0").as_int64()),
                        static_cast<int>(jsroad.at("y0").as_int64())}, static_cast<int>(jsroad.at("y1").as_int64())));
            }
        }

        for (const auto& jsbuilding : jsmap.at("buildings").as_array()) {
            auto point = model::Point(static_cast<int>(jsbuilding.at("x").as_int64()), static_cast<int>(jsbuilding.at("y").as_int64()));
            auto size = model::Size(static_cast<int>(jsbuilding.at("w").as_int64()), static_cast<int>(jsbuilding.at("h").as_int64()));
            auto rectangle = model::Rectangle(std::move(point), std::move(size));
            map.AddBuilding(model::Building(std::move(rectangle)));
        }

        for (const auto& jsoffice : jsmap.at("offices").as_array()) {
            auto point = model::Point(static_cast<int>(jsoffice.at("x").as_int64()), static_cast<int>(jsoffice.at("y").as_int64()));
            auto offset = model::Offset(static_cast<int>(jsoffice.at("offsetX").as_int64()), static_cast<int>(jsoffice.at("offsetY").as_int64()));
            IdOffice office_id(jsoffice.at("id").as_string().c_str());
            map.AddOffice(model::Office(office_id, std::move(point), std::move(offset)));
        }
        game.AddMap(std::move(map));
    }
    return game;
}

}  // namespace json_loader

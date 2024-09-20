#pragma once

#include "./game/model.h"
#include "./game/loot_data.h"
#include "./game/loot_generator.h"

#include <boost/json.hpp>
#include <filesystem>


namespace json = boost::json;

namespace json_loader {

json::object GetRootJsonObject(const std::filesystem::path& json_path);

model::Game LoadGame(const json::object& jroot);
loot::Generator LoadLootGenerator(const json::object& jroot);
loot::Data LoadLootData(const json::object& jroot);

}  // namespace json_loader

#pragma once

#include <filesystem>
#include <boost/json.hpp>

#include <iostream>
#include <string>

namespace json = boost::json;
using namespace std::literals;

#include "model.h"

namespace json_loader {

namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path);

model::Road&& MakeObjectRoadFromJson(const boost::json::value& jsroad);

model::Building&& MakeObjectBuildingFromJson(const boost::json::value& jsbuilding);

inline model::Office&& MakeObjectOfficeFromJson(const boost::json::value& jsoffice);

std::optional<std::string> FoundAndGetStringMap(const std::vector<model::Map>& maps, const std::string& needIdMap);

std::string GetStringFromMaps(const std::vector<model::Map>& maps);

boost::json::object&& ParseRoadInJson(const model::Road& road);

boost::json::object&& ParseBuildingInJson(const model::Building& building);

boost::json::object&& ParseOfficeInJson(const model::Office& office);

}  // namespace json_loader


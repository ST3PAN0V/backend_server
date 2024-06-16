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

}  // namespace json_loader


#pragma once
#include <unordered_map>
#include <vector>
#include <string>

namespace loot {

class Data {
public:

    Data(std::unordered_map<std::string, std::string>&& map) : map_(std::move(map)){}
    const std::string& GetLootTypesOnMap(const std::string& map_id) const;

private:
    std::unordered_map<std::string, std::string> map_;
};

}  // namespace loot

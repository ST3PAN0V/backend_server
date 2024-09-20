#pragma once

#include <unordered_map>
#include <vector>
#include <string>

namespace loot {

class Data {
public:
    explicit Data(std::unordered_map<std::string, std::string>&& map);
    const std::string& GetLootTypesOnMap(const std::string& map_id) const;
    const std::vector<int>& GetLootValuesOnMap(const std::string& map_id) const;

private:
    std::unordered_map<std::string, std::string> map_;
    std::unordered_map<std::string, std::vector<int>> values_;
};

}  // namespace loot

#include "./../../headers/game/loot_data.h"

#include <boost/json.hpp>

namespace json = boost::json;

namespace loot {

Data::Data(std::unordered_map<std::string, std::string>&& map) : map_(std::move(map)){
    for (const auto& [map_id, jstr] : map_) {
        auto jarray = json::parse(jstr).as_array();
        std::vector<int> values;

        for (const auto& jvalue : jarray) {
            const auto& jobj = jvalue.as_object();

            if (jobj.contains("value")) {
                int value = static_cast<int>(jobj.at("value").as_int64());
                values.push_back(value);
            }
        }
        values_.insert({map_id, values});
    }
}

const std::string& Data::GetLootTypesOnMap(const std::string& map_id) const {
    auto it = map_.find(map_id);
    if (it == map_.end()) {
        static const std::string empty;
        return empty;
    }
    return it->second;
}

const std::vector<int>& Data::GetLootValuesOnMap(const std::string& map_id) const {
    auto it = values_.find(map_id);
    if (it == values_.end()) {
        static const std::vector<int> empty;
        return empty;
    }
    return it->second;
}


}  // namespace loot

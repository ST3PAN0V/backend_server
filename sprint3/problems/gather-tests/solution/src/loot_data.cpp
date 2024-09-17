#include "loot_data.h"

namespace loot {

const std::string& Data::GetLootTypesOnMap(const std::string& map_id) const {
    auto it = map_.find(map_id);
    if (it == map_.end()) {
        static const std::string empty;
        return empty;
    }
    return it->second;
}

}  // namespace loot

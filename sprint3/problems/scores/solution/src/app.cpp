#include "app.h"
#include "collision_detector.h"

#include <sstream>
#include <iomanip>


using namespace collision_detector;

const double dog_width = 0.6;
const double loot_width = 0.0;
const double office_width = 0.5;

class MapProvider : public ItemGathererProvider {
public:
    MapProvider(size_t id, const model::Map* map, const Players& players)
    : map_(map), players_(players), id_(id) {
    }
    
    size_t GetId() const override {return id_;}

    size_t GatherersCount() const override {
        return players_.size();
    }

    Gatherer GetGatherer(size_t index) const override {
        const auto* dog_ptr = players_[index]->GetDog();
        return Gatherer{dog_ptr->GetLastPosition(), dog_ptr->GetPosition(), dog_width/2};
    }

    size_t ItemsCount() const override {
        return map_->GetLostObjects().size() + map_->GetOffices().size();
    }

    Item GetItem(size_t index) const override {
        const auto& loots = map_->GetLostObjects();
        const auto& offices = map_->GetOffices();
        if (index < loots.size()) {
            const auto& loot = loots.at(index);
            return Item{loot.position, loot_width/2};
        }
        index -= loots.size();
        const auto& office = offices.at(index);
        return Item{{static_cast<double>(office.GetPosition().x), static_cast<double>(office.GetPosition().y)}, office_width/2};
    }

    const model::Map* GetMap() const {return map_;}
    const Players& GetPlayers() const {return players_;}

private:
    const model::Map* map_;
    Players players_;
    size_t id_;
};

std::string GenerateRandomHex(std::mt19937_64& generator) {
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << generator();
    return oss.str();
}

Token PlayerTokens::GenerateToken() {
    auto part1 = GenerateRandomHex(generator1_);
    auto part2 = GenerateRandomHex(generator2_);
    return Token(part1 + part2);
}

void Player::SetDogToMapRandom(const model::Map* map) {
    if (!map) {
        return;
    }
    map_ = map;
    dog_.SetStartPosition(map->GetRandomPoint());
    dog_.SetBagCapacity(map->GetBagCapacity());
}

void Player::SetDogToMap(const model::Map* map) {
    if (!map) {
        return;
    }
    map_ = map;
    dog_.SetStartPosition(map->GetInitialPoint());
    dog_.SetBagCapacity(map->GetBagCapacity());
}

std::tuple<Token, int> App::AddPlayer(const std::string& name, const model::Map* map) {
    static int player_id = 0;
    const int new_id = player_id++;
    const Token new_token = generator_.GenerateToken();
    players_.emplace(new_token, std::make_unique<Player>(new_id, name));

    auto* player_ptr = players_.at(new_token).get();
    if (randomize_spawn_) {
        player_ptr->SetDogToMapRandom(map);
    }
    else {
        player_ptr->SetDogToMap(map);
    }
    players_on_map_[map->GetId()].push_back(player_ptr);
    return {new_token, new_id};
}

Player* App::GetPlayer(Token token) const {
    if (players_.contains(token)) {
        return players_.at(token).get();
    }
    return nullptr;
}

Players App::GetPlayers() const {
    Players result;
    for (const auto& [_, player] : players_) {
        result.push_back(player.get());
    }
    return result;
}


const Players& App::GetPlayersOnMap(const model::Map* map) const {
    static const Players empty_players;
    if (map && players_on_map_.contains(map->GetId())) {
        return players_on_map_.at(map->GetId());
    }
    return empty_players;
}

void App::Move(int64_t time_ms) {
    for (const auto& [_, player] : players_) {
        player->GetDog()->Move(player->GetMap(), time_ms);
    }

    using namespace collision_detector;
    std::vector<GatheringEvent> all_events;

    std::vector<MapProvider> providers;
    for (const auto& [_, players] : players_on_map_) {
        if (!players.empty()) {
            providers.emplace_back(providers.size(), players.front()->GetMap(), players);
            
            auto events = FindGatherEvents(providers.back());
            std::move(events.begin(), events.end(), std::back_inserter(all_events));
        }
    }

    std::sort(all_events.begin(), all_events.end(), [](const GatheringEvent& a, const GatheringEvent& b) {
        return a.time < b.time;
    });

    for (const auto& event : all_events) {
        const auto& provider = providers[event.provider_id];
        const auto& item = provider.GetItem(event.item_id);

        auto* map_ptr = const_cast<model::Map*>(provider.GetMap());
        auto* player_ptr = provider.GetPlayers().at(event.gatherer_id);

        const auto& loot_opt = map_ptr->TakeLootAtPosition(item.position);
        if (loot_opt) {
            player_ptr->GetDog()->Loot(*loot_opt);
        }
        else if (map_ptr->IsOfficeAtPosition(item.position)) {
            player_ptr->GetDog()->ReturnLoot();
        }
    }
}
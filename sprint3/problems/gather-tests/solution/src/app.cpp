#include "app.h"

#include <sstream>
#include <iomanip>


Token PlayerTokens::GenerateToken() {
    auto part1 = GenerateRandomHex(generator1_);
    auto part2 = GenerateRandomHex(generator2_);
    return Token(part1 + part2);
}

std::string PlayerTokens::GenerateRandomHex(std::mt19937_64& generator) {
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << generator();
    return oss.str();
}

void Player::SetDogToMapRandom(const model::Map* map) {
    if (!map) {
        return;
    }
    map_ = map;
    dog_.SetPosition(map->GetRandomPoint());
}

void Player::SetDogToMap(const model::Map* map) {
    if (!map) {
        return;
    }
    map_ = map;
    dog_.SetPosition(map->GetInitialPoint());
}

std::tuple<Token, int> App::AddPlayer(const std::string& name, const model::Map* map) {
    static int player_id = 0;
    const int new_id = player_id++;
    const Token new_token = generator_.GenerateToken();
    players_.emplace(new_token, std::make_unique<Player>(new_id, name));

    auto* player = players_.at(new_token).get();
    if (randomize_spawn_) {
        player->SetDogToMapRandom(map);
    }
    else {
        player->SetDogToMap(map);
    }
    players_on_map_[map->GetId()].insert(player);
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
        result.insert(player.get());
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
}
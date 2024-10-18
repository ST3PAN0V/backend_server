#pragma once

#include "loot_generator.h"
#include "loot_data.h"

#include "../model/model.h"

#include <random>
#include <string>
#include <unordered_set>
#include <memory>


namespace detail {
    struct TokenTag {};
}

using Token = util::Tagged<std::string, detail::TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;

using MapId = model::Map::Id;
using MapIdHasher = util::TaggedHasher<MapId>;

class PlayerTokens {
public:
    Token GenerateToken();

private:
    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
};

class Player {
public:
    Player() = default;
    explicit Player(int id, const std::string& name)
    : id_(id)
    , name_(name)
    {}

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    int GetId() const {return id_;}
    std::string GetName() const {return name_;}

    void SetMap(const model::Map* map);
    void SetDogToMapRandom(const model::Map* map);
    void SetDogToMap(const model::Map* map);
    const model::Map* GetMap() const {return map_;}
    model::Dog* GetDog() {return &dog_;}
    const model::Dog* GetDog() const {return &dog_;}

private:
    int id_ = 0;
    std::string name_;
    const model::Map* map_ = nullptr;
    model::Dog dog_;
};

using Players = std::vector<Player*>;
using PlayerPtr = std::unique_ptr<Player>;
using PlayersMap = std::unordered_map<Token, PlayerPtr, TokenHasher>;

class App {
public:
    App(bool randomize_spawn = false) 
    : randomize_spawn_(randomize_spawn)
    {}

    std::tuple<Token, int> AddPlayer(const std::string& name, const model::Map* map);
    void AddPlayer(Token&& token, PlayerPtr&& player);
    Player* GetPlayer(Token token) const;
    const PlayersMap& GetPlayers() const {return players_;}
    const Players& GetPlayersOnMap(const model::Map* map) const;
    void Move(int64_t time_ms);
    std::vector<PlayerPtr> RemoveRetiredPlayers();

private:
    static int player_id_;
    PlayerTokens generator_;
    PlayersMap players_;
    std::unordered_map<MapId, Players, MapIdHasher> players_on_map_;
    bool randomize_spawn_ = false;
};

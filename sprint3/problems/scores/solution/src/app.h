#pragma once

#include "tagged.h"
#include "model.h"
#include "loot_generator.h"
#include "loot_data.h"

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

    void SetDogToMapRandom(const model::Map* map);
    void SetDogToMap(const model::Map* map);
    const model::Map* GetMap() const {return map_;}
    model::Dog* GetDog() {return &dog_;}

private:
    int id_ = 0;
    std::string name_;
    const model::Map* map_ = nullptr;
    model::Dog dog_;
};

using Players = std::vector<Player*>;

class App {
public:
    App(bool randomize_spawn = false) 
    : randomize_spawn_(randomize_spawn)
    {}

    std::tuple<Token, int> AddPlayer(const std::string& name, const model::Map* map);
    Player* GetPlayer(Token token) const;
    Players GetPlayers() const;
    const Players& GetPlayersOnMap(const model::Map* map) const;
    void Move(int64_t time_ms);

private:
    PlayerTokens generator_;
    std::unordered_map<Token, std::unique_ptr<Player>, TokenHasher> players_;
    std::unordered_map<MapId, Players, MapIdHasher> players_on_map_;
    bool randomize_spawn_ = false;
};

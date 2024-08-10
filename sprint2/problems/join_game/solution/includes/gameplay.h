#pragma once
#include <sdk.h>
#include <boost/json.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <string>
#include <random>
#include <model.h>
#include <sstream>
#include <optional>
#include <iomanip>

namespace gameplay {

using namespace std::literals;

class ModelJsonSerializer {
public:
    ModelJsonSerializer(model::Game& game) : game_{game} {}

    std::string SerializeMaps();
    std::optional<std::string> SerializeMap(const std::string& map_name);
private:
    boost::json::array SerializeOffices(const model::Map* map);
    boost::json::array SerializeBuildings(const model::Map* map);
    boost::json::array SerializeRoads(const model::Map* map);

    model::Game& game_;
};

#define PLAYER_INDEX() PlayerIndexer::GetInstance().GetIndex()
class PlayerIndexer {
    PlayerIndexer() = default;
    PlayerIndexer(const PlayerIndexer&) = delete;
public:
    static PlayerIndexer& GetInstance() {
        static PlayerIndexer obj;
        return obj;
    }

    int GetIndex() {
        return index++;
    }

private:
    std::atomic<int> index{0};
};

class Player {
public:
    using Id = util::Tagged<uint64_t, Player>;
    Player(model::GameSession* session, model::Dog* dog) : session_(session), dog_(dog), id_(PLAYER_INDEX()) {}
    const Id& GetId() {
        return id_;
    }
    const model::Map::Id& MapId() {
        return session_->MapId();
    }
    std::string GetName() {
        return dog_->GetName();
    }
    const model::GameSession* GetSession() {
        return session_;
    }

    void Move(const std::string& command);

private:
    Id id_;
    model::GameSession* session_;
    model::Dog* dog_;
};


namespace detail {struct TokenTag {};} 
using Token = util::Tagged<std::string, detail::TokenTag>;


class PlayerTokens {
public:
    Player* FindPlayer(const Token& token);
    Token AddPlayer(Player* player);

private:
    std::unordered_map<Token, Player*, util::TaggedHasher<Token>> token_to_player;
    std::random_device random_device_;

    std::mt19937_64 rnd_generator_{
        [this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()
    };

    std::string GetToken() {
        std::string r1 = ToHex(rnd_generator_());
        std::string r2 = ToHex(rnd_generator_());
        return r1 + r2;
    }

    template<typename T>
    std::string ToHex(T i) {
        std::stringstream stream;
        stream  << std::setfill ('0') << std::setw(sizeof(T)*2) 
                << std::hex << i;
        return stream.str();
    }
};


class Players {
public:
    Player* Add(model::Dog *dog, model::GameSession *session);
    Player* FindPlayer(Player::Id player_id, model::Map::Id map_id);
    const Player::Id* FindPlayerId(std::string player_name, model::Map::Id map_id);
    Player* FindPlayer(Player::Id player_id);
    
private:
    using PlayerIdHasher = util::TaggedHasher<Player::Id>;
    using PlayerIdToIndex = std::unordered_map<Player::Id, size_t, PlayerIdHasher>;
    std::deque<std::unique_ptr<Player>> players_;
    PlayerIdToIndex player_id_to_index_;
};

}
#include <boost/serialization/vector.hpp>

#include "../model.h"
#include "../app.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vector2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, Loot& loot, [[maybe_unused]] const unsigned version) {
    ar& loot.id;
    ar& loot.type;
    ar& loot.position;
    ar& loot.value;
}

}  // namespace model

namespace serialization {

class LootRepresentation {
public:
    LootRepresentation() = default;

    explicit LootRepresentation(const model::Map& map)
        : map_id_(map.GetId())
        , lost_objects_(map.GetLostObjects()) {
    }

    model::Loots Restore() const {
        return lost_objects_;
    }

    model::Map::Id GetMapId() const {return map_id_;}

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *map_id_;
        ar& lost_objects_;
    }

private:
    model::Map::Id map_id_ = model::Map::Id{""};
    model::Loots lost_objects_;
};


class PlayerRepresentation {
public:
    PlayerRepresentation() = default;

    explicit PlayerRepresentation(Token token, const Player& player)
        : token_{*token}
        , id_(player.GetId())
        , name_(player.GetName())
        , map_id_(player.GetMap()->GetId())
        //dog properties
        , last_position_(player.GetDog()->GetLastPosition())
        , position_(player.GetDog()->GetPosition())
        , speed_(player.GetDog()->GetSpeed())
        , direction_(player.GetDog()->GetDirection())
        , bag_(player.GetDog()->GetBag())
        , bag_capacity_(player.GetDog()->GetBagCapacity())
        , score_(player.GetDog()->GetScore()) {
    }

    auto Restore() const -> std::tuple<Token, std::unique_ptr<Player>> {
        auto player = std::make_unique<Player>(id_, name_);
        auto& dog = *player->GetDog();
        dog.SetStartPosition(last_position_);
        dog.SetPosition(position_);
        dog.SetSpeed(speed_);
        dog.SetDirection(direction_);
        dog.SetBagCapacity(bag_capacity_);
        for (const auto& loot : bag_) {
            dog.Loot(loot);
        }
        dog.AddScore(score_);
        return std::make_tuple(Token{token_}, std::move(player));
    }

    model::Map::Id GetMapId() const {return map_id_;}

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& token_;
        ar& id_;
        ar& name_;
        ar& *map_id_;
        ar& last_position_;
        ar& position_;
        ar& speed_;
        ar& direction_;
        ar& bag_;
        ar& bag_capacity_;
        ar& score_;
    }

private:
    std::string token_;
    int id_;
    std::string name_;
    model::Map::Id map_id_ = model::Map::Id{""};
    geom::Point2D last_position_;
    geom::Point2D position_;
    geom::Vector2D speed_;
    char direction_ = 'U';
    model::Loots bag_;
    int bag_capacity_ = 0;
    int score_ = 0;
};


} // namespace serialization

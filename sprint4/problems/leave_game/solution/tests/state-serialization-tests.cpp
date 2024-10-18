#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "model/model.h"
#include "app/app.h"
#include "serialization/representations.h"

using namespace model;
using namespace std::literals;
namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

}  // namespace

SCENARIO_METHOD(Fixture, "Point serialization") {
    GIVEN("A point") {
        const geom::Point2D p{10, 20};
        WHEN("point is serialized") {
            output_archive << p;

            THEN("it is equal to point after serialization") {
                InputArchive input_archive{strm};
                geom::Point2D restored_point;
                input_archive >> restored_point;
                CHECK(p == restored_point);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Player Serialization") {
    GIVEN("A token and a player") {
        Token token{"a805f611a9fb050db88f01f21985d939"};

        model::Map dummy_map{model::Map::Id{"1"}, "dummy"};
        dummy_map.AddRoad({model::Road::HORIZONTAL, {0, 0}, {10}});

        Player player{42, "John"s};
        player.SetDogToMap(&dummy_map);
        auto& dog = *player.GetDog();

        dog.AddScore(42);
        dog.SetStartPosition({1.3, 2.2});
        dog.SetPosition({1.3, 5.9});
        dog.SetNextMove({2.3, -1.2}, 'R');
        dog.SetBagCapacity(2);
        dog.Loot(model::Loot{10, 2u, {1.0, 2.0}, 3});

        WHEN("dog is serialized") {
            {
                serialization::PlayerRepresentation repr{token, player};
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::PlayerRepresentation repr;
                input_archive >> repr;
                const auto [restored_token, restored_player] = repr.Restore();

                CHECK(token == restored_token);

                CHECK(player.GetId() == restored_player->GetId());
                CHECK(player.GetName() == restored_player->GetName());
                CHECK(player.GetDog()->GetLastPosition() == restored_player->GetDog()->GetLastPosition());
                CHECK(player.GetDog()->GetPosition() == restored_player->GetDog()->GetPosition());
                CHECK(player.GetDog()->GetSpeed() == restored_player->GetDog()->GetSpeed());
                CHECK(player.GetDog()->GetBagCapacity() == restored_player->GetDog()->GetBagCapacity());
                CHECK(player.GetDog()->GetBag() == restored_player->GetDog()->GetBag());
            }
        }
    }
}

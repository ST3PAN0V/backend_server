#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "model/model.h"

#include <stdexcept>

TEST_CASE("Model manages maps and objects", "[Model]") {
    model::Game game;
    auto map_id = model::Map::Id{"map1"};

    SECTION("AddMap and FindMap work correctly") {
        model::Map map(map_id, "Test Map");
        game.AddMap(std::move(map));

        const model::Map* found_map = game.FindMap(map_id);
        REQUIRE(found_map != nullptr);
        REQUIRE(found_map->GetName() == "Test Map");
    }

    SECTION("FindMap returns nullptr for non-existing map") {
        const model::Map* found_map = game.FindMap(model::Map::Id{"non_existing_map"});
        REQUIRE(found_map == nullptr);
    }

    SECTION("AddOffice throws exception on duplicate ID") {
        model::Map map(map_id, "Test Map");
        game.AddMap(std::move(map));

        model::Map* found_map = const_cast<model::Map*>(game.FindMap(map_id));
        REQUIRE(found_map != nullptr);

        model::Office office1(model::Office::Id{"office"}, model::Point{0, 0}, model::Offset{10, 10});
        model::Office office2(model::Office::Id{"office"}, model::Point{20, 20}, model::Offset{10, 10});
        
        found_map->AddOffice(office1);
        REQUIRE_THROWS_AS(found_map->AddOffice(office2), std::invalid_argument);
    }
}


TEST_CASE("Dog moves correctly on the map", "[Dog]") {
    model::Dog dog;
    model::Map dummy_map{model::Map::Id{""}, ""};
    dummy_map.AddRoad({model::Road::HORIZONTAL, {0, 0}, {10}});

    SECTION("Default position is at origin") {
        REQUIRE(dog.GetPosition().x == 0.0);
        REQUIRE(dog.GetPosition().y == 0.0);
    }

    SECTION("Setting position works correctly") {
        geom::Point2D new_position(10.0, 15.0);
        dog.SetStartPosition(new_position);
        REQUIRE(dog.GetPosition().x == 10.0);
        REQUIRE(dog.GetPosition().y == 15.0);
    }

    SECTION("Setting speed and moving the dog to the right (R)") {
        using Catch::Matchers::WithinRel;
        using Catch::Matchers::WithinAbs;
        dog.SetStartPosition({0.0, 0.0});
        dog.SetNextMove(2.0, 'R');

        dog.Move(&dummy_map, 1000);

        REQUIRE_THAT(dog.GetPosition().x, WithinRel(2.0, 1e-16));
        REQUIRE_THAT(dog.GetPosition().y, WithinAbs(0.0, 1e-16));
    }

    SECTION("Dog does not move when speed is zero") {
        dog.SetNextMove(0.0, 0);
        dog.Move(&dummy_map, 1000);
        REQUIRE(dog.GetPosition().x == 0.0);
        REQUIRE(dog.GetPosition().y == 0.0);
    }
}

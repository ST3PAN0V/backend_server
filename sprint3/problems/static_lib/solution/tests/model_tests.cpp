#include <catch2/catch_test_macros.hpp>

#include "../src/model.h"

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

    SECTION("Default position is at origin") {
        REQUIRE(dog.GetPosition().x == 0.0);
        REQUIRE(dog.GetPosition().y == 0.0);
    }

    SECTION("Setting position works correctly") {
        model::Vector2D new_position(10.0, 15.0);
        dog.SetPosition(new_position);
        REQUIRE(dog.GetPosition().x == 10.0);
        REQUIRE(dog.GetPosition().y == 15.0);
    }

    SECTION("Setting speed and moving the dog") {
        model::Vector2D speed(2.0, 3.0);
        dog.SetSpeed(2.0);
        dog.SetDirection('R');  // Предполагаем, что 'R' означает движение вправо

        model::Map dummy_map{model::Map::Id{""}, ""};
        dog.Move(&dummy_map, 1000); // Перемещение в течение 1 секунды

        // Используем WithinRel для сравнения с относительной погрешностью
        REQUIRE((dog.GetPosition().x - 2.0) <= 0.01);
        REQUIRE((dog.GetPosition().y - 3.0) <= 0.01);
    }

    SECTION("Dog does not move when speed is zero") {
        dog.SetSpeed(0.0);
        model::Map dummy_map{model::Map::Id{""}, ""};
        dog.Move(&dummy_map, 1000); // Move for 1 second
        REQUIRE(dog.GetPosition().x == 0.0);
        REQUIRE(dog.GetPosition().y == 0.0);
    }
}

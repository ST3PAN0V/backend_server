#include <catch2/catch_test_macros.hpp>

#include "./../headers/game/loot_generator.h"

#include <chrono>

TEST_CASE("LootGenerator generates correct amount of loot", "[LootGenerator]") {
    using namespace loot;
    using namespace std::chrono;

    SECTION("Generate method works with 100% probability and 0 loot on the map") {
        Generator generator(milliseconds(5000), 1.0); // 100% probability every 5 seconds

        unsigned loot_count = 0;
        unsigned looter_count = 10;

        auto generated_loot = generator.Generate(milliseconds(5000), loot_count, looter_count);
        REQUIRE(generated_loot == looter_count); // All looters should receive loot
    }

    SECTION("Generate method works with 50% probability and 0 loot on the map") {
        Generator generator(milliseconds(5000), 0.5); // 50% probability every 5 seconds

        unsigned loot_count = 0;
        unsigned looter_count = 10;

        auto generated_loot = generator.Generate(milliseconds(5000), loot_count, looter_count);
        REQUIRE(generated_loot <= looter_count); // Generated loot should be less than or equal to looters
    }

    SECTION("Generate method generates no loot if looters are less than loot") {
        Generator generator(milliseconds(5000), 1.0); // 100% probability every 5 seconds

        unsigned loot_count = 10;
        unsigned looter_count = 5;

        auto generated_loot = generator.Generate(milliseconds(5000), loot_count, looter_count);
        REQUIRE(generated_loot == 0); // No new loot should be generated
    }

    SECTION("Generate method works with time intervals") {
        Generator generator(milliseconds(5000), 0.5); // 50% probability every 5 seconds

        unsigned loot_count = 0;
        unsigned looter_count = 10;

        // First call with 1 second interval
        auto generated_loot = generator.Generate(milliseconds(1), loot_count, looter_count);
        REQUIRE(generated_loot == 0); // Not enough time has passed

        // Second call with 4 seconds interval
        generated_loot = generator.Generate(milliseconds(4000), loot_count, looter_count);
        REQUIRE(generated_loot <= looter_count); // Total time = 5 seconds, so loot may be generated
    }
}

TEST_CASE("LootGenerator handles edge cases", "[LootGenerator]") {
    using namespace loot;
    using namespace std::chrono;

    SECTION("Generate method works with zero probability") {
        Generator generator(milliseconds(5000), 0.0); // 0% probability

        unsigned loot_count = 0;
        unsigned looter_count = 10;

        auto generated_loot = generator.Generate(milliseconds(5000), loot_count, looter_count);
        REQUIRE(generated_loot == 0); // No loot should be generated
    }

    SECTION("Generate method works with very high loot count") {
        Generator generator(milliseconds(5000), 1.0); // 100% probability

        unsigned loot_count = 1000;
        unsigned looter_count = 10;

        auto generated_loot = generator.Generate(milliseconds(5000), loot_count, looter_count);
        REQUIRE(generated_loot == 0); // No new loot should be generated as loot count exceeds looter count
    }
}

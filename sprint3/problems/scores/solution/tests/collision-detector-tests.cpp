#define _USE_MATH_DEFINES

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "./../headers/game/collision_detector.h"

#include <sstream>
#include <string>

using namespace collision_detector;

namespace Catch {
template<>
struct StringMaker<collision_detector::GatheringEvent> {
    static std::string convert(collision_detector::GatheringEvent const& value) {
        std::ostringstream tmp;
        tmp << "(" << value.gatherer_id << "," << value.item_id << "," 
            << value.sq_distance << "," << value.time << ")";
        return tmp.str();
    }
};
}  // namespace Catch

class TestItemGathererProvider : public collision_detector::ItemGathererProvider {
public:
    std::vector<Item> items;
    std::vector<Gatherer> gatherers;

    size_t ItemsCount() const override {
        return items.size();
    }

    Item GetItem(size_t idx) const override {
        return items[idx];
    }

    size_t GatherersCount() const override {
        return gatherers.size();
    }

    Gatherer GetGatherer(size_t idx) const override {
        return gatherers[idx];
    }

    size_t GetId() const override {
        return 0;
    }
};

TEST_CASE("FindGatherEvents detects all collision events", "[FindGatherEvents]") {
    TestItemGathererProvider provider;

    provider.items = {
        {{0.0, 0.0}, 1.0}, // Item 0
        {{2.0, 2.0}, 1.0}, // Item 1
    };

    provider.gatherers = {
        {{-1.0, 0.0}, {3.0, 0.0}, 1.0}, // Gatherer 0
        {{2.0, 1.0}, {2.0, 3.0}, 1.0},  // Gatherer 1
    };

    std::vector<GatheringEvent> events = collision_detector::FindGatherEvents(provider);

    REQUIRE(events.size() == 3);

    REQUIRE(events[0].gatherer_id == 0);
    REQUIRE(events[0].item_id == 0);

    REQUIRE(events[1].gatherer_id == 1);
    REQUIRE(events[1].item_id == 1);
}

TEST_CASE("FindGatherEvents does not detect false events", "[FindGatherEvents]") {
    TestItemGathererProvider provider;

    provider.items = {
        {{10.0, 10.0}, 1.0}, // Item 0
        {{20.0, 20.0}, 1.0}, // Item 1
    };

    provider.gatherers = {
        {{0.0, 0.0}, {5.0, 5.0}, 1.0}, // Gatherer 0
    };

    std::vector<GatheringEvent> events = collision_detector::FindGatherEvents(provider);

    REQUIRE(events.empty());
}

TEST_CASE("FindGatherEvents detects events in chronological order", "[FindGatherEvents]") {
    TestItemGathererProvider provider;

    provider.items = {
        {{1.0, 1.0}, 1.0}, // Item 0
        {{3.0, 3.0}, 1.0}, // Item 1
    };

    provider.gatherers = {
        {{0.0, 0.0}, {4.0, 4.0}, 1.0}, // Gatherer 0
    };

    std::vector<GatheringEvent> events = collision_detector::FindGatherEvents(provider);

    REQUIRE(events.size() == 2);
    REQUIRE(events[0].time <= events[1].time);
}

TEST_CASE("FindGatherEvents detects correct event data", "[FindGatherEvents]") {
    TestItemGathererProvider provider;

    provider.items = {
        {{1.0, 1.0}, 0.5}, // Item 0
    };

    provider.gatherers = {
        {{0.0, 0.0}, {2.0, 2.0}, 0.5}, // Gatherer 0
    };

    std::vector<GatheringEvent> events = collision_detector::FindGatherEvents(provider);

    REQUIRE(events.size() == 1);

    REQUIRE(events[0].item_id == 0);
    REQUIRE(events[0].gatherer_id == 0);
    REQUIRE_THAT(events[0].sq_distance, Catch::Matchers::WithinAbs(0.0, 1e-10)); // Поскольку прямое столкновение
    REQUIRE_THAT(events[0].time, Catch::Matchers::WithinRel(0.5, 1e-10)); // Половина времени движения
}

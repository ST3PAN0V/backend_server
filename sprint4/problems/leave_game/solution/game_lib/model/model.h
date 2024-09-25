#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

#include "geom.h"
#include "tagged.h"
#include "collision_detector.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

struct Loot {
    int id;
    int type;
    geom::Point2D position;
    int value;
    bool operator==(const Loot& other) const {
        return (id == other.id
            && type == other.type
            && position == other.position
            && value == other.value);
    }
};
using Loots = std::vector<Loot>;

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

    Point GetMin() const noexcept {
        if (IsHorizontal()) {
            return start_.x < end_.x ? start_ : end_;
        }
        return start_.y < end_.y ? start_ : end_;
    }

    Point GetMax() const noexcept {
        if (IsHorizontal()) {
            return start_.x > end_.x ? start_ : end_;
        }
        return start_.y > end_.y ? start_ : end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {return id_;}
    const std::string& GetName() const noexcept {return name_;}
    const Buildings& GetBuildings() const noexcept {return buildings_;}
    const Roads& GetRoads() const noexcept {return roads_;}
    const Offices& GetOffices() const noexcept {return offices_;}

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    void SetDogSpeed(double speed) {dog_speed_ = speed;}
    double GetDogSpeed() const noexcept {return dog_speed_;}

    void SetBagCapacity(int capacity) {bag_capacity_ = capacity;}
    int GetBagCapacity() const noexcept {return bag_capacity_;}

    void SetDogRetirementTime(int time_s) {dog_retirement_time_s = time_s;}
    double GetDogRetirementTime() const noexcept {return dog_retirement_time_s;}

    geom::Point2D GetRandomPoint() const;
    geom::Point2D GetInitialPoint() const;

    void AddLostObject(const std::vector<int>& values);
    void AddLostObjects(Loots&& loots);
    Loots& GetLostObjects() {return loots_;}
    const Loots& GetLostObjects() const noexcept {return loots_;}
    std::optional<Loot> TakeLootAtPosition(const geom::Point2D& position);
    bool IsOfficeAtPosition(const geom::Point2D& position) const;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    double dog_speed_ = 1.0;
    int bag_capacity_ = 3;
    double dog_retirement_time_s = 60.0;

    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    Loots loots_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    Maps& GetMaps() noexcept {
        return maps_;
    }

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
};

class Dog {
public:
    Dog();

    Dog(const Dog&) = delete;
    Dog& operator=(const Dog&) = delete;

    geom::Point2D GetPosition() const {return position_;}
    geom::Point2D GetLastPosition() const {return last_position_;}
    geom::Vector2D GetSpeed() const {return speed_;}
    char GetDirection() const {return direction_;}
    Loots GetBag() const {return bag_;}
    int GetBagCapacity() const {return bag_capacity_;}
    int GetScore() const {return score_;}
    double GetPlayTime() const {return static_cast<double>(full_time_);}
    double GetPauseTime() const {return static_cast<double>(pause_time_);}
    bool IsRetired() const {return retired_;}

    void SetStartPosition(const geom::Point2D& position);
    void SetPosition(const geom::Point2D& position);
    void SetBagCapacity(int capacity) {bag_capacity_ = capacity;}

    void Loot(const Loot& loot);
    void ReturnLoot();

    void SetNextMove(double speed, char direction);
    void SetNextMove(const geom::Vector2D& speed, char direction);
    void Move(const Map* map, int64_t time_ms);

    void AddScore(int score);

private:
    geom::Vector2D speed_multiplier() const;
    void SetSpeed(double speed);
    void SetDirection(char direction);

    geom::Point2D last_position_;
    geom::Point2D position_;
    geom::Vector2D speed_;
    char direction_;
    Loots bag_;
    int bag_capacity_ = 3;
    int score_ = 0;
    int64_t pause_time_ = 0;
    int64_t full_time_ = 0;
    bool retired_ = false;
};

}  // namespace model

#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

#include "tagged.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Vector2D {
    double x;
    double y;

    Vector2D() : x(0.0), y(0.0) {}
    Vector2D(double x, double y) : x(x), y(y) {}
};

Vector2D operator*(const Vector2D& vec, double num);
Vector2D operator+(const Vector2D& vec1, const Vector2D& vec2);

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
    Vector2D position;
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

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    void SetDogSpeed(double speed) {dog_speed_ = speed;}
    double GetDogSpeed() const {return dog_speed_;}

    Vector2D GetRandomPoint() const;
    Vector2D GetInitialPoint() const;

    void AddLostObject(int types_count);
    Loots GetLostObjects() const {return loots_;}

private:
    int GetRandomType(int types_count) const;

    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    double dog_speed_ = 1.0;

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

    Vector2D GetPosition() const {return position_;}
    Vector2D GetSpeed() const {return speed_;}
    char GetDirection() const {return direction_;}

    void SetPosition(const Vector2D& position);
    void SetSpeed(double speed);
    void SetDirection(char direction);
    void Move(const Map* map, int64_t time_ms);

private:
    Vector2D speed_multiplier() const;

    Vector2D position_;
    Vector2D speed_;
    char direction_;
};

}  // namespace model

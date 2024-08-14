#pragma once

#include "tagged.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <atomic>
#include <map>
#include <memory>

namespace model {

class GameSession;

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

private:
    Point start_;
    Point end_;
};

class Building {
public:
    Building(Rectangle bounds) noexcept
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

    void AddOffice(const Office& office);

    void SetDogSpeed(double dog_speed) {
        dog_speed_ = dog_speed;
    }

    double GetDogSpeed() const {
        return dog_speed_;
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    double dog_speed_;


    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(const Map& map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    void SetDefaultDogSpeed(double speed) {
        DefaultDogSpeed = speed;
    }

    const Map* FindMap(const Map::Id& id) const noexcept;

    GameSession* FindGameSession(const Map::Id& id) noexcept;

    GameSession* AddGameSession(const Map::Id& id);

    void Tick(uint64_t time_delta);

    void SetRandomizeSpawnPoints(bool randomize_spawn_points) {
        randomize_spawn_points_ = randomize_spawn_points;
    }

    bool IsRandomizeSpawnPoints() {
        return randomize_spawn_points_;
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
     
    Maps maps_;
    double DefaultDogSpeed {0};
    std::deque<GameSession> game_sessions_;
    MapIdToIndex map_id_to_index_;
    MapIdToIndex map_id_to_game_sessions_index_;
    bool randomize_spawn_points_ {false};
};


class DogCoordinate {
public:
    double x{0};
    double y{0};

    bool operator==(const DogCoordinate& dog) const {
        if (x == dog.x && y == dog.y) {
            return true;
        }
        else {
            return false;
        }
    }

    bool operator!=(const DogCoordinate& dog) const {
        if (x != dog.x || y != dog.y) {
            return true;
        }
        else {
            return false;
        }        
    }
};

class DogSpeed {
public:
    double x{0};
    double y{0};

    bool operator==(const DogSpeed& dog_speed) const {
        if (x == dog_speed.x && y == dog_speed.y) {
            return true;
        }
        else {
            return false;
        }
    }
};

enum DOG_DIRECTION {
    NORTH,
    SOUTH,
    WEST,
    EAST
};

enum DOG_MOVE {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    STAND
};

#define DOG_INDEX() DogIndexer::GetInstance().GetIndex()
class DogIndexer {
    DogIndexer() = default;
    DogIndexer(const DogIndexer&) = delete;
public:
    static DogIndexer& GetInstance() {
        static DogIndexer obj;
        return obj;
    }

    int GetIndex() {
        return index++;
    }

private:
    std::atomic<int> index{0};
};

class Dog {
public:
    using Id = util::Tagged<size_t, Dog>;
    Dog(
        const std::string& dog_name,
        const DogCoordinate& dog_coordinate
    ) : 
        dog_name_(dog_name), 
        id_(Id{ DOG_INDEX() }),  
        dog_coordinate_(dog_coordinate) {}
    
    const Id& GetId() {
        return id_;
    }

    std::string GetName() {
        return dog_name_;
    }

    const DogCoordinate& GetCoordinate() {
        return dog_coordinate_;
    }

    void SetCoordinate(const DogCoordinate& coordinate) {
        dog_coordinate_.x = coordinate.x;
        dog_coordinate_.y = coordinate.y;
    }

    const DogSpeed& GetSpeed() {
        return dog_speed_;
    }

    std::string GetDirection();

    void SetDirection(DOG_DIRECTION dog_direction) {
        dog_direction_ = dog_direction;
    }

    void Direction(DOG_MOVE dog_move, double speed);

    DogCoordinate GetEndCoordinate(uint64_t move_time_ms);

    bool IsStanding();

    void StopMove() {
        dog_speed_.x = 0;
        dog_speed_.y = 0;
    }

    Dog(const Dog& dog) : 
        id_(dog.id_), 
        dog_name_(dog.dog_name_), 
        dog_coordinate_(dog.dog_coordinate_),
        dog_speed_(dog.dog_speed_),
        dog_direction_(dog.dog_direction_) {}

    Dog(Dog&& dog) : 
        id_(std::move(dog.id_)),
        dog_name_(std::move(dog.dog_name_)),
        dog_coordinate_(std::move(dog.dog_coordinate_)),
        dog_speed_(std::move(dog.dog_speed_)),
        dog_direction_(std::move(dog.dog_direction_)) {}

private:
    Id id_ = Id{0};
    std::string dog_name_;
    DogCoordinate dog_coordinate_;
    DogSpeed dog_speed_{0, 0};
    DOG_DIRECTION dog_direction_ {DOG_DIRECTION::NORTH};
};


class KeyHash {
public:
    template<typename T>
    size_t operator()(const T& k) const {
        return std::hash<decltype(k.x)>()(k.x) ^ (std::hash<decltype(k.y)>()(k.y) << 1);
    }
};

class KeyEqual {
public:
    template<typename T>
    bool operator()(const T& lhs, const T& rhs) const {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }
};

class GameSession {
    using RoadMap = std::unordered_multimap<Point, const Road*, KeyHash, KeyEqual>;
    using RoadMapIter = decltype(RoadMap{}.equal_range(Point{}));
public:
    using Dogs = std::deque<Dog>;

    GameSession(const Map* map, bool randomize_spawn_points) : 
        map_(map),
        randomize_spawn_points_ (randomize_spawn_points) {
            LoadRoadMap();
    }

    const Map::Id& MapId() {
        return map_->GetId();
    } 

    Dog* FindDog(const std::string& nick_name);

    Dog* AddDog(const std::string& nick_name);

    DogCoordinate GetRandomRoadCoordinate();

    const Dogs& GetDogs() const {
        return const_cast<Dogs&>(dogs_);
    }

    void MoveDog(const Dog::Id& dog_id, const DOG_MOVE& dog_move);

    void Tick(uint64_t time_delta);

    DogCoordinate MoveDog(const DogCoordinate& start_coordinate, const DogCoordinate& end_coordinate);

    bool IsCoordinateOnRoad(const RoadMapIter& roads, const DogCoordinate& coordinate);

    DogCoordinate FindBorderCoordinate(const RoadMapIter& roads, const DogCoordinate& end_coordinate);

    void LoadRoadMap();

private:
    using DogsIdHasher = util::TaggedHasher<Dog::Id>;
    using DogsIdToIndex = std::unordered_map<Dog::Id, size_t, DogsIdHasher>;
    Dogs dogs_;
    DogsIdToIndex dogs_id_to_index_;
    const Map* map_;
    RoadMap road_map_;
    bool randomize_spawn_points_;
};

}  // namespace model

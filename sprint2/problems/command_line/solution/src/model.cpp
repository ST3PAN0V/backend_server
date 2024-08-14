#include "model.h"

#include <random>
#include <stdexcept>

namespace model {
using namespace std::literals;

void Map::AddOffice(const Office &office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }
    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(const Map& map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id " + *map.GetId() + " already exists");
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

GameSession* Game::FindGameSession(const Map::Id& id) noexcept {
    if (auto it = map_id_to_game_sessions_index_.find(id);
        it != map_id_to_game_sessions_index_.end()) {
        return &game_sessions_.at(it->second);
    }
    return nullptr;
}

GameSession* Game::AddGameSession(const Map::Id& id) {
    GameSession gs{ FindMap(id), IsRandomizeSpawnPoints()};
    auto index = game_sessions_.size();
    game_sessions_.emplace_back(std::move(gs));
    try {
        map_id_to_game_sessions_index_.emplace(id, index);
    }
    catch (...) {
        game_sessions_.pop_back();
        throw;
    }
    return &game_sessions_.back();
}

void Game::Tick(uint64_t time_delta) {
    for (auto& game_session : game_sessions_) {
        game_session.Tick(time_delta);
    }
}

void GameSession::Tick(uint64_t time_delta) {
    for (auto& dog : dogs_) {
        if (dog.IsStanding()) {
            continue;
        }

        auto start_pos = dog.GetCoordinate();
        auto end_pos = dog.GetEndCoordinate(time_delta);
        auto move_pos = MoveDog(start_pos, end_pos);
        dog.SetCoordinate(move_pos);

        // if the dog is on the border, it is necessery to stop him
        if (move_pos != end_pos) 
            dog.StopMove();
    }
}

bool Dog::IsStanding() {
    return dog_speed_ == DogSpeed{0,0};
}

DogCoordinate Dog::GetEndCoordinate(uint64_t move_time) {
    if (IsStanding()) {
        return dog_coordinate_;
    }

    double move_time_sec = double(move_time) / 1000.0;
    return DogCoordinate {
        dog_coordinate_.x + dog_speed_.x * move_time_sec,
        dog_coordinate_.y + dog_speed_.y * move_time_sec
    };
}

Dog* GameSession::AddDog(const std::string& dog_name)
{
    if (FindDog(dog_name) != nullptr) {
        throw std::invalid_argument("Dog with name <" + dog_name + "> already exists!");
    }
    auto dog = randomize_spawn_points_ ? Dog(dog_name, GetRandomRoadCoordinate()) : Dog(dog_name, DogCoordinate{.x = 0.0, .y = 0.0});
    auto index = dogs_.size();
    auto o = dogs_.emplace_back(std::move(dog));
    try {
        dogs_id_to_index_.emplace(o.GetId(), index);
    }
    catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        dogs_.pop_back();
        throw;
    }
    return &dogs_.back();
    //return nullptr;
}

Dog* GameSession::FindDog(const std::string& dog_name)
{
    for (auto& dog : dogs_) {
        if (dog.GetName() == dog_name) {
            return &dog;
        }
    }
    return nullptr;
}

DogCoordinate GameSession::GetRandomRoadCoordinate()
{
    std::random_device random_device_;
    auto random_coordinate = [&](auto x, auto y) {
        if (y < x) {
            std::swap(x, y);
        }
        std::uniform_real_distribution<double> dist(x, y);
        return dist(random_device_); 
    };

    auto roads = map_->GetRoads();

    if (roads.size() == 0) {
        return DogCoordinate();
    }

    // choose random road
    std::uniform_int_distribution<size_t> dist{0, roads.size()-1};
    auto road = roads[dist(random_device_)];

    DogCoordinate coordinate;

    if (road.IsHorizontal()) {
        auto start_point = road.GetStart();
        auto end_point = road.GetEnd();
        coordinate.x = random_coordinate(start_point.x, end_point.x);
        coordinate.y = end_point.y;
    }
    else {
        auto start_point = road.GetStart();
        auto end_point = road.GetEnd();
        coordinate.y = random_coordinate(start_point.y, end_point.y);
        coordinate.x = end_point.x;
    }
    return coordinate;
}

void GameSession::MoveDog(const Dog::Id& dog_id, const DOG_MOVE& dog_move) {
    auto& dog = dogs_[dogs_id_to_index_[dog_id]];
    dog.Direction(dog_move, map_->GetDogSpeed());
}

bool GameSession::IsCoordinateOnRoad(const RoadMapIter& roads, const DogCoordinate& coordinate) {
    for (auto it = roads.first; it != roads.second; ++it) {
        auto road = it->second;
        
        // road border coordinates
        auto road_start = road->GetStart();
        auto road_end = road->GetEnd();
        auto x0 = std::min(road_start.x, road_end.x) - 0.4;
        auto x1 = std::max(road_start.x, road_end.x) + 0.4;
        auto y0 = std::min(road_start.y, road_end.y) - 0.4;
        auto y1 = std::max(road_start.y, road_end.y) + 0.4;

        if (
            coordinate.x >= x0 && 
            coordinate.x <= x1 && 
            coordinate.y >= y0 && 
            coordinate.y <= y1
        ) {
            return true;
        }
    }
    return false;
}

DogCoordinate GameSession::MoveDog(const DogCoordinate& start_coordinate, const DogCoordinate& end_coordinate) {
    auto road_point_round = [](auto pos) {
        return Point { 
            .x = static_cast<Coord>(std::round(pos.x)),
            .y = static_cast<Coord>(std::round(pos.y))
        };
    };

    // start_coordinate and end_coordinate are same
    if (start_coordinate == end_coordinate) {
        return start_coordinate;
    }

    // round start_coordinate for road Point format
    auto start_pos_round = road_point_round(start_coordinate);

    // select roads which contain start_pos_round
    auto selected_start_pos_roads = road_map_.equal_range(start_pos_round);

    // check that end_coordinate in on one of selected_start_pos_roads (on road)
    if (IsCoordinateOnRoad(selected_start_pos_roads, end_coordinate)) {
        return end_coordinate;
    }

    // dog is on road border
    // find border coordinate
    return FindBorderCoordinate(selected_start_pos_roads, end_coordinate);
}

DogCoordinate GameSession::FindBorderCoordinate(const RoadMapIter& roads, const DogCoordinate& end_coordinate) {
    double minimum_distance = std::numeric_limits<double>::max();
    double distance;
    DogCoordinate border_coordinate;
    for (auto it = roads.first; it != roads.second; ++it) {
        // get road
        auto road = it->second;
        
        // calculate road border coordinates
        auto road_start = road->GetStart();
        auto road_end = road->GetEnd();
        auto x0 = std::min(road_start.x, road_end.x) - 0.4;
        auto x1 = std::max(road_start.x, road_end.x) + 0.4;
        auto y0 = std::min(road_start.y, road_end.y) - 0.4;
        auto y1 = std::max(road_start.y, road_end.y) + 0.4;

        // find border_coordinate with minimum distance to end_coordinate from roads which contain start coordinate
        if (end_coordinate.x >= x0 && end_coordinate.x <= x1) {
            if (end_coordinate.y <= y0) {
                if ((distance = std::abs(y0 - end_coordinate.y)) < minimum_distance) {
                    minimum_distance = distance;
                    border_coordinate.y = y0;
                    border_coordinate.x = end_coordinate.x;
                    continue;
                }
            }
            if (end_coordinate.y >= y1) {
                if ((distance = std::abs(y1 - end_coordinate.y)) < minimum_distance) {
                    minimum_distance = distance;
                    border_coordinate.y = y1;
                    border_coordinate.x = end_coordinate.x;
                    continue;
                }
            }
        }
        if (end_coordinate.y >= y0 && end_coordinate.y <= y1) {
            if (end_coordinate.x <= x0) {
                if ((distance = std::abs(x0 - end_coordinate.x)) < minimum_distance) {
                    minimum_distance = distance;
                    border_coordinate.x = x0;
                    border_coordinate.y = end_coordinate.y;
                    continue;
                }   
            }
            if (end_coordinate.x >= x1) {
                if ((distance = std::abs(x1 - end_coordinate.x)) < minimum_distance) {
                    minimum_distance = distance;
                    border_coordinate.x = x1;
                    border_coordinate.y = end_coordinate.y;
                    continue;
                }
            }
        }
    }
    return border_coordinate;
}

void GameSession::LoadRoadMap() {
    auto& roads = map_->GetRoads();
    for (const auto& road : roads) {
        auto start = road.GetStart();
        auto end = road.GetEnd();
        if (road.IsHorizontal()) {
            if (start.x > end.x) {
                std::swap(start, end);
            }
            for (auto x = start.x; x <= end.x; x++) {
                road_map_.emplace(Point{.x = x, .y = start.y}, &road);
            }
        }
        if (road.IsVertical()) {
            if (start.y > end.y) {
                std::swap(start, end);
            }
            for (auto y = start.y; y <= end.y; y++) {
                road_map_.emplace(Point{ .x = start.x, .y = y }, &road);
            }
        }
    }
}

std::string Dog::GetDirection()
{
    switch (dog_direction_) {
        case DOG_DIRECTION::NORTH:
        {
            return "U";
        }
        case DOG_DIRECTION::EAST:
        {
            return "R";
        }
        case DOG_DIRECTION::SOUTH:
        {
            return "D";
        }
        case DOG_DIRECTION::WEST:
        {
            return "L";
        }
    }
}

void Dog::Direction(DOG_MOVE dog_move, double speed) {
    switch (dog_move) {
        case DOG_MOVE::LEFT:
        {
            dog_speed_ = { -1 * speed, 0 };
            SetDirection(DOG_DIRECTION::WEST);
            break;
        }
        case DOG_MOVE::RIGHT:
        {
            dog_speed_ = { speed, 0 };
            SetDirection(DOG_DIRECTION::EAST);
            break;
        }
        case DOG_MOVE::UP:
        {
            dog_speed_ = { 0, -1 * speed };
            SetDirection(DOG_DIRECTION::NORTH);
            break;
        }
        case DOG_MOVE::DOWN:
        {
            dog_speed_ = { 0, speed };
            SetDirection(DOG_DIRECTION::SOUTH);
            break;
        }
        case DOG_MOVE::STAND:
        {
            dog_speed_ = { 0, 0 };
            break;
        }
    }
}

}  // namespace model

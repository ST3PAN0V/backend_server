#include "model.h"

#include <stdexcept>
#include <cmath>
#include <iostream>
#include <random>

namespace model {
using namespace std::literals;
using namespace geom;

const double delta = 1e-6;
const double span = 0.4;

void Map::AddOffice(Office office) {
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

Point2D Map::GetRandomPoint() const {
    std::random_device random_device;
    std::mt19937_64 generator(random_device());

    if (roads_.empty()) {
        throw std::runtime_error("No roads available on the map.");
    }

    // Выбираем случайную дорогу
    std::uniform_int_distribution<size_t> road_distribution(0, roads_.size() - 1);
    const model::Road& selected_road = roads_[road_distribution(generator)];

    // Генерируем случайную позицию на выбранной дороге
    Point2D position;
    if (selected_road.IsHorizontal()) {
        std::uniform_real_distribution<double> x_distribution(
            selected_road.GetStart().x, selected_road.GetEnd().x);
        double x_random = x_distribution(generator);
        position = {x_random, static_cast<double>(selected_road.GetStart().y)};
    } else if (selected_road.IsVertical()) {
        std::uniform_real_distribution<double> y_distribution(
            selected_road.GetStart().y, selected_road.GetEnd().y);
        double y_random = y_distribution(generator);
        position = {static_cast<double>(selected_road.GetStart().x), y_random};
    }

    return position;
}

Point2D Map::GetInitialPoint() const {
    if (roads_.empty()) {
        throw std::runtime_error("No roads available on the map.");
    }

    const model::Road& road = roads_.front();
    return {static_cast<double>(road.GetStart().x), static_cast<double>(road.GetStart().y)};
}

int Map::GetRandomType(int types_count) const {
    if (types_count <= 0) {
        throw std::invalid_argument("types_count must be a positive integer.");
    }

    // Генерируем случайное число в диапазоне от 0 до types_count-1
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<int> distribution(0, types_count - 1);

    return distribution(generator);
}

void Map::AddLostObject(int types_count) {
    static int lost_object_id = 0;
    loots_.emplace_back(lost_object_id++, GetRandomType(types_count), GetRandomPoint());
}

std::optional<Loot> Map::TakeLootAtPosition(const geom::Point2D& position) {
    auto it = std::find_if(loots_.begin(), loots_.end(), [&position](const Loot& loot) {
        return loot.position == position;
    });

    if (it != loots_.end()) {
        Loot found_loot = *it;
        loots_.erase(it);
        return found_loot;
    }

    return std::nullopt;
}

bool Map::IsOfficeAtPosition(const geom::Point2D& position) const {
    // Ищем офис в заданной позиции
    auto it = std::find_if(offices_.begin(), offices_.end(), [&position](const Office& office) {
        return office.GetPosition().x == static_cast<int>(position.x)
            && office.GetPosition().y == static_cast<int>(position.y);
    });

    // Если офис найден, возвращаем true, иначе false
    return it != offices_.end();
}



void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

Dog::Dog()
    : position_(0.0, 0.0), speed_(0.0, 0.0), direction_('U') {}

void Dog::SetStartPosition(const Point2D& position) {
    SetPosition(position);
    last_position_ = position;
}

void Dog::SetPosition(const Point2D& position) {
    position_ = position;
}

void Dog::SetSpeed(double speed) {
    speed_ = speed ? speed_multiplier() * speed : Vector2D{0.0, 0.0};
}

void Dog::SetDirection(char direction) {
    direction_ = direction;
}

void Dog::Loot(const model::Loot& loot) {
    if (bag_.size() < bag_capacity_) {
        bag_.push_back(loot);
    }
}

void Dog::ReturnLoot() {
    //TODO: очки за лут
    bag_.clear();
}

void Dog::Move(const Map* map, int64_t time_ms) {
    last_position_ = position_;

    const Road* best_road = nullptr;
    const double target_distance = std::fabs(speed_.x + speed_.y) * time_ms/1000;

    double max_distance = 0.0;
    for (const auto& road : map->GetRoads()) {
        double distance = 0.0;
        //сперва проверим, что координата пренадлежит дороге
        if (road.IsHorizontal() 
            && (position_.x >= road.GetMin().x - span - delta && position_.x <= road.GetMax().x + span + delta)
            && std::fabs(position_.y - road.GetMin().y) <= span + delta
            || road.IsVertical() 
            && (position_.y >= road.GetMin().y - span - delta && position_.y <= road.GetMax().y + span + delta)
            && std::fabs(position_.x - road.GetMin().x) <= span + delta) {

            switch (direction_) {
            case 'L':
                distance = std::fabs(road.GetMin().x - span - position_.x);
                break;
            case 'R':
                distance = std::fabs(road.GetMax().x + span - position_.x);
                break;
            case 'U':
                distance = std::fabs(road.GetMin().y - span - position_.y);
                break;
            case 'D':
                distance = std::fabs(road.GetMax().y + span - position_.y);
                break;
            }
        }

        //если дистанция позволяет, перемещаемся по текущей дороге
        if (distance >= target_distance) {
            SetPosition(position_ + (speed_multiplier() * target_distance));
            return;            
        }

        // Если места недостаточно, то запоминаем расстояние как максимально возможное
        if (distance > max_distance) {
            max_distance = distance;
        }
    }

    //идем на максимально возможное расстояние и останавливаемся
    SetPosition(position_ + (speed_multiplier() * max_distance));
    SetSpeed(0.0);
}

Vector2D Dog::speed_multiplier() const {
    switch(direction_) {
    case 'L':
        return {-1.0, 0.0};
        break;
    case 'R':
        return {1.0, 0.0};
        break;
    case 'U':
        return {0.0, -1.0};
        break;
    case 'D':
        return {0.0, 1.0};
        break;
    }
    return {0.0, 0.0};
}


}  // namespace model

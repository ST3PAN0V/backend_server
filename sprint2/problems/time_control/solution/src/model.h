#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstddef>

#include "tagged.h"


#include "gamesession.h"

namespace model 
{

    using Dimension = int;
    using Coord = Dimension;
    
    struct Point 
    {
        Coord x, y;
    };
    
    struct Size 
    {
        Dimension width, height;
    };
    
    struct Rectangle 
    {
        Point position;
        Size size;
    };
    
    struct Offset
    {
        Dimension dx, dy;
    };
    
    class Road 
    {
            struct HorizontalTag 
            {
                explicit HorizontalTag() = default;
            };
        
            struct VerticalTag 
            {
                explicit VerticalTag() = default;
            };
        
        public:
            constexpr static HorizontalTag HORIZONTAL{};
            constexpr static VerticalTag VERTICAL{};
        
            Road(HorizontalTag, Point start, Coord end_x) noexcept
                : start_{start}
                , end_{end_x, start.y} 
            {
                init();
            }
        
            Road(VerticalTag, Point start, Coord end_y) noexcept
                : start_{start}
                , end_{start.x, end_y} 
            {
                init();
            }
        
            bool IsHorizontal() const noexcept 
            {
                return start_.y == end_.y;
            }
        
            bool IsVertical() const noexcept 
            {
                return start_.x == end_.x;
            }
        
            Point GetStart() const noexcept 
            {
                return start_;
            }
        
            Point GetEnd() const noexcept 
            {
                return end_;
            }
        
            bool contain(const Position p) const;
            
            Position calculatePositionOnBorder(const Position p) const;
            
            Position lbPos() const;
            Position rtPos() const;
            
        protected:
            Point start_;
            Point end_;
            double width_ = 0.4;
            //bounded rect
            Position left_bottom_;
            Position right_top_;
            
        protected:
            void init() noexcept;
    };
    
    class Building 
    {
        public:
            explicit Building(Rectangle bounds) noexcept
                : bounds_{bounds}
                {
            }
        
            const Rectangle& GetBounds() const noexcept
            {
                return bounds_;
            }
        
        private:
            Rectangle bounds_;
    };
    
    class Office 
    {
        public:
            using Id = util::Tagged<std::string, Office>;
        
            Office(Id id, Point position, Offset offset) noexcept
                : id_{std::move(id)}
                , position_{position}
                , offset_{offset} 
            {
            }
        
            const Id& GetId() const noexcept 
            {
                return id_;
            }
        
            Point GetPosition() const noexcept
            {
                return position_;
            }
        
            Offset GetOffset() const noexcept 
            {
                return offset_;
            }
        
        private:
            Id id_;
            Point position_;
            Offset offset_;
    };
    
    class Map 
    {
        public:
            using Id = util::Tagged<std::string, Map>;
            using Roads = std::vector<Road>;
            using Buildings = std::vector<Building>;
            using Offices = std::vector<Office>;
        
            Map(Id id, std::string name) noexcept
                : id_(std::move(id))
                , name_(std::move(name)) 
                , dogSpeed_{1.0}
            {
            }
        
            const Id& GetId() const noexcept 
            {
                return id_;
            }
        
            const std::string& GetName() const noexcept 
            {
                return name_;
            }
        
            const Buildings& GetBuildings() const noexcept 
            {
                return buildings_;
            }
        
            const Roads& GetRoads() const noexcept 
            {
                return roads_;
            }
        
            const Offices& GetOffices() const noexcept 
            {
                return offices_;
            }
        
            void AddRoad(const Road& road) 
            {
                roads_.emplace_back(road);
            }
        
            void AddBuilding(const Building& building)
            {
                buildings_.emplace_back(building);
            }
        
            void AddOffice(Office office);
        
            void setDogSpeed(const double speed);
            
            double dogSpeed() const;
            
        private:
            using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
        
            Id id_;
            std::string name_;
            Roads roads_;
            Buildings buildings_;
        
            OfficeIdToIndex warehouse_id_to_index_;
            Offices offices_;
            
            double dogSpeed_;
    };
        
    class Game 
    {
        public:
            using Maps = std::vector<Map>;
        
            void AddMap(Map map);
        
            const Maps& GetMaps() const noexcept;
        
            const Map* FindMap(const Map::Id& id) const noexcept;
        
            GameSession* FindSession(const Map::Id& id);
            
            void allTick(const int64_t delta);
            
        private:
            using MapIdHasher = util::TaggedHasher<Map::Id>;
            using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
        
            std::vector<Map> maps_;
            MapIdToIndex map_id_to_index_;
            
            std::unordered_map<std::size_t, GameSession> sessions_;
    };

}  // namespace model

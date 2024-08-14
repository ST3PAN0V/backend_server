#pragma once

#include <string>

namespace model 
{
    enum class Direction
    {
        NORTH, SOUTH, WEST, EAST
    };
    
    struct Position
    {
            double x;
            double y;
            
            bool operator==(const Position& other) const;
            
            bool operator!=(const Position& other) const;
    };
    
    struct Speed
    {
            double v_x;
            double v_y;
    };
    
    class Dog
    {
        public:
            Dog() = default;
            explicit Dog(const std::string& name);
            
            void setName(const std::string& name);
            const std::string& name() const;
            
            Position position() const;
            void setPosition(const double x, 
                             const double y);
            void setPosition(const Position p);
            
            Speed speed() const;
            Direction dir() const;
            
            void setSpeed(const double v_x, 
                          const double v_y);
            void setDir(Direction d);
            
        protected:
            std::string name_;
            
            Position m_pos;
            Speed m_speed;
            Direction m_dir;
            
    };
}



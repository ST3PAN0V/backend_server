#include "dog.h"


namespace model 
{
    
    Dog::Dog(const std::string& name):
        name_{name}
      , m_pos{0.0, 0.0}
      , m_speed{0.0, 0.0}
      , m_dir{Direction::NORTH}
    {        
    }
    
    void Dog::setName(const std::string& name)
    {
        name_ = name;
    }
    
    const std::string& Dog::name() const
    {
        return name_;
    }
    
    model::Position model::Dog::position() const
    {
        return m_pos;
    }
    
    void Dog::setPosition(const double x, const double y)
    {
        m_pos.x = x;
        m_pos.y = y;
    }
    
    void Dog::setPosition(const Position p)
    {
        m_pos = p;
    }
    
    Speed Dog::speed() const
    {
        return m_speed;
    }
    
    Direction Dog::dir() const
    {
        return m_dir;
    }
    
    void Dog::setSpeed(const double v_x, const double v_y)
    {
        m_speed.v_x = v_x;
        m_speed.v_y = v_y;
    }
    
    void model::Dog::setDir(Direction d)
    {
        m_dir = d;
    }
    
    bool Position::operator==(const Position& other) const 
    {
        return (x == other.x) && (y == other.y);
    }
    
    bool Position::operator!=(const Position& other) const 
    {
        return !(*this == other);
    }
    
    
    
    
}


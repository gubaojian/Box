#include <Box/Event.hpp>
#include <typeinfo>

namespace box
{
    Event::Event() :
        m_bStopPropagation(false)
    {

    }

    Event::~Event()
    {

    }

    stick::String Event::name()
    {
        return stick::String(typeid(*this).name());
    }

    void Event::stopPropagation() const
    {
        m_bStopPropagation = true;
    }

    bool Event::propagationStopped() const
    {
        return m_bStopPropagation;
    }
}

#include <Box/Event.hpp>
#include <typeinfo>

namespace box
{
    Event::Event()
    {

    }

    Event::~Event()
    {

    }

    stick::String Event::name()
    {
        return stick::String(typeid(*this).name());
    }
}
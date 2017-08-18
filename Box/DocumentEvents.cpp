#include <Box/DocumentEvents.hpp>

namespace box
{
    using namespace stick;

    ComponentChangedEvent::ComponentChangedEvent(TypeID _id, const String & _name) :
    m_typeID(_id),
    m_namePtr(&_name)
    {

    }

    TypeID ComponentChangedEvent::componentTypeID() const
    {
        return m_typeID;
    }

    const String & ComponentChangedEvent::name() const
    {
        return *m_namePtr;
    }
}

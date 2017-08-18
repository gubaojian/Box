#ifndef BOX_DOCUMENTEVENTS_HPP
#define BOX_DOCUMENTEVENTS_HPP

#include <Box/Event.hpp>

namespace box
{
    class STICK_API ComponentChangedEvent : public EventT<ComponentChangedEvent>
    {
    public:

        ComponentChangedEvent(stick::TypeID _id, const stick::String & _name);

        stick::TypeID componentTypeID() const;

        const stick::String & name() const;

    private:

        stick::TypeID m_typeID;
        const stick::String * m_namePtr;
    };
}

#endif //BOX_DOCUMENTEVENTS_HPP

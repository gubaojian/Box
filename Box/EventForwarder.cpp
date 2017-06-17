#include <Box/EventForwarder.hpp>

namespace box
{
    using namespace stick;

    EventForwarder::EventForwarder(Allocator & _alloc) :
        m_filterStorage(_alloc),
        m_modifierStorage(_alloc),
        m_children(_alloc)
    {

    }

    CallbackID EventForwarder::addEventFilter(const Filter & _filter)
    {
        m_filterStorage.addCallback({nextID(), _filter.eventTypeID}, _filter.holder);
    }

    CallbackID EventForwarder::addEventModifier(const Modifier & _modifier)
    {
        m_modifierStorage.addCallback({nextID(), _modifier.eventTypeID}, _modifier.holder);
    }

    void EventForwarder::removeEventFilter(const CallbackID & _id)
    {
        m_filterStorage.removeCallback(_id);
    }

    void EventForwarder::removeEventModifier(const CallbackID & _id)
    {
        m_modifierStorage.removeCallback(_id);
    }

    bool EventForwarder::forward(const Event & _evt)
    {
        //apply filters
        if (filterAny(_evt))
            return false;

        {
            auto it = m_filterStorage.callbackMap.find(_evt.eventTypeID());
            if (it != m_filterStorage.callbackMap.end())
            {
                for (auto * filter : it->value)
                {
                    if (filter->call(_evt))
                        return false;
                }
            }
        }

        EventPtr tempStorage;
        const Event * evt = &_evt;

        //check if we need to modify the event
        {
            auto it = m_modifierStorage.callbackMap.find(_evt.eventTypeID());
            if (it != m_modifierStorage.callbackMap.end())
            {
                for (auto * modifier : it->value)
                {
                    tempStorage = modifier->call(*evt);
                    evt = tempStorage.get();
                }
            }
        }

        for (auto * child : m_children)
        {
            child->forward(*evt);
        }

        return true;
    }

    void EventForwarder::addForwarder(EventForwarder & _forwarder)
    {
        m_children.append(&_forwarder);
    }

    void EventForwarder::removeForwarder(EventForwarder & _forwarder)
    {
        auto it = stick::find(m_children.begin(), m_children.end(), &_forwarder);
        if (it != m_children.end())
            m_children.remove(it);
    }
}

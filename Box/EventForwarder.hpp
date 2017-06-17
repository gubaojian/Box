#ifndef BOX_EVENTFORWARDER_HPP
#define BOX_EVENTFORWARDER_HPP

#include <Box/Event.hpp>
#include <Box/Private/Callback.hpp>
#include <Box/Private/MappedCallbackStorage.hpp>

namespace box
{
    class STICK_API EventForwarder
    {
    public:

        using Filter = detail::CallbackT<bool, Event>;
        using Modifier = detail::CallbackT<EventPtr, Event>;
        using ForwarderArray = stick::DynamicArray<EventForwarder *>;
        using MappedFilterStorage = detail::MappedCallbackStorageT<typename Filter::CallbackBaseType>;
        using MappedModifierStorage = detail::MappedCallbackStorageT<typename Modifier::CallbackBaseType>;

        EventForwarder(stick::Allocator & _alloc);

        virtual ~EventForwarder() {}


        EventForwarder(const EventForwarder &) = default;
        EventForwarder(EventForwarder &&) = default;
        EventForwarder & operator = (const EventForwarder &) = default;
        EventForwarder & operator = (EventForwarder &&) = default;


        CallbackID addEventFilter(const Filter & _filter);

        CallbackID addEventModifier(const Modifier & _modifier);

        void removeEventFilter(const CallbackID & _id);

        void removeEventModifier(const CallbackID & _id);

        bool forward(const Event & _evt);

        void addForwarder(EventForwarder & _forwarder);

        void removeForwarder(EventForwarder & _forwarder);

    private:

        virtual bool filterAny(const Event & _any) { return false; };

        inline stick::Size nextID() const
        {
            static std::atomic<stick::Size> s_id(0);
            return s_id++;
        }


        MappedFilterStorage m_filterStorage;
        MappedModifierStorage m_modifierStorage;
        ForwarderArray m_children;
    };
}

#endif //BOX_EVENTFORWARDER_HPP

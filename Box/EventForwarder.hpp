#ifndef BOX_EVENTFORWARDER_HPP
#define BOX_EVENTFORWARDER_HPP

#include <Box/EventPublisher.hpp>

namespace box
{
    namespace detail
    {
        template<class PublisherType>
        struct STICK_API ForwardingPolicyBasic
        {
            using MappedFilterStorage = typename PublisherType::MappedFilterStorage;
            using MappedModifierStorage = typename PublisherType::MappedModifierStorage;
            using MutexType = stick::NoMutex;
            using ForwarderArray = typename PublisherType::ForwarderArray;
            using EventType = typename PublisherType::EventType;
            using EventUniquePtr = typename PublisherType::EventUniquePtr;

            template<class...PassAlongArgs>
            inline bool filter(const MappedFilterStorage & _filters, const EventType & _evt, PassAlongArgs..._args)
            {
                auto it = _filters.callbackMap.find(_evt.eventTypeID());
                if (it != _filters.callbackMap.end())
                {
                    for (auto * cb : it->value)
                    {
                        if (cb->call(_evt, std::forward<PassAlongArgs>(_args)...))
                            return true;
                    }
                }

                return false;
            }

            template<class...PassAlongArgs>
            inline const EventType & modify(const MappedModifierStorage & _modifiers, EventPtr & _tmpStorage, const EventType & _evt, PassAlongArgs..._args)
            {
                const EventType * ret = &_evt;
                auto it = _modifiers.callbackMap.find(_evt.eventTypeID());
                if (it != _modifiers.callbackMap.end())
                {
                    for (auto * cb : it->value)
                    {
                        _tmpStorage = cb->call(*ret, std::forward<PassAlongArgs>(_args)...);
                        ret = _tmpStorage.get();
                    }
                }

                return *ret;
            }

            inline void forward(const EventType & _evt, const ForwarderArray & _forwarders)
            {
                for (auto * f : _forwarders)
                {
                    f->publish(_evt, true);
                }
            }

            mutable MutexType modifierMutex;
            mutable MutexType filterMutex;
            mutable MutexType forwarderMutex;
        };
    }

    template<class EventT, template<class> class ForwardingPolicyT, template<class> class PublishingPolicyT, class...PassAlongArgs>
    class STICK_API EventForwarderT : public EventPublisherT<EventT, PublishingPolicyT, PassAlongArgs...>
    {
    public:

        using EventType = EventT;
        using EventUniquePtr = stick::UniquePtr<EventType>;
        using ForwardingPolicy = ForwardingPolicyT<EventForwarderT>;
        using EventPublisherType = EventPublisherT<EventT, PublishingPolicyT, PassAlongArgs...>;
        using Filter = detail::CallbackT<bool, EventType, PassAlongArgs...>;
        using Modifier = detail::CallbackT<EventUniquePtr, EventType, PassAlongArgs...>;
        using ForwarderArray = stick::DynamicArray<EventForwarderT *>;
        using MappedFilterStorage = detail::MappedCallbackStorageT<typename Filter::CallbackBaseType>;
        using MappedModifierStorage = detail::MappedCallbackStorageT<typename Modifier::CallbackBaseType>;

        EventForwarderT() :
            m_filterStorage(stick::defaultAllocator()),
            m_modifierStorage(stick::defaultAllocator()),
            m_children(stick::defaultAllocator())
        {

        }

        EventForwarderT(stick::Allocator & _alloc, PassAlongArgs..._args) :
            EventPublisherType(_alloc, std::forward<PassAlongArgs>(_args)...),
            m_filterStorage(_alloc),
            m_modifierStorage(_alloc),
            m_children(_alloc)
        {

        }

        virtual ~EventForwarderT()
        {

        }

        CallbackID addEventFilter(const Filter & _filter)
        {
            stick::ScopedLock<typename ForwardingPolicy::MutexType> lock(m_forwardingPolicy.filterMutex);
            m_filterStorage.addCallback({this->nextID(), _filter.eventTypeID}, _filter.holder);
        }

        CallbackID addEventModifier(const Modifier & _modifier)
        {
            stick::ScopedLock<typename ForwardingPolicy::MutexType> lock(m_forwardingPolicy.modifierMutex);
            m_modifierStorage.addCallback({this->nextID(), _modifier.eventTypeID}, _modifier.holder);
        }

        void removeEventFilter(const CallbackID & _id)
        {
            stick::ScopedLock<typename ForwardingPolicy::MutexType> lock(m_forwardingPolicy.filterMutex);
            m_filterStorage.removeCallback(_id);
        }

        void removeEventModifier(const CallbackID & _id)
        {
            stick::ScopedLock<typename ForwardingPolicy::MutexType> lock(m_forwardingPolicy.modifierMutex);
            m_modifierStorage.removeCallback(_id);
        }

        bool publish(const EventType & _evt, bool _bPropagate)
        {
            //apply filters
            if (filterAny(_evt))
                return false;

            bool bFilter = filterImpl(_evt, detail::MakeIndexSequence<sizeof...(PassAlongArgs)>());
            if (bFilter) return false;

            EventUniquePtr tempStorage;
            const EventType & evt = modifyImpl(tempStorage, _evt, detail::MakeIndexSequence<sizeof...(PassAlongArgs)>());

            EventPublisherType::publish(evt);

            if(_bPropagate && !_evt.propagationStopped())
                m_forwardingPolicy.forward(evt, m_children);

            return true;
        }

        void addForwarder(EventForwarderT & _forwarder)
        {
            stick::ScopedLock<typename ForwardingPolicy::MutexType> lock(m_forwardingPolicy.forwarderMutex);
            m_children.append(&_forwarder);
        }

        void removeForwarder(EventForwarderT & _forwarder)
        {
            stick::ScopedLock<typename ForwardingPolicy::MutexType> lock(m_forwardingPolicy.forwarderMutex);
            auto it = stick::find(m_children.begin(), m_children.end(), &_forwarder);
            if (it != m_children.end())
                m_children.remove(it);
        }

    protected:

        virtual bool filterAny(const EventType & _any) { return false; };

        template<stick::Size...S>
        inline bool filterImpl(const EventType & _evt, detail::IndexSequence<S...>)
        {
            return m_forwardingPolicy.filter(m_filterStorage, _evt, std::get<S>(this->m_passedArgsStorage)...);
        }

        template<stick::Size...S>
        inline const EventType & modifyImpl(EventUniquePtr & _tmpStorage, const EventType & _evt, detail::IndexSequence<S...>)
        {
            return m_forwardingPolicy.modify(m_modifierStorage, _tmpStorage, _evt, std::get<S>(this->m_passedArgsStorage)...);
        }

    private:

        MappedFilterStorage m_filterStorage;
        MappedModifierStorage m_modifierStorage;
        ForwarderArray m_children;
        ForwardingPolicy m_forwardingPolicy;
    };
}

#endif //BOX_EVENTFORWARDER_HPP

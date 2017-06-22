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

            template<class...PassAlongArgs>
            inline bool filter(const MappedFilterStorage & _filters, const Event & _evt, PassAlongArgs..._args)
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
            inline const Event & modify(const MappedModifierStorage & _modifiers, EventPtr & _tmpStorage, const Event & _evt, PassAlongArgs..._args)
            {
                const Event * ret = &_evt;
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

            inline void forward(const Event & _evt, const ForwarderArray & _forwarders)
            {
                for(auto * f : _forwarders)
                {
                    f->publish(_evt);
                }
            }

            mutable MutexType modifierMutex;
            mutable MutexType filterMutex;
            mutable MutexType forwarderMutex;
        };
    }

    template<template<class> class ForwardingPolicyT, template<class> class PublishingPolicyT, class...PassAlongArgs>
    class STICK_API EventForwarderT : public EventPublisherT<PublishingPolicyT>
    {
    public:

        using ForwardingPolicy = ForwardingPolicyT<EventForwarderT>;
        using EventPublisherType = EventPublisherT<PublishingPolicyT>;
        using Filter = detail::CallbackT<bool, Event, PassAlongArgs...>;
        using Modifier = detail::CallbackT<EventPtr, Event, PassAlongArgs...>;
        using ForwarderArray = stick::DynamicArray<EventForwarderT *>;
        using MappedFilterStorage = detail::MappedCallbackStorageT<typename Filter::CallbackBaseType>;
        using MappedModifierStorage = detail::MappedCallbackStorageT<typename Modifier::CallbackBaseType>;

        EventForwarderT(stick::Allocator & _alloc = stick::defaultAllocator(), PassAlongArgs..._args) :
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

        bool publish(const Event & _evt)
        {
            //apply filters
            if (filterAny(_evt))
                return false;

            bool bFilter = filterImpl(_evt, detail::MakeIndexSequence<sizeof...(PassAlongArgs)>());
            if (bFilter) return false;

            EventPtr tempStorage;
            const Event & evt = modifyImpl(tempStorage, _evt, detail::MakeIndexSequence<sizeof...(PassAlongArgs)>());

            EventPublisherType::publish(evt);

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

        virtual bool filterAny(const Event & _any) { return false; };

        template<stick::Size...S>
        inline bool filterImpl(const Event & _evt, detail::IndexSequence<S...>)
        {
            return m_forwardingPolicy.filter(m_filterStorage, _evt, std::get<S>(this->m_passedArgsStorage)...);
        }

        template<stick::Size...S>
        inline const Event & modifyImpl(EventPtr & _tmpStorage, const Event & _evt, detail::IndexSequence<S...>)
        {
            return m_forwardingPolicy.modify(m_modifierStorage, _tmpStorage, _evt, std::get<S>(this->m_passedArgsStorage)...);
        }

    private:

        MappedFilterStorage m_filterStorage;
        MappedModifierStorage m_modifierStorage;
        ForwarderArray m_children;
        ForwardingPolicy m_forwardingPolicy;
    };

    using EventForwarder = EventForwarderT<detail::ForwardingPolicyBasic, detail::PublishingPolicyBasic>;
}

#endif //BOX_EVENTFORWARDER_HPP

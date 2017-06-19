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

            inline bool filter(const Event & _evt, const MappedFilterStorage & _filters)
            {
                auto it = _filters.callbackMap.find(_evt.eventTypeID());
                if (it != _filters.callbackMap.end())
                {
                    for (auto * cb : it->value)
                    {
                        if (cb->call(_evt))
                            return true;
                    }
                }

                return false;
            }

            inline const Event & modify(const Event & _evt, EventPtr & _tmpStorage, const MappedModifierStorage & _modifiers)
            {
                const Event * ret = &_evt;
                auto it = _modifiers.callbackMap.find(_evt.eventTypeID());
                if (it != _modifiers.callbackMap.end())
                {
                    for (auto * cb : it->value)
                    {
                        _tmpStorage = cb->call(*ret);
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

    template<template<class> class ForwardingPolicyT, template<class> class PublishingPolicyT>
    class STICK_API EventForwarderT : public EventPublisherT<PublishingPolicyT>
    {
    public:

        using ForwardingPolicy = ForwardingPolicyT<EventForwarderT>;
        using EventPublisherType = EventPublisherT<PublishingPolicyT>;
        using Filter = detail::CallbackT<bool, Event>;
        using Modifier = detail::CallbackT<EventPtr, Event>;
        using ForwarderArray = stick::DynamicArray<EventForwarderT *>;
        using MappedFilterStorage = detail::MappedCallbackStorageT<typename Filter::CallbackBaseType>;
        using MappedModifierStorage = detail::MappedCallbackStorageT<typename Modifier::CallbackBaseType>;

        EventForwarderT(stick::Allocator & _alloc = stick::defaultAllocator()) :
            EventPublisherType(_alloc),
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
            stick::ScopedLock<typename ForwardingPolicy::MutexTye> lock(this->m_policy.filterMutex);
            m_filterStorage.addCallback({this->nextID(), _filter.eventTypeID}, _filter.holder);
        }

        CallbackID addEventModifier(const Modifier & _modifier)
        {
            stick::ScopedLock<typename ForwardingPolicy::MutexTye> lock(this->m_policy.modifierMutex);
            m_modifierStorage.addCallback({this->nextID(), _modifier.eventTypeID}, _modifier.holder);
        }

        void removeEventFilter(const CallbackID & _id)
        {
            stick::ScopedLock<typename ForwardingPolicy::MutexTye> lock(this->m_policy.filterMutex);
            m_filterStorage.removeCallback(_id);
        }

        void removeEventModifier(const CallbackID & _id)
        {
            stick::ScopedLock<typename ForwardingPolicy::MutexTye> lock(this->m_policy.modifierMutex);
            m_modifierStorage.removeCallback(_id);
        }

        bool publish(const Event & _evt)
        {
            //apply filters
            if (filterAny(_evt))
                return false;

            bool bFilter = m_forwardingPolicy.filter(_evt, m_filterStorage);
            if (bFilter) return false;

            EventPtr tempStorage;
            const Event & evt = m_forwardingPolicy.modify(_evt, tempStorage, m_modifierStorage);

            EventPublisherType::publish(evt);

            m_forwardingPolicy.forward(evt, m_children);

            return true;
        }

        void addForwarder(EventForwarderT & _forwarder)
        {
            stick::ScopedLock<typename ForwardingPolicy::MutexTye> lock(this->m_policy.forwarderMutex);
            m_children.append(&_forwarder);
        }

        void removeForwarder(EventForwarderT & _forwarder)
        {
            stick::ScopedLock<typename ForwardingPolicy::MutexTye> lock(this->m_policy.forwarderMutex);
            auto it = stick::find(m_children.begin(), m_children.end(), &_forwarder);
            if (it != m_children.end())
                m_children.remove(it);
        }

    protected:

        virtual bool filterAny(const Event & _any) { return false; };

    private:

        MappedFilterStorage m_filterStorage;
        MappedModifierStorage m_modifierStorage;
        ForwarderArray m_children;
        ForwardingPolicy m_forwardingPolicy;
    };

    using EventForwarder = EventForwarderT<detail::ForwardingPolicyBasic, detail::PublishingPolicyBasic>;
}

#endif //BOX_EVENTFORWARDER_HPP

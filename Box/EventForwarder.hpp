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

            inline void forward(const Event & _evt, const ForwarderArray & _forwarders)
            {
                for(auto * f : _forwarders)
                {
                    f->publish(_evt);
                }
            }

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
        using ForwarderArray = stick::DynamicArray<EventForwarderT *>;
        using MappedFilterStorage = detail::MappedCallbackStorageT<typename Filter::CallbackBaseType>;

        EventForwarderT(stick::Allocator & _alloc = stick::defaultAllocator()) :
            EventPublisherType(_alloc),
            m_filterStorage(_alloc),
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

        void removeEventFilter(const CallbackID & _id)
        {
            stick::ScopedLock<typename ForwardingPolicy::MutexType> lock(m_forwardingPolicy.filterMutex);
            m_filterStorage.removeCallback(_id);
        }

        bool publish(const Event & _evt)
        {
            //apply filters
            if (filterAny(_evt))
                return false;

            bool bFilter = m_forwardingPolicy.filter(_evt, m_filterStorage);
            if (bFilter) return false;

            EventPublisherType::publish(_evt);

            m_forwardingPolicy.forward(_evt, m_children);

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

    private:

        MappedFilterStorage m_filterStorage;
        ForwarderArray m_children;
        ForwardingPolicy m_forwardingPolicy;
    };

    using EventForwarder = EventForwarderT<detail::ForwardingPolicyBasic, detail::PublishingPolicyBasic>;
}

#endif //BOX_EVENTFORWARDER_HPP

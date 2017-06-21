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
            using Modifier = typename PublisherType::Modifier;
            using MutexType = stick::NoMutex;
            using ForwarderArray = typename PublisherType::ForwarderArray;

            template<class...Args>
            inline bool filter(const MappedFilterStorage & _filters, Args..._args)
            {
                auto it = _filters.callbackMap.find(stick::TypeInfoT<std::tuple<typename RawType<Args>::Type...>>::typeID());
                if (it != _filters.callbackMap.end())
                {
                    for (auto * cb : it->value)
                    {
                        if (cb->call(std::forward<Args>(_args)...))
                            return true;
                    }
                }

                return false;
            }

            template<class...Args>
            inline std::tuple<typename std::remove_reference<Args>::type...> modify(const MappedModifierStorage & _modifiers, Args..._args)
            {
                std::tuple<typename std::remove_reference<Args>::type...> ret = std::make_tuple(_args...);
                auto it = _modifiers.callbackMap.find(stick::TypeInfoT<std::tuple<typename RawType<Args>::Type...>>::typeID());
                if (it != _modifiers.callbackMap.end())
                {
                    for (auto * cb : it->value)
                    {
                        ret = cb->call(_args...);
                    }
                }

                return ret;
            }

            template<class...Args>
            inline void forward(const ForwarderArray & _forwarders, Args..._args)
            {
                for (auto * f : _forwarders)
                {
                    f->publish(std::forward<Args>(_args)...);
                }
            }

            mutable MutexType modifierMutex;
            mutable MutexType filterMutex;
            mutable MutexType forwarderMutex;
        };
    }

    template<template<class> class ForwardingPolicyT, template<class> class PublishingPolicyT, class Ret, class...Args>
    class STICK_API EventForwarderT : public EventPublisherT<PublishingPolicyT, Ret, Args...>
    {
    public:

        using ForwardingPolicy = ForwardingPolicyT<EventForwarderT>;
        using EventPublisherType = EventPublisherT<PublishingPolicyT, Ret, Args...>;
        using Filter = detail::CallbackT<bool, Args...>;
        using Modifier = detail::CallbackT<std::tuple<typename std::remove_reference<Args>::type...>, Args...>;
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

        EventForwarderT(const EventForwarderT &) = default;
        EventForwarderT(EventForwarderT &&) = default;

        EventForwarderT & operator = (const EventForwarderT &) = default;
        EventForwarderT & operator = (EventForwarderT &&) = default;

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

        template<class...Args2>
        bool publish(Args2..._args)
        {
            //apply filters
            if (filterAny(std::forward<Args2>(_args)...))
                return false;

            bool bFilter = m_forwardingPolicy.filter(m_filterStorage, std::forward<Args2>(_args)...);
            if (bFilter) return false;

            // EventPtr tempStorage;
            // const Event & evt = m_forwardingPolicy.modify(_evt, tempStorage, m_modifierStorage);

            auto args = m_forwardingPolicy.modify(m_modifierStorage, std::forward<Args2>(_args)...);

            EventPublisherType::publish(std::forward<Args2>(_args)...);

            m_forwardingPolicy.forward(m_children, std::forward<Args2>(_args)...);

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

        virtual bool filterAny(Args..._args) { return false; };

    private:

        MappedFilterStorage m_filterStorage;
        MappedModifierStorage m_modifierStorage;
        ForwarderArray m_children;
        ForwardingPolicy m_forwardingPolicy;
    };

    using EventForwarder = EventForwarderT<detail::ForwardingPolicyBasic, detail::PublishingPolicyBasic, void, const Event &>;
}

#endif //BOX_EVENTFORWARDER_HPP

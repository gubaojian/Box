#ifndef BOX_EVENTPUBLISHER_HPP
#define BOX_EVENTPUBLISHER_HPP

#include <Box/Private/Callback.hpp>
#include <Box/Private/MappedCallbackStorage.hpp>
#include <Box/Private/IndexSequence.hpp>
#include <Box/Event.hpp>
#include <Stick/Mutex.hpp>
#include <Stick/ScopedLock.hpp>

namespace box
{
    namespace detail
    {
        template<class PublisherType>
        struct STICK_API PublishingPolicyBasic
        {
            using MappedStorage = typename PublisherType::MappedStorage;
            using MutexType = stick::NoMutex;

            template<class...PassAlongArgs>
            inline void publish(const MappedStorage & _callbacks, const Event & _evt, PassAlongArgs..._args)
            {
                auto it = _callbacks.callbackMap.find(_evt.eventTypeID());
                if (it != _callbacks.callbackMap.end())
                {
                    for (auto * cb : it->value)
                    {
                        cb->call(_evt, std::forward<PassAlongArgs>(_args)...);
                    }
                }
            }

            mutable MutexType mutex;
        };

        template<class PublisherType>
        struct STICK_API PublishingPolicyLocking
        {
            using MappedStorage = typename PublisherType::MappedStorage;
            using MutexType = stick::Mutex;

            template<class...PassAlongArgs>
            inline void publish(const MappedStorage & _callbacks, const Event & _evt, PassAlongArgs..._args)
            {
                typename MappedStorage::RawPtrArray callbacks(_callbacks.storage.allocator());
                {
                    stick::ScopedLock<MutexType> lck(mutex);
                    auto it = _callbacks.callbackMap.find(_evt.eventTypeID());
                    if (it != _callbacks.callbackMap.end())
                    {
                        //we copy the array here so we can savely add new callbacks from within callbacks etc.
                        callbacks = it->value;
                    }
                }

                for (auto * cb : callbacks)
                {
                    cb->call(_evt, std::forward<PassAlongArgs>(_args)...);
                }
            }

            mutable MutexType mutex;
        };
    }

    template<template<class> class PublishingPolicyT, class...PassAlongArgs>
    class STICK_API EventPublisherT
    {
    public:

        using PublishingPolicy = PublishingPolicyT<EventPublisherT>;
        using Callback = detail::CallbackT<void, Event, PassAlongArgs...>;
        using MappedStorage = detail::MappedCallbackStorageT<typename Callback::CallbackBaseType>;
        using PassAlongArgsStorage = std::tuple<PassAlongArgs...>;


        EventPublisherT(stick::Allocator & _alloc = stick::defaultAllocator(), PassAlongArgs..._args) :
            m_alloc(&_alloc),
            m_storage(_alloc),
            m_passedArgsStorage(std::forward<PassAlongArgs>(_args)...)
        {

        }

        /**
         * @brief Virtual Destructor, you usually derive from this class.
         */
        virtual ~EventPublisherT()
        {

        }

        /**
         * @brief Publish an event to all registered subscribers.
         *
         * @param _event The event to publish.
         */
        void publish(const Event & _event)
        {
            beginPublishing(_event);
            publishImpl(_event, detail::MakeIndexSequence<sizeof...(PassAlongArgs)>());
            endPublishing(_event);
        }

        // template<class T, class...Args>
        // void publish(Args..._args)
        // {
        //     publish(stick::makeUnique<T>(*m_alloc, _args...));
        // }

        CallbackID addEventCallback(const Callback & _cb)
        {
            stick::ScopedLock<typename PublishingPolicy::MutexType> lock(m_policy.mutex);
            m_storage.addCallback({nextID(), _cb.eventTypeID}, _cb.holder);
        }

        /**
         * @brief Removes a callback.
         * @param _id The callback id to remove.
         */
        void removeEventCallback(const CallbackID & _id)
        {
            stick::ScopedLock<typename PublishingPolicy::MutexTye> lock(m_policy.mutex);
            m_storage.removeCallback(_id);
        }

        /**
         * @brief Can be overwritten if specific things need to happen right before the publisher emits its events.
         */
        virtual void beginPublishing(const Event & _evt)
        {

        }

        /**
         * @brief Can be overwritten if specific things need to happen right after the publisher emits its events.
         */
        virtual void endPublishing(const Event & _evt)
        {

        }


    protected:

        inline stick::Size nextID() const
        {
            static stick::Size s_id(0);
            return s_id++;
        }

        template<stick::Size...S>
        inline void publishImpl(const Event & _e, detail::IndexSequence<S...>)
        {
            m_policy.publish(m_storage, _e, std::get<S>(m_passedArgsStorage)...);
        }

        stick::Allocator * m_alloc;
        MappedStorage m_storage;
        PassAlongArgsStorage m_passedArgsStorage;
        PublishingPolicy m_policy;
    };

    using EventPublisher = EventPublisherT<detail::PublishingPolicyBasic>;
}

#endif //BOX_EVENTPUBLISHER_HPP

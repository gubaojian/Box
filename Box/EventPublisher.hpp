#ifndef BOX_EVENTPUBLISHER_HPP
#define BOX_EVENTPUBLISHER_HPP

#include <Box/Event.hpp>
#include <Box/Private/Callback.hpp>
#include <Box/Private/MappedCallbackStorage.hpp>
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
            using Arguments = typename PublisherType::Arguments;

            template<class...Args>
            inline void publish(const MappedStorage & _callbacks, Args..._args)
            {
                auto it = _callbacks.callbackMap.find(stick::TypeInfoT<std::tuple<typename RawType<Args>::Type...>>::typeID());
                if (it != _callbacks.callbackMap.end())
                {
                    printf("FOUNd\n");
                    for (auto * cb : it->value)
                    {
                        cb->call(std::forward<Args>(_args)...);
                    }
                }
            }

            mutable MutexType mutex;
        };

        // template<class PublisherType>
        // struct STICK_API PublishingPolicyLocking
        // {
        //     using MappedStorage = typename PublisherType::MappedStorage;
        //     using MutexType = stick::Mutex;

        //     inline void publish(const Event & _evt, const MappedStorage & _callbacks)
        //     {
        //         typename MappedStorage::RawPtrArray callbacks(_callbacks.storage.allocator());
        //         {
        //             stick::ScopedLock<MutexType> lck(mutex);
        //             auto it = _callbacks.callbackMap.find(_evt.eventTypeID());
        //             if (it != _callbacks.callbackMap.end())
        //             {
        //                 //we copy the array here so we can savely add new callbacks from within callbacks etc.
        //                 callbacks = it->value;
        //             }
        //         }

        //         for (auto * cb : callbacks)
        //         {
        //             cb->call(_evt);
        //         }
        //     }

        //     mutable MutexType mutex;
        // };
    }

    template<template<class> class PublishingPolicyT, class Ret, class...Args>
    class STICK_API EventPublisherT
    {
    public:

        using PublishingPolicy = PublishingPolicyT<EventPublisherT>;
        using Callback = detail::CallbackT<Ret, Args...>;
        using Arguments = typename Callback::Arguments;
        using MappedStorage = detail::MappedCallbackStorageT<typename Callback::CallbackBaseType>;


        EventPublisherT(stick::Allocator & _alloc = stick::defaultAllocator()) :
            m_alloc(&_alloc),
            m_storage(_alloc)
        {

        }

        /**
         * @brief Virtual Destructor, you usually derive from this class.
         */
        virtual ~EventPublisherT()
        {

        }


        EventPublisherT(const EventPublisherT &) = default;
        EventPublisherT(EventPublisherT &&) = default;

        EventPublisherT & operator = (const EventPublisherT &) = default;
        EventPublisherT & operator = (EventPublisherT &&) = default;

        /**
         * @brief Publish an event to all registered subscribers.
         *
         * @param _event The event to publish.
         */
        template<class...Args2>
        void publish(Args2..._args)
        {
            beginPublishing();
            m_policy.publish(m_storage, std::forward<Args2...>(_args...));
            endPublishing();
        }

        // template<class T, class...Args>
        // void publish(Args..._args)
        // {
        //     publish(stick::makeUnique<T>(*m_alloc, _args...));
        // }

        CallbackID addEventCallback(const Callback & _cb)
        {
            stick::ScopedLock<typename PublishingPolicy::MutexType> lock(m_policy.mutex);
            printf("DA TID %lu\n", _cb.eventTypeID);
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
        virtual void beginPublishing()
        {

        }

        /**
         * @brief Can be overwritten if specific things need to happen right after the publisher emits its events.
         */
        virtual void endPublishing()
        {

        }


    protected:

        inline stick::Size nextID() const
        {
            static stick::Size s_id(0);
            return s_id++;
        }

        stick::Allocator * m_alloc;
        MappedStorage m_storage;
        PublishingPolicy m_policy;
    };

    using EventPublisher = EventPublisherT<detail::PublishingPolicyBasic, void, const Event &>;
}

#endif //BOX_EVENTPUBLISHER_HPP

#ifndef BOX_EVENTPUBLISHER_HPP
#define BOX_EVENTPUBLISHER_HPP

#include <Box/Event.hpp>
#include <Box/Private/Callback.hpp>
#include <Box/Private/MappedCallbackStorage.hpp>
#include <Stick/Mutex.hpp>
#include <Stick/DynamicArray.hpp>

namespace box
{
    class STICK_API EventPublisher
    {
    public:

        using Callback = detail::CallbackT<void, Event>;
        using EventQueue = stick::DynamicArray<EventPtr>;
        using MappedStorage = detail::MappedCallbackStorageT<typename Callback::CallbackBaseType>;

        EventPublisher(stick::Allocator & _alloc = stick::defaultAllocator());

        /**
         * @brief Virtual Destructor, you usually derive from this class.
         */
        virtual ~EventPublisher();


        void queueEvent(EventPtr _event);

        template<class T, class...Args>
        void queueEvent(Args..._args)
        {
            auto evt = stick::makeUnique<T>(m_eventQueue.allocator(), _args...);
            queueEvent(std::move(evt));
        }

        /**
         * @brief Publish an event to all registered subscribers.
         *
         * @param _event The event to publish.
         */
        void publishEvent(EventPtr _event);

        template<class T, class...Args>
        void publishEvent(Args..._args)
        {
            queueEvent<T>(_args...);
            publish();
        }

        void publish();

        CallbackID addEventCallback(const Callback & _cb);

        /**
         * @brief Removes a callback.
         * @param _id The callback id to remove.
         */
        void removeEventCallback(const CallbackID & _id);

        /**
         * @brief Can be overwritten if specific things need to happen right before the publisher emits its events.
         */
        virtual void beginPublishing();

        /**
         * @brief Can be overwritten if specific things need to happen right after the publisher emits its events.
         */
        virtual void endPublishing();

    protected:

        inline stick::Size nextID() const
        {
            static std::atomic<stick::Size> s_id(0);
            return s_id++;
        }

        mutable stick::Mutex m_callbackMutex;
        MappedStorage m_storage;
        mutable stick::Mutex m_eventQueueMutex;
        EventQueue m_eventQueue;
    };
}

#endif //BOX_EVENTPUBLISHER_HPP

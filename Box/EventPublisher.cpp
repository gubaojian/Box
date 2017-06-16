#include <Box/EventPublisher.hpp>
#include <Stick/ScopedLock.hpp>

namespace box
{
    using namespace stick;

    EventPublisher::EventPublisher(Allocator & _alloc) :
        m_storage(_alloc),
        m_eventQueue(_alloc)
    {

    }

    EventPublisher::~EventPublisher()
    {

    }

    void EventPublisher::publish(EventPtr & _event)
    {
        queueEvent(_event);
        publish();
    }

    void EventPublisher::publish()
    {
        beginPublishing();

        stick::DynamicArray<Event *> evts;
        {
            ScopedLock<Mutex> lck;
            evts.resize(m_eventQueue.count());
            for (Size i = 0; i < m_eventQueue.count(); ++i)
            {
                evts[i] = m_eventQueue[i].get();
            }
        }

        MappedStorage::RawPtrArray callbacks;
        for (Event * _e : evts)
        {
            callbacks.clear();

            {
                ScopedLock<Mutex> lock(m_callbackMutex);
                auto it = m_storage.callbackMap.find(_e->eventTypeID());
                if (it != m_storage.callbackMap.end())
                {
                    //we copy the array here so we can savely add new callbacks from within callbacks etc.
                    callbacks = it->value;
                }
            }

            for(auto * cb : callbacks)
            {
                cb->call(*_e);
            }
        }

        {
            ScopedLock<Mutex> lck;
            m_eventQueue.clear();
        }

        endPublishing();
    }

    void EventPublisher::queueEvent(EventPtr & _event)
    {
        ScopedLock<Mutex> lck(m_eventQueueMutex);
        m_eventQueue.append(std::move(_event));
    }

    CallbackID EventPublisher::addEventCallback(const Callback & _cb)
    {
        ScopedLock<Mutex> lck(m_callbackMutex);
        m_storage.addCallback({nextID(), _cb.eventTypeID}, _cb.holder);
    }

    void EventPublisher::removeEventCallback(const CallbackID & _id)
    {
        ScopedLock<Mutex> lck(m_callbackMutex);
        m_storage.removeCallback(_id);
    }

    void EventPublisher::beginPublishing()
    {

    }

    void EventPublisher::endPublishing()
    {

    }
}

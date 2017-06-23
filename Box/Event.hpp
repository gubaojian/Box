#ifndef BOX_EVENT_HPP
#define BOX_EVENT_HPP

#include <Stick/TypeInfo.hpp>
#include <Stick/UniquePtr.hpp>
#include <Stick/String.hpp>

namespace box
{
    /**
     * @brief Base class for all events dispatched and received by EventPublisher and EventSubscriber.
     * @see EventT
     */
    class STICK_API Event
    {
    public:

        /**
         * @brief Default constructor.
         */
        Event();

        /**
         * @brief Virtual destructor.
         */
        virtual ~Event();

        /**
         * @brief Returns the name of the Event (mainly for logging purposes).
         *
         * The default implementation
         * simply returns the rtti name from c++ typeid. Can be overwritten.
         */
        virtual stick::String name();

        /**
         * @brief Returns a run time identifier that uniquely identifies the type of the event.
         */
        virtual stick::TypeID eventTypeID() const = 0;

        // @TODO: This is kinda weird in terms of const correctness
        // as we pass events by const reference.
        //
        void stopPropagation() const;

        bool propagationStopped() const;

    private:

        mutable bool m_bStopPropagation;
    };

    typedef stick::UniquePtr<Event> EventPtr;


    /**
     * @brief Templated helper class that implements the eventTypeID function and adds some typedefs for the Event of
     * type T.
     *
     * Usually you derive from this class, rather than from Event directly.
     */
    template<class T>
    class STICK_API EventT : public Event
    {
    public:

        /**
         * @brief The TypeInfo of T.
         */
        using TypeInfo = stick::TypeInfoT<T>;

        /**
         * @brief The std::shared_ptr of T.
         */
        using EventPtr = stick::UniquePtr<T>;

        /**
         * @brief Returns the unique identifier of T.
         */
        stick::TypeID eventTypeID() const
        {
            return TypeInfo::typeID();
        }
    };

    // /**
    //  * @brief Checks if _event is of type T.
    //  * @param _event The event whose type you want to compare to T.
    //  */
    // template<class T>
    // bool is(const EventPtr & _event)
    // {
    //     return _event->eventTypeID() == T::TypeInfo::typeID();
    // }

    // *
    //  * @brief Casts an EventPtr to the Event type T.
    //  *
    //  * Internally this just does a static_cast.
    //  * It is your responsibility to ensure that _from can actually be casted to T.
    //  * @param _from The Event you want to cast to T.
    //  * @return A reference to the event of type T.

    // template<class T>
    // T & eventCast(const EventPtr & _from)
    // {
    //     MOTOR_ASSERT(is<T>(_from));
    //     return *static_cast<T *>(_from.get());
    // }

    // typedef Size CallbackID;
}

#endif //BOX_EVENT_HPP

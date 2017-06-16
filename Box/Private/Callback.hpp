#ifndef BOX_PRIVATE_CALLBACK_HPP
#define BOX_PRIVATE_CALLBACK_HPP

#include <Box/Private/FunctionTraits.hpp>

namespace box
{
    namespace detail
    {
        template<class Ret, class EventBase>
        struct CallbackBaseT
        {
            virtual ~CallbackBaseT() {}

            virtual Ret call(const EventBase & _event) const  = 0;
        };

        template<class Ret, class EventBase, class EventT>
        struct FunctionCallbackT : public CallbackBaseT<Ret, EventBase>
        {
            using EventType = EventT;

            typedef Ret (*Function)(const EventType &);


            FunctionCallbackT(Function _func) :
                function(_func)
            {

            }

            Ret call(const EventBase & _event) const override
            {
                return (function)(static_cast<const EventType &>(_event));
            }

            Function function;
        };

        template<class Ret, class EventBase, class T, class EventT>
        struct MemberFunctionCallbackT : public CallbackBaseT<Ret, EventBase>
        {

            using EventType = EventT;

            typedef void (T::*MemberFunction)(const EventType &);

            MemberFunctionCallbackT(T * _obj, MemberFunction _memFn) :
                obj(_obj),
                function(_memFn)
            {

            }

            Ret call(const EventBase & _event) const override
            {
                return (obj->*function)(static_cast<const EventType &>(_event));
            }

            T * obj;
            MemberFunction function;
        };

        template<class Ret, class EventBase, class T>
        struct FunctorEventCallbackT : public CallbackBaseT<Ret, EventBase>
        {
            using EventArgType = typename FunctionTraits<T>::template Argument<0>::Type;
            using EventType = typename std::remove_cv<typename std::remove_reference<EventArgType>::type>::type;


            FunctorEventCallbackT(T _func) :
                functor(_func)
            {

            }

            Ret call(const EventBase & _event) const override
            {
                return functor(static_cast<const EventType &>(_event));
            }

            T functor;
        };

        template<class Ret, class EventBase>
        struct CallbackT
        {
            using CallbackBaseType = CallbackBaseT<Ret, EventBase>;

            CallbackT() = default;
            CallbackT(const CallbackT &) = default;
            CallbackT(CallbackT &&) = default;

            //construct from free function
            template<class EventT>
            CallbackT(Ret (*_function)(const EventT &)) :
            eventTypeID(stick::TypeInfoT<EventT>::typeID())
            {
                using FT = FunctionCallbackT<Ret, EventBase, EventT>;
                //@TODO: Allow custom allocator
                holder = stick::defaultAllocator().create<FT>(_function);
            }

            //construct from member function
            template<class T, class EventT>
            CallbackT(T * _obj, Ret (T::*_memFunction)(const EventT &)) :
            eventTypeID(stick::TypeInfoT<EventT>::typeID())
            {
                using FT = MemberFunctionCallbackT<Ret, EventBase, T, EventT>;
                //@TODO: Allow custom allocator
                holder = stick::defaultAllocator().create<FT>(_obj, _memFunction);
            }

            //construct from functor
            template<class F>
            CallbackT(F _functor)
            {
                using FT = FunctorEventCallbackT<Ret, EventBase, F>;
                eventTypeID = stick::TypeInfoT<typename FT::EventType>::typeID();
                //@TODO: Allow custom allocator
                holder = stick::defaultAllocator().create<FT>(_functor);
            }

            Ret call(const EventBase & _evt) const
            {
                return holder->call(_evt);
            }

            CallbackBaseType * holder;
            stick::TypeID eventTypeID;
        };
    }
}

#endif //BOX_PRIVATE_CALLBACK_HPP

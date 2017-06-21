#ifndef BOX_PRIVATE_CALLBACK_HPP
#define BOX_PRIVATE_CALLBACK_HPP

#include <Stick/TypeInfo.hpp>
#include <Box/Private/FunctionTraits.hpp>
#include <tuple>

namespace box
{
    namespace detail
    {
        template<class T>
        struct NoCast
        {
            using BaseType = T;
            using ActualType = T;

            static T cast(T _val)
            {
                return _val;
            }
        };

        template<class F, class T>
        struct Cast
        {
            using BaseType = F;
            using ActualType = T;

            static T cast(F _val)
            {
                return static_cast<T>(_val);
            }
        };

        template<class Ret, class...Args>
        struct CallbackBaseT
        {
            virtual ~CallbackBaseT() {}

            virtual Ret call(Args..._args) const  = 0;
        };

        template<class Ret, class...Args>
        struct FunctionCallbackT : public CallbackBaseT<Ret, typename Args::BaseType...>
        {
            // using EventType = EventT;

            typedef Ret (*Function)(typename Args::ActualType...);


            FunctionCallbackT(Function _func) :
                function(_func)
            {

            }

            Ret call(typename Args::BaseType..._args) const override
            {
                return (function)(Args::cast(_args)...);
            }

            Function function;
        };

        template<class Ret, class T, class...Args>
        struct MemberFunctionCallbackT : public CallbackBaseT<Ret, typename Args::BaseType...>
        {

            // using EventType = EventT;

            typedef void (T::*MemberFunction)(typename Args::ActualType...);

            MemberFunctionCallbackT(T * _obj, MemberFunction _memFn) :
                obj(_obj),
                function(_memFn)
            {

            }

            Ret call(typename Args::BaseType..._args) const override
            {
                return (obj->*function)(Args::cast(_args)...);
            }

            T * obj;
            MemberFunction function;
        };

        template<class Ret, class T, class...Args>
        struct FunctorEventCallbackT : public CallbackBaseT<Ret, typename Args::BaseType...>
        {
            // using EventArgType = typename FunctionTraits<T>::template Argument<0>::Type;
            // using EventType = typename std::remove_cv<typename std::remove_reference<EventArgType>::type>::type;


            FunctorEventCallbackT(T _func) :
                functor(_func)
            {

            }

            Ret call(typename Args::BaseType..._args) const override
            {
                return functor(Args::cast(_args)...);
            }

            T functor;
        };

        template<class T>
        struct RawType
        {
            using Type = typename std::remove_cv<typename std::remove_reference< typename std::remove_pointer<T>::type>::type>::type;
        };

        template<stick::Size ...> struct seq {};

        template<stick::Size N, stick::Size ...S> struct gens : gens < N - 1, N - 1, S... > { };

        template<stick::Size ...S> struct gens<0, S...> { typedef seq<S...> type; };

        template<class F, class T, bool>
        struct IsConvertibleHelper;

        template<class F, class T>
        struct IsConvertibleHelper<F, T, false>
        {
            using Type = F;
        };

        template<class F, class T>
        struct IsConvertibleHelper<F, T, true>
        {
            using Type = T;
        };

        template<class F, class T>
        struct IsConvertible
        {
            static_assert(std::is_convertible<F, T>::value, "YOYOOYOOY");
            using Type = typename IsConvertibleHelper<F, T, std::is_convertible<F, T>::value>::Type;
        };

        template<class TargetTuple, class...Args>
        struct MatchArguments
        {
            using Arguments = std::tuple<typename RawType<Args>::Type...>;
        };

        template<class Ret, class...OArgs>
        struct CallbackT
        {
            using CallbackBaseType = CallbackBaseT<Ret, OArgs...>;
            using Arguments = std::tuple<OArgs...>;

            CallbackT() = default;
            CallbackT(const CallbackT &) = default;
            CallbackT(CallbackT &&) = default;

            //construct from free function
            template<class...Args>
            CallbackT(Ret (*_function)(Args...)) :
                eventTypeID(stick::TypeInfoT<std::tuple<typename RawType<Args>::Type...>>::typeID())
            {
                using FT = FunctionCallbackT<Ret, Cast<OArgs, Args>...>;

                //@TODO: Allow custom allocator
                holder = stick::defaultAllocator().create<FT>(_function);
            }

            //construct from member function
            template<class T, class...Args>
            CallbackT(T * _obj, Ret (T::*_memFunction)(Args...)) :
                eventTypeID(stick::TypeInfoT<std::tuple<typename RawType<Args>::Type...>>::typeID())
            {
                using FT = MemberFunctionCallbackT<Ret, T, Cast<OArgs, Args>...>;

                //@TODO: Allow custom allocator
                holder = stick::defaultAllocator().create<FT>(_obj, _memFunction);
            }

            //construct from functor
            template<class F>
            CallbackT(F _functor)
            {
                using Args = typename FunctionTraits<F>::Arguments;
                initializeFunctor<F, Args>(_functor, typename gens<std::tuple_size<Args>::value>::type());
            }

            template<class F, class ArgTuple, stick::Size...S>
            void initializeFunctor(F _functor, seq<S...>)
            {
                using FT = FunctorEventCallbackT<Ret, F, Cast<OArgs, typename std::tuple_element<S, ArgTuple>::type>...>;
                eventTypeID = stick::TypeInfoT<std::tuple<typename RawType<typename std::tuple_element<S, ArgTuple>::type>::Type...>>::typeID();
                //@TODO: Allow custom allocator
                holder = stick::defaultAllocator().create<FT>(_functor);
            }

            Ret call(OArgs..._args) const
            {
                return holder->call(std::forward<OArgs>(_args)...);
            }

            CallbackBaseType * holder;
            stick::TypeID eventTypeID;
        };
    }
}

#endif //BOX_PRIVATE_CALLBACK_HPP

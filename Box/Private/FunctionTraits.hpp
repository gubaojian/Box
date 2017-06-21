#ifndef BOX_PRIVATE_FUNCTIONTRAITS_HPP
#define BOX_PRIVATE_FUNCTIONTRAITS_HPP

#include <Stick/Platform.hpp>
#include <tuple>

namespace box
{
    namespace detail
    {
        /**
         * @brief Simple function traits for compile time function signature deduction.
         *
         * based on the blog post here: http://functionalcpp.wordpress.com/2013/08/05/function-traits/
         */
        template<class F>
        struct FunctionTraits;
        
        template<class R, class... Args>
        struct FunctionTraits<R(*)(Args...)> :
        public FunctionTraits<R(Args...)>
        {
        };
        
        /**
         * @arg R The return type of the function.
         * @arg Args Variadic template argument the expands to the number and types of the function arguments.
         */
        template<class R, class... Args>
        struct FunctionTraits<R(Args...)>
        {
            using Arguments = std::tuple<Args...>;
            
            /**
             * @brief R The return type of the function.
             */
            typedef R ReturnType;
            
            /**
             * @brief Holds the number of function arguments.
             */
            static const stick::Size arity = sizeof...(Args);
            
            /**
             * @brief Helper to get the type of the function argument at a certain position.
             * @arg N The index of the function argument to retrieve.
             */
            template<stick::Size N>
            struct Argument
            {
                static_assert(N < arity, "error: invalid parameter index.");
                
                /**
                 * @brief The type of the function argument at N.
                 */
                typedef typename std::tuple_element<N,std::tuple<Args...> >::type Type;
            };
        };
        
        /**
         * @brief Gets the function traits of a member function.
         */
        template<class C, class R, class... Args>
        struct FunctionTraits<R(C::*)(Args...)> :
        public FunctionTraits<R(Args...)>
        {
        };
        
        /**
         * @brief Gets the function traits of a const member function.
         */
        template<class C, class R, class... Args>
        struct FunctionTraits<R(C::*)(Args...) const> :
        public FunctionTraits<R(Args...)>
        {
        };
        
        /**
         * @brief Gets the traits of a member object pointer.
         */
        template<class C, class R>
        struct FunctionTraits<R(C::*)> :
        public FunctionTraits<R()>
        {
        };
        
        /**
         * @brief Gets the traits of a functor (i.e. std::function or lambda).
         */
        template<class F>
        struct FunctionTraits :
        public FunctionTraits<decltype(&F::operator())>
        {
        };
        
        /**
         * @brief Removes l value reference before getting the function traits.
         */
        template<class F>
        struct FunctionTraits<F&> :
        public FunctionTraits<F>
        {
        };
        
        /**
         * @brief Removes r value reference before getting the function traits.
         */
        template<class F>
        struct FunctionTraits<F&&> :
        public FunctionTraits<F>
        {
        };
    }
}

#endif //BOX_PRIVATE_FUNCTIONTRAITS_HPP

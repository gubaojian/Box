#ifndef BOX_PRIVATE_INDEXSEQUENCE_HPP
#define BOX_PRIVATE_INDEXSEQUENCE_HPP

#include <Stick/Platform.hpp>

namespace box
{
    namespace detail
    {
        template<class T, T... Ints> struct IntegerSequence
        {
        };

        template<class S> struct NextIntegerSequence;

        template<class T, T... Ints> struct NextIntegerSequence<IntegerSequence<T, Ints...>>
        {
            using type = IntegerSequence<T, Ints..., sizeof...(Ints)>;
        };

        template<class T, T I, T N> struct MakeIntSequenceImpl;

        template<class T, T N>
        using MakeIntegerSequence = typename MakeIntSequenceImpl<T, 0, N>::type;

        template<class T, T I, T N> struct MakeIntSequenceImpl
        {
            using type = typename NextIntegerSequence <
                         typename MakeIntSequenceImpl < T, I + 1, N >::type >::type;
        };

        template<class T, T N> struct MakeIntSequenceImpl<T, N, N>
        {
            using type = IntegerSequence<T>;
        };

        template<stick::Size... Ints>
        using IndexSequence = IntegerSequence<stick::Size, Ints...>;

        template<stick::Size N>
        using MakeIndexSequence = MakeIntegerSequence<stick::Size, N>;
    }
}

#endif //BOX_PRIVATE_INDEXSEQUENCE_HPP

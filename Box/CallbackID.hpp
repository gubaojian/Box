#ifndef BOX_CALLBACKID_HPP
#define BOX_CALLBACKID_HPP

#include <Stick/TypeInfo.hpp>

namespace box
{
    struct STICK_API CallbackID
    {
        stick::Size id;
        stick::TypeID typeID;
    };
}

#endif //BOX_CALLBACKID_HPP

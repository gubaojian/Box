#ifndef BOX_MOUSESTATE_HPP
#define BOX_MOUSESTATE_HPP

#include <Stick/Platform.hpp>

namespace box
{
    enum class STICK_API MouseButton
    {
        Left = 1,
        Right = 1 << 1,
        Middle = 1 << 2,
        Button3 = 1 << 3,
        Button4 = 1 << 4,
        Button5 = 1 << 5,
        Button6 = 1 << 6,
        Button7 = 1 << 7,
        None = 1 << 8 //also used for unknown buttons
    };

    class STICK_API MouseState
    {
    public:

        MouseState();

        MouseState(stick::Float32 _x, stick::Float32 _y, stick::UInt32 _buttonStateBitMask);


        void reset();

        void setPosition(stick::Float32 _x, stick::Float32 _y);

        void setButtonBitMask(stick::UInt32 _mask);

        stick::Float32 x() const;

        stick::Float32 y() const;

        bool isButtonDown(MouseButton _button) const;

        stick::UInt32 buttonBitMask() const;


    private:

        stick::Float32 m_x, m_y;
        stick::UInt32 m_buttonStateBitMask;
    };
}

#endif //BOX_MOUSESTATE_HPP

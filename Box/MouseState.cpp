#include <Box/MouseState.hpp>

namespace box
{
    using namespace stick;

    MouseState::MouseState()
    {
        reset();
    }

    MouseState::MouseState(Float32 _x, Float32 _y, UInt32 _buttonStateBitMask) :
        m_x(_x),
        m_y(_y),
        m_buttonStateBitMask(_buttonStateBitMask)
    {

    }

    void MouseState::reset()
    {
        m_x = 0;
        m_y = 0;
        m_buttonStateBitMask = 0;
    }

    void MouseState::setPosition(Float32 _x, Float32 _y)
    {
        m_x = _x;
        m_y = _y;
    }

    void MouseState::setButtonBitMask(UInt32 _mask)
    {
        m_buttonStateBitMask = _mask;
    }

    Float32 MouseState::x() const
    {
        return m_x;
    }

    Float32 MouseState::y() const
    {
        return m_y;
    }

    bool MouseState::isButtonDown(MouseButton _button) const
    {
        return (m_buttonStateBitMask & static_cast<UInt32>(_button));
    }

    UInt32 MouseState::buttonBitMask() const
    {
        return m_buttonStateBitMask;
    }
}

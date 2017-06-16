#include <Box/MouseEvents.hpp>

namespace box
{
    using namespace stick;

    MouseEvent::MouseEvent(const MouseState & _state, Float32 _scrollX, Float32 _scrollY, MouseButton _button) :
        m_state(_state),
        m_modifiedButton(_button),
        m_scrollX(_scrollX),
        m_scrollY(_scrollY)
    {
    }

    MouseEvent::~MouseEvent()
    {

    }

    Float32 MouseEvent::x() const
    {
        return m_state.x();
    }

    Float32 MouseEvent::y() const
    {
        return m_state.y();
    }

    Float32 MouseEvent::scrollX() const
    {
        return m_scrollX;
    }

    Float32 MouseEvent::scrollY() const
    {
        return m_scrollY;
    }

    MouseButton MouseEvent::button() const
    {
        return m_modifiedButton;
    }

    MouseMoveEvent::MouseMoveEvent(const MouseState & _state) :
        MouseEvent(_state, 0, 0, MouseButton::None)
    {

    }

    MouseDragEvent::MouseDragEvent(const MouseState & _state, MouseButton _button) :
        MouseEvent(_state, 0, 0, _button)
    {

    }

    MouseDownEvent::MouseDownEvent(const MouseState & _state, MouseButton _button) :
        MouseEvent(_state, 0, 0, _button)
    {

    }

    MouseUpEvent::MouseUpEvent(const MouseState & _state, MouseButton _button) :
        MouseEvent(_state, 0, 0, _button)
    {

    }

    MouseScrollEvent::MouseScrollEvent(const MouseState & _state, Float32 _scrollX, Float32 _scrollY) :
        MouseEvent(_state, _scrollX, _scrollY, MouseButton::None)
    {

    }
}

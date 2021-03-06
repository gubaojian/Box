#ifndef BOX_MOUSEEVENTS_HPP
#define BOX_MOUSEEVENTS_HPP

#include <Stick/Event.hpp>
#include <Box/MouseState.hpp>

namespace box
{
    class STICK_API MouseEvent
    {
    public:

        MouseEvent(const MouseState & _state, stick::Float32 _scrollX, stick::Float32 _scrollY, MouseButton _button = MouseButton::None);

        virtual ~MouseEvent();

        stick::Float32 x() const;

        stick::Float32 y() const;

        stick::Float32 scrollX() const;

        stick::Float32 scrollY() const;

        //returns the button associated with this event
        MouseButton button() const;

        const MouseState & mouseState() const;

    private:

        MouseState m_state;
        MouseButton m_modifiedButton;
        //scroll data is not in MouseStae since it is only a relative change and thus only valid during the event
        stick::Float32 m_scrollX, m_scrollY;
    };

    class STICK_API MouseMoveEvent :
        public MouseEvent,
        public stick::EventT<MouseMoveEvent>
    {
    public:

        MouseMoveEvent(const MouseState & _state);

        MouseMoveEvent(const MouseMoveEvent&) = default;
        MouseMoveEvent(MouseMoveEvent&&) = default;
        MouseMoveEvent & operator = (const MouseMoveEvent&) = default;
        MouseMoveEvent & operator = (MouseMoveEvent&&) = default;
    };

    class STICK_API MouseDragEvent :
        public MouseEvent,
        public stick::EventT<MouseDragEvent>
    {
    public:

        MouseDragEvent(const MouseState & _state, MouseButton _button);
    };

    class STICK_API MouseDownEvent :
        public MouseEvent,
        public stick::EventT<MouseDownEvent>
    {
    public:

        MouseDownEvent(const MouseState & _state, MouseButton _button);
    };

    class STICK_API MouseUpEvent :
        public MouseEvent,
        public stick::EventT<MouseUpEvent>
    {
    public:

        MouseUpEvent(const MouseState & _state, MouseButton _button);
    };

    class STICK_API MouseScrollEvent :
        public MouseEvent,
        public stick::EventT<MouseScrollEvent>
    {
    public:

        MouseScrollEvent(const MouseState & _state, stick::Float32 _scrollX, stick::Float32 _scrollY);
    };

    class STICK_API MouseEnterEvent :
        public MouseEvent,
        public stick::EventT<MouseEnterEvent>
    {
    public:

        MouseEnterEvent(const MouseState & _state);
    };

    class STICK_API MouseLeaveEvent :
        public MouseEvent,
        public stick::EventT<MouseLeaveEvent>
    {
    public:

        MouseLeaveEvent(const MouseState & _state);
    };
}

#endif //BOX_MOUSEEVENTS_HPP

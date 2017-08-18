#ifndef BOX_BOXLUA_HPP
#define BOX_BOXLUA_HPP

#include <Box/Box.hpp>
#include <Box/MouseEvents.hpp>
#include <Luanatic/Luanatic.hpp>
#include <Stick/Path.hpp>

namespace box
{
    namespace detail
    {
        struct STICK_API LuaCallback
        {
            luanatic::LuaValue function;
        };

        template<class EventT>
        inline stick::Int32 luaEventCallback(lua_State * _state)
        {
            printf("DA EVENT CAB TO LUA BABY\n");
            brick::Entity * e = luanatic::convertToTypeAndCheck<brick::Entity>(_state, 1);

            if (!lua_isfunction(_state, 2))
                luanatic::detail::luaErrorWithStackTrace(_state, 2, "Function expected");

            luanatic::LuaValue luaCallback(_state, 2);
            addEventCallback(*e, [luaCallback, _state](const EventT & _e, brick::Entity _self)
            {
                printf("CALLED IT BRO\n");
                //01. push callback function
                luaCallback.push();
                //02. push event
                luanatic::push<EventT>(_state, &_e);
                //03. push entity
                luanatic::push<brick::Entity>(_state, &_self);
                //04. call function
                lua_call(_state, 2, 0);

                printf("CLICKED BABY\n");
            });

            printf("END YOOOOO\n");

            return 0;
        }

        // @TODO: Not sure if we need this, LUANATIC_FUNCTION(&setComponent<comps::Name, const stick::String&>)
        // did not compile though so we use dis glue function.
        void luaSetName(brick::Entity _e, const stick::String & _name)
        {
            setComponent<comps::Name>(_e, _name);
        }

        void luaPublish(brick::Entity _e, const MouseMoveEvent & _event, bool _bPropagate)
        {
            printf("PUBLISH LUA MMMM %lu\n", _e.id());
            publish(_e, _event, _bPropagate);
        }

        // inline stick::Int32 addMouseMoveCallback(lua_State * _state)
        // {
        //     brick::Entity * e = luanatic::convertToTypeAndCheck<brick::Entity>(_state, 1);

        //     if (!lua_isfunction(_state, 2))
        //         luanatic::detail::luaErrorWithStackTrace(_state, 2, "Function expected");

        //     luanatic::LuaValue luaCallback(_state, 2);
        //     addEventCallback(*e, [luaCallback, _state](const MouseMoveEvent & _e, brick::Entity _self)
        //     {
        //         //01. push callback function
        //         luaCallback.push();
        //         //02. push event
        //         luanatic::push<MouseMoveEvent>(_state, &_e);
        //         //03. push entity
        //         luanatic::push<brick::Entity>(_state, &_self);
        //         //04. call function
        //         lua_call(_state, 2, 0);

        //         printf("CLICKED BABY\n");
        //     });

        //     return 0;
        // }
    }

    // the core binding to the box c++ api
    inline void registerBox(lua_State * _state, brick::Hub * _hub, const stick::String & _namespace = "core")
    {
        using namespace luanatic;
        using namespace brick;
        using namespace stick;

        LuaValue g = globalsTable(_state);
        LuaValue namespaceTable = g;

        //TODO: Move this into a utility function in Luanatic
        if (!_namespace.isEmpty())
        {
            auto tokens = path::segments(_namespace, '.');
            for (const String & token : tokens)
            {
                LuaValue table = namespaceTable.findOrCreateTable(token);
                namespaceTable = table;
            }
        }

        g["__coreNS"].set(namespaceTable);

        //We don't expose any of the Hub API to Lua
        //It needs to know about the type for createNode though.
        ClassWrapper<Hub> hubCW("Hub");
        namespaceTable.registerClass(hubCW);

        LuaValue val = namespaceTable["hub"];
        val.set(_hub);

        //@TODO: This is really more of a hidden class that serves to identify a DOM element.
        //
        //This way right now there will be a heap allocation whenever creating/copying a entity
        //handle which is not very optimal.
        //Should we keep it like this or find a better way to send it to lua?
        //Providing custom bucket memory allocation should be the easiest/cleanest solution?
        ClassWrapper<Entity> entityCW("Entity");
        entityCW.
        addConstructor<>().
        addConstructor<const Entity &>().
        addMemberFunction(EqualOperatorFlag, LUANATIC_FUNCTION(&Entity::operator==)).
        addMemberFunction("invalidate", LUANATIC_FUNCTION(&Entity::invalidate)).
        addMemberFunction("destroy", LUANATIC_FUNCTION(&Entity::destroy)).
        addMemberFunction("isValid", LUANATIC_FUNCTION(&Entity::isValid)).
        addMemberFunction("id", LUANATIC_FUNCTION(&Entity::id));
        //@TODO: Do we need any more of Entity member functions?

        namespaceTable.registerClass(entityCW);

        // register all the DOM related functions
        namespaceTable.
        registerFunction("createNode", LUANATIC_FUNCTION(&createNode)).
        registerFunction("addChild", LUANATIC_FUNCTION(&addChild)).
        registerFunction("removeChild", LUANATIC_FUNCTION(&removeChild)).
        registerFunction("removeChildren", LUANATIC_FUNCTION(&removeChild)).
        registerFunction("reverseChildren", LUANATIC_FUNCTION(&removeChild)).
        registerFunction("remove", LUANATIC_FUNCTION(&remove)).
        registerFunction("layout", LUANATIC_FUNCTION(&layout)).
        registerFunction("setName", LUANATIC_FUNCTION(&detail::luaSetName)).
        registerFunction("setTag", LUANATIC_FUNCTION(&setTag)).
        registerFunction("setSize", LUANATIC_FUNCTION(&setSize)).
        registerFunction("setWidth", LUANATIC_FUNCTION(&setWidth)).
        registerFunction("setHeight", LUANATIC_FUNCTION(&setHeight)).
        registerFunction("setMinSize", LUANATIC_FUNCTION(&setMinSize)).
        registerFunction("setMinWidth", LUANATIC_FUNCTION(&setMinWidth)).
        registerFunction("setMinHeight", LUANATIC_FUNCTION(&setMinHeight)).
        registerFunction("setMaxSize", LUANATIC_FUNCTION(&setMaxSize)).
        registerFunction("setMaxWidth", LUANATIC_FUNCTION(&setMaxWidth)).
        registerFunction("setMaxHeight", LUANATIC_FUNCTION(&setMaxHeight)).
        registerFunction("setMargin", LUANATIC_FUNCTION_OVERLOAD(void(*)(Entity, Value), &setMargin)).
        registerFunction("setMargin", LUANATIC_FUNCTION_OVERLOAD(void(*)(Entity, Value, Value), &setMargin)).
        registerFunction("setMargin", LUANATIC_FUNCTION_OVERLOAD(void(*)(Entity, Value, Value, Value), &setMargin)).
        registerFunction("setMargin", LUANATIC_FUNCTION_OVERLOAD(void(*)(Entity, Value, Value, Value, Value), &setMargin)).
        registerFunction("setMarginLeft", LUANATIC_FUNCTION(&setMarginLeft)).
        registerFunction("setMarginTop", LUANATIC_FUNCTION(&setMarginTop)).
        registerFunction("setMarginRight", LUANATIC_FUNCTION(&setMarginRight)).
        registerFunction("setMarginBottom", LUANATIC_FUNCTION(&setMarginBottom)).
        registerFunction("setPadding", LUANATIC_FUNCTION_OVERLOAD(void(*)(Entity, Value), &setPadding)).
        registerFunction("setPadding", LUANATIC_FUNCTION_OVERLOAD(void(*)(Entity, Value, Value), &setPadding)).
        registerFunction("setPadding", LUANATIC_FUNCTION_OVERLOAD(void(*)(Entity, Value, Value, Value), &setPadding)).
        registerFunction("setPadding", LUANATIC_FUNCTION_OVERLOAD(void(*)(Entity, Value, Value, Value, Value), &setPadding)).
        registerFunction("setPaddingLeft", LUANATIC_FUNCTION(&setPaddingLeft)).
        registerFunction("setPaddingTop", LUANATIC_FUNCTION(&setPaddingTop)).
        registerFunction("setPaddingRight", LUANATIC_FUNCTION(&setPaddingRight)).
        registerFunction("setPaddingBottom", LUANATIC_FUNCTION(&setPaddingBottom));

        //regsiter constants/enums
        LuaValue mbTable = namespaceTable.findOrCreateTable("MouseButton");
        mbTable["Left"].set(MouseButton::Left);
        mbTable["Right"].set(MouseButton::Right);
        mbTable["Middle"].set(MouseButton::Middle);
        mbTable["Button3"].set(MouseButton::Button3);
        mbTable["Button4"].set(MouseButton::Button4);
        mbTable["Button5"].set(MouseButton::Button5);
        mbTable["Button6"].set(MouseButton::Button6);
        mbTable["Button7"].set(MouseButton::Button7);
        mbTable["None"].set(MouseButton::None);

        //register all the event classes
        ClassWrapper<Event> eventCW("Event");
        eventCW.
        addMemberFunction("name", LUANATIC_FUNCTION(&Event::name)).
        addMemberFunction("eventTypeID", LUANATIC_FUNCTION(&Event::eventTypeID)).
        addMemberFunction("stopPropagation", LUANATIC_FUNCTION(&Event::stopPropagation)).
        addMemberFunction("propagationStopped", LUANATIC_FUNCTION(&Event::propagationStopped));
        namespaceTable.registerClass(eventCW);

        ClassWrapper<MouseState> mouseStateCW("MouseState");
        mouseStateCW.
        addConstructor<stick::Float32, stick::Float32, stick::UInt32>().
        addConstructor<const MouseState&>();
        namespaceTable.registerClass(mouseStateCW);

        ClassWrapper<MouseEvent> mouseEventCW("MouseEvent");
        mouseEventCW.
        addMemberFunction("x", LUANATIC_FUNCTION(&MouseEvent::x)).
        addMemberFunction("y", LUANATIC_FUNCTION(&MouseEvent::y)).
        addMemberFunction("scrollX", LUANATIC_FUNCTION(&MouseEvent::scrollX)).
        addMemberFunction("scrollY", LUANATIC_FUNCTION(&MouseEvent::scrollY)).
        addMemberFunction("button", LUANATIC_FUNCTION(&MouseEvent::button)).
        addMemberFunction("mouseState", LUANATIC_FUNCTION(&MouseEvent::mouseState));
        namespaceTable.registerClass(mouseEventCW);

        ClassWrapper<MouseMoveEvent> mouseMoveEventCW("MouseMoveEvent");
        mouseMoveEventCW.
        addBase<MouseEvent>().
        addBase<Event>().
        addConstructor<const MouseState&>();
        namespaceTable.registerClass(mouseMoveEventCW);

        ClassWrapper<MouseDragEvent> mouseDragEventCW("MouseDragEvent");
        mouseDragEventCW.
        addBase<MouseEvent>().
        addBase<Event>();
        namespaceTable.registerClass(mouseDragEventCW);

        ClassWrapper<MouseDownEvent> mouseDownEventCW("MouseDownEvent");
        mouseDownEventCW.
        addBase<MouseEvent>().
        addBase<Event>();
        namespaceTable.registerClass(mouseDownEventCW);

        ClassWrapper<MouseUpEvent> mouseUpEventCW("MouseUpEvent");
        mouseUpEventCW.
        addBase<MouseEvent>().
        addBase<Event>();
        namespaceTable.registerClass(mouseUpEventCW);

        ClassWrapper<MouseScrollEvent> mouseScrollEventCW("MouseScrollEvent");
        mouseScrollEventCW.
        addBase<MouseEvent>().
        addBase<Event>();
        namespaceTable.registerClass(mouseScrollEventCW);

        ClassWrapper<MouseEnterEvent> mouseEnterEventCW("MouseEnterEvent");
        mouseEnterEventCW.
        addBase<MouseEvent>().
        addBase<Event>();
        namespaceTable.registerClass(mouseEnterEventCW);

        ClassWrapper<MouseLeaveEvent> mouseLeaveEventCW("MouseLeaveEvent");
        mouseLeaveEventCW.
        addBase<MouseEvent>().
        addBase<Event>();
        namespaceTable.registerClass(mouseLeaveEventCW);

        namespaceTable.
        registerFunction("addMouseMoveCallback", detail::luaEventCallback<MouseMoveEvent>).
        registerFunction("publish", LUANATIC_FUNCTION(&detail::luaPublish));
    }
}

namespace luanatic
{
    template<>
    struct ValueTypeConverter<box::Value>
    {
        static void push(lua_State * _state, const box::Value & _val)
        {
            lua_newtable(_state);
            lua_pushnumber(_state, _val.value);
            lua_setfield(_state, -2, "value");
            lua_pushstring(_state, _val.unit == box::Unit::Percent ? "%" : "px");
            lua_setfield(_state, -2, "unit");
        }

        static box::Value convertAndCheck(lua_State * _state, stick::Int32 _index)
        {
            if (!lua_istable(_state, _index))
            {
                const char * msg = lua_pushfstring(_state, "Table expected to conver to box::Value, got %s", luaL_typename(_state, _index));
                luaL_argerror(_state, _index, msg);
            }

            lua_getfield(_state, _index, "value");
            lua_getfield(_state, _index, "unit");
            return box::Value(luaL_checknumber(_state, -2), std::strcmp(luaL_checkstring(_state, -1), "%") == 0 ? box::Unit::Percent : box::Unit::Pixels);
        }
    };
}

// namespace luanatic
// {
//     //@TODO: Should this really be a value type converter or just a bound type?
//     //Value Converter implementation to seemlessly convert between lua values and a 2D vector
//     template<>
//     struct ValueTypeConverter<brick::Entity>
//     {
//         static void push(lua_State * _state, const brick::Entity & _entity)
//         {
//             lua_newtable(_state);
//             lua_pushlightuserdata(_state, (void *)_entity.version());
//             lua_setfield(_state, -2, "version");
//             lua_pushlightuserdata(_state, (void *)_entity.id());
//             lua_setfield(_state, -2, "id");
//             lua_pushlightuserdata(_state, (void *)_entity.hub());
//             lua_setfield(_state, -2, "hub");
//         }

// static brick::Entity convertAndCheck(lua_State * _state, stick::Int32 _index)
// {
//     if (!lua_istable(_state, _index))
//     {
//         const char * msg = lua_pushfstring(_state, "Table expected to conver to brick::Entity, got %s", luaL_typename(_state, _index));
//         luaL_argerror(_state, _index, msg);
//     }

//     return brick::Entity()
// }
//     };
// }

#endif //BOX_BOXLUA_HPP

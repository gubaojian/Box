#include <Box/Box.hpp>
#include <Box/MouseEvents.hpp>

#include <Stick/ErrorCodes.hpp>

namespace box
{
    using namespace stick;
    using namespace brick;

    namespace detail
    {
        bool isUndefined(Float _value)
        {
            return std::isnan(_value);
        }

        bool isWidthDefinite(Entity _e)
        {
            auto m = _e.maybe<comps::Width>();
            if (m && (*m).unit != Unit::Percent)
                return true;
            return false;
        }

        bool isHeightDefinite(Entity _e)
        {
            auto m = _e.maybe<comps::Height>();
            if (m && (*m).unit != Unit::Percent)
                return true;
            return false;
        }

        bool isHeightDefined(Entity _e)
        {
            auto v = _e.maybe<comps::Height>();
            return v && !isUndefined((*v).value);
        }

        bool isWidthDefined(Entity _e)
        {
            auto v = _e.maybe<comps::Width>();
            return v && !isUndefined((*v).value);
        }

        DirtyFlag dirty(Entity _e)
        {
            if (auto df = _e.maybe<comps::Dirty>())
            {
                return *df;
            }
            return DirtyFlag::NotDirty;
        }

        void markChildrenDirty(Entity _e)
        {
            auto mc = _e.maybe<comps::Children>();
            if (mc)
            {
                for (Entity c : *mc)
                {
                    if (isWidthDefined(c) || !isHeightDefined(c))
                        c.set<comps::Dirty>(DirtyFlag::Dirty);
                    else
                        c.set<comps::Dirty>(DirtyFlag::PositionDirty);

                    markChildrenDirty(c);
                }
            }
        }

        void markParentDirty(Entity _e)
        {
            auto p = _e.maybe<comps::Parent>();
            while (p)
            {
                if (!isWidthDefined(*p) || !isHeightDefined(*p))
                    (*p).set<comps::Dirty>(DirtyFlag::Dirty);
                else
                    (*p).set<comps::Dirty>(DirtyFlag::ChildrenDirty);

                auto mchildren = (*p).maybe<comps::Children>();
                for (Entity & child : *mchildren)
                {
                    if (child != _e)
                    {
                        child.set<comps::Dirty>(DirtyFlag::Dirty);
                        markChildrenDirty(child);
                    }
                }

                p = (*p).maybe<comps::Parent>();
            }
        }

        void markDirty(Entity _e)
        {
            // printf("A\n");
            // //if it's allready marked dirty, we done here
            // if (dirty(_e) == DirtyFlag::Dirty)
            //     return;

            // printf("B\n");

            //otherwise do da stuff
            _e.set<comps::Dirty>(DirtyFlag::Dirty);

            //check if we need to mark the parent dirty
            // auto p = _e.maybe<comps::Parent>();
            // if (p)
            // {
            //     printf("GOT PARENT %s\n", (*p).get<comps::Name>().cString());
            //     // if (!isWidthDefined(*p) || !isHeightDefined(*p))
            //     {
            //         printf("MARK PARENT DIRTY\n");
            //         markDirty(*p);
            //     }
            // }

            // // while (p)
            // // {
            // //     if (!isWidthDefined(*p) || !isHeightDefined(*p))
            // //         (*p).set<comps::Dirty>(true);
            // //     else
            // //         (*p).set<comps::ChildrenDirty>(true);
            // //     p = (*p).maybe<comps::Parent>();
            // // }
            markParentDirty(_e);
            markChildrenDirty(_e);
        }

        void removeFromParent(Entity _e)
        {
            auto mp = _e.maybe<comps::Parent>();
            if (mp)
            {
                if ((*mp).isValid())
                {
                    auto & children = (*mp).get<comps::Children>();
                    auto it = stick::find(children.begin(), children.end(), _e);
                    STICK_ASSERT(it != children.end());
                    children.remove(it);
                    (*mp).get<comps::EventHandler>()->removeForwarder(*_e.get<comps::EventHandler>());

                    _e.set<comps::Parent>(Entity());

                    //mark the parent dirty if its size is not defined
                    if (!isWidthDefined(*mp) || !isHeightDefined(*mp))
                    {
                        detail::markDirty(*mp);
                    }
                }
            }
        }

        void removeImpl(Entity _e, bool _bRemoveFromParent)
        {
            removeChildren(_e);
            if (_bRemoveFromParent)
                removeFromParent(_e);
            _e.destroy();
        }
    }

    Hub & defaultHub()
    {
        static Hub s_hub;
        return s_hub;
    }

    Entity createNode(const String & _name, Hub & _hub)
    {
        Entity node = _hub.createEntity();
        auto eh = makeUnique<EventHandler>();
        eh->setPassAlongArguments(node);

        eh->addEventFilter([](const MouseDownEvent & _e, Entity _self)
        {
            STICK_ASSERT(_self.hasComponent<comps::ComputedLayout>());
            return !_self.get<comps::ComputedLayout>().box.contains(_e.x(), _e.y());
        });

        eh->addEventFilter([](const MouseUpEvent & _e, Entity _self)
        {
            STICK_ASSERT(_self.hasComponent<comps::ComputedLayout>());
            return !_self.get<comps::ComputedLayout>().box.contains(_e.x(), _e.y());
        });

        eh->addEventFilter([](const MouseMoveEvent & _e, Entity _self)
        {
            STICK_ASSERT(_self.hasComponent<comps::ComputedLayout>());
            bool bContains = _self.get<comps::ComputedLayout>().box.contains(_e.x(), _e.y());

            // @TODO: Is it kinda dirty to detect the MouseEnter and MouseLeave event here?
            // works fine :)
            auto mbon = _self.maybe<comps::MouseOn>();
            if (!mbon && bContains)
            {
                _self.set<comps::MouseOn>(true);
                _self.get<comps::EventHandler>()->publish(MouseEnterEvent(_e.mouseState()), false);
            }
            else
            {
                // check if we need to fire the mouse leave event for any of the direct children
                // @Note: This is not super efficient. Maybe have a list of actively hovered items?
                // @Note2: We need to solve MouseLeave on the parent level to avoid the case where
                // we leave the parent and the child at the same time in which case the MouseMoveEvent
                // modifier of the child will never be reached and thus the MouseLeaveEvent never fire.
                auto mchildren = _self.maybe<comps::Children>();
                if (mchildren)
                {
                    for (Entity & child : *mchildren)
                    {
                        if (child.hasComponent<comps::MouseOn>() && !child.get<comps::ComputedLayout>().box.contains(_e.x(), _e.y()))
                        {
                            child.removeComponent<comps::MouseOn>();
                            child.get<comps::EventHandler>()->publish(MouseLeaveEvent(_e.mouseState()), false);
                        }
                    }
                }
            }
            // End Sketchy portion

            return !bContains;
        });

        eh->addEventFilter([](const MouseEnterEvent & _e, Entity _self)
        {
            STICK_ASSERT(_self.hasComponent<comps::ComputedLayout>());
            return !_self.get<comps::ComputedLayout>().box.contains(_e.x(), _e.y());
        });

        node.set<comps::EventHandler>(std::move(eh));

        if (_name.length())
            node.set<comps::Name>(_name);
        return node;
    }

    void addChild(Entity _e, Entity _child)
    {
        STICK_ASSERT(_e.isValid());
        STICK_ASSERT(_child.isValid());

        if (!_e.hasComponent<comps::Children>())
            _e.set<comps::Children>(EntityArray());

        //possibly remove from previous parent
        detail::removeFromParent(_child);

        _e.get<comps::Children>().append(_child);
        _e.get<comps::EventHandler>()->addForwarder(*_child.get<comps::EventHandler>());
        _child.set<comps::Parent>(_e);
    }

    void removeChildren(Entity _e)
    {
        if (_e.hasComponent<comps::Children>())
        {
            auto & cs = _e.get<comps::Children>();
            for (auto & child : cs)
                detail::removeImpl(child, false);
            cs.clear();
        }
    }

    bool removeChild(Entity _e, Entity _child)
    {
        if (_e.hasComponent<comps::Children>())
        {
            auto & cs = _e.get<comps::Children>();
            auto it = stick::find(cs.begin(), cs.end(), _child);
            if (it != cs.end())
            {
                _child.removeComponent<comps::Parent>();
                cs.remove(it);
                return true;
            }
        }
        return false;
    }

    void reverseChildren(Entity _e)
    {
        auto & cs = _e.get<comps::Children>();
        std::reverse(cs.begin(), cs.end());
    }

    void remove(Entity _e)
    {
        detail::removeImpl(_e, true);
    }

    void setSize(Entity _e, Float _width, Float _height, Unit _unit)
    {
        _e.set<comps::Width>(Value(_width, _unit));
        _e.set<comps::Height>(Value(_height, _unit));
        detail::markDirty(_e);
    }

    void setSize(Entity _e, Value _width, Value _height)
    {
        _e.set<comps::Width>(_width);
        _e.set<comps::Height>(_height);
        detail::markDirty(_e);
    }

    void setWidth(Entity _e, Float _width, Unit _unit)
    {
        _e.set<comps::Width>(Value(_width, _unit));
        detail::markDirty(_e);
    }

    void setHeight(Entity _e, Float _height, Unit _unit)
    {
        _e.set<comps::Height>(Value(_height, _unit));
        detail::markDirty(_e);
    }

    void setMinSize(Entity _e, Float _width, Float _height, Unit _unit)
    {
        _e.set<comps::MinWidth>(Value(_width, _unit));
        _e.set<comps::MinHeight>(Value(_height, _unit));
        detail::markDirty(_e);
    }

    void setMinSize(Entity _e, Value _width, Value _height)
    {
        _e.set<comps::MinWidth>(_width);
        _e.set<comps::MinHeight>(_height);
        detail::markDirty(_e);
    }

    void setMinWidth(Entity _e, Float _width, Unit _unit)
    {
        _e.set<comps::MinWidth>(Value(_width, _unit));
        detail::markDirty(_e);
    }

    void setMinHeight(Entity _e, Float _height, Unit _unit)
    {
        _e.set<comps::MinHeight>(Value(_height, _unit));
        detail::markDirty(_e);
    }

    void setMaxSize(Entity _e, Float _width, Float _height, Unit _unit)
    {
        _e.set<comps::MaxWidth>(Value(_width, _unit));
        _e.set<comps::MaxHeight>(Value(_height, _unit));
        detail::markDirty(_e);
    }

    void setMaxSize(Entity _e, Value _width, Value _height)
    {
        _e.set<comps::MaxWidth>(_width);
        _e.set<comps::MaxHeight>(_height);
        detail::markDirty(_e);
    }

    void setMaxWidth(Entity _e, Float _width, Unit _unit)
    {
        _e.set<comps::MaxWidth>(Value(_width, _unit));
        detail::markDirty(_e);
    }

    void setMaxHeight(Entity _e, Float _height, Unit _unit)
    {
        _e.set<comps::MaxHeight>(Value(_height, _unit));
        detail::markDirty(_e);
    }

    void setPadding(Entity _e, Float _padding, Unit _unit)
    {
        setPadding(_e, Value(_padding, _unit));
    }

    void setPadding(Entity _e, Value _value)
    {
        _e.set<comps::PaddingLeft>(_value);
        _e.set<comps::PaddingTop>(_value);
        _e.set<comps::PaddingRight>(_value);
        _e.set<comps::PaddingBottom>(_value);
        //@TODO: Optimize by only marking children dirty and self + parents with DirtyFlag::ChildrenDirty
        detail::markDirty(_e);
    }

    namespace detail
    {
        struct Line
        {
            DynamicArray<Entity> items;
        };

        Direction resolveDirection(Entity _e, Direction _parentDirection)
        {
            auto m = _e.maybe<comps::Direction>();
            if (m)
                return *m;
            else
                return _parentDirection;
        }

        template<class C, class T>
        T findComponentOr(Entity _e, T _or)
        {
            auto mo = _e.maybe<C>();
            if (mo) return *mo;
            return _or;
        }

        Overflow resolveOverflow(Entity _e)
        {
            return findComponentOr<comps::Overflow>(_e, Overflow::Visible);
        }

        Flow resolveFlow(Entity _e)
        {
            return findComponentOr<comps::Flow>(_e, Flow::Row);
        }

        Direction resolveDirection(Entity _e)
        {
            return findComponentOr<comps::Direction>(_e, Direction::LeftToRight);
        }

        Wrap resolveWrap(Entity _e)
        {
            return findComponentOr<comps::Wrap>(_e, Wrap::Normal);
        }

        Justify resolveJustify(Entity _e)
        {
            return findComponentOr<comps::Justify>(_e, Justify::Start);
        }

        AlignItems resolveAlignItems(Entity _e)
        {
            return findComponentOr<comps::AlignItems>(_e, AlignItems::Stretch);
        }

        AlignLines resolveAlignLines(Entity _e)
        {
            return findComponentOr<comps::AlignLines>(_e, AlignLines::Stretch);
        }

        Position resolvePosition(Entity _e)
        {
            return findComponentOr<comps::Position>(_e, Position::Relative);
        }

        Float resolveValue(const Value & _value, Float _parentSize)
        {
            if (_value.unit == Unit::Pixels)
                return _value.value;

            printf("PERCENTAGE %f\n", _parentSize);
            if (isUndefined(_parentSize))
                return 0.0;

            return _value.value * _parentSize * 0.01;
        }

        Float resolveWidth(Entity _e, Float _w, Float _parentWidth)
        {
            Float minval = resolveValue(_e.getOrDefault<comps::MinWidth>(Value(0.0f)), _parentWidth);
            Float maxval = resolveValue(_e.getOrDefault<comps::MaxWidth>(Value(std::numeric_limits<Float>::max())), _parentWidth);
            return std::min(std::max(_w, minval), maxval);
        }

        Float resolveHeight(Entity _e, Float _w, Float _parentHeight)
        {
            Float minval = resolveValue(_e.getOrDefault<comps::MinHeight>(Value(0.0f)), _parentHeight);
            Float maxval = resolveValue(_e.getOrDefault<comps::MaxHeight>(Value(std::numeric_limits<Float>::max())), _parentHeight);
            return std::min(std::max(_w, minval), maxval);
        }

        Size generation(Entity _e)
        {
            auto mcl = _e.maybe<comps::ComputedLayout>();
            return mcl ? (*mcl).generation : 0;
        }

        Size nextGeneration(Entity _e)
        {
            // printf("NEXT GEN %lu\n", generation(_e) + 1);
            return generation(_e) + 1;
        }

        Float layoutLine(Entity _e, Float _availableSpace)
        {
            //layoutImpl()
        }

        void layoutImpl(Entity _e,
                        Float _x, Float _y,
                        Float _availableWidth, Float _availableHeight,
                        Float _parentWidth, Float _parentHeight, Size _generation,
                        Error & _outError);

        struct SizeHolder
        {
            Value width;
            Value height;
        };

        using TmpSize = brick::Component<ComponentName("TmpSize"), SizeHolder>;

        void recursivelyMoveBy(Entity _e, Float _x, Float _y)
        {
            auto mcl = _e.maybe<comps::ComputedLayout>();
            STICK_ASSERT(mcl);
            (*mcl).box.moveBy(_x, _y);
            auto mchildren = _e.maybe<comps::Children>();
            if (mchildren)
            {
                auto & children = *mchildren;
                for (Entity & child : children)
                {
                    recursivelyMoveBy(child, _x, _y);
                }
            }
        }

        Vec2f generateLines(Entity _e, Float _x, Float _y, Float _availableWidth, Float _availableHeight,
                            Float _parentWidth, Float _parentHeight,
                            Size _generation, DynamicArray<Line> & _outLines, Error & _outError)
        {
            // printf("GENERATING LINES FOR %s\n", _e.get<comps::Name>().cString());
            auto mchildren = _e.maybe<comps::Children>();
            Float paddingLeft = resolveValue(_e.getOrDefault<comps::PaddingLeft>(Value(0.0)), _parentWidth);
            Float paddingTop = resolveValue(_e.getOrDefault<comps::PaddingTop>(Value(0.0)), _parentHeight);
            Float paddingRight = resolveValue(_e.getOrDefault<comps::PaddingRight>(Value(0.0)), _parentWidth);
            Float paddingBottom = resolveValue(_e.getOrDefault<comps::PaddingBottom>(Value(0.0)), _parentHeight);

            _x += paddingLeft;
            _y += paddingTop;
            _availableWidth -= paddingLeft + paddingRight;
            _availableHeight -= paddingTop + paddingBottom;
            Float aw = _availableWidth;
            Float currentX = _x;
            Float currentY = _y;
            if (mchildren && (*mchildren).count())
            {
                auto & children = *mchildren;
                Line currentLine;
                // printf("CHILD COUNT %lu\n", children.count());
                for (Entity & child : children)
                {
                    // printf("AW %f AH %f\n", aw, _availableHeight);
                    layoutImpl(child, currentX, _y, _availableWidth, _availableHeight, _parentWidth, _parentHeight, _generation, _outError);
                    //@TODO: Handle error?
                    STICK_ASSERT(child.hasComponent<comps::ComputedLayout>());
                    Float s = child.get<comps::ComputedLayout>().box.width();
                    // printf("DATA: %f %f\n", _availableWidth, s);
                    if (aw >= s)
                    {
                        currentLine.items.append(child);
                        aw -= s;
                        currentX += s; //@TODO:Margin + Border
                    }
                    else
                    {
                        // printf("MAKING NEW LINE YOOOO!!!\n");
                        _outLines.append(currentLine);
                        currentLine = Line();
                        child.get<comps::ComputedLayout>().box.setPosition(_x, _y);
                        currentLine.items.append(child);
                        aw = _availableWidth - s;
                        currentX = s + _x; //@TODO:Margin + Border
                    }
                }
                if (currentLine.items.count())
                    _outLines.append(currentLine);


                bool bRelayoutChildren = false;
                auto ma = resolveAlignItems(_e);
                auto mal = resolveAlignLines(_e);
                auto mj = _e.maybe<comps::Justify>();
                if (ma == AlignItems::Stretch || mal == AlignLines::Stretch)
                {
                    printf("I THINK WE STRETCH\n");
                    bRelayoutChildren = true;
                }

                Float allHeight = 0;
                for (Line & line : _outLines)
                {
                    Float lineHeight = 0;
                    for (Entity & c : line.items)
                    {
                        Rect & b = c.get<comps::ComputedLayout>().box;
                        b.setPosition(b.min().x, currentY);
                        if (b.height() > lineHeight)
                            lineHeight = b.height();
                    }

                    currentY += lineHeight;
                    allHeight += lineHeight;

                    if (ma == AlignItems::Stretch)
                    {
                        for (Entity & c : line.items)
                        {
                            Rect & b = c.get<comps::ComputedLayout>().box;
                            // Float minh = c.maybe<comps::
                            b.setSize(b.width(), resolveHeight(c, lineHeight, _parentHeight));
                        }
                    }

                    if (mj)
                    {
                        if (*mj != Justify::Start)
                        {
                            Float width = 0;
                            for (Entity & c : line.items)
                            {
                                width += c.get<comps::ComputedLayout>().box.width();
                            }

                            Float spaceLeft = _availableWidth - width;

                            if (*mj == Justify::SpaceBetween)
                            {
                                Float extraSpace = spaceLeft / (Float)(line.items.count() - 1);
                                for (stick::Size i = 1; i < line.items.count(); ++i)
                                {
                                    recursivelyMoveBy(line.items[i], extraSpace * i, 0);
                                }
                            }
                            else if (*mj == Justify::SpaceAround)
                            {
                                Float extraSpace = spaceLeft / (Float)(line.items.count() * 2);
                                Float moveBy = 0;
                                for (stick::Size i = 0; i < line.items.count(); ++i)
                                {
                                    moveBy += extraSpace;
                                    recursivelyMoveBy(line.items[i], moveBy, 0);
                                    moveBy += extraSpace;
                                }
                            }
                            else if (*mj == Justify::Center || *mj == Justify::End)
                            {
                                Float extraSpace = spaceLeft / 2;
                                if (*mj == Justify::End)
                                    extraSpace = spaceLeft;
                                for (stick::Size i = 0; i < line.items.count(); ++i)
                                {
                                    recursivelyMoveBy(line.items[i], extraSpace, 0);
                                }
                            }
                        }
                    }
                }

                if (mal == AlignLines::Stretch)
                {
                    Float left = _availableHeight - allHeight;
                    Float perLine = left / _outLines.count();
                    printf("%s LEFT %f %f %f %f\n", _e.get<comps::Name>().cString(), _availableHeight, allHeight, left, perLine);
                    Float delta = 0;
                    for (Line & line : _outLines)
                    {
                        for (Entity & c : line.items)
                        {
                            Rect & b = c.get<comps::ComputedLayout>().box;
                            b.setSize(b.width(), resolveHeight(c, b.height() + perLine, _parentHeight));
                            b.moveBy(0, delta);
                        }
                        delta += perLine;
                    }
                }

                if (bRelayoutChildren)
                {
                    printf("RELAYOUT CHILDREN\n");
                    for (Line & line : _outLines)
                    {
                        for (Entity & c : line.items)
                        {
                            Rect & b = c.get<comps::ComputedLayout>().box;

                            //@TODO: the mark dirty call is slow and ugly :(
                            DynamicArray<Line> lines;
                            markChildrenDirty(c);
                            generateLines(c, b.min().x, b.min().y, b.width(), b.height(), b.width(), b.height(), _generation, lines, _outError);
                            //@TODO: Handle error?
                        }
                    }
                }
            }
            return Vec2f(currentX, currentY);
        }

        void layoutImpl(Entity _e,
                        Float _x, Float _y,
                        Float _availableWidth, Float _availableHeight,
                        Float _parentWidth, Float _parentHeight, Size _generation,
                        Error & _outError)
        {
            // printf("layoutImpl %s %lu %lu %f %f\n", _e.get<comps::Name>().cString(), generation(_e), _generation, _x, _y);

            DirtyFlag df = dirty(_e);
            if (df == DirtyFlag::Dirty)
            {
                printf("LAYOUT SELF &  CHLDREN %s\n", _e.get<comps::Name>().cString());
                //Direction dir = resolveDirection(_e, _parentDirection);
                // auto mcl = _e.maybe<comps::ComputedLayout>();

                // //This node was allready layouted and has a fixed size...
                // if (generation(_e) == _generation && mcl && (*mcl).bFixedWidth && (*mcl).bFixedHeight)
                // {
                //     printf("GENERATION %lu\n", _generation);
                //     //...so at best we need to recursively adjust the positioning for this node
                //     //and all its children
                //     if (_x != (*mcl).box.min().x || _y != (*mcl).box.min().y)
                //     {
                //         // printf("MOVING BRO\n");
                //         recursivelyMoveBy(_e, _x - (*mcl).box.min().x, _y - (*mcl).box.min().y);
                //     }
                //     _e.set<comps::Dirty>(DirtyFlag::NotDirty);
                //     return;
                // }

                auto mw = _e.maybe<comps::Width>();
                auto mh = _e.maybe<comps::Height>();
                bool bWidthFixed = false;
                bool bHeightFixed = false;
                Float w, h;
                Float availableWidth = _availableWidth;
                Float availableHeight = _availableHeight;
                if (mw)
                {
                    availableWidth = resolveWidth(_e, resolveValue(*mw, _parentWidth), _parentWidth);
                    bWidthFixed = true;
                }

                if (mh)
                {
                    availableHeight = resolveHeight(_e, resolveValue(*mh, _parentHeight), _parentHeight);
                    bHeightFixed = true;
                }

                w = availableWidth;
                h = availableHeight;

                DynamicArray<Line> lines;
                //if this is a relative width item, we save the widht of the resulting line of its children
                // printf("GENERATING LINES %f %f %f %f\n", availableWidth, availableHeight, w, h);
                Vec2f pos = generateLines(_e, _x, _y, availableWidth, availableHeight, w, h, _generation, lines, _outError);

                //@TODO: Handle error?

                if (!bWidthFixed)
                {
                    w = resolveWidth(_e, pos.x - _x, _parentWidth);
                    // printf("NO FIXED W %f\n", w);
                }

                if (!bHeightFixed)
                {
                    // printf("SETTING TO AVAILABLE HEIGHT %s\n", _e.get<comps::Name>().cString());
                    h = resolveHeight(_e, pos.y - _y, _parentHeight);
                }

                // printf("%s X %f Y %f MW %f MH %f\n", _e.get<comps::Name>().cString(), _x, _y, w, h);
                _e.set<comps::ComputedLayout>(Rect(_x, _y, _x + w, _y + h), bWidthFixed, bHeightFixed, _generation);
            }
            else if (df == DirtyFlag::PositionDirty)
            {
                printf("MOVE BYYYY\n");
                auto mcl = _e.maybe<comps::ComputedLayout>();
                if (_x != (*mcl).box.min().x || _y != (*mcl).box.min().y)
                {
                    // printf("MOVING BRO\n");
                    recursivelyMoveBy(_e, _x - (*mcl).box.min().x, _y - (*mcl).box.min().y);
                }
            }
            else if (df == DirtyFlag::ChildrenDirty)
            {
                printf("LAYOUT  CHLDREN\n");
                DynamicArray<Line> lines;
                auto & box = _e.get<comps::ComputedLayout>().box;
                _e.get<comps::ComputedLayout>().generation = _generation;
                Vec2f pos = generateLines(_e, _x, _y, box.width(), box.height(), box.width(), box.height(), _generation, lines, _outError);
            }
            _e.set<comps::Dirty>(DirtyFlag::NotDirty);
        }
    }

    Error layout(Entity _e, Float _width, Float _height)
    {
        Error ret;
        auto mparent = _e.maybe<comps::Parent>();
        //this is the root node
        /*if(!mparent)
        {
            //make sure the root node provides the things we need
            auto mw = _e.maybe<comps::Width>();
            auto mh = _e.maybe<comps::Height>();
            if(!mw || !mh)
            {
                return Error(ec::InvalidOperation, "The root node needs to specify width and height", STICK_FILE, STICK_LINE);
            }
            if((*mw).unit == Unit::Percent || (*mh).unit == Unit::Percent)
            {
                return Error(ec::InvalidOperation, "The root node can't use relative units", STICK_FILE, STICK_LINE);
            }
        }*/

        detail::layoutImpl(_e, 0, 0, _width, _height, _width, _height, detail::nextGeneration(_e), ret);

        return ret;
    }

    String debugString(const Entity & _e, Size _indentLevel, DebugStringOptions _options)
    {
        String ret;
        ret.reserve(512);

        for (Size i = 0; i < _indentLevel; ++i)
            ret.append("    ");

        if (hasFields(_options, DebugStringOptions::ComputedLayout))
        {
            // //auto & ml = _e.maybe<comps::ComputedLayout>();
            // if(ml)
            // {
            //     //ret.append(AppendVariadicFlag(), "Computed Layout: {\n    ", "top: ", toString((*ml).min().x), "\ntoString((*ml).min().y))
            // }
        }

        return ret;
    }

    void addEventCallback(Entity _e, const EventHandler::Callback & _cb)
    {
        _e.get<comps::EventHandler>()->addEventCallback(_cb);
    }

    Entity nodeAtPosition(Entity _e, Float _x, Float _y)
    {
        STICK_ASSERT(_e.hasComponent<comps::ComputedLayout>());
        if (_e.get<comps::ComputedLayout>().box.contains(_x, _y))
        {
            // find the top most child that contains _x, _y
            auto mchildren = _e.maybe<comps::Children>();
            if (mchildren)
            {
                for (Entity & child : *mchildren)
                {
                    // @TODO: Get rid of recursive call to avoid max stack depth errors for
                    // hugely nested documents? (most likely not gonna be a real issue though)
                    Entity tmp = nodeAtPosition(child, _x, _y);
                    if (tmp)
                        return tmp;
                }

                //if none of the children were hit, return self
                return _e;
            }
            else
            {
                // if there are no children, return self
                return _e;
            }
        }

        // nothing hit braa
        return Entity();
    }
}

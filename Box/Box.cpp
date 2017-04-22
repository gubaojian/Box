#include <Box/Box.hpp>
#include <Stick/ErrorCodes.hpp>

namespace box
{
    using namespace stick;
    using namespace brick;

    namespace detail
    {
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
                    _e.set<comps::Parent>(Entity());
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

    bool isUndefined(Float _value)
    {
        return std::isnan(_value);
    }

    Hub & defaultHub()
    {
        static Hub s_hub;
        return s_hub;
    }

    Entity createNode(const String & _name, Hub & _hub)
    {
        Entity node = _hub.createEntity();
        node.set<comps::HubPointer>(&_hub);
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
            auto mo = findComponent<C>(_e);
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

        Align resolveAlign(Entity _e)
        {
            return findComponentOr<comps::Align>(_e, Align::Start);
        }

        Position resolvePosition(Entity _e)
        {
            return findComponentOr<comps::Position>(_e, Position::Relative);
        }

        Size generation(Entity _e)
        {
            auto mcl = _e.maybe<comps::ComputedLayout>();
            return mcl ? (*mcl).generation : 0;
        }

        Size nextGeneration(Entity _e)
        {
            return generation(_e) + 1;
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

        }

        Float layoutLine(Entity _e, Float _availableSpace)
        {
            //layoutImpl()
        }

        bool layoutImpl(Entity _e,
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
            printf("GENERATING LINES FOR %s\n", _e.get<comps::Name>().cString());
            auto mchildren = _e.maybe<comps::Children>();
            Float currentX = _x;
            Float currentY = _y;
            Float aw = _availableWidth;
            if (mchildren)
            {
                auto & children = *mchildren;
                Line currentLine;
                printf("CHILD COUNT %lu\n", children.count());
                for (Entity & child : children)
                {
                    printf("AW %f AH %f\n", aw, _availableHeight);
                    layoutImpl(child, currentX, _y, _availableWidth, _availableHeight, _parentWidth, _parentHeight, _generation, _outError);
                    //@TODO: Handle error?
                    STICK_ASSERT(child.hasComponent<comps::ComputedLayout>());
                    Float s = child.get<comps::ComputedLayout>().box.width();
                    printf("DATA: %f %f\n", _availableWidth, s);
                    if (aw >= s)
                    {
                        currentLine.items.append(child);
                        aw -= s;
                        currentX += s; //@TODO:Margin + Border
                    }
                    else
                    {
                        printf("MAKING NEW LINE YOOOO!!!\n");
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

                    auto ma = _e.maybe<comps::Align>();
                    if (!ma || *ma == Align::Stretch)
                    {
                        for (Entity & c : line.items)
                        {
                            printf("RELAYOUT STRETCH\n");
                            Rect & b = c.get<comps::ComputedLayout>().box;
                            b.setSize(b.width(), lineHeight);
                            DynamicArray<Line> lines;
                            generateLines(c, b.min().x, b.min().y, b.width(), b.height(), b.width(), b.height(), _generation, lines, _outError);
                            //@TODO: Handle error?
                        }
                    }

                    auto mj = _e.maybe<comps::Justify>();
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
                                if(*mj == Justify::End)
                                    extraSpace = spaceLeft;
                                for (stick::Size i = 0; i < line.items.count(); ++i)
                                {
                                    recursivelyMoveBy(line.items[i], extraSpace, 0);
                                }
                            }
                        }
                    }
                }
            }
            return Vec2f(currentX, currentY);
        }

        bool layoutImpl(Entity _e,
                        Float _x, Float _y,
                        Float _availableWidth, Float _availableHeight,
                        Float _parentWidth, Float _parentHeight, Size _generation,
                        Error & _outError)
        {
            printf("layoutImpl %s %lu %lu %f %f\n", _e.get<comps::Name>().cString(), generation(_e), _generation, _x, _y);

            //Direction dir = resolveDirection(_e, _parentDirection);
            auto mcl = _e.maybe<comps::ComputedLayout>();

            //This node was allready layouted and has a fixed size...
            if (generation(_e) == _generation && mcl && (*mcl).bFixedWidth && (*mcl).bFixedHeight)
            {
                //...so at best we need to recursively adjust the positioning for this node
                //and all its children
                if (_x != (*mcl).box.min().x || _y != (*mcl).box.min().y)
                {
                    printf("MOVING BRO\n");
                    recursivelyMoveBy(_e, _x - (*mcl).box.min().x, _y - (*mcl).box.min().y);
                }
                return false;
            }

            auto mw = _e.maybe<comps::Width>();
            auto mminw = _e.maybe<comps::MinWidth>();
            auto mmaxw = _e.maybe<comps::MaxWidth>();
            auto mh = _e.maybe<comps::Height>();
            auto mminh = _e.maybe<comps::MinHeight>();
            auto mmaxh = _e.maybe<comps::MinWidth>();
            bool bWidthFixed = false;
            bool bHeightFixed = false;
            Float w, h;
            Float availableWidth = _availableWidth;
            Float availableHeight = _availableHeight;
            if (mw)
            {
                printf("GOT A WIDTH\n");
                if ((*mw).unit == Unit::Pixels)
                {
                    availableWidth = (*mw).value;
                    bWidthFixed = true;
                }
                else
                {
                    if (!isUndefined(_parentWidth))
                    {
                        availableWidth = (*mw).value * _parentWidth * 0.01;
                        printf("SETTING PERCENTAGE WIDTH\n");
                        bWidthFixed = true;
                    }
                }
            }

            if (mh)
            {
                printf("GOT A HEIGHT\n");
                if ((*mh).unit == Unit::Pixels)
                {
                    availableHeight = (*mh).value;
                    bHeightFixed = true;
                }
                else
                {
                    if (!isUndefined(_parentHeight))
                    {
                        availableHeight = (*mh).value * _parentHeight * 0.1;
                        bWidthFixed = true;
                    }
                }
            }

            w = availableWidth;
            h = availableHeight;

            DynamicArray<Line> lines;
            //if this is a relative width item, we save the widht of the resulting line of its children
            printf("GENERATING LINES %f %f %f %f\n", availableWidth, availableHeight, w, h);
            Vec2f pos = generateLines(_e, _x, _y, availableWidth, availableHeight, w, h, _generation, lines, _outError);

            //@TODO: Handle error?

            if (!bWidthFixed)
            {
                w = pos.x - _x;
                printf("NO FIXED W %f\n", w);
            }

            if (mminw)
            {
                printf("GOT MINW\n");
                w = std::max((*mminw).value, w);
            }

            if (mmaxw)
            {
                printf("GOT MAXW\n");
                w = std::min((*mmaxw).value, w);
            }

            if (!bHeightFixed)
            {
                printf("SETTING TO AVAILABLE HEIGHT %s\n", _e.get<comps::Name>().cString());
                h = pos.y - _y;
            }

            if (mminh)
            {
                printf("GOT MINH\n");
                h = std::max((*mminh).value, h);
            }

            if (mmaxh)
            {
                printf("GOT MAXH\n");
                h = std::min((*mmaxh).value, h);
            }

            printf("%s X %f Y %f MW %f MH %f\n", _e.get<comps::Name>().cString(), _x, _y, w, h);
            _e.set<comps::ComputedLayout>(Rect(_x, _y, _x + w, _y + h), bWidthFixed, bHeightFixed, _generation);

            /*//if my size was relative, iterate over all children again and relayout with the now known dimensions
            if (!bHeightFixed || !bWidthFixed)
            {
                printf("LAST LAYOUT BRO\n");
                lines.clear();
                generateLines(_e, _x, _y, w, h, w, h, generation, lines, _outError);
            }*/

            return true;
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
}

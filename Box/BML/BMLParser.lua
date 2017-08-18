local lexer = require("BMLLexer")
local lpeg = require("lpeg")
local re = require("re")

local Node = require("BMLNode")

local function createNode(_name)
    if _name ~= nil then
        return Node(__coreNS.createNode(_name, __coreNS.hub))
    else
        return Node(__coreNS.createNode("", __coreNS.hub))
    end
end

local function printHierarchy(_node, _indent)
    local indentStr = string.rep("    ", _indent)
    print(indentStr .. "{")
    if _node.tag then print(indentStr .. "    tag = " .. _node.tag) end
    if _node.attributes ~= nil then 
        local attrString = indentStr .. "    attributes = {"
        for k,v in pairs(_node.attributes) do
            attrString = attrString.. " " .. k .. " = " .. v .. " "
        end
        attrString = attrString .. " }"
        print(attrString)
    end
    if #_node.children > 0 then
        print(indentStr .. "    children = ")
        for _, v in ipairs(_node.children) do
            printHierarchy(v, _indent + 1)
        end
    end
    print(indentStr .. "},")
end

local function parse(_bml)
    -- lex it
    local res = lexer.lex(_bml)

    -- get the namespace table for the core functions
    local ns = __coreNS

    -- check if there was a lexing error, if so return it!
    if type(res) == "string" then
        return res
    end

    printHierarchy(res, 0)

    -- otherwise we expect a table yo
    assert(type(res) == "table")

    -- create the box document based on the parsed hierarchy etc.

    -- recursive helper
    local function parseNode(_tbl)
        print("PARSE DA NODE ", _tbl, _tbl.tag)
        local name = nil
        if _tbl.attributes ~= nil then
            name = _tbl.attributes.name
        end
        local c = createNode(name)
        print("DA TYPE", type(_tbl.tag))
        __coreNS.setTag(c.handle, _tbl.tag)
        -- TODO: Parse attributes etc.

        if _tbl.attributes ~= nil then
            print("GOT ATTRIBUTES YOOOOOOOOO")
            for k,v in pairs(_tbl.attributes) do
                print("GOT ATTRIBUTE BRO ", k, v)
                c[k] = v
            end
        end

        if _tbl.children then
            print("child count", _tbl.tag, #_tbl.children)
            for _,v in ipairs(_tbl.children) do
                print("WHAT??", _tbl.tag, v)
                __coreNS.addChild(c.handle, parseNode(v).handle)
            end
        end

        print("PARSE DA NODE EEEEEEND ", _tbl.tag)
        return c
    end

    local document = createNode("document")
    for k,v in pairs(res.children) do
        __coreNS.addChild(document.handle, parseNode(v).handle)
    end
end

local tn = createNode("test")
print("MY IDDDDDDDD", tn.handle:id())
tn:addEventCallback("mouseMove", function(_event, _self)
    tn.width = "10%"
    tn.test = 99.5
end)

__coreNS.publish(tn.handle, core.MouseMoveEvent(core.MouseState(100, 100, 2)), false)

return parse

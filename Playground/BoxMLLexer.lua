-- local lpeg = require("lpeg")

-- local Whitespace = lpeg.S(" \t\r\n") ^ 0

-- local Digit = lpeg.R('09')
-- local ValidNameStart = lpeg.R('AZ', 'az', '\127\255') + lpeg.P('_')
-- local ValidName = ValidNameStart * (ValidNameStart + Digit + lpeg.P('.')) ^ 0
-- local Element = ValidName
-- local AttributesStart = lpeg.P("(")
-- local AttributesEnd = lpeg.P(")")
-- local NotAttributeStart = (1 - AttributesStart) ^ 0
-- local NotAttributeEnd = (1 - AttributesEnd) ^ 0
-- local AttributesContent = AttributesStart * NotAttributeEnd * AttributesEnd

-- local AttributeName = Whitespace * ValidName
-- local AttributeValueStartEnd = lpeg.P("'") + lpeg.P('"')
-- local NotAttributeValueStartEnd = (1 - AttributeValueStartEnd) ^ 0
-- local AttributeValue = AttributeValueStartEnd * NotAttributeValueStartEnd * AttributeValueStartEnd


-- local attributeParser = (NotAttributeStart * lpeg.C(AttributesContent)) ^ 0
-- local attributeValue = (NotAttributeValueStartEnd * lpeg.C(AttributeValue)) ^ 0

-- local Hierarchy = lpeg.P(function(input, index)
--     local indentLevel = 
-- end)

-- local ts = "el\n    child\n    child2"

-- local Digit = lpeg.R('09')
-- local Whitespace = lpeg.S(" \t\r\n")
-- local NoSpace = 1 - Whitespace
-- local Trim = Whitespace ^ 0 * lpeg.C((Whitespace ^ 0 * NoSpace ^ 1) ^ 0)
-- local ValidNameStart = lpeg.R('AZ', 'az', '\127\255') + lpeg.P('_')
-- local ValidName = ValidNameStart * (ValidNameStart + Digit + lpeg.P('.')) ^ 0
-- local currentIndent = 0
-- local currentNode = {}

-- local function pushIndent(_str, _pos, _indent)
--     currentIndent = _indent
-- end

-- local Indent = lpeg.P("    ") ^ 0
-- local Newline = lpeg.S("\n\r")
-- local Node = ValidName * lpeg.V("Block")^-1
-- local Block = lpeg.C((ValidName + Newline + -1) * lpeg.P(function(input, index)
--     return index
-- end))

-- -- local Grammar = lpeg.P({
-- --         "document",
-- --         document = lpeg.C(lpeg.P(Newline^0 * ValidName)^0)
-- --     })

-- local Grammar = lpeg.Cmt((Newline ^ -1 * ValidName)^0, function(_str, _index, _name)
--     print("DA NAME", _name, _str, "ARGH")
-- end)

-- -- Grammar = lpeg.C((Newline ^ -1 * ValidName)^0)

-- local res = Grammar:match("el\nchild1\nchild2\n")
-- print(res)

-- -- for k, v in pairs(res) do
-- --     print("{" .. k .. " = " .. v, "}")
-- -- end

-- function tprint (tbl, indent)
--   if not indent then indent = 0 end
--   for k, v in pairs(tbl) do
--     formatting = string.rep("    ", indent) .. k .. ": "
--     if type(v) == "table" then
--       print(formatting)
--       tprint(v, indent+1)
--     else
--       print(formatting .. v)
--     end
--   end
-- end

-- local white = lpeg.S(" \t\r\n") ^ 0

-- local integer = white * lpeg.R("09") ^ 1 / tonumber
-- local muldiv = white * lpeg.C(lpeg.S("/*"))
-- local addsub = white * lpeg.C(lpeg.S("+-"))

-- local function node(p)
--   return p / function(left, op, right)
--     return { op, left, right }
--   end
-- end

-- local calculator = lpeg.P({
--   "input",
--   input = lpeg.V("exp") * -1,
--   exp = lpeg.V("term") + lpeg.V("factor") + integer,
--   term = node((lpeg.V("factor") + integer) * addsub * lpeg.V("exp")),
--   factor = node(integer * muldiv * (lpeg.V("factor") + integer))
-- })

-- local result = calculator:match("1 * 2 + 3")

-- tprint(result, 0)

local lpeg = require("lpeg")
local re = require("re")

local SelectorView = luanatic.class()

SelectorView.__index = function(view, key)
    local field = SelectorView[key]
    if field then return field end

    local callProxy = {}
    local cpmt = {}
    function cpmt.__call(table, ...)
        print("DA CALL")
        for _, v in ipairs(view.children) do
            local fn = v[key]
            if fn then fn(v, ...) end
        end
    end
    setmetatable(callProxy, cpmt)

    print("RETURN CALL PROXY")
    return callProxy
    -- return nil
end

function SelectorView:__init(_tbl)
    self.children = _tbl
end

function SelectorView:someFunction()
    print("SOME FUNCTION BRO")
end

-- function SelectorView:call(_memberFn)
--     for _, v in ipairs(self.children) do
--         local fn = v[_memberFn]
--     end
-- end

local TestClass = luanatic.class()

function TestClass:__init(val)
    self.val = val
    print(self, self.val)
end

function TestClass:blub()
    print("BLUB", self, self.val)
end

-- local a = TestClass("abc")
-- local b = TestClass("def")
-- local c = TestClass("ghi")

-- a:blub()
-- b:blub()
-- c:blub()

local view = SelectorView({TestClass("a"), TestClass("b"), TestClass("c")})
view:someFunction()
view:blub()


local initialIndent = lpeg.P("\n")^0 * lpeg.Cg(lpeg.P(""), "indent")

local matchSameIndent = lpeg.Cmt(lpeg.Cb('indent'),
                                function(s, p, indent)
                                    local sub = s:sub(p, p + #indent - 1)
                                    if sub ~= indent then
                                        return false
                                    end
                                    return p + #indent
                                end)

local captureDeeperIndent = lpeg.Cg(matchSameIndent * lpeg.S(" \t")^1, "indent")

local function loc(str, where)
    local line, pos, linepos = 1, 1, 0
    while true do
        pos =  str:find("\n", pos, true)
        if pos and pos < where then
            line = line + 1
            linepos = pos
            pos = pos + 1
        else
            break
        end
    end
    return "line " .. line .. ", column " .. (where - linepos)
end

local function errorCall(str, pos, msg, state)
    print("FUUUUUCK", pos)
    if not state.msg then
        print("AAAAA", msg)
        state.msg = msg .. " at " .. loc (str, pos)
        state.pos = pos
    end
    return false
 end

 local function expectedErrorCall(str, pos, msg, state)
    print("FUUUUUCK2", pos)
    if not state.msg then
        print("AAAAA2", msg)
        state.msg = msg .. " expected, got " .. str:sub(pos, pos) .. " at " .. loc (str, pos)
        state.pos = pos
    end
    return false
 end

local function err(msg)
    return lpeg.Cmt (lpeg.Cc (msg) * lpeg.Carg (1), errorCall)
end

local function expectedErr(msg)
    return lpeg.Cmt (lpeg.Cc (msg) * lpeg.Carg (1), expectedErrorCall)
end


-- {:attributes: {| <attributes> |} :}

local hierarchy = re.compile([[
 hierarchy <- %initial_indent {|
   {:children: {| (<first_root> <next_root>*)? |} :}
 |} !.

 first_root <- <entry>
 next_root <- %nl <entry>

 entry <- {|
   {:tag: <tag> :}
   {:attributes: <fields> :} ?
   {:children: {| (<first_child> <next_child>*)? |} :}
 |}

 first_child <- %nl %capture_deeper_indent <entry>
 next_child <- %nl %match_same_indent <entry>

 identifier <- !%nl [a-zA-Z][a-zA-Z0-9_]*
 tag <- <identifier>

 fields <- {| "(" <kvpair>* ")" |} -> kvpairs_to_fields
 kvpair <- {| %s* {:key: <key> :} %s* '=' %s* {:value: <value> :} %s* |}
 key <- [a-zA-Z_] [a-zA-Z_0-9]*
 value <- <single_quoted> / <double_quoted>
 single_quoted <- ['] { [^']* } [']
 double_quoted <- ["] { [^"]* } ["]

]], {
 initial_indent = initialIndent,
 match_same_indent = matchSameIndent,
 capture_deeper_indent = captureDeeperIndent,
 kvpairs_to_fields = function(kvpairs)
   local attrs = {}
   for _, kv in ipairs(kvpairs) do
    print(kv.key, kv.value)
     attrs[kv.key] = kv.value
   end
   return attrs
 end
})


local function makeAttributeTable(tbl)
    local attrs = {}
    for _, kv in ipairs(tbl) do
        print(kv.key, kv.value)
        attrs[kv.key] = kv.value
    end
   return attrs
end

local sq = lpeg.P("'")
local dq = lpeg.P('"')
local sw = lpeg.P(" ")^0
local identifier = re.compile([[[a-zA-Z_][a-zA-Z0-9_]*]])
-- We don't emit an error here, as we only parse for attributes, if there was an open
-- paranthesis in the first place
local openParan = (sw * lpeg.P("("))
local closeParan = (sw * (lpeg.P(")") + err("Missing closing )")))

-- local status = {}
-- local res = lpeg.C(identifier):match("test.123='bla'", 1, status)
-- print("DA STATUS", status.msg, res)

local function pre(o) print("DA PREEEEE") return o end

local hierarchy = lpeg.P 
{
 'hierarchy',

 hierarchy = initialIndent
   *
   lpeg.Ct(
     lpeg.Cg(
       lpeg.Ct(
         (
           lpeg.V('first_root')
           *
           lpeg.V('next_root')^0
         )^-1
       ),
       'children'
     )
   )
   *
   lpeg.P(-1),

 first_root = lpeg.V('entry'),
 next_root = '\n' * lpeg.V('entry'),

 entry = lpeg.Ct(
   lpeg.Cg(lpeg.V('tag'), 'tag')
   *
   lpeg.Cg(lpeg.V('attributes'), 'attributes')^0
   *
   lpeg.Cg(
     lpeg.Ct(
       (
         lpeg.V('first_child')
         *
         lpeg.V('next_child')^0
       )^-1
     ),
     'children'
   )
 ),
 first_child = '\n' * captureDeeperIndent * lpeg.V('entry'),
 next_child = '\n' * matchSameIndent * lpeg.V('entry'),
 tag = identifier,
 attributes = openParan * lpeg.Ct(lpeg.V("kvpair")^1) / makeAttributeTable * closeParan * sw,
 kvpair = lpeg.Ct(sw * lpeg.Cg(lpeg.V("key"), "key") * sw * (lpeg.P("=") + expectedErr("=")) * sw * lpeg.Cg(lpeg.V("value"), "value")),
 key = identifier,
 value = lpeg.V("single_quoted") + lpeg.V("double_quoted"),
 single_quoted = sq * lpeg.C((1 - sq)^0) * (sq + expectedErr("'")),
 double_quoted = dq * lpeg.C((1 - dq)^0) * (dq + expectedErr('"'))
}

local blubber = {}
print("DOOOOOO IIIIIIIT")
local res = hierarchy:match("el(a = \"b\")\n    el(uno='dos')\n        el2(argj='fart'    bla=\"blaaaa\")", 1, blubber)

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

if not blubber.msg then
    printHierarchy(res, 0)
else
    print("Error: ", blubber.msg)
end

-- local attr = lpeg.P(
-- {
--  "attributes",

--  tag = lpeg.V("key") * lpeg.P(" ")^0,
--  attributes = lpeg.P("(") * lpeg.Ct(lpeg.V("kvpair")^1) * lpeg.P(")"),
--  kvpair = lpeg.Ct(lpeg.P(" ")^0  * lpeg.Cg(lpeg.V("key"), "key") * lpeg.P(" ")^0 * lpeg.P("=") * lpeg.P(" ")^0 * lpeg.Cg(lpeg.V("value"), "value")),
--  key = lpeg.R("az", "AZ") * (lpeg.R("az", "AZ", "09") + "_")^0,
--  value = lpeg.V("single_quoted") + lpeg.V("double_quoted"),
--  single_quoted = re.compile("['] { [^']* } [']"),
--  double_quoted = re.compile('["] { [^"]* } ["]')
-- }
-- )

-- local attrs = attr:match("( a =\"b\" c = 'd')")
-- print("DA ATS ", attrs, attrs[2].value, #attrs)

-- local attributes = re.compile([[
--     attributes <- {| <attribute> (" "+ <attribute>)+ |}
--     attribute <- {| {:<identifier>: <attributeValue> :} |}
--     attributeValue <- (" "+ "=" " "+ "'" {<identifier>} "'")
--     identifier <- (!%nl [a-zA-Z][a-zA-Z0-9_]*)+
--     ]]):match("a = 'b' c = 'd'")

-- print(test[1].a)

-- local fields = re.compile([[
--  fields <- {| "(" <kvpair>* ")" !. |} -> kvpairs_to_fields
--  kvpair <- {| %s* {:key: <key> :} %s* '=' %s* {:value: <value> :} %s* |}
--  key <- [a-zA-Z_] [a-zA-Z_0-9]*
--  value <- <single_quoted> / <double_quoted>
--  single_quoted <- ['] { [^']* } [']
--  double_quoted <- ["] { [^"]* } ["]
-- ]], {
--  kvpairs_to_fields = function(kvpairs)
--    local attrs = {}
--    for _, kv in ipairs(kvpairs) do
--      attrs[kv.key] = kv.value
--    end
--    return attrs
--  end;
-- })

-- for k,v in pairs(fields:match('(a="one" b = "two" c="three")')) do
--  print("KEY VAL", k, v)
-- end


function setSomething(_node, _something)
    print("SETTER", _node, _something)
end

function getSomething(_node)
    print("DA NODE", _node)
    return "DA GETTER"
end

local BMLNode = luanatic.class()

function BMLNode:__init(_nativeHandle)
    self.handle = _nativeHandle
end

function BMLNode:test()
    print("TEST")
end

function BMLNode:setTest(_bla)
    print("SETTING DA TEST", _bla)
end

local function firstToUpper(str)
    return (str:gsub("^%l", string.upper))
end

local mt = getmetatable(BMLNode)
mt.__index = function(_table, _key)
    print("INDEX")
    local ret = mt[_key]
    if ret ~= nil then return ret end
    return _G["get" .. firstToUpper(_key)](_table)
end

mt.__newindex = function(_table, _key, _value)
    local setterStr = "set" .. firstToUpper(_key)
    local setter = _G[setterStr]
    print("NEW INDEX", setterStr, _key, _value, setter)
    if setter ~= nil then 
        print("CALL")
        setter(_table, _value) 
    else
        print("SET")
        rawset(_table, _key, _value) 
     end
end

 rawset(BMLNode, "__newindex", mt.__newindex)

local node = BMLNode()

node:test()
print("WOOOOOP", node.something, mt)
node:setTest("DIETER")
node.something = "PETER"
print(node.something)

local function parseUnit(_str)
    local np, up = re.compile("{[0-9]*} %s* {'px' / '%'}?"):match(_str)
    local number = tonumber(np)
    print(np, up, number)
end

parseUnit("100 %")
print(__coreNS.setSize, core.setSize)

-- print("DA PACKAGE PATH", package.path)
local parse = require("BMLParser")

print("DOING DA PARSING")
print("DOING DA PARSING")
print("DOING DA PARSING")
print("DOING DA PARSING")
print("DOING DA PARSING")
-- parse("box(name='rootBox' width='10px' height='50%' marginLeft='10px' padding='10px')\n    box")




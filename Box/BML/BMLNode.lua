local lpeg = require("lpeg")
local re = require("re")
local util = require("Utilities")

local BMLNode = luanatic.class()

-- helper to parse a box value (i.e. 100px or 100%)
local captureNumber = lpeg.R("09")^1 / tonumber
local valueParse = re.compile("%s* {| {:value: %cn :} %s* {:unit: ('px' / '%')? :} |}", {cn = captureNumber})
local function parseValue(_str)
    -- local np, up = valueParse:match(_str)
    -- local number = tonumber(np)
    -- return {value = number, unit = up}
    return valueParse:match(_str)
end

-- same as parse value but parses up to four values (used by i.e. margin/padding which accept 1-4 values)
-- we still keep the single parseValue version for better speed
local fourValueParse = valueParse^1
local function parseValues(_str)
    print("PARSE DA VALUES", _str)
    -- local av, au, bv, bu, cv, cu, dv, du = fourValueParse:match(_str)
    -- print(av, au, bv, bu, cv, cu, dv, du)
    -- local ret = {}
    -- if av then table.insert(ret, {value = av, unit = au}) end
    -- if bv then table.insert(ret, {value = bv, unit = bu}) end
    -- if cv then table.insert(ret, {value = cv, unit = cu}) end
    -- if dv then table.insert(ret, {value = dv, unit = du}) end
    -- return ret
    return fourValueParse:match(_str)
end

-- -- if an attribute does not need any special parsing
-- -- we just call the setter with the string as the argument
-- local function defaultSetter(_node, _attributeName, _value)
--     _node[_attributeName] = _value
-- end

-- -- parses the content of an attribute to a Box value and 
-- -- sets it on the nodes attribute name.
-- local function parseValueSetter(_node, _attributeName, _value)
--     local val = parseValue(_value)
--     _node[_attributeName] = val
-- end

-- local function parseValuesSetter(_node, _attributeName, _values)
--     local values = parseValues(_values)
--     if #values == 1 then
--         _node[_attributeName] = values[1]
--     elseif #values == 2 then
--         _node[_attributeName] = values[1] values[2]
--     elseif #values == 3 then
--         _node[_attributeName] = values[1] values[2] values[3]
--     elseif #values == 4 then
--         _node[_attributeName] = values[1] values[2] values[3] values[4]
--     end
-- end

-- a table that maps attribute names to specialized parse functions
local specializedAttributeParsers = 
{
    width = parseValue,
    height = parseValue,
    marginLeft = parseValue,
    marginTop = parseValue,
    marginRight = parseValue,
    marginBottom = parseValue,
    paddingLeft = parseValue,
    paddingTop = parseValue,
    paddingRight = parseValue,
    paddingBottom = parseValue,
    margin = parseValues,
    padding = parseValues
}

local function passThrough(_arg)
    return _arg
end

-- helper to convert the first letter of a string to upper
local function firstToUpper(str)
    return (str:gsub("^%l", string.upper))
end

-- we overwrite the __index and __newindex metamethods
-- to forward attributes to setters/getters that call into
-- the c++ core API
--
-- This will i.e. internally call core.setWidth(node, 100) 
-- for an expression like node.width = 100
-- and core.width(node) for an expression like node.width
-- __coreNS is the core API namespace table. This is where
-- it expects the functions to be!
local mt = getmetatable(BMLNode)
mt.__index = function(_table, _key)
    local ret = mt[_key]
    if ret ~= nil then return ret end
    return __coreNS["get" .. firstToUpper(_key)](_table)
end

mt.__newindex = function(_table, _key, _value)
    local setterStr = "set" .. firstToUpper(_key)
    local setter = __coreNS[setterStr]
    -- if we found a setter, call it
    -- otherwise we set the field in the table
    -- to _value 
    if setter ~= nil then
        print("SETTER", setterStr, setter)
        local argFunc = passThrough
        local sp = specializedAttributeParsers[_key]
        if sp then argFunc = sp end
        local a, b, c, d = argFunc(_value)
        print("DA VALS", a.value, a.unit)
        if a and b and c and d then
            setter(_table.handle, a, b, c, d)
        elseif a and b and c then
            setter(_table.handle, a, b, c)
        elseif a and b then
            setter(_table.handle, a, b)
        else
            setter(_table.handle, a)
        end
    else
        rawset(_table, _key, _value) 
    end
end

-- forward the __newindex call to the class metatable
rawset(BMLNode, "__newindex", mt.__newindex)


-- actual member functions of BMLNode
function BMLNode:__init(_nativeHandle)
    self.handle = _nativeHandle
    self.eventCallbacksMap = {}
end

local function isNativeEvent(_type)
    if _type == "mouseMove" then
        return true
    end
    return false
end

function BMLNode:addEventCallback(_type, _cb)
    local tbl = self.eventCallbacksMap[_type]
    if tbl == nil then
        tbl = {}
        self.eventCallbacksMap[_type] = tbl
    end
    if isNativeEvent(_type) then
        local coreAdderName = "add" .. firstToUpper(_type) .. "Callback"
        local cbid = __coreNS[coreAdderName](self.handle, function(_event, _self)
            _cb(_event, self)
        end)
        print("ADDED DA EVENT CALLBAAAAACK!")
        table.insert(tbl, {callback = _cb, id = cbid})
    end
end

function BMLNode:removeEventCallback(_type, _cb)
    local tbl = self.eventCallbacksMap[_type]
    if tbl ~= nil then
        util.removeIf(tbl, function(key, value)
            if value.callback == _cb then
                __coreNS.removeEventCallback(self.handle, value.id)
            end
        end)
    end
end

return BMLNode

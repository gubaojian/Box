-- @TODO: Comment me
local lpeg = require("lpeg")
local re = require("re")

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
local closeParan = (sw * (lpeg.P(")") + expectedErr(")")))

local hierarchy = lpeg.P 
{
 'hierarchy',

 hierarchy = initialIndent
   *
   lpeg.Ct(
     lpeg.Cg(
       lpeg.Ct(
         (
           lpeg.V('firstRoot')
           *
           lpeg.V('nextRoot')^0
         )^-1
       ),
       'children'
     )
   )
   *
   lpeg.P(-1),

 firstRoot = lpeg.V('entry'),
 nextRoot = '\n' * lpeg.V('entry'),

 entry = lpeg.Ct(
   lpeg.Cg(lpeg.V('tag'), 'tag')
   *
   lpeg.Cg(lpeg.V('attributes'), 'attributes')^0
   *
   lpeg.Cg(
     lpeg.Ct(
       (
         lpeg.V('firstChild')
         *
         lpeg.V('nextChild')^0
       )^-1
     ),
     'children'
   )
 ),
 firstChild = '\n' * captureDeeperIndent * lpeg.V('entry'),
 nextChild = '\n' * matchSameIndent * lpeg.V('entry'),
 tag = identifier,
 attributes = openParan * lpeg.Ct(lpeg.V("kvpair")^1) / makeAttributeTable * closeParan * sw,
 kvpair = lpeg.Ct(sw * lpeg.Cg(lpeg.V("key"), "key") * sw * (lpeg.P("=") + expectedErr("=")) * sw * lpeg.Cg(lpeg.V("value"), "value")),
 key = identifier,
 value = lpeg.V("singleQuoted") + lpeg.V("doubleQuoted"),
 singleQuoted = sq * lpeg.C((1 - sq)^0) * (sq + expectedErr("'")),
 doubleQuoted = dq * lpeg.C((1 - dq)^0) * (dq + expectedErr('"'))
}

local function lex(_bml)
  local status = {}
  local ret = hierarchy:match(_bml, 1, status)
  if status.msg then return status.msg else return ret end
end

return {lex = lex}

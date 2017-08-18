local SelectorView = luanatic.class()

-- we implement a custom __index function to possibly forward
-- a function call to all children
SelectorView.__index = function(view, key)
    local field = SelectorView[key]
    if field then return field end

    local callProxy = {}
    local cpmt = {}
    function cpmt.__call(table, ...)
        for _, v in ipairs(view.children) do
            local fn = v[key]
            if fn then fn(v, ...) end
        end
    end
    setmetatable(callProxy, cpmt)

    return callProxy
end

function SelectorView:__init(_table)
    self.children = _table
end

return SelectorView

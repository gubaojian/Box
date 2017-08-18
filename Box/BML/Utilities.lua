local util = {}

util.find = function(_table, _value)

end

util.remove = function(_table, _value)
    for i,v in ipairs(_table) do
        if v == _value then
            table.remove(_table, i)
            return i
        end
    end
end

util.removeIf = function(_table, _func)
    for k,v in pairs(_table) do
        if _func(k, v) then
            _table[k] = nil
            return
        end
    end
end

return util

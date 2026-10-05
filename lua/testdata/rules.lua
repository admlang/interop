-- The module the binding tests declare functions from.
local M = {}
local calls = 0

function M.total(items)
    local sum = 0
    for _, item in ipairs(items) do sum = sum + item.price * item.quantity end
    return sum
end

function M.count()
    calls = calls + 1
    return calls
end

function M.greet(name, excited) return "hello " .. name .. (excited and "!" or "") end

function M.maybe(n) if n > 0 then return n end end

function M.orElse(value, fallback) return value or fallback end

function M.checksum(bytes)
    local sum = 0
    for i = 1, #bytes do sum = (sum + bytes:byte(i)) & 0xff end
    return sum
end

function M.squares(n)
    local out = {}
    for i = 1, n do out[i] = (i - 1) * (i - 1) end
    return out
end

function M.halves(n)
    local out = {}
    for i = 1, n do out[i] = (i - 1) / 2 end
    return out
end

function M.words(text)
    local out = {}
    for word in text:gmatch("%S+") do out[#out + 1] = word end
    return out
end

function M.flags() return { true, false, true } end

function M.reversed(bytes) return bytes:reverse() end

function M.big() return 1 << 62 end

function M.tooBig() return 300 end

function M.refuse(reason) error(reason, 0) end

function M.nothing() end

function M.summary(item) return { label = item.name .. " x" .. item.quantity, tags = item.tags } end

function M.receipt(items, note)
    local tags, byName = {}, {}
    for i, item in ipairs(items) do
        tags[i] = item.name
        byName[item.name] = item.price
    end
    return {
        label = #items .. " lines",
        lines = #items,
        paid = true,
        total = M.total(items),
        tags = tags,
        first = items[1],
        all = items,
        byName = byName,
        note = note,
    }
end

function M.receipts(items)
    local out = {}
    for i, item in ipairs(items) do out[i] = M.receipt({ item }) end
    return out
end

function M.priceList(items)
    local out = {}
    for _, item in ipairs(items) do out[item.name] = item.price end
    return out
end

function M.index(items)
    local out = {}
    for _, item in ipairs(items) do out[item.name] = item end
    return out
end

function M.wrongShape() return { label = {} } end

M.notAFunction = 42

return M

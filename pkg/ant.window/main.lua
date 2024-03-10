local function start(initargs)
    local platform = require "bee.platform"
    local exclusive = {}
    if platform.os == "ios" then
        exclusive[#exclusive+1] = "ant.window|ios"
    end
    exclusive[#exclusive+1] = "timer"
    dofile "/engine/ltask.lua" {
        bootstrap = { "ant.window|boot", initargs },
        exclusive = exclusive,
        worker_bind = {
            "ant.window|window",
            "ant.hwi|bgfx",
        },
    }
end

local function newproxy(t, k)
    local ltask = require "ltask"
    local ServiceWindow = ltask.queryservice "ant.window|window"

    local function reboot(initargs)
        ltask.send(ServiceWindow, "reboot", initargs)
    end

    local function set_cursor(cursor)
        ltask.call(ServiceWindow, "set_cursor", cursor)
    end

    local function set_title(title)
        ltask.call(ServiceWindow, "set_title", title)
    end

    t.reboot = reboot
    t.set_cursor = set_cursor
    t.set_title = set_title
    return t[k]
end

return setmetatable({ start = start }, { __index = newproxy })
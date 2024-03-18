local ecs = ...
local world = ecs.world
local w = world.w
local bgfx      = require "bgfx"
local imaterial = ecs.require "ant.render|material"
local dynamic2d_sys = ecs.system "dynamic2d_system"
local assetmgr  = import_package "ant.asset"
local ltask = require "ltask"
local ServiceResource = ltask.queryservice "ant.resource_manager|resource"
local idq = {}

local function get_memory(width, height, clear)
    local t = {}
    for i = 1, width do
        for j = 1, height do
            for k = 1, #clear do
                t[#t+1] = clear[k]
            end
        end
    end
    return bgfx.memory_buffer("bbbb", t)
end

function idq.update_pixels(eid, pixels)
    if not pixels or #pixels == 0 then return end
    local de = world:entity(eid, "dynamicquad:update")
    local dq = de.dynamicquad
    local tw, th, memory, handle = dq.tw, dq.th, dq.memory, dq.handle
    for _, pixel in ipairs(pixels) do
        local pos, value = pixel.pos, pixel.value
        local px, py = pos.x, pos.y
        assert(px >= 1 and px <= tw and py >= 1 and py <= th)
        local pitch = (py - 1) * tw + px - 1
        memory[pitch*4 + 1] = ("BBBB"):pack(table.unpack(value))
    end
    bgfx.update_texture2d(handle, 0, 0, 0, 0, tw, th, memory)
end

function dynamic2d_sys:component_init()
    for e in w:select "INIT dynamicquad:in viewrect2d?out dynamicquad_ready?out eid:in" do
        local dq = e.dynamicquad
        local tex = assetmgr.resource(dq.texture)
        local id, tw, th = tex.id, tex.texinfo.width, tex.texinfo.height
        local clear = dq.clear or {0, 0, 0, 0}
        if (not dq.width) or (not dq.height) then
            dq.width, dq.height = tw, th
        end
        local texture_content = ltask.call(ServiceResource, "texture_content", id)
        e.viewrect2d = {
            x=0, y=0, w=dq.width, h=dq.height
        }
        e.dynamicquad_ready = true
        local memory = get_memory(tw, th, clear)
        dq.id, dq.tw, dq.th, dq.memory, dq.handle = id, tw, th, memory, texture_content.handle
    end

end

function dynamic2d_sys:entity_ready()
    for e in w:select "dynamicquad_ready:update dynamicquad:in filter_material:in" do
        local dq = e.dynamicquad
        local id, tw, th, memory, handle = dq.id, dq.tw, dq.th, dq.memory, dq.handle
        bgfx.update_texture2d(handle, 0, 0, 0, 0, tw, th, memory)
        imaterial.set_property(e, "s_basecolor", id)
    end
end

function dynamic2d_sys:entity_remove()
    for e in w:select "REMOVED dynamicquad:in" do
        bgfx.destroy(e.dynamicquad.id)
        bgfx.destroy(e.dynamicquad.memory)
    end
end

return idq
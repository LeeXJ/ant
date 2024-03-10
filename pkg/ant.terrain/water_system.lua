local ecs   = ...
local world = ecs.world
local w     = world.w

local renderpkg = import_package "ant.render"
local layoutmgr = renderpkg.layoutmgr

local irender   = ecs.require "ant.render|render"

local bgfx      = require "bgfx"

local layout    = layoutmgr.get "p3|t2"

local water_sys = ecs.system "water_system"
local layoutfmt<const> = "fffff"    --p3|t2

--[[
    1-----2
    |     |
    0-----3
]]

local function create_indices_buffer(gw, gh)
    return {
        start = 0,
        num = gw * gh * 6,
        handle = irender.quad_ib(),
    }
end

local function gen_water_grid_mesh(gw, gh, unit, height)
    local h = height or 0.0
    h = h * unit
    local vw, vh = gw+1, gh+1
    local stride = layout.stride
    local s = vw*vh*stride
    local m = bgfx.memory_buffer(s)

    --vertex number == (gw+1)*(gh+1)
    for ih=0, gh do
        for iw=0, gw do
            local vi = ih*vw+iw
            local midx= vi*stride+1
            local u, v = iw/gw, ih/gh
            m[midx]   = layoutfmt:pack(iw, h, ih, u, v)
        end
    end
    return {
        vb = {
            start = 0,
            num = vw * vh,
            declname = "p3|t2",
            handle = bgfx.create_vertex_buffer(m, layout.handle),
        },
        ib = create_indices_buffer(gw, gh),
    }
end

function water_sys:component_init()
    for e in w:select "INIT water:in mesh_result:out owned_mesh_buffer?out" do
        local water = e.water
        local gw, gh = water.grid_width, water.grid_height
        local unit = water.unit
        e.mesh_result = gen_water_grid_mesh(gw, gh, unit)
        e.owned_mesh_buffer = true
    end
end
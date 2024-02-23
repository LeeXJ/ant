local aio = import_package "ant.io"
local fontutil = require "font.util"
local fastio = require "fastio"
local bgfx = require "bgfx"
local viewIdPool = require "viewid_pool"

local ImGui = require "imgui"
local ImGuiBackend = require "imgui.backend"
local ImGuiAnt = {}
---@class ImGuiIO
local ImGuiIO

local FontAtlas = {}

local function glyphRanges(t)
	assert(#t % 2 == 0)
	local s = {}
	for i = 1, #t do
		s[#s+1] = ("<I4"):pack(t[i])
	end
	s[#s+1] = "\x00\x00\x00"
	return table.concat(s)
end

function ImGuiAnt.FontAtlasClear()
    FontAtlas = {}
end

function ImGuiAnt.FontAtlasAddFont(config)
    if config.SystemFont then
        FontAtlas[#FontAtlas+1] = {
            FontData = fontutil.systemfont(config.SystemFont),
            SizePixels = config.SizePixels,
            GlyphRanges = glyphRanges(config.GlyphRanges),
        }
        return
    end
    FontAtlas[#FontAtlas+1] = {
        FontData = aio.readall_v(config.FontPath),
        SizePixels = config.SizePixels,
        GlyphRanges = glyphRanges(config.GlyphRanges),
    }
end

function ImGuiAnt.FontAtlasBuild()
    local atlas = ImGui.GetIO().Fonts
    atlas.Clear()
    local _, ImFontConfig = ImGui.FontConfig()
    ImFontConfig.FontDataOwnedByAtlas = false
    for i, config in ipairs(FontAtlas) do
        local data, size = fastio.wrap(config.FontData)()
        ImFontConfig.MergeMode = i > 1
        atlas.AddFontFromMemoryTTF(data, size, config.SizePixels, ImFontConfig, config.GlyphRanges)
    end
    atlas.Build()
    ImGuiBackend.RenderCreateFontsTexture()
    for _, config in ipairs(FontAtlas) do
        fastio.free(config.FontData)
    end
    FontAtlas = {}
end

local ImGuiEvent = {}

local CaptureMouse = {}

function ImGuiEvent.mouseclick(e)
    local btn = 0
    if e.what == "LEFT" then
        btn = 0
    elseif e.what == "RIGHT" then
        btn = 1
    elseif e.what == "MIDDLE" then
        btn = 2
    end
    if e.state == "DOWN" then
        ImGuiIO.AddMouseButtonEvent(btn, true)
        if ImGuiIO.WantCaptureMouse then
            CaptureMouse[e.what] = true
        end
        return ImGuiIO.WantCaptureMouse
    elseif e.state == "UP" then
        local capture = CaptureMouse[e.what]
        CaptureMouse[e.what] = nil
        ImGuiIO.AddMouseButtonEvent(btn, false)
        return capture
    end
end

function ImGuiEvent.mousemove(e)
    local capture
    for btn in pairs(CaptureMouse) do
        e.what[btn] = nil
        capture = true
    end
    return capture and (next(e.what) == nil)
end

function ImGuiEvent.mousewheel(e)
    ImGuiIO.AddMouseWheelEvent(0, e.delta)
    return ImGuiIO.WantCaptureMouse
end

function ImGuiEvent.keyboard(e)
    ImGuiIO.AddKeyEvent(ImGui.Mod.Ctrl, e.state.CTRL ~= nil);
    ImGuiIO.AddKeyEvent(ImGui.Mod.Shift, e.state.SHIFT ~= nil);
    ImGuiIO.AddKeyEvent(ImGui.Mod.Alt, e.state.ALT ~= nil);
    ImGuiIO.AddKeyEvent(ImGui.Mod.Super, e.state.SYS ~= nil);
    if e.press == 1 then
        ImGuiIO.AddKeyEvent(e.key, true);
    elseif e.press == 0 then
        ImGuiIO.AddKeyEvent(e.key, false);
    end
    return ImGuiIO.WantCaptureKeyboard
end

function ImGuiEvent.inputchar(e)
    if e.what == "native" then
        ImGuiIO.AddInputCharacter(e.code)
    elseif e.what == "utf16" then
        ImGuiIO.AddInputCharacterUTF16(e.code)
    end
end

function ImGuiEvent.focus(e)
    ImGuiIO.AddFocusEvent(e.focused)
end

function ImGuiAnt.DispatchEvent(e)
    ImGuiIO = ImGui.GetIO()
    local func = ImGuiEvent[e.type]
    return func and func(e)
end

function ImGuiAnt.SetViewClear(...)
    for _, viewid in ipairs(viewIdPool) do
        bgfx.set_view_clear(viewid, ...)
    end
end

return ImGuiAnt

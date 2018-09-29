dofile "libs/init.lua"
local elog = require "editor.log"
local inputmgr = require "inputmgr"
local mapiup = require "inputmgr.mapiup"
local bgfx = require "bgfx"
local rhwi = require "render.hardware_interface"
local scene = require "scene.util"
local fs_util = require "filesystem.util"
local eu = require "editor.util"
require "iuplua"

local fb_width, fb_height = 1024, 768
local canvas = iup.canvas {
	rastersize = fb_width .. "x" .. fb_height
}

local dlg = iup.dialog {
	iup.split {
		canvas,
		elog.window,
		SHOWGRIP = "NO",
	},
	margin = "4x4",
	size = "HALFxHALF",
	shrink = "Yes",
	title = "Model",
}

local input_queue = inputmgr.queue(mapiup)
eu.regitster_iup(input_queue, canvas)

dlg:showxy(iup.CENTER, iup.CENTER)
dlg.usersize = nil

rhwi.init(iup.GetAttributeData(canvas,"HWND"), fb_width, fb_height)
local module_description_file = "mem://model_main_window.module"
fs_util.write_to_file(module_description_file, [[
modules = {
	"modelloader.renderworld",
	"modelloader.camera_controller",
	"editor.ecs.editor_component",
	--"editor.ecs.general_editor_entities",
}
]])
scene.start_new_world(input_queue, fb_width, fb_height, {module_description_file, "engine.module"})

if iup.MainLoopLevel() == 0 then
	iup.MainLoop()
	iup.Close()
	bgfx.shutdown()
end

local ecs = ...
local world     = ecs.world
local w         = world.w
local math3d    = require "math3d"
local imgui     = require "imgui"
local rhwi      = import_package "ant.hwi"
local asset_mgr = import_package "ant.asset"
local effekseer_filename_mgr = ecs.import.interface "ant.effekseer|filename_mgr"
local irq       = ecs.import.interface "ant.render|irenderqueue"
local ies       = ecs.import.interface "ant.scene|ientity_state"
local iom       = ecs.import.interface "ant.objcontroller|obj_motion"
local icoll     = ecs.import.interface "ant.collision|collider"
local drawer    = ecs.import.interface "ant.render|iwidget_drawer"
local isp 		= ecs.import.interface "ant.render|isystem_properties"
local iwd       = ecs.import.interface "ant.render|iwidget_drawer"
local resource_browser  = ecs.require "widget.resource_browser"
local anim_view         = ecs.require "widget.animation_view"
local material_view     = ecs.require "widget.material_view"
local toolbar           = ecs.require "widget.toolbar"
local scene_view        = ecs.require "widget.scene_view"
local inspector         = ecs.require "widget.inspector"
local gridmesh_view     = ecs.require "widget.gridmesh_view"
local prefab_view       = ecs.require "widget.prefab_view"
local prefab_mgr        = ecs.require "prefab_manager"
prefab_mgr.set_anim_view(anim_view)
local menu              = ecs.require "widget.menu"
local camera_mgr        = ecs.require "camera_manager"
local gizmo             = ecs.require "gizmo.gizmo"
local uiconfig          = require "widget.config"
local vfs       = require "vfs"
local access    = require "vfs.repoaccess"
local fs        = require "filesystem"
local lfs       = require "filesystem.local"
local hierarchy = require "hierarchy_edit"
local global_data = require "common.global_data"
local gizmo_const = require "gizmo.const"
local new_project = require "common.new_project"
local widget_utils = require "widget.utils"
local utils = require "common.utils"
local log_widget = require "widget.log"(asset_mgr)
local console_widget = require "widget.console"(asset_mgr)
local m = ecs.system 'gui_system'
local drag_file = nil

local second_view_width = 384
local second_vew_height = 216

local function on_new_project(path)
    new_project.set_path(path)
    new_project.gen_mount()
    new_project.gen_init_system()
    new_project.gen_main()
    new_project.gen_package_ecs()
    new_project.gen_package()
    new_project.gen_settings()
    new_project.gen_bat()
    new_project.gen_prebuild()
end

local function choose_project_dir()
    local filedialog = require 'filedialog'
    local dialog_info = {
        Owner = rhwi.native_window(),
        Title = "Choose project folder"
    }
    local ok, path = filedialog.open(dialog_info)
    if ok then
        return path[1]
    end
end

local function get_package(entry_path, readmount)
    local repo = {_root = entry_path}
    if readmount then
        access.readmount(repo)
    end
    local packages = {}
    for _, name in ipairs(repo._mountname) do
        vfs.mount(name, repo._mountpoint[name]:string())
        local key = name
        local skip = false
        if utils.start_with(name, "/pkg/ant.") then
            if name ~= "/pkg/ant.resources" and name ~= "/pkg/ant.resources.binary" then
                skip = true
            end
        end
        if not skip then
            packages[#packages + 1] = {name = key, path = repo._mountpoint[name]}
        end
    end
    global_data.repo = repo
    return packages
end

local function choose_project()
    if global_data.project_root then return end
    local title = "Choose project"
    if not imgui.windows.IsPopupOpen(title) then
        imgui.windows.OpenPopup(title)
    end
    local change, opened = imgui.windows.BeginPopupModal(title, imgui.flags.Window{"AlwaysAutoResize", "NoClosed"})
    if change then
        imgui.widget.Text("Create new or open existing project.")
        if imgui.widget.Button("Create project") then
            local path = choose_project_dir()
            if path then
                local lpath = lfs.path(path)
                local not_empty
                for path in fs.pairs(lpath) do
                    not_empty = true
                    break
                end
                if not_empty then
                    log_widget.error({tag = "Editor", message = "folder not empty!"})
                else
                    global_data.project_root = lpath
                    on_new_project(path)
                    global_data.packages = get_package(lfs.absolute(global_data.project_root), true)
                end
            end
        end
        imgui.cursor.SameLine()
        if imgui.widget.Button("Open project") then
            local path = choose_project_dir()
            if path then
                local lpath = lfs.path(path)
                if lfs.exists(lpath / ".mount") then
                    global_data.project_root = lpath
                    global_data.packages = get_package(lfs.absolute(global_data.project_root), true)
                    --file server
                    local cthread = require "bee.thread"
                    cthread.newchannel "log_channel"
                    cthread.newchannel "fileserver_channel"
                    cthread.newchannel "console_channel"
                    local produce = cthread.channel "fileserver_channel"
                    produce:push(arg, path)
                    local lthread = require "editor.thread"
                    fileserver_thread = lthread.create [[
                        package.path = "engine/?.lua"
                        require "bootstrap"
                        local fileserver = dofile "/pkg/tools.prefab_editor/fileserver_adapter.lua"()
                        fileserver.run()
                    ]]
                    log_widget.init_log_receiver()
                    console_widget.init_console_sender()
                    local topname
                    for _, package in ipairs(global_data.packages) do
                        if package.path == global_data.project_root then
                            topname = package.name
                            break
                        end
                    end
                    if topname then
                        global_data.package_path = topname .. "/"
                        effekseer_filename_mgr.add_path(global_data.package_path .. "res")
                    else
                        print("Can not add effekseer resource seacher path.")
                    end
                else
                    log_widget.error({tag = "Editor", message = "no project exist!"})
                end
            end
        end
        imgui.cursor.SameLine()
        if imgui.widget.Button("Quit") then
            local res_root_str = tostring(fs.path "":localpath())
            global_data.project_root = lfs.path(string.sub(res_root_str, 1, #res_root_str - 1))
            global_data.packages = get_package(global_data.project_root, true)
            imgui.windows.CloseCurrentPopup();
        end
        if global_data.project_root then
            local fw = require "bee.filewatch"
            fw.add(global_data.project_root:string())
            local res_root_str = tostring(fs.path "":localpath())
            global_data.editor_root = fs.path(string.sub(res_root_str, 1, #res_root_str - 1))
            effekseer_filename_mgr.add_path("/pkg/tools.prefab_editor/res")
        end
        imgui.windows.EndPopup()
    end
end

local fileserver_thread

local function show_dock_space(offset_x, offset_y)
    local viewport = imgui.GetMainViewport()
    imgui.windows.SetNextWindowPos(viewport.WorkPos[1] + offset_x, viewport.WorkPos[2] + offset_y)
    imgui.windows.SetNextWindowSize(viewport.WorkSize[1] - offset_x, viewport.WorkSize[2] - offset_y)
    imgui.windows.SetNextWindowViewport(viewport.ID)
	imgui.windows.PushStyleVar(imgui.enum.StyleVar.WindowRounding, 0.0);
	imgui.windows.PushStyleVar(imgui.enum.StyleVar.WindowBorderSize, 0.0);
    imgui.windows.PushStyleVar(imgui.enum.StyleVar.WindowPadding, 0.0, 0.0);
    local wndflags = imgui.flags.Window {
        "NoDocking",
        "NoTitleBar",
        "NoCollapse",
        "NoResize",
        "NoMove",
        "NoBringToFrontOnFocus",
        "NoNavFocus",
        "NoBackground",
    }
    local dockflags = imgui.flags.DockNode {
        "NoDockingInCentralNode",
        "PassthruCentralNode",
    }
    if not imgui.windows.Begin("DockSpace Demo", wndflags) then
        imgui.windows.PopStyleVar(3)
        imgui.windows.End()
        return
    end
    imgui.dock.Space("MyDockSpace", dockflags)
    local x,y,w,h = imgui.dock.BuilderGetCentralRect("MyDockSpace")
    imgui.windows.PopStyleVar(3)
    imgui.windows.End()
    return x,y,w,h
end
local iRmlUi    = ecs.import.interface "ant.rmlui|rmlui"
local irq       = ecs.import.interface "ant.render|irenderqueue"
local bgfx      = require "bgfx"
local stat_window
local dock_x, dock_y, dock_width, dock_height
local last_x = -1
local last_y = -1
local last_width = -1
local last_height = -1
function m:ui_update()
    imgui.windows.PushStyleVar(imgui.enum.StyleVar.WindowRounding, 0)
    imgui.windows.PushStyleColor(imgui.enum.StyleCol.WindowBg, 0.2, 0.2, 0.2, 1)
    imgui.windows.PushStyleColor(imgui.enum.StyleCol.TitleBg, 0.2, 0.2, 0.2, 1)
    choose_project()
    widget_utils.show_message_box()
    menu.show()
    toolbar.show()
    dock_x, dock_y, dock_width, dock_height = show_dock_space(0, uiconfig.ToolBarHeight)
    scene_view.show()
    gridmesh_view.show()
    prefab_view.show()
    inspector.show()
    resource_browser.show()
    anim_view.show()
    console_widget.show()
    log_widget.show()
    imgui.windows.PopStyleColor(2)
    imgui.windows.PopStyleVar()
    
    --drag file to view
    if imgui.util.IsMouseDragging(0) then
        local x, y = imgui.util.GetMousePos()
        if (x > last_x and x < (last_x + last_width) and y > last_y and y < (last_y + last_height)) then
            if not drag_file then
                local dropdata = imgui.widget.GetDragDropPayload()
                if dropdata and (string.sub(dropdata, -7) == ".prefab"
                    or string.sub(dropdata, -4) == ".efk" or string.sub(dropdata, -4) == ".glb") then
                    drag_file = dropdata
                end
            end
        else
            drag_file = nil
        end
    else
        if drag_file then
            world:pub {"AddPrefabOrEffect", drag_file}
            drag_file = nil
        end
    end

    if not stat_window then
        local iRmlUi = ecs.import.interface "ant.rmlui|rmlui"
        stat_window = iRmlUi.open "bgfx_stat.rml"
    end
    local bgfxstat = bgfx.get_stats("sdcpnmtv")
    if bgfxstat then
        stat_window.postMessage(string.format("DrawCall: %d\nTriangle: %d\nTexture: %d\ncpu(ms): %f\ngpu(ms): %f\nfps: %d", bgfxstat.numDraw, bgfxstat.numTriList, bgfxstat.numTextures, bgfxstat.cpu, bgfxstat.gpu, bgfxstat.fps))
    end
end

local hierarchy_event = world:sub {"HierarchyEvent"}
local drop_files_event = world:sub {"OnDropFiles"}
local entity_event = world:sub {"EntityEvent"}
local event_keyboard = world:sub{"keyboard"}
local event_open_prefab = world:sub {"OpenPrefab"}
local event_preopen_prefab = world:sub {"PreOpenPrefab"}
local event_open_fbx = world:sub {"OpenFBX"}
local event_add_prefab = world:sub {"AddPrefabOrEffect"}
local event_resource_browser = world:sub {"ResourceBrowser"}
local event_window_title = world:sub {"WindowTitle"}
local event_create = world:sub {"Create"}
local event_gizmo = world:sub {"Gizmo"}

local light_gizmo = ecs.require "gizmo.light"

local aabb_color_i <const> = 0x6060ffff
local aabb_color <const> = {1.0, 0.38, 0.38, 1.0}
local highlight_aabb = {
    visible = false,
    min = nil,
    max = nil,
}
local function update_heightlight_aabb(e)
    if e then
        w:sync("render_object?in", e)
        local ro = e.render_object
        if ro and ro.aabb then
            local minv, maxv = math3d.index(ro.aabb, 1, 2)
            highlight_aabb.min = math3d.tovalue(minv)
            highlight_aabb.max = math3d.tovalue(maxv)
        end
        highlight_aabb.visible = true
    else
        highlight_aabb.visible = false
    end

end

local function on_target(old, new)
    local old_entity = old--type(old) == "table" and icamera.find_camera(old) or world[old]
    if old and old_entity then
        world.w:sync("camera?in", old_entity)
        world.w:sync("light?in", old_entity)
        if old_entity.camera then
            camera_mgr.show_frustum(old, false)
        elseif old_entity.light then
            light_gizmo.bind(nil)
        end
    end
    if new then
        local new_entity = new--type(new) == "table" and icamera.find_camera(new) or world[new]
        world.w:sync("camera?in", new_entity)
        world.w:sync("light?in", new_entity)
        if new_entity.camera then
            camera_mgr.set_second_camera(new, true)
        elseif new_entity.light then
            light_gizmo.bind(new)
        end
    end
    world:pub {"UpdateAABB", new}
    anim_view.bind(new)
end

local function on_update(e)
    update_heightlight_aabb(e)
    if not e then return end
    world.w:sync("camera?in", e)
    world.w:sync("light?in", e)
    if e.camera then
        camera_mgr.update_frustrum(e)
    elseif e.light then
        light_gizmo.update()
    end
    inspector.update_template_tranform(e)
end

local cmd_queue = ecs.require "gizmo.command_queue"
local event_update_aabb = world:sub {"UpdateAABB"}
function m:handle_event()
    for _, e in event_update_aabb:unpack() do
        update_heightlight_aabb(e)
    end
    for _, action, value1, value2 in event_gizmo:unpack() do
        if action == "update" or action == "ontarget" then
            inspector.update_ui(action == "update")
            if action == "ontarget" then
                on_target(value1, value2)
            elseif action == "update" then
                on_update(gizmo.target_eid)
            end
        end
    end
    for _, what, target, v1, v2 in entity_event:unpack() do
        local transform_dirty = true
        local template = hierarchy:get_template(target)
        if what == "move" then
            gizmo:set_position(v2)
            cmd_queue:record {action = gizmo_const.MOVE, eid = target, oldvalue = v1, newvalue = v2}
        elseif what == "rotate" then
            gizmo:set_rotation(math3d.quaternion{math.rad(v2[1]), math.rad(v2[2]), math.rad(v2[3])})
            cmd_queue:record {action = gizmo_const.ROTATE, eid = target, oldvalue = v1, newvalue = v2}
        elseif what == "scale" then
            gizmo:set_scale(v2)
            cmd_queue:record {action = gizmo_const.SCALE, eid = target, oldvalue = v1, newvalue = v2}
        elseif what == "name" or what == "tag" then
            transform_dirty = false
            if what == "name" then 
                hierarchy:update_display_name(target, v1)
                world.w:sync("collider?in", target)
                if target.collider then
                    hierarchy:update_collider_list(world)
                end
            else
                world.w:sync("slot?in", target)
                if target.slot then
                    hierarchy:update_slot_list(world)
                end
            end
        elseif what == "parent" then
            hierarchy:set_parent(target, v1)
            local sourceWorldMat = iom.worldmat(target)
            local targetWorldMat = v1 and iom.worldmat(v1) or math3d.matrix{}
            iom.set_srt(target, math3d.mul(math3d.inverse(targetWorldMat), sourceWorldMat))
            ecs.method.set_parent(target, v1)
            local isslot
            if v1 then
                world.w:sync("slot?in", v1)
                isslot = v1.slot
            end
            if (not v1 or isslot) then
                for e in world.w:select "scene:in slot_name:out" do
                    if e.scene == target.scene then
                        e.slot_name = "None"
                    end
                end
            end
        end
        if transform_dirty then
            on_update(target)
        end
    end
    for _, what, eid, value in hierarchy_event:unpack() do
        if what == "visible" then
            hierarchy:set_visible(eid, value)
            ies.set_state(eid, what, value)
            local template = hierarchy:get_template(eid)
            if template and template.children then
                for _, e in ipairs(template.children) do
                    ies.set_state(e, what, value)
                end
            end
            world.w:sync("light?in", eid)
            if eid.light then
                world:pub{"component_changed", "light", eid, "visible", value}
            end
            for e in world.w:select "scene:in ibl:in" do
                if e.scene.parent == eid then
                    isp.enable_ibl(value)
                    break
                end
            end
        elseif what == "lock" then
            hierarchy:set_lock(eid, value)
        elseif what == "delete" then
            world.w:sync("collider?in", gizmo.target_eid)
            if gizmo.target_eid.collider then
                anim_view.on_remove_entity(gizmo.target_eid)
            end
            prefab_mgr:remove_entity(eid)
            update_heightlight_aabb()
        elseif what == "movetop" then
            hierarchy:move_top(eid)
        elseif what == "moveup" then
            hierarchy:move_up(eid)
        elseif what == "movedown" then
            hierarchy:move_down(eid)
        elseif what == "movebottom" then
            hierarchy:move_bottom(eid)
        end
    end
    
    for _, filename in event_preopen_prefab:unpack() do
        anim_view:clear()
    end
    for _, filename in event_open_prefab:unpack() do
        prefab_mgr:open(filename)
    end
    for _, filename in event_open_fbx:unpack() do
        prefab_mgr:open_fbx(filename)
    end
    for _, filename in event_add_prefab:unpack() do
        if string.sub(filename, -4) == ".efk" then
            prefab_mgr:add_effect(filename)
        else
            prefab_mgr:add_prefab(filename)
        end
    end
    for _, files in drop_files_event:unpack() do
        on_drop_files(files)
    end

    for _, what in event_resource_browser:unpack() do
        if what == "dirty" then
            resource_browser.dirty = true
        end
    end

    for _, what in event_window_title:unpack() do
        local title = "PrefabEditor - " .. what
        imgui.SetWindowTitle(title)
        gizmo:set_target(nil)
    end

    for _, key, press, state in event_keyboard:unpack() do
        if key == "DELETE" and press == 1 then
            world:pub { "HierarchyEvent", "delete", gizmo.target_eid }
        elseif state.CTRL and key == "S" and press == 1 then
            prefab_mgr:save_prefab()
        elseif state.CTRL and key == "R" and press == 1 then
            anim_view:clear()
            prefab_mgr:reload()
        end
    end

    for _, what, type in event_create:unpack() do
        prefab_mgr:create(what, type)
    end
end

function m:start_frame()
    local viewport = imgui.GetMainViewport()
    local io = imgui.IO
    local current_mx = io.MousePos[1] - viewport.MainPos[1]
    local current_my = io.MousePos[2] - viewport.MainPos[2]
    if global_data.mouse_pos_x ~= current_mx or global_data.mouse_pos_y ~= current_my then
        global_data.mouse_pos_x = current_mx
        global_data.mouse_pos_y = current_my
        global_data.mouse_move = true
    end
end

function m:end_frame()
    local dirty = false
    if last_x ~= dock_x then last_x = dock_x dirty = true end
    if last_y ~= dock_y then last_y = dock_y dirty = true  end
    if last_width ~= dock_width then last_width = dock_width dirty = true  end
    if last_height ~= dock_height then last_height = dock_height dirty = true  end
    if dirty then
        local mvp = imgui.GetMainViewport()
        local viewport = {x = dock_x - mvp.WorkPos[1], y = dock_y - mvp.WorkPos[2] + uiconfig.MenuHeight, w = dock_width, h = dock_height}
        irq.set_view_rect("main_queue", viewport)

        local secondViewport = {x = viewport.x + (dock_width - second_view_width), y = viewport.y + (dock_height - second_vew_height), w = second_view_width, h = second_vew_height}
        irq.set_view_rect(camera_mgr.second_view, secondViewport)
        world:pub {"ViewportDirty", viewport}
    end
    global_data.mouse_move = false
end

function m:data_changed()
    if highlight_aabb.visible and highlight_aabb.min and highlight_aabb.max then
        iwd.draw_aabb_box(highlight_aabb, nil, aabb_color_i)
    end
end

package.path = "engine/?.lua"
require "bootstrap"
import_package "ant.window".start {
    import = {
        "@ant.test.light",
    },
    pipeline = {
        "init",
        "update",
        "exit",
    },
    system = {
        "ant.test.light|init_system",
    },
    interface = {
        "ant.objcontroller|iobj_motion",
    },
    policy = {
        "ant.general|name",
        "ant.scene|scene_object",
        "ant.render|render",
        "ant.render|render_queue",
    }
}
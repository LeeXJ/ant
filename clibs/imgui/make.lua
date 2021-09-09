local lm = require "luamake"

dofile "../common.lua"
lm:import "../luabind/build.lua"

lm:lua_dll "imgui" {
    deps = {
        "sdl",
        "luabind"
    },
    includes = {
        ".",
        Ant3rd .. "imgui",
        Ant3rd .. "glm",
        Ant3rd .. "SDL/include",
        BgfxInclude,
        "../bgfx",
        "../luabind"
    },
    sources = {
        "imgui_config.cpp",
        "imgui_renderer.cpp",
        "imgui_platform.cpp",
        "imgui_window.cpp",
        "luaimgui_tables.cpp",
        "luaimgui.cpp",
        "widgets/*.cpp",
        Ant3rd .. "imgui/imgui_draw.cpp",
        Ant3rd .. "imgui/imgui_tables.cpp",
        Ant3rd .. "imgui/imgui_widgets.cpp",
        Ant3rd .. "imgui/imgui.cpp",
        Ant3rd .. "imgui/backends/imgui_impl_sdl.cpp",
    },
    defines = {
        "GLM_FORCE_QUAT_DATA_XYZW",
        "IMGUI_DISABLE_OBSOLETE_FUNCTIONS",
        "IMGUI_DISABLE_METRICS_WINDOW",
        "IMGUI_DISABLE_DEMO_WINDOWS",
        "IMGUI_DISABLE_DEFAULT_ALLOCATORS",
        "IMGUI_USER_CONFIG=\\\"imgui_config.h\\\"",
        "_UNICODE",
    },
    windows = {
        defines = {
            "UNICODE",
            "IMGUI_ENABLE_WIN32_DEFAULT_IME_FUNCTIONS",
        },
        links = {
            "user32",
            "shell32",
            "ole32",
            "imm32",
            "dwmapi",
            "gdi32",
            "uuid"
        },
    },
    macos = {
        sources = "platform/imgui_osx.mm",
    }
}

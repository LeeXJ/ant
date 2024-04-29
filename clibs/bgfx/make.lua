local lm = require "luamake"

lm:import "bgfx.lua"

lm:lua_src "bgfx" {
    confs = { "bgfx" },
    deps = {
        "bx",
        "copy_bgfx_shader",
    },
    includes = {
        lm.AntDir.."/3rd/bee.lua/3rd/lua-seri"
    },
    sources = {
        "*.c",
        "*.cpp"
    },
    msvc = {
        flags = {
            "-wd4244",
            "-wd4267",
        }
    },
}

-- 这段 Lua 代码主要实现了以下功能：

-- 1. 导入了 luamake 库，用于构建项目。
-- 2. 导入了一个名为 "common.lua" 的公共配置文件，可能包含了项目的一些通用配置信息。
-- 3. 导入了一个名为 "bgfx.lua" 的配置文件，可能包含了与 bgfx 相关的配置信息。
-- 4. 配置了一个名为 "bgfx" 的源文件集合，指定了依赖项、包含目录、源文件、宏定义和编译器选项。
-- 5. 通过设置依赖项确保在构建该源文件集合前会先构建依赖的项目。
-- 6. 指定了源文件集合的源文件和包含目录，以及根据构建模式设置的宏定义。
-- 7. 对于 MSVC 编译器，设置了一些编译器选项，包括了禁止特定的警告。
   
-- 综上所述，该代码片段是一个构建脚本，用于配置和构建一个 bgfx 相关的项目。
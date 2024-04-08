local lm = require "luamake"

lm.BgfxDir = lm.AntDir.."/3rd/bgfx"
lm.BxDir = lm.AntDir.."/3rd/bx"
lm.BimgDir = lm.AntDir.."/3rd/bimg"

lm:conf {
    defines = {
        lm.mode == "debug" and "BGFX_CONFIG_DEBUG_UNIFORM=0",
        "BGFX_CONFIG_ENCODER_API_ONLY=1",
    }
}

lm:import(lm.AntDir.."/3rd/bgfx.luamake/use.lua")

lm:copy "copy_bgfx_shader" {
    inputs = {
        lm.BgfxDir .. "/src/bgfx_shader.sh",
        lm.BgfxDir .. "/src/bgfx_compute.sh",
        lm.BgfxDir .. "/examples/common/shaderlib.sh",
    },
    outputs = {
        lm.AntDir .. "/pkg/ant.resources/shaders/bgfx_shader.sh",
        lm.AntDir .. "/pkg/ant.resources/shaders/bgfx_compute.sh",
        lm.AntDir .. "/pkg/ant.resources/shaders/shaderlib.sh",
    }
}

-- 这段 Lua 代码用于构建一个项目，并进行以下操作：

-- 1. 设置了 `lm.BgfxDir`、`lm.BxDir` 和 `lm.BimgDir` 分别指向 bgfx、bx 和 bimg 库的目录路径。
-- 2. 导入了 bgfx 库的构建配置文件，通过 `lm:import` 函数引入了 `"luamake.AntDir/3rd/bgfx.luamake/use.lua"` 文件。
-- 3. 声明了一个拷贝任务 `copy_bgfx_shader`，用于将指定的 shader 脚本文件从 bgfx 目录复制到项目目录中的指定路径。
-- 4. 指定了拷贝任务的输入文件列表，包括了 `bgfx_shader.sh`、`bgfx_compute.sh` 和 `shaderlib.sh` 三个文件。
-- 5. 指定了拷贝任务的输出文件列表，分别指定了复制到项目目录中的目标路径。

-- 综上所述，该代码片段实现了将 bgfx 库的 shader 脚本文件复制到项目目录的指定位置的功能。
local lm = require "luamake"

lm.rootdir = lm.AntDir.."/3rd/bee.lua"

local OS = {
    "win",
    "posix",
    "osx",
    "linux",
    "bsd",
}

local function need(lst)
    local map = {}
    if type(lst) == "table" then
        for _, v in ipairs(lst) do
            map[v] = true
        end
    else
        map[lst] = true
    end
    local t = {}
    for _, v in ipairs(OS) do
        if not map[v] then
            t[#t+1] = "!bee/**/*_"..v..".cpp"
            t[#t+1] = "!bee/"..v.."/**/*.cpp"
        end
    end
    return t
end

lm:source_set "bee" {
    sources = "3rd/fmt/format.cc",
}

lm:source_set "bee" {
    includes = {
        ".",
        "3rd/lua",
    },
    sources = "bee/**/*.cpp",
    windows = {
        sources = need "win"
    },
    macos = {
        sources = {
            "bee/**/*.mm",
            need {
                "osx",
                "posix",
            }
        }
    },
    ios = {
        sources = {
            "bee/**/*.mm",
            "!bee/filewatch/**/",
            need {
                "osx",
                "posix",
            }
        }
    },
    linux = {
        sources = need {
            "linux",
            "posix",
        }
    },
    android = {
        sources = need {
            "linux",
            "posix",
        }
    }
}

lm:lua_src "bee" {
    includes = {
        "3rd/lua-seri",
        "."
    },
    defines = "BEE_STATIC",
    sources = "binding/*.cpp",
    windows = {
        defines = "_CRT_SECURE_NO_WARNINGS",
        sources = {
            "binding/port/lua_windows.cpp",
        },
        links = {
            "ntdll",
            "ws2_32",
            "ole32",
            "user32",
            "version",
            "synchronization",
        },
    },
    mingw = {
        links = {
            "uuid",
            "stdc++fs"
        }
    },
    linux = {
        links = {
            "pthread",
        }
    },
    macos = {
        frameworks = {
            "Foundation",
            "CoreFoundation",
            "CoreServices",
        }
    },
    ios = {
        sources = {
            "!binding/lua_filewatch.cpp",
        },
        frameworks = {
            "Foundation",
        }
    },
}

-- 这段 Lua 代码实现了以下功能：

-- 1. 引入了 luamake 库。
-- 2. 定义了一些操作系统名称。
-- 3. 实现了一个函数 `need`，用于根据操作系统列表生成源文件匹配规则。
-- 4. 配置了名为 "bee" 的源文件集合，包括指定不同操作系统下的源文件或排除规则。
-- 5. 配置了另一个名为 "bee" 的 Lua 源文件集合，同样包括指定不同操作系统下的源文件、定义和链接库。
-- 6. 引入了操作系统特定的库或框架。
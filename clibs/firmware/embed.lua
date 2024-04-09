local input, output = ...

local function readfile(filename)
    local f <close> = assert(io.open(filename, "rb"))
    return f:read "a"
end

local fs = require "bee.filesystem"

local name = fs.path(output):stem():string()
local f <close> = assert(io.open(output, "wb"))
local function writeline(data)
    f:write(data)
    f:write "\r\n"
end

writeline [[#pragma once]]
writeline ""

local data = readfile(input)
if #data >= 16380 then
    writeline(([[static const char embed_hex_%s[] = {]]):format(name))
    local n = #data - #data % 16
    for i = 1, n, 16 do
        local b00, b01, b02, b03, b04, b05, b06, b07, b08, b09, b0a, b0b, b0c, b0d, b0e, b0f = string.byte(data, i, i + 15)
        writeline(("0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,"):format(b00, b01, b02, b03, b04, b05, b06, b07, b08, b09, b0a, b0b, b0c, b0d, b0e, b0f))
    end
    for i = n+1, #data do
        local b = string.byte(data, i)
        f:write(("0x%02x,"):format(b))
    end
    f:write "0x00,"
    writeline ""
    writeline [[};]]
    writeline(([[static constexpr std::string_view embed_%s { embed_hex_%s, sizeof(embed_hex_%s)-1 };]]):format(name, name, name))
else
    f:write(([[static constexpr std::string_view embed_%s = R"firmware(]]):format(name))
    f:write(data)
    f:write([[)firmware"sv;]])
    f:write "\r\n"
end

-- 这段代码是一个 Lua 脚本，用于将输入文件嵌入到 C/C++ 代码中作为字符串。

-- 1. 通过参数 `input` 和 `output` 指定输入文件和输出文件。
-- 2. 函数 `readfile(filename)` 用于读取文件内容，并返回文件内容的字符串。
-- 3. 使用 `bee.filesystem` 模块获取输出文件的名称。
-- 4. 打开输出文件，准备写入数据。
-- 5. 根据输入文件的大小决定如何嵌入文件：
--    - 如果文件大小超过16380字节，则将文件内容按16进制格式写入 C/C++ 数组中。
--    - 如果文件大小未超过16380字节，则直接将文件内容作为字符串写入 C/C++ 代码中。
-- 6. 将处理后的数据写入输出文件。

-- 这段代码的主要目的是将输入文件的内容嵌入到 C/C++ 代码中，以便在编译时将文件内容作为字符串或数组直接嵌入到可执行文件中。
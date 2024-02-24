#pragma once

#include <bgfx/c99/bgfx.h>

#if defined(__cplusplus)
extern "C" {
#endif

int luabgfx_getalloc(bgfx_allocator_interface_t** interface_t);
int luabgfx_info(int64_t* psize);

#if defined(__cplusplus)
}
#endif

// 这段 C 头文件代码实现了以下功能：

// 1. 包含了 `<bgfx/c99/bgfx.h>` 头文件，以便在文件中使用 bgfx 库的函数和类型。
// 2. 提供了两个函数声明，分别是 `luabgfx_getalloc` 和 `luabgfx_info`，用于在 Lua 脚本中获取分配器接口和内存使用信息。
// 3. 使用了条件编译来确保在 C++ 中包含该头文件时不会出现问题，并使用 `extern "C"` 来指定 C++ 的链接规范。
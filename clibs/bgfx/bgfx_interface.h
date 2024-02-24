#ifndef lua_bgfx_interface_h
#define lua_bgfx_interface_h

#include <lua.h>
#include <lauxlib.h>
#include <bgfx/c99/bgfx.h>

#if defined(__cplusplus)
extern "C"
#else
extern
#endif
bgfx_interface_vtbl_t* bgfx_inf_;

#define BGFX(api) bgfx_inf_->api
#define BGFX_ENCODER(api, encoder, ...) (encoder ? (BGFX(encoder_##api)( encoder, ## __VA_ARGS__ )) : BGFX(api)( __VA_ARGS__ ))

#endif

// 这段 C 头文件代码实现了以下功能：

// 1. 定义了一个用于 Lua 绑定的接口，用于在 Lua 脚本中调用 bgfx 库的函数。
// 2. 提供了宏以简化在 Lua 脚本中调用 bgfx 函数的方式。
// 3. 实现了 C 和 C++ 兼容性，以确保在包含此头文件时不会出现问题。
// 4. 通过声明一个指向 `bgfx_interface_vtbl_t` 结构体的指针，提供了访问 bgfx 库函数的方法。
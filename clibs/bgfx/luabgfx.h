#ifndef lua_bgfx_h
#define lua_bgfx_h

#include <stdint.h>

enum BGFX_HANDLE {
	BGFX_HANDLE_PROGRAM = 1,
	BGFX_HANDLE_SHADER,
	BGFX_HANDLE_VERTEX_BUFFER,
	BGFX_HANDLE_INDEX_BUFFER,
	BGFX_HANDLE_DYNAMIC_VERTEX_BUFFER,
	BGFX_HANDLE_DYNAMIC_VERTEX_BUFFER_TYPELESS,
	BGFX_HANDLE_DYNAMIC_INDEX_BUFFER,
	BGFX_HANDLE_DYNAMIC_INDEX_BUFFER_32,
	BGFX_HANDLE_FRAME_BUFFER,
	BGFX_HANDLE_INDIRECT_BUFFER,
	BGFX_HANDLE_TEXTURE,
	BGFX_HANDLE_UNIFORM,
	BGFX_HANDLE_OCCLUSION_QUERY,
};

#define BGFX_LUAHANDLE(type, handle) (BGFX_HANDLE_##type << 16 | handle.idx)
#define BGFX_LUAHANDLE_ID(type, idx) check_handle_type(L, BGFX_HANDLE_##type, (idx), #type)
#define BGFX_LUAHANDLE_WITHTYPE(idx, subtype) ( (idx) | (subtype) << 20 )
#define BGFX_LUAHANDLE_SUBTYPE(idx) ( (idx) >> 20 )

static inline uint16_t
check_handle_type(lua_State *L, int type, int id, const char * tname) {
	int idtype = (id >> 16) & 0x0f;
	if (idtype != type) {
		return luaL_error(L, "Invalid handle type %s (id = %d:%d)", tname, idtype, id&0xffff);
	}
	return (uint16_t)(id & 0xffff);
}

struct memory {
	void *data;
	size_t size;
	int ref;
	int constant;
};

#if LUA_VERSION_NUM < 504
// lua 5.3

static inline void *
lua_newuserdatauv(lua_State *L, size_t size, int nuvalue) {
	if (nuvalue > 1)
		luaL_error(L, "Don't support nuvalue (%d) > 1", nuvalue);
	return lua_newuserdata(L, size);
}

static inline int
lua_setiuservalue(lua_State *L, int idx, int n) {
	if (n != 1)
		return luaL_error(L, "Don't support setiuservalue %d !=1", n);
	return lua_setuservalue(L, idx);
}

#endif

#endif

// 这段 C 头文件代码实现了以下功能：

// 1. 定义了一个枚举 `BGFX_HANDLE`，列出了一系列 bgfx 句柄的类型，如程序、着色器、顶点缓冲等。
// 2. 定义了几个宏来处理 bgfx 句柄，例如 `BGFX_LUAHANDLE` 用于构造句柄，`BGFX_LUAHANDLE_ID` 用于检查句柄类型。
// 3. 实现了一个辅助函数 `check_handle_type`，用于检查句柄类型是否正确。
// 4. 声明了一个结构体 `memory`，用于表示内存块，包括了指向数据的指针、大小、引用计数和是否为常量的标志。
// 5. 对于 Lua 版本号低于 5.4，提供了一些兼容性的宏定义，用于处理新版 Lua 中的 userdata 的操作。

// 综上所述，该头文件主要用于定义了一些常量、宏和结构体，以支持在 Lua 中与 bgfx 库进行交互时所需的功能。
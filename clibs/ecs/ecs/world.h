#pragma once

#if defined(__cplusplus)
#	include <lua.hpp>
#	include <utility>
#	include <cstdint>
#else
#	include <lua.h>
#	include <lauxlib.h>
#	include <stdint.h>
#endif

struct ecs_context;
struct bgfx_interface_vtbl;
struct bgfx_encoder_s;
struct math3d_api;
struct render_material;
struct queue_container;
struct submit_cache;
struct mesh_container;

struct bgfx_encoder_holder {
	struct bgfx_encoder_s* encoder;
};

struct cull_cached;

struct ecs_world {
	struct ecs_context*           ecs;
	struct bgfx_interface_vtbl*   bgfx;
	struct math3d_api*            math3d;
	struct bgfx_encoder_holder*   holder;
	struct cull_cached*           cull_cached;
	struct render_material*       R;
	uint64_t                      frame;
	struct queue_container*       Q;
	struct submit_cache*          submit_cache;
	struct mesh_container*        MESH;
	uint64_t                      unused1;
	uint64_t                      unused2;
};

static inline struct ecs_world* getworld(lua_State* L) {
#if !defined(NDEBUG)
	luaL_checktype(L, lua_upvalueindex(1), LUA_TUSERDATA);
	if (sizeof(struct ecs_world) > lua_rawlen(L, lua_upvalueindex(1))) {
		luaL_error(L, "invalid ecs_world");
		return NULL;
	}
#endif
	return (struct ecs_world*)lua_touserdata(L, lua_upvalueindex(1));
}

// 这段代码主要实现了以下功能：

// 1. 条件编译：根据编译环境选择包含不同的头文件。
// 2. 定义了多个结构体，包括 `bgfx_encoder_holder` 和 `ecs_world` 等。
// 3. 提供了 `getworld` 函数，用于从 Lua 栈中获取 `ecs_world` 结构体的指针。
// 4. 声明了多个指针变量，如 `bgfx_interface_vtbl` 和 `render_material`。
// 5. 定义了 `ecs_world` 结构体，包含了多个成员变量和指针。
// 6. 代码中包含了调试模式下对 `ecs_world` 结构体有效性的检查。
#define LUA_LIB

#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>
#include <math.h>
#include <lua-seri.h>
#include <string.h>

#include "bgfx_interface.h"

static inline float
fclamp(float _a, float _min, float _max) {
	return fmin(fmax(_a, _min), _max);
}

static inline float
fsaturate(float _a) {
	return fclamp(_a, 0.0f, 1.0f);
}

static inline float
fround(float _f) {
	return floor(_f + 0.5f);
}

static inline uint32_t
toUnorm(float value, float scale) {
	return (uint32_t)(fround(fsaturate(value) * scale));
}

static inline void
packRgba8(void* dst_, const float* src) {
	uint8_t* dst = (uint8_t*)dst_;
	dst[0] = (uint8_t)(toUnorm(src[0], 255.0f) );
	dst[1] = (uint8_t)(toUnorm(src[1], 255.0f) );
	dst[2] = (uint8_t)(toUnorm(src[2], 255.0f) );
	dst[3] = (uint8_t)(toUnorm(src[3], 255.0f) );
}

static int
lencodeNormalRgba8(lua_State *L) {
	float x = luaL_checknumber(L, 1);
	float y = luaL_optnumber(L, 2, 0);
	float z = luaL_optnumber(L, 3, 0);
	float w = luaL_optnumber(L, 4, 0);

	const float src[] =	{
		x * 0.5f + 0.5f,
		y * 0.5f + 0.5f,
		z * 0.5f + 0.5f,
		w * 0.5f + 0.5f,
	};
	uint32_t dst;
	packRgba8(&dst, src);
	lua_pushinteger(L, dst);
	return 1;
}

#define BGFX_LOG_ID 'bgfx'
#define MAX_LOGBUFFER (64*1024)

struct ltask;
typedef struct {
    unsigned int id;
} service_id;

typedef int (*ltask_pushlog)(struct ltask* ltask, service_id id, void *data, uint32_t sz);

struct context {
    struct ltask* ltask;
    ltask_pushlog pushlog;
    lua_State* L;
};

static int
context_destroy(lua_State *L) {
    struct context* ctx = lua_touserdata(L, 1);
    lua_close(ctx->L);
    ctx->L = NULL;
    return 0;
}

#define PREFIX(str, cstr) (strncmp(str, cstr"", sizeof(cstr)-1) == 0)

enum LOG_LEVEL {
	LOG_INFO  = 1,
	LOG_ERROR = 2,
	LOG_WARN  = 3,
	LOG_TRACE = 4,
};

static const char*
parse_level(const char *format) {
    if (!PREFIX(format, "BGFX "))
        return "INFO";
    format += 5;    // skip "BGFX "
    if (PREFIX(format, "ASSERT ")) {
        return "ERROR";
    }
    if (PREFIX(format, "WARN ")) {
        return "WARN";
    }
    return "TRACE";
}

static void
bgfx_pushlog(struct context* context, const char *file, uint16_t line, const char *format, va_list ap) {
    char tmp[MAX_LOGBUFFER];
    int n = sprintf(tmp, "( bgfx )(%s:%d) ", file, line);
    n += vsnprintf(tmp+n, sizeof(tmp)-n, format, ap);
    if (n > MAX_LOGBUFFER) {
            // truncated
        n = MAX_LOGBUFFER;
    }
    if (tmp[n-1] == '\n') {
        n--;
    }

    lua_State* L = context->L;
    const char* level = parse_level(format);
    lua_settop(L, 0);
    lua_pushstring(L, level);
    lua_pushlstring(L, tmp, n);
    int sz = 0;
    void* data = seri_pack(L, 0, &sz);
    service_id id = { BGFX_LOG_ID };
    context->pushlog(context->ltask, id, data, sz);
}

static int
get_pushlog(lua_State *L) {
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
    struct context* ctx = lua_newuserdatauv(L, sizeof(struct context), 0);
    ctx->pushlog = lua_touserdata(L, 1);
    ctx->ltask = lua_touserdata(L, 2);
    ctx->L = luaL_newstate();
    lua_newtable(L);
    lua_pushcfunction(L, context_destroy);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    lua_pushlightuserdata(L, bgfx_pushlog);
    lua_pushlightuserdata(L, ctx);
    return 3;
}

LUAMOD_API int
luaopen_bgfx_util(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "encodeNormalRgba8", lencodeNormalRgba8 },
		{ "get_pushlog", get_pushlog },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}

// 这段代码实现了一些与 bgfx 相关的实用功能和日志记录功能，并提供了 Lua 绑定接口。以下是其功能罗列：

// 1. 定义了几个内联函数，用于处理浮点数的范围限制、饱和度、四舍五入等操作。
// 2. 实现了一个函数 `packRgba8`，用于将浮点数表示的颜色值打包成 RGBA8 格式的整数。
// 3. 实现了 Lua 函数 `lencodeNormalRgba8`，用于将法线向量编码成 RGBA8 格式的整数。
// 4. 定义了日志相关的宏和结构体，用于日志记录功能。
// 5. 实现了函数 `bgfx_pushlog`，用于将 bgfx 日志推送到指定的任务中。
// 6. 实现了函数 `get_pushlog`，用于创建一个 Lua 上下文，并将其与日志记录函数关联。
// 7. 最后，定义了 Lua 模块 `luaopen_bgfx_util`，将上述函数打包成 Lua 模块，供 Lua 脚本使用。

// 综上所述，该代码提供了一些实用功能函数和日志记录功能，并通过 Lua 绑定接口暴露给了 Lua 脚本使用。
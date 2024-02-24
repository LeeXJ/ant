#ifndef transient_buffer_h
#define transient_buffer_h

#include <bgfx/c99/bgfx.h>

struct transient_buffer {
	bgfx_transient_vertex_buffer_t tvb;
	bgfx_transient_index_buffer_t tib;
	int cap_v;
	int cap_i;
	char index32;
	char format[1];
};

typedef int (*lua_TBFunction)(lua_State *L, struct transient_buffer *tb);

#endif

// 该头文件定义了一个用于存储临时顶点缓冲和索引缓冲信息的结构体，
// 并提供了一个函数指针类型用于定义操作该结构体的 Lua 函数。
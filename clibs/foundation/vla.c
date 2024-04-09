#include "vla.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>

#define VLA_HEAP_DEFAULT 64

struct vla_heap {
	VLA_COMMON_HEADER
	unsigned char *buffer;
};

struct vla_lua {
	VLA_COMMON_HEADER
	unsigned char buffer[1];
};

static void
vla_close_heap(struct vla_heap *h) {
	free(h->buffer);
	free(h);
}

void
vla_handle_close_(vla_handle_t h) {
	if (h.h == NULL || !(h.h->type & VLA_TYPE_NEEDCLOSE))
		return;
	switch (h.h->type & VLA_TYPE_MASK) {
	case VLA_TYPE_STACK:
		vla_handle_close_(h.s->extra);
		break;
	case VLA_TYPE_HEAP:
		vla_close_heap(h.m);
		break;
	default:
		assert(0);
		break;
	}
}

static struct vla_heap *
vla_resize_heap(struct vla_heap *h, int n, int esize) {
	if (h->buffer == NULL) {
		h->cap = n < VLA_HEAP_DEFAULT ? VLA_HEAP_DEFAULT : n;
		h->buffer = (unsigned char *)malloc(h->cap * esize);
		if (h->buffer == NULL) {
			free(h);
			return NULL;
		}
	} else {
		int newcap = h->cap;
		do newcap = newcap * 3 / 2; while (newcap < n);
		unsigned char * buf = (unsigned char *)realloc(h->buffer, newcap * esize);
		if (buf == NULL) {
			free(h->buffer);
			free(h);
			return NULL;
		}
		h->buffer = buf;
		h->cap = newcap;
	}
	h->n = n;
	return h;
}

int
vla_init_lua_(void *L_) {
	lua_State *L = (lua_State *)L_;
	lua_pushnil(L);
	return lua_gettop(L);
}

static inline void
vla_map_heap(struct vla_heap *h, void **p) {
	*p = (void *)h->buffer;
}

static inline void
vla_map_stack(struct vla_stack *s, void **p) {
	if (s->extra.h == NULL)
		*p = (void *)s->buffer;
	else
		vla_handle_map_(s->extra, p);
}

static inline void
vla_map_lua(struct vla_lua *l, void **p) {
	*p = (void *)l->buffer;
}

void
vla_handle_map_(vla_handle_t h, void **p) {
	switch (h.h->type & VLA_TYPE_MASK) {
	case VLA_TYPE_STACK:
		vla_map_stack(h.s, p);
		break;
	case VLA_TYPE_HEAP:
		vla_map_heap(h.m, p);
		break;
	case VLA_TYPE_LUA:
		vla_map_lua(h.l, p);
		break;
	default:
		assert(0);
		break;
	}
}

vla_handle_t
vla_heap_new(int n, int esize) {
	static const vla_handle_t invalid;
	struct vla_heap * h = (struct vla_heap *)malloc(sizeof(*h));
	if (h == NULL)
		return invalid;
	h->n = n;
	h->cap = n;
	h->type = VLA_TYPE_HEAP | VLA_TYPE_NEEDCLOSE;
	if (n > 0) {
		h->buffer = (unsigned char *)malloc(n * esize);
		if (h->buffer == NULL) {
			free(h);
			return invalid;
		}
	} else
		h->buffer = NULL;
	vla_handle_t ret;
	ret.m = h;
	return ret;
}

vla_handle_t
vla_lua_new(void *L_, int n, int esize) {
	lua_State *L = (lua_State *)L_;
	int cap = n < VLA_HEAP_DEFAULT ? VLA_HEAP_DEFAULT : n;
	int sz = cap * esize + offsetof(struct vla_lua, buffer);
	struct vla_lua * l = (struct vla_lua *)lua_newuserdatauv(L, sz, 0);
	l->n = n;
	l->cap = cap;
	l->type = VLA_TYPE_LUA;
	vla_handle_t ret;
	ret.l = l;
	return ret;
}

static struct vla_stack *
vla_resize_stack(void *L_, struct vla_stack *s, int n, int esize, int *lua_id) {
	if (s->extra.h == NULL) {
		if (L_ == NULL) {
			s->type |= VLA_TYPE_NEEDCLOSE;
			s->extra = vla_heap_new(n, esize);
			if (s->extra.h == NULL) {
				return NULL;
			}
		} else {
			lua_State *L = (lua_State *)L_;
			s->extra = vla_lua_new(L, n, esize);
			if (*lua_id == 0) {
				*lua_id = lua_gettop(L);
			} else {
				lua_replace(L, *lua_id);
			}
		}
		void *addr;
		vla_handle_map_(s->extra, &addr);
		int sz = VLA_STACK_SIZE;
		if (sz > n * esize)
			sz = n * esize;
		memcpy(addr, s->buffer, sz);
	} else {
		vla_handle_resize_(L_, &s->extra, n, esize, lua_id);
		if (s->extra.h == NULL)
			return NULL;
	}
	s->n = s->extra.h->n;
	s->cap = s->extra.h->cap;
	return s;
}

static struct vla_lua *
vla_resize_lua(lua_State *L, struct vla_lua *l, int n, int esize, int *lua_id) {
	int newcap = l->cap;
	do newcap = newcap * 3 / 2; while (newcap < n);
	vla_handle_t ret = vla_lua_new(L, newcap, esize);
	memcpy(ret.l->buffer, l->buffer, l->n * esize);
	ret.l->n = n;
	if (*lua_id == 0) {
		*lua_id = lua_gettop(L);
	} else {
		lua_replace(L, *lua_id);
	}
	return ret.l;
}

void
vla_handle_resize_(void *L, vla_handle_t *h, int n, int esize, int *lua_id) {
	switch (h->h->type & VLA_TYPE_MASK) {
	case VLA_TYPE_STACK:
		h->s = vla_resize_stack(L, h->s, n, esize, lua_id);
		break;
	case VLA_TYPE_HEAP:
		h->m = vla_resize_heap(h->m, n, esize);
		break;
	case VLA_TYPE_LUA:
		h->l = vla_resize_lua((lua_State *)L, h->l, n, esize, lua_id);
		break;
	default:
		assert(0);
		break;
	}
}

// 这段代码实现了一个动态数组的库，其中包含了针对堆、栈和 Lua 环境的不同实现。

// 主要的结构体有：

// - `struct vla_heap`：堆实现的动态数组结构体，包含了数组的容量、大小以及数据缓冲区。
// - `struct vla_stack`：栈实现的动态数组结构体，除了包含了数组的容量、大小以及数据缓冲区外，还有一个额外的 `vla_handle_t` 字段，用于引用 Lua 环境的动态数组。
// - `struct vla_lua`：Lua 环境的动态数组结构体，除了包含了数组的容量、大小以及数据缓冲区外，还有一个额外的 `lua_State*` 字段，用于引用 Lua 环境。

// 主要函数包括：

// - `vla_handle_close_`：关闭动态数组，释放内存。
// - `vla_handle_map_`：将动态数组映射到一个指针。
// - `vla_handle_resize_`：调整动态数组的大小。
// - `vla_heap_new`：创建堆实现的动态数组。
// - `vla_lua_new`：创建 Lua 环境的动态数组。
// - `vla_resize_heap`：调整堆实现的动态数组的大小。
// - `vla_resize_stack`：调整栈实现的动态数组的大小。
// - `vla_resize_lua`：调整 Lua 环境的动态数组的大小。

// 这些函数使得用户可以在不同的场景下创建和操作动态数组，包括堆、栈和 Lua 环境。
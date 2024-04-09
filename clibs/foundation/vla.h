#ifndef VARIANT_LENGTH_ARRAY_H
#define VARIANT_LENGTH_ARRAY_H

#include <stddef.h>

#define VLA_STACK_SIZE 244

#define VLA_TYPE_STACK 0
#define VLA_TYPE_HEAP 1
#define VLA_TYPE_LUA 2
#define VLA_TYPE_MASK 0xf
#define VLA_TYPE_NEEDCLOSE 0x10
#define VLA_COMMON_HEADER int type;	int n; int cap;

struct vla_header { VLA_COMMON_HEADER };
struct vla_stack;
struct vla_heap;
struct vla_lua;

union vla_handle {
	struct vla_header *h;
	struct vla_stack *s;
	struct vla_heap *m;
	struct vla_lua *l;
};

typedef union vla_handle vla_handle_t;

struct vla_stack {
	VLA_COMMON_HEADER
	unsigned char buffer[VLA_STACK_SIZE];
	vla_handle_t extra;
};

static inline vla_handle_t
vla_stack_new_(struct vla_stack *s, int esize) {
	s->type = VLA_TYPE_STACK;
	s->n = 0;
	s->cap = (VLA_STACK_SIZE + esize - 1) / esize;
	s->extra.h = NULL;
	vla_handle_t ret;
	ret.s = s;
	return ret;
};

#define vla_stack_handle(name, type) \
	struct vla_stack name##_stack_; \
	vla_handle_t name = vla_stack_new_(&name##_stack_, sizeof(type))

vla_handle_t vla_heap_new(int n, int esize);
vla_handle_t vla_lua_new(void *L, int n, int esize);

#define vla_new(type, n, L) (L == NULL ? vla_heap_new(n, sizeof(type)) : vla_lua_new(L, n, sizeof(type)))

void vla_handle_map_(vla_handle_t h, void **p);

static inline void
vla_using_(vla_handle_t h, void **p) {
	if ((h.h->type & VLA_TYPE_MASK) == VLA_TYPE_STACK && h.s->extra.h == NULL)
		*p = (void *)h.s->buffer;
	else
		vla_handle_map_(h, p);

}

int vla_init_lua_(void *L);

#define vla_using(name, type, h, L) \
	type * name; \
	vla_handle_t * name##_ref_ = &h; \
	int name##_lua_ = 0; (void) name##_lua_; \
	if (L) { name##_lua_ = vla_init_lua_(L); } \
	vla_using_(h, (void **)&name)

#define vla_sync(name) vla_using_( *name##_ref_, (void **)&name)

void vla_handle_close_(vla_handle_t h);

static inline void
vla_close_handle(vla_handle_t h) {
	if (h.h && (h.h->type & VLA_TYPE_NEEDCLOSE)) {
		vla_handle_close_(h);
	}
}

#define vla_close(name) vla_close_handle(*name##_ref_)

void vla_handle_resize_(void *L, vla_handle_t *h, int n, int esize, int *lua_id);

static inline void
vla_resize_(void *L, void **p, vla_handle_t *h, int n, int esize, int *lua_id) {
	if (n <= h->h->cap)
		h->h->n = n;
	else {
		vla_handle_resize_(L, h, n, esize, lua_id);
		vla_handle_map_(*h, p);
	}
}

#define vla_resize(name, n, L) vla_resize_(L, (void **)&name, name##_ref_, n, sizeof(*name), &name##_lua_)

#define vla_size(name) (name##_ref_->h->n)

#define vla_push(name, v, L) vla_resize(name, vla_size(name) + 1, L); name[vla_size(name)-1] = v

#define vla_luaid(name, L) (lua_isnoneornil(L, name##_lua_) ? 0 : name##_lua_)

#endif

/*
	// Use vla on Stack :

	lua_State *L = NULL;

	vla_stack_handle(handle, int);		// Create an int array on stack
	vla_using(p, int, handle, L);		// Create an acessor (int *p) for data accessing.

	int i;
	for (i=0;i<1000;i++) {
		vla_push(p, i, L);
	}

	assert(vla_size(p) == 1000);

	int i;
	for (i=0;i<1000;i++) {
		assert(p[i] == i);
	}

	vla_close(p);	// close before return


	// Use VLA on heap :

	vla_handle_t handle2 = vla_new(int, 0, NULL);	// Create handle on heap
	vla_using(p2, int, handle2, NULL);

	// do something with p2

	vla_close_handle(handle2);
*/

// 这段代码实现了一个支持栈和堆上动态数组的库，并提供了方便的宏来简化使用。

// 主要的结构体和联合包括：

// - `struct vla_header`：动态数组头部的通用结构体，包含数组类型、大小和容量。
// - `struct vla_stack`：栈上动态数组的结构体，包含了数组大小、容量和数据缓冲区，以及可能存在的额外句柄。
// - `struct vla_heap`：堆上动态数组的结构体，包含了数组大小、容量和数据缓冲区。
// - `struct vla_lua`：Lua 环境中动态数组的结构体，包含了数组大小、容量和数据缓冲区。
// - `union vla_handle`：动态数组句柄的联合类型，可以指向不同类型的动态数组。

// 主要的宏和函数包括：

// - `vla_stack_new_`：创建栈上动态数组。
// - `vla_heap_new`：创建堆上动态数组。
// - `vla_lua_new`：创建 Lua 环境中的动态数组。
// - `vla_new`：根据是否在 Lua 环境中创建动态数组。
// - `vla_using_`：根据动态数组类型映射到对应的数据缓冲区。
// - `vla_using`：创建动态数组的访问器。
// - `vla_sync`：同步动态数组的数据缓冲区。
// - `vla_handle_close_`：关闭动态数组。
// - `vla_close_handle`：关闭动态数组句柄。
// - `vla_handle_resize_`：调整动态数组的大小。
// - `vla_resize_`：调整动态数组大小并映射到对应的数据缓冲区。
// - `vla_size`：获取动态数组的大小。
// - `vla_push`：向动态数组中添加元素。
// - `vla_luaid`：获取动态数组在 Lua 环境中的 ID。

// 通过这些宏和函数，用户可以方便地创建和管理栈和堆上的动态数组，并在需要时调整大小、添加元素和关闭动态数组。
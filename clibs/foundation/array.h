#ifndef carray_userdata_h
#define carray_userdata_h

#include <lua.h>
#include <lauxlib.h>
#include <string.h>

struct array {
	int cap;
	int n;
};

struct array_accessor {
	struct array *a;
	int root_index;
	int uv_index;
	size_t esize;
};

#define DEFAULT_ARRAY_SIZE 16
#define array_init(uv_index, type) array_init_(L, uv_index, sizeof(type))

#define array_using(rootv, field_name, root_index, uv_index, type) \
	union { type * a ; void *p; } rootv##_##field_name##_u;	\
	struct array_accessor rootv##_##field_name####_aa = { rootv->field_name, lua_absindex(L, root_index), uv_index, sizeof(type) };

#define array(rootv, field_name) ( rootv##_##field_name##_u.p = (void *)(rootv##_##field_name##_aa.a + 1), rootv##_##field_name##_u.a )
#define array_resize(rootv, field_name, n) array_resize_(L, &rootv##_##field_name##_aa, &(rootv->field_name), n)
#define array_push(rootv, field_name, v) { int n = rootv->field_name->n; array_resize(rootv, field_name, n+1); array(rootv, field_name)[n] = v; }
#define array_size(rootv, field_name) (rootv->field_name->n)

static inline struct array *
array_init_(lua_State *L, int uv_index, size_t esize) {
	struct array * a = (struct array *)lua_newuserdatauv(L, sizeof(*a) + DEFAULT_ARRAY_SIZE * esize, 0);
	a->cap = DEFAULT_ARRAY_SIZE;
	a->n = 0;
	if (!lua_setiuservalue(L, -2, uv_index)) {
		luaL_error(L, "lua_setiuservalue %d failed" , uv_index);
	}
	return a;
}

static inline void
array_resize_(lua_State *L, struct array_accessor *aa, struct array **ref, int newsize) {
	struct array * a = aa->a;
	if (newsize <= a->cap) {
		a->n = newsize;
		return;
	}
	int newcap = a->cap * 2;
	while (newcap < newsize) {
		newcap *= 2;
	}
	size_t sz = sizeof(*a) + newcap * aa->esize;
	a = (struct array *)lua_newuserdatauv(L, sz, 0);
	memcpy(a, aa->a, sizeof(*a) + aa->a->n * aa->esize);
	a->cap = newcap;
	aa->a = a;
	*ref= a;
	if (!lua_setiuservalue(L, aa->root_index, aa->uv_index)) {
		luaL_error(L, "lua_setuservalue %d failed", aa->uv_index);
	}
	a->n = newsize;
}

/*
	Usage :

	struct material {
		struct array * list;	// list is an int array
	};

	#define LIST_UV_INDEX 1	// ref list memory(userdata) in uservalue 1

	void init(lua_State *L) {
		struct material * mat = (struct material *)lua_newuserdatauv(L, sizeof(*mat), 1);
		mat->list = array_init(LIST_UV_INDEX, int);
	}

	void push(lua_State *L, int index, int v) {
		struct material * mat = (struct material *)lua_touserdata(L, index);
		array_using(mat, list, index, LIST_UV_INDEX, int);
		array_push(mat, list, v);
	}

	void access(lua_State *L, int index) {
		struct material * mat = (struct material *)lua_touserdata(L, index);
		array_using(mat, list, index, LIST_UV_INDEX, int);
		int n = array_size(mat, list);
		int i;
		for (i=0;i<n;i++) {
			printf("%d,", array(mat,list)[i]);
		}
	}

	void clear(lua_State *L, int index) {
		struct material * mat = (struct material *)lua_touserdata(L, index);
		array_using(mat, list, index, LIST_UV_INDEX, int);
		array_resize(mat, list, 0);
	}
 */

#endif


// 这是一个 C 语言头文件，定义了一个简单的动态数组结构以及一些操作动态数组的函数。该动态数组结构 `array` 包含两个字段：`cap` 表示数组的容量，`n` 表示数组的元素个数。同时，还定义了一个 `array_accessor` 结构体，用于访问动态数组的元素。

// 该头文件提供了以下几个宏和函数：

// - `array_init(uv_index, type)`: 初始化一个动态数组，并将其存储到 Lua 的 uservalue 中，以便在 Lua 中保持对其的引用。
// - `array_using(rootv, field_name, root_index, uv_index, type)`: 定义一个动态数组的访问器，方便后续操作该动态数组。
// - `array(rootv, field_name)`: 获取动态数组的指针。
// - `array_resize(rootv, field_name, n)`: 调整动态数组的大小。
// - `array_push(rootv, field_name, v)`: 在动态数组中压入一个新元素。
// - `array_size(rootv, field_name)`: 获取动态数组的元素个数。

// 使用示例和详细说明都包含在头文件的注释中。
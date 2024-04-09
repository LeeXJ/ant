#include <lua.hpp>
#include "firmware.h"
#include "memfile.h"

static std::string_view luaL_checkstrview(lua_State* L, int idx) {
	size_t sz = 0;
	const char* str = luaL_checklstring(L, idx, &sz);
	return std::string_view(str, sz);
}

static int readall_v(lua_State* L) {
	std::string_view filename = luaL_checkstrview(L, 1);
	auto it = firmware.find(filename);
	if (it == firmware.end()) {
		return luaL_error(L, "%s:No such file or directory.", filename.data());
	}
	auto file = memory_file_cstr(it->second.data(), it->second.size());
	if (!file) {
		return luaL_error(L, "not enough memory");
	}
	lua_pushlightuserdata(L, file);
	return 1;
}

extern "C"
int luaopen_firmware(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "readall_v", readall_v },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}

// 这段代码实现了一个 Lua C 模块，用于从内存中读取固件数据。

// 1. **辅助函数 `luaL_checkstrview`**：
//    - 该函数用于从 Lua 栈中获取字符串，并返回一个 `std::string_view` 对象，表示该字符串的视图。

// 2. **函数 `readall_v`**：
//    - 该函数是模块中提供的 Lua 绑定函数，用于读取固件数据。
//    - 首先从 Lua 栈中获取文件名，然后在内存中查找对应的固件数据。
//    - 如果找到了对应的固件数据，将其包装为内存文件对象，并将其压入 Lua 栈中返回。
//    - 如果未找到对应的固件数据，则通过 `luaL_error` 报告错误。

// 3. **模块入口函数 `luaopen_firmware`**：
//    - 该函数注册了模块中的所有功能函数，并返回 Lua 模块。

// 该模块假设固件数据存储在内存中，并通过 `firmware` 数据结构进行访问。模块提供了一个接口供 Lua 脚本使用，从而可以在 Lua 环境中方便地读取固件数据。
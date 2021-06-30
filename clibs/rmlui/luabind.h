#pragma once

#include <lua.hpp>
#include <functional>
#include <deque>

namespace luabind {
	typedef std::function<void(lua_State*)> call_t;
	typedef std::function<void(const char*)> error_t;
	inline int errhandler(lua_State* L) {
		const char* msg = lua_tostring(L, 1);
		if (msg == NULL) {
			if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)
				return 1;
			else
				msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
		}
		luaL_traceback(L, L, msg, 1);
		return 1;
	}
	inline void errfunc(const char* msg) {
		// todo: use Rml log
		lua_writestringerror("%s\n", msg);
	}
	inline int function_call(lua_State* L) {
		call_t& f = *(call_t*)lua_touserdata(L, 1);
		f(L);
		return 0;
	}
	inline lua_State* createthread(lua_State* mL) {
		lua_State* L;
		if (LUA_TTHREAD != lua_getfield(mL, LUA_REGISTRYINDEX, "LUABIND_INVOKE")) {
			L = lua_newthread(mL);
			lua_setfield(mL, LUA_REGISTRYINDEX, "LUABIND_INVOKE");
		}
		else {
			L = lua_tothread(mL, -1);
			lua_pop(mL, 1);
		}
		return L;
	}
	inline lua_State* getthread(lua_State* mL) {
		static lua_State* L = createthread(mL);
		return L;
	}
	inline bool invoke(call_t f) {
		lua_State* L = getthread(NULL);
		if (!lua_checkstack(L, 2)) {
			errfunc("stack overflow");
			return false;
		}
		lua_pushcfunction(L, function_call);
		lua_pushlightuserdata(L, &f);
		int nresults = 0;
		int r = lua_resume(L, NULL, 1, &nresults);
		if (r == LUA_OK) {
			assert(nresults == 0);
			lua_settop(L, 0);
			return true;
		}
		if (r == LUA_YIELD) {
			errfunc("shouldn't yield");
			assert(nresults == 0);
			lua_settop(L, 0);
			return false;
		}
		if (!lua_checkstack(L, LUA_MINSTACK)) {
			errfunc(lua_tostring(L, -1));
			lua_pop(L, 1);
			return false;
		}
		luaL_traceback(L, L, lua_tostring(L, -1), 0);
		errfunc(lua_tostring(L, -1));
		lua_pop(L, 2);
		return false;
	}

	struct reference {
		reference(lua_State* L)
			: dataL(NULL) {
			dataL = lua_newthread(L);
			lua_rawsetp(L, LUA_REGISTRYINDEX, this);
		}
		~reference() {
			lua_pushnil(dataL);
			lua_rawsetp(dataL, LUA_REGISTRYINDEX, this);
		}
		int ref(lua_State* L) {
			if (!lua_checkstack(dataL, 2)) {
				return -1;
			}
			lua_xmove(L, dataL, 1);
			if (freelist.empty()) {
				return lua_gettop(dataL);
			}
			else {
				int r = freelist.back();
				freelist.pop_back();
				lua_replace(dataL, r);
				return r;
			}
		}
		void unref(int ref) {
			int top = lua_gettop(dataL);
			if (top != ref) {
				for (auto it = freelist.begin(); it != freelist.end(); ++it) {
					if (ref < *it) {
						freelist.insert(it, ref);
						return;
					}
				}
				freelist.push_back(ref);
				return;
			}
			--top;
			if (freelist.empty()) {
				lua_settop(dataL, top);
				return;
			}
			for (auto it = freelist.begin(); it != freelist.end(); --top, ++it) {
				if (top != *it) {
					lua_settop(dataL, top);
					freelist.erase(freelist.begin(), it);
					return;
				}
			}
			lua_settop(dataL, 0);
			freelist.clear();
		}
		void get(lua_State* L, int ref) {
			lua_pushvalue(dataL, ref);
			lua_xmove(dataL, L, 1);
		}
		lua_State* dataL;
		std::deque<int> freelist;
	};
}

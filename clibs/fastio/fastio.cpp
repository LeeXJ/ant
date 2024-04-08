#include <lua.hpp>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <array>
#include "memfile.h"

extern "C" {
#include "sha1.h"
}

#if defined(LUA_USE_POSIX)
#   include <sys/types.h>
#   define l_fseek(f,o,w)	fseeko(f,o,w)
#   define l_ftell(f)		ftello(f)
#   define l_seeknum		off_t
#elif defined(LUA_USE_WINDOWS) && !defined(_CRTIMP_TYPEINFO) && defined(_MSC_VER) && (_MSC_VER >= 1400)
#   define l_fseek(f,o,w)	_fseeki64(f,o,w)
#   define l_ftell(f)		_ftelli64(f)
#   define l_seeknum		__int64
#else
#   define l_fseek(f,o,w)	fseek(f,o,w)
#   define l_ftell(f)		ftell(f)
#   define l_seeknum		long
#endif

#if defined(_WIN32)
#include <Windows.h>
static wchar_t* u2w(lua_State* L, const char *str) {
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (!len) {
        luaL_error(L, "MultiByteToWideChar Failed: %d", GetLastError());
        return NULL;
    }
    wchar_t* buf = (wchar_t*)lua_newuserdatauv(L, len * sizeof(wchar_t), 0);
    if (!buf) {
        luaL_error(L, "not enough memory");
        return NULL;
    }
    int out_len = MultiByteToWideChar(CP_UTF8, 0, str, -1, buf, len);
    if (!out_len) {
        luaL_error(L, "MultiByteToWideChar Failed: %d", GetLastError());
        return NULL;
    }
    return buf;
}
#endif

struct file_t {
    static file_t open(lua_State* L, const char* filename) {
#if defined(_WIN32)
        return file_t { _wfopen(u2w(L, filename), L"rb") };
#else
        return file_t { fopen(filename, "r") };
#endif
    }
    ~file_t() {
        if (f) fclose(f);
    }
    void close() {
        if (f) {
            fclose(f);
            f = NULL;
        }
    }
    bool suc() const {
        return !!f;
    }
    size_t size() {
        l_fseek(f, 0, SEEK_END);
        l_seeknum size = l_ftell(f);
        l_fseek(f, 0, SEEK_SET);
        return (size_t)size;
    }
    size_t read(void* buf, size_t sz) {
        return fread(buf, sizeof(char), sz, f);
    }
    FILE* f;
};

template <bool RAISE>
static int raise_error(lua_State *L, const char* what, const char *filename) {
    int en = errno;
    if constexpr (RAISE) {
        return luaL_error(L, "cannot %s %s: %s", what, filename, strerror(en));
    }
    else {
        luaL_pushfail(L);
        lua_pushfstring(L, "cannot %s %s: %s", what, filename, strerror(en));
        lua_pushinteger(L, en);
        return 3;
    }
}

static const char* getfile(lua_State *L) {
    if (lua_type(L, 1) != LUA_TSTRING) {
        if (lua_type(L, 2) == LUA_TSTRING) {
            luaL_error(L, "unable to decode filename: %s", lua_tostring(L, 2));
        }
        else {
            luaL_error(L, "unable to decode filename: type(%s)", luaL_typename(L, 1));
        }
        return nullptr;
    }
    return lua_tostring(L, 1);
}

static const char* getsymbol(lua_State *L, const char* filename) {
#if defined(__ANT_RUNTIME__)
    return luaL_optstring(L, 2, filename);
#else
    return filename;
#endif
}

struct wrap {
    memory_file* file;
};

static int wrap_close(lua_State* L) {
    struct wrap& wrap = *(struct wrap*)lua_touserdata(L, 1);
    if (wrap.file) {
        memory_file_close(wrap.file);
        wrap.file = nullptr;
    }
    return 0;
}

static int wrap_closure(lua_State* L) {
    memory_file* file = (memory_file*)lua_touserdata(L, lua_upvalueindex(1));
    lua_pushlightuserdata(L, (void*)file->data);
    lua_pushinteger(L, file->sz);
    struct wrap& wrap = *(struct wrap*)lua_newuserdatauv(L, sizeof(struct wrap), 0);
    wrap.file = file;
    if (luaL_newmetatable(L, "fastio::wrap")) {
        luaL_Reg lib[] = {
            { "__close", wrap_close },
            { NULL, NULL },
        };
        luaL_setfuncs(L, lib, 0);
    }
    lua_setmetatable(L, -2);
    return 3;
}

template <bool RAISE>
static int readall_v(lua_State *L) {
    const char* filename = getfile(L);
    lua_settop(L, 2);
    file_t f = file_t::open(L, filename);
    if (!f.suc()) {
        return raise_error<RAISE>(L, "open", getsymbol(L, filename));
    }
    size_t size = f.size();
    auto file = memory_file_alloc(size);
    if (!file) {
        f.close();
        return luaL_error(L, "not enough memory");
    }
    size_t nr = f.read((void*)file->data, file->sz);
    if (nr != size) {
        memory_file_close(file);
        return luaL_error(L, "unknown read error");
    }
    lua_pushlightuserdata(L, file);
    return 1;
}

template <bool RAISE>
static int readall_f(lua_State *L) {
    const char* filename = getfile(L);
    lua_settop(L, 2);
    file_t f = file_t::open(L, filename);
    if (!f.suc()) {
        return raise_error<RAISE>(L, "open", getsymbol(L, filename));
    }
    size_t size = f.size();
    auto file = memory_file_alloc(size);
    if (!file) {
        f.close();
        return luaL_error(L, "not enough memory");
    }
    size_t nr = f.read((void*)file->data, file->sz);
    if (nr != size) {
        memory_file_close(file);
        return luaL_error(L, "unknown read error");
    }
    lua_pushlightuserdata(L, file);
    lua_pushcclosure(L, wrap_closure, 1);
    return 1;
}

template <bool RAISE>
static int readall_s(lua_State *L) {
    const char* filename = getfile(L);
    lua_settop(L, 2);
    file_t f = file_t::open(L, filename);
    if (!f.suc()) {
        return raise_error<RAISE>(L, "open", getsymbol(L, filename));
    }
    size_t size = f.size();
    void* data = lua_newuserdatauv(L, size, 0);
    if (!data) {
        f.close();
        return luaL_error(L, "not enough memory");
    }
    size_t nr = f.read(data, size);
    lua_pushlstring(L, (const char*)data, nr);
    return 1;
}

static char hex[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

template <bool RAISE>
static int sha1(lua_State *L) {
    const char* filename = getfile(L);
    lua_settop(L, 2);
    file_t f = file_t::open(L, filename);
    if (!f.suc()) {
        return raise_error<RAISE>(L, "open", getsymbol(L, filename));
    }
    std::array<uint8_t, 1024> buffer;
    SHA1_CTX ctx;
    sat_SHA1_Init(&ctx);
    for (;;) {
        size_t n = f.read(buffer.data(), buffer.size());
        if (n != buffer.size()) {
            sat_SHA1_Update(&ctx, buffer.data(), n);
            break;
        }
        sat_SHA1_Update(&ctx, buffer.data(), buffer.size());
    }
    std::array<uint8_t, SHA1_DIGEST_SIZE> digest;
    std::array<char, SHA1_DIGEST_SIZE*2> hexdigest;
    sat_SHA1_Final(&ctx, digest.data());
    for (size_t i = 0; i < SHA1_DIGEST_SIZE; ++i) {
        auto u = digest[i];
        hexdigest[2*i+0] = hex[u / 16];
        hexdigest[2*i+1] = hex[u % 16];
    }
    lua_pushlstring(L, hexdigest.data(), hexdigest.size());
    return 1;
}

static int str2sha1(lua_State *L) {
	size_t sz = 0;
	const uint8_t * buffer = (const uint8_t *)luaL_checklstring(L, 1, &sz);
	SHA1_CTX ctx;
	sat_SHA1_Init(&ctx);
	sat_SHA1_Update(&ctx, buffer, sz);
    std::array<uint8_t, SHA1_DIGEST_SIZE> digest;
    std::array<char, SHA1_DIGEST_SIZE*2> hexdigest;
    sat_SHA1_Final(&ctx, digest.data());
    for (size_t i = 0; i < SHA1_DIGEST_SIZE; ++i) {
        auto u = digest[i];
        hexdigest[2*i+0] = hex[u / 16];
        hexdigest[2*i+1] = hex[u % 16];
    }
    lua_pushlstring(L, hexdigest.data(), hexdigest.size());
    return 1;
}

static int wrap(lua_State* L) {
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    lua_settop(L, 1);
    lua_pushcclosure(L, wrap_closure, 1);
    return 1;
}

static int tostring(lua_State *L) {
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    memory_file* file = (memory_file*)lua_touserdata(L, 1);
    if (lua_gettop(L) == 1) {
        lua_pushlstring(L, (const char*)file->data, file->sz);
        memory_file_close(file);
        return 1;
    }
    const size_t offset = (size_t)luaL_optinteger(L, 2, 1) - 1;
    const size_t size = (size_t)luaL_checkinteger(L, 3);
    lua_pushlstring(L, (const char*)file->data+offset, size);
    memory_file_close(file);
    return 1;
}

static int free(lua_State *L) {
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    memory_file* file = (memory_file*)lua_touserdata(L, 1);
    memory_file_close(file);
    return 0;
}

struct LoadS {
    const char *s;
    size_t size;
};

static const char* getS(lua_State *L, void *ud, size_t *size) {
    LoadS *ls = (LoadS *)ud;
    (void)L;
    if (ls->size == 0) return NULL;
    *size = ls->size;
    ls->size = 0;
    return ls->s;
}

static int loadlua(lua_State* L) {
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    memory_file* file = (memory_file*)lua_touserdata(L, 1);
    const char* symbol = luaL_checkstring(L, 2);
    lua_settop(L, 3);
    LoadS ls;
    ls.s = (const char*)file->data;
    ls.size = file->sz;
    lua_pushfstring(L, "@%s", symbol);
    int status = lua_load(L, getS, &ls, lua_tostring(L, -1), "t");
    memory_file_close(file);
    if (status != LUA_OK) {
        luaL_pushfail(L);
        lua_insert(L, -2);
        return 2;
    }
    if (!lua_isnoneornil(L, 3)) {
        lua_pushvalue(L, 3);
        if (!lua_setupvalue(L, -2, 1)) {
            lua_pop(L, 1);
        }
    }
    return 1;
}

extern "C" int
luaopen_fastio(lua_State* L) {
    luaL_Reg l[] = {
        {"readall_v", readall_v<true>},
        {"readall_v_noerr", readall_v<false>},
        {"readall_f", readall_f<true>},
        {"readall_s", readall_s<true>},
        {"readall_s_noerr", readall_s<false>},
        {"sha1", sha1<true>},
        {"str2sha1", str2sha1},
        {"wrap", wrap},
        {"tostring", tostring},
        {"free", free},
        {"loadlua", loadlua},
        {NULL, NULL},
    };
    luaL_newlib(L, l);
    return 1;
}

#include "lua.hpp"
#include "BakerInterface.h"

#include "lua2struct.h"
#include "luabgfx.h"

namespace lua_struct {
    template<>
    // 定义了一个模板特化函数，用于解析 Lua 表中的数据并填充到 glm::vec3 类型的变量中。
    inline void unpack<glm::vec3>(lua_State *L, int idx, glm::vec3 &v, void*) {
        // 检查 Lua 栈中指定索引处的值是否为 Lua 表
        luaL_checktype(L, idx, LUA_TTABLE);
        // 遍历 Lua 表的每个元素，将其填充到 glm::vec3 变量中
        for (int ii=0; ii<3; ++ii) {
            // 获取 Lua 表中索引为 ii+1 的值
            lua_geti(L, idx, ii+1);
            // 将获取到的值转换为浮点数，并赋值给 glm::vec3 的相应分量
            v[ii] = (float)lua_tonumber(L, -1);
            // 弹出 Lua 栈顶部的值
            lua_pop(L, 1);
        }
    }

    template<>
    // 定义了一个模板特化函数，用于解析 Lua 表中的数据并填充到 glm::vec4 类型的变量中。
    inline void unpack<glm::vec4>(lua_State *L, int idx, glm::vec4 &v, void*) {
        // 检查 Lua 栈中指定索引处的值是否为 Lua 表
        luaL_checktype(L, idx, LUA_TTABLE);
        // 遍历 Lua 表的每个元素，将其填充到 glm::vec4 变量中
        for (int ii=0; ii<4; ++ii) {
            // 获取 Lua 表中索引为 ii+1 的值
            lua_geti(L, idx, ii+1);
            // 将获取到的值转换为浮点数，并赋值给 glm::vec4 的相应分量
            v[ii] = (float)lua_tonumber(L, -1);
            // 弹出 Lua 栈顶部的值
            lua_pop(L, 1);
        }
    }

    template<>
    // 定义了一个模板特化函数，用于解析 Lua 表中的数据并填充到 glm::mat4 类型的变量中。
    inline void unpack<glm::mat4>(lua_State *L, int idx, glm::mat4 &v, void*) {
        // 检查 Lua 栈中指定索引处的值是否为 Lua 表
        luaL_checktype(L, idx, LUA_TTABLE);
        // 获取 glm::mat4 变量的底层数据指针
        float *vv = &v[0].x;
        // 遍历 Lua 表的每个元素，将其填充到 glm::mat4 变量中
        for (int ii=0; ii<16; ++ii) {
            // 获取 Lua 表中索引为 ii+1 的值
            lua_geti(L, idx, ii+1);
            // 将获取到的值转换为浮点数，并赋值给 glm::mat4 的相应元素
            vv[ii] = (float)lua_tonumber(L, -1);
            // 弹出 Lua 栈顶部的值
            lua_pop(L, 1);
        }
    }

    template <>
    inline void unpack<BufferData>(lua_State* L, int idx, BufferData& v, void*) {
        luaL_checktype(L, idx, LUA_TTABLE);
        unpack_field(L, idx, "data", v.data);
        unpack_field(L, idx, "offset", v.offset);
        unpack_field(L, idx, "stride", v.stride);
        const char* type = nullptr;
        unpack_field(L, idx, "type", type);
        switch (type[0]){
            case 'B': v.type = BT_Byte; break;
            case 'H': v.type = BT_Uint16; break;
            case 'I': v.type = BT_Uint32; break;
            case 'f': v.type = BT_Float; break;
            case '\0':v.type = BT_None; break;
            default: luaL_error(L, "invalid data type:%s", type);
        }
    }

    template <>
    inline void unpack<MeshData>(lua_State* L, int idx, MeshData& v, void*) {
        luaL_checktype(L, idx, LUA_TTABLE);
        unpack_field(L, idx, "worldmat", v.worldmat);
        unpack_field(L, idx, "normalmat", v.normalmat);

        unpack_field(L, idx, "positions", v.positions);
        unpack_field(L, idx, "normals",   v.normals);
        v.tangents.type = BT_None;
        unpack_field_opt(L, idx, "tangents", v.tangents);
        v.bitangents.type = BT_None;
        unpack_field_opt(L, idx, "bitangents", v.bitangents);

        unpack_field(L, idx, "texcoords0", v.texcoords0);

        v.texcoords1.type = BT_None;
        unpack_field_opt(L, idx, "texcoords1", v.texcoords1);
        if (v.texcoords1.type == BT_None){
            v.texcoords1 = v.texcoords0;
        }

        unpack_field(L, idx, "vertexCount", v.vertexCount);

        // for indices
        v.indices.type = BT_None;
        unpack_field_opt(L, idx, "indices", v.indices);
        v.indexCount = 0;
        unpack_field_opt(L, idx, "indexCount", v.indexCount);

        unpack_field(L, idx, "materialidx", v.materialidx);
        assert(v.materialidx > 0);
        --v.materialidx;

        unpack_field(L, idx, "lightmap", v.lightmap);
    }

    template <>
    inline void unpack<MaterialData>(lua_State* L, int idx, MaterialData& v, void*) {
        luaL_checktype(L, idx, LUA_TTABLE);
        unpack_field_opt(L, idx, "diffuse", v.diffuse);
        unpack_field_opt(L, idx, "normal", v.normal);
        unpack_field_opt(L, idx, "roughness", v.roughness);
        unpack_field_opt(L, idx, "metallic",   v.metallic);
    }

    template <>
    inline void unpack<Light>(lua_State* L, int idx, Light& v, void*) {
        luaL_checktype(L, idx, LUA_TTABLE);
        unpack_field(L, idx, "dir", v.dir);
        unpack_field(L, idx, "pos", v.pos);
        unpack_field(L, idx, "color", v.color);

        unpack_field(L, idx, "intensity", v.intensity);
        unpack_field(L, idx, "range", v.range);
        unpack_field(L, idx, "inner_cutoff", v.inner_cutoff);
        unpack_field(L, idx, "outter_cutoff", v.outter_cutoff);
        unpack_field(L, idx, "angular_radius", v.angular_radius);

        const char* type = nullptr;
        unpack_field(L, idx, "type", type);
        if (strcmp(type, "directional") == 0){
            v.type = Light::LT_Directional;
        } else if (strcmp(type, "point") == 0){
            v.type = Light::LT_Point;
            if (v.range == 0.f){
                luaL_error(L, "invalid point light, range must not be 0.0");
            }
        } else if (strcmp(type, "spot") == 0){
            v.type = Light::LT_Spot;
            if (v.range == 0.f){
                luaL_error(L, "invalid spot light, range must not be 0.0");
            }
            if (v.inner_cutoff == 0.f || v.outter_cutoff == 0.f){
                luaL_error(L, "invalid spot light, inner_cutoff and outter_cutoff must not be 0.0");
            }
        } else if (strcmp(type, "area") == 0){
            v.type = Light::LT_Area;
            if (v.angular_radius == 0.f){
                luaL_error(L, "invalid area light, angular_radius must not be 0.0");
            }
        } else {
            luaL_error(L, "invalid light type:%s", type);
        }
    }
}

LUA2STRUCT(Lightmap, size);

LUA2STRUCT(Scene, models, lights, materials);

    //{
    //     MeshData md;
    //     glm::vec3 pos[] = {
    //         glm::vec3(-1.f, 0.f, 1.f),
    //         glm::vec3(1.f, 0.f, 1.f),
    //         glm::vec3(1.f, 0.f, -1.f),
    //         glm::vec3(-1.f, 0.f, -1.f),

    //         glm::vec3(-1.f,  1.f, 0.f),
    //         glm::vec3(1.f,  1.f, 0.f),
    //         glm::vec3(1.f, -1.f, 0.f),
    //         glm::vec3(-1.f, -1.f, 0.f),
    //     };

    //     glm::vec3 nor[] = {
    //         glm::vec3(0.f, 1.f, 0.f),
    //         glm::vec3(0.f, 1.f, 0.f),
    //         glm::vec3(0.f, 1.f, 0.f),
    //         glm::vec3(0.f, 1.f, 0.f),

    //         glm::vec3(0.f, 0.f, 1.f),
    //         glm::vec3(0.f, 0.f, 1.f),
    //         glm::vec3(0.f, 0.f, 1.f),
    //         glm::vec3(0.f, 0.f, 1.f),
    //     };

    //     glm::vec2 tex0[] = {
    //         glm::vec2(0.f, 0.f),
    //         glm::vec2(1.f, 0.f),
    //         glm::vec2(1.f, 1.f),
    //         glm::vec2(0.f, 1.f),

    //         glm::vec2(0.f, 0.f),
    //         glm::vec2(1.f, 0.f),
    //         glm::vec2(1.f, 1.f),
    //         glm::vec2(0.f, 1.f),
    //     };

    //     glm::vec2 lm_uv[] = {
    //         glm::vec2(0.f, 0.f),
    //         glm::vec2(1.f, 0.f),
    //         glm::vec2(1.f, 1.f),
    //         glm::vec2(0.f, 1.f),

    //         glm::vec2(0.f, 0.f),
    //         glm::vec2(1.f, 0.f),
    //         glm::vec2(1.f, 1.f),
    //         glm::vec2(0.f, 1.f),
    //     };

    //     auto set_buffer = [](auto &b, auto data, auto type, auto stride, auto offset){
    //         b.data = (const char*)data;
    //         b.type = type;
    //         b.stride = stride, b.offset = offset;
    //     };
        
    //     set_buffer(md.positions, pos, BT_Float, 12, 0);
    //     set_buffer(md.normals, nor, BT_Float, 12, 0);
    //     set_buffer(md.texcoords0, tex0, BT_Float, 8, 0);
    //     set_buffer(md.texcoords1, lm_uv, BT_Float, 8, 0);

    //     set_buffer(md.tangents, nullptr, BT_None, 0, 0);
    //     set_buffer(md.bitangents, nullptr, BT_None, 0, 0);
    //     md.vertexCount = 8;

    //     uint16_t indices[] = {
    //         0, 1, 2,
    //         2, 3, 0,

    //         4, 5, 6,
    //         6, 7, 4,
    //     };

    //     md.indexCount = 12;

    //     set_buffer(md.indices, indices, BT_Uint16, 2, 0);

    //     md.worldmat = glm::mat4(1.f);
    //     md.normalmat = glm::mat4(1.f);
        
    //     md.materialidx = 0;
    //     s.models.push_back(md);
    //     s.materials.push_back(MaterialData());
    // }
static int
// 定义了一个静态函数，返回一个整数值，表示 Lua 调用此函数后压入栈的值的数量。
// 这里的 static 关键字表示该函数只能在当前文件中可见，不能在其他文件中使用。
lbaker_create(lua_State *L) {
    // 创建一个 Scene 结构体，用于存储 Lua 环境传入的场景数据
    Scene s;
    // 使用 lua_struct 库中的 unpack 函数将 Lua 栈中的第一个参数解包到 Scene 结构体中
    lua_struct::unpack(L, 1, s);
    // 调用 CreateBaker 函数创建一个烘焙器对象，并将场景数据传入
    BakerHandle bh = CreateBaker(&s);
    // 将烘焙器对象的指针压入 Lua 栈
    lua_pushlightuserdata(L, bh);
    // 返回值为 1，代表压入栈的值数量为 1
    return 1;
}

static int
// 定义了一个静态函数，返回一个整数值，表示 Lua 调用此函数后压入栈的值的数量。
// 这里的 static 关键字表示该函数只能在当前文件中可见，不能在其他文件中使用。
lbaker_bake(lua_State *L) {
    // 从 Lua 栈中获取第一个参数，这里假设该参数是一个指向烘焙器对象的指针
    auto bh = (BakerHandle)lua_touserdata(L, 1);
    // 创建一个用于存储烘焙结果的结构体
    BakeResult br;
    // 调用 Bake 函数执行烘焙操作，并将结果存储在 br 变量中
    Bake(bh, &br);

    // 创建一个 Lua 表，用于存储烘焙结果的光照贴图
    lua_createtable(L, (int)br.lightmaps.size(), 0);
    // 遍历烘焙结果中的每个光照贴图
    for (size_t ii=0; ii<br.lightmaps.size(); ++ii) {
        // 创建一个 Lua 表，用于存储当前光照贴图的信息
        lua_createtable(L, 0, 3);{
            // 获取当前光照贴图的数据并压入 Lua 栈
            const auto &lm = br.lightmaps[ii];
            const auto texelsize = sizeof(glm::vec4);
            lua_pushlstring(L, (const char*)lm.data.data(), lm.data.size() * texelsize);
            // 将数据存储在 Lua 表中，并命名为 "data"
            lua_setfield(L, -2, "data");

            // 将当前光照贴图的尺寸存储在 Lua 表中，并命名为 "size"
            lua_pushinteger(L, lm.size);
            lua_setfield(L, -2, "size");

            // 将当前光照贴图的像素大小存储在 Lua 表中，并命名为 "texelsize"
            lua_pushinteger(L, texelsize);
            lua_setfield(L, -2, "texelsize");
        }
        // 将当前光照贴图的 Lua 表存储在结果表中
        lua_seti(L, -2, ii+1);
    }

    // 返回值为 1，代表压入栈的值数量为 1
    return 1;
}

static int
// 定义了一个静态函数，返回一个整数值，表示 Lua 调用此函数后压入栈的值的数量。
// 这里的 static 关键字表示该函数只能在当前文件中可见，不能在其他文件中使用。
lbaker_destroy(lua_State *L) {
    // 从 Lua 栈中获取第一个参数，这里假设该参数是一个指向烘焙器对象的指针
    auto bh = (BakerHandle)lua_touserdata(L, 1);
    // 调用 DestroyBaker 函数销毁烘焙器对象
    DestroyBaker(bh);
    // 返回值为 0，表示此函数没有返回值压入栈
    return 0;
}

// extern "C" 用于指定以下代码块中的函数使用 C 语言的调用约定，
// 这样可以确保编译器不会修改函数名或参数列表，以允许它们与其他语言进行交互（如 Lua）。
extern "C" {

// LUAMOD_API 宏用于声明一个 Lua 模块的入口函数，
// 它是一个预处理器宏，用于指定函数的导出属性，以便 Lua 虚拟机能够找到并正确调用它。
LUAMOD_API int
// luaopen_bake 是 Lua 加载模块时调用的函数，它应该返回一个整数值，
// 代表加载成功后压入栈的值的数量，通常是 1。
luaopen_bake(lua_State* L) {
    // 定义一个包含函数名和对应 C 函数指针的 luaL_Reg 结构体数组，
    // 其中每个元素都对应一个 Lua 函数及其实现。
    luaL_Reg lib[] = {
        {"create",  lbaker_create},   // 对应 Lua 中的 create 函数
        {"bake",    lbaker_bake},     // 对应 Lua 中的 bake 函数
        {"destroy", lbaker_destroy},  // 对应 Lua 中的 destroy 函数
        { nullptr, nullptr },         // 数组的结尾，用于标识数组结束
    };
    // 创建一个新的 Lua 库，并将其中的函数注册到 Lua 虚拟机中
    luaL_newlib(L, lib);
    // 返回值为 1，代表压入栈的值数量为 1
    return 1;
}

} // extern "C"
}
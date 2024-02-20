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

    // 在解包时，针对特定类型的BufferData，这是一个模板特化函数
    template <>
    // inline关键字用于优化性能，通常用于简单的函数
    inline void unpack<BufferData>(lua_State* L, int idx, BufferData& v, void*) {
        // 检查 Lua 栈上指定位置的值是否为表
        luaL_checktype(L, idx, LUA_TTABLE);
        // 从表中解包"data"字段，并将其赋值给BufferData的"data"成员
        unpack_field(L, idx, "data", v.data);
        // 从表中解包"offset"字段，并将其赋值给BufferData的"offset"成员
        unpack_field(L, idx, "offset", v.offset);
        // 从表中解包"stride"字段，并将其赋值给BufferData的"stride"成员
        unpack_field(L, idx, "stride", v.stride);
        // 声明一个指针，用于存储从 Lua 表中解包出来的"type"字段的值
        const char* type = nullptr;
        // 从 Lua 表中解包"type"字段的值
        unpack_field(L, idx, "type", type);
        // 根据解包得到的数据类型进行处理
        switch (type[0]){
            // 如果数据类型是 'B'，将BufferData的"type"成员设置为 BT_Byte
            case 'B': v.type = BT_Byte; break;
            // 如果数据类型是 'H'，将BufferData的"type"成员设置为 BT_Uint16
            case 'H': v.type = BT_Uint16; break;
            // 如果数据类型是 'I'，将BufferData的"type"成员设置为 BT_Uint32
            case 'I': v.type = BT_Uint32; break;
            // 如果数据类型是 'f'，将BufferData的"type"成员设置为 BT_Float
            case 'f': v.type = BT_Float; break;
            // 如果数据类型是空字符，将BufferData的"type"成员设置为 BT_None
            case '\0':v.type = BT_None; break;
            // 如果数据类型不是上述任何一种，抛出 Lua 错误，提示数据类型无效
            default: luaL_error(L, "invalid data type:%s", type);
        }
    }

    // 对特定类型 MeshData 进行的模板特化函数
    template <>
    // 使用 inline 关键字进行函数内联，可能提高性能
    inline void unpack<MeshData>(lua_State* L, int idx, MeshData& v, void*) {
        // 检查 Lua 栈中指定位置的值是否为表
        luaL_checktype(L, idx, LUA_TTABLE);
        // 从 Lua 表中解包"worldmat"字段，并将其赋值给 MeshData 的 worldmat 成员
        unpack_field(L, idx, "worldmat", v.worldmat);
        // 从 Lua 表中解包"normalmat"字段，并将其赋值给 MeshData 的 normalmat 成员
        unpack_field(L, idx, "normalmat", v.normalmat);

        // 从 Lua 表中解包"positions"字段，并将其赋值给 MeshData 的 positions 成员
        unpack_field(L, idx, "positions", v.positions);
        // 从 Lua 表中解包"normals"字段，并将其赋值给 MeshData 的 normals 成员
        unpack_field(L, idx, "normals",   v.normals);
        // 设置 tangents 成员的 type 为 BT_None
        v.tangents.type = BT_None;
        // 从 Lua 表中解包"tangents"字段，并将其赋值给 MeshData 的 tangents 成员（可选字段）
        unpack_field_opt(L, idx, "tangents", v.tangents);
        // 设置 bitangents 成员的 type 为 BT_None
        v.bitangents.type = BT_None;
        // 从 Lua 表中解包"bitangents"字段，并将其赋值给 MeshData 的 bitangents 成员（可选字段）
        unpack_field_opt(L, idx, "bitangents", v.bitangents);

        // 从 Lua 表中解包"texcoords0"字段，并将其赋值给 MeshData 的 texcoords0 成员
        unpack_field(L, idx, "texcoords0", v.texcoords0);

        // 设置 texcoords1 成员的 type 为 BT_None
        v.texcoords1.type = BT_None;
        // 从 Lua 表中解包"texcoords1"字段，并将其赋值给 MeshData 的 texcoords1 成员（可选字段）
        unpack_field_opt(L, idx, "texcoords1", v.texcoords1);
        // 如果 texcoords1 成员的 type 为 BT_None，则将其值设置为 texcoords0 的值
        if (v.texcoords1.type == BT_None){
            v.texcoords1 = v.texcoords0;
        }

        // 从 Lua 表中解包"vertexCount"字段，并将其赋值给 MeshData 的 vertexCount 成员
        unpack_field(L, idx, "vertexCount", v.vertexCount);

        // 为 indices 成员的 type 设置为 BT_None
        v.indices.type = BT_None;
        // 从 Lua 表中解包"indices"字段，并将其赋值给 MeshData 的 indices 成员（可选字段）
        unpack_field_opt(L, idx, "indices", v.indices);
        // 设置 indexCount 成员的初始值为 0
        v.indexCount = 0;
        // 从 Lua 表中解包"indexCount"字段，并将其赋值给 MeshData 的 indexCount 成员（可选字段）
        unpack_field_opt(L, idx, "indexCount", v.indexCount);

        // 从 Lua 表中解包"materialidx"字段，并将其赋值给 MeshData 的 materialidx 成员
        unpack_field(L, idx, "materialidx", v.materialidx);
        // 确保 materialidx 大于 0
        assert(v.materialidx > 0);
        // 将 materialidx 减去 1（一般索引从 0 开始）
        --v.materialidx;

        // 从 Lua 表中解包"lightmap"字段，并将其赋值给 MeshData 的 lightmap 成员
        unpack_field(L, idx, "lightmap", v.lightmap);
    }

    // 对特定类型 MaterialData 进行的模板特化函数
    template <>
    // 使用 inline 关键字进行函数内联，可能提高性能
    inline void unpack<MaterialData>(lua_State* L, int idx, MaterialData& v, void*) {
        // 检查 Lua 栈中指定位置的值是否为表
        luaL_checktype(L, idx, LUA_TTABLE);
        // 从 Lua 表中解包"diffuse"字段，并将其赋值给 MaterialData 的 diffuse 成员（可选字段）
        unpack_field_opt(L, idx, "diffuse", v.diffuse);
        // 从 Lua 表中解包"normal"字段，并将其赋值给 MaterialData 的 normal 成员（可选字段）
        unpack_field_opt(L, idx, "normal", v.normal);
        // 从 Lua 表中解包"roughness"字段，并将其赋值给 MaterialData 的 roughness 成员（可选字段）
        unpack_field_opt(L, idx, "roughness", v.roughness);
        // 从 Lua 表中解包"metallic"字段，并将其赋值给 MaterialData 的 metallic 成员（可选字段）
        unpack_field_opt(L, idx, "metallic",   v.metallic);
    }

    // 对特定类型 Light 进行的模板特化函数
    template <>
    // 使用 inline 关键字进行函数内联，可能提高性能
    inline void unpack<Light>(lua_State* L, int idx, Light& v, void*) {
        // 检查 Lua 栈中指定位置的值是否为表
        luaL_checktype(L, idx, LUA_TTABLE);
        // 从 Lua 表中解包"dir"字段，并将其赋值给 Light 结构体的 dir 成员
        unpack_field(L, idx, "dir", v.dir);
        // 从 Lua 表中解包"pos"字段，并将其赋值给 Light 结构体的 pos 成员
        unpack_field(L, idx, "pos", v.pos);
        // 从 Lua 表中解包"color"字段，并将其赋值给 Light 结构体的 color 成员
        unpack_field(L, idx, "color", v.color);

        // 从 Lua 表中解包"intensity"字段，并将其赋值给 Light 结构体的 intensity 成员
        unpack_field(L, idx, "intensity", v.intensity);
        // 从 Lua 表中解包"range"字段，并将其赋值给 Light 结构体的 range 成员
        unpack_field(L, idx, "range", v.range);
        // 从 Lua 表中解包"inner_cutoff"字段，并将其赋值给 Light 结构体的 inner_cutoff 成员
        unpack_field(L, idx, "inner_cutoff", v.inner_cutoff);
        // 从 Lua 表中解包"outter_cutoff"字段，并将其赋值给 Light 结构体的 outter_cutoff 成员
        unpack_field(L, idx, "outter_cutoff", v.outter_cutoff);
        // 从 Lua 表中解包"angular_radius"字段，并将其赋值给 Light 结构体的 angular_radius 成员
        unpack_field(L, idx, "angular_radius", v.angular_radius);

        // 声明一个指针，用于存储从 Lua 表中解包出来的"type"字段的值
        const char* type = nullptr;
        // 从 Lua 表中解包"type"字段的值
        unpack_field(L, idx, "type", type);
        // 根据解包得到的光源类型进行处理
        if (strcmp(type, "directional") == 0){
            v.type = Light::LT_Directional;
        } else if (strcmp(type, "point") == 0){
            v.type = Light::LT_Point;
            // 对于点光源，确保其 range 不为 0
            if (v.range == 0.f){
                luaL_error(L, "invalid point light, range must not be 0.0");
            }
        } else if (strcmp(type, "spot") == 0){
            v.type = Light::LT_Spot;
            // 对于聚光灯光源，确保其 range、inner_cutoff 和 outter_cutoff 不为 0
            if (v.range == 0.f){
                luaL_error(L, "invalid spot light, range must not be 0.0");
            }
            if (v.inner_cutoff == 0.f || v.outter_cutoff == 0.f){
                luaL_error(L, "invalid spot light, inner_cutoff and outter_cutoff must not be 0.0");
            }
        } else if (strcmp(type, "area") == 0){
            v.type = Light::LT_Area;
            // 对于面光源，确保其 angular_radius 不为 0
            if (v.angular_radius == 0.f){
                luaL_error(L, "invalid area light, angular_radius must not be 0.0");
            }
        } else {
            // 如果光源类型不是上述任何一种，抛出 Lua 错误，提示光源类型无效
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
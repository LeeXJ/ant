//use for exprot interface
#pragma once

typedef void* BakerHandle;

#include <cstdint>
#include <vector>
#include <string>
#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

// Light 结构体定义了描述光源的各种属性
struct Light {
    // 光源的位置
    glm::vec3 pos;
    // 光源的方向
    glm::vec3 dir;

    // 光源的颜色
    glm::vec3 color;
    // 光源的强度
    float intensity;

    // 光源的范围
    float range;
    // 光源的内切角
    float inner_cutoff;
    // 光源的外切角
    float outter_cutoff;
    // 光源的角度半径
    float angular_radius;

    // 枚举类型，表示光源的类型
    enum LightType {
        LT_Directional = 0, // 方向光
        LT_Point,           // 点光源
        LT_Spot,            // 聚光灯
        LT_Area,            // 面光源
    };
    // 光源的类型
    LightType type;
};

// MaterialData 结构体定义了描述材质的各种属性
struct MaterialData{
    // 漫反射贴图
    std::string diffuse;
    // 法线贴图
    std::string normal;
    // 粗糙度贴图
    std::string roughness;
    // 金属度贴图
    std::string metallic;
};

// BufferType 枚举类型用于表示缓冲区的数据类型
enum BufferType {
    BT_None = 0,    // 无类型
    BT_Byte,        // 字节类型
    BT_Uint16,      // 16位无符号整型
    BT_Uint32,      // 32位无符号整型
    BT_Float,       // 浮点型
};

// BufferData 结构体定义了描述缓冲区数据的各种属性
struct BufferData{
    const char* data;   // 缓冲区数据的指针
    uint32_t offset;    // 缓冲区偏移量
    uint32_t stride;    // 缓冲区步长
    BufferType type;    // 缓冲区数据类型
};

// Lightmap 结构体定义了描述光照贴图的属性
struct Lightmap {
    uint16_t size;  // 光照贴图的尺寸
};

// MeshData 结构体定义了描述网格数据的各种属性
struct MeshData {
    glm::mat4 worldmat;     // 世界变换矩阵
    glm::mat4 normalmat;    // 法线变换矩阵

    BufferData positions;   // 顶点数据缓冲区
    BufferData normals;     // 法线数据缓冲区
    BufferData tangents;    // 切线数据缓冲区
    BufferData bitangents;  // 双切线数据缓冲区
    BufferData texcoords0;  // 第一组纹理坐标数据缓冲区
    BufferData texcoords1;  // 第二组纹理坐标数据缓冲区
    BufferData indices;     // 索引数据缓冲区

    uint32_t vertexCount;   // 顶点数量
    uint32_t indexCount;    // 索引数量
    uint32_t materialidx;   // 材质索引

    Lightmap lightmap;      // 光照贴图
};

struct Sky{
    enum SkyType{
        SimpleColor = 0,
        CubeMap = 1,
    };
    SkyType     type;
    std::string cubemapTexture;
    glm::vec3   skyColor;
};

struct Scene {
    std::vector<Light>          lights;
    std::vector<MeshData>       models;
    std::vector<MaterialData>   materials;

    Sky sky;
};

struct LightmapResult{
    std::vector<glm::vec4>   data;
    uint16_t size;
};

struct BakeResult {
    std::vector<LightmapResult> lightmaps;
};

extern BakerHandle CreateBaker(const Scene* scene);
extern void Bake(BakerHandle handle, BakeResult *result);
extern void DestroyBaker(BakerHandle handle);
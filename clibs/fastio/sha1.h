#pragma once

#include <stdint.h>

typedef struct {
	uint32_t state[5];
	uint32_t count[2];
	uint8_t  buffer[64];
} SHA1_CTX;
 
#define SHA1_DIGEST_SIZE 20

void sat_SHA1_Init(SHA1_CTX* context);
void sat_SHA1_Update(SHA1_CTX* context,	const uint8_t* data, const size_t len);
void sat_SHA1_Final(SHA1_CTX* context, uint8_t digest[SHA1_DIGEST_SIZE]);

// 这段代码定义了一个 SHA-1 哈希算法的接口：

// 1. **`SHA1_CTX` 结构体定义**：包含了 SHA-1 计算中需要的状态、计数和缓冲区等信息。
// 2. **`SHA1_DIGEST_SIZE` 宏定义**：指定了 SHA-1 哈希值的长度为 20 字节。
// 3. **`sat_SHA1_Init` 函数**：用于初始化 SHA-1 计算上下文。
// 4. **`sat_SHA1_Update` 函数**：用于向 SHA-1 计算上下文中添加数据，并根据需要进行哈希计算。
// 5. **`sat_SHA1_Final` 函数**：用于结束 SHA-1 计算，并生成最终的哈希值。

// 这些函数和数据结构提供了一个简单的接口，允许用户使用 SHA-1 哈希算法来处理数据。
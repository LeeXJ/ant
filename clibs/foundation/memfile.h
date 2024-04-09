#ifndef memory_file_h
#define memory_file_h

#include <stdlib.h>

typedef void (*memory_file_closefunc)(void *ud);

struct memory_file {
	void *ud;
	const char *data;
	size_t sz;
	memory_file_closefunc close;
};

static inline void
memory_file_close(struct memory_file *mf) {
	if (mf) {
		mf->close(mf->ud);
	}
}

static inline struct memory_file *
memory_file_cstr(const char *data, size_t sz) {
	struct memory_file * cmf = (struct memory_file *)malloc(sizeof(*cmf));
	if (cmf == NULL)
		return NULL;
	cmf->ud = (void *)cmf;
	cmf->data = data;
	cmf->sz = sz;
	cmf->close = free;
	return cmf;
}

static inline struct memory_file *
memory_file_alloc(size_t sz) {
	struct memory_file * cmf = (struct memory_file *)malloc(sizeof(*cmf) + sz);
	if (cmf == NULL)
		return NULL;
	cmf->ud = (void *)cmf;
	cmf->data = (const char *)(cmf + 1);
	cmf->sz = sz;
	cmf->close = free;
	return cmf;
}

#endif

// 这是一个简单的内存文件操作的 C 语言头文件。它定义了一个结构体 `memory_file`，表示内存中的文件，包含以下字段：

// - `ud`: 一个指向用户数据的指针，可用于存储自定义的数据。
// - `data`: 指向文件数据的指针。
// - `sz`: 文件数据的大小。
// - `close`: 一个函数指针，用于关闭文件并释放资源。

// 该头文件提供了以下几个函数：

// - `memory_file_close(struct memory_file *mf)`: 关闭内存文件，并释放相关资源。
// - `memory_file_cstr(const char *data, size_t sz)`: 创建一个指向以 null 结尾的 C 字符串的内存文件。
// - `memory_file_alloc(size_t sz)`: 分配指定大小的内存文件。

// 这些函数可以用于创建和操作内存中的文件。
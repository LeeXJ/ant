#pragma once

extern "C" {
#include "mathid.h"
}

// 这段代码使用了 `#pragma once` 来确保头文件只被编译一次。
// 然后，通过 `extern "C"` 来定义了一个代码块，其中包含了 `mathid.h` 头文件的引用，
// 这表示该头文件中的内容在 C++ 环境中应当以 C 的方式进行编译和链接。
#pragma once
#include <stdint.h>

namespace gcc_demo
{

#if (defined(_WIN32) || defined(WIN64))
#define SYS_WINDOWS
#elif defined(linux) || defined(LINUX)
#define SYS_LINUX
#elif defined(ANDROID)
#define SYS_ANDROID
#elif defined(__APPLE__)
#define SYS_APPLE
#endif
}


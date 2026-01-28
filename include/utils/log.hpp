#pragma once

#include <cassert>
#include <iostream>

#ifdef LL_DEBUG
#define DEBUG_LOG(message) \
    do { std::cerr << "[DEBUG] " << message << '\n'; } while (0)
#else
#define DEBUG_LOG(message) do { } while (0)
#endif

#define LL_ASSERT(condition) assert(condition)

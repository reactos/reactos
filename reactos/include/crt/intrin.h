
#pragma once

#ifdef __MINGW32__
#  include "mingw32/intrin.h"
#elif defined(_MSC_VER)
#  include "msc/intrin.h"
#else
#  error Please implement intrinsics for your target compiler
#endif

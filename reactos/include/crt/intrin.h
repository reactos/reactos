
#pragma once

#ifndef RC_INVOKED
#if defined(__GNUC__) && defined(_WIN32) // We can't use __MINGW32__ here
#  include "mingw32/intrin.h"
#elif defined(_MSC_VER)
#  include "msc/intrin.h"
#else
#  error Please implement intrinsics for your target compiler
#endif
#endif

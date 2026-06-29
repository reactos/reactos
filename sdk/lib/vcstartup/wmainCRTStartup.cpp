//
// wmainCRTStartup.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of unicode executable entry point.
//
// SPDX-License-Identifier: MIT
//

#include "commonCRTStartup.hpp"

extern "C" unsigned long wmainCRTStartup(void*)
{
    __security_init_cookie();

    return __commonCRTStartup<decltype(wmain)>();
}

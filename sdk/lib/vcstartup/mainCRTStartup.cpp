//
// mainCRTStartup.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of ANSI executable entry point.
//
// SPDX-License-Identifier: MIT
//

#include "commonCRTStartup.hpp"

extern "C" unsigned long mainCRTStartup(void*)
{
    __security_init_cookie();

    return __commonCRTStartup<decltype(main)>();
}

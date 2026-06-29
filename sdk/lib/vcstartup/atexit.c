//
// atexit.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of atexit.
//
// SPDX-License-Identifier: MIT
//

#include <stdlib.h>

int __cdecl atexit(void (__cdecl* _Func)(void))
{
    // Go through _onexit, so that the initializer is pulled in.
    _onexit_t result = _onexit((_onexit_t)_Func);
    return (result == NULL) ? -1 : 0;
}

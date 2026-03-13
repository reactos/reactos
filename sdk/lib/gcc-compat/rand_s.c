/*
 * PROJECT:     GCC C++ support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Workaround for missing rand_s on pre-NT6 builds
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>

errno_t __cdecl rand_s(_Out_ unsigned int *RandomValue)
{
    if (!RandomValue)
        return EINVAL;

    *RandomValue = rand();
    return 0;
}

#ifdef _M_IX86
void* _imp__rand_s = rand_s;
#else
void* __imp_rand_s = rand_s;
#endif

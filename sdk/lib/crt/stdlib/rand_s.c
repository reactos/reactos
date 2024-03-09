/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     rand_s implementation
 * COPYRIGHT:   Timo Kreuzer <timo.kreuzer@reactos.org>
 */

// Evil hack necessary, because we're linking to the RosBE-provided libstdc++ when using GCC.
// This can only be solved cleanly by adding a GCC-compatible C++ standard library to our tree.
#ifdef __GNUC__

#include <precomp.h>

/*********************************************************************
 *    __acrt_iob_func(MSVCRT.@)
 */
errno_t rand_s(unsigned int* randomValue)
{
    if (!randomValue)
    {
        return EINVAL;
    }

    *randomValue = rand();
    return 0;
}

#ifdef WIN64
const void* __imp_rand_s = rand_s;
#else
const void* _imp_rand_s = rand_s;
#endif

#endif

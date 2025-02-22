/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     __acrt_iob_func implementation
 * COPYRIGHT:   Victor Perevertkin <victor.perevertkin@reactos.org>
 */

// Evil hack necessary, because we're linking to the RosBE-provided libstdc++ when using GCC.
// This can only be solved cleanly by adding a GCC-compatible C++ standard library to our tree.
#ifdef __GNUC__

#include <precomp.h>

/*********************************************************************
 *    __acrt_iob_func(MSVCRT.@)
 */
FILE * CDECL __acrt_iob_func(int index)
{
    return &__iob_func()[index];
}

#ifdef WIN64
const void* __imp___acrt_iob_func = __acrt_iob_func;
#else
const void* _imp____acrt_iob_func = __acrt_iob_func;
#endif

#endif

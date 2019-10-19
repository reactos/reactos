/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     __acrt_iob_func implementation
 * COPYRIGHT:   Victor Perevertkin <victor.perevertkin@reactos.org>
 */

#include <precomp.h>

/*********************************************************************
 *    __acrt_iob_func(MSVCRT.@)
 */
FILE * CDECL __acrt_iob_func(int index)
{
    return &__iob_func()[index];
}

// Evil hack necessary, because we're linking to the RosBE-provided libstdc++ when using GCC.
// This can only be solved cleanly by adding a GCC-compatible C++ standard library to our tree.
const void* _imp____acrt_iob_func = __acrt_iob_func;

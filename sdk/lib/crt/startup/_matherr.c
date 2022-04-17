/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _matherr dummy
 * COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

// DO NOT SYNC WITH WINE OR MINGW32

#include <math.h>

/* Dummy function, like in MS CRT */
int
__cdecl
_matherr(struct _exception *pexcept)
{
    return 0;
}

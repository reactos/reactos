/*
 * COPYRIGHT:       BSD - See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS CRT library
 * PURPOSE:         Implementation of fabsf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <math.h>

_Check_return_
float
__cdecl
fabsf(
    _In_ float x)
{
    return (float)fabs((double)x);
}


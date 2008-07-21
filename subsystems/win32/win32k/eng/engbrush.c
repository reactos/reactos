/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engbrush.c
 * PURPOSE:         BRUSHOBJ Manipulation Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HANDLE
APIENTRY
BRUSHOBJ_hGetColorTransform(IN BRUSHOBJ* BrushObj)
{
    UNIMPLEMENTED;
    return NULL;
}

PVOID
APIENTRY
BRUSHOBJ_pvAllocRbrush(IN BRUSHOBJ* BrushObj,
                       IN ULONG ObjSize)
{
    UNIMPLEMENTED;
    return NULL;
}

PVOID
APIENTRY
BRUSHOBJ_pvGetRbrush(IN BRUSHOBJ* BrushObj)
{
    UNIMPLEMENTED;
    return NULL;
}

ULONG
APIENTRY
BRUSHOBJ_ulGetBrushColor(IN BRUSHOBJ* BrushObj)
{
    UNIMPLEMENTED;
    return 0;
}

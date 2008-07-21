/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engclip.c
 * PURPOSE:         CLIPOBJ Manipulation Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
CLIPOBJ_bEnum(IN CLIPOBJ* ClipObj,
              IN ULONG ObjSize,
              OUT PULONG EnumRects)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG
APIENTRY
CLIPOBJ_cEnumStart(IN CLIPOBJ* ClipObj,
                   IN BOOL DoAll,
                   IN ULONG ClipType,
                   IN ULONG BuildOrder,
                   IN ULONG MaxRects)
{
    UNIMPLEMENTED;
    return 0;
}

PATHOBJ*
APIENTRY
CLIPOBJ_ppoGetPath(IN CLIPOBJ* ClipObj)
{
    UNIMPLEMENTED;
    return NULL;
}

CLIPOBJ*
APIENTRY
EngCreateClip(VOID)
{
    UNIMPLEMENTED;
    return NULL;
}

VOID
APIENTRY
EngDeleteClip(CLIPOBJ* ClipObj)
{
    UNIMPLEMENTED;
}

/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engfloat.c
 * PURPOSE:         Floating Point Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

ULONG
APIENTRY
EngSaveFloatingPointState(OUT PVOID Buffer,
                          IN ULONG cjBufferSize)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
EngRestoreFloatingPointState(IN PVOID Buffer)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
APIENTRY
FLOATOBJ_AddFloatObj(PFLOATOBJ Float1,
                     PFLOATOBJ Float2)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
FLOATOBJ_DivFloatObj(PFLOATOBJ Float1,
                     PFLOATOBJ Float2)
{
    UNIMPLEMENTED;
}


VOID
APIENTRY
FLOATOBJ_MulFloatObj(PFLOATOBJ Float1,
                     PFLOATOBJ Float2)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
FLOATOBJ_SubFloatObj(PFLOATOBJ Float1,
                     PFLOATOBJ Float2)
{
    UNIMPLEMENTED;
}

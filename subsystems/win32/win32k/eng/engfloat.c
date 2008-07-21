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
FLOATOBJ_Add (IN OUT PFLOATOBJ Float1,
              IN PFLOATOBJ Float2)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
FLOATOBJ_AddFloat(IN OUT PFLOATOBJ Float,
                  IN FLOATL fAddend)
{
    UNIMPLEMENTED;
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
FLOATOBJ_AddLong(IN OUT PFLOATOBJ Float,
                 IN LONG lAddend)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
FLOATOBJ_Div(IN OUT PFLOATOBJ Float1,
             IN PFLOATOBJ Float2)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
FLOATOBJ_DivFloat(IN OUT PFLOATOBJ Float,
                  IN FLOATL fDivisor)
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
FLOATOBJ_DivLong(IN OUT PFLOATOBJ Float,
                 IN LONG lDivisor)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
FLOATOBJ_Mul(IN OUT PFLOATOBJ Float1,
             IN PFLOATOBJ Float2)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
FLOATOBJ_MulFloat(IN OUT PFLOATOBJ Float1,
                  IN FLOATL fMultiplicand)
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
FLOATOBJ_MulLong(IN OUT PFLOATOBJ Float,
                 IN LONG lMultiplicand)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
FLOATOBJ_Sub(IN OUT PFLOATOBJ Float1,
             IN PFLOATOBJ Float2)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
FLOATOBJ_SubFloat(IN OUT PFLOATOBJ Float,
                  IN FLOATL fSubtrahend)
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

VOID
APIENTRY
FLOATOBJ_SubLong(IN OUT PFLOATOBJ Float,
                 IN LONG lSubtrahend)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
FLOATOBJ_Neg(IN OUT PFLOATOBJ Float)
{
    UNIMPLEMENTED;
}

LONG
APIENTRY
FLOATOBJ_GetFloat(IN PFLOATOBJ Float)
{
    UNIMPLEMENTED;
    return 0;
}

LONG
APIENTRY
FLOATOBJ_GetLong(IN PFLOATOBJ Float)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
APIENTRY
FLOATOBJ_SetFloat(OUT PFLOATOBJ Float,
                  IN FLOATL fAssignement)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
FLOATOBJ_SetLong(OUT PFLOATOBJ Float,
                 IN LONG lAssignment)
{
    UNIMPLEMENTED;
}

BOOL
APIENTRY
FLOATOBJ_Equal(IN PFLOATOBJ Float1,
               IN PFLOATOBJ Float2)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
FLOATOBJ_EqualLong(IN PFLOATOBJ Float,
                   IN LONG lCompare)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
FLOATOBJ_GreaterThan(IN PFLOATOBJ Float1,
                     IN PFLOATOBJ Float2)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
FLOATOBJ_GreaterThanLong(IN PFLOATOBJ Float,
                         IN LONG lCompare)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
FLOATOBJ_LessThan(IN PFLOATOBJ Float1,
                  IN PFLOATOBJ Float2)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
FLOATOBJ_LessThanLong(IN PFLOATOBJ Float,
                      IN LONG lCompare)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include "font.h"

#define NDEBUG
#include <debug.h>


/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
ESTROBJ_vInit(
    IN ESTROBJ *pestro,
    IN PWSTR pwsz,
    IN ULONG cwc)
{

}


/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
STROBJ_bEnum(
	IN STROBJ *pstro,
	OUT ULONG *pc,
	OUT PGLYPHPOS *ppgpos)
{
    ASSERT(FALSE);
    return FALSE;
}

DWORD
APIENTRY
STROBJ_dwGetCodePage(IN STROBJ *pstro)
{
    ASSERT(FALSE);
    return FALSE;
}

VOID
APIENTRY
STROBJ_vEnumStart(IN STROBJ *pstro)
{
    ASSERT(FALSE);
}

BOOL
APIENTRY
STROBJ_bEnumPositionsOnly(
   IN STROBJ *StringObj,
   OUT ULONG *Count,
   OUT PGLYPHPOS *Pos)
{
    ASSERT(FALSE);
    return (BOOL) DDI_ERROR;
}

ENGAPI
BOOL
APIENTRY
STROBJ_bGetAdvanceWidths(
    STROBJ *pso,
    ULONG iFirst,
    ULONG c,
    POINTQF *pptqD)
{
    ASSERT(FALSE);
    return FALSE;
}

ENGAPI
FIX
APIENTRY
STROBJ_fxBreakExtra(
    STROBJ *pstro)
{
    ASSERT(FALSE);
    return 0;
}

ENGAPI
FIX
APIENTRY
STROBJ_fxCharacterExtra(
    STROBJ  *pstro)
{
    ASSERT(FALSE);
    return 0;
}


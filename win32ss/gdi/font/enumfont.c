/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Font enumeration
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include "font.h"

#define NDEBUG
#include <debug.h>

W32KAPI
BOOL
APIENTRY
NtGdiEnumFontChunk(
    IN HDC hdc,
    IN ULONG_PTR idEnum,
    IN ULONG cjEfdw,
    OUT ULONG *pcjEfdw,
    OUT PENUMFONTDATAW pefdw)
{
    ASSERT(FALSE);
#if 0
    PEFSTATE pefstate;

    /* Lock the EFSTATE object */
    pefstate = GDIOBJ_LockObject((HGDIOBJ)idEnum, GDIObjType_EFSTATE_TYPE);
    if (!pefstate)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

#endif
    return 0;
}

W32KAPI
ULONG_PTR
APIENTRY
NtGdiEnumFontOpen(
    IN HDC hdc,
    IN ULONG iEnumType,
    IN FLONG flWin31Compat,
    IN ULONG cwchMax,
    IN OPTIONAL LPWSTR pwszFaceName,
    IN ULONG lfCharSet,
    OUT ULONG *pulCount)
{
    ULONG_PTR idEnum = 0;

    ASSERT(FALSE);

    /* Allocate an EFSTATE object */


    return idEnum;
}


W32KAPI
BOOL
APIENTRY
NtGdiEnumFontClose(
    IN ULONG_PTR idEnum)
{

    /* Verify that idEnum is a handle to an EFSTATE object */
    if (((idEnum >> 16) & 0x1f) != GDIObjType_EFSTATE_TYPE)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Delete the object */
    return GreDeleteObject((HGDIOBJ)idEnum);
}

W32KAPI
ULONG
APIENTRY
NtGdiSetFontEnumeration(
    IN ULONG ulType)
{
    ASSERT(FALSE);
    return 0;
}


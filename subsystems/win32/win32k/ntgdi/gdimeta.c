/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdimeta.c
 * PURPOSE:         GDI Metafile Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

LONG
APIENTRY
NtGdiConvertMetafileRect(IN HDC hdc,
                         IN OUT PRECTL prect)
{
    UNIMPLEMENTED;
    return 0;
}

HDC
APIENTRY
NtGdiCreateMetafileDC(IN HDC hdc)
{
    UNIMPLEMENTED;
    return NULL;
}

HANDLE
APIENTRY
NtGdiCreateServerMetaFile(IN DWORD iType,
                          IN ULONG cjData,
                          IN LPBYTE pjData,
                          IN DWORD mm,
                          IN DWORD xExt,
                          IN DWORD yExt)
{
    UNIMPLEMENTED;
    return NULL;
}

ULONG
APIENTRY
NtGdiGetServerMetaFileBits(IN HANDLE hmo,
                           IN ULONG cjData,
                           OUT OPTIONAL LPBYTE pjData,
                           OUT PDWORD piType,
                           OUT PDWORD pmm,
                           OUT PDWORD pxExt,
                           OUT PDWORD pyExt)
{
    UNIMPLEMENTED;
    return 0;
}

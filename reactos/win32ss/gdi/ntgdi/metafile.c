/*
 * PROJECT:         ReactOS Win32k Subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/metafile.c
 * PURPOSE:         Metafile Implementation
 * PROGRAMMERS:     ...
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/* System Service Calls ******************************************************/

/*
 * @unimplemented
 */
LONG
APIENTRY
NtGdiConvertMetafileRect(IN HDC hDC,
                         IN OUT PRECTL pRect)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
HDC
APIENTRY
NtGdiCreateMetafileDC(IN HDC hdc)
{
    /* Call the internal function to create an alternative info DC */
    return GreCreateCompatibleDC(hdc, TRUE);
}

/*
 * @unimplemented
 */
HANDLE
APIENTRY
NtGdiCreateServerMetaFile(IN DWORD iType,
                          IN ULONG cjData,
                          IN PBYTE pjData,
                          IN DWORD mm,
                          IN DWORD xExt,
                          IN DWORD yExt)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiGetServerMetaFileBits(IN HANDLE hmo,
                           IN ULONG cjData,
                           OUT OPTIONAL PBYTE pjData,
                           OUT PDWORD piType,
                           OUT PDWORD pmm,
                           OUT PDWORD pxExt,
                           OUT PDWORD pyExt)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */

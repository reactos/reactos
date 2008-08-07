/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdiprint.c
 * PURPOSE:         GDI Printing Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

INT
APIENTRY
NtGdiStartDoc(IN HDC hdc,
              IN DOCINFOW *pdi,
              OUT BOOL *pbBanding,
              IN INT iJob)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiEndDoc(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiAbortDoc(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiStartPage(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiEndPage(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiDoBanding(IN HDC hdc,
               IN BOOL bStart,
               OUT POINTL *pptl,
               OUT PSIZE pSize)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtGdiExtEscape(IN HDC hdc,
               IN OPTIONAL PWCHAR pDriver,
               IN INT nDriver,
               IN INT iEsc,
               IN INT cjIn,
               IN OPTIONAL LPSTR pjIn,
               IN INT cjOut,
               OUT OPTIONAL LPSTR pjOut)
{
    UNIMPLEMENTED;
    return 0;
}

ULONG
APIENTRY
NtGdiGetPerBandInfo(IN HDC hdc,
                    IN OUT PERBANDINFO *ppbi)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiUnloadPrinterDriver(IN LPWSTR pDriverName,
                         IN ULONG cbDriverName)
{
    UNIMPLEMENTED;
    return FALSE;
}

/* FIXME: Parameters! */
INT
APIENTRY
NtGdiGetSpoolMessage( DWORD u1,
                      DWORD u2,
                      DWORD u3,
                      DWORD u4)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
/* Missing APIENTRY! */
NtGdiSetPUMPDOBJ(IN HUMPD humpd,
                 IN BOOL bStoreID,
                 OUT HUMPD *phumpd,
                 OUT BOOL *pbWOW64)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
/* Missing APIENTRY! */
NtGdiUMPDEngFreeUserMem(IN KERNEL_PVOID *ppv)
{
    UNIMPLEMENTED;
    return FALSE;
}

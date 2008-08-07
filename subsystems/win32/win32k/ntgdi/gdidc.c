/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdidc.c
 * PURPOSE:         GDI Device Context Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtGdiCancelDC(IN HDC hDC)
{
    UNIMPLEMENTED;
    return FALSE;
}

HDC
APIENTRY
NtGdiCreateCompatibleDC(IN HDC hDC)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiGetAndSetDCDword(IN HDC hdc,
                      IN UINT u,
                      IN DWORD dwIn,
                      OUT DWORD *pdwResult)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiGetDCDword(IN HDC hdc,
                IN UINT u,
                OUT DWORD *Result)
{
    UNIMPLEMENTED;
    return FALSE;
}

HANDLE
APIENTRY
NtGdiGetDCObject(IN HDC hDC,
                 IN INT iType)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiGetDCPoint(IN HDC hDC,
                IN UINT iPoint,
                OUT PPOINTL pptOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiMakeInfoDC(IN HDC hdc,
                IN BOOL bSet)
{
    UNIMPLEMENTED;
    return FALSE;
}

HDC
APIENTRY
NtGdiOpenDCW(IN OPTIONAL PUNICODE_STRING pustrDevice,
             IN DEVMODEW *pdm,
             IN PUNICODE_STRING pustrLogAddr,
             IN ULONG iType,
             IN OPTIONAL HANDLE hspool,
             IN OPTIONAL VOID *pDriverInfo2,
             OUT VOID *pUMdhpdev)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiResetDC(IN HDC hdc,
             IN LPDEVMODEW pdm,
             OUT PBOOL pbBanding,
             IN OPTIONAL VOID *pDriverInfo2,
             OUT VOID *ppUMdhpdev)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiRestoreDC(IN HDC hdc,
               IN INT iLevel)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtGdiSaveDC(IN HDC hdc)
{
    UNIMPLEMENTED;
    return 0;
}

INT
APIENTRY
NtGdiDrawEscape(IN HDC hdc,
                IN INT iEsc,
                IN INT cjIn,
                IN OPTIONAL LPSTR pjIn)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiDeleteObjectApp(IN HANDLE hobj)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtGdiExtGetObjectW(IN HANDLE h,
                   IN INT cj,
                   OUT OPTIONAL LPVOID pvOut)
{
    UNIMPLEMENTED;
    return 0;
}

ULONG
APIENTRY
NtGdiEnumObjects(IN HDC hdc,
                 IN INT iObjectType,
                 IN ULONG cjBuf,
                 OUT OPTIONAL PVOID pvBuf)
{
    UNIMPLEMENTED;
    return 0;
}

INT
APIENTRY
NtGdiGetDeviceCaps(IN HDC hdc,
                   IN INT i)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiGetDeviceCapsAll(IN HDC hdc,
                      OUT PDEVCAPS pDevCaps)
{
    UNIMPLEMENTED;
    return FALSE;
}

LONG
APIENTRY
NtGdiGetDeviceWidth(IN HDC hdc)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiGetMonitorID(IN HDC hdc,
                  IN DWORD dwSize,
                  OUT LPWSTR pszMonitorID)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
APIENTRY
NtGdiGetStats(IN HANDLE hProcess,
              IN INT iIndex,
              IN INT iPidType,
              OUT PVOID pResults,
              IN UINT cjResultSize)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

DWORD
APIENTRY
NtGdiSetLayout(IN HDC hdc,
               IN LONG wox,
               IN DWORD dwLayout)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
APIENTRY
NtGdiFlush(VOID)
{
    UNIMPLEMENTED;
}

NTSTATUS
APIENTRY
NtGdiFlushUserBatch(VOID)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

HANDLE
APIENTRY
NtGdiCreateClientObj(IN ULONG ulType)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiDeleteClientObj(IN HANDLE h)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
/* Missing APIENTRY! */
NtGdiAddRemoteMMInstanceToDC(IN HDC hdc,
                             IN DOWNLOADDESIGNVECTOR *pddv,
                             IN ULONG cjDDV)
{
    UNIMPLEMENTED;
    return FALSE;
}

DHPDEV
/* Missing APIENTRY! */
NtGdiGetDhpdev(IN HDEV hdev)
{
    UNIMPLEMENTED;
    return FALSE;
}

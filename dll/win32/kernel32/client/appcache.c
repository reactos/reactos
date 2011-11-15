/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/appcache.c
 * PURPOSE:         Application Compatibility Cache
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
BasepCheckBadapp(int a, wchar_t *Str, int b, int c, int d, int e, int f, int g, int h)
{
    STUB;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
WINAPI
BaseDumpAppcompatCache(VOID)
{
    STUB;
}

/*
 * @unimplemented
 */
VOID
WINAPI
BaseFlushAppcompatCache(VOID)
{
    STUB;
}

/*
 * @unimplemented
 */
VOID
WINAPI
BasepFreeAppCompatData(PVOID A, PVOID B)
{
    STUB;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
BaseIsAppcompatInfrastructureDisabled(VOID)
{
    STUB;
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
BaseCheckAppcompatCache(ULONG Unknown1,
                        ULONG Unknown2,
                        ULONG Unknown3,
                        PULONG Unknown4)
{
    STUB;
    if (Unknown4) *Unknown4 = 0;
    return TRUE;
}

/*
 * @unimplemented
 */
VOID
WINAPI
BaseUpdateAppcompatCache(ULONG Unknown1,
                         ULONG Unknown2,
                         ULONG Unknown3)
{
    STUB;
}

NTSTATUS
WINAPI
BaseCleanupAppcompatCache(VOID)
{
    STUB;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
WINAPI
BaseCleanupAppcompatCacheSupport(PVOID pUnknown)
{
    STUB;
    return STATUS_NOT_IMPLEMENTED;
}

BOOL
WINAPI
BaseInitAppcompatCache(VOID)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
BaseInitAppcompatCacheSupport(VOID)
{
    STUB;
    return FALSE;
}

PVOID
WINAPI
GetComPlusPackageInstallStatus(VOID)
{
    STUB;
    return NULL;
}

BOOL
WINAPI
SetComPlusPackageInstallStatus(LPVOID lpInfo)
{ 
   STUB;
   return FALSE;
}
 
 /*
 * @unimplemented
 */
VOID
WINAPI
SetTermsrvAppInstallMode(IN BOOL bInstallMode)
{
    STUB;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
TermsrvAppInstallMode(VOID)
{
    STUB;
    return FALSE;
}

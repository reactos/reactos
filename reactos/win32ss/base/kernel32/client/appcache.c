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

/* GLOBALS ********************************************************************/

ULONG g_ShimsEnabled;
 
/* FUNCTIONS ******************************************************************/

BOOLEAN
WINAPI
IsShimInfrastructureDisabled(VOID)
{
    HANDLE KeyHandle;
    NTSTATUS Status;
    KEY_VALUE_PARTIAL_INFORMATION KeyInfo;
    ULONG ResultLength;
    UNICODE_STRING OptionKey = RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\SafeBoot\\Option");
    UNICODE_STRING AppCompatKey = RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility");
    UNICODE_STRING PolicyKey = RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\Software\\Policies\\Microsoft\\Windows\\AppCompat");
    UNICODE_STRING OptionValue = RTL_CONSTANT_STRING(L"OptionValue");
    UNICODE_STRING DisableAppCompat = RTL_CONSTANT_STRING(L"DisableAppCompat");
    UNICODE_STRING DisableEngine = RTL_CONSTANT_STRING(L"DisableEngine");
    OBJECT_ATTRIBUTES OptionKeyAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&OptionKey, OBJ_CASE_INSENSITIVE);
    OBJECT_ATTRIBUTES AppCompatKeyAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&AppCompatKey, OBJ_CASE_INSENSITIVE);
    OBJECT_ATTRIBUTES PolicyKeyAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&PolicyKey, OBJ_CASE_INSENSITIVE);

    /*
     * This is a TROOLEAN, -1 means we haven't yet figured it out.
     * 0 means shims are enabled, and 1 means shims are disabled!
     */
    if (g_ShimsEnabled == -1)
    {
        /* Open the safe mode key */
        Status = NtOpenKey(&KeyHandle, 1, &OptionKeyAttributes);
        if (NT_SUCCESS(Status))
        {
            /* Check if this is safemode */
            Status = NtQueryValueKey(KeyHandle,
                                     &OptionValue,
                                     KeyValuePartialInformation,
                                     &KeyInfo,
                                     sizeof(KeyInfo),
                                     &ResultLength);
            NtClose(KeyHandle);
            if ((NT_SUCCESS(Status)) &&
                 (KeyInfo.Type == REG_DWORD) &&
                 (KeyInfo.DataLength == sizeof(ULONG)) &&
                 (KeyInfo.Data[0] == TRUE))
            {
                /* It is, so disable shims! */
                g_ShimsEnabled = TRUE;
            }
            else
            {
                /* Open the app compatibility engine settings key */
                Status = NtOpenKey(&KeyHandle, 1, &AppCompatKeyAttributes);
                if (NT_SUCCESS(Status))
                {
                    /* Check if the app compat engine is turned off */
                    Status = NtQueryValueKey(KeyHandle,
                                             &DisableAppCompat,
                                             KeyValuePartialInformation,
                                             &KeyInfo,
                                             sizeof(KeyInfo),
                                             &ResultLength);
                    NtClose(KeyHandle);
                    if ((NT_SUCCESS(Status)) &&
                        (KeyInfo.Type == REG_DWORD) &&
                        (KeyInfo.DataLength == sizeof(ULONG)) &&
                        (KeyInfo.Data[0] == TRUE))
                    {
                        /* It is, so disable shims! */
                        g_ShimsEnabled = TRUE;
                    }
                    else
                    {
                        /* Finally, open the app compatibility policy key */
                        Status = NtOpenKey(&KeyHandle, 1, &PolicyKeyAttributes);
                        if (NT_SUCCESS(Status))
                        {
                            /* Check if the system policy disables app compat */
                            Status = NtQueryValueKey(KeyHandle,
                                                     &DisableEngine,
                                                     KeyValuePartialInformation,
                                                     &KeyInfo,
                                                     sizeof(KeyInfo),
                                                     &ResultLength),
                                                     NtClose(KeyHandle);
                            if ((NT_SUCCESS(Status)) &&
                                (KeyInfo.Type == REG_DWORD) &&
                                (KeyInfo.DataLength == sizeof(ULONG)) &&
                                (KeyInfo.Data[0] == TRUE))
                            {
                                /* It does, so disable shims! */
                                g_ShimsEnabled = TRUE;
                            }
                            else
                            {
                                /* No keys are set, so enable shims! */
                                g_ShimsEnabled = FALSE;
                            }
                        }
                    }
                }
            }
        }
    }

    /* Return if shims are disabled or not ("Enabled == 1" means disabled!) */
    return g_ShimsEnabled ? TRUE : FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
BaseCheckAppcompatCache(IN PWCHAR ApplicationName,
                        IN HANDLE FileHandle,
                        IN PWCHAR Environment,
                        OUT PULONG Reason)
{
    UNIMPLEMENTED;
    if (Reason) *Reason = 0;
    return TRUE;
}

/*
 * @implemented
 */
NTSTATUS
WINAPI
BasepCheckBadapp(IN HANDLE FileHandle,
                 IN PWCHAR ApplicationName,
                 IN PWCHAR Environment,
                 IN USHORT ExeType,
                 IN PVOID* SdbQueryAppCompatData,
                 IN PULONG SdbQueryAppCompatDataSize,
                 IN PVOID* SxsData,
                 IN PULONG SxsDataSize,
                 OUT PULONG FusionFlags)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Reason = 0;

    /* Is shimming enabled by group policy? */
    if (IsShimInfrastructureDisabled())
    {
        /* Nothing to worry about */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* It is, check if we know about this app */
        if (!BaseCheckAppcompatCache(ApplicationName,
                                     FileHandle,
                                     Environment,
                                     &Reason))
        {
            /* We don't support this yet */
            UNIMPLEMENTED;
            Status = STATUS_ACCESS_DENIED;
        }
    }

    /* Return caller the status */
    return Status;
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
 * @implemented
 */
VOID
WINAPI
BasepFreeAppCompatData(IN PVOID AppCompatData,
                       IN PVOID AppCompatSxsData)
{
    /* Free the input pointers if present */
    if (AppCompatData) RtlFreeHeap(RtlGetProcessHeap(), 0, AppCompatData);
    if (AppCompatSxsData) RtlFreeHeap(RtlGetProcessHeap(), 0, AppCompatSxsData);
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

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
BaseCleanupAppcompatCache(VOID)
{
    STUB;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
BaseCleanupAppcompatCacheSupport(PVOID pUnknown)
{
    STUB;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
BaseInitAppcompatCache(VOID)
{
    STUB;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
BaseInitAppcompatCacheSupport(VOID)
{
    STUB;
    return FALSE;
}

/*
 * @unimplemented
 */
PVOID
WINAPI
GetComPlusPackageInstallStatus(VOID)
{
    STUB;
    return NULL;
}

/*
 * @unimplemented
 */
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

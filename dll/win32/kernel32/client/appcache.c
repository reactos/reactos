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

ULONG g_ShimsDisabled = -1;
static BOOL g_ApphelpInitialized = FALSE;
static PVOID g_pApphelpCheckRunAppEx;
static PVOID g_pSdbPackAppCompatData;

typedef BOOL (WINAPI *tApphelpCheckRunAppEx)(HANDLE FileHandle, PVOID Unk1, PVOID Unk2, PWCHAR ApplicationName, PVOID Environment, USHORT ExeType, PULONG Reason,
                                             PVOID* SdbQueryAppCompatData, PULONG SdbQueryAppCompatDataSize, PVOID* SxsData, PULONG SxsDataSize,
                                             PULONG FusionFlags, PULONG64 SomeFlag1, PULONG SomeFlag2);
typedef BOOL (WINAPI *tSdbPackAppCompatData)(PVOID hsdb, PVOID pQueryResult, PVOID* ppData, DWORD *dwSize);

#define APPHELP_VALID_RESULT        0x10000
#define APPHELP_RESULT_NOTFOUND     0x20000
#define APPHELP_RESULT_FOUND        0x40000


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
    if (g_ShimsDisabled == -1)
    {
        ULONG DisableShims = FALSE;

        /* Open the safe mode key */
        Status = NtOpenKey(&KeyHandle, KEY_QUERY_VALUE, &OptionKeyAttributes);
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
                 (KeyInfo.Data[0] != FALSE))
            {
                /* It is, so disable shims! */
                DisableShims = TRUE;
            }
        }

        if (!DisableShims)
        {
            /* Open the app compatibility engine settings key */
            Status = NtOpenKey(&KeyHandle, KEY_QUERY_VALUE, &AppCompatKeyAttributes);
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
                    DisableShims = TRUE;
                }
            }
        }
        if (!DisableShims)
        {
            /* Finally, open the app compatibility policy key */
            Status = NtOpenKey(&KeyHandle, KEY_QUERY_VALUE, &PolicyKeyAttributes);
            if (NT_SUCCESS(Status))
            {
                /* Check if the system policy disables app compat */
                Status = NtQueryValueKey(KeyHandle,
                                         &DisableEngine,
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
                    /* It does, so disable shims! */
                    DisableShims = TRUE;
                }
            }
        }
        g_ShimsDisabled = DisableShims;
    }

    /* Return if shims are disabled or not ("Enabled == 1" means disabled!) */
    return g_ShimsDisabled ? TRUE : FALSE;
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
    DPRINT("BaseCheckAppcompatCache is UNIMPLEMENTED\n");

    if (Reason) *Reason = 0;

    // We don't know this app.
    return FALSE;
}

static
VOID
BaseInitApphelp(VOID)
{
    WCHAR Buffer[MAX_PATH*2];
    UNICODE_STRING DllPath = {0};
    PVOID ApphelpAddress;
    PVOID pApphelpCheckRunAppEx = NULL, pSdbPackAppCompatData = NULL;

    RtlInitEmptyUnicodeString(&DllPath, Buffer, sizeof(Buffer));
    RtlCopyUnicodeString(&DllPath, &BaseWindowsDirectory);
    RtlAppendUnicodeToString(&DllPath, L"\\system32\\apphelp.dll");

    if (NT_SUCCESS(LdrLoadDll(NULL, NULL, &DllPath, &ApphelpAddress)))
    {
        ANSI_STRING ProcName;

        RtlInitAnsiString(&ProcName, "ApphelpCheckRunAppEx");
        if (!NT_SUCCESS(LdrGetProcedureAddress(ApphelpAddress, &ProcName, 0, &pApphelpCheckRunAppEx)))
            pApphelpCheckRunAppEx = NULL;

        RtlInitAnsiString(&ProcName, "SdbPackAppCompatData");
        if (!NT_SUCCESS(LdrGetProcedureAddress(ApphelpAddress, &ProcName, 0, &pSdbPackAppCompatData)))
            pSdbPackAppCompatData = NULL;
    }

    if (InterlockedCompareExchangePointer(&g_pApphelpCheckRunAppEx, RtlEncodeSystemPointer(pApphelpCheckRunAppEx), NULL) == NULL)
    {
        g_pSdbPackAppCompatData = RtlEncodeSystemPointer(pSdbPackAppCompatData);
    }
}

/*
 *
 */
BOOL
WINAPI
BaseCheckRunApp(IN HANDLE FileHandle,
                 IN PWCHAR ApplicationName,
                 IN PWCHAR Environment,
                 IN USHORT ExeType,
                 IN PULONG pReason,
                 IN PVOID* SdbQueryAppCompatData,
                 IN PULONG SdbQueryAppCompatDataSize,
                 IN PVOID* SxsData,
                 IN PULONG SxsDataSize,
                 OUT PULONG FusionFlags)
{
    ULONG Reason = 0;
    ULONG64 Flags1 = 0;
    ULONG Flags2 = 0;
    BOOL Continue, NeedCleanup = FALSE;
    tApphelpCheckRunAppEx pApphelpCheckRunAppEx;
    tSdbPackAppCompatData pSdbPackAppCompatData;
    PVOID QueryResult = NULL;
    ULONG QueryResultSize = 0;

    if (!g_ApphelpInitialized)
    {
        BaseInitApphelp();
        g_ApphelpInitialized = TRUE;
    }

    pApphelpCheckRunAppEx = RtlDecodeSystemPointer(g_pApphelpCheckRunAppEx);
    pSdbPackAppCompatData = RtlDecodeSystemPointer(g_pSdbPackAppCompatData);

    if (!pApphelpCheckRunAppEx || !pSdbPackAppCompatData)
        return TRUE;

    if (pReason)
        Reason = *pReason;

    Continue = pApphelpCheckRunAppEx(FileHandle, NULL, NULL, ApplicationName, Environment, ExeType, &Reason,
        &QueryResult, &QueryResultSize, SxsData, SxsDataSize, FusionFlags, &Flags1, &Flags2);

    if (pReason)
        *pReason = Reason;

    if (Continue)
    {
        if ((Reason & (APPHELP_VALID_RESULT|APPHELP_RESULT_FOUND)) == (APPHELP_VALID_RESULT|APPHELP_RESULT_FOUND))
        {
            if (!pSdbPackAppCompatData(NULL, QueryResult, SdbQueryAppCompatData, SdbQueryAppCompatDataSize))
            {
                DPRINT1("SdbPackAppCompatData returned a failure!\n");
                NeedCleanup = TRUE;
            }
        }
        else
        {
            NeedCleanup = TRUE;
        }
    }

    if (QueryResult)
        RtlFreeHeap(RtlGetProcessHeap(), 0, QueryResult);

    if (NeedCleanup)
    {
        BasepFreeAppCompatData(*SdbQueryAppCompatData, *SxsData);
        *SdbQueryAppCompatData = NULL;
        if (SdbQueryAppCompatDataSize)
            *SdbQueryAppCompatDataSize = 0;
        *SxsData = NULL;
        if (SxsDataSize)
            *SxsDataSize = 0;
    }

    return Continue;
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
            if (!BaseCheckRunApp(FileHandle, ApplicationName, Environment, ExeType, &Reason,
                                SdbQueryAppCompatData, SdbQueryAppCompatDataSize, SxsData, SxsDataSize, FusionFlags))
            {
                Status = STATUS_ACCESS_DENIED;
            }
        }
    }

    /* Return caller the status */
    return Status;
}

/*
 * @implemented
 */
BOOL
WINAPI
BaseDumpAppcompatCache(VOID)
{
    NTSTATUS Status;

    Status = NtApphelpCacheControl(ApphelpCacheServiceDump, NULL);
    return NT_SUCCESS(Status);
}

/*
 * @implemented
 */
BOOL
WINAPI
BaseFlushAppcompatCache(VOID)
{
    NTSTATUS Status;

    Status = NtApphelpCacheControl(ApphelpCacheServiceFlush, NULL);
    return NT_SUCCESS(Status);
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

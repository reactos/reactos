/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     apphelp entrypoint / generic interface functions
 * COPYRIGHT:   Copyright 2011 André Hentschel
 *              Copyright 2013 Mislav Blaževic
 *              Copyright 2015-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "strsafe.h"
#include "apphelp.h"
#include <ndk/rtlfuncs.h>
#include <ndk/cmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/kdtypes.h>


ACCESS_MASK Wow64QueryFlag(void);

const UNICODE_STRING InstalledSDBKeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\InstalledSDB");

/* from dpfilter.h */
#define DPFLTR_APPCOMPAT_ID 123

#define MAX_GUID_STRING_LEN     sizeof("{12345678-1234-1234-0123-456789abcdef}")

#ifndef NT_SUCCESS
#define NT_SUCCESS(StatCode)  ((NTSTATUS)(StatCode) >= 0)
#endif

ULONG g_ShimDebugLevel = ~0;
HMODULE g_hInstance;

void ApphelppInitDebugLevel(void)
{
    static const UNICODE_STRING DebugKey = RTL_CONSTANT_STRING(L"SHIM_DEBUG_LEVEL");
    UNICODE_STRING DebugValue;
    NTSTATUS Status;
    ULONG NewLevel = SHIM_ERR;
    WCHAR Buffer[40];

    RtlInitEmptyUnicodeString(&DebugValue, Buffer, sizeof(Buffer));

    Status = RtlQueryEnvironmentVariable_U(NULL, &DebugKey, &DebugValue);

    if (NT_SUCCESS(Status))
    {
        if (!NT_SUCCESS(RtlUnicodeStringToInteger(&DebugValue, 10, &NewLevel)))
            NewLevel = SHIM_ERR;
    }
    g_ShimDebugLevel = NewLevel;
}


BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hinst;
        DisableThreadLibraryCalls(hinst);
        SdbpHeapInit();
        break;
    case DLL_PROCESS_DETACH:
        SdbpHeapDeinit();
        break;
    }
    return TRUE;
}

BOOL WINAPI ApphelpCheckInstallShieldPackage(void* ptr, LPCWSTR path)
{
    SHIM_WARN("stub: ptr=%p, path='%S'\n", ptr, path);
    return TRUE;
}


BOOL WINAPI ApphelpCheckShellObject(REFCLSID ObjectCLSID, BOOL bShimIfNecessary, ULONGLONG *pullFlags)
{
    WCHAR GuidString[MAX_GUID_STRING_LEN];
    if (!ObjectCLSID || !SdbGUIDToString(ObjectCLSID, GuidString, RTL_NUMBER_OF(GuidString)))
        GuidString[0] = L'\0';
    SHIM_WARN("stub: ObjectCLSID='%S', bShimIfNecessary=%d, pullFlags=%p)\n", GuidString, bShimIfNecessary, pullFlags);

    if (pullFlags)
        *pullFlags = 0;

    return TRUE;
}

/**
 * Outputs diagnostic info.
 *
 * @param [in]  Level           The level to log this message with, choose any of [SHIM_ERR,
 *                              SHIM_WARN, SHIM_INFO].
 * @param [in]  FunctionName    The function this log should be attributed to.
 * @param [in]  Format          The format string.
 * @param   ...                 Variable arguments providing additional information.
 *
 * @return  Success: TRUE Failure: FALSE.
 */
BOOL WINAPIV ShimDbgPrint(SHIM_LOG_LEVEL Level, PCSTR FunctionName, PCSTR Format, ...)
{
    char Buffer[512];
    va_list ArgList;
    char* Current = Buffer;
    const char* LevelStr;
    size_t Length = sizeof(Buffer);

    if (g_ShimDebugLevel == ~0)
        ApphelppInitDebugLevel();

    if (Level > g_ShimDebugLevel)
        return FALSE;

    switch (Level)
    {
    case SHIM_ERR:
        LevelStr = "Err ";
        Level = DPFLTR_MASK | (1 << DPFLTR_ERROR_LEVEL);
        break;
    case SHIM_WARN:
        LevelStr = "Warn";
        Level = DPFLTR_MASK | (1 << DPFLTR_WARNING_LEVEL);
        break;
    case SHIM_INFO:
        LevelStr = "Info";
        Level = DPFLTR_MASK | (1 << DPFLTR_INFO_LEVEL);
        break;
    default:
        LevelStr = "User";
        Level = DPFLTR_MASK | (1 << DPFLTR_INFO_LEVEL);
        break;
    }
    StringCchPrintfExA(Current, Length, &Current, &Length, STRSAFE_NULL_ON_FAILURE, "[%s][%-20s] ", LevelStr, FunctionName);

    va_start(ArgList, Format);
    StringCchVPrintfExA(Current, Length, &Current, &Length, STRSAFE_NULL_ON_FAILURE, Format, ArgList);
    va_end(ArgList);

#if defined(APPCOMPAT_USE_DBGPRINTEX) && APPCOMPAT_USE_DBGPRINTEX
    return NT_SUCCESS(DbgPrintEx(DPFLTR_APPCOMPAT_ID, Level, "%s", Buffer));
#else
    DbgPrint("%s", Buffer);
    return TRUE;
#endif
}


#define APPHELP_DONTWRITE_REASON    2
#define APPHELP_CLEARBITS           0x100   /* TODO: Investigate */
#define APPHELP_IGNORE_ENVIRONMENT  0x400

#define APPHELP_VALID_RESULT        0x10000
#define APPHELP_RESULT_NOTFOUND     0x20000
#define APPHELP_RESULT_FOUND        0x40000

/**
 * Lookup Shims / Fixes for the specified application
 *
 * @param [in]  FileHandle                  Handle to the file to check.
 * @param [in]  Unk1
 * @param [in]  Unk2
 * @param [in]  ApplicationName             Exe to check
 * @param [in]  Environment                 The environment variables to use, or NULL to use the current environment.
 * @param [in]  ExeType                     Exe type (MACHINE_TYPE_XXXX)
 * @param [in,out]  Reason                  Input/output flags
 * @param [in]  SdbQueryAppCompatData       The resulting data.
 * @param [in]  SdbQueryAppCompatDataSize   The resulting data size.
 * @param [in]  SxsData                     TODO
 * @param [in]  SxsDataSize                 TODO
 * @param [in]  FusionFlags                 TODO
 * @param [in]  SomeFlag1                   TODO
 * @param [in]  SomeFlag2                   TODO
 *
 * @return  TRUE if the application is allowed to run.
 */
BOOL
WINAPI
ApphelpCheckRunAppEx(
    _In_ HANDLE FileHandle,
    _In_opt_ PVOID Unk1,
    _In_opt_ PVOID Unk2,
    _In_opt_z_ PWCHAR ApplicationName,
    _In_opt_ PVOID Environment,
    _In_opt_ USHORT ExeType,
    _Inout_opt_ PULONG Reason,
    _Out_opt_ PVOID* SdbQueryAppCompatData,
    _Out_opt_ PULONG SdbQueryAppCompatDataSize,
    _Out_opt_ PVOID* SxsData,
    _Out_opt_ PULONG SxsDataSize,
    _Out_opt_ PULONG FusionFlags,
    _Out_opt_ PULONG64 SomeFlag1,
    _Out_opt_ PULONG SomeFlag2)
{
    SDBQUERYRESULT* result = NULL;
    HSDB hsdb = NULL;
    DWORD dwFlags = 0;

    if (SxsData)
        *SxsData = NULL;
    if (SxsDataSize)
        *SxsDataSize = 0;
    if (FusionFlags)
        *FusionFlags = 0;
    if (SomeFlag1)
        *SomeFlag1 = 0;
    if (SomeFlag2)
        *SomeFlag2 = 0;
    if (Reason)
        dwFlags = *Reason;

    dwFlags &= ~APPHELP_CLEARBITS;

    *SdbQueryAppCompatData = result = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SDBQUERYRESULT));
    if (SdbQueryAppCompatDataSize)
        *SdbQueryAppCompatDataSize = sizeof(*result);


    hsdb = SdbInitDatabase(HID_DOS_PATHS | SDB_DATABASE_MAIN_SHIM, NULL);
    if (hsdb)
    {
        BOOL FoundMatch;
        DWORD MatchingExeFlags = 0;

        if (dwFlags & APPHELP_IGNORE_ENVIRONMENT)
            MatchingExeFlags |= SDBGMEF_IGNORE_ENVIRONMENT;

        FoundMatch = SdbGetMatchingExe(hsdb, ApplicationName, NULL, Environment, MatchingExeFlags, result);
        if (FileHandle != INVALID_HANDLE_VALUE)
        {
            dwFlags |= APPHELP_VALID_RESULT;
            dwFlags |= (FoundMatch ? APPHELP_RESULT_FOUND : APPHELP_RESULT_NOTFOUND);
        }

        SdbReleaseDatabase(hsdb);
    }

    if (Reason && !(dwFlags & APPHELP_DONTWRITE_REASON))
        *Reason = dwFlags;


    /* We should _ALWAYS_ return TRUE here, unless we want to block an application from starting! */
    return TRUE;
}


/**
 * @name SdbRegisterDatabaseEx
 * Register an application compatibility database
 *
 * @param pszDatabasePath   The database. Required
 * @param dwDatabaseType    The database type. SDB_DATABASE_*
 * @param pTimeStamp        The timestamp. When this argument is not provided, the system time is used.
 * @return                  TRUE on success, or FALSE on failure.
 */
BOOL WINAPI SdbRegisterDatabaseEx(
    _In_ LPCWSTR pszDatabasePath,
    _In_ DWORD dwDatabaseType,
    _In_opt_ const PULONGLONG pTimeStamp)
{
    PDB pdb;
    DB_INFORMATION Information;
    WCHAR GuidBuffer[MAX_GUID_STRING_LEN];
    UNICODE_STRING KeyName;
    ACCESS_MASK KeyAccess;
    OBJECT_ATTRIBUTES ObjectKey = RTL_INIT_OBJECT_ATTRIBUTES(&KeyName, OBJ_CASE_INSENSITIVE);
    NTSTATUS Status;
    HANDLE InstalledSDBKey;

    pdb = SdbOpenDatabase(pszDatabasePath, DOS_PATH);
    if (!pdb)
    {
        SHIM_ERR("Unable to open DB %S\n", pszDatabasePath);
        return FALSE;
    }

    if (!SdbGetDatabaseInformation(pdb, &Information) ||
        !(Information.dwFlags & DB_INFO_FLAGS_VALID_GUID))
    {
        SHIM_ERR("Unable to retrieve DB info\n");
        SdbCloseDatabase(pdb);
        return FALSE;
    }

    if (!SdbGUIDToString(&Information.Id, GuidBuffer, RTL_NUMBER_OF(GuidBuffer)))
    {
        SHIM_ERR("Unable to Convert GUID to string\n");
        SdbFreeDatabaseInformation(&Information);
        SdbCloseDatabase(pdb);
        return FALSE;
    }

    KeyName = InstalledSDBKeyName;
    KeyAccess = Wow64QueryFlag() | KEY_WRITE | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS;
    Status = NtCreateKey(&InstalledSDBKey, KeyAccess, &ObjectKey, 0, NULL, 0, NULL);
    if (NT_SUCCESS(Status))
    {
        HANDLE DbKey;

        RtlInitUnicodeString(&KeyName, GuidBuffer);
        ObjectKey.RootDirectory = InstalledSDBKey;

        Status = NtCreateKey(&DbKey, KeyAccess, &ObjectKey, 0, NULL, 0, NULL);
        if (NT_SUCCESS(Status))
        {
            UNICODE_STRING DatabasePathKey = RTL_CONSTANT_STRING(L"DatabasePath");
            UNICODE_STRING DatabaseInstallTimeStampKey = RTL_CONSTANT_STRING(L"DatabaseInstallTimeStamp");
            UNICODE_STRING DatabaseTypeKey = RTL_CONSTANT_STRING(L"DatabaseType");
            UNICODE_STRING DatabaseDescriptionKey = RTL_CONSTANT_STRING(L"DatabaseDescription");

            Status = NtSetValueKey(DbKey, &DatabasePathKey, 0, REG_SZ,
                                   (PVOID)pszDatabasePath, ((ULONG)wcslen(pszDatabasePath) + 1) * sizeof(WCHAR));
            if (!NT_SUCCESS(Status))
                SHIM_ERR("Unable to write %wZ\n", &DatabasePathKey);

            if (NT_SUCCESS(Status))
            {
                ULARGE_INTEGER ulTimeStamp;
                if (pTimeStamp)
                {
                    ulTimeStamp.QuadPart = *pTimeStamp;
                }
                else
                {
                    FILETIME fi;
                    GetSystemTimeAsFileTime(&fi);
                    ulTimeStamp.LowPart = fi.dwLowDateTime;
                    ulTimeStamp.HighPart = fi.dwHighDateTime;
                }
                Status = NtSetValueKey(DbKey, &DatabaseInstallTimeStampKey, 0, REG_QWORD,
                                       &ulTimeStamp.QuadPart, sizeof(ulTimeStamp));
                if (!NT_SUCCESS(Status))
                    SHIM_ERR("Unable to write %wZ\n", &DatabaseInstallTimeStampKey);
            }

            if (NT_SUCCESS(Status))
            {
                Status = NtSetValueKey(DbKey, &DatabaseTypeKey, 0, REG_DWORD,
                                       &dwDatabaseType, sizeof(dwDatabaseType));
                if (!NT_SUCCESS(Status))
                    SHIM_ERR("Unable to write %wZ\n", &DatabaseTypeKey);
            }

            if (NT_SUCCESS(Status) && Information.Description)
            {
                Status = NtSetValueKey(DbKey, &DatabaseDescriptionKey, 0, REG_SZ,
                                       (PVOID)Information.Description, ((ULONG)wcslen(Information.Description) + 1) * sizeof(WCHAR));
                if (!NT_SUCCESS(Status))
                    SHIM_ERR("Unable to write %wZ\n", &DatabaseDescriptionKey);
            }

            NtClose(DbKey);

            if (NT_SUCCESS(Status))
            {
                SHIM_INFO("Installed %wS as %wZ\n", pszDatabasePath, &KeyName);
            }
        }
        else
        {
            SHIM_ERR("Unable to create key %wZ\n", &KeyName);
        }

        NtClose(InstalledSDBKey);
    }
    else
    {
        SHIM_ERR("Unable to create key %wZ\n", &KeyName);
    }

    SdbFreeDatabaseInformation(&Information);
    SdbCloseDatabase(pdb);

    return NT_SUCCESS(Status);
}


/**
 * @name SdbRegisterDatabase
 * Register an application compatibility database
 *
 * @param pszDatabasePath   The database. Required
 * @param dwDatabaseType    The database type. SDB_DATABASE_*
 * @return                  TRUE on success, or FALSE on failure.
 */
BOOL WINAPI SdbRegisterDatabase(
    _In_ LPCWSTR pszDatabasePath,
    _In_ DWORD dwDatabaseType)
{
    return SdbRegisterDatabaseEx(pszDatabasePath, dwDatabaseType, NULL);
}


/**
 * @name SdbUnregisterDatabase
 *
 *
 * @param pguidDB
 * @return
 */
BOOL WINAPI SdbUnregisterDatabase(_In_ const GUID *pguidDB)
{
    WCHAR KeyBuffer[MAX_PATH], GuidBuffer[50];
    UNICODE_STRING KeyName;
    ACCESS_MASK KeyAccess;
    OBJECT_ATTRIBUTES ObjectKey = RTL_INIT_OBJECT_ATTRIBUTES(&KeyName, OBJ_CASE_INSENSITIVE);
    NTSTATUS Status;
    HANDLE DbKey;

    if (!SdbGUIDToString(pguidDB, GuidBuffer, RTL_NUMBER_OF(GuidBuffer)))
    {
        SHIM_ERR("Unable to Convert GUID to string\n");
        return FALSE;
    }

    RtlInitEmptyUnicodeString(&KeyName, KeyBuffer, sizeof(KeyBuffer));
    RtlAppendUnicodeStringToString(&KeyName, &InstalledSDBKeyName);
    RtlAppendUnicodeToString(&KeyName, L"\\");
    RtlAppendUnicodeToString(&KeyName, GuidBuffer);

    KeyAccess = Wow64QueryFlag() | DELETE;
    Status = NtCreateKey(&DbKey, KeyAccess, &ObjectKey, 0, NULL, 0, NULL);
    if (!NT_SUCCESS(Status))
    {
        SHIM_ERR("Unable to open %wZ\n", &KeyName);
        return FALSE;
    }

    Status = NtDeleteKey(DbKey);
    if (!NT_SUCCESS(Status))
        SHIM_ERR("Unable to delete %wZ\n", &KeyName);

    NtClose(DbKey);
    return NT_SUCCESS(Status);
}


/* kernel32.dll */
BOOL WINAPI BaseDumpAppcompatCache(VOID);
BOOL WINAPI BaseFlushAppcompatCache(VOID);


/**
 * @name ShimDumpCache
 * Dump contents of the shim cache.
 *
 * @param hwnd          Unused, pass 0
 * @param hInstance     Unused, pass 0
 * @param lpszCmdLine   Unused, pass 0
 * @param nCmdShow      Unused, pass 0
 * @return
 */
BOOL WINAPI ShimDumpCache(HWND hwnd, HINSTANCE hInstance, LPCSTR lpszCmdLine, int nCmdShow)
{
    return BaseDumpAppcompatCache();
}

/**
* @name ShimFlushCache
* Flush the shim cache. Call this after installing a new shim database
*
* @param hwnd          Unused, pass 0
* @param hInstance     Unused, pass 0
* @param lpszCmdLine   Unused, pass 0
* @param nCmdShow      Unused, pass 0
* @return
*/
BOOL WINAPI ShimFlushCache(HWND hwnd, HINSTANCE hInstance, LPCSTR lpszCmdLine, int nCmdShow)
{
    return BaseFlushAppcompatCache();
}

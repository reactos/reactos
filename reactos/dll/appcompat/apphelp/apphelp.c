/*
 * Copyright 2011 André Hentschel
 * Copyright 2013 Mislav Blažević
 * Copyright 2015-2017 Mark Jansen (mark.jansen@reactos.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winver.h"
#include "strsafe.h"
#include "apphelp.h"
#include "ndk/rtlfuncs.h"
#include "ndk/kdtypes.h"


/* from dpfilter.h */
#define DPFLTR_APPCOMPAT_ID 123

#ifndef NT_SUCCESS
#define NT_SUCCESS(StatCode)  ((NTSTATUS)(StatCode) >= 0)
#endif

ULONG g_ShimDebugLevel = 0xffffffff;
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
            NewLevel = 0;
    }
    g_ShimDebugLevel = NewLevel;
}


BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
#ifndef __REACTOS__
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
#endif
        case DLL_PROCESS_ATTACH:
            g_hInstance = hinst;
            DisableThreadLibraryCalls( hinst );
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
    SHIM_WARN("stub: ptr=%p, path='%S'\r\n", ptr, path);
    return TRUE;
}


BOOL WINAPI ApphelpCheckShellObject(REFCLSID ObjectCLSID, BOOL bShimIfNecessary, ULONGLONG *pullFlags)
{
    WCHAR GuidString[100];
    if (!ObjectCLSID || !SdbGUIDToString(ObjectCLSID, GuidString, 100))
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

    if (g_ShimDebugLevel == 0xffffffff)
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



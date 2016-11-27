/*
 * Copyright 2011 André Hentschel
 * Copyright 2013 Mislav Blažević
 * Copyright 2015 Mark Jansen
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

#include "wine/winternl.h"

/* from dpfilter.h */
#define DPFLTR_APPCOMPAT_ID 123

#ifndef NT_SUCCESS
#define NT_SUCCESS(StatCode)  ((NTSTATUS)(StatCode) >= 0)
#endif

ULONG g_ShimDebugLevel = 0xffffffff;

void ApphelppInitDebugLevel(void)
{
    UNICODE_STRING DebugKey, DebugValue;
    NTSTATUS Status;
    ULONG NewLevel = SHIM_ERR;
    WCHAR Buffer[40];

    RtlInitUnicodeString(&DebugKey, L"SHIM_DEBUG_LEVEL");
    DebugValue.MaximumLength = sizeof(Buffer);
    DebugValue.Buffer = Buffer;
    DebugValue.Length = 0;

    /* Hold the lock as short as possible. */
    RtlAcquirePebLock();
    Status = RtlQueryEnvironmentVariable_U(NULL, &DebugKey, &DebugValue);
    RtlReleasePebLock();

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
    OutputDebugStringA(Buffer);
    return TRUE;
#endif
}


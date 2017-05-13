/*
 * Copyright 2015-2017 Mark Jansen
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
#include "windows.h"
#include "ntndk.h"
#include "shimlib.h"
#include <strsafe.h>

HANDLE g_pShimEngModHandle = 0;


ULONG g_ShimEngDebugLevel = 0xffffffff;




VOID SeiInitDebugSupport(VOID)
{
    static const UNICODE_STRING DebugKey = RTL_CONSTANT_STRING(L"SHIMENG_DEBUG_LEVEL");
    UNICODE_STRING DebugValue;
    NTSTATUS Status;
    ULONG NewLevel = 0;
    WCHAR Buffer[40];

    RtlInitEmptyUnicodeString(&DebugValue, Buffer, sizeof(Buffer));

    Status = RtlQueryEnvironmentVariable_U(NULL, &DebugKey, &DebugValue);

    if (NT_SUCCESS(Status))
    {
        if (!NT_SUCCESS(RtlUnicodeStringToInteger(&DebugValue, 10, &NewLevel)))
            NewLevel = 0;
    }
    g_ShimEngDebugLevel = NewLevel;
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
BOOL WINAPIV SeiDbgPrint(SEI_LOG_LEVEL Level, PCSTR Function, PCSTR Format, ...)
{
    char Buffer[512];
    char* Current = Buffer;
    const char* LevelStr;
    size_t Length = sizeof(Buffer);
    va_list ArgList;
    HRESULT hr;

    if (g_ShimEngDebugLevel == 0xffffffff)
        SeiInitDebugSupport();

    if (Level > g_ShimEngDebugLevel)
        return FALSE;

    switch (Level)
    {
    case SEI_MSG:
        LevelStr = "MSG ";
        break;
    case SEI_FAIL:
        LevelStr = "FAIL";
        break;
    case SEI_WARN:
        LevelStr = "WARN";
        break;
    case SEI_INFO:
        LevelStr = "INFO";
        break;
    default:
        LevelStr = "USER";
        break;
    }

    if (Function)
        hr = StringCchPrintfExA(Current, Length, &Current, &Length, STRSAFE_NULL_ON_FAILURE, "[%s] [%s] ", LevelStr, Function);
    else
        hr = StringCchPrintfExA(Current, Length, &Current, &Length, STRSAFE_NULL_ON_FAILURE, "[%s] ", LevelStr);

    if (!SUCCEEDED(hr))
        return FALSE;

    va_start(ArgList, Format);
    hr = StringCchVPrintfExA(Current, Length, &Current, &Length, STRSAFE_NULL_ON_FAILURE, Format, ArgList);
    va_end(ArgList);
    if (!SUCCEEDED(hr))
        return FALSE;

    DbgPrint("%s", Buffer);
    return TRUE;
}






VOID NotifyShims(DWORD dwReason, PVOID Info)
{
    /* Enumerate all loaded shims */
}


VOID NTAPI SE_InstallBeforeInit(PUNICODE_STRING ProcessImage, PVOID pShimData)
{
    SHIMENG_FAIL("(%wZ, %p)", ProcessImage, pShimData);
    /* Find & Load all shims.. */
}

VOID NTAPI SE_InstallAfterInit(PUNICODE_STRING ProcessImage, PVOID pShimData)
{
    SHIMENG_FAIL("(%wZ, %p)", ProcessImage, pShimData);
    NotifyShims(SHIM_NOTIFY_ATTACH, NULL);
}

VOID NTAPI SE_ProcessDying(VOID)
{
    SHIMENG_FAIL("()");
    NotifyShims(SHIM_NOTIFY_DETACH, NULL);
}

VOID WINAPI SE_DllLoaded(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    SHIMENG_FAIL("(%p)", LdrEntry);
    NotifyShims(SHIM_REASON_DLL_LOAD, LdrEntry);
}

VOID WINAPI SE_DllUnloaded(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    SHIMENG_FAIL("(%p)", LdrEntry);
    NotifyShims(SHIM_REASON_DLL_UNLOAD, LdrEntry);
}

BOOL WINAPI SE_IsShimDll(PVOID BaseAddress)
{
    SHIMENG_FAIL("(%p)", BaseAddress);
    return FALSE;
}


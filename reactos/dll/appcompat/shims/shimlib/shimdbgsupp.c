/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Shim library
 * FILE:            dll/appcompat/shims/shimlib/shimdbgsupp.c
 * PURPOSE:         Shim debug helper functions
 * PROGRAMMER:      Mark Jansen
 */

#include <windef.h>
#include <winbase.h>
#include <shimlib.h>
#include <strsafe.h>

#include "wine/winternl.h"

#define DPFLTR_APPCOMPAT_ID 123

void ApphelppInitDebugSupport(PCWSTR KeyName, PULONG Level);
static void SeiInitDebugSupport()
{
    ApphelppInitDebugSupport(L"SHIMENG_DEBUG_LEVEL", &g_ShimEngDebugLevel);
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
BOOL WINAPIV SeiDbgPrint(SEI_LOG_LEVEL Level, PCSTR FunctionName, PCSTR Format, ...)
{
    char Buffer[512];
    va_list ArgList;
    char* Current = Buffer;
    const char* LevelStr;
    size_t Length = sizeof(Buffer);

    if (g_ShimEngDebugLevel == 0xffffffff)
        SeiInitDebugSupport();

    if (Level > g_ShimEngDebugLevel)
        return FALSE;

    switch (Level)
    {
    case SEI_ERR:
        LevelStr = "Err ";
        Level = DPFLTR_MASK | (1 << DPFLTR_ERROR_LEVEL);
        break;
    case SEI_WARN:
        LevelStr = "Warn";
        Level = DPFLTR_MASK | (1 << DPFLTR_WARNING_LEVEL);
        break;
    case SEI_INFO:
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

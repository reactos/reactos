/*
 * PROJECT:     ReactOS 'General' Shim library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Ignore debug output shim
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <shimlib.h>
#include <strsafe.h>


#define SHIM_NS         IgnoreDebugOutput
#include <setup_shim.inl>

void WINAPI SHIM_OBJ_NAME(OutputDebugStringA)(LPCSTR lpOutputString)
{
    (VOID)lpOutputString;
}

void WINAPI SHIM_OBJ_NAME(OutputDebugStringW)(LPCWSTR lpOutputString)
{
    (VOID)lpOutputString;
}

#define SHIM_NUM_HOOKS  2
#define SHIM_SETUP_HOOKS \
    SHIM_HOOK(0, "KERNEL32.DLL", "OutputDebugStringA", SHIM_OBJ_NAME(OutputDebugStringA)) \
    SHIM_HOOK(1, "KERNEL32.DLL", "OutputDebugStringW", SHIM_OBJ_NAME(OutputDebugStringW))

#include <implement_shim.inl>

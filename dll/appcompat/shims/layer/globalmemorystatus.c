/*
 * PROJECT:     ReactOS 'Layers' Shim library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     GlobalMemoryStatus2GB shim
 * COPYRIGHT:   Copyright 2026 Mark Jansen <mark.jansen@reactos.org>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <shimlib.h>

typedef VOID(NTAPI *GLOBALMEMORYSTATUSPROC)(LPMEMORYSTATUS lpBuffer);

#define SHIM_NS GlobalMemoryStatus2GB
#include <setup_shim.inl>

VOID NTAPI
SHIM_OBJ_NAME(APIHook_GlobalMemoryStatus)(LPMEMORYSTATUS lpBuffer)
{
    CALL_SHIM(0, GLOBALMEMORYSTATUSPROC)(lpBuffer);

    if (lpBuffer->dwTotalPhys > 0x3FFFFFFF)
        lpBuffer->dwTotalPhys = 0x3FFFFFFF;

    if (lpBuffer->dwAvailPhys > 0x3FFFFFFF)
        lpBuffer->dwAvailPhys = 0x3FFFFFFF;

    if (lpBuffer->dwTotalPageFile > 0x7FFFFFFF)
        lpBuffer->dwTotalPageFile = 0x7FFFFFFF;

    if (lpBuffer->dwAvailPageFile > 0x3FFFFFFF)
        lpBuffer->dwAvailPageFile = 0x3FFFFFFF;
}

#define SHIM_NUM_HOOKS 1
#define SHIM_SETUP_HOOKS SHIM_HOOK(0, "KERNEL32.DLL", "GlobalMemoryStatus", SHIM_OBJ_NAME(APIHook_GlobalMemoryStatus))

#include <implement_shim.inl>

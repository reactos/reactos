/*
 * PROJECT:     ReactOS 'General' Shim library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     MsysDecoy
 * COPYRIGHT:   Timo kreuzer (timo.kreuzer@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <rtlfuncs.h>
#include <shimlib.h>

extern PVOID RtlGetCurrentDirectory_U_RtlpMsysDecoy;
// This is not correct, because the symbols should come from ntdll...
PVOID FastPebLock = NULL;
PVOID RtlpCurDirRef = NULL;

#define SHIM_NS         MsysDecoy
#include <setup_shim.inl>


BOOL WINAPI SHIM_OBJ_NAME(Notify)(DWORD fdwReason, PVOID ptr)
{
    if (fdwReason == SHIM_REASON_INIT)
    {
        FastPebLock = NtCurrentPeb()->FastPebLock;
        SHIM_MSG("FastPebLock=%p\n", FastPebLock);
    }
    return TRUE;
}

#define SHIM_NOTIFY_FN SHIM_OBJ_NAME(Notify)
#define SHIM_NUM_HOOKS  1
#define SHIM_SETUP_HOOKS \
    SHIM_HOOK(0, "NTDLL.DLL", "RtlGetCurrentDirectory_U", RtlGetCurrentDirectory_U_RtlpMsysDecoy)

#include <implement_shim.inl>

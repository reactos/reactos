/*
 * PROJECT:     ReactOS 'General' Shim library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Shim to apply the msys2 decoy
 * COPYRIGHT:   Copyright 2025 Timo kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2025 Mark Jansen <mark.jansen@reactos.org>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <rtlfuncs.h>
#include <shimlib.h>
#include <compat_undoc.h>

static PVOID RtlGetCurrentDirectory_U_RtlpMsysDecoy;

#define SHIM_NS         MsysDecoy
#include <setup_shim.inl>


BOOL WINAPI SHIM_OBJ_NAME(Notify)(DWORD fdwReason, PVOID ptr)
{
    if (fdwReason == SHIM_NOTIFY_ATTACH)
    {
        ReactOS_ShimData *pShimData = (ReactOS_ShimData *)NtCurrentPeb()->pShimData;
        if (pShimData && pShimData->dwMagic == REACTOS_SHIMDATA_MAGIC)
        {
            /* Grab the private function from ntdll */
            RtlGetCurrentDirectory_U_RtlpMsysDecoy = pShimData->RtlGetCurrentDirectory_U_RtlpMsysDecoy;
            SHIM_MSG("RtlGetCurrentDirectory_U_RtlpMsysDecoy=%p\n", RtlGetCurrentDirectory_U_RtlpMsysDecoy);
            return RtlGetCurrentDirectory_U_RtlpMsysDecoy != NULL;
        }
        SHIM_FAIL("Invalid pShimData @ %p\n", pShimData);
        /* Returning false here will not register the hooks */
        return FALSE;
    }
    return TRUE;
}

#define SHIM_NOTIFY_FN SHIM_OBJ_NAME(Notify)
#define SHIM_NUM_HOOKS  1
#define SHIM_SETUP_HOOKS \
    SHIM_HOOK(0, "NTDLL.DLL", "RtlGetCurrentDirectory_U", RtlGetCurrentDirectory_U_RtlpMsysDecoy)

#include <implement_shim.inl>

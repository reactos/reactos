/*
 * PROJECT:     ReactOS Networking Debugging Module
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     kdnet initialization
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "kdnet.h"
#include <ndk/haltypes.h>
#include <ndk/halfuncs.h>

KDNET_SHARED_DATA KdNetSharedData = {0};
BOOLEAN KdNetInitialized = FALSE;
PVOID KdNetHardwareContext = NULL;

ULONG (*FrLdrDbgPrint)(const char *Format, ...);

NTSTATUS
NTAPI
KdDebuggerInitialize0(_In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    if (KdNetInitialized)
        return STATUS_SUCCESS;
    if (!LoaderBlock)
        return STATUS_INVALID_PARAMETER;

    FrLdrDbgPrint = (PVOID)LoaderBlock->u.I386.CommonDataArea;
    FrLdrDbgPrint("KdDebuggerInitialize0 called with LoaderBlock %p\n", LoaderBlock);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
KdDebuggerInitialize1(_In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* 
     *  TODO:
     * - Write the kdnet reg key (debug params, hardware info post startup)
     */
    return STATUS_NOT_IMPLEMENTED;
}

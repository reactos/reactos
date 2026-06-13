/*
 * PROJECT:     ReactOS Networking Debugging Module
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Standard power APIs for kdnet
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "kdnet.h"

NTSTATUS
NTAPI
KdD0Transition(VOID)
{

    if (KdNetInitialized &&
        KdNetExtensibilityExports &&
        KdNetExtensibilityExports->KdInitializeController)
    {
        (VOID)KdNetExtensibilityExports->KdInitializeController(&KdNetSharedData);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdD3Transition(VOID)
{
    if (KdNetInitialized && KdNetExtensibilityExports)
    {
        if (KdNetSharedData.LinkState)
            *KdNetSharedData.LinkState = 0;

        if (KdNetExtensibilityExports->KdShutdownController && KdNetHardwareContext)
            KdNetExtensibilityExports->KdShutdownController(KdNetHardwareContext);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdRestore(IN BOOLEAN SleepTransition)
{
    //TODO:
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdSave(IN BOOLEAN SleepTransition)
{
    //TODO:
    return STATUS_SUCCESS;
}

/*
 * PROJECT:     ReactOS Kernel Debugger over Network extension stubs
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Main source file
 * COPYRIGHT:   Copyright 2022 Hervé Poussineau <hpoussin@reactos.org>
 */

#define NOEXTAPI
#include <ntifs.h>
#include <kdnetextensibility.h>

NTSTATUS
NTAPI
KdInitializeLibrary(
    _In_ PKDNET_EXTENSIBILITY_IMPORTS ImportTable,
    _In_opt_ PCHAR LoaderOptions,
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR Device)
{
    return STATUS_NOT_IMPLEMENTED;
}


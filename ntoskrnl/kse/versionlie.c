/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     KSE 'VersionLie' shim implementation
 * COPYRIGHT:   Copyright 2020
 */

#include <ntoskrnl.h>

//#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
KseVersionLieInitialize(VOID)
{
    UNIMPLEMENTED_ONCE;

    return STATUS_SUCCESS;
}

/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     KSE 'DriverScope' shim implementation
 * COPYRIGHT:   Copyright 2020 Hervé Poussineau (hpoussin@reactos.org)
 */

#include <ntoskrnl.h>

//#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
KseDriverScopeInitialize()
{
    UNIMPLEMENTED_ONCE;

    return STATUS_SUCCESS;
}

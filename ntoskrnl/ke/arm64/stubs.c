/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     x86 only services on ARM64
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
NtSetLdtEntries(
    _In_ ULONG Selector1,
    _In_ LDT_ENTRY LdtEntry1,
    _In_ ULONG Selector2,
    _In_ LDT_ENTRY LdtEntry2)
{
    UNREFERENCED_PARAMETER(Selector1);
    UNREFERENCED_PARAMETER(LdtEntry1);
    UNREFERENCED_PARAMETER(Selector2);
    UNREFERENCED_PARAMETER(LdtEntry2);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtVdmControl(
    _In_ ULONG ControlCode,
    _In_ PVOID ControlData)
{
    UNREFERENCED_PARAMETER(ControlCode);
    UNREFERENCED_PARAMETER(ControlData);
    return STATUS_NOT_IMPLEMENTED;
}

/* Counted by the process manager on every 64-bit target */
ULONG ProcessCount;

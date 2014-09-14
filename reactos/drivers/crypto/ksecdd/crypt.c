/*
 * PROJECT:         ReactOS Drivers
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Kernel Security Support Provider Interface Driver
 *
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "ksecdd.h"

NTSTATUS
NTAPI
KsecEncryptMemory (
    _Inout_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ ULONG OptionFlags)
{
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KsecDecryptMemory (
    _Inout_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ ULONG OptionFlags)
{
    return STATUS_SUCCESS;
}

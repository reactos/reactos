/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Support routines for flushing the TLB
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>

/* FUNCTIONS ******************************************************************/

#ifdef CONFIG_SMP
static
VOID
NTAPI
KiFlushSingleTbIpiWorker(
    _In_ PKIPI_CONTEXT PacketContext,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3)
{
    KxFlushSingleCurrentTb(Parameter1);
}
#endif

VOID
NTAPI
KeFlushSingleTb(
    _In_ PVOID Address,
    _In_ BOOLEAN AllProcessors)
{
#ifdef CONFIG_SMP
    KAFFINITY TargetSet = AllProcessors ?
        KeActiveProcessors : KeGetCurrentProcess()->ActiveProcessors;
    KiIpiSendRequest(TargetSet,
                     KiFlushSingleTbIpiWorker,
                     Address,
                     NULL,
                     NULL);
#else
    KxFlushSingleCurrentTb(Address);
#endif
}

#ifdef CONFIG_SMP
static
VOID
NTAPI
KiFlushRangeTbIpiWorker(
    _In_ PKIPI_CONTEXT PacketContext,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3)
{
    KxFlushRangeCurrentTb(Parameter1, PtrToUlong(Parameter2));
}
#endif

VOID
NTAPI
KeFlushRangeTb(
    _In_ PVOID Address,
    _In_ ULONG NumberOfPages,
    _In_ BOOLEAN Global)
{

    if (Global)
    {
        if (NumberOfPages > KxFlushIndividualGlobalPagesMaximum)
        {
            /* Flush the entire TLB instead */
            KeFlushEntireTb(TRUE, TRUE);
            return;
        }
    }
    else
    {
        if (NumberOfPages > KxFlushIndividualProcessPagesMaximum)
        {
            /* Flush the entire TLB instead */
            KeFlushProcessTb();
            return;
        }
    }

#ifdef CONFIG_SMP
    KAFFINITY TargetSet = Global ?
        KeActiveProcessors : KeGetCurrentProcess()->ActiveProcessors;
    KiIpiSendRequest(TargetSet,
                     KiFlushRangeTbIpiWorker,
                     Address,
                     ULongToPtr(NumberOfPages),
                     NULL);
#else
    KxFlushRangeCurrentTb(Address, NumberOfPages);
#endif
}

#ifdef CONFIG_SMP
static
VOID
NTAPI
KiFlushProcessTbIpiWorker(
    _In_ PKIPI_CONTEXT PacketContext,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3)
{
    KxFlushProcessCurrentTb();
}
#endif

VOID
NTAPI
KeFlushProcessTb(VOID)
{
#ifdef CONFIG_SMP
    KiIpiSendRequest(KeGetCurrentProcess()->ActiveProcessors,
                     KiFlushProcessTbIpiWorker,
                     NULL,
                     NULL,
                     NULL);
#else
    KxFlushProcessCurrentTb();
#endif
}

#ifdef CONFIG_SMP
static
VOID
NTAPI
KiFlushEntireTbIpiWorker(
    _In_ PKIPI_CONTEXT PacketContext,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3)
{
    /* Flush the current TB */
    KxFlushEntireCurrentTb();
}
#endif

VOID
NTAPI
KeFlushEntireTb(
    _In_ BOOLEAN Invalid,
    _In_ BOOLEAN AllProcessors)
{
#ifdef CONFIG_SMP
    KAFFINITY TargetSet = AllProcessors ?
        KeActiveProcessors : KeGetCurrentProcess()->ActiveProcessors;
    KiIpiSendRequest(TargetSet,
                     KiFlushEntireTbIpiWorker,
                     NULL,
                     NULL,
                     NULL);
#else
    KxFlushEntireCurrentTb();
#endif
}

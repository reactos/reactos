/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NdisMGetDmaAlignment/NdisMReadDmaCounter without a DMA adapter
 * COPYRIGHT:   Copyright 2026 ReactOS Lab
 */

#include <kmt_test.h>
#include <ndis.h>

/* LOGICAL_ADAPTER is internal to the NDIS wrapper and is included by path:
 * <miniport.h> is already taken by sdk/include/ddk/miniport.h, and putting the
 * NDIS directory on the include path would shadow the <debug.h> that the kmtest
 * standalone harness includes. */
#include "../../../../drivers/network/ndis/include/miniport.h"

/* Exported by ndis.sys, see drivers/network/ndis/ndis.spec */
ULONG NTAPI NdisMGetDmaAlignment(NDIS_HANDLE MiniportAdapterHandle);
ULONG NTAPI NdisMReadDmaCounter(NDIS_HANDLE MiniportDmaHandle);

#define NDIS_DMA_TEST_TAG 'amdN'    /* "Ndma" */

static
VOID
TestDmaOnUninitializedAdapter(VOID)
{
    PLOGICAL_ADAPTER Adapter;
    PNDIS_DMA_BLOCK DmaBlock;
    ULONG Alignment = 0;
    ULONG Counter = 0;

    ok_irql(PASSIVE_LEVEL);

    Adapter = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Adapter), NDIS_DMA_TEST_TAG);
    if (skip(Adapter != NULL, "Failed to allocate a logical adapter\n"))
        return;

    /* SystemAdapterObject is only set by NdisMAllocateMapRegisters,
     * NdisMRegisterDmaChannel and NdisMInitializeScatterGatherDma. A zeroed
     * adapter is what a miniport passes before it calls them. Without the NULL
     * guard both exports dereference a NULL adapter object inside ndis.sys and
     * the guest bugchecks with STOP 0x7E / 0xC0000005. */
    RtlZeroMemory(Adapter, sizeof(*Adapter));

    Alignment = NdisMGetDmaAlignment((NDIS_HANDLE)Adapter);
    trace("NdisMGetDmaAlignment(uninitialized adapter) = %lu\n", Alignment);
    ok_eq_ulong(Alignment, 1);

    DmaBlock = ExAllocatePoolWithTag(NonPagedPool, sizeof(*DmaBlock), NDIS_DMA_TEST_TAG);
    if (skip(DmaBlock != NULL, "Failed to allocate a DMA block\n"))
    {
        ExFreePoolWithTag(Adapter, NDIS_DMA_TEST_TAG);
        return;
    }

    RtlZeroMemory(DmaBlock, sizeof(*DmaBlock));

    Counter = NdisMReadDmaCounter((NDIS_HANDLE)DmaBlock);
    trace("NdisMReadDmaCounter(uninitialized DMA block) = %lu\n", Counter);
    ok_eq_ulong(Counter, 1);

    if (Alignment == 1 && Counter == 1)
        trace("NDIS DMA alignment test: PASS\n");
    else
        trace("NDIS DMA alignment test: FAIL\n");

    ExFreePoolWithTag(DmaBlock, NDIS_DMA_TEST_TAG);
    ExFreePoolWithTag(Adapter, NDIS_DMA_TEST_TAG);
}

NTSTATUS
TestEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PCUNICODE_STRING RegistryPath,
    OUT PCWSTR *DeviceName,
    IN OUT INT *Flags)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(Flags);

    *DeviceName = L"NdisDma";

    TestDmaOnUninitializedAdapter();

    return STATUS_SUCCESS;
}

VOID
TestUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverObject);
}

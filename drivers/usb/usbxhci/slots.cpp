/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device context base address array, scratchpad and slot map
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhcidrv.h"

#define NDEBUG
#include <debug.h>

/* The register module only accepts controllers with a 4 KB page, so scratchpads are 4 KB */
#define XHCI_SCRATCHPAD_BYTES   4096

NTSTATUS
XhciSlotTable::Create(
    _In_ XhciController* Controller)
{
    m_Controller = Controller;

    /* Release walks this list even when Prepare never ran */
    InitializeListHead(&m_ScratchpadPages);
    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
XhciSlotTable::Prepare()
{
    XhciCommonBuffer* Pool = &m_Controller->m_Buffers;
    volatile ULONG64* Entries;
    PULONG64 Array;
    PLIST_ENTRY Entry;
    SIZE_T MapBytes;
    ULONG Index;
    NTSTATUS Status;

    InitializeListHead(&m_ScratchpadPages);

    m_Dcbaa = Pool->Acquire(XHCI_DCBAA_ENTRIES * sizeof(ULONG64));
    if (m_Dcbaa == NULL)
    {
        DPRINT1("No common buffer for the DCBAA\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Fail;
    }

    Entries = (volatile ULONG64*)m_Dcbaa->VirtualAddress;

    m_ScratchpadCount = m_Controller->m_Registers.ScratchpadCount();
    if (m_ScratchpadCount != 0)
    {
        /* QUIRK: above 512 scratchpads the array no longer fits a pool page and this fails */
        m_ScratchpadArray = Pool->Acquire(m_ScratchpadCount * sizeof(ULONG64));
        if (m_ScratchpadArray == NULL)
        {
            DPRINT1("No common buffer for the array of %lu scratchpads\n", m_ScratchpadCount);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Fail;
        }

        Status = Pool->AcquireList(XHCI_SCRATCHPAD_BYTES, m_ScratchpadCount, &m_ScratchpadPages);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Acquiring %lu scratchpad pages failed 0x%lx\n", m_ScratchpadCount, Status);
            goto Fail;
        }

        Array = (PULONG64)m_ScratchpadArray->VirtualAddress;
        Index = 0;
        for (Entry = m_ScratchpadPages.Flink; Entry != &m_ScratchpadPages; Entry = Entry->Flink)
        {
            XhciDmaBuffer* Page = CONTAINING_RECORD(Entry, XhciDmaBuffer, Link);

            Array[Index++] = Page->LogicalAddress.QuadPart;
        }

        Entries[0] = m_ScratchpadArray->LogicalAddress.QuadPart;
    }

    m_SlotCount = m_Controller->m_Registers.MaxSlots();

    /* Index 0 is never a slot id; keeping it makes the slot id the index */
    MapBytes = (m_SlotCount + 1) * sizeof(*m_SlotMap);
    m_SlotMap = (PVOID*)ExAllocatePoolWithTag(NonPagedPool, MapBytes, XHCI_TAG_SLOT);
    if (m_SlotMap == NULL)
    {
        DPRINT1("No pool for the map of %lu slots\n", m_SlotCount);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Fail;
    }

    RtlZeroMemory(m_SlotMap, MapBytes);

    DPRINT("Slot table: %lu slots, %lu scratchpads, DCBAA at 0x%I64x\n",
           m_SlotCount, m_ScratchpadCount, m_Dcbaa->LogicalAddress.QuadPart);
    return STATUS_SUCCESS;

Fail:
    FreeTables();
    return Status;
}

VOID
XhciSlotTable::FreeTables()
{
    XhciCommonBuffer* Pool = &m_Controller->m_Buffers;

    if (m_ScratchpadPages.Flink != NULL)
        Pool->FreeList(&m_ScratchpadPages);

    if (m_ScratchpadArray != NULL)
    {
        Pool->Free(m_ScratchpadArray);
        m_ScratchpadArray = NULL;
    }

    if (m_Dcbaa != NULL)
    {
        Pool->Free(m_Dcbaa);
        m_Dcbaa = NULL;
    }

    if (m_SlotMap != NULL)
    {
        ExFreePoolWithTag(m_SlotMap, XHCI_TAG_SLOT);
        m_SlotMap = NULL;
    }

    m_SlotCount = 0;
    m_ScratchpadCount = 0;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
XhciSlotTable::Release()
{
    if (m_Dcbaa != NULL)
    {
        /* QUIRK: written even when the controller is no longer accessible */
        m_Controller->m_Registers.WriteOperational32(XHCI_OP_CONFIG, 0);
        m_Controller->m_Registers.WriteOperational64(XHCI_OP_DCBAAP, 0);
    }

    FreeTables();
}

VOID
XhciSlotTable::Program()
{
    if (m_Dcbaa == NULL || !m_Controller->IsAccessible())
        return;

    /* CIE stays 0: configuration information is not used */
    m_Controller->m_Registers.WriteOperational32(XHCI_OP_CONFIG,
                                                 m_SlotCount & XHCI_CONFIG_MAX_SLOTS_MASK);
    m_Controller->m_Registers.WriteOperational64(XHCI_OP_DCBAAP,
                                                 m_Dcbaa->LogicalAddress.QuadPart);
}

VOID
XhciSlotTable::D0Entry()
{
    Program();
}

VOID
XhciSlotTable::ZeroScratchpads()
{
    PLIST_ENTRY Entry;

    if (m_ScratchpadPages.Flink == NULL)
        return;

    for (Entry = m_ScratchpadPages.Flink; Entry != &m_ScratchpadPages; Entry = Entry->Flink)
    {
        XhciDmaBuffer* Page = CONTAINING_RECORD(Entry, XhciDmaBuffer, Link);

        RtlZeroMemory(Page->VirtualAddress, Page->Size);
    }
}

VOID
XhciSlotTable::DiscardState()
{
    ZeroScratchpads();
    DisableAll();
}

VOID
XhciSlotTable::DisableAll()
{
    ULONG SlotId;

    for (SlotId = 1; IsValidSlot(SlotId); SlotId++)
    {
        if (m_SlotMap[SlotId] == NULL)
            continue;

        /* No USB device module yet to mark the device disabled, so drop the slot here */
        DPRINT("Slot %lu lost its device %p\n", SlotId, m_SlotMap[SlotId]);
        SetSlot(SlotId, NULL, 0);
    }
}

VOID
XhciSlotTable::PreReset()
{
    /* Endpoints get ControllerResetStarting here once USB devices exist */
    DPRINT("Controller reset starting, %lu slots\n", m_SlotCount);
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
XhciSlotTable::PostReset()
{
    /* Endpoints get the reset complete handshake here once USB devices exist */
    DisableAll();
    ZeroScratchpads();
    Program();
}

VOID
XhciSlotTable::OnHostLost()
{
    /* Endpoints get ControllerRemoved here once USB devices exist */
    DPRINT("Controller gone, %lu slots\n", m_SlotCount);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XhciSlotTable::SetSlot(
    _In_ ULONG SlotId,
    _In_ PVOID Device,
    _In_ ULONG64 OutputContextAddress)
{
    volatile ULONG64* Entries;

    if (!IsValidSlot(SlotId) || m_Dcbaa == NULL)
    {
        DPRINT1("Slot id %lu is outside the slot table\n", SlotId);
        return;
    }

    Entries = (volatile ULONG64*)m_Dcbaa->VirtualAddress;

    /* Software clears the entry once the slot is disabled (xHCI 4.6.4) */
    if (OutputContextAddress == 0)
    {
        ASSERT(Device == NULL || m_SlotMap[SlotId] == Device);
        m_SlotMap[SlotId] = NULL;
        Entries[SlotId] = 0;
        return;
    }

    /* QUIRK: nothing is recorded while the controller is inaccessible */
    if (!m_Controller->IsAccessible())
        return;

    if (Entries[SlotId] != 0)
    {
        DPRINT1("DCBAA entry of slot %lu is already 0x%I64x\n", SlotId, (ULONG64)Entries[SlotId]);
        return;
    }

    m_SlotMap[SlotId] = Device;
    Entries[SlotId] = OutputContextAddress;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
PVOID
XhciSlotTable::LookupSlot(
    _In_ ULONG SlotId) const
{
    if (!IsValidSlot(SlotId))
        return NULL;

    return m_SlotMap[SlotId];
}

VOID
XhciSlotTable::OnTransferEvent(
    _In_ const XHCI_TRB* Event)
{
    ULONG SlotId = XhciTrbSlotId(Event);
    ULONG Endpoint = (Event->Dword[3] & XHCI_TRB_ENDPOINT_MASK) >> XHCI_TRB_ENDPOINT_SHIFT;
    ULONG Code = XhciTrbCompletionCode(Event);

    if (LookupSlot(SlotId) == NULL)
    {
        DPRINT1("Transfer event for slot %lu endpoint %lu code %lu has no device, dropped\n",
                SlotId, Endpoint, Code);
        return;
    }

    DPRINT("Transfer event slot %lu endpoint %lu code %lu dropped\n", SlotId, Endpoint, Code);
}

VOID
XhciSlotTable::OnDeviceNotificationEvent(
    _In_ const XHCI_TRB* Event)
{
    ULONG Type = (Event->Dword[0] & XHCI_NOTIFICATION_TYPE_MASK) >> XHCI_NOTIFICATION_TYPE_SHIFT;

    /* Function wake would go to UcxUsbDeviceRemoteWakeNotification once devices exist */
    DPRINT("Device notification slot %lu type %lu code %lu dropped\n",
           XhciTrbSlotId(Event), Type, XhciTrbCompletionCode(Event));
}

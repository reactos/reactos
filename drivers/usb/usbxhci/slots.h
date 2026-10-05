/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device context base address array, scratchpad and slot map
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class XhciController;

/** DCBAA, scratchpad buffers and slot id lookup. */
class XhciSlotTable
{
public:
    NTSTATUS
    Create(
        _In_ XhciController* Controller);
    NTSTATUS Prepare();
    VOID Release();

    /** Writes DCBAAP and CONFIG.MaxSlotsEn; the controller calls it before Run. */
    VOID Program();

    VOID D0Entry();
    VOID DisableAll();
    _IRQL_requires_(PASSIVE_LEVEL)
    VOID PreReset();
    _IRQL_requires_(PASSIVE_LEVEL)
    VOID PostReset();
    VOID OnHostLost();

    /** Sets the DCBAA entry of a slot, or clears it for LogicalAddress 0; STATUS_ACCESS_DENIED if in use. */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    NTSTATUS
    SetSlot(
        _In_ ULONG SlotId,
        _In_ PVOID Device,
        _In_ ULONG64 OutputContextAddress);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    PVOID
    LookupSlot(
        _In_ ULONG SlotId) const;

    /* From the interrupter DPC; routed to the device that owns the slot */
    VOID
    OnTransferEvent(
        _In_ const XHCI_TRB* Event);
    VOID
    OnDeviceNotificationEvent(
        _In_ const XHCI_TRB* Event);


    /** State was lost: zero the scratchpad pages, then disable every slot (D0 entry cleanup). */
    VOID DiscardState();

private:
    VOID ZeroScratchpads();
    VOID FreeTables();

    BOOLEAN
    IsValidSlot(
        _In_ ULONG SlotId) const
    {
        return m_SlotMap != NULL && SlotId != 0 && SlotId <= m_SlotCount;
    }

    XhciController* m_Controller;
    ULONG m_SlotCount;
    ULONG m_ScratchpadCount;
    XhciDmaBuffer* m_Dcbaa;
    XhciDmaBuffer* m_ScratchpadArray;
    LIST_ENTRY m_ScratchpadPages;
    PVOID* m_SlotMap;
};

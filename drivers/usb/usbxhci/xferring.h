/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Transfer ring object shared by the control, bulk and isoch transfer types
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class XhciController;
class XhciUsbDevice;
class XhciEndpoint;

/** Processor stamp for URBs the driver builds itself; UCX never stamps this value. */
#define XHCI_URB_NO_PROCESSOR   MAXULONG

/** Mapping permission of a ring. */
enum class XhciMapState : LONG
{
    Halted = 0,
    Halting = 1,
    PausedFill = 2,
    Filling = 3,
    AwaitingStarve = 4       /**< Isoch only: waiting for an underrun or overrun */
};

/** One transfer ring, stored as its WDFQUEUE context; the type specific state follows in m_Type. */
class XhciTransferRing
{
public:
    /** PASSIVE_LEVEL. */
    static NTSTATUS
    Create(
        _In_ XhciEndpoint* Endpoint,
        _In_ WDFOBJECT Parent,
        _In_ ULONG StreamId,
        _Out_ XhciTransferRing** Ring);

    static XhciTransferRing*
    FromQueue(
        _In_ WDFQUEUE Queue);

    WDFQUEUE Queue() const;

    _IRQL_requires_max_(DISPATCH_LEVEL)
    NTSTATUS Enable();
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    Disable(
        _In_ BOOLEAN FreeResources);

    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS
    EnableForwardProgress(
        _In_ ULONG MaxTransferSize);

    /** Zeroes the current segment and resets enqueue and cycle state. */
    VOID InitializeRing();

    ULONG64 SegmentBase() const;
    ULONG64 EnqueuePointer() const;

    /** Enqueue pointer with DCS, plus SCT 1 for a stream ring (context and Set TR Dequeue value). */
    ULONG64 DequeuePointerValue() const;

    /* Endpoint machine actions fanned out by the endpoint */
    VOID ResumeRingFill();
    VOID SuspendRingFill();
    VOID OnPipeHalted();
    VOID OnClientPipeReset();
    VOID StoppedEventReceived();
    VOID AllowReclaimOnCancel();
    VOID ConsumePendingEvents();
    VOID RecoverTransfers();
    BOOLEAN DoorbellRungSinceFill() const;
    BOOLEAN HasQueuedWork() const;

    /** TRUE when this ring recognized the event (the endpoint gave it to this ring). */
    BOOLEAN
    OnTransferEvent(
        _In_ const XHCI_TRB* Event);

    /** Interrupter fast path: finds the ring by slot and DCI; the type module then checks the Event Data value. */
    static BOOLEAN
    OnUntargetedTransferEvent(
        _In_ XhciController* Controller,
        _In_ const XHCI_TRB* Event);

    /** Stream duplicate heuristic for ED = 0 events nobody claimed (bulk only). */
    BOOLEAN
    IsLikelyDuplicate(
        _In_ const XHCI_TRB* Event) const;

    /* Helpers for the control and bulk type modules */

    /** Makes sure the free pool holds Count segments; Async may return STATUS_PENDING. */
    NTSTATUS
    EnsureFreeSegments(
        _In_ ULONG Count,
        _In_ BOOLEAN Async);

    /** Moves Count pool segments to the head of List. Same results as EnsureFreeSegments. */
    NTSTATUS
    TakeSegments(
        _In_ ULONG Count,
        _Inout_ PLIST_ENTRY List);

    /** Returns segments to the pool; the caller holds m_Lock. */
    VOID
    ReturnSegments(
        _Inout_ PLIST_ENTRY List,
        _In_ BOOLEAN ToHead);

    /** 0x200 byte buffer from the double buffer cache, or a new one; NULL on failure. */
    XhciDmaBuffer* BorrowBounceBuffer();
    VOID
    ReturnBounceBuffer(
        _In_ XhciDmaBuffer* Buffer);

    /** Interlocked map state change, unconditional when FromAny; returns the old state. */
    XhciMapState
    ChangeMapState(
        _In_ BOOLEAN FromAny,
        _In_ XhciMapState From,
        _In_ XhciMapState To);

    /** Zeroed link TRB with type, cycle, chain and interrupter target set; the caller adds pointer and TC. */
    VOID
    BuildLinkTrb(
        _Out_ PXHCI_TRB Trb,
        _In_ BOOLEAN FirstOfTd,
        _In_ BOOLEAN InDataStage) const;

    ULONG
    PacketCount(
        _In_ ULONG Bytes) const;

    /** TD Size field (xHCI 4.11.2.4) for a TRB, given bytes mapped so far including it. */
    ULONG
    TdSize(
        _In_ ULONG PacketCount,
        _In_ ULONG BytesMappedSoFar,
        _In_ BOOLEAN LastTrbOfTd) const;

    /** Recomputes the 10 bit interrupter target for work issued on this processor. */
    VOID UpdateInterrupterTarget();

    /** Same, for the processor index UCX stamped into the URB; XHCI_URB_NO_PROCESSOR means the current one. */
    VOID
    UpdateInterrupterTarget(
        _In_ ULONG ProcessorIndex);

    /** Processor index UCX stamped into the first ULONG of the URB's HCD area. */
    static ULONG
    UrbProcessorIndex(
        _In_ PURB Urb);

    /** Rings this ring's doorbell (DCI and stream id) and records that it was rung. */
    VOID RingDoorbell();

    static USBD_STATUS
    UsbdStatusFromCompletionCode(
        _In_ ULONG CompletionCode);
    static NTSTATUS
    NtStatusFromUsbdStatus(
        _In_ USBD_STATUS UsbdStatus);

    /* Common ring state; the type modules read and write it under the rules of each field */
    XhciController* m_Controller;
    XhciUsbDevice* m_Device;
    XhciEndpoint* m_Endpoint;
    ULONG m_StreamId;
    ULONG m_TransferType;           /**< bmAttributes & 3 */
    WDFQUEUE m_Queue;
    WDFDPC m_CompletionDpc;
    KSPIN_LOCK m_Lock;              /**< Guards the segment fields, pools and type state */
    volatile LONG m_MapState;       /**< XhciMapState */
    ULONG m_InterrupterTarget;
    BOOLEAN m_DoorbellRung;         /**< Since the last ResumeRingFill */
    PMDL m_ReservedMdl;

    ULONG m_SegmentSize;
    ULONG m_DoubleBufferSize;
    ULONG m_MaxStageSize;
    ULONG m_MapRegisterCount;

    XhciDmaBuffer* m_Segment;       /**< Current segment */
    PXHCI_TRB m_Trbs;
    ULONG m_EnqueueIndex;
    ULONG m_LastIndex;              /**< Segment size / 16 - 1 */
    ULONG m_Cycle;                  /**< Producer cycle state */
    LIST_ENTRY m_FreeSegments;
    LIST_ENTRY m_DoubleBuffers;

    union
    {
        XhciControlRing Control;
        XhciBulkRing Bulk;
        XhciIsochRing Isoch;
    } m_Type;

    /* Implementation state of the common ring; the type modules do not touch these */
    VOID Cleanup();
    _IRQL_requires_(PASSIVE_LEVEL)
    VOID GrowSegments();

    BOOLEAN IsControl() const;
    BOOLEAN IsBulkOrInterrupt() const;
    BOOLEAN IsIsoch() const;

    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID WaitForMappingIdle();

    VOID MakeLive();
    VOID RetireLive();

    /** Finds the non stream ring of Device and Dci and takes an event reference on it. */
    static XhciTransferRing*
    ReferenceLiveRing(
        _In_ XhciController* Controller,
        _In_ const XhciUsbDevice* Device,
        _In_ ULONG Dci);
    VOID DereferenceLive();

    LIST_ENTRY m_LiveLink;
    volatile LONG m_EventUsers;
    BOOLEAN m_Live;
    BOOLEAN m_TypeReady;
    BOOLEAN m_GrowthQueued;
    BOOLEAN m_ForwardProgressQueue;
    ULONG m_GrowthWanted;
    WDFWORKITEM m_GrowthItem;
};

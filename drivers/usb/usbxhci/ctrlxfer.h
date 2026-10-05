/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Control transfer ring type
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class XhciController;
class XhciTransferRing;
struct XhciControlTransfer;

/** Control ring state and the entry points the transfer ring calls for it. */
class XhciControlRing
{
public:
    static NTSTATUS
    Initialize(
        _In_ XhciTransferRing* Ring);
    static VOID
    Enable(
        _In_ XhciTransferRing* Ring);
    static VOID
    Disable(
        _In_ XhciTransferRing* Ring);
    static NTSTATUS
    EnableForwardProgress(
        _In_ XhciTransferRing* Ring,
        _In_ ULONG MaxTransferSize);
    static VOID
    Cleanup(
        _In_ XhciTransferRing* Ring);

    static EVT_WDF_IO_QUEUE_IO_DEFAULT EvtIoDefault;
    static EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE EvtIoCanceledOnQueue;
    static EVT_WDF_DPC EvtCompletionDpc;

    static VOID
    ResumeRingFill(
        _In_ XhciTransferRing* Ring);
    static VOID
    SuspendRingFill(
        _In_ XhciTransferRing* Ring);
    static VOID
    AllowReclaimOnCancel(
        _In_ XhciTransferRing* Ring);
    static VOID
    ConsumePendingEvents(
        _In_ XhciTransferRing* Ring);
    static VOID
    RecoverTransfers(
        _In_ XhciTransferRing* Ring);
    static BOOLEAN
    DoorbellRungSinceFill(
        _In_ const XhciTransferRing* Ring);
    static BOOLEAN
    HasQueuedWork(
        _In_ const XhciTransferRing* Ring);

    /** The ring is always known; the NULL ring form is kept only for callers without one. */
    static BOOLEAN
    OnTransferEvent(
        _In_opt_ XhciTransferRing* Ring,
        _In_ XhciController* Controller,
        _In_ const XHCI_TRB* Event);

private:
    /**
     * What an Event Data TRB points at. Tags live in the ring, not in the request, so
     * a late or duplicate event never reads freed request memory.
     */
    struct DECLSPEC_ALIGN(8) EventTag
    {
        ULONG Signature;
        ULONG Index;
        XhciTransferRing* Ring;
        XhciControlTransfer* Transfer;
    };

    static const ULONG TagCount = 4;

    static EVT_WDF_TIMER EvtTimeout;
    static EVT_WDF_DPC EvtCanceledOnQueueDpc;
    static EVT_WDF_REQUEST_CANCEL EvtRequestCancel;
    static DRIVER_LIST_CONTROL EvtDmaReady;

    VOID
    InitTransfer(
        _Out_ XhciControlTransfer* Transfer,
        _In_ WDFREQUEST Request);
    NTSTATUS
    PrepareTransfer(
        _Inout_ XhciControlTransfer* Transfer);
    VOID
    BuildControlTd(
        _Inout_ XhciControlTransfer* Transfer);
    VOID
    WriteTd(
        _Inout_ XhciControlTransfer* Transfer);
    VOID
    ReleaseResources(
        _Inout_ XhciControlTransfer* Transfer);
    _Requires_lock_held_(m_Ring->m_Lock)
    NTSTATUS CompleteHeld();
    _Requires_lock_held_(m_Ring->m_Lock)
    VOID
    Finish(
        _Inout_ XhciControlTransfer* Transfer);
    VOID
    Abort(
        _Inout_ XhciControlTransfer* Transfer,
        _In_ BOOLEAN FromTimer);
    VOID
    QueueCompletion();
    _Requires_lock_held_(m_Ring->m_Lock)
    VOID
    CopyIn(
        _Inout_ XhciControlTransfer* Transfer,
        _In_ ULONG Bytes);
    /** The tag an Event Data value names, or NULL when it is not a live tag (of Ring, when given). */
    static EventTag*
    TagFromEvent(
        _In_opt_ XhciTransferRing* Ring,
        _In_ const XHCI_TRB* Event);
    BOOLEAN
    OnEventDataEvent(
        _In_ EventTag* Tag,
        _In_ const XHCI_TRB* Event);
    BOOLEAN
    OnTrbEvent(
        _In_ const XHCI_TRB* Event);
    BOOLEAN
    LocateTrb(
        _In_ const XhciControlTransfer* Transfer,
        _In_ const XHCI_TRB* Event,
        _Out_ PULONG Bytes,
        _Out_ PULONG SkippedTds) const;

    EventTag m_Tags[TagCount];
    XhciTransferRing* m_Ring;
    XhciControlTransfer* m_Current;     /**< The one request on this ring, or NULL */
    LIST_ENTRY m_CanceledOnQueue;
    WDFDPC m_CanceledOnQueueDpc;
    WDFTIMER m_Timer;
    ULONG m_NextTag;
    LONG m_Outstanding;                 /**< Events still expected after a stop */
    volatile LONG m_ReservedMdlBusy;
    BOOLEAN m_ImmediateAllowed;
    BOOLEAN m_ReclaimAndAcknowledge;
    BOOLEAN m_AcknowledgeExpectedEvents;
    BOOLEAN m_ReclaimOnCancelAllowed;
    BOOLEAN m_CanceledDpcRunning;
};

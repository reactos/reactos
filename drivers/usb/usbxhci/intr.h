/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Interrupts, event rings and event dispatch
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class XhciController;

/** All interrupters of a controller. */
class XhciInterrupters
{
public:
    NTSTATUS
    Create(
        _In_ XhciController* Controller);

    /** Called from EvtDeviceFilterRemoveResourceRequirements. */
    NTSTATUS
    FilterResourceRequirements(
        _In_ WDFIORESREQLIST Requirements);

    NTSTATUS
    Prepare(
        _In_ WDFCMRESLIST Raw,
        _In_ WDFCMRESLIST Translated);
    VOID Release();

    /** Programs ERST, ERDP and IMAN for every interrupter before Run. */
    NTSTATUS
    D0Entry(
        _In_ BOOLEAN StatePreserved);
    VOID D0ExitPreInterruptsDisabled();
    VOID D0Exit();
    VOID PostReset();

    ULONG Count() const;

    /** Interrupter target for work issued on the current processor. */
    ULONG TargetForCurrentProcessor() const;


    /** Called from EvtDeviceFilterAddResourceRequirements. */
    NTSTATUS
    AffinitizeResourceRequirements(
        _In_ WDFIORESREQLIST Requirements);

    /** One interrupter register set and its event ring. The WDFINTERRUPT context points here. */
    struct Interrupter
    {
        XhciController* Controller;
        WDFINTERRUPT Interrupt;
        ULONG Index;
        ULONG RegisterOffset;       /**< Runtime offset of IMAN for this set */
        USHORT Group;
        KAFFINITY Affinity;

        BOOLEAN MessageSignaled;
        BOOLEAN Prepared;
        BOOLEAN Enabled;
        BOOLEAN PendingDisable;     /**< Guarded by Lock */
        BOOLEAN DpcRunning;         /**< Guarded by Lock */

        ULONG SegmentSize;
        ULONG SegmentCount;
        ULONG TrbsPerSegment;
        ULONG CycleState;
        ULONG DequeueIndex;
        ULONG DequeueSegment;
        XhciDmaBuffer* Segment;     /**< Segment holding the dequeue position */
        LIST_ENTRY SegmentList;
        XhciDmaBuffer* Erst;

        WDFWORKITEM Requeue;
        KEVENT RequeueIdle;
        KSPIN_LOCK Lock;
        ULONG RingFullCount;
    };

private:
    enum class Mechanism : ULONG
    {
        Line,
        Msi,
        MsiX
    };

    NTSTATUS
    PrepareOne(
        _Inout_ Interrupter* Entry,
        _In_ WDFINTERRUPT Interrupt,
        _In_ ULONG Index,
        _In_ BOOLEAN MessageSignaled);

    VOID
    ReleaseOne(
        _Inout_ Interrupter* Entry);

    NTSTATUS
    CreateInterrupt(
        _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Raw,
        _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
        _Out_ WDFINTERRUPT* Interrupt);

    ULONG
    SecondaryCountFor(
        _In_ ULONG Messages) const;

    NTSTATUS BuildLookup();

    XhciController* m_Controller;
    WDFINTERRUPT m_PrimaryInterrupt;
    Mechanism m_Mechanism;
    ULONG m_SecondaryCount;
    ULONG m_Count;              /**< Interrupters actually prepared */
    ULONG m_Capacity;
    Interrupter* m_Table;
    PUSHORT m_Lookup;
    ULONG m_LookupSize;
};

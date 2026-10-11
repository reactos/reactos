/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     DMA enabler and the pool of zeroed common buffers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class XhciController;

/** One piece of common buffer memory handed out by the pool. */
struct XhciDmaBuffer
{
    LIST_ENTRY Link;            /**< Free for the holder to use while it owns the buffer */
    PVOID VirtualAddress;
    PHYSICAL_ADDRESS LogicalAddress;
    ULONG Size;
    PVOID Page;                 /**< Pool page this segment belongs to */
    BOOLEAN Owned;              /**< FALSE while the segment sits on a free list */
};

/** DMA enabler and buffer pool. */
class XhciCommonBuffer
{
public:
    NTSTATUS
    Create(
        _In_ XhciController* Controller);
    NTSTATUS Prepare();
    VOID Release();
    VOID D0Exit();
    VOID PostResetFlush();
    VOID OnHostLost();

    /** Driven by the controller watchdog. */
    VOID Rebalance();

    /** Zeroed buffer of at least Size bytes, Size <= PAGE_SIZE. NULL when none can be had. */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    XhciDmaBuffer*
    Acquire(
        _In_ ULONG Size);

    /** Count zeroed buffers onto List (linked through Link); all or nothing. */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    NTSTATUS
    AcquireList(
        _In_ ULONG Size,
        _In_ ULONG Count,
        _Inout_ PLIST_ENTRY List);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    Free(
        _In_ XhciDmaBuffer* Buffer);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    FreeList(
        _Inout_ PLIST_ENTRY List);

    WDFDMAENABLER DmaEnabler() const;


    /** ReadFromDevice adapter, used for both directions. */
    PDMA_ADAPTER DmaAdapter() const;
    ULONG MaxDmaSize() const;
    ULONG MapRegisterCount() const;

    /* Work item and cleanup bodies, called only from dmapool.cpp */
    _IRQL_requires_(PASSIVE_LEVEL)
    VOID GrowthWork();
    _IRQL_requires_(PASSIVE_LEVEL)
    VOID ReclaimWork();
    VOID EnablerCleanup();

private:
    /** One size class: 512 byte or 4096 byte segments. */
    struct SizeClass
    {
        LIST_ENTRY FreeList;
        LIST_ENTRY PageList;
        ULONG SegmentSize;
        ULONG Total;
        ULONG Available;
        ULONG GrowBelow;        /**< Grow when Available drops under this */
        ULONG GrowBy;           /**< Segments added per growth */
        ULONG KeepAtLeast;      /**< Never shrink while Available <= this */
    };

    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS CreateEnabler();

    SizeClass*
    ClassForSize(
        _In_ ULONG Size);

    _IRQL_requires_(PASSIVE_LEVEL)
    VOID
    AddSegments(
        _Inout_ SizeClass* Class,
        _In_ ULONG Count);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    BOOLEAN
    Grab(
        _Inout_ SizeClass* Class,
        _In_ ULONG Count,
        _Out_ PLIST_ENTRY Taken,
        _Out_ PBOOLEAN NeedGrow);

    _Requires_lock_held_(m_Lock)
    BOOLEAN
    ReturnFree(
        _Inout_ XhciDmaBuffer* Buffer);

    _Requires_lock_held_(m_Lock)
    VOID
    ShrinkClass(
        _Inout_ SizeClass* Class);

    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID QueueGrowth();

    _IRQL_requires_(PASSIVE_LEVEL)
    VOID Flush();

    XhciController* m_Controller;
    WDFDMAENABLER m_Enabler;
    PDMA_ADAPTER m_Adapter;
    ULONG m_MaxDmaSize;
    ULONG m_MapRegisters;
    BOOLEAN m_Initialized;
    BOOLEAN m_HoldsUcxReference;

    KSPIN_LOCK m_Lock;
    SizeClass m_Small;
    SizeClass m_Large;
    ULONG m_PagesTotal;
    ULONG m_PagesInUse;

    LIST_ENTRY m_ReclaimList;
    ULONG m_ReclaimPages;
    WDFWORKITEM m_ReclaimItem;

    WDFWORKITEM m_GrowthItem;
    BOOLEAN m_GrowthQueued;
    ULONG m_GrowthOutstanding;
    KEVENT m_GrowthIdle;
};

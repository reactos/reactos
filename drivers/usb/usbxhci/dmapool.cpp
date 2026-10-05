/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     DMA enabler and the pool of zeroed common buffers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhcidrv.h"

#define NDEBUG
#include <debug.h>

/* Every pool page is one 4 KB common buffer split into 1 or 8 segments */
#define XHCI_POOL_PAGE_BYTES        4096
#define XHCI_POOL_SMALL_BYTES       512
#define XHCI_POOL_DMA_LIMIT         0xFFFE00

/** WDFCOMMONBUFFER context: the page header followed by its segments. */
typedef struct _XHCI_POOL_PAGE
{
    LIST_ENTRY Link;
    WDFCOMMONBUFFER CommonBuffer;
    BOOLEAN ReleaseQueued;
    ULONG SegmentCount;
    XhciDmaBuffer Segments[ANYSIZE_ARRAY];
} XHCI_POOL_PAGE, *PXHCI_POOL_PAGE;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XHCI_POOL_PAGE, XhciPoolPageFromBuffer);

/** Context of the enabler and the pool work items. */
typedef struct _XHCI_POOL_OWNER
{
    XhciCommonBuffer* Pool;
} XHCI_POOL_OWNER, *PXHCI_POOL_OWNER;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XHCI_POOL_OWNER, XhciPoolOwner);

static EVT_WDF_WORKITEM XhciPoolGrowthWorkItem;
static EVT_WDF_WORKITEM XhciPoolReclaimWorkItem;
static EVT_WDF_OBJECT_CONTEXT_CLEANUP XhciPoolEnablerCleanup;

static
VOID
NTAPI
XhciPoolGrowthWorkItem(
    _In_ WDFWORKITEM WorkItem)
{
    XhciPoolOwner(WorkItem)->Pool->GrowthWork();
}

static
VOID
NTAPI
XhciPoolReclaimWorkItem(
    _In_ WDFWORKITEM WorkItem)
{
    XhciPoolOwner(WorkItem)->Pool->ReclaimWork();
}

static
VOID
NTAPI
XhciPoolEnablerCleanup(
    _In_ WDFOBJECT Object)
{
    XhciCommonBuffer* Pool = XhciPoolOwner(Object)->Pool;

    /* Creation failed before the enabler was handed to the pool */
    if (Pool != NULL)
        Pool->EnablerCleanup();
}

static
VOID
NTAPI
XhciPoolInitClass(
    _Out_ PLIST_ENTRY FreeList,
    _Out_ PLIST_ENTRY PageList)
{
    InitializeListHead(FreeList);
    InitializeListHead(PageList);
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
XhciCommonBuffer::Create(
    _In_ XhciController* Controller)
{
    if (!m_Initialized)
    {
        m_Controller = Controller;
        KeInitializeSpinLock(&m_Lock);
        InitializeListHead(&m_ReclaimList);
        KeInitializeEvent(&m_GrowthIdle, NotificationEvent, TRUE);

        /* Pool sizing policy; the test signing overrides are not supported */
        XhciPoolInitClass(&m_Small.FreeList, &m_Small.PageList);
        m_Small.SegmentSize = XHCI_POOL_SMALL_BYTES;
        m_Small.GrowBelow = 4;
        m_Small.GrowBy = 8;
        m_Small.KeepAtLeast = 16;

        XhciPoolInitClass(&m_Large.FreeList, &m_Large.PageList);
        m_Large.SegmentSize = XHCI_POOL_PAGE_BYTES;
        m_Large.GrowBelow = 1;
        m_Large.GrowBy = 1;
        m_Large.KeepAtLeast = 2;

        m_Initialized = TRUE;
    }

    /* The enabler needs AC64, so it waits for the registers when called early */
    if (m_Enabler == NULL && Controller->IsAccessible())
        return CreateEnabler();

    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
XhciCommonBuffer::Prepare()
{
    if (!m_Initialized)
    {
        DPRINT1("DMA pool prepared before it was created\n");
        return STATUS_INVALID_DEVICE_STATE;
    }

    /* The enabler and its pages live as long as the WDFDEVICE */
    if (m_Enabler != NULL)
        return STATUS_SUCCESS;

    return CreateEnabler();
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
XhciCommonBuffer::CreateEnabler()
{
    WDF_DMA_ENABLER_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_WORKITEM_CONFIG ItemConfig;
    WDF_DMA_PROFILE Profile;
    WDFDMAENABLER Enabler;
    size_t Fragment;
    NTSTATUS Status;

    if (m_Controller->m_Registers.Addressing64())
    {
        Profile = WdfDmaProfileScatterGather64Duplex;
    }
    else
    {
        m_Controller->SetErrata(XhciErrata::RegSplit64BitAccess);
        Profile = WdfDmaProfileScatterGatherDuplex;
    }

    /* QUIRK: the limit is a multiple of 512 but not of 1024 */
    WDF_DMA_ENABLER_CONFIG_INIT(&Config, Profile, XHCI_POOL_DMA_LIMIT);
    Config.Flags = WDF_DMA_ENABLER_CONFIG_NO_SGLIST_PREALLOCATION;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XHCI_POOL_OWNER);
    Attributes.EvtCleanupCallback = XhciPoolEnablerCleanup;

    Status = WdfDmaEnablerCreate(m_Controller->m_Device, &Config, &Attributes, &Enabler);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WdfDmaEnablerCreate failed 0x%lx (64 bit %u)\n",
                Status, m_Controller->m_Registers.Addressing64());
        return Status;
    }

    Fragment = WdfDmaEnablerGetFragmentLength(Enabler, WdfDmaDirectionReadFromDevice);
    m_MaxDmaSize = (ULONG)min(Fragment, (size_t)XHCI_POOL_DMA_LIMIT);
    m_MapRegisters = BYTES_TO_PAGES(m_MaxDmaSize) + 1;
    m_Adapter = WdfDmaEnablerWdmGetDmaAdapter(Enabler, WdfDmaDirectionReadFromDevice);

    /* Both work items exist before any page so a failure only has the enabler to undo */
    WDF_WORKITEM_CONFIG_INIT(&ItemConfig, XhciPoolReclaimWorkItem);
    ItemConfig.AutomaticSerialization = FALSE;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XHCI_POOL_OWNER);
    Attributes.ParentObject = Enabler;

    Status = WdfWorkItemCreate(&ItemConfig, &Attributes, &m_ReclaimItem);
    if (NT_SUCCESS(Status))
    {
        XhciPoolOwner(m_ReclaimItem)->Pool = this;

        WDF_WORKITEM_CONFIG_INIT(&ItemConfig, XhciPoolGrowthWorkItem);
        ItemConfig.AutomaticSerialization = FALSE;
        Status = WdfWorkItemCreate(&ItemConfig, &Attributes, &m_GrowthItem);
        if (NT_SUCCESS(Status))
            XhciPoolOwner(m_GrowthItem)->Pool = this;
    }

    /* QUIRK: the reclaim item is optional at run time, yet failing to create it fails the pool */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("DMA pool work item creation failed 0x%lx\n", Status);
        m_ReclaimItem = NULL;
        m_GrowthItem = NULL;
        m_Adapter = NULL;
        WdfObjectDelete(Enabler);
        return Status;
    }

    /* The enabler cleanup reads this object, so keep the UCXCONTROLLER context alive until then */
    if (m_Controller->m_Ucx != NULL)
    {
        WdfObjectReference(m_Controller->m_Ucx);
        m_HoldsUcxReference = TRUE;
    }

    m_Enabler = Enabler;
    XhciPoolOwner(Enabler)->Pool = this;

    /* Short initial lists are not fatal; acquire grows them on demand */
    AddSegments(&m_Large, 4);
    AddSegments(&m_Small, 8);

    DPRINT("DMA pool ready: max transfer 0x%lx, %lu map registers, %lu large, %lu small\n",
           m_MaxDmaSize, m_MapRegisters, m_Large.Total, m_Small.Total);
    return STATUS_SUCCESS;
}

VOID
XhciCommonBuffer::Release()
{
    /* Nothing to do: the pool survives ReleaseHardware and dies with the enabler */
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
XhciCommonBuffer::D0Exit()
{
    Flush();
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
XhciCommonBuffer::PostResetFlush()
{
    Flush();
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
XhciCommonBuffer::OnHostLost()
{
    Flush();
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
XhciCommonBuffer::Flush()
{
    LARGE_INTEGER Timeout;

    if (!m_Initialized)
        return;

    Timeout = XhciRelativeMs(60 * 1000);

    /* Keep waiting, but log each time the timeout expires */
    while (KeWaitForSingleObject(&m_GrowthIdle, Executive, KernelMode, FALSE, &Timeout) == STATUS_TIMEOUT)
        DPRINT1("DMA pool growth still running after 60 s, still waiting\n");

    if (m_ReclaimItem != NULL)
        WdfWorkItemFlush(m_ReclaimItem);
}

XhciCommonBuffer::SizeClass*
XhciCommonBuffer::ClassForSize(
    _In_ ULONG Size)
{
    if (Size <= XHCI_POOL_SMALL_BYTES)
        return &m_Small;
    if (Size <= XHCI_POOL_PAGE_BYTES)
        return &m_Large;
    return NULL;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
XhciCommonBuffer::AddSegments(
    _Inout_ SizeClass* Class,
    _In_ ULONG Count)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFCOMMONBUFFER CommonBuffer;
    PHYSICAL_ADDRESS Logical;
    PXHCI_POOL_PAGE Page;
    PUCHAR Virtual;
    ULONG PerPage;
    ULONG Pages;
    ULONG Index;
    NTSTATUS Status;

    if (m_Enabler == NULL || Count == 0)
        return;

    PerPage = XHCI_POOL_PAGE_BYTES / Class->SegmentSize;
    Pages = (Count + PerPage - 1) / PerPage;

    while (Pages-- != 0)
    {
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XHCI_POOL_PAGE);
        Attributes.ContextSizeOverride = FIELD_OFFSET(XHCI_POOL_PAGE, Segments) +
                                         PerPage * sizeof(XhciDmaBuffer);

        Status = WdfCommonBufferCreate(m_Enabler, XHCI_POOL_PAGE_BYTES, &Attributes, &CommonBuffer);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("WdfCommonBufferCreate failed 0x%lx, %lu byte class stays short\n",
                    Status, Class->SegmentSize);
            return;
        }

        Logical = WdfCommonBufferGetAlignedLogicalAddress(CommonBuffer);
        if ((Logical.QuadPart & (XHCI_POOL_PAGE_BYTES - 1)) != 0)
        {
            DPRINT1("Common buffer at 0x%I64x is not page aligned, dropped\n", Logical.QuadPart);
            WdfObjectDelete(CommonBuffer);
            continue;
        }

        Virtual = (PUCHAR)WdfCommonBufferGetAlignedVirtualAddress(CommonBuffer);
        Page = XhciPoolPageFromBuffer(CommonBuffer);
        Page->CommonBuffer = CommonBuffer;
        Page->SegmentCount = PerPage;

        /* A page nobody used yet is a reclaim candidate right away */
        Page->ReleaseQueued = TRUE;

        for (Index = 0; Index < PerPage; Index++)
        {
            XhciDmaBuffer* Segment = &Page->Segments[Index];

            Segment->VirtualAddress = Virtual + Index * Class->SegmentSize;
            Segment->LogicalAddress.QuadPart = Logical.QuadPart + Index * Class->SegmentSize;
            Segment->Size = Class->SegmentSize;
            Segment->Page = Page;
            Segment->Owned = FALSE;
        }

        XhciSpinLockGuard Guard(&m_Lock);

        m_PagesTotal++;
        InsertTailList(&Class->PageList, &Page->Link);
        for (Index = 0; Index < PerPage; Index++)
            InsertTailList(&Class->FreeList, &Page->Segments[Index].Link);
        Class->Total += PerPage;
        Class->Available += PerPage;
    }
}

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
XhciCommonBuffer::Grab(
    _Inout_ SizeClass* Class,
    _In_ ULONG Count,
    _Out_ PLIST_ENTRY Taken,
    _Out_ PBOOLEAN NeedGrow)
{
    PLIST_ENTRY Entry;
    BOOLEAN Got = FALSE;

    InitializeListHead(Taken);

    {
        XhciSpinLockGuard Guard(&m_Lock);

        if (Class->Available >= Count)
        {
            ULONG Left;

            for (Left = Count; Left != 0; Left--)
            {
                XhciDmaBuffer* Segment;
                PXHCI_POOL_PAGE Page;

                Segment = CONTAINING_RECORD(RemoveHeadList(&Class->FreeList), XhciDmaBuffer, Link);
                Segment->Owned = TRUE;

                Page = (PXHCI_POOL_PAGE)Segment->Page;
                if (Page->ReleaseQueued)
                {
                    Page->ReleaseQueued = FALSE;
                    m_PagesInUse++;
                }

                InsertTailList(Taken, &Segment->Link);
            }

            Class->Available -= Count;
            Got = TRUE;
        }

        *NeedGrow = (Class->Available < Class->GrowBelow);
    }

    /* Nobody else can reach these segments now, so zero them outside the lock */
    for (Entry = Taken->Flink; Entry != Taken; Entry = Entry->Flink)
    {
        XhciDmaBuffer* Segment = CONTAINING_RECORD(Entry, XhciDmaBuffer, Link);

        RtlZeroMemory(Segment->VirtualAddress, Segment->Size);
    }

    return Got;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
XhciDmaBuffer*
XhciCommonBuffer::Acquire(
    _In_ ULONG Size)
{
    XhciDmaBuffer* Buffer;
    LIST_ENTRY Taken;
    SizeClass* Class;
    BOOLEAN NeedGrow;
    BOOLEAN Got;

    Class = ClassForSize(Size);
    if (Class == NULL)
    {
        DPRINT1("Common buffer of %lu bytes is larger than a page\n", Size);
        return NULL;
    }

    Got = Grab(Class, 1, &Taken, &NeedGrow);
    if (!Got || NeedGrow)
    {
        if (KeGetCurrentIrql() == PASSIVE_LEVEL)
        {
            AddSegments(Class, (Got ? 0 : 1) + (NeedGrow ? Class->GrowBy : 0));
            if (!Got)
                Got = Grab(Class, 1, &Taken, &NeedGrow);
        }
        else
        {
            QueueGrowth();
        }
    }

    if (!Got)
    {
        DPRINT1("No free %lu byte common buffer\n", Class->SegmentSize);
        return NULL;
    }

    Buffer = CONTAINING_RECORD(RemoveHeadList(&Taken), XhciDmaBuffer, Link);
    XhciClearListEntry(&Buffer->Link);
    return Buffer;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
XhciCommonBuffer::AcquireList(
    _In_ ULONG Size,
    _In_ ULONG Count,
    _Inout_ PLIST_ENTRY List)
{
    LIST_ENTRY Taken;
    SizeClass* Class;
    BOOLEAN NeedGrow;
    BOOLEAN Got;

    Class = ClassForSize(Size);
    if (Class == NULL)
    {
        DPRINT1("Common buffer list of %lu bytes each is larger than a page\n", Size);
        return STATUS_INVALID_PARAMETER;
    }

    if (Count == 0)
        return STATUS_SUCCESS;

    Got = Grab(Class, Count, &Taken, &NeedGrow);
    if (!Got || NeedGrow)
    {
        if (KeGetCurrentIrql() == PASSIVE_LEVEL)
        {
            AddSegments(Class, (Got ? 0 : Count) + (NeedGrow ? Class->GrowBy : 0));
            if (!Got)
                Got = Grab(Class, Count, &Taken, &NeedGrow);
        }
        else
        {
            QueueGrowth();
        }
    }

    if (!Got)
    {
        DPRINT1("Not enough free %lu byte common buffers for %lu\n", Class->SegmentSize, Count);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    while (!IsListEmpty(&Taken))
        InsertTailList(List, RemoveHeadList(&Taken));

    return STATUS_SUCCESS;
}

_Requires_lock_held_(m_Lock)
BOOLEAN
XhciCommonBuffer::ReturnFree(
    _Inout_ XhciDmaBuffer* Buffer)
{
    SizeClass* Class;

    if (Buffer->Size == XHCI_POOL_SMALL_BYTES)
        Class = &m_Small;
    else if (Buffer->Size == XHCI_POOL_PAGE_BYTES)
        Class = &m_Large;
    else
        return FALSE;

    if (!Buffer->Owned)
    {
        DPRINT1("Common buffer %p freed twice\n", Buffer);
        return TRUE;
    }

    /* The most recently used segment goes first so it is reused while still cached */
    Buffer->Owned = FALSE;
    InsertHeadList(&Class->FreeList, &Buffer->Link);
    Class->Available++;
    return TRUE;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XhciCommonBuffer::Free(
    _In_ XhciDmaBuffer* Buffer)
{
    XhciSpinLockGuard Guard(&m_Lock);

    if (!ReturnFree(Buffer))
        DPRINT1("Common buffer %p has bad size %lu, dropped\n", Buffer, Buffer->Size);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XhciCommonBuffer::FreeList(
    _Inout_ PLIST_ENTRY List)
{
    XhciSpinLockGuard Guard(&m_Lock);

    while (!IsListEmpty(List))
    {
        XhciDmaBuffer* Buffer = CONTAINING_RECORD(RemoveHeadList(List), XhciDmaBuffer, Link);

        if (!ReturnFree(Buffer))
            DPRINT1("Common buffer %p in list has bad size %lu, dropped\n", Buffer, Buffer->Size);
    }
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XhciCommonBuffer::QueueGrowth()
{
    BOOLEAN Enqueue = FALSE;

    if (m_GrowthItem == NULL)
        return;

    {
        XhciSpinLockGuard Guard(&m_Lock);

        /* Overlapping growth items share one idle event, so count them */
        if (!m_GrowthQueued)
        {
            m_GrowthQueued = TRUE;
            m_GrowthOutstanding++;
            KeClearEvent(&m_GrowthIdle);
            Enqueue = TRUE;
        }
    }

    if (Enqueue)
        WdfWorkItemEnqueue(m_GrowthItem);
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
XhciCommonBuffer::GrowthWork()
{
    ULONG SmallCount;
    ULONG LargeCount;

    {
        XhciSpinLockGuard Guard(&m_Lock);

        /* From here on a new request has to queue the item again */
        m_GrowthQueued = FALSE;
        SmallCount = (m_Small.Available < m_Small.GrowBelow) ? m_Small.GrowBy : 0;
        LargeCount = (m_Large.Available < m_Large.GrowBelow) ? m_Large.GrowBy : 0;
    }

    AddSegments(&m_Small, SmallCount);
    AddSegments(&m_Large, LargeCount);

    XhciSpinLockGuard Guard(&m_Lock);

    m_GrowthOutstanding--;
    if (m_GrowthOutstanding == 0)
        KeSetEvent(&m_GrowthIdle, IO_NO_INCREMENT, FALSE);
}

_Requires_lock_held_(m_Lock)
VOID
XhciCommonBuffer::ShrinkClass(
    _Inout_ SizeClass* Class)
{
    PLIST_ENTRY Entry = Class->PageList.Flink;

    while (Entry != &Class->PageList && Class->Available > Class->KeepAtLeast)
    {
        PXHCI_POOL_PAGE Page = CONTAINING_RECORD(Entry, XHCI_POOL_PAGE, Link);
        ULONG Index;

        Entry = Entry->Flink;

        if (Page->ReleaseQueued)
        {
            /* Unused since the last tick: every segment is on the free list */
            for (Index = 0; Index < Page->SegmentCount; Index++)
                RemoveEntryList(&Page->Segments[Index].Link);

            Class->Available -= Page->SegmentCount;
            Class->Total -= Page->SegmentCount;
            RemoveEntryList(&Page->Link);
            InsertTailList(&m_ReclaimList, &Page->Link);
            m_ReclaimPages++;
            continue;
        }

        for (Index = 0; Index < Page->SegmentCount; Index++)
        {
            if (Page->Segments[Index].Owned)
                break;
        }

        /* Fully free now: freed on the next tick unless somebody takes a segment first */
        if (Index == Page->SegmentCount)
        {
            Page->ReleaseQueued = TRUE;
            m_PagesInUse--;
        }
    }
}

VOID
XhciCommonBuffer::Rebalance()
{
    BOOLEAN Reclaim;

    if (m_ReclaimItem == NULL)
        return;

    {
        XhciSpinLockGuard Guard(&m_Lock);

        ShrinkClass(&m_Small);
        ShrinkClass(&m_Large);
        Reclaim = !IsListEmpty(&m_ReclaimList);
    }

    if (Reclaim)
        WdfWorkItemEnqueue(m_ReclaimItem);
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
XhciCommonBuffer::ReclaimWork()
{
    LIST_ENTRY Pages;

    InitializeListHead(&Pages);

    {
        XhciSpinLockGuard Guard(&m_Lock);

        while (!IsListEmpty(&m_ReclaimList))
            InsertTailList(&Pages, RemoveHeadList(&m_ReclaimList));

        m_PagesTotal -= m_ReclaimPages;
        m_ReclaimPages = 0;
    }

    while (!IsListEmpty(&Pages))
    {
        PXHCI_POOL_PAGE Page = CONTAINING_RECORD(RemoveHeadList(&Pages), XHCI_POOL_PAGE, Link);

        /* The header is the buffer's context, so it goes away with the buffer */
        WdfObjectDelete(Page->CommonBuffer);
    }
}

VOID
XhciCommonBuffer::EnablerCleanup()
{
    if (m_Small.Total != m_Small.Available || m_Large.Total != m_Large.Available)
    {
        DPRINT1("DMA pool torn down with buffers still owned: small %lu of %lu, large %lu of %lu\n",
                m_Small.Total - m_Small.Available, m_Small.Total,
                m_Large.Total - m_Large.Available, m_Large.Total);
        ASSERT(FALSE);
    }

    /* KMDF frees the pages and work items as children of the enabler */
    m_Enabler = NULL;
    m_Adapter = NULL;
    m_ReclaimItem = NULL;
    m_GrowthItem = NULL;
    XhciPoolInitClass(&m_Small.FreeList, &m_Small.PageList);
    XhciPoolInitClass(&m_Large.FreeList, &m_Large.PageList);
    InitializeListHead(&m_ReclaimList);
    m_Small.Total = m_Small.Available = 0;
    m_Large.Total = m_Large.Available = 0;

    /* Last touch of this object; the controller context may go away right after */
    if (m_HoldsUcxReference)
    {
        m_HoldsUcxReference = FALSE;
        WdfObjectDereference(m_Controller->m_Ucx);
    }
}

WDFDMAENABLER
XhciCommonBuffer::DmaEnabler() const
{
    return m_Enabler;
}

PDMA_ADAPTER
XhciCommonBuffer::DmaAdapter() const
{
    return m_Adapter;
}

ULONG
XhciCommonBuffer::MaxDmaSize() const
{
    return m_MaxDmaSize;
}

ULONG
XhciCommonBuffer::MapRegisterCount() const
{
    return m_MapRegisters;
}

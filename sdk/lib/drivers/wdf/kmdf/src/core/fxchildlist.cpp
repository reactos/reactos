/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    FxChildList.cpp

Abstract:

    This module implements the FxChildList class

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#include "fxcorepch.hpp"

extern "C" {
// #include "FxChildList.tmh"
}

FxDeviceDescriptionEntry::FxDeviceDescriptionEntry(
    __inout FxChildList* DeviceList,
    __in ULONG AddressDescriptionSize,
    __in ULONG IdentificationDescriptionSize
    )
{
    m_IdentificationDescription =
        (PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER) WDF_PTR_ADD_OFFSET(
        this, WDF_ALIGN_SIZE_UP(sizeof(*this), sizeof(PVOID)));

    m_IdentificationDescription->IdentificationDescriptionSize =
        AddressDescriptionSize;

    if (IdentificationDescriptionSize > 0) {
        m_AddressDescription =
            (PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER) WDF_PTR_ADD_OFFSET(
                m_IdentificationDescription,
                WDF_ALIGN_SIZE_UP(AddressDescriptionSize, sizeof(PVOID)));

        m_AddressDescription->AddressDescriptionSize =
            IdentificationDescriptionSize;
    }

    InitializeListHead(&m_DescriptionLink);
    InitializeListHead(&m_ModificationLink);

    m_ModificationState = ModificationInsert;

    m_DeviceList = DeviceList;

    m_FoundInLastScan = FALSE;
    m_ProcessingSurpriseRemove = FALSE;
    m_PendingDeleteOnScanEnd = FALSE;

    //
    // The parent DO can go away while the child still exists (stuck in a
    // suprise remove state w/an open handle).  As such, when the parent is
    // destroyed, it will release its reference on the FxChildList.  Each
    // description will have its own reference to the list to keep the list
    // alive as long as the child DO exists.
    //
    m_DeviceList->ADDREF(this);
}

FxDeviceDescriptionEntry::~FxDeviceDescriptionEntry()
{
    m_DeviceList->RELEASE(this);
}

_Must_inspect_result_
PVOID
FxDeviceDescriptionEntry::operator new(
    __in size_t AllocatorBlock,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in size_t TotalDescriptionSize
    )
{
    PVOID p;

    UNREFERENCED_PARAMETER(AllocatorBlock);

    p = FxPoolAllocate(FxDriverGlobals, NonPagedPool, TotalDescriptionSize);

    if (p != NULL) {
        RtlZeroMemory(p, TotalDescriptionSize);
    }

    return p;
}

VOID
FxDeviceDescriptionEntry::DeviceSurpriseRemoved(
    VOID
    )
{
    KIRQL irql;

    KeAcquireSpinLock(&m_DeviceList->m_ListLock, &irql);
    m_ProcessingSurpriseRemove = TRUE;
    KeReleaseSpinLock(&m_DeviceList->m_ListLock, irql);
}

BOOLEAN
FxDeviceDescriptionEntry::IsDeviceReportedMissing(
    VOID
    )
/*++

Routine Description:
    This function tells the caller if the description has been reported missing
    to the Pnp manager.  It does not change the actual state of the description,
    unlike IsDeviceRemoved().

Arguments:
    None

Return Value:
    TRUE if it has been reported missing, FALSE otherwise

  --*/
{
    KIRQL irql;
    BOOLEAN result;

    KeAcquireSpinLock(&m_DeviceList->m_ListLock, &irql);
    if (m_DescriptionState == DescriptionReportedMissing) {
        result = TRUE;
    }
    else {
        result = FALSE;
    }
    KeReleaseSpinLock(&m_DeviceList->m_ListLock, irql);

    return result;
}

BOOLEAN
FxDeviceDescriptionEntry::IsDeviceRemoved(
    VOID
    )
{
    KIRQL irql;
    BOOLEAN removed;
    FxChildList* pList;

    pList = GetParentList();
    removed = FALSE;
    FxVerifierCheckIrqlLevel(m_DeviceList->GetDriverGlobals(), PASSIVE_LEVEL);

    KeAcquireSpinLock(&m_DeviceList->m_ListLock, &irql);

    m_ProcessingSurpriseRemove = FALSE;

    if (m_DescriptionState == DescriptionReportedMissing) {
        //
        // We should delete this entry as it was reported missing.
        //
        ASSERT(m_ModificationState == ModificationUnspecified);

        m_DescriptionState = DescriptionUnspecified;

        //
        // Remove from the current list if no scan going on.
        // Note that the description entry can't be removed from list if scan
        // count is > 0 because it might be part of an iterator that driver is
        // still using to iterate thru the child list.
        //
        if (pList->GetScanCount() == 0) {
            //
            // Remove from the current list
            //
            RemoveEntryList(&m_DescriptionLink);
            InitializeListHead(&m_DescriptionLink);
        }
        else {
            //
            // The entry will be removed and deleted when scan count goes to
            // zero by the scanning thread, so make sure pdo deosn't reference
            // the entry any more.
            //
            m_PendingDeleteOnScanEnd = TRUE;
            if (m_Pdo != NULL) {
                m_Pdo->GetPdoPkg()->m_Description = NULL;
            }
        }

        removed = TRUE;

        //
        // No need to add a reference to m_DeviceList becuase as we hold a
        // reference to it already.  This reference is bound by the lifetime
        // of this object, so we can always safely touch m_DeviceList even if
        // the FDO is gone before the child is removed.
        //
    }

    KeReleaseSpinLock(&m_DeviceList->m_ListLock, irql);

    return removed;
}

VOID
FxDeviceDescriptionEntry::ProcessDeviceRemoved(
    VOID
    )
{
    LIST_ENTRY freeHead;
    KIRQL irql;
    FxChildList* pList;

    FxVerifierCheckIrqlLevel(m_DeviceList->GetDriverGlobals(), PASSIVE_LEVEL);

    pList = GetParentList();
    InitializeListHead(&freeHead);
    KeAcquireSpinLock(&m_DeviceList->m_ListLock, &irql);

    //
    // Remove from the current list. In some cases the entry may not be in any
    // list in which case RemoveEntryList() will be a noop.
    // Note that the description entry can't be removed from list if scan count
    // is > 0 because it might be part of an iterator that driver is still using
    // to iterate thru the child list.
    //
    if (pList->GetScanCount() == 0 || IsListEmpty(&m_DescriptionLink)) {
        RemoveEntryList(&m_DescriptionLink);

        //
        // Instead of reimplementing a single description cleanup, just use the
        // version which cleans up a list.
        //
        InsertTailList(&freeHead, &m_DescriptionLink);
    }
    else {
        //
        // The entry will be removed when scan count goes to zero.
        //
        ASSERT(m_ModificationState == ModificationUnspecified &&
                m_DescriptionState == DescriptionUnspecified);
        m_PendingDeleteOnScanEnd = TRUE;
    }

    KeReleaseSpinLock(&m_DeviceList->m_ListLock, irql);

    m_DeviceList->DrainFreeListHead(&freeHead);
}

_Must_inspect_result_
FxDeviceDescriptionEntry*
FxDeviceDescriptionEntry::Clone(
    __inout PLIST_ENTRY FreeListHead
    )
{
    FxDeviceDescriptionEntry* pNewEntry;
    NTSTATUS status;

    pNewEntry = new(m_DeviceList->GetDriverGlobals(),
                    m_DeviceList->m_TotalDescriptionSize)
        FxDeviceDescriptionEntry(m_DeviceList,
                                 m_DeviceList->m_IdentificationDescriptionSize,
                                 m_DeviceList->m_AddressDescriptionSize);

    if (pNewEntry == NULL) {
        return NULL;
    }

    status = m_DeviceList->DuplicateId(pNewEntry->m_IdentificationDescription,
                                       m_IdentificationDescription);

    if (NT_SUCCESS(status) && m_DeviceList->HasAddressDescriptions()) {
        status = m_DeviceList->DuplicateAddress(pNewEntry->m_AddressDescription,
                                                m_AddressDescription);
    }

    if (NT_SUCCESS(status)) {
        pNewEntry->m_FoundInLastScan = TRUE;
    }
    else {
        //
        // Free it later
        //
        InsertTailList(FreeListHead, &pNewEntry->m_DescriptionLink);
        pNewEntry = NULL;
    }

    return pNewEntry;
}

FxChildList::FxChildList(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in size_t TotalDescriptionSize,
    __in FxDevice* Device,
    __in BOOLEAN Static
    ) :
    FxNonPagedObject(FX_TYPE_CHILD_LIST,sizeof(FxChildList), FxDriverGlobals),
    m_TotalDescriptionSize(TotalDescriptionSize),
    m_EvtCreateDevice(FxDriverGlobals),
    m_EvtScanForChildren(FxDriverGlobals)
{
    //
    // Transaction link into list of FxChildList pointers maintained by
    // FxDevice's pnp package.
    //
    m_TransactionLink.SetTransactionedObject(this);

    m_Device = Device;

    m_IdentificationDescriptionSize = 0;
    m_AddressDescriptionSize = 0;

    m_EvtIdentificationDescriptionDuplicate = NULL;
    m_EvtIdentificationDescriptionCopy = NULL;
    m_EvtIdentificationDescriptionCleanup = NULL;
    m_EvtIdentificationDescriptionCompare = NULL;

    m_EvtAddressDescriptionDuplicate = NULL;
    m_EvtAddressDescriptionCopy = NULL;
    m_EvtAddressDescriptionCleanup = NULL;

    KeInitializeSpinLock(&m_ListLock);
    InitializeListHead(&m_DescriptionListHead);
    InitializeListHead(&m_ModificationListHead);

    m_State = ListUnlocked;

    m_InvalidationNeeded = FALSE;
    m_StaticList = Static;
    m_IsAdded = FALSE;

    m_EnumRetries = 0;

    m_ScanTag = NULL;
    m_ScanCount = 0;

    //
    // We want all waiters on the event to be satisfied, not just the first one
    //
    m_ScanEvent.Initialize(NotificationEvent, TRUE);

    MarkDisposeOverride(ObjectDoNotLock);
}

BOOLEAN
FxChildList::Dispose(
    VOID
    )
{
    if (m_IsAdded) {
        ASSERT(m_Device != NULL);
        m_Device->RemoveChildList(this);
    }

    return TRUE;
}

_Must_inspect_result_
NTSTATUS
FxChildList::_CreateAndInit(
    __out FxChildList** ChildList,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_OBJECT_ATTRIBUTES ListAttributes,
    __in size_t TotalDescriptionSize,
    __in FxDevice* Device,
    __in PWDF_CHILD_LIST_CONFIG ListConfig,
    __in BOOLEAN Static
    )
{
    NTSTATUS ntStatus;
    FxChildList *childList = NULL;

    //
    // Initialize
    //
    *ChildList = NULL;

    //
    // Allocate a new child list object
    //
    childList = new(FxDriverGlobals, ListAttributes)
        FxChildList(FxDriverGlobals, TotalDescriptionSize, Device, Static);
    if (childList == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "No memory for WDFCHILDLIST handle, %!STATUS!",
                            ntStatus);
        goto exit;
    }

    //
    // Initialize the child list object
    //
    childList->Initialize(ListConfig);

    *ChildList = childList;
    ntStatus = STATUS_SUCCESS;

exit:
    if (!NT_SUCCESS(ntStatus)) {
        if (NULL != childList) {
            childList->DeleteFromFailedCreate();
        }
    }
    return ntStatus;
}

_Must_inspect_result_
VOID
FxChildList::Initialize(
    __in PWDF_CHILD_LIST_CONFIG Config
    )
{
    //
    // Driver cannot call WdfObjectDelete on this handle
    //
    MarkNoDeleteDDI();

    m_IdentificationDescriptionSize = Config->IdentificationDescriptionSize;
    m_AddressDescriptionSize = Config->AddressDescriptionSize;

    m_EvtCreateDevice.m_Method = Config->EvtChildListCreateDevice;
    m_EvtScanForChildren.m_Method = Config->EvtChildListScanForChildren;

    m_EvtIdentificationDescriptionDuplicate = Config->EvtChildListIdentificationDescriptionDuplicate;
    m_EvtIdentificationDescriptionCopy = Config->EvtChildListIdentificationDescriptionCopy;
    m_EvtIdentificationDescriptionCleanup = Config->EvtChildListIdentificationDescriptionCleanup;
    m_EvtIdentificationDescriptionCompare = Config->EvtChildListIdentificationDescriptionCompare;

    m_EvtAddressDescriptionDuplicate = Config->EvtChildListAddressDescriptionDuplicate;
    m_EvtAddressDescriptionCopy = Config->EvtChildListAddressDescriptionCopy;
    m_EvtAddressDescriptionCleanup = Config->EvtChildListAddressDescriptionCleanup;

    m_EvtChildListDeviceReenumerated = Config->EvtChildListDeviceReenumerated;

    //
    // Add this ChildList to the parent device's list of children lists.
    //
    m_Device->AddChildList(this);
    m_IsAdded = TRUE;
}

WDFDEVICE
FxChildList::GetDevice(
    VOID
    )
{
    return m_Device->GetHandle();
}

FxDevice*
FxChildList::GetDeviceFromId(
    __inout PWDF_CHILD_RETRIEVE_INFO Info
    )
{
    FxDeviceDescriptionEntry* pEntry;
    FxDevice* device;
    WDF_CHILD_LIST_RETRIEVE_DEVICE_STATUS status;
    KIRQL irql;

    device = NULL;

    KeAcquireSpinLock(&m_ListLock, &irql);

    pEntry = SearchBackwardsForMatchingModificationLocked(
        Info->IdentificationDescription);

    if (pEntry != NULL && pEntry->m_ModificationState == ModificationInsert) {
        status = WdfChildListRetrieveDeviceNotYetCreated;
    }
    else {
        pEntry = SearchBackwardsForMatchingDescriptionLocked(
            Info->IdentificationDescription);

        if (pEntry != NULL) {
            if (pEntry->m_DescriptionState == DescriptionPresentNeedsInstantiation) {
                status = WdfChildListRetrieveDeviceNotYetCreated;
            }
            else {
                status = WdfChildListRetrieveDeviceSuccess;
                device = pEntry->m_Pdo;
            }
        }
        else {
            status = WdfChildListRetrieveDeviceNoSuchDevice;
        }
    }

    if (pEntry != NULL && Info->AddressDescription != NULL) {
        CopyAddress(Info->AddressDescription,
                    pEntry->m_AddressDescription);
    }

    KeReleaseSpinLock(&m_ListLock, irql);

    Info->Status = status;

    return device;
}

_Must_inspect_result_
NTSTATUS
FxChildList::GetAddressDescription(
    __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    __out PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    FxDeviceDescriptionEntry* pEntry;
    NTSTATUS status;
    KIRQL irql;

    KeAcquireSpinLock(&m_ListLock, &irql);

    pEntry = SearchBackwardsForMatchingModificationLocked(
        IdentificationDescription);

    if (pEntry == NULL) {
        pEntry = SearchBackwardsForMatchingDescriptionLocked(
            IdentificationDescription);
    }

    if (pEntry != NULL) {
        CopyAddress(AddressDescription, pEntry->m_AddressDescription);
        status = STATUS_SUCCESS;
    }
    else {
        status = STATUS_NO_SUCH_DEVICE;
    }

    KeReleaseSpinLock(&m_ListLock, irql);

    return status;
}

VOID
FxChildList::GetAddressDescriptionFromEntry(
    __in FxDeviceDescriptionEntry* Entry,
    __out PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    KIRQL irql;

    KeAcquireSpinLock(&m_ListLock, &irql);
    CopyAddress(AddressDescription, Entry->m_AddressDescription);
    KeReleaseSpinLock(&m_ListLock, irql);
}

VOID
FxChildList::BeginScan(
    __out_opt PULONG ScanTag
    )
{
    FxDeviceDescriptionEntry* pEntry;
    PLIST_ENTRY ple;
    KIRQL irql;

    KeAcquireSpinLock(&m_ListLock, &irql);

    // KeClearEvent is faster than KeResetEvent for putting event to not-signaled state
    m_ScanEvent.Clear();
    m_ScanCount++;

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "Begin scan on WDFCHILDLIST %p, scan count %d",
        GetHandle(), m_ScanCount);

    if (ScanTag != NULL) {
        if (m_ScanTag != NULL) {
            *m_ScanTag = ScanTagCancelled;
        }

        *ScanTag = ScanTagActive;
        m_ScanTag = ScanTag;
    }

    //
    // Start by walking over all entries and setting their FoundInLastScan
    // fields to FALSE. We need to set both present items in the Desc list
    // and inserts in the Mod list. It's actually easier just to walk over
    // everything.
    //
    for (ple = m_DescriptionListHead.Flink;
         ple != &m_DescriptionListHead;
         ple = ple->Flink) {
        pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);
        pEntry->m_FoundInLastScan = FALSE;
    }

    for (ple = m_ModificationListHead.Flink;
         ple != &m_ModificationListHead;
         ple = ple->Flink) {
        pEntry = FxDeviceDescriptionEntry::_FromModificationLink(ple);
        pEntry->m_FoundInLastScan = FALSE;
    }

    KeReleaseSpinLock(&m_ListLock, irql);
}

VOID
FxChildList::EndScan(
    __inout_opt PULONG ScanTag
    )
{
    PLIST_ENTRY pCur, pNext;
    LIST_ENTRY freeHead;
    FxDeviceDescriptionEntry* pEntry;
    KIRQL irql;

    InitializeListHead(&freeHead);

    KeAcquireSpinLock(&m_ListLock, &irql);

    if (ScanTag != NULL) {
        if (*ScanTag != ScanTagActive) {
            ASSERT(*ScanTag == ScanTagCancelled);
            KeReleaseSpinLock(&m_ListLock, irql);
            return;
        }

        *ScanTag = ScanTagFinished;
        m_ScanTag = NULL;
    }

    m_ScanCount--;

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "End scan on WDFCHILDLIST %p, scan count %d",
        GetHandle(), m_ScanCount);

    if (m_ScanCount == 0) {
        //
        // Walk over all modification entries and remove any pending insert that
        // wasn't seen in the scan. Note that MarkDescriptionNotPresentWorker will
        // enqueue the entry to be freed later.
        //
        // Also, convert any clones of devices not reported as present to
        // removal modifications so that the original is still reported missing.
        //
        for (pCur = m_ModificationListHead.Flink;
             pCur != &m_ModificationListHead;
             pCur = pNext) {

            pNext = pCur->Flink;

            pEntry = FxDeviceDescriptionEntry::_FromModificationLink(pCur);

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "entry %p modified in last scan, "
                "mod state  %!FxChildListModificationState!,"
                "desc state %!FxChildListDescriptionState!",
                pEntry, pEntry->m_ModificationState,
                pEntry->m_DescriptionState);

            if (pEntry->m_FoundInLastScan == FALSE) {
                switch (pEntry->m_ModificationState) {
                case ModificationInsert:
                    //
                    // Insertion that never made through to the end of a scan.
                    //
                    MarkModificationNotPresentWorker(&freeHead, pEntry);
                    break;

                case ModificationClone:
                    DoTraceLevelMessage(
                        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "clone of PDO WDFDEVICE %p, !devobj %p dropped because "
                        "it is missing", pEntry->m_Pdo->GetHandle(),
                        pEntry->m_Pdo->GetDeviceObject());

                    //
                    // We were asked for a clone in the middle of a scan and the
                    // device was not reported, so it is now missing.  Convert
                    // the clone modification to a remove modification.
                    //
                    pEntry->m_ModificationState = ModificationRemoveNotify;

                    //
                    // Remove the modification from the current mod list.  It
                    // will be re-added below when we iterate over the
                    // descriptions list.
                    //
                    RemoveEntryList(&pEntry->m_ModificationLink);
                    InitializeListHead(&pEntry->m_ModificationLink);

                    //
                    // We can only have been cloning a device which is present
                    // and not yet reported missing to the OS.
                    //
                    ASSERT(pEntry->m_DescriptionState ==
                              DescriptionInstantiatedHasObject);
                    break;
                }
            }
        }

        //
        // Walk over all desc entries and remove anything that wasn't found in the
        // scan. Since MarkDescriptionNotPresentWorker doesn't drain the
        // modifications, we don't have to worry about any entries disappearing.
        //
        for (pCur = m_DescriptionListHead.Flink;
             pCur != &m_DescriptionListHead;
             pCur = pCur->Flink) {

            pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(pCur);

            if (pEntry->m_PendingDeleteOnScanEnd) {
                //
                // The entry was pnp removed but was not removed from list
                // because scan count was > 0. It is safe to remove and delete
                // it now.

                // Update the current pointer before the entry is removed.
                pCur = pCur->Blink;

                RemoveEntryList(&pEntry->m_DescriptionLink);
                InsertTailList(&freeHead, &pEntry->m_DescriptionLink);
                pEntry = NULL;
                continue;
            }
            else if (pEntry->IsPresent() && pEntry->m_FoundInLastScan == FALSE) {
                if (pEntry->m_Pdo != NULL) {
                    DoTraceLevelMessage(
                        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "marking PDO WDFDEVICE %p, !devobj %p as not present",
                        pEntry->m_Pdo->GetHandle(),
                        pEntry->m_Pdo->GetDeviceObject());

                }
                else {
                    DoTraceLevelMessage(
                        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "marking PDO (entry %p) which needs instantiation as "
                        "not present", pEntry);
                }

                MarkDescriptionNotPresentWorker(pEntry, TRUE);
            }
        }

        //
        // Attempt to drain any mods we made.
        //
        ProcessModificationsLocked(&freeHead);

        //
        // We need to check m_KnownPdo here because if this
        // object is attached to a PDO, the PDO's devobj could be in a state where
        // PnP does not know it is a PDO yet.  m_KnownPdo
        // is set when the PDO receives a start device and by the time start
        // device arrives, we definitely know the PDO is known the PnP.
        //
        // No need to hold a lock while checking m_KnownPdo
        // because if miss the transition from FALSE to TRUE, relations will
        // automaticaly be invalidated by the cause of that transition (start
        // device arrival).  PnP automatically sends a QDR after a successful
        // start.
        //

        if (m_InvalidationNeeded) {
            PDEVICE_OBJECT pdo;

            //
            // This does the m_KnownPdo check.
            //
            pdo = m_Device->GetSafePhysicalDevice();

            if (pdo != NULL) {
                m_InvalidationNeeded = FALSE;
                IoInvalidateDeviceRelations(pdo, BusRelations);
            }
        }

        m_ScanEvent.Set();
    }

    KeReleaseSpinLock(&m_ListLock, irql);

    //
    // Free structures outside of any internal WDF lock
    //
    DrainFreeListHead(&freeHead);
}

VOID
FxChildList::CancelScan(
    __in BOOLEAN EndTheScan,
    __inout_opt PULONG ScanTag
    )
{
    *ScanTag = ScanTagCancelled;

    if (EndTheScan) {
        EndScan(ScanTag);
    }
}

VOID
FxChildList::InitIterator(
    __inout PWDF_CHILD_LIST_ITERATOR Iterator
    )
{
    Iterator->Reserved[DescriptionIndex] = &m_DescriptionListHead;

    if (Iterator->Flags & WdfRetrievePendingChildren) {
        Iterator->Reserved[ModificationIndex] = ULongToPtr(1);
    }
}

VOID
FxChildList::BeginIteration(
    __inout PWDF_CHILD_LIST_ITERATOR Iterator
    )
{
    KIRQL irql;

    //
    // Almost the same semantic as BeginScan except we don't mark all of the
    // children as missing when we start.
    //
    KeAcquireSpinLock(&m_ListLock, &irql);

    InitIterator(Iterator);
    //
    // Set the scanevent to non-signaled state. Some code paths such as
    // PDO eject will wait for the completion of child list iteration or scan
    // so that a QDR that follows eject is guaranteed to pick up the change in
    // PDO state that the code path made. Note that scan and iteration can be
    // nested and in that case this event will be clear each time but it will be
    // set (signalled) only after all the iteration and scan opeartions have
    // completed.
    //
    m_ScanEvent.Clear();
    m_ScanCount++;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Begin iteration on WDFCHILDLIST %p, scan count %d",
                        GetHandle(), m_ScanCount);

    KeReleaseSpinLock(&m_ListLock, irql);
}

VOID
FxChildList::EndIteration(
    __inout PWDF_CHILD_LIST_ITERATOR Iterator
    )
{
    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "end iteration on WDFCHILDLIST");

    //
    // Semantic is exactly the same as EndScan.  Basically, all changes are
    // delayed until there are no more active scans, so the end of an interation
    // is the same as the end of a scan.
    //
    EndScan();

    RtlZeroMemory(&Iterator->Reserved[0], sizeof(Iterator->Reserved));
}

_Must_inspect_result_
NTSTATUS
FxChildList::GetNextDevice(
    __out WDFDEVICE* Device,
    __inout PWDF_CHILD_LIST_ITERATOR Iterator,
    __inout_opt PWDF_CHILD_RETRIEVE_INFO Info
    )
{
    FxDeviceDescriptionEntry* pEntry;
    PLIST_ENTRY ple;
    NTSTATUS status;
    WDF_CHILD_LIST_RETRIEVE_DEVICE_STATUS dstatus = WdfChildListRetrieveDeviceNoSuchDevice;
    ULONG cur, i;
    KIRQL irql;
    BOOLEAN found;

    pEntry = NULL;
    status = STATUS_NO_MORE_ENTRIES;

    KeAcquireSpinLock(&m_ListLock, &irql);

    ASSERT(m_ScanCount > 0);
    if (m_ScanCount == 0) {
        status = STATUS_INVALID_DEVICE_STATE;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFCHILDLIST %p cannot retrieve next device if the list is not "
            "locked for iteration, %!STATUS!", GetHandle(), status);

        goto Done;
    }

    ple = (PLIST_ENTRY) Iterator->Reserved[DescriptionIndex];

    if (ple != NULL) {
        if (GetDriverGlobals()->FxVerifierOn) {
            status = VerifyDescriptionEntry(ple);
            if (!NT_SUCCESS(status)) {
                goto Done;
            }
        }

        found = FALSE;

        //
        // Try to find the next entry
        //
        for (ple = ple->Flink; ple != &m_DescriptionListHead; ple = ple->Flink) {
            pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

            if (pEntry->MatchStateToFlags(Iterator->Flags)) {
                //
                // An item was found
                //
                found = TRUE;

                //
                // Call the caller's compare function to further refine the match
                // if necessary.
                //
                if (Info != NULL &&
                    Info->EvtChildListIdentificationDescriptionCompare != NULL) {

                    found = Info->EvtChildListIdentificationDescriptionCompare(
                        GetHandle(),
                        Info->IdentificationDescription,
                        pEntry->m_IdentificationDescription);
                }

                if (found) {
                    break;
                }
            }
        }

        if (found) {
            //
            // We can safely store the description entry because we are
            // guaranteed it will stay in the description list until m_ScanCount
            // reaches zero.  In that case, the caller would need to call
            // EndIteration
            //
            Iterator->Reserved[DescriptionIndex] = ple;

            if (pEntry->m_Pdo != NULL) {
                *Device = pEntry->m_Pdo->GetHandle();
                dstatus = WdfChildListRetrieveDeviceSuccess;
            }
            else {
                dstatus = WdfChildListRetrieveDeviceNotYetCreated;
            }

            if (Info != NULL) {
                if (Info->IdentificationDescription != NULL) {
                    CopyId(Info->IdentificationDescription,
                           pEntry->m_IdentificationDescription);
                }
                if (Info->AddressDescription != NULL) {
                    CopyAddress(Info->AddressDescription,
                                pEntry->m_AddressDescription);
                }

                Info->Status = dstatus;
            }

            status = STATUS_SUCCESS;
        }
        else {
            Iterator->Reserved[DescriptionIndex] = NULL;
            ple = NULL;
            status = STATUS_NO_MORE_ENTRIES;
        }
    }

    cur = PtrToUlong(Iterator->Reserved[ModificationIndex]);

    if ((Iterator->Flags & WdfRetrievePendingChildren) == 0 ||
        cur == 0) {
        goto Done;
    }


    //
    // Walk the modification list.  Since this list can change from call to call
    // to GetNextDevice we can't rely on the previous entry still being present
    // so we must use an index into the list.  This is not guaranteed to
    // enumerate all pending additions, but it should be good enough.
    //
    found = FALSE;

    for (i = 1, ple = m_ModificationListHead.Flink;
         ple != &m_ModificationListHead;
         ple = ple->Flink) {

        pEntry = FxDeviceDescriptionEntry::_FromModificationLink(ple);

        if (pEntry->m_ModificationState == ModificationInsert) {
            i++;

            if (i > cur) {
                found = TRUE;

                if (Info != NULL &&
                    Info->EvtChildListIdentificationDescriptionCompare != NULL) {

                    found = Info->EvtChildListIdentificationDescriptionCompare(
                        GetHandle(),
                        Info->IdentificationDescription,
                        pEntry->m_IdentificationDescription);
                }

                if (found) {
                    //
                    // Store the next item number
                    //
                    Iterator->Reserved[ModificationIndex] = ULongToPtr(i);

                    if (Info != NULL) {
                        if (Info->IdentificationDescription != NULL) {
                            CopyId(Info->IdentificationDescription,
                                   pEntry->m_IdentificationDescription);
                        }
                        if (Info->AddressDescription != NULL) {
                            CopyAddress(Info->AddressDescription,
                                        pEntry->m_AddressDescription);
                        }

                        Info->Status = WdfChildListRetrieveDeviceNotYetCreated;
                    }

                    status = STATUS_SUCCESS;
                    break;
                }
            }
        }
    }

    if (found == FALSE) {
        Iterator->Reserved[ModificationIndex] = ULongToPtr(0);
    }

Done:
    KeReleaseSpinLock(&m_ListLock, irql);

    return status;
}

WDFDEVICE
FxChildList::GetNextStaticDevice(
    __in WDFDEVICE PreviousDevice,
    __in ULONG Flags
    )
{
    FxStaticChildDescription *pStatic;
    FxDeviceDescriptionEntry* pEntry;
    WDFDEVICE result;
    PLIST_ENTRY ple;
    KIRQL irql;
    BOOLEAN isNext;

    ASSERT(m_StaticList);

    result = NULL;

    if (PreviousDevice == NULL) {
        //
        // No previous handle, we want the first device we find
        //
        isNext = TRUE;
    }
    else {
        //
        // We have a previuos handle, find it in the list first before setting
        // isNext.
        //
        isNext = FALSE;
    }

    KeAcquireSpinLock(&m_ListLock, &irql);

    ASSERT(m_ScanCount > 0);
    if (m_ScanCount == 0) {
        goto Done;
    }

    //
    // Try to find the next entry
    //
    if (Flags & WdfRetrievePresentChildren) {
        for (ple = m_DescriptionListHead.Flink;
             ple != &m_DescriptionListHead;
             ple = ple->Flink) {
            pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

            //
            // If the entry is marked pending delete, skip it, because the
            // relationship between entry and its PDO has been removed and PDO
            // may have actually been deleted by this time. The only reason
            // this entry is here is because there is still a scan going on
            // and the entry can't be removed in the midst of a scan.
            //
            if (pEntry->m_PendingDeleteOnScanEnd) {
                continue;
            }

            pStatic = CONTAINING_RECORD(pEntry->m_IdentificationDescription,
                                        FxStaticChildDescription,
                                        Header);

            //
            // Only return a handle if it is in a state in which the caller is
            // interested in hearing about.
            //
            if (isNext && pEntry->MatchStateToFlags(Flags)) {
                result = pStatic->Pdo->GetHandle();
                break;
            }
            else if (pStatic->Pdo->GetHandle() == PreviousDevice) {
                isNext = TRUE;
            }
        }
    }

    //
    // We have taken care of all the other states (reported, pending removal,
    //  removed) in the previuos loop.
    //
    if (result == NULL && (Flags & WdfRetrievePendingChildren)) {
        //
        // Walk the modification list.  Since this list can change from call to
        // call, there is no way to definitively give the driver a snapshot of
        // this list of modifications.
        //
        for (ple = m_ModificationListHead.Flink;
             ple != &m_ModificationListHead;
             ple = ple->Flink) {

            pEntry = FxDeviceDescriptionEntry::_FromModificationLink(ple);

            if (pEntry->m_PendingDeleteOnScanEnd) {
                continue;
            }

            pStatic = CONTAINING_RECORD(pEntry->m_IdentificationDescription,
                                        FxStaticChildDescription,
                                        Header);

            //
            // Only return a handle if it is in a state in which the caller is
            // interested in hearing about.
            //
            if (isNext && pEntry->m_ModificationState == ModificationInsert) {
                result = pStatic->Pdo->GetHandle();
                break;
            }
            else if (pStatic->Pdo->GetHandle() == PreviousDevice) {
                isNext = TRUE;
            }
        }
    }

Done:
    KeReleaseSpinLock(&m_ListLock, irql);

    return result;
}

_Must_inspect_result_
NTSTATUS
FxChildList::UpdateAsMissing(
    __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Description
    )
{
    FxDeviceDescriptionEntry* pEntry;
    LIST_ENTRY freeHead;
    NTSTATUS status;
    KIRQL irql;

    //
    // Assume success
    //
    status = STATUS_SUCCESS;
    InitializeListHead(&freeHead);

    KeAcquireSpinLock(&m_ListLock, &irql);

    pEntry = SearchBackwardsForMatchingModificationLocked(Description);

    if (pEntry != NULL) {








        MarkModificationNotPresentWorker(&freeHead, pEntry);
    }
    else {
        pEntry = SearchBackwardsForMatchingDescriptionLocked(Description);

        if (pEntry != NULL) {
            if (pEntry->IsPresent()) {
                MarkDescriptionNotPresentWorker(pEntry, FALSE);
            }
        }
        else {
            //
            // Couldn't find anything
            //
            status = STATUS_NO_SUCH_DEVICE;
        }
    }

    ProcessModificationsLocked(&freeHead);

    KeReleaseSpinLock(&m_ListLock, irql);

    DrainFreeListHead(&freeHead);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxChildList::UpdateDeviceAsMissing(
    __in FxDevice* Device
    )
/*++

Routine Description:
    Same as UpdateAsMissing except instead of a device description, we have the
    device itself.

Arguments:
    Device - the device to mark as missing

Return Value:
    NTSTATUS

  --*/
{
    FxDeviceDescriptionEntry* pEntry;
    LIST_ENTRY freeHead, *ple;
    KIRQL irql;
    BOOLEAN found;
    CfxDevice *pdo;

    found = FALSE;
    pdo = NULL;
    InitializeListHead(&freeHead);

    KeAcquireSpinLock(&m_ListLock, &irql);

    for (ple = m_ModificationListHead.Blink;
         ple != &m_ModificationListHead;
         ple = ple->Blink) {

        pEntry = FxDeviceDescriptionEntry::_FromModificationLink(ple);

        //
        // Static PDO may not be populated in m_Pdo yet, so check the PDO
        // from id description.
        //
        if (m_StaticList) {
            pdo = CONTAINING_RECORD(pEntry->m_IdentificationDescription,
                                    FxStaticChildDescription,
                                    Header)->Pdo;
        }
        else {
            pdo = pEntry->m_Pdo;
        }

        if  (pdo == Device) {
            found = TRUE;
            MarkModificationNotPresentWorker(&freeHead, pEntry);
            break;
        }
    }

    if (found == FALSE) {
        for (ple = m_DescriptionListHead.Blink;
             ple != &m_DescriptionListHead;
             ple = ple->Blink) {

            pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

            //
            // Static PDO may not be populated in m_Pdo yet, so check the PDO
            // from id description.
            //
            if (m_StaticList) {
                pdo = CONTAINING_RECORD(pEntry->m_IdentificationDescription,
                                        FxStaticChildDescription,
                                        Header)->Pdo;
            }
            else {
                pdo = pEntry->m_Pdo;
            }

            if (pdo == Device) {
                found = TRUE;

                if (pEntry->IsPresent()) {
                    MarkDescriptionNotPresentWorker(pEntry, FALSE);
                }

                break;
            }
        }
    }

    ProcessModificationsLocked(&freeHead);

    KeReleaseSpinLock(&m_ListLock, irql);

    DrainFreeListHead(&freeHead);

    if (found) {
        return STATUS_SUCCESS;
    }
    else {
        return STATUS_NO_SUCH_DEVICE;
    }
}

BOOLEAN
FxChildList::ReenumerateEntryLocked(
    __inout FxDeviceDescriptionEntry* Entry,
    __in BOOLEAN FromQDR
    )
{
    BOOLEAN result;

    //
    // Check to see if there are modifications pending
    //
    if (IsListEmpty(&Entry->m_ModificationLink) && Entry->IsPresent()) {
        //
        // No mods pending...
        //

        //
        // We can only go down this path if the device was reported to the system
        // b/c the stack on our PDO is the one invoking this call.
        //
        ASSERT(Entry->m_DescriptionState == DescriptionInstantiatedHasObject);

        if (FromQDR == FALSE) {
            //
            // The entry has not been reported as missing by the driver, try to
            // clone it and insert a modification.
            //
            Entry->m_ModificationState = ModificationClone;
            InsertTailList(&m_ModificationListHead, &Entry->m_ModificationLink);
        }

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
            "Inserting clone modification for PDO WDFDEVICE %p, !devobj %p",
            Entry->m_Pdo->GetHandle(), Entry->m_Pdo->GetDeviceObject());

        result = TRUE;
    }
    else {
        //
        // If there is a pending modification, we don't do anything.
        // If the device is already not present, we don't do anything.
        //
        // ModficiationRemove or
        // ModficiationRemoveNotify:  we would be reenumerating a device which
        //                            is being removed which is not what we want
        //
        // ModificationClone:  we would be cloning a clone which is not what
        //                     we want either
        //
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
            "Requested reenumeration for PDO WDFDEVICE %p, !devobj %p"
            " no possible (pending mod %d, currently present %d)",
            Entry->m_Pdo->GetHandle(), Entry->m_Pdo->GetDeviceObject(),
            IsListEmpty(&Entry->m_ModificationLink), Entry->IsPresent());

        result = FALSE;
    }

    return result;
}

VOID
FxChildList::ReenumerateEntry(
    __inout FxDeviceDescriptionEntry* Entry
    )
{
    LIST_ENTRY freeHead;
    KIRQL irql;

    InitializeListHead(&freeHead);

    KeAcquireSpinLock(&m_ListLock, &irql);
    (void) ReenumerateEntryLocked(Entry, FALSE);
    ProcessModificationsLocked(&freeHead);
    KeReleaseSpinLock(&m_ListLock, irql);

    DrainFreeListHead(&freeHead);
}


VOID
FxChildList::UpdateAllAsPresent(
    __in_opt PULONG ScanTag
    )
{
    PLIST_ENTRY pCur, pNext;
    LIST_ENTRY freeHead;
    FxDeviceDescriptionEntry* pEntry;
    KIRQL irql;

    KeAcquireSpinLock(&m_ListLock, &irql);

    if (ScanTag != NULL) {
        if (*ScanTag != ScanTagActive) {
            ASSERT(*ScanTag == ScanTagCancelled);
            KeReleaseSpinLock(&m_ListLock, irql);
            return;
        }
    }

    //
    // Walk over all mod entries and ensure any inserts are still marked
    // present.
    //
    for (pCur = m_ModificationListHead.Flink;
         pCur != &m_ModificationListHead;
         pCur = pNext) {

        pNext = pCur->Flink;

        pEntry = FxDeviceDescriptionEntry::_FromModificationLink(pCur);

        if (pEntry->m_ModificationState == ModificationInsert) {
            pEntry->m_FoundInLastScan = TRUE;
        }
    }

    //
    // Walk over all desc entries and ensure they are marked present.
    //
    for (pCur = m_DescriptionListHead.Flink;
         pCur != &m_DescriptionListHead;
         pCur = pCur->Flink) {

        pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(pCur);

        if (pEntry->IsPresent()) {
            ASSERT(pEntry->m_ModificationState != ModificationRemoveNotify);

            if (pEntry->m_ModificationState != ModificationRemove) {
                pEntry->m_FoundInLastScan = TRUE;
            }
        }
    }

    //
    // Attempt to drain any mods we made.
    //
    InitializeListHead(&freeHead);
    ProcessModificationsLocked(&freeHead);

    KeReleaseSpinLock(&m_ListLock, irql);

    DrainFreeListHead(&freeHead);
}

_Must_inspect_result_
NTSTATUS
FxChildList::Add(
    __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    __in_opt PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription,
    __in_opt PULONG ScanTag
    )
{
    LIST_ENTRY freeHead;
    KIRQL irql;
    NTSTATUS status;
    FxDeviceDescriptionEntry* pEntry;
    BOOLEAN allocNewEntry;

    if (ScanTag != NULL) {
        if (*ScanTag != ScanTagActive) {
            ASSERT(*ScanTag == ScanTagCancelled);

            //
            // The descriptions are duplicated if needed, so no need to clean
            // them up
            //
            // CleanupDescriptions(IdentificationDescription,
            //                     AddressDescription);

            return STATUS_CANCELLED;
        }
    }

    status = STATUS_UNSUCCESSFUL;
    allocNewEntry = FALSE;
    InitializeListHead(&freeHead);

    KeAcquireSpinLock(&m_ListLock, &irql);

    pEntry = SearchBackwardsForMatchingModificationLocked(
        IdentificationDescription
        );

    if (pEntry != NULL) {
        switch (pEntry->m_ModificationState) {
        case ModificationInsert:
            if (HasAddressDescriptions()) {
                CopyAddress(pEntry->m_AddressDescription,
                            AddressDescription);
            }

            //
            // There is already a pending insert operation. All we need do is
            // update the stored description.
            //
            pEntry->m_FoundInLastScan = TRUE;

            status = STATUS_OBJECT_NAME_EXISTS;
            break;

        case ModificationRemove:
        case ModificationRemoveNotify:
            //
            // We found a pending remove. Therefore we need to add an insert
            // operation to the end of the mod list.
            //
            allocNewEntry = TRUE;
            break;

        default:
            ASSERTMSG("Invalid description modification state\n", FALSE);
            break;
        }
    }
    else {
        //
        // There are no matching modifications queued in the list. Go examine
        // the main list. We can't modify any of the child links right now
        // because the list might be locked down, but it is scannable!
        //
        pEntry = SearchBackwardsForMatchingDescriptionLocked(
            IdentificationDescription
            );

        if (pEntry != NULL && pEntry->IsPresent()) {
            //
            // This is an update. Enqueue it through this node.
            //
            ASSERT(pEntry->m_ModificationState == ModificationUnspecified);

            if (HasAddressDescriptions()) {
                CopyAddress(pEntry->m_AddressDescription,
                            AddressDescription);
            }

            pEntry->m_FoundInLastScan = TRUE;
            status = STATUS_OBJECT_NAME_EXISTS;
        }
        else {
            //
            // Either no entry was found, or a "missing" entry was identified
            // instead. In both cases we need to add an insert operation to the
            // end of the modification list.
            //
            allocNewEntry = TRUE;
        }
    }

    if (allocNewEntry) {

        pEntry =
            new(GetDriverGlobals(), m_TotalDescriptionSize)
                FxDeviceDescriptionEntry(this,
                                         m_IdentificationDescriptionSize,
                                         m_AddressDescriptionSize);

        if (pEntry != NULL) {
            status = DuplicateId(pEntry->m_IdentificationDescription,
                   IdentificationDescription);

            if (NT_SUCCESS(status) && HasAddressDescriptions()) {
                status = DuplicateAddress(pEntry->m_AddressDescription,
                                          AddressDescription);
            }

            if (NT_SUCCESS(status)) {
                pEntry->m_FoundInLastScan = TRUE;
                InsertTailList(&m_ModificationListHead,
                               &pEntry->m_ModificationLink);

                if (m_StaticList) {
                    FxDevice* pPdo;

                    pPdo = CONTAINING_RECORD(pEntry->m_IdentificationDescription,
                                             FxStaticChildDescription,
                                             Header)->Pdo;

                    //
                    // The device is no longer deletable by the driver writer
                    //
                    pPdo->MarkNoDeleteDDI();

                    //
                    // Plug in the description like we do for dynamic PDOs in
                    // FxPkgPdo::Initialize.
                    //
                    pPdo->GetPdoPkg()->m_Description = pEntry;

                    //
                    // Let the device know it was inserted into the list.  This
                    // used to know in pPdo::DeleteObject how to proceed.
                    //
                    pPdo->GetPdoPkg()->m_AddedToStaticList = TRUE;
                }
            }
            else {
                InsertTailList(&freeHead, &pEntry->m_DescriptionLink);
            }
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    ProcessModificationsLocked(&freeHead);

    KeReleaseSpinLock(&m_ListLock, irql);

    DrainFreeListHead(&freeHead);

    return status;
}

VOID
FxChildList::UpdateAddressDescriptionFromEntry(
    __inout FxDeviceDescriptionEntry* Entry,
    __in PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    KIRQL irql;

    KeAcquireSpinLock(&m_ListLock, &irql);
    CopyAddress(Entry->m_AddressDescription, AddressDescription);
    KeReleaseSpinLock(&m_ListLock, irql);






}

BOOLEAN
FxChildList::CloneEntryLocked(
    __inout PLIST_ENTRY FreeListHead,
    __inout FxDeviceDescriptionEntry* Entry,
    __in BOOLEAN FromQDR
    )
{
    FxDeviceDescriptionEntry* pClone;
    BOOLEAN invalidateRelations;

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "attempting to clone PDO WDFDEVICE %p, !devobj %p, From QDR %d",
        Entry->m_Pdo->GetHandle(), Entry->m_Pdo->GetDeviceObject(), FromQDR);

    invalidateRelations = FALSE;

    pClone = Entry->Clone(FreeListHead);

    if (pClone != NULL) {
        //
        // Check to see if the driver writer OKs the cloning, aka
        // reenumeration.
        //
        if (m_EvtChildListDeviceReenumerated != NULL &&
            m_EvtChildListDeviceReenumerated(
                GetHandle(),
                Entry->m_Pdo->GetHandle(),
                Entry->m_AddressDescription,
                pClone->m_AddressDescription) == FALSE){
            //
            // The clone will be freed later by the caller by placing
            // it on the free list.
            //
            InsertTailList(FreeListHead, &pClone->m_DescriptionLink);
        }
        else {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "clone successful (new entry is %p), marking PDO "
                "WDFDEVICE %p, !devobj %p as missing", pClone,
                Entry->m_Pdo->GetHandle(), Entry->m_Pdo->GetDeviceObject());

            //
            // Setup the state of the clone
            //
            pClone->m_DescriptionState = DescriptionPresentNeedsInstantiation;
            pClone->m_ModificationState = ModificationUnspecified;

            //
            // This is important that this is last.  If a subsequent
            // modification arrives for this device, we want to find the
            // clone before the old device.  Since all changes search the
            // list backwards, inserting at the tail accomplishes this.
            //
            InsertTailList(&m_DescriptionListHead,
                           &pClone->m_DescriptionLink);

            if (FromQDR == FALSE) {
                Entry->m_DescriptionState = DescriptionNotPresent;
                invalidateRelations = TRUE;
            }
        }
    }
    else {
        //
        // Could not allocate a clone, do not anything.
        //
        DO_NOTHING();
    }

    //
    // Convert the original device's modification state back to a
    // unspecified in all cases.
    //
    Entry->m_ModificationState = ModificationUnspecified;

    return invalidateRelations;
}

VOID
FxChildList::ProcessModificationsLocked(
    __inout PLIST_ENTRY FreeListHead
    )
{
    FxDeviceDescriptionEntry *pEntry;
    PLIST_ENTRY pCur, pNext;
    BOOLEAN invalidateRelations;

    //
    // If the list is locked or there are still active scans, we must not process
    // any modifications.
    //
    if (m_State != ListUnlocked || m_ScanCount > 0) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
            "Not processing modifications on WDFCHILDLIST %p (list state %d, "
            "scan count %d)", GetObjectHandle(), m_State, m_ScanCount);
        return;
    }

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "Begin processing modifications on WDFCHILDLIST %p", GetObjectHandle());

    //
    // First do the mod updates. As quick as possible. Note that we need to
    // update
    //
    for (pCur = m_ModificationListHead.Flink;
         pCur != &m_ModificationListHead;
         pCur = pNext) {

        pNext = pCur->Flink;

        pEntry = FxDeviceDescriptionEntry::_FromModificationLink(pCur);

        switch (pEntry->m_ModificationState) {
        case ModificationRemoveNotify:















            pEntry->m_ModificationState = ModificationRemove;
            break;

        default:
            break;
        }
    }

    invalidateRelations = FALSE;
    while(!IsListEmpty(&m_ModificationListHead)) {
        pCur = RemoveHeadList(&m_ModificationListHead);

        InitializeListHead(pCur);
        pEntry = FxDeviceDescriptionEntry::_FromModificationLink(pCur);

        ASSERT(pEntry->m_ModificationState == ModificationInsert ||
                  pEntry->m_ModificationState == ModificationClone ||
                  pEntry->m_ModificationState == ModificationRemove);

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
            "entry %p, mod state is %!FxChildListModificationState!",
            pEntry, pEntry->m_ModificationState);

        switch (pEntry->m_ModificationState) {
        case ModificationRemove:
            //
            // Remove's are stacked on top of the entry they need to take out.
            //
            ASSERT(pEntry->IsPresent());

            pEntry->m_ModificationState = ModificationUnspecified;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "processing remove on entry %p, description state is "
                "%!FxChildListDescriptionState!",
                pEntry, pEntry->m_DescriptionState);

            switch (pEntry->m_DescriptionState) {
            case DescriptionPresentNeedsInstantiation:
                //
                // We got to the entry before a DO could be created for it or
                // an instantiation failed.  If deleeted before creation, the
                // list entry points to itself.  If the instantiation failed,
                // the list entry is in the description list and must be
                // removed from it.
                //
                // Mark it for deletion now (outside of the lock being held).
                //
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                    "entry %p never reported to pnp, mark for deletion", pEntry);

                RemoveEntryList(&pEntry->m_DescriptionLink);
                InsertTailList(FreeListHead, &pEntry->m_DescriptionLink);
                break;

            case DescriptionInstantiatedHasObject:
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                    "committing PDO WDFDEVICE %p, !devobj %p as not present",
                    pEntry->m_Pdo->GetHandle(), pEntry->m_Pdo->GetDeviceObject());

                pEntry->m_DescriptionState = DescriptionNotPresent;
                invalidateRelations = TRUE;
                break;

            default:
                ASSERTMSG("Invalid description state\n", FALSE);
                break;
            }

            break;

        case ModificationInsert:
            //
            // CurEntry is our entry to insert into the list. We simply "move"
            // the mod entries to desc entries.
            //
            ASSERT(pEntry->m_DescriptionState == DescriptionUnspecified);
            pEntry->m_DescriptionState = DescriptionPresentNeedsInstantiation;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "marking entry %p as needing instantiation", pEntry);

            InsertTailList(&m_DescriptionListHead, &pEntry->m_DescriptionLink);
            pEntry->m_ModificationState = ModificationUnspecified;
            invalidateRelations = TRUE;
            break;

        case ModificationClone:
            invalidateRelations = CloneEntryLocked(FreeListHead, pEntry, FALSE);
            break;

        default:
            ASSERTMSG("Invalid description modification state\n", FALSE);
            break;
        }
    }

    if (invalidateRelations) {
        PDEVICE_OBJECT pdo;

        if (m_ScanCount) {
            m_InvalidationNeeded = TRUE;
        }
        else if ((pdo = m_Device->GetSafePhysicalDevice()) != NULL) {
            //
            // See previous usage of m_Device->m_KnownPdo for
            // comments on why a lock is not necessary when reading its value.
            //
            m_InvalidationNeeded = FALSE;
            IoInvalidateDeviceRelations(pdo, BusRelations);
        }
        else {
            m_InvalidationNeeded = TRUE;
        }
    }

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
       "end processing modifications on WDFCHILDLIST %p", GetObjectHandle());
}

VOID
FxChildList::MarkDescriptionNotPresentWorker(
    __inout FxDeviceDescriptionEntry* DescriptionEntry,
    __in BOOLEAN ModificationCanBeQueued
    )
/*++

Routine Description:

    This worker function marks the passed in mod or desc entry in the device
    list "not present". The change is enqueued in the mod list but the mod
    list is not drained.

Arguments:

    DescriptionEntry - Entry to be marked as "not present".

    ModificationCanBeQueued - whether the caller allows for this description
        to be a modification already queued on the modification list

Assumes:
    DescriptionEntry->IsPresent() == TRUE

Return Value:
    None

--*/
{
    BOOLEAN queueMod;

    ASSERT(DescriptionEntry->IsPresent());

    queueMod = FALSE;

    if (ModificationCanBeQueued) {
        if (IsListEmpty(&DescriptionEntry->m_ModificationLink)) {
            queueMod = TRUE;
        }
        else {
            //
            // If the modification is queued, it must be removal
            //
            ASSERT(DescriptionEntry->m_ModificationState ==
                                                      ModificationRemoveNotify);
            ASSERT(DescriptionEntry->m_FoundInLastScan == FALSE);
        }
    }
    else {
        queueMod = TRUE;
    }

    if (queueMod) {
        ASSERT(IsListEmpty(&DescriptionEntry->m_ModificationLink));
        ASSERT(DescriptionEntry->m_ModificationState == ModificationUnspecified);

        //
        // Add a removal modification entry into the modification list.
        // The ModificationRemoveNotify state will be converted into
        // ModificationRemove later while the list lock is still being held.
        //
        DescriptionEntry->m_ModificationState = ModificationRemoveNotify;
        DescriptionEntry->m_FoundInLastScan = FALSE;

        InsertTailList(&m_ModificationListHead,
                        &DescriptionEntry->m_ModificationLink);
    }
}

VOID
FxChildList::MarkModificationNotPresentWorker(
    __inout PLIST_ENTRY FreeListHead,
    __inout FxDeviceDescriptionEntry* ModificationEntry
    )
/*++

Routine Description:

    This worker function marks the passed in mod or desc entry in the device
    list "not present". The change is enqueued in the mod list but the mod
    list is not drained.

Arguments:

    FreeListHead - Free list of entries.

    ModificationEntry - Entry to be marked as "not present".

Return Value:
    None

--*/
{
    switch (ModificationEntry->m_ModificationState) {
    case ModificationInsert:
        //
        // This one was never reported to the OS. Remove it now.
        //
        RemoveEntryList(&ModificationEntry->m_ModificationLink);
        InitializeListHead(&ModificationEntry->m_ModificationLink);

        if (IsStaticList()) {
            //
            // There is a corner case of a static PDO being added and
            // immediately marked as missing before it has a chance to move to
            // description list. This case is handled here by marking the
            // modification state to ModificationNeedsPnpRemoval. Fx cannot just
            // delete the description because there is a driver-created PDO
            // associated with the description and it needs to be cleaned up.
            //
            ModificationEntry->m_ModificationState = ModificationNeedsPnpRemoval;
            ASSERT(ModificationEntry->m_DescriptionState == DescriptionUnspecified);
        }

        ASSERT(IsListEmpty(&ModificationEntry->m_DescriptionLink));
        InsertTailList(FreeListHead, &ModificationEntry->m_DescriptionLink);
        break;

    case ModificationClone:
        //
        // In between the PDOs stack asking for a reenumeration and applying
        // this change, the bus driver has reported the child as missing.
        // Convert the clone modification to a removal modification.
        //

        //
        // MarkDescriptionNotPresentWorker expects that m_ModificationEntry is
        // not in any list and points to itself.
        //
        RemoveEntryList(&ModificationEntry->m_ModificationLink);
        InitializeListHead(&ModificationEntry->m_ModificationLink);

        MarkDescriptionNotPresentWorker(ModificationEntry, FALSE);
        break;

    default:
        DO_NOTHING();
        break;
    }
}

VOID
FxChildList::DrainFreeListHead(
    __inout PLIST_ENTRY FreeListHead
    )
{
    FxDeviceDescriptionEntry *pEntry;
    FxDevice* pPdo;
    PLIST_ENTRY pCur;

    while (!IsListEmpty(FreeListHead)) {
        pCur = RemoveHeadList(FreeListHead);
        InitializeListHead(pCur);

        pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(pCur);

        //
        // If this is a static list and the entry has not yet been instantiated,
        // tell PnP to remove the device.
        //
        // There is a corner case of a static PDO being added and
        // immediately marked as missing before it has a chance to move to
        // description list. This case is handled here by checking the
        // modification state of ModificationNeedsPnpRemoval. Fx cannot just
        // delete the description because there is a driver-created PDO
        // associated with the description and it needs to be cleaned up.
        //
        if (m_StaticList &&
            (pEntry->m_DescriptionState == DescriptionPresentNeedsInstantiation ||
             pEntry->m_ModificationState == ModificationNeedsPnpRemoval )) {

            pPdo = CONTAINING_RECORD(pEntry->m_IdentificationDescription,
                                     FxStaticChildDescription,
                                     Header)->Pdo;

            //
            // The pnp removal path expects that there is no modifcation pending
            // when removing the description.
            //
            if (pEntry->m_ModificationState == ModificationNeedsPnpRemoval) {
                ASSERT(pEntry->m_DescriptionState == DescriptionUnspecified);
                ASSERT(IsListEmpty(&pEntry->m_ModificationLink));

                pEntry->m_ModificationState = ModificationUnspecified;
            }

            //
            // Change the state to reported missing (which is what we are basically
            // simulating here anyways) so that when we process the entry again
            // when the PDO really goes away we cleanup the entry.
            //
            // Also, when posting PnpEventRemove to the child, child will
            // call FxDeviceDescriptionEntry::IsDeviceRemoved which checks for
            // reported missing before reporting if the device has been removed
            // (and we want the device to be reported as removed).
            //
            pEntry->m_DescriptionState = DescriptionReportedMissing;

            //
            // Not yet reported to PnP,  so it should be in the initial state
            //
            ASSERT(pPdo->GetDevicePnpState() == WdfDevStatePnpInit);

            //
            // m_Description should be set when the FxPkgPdo was created
            //
            ASSERT(pPdo->GetPdoPkg()->m_Description != NULL);

            //
            // Since the pPdo->m_PkgPnp->m_DeviceRemoveProcessed == NULL, the
            // pnp state machine will delete the PDO.  We are simulating the same
            // situation where an FDO is in the remove path and is telling each
            // of its removed PDOs to delete themselves.
            //
            pPdo->m_PkgPnp->PnpProcessEvent(PnpEventRemove);
        }
        else {
            CleanupDescriptions(pEntry->m_IdentificationDescription,
                               pEntry->m_AddressDescription);

            delete pEntry;
        }
    }
}

FxDeviceDescriptionEntry*
FxChildList::SearchBackwardsForMatchingModificationLocked(
    __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Id
    )
{
    PLIST_ENTRY ple;
    FxDeviceDescriptionEntry* pEntry;

    for (ple = m_ModificationListHead.Blink;
         ple != &m_ModificationListHead;
         ple = ple->Blink) {

        pEntry = FxDeviceDescriptionEntry::_FromModificationLink(ple);

        if (CompareId(pEntry->m_IdentificationDescription, Id)) {
            return pEntry;
        }
    }

    return NULL;
}

FxDeviceDescriptionEntry*
FxChildList::SearchBackwardsForMatchingDescriptionLocked(
    __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Id
    )
{
    PLIST_ENTRY ple;
    FxDeviceDescriptionEntry* pEntry;

    for (ple = m_DescriptionListHead.Blink;
         ple != &m_DescriptionListHead;
         ple = ple->Blink) {

        pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

        if (CompareId(pEntry->m_IdentificationDescription, Id)) {
            return pEntry;
        }
    }

    return NULL;
}

_Must_inspect_result_
NTSTATUS
FxChildList::VerifyDescriptionEntry(
    __in PLIST_ENTRY Entry
    )
{
    if (Entry == &m_DescriptionListHead) {
        return STATUS_SUCCESS;
    }
    else {
        PLIST_ENTRY ple;

        for (ple = m_DescriptionListHead.Flink;
             ple != &m_DescriptionListHead;
             ple = ple->Flink) {
            if (Entry == ple) {
                return STATUS_SUCCESS;
            }
        }

        return STATUS_INVALID_PARAMETER;
    }
}

_Must_inspect_result_
NTSTATUS
FxChildList::VerifyModificationEntry(
    __in PLIST_ENTRY Entry
    )
{
    if (Entry == &m_ModificationListHead) {
        return STATUS_SUCCESS;
    }
    else {
        PLIST_ENTRY ple;

        for (ple = m_ModificationListHead.Flink;
             ple != &m_ModificationListHead;
             ple = ple->Flink) {
            if (Entry == ple) {
                return STATUS_SUCCESS;
            }
        }

        return STATUS_INVALID_PARAMETER;
    }
}

BOOLEAN
FxChildList::CreateDevice(
    __inout FxDeviceDescriptionEntry* Entry,
    __inout PBOOLEAN InvalidateRelations
    )
{
    WDFDEVICE_INIT init(m_Device->GetDriver());
    NTSTATUS status;
    KIRQL irql;

    init.CreatedOnStack = TRUE;
    init.SetPdo(m_Device);
    init.Pdo.DescriptionEntry = Entry;

    if (m_StaticList) {
        init.CreatedDevice =
            CONTAINING_RECORD(Entry->m_IdentificationDescription,
                              FxStaticChildDescription,
                              Header)->Pdo;
    }
    else {
        //
        // This description needs a device, create it now.
        //
        status = m_EvtCreateDevice.Invoke(
            GetHandle(), Entry->m_IdentificationDescription, &init);

        if (status == STATUS_RETRY) {
            if (init.CreatedDevice != NULL) {
                //
                // Destroy any allocations assocated with the device.
                //
                init.CreatedDevice->Destroy();
            }

            *InvalidateRelations = TRUE;

            return FALSE;
        }

        if (NT_SUCCESS(status)) {
            //
            // Make sure that a framework device was actually created
            //
            if (init.CreatedDevice == NULL) {
                //
                // Driver didn't actually create the device, even though its
                // EvtChildListCreateDevice returned a successful return code.
                // Change the status to indicate an error, so that we enter the
                // error handling code below.
                //
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "EvtChildListCreateDevice returned a successful status "
                    "%!STATUS! but did not create a device object", status);

                FxVerifierDbgBreakPoint(GetDriverGlobals());

                status = STATUS_INVALID_DEVICE_OBJECT_PARAMETER;
            }
        }

        if (!NT_SUCCESS(status)) {
            if (init.CreatedDevice != NULL) {
                KeAcquireSpinLock(&m_ListLock, &irql);
                //
                // Set to missing so that when the pnp machine evaluates whether the
                // PDO has been reported missing or not, it chooses missing and
                // deletes the PDO immediately.
                //
                Entry->m_DescriptionState = DescriptionReportedMissing;

                if (Entry->m_ModificationState != ModificationUnspecified) {
                    //
                    // Cannot be ModificationInsert since this device ID is already
                    // in the list.
                    //
                    // Cannot be ModificationClone since the FDO for this device
                    // is the only one who can ask for that and since PDO creation
                    // failed, there is no FDO.
                    //
                    // Cannot be ModificationRemove because the list is locked
                    // against changes and ModificationRemoveNotify becomes
                    // ModificationRemove only after the list has been unlocked.
                    //
                    ASSERT(Entry->m_ModificationState == ModificationRemoveNotify);

                    //
                    // Remove the modification from the list.  The call to
                    // DeleteDeviceFromFailedCreate below will destroy this
                    // Entry due to the pnp state machine deleting the PDO.
                    //
                    RemoveEntryList(&Entry->m_ModificationLink);
                }
                KeReleaseSpinLock(&m_ListLock, irql);

                ASSERT(init.CreatedDevice->IsPnp());
                ASSERT(init.CreatedDevice->GetDevicePnpState() == WdfDevStatePnpInit);
                ASSERT(init.CreatedDevice->GetPdoPkg()->m_Description != NULL);

                ASSERT(Entry->m_Pdo == NULL);

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "WDFDEVICE %p !devobj %p created, but EvtChildListCreateDevice "
                    "returned status %!STATUS!", init.CreatedDevice->GetHandle(),
                    init.CreatedDevice->GetDeviceObject(), status);

                //
                // Simulate a remove event coming to the device.  After this call
                // returns, init.CreatedDevice is no longer a valid pointer!
                //
                // Please note that DeleteDeviceFromFailedCreate just returns
                // the passed status back unless device is a filter, in which
                // case it changes it to success.
                //
                // It is not really the status of DeleteDeviceFromFailedCreate
                // operation, which is why we don't check it.
                //
                (void) init.CreatedDevice->DeleteDeviceFromFailedCreate(
                                                                status,
                                                                TRUE);

                init.CreatedDevice = NULL;
            }
            else {
                LIST_ENTRY freeHead;

                InitializeListHead(&freeHead);

                //
                // Add a modification that this entry is now gone.  If there is
                // an active scan and it has added this device, there would
                // be no entry in the modification case list because the add
                // would have updated the description entry.
                //
                KeAcquireSpinLock(&m_ListLock, &irql);
                MarkDescriptionNotPresentWorker(Entry, TRUE);
                ProcessModificationsLocked(&freeHead);
                KeReleaseSpinLock(&m_ListLock, irql);

                //
                // Free structures outside of any internal WDF lock
                //
                DrainFreeListHead(&freeHead);
            }

            return FALSE;
        }
    }

    //
    // Assign m_Pdo here once we have a fully baked and created device.  We only
    // assign m_Pdo after we have completely initalized device because we check
    // for m_Pdo in PostParentToD0.
    //
    Entry->m_Pdo = init.CreatedDevice;
    Entry->m_DescriptionState = DescriptionInstantiatedHasObject;

    return TRUE;
}

_Must_inspect_result_
NTSTATUS
FxChildList::ProcessBusRelations(
    __inout PDEVICE_RELATIONS *DeviceRelations
    )
{
    LIST_ENTRY freeHead;
    PLIST_ENTRY ple, pNext;
    FxDeviceDescriptionEntry* pEntry;
    PDEVICE_RELATIONS pPriorRelations, pNewRelations;
    PDEVICE_OBJECT pDevice;
    BOOLEAN needToReportMissingChildren, invalidateRelations, cleanupRelations;
    ULONG additionalCount, totalCount, i;
    size_t size;
    NTSTATUS status;
    KIRQL irql;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxVerifierCheckIrqlLevel(GetDriverGlobals(), PASSIVE_LEVEL);

    pNewRelations = NULL;
    invalidateRelations = FALSE;
    cleanupRelations = TRUE;
    InitializeListHead(&freeHead);
    pFxDriverGlobals = GetDriverGlobals();

    //
    // Get the count of DO's we will need to add. While we're at it, free any
    // extensions that never got a chance for a DO, and note extensions that
    // will need to wait for a remove IRP.
    //
    KeAcquireSpinLock(&m_ListLock, &irql);

    m_State = ListLockedForEnum;

    additionalCount = 0;
    needToReportMissingChildren = FALSE;

    for (ple = m_DescriptionListHead.Flink;
         ple != &m_DescriptionListHead;
         ple = ple->Flink) {

        pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

        switch (pEntry->m_DescriptionState) {

        case DescriptionPresentNeedsInstantiation:
        case DescriptionInstantiatedHasObject:
            //
            // We'll be needing a DO here.
            //
            additionalCount++;
            break;

        case DescriptionNotPresent:
            //
            // We will now report the description as missing
            //
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "PDO WDFDEVICE %p !devobj %p in a not present state, need to "
                "report as missing",
                pEntry->m_Pdo->GetHandle(), pEntry->m_Pdo->GetDeviceObject());

            needToReportMissingChildren = TRUE;
            break;

        case DescriptionReportedMissing:
            //
            // Already reported missing in a previous handling of QDR
            //
            break;

        default:
            ASSERTMSG("Invalid description state\n", FALSE);
            break;
        }
    }

    KeReleaseSpinLock(&m_ListLock, irql);

    pPriorRelations = *DeviceRelations;

    //
    // If we have
    // 1)  no devices in the list AND
    //   a) we have nothing to report OR
    //   b) we have something to report and there are previous relations (which
    //      if left unchanged will be used to report our missing devices)
    //
    // THEN nothing else to do except marking the NotPresent children as
    // missing, unlock the list and return the special return
    // code STATUS_NOT_SUPPORTED indicating this condition.
    //
    if (additionalCount == 0 &&
        (needToReportMissingChildren == FALSE || pPriorRelations != NULL)) {
        //
        // We have nothing to add, nor subtract from prior allocations. Note
        // the careful logic with counting missing children - the OS does not
        // treat no list and an empty list identically. As such we must be
        // sure to return some kind of list in the case where we reported
        // something previously.
        //
        // Mark the NotPresent children as Missing
        // We can walk the ChildList without holding the spinlock because we set
        // m_State to ListLockedForEnum.
        //
        if (needToReportMissingChildren) {
            for (ple = m_DescriptionListHead.Flink;
                 ple != &m_DescriptionListHead;
                 ple = pNext) {
                pNext = ple->Flink;

                pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

                if (pEntry->m_DescriptionState == DescriptionNotPresent) {
                    //
                    // We will now report the description as missing
                    //
                    DoTraceLevelMessage(
                        pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGPNP,
                        "PDO WDFDEVICE %p !devobj %p reported as missing to pnp",
                        pEntry->m_Pdo->GetHandle(), pEntry->m_Pdo->GetDeviceObject());

                    pEntry->m_DescriptionState = DescriptionReportedMissing;
                    pEntry->m_ReportedMissingCallbackState = CallbackNeedsToBeInvoked;
                }
            }
        }
        else {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "Nothing to report on WDFCHILDLIST %p, returning early", GetHandle());
        }

        cleanupRelations = FALSE;
        status = STATUS_NOT_SUPPORTED;
        goto Done;
    }

    //
    // Adjust the count for any existing relations.
    //
    totalCount = additionalCount;
    if (pPriorRelations != NULL) {
        totalCount += pPriorRelations->Count;
    }

    size = _ComputeRelationsSize(totalCount);

    pNewRelations = (PDEVICE_RELATIONS)
        ExAllocatePoolWithTag(PagedPool, size, pFxDriverGlobals->Tag);

    if (pNewRelations == NULL) {
        //
        // We can't add our information.
        //
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Could not allocate relations for %d devices",
                            totalCount);

        //
        // Just like above, STATUS_NOT_SUPPORTED is a special value indicating
        // to the caller that the QDR has not been handled and that the caller
        // should not change the status or the existing relations in the irp.
        //
        cleanupRelations = FALSE;
        status = STATUS_NOT_SUPPORTED;

        m_EnumRetries++;
        if (m_EnumRetries <= FX_CHILD_LIST_MAX_RETRIES) {
            invalidateRelations = TRUE;
        }
        else {
            if (needToReportMissingChildren) {
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                    "WDFCHILDLIST %p could not allocate relations required for "
                    "reporting children as missing after max retries",
                    GetHandle());
            }

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFCHILDLIST %p retried %d times to report relations, but "
                "failed each time", GetHandle(), FX_CHILD_LIST_MAX_RETRIES);
        }

        if (pPriorRelations == NULL) {
            //
            // If the prior relations are NULL we can just return failure and
            // not affect anyone else's state that had previous thought they
            // committed a relations structure back to pnp (which we tried to
            // update but could not b/c of no memory).
            //
            // By returning failure that is != STATUS_NOT_SUPPORTED, the pnp
            // manager will not process a change in the relations.
            //
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else {
            //
            // Do *not* throw away the old list and return error because that
            // will put the driver which created the existing relations into an
            // inconsistent state with respect to the pnp manager state if we
            // fail it here and the pnp manager never seeds the changes.
            //
            // Instead, we must process the failure locally and mark each object
            // which is already reported as present as missing.
            //
            for (ple = m_DescriptionListHead.Flink;
                 ple != &m_DescriptionListHead;
                 ple = pNext) {

                pNext = ple->Flink;
                pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

                switch (pEntry->m_DescriptionState) {
                case DescriptionPresentNeedsInstantiation:
                    //
                    // This description can stay in this state b/c we will just
                    // ask for another QDR.
                    //
                    break;

                case DescriptionInstantiatedHasObject:
                    DoTraceLevelMessage(
                        pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGPNP,
                        "PDO WDFDEVICE %p !devobj %p being marked as missing "
                        "because of failure to allocate device relations and "
                        "an already existing relations %p",
                        pEntry->m_Pdo->GetHandle(),
                        pEntry->m_Pdo->GetDeviceObject(), pPriorRelations);

                    KeAcquireSpinLock(&m_ListLock, &irql);
                    if (m_StaticList == FALSE) {
                        if (ReenumerateEntryLocked(pEntry, TRUE)) {
                            DoTraceLevelMessage(
                                pFxDriverGlobals,
                                TRACE_LEVEL_INFORMATION, TRACINGPNP,
                                "PDO WDFDEVICE %p !devobj %p being cloned "
                                "because of the failure to allocate device "
                                "relations",
                                pEntry->m_Pdo->GetHandle(),
                                pEntry->m_Pdo->GetDeviceObject());

                            (void) CloneEntryLocked(&freeHead, pEntry, TRUE);
                        }
                    }
                    else {
                        DoTraceLevelMessage(
                            pFxDriverGlobals,
                            TRACE_LEVEL_WARNING, TRACINGPNP,
                            "PDO WDFDEVICE %p !devobj %p is a statically "
                            "enumerated PDO therefore can not be cloned and is "
                            "being marked missing because of failure to "
                            "allocate device relations. It will be surprise "
                            "removed by pnp manager. Bus driver may continue "
                            "to function normally but will lose this child PDO",
                            pEntry->m_Pdo->GetHandle(),
                            pEntry->m_Pdo->GetDeviceObject());
                    }
                    pEntry->m_DescriptionState = DescriptionReportedMissing;
                    pEntry->m_ReportedMissingCallbackState = CallbackNeedsToBeInvoked;
                    KeReleaseSpinLock(&m_ListLock, irql);
                    break;

                case DescriptionNotPresent:
                    //
                    // We will now report the description as missing
                    //
                    DoTraceLevelMessage(
                        pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGPNP,
                        "PDO WDFDEVICE %p !devobj %p reported as missing to pnp "
                        "(by using existing relations)",
                        pEntry->m_Pdo->GetHandle(),
                        pEntry->m_Pdo->GetDeviceObject());

                    pEntry->m_DescriptionState = DescriptionReportedMissing;
                    pEntry->m_ReportedMissingCallbackState = CallbackNeedsToBeInvoked;
                    break;
                }
            }
        }

        goto Done;
    }

    RtlZeroMemory(pNewRelations, size);

    if (pPriorRelations != NULL && pPriorRelations->Count > 0) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
            "WDFCHILDLIST %p prior relations %p contained %d objects",
            GetHandle(), pPriorRelations, pPriorRelations->Count
            );

        RtlCopyMemory(pNewRelations,
                      pPriorRelations,
                      _ComputeRelationsSize(pPriorRelations->Count));
    }

    //
    // We can walk the ChildList without holding the spinlock because we set
    // m_State to ListLockedForEnum.
    //
    status = STATUS_SUCCESS;

    for (ple = m_DescriptionListHead.Flink;
         ple != &m_DescriptionListHead;
         ple = pNext) {
        pNext = ple->Flink;

        pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

        switch (pEntry->m_DescriptionState) {
        case DescriptionPresentNeedsInstantiation:
            //
            // This extension needs a device handle. Create one now.
            //
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                                "Creating PDO device object from reported device");
            if (CreateDevice(pEntry, &invalidateRelations) == FALSE) {
                break;
            }

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "PDO created successfully, WDFDEVICE %p !devobj %p",
                pEntry->m_Pdo->GetHandle(), pEntry->m_Pdo->GetDeviceObject());

            // || || Fall through || ||
            // \/ \/              \/ \/
        case DescriptionInstantiatedHasObject:

            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                                "Reporting PDO WDFDEVICE %p !devobj %p",
                                pEntry->m_Pdo->GetHandle(),
                                pEntry->m_Pdo->GetDeviceObject());

            pDevice = pEntry->m_Pdo->GetDeviceObject();
            ObReferenceObject(pDevice);
            pNewRelations->Objects[pNewRelations->Count] = pDevice;
            pNewRelations->Count++;
            break;

        case DescriptionNotPresent:
            //
            // We will now report the description as missing
            //
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "PDO WDFDEVICE %p !devobj %p reported as missing to pnp",
                pEntry->m_Pdo->GetHandle(), pEntry->m_Pdo->GetDeviceObject());

            pEntry->m_DescriptionState = DescriptionReportedMissing;
            pEntry->m_ReportedMissingCallbackState = CallbackNeedsToBeInvoked;

            break;

        case DescriptionReportedMissing:
            break;

        default:
            ASSERTMSG("Invalid description state\n", FALSE);
            break;
        }
    }

Done:
    KeAcquireSpinLock(&m_ListLock, &irql);

    //
    // Make sure that the description we just dequeued is not on the modification
    // list.
    //
    m_State = ListUnlocked;
    ProcessModificationsLocked(&freeHead);

    if (NT_SUCCESS(status)) {
        m_EnumRetries = 0;
    }

    KeReleaseSpinLock(&m_ListLock, irql);

    if (invalidateRelations) {
        //
        // We failed for some reason or other. Queue up a new attempt.
        //
        // NOTE:  no need to check m_Device->m_KnownPdo here
        //        because we are in the middle of a QDR for m_Device which means
        //        that it, or its stack, is already known to PnP.
        //
        IoInvalidateDeviceRelations(m_Device->GetPhysicalDevice(), BusRelations);
    }

    DrainFreeListHead(&freeHead);

    if (cleanupRelations) {
        if (pPriorRelations != NULL) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "Freeing prior relations %p", pPriorRelations);

            ExFreePool(pPriorRelations);
            pPriorRelations = NULL;
        }

        if (!NT_SUCCESS(status) && pNewRelations != NULL) {
            for(i = 0; i < pNewRelations->Count; i++) {
                ObDereferenceObject(pNewRelations->Objects[i]);
            }

            ExFreePool(pNewRelations);
            pNewRelations = NULL;
        }

        *DeviceRelations = pNewRelations;
    }

    return status;
}

VOID
FxChildList::InvokeReportedMissingCallback(
    VOID
    )
{
    PLIST_ENTRY ple, pNext;
    KIRQL irql;
    FxDeviceDescriptionEntry* pEntry;
    LIST_ENTRY freeHead;

    InitializeListHead(&freeHead);

    //
    // Prevent modification list processing so that we can walk the
    // description list safely.
    //
    KeAcquireSpinLock(&m_ListLock, &irql);
    m_State = ListLockedForEnum;
    KeReleaseSpinLock(&m_ListLock, irql);

    //
    // Invoke the ReportedMissing callback if present for children reported
    // missing.
    //
    for (ple = m_DescriptionListHead.Flink;
         ple != &m_DescriptionListHead;
         ple = pNext) {
        pNext = ple->Flink;

        pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

        if (pEntry->m_ReportedMissingCallbackState == CallbackNeedsToBeInvoked) {
            (pEntry->m_Pdo->GetPdoPkg())->m_DeviceReportedMissing.Invoke(
                    pEntry->m_Pdo->GetHandle());

            pEntry->m_ReportedMissingCallbackState = CallbackInvoked;
        }
    }

    KeAcquireSpinLock(&m_ListLock, &irql);
    m_State = ListUnlocked;
    ProcessModificationsLocked(&freeHead);
    KeReleaseSpinLock(&m_ListLock, irql);

    DrainFreeListHead(&freeHead);
}

VOID
FxChildList::PostParentToD0(
    VOID
    )
{
    FxDeviceDescriptionEntry* pEntry;
    PLIST_ENTRY ple;
    KIRQL irql;

    KeAcquireSpinLock(&m_ListLock, &irql);
    for (ple = m_DescriptionListHead.Flink;
         ple != &m_DescriptionListHead;
         ple = ple->Flink) {

        pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

        if (pEntry->m_PendingDeleteOnScanEnd) {
            COVERAGE_TRAP();
            continue;
        }

        if (pEntry->m_Pdo != NULL) {
            pEntry->m_Pdo->m_PkgPnp->PowerProcessEvent(PowerParentToD0);
        }
    }
    KeReleaseSpinLock(&m_ListLock, irql);
}

VOID
FxChildList::IndicateWakeStatus(
    __in NTSTATUS WaitWakeStatus
    )
/*++

Routine Description:
    Propagates the wait wake status to all the child PDOs. This will cause any
    pended wait wake requests to be completed with the given wait wake status.

Arguments:
    WaitWakeStatus - The NTSTATUS value to use for compeleting any pended wait
        wake requests on the child PDOs.

Return Value:
    None

  --*/
{
    FxDeviceDescriptionEntry* pEntry;
    PLIST_ENTRY ple;
    KIRQL irql;

    KeAcquireSpinLock(&m_ListLock, &irql);
    for (ple = m_DescriptionListHead.Flink;
         ple != &m_DescriptionListHead;
         ple = ple->Flink) {

        pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

        if (pEntry->m_PendingDeleteOnScanEnd) {
            COVERAGE_TRAP();
            continue;
        }

        if (pEntry->m_Pdo != NULL) {
            ASSERT(pEntry->m_Pdo->m_PkgPnp->m_SharedPower.m_WaitWakeOwner == TRUE);
            pEntry->m_Pdo->m_PkgPnp->PowerIndicateWaitWakeStatus(WaitWakeStatus);
        }
    }
    KeReleaseSpinLock(&m_ListLock, irql);
}

VOID
FxChildList::NotifyDeviceSurpriseRemove(
    VOID
    )
/*++

Routine Description:
    Notification through IFxStateChangeNotification that the parent device is
    being surprise removed

Arguments:
    None

Return Value:
    None

  --*/
{
    PLIST_ENTRY pCur, pNext;
    LIST_ENTRY freeHead;
    FxDeviceDescriptionEntry* pEntry;
    KIRQL irql;

    InitializeListHead(&freeHead);

    FxVerifierCheckIrqlLevel(GetDriverGlobals(), PASSIVE_LEVEL);

    KeAcquireSpinLock(&m_ListLock, &irql);

    //
    // Walk over all mod entries and remove any pending inserts. Note that
    // MarkModificationNotPresentWorker will add any such INSERTs found in the
    // mod list to the free list.
    //
    for (pCur = m_ModificationListHead.Flink;
         pCur != &m_ModificationListHead;
         pCur = pNext) {

        pNext = pCur->Flink;

        pEntry = FxDeviceDescriptionEntry::_FromModificationLink(pCur);

        if (pEntry->m_ModificationState == ModificationInsert) {
            MarkModificationNotPresentWorker(&freeHead, pEntry);
        }
    }

    //
    // Walk over all desc entries and remove them. Since
    // MarkDescriptionNotPresentWorker doesn't drain the mods,
    // we don't have to worry about any entries disappearing.
    //
    for (pCur = m_DescriptionListHead.Flink;
         pCur != &m_DescriptionListHead;
         pCur = pCur->Flink) {

        pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(pCur);

        if (pEntry->IsPresent()) {
            MarkDescriptionNotPresentWorker(pEntry, TRUE);
        }
    }

    //
    // Attempt to drain any mods we made.
    //
    ProcessModificationsLocked(&freeHead);

    //
    // Walk over all desc entries and advance them to missing.
    //
    for (pCur = m_DescriptionListHead.Flink;
         pCur != &m_DescriptionListHead;
         pCur = pCur->Flink) {

        pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(pCur);

        if (pEntry->m_DescriptionState == DescriptionNotPresent) {
            pEntry->m_DescriptionState = DescriptionReportedMissing;
        }
    }

    KeReleaseSpinLock(&m_ListLock, irql);

    DrainFreeListHead(&freeHead);
}

VOID
FxChildList::NotifyDeviceRemove(
    __inout PLONG ChildCount
    )
/*++

Routine Description:
    Notification through IFxStateChangeNotification that the parent device is
    being removed.

Arguments:
    None

Return Value:
    None

  --*/
{
    PLIST_ENTRY pCur;
    LIST_ENTRY freeHead;
    FxDeviceDescriptionEntry* pEntry;
    KIRQL irql;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                        "WDFCHILDLIST %p:  removing children", GetHandle());

    InitializeListHead(&freeHead);
    pEntry = NULL;

    FxVerifierCheckIrqlLevel(GetDriverGlobals(), PASSIVE_LEVEL);

    //
    // Surprise remove handling covers the first stage of processing.  This will
    // process the modification list, esp the insert modifications which will
    // be cleaned up.
    //
    NotifyDeviceSurpriseRemove();

    KeAcquireSpinLock(&m_ListLock, &irql);

    ProcessModificationsLocked(&freeHead);

    m_State = ListLockedForParentRemove;

    for ( ; ; ) {

        //
        // Find the first child which was not surprise removed.  If surprise
        // removed and not yet removed, then there is an outstanding handle
        // which has not yet been closed.
        //
        for (pCur = m_DescriptionListHead.Flink;
             pCur != &m_DescriptionListHead;
             pCur = pCur->Flink) {

            pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(pCur);

            ASSERT(pEntry->m_DescriptionState == DescriptionReportedMissing);

            if (pEntry->m_ProcessingSurpriseRemove == FALSE) {
                break;
            }
        }

        //
        // If we are at the end of the list, we are done
        //
        if (pCur == &m_DescriptionListHead) {
            break;
        }

        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                            "Removing entry %p, WDFDEVICE %p, PDO %p",
                            pEntry, pEntry->m_Pdo->GetHandle(),
                            pEntry->m_Pdo->GetPhysicalDevice());

        ASSERT(pEntry->m_ModificationState == ModificationUnspecified);
        RemoveEntryList(&pEntry->m_DescriptionLink);
        InitializeListHead(&pEntry->m_DescriptionLink);

        KeReleaseSpinLock(&m_ListLock, irql);

        //
        // Go through FxPkgPdo to handle cleanup.  CleanupOrphanedDevice will
        // free this entry.
        //
        ASSERT(pEntry->m_Pdo != NULL);
        ASSERT(pEntry->m_PendingDeleteOnScanEnd == FALSE);

        pEntry->m_Pdo->SetParentWaitingOnRemoval();
        InterlockedIncrement(ChildCount);

        //
        // Start the child going away
        //
        pEntry->m_Pdo->m_PkgPnp->PnpProcessEvent(PnpEventParentRemoved);

        KeAcquireSpinLock(&m_ListLock, &irql);
    }

    m_State = ListUnlocked;
    ProcessModificationsLocked(&freeHead);

    KeReleaseSpinLock(&m_ListLock, irql);

    DrainFreeListHead(&freeHead);
}

_Must_inspect_result_
NTSTATUS
FxChildList::_ValidateConfig(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_CHILD_LIST_CONFIG Config,
    __in size_t* TotalDescriptionSize
    )
{
    NTSTATUS status;

    if (Config == NULL) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Invalid Config, NULL is not allowed, %!STATUS!", status);
        return status;
    }

    if (Config->Size != sizeof(WDF_CHILD_LIST_CONFIG)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Config->Size incorrect, expected %d, got %d, %!STATUS!",
                        sizeof(WDF_CHILD_LIST_CONFIG), Config->Size, status);
        return status;
    }
    if (Config->IdentificationDescriptionSize == 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Config->IdentificationDescriptionSize must be non zero, "
                    "%!STATUS!", status);
        return status;
    }
    if (Config->EvtChildListCreateDevice == NULL) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Config->EvtChildListCreateDevice, NULL is not allowed, "
                    "%!STATUS!", status);
        return status;
    }

    return _ComputeTotalDescriptionSize(FxDriverGlobals,
                                        Config,
                                        TotalDescriptionSize);
}

_Must_inspect_result_
NTSTATUS
FxChildList::_ComputeTotalDescriptionSize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_CHILD_LIST_CONFIG Config,
    __in size_t* TotalDescriptionSize
    )
{
    size_t addressAligned, idAligned;
    NTSTATUS status;

    *TotalDescriptionSize = 0;

    //
    // FxDeviceDescriptionEntry::operator new() will allocate a block of memory
    // that is
    // size =
    //  WDF_ALIGN_SIZE_UP(sizeof(FxDeviceDescriptionEntry), sizeof(PVOID)) +
    //  WDF_ALIGN_SIZE_UP(AddressDescriptionSize, sizeof(PVOID)) +
    //  WDF_ALIGN_SIZE_UP(IdentificationDescriptionSize, sizeof(PVOID));
    //
    // Validate the size now beforehand, not every we allocate
    //
    //
    // Test for overflow
    //
    idAligned = WDF_ALIGN_SIZE_UP(Config->IdentificationDescriptionSize,
                                  sizeof(PVOID));
    if (idAligned < Config->IdentificationDescriptionSize) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Config->IdentificationDescriptionSize %d too large"
                            "%!STATUS!", Config->IdentificationDescriptionSize,
                            status);
        return status;
    }

    addressAligned = WDF_ALIGN_SIZE_UP(Config->AddressDescriptionSize,
                                       sizeof(PVOID));
    if (addressAligned < Config->AddressDescriptionSize) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                        "Config->AddressDescriptionSize %d too large, %!STATUS!",
                        Config->AddressDescriptionSize, status);
        return status;
    }

    status = RtlSizeTAdd(
        WDF_ALIGN_SIZE_UP(sizeof(FxDeviceDescriptionEntry), sizeof(PVOID)),
        idAligned,
        TotalDescriptionSize
        );
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Could not add ID description size to block size, %!STATUS!",
            status);
        return status;
    }

    status = RtlSizeTAdd(*TotalDescriptionSize,
                         addressAligned,
                         TotalDescriptionSize);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Could not add address description size to block size, %!STATUS!",
            status);
        return status;
    }

    return STATUS_SUCCESS;
}

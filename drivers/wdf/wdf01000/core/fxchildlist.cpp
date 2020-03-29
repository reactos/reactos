#include "common/fxchildlist.h"
#include "common/fxdevice.h"
#include "common/fxdeviceinit.h"
#include <ntintsafe.h>


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

    if (childList == NULL)
    {
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
    if (!NT_SUCCESS(ntStatus))
    {
        if (NULL != childList)
        {
            childList->DeleteFromFailedCreate();
        }
    }
    return ntStatus;
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

    if (idAligned < Config->IdentificationDescriptionSize)
    {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Config->IdentificationDescriptionSize %d too large"
                            "%!STATUS!", Config->IdentificationDescriptionSize,
                            status);
        return status;
    }

    addressAligned = WDF_ALIGN_SIZE_UP(Config->AddressDescriptionSize,
                                       sizeof(PVOID));

    if (addressAligned < Config->AddressDescriptionSize)
    {
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

    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Could not add ID description size to block size, %!STATUS!",
            status);
        return status;
    }

    status = RtlSizeTAdd(*TotalDescriptionSize,
                         addressAligned,
                         TotalDescriptionSize);

    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Could not add address description size to block size, %!STATUS!",
            status);
        return status;
    }

    return STATUS_SUCCESS;
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
         ple = ple->Flink)
    {
        pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

        if (pEntry->m_PendingDeleteOnScanEnd)
        {
            COVERAGE_TRAP();
            continue;
        }

        if (pEntry->m_Pdo != NULL)
        {
            pEntry->m_Pdo->m_PkgPnp->PowerProcessEvent(PowerParentToD0);
        }
    }
    KeReleaseSpinLock(&m_ListLock, irql);
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
         ple = ple->Flink)
    {
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
        (needToReportMissingChildren == FALSE || pPriorRelations != NULL))
    {
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
        if (needToReportMissingChildren)
        {
            for (ple = m_DescriptionListHead.Flink;
                 ple != &m_DescriptionListHead;
                 ple = pNext)
            {
                pNext = ple->Flink;

                pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

                if (pEntry->m_DescriptionState == DescriptionNotPresent)
                {
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
        else
        {
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
    if (pPriorRelations != NULL)
    {
        totalCount += pPriorRelations->Count;
    }

    size = _ComputeRelationsSize(totalCount);

    pNewRelations = (PDEVICE_RELATIONS)
        ExAllocatePoolWithTag(PagedPool, size, pFxDriverGlobals->Tag);

    if (pNewRelations == NULL)
    {
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
        if (m_EnumRetries <= FX_CHILD_LIST_MAX_RETRIES)
        {
            invalidateRelations = TRUE;
        }
        else
        {
            if (needToReportMissingChildren)
            {
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

        if (pPriorRelations == NULL)
        {
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
        else
        {
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
                 ple = pNext)
            {
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
                    if (m_StaticList == FALSE)
                    {
                        if (ReenumerateEntryLocked(pEntry, TRUE))
                        {
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
                    else
                    {
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

    if (pPriorRelations != NULL && pPriorRelations->Count > 0)
    {
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
         ple = pNext)
    {
        pNext = ple->Flink;

        pEntry = FxDeviceDescriptionEntry::_FromDescriptionLink(ple);

        switch (pEntry->m_DescriptionState) {
        case DescriptionPresentNeedsInstantiation:
            //
            // This extension needs a device handle. Create one now.
            //
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                                "Creating PDO device object from reported device");
            if (CreateDevice(pEntry, &invalidateRelations) == FALSE)
            {
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

    if (NT_SUCCESS(status))
    {
        m_EnumRetries = 0;
    }

    KeReleaseSpinLock(&m_ListLock, irql);

    if (invalidateRelations)
    {
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

    if (cleanupRelations)
    {
        if (pPriorRelations != NULL)
        {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "Freeing prior relations %p", pPriorRelations);

            ExFreePool(pPriorRelations);
            pPriorRelations = NULL;
        }

        if (!NT_SUCCESS(status) && pNewRelations != NULL)
        {
            for(i = 0; i < pNewRelations->Count; i++)
            {
                ObDereferenceObject(pNewRelations->Objects[i]);
            }

            ExFreePool(pNewRelations);
            pNewRelations = NULL;
        }

        *DeviceRelations = pNewRelations;
    }

    return status;
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

    if (pClone != NULL)
    {
        //
        // Check to see if the driver writer OKs the cloning, aka
        // reenumeration.
        //
        if (m_EvtChildListDeviceReenumerated != NULL &&
            m_EvtChildListDeviceReenumerated(
                GetHandle(),
                Entry->m_Pdo->GetHandle(),
                Entry->m_AddressDescription,
                pClone->m_AddressDescription) == FALSE)
        {
            //
            // The clone will be freed later by the caller by placing
            // it on the free list.
            //
            InsertTailList(FreeListHead, &pClone->m_DescriptionLink);
        }
        else
        {
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

            if (FromQDR == FALSE)
            {
                Entry->m_DescriptionState = DescriptionNotPresent;
                invalidateRelations = TRUE;
            }
        }
    }
    else
    {
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

    if (pNewEntry == NULL)
    {
        return NULL;
    }

    status = m_DeviceList->DuplicateId(pNewEntry->m_IdentificationDescription,
                                       m_IdentificationDescription);

    if (NT_SUCCESS(status) && m_DeviceList->HasAddressDescriptions())
    {
        status = m_DeviceList->DuplicateAddress(pNewEntry->m_AddressDescription,
                                                m_AddressDescription);
    }

    if (NT_SUCCESS(status))
    {
        pNewEntry->m_FoundInLastScan = TRUE;
    }
    else
    {
        //
        // Free it later
        //
        InsertTailList(FreeListHead, &pNewEntry->m_DescriptionLink);
        pNewEntry = NULL;
    }

    return pNewEntry;
}

FxDeviceDescriptionEntry::FxDeviceDescriptionEntry(
    __inout FxChildList* DeviceList,
    __in ULONG IdentificationDescriptionSize,
    __in ULONG AddressDescriptionSize
    )
{
    m_IdentificationDescription =
        (PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER) WDF_PTR_ADD_OFFSET(
        this, WDF_ALIGN_SIZE_UP(sizeof(*this), sizeof(PVOID)));

    m_IdentificationDescription->IdentificationDescriptionSize =
        IdentificationDescriptionSize;

    if (AddressDescriptionSize > 0)
    {
        m_AddressDescription =
            (PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER) WDF_PTR_ADD_OFFSET(
                m_IdentificationDescription,
                WDF_ALIGN_SIZE_UP(IdentificationDescriptionSize, sizeof(PVOID)));

        m_AddressDescription->AddressDescriptionSize =
            AddressDescriptionSize;
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

    if (p != NULL)
    {
        RtlZeroMemory(p, TotalDescriptionSize);
    }

    return p;
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
    if (IsListEmpty(&Entry->m_ModificationLink) && Entry->IsPresent())
    {
        //
        // No mods pending...
        //

        //
        // We can only go down this path if the device was reported to the system
        // b/c the stack on our PDO is the one invoking this call.
        //
        ASSERT(Entry->m_DescriptionState == DescriptionInstantiatedHasObject);

        if (FromQDR == FALSE)
        {
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
    else
    {
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
FxChildList::DrainFreeListHead(
    __inout PLIST_ENTRY FreeListHead
    )
{
    FxDeviceDescriptionEntry *pEntry;
    FxDevice* pPdo;
    PLIST_ENTRY pCur;

    while (!IsListEmpty(FreeListHead))
    {
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
             pEntry->m_ModificationState == ModificationNeedsPnpRemoval ))
        {
            pPdo = CONTAINING_RECORD(pEntry->m_IdentificationDescription,
                                     FxStaticChildDescription,
                                     Header)->Pdo;

            //
            // The pnp removal path expects that there is no modifcation pending
            // when removing the description.
            //
            if (pEntry->m_ModificationState == ModificationNeedsPnpRemoval)
            {
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
        else
        {
            CleanupDescriptions(pEntry->m_IdentificationDescription,
                               pEntry->m_AddressDescription);

            delete pEntry;
        }
    }
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
    if (m_State != ListUnlocked || m_ScanCount > 0)
    {
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
         pCur = pNext)
    {
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
    while (!IsListEmpty(&m_ModificationListHead))
    {
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

    if (invalidateRelations)
    {
        PDEVICE_OBJECT pdo;

        if (m_ScanCount)
        {
            m_InvalidationNeeded = TRUE;
        }
        else if ((pdo = m_Device->GetSafePhysicalDevice()) != NULL)
        {
            //
            // See previous usage of m_Device->m_KnownPdo for
            // comments on why a lock is not necessary when reading its value.
            //
            m_InvalidationNeeded = FALSE;
            IoInvalidateDeviceRelations(pdo, BusRelations);
        }
        else
        {
            m_InvalidationNeeded = TRUE;
        }
    }

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
       "end processing modifications on WDFCHILDLIST %p", GetObjectHandle());
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

    if (m_StaticList)
    {
        init.CreatedDevice =
            CONTAINING_RECORD(Entry->m_IdentificationDescription,
                              FxStaticChildDescription,
                              Header)->Pdo;
    }
    else
    {
        //
        // This description needs a device, create it now.
        //
        status = m_EvtCreateDevice.Invoke(
            GetHandle(), Entry->m_IdentificationDescription, &init);

        if (status == STATUS_RETRY)
        {
            if (init.CreatedDevice != NULL)
            {
                //
                // Destroy any allocations assocated with the device.
                //
                ((FxDevice*)init.CreatedDevice)->Destroy();
            }

            *InvalidateRelations = TRUE;

            return FALSE;
        }

        if (NT_SUCCESS(status))
        {
            //
            // Make sure that a framework device was actually created
            //
            if (init.CreatedDevice == NULL)
            {
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

        if (!NT_SUCCESS(status))
        {
            if (init.CreatedDevice != NULL)
            {
                FxDevice* createdDevice;
                createdDevice = (FxDevice*)init.CreatedDevice;

                KeAcquireSpinLock(&m_ListLock, &irql);
                //
                // Set to missing so that when the pnp machine evaluates whether the
                // PDO has been reported missing or not, it chooses missing and
                // deletes the PDO immediately.
                //
                Entry->m_DescriptionState = DescriptionReportedMissing;

                if (Entry->m_ModificationState != ModificationUnspecified)
                {
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

                ASSERT(createdDevice->IsPnp());
                ASSERT(createdDevice->GetDevicePnpState() == WdfDevStatePnpInit);
                ASSERT(createdDevice->GetPdoPkg()->m_Description != NULL);

                ASSERT(Entry->m_Pdo == NULL);

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "WDFDEVICE %p !devobj %p created, but EvtChildListCreateDevice "
                    "returned status %!STATUS!", createdDevice->GetHandle(),
                    createdDevice->GetDeviceObject(), status);

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
                (void) createdDevice->DeleteDeviceFromFailedCreate(
                                                                status,
                                                                TRUE);

                init.CreatedDevice = NULL;
            }
            else
            {
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
    Entry->m_Pdo = (FxDevice*)init.CreatedDevice;
    Entry->m_DescriptionState = DescriptionInstantiatedHasObject;

    return TRUE;
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

    if (ModificationCanBeQueued)
    {
        if (IsListEmpty(&DescriptionEntry->m_ModificationLink))
        {
            queueMod = TRUE;
        }
        else
        {
            //
            // If the modification is queued, it must be removal
            //
            ASSERT(DescriptionEntry->m_ModificationState ==
                                                      ModificationRemoveNotify);
            ASSERT(DescriptionEntry->m_FoundInLastScan == FALSE);
        }
    }
    else
    {
        queueMod = TRUE;
    }

    if (queueMod)
    {
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

/*++

Copyright (C) Microsoft Corporation, 1991 - 2010

Module Name:

    xferpkt.c

Abstract:

    Packet routines for CLASSPNP

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "classp.h"
#include "debug.h"

#ifdef DEBUG_USE_WPP
#include "xferpkt.tmh"
#endif

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, InitializeTransferPackets)
    #pragma alloc_text(PAGE, DestroyAllTransferPackets)
    #pragma alloc_text(PAGE, SetupEjectionTransferPacket)
    #pragma alloc_text(PAGE, SetupModeSenseTransferPacket)
    #pragma alloc_text(PAGE, CleanupTransferPacketToWorkingSetSizeWorker)
    #pragma alloc_text(PAGE, ClasspSetupPopulateTokenTransferPacket)
#endif

/*
 *  InitializeTransferPackets
 *
 *      Allocate/initialize TRANSFER_PACKETs and related resources.
 */
NTSTATUS InitializeTransferPackets(PDEVICE_OBJECT Fdo)
{
    PCOMMON_DEVICE_EXTENSION commonExt = Fdo->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    PSTORAGE_ADAPTER_DESCRIPTOR adapterDesc = commonExt->PartitionZeroExtension->AdapterDescriptor;
    PSTORAGE_DEVICE_IO_CAPABILITY_DESCRIPTOR devIoCapabilityDesc = NULL;
    STORAGE_PROPERTY_ID propertyId;
    OSVERSIONINFOEXW osVersionInfo;
    ULONG hwMaxPages;
    ULONG arraySize;
    ULONG index;
    ULONG maxOutstandingIOPerLUN;
    ULONG minWorkingSetTransferPackets;
    ULONG maxWorkingSetTransferPackets;

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    //  Precompute the maximum transfer length
    //
    NT_ASSERT(adapterDesc->MaximumTransferLength);

    hwMaxPages = adapterDesc->MaximumPhysicalPages ? adapterDesc->MaximumPhysicalPages-1 : 0;

    fdoData->HwMaxXferLen = MIN(adapterDesc->MaximumTransferLength, hwMaxPages << PAGE_SHIFT);
    fdoData->HwMaxXferLen = MAX(fdoData->HwMaxXferLen, PAGE_SIZE);

    //
    // Allocate per-node free packet lists
    //
    arraySize = KeQueryHighestNodeNumber() + 1;
    fdoData->FreeTransferPacketsLists =
        ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                              sizeof(PNL_SLIST_HEADER) * arraySize,
                              CLASS_TAG_PRIVATE_DATA);

    if (fdoData->FreeTransferPacketsLists == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    for (index = 0; index < arraySize; index++) {
        InitializeSListHead(&(fdoData->FreeTransferPacketsLists[index].SListHeader));
        fdoData->FreeTransferPacketsLists[index].NumTotalTransferPackets = 0;
        fdoData->FreeTransferPacketsLists[index].NumFreeTransferPackets = 0;
    }

    InitializeListHead(&fdoData->AllTransferPacketsList);

    //
    // Set the packet threshold numbers based on the Windows Client or Server SKU.
    //

    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    status = RtlGetVersion((POSVERSIONINFOW) &osVersionInfo);

    NT_ASSERT( NT_SUCCESS(status));

    //
    // Retrieve info on IO capability supported by port drivers
    //

    propertyId = StorageDeviceIoCapabilityProperty;
    status = ClassGetDescriptor(fdoExt->CommonExtension.LowerDeviceObject,
                                &propertyId,
                                (PVOID *)&devIoCapabilityDesc);

    if (NT_SUCCESS(status) && (devIoCapabilityDesc != NULL)) {
        maxOutstandingIOPerLUN = devIoCapabilityDesc->LunMaxIoCount;
        FREE_POOL(devIoCapabilityDesc);

#if DBG
        fdoData->MaxOutstandingIOPerLUN = maxOutstandingIOPerLUN;
#endif

    } else {
        maxOutstandingIOPerLUN = MAX_OUTSTANDING_IO_PER_LUN_DEFAULT;

#if DBG
        fdoData->MaxOutstandingIOPerLUN = 0;
#endif

    }

    //
    // StorageDeviceIoCapabilityProperty support is optional so
    // ignore any failures.
    //

    status = STATUS_SUCCESS;


    if ((osVersionInfo.wProductType != VER_NT_DOMAIN_CONTROLLER) &&
        (osVersionInfo.wProductType != VER_NT_SERVER)) {

        // this is Client SKU

            minWorkingSetTransferPackets = MIN_WORKINGSET_TRANSFER_PACKETS_Client;

        // Note: the reason we use max here is to guarantee a reasonable large max number
        // in the case where the port driver may return a very small supported outstanding
        // IOs. For example, even EMMC drive only reports 1 outstanding IO supported, we
        // may still want to set this value to be at least
        // MAX_WORKINGSET_TRANSFER_PACKETS_Client.
        maxWorkingSetTransferPackets = max(MAX_WORKINGSET_TRANSFER_PACKETS_Client,
                                           2 * maxOutstandingIOPerLUN);

    } else {

        // this is Server SKU
        // Note: the addition max here to make sure we set the min to be at least
        // MIN_WORKINGSET_TRANSFER_PACKETS_Server_LowerBound no matter what maxOutstandingIOPerLUN
        // reported. We shouldn't set this value to be smaller than client system.
        // In other words, the minWorkingSetTransferPackets for server will always between
        // MIN_WORKINGSET_TRANSFER_PACKETS_Server_LowerBound and MIN_WORKINGSET_TRANSFER_PACKETS_Server_UpperBound

        minWorkingSetTransferPackets =
            max(MIN_WORKINGSET_TRANSFER_PACKETS_Server_LowerBound,
                min(MIN_WORKINGSET_TRANSFER_PACKETS_Server_UpperBound,
                    maxOutstandingIOPerLUN));

        maxWorkingSetTransferPackets = max(MAX_WORKINGSET_TRANSFER_PACKETS_Server,
                                           2 * maxOutstandingIOPerLUN);
    }


    fdoData->LocalMinWorkingSetTransferPackets = minWorkingSetTransferPackets;
    fdoData->LocalMaxWorkingSetTransferPackets = maxWorkingSetTransferPackets;

    //
    //  Allow class driver to override the settings
    //
    if (commonExt->DriverExtension->WorkingSet != NULL) {
        PCLASS_WORKING_SET workingSet = commonExt->DriverExtension->WorkingSet;

        // override only if non-zero
        if (workingSet->XferPacketsWorkingSetMinimum != 0)
        {
            fdoData->LocalMinWorkingSetTransferPackets = workingSet->XferPacketsWorkingSetMinimum;
            // adjust maximum upwards if needed
            if (fdoData->LocalMaxWorkingSetTransferPackets < fdoData->LocalMinWorkingSetTransferPackets)
            {
                fdoData->LocalMaxWorkingSetTransferPackets = fdoData->LocalMinWorkingSetTransferPackets;
            }
        }
        // override only if non-zero
        if (workingSet->XferPacketsWorkingSetMaximum != 0)
        {
            fdoData->LocalMaxWorkingSetTransferPackets = workingSet->XferPacketsWorkingSetMaximum;
            // adjust minimum downwards if needed
            if (fdoData->LocalMinWorkingSetTransferPackets > fdoData->LocalMaxWorkingSetTransferPackets)
            {
                fdoData->LocalMinWorkingSetTransferPackets = fdoData->LocalMaxWorkingSetTransferPackets;
            }
        }
        // that's all the adjustments required/allowed
    } // end working set size special code

    for (index = 0; index < arraySize; index++) {
        while (fdoData->FreeTransferPacketsLists[index].NumFreeTransferPackets < MIN_INITIAL_TRANSFER_PACKETS){
            PTRANSFER_PACKET pkt = NewTransferPacket(Fdo);
            if (pkt) {
                InterlockedIncrement((volatile LONG *)&(fdoData->FreeTransferPacketsLists[index].NumTotalTransferPackets));
                pkt->AllocateNode = index;
                EnqueueFreeTransferPacket(Fdo, pkt);
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        }
        fdoData->FreeTransferPacketsLists[index].DbgPeakNumTransferPackets = fdoData->FreeTransferPacketsLists[index].NumTotalTransferPackets;
    }

    //
    //  Pre-initialize our SCSI_REQUEST_BLOCK template with all
    //  the constant fields.  This will save a little time for each xfer.
    //  NOTE: a CdbLength field of 10 may not always be appropriate
    //

    if (NT_SUCCESS(status))  {
        if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
            ULONG ByteSize = 0;

    #if (NTDDI_VERSION >= NTDDI_WINBLUE)
            if ((fdoExt->MiniportDescriptor != NULL) &&
                (fdoExt->MiniportDescriptor->Size >= RTL_SIZEOF_THROUGH_FIELD(STORAGE_MINIPORT_DESCRIPTOR, ExtraIoInfoSupported)) &&
                (fdoExt->MiniportDescriptor->ExtraIoInfoSupported == TRUE)) {
                status = CreateStorageRequestBlock((PSTORAGE_REQUEST_BLOCK *)&fdoData->SrbTemplate,
                                                    fdoExt->AdapterDescriptor->AddressType,
                                                    DefaultStorageRequestBlockAllocateRoutine,
                                                    &ByteSize,
                                                    2,
                                                    SrbExDataTypeScsiCdb16,
                                                    SrbExDataTypeIoInfo
                                                  );
            } else {
                status = CreateStorageRequestBlock((PSTORAGE_REQUEST_BLOCK *)&fdoData->SrbTemplate,
                                                    fdoExt->AdapterDescriptor->AddressType,
                                                    DefaultStorageRequestBlockAllocateRoutine,
                                                    &ByteSize,
                                                    1,
                                                    SrbExDataTypeScsiCdb16
                                                  );
            }
    #else
            status = CreateStorageRequestBlock((PSTORAGE_REQUEST_BLOCK *)&fdoData->SrbTemplate,
                                                fdoExt->AdapterDescriptor->AddressType,
                                                DefaultStorageRequestBlockAllocateRoutine,
                                                &ByteSize,
                                                1,
                                                SrbExDataTypeScsiCdb16
                                                );
    #endif
            if (NT_SUCCESS(status)) {
                ((PSTORAGE_REQUEST_BLOCK) fdoData->SrbTemplate)->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
            } else {
                NT_ASSERT(FALSE);
            }
        } else {
            fdoData->SrbTemplate = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(SCSI_REQUEST_BLOCK), '-brs');
            if (fdoData->SrbTemplate == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                RtlZeroMemory(fdoData->SrbTemplate, sizeof(SCSI_REQUEST_BLOCK));
                fdoData->SrbTemplate->Length = sizeof(SCSI_REQUEST_BLOCK);
                fdoData->SrbTemplate->Function = SRB_FUNCTION_EXECUTE_SCSI;
            }
        }
    }

    if (status == STATUS_SUCCESS) {
        SrbSetRequestAttribute(fdoData->SrbTemplate, SRB_SIMPLE_TAG_REQUEST);
        SrbSetSenseInfoBufferLength(fdoData->SrbTemplate, SENSE_BUFFER_SIZE_EX);
        SrbSetCdbLength(fdoData->SrbTemplate, 10);
    }

    return status;
}


VOID DestroyAllTransferPackets(PDEVICE_OBJECT Fdo)
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    TRANSFER_PACKET *pkt;
    ULONG index;
    ULONG arraySize;

    PAGED_CODE();

    //
    // fdoData->FreeTransferPacketsLists could be NULL if
    // there was an error during start device.
    //
    if (fdoData->FreeTransferPacketsLists != NULL) {

        NT_ASSERT(IsListEmpty(&fdoData->DeferredClientIrpList));

        arraySize = KeQueryHighestNodeNumber() + 1;
        for (index = 0; index < arraySize; index++) {
            pkt = DequeueFreeTransferPacketEx(Fdo, FALSE, index);
            while (pkt) {
                DestroyTransferPacket(pkt);
                InterlockedDecrement((volatile LONG *)&(fdoData->FreeTransferPacketsLists[index].NumTotalTransferPackets));
                pkt = DequeueFreeTransferPacketEx(Fdo, FALSE, index);
            }

            NT_ASSERT(fdoData->FreeTransferPacketsLists[index].NumTotalTransferPackets == 0);
        }
    }

    FREE_POOL(fdoData->SrbTemplate);
}

__drv_allocatesMem(Mem)
#ifdef _MSC_VER
#pragma warning(suppress:28195) // This function may not allocate memory in some error cases.
#endif
PTRANSFER_PACKET NewTransferPacket(PDEVICE_OBJECT Fdo)
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    PTRANSFER_PACKET newPkt = NULL;
    ULONG transferLength = (ULONG)-1;
    NTSTATUS status = STATUS_SUCCESS;

    if (NT_SUCCESS(status)) {
        status = RtlULongAdd(fdoData->HwMaxXferLen, PAGE_SIZE, &transferLength);
        if (!NT_SUCCESS(status)) {

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW, "Integer overflow in calculating transfer packet size."));
            status = STATUS_INTEGER_OVERFLOW;
        }
    }

    /*
     *  Allocate the actual packet.
     */
    if (NT_SUCCESS(status)) {
        newPkt = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(TRANSFER_PACKET), 'pnPC');
        if (newPkt == NULL) {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_RW, "Failed to allocate transfer packet."));
            status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            RtlZeroMemory(newPkt, sizeof(TRANSFER_PACKET));
            newPkt->AllocateNode = KeGetCurrentNodeNumber();
            if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
                if ((fdoExt->MiniportDescriptor != NULL) &&
                    (fdoExt->MiniportDescriptor->Size >= RTL_SIZEOF_THROUGH_FIELD(STORAGE_MINIPORT_DESCRIPTOR, ExtraIoInfoSupported)) &&
                    (fdoExt->MiniportDescriptor->ExtraIoInfoSupported == TRUE)) {
                    status = CreateStorageRequestBlock((PSTORAGE_REQUEST_BLOCK *)&newPkt->Srb,
                                        fdoExt->AdapterDescriptor->AddressType,
                                        DefaultStorageRequestBlockAllocateRoutine,
                                        NULL,
                                        2,
                                        SrbExDataTypeScsiCdb16,
                                        SrbExDataTypeIoInfo
                                        );
                } else {
                    status = CreateStorageRequestBlock((PSTORAGE_REQUEST_BLOCK *)&newPkt->Srb,
                                        fdoExt->AdapterDescriptor->AddressType,
                                        DefaultStorageRequestBlockAllocateRoutine,
                                        NULL,
                                        1,
                                        SrbExDataTypeScsiCdb16
                                        );
                }
#else
                status = CreateStorageRequestBlock((PSTORAGE_REQUEST_BLOCK *)&newPkt->Srb,
                                    fdoExt->AdapterDescriptor->AddressType,
                                    DefaultStorageRequestBlockAllocateRoutine,
                                    NULL,
                                    1,
                                    SrbExDataTypeScsiCdb16
                                    );
#endif
            } else {
#ifdef _MSC_VER
#pragma prefast(suppress:6014, "The allocated memory that Pkt->Srb points to will be freed in DestroyTransferPacket().")
#endif
                newPkt->Srb = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(SCSI_REQUEST_BLOCK), '-brs');
                if (newPkt->Srb == NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }

            }

            if (status != STATUS_SUCCESS)
            {
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_RW, "Failed to allocate SRB."));
                FREE_POOL(newPkt);
            }
        }
    }

    /*
     *  Allocate Irp for the packet.
     */
    if (NT_SUCCESS(status) && newPkt != NULL) {
        newPkt->Irp = IoAllocateIrp(Fdo->StackSize, FALSE);
        if (newPkt->Irp == NULL) {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_RW, "Failed to allocate IRP for transfer packet."));
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /*
     * Allocate a MDL.  Add one page to the length to insure an extra page
     * entry is allocated if the buffer does not start on page boundaries.
     */
    if (NT_SUCCESS(status) && newPkt != NULL) {

        NT_ASSERT(transferLength != (ULONG)-1);

        newPkt->PartialMdl = IoAllocateMdl(NULL,
                                           transferLength,
                                           FALSE,
                                           FALSE,
                                           NULL);
        if (newPkt->PartialMdl == NULL) {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_RW, "Failed to allocate MDL for transfer packet."));
            status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            NT_ASSERT(newPkt->PartialMdl->Size >= (CSHORT)(sizeof(MDL) + BYTES_TO_PAGES(fdoData->HwMaxXferLen) * sizeof(PFN_NUMBER)));
        }

    }

    /*
     * Allocate per-packet retry history, if required
     */
    if (NT_SUCCESS(status) &&
        (fdoData->InterpretSenseInfo != NULL) &&
        (newPkt != NULL)
        ) {
        // attempt to allocate also the history
        ULONG historyByteCount = 0;

        // SAL annotation and ClassInitializeEx() should both catch this case
        NT_ASSERT(fdoData->InterpretSenseInfo->HistoryCount != 0);
        _Analysis_assume_(fdoData->InterpretSenseInfo->HistoryCount != 0);

        historyByteCount = sizeof(SRB_HISTORY_ITEM) * fdoData->InterpretSenseInfo->HistoryCount;
        historyByteCount += sizeof(SRB_HISTORY) - sizeof(SRB_HISTORY_ITEM);

        newPkt->RetryHistory = (PSRB_HISTORY)ExAllocatePoolWithTag(NonPagedPoolNx, historyByteCount, 'hrPC');

        if (newPkt->RetryHistory == NULL) {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_RW, "Failed to allocate MDL for transfer packet."));
            status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            // call this routine directly once since it's the first initialization of
            // the structure and the internal maximum count field is not yet setup.
            HistoryInitializeRetryLogs(newPkt->RetryHistory, fdoData->InterpretSenseInfo->HistoryCount);
        }
    }

    /*
     *  Enqueue the packet in our static AllTransferPacketsList
     *  (just so we can find it during debugging if its stuck somewhere).
     */
    if (NT_SUCCESS(status) && newPkt != NULL)
    {
        KIRQL oldIrql;
        newPkt->Fdo = Fdo;
#if DBG
        newPkt->DbgPktId = InterlockedIncrement((volatile LONG *)&fdoData->DbgMaxPktId);
#endif
        KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);
        InsertTailList(&fdoData->AllTransferPacketsList, &newPkt->AllPktsListEntry);
        KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);

    } else {
        // free any resources acquired above (in reverse order)
        if (newPkt != NULL) {
            FREE_POOL(newPkt->RetryHistory);
            if (newPkt->PartialMdl != NULL) { IoFreeMdl(newPkt->PartialMdl); }
            if (newPkt->Irp        != NULL) { IoFreeIrp(newPkt->Irp);        }
            if (newPkt->Srb        != NULL) { FREE_POOL(newPkt->Srb);        }
            FREE_POOL(newPkt);
        }
    }

    return newPkt;
}


/*
 *  DestroyTransferPacket
 *
 */
VOID DestroyTransferPacket(_In_ __drv_freesMem(mem) PTRANSFER_PACKET Pkt)
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Pkt->Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    KIRQL oldIrql;

    NT_ASSERT(!Pkt->SlistEntry.Next);
//    NT_ASSERT(!Pkt->OriginalIrp);

    KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);

    /*
     *  Delete the packet from our all-packets queue.
     */
    NT_ASSERT(!IsListEmpty(&Pkt->AllPktsListEntry));
    NT_ASSERT(!IsListEmpty(&fdoData->AllTransferPacketsList));
    RemoveEntryList(&Pkt->AllPktsListEntry);
    InitializeListHead(&Pkt->AllPktsListEntry);

    KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);

    IoFreeMdl(Pkt->PartialMdl);
    IoFreeIrp(Pkt->Irp);
    FREE_POOL(Pkt->RetryHistory);
    FREE_POOL(Pkt->Srb);
    FREE_POOL(Pkt);
}


VOID EnqueueFreeTransferPacket(PDEVICE_OBJECT Fdo, __drv_aliasesMem PTRANSFER_PACKET Pkt)
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    ULONG allocateNode;
    KIRQL oldIrql;

    NT_ASSERT(!Pkt->SlistEntry.Next);

    allocateNode = Pkt->AllocateNode;
    InterlockedPushEntrySList(&(fdoData->FreeTransferPacketsLists[allocateNode].SListHeader), &Pkt->SlistEntry);
    InterlockedIncrement((volatile LONG *)&(fdoData->FreeTransferPacketsLists[allocateNode].NumFreeTransferPackets));

    /*
     *  If the total number of packets is larger than LocalMinWorkingSetTransferPackets,
     *  that means that we've been in stress.  If all those packets are now
     *  free, then we are now out of stress and can free the extra packets.
     *  Attempt to free down to LocalMaxWorkingSetTransferPackets immediately, and
     *  down to LocalMinWorkingSetTransferPackets lazily (one at a time).
     *  However, since we're at DPC, do this is a work item. If the device is removed
     *  or we are unable to allocate the work item, do NOT free more than
     *  MAX_CLEANUP_TRANSFER_PACKETS_AT_ONCE. Subsequent IO completions will end up freeing
     *  up the rest, even if it is MAX_CLEANUP_TRANSFER_PACKETS_AT_ONCE at a time.
     */
    if (fdoData->FreeTransferPacketsLists[allocateNode].NumFreeTransferPackets >=
        fdoData->FreeTransferPacketsLists[allocateNode].NumTotalTransferPackets) {

        /*
         *  1.  Immediately snap down to our UPPER threshold.
         */
        if (fdoData->FreeTransferPacketsLists[allocateNode].NumTotalTransferPackets >
            fdoData->LocalMaxWorkingSetTransferPackets) {

            ULONG isRemoved;
            PIO_WORKITEM workItem = NULL;

            workItem = IoAllocateWorkItem(Fdo);

            //
            // Acquire a remove lock in order to make sure the device object and its
            // private data structures will exist when the workitem fires.
            // The remove lock will be released by the workitem (CleanupTransferPacketToWorkingSetSize).
            //
            isRemoved = ClassAcquireRemoveLock(Fdo, (PIRP)workItem);

            if (workItem && !isRemoved) {

                TracePrint((TRACE_LEVEL_INFORMATION,
                            TRACE_FLAG_GENERAL,
                            "EnqueueFreeTransferPacket: Device (%p), queuing work item to clean up free transfer packets.\n",
                            Fdo));

                //
                // Queue a work item to trim down the total number of transfer packets to with the
                // working size.
                //
                IoQueueWorkItemEx(workItem, CleanupTransferPacketToWorkingSetSizeWorker, DelayedWorkQueue, (PVOID) allocateNode);

            } else {

                if (workItem) {
                    IoFreeWorkItem(workItem);
                }

                if (isRemoved != REMOVE_COMPLETE) {
                    ClassReleaseRemoveLock(Fdo, (PIRP)workItem);
                }

                TracePrint((TRACE_LEVEL_ERROR,
                            TRACE_FLAG_GENERAL,
                            "EnqueueFreeTransferPacket: Device (%p), Failed to allocate memory for the work item.\n",
                            Fdo));

                CleanupTransferPacketToWorkingSetSize(Fdo, TRUE, allocateNode);
            }
        }

        /*
         *  2.  Lazily work down to our LOWER threshold (by only freeing one packet at a time).
         */
        if (fdoData->FreeTransferPacketsLists[allocateNode].NumTotalTransferPackets >
            fdoData->LocalMinWorkingSetTransferPackets){
            /*
             *  Check the counter again with lock held.  This eliminates a race condition
             *  while still allowing us to not grab the spinlock in the common codepath.
             *
             *  Note that the spinlock does not synchronize with threads dequeuing free
             *  packets to send (DequeueFreeTransferPacket does that with a lightweight
             *  interlocked exchange); the spinlock prevents multiple threads in this function
             *  from deciding to free too many extra packets at once.
             */
            PTRANSFER_PACKET pktToDelete = NULL;

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_RW, "Exiting stress, lazily freeing one of %d/%d packets from node %d.",
                fdoData->FreeTransferPacketsLists[allocateNode].NumTotalTransferPackets,
                fdoData->LocalMinWorkingSetTransferPackets,
                allocateNode));

            KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);
            if ((fdoData->FreeTransferPacketsLists[allocateNode].NumFreeTransferPackets >=
                fdoData->FreeTransferPacketsLists[allocateNode].NumTotalTransferPackets) &&
                (fdoData->FreeTransferPacketsLists[allocateNode].NumTotalTransferPackets >
                fdoData->LocalMinWorkingSetTransferPackets)){

                pktToDelete = DequeueFreeTransferPacketEx(Fdo, FALSE, allocateNode);
                if (pktToDelete) {
                    InterlockedDecrement((volatile LONG *)&(fdoData->FreeTransferPacketsLists[allocateNode].NumTotalTransferPackets));
                } else {
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_RW,
                        "Extremely unlikely condition (non-fatal): %d packets dequeued at once for Fdo %p. NumTotalTransferPackets=%d (2). Node=%d",
                        fdoData->LocalMinWorkingSetTransferPackets,
                        Fdo,
                        fdoData->FreeTransferPacketsLists[allocateNode].NumTotalTransferPackets,
                        allocateNode));
                }
            }
            KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);

            if (pktToDelete) {
                DestroyTransferPacket(pktToDelete);
            }
        }

    }

}

PTRANSFER_PACKET DequeueFreeTransferPacket(PDEVICE_OBJECT Fdo, BOOLEAN AllocIfNeeded)
{
    return DequeueFreeTransferPacketEx(Fdo, AllocIfNeeded, KeGetCurrentNodeNumber());
}

PTRANSFER_PACKET DequeueFreeTransferPacketEx(
    _In_ PDEVICE_OBJECT Fdo,
    _In_ BOOLEAN AllocIfNeeded,
    _In_ ULONG Node)
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    PTRANSFER_PACKET pkt;
    PSLIST_ENTRY slistEntry;

    slistEntry = InterlockedPopEntrySList(&(fdoData->FreeTransferPacketsLists[Node].SListHeader));

    if (slistEntry) {
        slistEntry->Next = NULL;
        pkt = CONTAINING_RECORD(slistEntry, TRANSFER_PACKET, SlistEntry);
        InterlockedDecrement((volatile LONG *)&(fdoData->FreeTransferPacketsLists[Node].NumFreeTransferPackets));

        // when dequeuing the packet, also reset the history data
        HISTORYINITIALIZERETRYLOGS(pkt);

    } else {
        if (AllocIfNeeded) {
            /*
             *  We are in stress and have run out of lookaside packets.
             *  In order to service the current transfer,
             *  allocate an extra packet.
             *  We will free it lazily when we are out of stress.
             */
            pkt = NewTransferPacket(Fdo);
            if (pkt) {
                InterlockedIncrement((volatile LONG *)&fdoData->FreeTransferPacketsLists[Node].NumTotalTransferPackets);
                fdoData->FreeTransferPacketsLists[Node].DbgPeakNumTransferPackets =
                    max(fdoData->FreeTransferPacketsLists[Node].DbgPeakNumTransferPackets,
                        fdoData->FreeTransferPacketsLists[Node].NumTotalTransferPackets);
            } else {
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_RW, "DequeueFreeTransferPacket: packet allocation failed"));
            }
        } else {
            pkt = NULL;
        }
    }

    return pkt;
}



/*
 *  SetupReadWriteTransferPacket
 *
 *        This function is called once to set up the first attempt to send a packet.
 *        It is not called before a retry, as SRB fields may be modified for the retry.
 *
 *      Set up the Srb of the TRANSFER_PACKET for the transfer.
 *        The Irp is set up in SubmitTransferPacket because it must be reset
 *        for each packet submission.
 */
VOID SetupReadWriteTransferPacket(  PTRANSFER_PACKET Pkt,
                                    PVOID Buf,
                                    ULONG Len,
                                    LARGE_INTEGER DiskLocation,
                                    PIRP OriginalIrp)
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Pkt->Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = Pkt->Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    PIO_STACK_LOCATION origCurSp = IoGetCurrentIrpStackLocation(OriginalIrp);
    UCHAR majorFunc = origCurSp->MajorFunction;
    LARGE_INTEGER logicalBlockAddr;
    ULONG numTransferBlocks;
    PCDB pCdb;
    ULONG srbLength;
    ULONG timeoutValue = fdoExt->TimeOutValue;

    logicalBlockAddr.QuadPart = Int64ShrlMod32(DiskLocation.QuadPart, fdoExt->SectorShift);
    numTransferBlocks = Len >> fdoExt->SectorShift;

    /*
     * This field is useful when debugging, since low-memory conditions are
     * handled differently for CDROM (which is the only driver using StartIO)
     */
    Pkt->DriverUsesStartIO = (commonExtension->DriverExtension->InitData.ClassStartIo != NULL);

    /*
     *  Slap the constant SRB fields in from our pre-initialized template.
     *  We'll then only have to fill in the unique fields for this transfer.
     *  Tell lower drivers to sort the SRBs by the logical block address
     *  so that disk seeks are minimized.
     */
    if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbLength = ((PSTORAGE_REQUEST_BLOCK) fdoData->SrbTemplate)->SrbLength;
        NT_ASSERT(((PSTORAGE_REQUEST_BLOCK) Pkt->Srb)->SrbLength >= srbLength);
    } else {
        srbLength = fdoData->SrbTemplate->Length;
    }
    RtlCopyMemory(Pkt->Srb, fdoData->SrbTemplate, srbLength); // copies _contents_ of SRB blocks
    SrbSetDataBuffer(Pkt->Srb, Buf);
    SrbSetDataTransferLength(Pkt->Srb, Len);
    SrbSetQueueSortKey(Pkt->Srb, logicalBlockAddr.LowPart);
    if (logicalBlockAddr.QuadPart > 0xFFFFFFFF) {
        //
        // If the requested LBA is more than max ULONG set the
        // QueueSortKey to the maximum value, so that these
        // requests can be added towards the end of the queue.
        //

        SrbSetQueueSortKey(Pkt->Srb, 0xFFFFFFFF);
    }
    SrbSetOriginalRequest(Pkt->Srb, Pkt->Irp);
    SrbSetSenseInfoBuffer(Pkt->Srb, &Pkt->SrbErrorSenseData);


    SrbSetTimeOutValue(Pkt->Srb, timeoutValue);

    /*
     *  Arrange values in CDB in big-endian format.
     */
    pCdb = SrbGetCdb(Pkt->Srb);
    if (pCdb) {
        if (TEST_FLAG(fdoExt->DeviceFlags, DEV_USE_16BYTE_CDB)) {
            REVERSE_BYTES_QUAD(&pCdb->CDB16.LogicalBlock, &logicalBlockAddr);
            REVERSE_BYTES(&pCdb->CDB16.TransferLength, &numTransferBlocks);
            pCdb->CDB16.OperationCode = (majorFunc==IRP_MJ_READ) ? SCSIOP_READ16 : SCSIOP_WRITE16;
            SrbSetCdbLength(Pkt->Srb, 16);
        } else {
            pCdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&logicalBlockAddr.LowPart)->Byte3;
            pCdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&logicalBlockAddr.LowPart)->Byte2;
            pCdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&logicalBlockAddr.LowPart)->Byte1;
            pCdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&logicalBlockAddr.LowPart)->Byte0;
            pCdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&numTransferBlocks)->Byte1;
            pCdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&numTransferBlocks)->Byte0;
            pCdb->CDB10.OperationCode = (majorFunc==IRP_MJ_READ) ? SCSIOP_READ : SCSIOP_WRITE;
        }
    }

    /*
     *  Set SRB and IRP flags
     */
    SrbAssignSrbFlags(Pkt->Srb, fdoExt->SrbFlags);
    if (TEST_FLAG(OriginalIrp->Flags, IRP_PAGING_IO) ||
        TEST_FLAG(OriginalIrp->Flags, IRP_SYNCHRONOUS_PAGING_IO)){
        SrbSetSrbFlags(Pkt->Srb, SRB_CLASS_FLAGS_PAGING);
    }
    SrbSetSrbFlags(Pkt->Srb, (majorFunc==IRP_MJ_READ) ? SRB_FLAGS_DATA_IN : SRB_FLAGS_DATA_OUT);

    /*
     *  Allow caching only if this is not a write-through request.
     *  If write-through and caching is enabled on the device, force
     *  media access.
     *  Ignore SL_WRITE_THROUGH for reads; it's only set because the file handle was opened with WRITE_THROUGH.
     */
    if ((majorFunc == IRP_MJ_WRITE) && TEST_FLAG(origCurSp->Flags, SL_WRITE_THROUGH) && pCdb) {
        pCdb->CDB10.ForceUnitAccess = fdoExt->CdbForceUnitAccess;
    } else {
        SrbSetSrbFlags(Pkt->Srb, SRB_FLAGS_ADAPTER_CACHE_ENABLE);
    }

    /*
     *  Remember the buf and len in the SRB because miniports
     *  can overwrite SRB.DataTransferLength and we may need it again
     *  for the retry.
     */
    Pkt->BufPtrCopy = Buf;
    Pkt->BufLenCopy = Len;
    Pkt->TargetLocationCopy = DiskLocation;

    Pkt->OriginalIrp = OriginalIrp;
    Pkt->NumRetries = fdoData->MaxNumberOfIoRetries;
    Pkt->SyncEventPtr = NULL;
    Pkt->CompleteOriginalIrpWhenLastPacketCompletes = TRUE;
#if !defined(__REACTOS__) && NTDDI_VERSION >= NTDDI_WINBLUE
    Pkt->NumIoTimeoutRetries = fdoData->MaxNumberOfIoRetries;
    Pkt->NumThinProvisioningRetries = 0;
#endif


    if (pCdb) {
        DBGLOGFLUSHINFO(fdoData, TRUE, (BOOLEAN)(pCdb->CDB10.ForceUnitAccess), FALSE);
    } else {
        DBGLOGFLUSHINFO(fdoData, TRUE, FALSE, FALSE);
    }
}


/*
 *  SubmitTransferPacket
 *
 *        Set up the IRP for the TRANSFER_PACKET submission and send it down.
 */
NTSTATUS SubmitTransferPacket(PTRANSFER_PACKET Pkt)
{
    PCOMMON_DEVICE_EXTENSION commonExtension = Pkt->Fdo->DeviceExtension;
    PDEVICE_OBJECT nextDevObj = commonExtension->LowerDeviceObject;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Pkt->Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;
    BOOLEAN idleRequest = FALSE;
    PIO_STACK_LOCATION nextSp;

    NT_ASSERT(Pkt->Irp->CurrentLocation == Pkt->Irp->StackCount+1);

    /*
     *  Attach the SRB to the IRP.
     *  The reused IRP's stack location has to be rewritten for each retry
     *  call because IoCompleteRequest clears the stack locations.
     */
    IoReuseIrp(Pkt->Irp, STATUS_NOT_SUPPORTED);


    nextSp = IoGetNextIrpStackLocation(Pkt->Irp);
    nextSp->MajorFunction = IRP_MJ_SCSI;
    nextSp->Parameters.Scsi.Srb = (PSCSI_REQUEST_BLOCK)Pkt->Srb;

    SrbSetScsiStatus(Pkt->Srb, 0);
    Pkt->Srb->SrbStatus = 0;
    SrbSetSenseInfoBufferLength(Pkt->Srb, SENSE_BUFFER_SIZE_EX);

    if (Pkt->CompleteOriginalIrpWhenLastPacketCompletes) {
        /*
         *  Only dereference the "original IRP"'s stack location
         *  if its a real client irp (as opposed to a static irp
         *  we're using just for result status for one of the non-IO scsi commands).
         *
         *  For read/write, propagate the storage-specific IRP stack location flags
         *  (e.g. SL_OVERRIDE_VERIFY_VOLUME, SL_WRITE_THROUGH).
         */
        PIO_STACK_LOCATION origCurSp = IoGetCurrentIrpStackLocation(Pkt->OriginalIrp);
        nextSp->Flags = origCurSp->Flags;
    }

    //
    // If the request is not split, we can use the original IRP MDL.  If the
    // request needs to be split, we need to use a partial MDL.  The partial MDL
    // is needed because more than one driver might be mapping the same MDL
    // and this causes problems.
    //
    if (Pkt->UsePartialMdl == FALSE) {
        Pkt->Irp->MdlAddress = Pkt->OriginalIrp->MdlAddress;
    } else {
        IoBuildPartialMdl(Pkt->OriginalIrp->MdlAddress, Pkt->PartialMdl, SrbGetDataBuffer(Pkt->Srb), SrbGetDataTransferLength(Pkt->Srb));
        Pkt->Irp->MdlAddress = Pkt->PartialMdl;
    }


    DBGLOGSENDPACKET(Pkt);
    HISTORYLOGSENDPACKET(Pkt);

    //
    // Set the original irp here for SFIO.
    //
    ClasspSrbSetOriginalIrp(Pkt->Srb, (PVOID) (Pkt->OriginalIrp));

    //
    // No need to lock for IdlePrioritySupported, since it will
    // be modified only at initialization time.
    //
    if (fdoData->IdlePrioritySupported == TRUE) {
        idleRequest = ClasspIsIdleRequest(Pkt->OriginalIrp);
        if (idleRequest) {
            InterlockedIncrement(&fdoData->ActiveIdleIoCount);
        } else {
            InterlockedIncrement(&fdoData->ActiveIoCount);
        }
    }

    IoSetCompletionRoutine(Pkt->Irp, TransferPktComplete, Pkt, TRUE, TRUE, TRUE);
    return IoCallDriver(nextDevObj, Pkt->Irp);
}


NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
TransferPktComplete(IN PDEVICE_OBJECT NullFdo, IN PIRP Irp, IN PVOID Context)
{
    PTRANSFER_PACKET pkt = (PTRANSFER_PACKET)Context;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = pkt->Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    BOOLEAN packetDone = FALSE;
    BOOLEAN idleRequest = FALSE;
    ULONG transferLength;
    LARGE_INTEGER completionTime;
    ULONGLONG lastIoCompletionTime;

    UNREFERENCED_PARAMETER(NullFdo);

    /*
     *  Put all the assertions and spew in here so we don't have to look at them.
     */
    DBGLOGRETURNPACKET(pkt);
    DBGCHECKRETURNEDPKT(pkt);
    HISTORYLOGRETURNEDPACKET(pkt);


    completionTime = ClasspGetCurrentTime();

    //
    // Record the time at which the last IO completed while snapping the old
    // value to be used later. This can occur on multiple threads and hence
    // could be overwritten with an older value. This is OK because this value
    // is maintained as a heuristic.
    //

#ifdef _WIN64
    lastIoCompletionTime = ReadULong64NoFence((volatile ULONG64*)&fdoData->LastIoCompletionTime.QuadPart);
    WriteULong64NoFence((volatile ULONG64*)&fdoData->LastIoCompletionTime.QuadPart,
                        completionTime.QuadPart);
#else
    lastIoCompletionTime = InterlockedExchangeNoFence64((volatile LONG64*)&fdoData->LastIoCompletionTime.QuadPart,
                                                        completionTime.QuadPart);
#endif

    if (fdoData->IdlePrioritySupported == TRUE) {
        idleRequest = ClasspIsIdleRequest(pkt->OriginalIrp);
        if (idleRequest) {
            InterlockedDecrement(&fdoData->ActiveIdleIoCount);
            NT_ASSERT(fdoData->ActiveIdleIoCount >= 0);
        } else {
            fdoData->LastNonIdleIoTime = completionTime;
            InterlockedDecrement(&fdoData->ActiveIoCount);
            NT_ASSERT(fdoData->ActiveIoCount >= 0);
        }
    }

    //
    // If partial MDL was used, unmap the pages.  When the packet is retried, the
    // MDL will be recreated.  If the packet is done, the MDL will be ready to be reused.
    //
    if (pkt->UsePartialMdl) {
        MmPrepareMdlForReuse(pkt->PartialMdl);
    }

    if (SRB_STATUS(pkt->Srb->SrbStatus) == SRB_STATUS_SUCCESS) {

        NT_ASSERT(NT_SUCCESS(Irp->IoStatus.Status));

        transferLength = SrbGetDataTransferLength(pkt->Srb);

        fdoData->LoggedTURFailureSinceLastIO = FALSE;

        /*
         *  The port driver should not have allocated a sense buffer
         *  if the SRB succeeded.
         */
        NT_ASSERT(!PORT_ALLOCATED_SENSE_EX(fdoExt, pkt->Srb));

        /*
         *  Add this packet's transferred length to the original IRP's.
         */
        InterlockedExchangeAdd((PLONG)&pkt->OriginalIrp->IoStatus.Information,
                              (LONG)transferLength);


        if ((pkt->InLowMemRetry) ||
            (pkt->DriverUsesStartIO && pkt->LowMemRetry_remainingBufLen > 0)) {
            packetDone = StepLowMemRetry(pkt);
        } else {
            packetDone = TRUE;
        }

    }
    else {
        /*
         *  The packet failed.  We may retry it if possible.
         */
        BOOLEAN shouldRetry;

        /*
         *  Make sure IRP status matches SRB error status (since we propagate it).
         */
        if (NT_SUCCESS(Irp->IoStatus.Status)){
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        }

        /*
         *  The packet failed.
         *  So when sending the packet down we either saw either an error or STATUS_PENDING,
         *  and so we returned STATUS_PENDING for the original IRP.
         *  So now we must mark the original irp pending to match that, _regardless_ of
         *  whether we actually switch threads here by retrying.
         *  (We also have to mark the irp pending if the lower driver marked the irp pending;
         *   that is dealt with farther down).
         */
        if (pkt->CompleteOriginalIrpWhenLastPacketCompletes){
            IoMarkIrpPending(pkt->OriginalIrp);
        }

        /*
         *  Interpret the SRB error (to a meaningful IRP status)
         *  and determine if we should retry this packet.
         *  This call looks at the returned SENSE info to figure out what to do.
         */
        shouldRetry = InterpretTransferPacketError(pkt);

        /*
         *  If the SRB queue is locked-up, release it.
         *  Do this after calling the error handler.
         */
        if (pkt->Srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN){
            ClassReleaseQueue(pkt->Fdo);
        }


        if (NT_SUCCESS(Irp->IoStatus.Status)){
            /*
             *  The error was recovered above in the InterpretTransferPacketError() call.
             */

            NT_ASSERT(!shouldRetry);

            /*
             *  In the case of a recovered error,
             *  add the transfer length to the original Irp as we would in the success case.
             */
            InterlockedExchangeAdd((PLONG)&pkt->OriginalIrp->IoStatus.Information,
                                  (LONG)SrbGetDataTransferLength(pkt->Srb));

            if ((pkt->InLowMemRetry) ||
                (pkt->DriverUsesStartIO && pkt->LowMemRetry_remainingBufLen > 0)) {
                packetDone = StepLowMemRetry(pkt);
            } else {
                packetDone = TRUE;
            }
        } else {
            if (shouldRetry && (pkt->NumRetries > 0)){
                packetDone = RetryTransferPacket(pkt);
            } else if (shouldRetry && (pkt->RetryHistory != NULL)){
                // don't limit retries if class driver has custom interpretation routines
                packetDone = RetryTransferPacket(pkt);
            } else {
                packetDone = TRUE;
            }
        }
    }

    /*
     *  If the packet is completed, put it back in the free list.
     *  If it is the last packet servicing the original request, complete the original irp.
     */
    if (packetDone){
        LONG numPacketsRemaining;
        PIRP deferredIrp;
        PDEVICE_OBJECT Fdo = pkt->Fdo;
        UCHAR uniqueAddr = 0;

        /*
         *  In case a remove is pending, bump the lock count so we don't get freed
         *  right after we complete the original irp.
         */
        ClassAcquireRemoveLock(Fdo, (PVOID)&uniqueAddr);


        /*
         *  Sometimes the port driver can allocates a new 'sense' buffer
         *  to report transfer errors, e.g. when the default sense buffer
         *  is too small.  If so, it is up to us to free it.
         *  Now that we're done using the sense info, free it if appropriate.
         *  Then clear the sense buffer so it doesn't pollute future errors returned in this packet.
         */
        if (PORT_ALLOCATED_SENSE_EX(fdoExt, pkt->Srb)) {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_RW, "Freeing port-allocated sense buffer for pkt %ph.", pkt));
            FREE_PORT_ALLOCATED_SENSE_BUFFER_EX(fdoExt, pkt->Srb);
            SrbSetSenseInfoBuffer(pkt->Srb, &pkt->SrbErrorSenseData);
            SrbSetSenseInfoBufferLength(pkt->Srb, sizeof(pkt->SrbErrorSenseData));
        } else {
            NT_ASSERT(SrbGetSenseInfoBuffer(pkt->Srb) == &pkt->SrbErrorSenseData);
            NT_ASSERT(SrbGetSenseInfoBufferLength(pkt->Srb) <= sizeof(pkt->SrbErrorSenseData));
        }

        RtlZeroMemory(&pkt->SrbErrorSenseData, sizeof(pkt->SrbErrorSenseData));

        /*
         *  Call IoSetMasterIrpStatus to set appropriate status
         *  for the Master IRP.
         */
        IoSetMasterIrpStatus(pkt->OriginalIrp, Irp->IoStatus.Status);

        if (!NT_SUCCESS(Irp->IoStatus.Status)){
            /*
             *  If the original I/O originated in user space (i.e. it is thread-queued),
             *  and the error is user-correctable (e.g. media is missing, for removable media),
             *  alert the user.
             *  Since this is only one of possibly several packets completing for the original IRP,
             *  we may do this more than once for a single request.  That's ok; this allows
             *  us to test each returned status with IoIsErrorUserInduced().
             */
            if (IoIsErrorUserInduced(Irp->IoStatus.Status) &&
                pkt->CompleteOriginalIrpWhenLastPacketCompletes &&
                pkt->OriginalIrp->Tail.Overlay.Thread){

                IoSetHardErrorOrVerifyDevice(pkt->OriginalIrp, Fdo);
            }
        }

        /*
         *  We use a field in the original IRP to count
         *  down the transfer pieces as they complete.
         */
        numPacketsRemaining = InterlockedDecrement(
            (PLONG)&pkt->OriginalIrp->Tail.Overlay.DriverContext[0]);

        if (numPacketsRemaining > 0){
            /*
             *  More transfer pieces remain for the original request.
             *  Wait for them to complete before completing the original irp.
             */
        } else {

            /*
             *  All the transfer pieces are done.
             *  Complete the original irp if appropriate.
             */
            NT_ASSERT(numPacketsRemaining == 0);
            if (pkt->CompleteOriginalIrpWhenLastPacketCompletes){

                IO_PAGING_PRIORITY priority = (TEST_FLAG(pkt->OriginalIrp->Flags, IRP_PAGING_IO)) ? IoGetPagingIoPriority(pkt->OriginalIrp) : IoPagingPriorityInvalid;
                KIRQL oldIrql;

                if (NT_SUCCESS(pkt->OriginalIrp->IoStatus.Status)){
                    NT_ASSERT((ULONG)pkt->OriginalIrp->IoStatus.Information ==  IoGetCurrentIrpStackLocation(pkt->OriginalIrp)->Parameters.Read.Length);
                    ClasspPerfIncrementSuccessfulIo(fdoExt);
                }
                ClassReleaseRemoveLock(Fdo, pkt->OriginalIrp);

                /*
                 *  We submitted all the downward irps, including this last one, on the thread
                 *  that the OriginalIrp came in on.  So the OriginalIrp is completing on a
                 *  different thread iff this last downward irp is completing on a different thread.
                 *  If BlkCache is loaded, for example, it will often complete
                 *  requests out of the cache on the same thread, therefore not marking the downward
                 *  irp pending and not requiring us to do so here.  If the downward request is completing
                 *  on the same thread, then by not marking the OriginalIrp pending we can save an APC
                 *  and get extra perf benefit out of BlkCache.
                 *  Note that if the packet ever cycled due to retry or LowMemRetry,
                 *  we set the pending bit in those codepaths.
                 */
                if (pkt->Irp->PendingReturned){
                    IoMarkIrpPending(pkt->OriginalIrp);
                }


                ClassCompleteRequest(Fdo, pkt->OriginalIrp, IO_DISK_INCREMENT);

                //
                // Drop the count only after completing the request, to give
                // Mm some amount of time to issue its next critical request
                //

                if (priority == IoPagingPriorityHigh)
                {
                    KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);

                    if (fdoData->MaxInterleavedNormalIo < ClassMaxInterleavePerCriticalIo)
                    {
                        fdoData->MaxInterleavedNormalIo = 0;
                    } else {
                        fdoData->MaxInterleavedNormalIo -= ClassMaxInterleavePerCriticalIo;
                    }

                    fdoData->NumHighPriorityPagingIo--;

                    if (fdoData->NumHighPriorityPagingIo == 0)
                    {
                        LARGE_INTEGER period;

                        //
                        // Exiting throttle mode
                        //

                        KeQuerySystemTime(&fdoData->ThrottleStopTime);

                        period.QuadPart = fdoData->ThrottleStopTime.QuadPart - fdoData->ThrottleStartTime.QuadPart;
                        fdoData->LongestThrottlePeriod.QuadPart = max(fdoData->LongestThrottlePeriod.QuadPart, period.QuadPart);
                    }

                    KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
                }

                if (idleRequest) {
                    ClasspCompleteIdleRequest(fdoExt);
                }

                /*
                 *  We may have been called by one of the class drivers (e.g. cdrom)
                 *  via the legacy API ClassSplitRequest.
                 *  This is the only case for which the packet engine is called for an FDO
                 *  with a StartIo routine; in that case, we have to call IoStartNextPacket
                 *  now that the original irp has been completed.
                 */
                if (fdoExt->CommonExtension.DriverExtension->InitData.ClassStartIo) {
                    if (TEST_FLAG(SrbGetSrbFlags(pkt->Srb), SRB_FLAGS_DONT_START_NEXT_PACKET)){
                        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_RW, "SRB_FLAGS_DONT_START_NEXT_PACKET should never be set here (?)"));
                    } else {
                        KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
                        IoStartNextPacket(Fdo, TRUE); // yes, some IO is now cancellable
                        KeLowerIrql(oldIrql);
                    }
                }
            }
        }

        /*
         *  If the packet was synchronous, write the final result back to the issuer's status buffer
         *  and signal his event.
         */
        if (pkt->SyncEventPtr){
            KeSetEvent(pkt->SyncEventPtr, 0, FALSE);
            pkt->SyncEventPtr = NULL;
        }

        /*
         *  If the operation isn't a normal read/write, but needs to do more
         *  operation-specific processing, call the operation's continuation
         *  routine.  The operation may create and queue another transfer packet
         *  within this routine, but pkt is still freed after returning from the
         *  continuation routine.
         */
        if (pkt->ContinuationRoutine != NULL){
            pkt->ContinuationRoutine(pkt->ContinuationContext);
            pkt->ContinuationRoutine = NULL;
        }

        /*
         *  Free the completed packet.
         */
        pkt->UsePartialMdl = FALSE;
//        pkt->OriginalIrp = NULL;
        pkt->InLowMemRetry = FALSE;
        EnqueueFreeTransferPacket(Fdo, pkt);

        /*
         *  Now that we have freed some resources,
         *  try again to send one of the previously deferred irps.
         */
        deferredIrp = DequeueDeferredClientIrp(Fdo);
        if (deferredIrp){
            ServiceTransferRequest(Fdo, deferredIrp, TRUE);
        }

        ClassReleaseRemoveLock(Fdo, (PVOID)&uniqueAddr);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}


/*
 *  SetupEjectionTransferPacket
 *
 *      Set up a transferPacket for a synchronous Ejection Control transfer.
 */
VOID SetupEjectionTransferPacket(   TRANSFER_PACKET *Pkt,
                                        BOOLEAN PreventMediaRemoval,
                                        PKEVENT SyncEventPtr,
                                        PIRP OriginalIrp)
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Pkt->Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    PCDB pCdb;
    ULONG srbLength;

    PAGED_CODE();

    if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbLength = ((PSTORAGE_REQUEST_BLOCK) fdoData->SrbTemplate)->SrbLength;
        NT_ASSERT(((PSTORAGE_REQUEST_BLOCK) Pkt->Srb)->SrbLength >= srbLength);
    } else {
        srbLength = fdoData->SrbTemplate->Length;
    }
    RtlCopyMemory(Pkt->Srb, fdoData->SrbTemplate, srbLength); // copies _contents_ of SRB blocks

    SrbSetRequestAttribute(Pkt->Srb, SRB_SIMPLE_TAG_REQUEST);
    SrbSetCdbLength(Pkt->Srb, 6);
    SrbSetOriginalRequest(Pkt->Srb, Pkt->Irp);
    SrbSetSenseInfoBuffer(Pkt->Srb, &Pkt->SrbErrorSenseData);
    SrbSetSenseInfoBufferLength(Pkt->Srb, sizeof(Pkt->SrbErrorSenseData));
    SrbSetTimeOutValue(Pkt->Srb, fdoExt->TimeOutValue);

    SrbAssignSrbFlags(Pkt->Srb, fdoExt->SrbFlags | SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_QUEUE_FREEZE);

    pCdb = SrbGetCdb(Pkt->Srb);
    if (pCdb) {
        pCdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
        pCdb->MEDIA_REMOVAL.Prevent = PreventMediaRemoval;
    }

    Pkt->BufPtrCopy = NULL;
    Pkt->BufLenCopy = 0;

    Pkt->OriginalIrp = OriginalIrp;
    Pkt->NumRetries = NUM_LOCKMEDIAREMOVAL_RETRIES;
    Pkt->SyncEventPtr = SyncEventPtr;
    Pkt->CompleteOriginalIrpWhenLastPacketCompletes = FALSE;
}


/*
 *  SetupModeSenseTransferPacket
 *
 *      Set up a transferPacket for a synchronous Mode Sense transfer.
 */
VOID SetupModeSenseTransferPacket(TRANSFER_PACKET *Pkt,
                                  PKEVENT SyncEventPtr,
                                  PVOID ModeSenseBuffer,
                                  UCHAR ModeSenseBufferLen,
                                  UCHAR PageMode,
                                  UCHAR SubPage,
                                  PIRP OriginalIrp,
                                  UCHAR PageControl)

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Pkt->Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    PCDB pCdb;
    ULONG srbLength;

    PAGED_CODE();

    if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbLength = ((PSTORAGE_REQUEST_BLOCK) fdoData->SrbTemplate)->SrbLength;
        NT_ASSERT(((PSTORAGE_REQUEST_BLOCK) Pkt->Srb)->SrbLength >= srbLength);
    } else {
        srbLength = fdoData->SrbTemplate->Length;
    }
    RtlCopyMemory(Pkt->Srb, fdoData->SrbTemplate, srbLength); // copies _contents_ of SRB blocks

    SrbSetRequestAttribute(Pkt->Srb, SRB_SIMPLE_TAG_REQUEST);
    SrbSetCdbLength(Pkt->Srb, 6);
    SrbSetOriginalRequest(Pkt->Srb, Pkt->Irp);
    SrbSetSenseInfoBuffer(Pkt->Srb, &Pkt->SrbErrorSenseData);
    SrbSetSenseInfoBufferLength(Pkt->Srb, sizeof(Pkt->SrbErrorSenseData));
    SrbSetTimeOutValue(Pkt->Srb, fdoExt->TimeOutValue);
    SrbSetDataBuffer(Pkt->Srb, ModeSenseBuffer);
    SrbSetDataTransferLength(Pkt->Srb, ModeSenseBufferLen);


    SrbAssignSrbFlags(Pkt->Srb, fdoExt->SrbFlags | SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_QUEUE_FREEZE);

    pCdb = SrbGetCdb(Pkt->Srb);
    if (pCdb) {
        pCdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        pCdb->MODE_SENSE.PageCode = PageMode;
        pCdb->MODE_SENSE.SubPageCode = SubPage;
        pCdb->MODE_SENSE.Pc = PageControl;
        pCdb->MODE_SENSE.AllocationLength = (UCHAR)ModeSenseBufferLen;
    }

    Pkt->BufPtrCopy = ModeSenseBuffer;
    Pkt->BufLenCopy = ModeSenseBufferLen;

    Pkt->OriginalIrp = OriginalIrp;
    Pkt->NumRetries = NUM_MODESENSE_RETRIES;
    Pkt->SyncEventPtr = SyncEventPtr;
    Pkt->CompleteOriginalIrpWhenLastPacketCompletes = FALSE;
}

/*
 *  SetupModeSelectTransferPacket
 *
 *      Set up a transferPacket for a synchronous Mode Select transfer.
 */
VOID SetupModeSelectTransferPacket(TRANSFER_PACKET *Pkt,
                                  PKEVENT SyncEventPtr,
                                  PVOID ModeSelectBuffer,
                                  UCHAR ModeSelectBufferLen,
                                  BOOLEAN SavePages,
                                  PIRP OriginalIrp)

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Pkt->Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    PCDB pCdb;
    ULONG srbLength;

    if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbLength = ((PSTORAGE_REQUEST_BLOCK) fdoData->SrbTemplate)->SrbLength;
        NT_ASSERT(((PSTORAGE_REQUEST_BLOCK) Pkt->Srb)->SrbLength >= srbLength);
    } else {
        srbLength = fdoData->SrbTemplate->Length;
    }
    RtlCopyMemory(Pkt->Srb, fdoData->SrbTemplate, srbLength); // copies _contents_ of SRB blocks

    SrbSetRequestAttribute(Pkt->Srb, SRB_SIMPLE_TAG_REQUEST);
    SrbSetCdbLength(Pkt->Srb, 6);
    SrbSetOriginalRequest(Pkt->Srb, Pkt->Irp);
    SrbSetSenseInfoBuffer(Pkt->Srb, &Pkt->SrbErrorSenseData);
    SrbSetSenseInfoBufferLength(Pkt->Srb, sizeof(Pkt->SrbErrorSenseData));
    SrbSetTimeOutValue(Pkt->Srb, fdoExt->TimeOutValue);
    SrbSetDataBuffer(Pkt->Srb, ModeSelectBuffer);
    SrbSetDataTransferLength(Pkt->Srb, ModeSelectBufferLen);

    SrbAssignSrbFlags(Pkt->Srb, fdoExt->SrbFlags | SRB_FLAGS_DATA_OUT | SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_QUEUE_FREEZE);

    pCdb = SrbGetCdb(Pkt->Srb);
    if (pCdb) {
        pCdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        pCdb->MODE_SELECT.SPBit = SavePages;
        pCdb->MODE_SELECT.PFBit = 1;
        pCdb->MODE_SELECT.ParameterListLength = (UCHAR)ModeSelectBufferLen;
    }

    Pkt->BufPtrCopy = ModeSelectBuffer;
    Pkt->BufLenCopy = ModeSelectBufferLen;

    Pkt->OriginalIrp = OriginalIrp;
    Pkt->NumRetries = NUM_MODESELECT_RETRIES;
    Pkt->SyncEventPtr = SyncEventPtr;
    Pkt->CompleteOriginalIrpWhenLastPacketCompletes = FALSE;
}


/*
 *  SetupDriveCapacityTransferPacket
 *
 *      Set up a transferPacket for a synchronous Drive Capacity transfer.
 */
VOID SetupDriveCapacityTransferPacket(   TRANSFER_PACKET *Pkt,
                                        PVOID ReadCapacityBuffer,
                                        ULONG ReadCapacityBufferLen,
                                        PKEVENT SyncEventPtr,
                                        PIRP OriginalIrp,
                                        BOOLEAN Use16ByteCdb)
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Pkt->Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    PCDB pCdb;
    ULONG srbLength;
    ULONG timeoutValue = fdoExt->TimeOutValue;

    if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbLength = ((PSTORAGE_REQUEST_BLOCK) fdoData->SrbTemplate)->SrbLength;
        NT_ASSERT(((PSTORAGE_REQUEST_BLOCK) Pkt->Srb)->SrbLength >= srbLength);
    } else {
        srbLength = fdoData->SrbTemplate->Length;
    }
    RtlCopyMemory(Pkt->Srb, fdoData->SrbTemplate, srbLength); // copies _contents_ of SRB blocks

    SrbSetRequestAttribute(Pkt->Srb, SRB_SIMPLE_TAG_REQUEST);
    SrbSetOriginalRequest(Pkt->Srb, Pkt->Irp);
    SrbSetSenseInfoBuffer(Pkt->Srb, &Pkt->SrbErrorSenseData);
    SrbSetSenseInfoBufferLength(Pkt->Srb, sizeof(Pkt->SrbErrorSenseData));


    SrbSetTimeOutValue(Pkt->Srb, timeoutValue);
    SrbSetDataBuffer(Pkt->Srb, ReadCapacityBuffer);
    SrbSetDataTransferLength(Pkt->Srb, ReadCapacityBufferLen);

    SrbAssignSrbFlags(Pkt->Srb, fdoExt->SrbFlags | SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_QUEUE_FREEZE);

    pCdb = SrbGetCdb(Pkt->Srb);
    if (pCdb) {
        if (Use16ByteCdb == TRUE) {
            NT_ASSERT(ReadCapacityBufferLen >= sizeof(READ_CAPACITY_DATA_EX));
            SrbSetCdbLength(Pkt->Srb, 16);
            pCdb->CDB16.OperationCode = SCSIOP_READ_CAPACITY16;
            REVERSE_BYTES(&pCdb->CDB16.TransferLength, &ReadCapacityBufferLen);
            pCdb->AsByte[1] = 0x10; // Service Action
        } else {
            SrbSetCdbLength(Pkt->Srb, 10);
            pCdb->CDB10.OperationCode = SCSIOP_READ_CAPACITY;
        }
    }

    Pkt->BufPtrCopy = ReadCapacityBuffer;
    Pkt->BufLenCopy = ReadCapacityBufferLen;

    Pkt->OriginalIrp = OriginalIrp;
    Pkt->NumRetries = NUM_DRIVECAPACITY_RETRIES;
#if !defined(__REACTOS__) && NTDDI_VERSION >= NTDDI_WINBLUE
    Pkt->NumIoTimeoutRetries = NUM_DRIVECAPACITY_RETRIES;
#endif

    Pkt->SyncEventPtr = SyncEventPtr;
    Pkt->CompleteOriginalIrpWhenLastPacketCompletes = FALSE;
}


#if 0
    /*
     *  SetupSendStartUnitTransferPacket
     *
     *      Set up a transferPacket for a synchronous Send Start Unit transfer.
     */
    VOID SetupSendStartUnitTransferPacket(   TRANSFER_PACKET *Pkt,
                                                    PIRP OriginalIrp)
    {
        PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Pkt->Fdo->DeviceExtension;
        PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
        PCDB pCdb;

        PAGED_CODE();

        RtlZeroMemory(&Pkt->Srb, sizeof(SCSI_REQUEST_BLOCK));

        /*
         *  Initialize the SRB.
         *  Use a very long timeout value to give the drive time to spin up.
         */
        Pkt->Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
        Pkt->Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        Pkt->Srb->TimeOutValue = START_UNIT_TIMEOUT;
        Pkt->Srb->CdbLength = 6;
        Pkt->Srb->OriginalRequest = Pkt->Irp;
        Pkt->Srb->SenseInfoBuffer = &Pkt->SrbErrorSenseData;
        Pkt->Srb->SenseInfoBufferLength = sizeof(Pkt->SrbErrorSenseData);
        Pkt->Srb->Lun = 0;

        SET_FLAG(Pkt->Srb->SrbFlags, SRB_FLAGS_NO_DATA_TRANSFER);
        SET_FLAG(Pkt->Srb->SrbFlags, SRB_FLAGS_DISABLE_AUTOSENSE);
        SET_FLAG(Pkt->Srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);

        pCdb = (PCDB)Pkt->Srb->Cdb;
        pCdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
        pCdb->START_STOP.Start = 1;
        pCdb->START_STOP.Immediate = 0;
        pCdb->START_STOP.LogicalUnitNumber = 0;

        Pkt->OriginalIrp = OriginalIrp;
        Pkt->NumRetries = 0;
        Pkt->SyncEventPtr = NULL;
        Pkt->CompleteOriginalIrpWhenLastPacketCompletes = FALSE;
    }
#endif


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
CleanupTransferPacketToWorkingSetSizeWorker(
    _In_ PVOID Fdo,
    _In_opt_ PVOID Context,
    _In_ PIO_WORKITEM IoWorkItem
    )
{
    ULONG node = (ULONG) (ULONG_PTR)Context;

    PAGED_CODE();

    CleanupTransferPacketToWorkingSetSize((PDEVICE_OBJECT)Fdo, FALSE, node);

    //
    // Release the remove lock acquired in EnqueueFreeTransferPacket
    //
    ClassReleaseRemoveLock((PDEVICE_OBJECT)Fdo, (PIRP)IoWorkItem);

    if (IoWorkItem != NULL) {
        IoFreeWorkItem(IoWorkItem);
    }
}


VOID
CleanupTransferPacketToWorkingSetSize(
    _In_ PDEVICE_OBJECT Fdo,
    _In_ BOOLEAN LimitNumPktToDelete,
    _In_ ULONG Node
    )

/*
Routine Description:

    This function frees the resources for the free transfer packets attempting
    to bring them down within the working set size.

Arguments:
    Fdo: The FDO that represents the device whose transfer packet size needs to be trimmed.
    LimitNumPktToDelete: Flag to indicate if the number of packets freed in one call should be capped.
    Node: NUMA node transfer packet is associated with.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    KIRQL oldIrql;
    SINGLE_LIST_ENTRY pktList;
    PSINGLE_LIST_ENTRY slistEntry;
    PTRANSFER_PACKET pktToDelete;
    ULONG requiredNumPktToDelete = fdoData->FreeTransferPacketsLists[Node].NumTotalTransferPackets -
                                   fdoData->LocalMaxWorkingSetTransferPackets;

    if (LimitNumPktToDelete) {
        requiredNumPktToDelete = MIN(requiredNumPktToDelete, MAX_CLEANUP_TRANSFER_PACKETS_AT_ONCE);
    }

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_RW, "CleanupTransferPacketToWorkingSetSize (%p): Exiting stress, block freeing %d packets.", Fdo, requiredNumPktToDelete));

    /*
     *  Check the counter again with lock held.  This eliminates a race condition
     *  while still allowing us to not grab the spinlock in the common codepath.
     *
     *  Note that the spinlock does not synchronize with threads dequeuing free
     *  packets to send (DequeueFreeTransferPacket does that with a lightweight
     *  interlocked exchange); the spinlock prevents multiple threads in this function
     *  from deciding to free too many extra packets at once.
     */
    SimpleInitSlistHdr(&pktList);
    KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);
    while ((fdoData->FreeTransferPacketsLists[Node].NumFreeTransferPackets >= fdoData->FreeTransferPacketsLists[Node].NumTotalTransferPackets) &&
           (fdoData->FreeTransferPacketsLists[Node].NumTotalTransferPackets > fdoData->LocalMaxWorkingSetTransferPackets) &&
           (requiredNumPktToDelete--)){

        pktToDelete = DequeueFreeTransferPacketEx(Fdo, FALSE, Node);
        if (pktToDelete){
            SimplePushSlist(&pktList,
                            (PSINGLE_LIST_ENTRY)&pktToDelete->SlistEntry);
            InterlockedDecrement((volatile LONG *)&fdoData->FreeTransferPacketsLists[Node].NumTotalTransferPackets);
        } else {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_RW,
                "Extremely unlikely condition (non-fatal): %d packets dequeued at once for Fdo %p. NumTotalTransferPackets=%d (1). Node=%d",
                fdoData->LocalMaxWorkingSetTransferPackets,
                Fdo,
                fdoData->FreeTransferPacketsLists[Node].NumTotalTransferPackets,
                Node));
            break;
        }
    }
    KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);

    slistEntry = SimplePopSlist(&pktList);
    while (slistEntry) {
        pktToDelete = CONTAINING_RECORD(slistEntry, TRANSFER_PACKET, SlistEntry);
        DestroyTransferPacket(pktToDelete);
        slistEntry = SimplePopSlist(&pktList);
    }

    return;
}


_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
ClasspSetupPopulateTokenTransferPacket(
    _In_ __drv_aliasesMem POFFLOAD_READ_CONTEXT OffloadReadContext,
    _In_ PTRANSFER_PACKET Pkt,
    _In_ ULONG Length,
    _In_reads_bytes_(Length) PUCHAR PopulateTokenBuffer,
    _In_ PIRP OriginalIrp,
    _In_ ULONG ListIdentifier
    )

/*++

Routine description:

    This routine is called once to set up a packet for PopulateToken.
    It builds up the SRB by setting the appropriate fields.

Arguments:

    Pkt - The transfer packet to be sent down to the lower driver
    SyncEventPtr - The event that gets signaled once the IRP contained in the packet completes
    Length - Length of the buffer being sent as part of the command
    PopulateTokenBuffer - The buffer that contains the LBA ranges information for the PopulateToken operation
    OriginalIrp - The Io request to be processed
    ListIdentifier - The identifier that will be used to correlate a subsequent command to retrieve the token

Return Value:

    Nothing

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    PCLASS_PRIVATE_FDO_DATA fdoData;
    PCDB pCdb;
    ULONG srbLength;

    PAGED_CODE();

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspSetupPopulateTokenTransferPacket (%p): Entering function. Irp %p\n",
                Pkt->Fdo,
                OriginalIrp));

    fdoExt = Pkt->Fdo->DeviceExtension;
    fdoData = fdoExt->PrivateFdoData;

    if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbLength = ((PSTORAGE_REQUEST_BLOCK) fdoData->SrbTemplate)->SrbLength;
        NT_ASSERT(((PSTORAGE_REQUEST_BLOCK) Pkt->Srb)->SrbLength >= srbLength);
    } else {
        srbLength = fdoData->SrbTemplate->Length;
    }
    RtlCopyMemory(Pkt->Srb, fdoData->SrbTemplate, srbLength); // copies _contents_ of SRB blocks

    SrbSetRequestAttribute(Pkt->Srb, SRB_SIMPLE_TAG_REQUEST);
    SrbSetCdbLength(Pkt->Srb, 16);
    SrbSetOriginalRequest(Pkt->Srb, Pkt->Irp);
    SrbSetSenseInfoBuffer(Pkt->Srb, &Pkt->SrbErrorSenseData);
    SrbSetSenseInfoBufferLength(Pkt->Srb, sizeof(Pkt->SrbErrorSenseData));
    SrbSetTimeOutValue(Pkt->Srb, fdoExt->TimeOutValue);
    SrbSetDataBuffer(Pkt->Srb, PopulateTokenBuffer);
    SrbSetDataTransferLength(Pkt->Srb, Length);

    SrbAssignSrbFlags(Pkt->Srb, fdoExt->SrbFlags | SRB_FLAGS_DATA_OUT | SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_QUEUE_FREEZE);

    pCdb = SrbGetCdb(Pkt->Srb);
    if (pCdb) {
        pCdb->TOKEN_OPERATION.OperationCode = SCSIOP_POPULATE_TOKEN;
        pCdb->TOKEN_OPERATION.ServiceAction = SERVICE_ACTION_POPULATE_TOKEN;

        REVERSE_BYTES(&pCdb->TOKEN_OPERATION.ListIdentifier, &ListIdentifier);
        REVERSE_BYTES(&pCdb->TOKEN_OPERATION.ParameterListLength, &Length);
    }

    Pkt->BufPtrCopy = PopulateTokenBuffer;
    Pkt->BufLenCopy = Length;

    Pkt->OriginalIrp = OriginalIrp;
    Pkt->NumRetries = NUM_POPULATE_TOKEN_RETRIES;
    Pkt->CompleteOriginalIrpWhenLastPacketCompletes = FALSE;

    Pkt->ContinuationRoutine = ClasspPopulateTokenTransferPacketDone;
    Pkt->ContinuationContext = OffloadReadContext;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspSetupPopulateTokenTransferPacket (%p): Exiting function with Irp %p\n",
                Pkt->Fdo,
                OriginalIrp));

    return;
}


_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
ClasspSetupReceivePopulateTokenInformationTransferPacket(
    _In_ POFFLOAD_READ_CONTEXT OffloadReadContext,
    _In_ PTRANSFER_PACKET Pkt,
    _In_ ULONG Length,
    _In_reads_bytes_(Length) PUCHAR ReceivePopulateTokenInformationBuffer,
    _In_ PIRP OriginalIrp,
    _In_ ULONG ListIdentifier
    )

/*++

Routine description:

    This routine is called once to set up a packet for read token retrieval.
    It builds up the SRB by setting the appropriate fields.

Arguments:

    Pkt - The transfer packet to be sent down to the lower driver
    Length - Length of the buffer being sent as part of the command
    ReceivePopulateTokenInformationBuffer - The buffer into which the target will pass back the token
    OriginalIrp - The Io request to be processed
    ListIdentifier - The identifier that will be used to correlate this command with its corresponding previous populate token operation

Return Value:

    Nothing

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    PCLASS_PRIVATE_FDO_DATA fdoData;
    PCDB pCdb;
    ULONG srbLength;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspSetupReceivePopulateTokenInformationTransferPacket (%p): Entering function. Irp %p\n",
                Pkt->Fdo,
                OriginalIrp));

    fdoExt = Pkt->Fdo->DeviceExtension;
    fdoData = fdoExt->PrivateFdoData;

    if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbLength = ((PSTORAGE_REQUEST_BLOCK) fdoData->SrbTemplate)->SrbLength;
        NT_ASSERT(((PSTORAGE_REQUEST_BLOCK) Pkt->Srb)->SrbLength >= srbLength);
    } else {
        srbLength = fdoData->SrbTemplate->Length;
    }
    RtlCopyMemory(Pkt->Srb, fdoData->SrbTemplate, srbLength); // copies _contents_ of SRB blocks

    SrbSetRequestAttribute(Pkt->Srb, SRB_SIMPLE_TAG_REQUEST);
    SrbSetCdbLength(Pkt->Srb, 16);
    SrbSetOriginalRequest(Pkt->Srb, Pkt->Irp);
    SrbSetSenseInfoBuffer(Pkt->Srb, &Pkt->SrbErrorSenseData);
    SrbSetSenseInfoBufferLength(Pkt->Srb, sizeof(Pkt->SrbErrorSenseData));
    SrbSetTimeOutValue(Pkt->Srb, fdoExt->TimeOutValue);
    SrbSetDataBuffer(Pkt->Srb, ReceivePopulateTokenInformationBuffer);
    SrbSetDataTransferLength(Pkt->Srb, Length);

    SrbAssignSrbFlags(Pkt->Srb, fdoExt->SrbFlags | SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_QUEUE_FREEZE);

    pCdb = SrbGetCdb(Pkt->Srb);
    if (pCdb) {
        pCdb->RECEIVE_TOKEN_INFORMATION.OperationCode = SCSIOP_RECEIVE_ROD_TOKEN_INFORMATION;
        pCdb->RECEIVE_TOKEN_INFORMATION.ServiceAction = SERVICE_ACTION_RECEIVE_TOKEN_INFORMATION;

        REVERSE_BYTES(&pCdb->RECEIVE_TOKEN_INFORMATION.ListIdentifier, &ListIdentifier);
        REVERSE_BYTES(&pCdb->RECEIVE_TOKEN_INFORMATION.AllocationLength, &Length);
    }

    Pkt->BufPtrCopy = ReceivePopulateTokenInformationBuffer;
    Pkt->BufLenCopy = Length;

    Pkt->OriginalIrp = OriginalIrp;
    Pkt->NumRetries = NUM_POPULATE_TOKEN_RETRIES;
    Pkt->CompleteOriginalIrpWhenLastPacketCompletes = FALSE;

    Pkt->ContinuationRoutine = ClasspReceivePopulateTokenInformationTransferPacketDone;
    Pkt->ContinuationContext = OffloadReadContext;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspSetupReceivePopulateTokenInformationTransferPacket (%p): Exiting function with Irp %p\n",
                Pkt->Fdo,
                OriginalIrp));

    return;
}


_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
ClasspSetupWriteUsingTokenTransferPacket(
    _In_ __drv_aliasesMem POFFLOAD_WRITE_CONTEXT OffloadWriteContext,
    _In_ PTRANSFER_PACKET Pkt,
    _In_ ULONG Length,
    _In_reads_bytes_ (Length) PUCHAR WriteUsingTokenBuffer,
    _In_ PIRP OriginalIrp,
    _In_ ULONG ListIdentifier
    )

/*++

Routine description:

    This routine is called once to set up a packet for WriteUsingToken.
    It builds up the SRB by setting the appropriate fields. It is not called
    before a retry as the SRB fields may be modified for the retry.

    The IRP is set up in SubmitTransferPacket because it must be reset for
    each packet submission.

Arguments:

    Pkt - The transfer packet to be sent down to the lower driver
    Length - Length of the buffer being sent as part of the command
    WriteUsingTokenBuffer - The buffer that contains the read token and the write LBA ranges information for the WriteUsingToken operation
    OriginalIrp - The Io request to be processed
    ListIdentifier - The identifier that will be used to correlate a subsequent command to retrieve extended results in case of command failure

Return Value:

    Nothing

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    PCLASS_PRIVATE_FDO_DATA fdoData;
    PCDB pCdb;
    ULONG srbLength;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspSetupWriteUsingTokenTransferPacket (%p): Entering function. Irp %p\n",
                Pkt->Fdo,
                OriginalIrp));

    fdoExt = Pkt->Fdo->DeviceExtension;
    fdoData = fdoExt->PrivateFdoData;

    if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbLength = ((PSTORAGE_REQUEST_BLOCK) fdoData->SrbTemplate)->SrbLength;
        NT_ASSERT(((PSTORAGE_REQUEST_BLOCK) Pkt->Srb)->SrbLength >= srbLength);
    } else {
        srbLength = fdoData->SrbTemplate->Length;
    }
    RtlCopyMemory(Pkt->Srb, fdoData->SrbTemplate, srbLength); // copies _contents_ of SRB blocks

    SrbSetRequestAttribute(Pkt->Srb, SRB_SIMPLE_TAG_REQUEST);
    SrbSetCdbLength(Pkt->Srb, 16);
    SrbSetOriginalRequest(Pkt->Srb, Pkt->Irp);
    SrbSetSenseInfoBuffer(Pkt->Srb, &Pkt->SrbErrorSenseData);
    SrbSetSenseInfoBufferLength(Pkt->Srb, sizeof(Pkt->SrbErrorSenseData));
    SrbSetTimeOutValue(Pkt->Srb, fdoExt->TimeOutValue);
    SrbSetDataBuffer(Pkt->Srb, WriteUsingTokenBuffer);
    SrbSetDataTransferLength(Pkt->Srb, Length);

    SrbAssignSrbFlags(Pkt->Srb, fdoExt->SrbFlags | SRB_FLAGS_DATA_OUT | SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_QUEUE_FREEZE);

    pCdb = SrbGetCdb(Pkt->Srb);
    if (pCdb) {
        pCdb->TOKEN_OPERATION.OperationCode = SCSIOP_WRITE_USING_TOKEN;
        pCdb->TOKEN_OPERATION.ServiceAction = SERVICE_ACTION_WRITE_USING_TOKEN;

        REVERSE_BYTES(&pCdb->TOKEN_OPERATION.ParameterListLength, &Length);
        REVERSE_BYTES(&pCdb->TOKEN_OPERATION.ListIdentifier, &ListIdentifier);
    }

    Pkt->BufPtrCopy = WriteUsingTokenBuffer;
    Pkt->BufLenCopy = Length;

    Pkt->OriginalIrp = OriginalIrp;
    Pkt->NumRetries = NUM_WRITE_USING_TOKEN_RETRIES;
    Pkt->CompleteOriginalIrpWhenLastPacketCompletes = FALSE;

    Pkt->ContinuationRoutine = ClasspWriteUsingTokenTransferPacketDone;
    Pkt->ContinuationContext = OffloadWriteContext;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspSetupWriteUsingTokenTransferPacket (%p): Exiting function with Irp %p\n",
                Pkt->Fdo,
                OriginalIrp));

    return;
}


_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
ClasspSetupReceiveWriteUsingTokenInformationTransferPacket(
    _In_ POFFLOAD_WRITE_CONTEXT OffloadWriteContext,
    _In_ PTRANSFER_PACKET Pkt,
    _In_ ULONG Length,
    _In_reads_bytes_ (Length) PUCHAR ReceiveWriteUsingTokenInformationBuffer,
    _In_ PIRP OriginalIrp,
    _In_ ULONG ListIdentifier
    )

/*++

Routine description:

    This routine is called once to set up a packet for extended results for
    WriteUsingToken operation. It builds up the SRB by setting the appropriate fields.

Arguments:

    Pkt - The transfer packet to be sent down to the lower driver
    SyncEventPtr - The event that gets signaled once the IRP contained in the packet completes
    Length - Length of the buffer being sent as part of the command
    ReceiveWriteUsingTokenInformationBuffer - The buffer into which the target will pass back the extended results
    OriginalIrp - The Io request to be processed
    ListIdentifier - The identifier that will be used to correlate this command with its corresponding previous write using token operation

Return Value:

    Nothing

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    PCLASS_PRIVATE_FDO_DATA fdoData;
    PCDB pCdb;
    ULONG srbLength;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspSetupReceiveWriteUsingTokenInformationTransferPacket (%p): Entering function. Irp %p\n",
                Pkt->Fdo,
                OriginalIrp));

    fdoExt = Pkt->Fdo->DeviceExtension;
    fdoData = fdoExt->PrivateFdoData;

    if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbLength = ((PSTORAGE_REQUEST_BLOCK) fdoData->SrbTemplate)->SrbLength;
        NT_ASSERT(((PSTORAGE_REQUEST_BLOCK) Pkt->Srb)->SrbLength >= srbLength);
    } else {
        srbLength = fdoData->SrbTemplate->Length;
    }
    RtlCopyMemory(Pkt->Srb, fdoData->SrbTemplate, srbLength); // copies _contents_ of SRB blocks

    SrbSetRequestAttribute(Pkt->Srb, SRB_SIMPLE_TAG_REQUEST);
    SrbSetCdbLength(Pkt->Srb, 16);
    SrbSetOriginalRequest(Pkt->Srb, Pkt->Irp);
    SrbSetSenseInfoBuffer(Pkt->Srb, &Pkt->SrbErrorSenseData);
    SrbSetSenseInfoBufferLength(Pkt->Srb, sizeof(Pkt->SrbErrorSenseData));
    SrbSetTimeOutValue(Pkt->Srb, fdoExt->TimeOutValue);
    SrbSetDataBuffer(Pkt->Srb, ReceiveWriteUsingTokenInformationBuffer);
    SrbSetDataTransferLength(Pkt->Srb, Length);

    SrbAssignSrbFlags(Pkt->Srb, fdoExt->SrbFlags | SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_QUEUE_FREEZE);

    pCdb = SrbGetCdb(Pkt->Srb);
    if (pCdb) {
        pCdb->RECEIVE_TOKEN_INFORMATION.OperationCode = SCSIOP_RECEIVE_ROD_TOKEN_INFORMATION;
        pCdb->RECEIVE_TOKEN_INFORMATION.ServiceAction = SERVICE_ACTION_RECEIVE_TOKEN_INFORMATION;

        REVERSE_BYTES(&pCdb->RECEIVE_TOKEN_INFORMATION.AllocationLength, &Length);
        REVERSE_BYTES(&pCdb->RECEIVE_TOKEN_INFORMATION.ListIdentifier, &ListIdentifier);
    }

    Pkt->BufPtrCopy = ReceiveWriteUsingTokenInformationBuffer;
    Pkt->BufLenCopy = Length;

    Pkt->OriginalIrp = OriginalIrp;
    Pkt->NumRetries = NUM_WRITE_USING_TOKEN_RETRIES;
    Pkt->CompleteOriginalIrpWhenLastPacketCompletes = FALSE;

    Pkt->ContinuationRoutine = (PCONTINUATION_ROUTINE) ClasspReceiveWriteUsingTokenInformationTransferPacketDone;
    Pkt->ContinuationContext = OffloadWriteContext;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspSetupReceiveWriteUsingTokenInformationTransferPacket (%p): Exiting function with Irp %p\n",
                Pkt->Fdo,
                OriginalIrp));

    return;
}



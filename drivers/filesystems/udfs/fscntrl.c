////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*

 Module Name: FsCntrl.cpp

 Abstract:

    Contains code to handle the "File System IOCTL" dispatch entry point.

 Environment:

    Kernel mode only

*/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID    UDF_FILE_FS_CONTROL

PDIR_INDEX_HDR UDFDirIndexAlloc(IN uint_di i);

NTSTATUS
UDFAllowExtendedDasdIo(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIRP Irp
    );

NTSTATUS
UDFInvalidateVolumes(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIRP Irp
    );

/*
 Function: UDFCommonFsControl()

 Description:
    The actual work is performed here.

 Expected Interrupt Level (for execution) :
  IRQL_PASSIVE_LEVEL (invocation at higher IRQL will cause execution
    to be deferred to a worker thread context)

 Return Value: STATUS_SUCCESS/Error
*/

NTSTATUS
UDFCommonFsControl(
    PIRP_CONTEXT IrpContext,
    PIRP Irp
    )
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    // We know this is a file system control so we'll case on the
    // minor function, and call a internal worker routine to complete
    // the irp.

    UDFPrint(("\nUDFCommonFSControl\n\n"));

    switch (IrpSp->MinorFunction)
    {
    case IRP_MN_USER_FS_REQUEST:
        UDFPrint(("  UDFFSControl: UserFsReq request ....\n"));

        Status = UDFUserFsCtrlRequest(IrpContext, Irp);
        break;
    case IRP_MN_MOUNT_VOLUME:

        UDFPrint(("  UDFFSControl: MOUNT_VOLUME request ....\n"));

        Status = UDFMountVolume(IrpContext, Irp);
        break;
    case IRP_MN_VERIFY_VOLUME:

        UDFPrint(("  UDFFSControl: VERIFY_VOLUME request ....\n"));

        Status = UDFVerifyVolume(IrpContext, Irp);
        break;
    default:

        UDFPrintErr(("  UDFFSControl: STATUS_INVALID_DEVICE_REQUEST MinorFunction %x\n", (IrpSp)->MinorFunction));
        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return Status;
} // end UDFCommonFSControl()

/*
Routine Description:
    This is the common routine for implementing the user's requests made
    through NtFsControlFile.

Arguments:
    Irp - Supplies the Irp being processed

Return Value:
    NTSTATUS - The return status for the operation

*/
NTSTATUS
UDFUserFsCtrlRequest(
    PIRP_CONTEXT IrpContext,
    PIRP Irp
    )
{
    NTSTATUS RC;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

#ifdef UDF_METADATA_BATCH_DIAGNOSTICS
    if (IrpSp->Parameters.FileSystemControl.FsControlCode ==
        CTL_CODE(FILE_DEVICE_FILE_SYSTEM,0x8e2,METHOD_BUFFERED,FILE_READ_ACCESS)) {
        PFCB fcb; PCCB ccb; NTSTATUS status;
        Irp->IoStatus.Information = 0;
        if (UDFDecodeFileObject(IrpSp->FileObject,&fcb,&ccb) != UserVolumeOpen)
            status = STATUS_INVALID_PARAMETER;
        else if (!Irp->AssociatedIrp.SystemBuffer ||
                 IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(fcb->Vcb->DirectoryAppendDiag))
            status = STATUS_BUFFER_TOO_SMALL;
        else {
            PVCB vcb = fcb->Vcb;
            PULONG values = Irp->AssociatedIrp.SystemBuffer;
            UDFAcquireVcbExclusive(IrpContext,vcb,FALSE);
            RtlCopyMemory(values,vcb->DirectoryAppendDiag,sizeof(vcb->DirectoryAppendDiag));
            values[0] = 1;
            values[1] = InterlockedCompareExchange(&vcb->DirectoryAppendBytes,0,0);
            UDFReleaseVcb(IrpContext,vcb);
            Irp->IoStatus.Information = sizeof(vcb->DirectoryAppendDiag);
            status = STATUS_SUCCESS;
        }
        UDFCompleteRequest(IrpContext,Irp,status);
        return status;
    }
    if (IrpSp->Parameters.FileSystemControl.FsControlCode ==
        CTL_CODE(FILE_DEVICE_FILE_SYSTEM,0x8e1,METHOD_BUFFERED,FILE_READ_ACCESS)) {
        PFCB fcb, next; PCCB ccb; NTSTATUS status;
        PVOID restart = NULL; PULONG values;
        Irp->IoStatus.Information = 0;
        if (UDFDecodeFileObject(IrpSp->FileObject,&fcb,&ccb) != UserVolumeOpen)
            status = STATUS_INVALID_PARAMETER;
        else if (!Irp->AssociatedIrp.SystemBuffer ||
                 IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(fcb->Vcb->MetadataBatchDiag))
            status = STATUS_BUFFER_TOO_SMALL;
        else {
            PVCB vcb = fcb->Vcb;
            UDFAcquireVcbExclusive(IrpContext,vcb,FALSE);
            values = Irp->AssociatedIrp.SystemBuffer;
            RtlCopyMemory(values,vcb->MetadataBatchDiag,sizeof(vcb->MetadataBatchDiag));
            RtlZeroMemory(values+48,16*sizeof(ULONG));
            values[58] = vcb->MetadataBatchDiag[58]; // successful batched FIDs, not an FCB snapshot
            UDFLockFcbTable(IrpContext,vcb);
            UDFLockVcb(IrpContext,vcb);
            while ((next = UDFGetNextFcb(IrpContext,vcb,&restart))) {
                BOOLEAN queued = InterlockedCompareExchange(&next->MetadataCloseQueued,0,0) != 0;
                BOOLEAN isNew = next->FileInfo && next->FileInfo->Dloc &&
                    (next->FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_NEW_UNPUBLISHED);
                values[48]++;
                if (queued) values[49]++;
                if (isNew) {
                    values[50]++;
                    if (queued) values[51]++;
                    if (queued && next->FcbReference == 1) values[52]++;
                    if (UDFNewFileCacheable(vcb,next->FileInfo)) {
                        values[53]++;
                        if (queued) values[54]++;
                    }
                    if (next->FcbReference == 1) values[55]++;
                    if (next->FcbReference > 1) values[56]++;
                    if (next->FcbCleanup) values[57]++;
                    if (UDFGetFileNameLinkCount(next->FileInfo) == 1) values[59]++;
                }
            }
            UDFUnlockVcb(IrpContext,vcb);
            UDFUnlockFcbTable(IrpContext,vcb);
            /* Global queue values are approximate, not a locked transaction. */
            values[60] = UdfData.DelayedCloseCount;
            values[61] = UdfData.MaxDelayedCloseCount;
            values[62] = UdfData.MinDelayedCloseCount;
            values[63] = UdfData.FspCloseActive;
            UDFReleaseVcb(IrpContext,vcb);
            Irp->IoStatus.Information = sizeof(vcb->MetadataBatchDiag);
            status = STATUS_SUCCESS;
        }
        UDFCompleteRequest(IrpContext,Irp,status);
        return status;
    }
#endif

    //  Case on the control code.
    switch (IrpSp->Parameters.FileSystemControl.FsControlCode) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1 :
    case FSCTL_REQUEST_OPLOCK_LEVEL_2 :
    case FSCTL_REQUEST_BATCH_OPLOCK :
    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE :
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING :
    case FSCTL_OPLOCK_BREAK_NOTIFY :
    case FSCTL_OPLOCK_BREAK_ACK_NO_2 :
    case FSCTL_REQUEST_FILTER_OPLOCK :

        //RC = UDFOplockRequest( IrpContext, Irp );
        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        RC = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case FSCTL_INVALIDATE_VOLUMES :

        RC = UDFInvalidateVolumes(IrpContext, Irp);
        break;

    case FSCTL_MARK_VOLUME_DIRTY:

        //RC = UDFMarkVolumeDirty(IrpContext, Irp);
        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        RC = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case FSCTL_IS_VOLUME_DIRTY:

        RC = UDFIsVolumeDirty(IrpContext, Irp);
        break;

    case FSCTL_ALLOW_EXTENDED_DASD_IO:

        RC = UDFAllowExtendedDasdIo(IrpContext, Irp);
        break;

    case FSCTL_DISMOUNT_VOLUME:

        RC = UDFDismountVolume(IrpContext, Irp);
        break;

    case FSCTL_IS_VOLUME_MOUNTED:

        RC = UDFIsVolumeMounted(IrpContext, Irp);
        break;

    case FSCTL_LOCK_VOLUME:

        RC = UDFLockVolume(IrpContext, Irp);
        break;

    case FSCTL_UNLOCK_VOLUME:

        RC = UDFUnlockVolume(IrpContext, Irp);
        break;

    case FSCTL_IS_PATHNAME_VALID:

        RC = UDFIsPathnameValid(IrpContext, Irp);
        break;

    case FSCTL_GET_VOLUME_BITMAP:

        UDFPrint(("UDFUserFsCtrlRequest: FSCTL_GET_VOLUME_BITMAP\n"));
        RC = UDFGetVolumeBitmap(IrpContext, Irp);
        break;

    case FSCTL_GET_RETRIEVAL_POINTERS:

        UDFPrint(("UDFUserFsCtrlRequest: FSCTL_GET_RETRIEVAL_POINTERS\n"));
        RC = UDFGetRetrievalPointers(IrpContext, Irp);
        break;

    case FSCTL_MOVE_FILE:

        //RC = UDFMoveFile(IrpContext, Irp);
        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        RC = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case FSCTL_MARK_HANDLE:

        //RC = UDFMarkHandle(IrpContext, Irp);
        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        RC = STATUS_INVALID_DEVICE_REQUEST;
        break;

/*
    // _WIN32_WINNT_WIN8
    case FSCTL_SET_PURGE_FAILURE_MODE:

        //RC = UDFSetPurgeFailureMode(IrpContext, Irp);
        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        RC = STATUS_INVALID_DEVICE_REQUEST;
        break;

    // _WIN32_WINNT_VISTA
    case FSCTL_MAKE_MEDIA_COMPATIBLE:

        //RC = UDFMakeCompatible(IrpContext, Irp);
        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        RC = STATUS_INVALID_DEVICE_REQUEST;
        break;

    // _WIN32_WINNT_VISTA
    case FSCTL_QUERY_SPARING_INFO:

        //RC = UDFQuerySparingInfo(IrpContext, Irp);
        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        RC = STATUS_INVALID_DEVICE_REQUEST;
        break;

    // _WIN32_WINNT_VISTA
    case FSCTL_QUERY_ON_DISK_VOLUME_INFO:

        //RC = UDFQueryOnDiskVolInfo(IrpContext, Irp);
        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        RC = STATUS_INVALID_DEVICE_REQUEST;
        break;

    // _WIN32_WINNT_VISTA
    case FSCTL_SET_DEFECT_MANAGEMENT:

        //RC = UDFSetDefectManagement(IrpContext, Irp);
        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        RC = STATUS_INVALID_DEVICE_REQUEST;
        break;
*/

    //  We don't support any of the known or unknown requests.
    default:

        UDFPrintErr(("UDFUserFsCtrlRequest: STATUS_INVALID_DEVICE_REQUEST for %x\n",
            IrpSp->Parameters.FileSystemControl.FsControlCode));

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        RC = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return RC;

} // end UDFUserFsCtrlRequest()


/*
Routine Description:
    This is the common routine for implementing the mount requests

Arguments:
    Irp - Supplies the Irp being processed

Return Value:
    NTSTATUS - The return status for the operation

*/
NTSTATUS
NTAPI
UDFMountVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )
{
    NTSTATUS                RC;
    PIO_STACK_LOCATION      IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_OBJECT          DeviceObjectWeTalkTo = IrpSp->Parameters.MountVolume.DeviceObject;
    PVPB                    Vpb = IrpSp->Parameters.MountVolume.Vpb;
    PVCB                    Vcb = NULL;
    PVOLUME_DEVICE_OBJECT   VolDo = NULL;
    IO_STATUS_BLOCK         Iosb;
    ULONG                   MediaChangeCount = 0;
    DEVICE_TYPE             FsDeviceType;
    BOOLEAN                 RestoreDoVerify = FALSE;
    BOOLEAN                 RemovableMedia = TRUE;
    BOOLEAN                 SetDoVerifyOnFail;
    BOOLEAN                 VcbAcquired = FALSE;
    BOOLEAN                 DeviceNotTouched = TRUE;
    DISK_GEOMETRY           DiskGeometry;

    ASSERT(IrpSp);
    UDFPrint(("\n !!! UDFMountVolume\n"));

    PDEVICE_OBJECT RealDevice = Vpb->RealDevice;
    
    SetDoVerifyOnFail = UDFRealDevNeedsVerify(RealDevice);

    if (FlagOn(DeviceObjectWeTalkTo->Characteristics, FILE_FLOPPY_DISKETTE)) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_UNRECOGNIZED_VOLUME);
        return STATUS_UNRECOGNIZED_VOLUME;
    }

    //  If we've shutdown disallow further mounts.

    if (FlagOn(UdfData.Flags, UDF_DATA_FLAGS_SHUTDOWN)) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_SYSTEM_SHUTDOWN);
        return STATUS_SYSTEM_SHUTDOWN;
    }

    RemovableMedia = FlagOn(DeviceObjectWeTalkTo->Characteristics, FILE_REMOVABLE_MEDIA);

    if (DeviceObjectWeTalkTo->DeviceType == FILE_DEVICE_CD_ROM) {
        FsDeviceType = FILE_DEVICE_CD_ROM_FILE_SYSTEM;
    } else {
        FsDeviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
    }

    //  Do a CheckVerify here to lift the MediaChange ticker from the driver
    RC = UDFPerformDevIoCtrl((RealDevice->DeviceType == FILE_DEVICE_CD_ROM ?
        IOCTL_CDROM_CHECK_VERIFY :
        IOCTL_DISK_CHECK_VERIFY),
        DeviceObjectWeTalkTo,
        NULL, 0,
        &MediaChangeCount, sizeof(ULONG),
        TRUE,
        &Iosb);

    if (!NT_SUCCESS(RC)) {

        UDFCompleteRequest(IrpContext, Irp, RC);
        return RC;
    }

    RC = UDFPerformDevIoCtrl((RealDevice->DeviceType == FILE_DEVICE_CD_ROM ?
            IOCTL_CDROM_GET_DRIVE_GEOMETRY :
            IOCTL_DISK_GET_DRIVE_GEOMETRY),
            DeviceObjectWeTalkTo,
            NULL, 0,
            &DiskGeometry, sizeof(DISK_GEOMETRY),
            TRUE,
            NULL);

    if (!NT_SUCCESS(RC)) {

        UDFCompleteRequest(IrpContext, Irp, RC);
        return RC;
    }

    // Acquire the global resource to do mount operations.

    UDFAcquireUdfData(IrpContext);

    _SEH2_TRY {

        UDFScanForDismountedVcb(IrpContext);

        if (!IS_ALIGNED_POWER_OF_2(DiskGeometry.BytesPerSector)) {

            try_return(RC = STATUS_DRIVER_INTERNAL_ERROR);
        }

        if (DiskGeometry.BytesPerSector > MAX_SECTOR_SIZE) {

            try_return(RC = STATUS_UNRECOGNIZED_VOLUME);
        }

        // Now before we can initialize the Vcb we need to set up the
        // Get our device object and alignment requirement.
        // Device extension == VCB
        UDFPrint(("UDFMountVolume: create device\n"));
        RC = IoCreateDevice( UdfData.DriverObject,
                                 sizeof(struct VOLUME_DEVICE_OBJECT) - sizeof(DEVICE_OBJECT),
                                 NULL,
                                 FsDeviceType,
                                 0,
                                 FALSE,
                                 (PDEVICE_OBJECT*)&VolDo);

        if (!NT_SUCCESS(RC)) try_return(RC);

        // Our alignment requirement is the larger of the processor alignment requirement
        // already in the volume device object and that in the DeviceObjectWeTalkTo
        if (DeviceObjectWeTalkTo->AlignmentRequirement > VolDo->DeviceObject.AlignmentRequirement) {
            VolDo->DeviceObject.AlignmentRequirement = DeviceObjectWeTalkTo->AlignmentRequirement;
        }

        // We must initialize the stack size in our device object before
        // the following reads, because the I/O system has not done it yet.

        VolDo->DeviceObject.StackSize = (CCHAR) (DeviceObjectWeTalkTo->StackSize + 1);

        ClearFlag(VolDo->DeviceObject.Flags, DO_DEVICE_INITIALIZING);

        // Initialize the overflow queue for the volume

        VolDo->OverflowQueueCount = 0;
        InitializeListHead(&VolDo->OverflowQueue);

        VolDo->PostedRequestCount = 0;
        KeInitializeSpinLock(&VolDo->OverflowQueueSpinLock);

        // device object field in the VPB to point to our new volume device
        // object.
        Vpb->DeviceObject = (PDEVICE_OBJECT)VolDo;

        // Initialize the Vcb.  This routine will raise on an allocation
        // failure.

        UDFInitializeVCB(IrpContext, &VolDo->Vcb, DeviceObjectWeTalkTo, Vpb, &DiskGeometry, MediaChangeCount);

        //  Show that we initialized the Vcb and can cleanup with the Vcb.

        Vcb = &VolDo->Vcb;
        VolDo = NULL;
        Vpb = NULL;

        // Store the Vcb in the IrpContext as we didn't have one before.

        IrpContext->Vcb = Vcb;

        UDFAcquireVcbExclusive(IrpContext, Vcb, FALSE);
        VcbAcquired = TRUE;

        // Let's reference the Vpb to make sure we are the one to
        // have the last dereference.
        Vcb->Vpb->ReferenceCount ++;

        Vcb->MediaChangeCount = MediaChangeCount;
        Vcb->FsDeviceType = FsDeviceType;

        // Clear the verify bit for the start of mount.
        UDFMarkRealDevVerifyOk(Vcb->Vpb->RealDevice);

        DeviceNotTouched = FALSE;
        RC = UDFGetDiskInfo(IrpContext, DeviceObjectWeTalkTo, Vcb);
        if (!NT_SUCCESS(RC)) try_return(RC);

        //     ****  Read registry settings  ****
        UDFReadRegKeys(Vcb, FALSE, FALSE);

        // Check and, only for the verified bounded class, repair an unclean
        // disk before loading its bitmap/LVID into the VCB. A failed ordered
        // commit must abort this mount so stale allocation state is not used.
        RC = UDFRecoverDirtyVolumeBeforeLoad(IrpContext, Vcb);
        if (!NT_SUCCESS(RC)) try_return(RC);

        Vcb->MountPhErrorCount = 0;

        UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),TRUE);
        RC = UDFGetDiskInfoAndVerify(IrpContext, DeviceObjectWeTalkTo,Vcb);
        UDFReleaseResource(&(Vcb->BitMapResource1));

        ASSERT(!Vcb->Modified);

        if (!NT_SUCCESS(RC)) {

            try_return(RC);

        } else {
            Vcb->MountPhErrorCount = -1;

            // Complete mount operations: create root FCB
            UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),TRUE);
            RC = UDFCompleteMount(IrpContext, Vcb);
            UDFReleaseResource(&(Vcb->BitMapResource1));
            if (!NT_SUCCESS(RC)) {
                // We must have Vcb->VcbReference = 1 for UDFBlankMount()
                // Thus, we should not decrement it here
                // Also, if we shall not perform BlankMount,
                // but simply cleanup and return error, Vcb->VcbReference
                // will be decremented during cleanup. Thus anyway it must
                // stay 1 unchanged here
                //InterlockedDecrement((PLONG)&Vcb->VcbReference);
                UDFCloseResidual(IrpContext, Vcb);
                Vcb->VcbReference = 1;

                try_return(RC);
            }
        }

        if ((Vcb->TargetDeviceObject->DeviceType == FILE_DEVICE_DISK) &&
                (
                    UDFPerformDevIoCtrl(IOCTL_DISK_IS_WRITABLE, Vcb->TargetDeviceObject,
                                    NULL, 0, NULL, 0, TRUE, NULL) == STATUS_MEDIA_WRITE_PROTECTED
                )
            ) {
            Vcb->VcbState |= VCB_STATE_MEDIA_WRITE_PROTECT;
        }

        if ((Vcb->VcbState & VCB_STATE_MEDIA_WRITE_PROTECT)) {
            UDFPrint(("UDFMountVolume: RO mount\n"));
            Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
        }


        if ((Vcb->origIntegrityType == INTEGRITY_TYPE_OPEN) && (Vcb->CDR_Mode == 0)) {
            UDFPrint(("UDFMountVolume: volume is dirty, Read Only mount\n"));
            Vcb->VcbState |= VCB_STATE_MOUNTED_DIRTY
                | VCB_STATE_VOLUME_READ_ONLY;
        }

        Vcb->Vpb->SerialNumber = Vcb->PhSerialNumber;
        Vcb->Vpb->VolumeLabelLength = Vcb->VolIdent.Length;
        RtlCopyMemory( Vcb->Vpb->VolumeLabel,
                       Vcb->VolIdent.Buffer,
                       Vcb->VolIdent.Length );

        // The new mount is complete.  Remove the additional references on this
        // Vcb and the device we are mounted on top of.

        Vcb->VcbReference -= Vcb->VcbResidualReference;
        NT_ASSERT(Vcb->VcbReference == Vcb->VcbResidualReference);

        Vcb->VcbCondition = VcbMounted;

        // Metadata copy-on-write checkpoints (cow.c), writable volumes only.
        if (!(Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY))
            (VOID)UDFCowStart(IrpContext, Vcb);

        Vcb->TotalAllocUnits = UDFGetTotalSpace(Vcb);
        Vcb->FreeAllocUnits = UDFGetFreeSpace(Vcb);

        //  The new mount is complete.
        UDFReleaseResource( &(Vcb->VcbResource) );
        VcbAcquired = FALSE;
        Vcb = NULL;

        RC = STATUS_SUCCESS;

try_exit: NOTHING;
    } _SEH2_FINALLY {

        if (!NT_SUCCESS(RC)) {

            // If we didn't complete the mount then cleanup any remaining structures.
            if (Vpb) {
               Vpb->DeviceObject = NULL;
            }

            if (Vcb) {
                // Restore the verify bit.
                if (RestoreDoVerify) {
                    Vcb->Vpb->RealDevice->Flags |= DO_VERIFY_VOLUME;
                }
                // Make sure there is no Vcb since it could go away
                if (Vcb->VcbReference)
                    InterlockedDecrement((PLONG)&Vcb->VcbReference);
                // This procedure will also delete the volume device object
                if (UDFDismountVcb(IrpContext, Vcb, FALSE)) {
                    UDFReleaseResource( &(Vcb->VcbResource) );
                }
            } else if (VolDo) {
                IoDeleteDevice((PDEVICE_OBJECT)VolDo);
            }
        }

        //  If we are not mounting the device,  then set the verify bit again.

        if ((_SEH2_AbnormalTermination() || !NT_SUCCESS(RC)) && 
            SetDoVerifyOnFail) {

            UDFMarkRealDevForVerify(RealDevice);
        }

        // Release the global resource.

        UDFReleaseUdfData(IrpContext);

    } _SEH2_END;

    //  Now send mount notification.
    if (NT_SUCCESS(RC)) {

        PFILE_OBJECT FileObject = IoCreateStreamFileObject(NULL, RealDevice);
        if (FileObject) {
            FsRtlNotifyVolumeEvent(FileObject, FSRTL_VOLUME_MOUNT);
            ObDereferenceObject(FileObject);
        }
    }

    // Complete the request if no exception.
    UDFCompleteRequest(IrpContext, Irp, RC);

    UDFPrint(("UDFMountVolume: final RC = %x\n", RC));
    return RC;

} // end UDFMountVolume()

VOID
UDFCloseResidual(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )
{
    //  Deinitialize Non-alloc file
    if (Vcb->VcbReference)
        InterlockedDecrement((PLONG)&Vcb->VcbReference);
    UDFPrint(("UDFCloseResidual: NonAllocFileInfo %x\n", Vcb->NonAllocFileInfo));
    if (Vcb->NonAllocFileInfo) {
        UDFCloseFile__(IrpContext, Vcb, Vcb->NonAllocFileInfo);
        UDFCleanUpFile__(Vcb, Vcb->NonAllocFileInfo);
        MyFreePool__(Vcb->NonAllocFileInfo);
        Vcb->NonAllocFileInfo = NULL;
    }
    //  Deinitialize Unique ID Mapping
    UDFPrint(("UDFCloseResidual: NonAllocFileInfo %x\n", Vcb->NonAllocFileInfo));
    if (Vcb->UniqueIDMapFileInfo) {
        UDFCloseFile__(IrpContext, Vcb, Vcb->UniqueIDMapFileInfo);
        UDFCleanUpFile__(Vcb, Vcb->UniqueIDMapFileInfo);
        MyFreePool__(Vcb->UniqueIDMapFileInfo);
        Vcb->UniqueIDMapFileInfo = NULL;
    }
    //  Deinitialize VAT file
    UDFPrint(("UDFCloseResidual: VatFileInfo %x\n", Vcb->VatFileInfo));
    if (Vcb->VatFileInfo) {
        UDFCloseFile__(IrpContext, Vcb,Vcb->VatFileInfo);
        UDFCleanUpFile__(Vcb, Vcb->VatFileInfo);
        MyFreePool__(Vcb->VatFileInfo);
        Vcb->VatFileInfo = NULL;
    }
    UDFReleaseMetadata(IrpContext, Vcb);
    //  System StreamDir
    UDFPrint(("UDFCloseResidual: SysSDirFileInfo %x\n", Vcb->SysSDirFileInfo));
    if (Vcb->SysSDirFileInfo) {
        UDFCloseFile__(IrpContext, Vcb, Vcb->SysSDirFileInfo);
        UDFCleanUpFile__(Vcb, Vcb->SysSDirFileInfo);
        MyFreePool__(Vcb->SysSDirFileInfo);
        Vcb->SysSDirFileInfo = NULL;
    }
/*    //  Deinitialize root dir fcb
    if (Vcb->RootDirFCB) {
        UDFCloseFile__(Vcb,Vcb->RootDirFCB->FileInfo);
        UDFCleanUpFile__(Vcb, Vcb->RootDirFCB->FileInfo);
        MyFreePool__(Vcb->RootDirFCB->FileInfo);
        UDFCleanUpFCB(Vcb->RootDirFCB);
        //  Remove root FCB reference in vcb
        if (Vcb->VcbReference) Vcb->VcbReference--;
    }

    // Deinitialize Non-alloc file
    if (Vcb->VcbReference) Vcb->VcbReference--;
    if (Vcb->NonAllocFileInfo) {
        UDFCloseFile__(Vcb,Vcb->NonAllocFileInfo);
        // We must release VCB here !!!!
//        UDFCleanUpFcbChain(Vcb, Vcb->NonAllocFileInfo, 1);
        Vcb->NonAllocFileInfo = NULL;
    }
    // Deinitialize VAT file
    if (Vcb->VatFileInfo) {
        UDFCloseFile__(Vcb,Vcb->VatFileInfo);
        // We must release VCB here !!!!
//        UDFCleanUpFcbChain(Vcb, Vcb->VatFileInfo, 1);
        Vcb->VatFileInfo = NULL;
    }*/

    // Deinitialize root dir fcb
    UDFPrint(("UDFCloseResidual: RootDirFCB %x\n", Vcb->RootIndexFcb));
    if (Vcb->RootIndexFcb) {
        UDFCloseFile__(IrpContext, Vcb, Vcb->RootIndexFcb->FileInfo);
        if (Vcb->RootIndexFcb->FcbCleanup)
            Vcb->RootIndexFcb->FcbCleanup--;
        {
            BOOLEAN RemovedFcb = FALSE;
            UDFAcquireFcbExclusive(IrpContext, Vcb->RootIndexFcb, FALSE);
            // LCB-based teardown: walks ParentLcbQueue to find and remove LCBs
            UDFTeardownStructures(IrpContext, Vcb->RootIndexFcb, FALSE, &RemovedFcb);
            if (!RemovedFcb) {
                UDFReleaseFcb(IrpContext, Vcb->RootIndexFcb);
            }
        }
        // Remove root FCB reference in vcb
        if (Vcb->VcbReference)
            InterlockedDecrement((PLONG)&Vcb->VcbReference);
        Vcb->RootIndexFcb = NULL;
    }
} // end UDFCloseResidual()

VOID
UDFCleanupVCB(
    IN PVCB Vcb
    )
{
    _SEH2_TRY {
        UDFReleaseFileIdCache(Vcb);
        UDFReleaseDeletePrefreeRecords(Vcb);
        UDFReleaseDlocList(Vcb);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;

    MyFreeMemoryAndPointer(Vcb->Partitions);
    MyFreeMemoryAndPointer(Vcb->LVid);
    MyFreeMemoryAndPointer(Vcb->Vat);
    MyFreeMemoryAndPointer(Vcb->SparingTable);

    // Teardown bitmap cache stream (per-page mode) or free NonPagedPool buffer (legacy)
    UDFUnpinBitmapPage(Vcb);
    UDFDeleteBitmapStream(Vcb);
    if (Vcb->FSBM_Bitmap) {
        DbgFreePool(Vcb->FSBM_Bitmap);
        Vcb->FSBM_Bitmap = NULL;
    }

    if (Vcb->BSBM_Bitmap) {
        DbgFreePool(Vcb->BSBM_Bitmap);
        Vcb->BSBM_Bitmap = NULL;
    }
#ifdef UDF_TRACK_ONDISK_ALLOCATION_OWNERS
    if (Vcb->FSBM_Bitmap_owners) {
        DbgFreePool(Vcb->FSBM_Bitmap_owners);
        Vcb->FSBM_Bitmap_owners = NULL;
    }
#endif //UDF_TRACK_ONDISK_ALLOCATION_OWNERS
    if (Vcb->FSBM_OldBitmap) {
        DbgFreePool(Vcb->FSBM_OldBitmap);
        Vcb->FSBM_OldBitmap = NULL;
    }

    MyFreeMemoryAndPointer(Vcb->VolIdent.Buffer);

    if (Vcb->ZBuffer) {
        DbgFreePool(Vcb->ZBuffer);
        Vcb->ZBuffer = NULL;
    }

    if (Vcb->fZBuffer) {
        DbgFreePool(Vcb->fZBuffer);
        Vcb->fZBuffer = NULL;
    }

    MyFreeMemoryAndPointer(Vcb->TrackMap);

} // end UDFCleanupVCB()

/*

Routine Description:

    This routine walks through the list of Vcb's looking for any which may
    now be deleted.  They may have been left on the list because there were
    outstanding references.

Arguments:

Return Value:

    None

*/
VOID
UDFScanForDismountedVcb(
    IN PIRP_CONTEXT IrpContext
    )
{
    PVCB Vcb;
    PLIST_ENTRY Link;


    // Walk through all of the Vcb's attached to the global data.
    Link = UdfData.VcbQueue.Flink;

    while (Link != &(UdfData.VcbQueue)) {

        Vcb = CONTAINING_RECORD(Link, struct VCB, VcbLinks);

        // Move to the next link now since the current Vcb may be deleted.
        Link = Link->Flink;

        // If dismount is already underway then check if this Vcb can
        // go away.
        if ((Vcb->VcbCondition == VcbDismountInProgress) ||
            (Vcb->VcbCondition == VcbInvalid) ||
            ((Vcb->VcbCondition == VcbNotMounted) && (Vcb->VcbReference <= Vcb->VcbResidualReference))) {

            UDFCheckForDismount(IrpContext, Vcb, FALSE);
        }
    }

    return;
} // end UDFScanForDismountedVcb()

/*
Routine Description:
    This routine determines if a volume is currently mounted.

Arguments:
    Irp - Supplies the Irp to process

Return Value:
    NTSTATUS - The return status for the operation

*/
NTSTATUS
UDFIsVolumeMounted(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFCB Fcb;
    PCCB Ccb;
    PVCB Vcb = IrpContext->Vcb;

    UDFPrint(("UDFIsVolumeMounted\n"));

    // Decode the file object.

    UDFDecodeFileObject(IrpSp->FileObject, &Fcb, &Ccb);

    ASSERT_CCB(Ccb);
    ASSERT_FCB(Fcb);
    ASSERT_VCB(Vcb);

    if (Fcb) {

        // Disable PopUps, we want to return any error.
        IrpContext->Flags |= IRP_CONTEXT_FLAG_DISABLE_POPUPS;

        UDFAcquireVcbShared(IrpContext, Vcb, FALSE);

        _SEH2_TRY {

            // Verify the Vcb.  This will raise in the error condition.

            UDFVerifyVcb(IrpContext, Vcb);

        } _SEH2_FINALLY {

            UDFReleaseVcb(IrpContext, Vcb);
        } _SEH2_END;
    }

    UDFCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);

    return STATUS_SUCCESS;
} // end UDFIsVolumeMounted()

/*
    This routine determines if pathname is valid path for UDF Filesystem
    We always succeed this request.

Arguments:
    Irp - Supplies the Irp to process

Return Value:
    NTSTATUS - The return status for the operation
*/
NTSTATUS
UDFIsPathnameValid(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )
{
    PAGED_CODE();

    UDFCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);
    return STATUS_SUCCESS;
} // end UDFIsPathnameValid()

/*
    This routine performs the actual unlock volume operation.
    The volume must be held exclusive by the caller.

Arguments:
    Vcb - The volume being locked.
    FileObject - File corresponding to the handle locking the volume.  If this
        is not specified, a system lock is assumed.

Return Value:
    NTSTATUS - The return status for the operation
    Attempting to remove a system lock that did not exist is OK.
*/
NTSTATUS
UDFUnlockVolumeInternal (
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    )
{
    KIRQL SavedIrql;
    NTSTATUS Status = STATUS_NOT_LOCKED;

    IoAcquireVpbSpinLock(&SavedIrql);

    if (Vcb->Vpb && FlagOn(Vcb->Vpb->Flags, VPB_LOCKED) &&
        FileObject == Vcb->VolumeLockFileObject) {

        // This one locked it, unlock the volume
        ClearFlag(Vcb->Vpb->Flags, VPB_LOCKED | VPB_DIRECT_WRITES_ALLOWED);
        ClearFlag(Vcb->VcbState, VCB_STATE_LOCKED);
        Vcb->VolumeLockFileObject = NULL;

        Status = STATUS_SUCCESS;
    }

    IoReleaseVpbSpinLock(SavedIrql);

    return Status;
} // end UDFUnlockVolumeInternal()

/*
    This routine performs the lock volume operation.  It is responsible for
    either completing of enqueuing the input Irp.
Arguments:
    Irp - Supplies the Irp to process
Return Value:
    NTSTATUS - The return status for the operation
*/
NTSTATUS
UDFLockVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP             Irp
    )
{
    NTSTATUS Status;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    // Decode the file object, the only type of opens we accept are
    // user volume opens.

    if (UDFDecodeFileObject(IrpSp->FileObject, &Fcb, &Ccb) != UserVolumeOpen) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);

        return STATUS_INVALID_PARAMETER;
    }

    UDFDecodeFileObject(IrpSp->FileObject, &Fcb, &Ccb);

    Vcb = Fcb->Vcb;

    ASSERT_CCB(Ccb);
    ASSERT_FCB(Fcb);
    ASSERT_VCB(Vcb);

    // Send our notification so that folks that like to hold handles on
    // volumes can get out of the way.

    FsRtlNotifyVolumeEvent(IrpSp->FileObject, FSRTL_VOLUME_LOCK);

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    UDFAcquireVcbExclusive(IrpContext, Vcb, FALSE);

    _SEH2_TRY {

        // Verify the Vcb.

        UDFVerifyVcb(IrpContext, Vcb);

        Status = UDFLockVolumeInternal(IrpContext, Vcb, IrpSp->FileObject);

    } _SEH2_FINALLY {

        // Release the Vcb.

        UDFReleaseVcb(IrpContext, Vcb);
        
        if (_SEH2_AbnormalTermination() || !NT_SUCCESS(Status)) {

            FsRtlNotifyVolumeEvent(IrpSp->FileObject, FSRTL_VOLUME_LOCK_FAILED);
        }
    } _SEH2_END;

    // Complete the request if there haven't been any exceptions.

    UDFCompleteRequest(IrpContext, Irp, Status);
    return Status;
} // end UDFLockVolume()

NTSTATUS
UDFPurgeVolume (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN RemovedFcb;
    PVOID RestartKey = NULL;
    PFCB NextFcb = NULL, ThisFcb = NULL;

    ASSERT_EXCLUSIVE_VCB(Vcb);
    UDFFspClose(Vcb);

    while (1) {
        UDFLockFcbTable(IrpContext, Vcb);
        UDFLockVcb(IrpContext, Vcb);
        NextFcb = UDFGetNextFcb(IrpContext, Vcb, &RestartKey);
        if (NextFcb) {
            NextFcb->FcbReference += 1;
        }

        if (!ThisFcb) {
            UDFUnlockVcb(IrpContext, Vcb);
            UDFUnlockFcbTable(IrpContext, Vcb);
        } else {
            UDFUnlockVcb(IrpContext, Vcb);
            UDFUnlockFcbTable(IrpContext, Vcb);

            // Keep the enumeration reference until its FCB resource is held.
            // A cached-section close can otherwise tear down the FCB between
            // dropping the last reference and acquiring this resource.
            UDF_CHECK_PAGING_IO_RESOURCE(ThisFcb);
            UDFAcquireFcbExclusive(IrpContext, ThisFcb, FALSE);
            UDFLockVcb(IrpContext, Vcb);
            ThisFcb->FcbReference -= 1;
            UDFUnlockVcb(IrpContext, Vcb);
            UDFTeardownStructures(IrpContext, ThisFcb, FALSE, &RemovedFcb);
            if (!RemovedFcb) {
                UDFReleaseFcb(IrpContext, ThisFcb);
            }
        }

        if (!NextFcb)
            break;
        ThisFcb = NextFcb;

        if (ThisFcb->FcbNonpaged->SegmentObject.ImageSectionObject) {
            MmFlushImageSection(&ThisFcb->FcbNonpaged->SegmentObject, MmFlushForWrite);
        }

        if ((Status == STATUS_SUCCESS) &&
            ThisFcb->FcbNonpaged->SegmentObject.DataSectionObject &&
            !CcPurgeCacheSection(&ThisFcb->FcbNonpaged->SegmentObject, NULL, 0, FALSE)) {
            Status = STATUS_UNABLE_TO_DELETE_SECTION;
        }
    }

    return Status;
}

_Requires_lock_held_(_Global_critical_region_)
_Requires_lock_held_(Vcb->VcbResource)
NTSTATUS
UDFLockVolumeInternal (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PVCB Vcb,
    _In_opt_ PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine performs the actual lock volume operation.  It will be called
    by anyone wishing to try to protect the volume for a long duration.  PNP
    operations are such a user.
    
    The volume must be held exclusive by the caller.

Arguments:

    Vcb - The volume being locked.
    
    FileObject - File corresponding to the handle locking the volume.  If this
        is not specified, a system lock is assumed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    KIRQL SavedIrql;
    NTSTATUS FinalStatus = (FileObject? STATUS_ACCESS_DENIED: STATUS_DEVICE_BUSY);
    ULONG RemainingUserReferences = (FileObject? 1: 0);

    ASSERT_EXCLUSIVE_VCB(Vcb);

    //
    //  The cleanup count for the volume only reflects the fileobject that
    //  will lock the volume.  Otherwise, we must fail the request.
    //
    //  Since the only cleanup is for the provided fileobject, we will try
    //  to get rid of all of the other user references.  If there is only one
    //  remaining after the purge then we can allow the volume to be locked.
    //

    Status = UDFFlushVolume(IrpContext, Vcb, 0);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = UDFPurgeVolume(IrpContext, Vcb);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    //  Now back out of our synchronization and wait for the lazy writer
    //  to finish off any lazy closes that could have been outstanding.
    //
    //  Since we purged, we know that the lazy writer will issue all
    //  possible lazy closes in the next tick - if we hadn't, an otherwise
    //  unopened file with a large amount of dirty data could have hung
    //  around for a while as the data trickled out to the disk.
    //
    //  This is even more important now since we send notification to
    //  alert other folks that this style of check is about to happen so
    //  that they can close their handles.  We don't want to enter a fast
    //  race with the lazy writer tearing down his references to the file.
    //

    UDFReleaseVcb(IrpContext, Vcb);

    Status = CcWaitForCurrentLazyWriterActivity();

    //
    //  This is intentional. If we were able to get the Vcb before, just
    //  wait for it and take advantage of knowing that it is OK to leave
    //  the flag up.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
    UDFAcquireVcbExclusive(IrpContext, Vcb, FALSE);
    
    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    UDFFspClose(Vcb);

    /* Purge and delayed closes can write metadata after the first full
     * flush. A successful volume lock must leave their LVID generation
     * closed and durable as well. */
    if (Vcb->Modified) {
        if (Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY)
            return STATUS_IO_DEVICE_ERROR;
        Status = UDFFlushVolume(IrpContext, Vcb, 0);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    //
    //  If the volume is already explicitly locked then fail.  We use the
    //  Vpb locked flag as an 'explicit lock' flag in the same way as Fat.
    //

    IoAcquireVpbSpinLock( &SavedIrql ); 

    if (Vcb->Vpb && !FlagOn(Vcb->Vpb->Flags, VPB_LOCKED) &&
        (Vcb->VcbCleanup == RemainingUserReferences) &&
        (Vcb->VcbUserReference == Vcb->VcbResidualUserReference + RemainingUserReferences)) {

        SetFlag(Vcb->VcbState, VCB_STATE_LOCKED);
        SetFlag(Vcb->Vpb->Flags, VPB_LOCKED);
        Vcb->VolumeLockFileObject = FileObject;
        FinalStatus = STATUS_SUCCESS;
    }
    
    IoReleaseVpbSpinLock( SavedIrql );  
    
    return FinalStatus;
}

/*
    This routine performs the unlock volume operation.  It is responsible for
    either completing of enqueuing the input Irp.
Arguments:
    Irp - Supplies the Irp to process
Return Value:
    NTSTATUS - The return status for the operation
*/
NTSTATUS
UDFUnlockVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    // Decode the file object, the only type of opens we accept are
    // user volume opens.

    if (UDFDecodeFileObject(IrpSp->FileObject, &Fcb, &Ccb) != UserVolumeOpen ) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    Vcb = Fcb->Vcb;

    ASSERT_CCB(Ccb);
    ASSERT_FCB(Fcb);
    ASSERT_VCB(Vcb);

    // Acquire the volume resource exclusive
    UDFAcquireVcbExclusive(IrpContext, Vcb, FALSE);

    // We won't check for a valid Vcb for this request.  An unlock will always
    // succeed on a locked volume.
    Status = UDFUnlockVolumeInternal(Vcb, IrpSp->FileObject);

    // Release all of our resources
    UDFReleaseVcb(IrpContext, Vcb);

    // Send notification that the volume is avaliable.
    if (NT_SUCCESS(Status)) {

        FsRtlNotifyVolumeEvent(IrpSp->FileObject, FSRTL_VOLUME_UNLOCK);
    }

    //  Complete the request if there haven't been any exceptions.

    UDFCompleteRequest(IrpContext, Irp, Status);

    return Status;
} // end UDFUnlockVolume()


/*
    This routine performs the dismount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.  We only dismount a volume which
    has been locked.  The intent here is that someone has locked the volume (they are the
    only remaining handle).  We set the verify bit here and the user will close his handle.
    We will dismount a volume with no user's handles in the verify path.
Arguments:
    Irp - Supplies the Irp to process
Return Value:
    NTSTATUS - The return status for the operation
*/
NTSTATUS
UDFDismountVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )
{
    NTSTATUS Status;
    KIRQL SavedIrql;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    // Decode the file object, the only type of opens we accept are
    // user volume opens.

    if (UDFDecodeFileObject(IrpSp->FileObject, &Fcb, &Ccb) != UserVolumeOpen ) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    Vcb = Fcb->Vcb;

    ASSERT_CCB(Ccb);
    ASSERT_FCB(Fcb);
    ASSERT_VCB(Vcb);

    // Send dismount notification.

    FsRtlNotifyVolumeEvent(IrpSp->FileObject, FSRTL_VOLUME_DISMOUNT);

    // Make this request waitable.

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT | IRP_CONTEXT_FLAG_ALLOW_MEDIA_EJECT);

    // Acquire exclusive access to the Vcb,  and take the global resource to
    // sync. against mounts,  verifies etc.

    UDFAcquireUdfData(IrpContext);
    UDFAcquireVcbExclusive(IrpContext, Vcb, FALSE);

    //  Mark the volume as needs to be verified, but only do it if
    //  the vcb is locked by this handle and the volume is currently mounted.

    if (Vcb->VcbCondition != VcbMounted) {

        Status = STATUS_VOLUME_DISMOUNTED;

    } else {

        Status = UDFFlushVolume(IrpContext, Vcb, 0);
        if (!NT_SUCCESS(Status))
            goto dismount_done;

        // Invalidate the volume right now.
        //
        // The intent here is to make every subsequent operation
        // on the volume fail and grease the rails toward dismount.
        // By definition there is no going back from a SURPRISE.

        UDFLockVcb(IrpContext, Vcb);

        if (Vcb->VcbCondition != VcbDismountInProgress) {

            UDFUpdateVcbCondition(Vcb, VcbInvalid);
        }

        UDFUnlockVcb(IrpContext, Vcb);

        // Set flag to tell the close path that we want to force dismount
        // the volume when this handle is closed.

        SetFlag(Ccb->Flags, CCB_FLAG_DISMOUNT_ON_CLOSE);

        // Set a flag in the VPB to let others know that direct volume access is allowed.

        IoAcquireVpbSpinLock(&SavedIrql);
        SetFlag(Vcb->Vpb->Flags, VPB_DIRECT_WRITES_ALLOWED);
        IoReleaseVpbSpinLock(SavedIrql);

        Status = STATUS_SUCCESS;
    }

 dismount_done:
     // Release all of our resources

     UDFReleaseVcb(IrpContext, Vcb);
     UDFReleaseUdfData(IrpContext);

#if (NTDDI_VERSION >= NTDDI_WIN8)

    FsRtlDismountComplete(Vcb->TargetDeviceObject, Status);

#endif

    UDFCompleteRequest(IrpContext, Irp, Status);
    return Status;
} // end UDFDismountVolume()

/*

    This routine returns the volume allocation bitmap.

        Input = the STARTING_LCN_INPUT_BUFFER data structure is passed in
            through the input buffer.
        Output = the VOLUME_BITMAP_BUFFER data structure is returned through
            the output buffer.

    We return as much as the user buffer allows starting the specified input
    LCN (trucated to a byte).  If there is no input buffer, we start at zero.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation.

 */
NTSTATUS
UDFGetVolumeBitmap(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    PFCB Fcb;
    PCCB Ccb;
    PVCB Vcb = IrpContext->Vcb;
    ULONG BytesToCopy;
    ULONG TotalClusters;
    ULONG StartingCluster;
    ULONG DesiredClusters;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    LARGE_INTEGER StartingLcn;
    PVOLUME_BITMAP_BUFFER OutputBuffer;
    ULONG i, lim;
    PULONG FSBM;
    BOOLEAN VcbAcquired = FALSE;

    ASSERT_VCB(Vcb);

    // Make this a synchronous IRP because we need access to the input buffer and
    // this Irp is marked METHOD_NEITHER.

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    TotalClusters = Vcb->FSBM_BitCount;

    _SEH2_TRY {

        // Decode the file object, the only type of opens we accept are
        // user volume opens.

        if (UDFDecodeFileObject(IrpSp->FileObject, &Fcb, &Ccb) != UserVolumeOpen) {

            try_return(Status = STATUS_INVALID_USER_BUFFER);
        }

        ASSERT_CCB(Ccb);
        ASSERT_FCB(Fcb);

        // Check for a minimum length on the input and output buffers.

        if ((InputBufferLength < sizeof(STARTING_LCN_INPUT_BUFFER)) ||
            (OutputBufferLength < sizeof(VOLUME_BITMAP_BUFFER))) {

            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        OutputBuffer = (PVOLUME_BITMAP_BUFFER)UDFMapUserBuffer(Irp);

        _SEH2_TRY {

            if (Irp->RequestorMode != KernelMode) {

                ProbeForRead(IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                    InputBufferLength,
                    sizeof(UCHAR));

                ProbeForWrite(OutputBuffer, OutputBufferLength, sizeof(UCHAR));
            }

            StartingLcn = ((PSTARTING_LCN_INPUT_BUFFER)IrpSp->Parameters.FileSystemControl.Type3InputBuffer)->StartingLcn;

        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {

            try_return(Status = STATUS_INVALID_USER_BUFFER);
        } _SEH2_END

        if (StartingLcn.HighPart || StartingLcn.LowPart >= TotalClusters) {

            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        // Align starting block to 8-bit boundary

        StartingCluster = StartingLcn.LowPart & ~7;

        DesiredClusters = TotalClusters - StartingCluster;

        // Fill in the fixed part of the output buffer

        _SEH2_TRY {

            // StartingLcn in output = aligned starting block

            OutputBuffer->StartingLcn.QuadPart = StartingCluster;

            // BitmapSize = remaining blocks from starting position

            OutputBuffer->BitmapSize.QuadPart = DesiredClusters;

        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {

            try_return(Status = STATUS_INVALID_USER_BUFFER);
        } _SEH2_END;

        OutputBufferLength -= FIELD_OFFSET(VOLUME_BITMAP_BUFFER, Buffer);


        if (OutputBufferLength < (DesiredClusters + 7) / 8) {

            BytesToCopy = OutputBufferLength;
            Status = STATUS_BUFFER_OVERFLOW;

        } else {

            BytesToCopy = (DesiredClusters + 7) / 8;
            Status = STATUS_SUCCESS;
        }

        UDFAcquireVcbShared(IrpContext, Vcb, FALSE);
        VcbAcquired = TRUE;

        RtlZeroMemory( &OutputBuffer->Buffer[0], BytesToCopy );
        lim = BytesToCopy * 8;
        FSBM = (PULONG)(&OutputBuffer->Buffer[0]);

        for(i=StartingCluster & ~7; i<lim; i++) {
            BOOLEAN isFree;
            if (Vcb->BitmapFcb) {
                isFree = UDFIsBitmapBitFree(Vcb, i);
            } else {
                isFree = UDFGetFreeBit((PULONG)(Vcb->FSBM_Bitmap), i);
            }
            if (isFree)
                UDFSetFreeBit(FSBM, i);
        }
        UDFUnpinBitmapPage(Vcb);

        Irp->IoStatus.Information = FIELD_OFFSET(VOLUME_BITMAP_BUFFER, Buffer) + BytesToCopy;

try_exit:    NOTHING;

    } _SEH2_FINALLY {

        if (VcbAcquired) {

            UDFReleaseVcb(IrpContext, Vcb);
        }
            
    } _SEH2_END;

    // Complete the request

    UDFCompleteRequest(IrpContext, Irp, Status);

    return Status;


} // end UDFGetVolumeBitmap()


NTSTATUS
UDFGetRetrievalPointers(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP  Irp
    )
{
    NTSTATUS RC;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PUDF_FILE_INFO FileInfo;

    ULONG InputBufferLength;
    ULONG OutputBufferLength;

    PRETRIEVAL_POINTERS_BUFFER OutputBuffer;
    PSTARTING_VCN_INPUT_BUFFER InputBuffer;

    LARGE_INTEGER StartingVcn;
    int64 AllocationSize;

    PCCB Ccb;
    PFCB Fcb;
    PVCB Vcb;
    TYPE_OF_OPEN TypeOfOpen;

    PEXTENT_MAP SubMapping = NULL;
    ULONG SubExtInfoSz;
    ULONG i;
    ULONG LBS;
    ULONG LBSh;

    UDFPrint(("UDFGetRetrievalPointers\n"));

    // Make this a synchronous IRP because we need access to the input buffer and
    // this Irp is marked METHOD_NEITHER.

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    TypeOfOpen = UDFDecodeFileObject(IrpSp->FileObject, &Fcb, &Ccb);

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    Vcb = Fcb->Vcb;

    ASSERT_CCB(Ccb);
    ASSERT_FCB(Fcb);
    ASSERT_VCB(Vcb);

    if (!Ccb) {

        UDFPrintErr(("  !Ccb\n"));
        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //  Get the input and output buffer lengths and pointers.
    //  Initialize some variables.
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    OutputBuffer = (PRETRIEVAL_POINTERS_BUFFER)UDFMapUserBuffer(Irp);

    InputBuffer = (PSTARTING_VCN_INPUT_BUFFER)IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    if (!InputBuffer) {
        InputBuffer = (PSTARTING_VCN_INPUT_BUFFER)OutputBuffer;
    }

    _SEH2_TRY {

        Irp->IoStatus.Information = 0;
        //  Check for a minimum length on the input and ouput buffers.
        if ((InputBufferLength < sizeof(STARTING_VCN_INPUT_BUFFER)) ||
            (OutputBufferLength < sizeof(RETRIEVAL_POINTERS_BUFFER))) {

            try_return( RC = STATUS_BUFFER_TOO_SMALL );
        }

        _SEH2_TRY {

            if (Irp->RequestorMode != KernelMode) {
                ProbeForRead( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                              InputBufferLength,
                              sizeof(UCHAR) );
                ProbeForWrite( OutputBuffer, OutputBufferLength, sizeof(UCHAR) );
            }
            StartingVcn = InputBuffer->StartingVcn;

        } _SEH2_EXCEPT(Irp->RequestorMode != KernelMode ? EXCEPTION_EXECUTE_HANDLER: EXCEPTION_CONTINUE_SEARCH) {

            RC = _SEH2_GetExceptionCode();
            RC = FsRtlIsNtstatusExpected(RC) ?
                              RC : STATUS_INVALID_USER_BUFFER;
            try_return(RC);
        } _SEH2_END;

        FileInfo = Fcb->FileInfo;

        if (!FileInfo) {
            try_return( RC = STATUS_OBJECT_NAME_NOT_FOUND );
        }

        AllocationSize = UDFGetFileAllocationSize(Vcb, FileInfo);

        LBS   = Vcb->SectorSize;
        LBSh  = Vcb->SectorShift;

        if (StartingVcn.HighPart ||
            StartingVcn.LowPart >= (ULONG)(AllocationSize >> LBSh)) {

            try_return( RC = STATUS_END_OF_FILE );
        }

        SubExtInfoSz = (OutputBufferLength - FIELD_OFFSET(RETRIEVAL_POINTERS_BUFFER, Extents[0])) / (sizeof(LARGE_INTEGER)*2);
        // re-use AllocationSize as NextVcn
        RC = UDFReadFileLocation__(Vcb, FileInfo, StartingVcn.QuadPart << LBSh,
                                   &SubMapping, &SubExtInfoSz, &AllocationSize);
        if (!NT_SUCCESS(RC))
            try_return(RC);

        OutputBuffer->ExtentCount = SubExtInfoSz;
        OutputBuffer->StartingVcn = StartingVcn;
        for(i=0; i<SubExtInfoSz; i++) {
            // assume, that
            // for not-allocated extents we have start Lba = -1
            // for not-recorded extents start Lba.LowPart contains real Lba, Lba.HighPart = 0x80000000
            // for recorded extents Lba.LowPart contains real Lba, Lba.HighPart = 0
            if (SubMapping[i].extLocation == LBA_NOT_ALLOCATED) {
                OutputBuffer->Extents[i].Lcn.QuadPart = (int64)(-1);
            } else
            if (SubMapping[i].extLocation & 0x80000000) {
                /* extLocation is already a sector LBA, not a byte offset. */
                OutputBuffer->Extents[i].Lcn.LowPart = SubMapping[i].extLocation & 0x7fffffff;
                OutputBuffer->Extents[i].Lcn.HighPart = 0x80000000;
            } else {
                OutputBuffer->Extents[i].Lcn.LowPart = SubMapping[i].extLocation;
                OutputBuffer->Extents[i].Lcn.HighPart = 0;
            }
            // alignment for last sector
            SubMapping[i].extLength += LBS-1;
            StartingVcn.QuadPart += SubMapping[i].extLength   >> LBSh;
            OutputBuffer->Extents[i].NextVcn = StartingVcn;
        }

        Irp->IoStatus.Information = FIELD_OFFSET(RETRIEVAL_POINTERS_BUFFER, Extents[0]) + i * sizeof(LARGE_INTEGER) * 2;

try_exit:   NOTHING;
    } _SEH2_FINALLY {

        if (SubMapping)
            MyFreePool__(SubMapping);
    } _SEH2_END;

    UDFCompleteRequest(IrpContext, Irp, RC);

    return RC;
} // end UDFGetRetrievalPointers()


NTSTATUS
UDFIsVolumeDirty(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )
{
    PULONG VolumeState;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PCCB Ccb;
    PFCB Fcb;
    PVCB Vcb;

    // Get a pointer to the output buffer.

    if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        VolumeState = (PULONG)Irp->AssociatedIrp.SystemBuffer;

    } else {

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_USER_BUFFER);
        return STATUS_INVALID_USER_BUFFER;
    }

    // Make sure the output buffer is large enough and then initialize
    // the answer to be that the volume isn't dirty.

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(ULONG)) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    *VolumeState = 0;

    if (UDFDecodeFileObject(IrpSp->FileObject, &Fcb, &Ccb) != UserVolumeOpen) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);

        return STATUS_INVALID_PARAMETER;
    }

    Vcb = Fcb->Vcb;

    ASSERT_CCB(Ccb);
    ASSERT_FCB(Fcb);
    ASSERT_VCB(Vcb);

    if (Vcb->VcbCondition != VcbMounted) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_VOLUME_DISMOUNTED);
        return STATUS_VOLUME_DISMOUNTED;
    }

    if (Vcb->origIntegrityType == INTEGRITY_TYPE_OPEN) {

        SetFlag(*VolumeState, VOLUME_IS_DIRTY);
    }

    Irp->IoStatus.Information = sizeof(ULONG);

    UDFCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);
    return STATUS_SUCCESS;

} // end UDFIsVolumeDirty()

NTSTATUS
UDFAllowExtendedDasdIo(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFCB Fcb;
    PCCB Ccb;

    // Extract and decode the file object and check for type of open.

    if (UDFDecodeFileObject(IrpSp->FileObject, &Fcb, &Ccb) != UserVolumeOpen) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    SetFlag(Ccb->Flags, CCB_FLAG_ALLOW_EXTENDED_DASD_IO);

    UDFCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);
    return STATUS_SUCCESS;
}


NTSTATUS
UDFInvalidateVolumes(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIRP Irp
    )
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    UDFPrint(("UDFInvalidateVolumes\n"));

    KIRQL SavedIrql;
    LUID TcbPrivilege = {SE_TCB_PRIVILEGE, 0};
    HANDLE Handle;
    PVCB Vcb;
    PLIST_ENTRY Links;
    PFILE_OBJECT FileToMarkBad;
    PDEVICE_OBJECT DeviceToMarkBad;

    Irp->IoStatus.Information = 0;

    //  Check for the correct security access.
    //  The caller must have the SeTcbPrivilege.
    if (IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
        IrpSp->MinorFunction == IRP_MN_USER_FS_REQUEST &&
        IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_INVALIDATE_VOLUMES &&
        !SeSinglePrivilegeCheck(TcbPrivilege, Irp->RequestorMode)) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_PRIVILEGE_NOT_HELD);
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    //  Try to get a pointer to the device object from the handle passed in.
#ifdef _WIN64
    if (IoIs32bitProcess(Irp)) {
        if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof(UINT32)) {

            UDFPrintErr(("UDFInvalidateVolumes: STATUS_INVALID_PARAMETER\n"));
            UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
            return STATUS_INVALID_PARAMETER;
        }

        Handle = (HANDLE)LongToHandle((*(UINT32*)Irp->AssociatedIrp.SystemBuffer));
    } else {
#endif
        if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof(HANDLE)) {

            UDFPrintErr(("UDFInvalidateVolumes: STATUS_INVALID_PARAMETER\n"));
            UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
            return STATUS_INVALID_PARAMETER;
        }

        Handle = *(HANDLE*)Irp->AssociatedIrp.SystemBuffer;
#ifdef _WIN64
    }
#endif

    Status = ObReferenceObjectByHandle(Handle,
                                       0,
                                       *IoFileObjectType,
                                       KernelMode,
                                       (PVOID*)&FileToMarkBad,
                                       NULL);

    if (!NT_SUCCESS(Status)) {

        UDFCompleteRequest(IrpContext, Irp, Status);
        return Status;
    }

    // Grab the DeviceObject from the FileObject.

    DeviceToMarkBad = FileToMarkBad->DeviceObject;

    // We only needed the pointer, not a reference.

    ObDereferenceObject(FileToMarkBad);

    // Make sure this request can wait.

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
    ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST);

    // Synchronise with pnp/mount/verify paths.

    UDFAcquireUdfData(IrpContext);

    //  Nothing can go wrong now.

    // Now walk through all the mounted Vcb's looking for candidates to
    // mark invalid.
    //
    // On volumes we mark invalid, check for dismount possibility (which is
    // why we have to get the next link so early).

    Links = UdfData.VcbQueue.Flink;

    while (Links != &UdfData.VcbQueue) {

        // Get 'next' Vcb
        Vcb = CONTAINING_RECORD(Links, struct VCB, VcbLinks);

        // Move to the next link now since the current Vcb may be deleted.
        Links = Links->Flink;

        // If we get a match, mark the volume Bad, and also check to
        // see if the volume should go away.

        UDFLockVcb(IrpContext, Vcb);

        if (Vcb->Vpb->RealDevice == DeviceToMarkBad) {

            // Take the VPB spinlock,  and look to see if this volume is the 
            // one currently mounted on the actual device.  If it is,  pull it 
            // off immediately.
            IoAcquireVpbSpinLock(&SavedIrql);

#pragma prefast(push)
#pragma prefast(disable: 28175, "this is a filesystem driver, touching the vpb is allowed")
            if (DeviceToMarkBad->Vpb == Vcb->Vpb) {

                PVPB NewVpb = Vcb->SwapVpb;

                ASSERT(FlagOn(Vcb->Vpb->Flags, VPB_MOUNTED));
                ASSERT(NewVpb);

                RtlZeroMemory(NewVpb, sizeof(VPB));

                NewVpb->Type = IO_TYPE_VPB;
                NewVpb->Size = sizeof(VPB);
                NewVpb->RealDevice = DeviceToMarkBad;
                NewVpb->Flags = FlagOn(DeviceToMarkBad->Vpb->Flags, VPB_REMOVE_PENDING);
                DeviceToMarkBad->Vpb = NewVpb;

                Vcb->SwapVpb = NULL;
            }

#pragma prefast(pop)

            IoReleaseVpbSpinLock(SavedIrql);

            if (Vcb->VcbCondition != VcbDismountInProgress) {

                UDFUpdateVcbCondition( Vcb, VcbInvalid);
            }

            UDFUnlockVcb(IrpContext, Vcb);

            UDFAcquireVcbExclusive(IrpContext, Vcb, FALSE);

            UDFFlushVolume(IrpContext, Vcb, 0);

            UDFToggleMediaEjectDisable(Vcb, FALSE);

            UDFReleaseVcb( IrpContext, Vcb);

        } else {

            UDFUnlockVcb(IrpContext, Vcb);
        }

    }

    UDFScanForDismountedVcb(IrpContext);

    UDFReleaseUdfData(IrpContext);

    UDFCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);
    return STATUS_SUCCESS;

} // end UDFInvalidateVolumes()

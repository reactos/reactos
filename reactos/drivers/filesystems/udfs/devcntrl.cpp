////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: Devcntrl.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains code to handle the "Device IOCTL" dispatch entry point.
*
*************************************************************************/

#include "udffs.h"

#include "CDRW/scsi_port.h"

#define UDF_CURRENT_BUILD 123456789

// define the file specific bug-check id
#ifdef UDF_BUG_CHECK_ID
#undef UDF_BUG_CHECK_ID
#endif
#define         UDF_BUG_CHECK_ID                UDF_FILE_DEVICE_CONTROL

NTSTATUS
UDFGetFileAllocModeFromICB(
    PtrUDFIrpContext PtrIrpContext,
    PIRP             Irp
    );

NTSTATUS
UDFSetFileAllocModeFromICB(
    PtrUDFIrpContext IrpContext,
    PIRP             Irp
    );

NTSTATUS
UDFProcessLicenseKey(
    PtrUDFIrpContext IrpContext,
    PIRP             Irp
    );

/*#if(_WIN32_WINNT < 0x0400)
#define IOCTL_REDIR_QUERY_PATH   CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 99, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef struct _QUERY_PATH_REQUEST {
    ULONG PathNameLength;
    PIO_SECURITY_CONTEXT SecurityContext;
    WCHAR FilePathName[1];
} QUERY_PATH_REQUEST, *PQUERY_PATH_REQUEST;

typedef struct _QUERY_PATH_RESPONSE {
    ULONG LengthAccepted;
} QUERY_PATH_RESPONSE, *PQUERY_PATH_RESPONSE;

#endif*/


/*************************************************************************
*
* Function: UDFDeviceControl()
*
* Description:
*   The I/O Manager will invoke this routine to handle a Device IOCTL
*   request
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL (invocation at higher IRQL will cause execution
*   to be deferred to a worker thread context)
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
NTAPI
UDFDeviceControl(
    PDEVICE_OBJECT          DeviceObject,       // the logical volume device object
    PIRP                    Irp)                // I/O Request Packet
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PtrUDFIrpContext    PtrIrpContext = NULL;
    BOOLEAN             AreWeTopLevel = FALSE;

    TmPrint(("UDFDeviceControl: \n"));

    FsRtlEnterFileSystem();
    ASSERT(DeviceObject);
    ASSERT(Irp);

    // set the top level context
    AreWeTopLevel = UDFIsIrpTopLevel(Irp);
    //ASSERT(!UDFIsFSDevObj(DeviceObject));

    _SEH2_TRY {

        // get an IRP context structure and issue the request
        PtrIrpContext = UDFAllocateIrpContext(Irp, DeviceObject);
        if(PtrIrpContext) {
            RC = UDFCommonDeviceControl(PtrIrpContext, Irp);
        } else {
            RC = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

    } _SEH2_EXCEPT(UDFExceptionFilter(PtrIrpContext, _SEH2_GetExceptionInformation())) {

        RC = UDFExceptionHandler(PtrIrpContext, Irp);

        UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);
    } _SEH2_END;

    if (AreWeTopLevel) {
        IoSetTopLevelIrp(NULL);
    }

    FsRtlExitFileSystem();

    return(RC);
} // end UDFDeviceControl()


/*************************************************************************
*
* Function: UDFCommonDeviceControl()
*
* Description:
*   The actual work is performed here. This routine may be invoked in one'
*   of the two possible contexts:
*   (a) in the context of a system worker thread
*   (b) in the context of the original caller
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
NTAPI
UDFCommonDeviceControl(
    PtrUDFIrpContext PtrIrpContext,
    PIRP             Irp
    )
{
    NTSTATUS                RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION      IrpSp = NULL;
//    PIO_STACK_LOCATION      PtrNextIoStackLocation = NULL;
    PFILE_OBJECT            FileObject = NULL;
    PtrUDFFCB               Fcb = NULL;
    PtrUDFCCB               Ccb = NULL;
    PVCB                    Vcb = NULL;
    BOOLEAN                 CompleteIrp = FALSE;
    ULONG                   IoControlCode = 0;
//    PVOID                   BufferPointer = NULL;
    BOOLEAN                 AcquiredVcb = FALSE;
    BOOLEAN                 FSDevObj;
    ULONG                   TrackNumber;
    BOOLEAN                 UnsafeIoctl = TRUE;
    UCHAR                   ScsiCommand;
    PPREVENT_MEDIA_REMOVAL_USER_IN Buf = NULL;    // FSD buffer
    PCDB                    Cdb;
    PCHAR                   CdbData;
    PCHAR                   ModeSelectData;

    KdPrint(("UDFCommonDeviceControl\n"));

    _SEH2_TRY {
        // First, get a pointer to the current I/O stack location
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(IrpSp);

        // Get the IoControlCode value
        IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

        FileObject = IrpSp->FileObject;
        ASSERT(FileObject);

        FSDevObj = UDFIsFSDevObj(PtrIrpContext->TargetDeviceObject);

        if(FSDevObj) {
            switch (IoControlCode) {
            case IOCTL_UDF_DISABLE_DRIVER:
            case IOCTL_UDF_INVALIDATE_VOLUMES:
            case IOCTL_UDF_SET_NOTIFICATION_EVENT:
#ifndef UDF_READ_ONLY_BUILD
            case IOCTL_UDF_SEND_LICENSE_KEY:
#endif //UDF_READ_ONLY_BUILD
            case IOCTL_UDF_REGISTER_AUTOFORMAT:
                break;
            default:
                KdPrint(("UDFCommonDeviceControl: STATUS_INVALID_PARAMETER %x for FsDevObj\n", IoControlCode));
                CompleteIrp = TRUE;
                try_return(RC = STATUS_INVALID_PARAMETER);
            }
        } else {
            Ccb = (PtrUDFCCB)(FileObject->FsContext2);
            if(!Ccb) {
                KdPrint(("  !Ccb\n"));
                goto ioctl_do_default;
            }
            ASSERT(Ccb);
            Fcb = Ccb->Fcb;
            ASSERT(Fcb);

            // Check if the IOCTL is suitable for this type of File
            if (Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB) {
                // Everything is acceptable for Volume
                Vcb = (PVCB)(Fcb);
            } else {
                Vcb = Fcb->Vcb;
                CompleteIrp = TRUE;
                // For files/disrs only the following are acceptable
                switch (IoControlCode) {
                case IOCTL_UDF_GET_RETRIEVAL_POINTERS:
                case IOCTL_UDF_GET_FILE_ALLOCATION_MODE:
                case IOCTL_UDF_SET_FILE_ALLOCATION_MODE:
                    break;
                default:
                    KdPrint(("UDFCommonDeviceControl: STATUS_INVALID_PARAMETER %x for File/Dir Obj\n", IoControlCode));
                    try_return(RC = STATUS_INVALID_PARAMETER);
                }
            }
            // check 'safe' IOCTLs
            switch (IoControlCode) {
            case IOCTL_CDROM_RAW_READ:

            case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
            case IOCTL_DISK_GET_DRIVE_GEOMETRY:
            case IOCTL_DISK_GET_PARTITION_INFO:
            case IOCTL_DISK_GET_DRIVE_LAYOUT:

            case IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX:
            case IOCTL_DISK_GET_PARTITION_INFO_EX:
            case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
            case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:

            case IOCTL_STORAGE_CHECK_VERIFY:
            case IOCTL_STORAGE_CHECK_VERIFY2:
            case IOCTL_DISK_CHECK_VERIFY:
            case IOCTL_CDROM_CHECK_VERIFY:

            case IOCTL_CDROM_LOAD_MEDIA:
            case IOCTL_DISK_LOAD_MEDIA:
            case IOCTL_STORAGE_LOAD_MEDIA:
            case IOCTL_STORAGE_LOAD_MEDIA2:

            case IOCTL_CDROM_GET_CONFIGURATION:
            case IOCTL_CDROM_GET_LAST_SESSION:
            case IOCTL_CDROM_READ_TOC:
            case IOCTL_CDROM_READ_TOC_EX:
            case IOCTL_CDROM_PLAY_AUDIO_MSF:
            case IOCTL_CDROM_READ_Q_CHANNEL:
            case IOCTL_CDROM_PAUSE_AUDIO:
            case IOCTL_CDROM_RESUME_AUDIO:
            case IOCTL_CDROM_SEEK_AUDIO_MSF:
            case IOCTL_CDROM_STOP_AUDIO:
            case IOCTL_CDROM_GET_CONTROL:
            case IOCTL_CDROM_GET_VOLUME:
            case IOCTL_CDROM_SET_VOLUME:

            case IOCTL_CDRW_SET_SPEED:
            case IOCTL_CDRW_GET_CAPABILITIES:
            case IOCTL_CDRW_GET_MEDIA_TYPE_EX:
            case IOCTL_CDRW_GET_MEDIA_TYPE:

            case IOCTL_DISK_GET_MEDIA_TYPES:
            case IOCTL_STORAGE_GET_MEDIA_TYPES:
            case IOCTL_STORAGE_GET_MEDIA_TYPES_EX:

            case IOCTL_DISK_IS_WRITABLE:

            case IOCTL_CDRW_GET_WRITE_MODE:
            case IOCTL_CDRW_READ_TRACK_INFO:
            case IOCTL_CDRW_READ_DISC_INFO:
            case IOCTL_CDRW_BUFFER_CAPACITY:
            case IOCTL_CDRW_GET_SIGNATURE:
            case IOCTL_CDRW_TEST_UNIT_READY:
            case IOCTL_CDRW_GET_LAST_ERROR:
            case IOCTL_CDRW_MODE_SENSE:
            case IOCTL_CDRW_LL_READ:
            case IOCTL_CDRW_READ_ATIP:
            case IOCTL_CDRW_READ_CD_TEXT:
            case IOCTL_CDRW_READ_TOC_EX:
            case IOCTL_CDRW_READ_FULL_TOC:
            case IOCTL_CDRW_READ_PMA:
            case IOCTL_CDRW_READ_SESSION_INFO:
            case IOCTL_CDRW_GET_DEVICE_INFO:
            case IOCTL_CDRW_GET_EVENT:

            case IOCTL_DVD_READ_STRUCTURE:

            case IOCTL_CDRW_GET_DEVICE_NAME:
            case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:

            case IOCTL_UDF_GET_RETRIEVAL_POINTERS:
            case IOCTL_UDF_GET_SPEC_RETRIEVAL_POINTERS:
            case IOCTL_UDF_GET_FILE_ALLOCATION_MODE:
            case IOCTL_UDF_GET_VERSION:
            case IOCTL_UDF_IS_VOLUME_JUST_MOUNTED:
            case IOCTL_UDF_SET_OPTIONS:
//            case :

            case FSCTL_IS_VOLUME_DIRTY:

                UnsafeIoctl = FALSE;
                break;
            }

            if(IoControlCode != IOCTL_CDROM_DISK_TYPE) {
                UDFAcquireResourceShared(&(Vcb->VCBResource), TRUE);
            } else {
                UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
            }
            AcquiredVcb = TRUE;
        }

        KdPrint(("UDF Irp %x, ctx %x, DevIoCtl %x\n", Irp, PtrIrpContext, IoControlCode));

        // We may wish to allow only   volume open operations.
        switch (IoControlCode) {

        case IOCTL_SCSI_PASS_THROUGH_DIRECT:
        case IOCTL_SCSI_PASS_THROUGH:

            if(!Irp->AssociatedIrp.SystemBuffer)
                goto ioctl_do_default;

            if(IoControlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT) {
                Cdb = (PCDB)&(((PSCSI_PASS_THROUGH_DIRECT)(Irp->AssociatedIrp.SystemBuffer))->Cdb);
                CdbData = (PCHAR)(((PSCSI_PASS_THROUGH_DIRECT)(Irp->AssociatedIrp.SystemBuffer))->DataBuffer);
            } else {
                Cdb = (PCDB)&(((PSCSI_PASS_THROUGH)(Irp->AssociatedIrp.SystemBuffer))->Cdb);
                if(((PSCSI_PASS_THROUGH)(Irp->AssociatedIrp.SystemBuffer))->DataBufferOffset) {
                    CdbData = ((PCHAR)Cdb) + 
                              ((PSCSI_PASS_THROUGH)(Irp->AssociatedIrp.SystemBuffer))->DataBufferOffset;
                } else {
                    CdbData = NULL;
                }
            }
            ScsiCommand = Cdb->CDB6.OperationCode;

            if(ScsiCommand == SCSIOP_WRITE_CD) {
                KdPrint(("Write10, LBA %2.2x%2.2x%2.2x%2.2x\n",
                         Cdb->WRITE_CD.LBA[0],
                         Cdb->WRITE_CD.LBA[1],
                         Cdb->WRITE_CD.LBA[2],
                         Cdb->WRITE_CD.LBA[3]
                         ));
            } else
            if(ScsiCommand == SCSIOP_WRITE12) {
                KdPrint(("Write12, LBA %2.2x%2.2x%2.2x%2.2x\n",
                         Cdb->CDB12READWRITE.LBA[0],
                         Cdb->CDB12READWRITE.LBA[1],
                         Cdb->CDB12READWRITE.LBA[2],
                         Cdb->CDB12READWRITE.LBA[3]
                         ));
            } else {
            }

            switch(ScsiCommand) {
            case SCSIOP_MODE_SELECT: {
//                PMODE_PARAMETER_HEADER ParamHdr = (PMODE_PARAMETER_HEADER)CdbData;
                ModeSelectData = CdbData+4;
                switch(ModeSelectData[0]) {
                case MODE_PAGE_MRW2:
                case MODE_PAGE_WRITE_PARAMS:
                case MODE_PAGE_MRW:
                    KdPrint(("Unsafe MODE_SELECT_6 via pass-through (%2.2x)\n", ModeSelectData[0]));
                    goto unsafe_direct_scsi_cmd;
                }
                break; }

            case SCSIOP_MODE_SELECT10: {
//                PMODE_PARAMETER_HEADER10 ParamHdr = (PMODE_PARAMETER_HEADER10)CdbData;
                ModeSelectData = CdbData+8;
                switch(ModeSelectData[0]) {
                case MODE_PAGE_MRW2:
                case MODE_PAGE_WRITE_PARAMS:
                case MODE_PAGE_MRW:
                    KdPrint(("Unsafe MODE_SELECT_10 via pass-through (%2.2x)\n", ModeSelectData[0]));
                    goto unsafe_direct_scsi_cmd;
                }
                break; }

            case SCSIOP_RESERVE_TRACK:
            case SCSIOP_SEND_CUE_SHEET:
            case SCSIOP_SEND_DVD_STRUCTURE:
            case SCSIOP_CLOSE_TRACK_SESSION:
            case SCSIOP_FORMAT_UNIT:
            case SCSIOP_WRITE6:
            case SCSIOP_WRITE_CD:
            case SCSIOP_BLANK:
            case SCSIOP_WRITE12:
            case SCSIOP_SET_STREAMING:
                KdPrint(("UDF Direct media modification via pass-through (%2.2x)\n", ScsiCommand));
unsafe_direct_scsi_cmd:
                if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED))
                    goto ioctl_do_default;

                KdPrint(("Forget this volume\n"));
                // Acquire Vcb resource (Shared -> Exclusive)
                UDFInterlockedIncrement((PLONG)&(Vcb->VCBOpenCount));
                UDFReleaseResource(&(Vcb->VCBResource));

                if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK)) {
                    UDFCloseAllSystemDelayedInDir(Vcb, Vcb->RootDirFCB->FileInfo);
                }
#ifdef UDF_DELAYED_CLOSE
                //  Acquire exclusive access to the Vcb.
                UDFCloseAllDelayed(Vcb);
#endif //UDF_DELAYED_CLOSE

                // allocate tmp buffer for FSD calls
                Buf = (PPREVENT_MEDIA_REMOVAL_USER_IN)MyAllocatePool__(NonPagedPool, sizeof(PREVENT_MEDIA_REMOVAL_USER_IN));
                if(!Buf)
                    try_return(RC = STATUS_INSUFFICIENT_RESOURCES);

                UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
                AcquiredVcb = TRUE;
                UDFInterlockedDecrement((PLONG)&(Vcb->VCBOpenCount));

                UDFDoDismountSequence(Vcb, Buf, FALSE);
                MyFreePool__(Buf);
                Buf = NULL;
                Vcb->MediaLockCount = 0;

                Vcb->VCBFlags &= ~UDF_VCB_FLAGS_VOLUME_MOUNTED;
                Vcb->WriteSecurity = FALSE;

                // Release the Vcb resource.
                UDFReleaseResource(&(Vcb->VCBResource));
                AcquiredVcb = FALSE;
                // disable Eject Request Waiter if any
                UDFStopEjectWaiter(Vcb);

                // Make sure, that volume will never be quick-remounted
                // It is very important for ChkUdf utility and
                // some CD-recording libraries
                Vcb->SerialNumber--;

                KdPrint(("Forgotten\n"));

                goto notify_media_change;

            case SCSIOP_START_STOP_UNIT:
            case SCSIOP_DOORLOCK:
            case SCSIOP_DOORUNLOCK:
            case SCSIOP_MEDIUM_REMOVAL:
                KdPrint(("UDF Medium/Tray control IOCTL via pass-through\n"));
            }
            goto ioctl_do_default;

        case IOCTL_CDRW_BLANK:
        case IOCTL_CDRW_LL_WRITE:
        case IOCTL_CDRW_FORMAT_UNIT:

notify_media_change:
/*            Vcb->VCBFlags |= UDF_VCB_FLAGS_UNSAFE_IOCTL;
            // Make sure, that volume will never be quick-remounted
            // It is very important for ChkUdf utility and
            // some CD-recording libraries
            Vcb->SerialNumber--;
*/          goto ioctl_do_default;

        case IOCTL_UDF_REGISTER_AUTOFORMAT: {

            KdPrint(("UDF Register Autoformat\n"));
            if(UDFGlobalData.AutoFormatCount) {
                RC = STATUS_SHARING_VIOLATION;
            } else {
                UDFGlobalData.AutoFormatCount = FileObject;
                RC = STATUS_SUCCESS;
            }
            CompleteIrp = TRUE;
            Irp->IoStatus.Information = 0;
            break;
        }

        case IOCTL_UDF_DISABLE_DRIVER: {

            KdPrint(("UDF Disable driver\n"));
            IoUnregisterFileSystem(UDFGlobalData.UDFDeviceObject);
            // Now, delete any device objects, etc. we may have created
            if (UDFGlobalData.UDFDeviceObject) {
                IoDeleteDevice(UDFGlobalData.UDFDeviceObject);
                UDFGlobalData.UDFDeviceObject = NULL;
            }

            // free up any memory we might have reserved for zones/lookaside
            //  lists
            if (UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_ZONES_INITIALIZED) {
                UDFDestroyZones();
            }

            // delete the resource we may have initialized
            if (UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_RESOURCE_INITIALIZED) {
                // un-initialize this resource
                UDFDeleteResource(&(UDFGlobalData.GlobalDataResource));
                UDFClearFlag(UDFGlobalData.UDFFlags, UDF_DATA_FLAGS_RESOURCE_INITIALIZED);
            }
            RC = STATUS_SUCCESS;
            CompleteIrp = TRUE;
            Irp->IoStatus.Information = 0;
            break;
        }
        case IOCTL_UDF_INVALIDATE_VOLUMES: {
            KdPrint(("UDF Invaidate volume\n"));
            if(AcquiredVcb) {
                UDFReleaseResource(&(Vcb->VCBResource));
                AcquiredVcb = FALSE;
            }
            RC = UDFInvalidateVolumes( PtrIrpContext, Irp );
            CompleteIrp = TRUE;
            Irp->IoStatus.Information = 0;
            break;
        }

        case IOCTL_UDF_SET_NOTIFICATION_EVENT:
        {
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(HANDLE))
            {
                RC = STATUS_INVALID_PARAMETER;
            }
            else
            {
                HANDLE MountEventHandle = *(PHANDLE)Irp->AssociatedIrp.SystemBuffer;
                if (MountEventHandle)
                {
                    if (!UDFGlobalData.MountEvent)
                    {
                        RC = ObReferenceObjectByHandle(
                            MountEventHandle,
                            0,
                            NULL,
                            UserMode,
                            (PVOID *) &UDFGlobalData.MountEvent,
                            NULL);
                            
                        if (!NT_SUCCESS(RC))
                        {
                            UDFGlobalData.MountEvent = NULL;
                        }
                    }
                    else
                    {
                        RC = STATUS_INVALID_PARAMETER;
                    }
                }
                else
                {
                    if (!UDFGlobalData.MountEvent)
                    {
                        RC = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        ObDereferenceObject(UDFGlobalData.MountEvent);
                        UDFGlobalData.MountEvent = NULL;
                    }
                }
            }

            CompleteIrp = TRUE;
            Irp->IoStatus.Information = 0;
            break;
        }

        case IOCTL_UDF_IS_VOLUME_JUST_MOUNTED:
        {
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(BOOLEAN))
            {
                RC = STATUS_INVALID_PARAMETER;
            }
            else
            {
                *(PBOOLEAN)Irp->AssociatedIrp.SystemBuffer = Vcb->IsVolumeJustMounted;
                Vcb->IsVolumeJustMounted = FALSE;
            }
            
            CompleteIrp = TRUE;
            Irp->IoStatus.Information = 0;
            break;
        }


        //case FSCTL_GET_RETRIEVAL_POINTERS
        case IOCTL_UDF_GET_RETRIEVAL_POINTERS: {
            KdPrint(("UDF: Get Retrieval Pointers\n"));
            RC = UDFGetRetrievalPointers( PtrIrpContext, Irp, 0 );
            CompleteIrp = TRUE;
            break;
        }
        case IOCTL_UDF_GET_SPEC_RETRIEVAL_POINTERS: {
            KdPrint(("UDF: Get Spec Retrieval Pointers\n"));
            PUDF_GET_SPEC_RETRIEVAL_POINTERS_IN SpecRetrPointer;
            SpecRetrPointer = (PUDF_GET_SPEC_RETRIEVAL_POINTERS_IN)(Irp->AssociatedIrp.SystemBuffer);
            RC = UDFGetRetrievalPointers( PtrIrpContext, Irp, SpecRetrPointer->Special );
            CompleteIrp = TRUE;
            break;
        }
        case IOCTL_UDF_GET_FILE_ALLOCATION_MODE: {
            KdPrint(("UDF: Get File Alloc mode (from ICB)\n"));
            RC = UDFGetFileAllocModeFromICB( PtrIrpContext, Irp );
            CompleteIrp = TRUE;
            break;
        }
#ifndef UDF_READ_ONLY_BUILD
        case IOCTL_UDF_SET_FILE_ALLOCATION_MODE: {
            KdPrint(("UDF: Set File Alloc mode\n"));
            RC = UDFSetFileAllocModeFromICB( PtrIrpContext, Irp );
            CompleteIrp = TRUE;
            break;
        }
#endif //UDF_READ_ONLY_BUILD
        case IOCTL_UDF_LOCK_VOLUME_BY_PID:
            if(AcquiredVcb) {
                UDFReleaseResource(&(Vcb->VCBResource));
                AcquiredVcb = FALSE;
            }
            RC = UDFLockVolume( PtrIrpContext, Irp, GetCurrentPID() );
            CompleteIrp = TRUE;
            break;
        case IOCTL_UDF_UNLOCK_VOLUME_BY_PID:
            if(AcquiredVcb) {
                UDFReleaseResource(&(Vcb->VCBResource));
                AcquiredVcb = FALSE;
            }
            RC = UDFUnlockVolume( PtrIrpContext, Irp, GetCurrentPID() );
            CompleteIrp = TRUE;
            break;
#ifndef UDF_READ_ONLY_BUILD
        case IOCTL_UDF_SEND_LICENSE_KEY:
            RC = STATUS_SUCCESS;

            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            CompleteIrp = TRUE;
            break;
#endif //UDF_READ_ONLY_BUILD
        case IOCTL_UDF_GET_VERSION: {

            PUDF_GET_VERSION_OUT udf_ver;

            KdPrint(("UDFUserFsCtrlRequest: IOCTL_UDF_GET_VERSION\n"));

            Irp->IoStatus.Information = 0;
            CompleteIrp = TRUE;

            if(!IrpSp->Parameters.DeviceIoControl.OutputBufferLength) {
                KdPrint(("!OutputBufferLength\n"));
                try_return(RC = STATUS_SUCCESS);
            }
            //  Check the size of the output buffer.
            if(IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(UDF_GET_VERSION_OUT)) {
                KdPrint(("OutputBufferLength < %x\n", sizeof(UDF_GET_VERSION_OUT)));
                try_return(RC = STATUS_BUFFER_TOO_SMALL);
            }

            udf_ver = (PUDF_GET_VERSION_OUT)(Irp->AssociatedIrp.SystemBuffer);
            if(!udf_ver) {
                KdPrint(("!udf_ver\n"));
                try_return(RC = STATUS_INVALID_USER_BUFFER);
            }

            RtlZeroMemory(udf_ver, IrpSp->Parameters.DeviceIoControl.OutputBufferLength);

            udf_ver->header.Length = sizeof(UDF_GET_VERSION_OUT);
            udf_ver->header.DriverVersionMj    = 0x00010005;
            udf_ver->header.DriverVersionMn    = 0x12;
            udf_ver->header.DriverVersionBuild = UDF_CURRENT_BUILD;

            udf_ver->FSVersionMj = Vcb->CurrentUDFRev >> 8;
            udf_ver->FSVersionMn = Vcb->CurrentUDFRev & 0xff;
            udf_ver->FSFlags     = Vcb->UserFSFlags;
            if( ((Vcb->origIntegrityType == INTEGRITY_TYPE_OPEN) &&
                (Vcb->CompatFlags & UDF_VCB_IC_DIRTY_RO)) 
                    ||
               (Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY) ) {
                KdPrint(("  UDF_USER_FS_FLAGS_RO\n"));
                udf_ver->FSFlags |= UDF_USER_FS_FLAGS_RO;
            }
            if(Vcb->VCBFlags & UDF_VCB_FLAGS_OUR_DEVICE_DRIVER) {
                KdPrint(("  UDF_USER_FS_FLAGS_OUR_DRIVER\n"));
                udf_ver->FSFlags |= UDF_USER_FS_FLAGS_OUR_DRIVER;
            }
            if(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK) {
                KdPrint(("  UDF_USER_FS_FLAGS_RAW\n"));
                udf_ver->FSFlags |= UDF_USER_FS_FLAGS_RAW;
            }
            if(Vcb->VCBFlags & UDF_VCB_FLAGS_MEDIA_READ_ONLY) {
                KdPrint(("  UDF_USER_FS_FLAGS_MEDIA_RO\n"));
                udf_ver->FSFlags |= UDF_USER_FS_FLAGS_MEDIA_RO;
            }
            if(Vcb->FP_disc) {
                KdPrint(("  UDF_USER_FS_FLAGS_FP\n"));
                udf_ver->FSFlags |= UDF_USER_FS_FLAGS_FP;
            }
            udf_ver->FSCompatFlags = Vcb->CompatFlags;

            udf_ver->FSCfgVersion = Vcb->CfgVersion;

            Irp->IoStatus.Information = sizeof(UDF_GET_VERSION_OUT);
            RC = STATUS_SUCCESS;
            CompleteIrp = TRUE;

            break; }
        case IOCTL_UDF_SET_OPTIONS: {

            PUDF_SET_OPTIONS_IN udf_opt;
            BOOLEAN PrevVerifyOnWrite;

            KdPrint(("UDF: IOCTL_UDF_SET_OPTIONS\n"));

            Irp->IoStatus.Information = 0;
            CompleteIrp = TRUE;

            if(IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(UDF_SET_OPTIONS_IN)) {
                KdPrint(("InputBufferLength < %x\n", sizeof(UDF_SET_OPTIONS_IN)));
                try_return(RC = STATUS_BUFFER_TOO_SMALL);
            }

            udf_opt = (PUDF_SET_OPTIONS_IN)(Irp->AssociatedIrp.SystemBuffer);
            if(!udf_opt) {
                KdPrint(("!udf_opt\n"));
                try_return(RC = STATUS_INVALID_USER_BUFFER);
            }

            if((udf_opt->header.Flags & UDF_SET_OPTIONS_FLAG_MASK) != UDF_SET_OPTIONS_FLAG_TEMPORARY) {
                KdPrint(("invalid opt target\n"));
                try_return(RC = STATUS_INVALID_PARAMETER);
            }

            if(AcquiredVcb) {
                UDFReleaseResource(&(Vcb->VCBResource));
                AcquiredVcb = FALSE;
            }
            UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
            AcquiredVcb = TRUE;

            PrevVerifyOnWrite = Vcb->VerifyOnWrite;

            Vcb->Cfg = ((PUCHAR)(udf_opt)) + udf_opt->header.HdrLength;
            Vcb->CfgLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength - offsetof(UDF_SET_OPTIONS_IN, Data);
            UDFReadRegKeys(Vcb, TRUE /*update*/, TRUE /*cfg*/);
            Vcb->Cfg = NULL;
            Vcb->CfgLength = 0;
            Vcb->CfgVersion++;
            //UDFReadRegKeys(Vcb, TRUE /*update*/, TRUE);
            if(PrevVerifyOnWrite != Vcb->VerifyOnWrite) {
                if(Vcb->VerifyOnWrite) {
                    UDFVInit(Vcb);
                } else {
                    WCacheFlushBlocks__(&(Vcb->FastCache), Vcb, 0, Vcb->LastLBA);
                    UDFVFlush(Vcb);
                    UDFVRelease(Vcb);
                }
            }

            RC = STATUS_SUCCESS;
            break; }
#if 0
        case IOCTL_UDF_GET_OPTIONS_VERSION: {

            PUDF_GET_OPTIONS_VERSION_OUT udf_opt_ver;

            KdPrint(("UDF: IOCTL_UDF_GET_OPTIONS_VERSION\n"));

            Irp->IoStatus.Information = 0;
            CompleteIrp = TRUE;

            if(IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(UDF_GET_OPTIONS_VERSION_OUT)) {
                KdPrint(("OutputBufferLength < %x\n", sizeof(UDF_GET_OPTIONS_VERSION_OUT)));
                try_return(RC = STATUS_BUFFER_TOO_SMALL);
            }

            udf_opt_ver = (PUDF_GET_OPTIONS_VERSION_OUT)(Irp->AssociatedIrp.SystemBuffer);
            if(!udf_opt_ver) {
                KdPrint(("!udf_opt-ver\n"));
                try_return(RC = STATUS_INVALID_USER_BUFFER);
            }
/*
            if(AcquiredVcb) {
                UDFReleaseResource(&(Vcb->VCBResource));
                AcquiredVcb = FALSE;
            }
            UDFAcquireResourceShared(&(Vcb->VCBResource), TRUE);
            AcquiredVcb = TRUE;
*/
            udf_opt_ver->CfgVersion = Vcb->CfgVersion;
            Irp->IoStatus.Information = sizeof(UDF_GET_OPTIONS_VERSION_OUT);

            RC = STATUS_SUCCESS;
            break; }
#endif //0
        case IOCTL_CDRW_RESET_DRIVER:

            KdPrint(("UDF: IOCTL_CDRW_RESET_DRIVER\n"));
            Vcb->MediaLockCount = 0;
            Vcb->VCBFlags &= ~UDF_VCB_FLAGS_MEDIA_LOCKED;
            goto ioctl_do_default;

        case FSCTL_ALLOW_EXTENDED_DASD_IO:

            KdPrint(("UDFUserFsCtrlRequest: FSCTL_ALLOW_EXTENDED_DASD_IO\n"));
            // DASD i/o is always permitted
            // So, no-op this call
            RC = STATUS_SUCCESS;

            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            CompleteIrp = TRUE;
            break;

        case FSCTL_IS_VOLUME_DIRTY:

            KdPrint(("UDFUserFsCtrlRequest: FSCTL_IS_VOLUME_DIRTY\n"));
            // DASD i/o is always permitted
            // So, no-op this call
            RC = UDFIsVolumeDirty(PtrIrpContext, Irp);
            CompleteIrp = TRUE;
            break;

        case IOCTL_STORAGE_EJECT_MEDIA:
        case IOCTL_DISK_EJECT_MEDIA:
        case IOCTL_CDROM_EJECT_MEDIA: {

            KdPrint(("UDF Reset/Eject request\n"));
//            PPREVENT_MEDIA_REMOVAL_USER_IN Buf;

            if(Vcb->EjectWaiter) {
                KdPrint(("  Vcb->EjectWaiter present\n"));
                Irp->IoStatus.Information = 0;
                Vcb->EjectWaiter->SoftEjectReq = TRUE;
                Vcb->SoftEjectReq = TRUE;
                CompleteIrp = TRUE;
                try_return(RC = STATUS_SUCCESS);
            }
            KdPrint(("  !Vcb->EjectWaiter\n"));
            goto ioctl_do_default;
/*
            Buf = (PPREVENT_MEDIA_REMOVAL_USER_IN)MyAllocatePool__(NonPagedPool, sizeof(PREVENT_MEDIA_REMOVAL_USER_IN));
            if(!Buf) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
            // Acquire Vcb resource (Shared -> Exclusive)
            UDFReleaseResource(&(Vcb->VCBResource));
            UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);

            Vcb->Vpb->RealDevice->Flags |= DO_VERIFY_VOLUME;
            Vcb->VCBFlags &= ~UDF_VCB_FLAGS_VOLUME_MOUNTED;

            UDFDoDismountSequence(Vcb, Buf, IoControlCode == IOCTL_CDROM_EJECT_MEDIA);
            // disable Eject Request Waiter if any
            MyFreePool__(Buf);
            // Release the Vcb resource.
            UDFReleaseResource(&(Vcb->VCBResource));
            AcquiredVcb = FALSE;
            UDFStopEjectWaiter(Vcb);
            CompleteIrp = TRUE;
            RC = STATUS_SUCCESS;
            break;*/
        }
        case IOCTL_CDROM_DISK_TYPE: {

            KdPrint(("UDF Cdrom Disk Type\n"));
            CompleteIrp = TRUE;
            //  Verify the Vcb in this case to detect if the volume has changed.
            Irp->IoStatus.Information = 0;
            RC = UDFVerifyVcb(PtrIrpContext,Vcb);
            if(!NT_SUCCESS(RC))
                try_return(RC);

            //  Check the size of the output buffer.
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(CDROM_DISK_DATA_USER_OUT))
                try_return(RC = STATUS_BUFFER_TOO_SMALL);

            //  Copy the data from the Vcb.
            ((PCDROM_DISK_DATA_USER_OUT)(Irp->AssociatedIrp.SystemBuffer))->DiskData = CDROM_DISK_DATA_TRACK;
            for(TrackNumber=Vcb->FirstTrackNum; TrackNumber<Vcb->LastTrackNum; TrackNumber++) {
                if((Vcb->TrackMap[TrackNumber].TrackParam & Trk_QSubChan_Type_Mask) ==
                    Trk_QSubChan_Type_Audio) {
                    ((PCDROM_DISK_DATA_USER_OUT)(Irp->AssociatedIrp.SystemBuffer))->DiskData |= CDROM_DISK_AUDIO_TRACK;
                    break;
                }
            }

            Irp->IoStatus.Information = sizeof(CDROM_DISK_DATA_USER_OUT);
            RC = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CDRW_LOCK_DOOR:
        case IOCTL_STORAGE_MEDIA_REMOVAL:
        case IOCTL_DISK_MEDIA_REMOVAL:
        case IOCTL_CDROM_MEDIA_REMOVAL: {
            KdPrint(("UDF Lock/Unlock\n"));
            PPREVENT_MEDIA_REMOVAL_USER_IN buffer; // user supplied buffer
            buffer = (PPREVENT_MEDIA_REMOVAL_USER_IN)(Irp->AssociatedIrp.SystemBuffer);
            if(!buffer) {
                if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED)) {
                    KdPrint(("!mounted\n"));
                    goto ioctl_do_default;
                }
                KdPrint(("abort\n"));
                CompleteIrp = TRUE;
                Irp->IoStatus.Information = 0;
                UnsafeIoctl = FALSE;
                RC = STATUS_INVALID_PARAMETER;
                break;
            }
            if(!buffer->PreventMediaRemoval &&
               !Vcb->MediaLockCount) {

                KdPrint(("!locked + unlock req\n"));
                if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED)) {
                    KdPrint(("!mounted\n"));
                    goto ioctl_do_default;
                }
#if 0
                // allocate tmp buffer for FSD calls
                Buf = (PPREVENT_MEDIA_REMOVAL_USER_IN)MyAllocatePool__(NonPagedPool, sizeof(PREVENT_MEDIA_REMOVAL_USER_IN));
                if(!Buf)
                    try_return(RC = STATUS_INSUFFICIENT_RESOURCES);

                // Acquire Vcb resource (Shared -> Exclusive)
                UDFInterlockedIncrement((PLONG)&(Vcb->VCBOpenCount));
                UDFReleaseResource(&(Vcb->VCBResource));

#ifdef UDF_DELAYED_CLOSE
                //  Acquire exclusive access to the Vcb.
                UDFCloseAllDelayed(Vcb);
#endif //UDF_DELAYED_CLOSE

                UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
                UDFInterlockedDecrement((PLONG)&(Vcb->VCBOpenCount));

                UDFDoDismountSequence(Vcb, Buf, FALSE);
                MyFreePool__(Buf);
                Buf = NULL;
                Vcb->MediaLockCount = 0;
                // Release the Vcb resource.
                UDFReleaseResource(&(Vcb->VCBResource));
                AcquiredVcb = FALSE;
                // disable Eject Request Waiter if any
                UDFStopEjectWaiter(Vcb);
#else
                // just ignore
#endif
ignore_lock:
                KdPrint(("ignore lock/unlock\n"));
                CompleteIrp = TRUE;
                Irp->IoStatus.Information = 0;
                RC = STATUS_SUCCESS;
                break;
            }
            if(buffer->PreventMediaRemoval) {
                KdPrint(("lock req\n"));
                Vcb->MediaLockCount++;
                Vcb->VCBFlags |= UDF_VCB_FLAGS_MEDIA_LOCKED;
                UnsafeIoctl = FALSE;
            } else {
                KdPrint(("unlock req\n"));
                if(Vcb->MediaLockCount) {
                    KdPrint(("lock count %d\n", Vcb->MediaLockCount));
                    UnsafeIoctl = FALSE;
                    Vcb->MediaLockCount--;
                }
            }
            if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED)) {
                KdPrint(("!mounted\n"));
                goto ioctl_do_default;
            }
            goto ignore_lock;
        }
        default:

            KdPrint(("default processing Irp %x, ctx %x, DevIoCtl %x\n", Irp, PtrIrpContext, IoControlCode));
ioctl_do_default:

            // make sure volume is Sync'ed BEFORE sending unsafe IOCTL
            if(Vcb && UnsafeIoctl) {
                UDFFlushLogicalVolume(NULL, NULL, Vcb, 0);
                KdPrint(("  sync'ed\n"));
            }
            // Invoke the lower level driver in the chain.
            //PtrNextIoStackLocation = IoGetNextIrpStackLocation(Irp);
            //*PtrNextIoStackLocation = *IrpSp;
            IoSkipCurrentIrpStackLocation(Irp);
/*
            // Set a completion routine.
            IoSetCompletionRoutine(Irp, UDFDevIoctlCompletion, PtrIrpContext, TRUE, TRUE, TRUE);
            // Send the request.
*/
            RC = IoCallDriver(Vcb->TargetDeviceObject, Irp);
            if(!CompleteIrp) {
                // since now we do not use IoSetCompletionRoutine()
                UDFReleaseIrpContext(PtrIrpContext);
            }
            break;
        }

        if(Vcb && UnsafeIoctl) {
            KdPrint(("  set UnsafeIoctl\n"));
            Vcb->VCBFlags |= UDF_VCB_FLAGS_UNSAFE_IOCTL;
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {

        if(AcquiredVcb) {
            UDFReleaseResource(&(Vcb->VCBResource));
            AcquiredVcb = FALSE;
        }

        if(Buf) {
            MyFreePool__(Buf);
        }

        if (!_SEH2_AbnormalTermination() &&
            CompleteIrp) {
            KdPrint(("  complete Irp %x, ctx %x, status %x, iolen %x\n",
                Irp, PtrIrpContext, RC, Irp->IoStatus.Information));
            Irp->IoStatus.Status = RC;
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
            // Release the IRP context
            UDFReleaseIrpContext(PtrIrpContext);
        }
    } _SEH2_END;

    return(RC);
} // end UDFCommonDeviceControl()


/*************************************************************************
*
* Function: UDFDevIoctlCompletion()
*
* Description:
*   Completion routine.
*   
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS
*
*************************************************************************/
NTSTATUS
NTAPI
UDFDevIoctlCompletion(
   PDEVICE_OBJECT          PtrDeviceObject,
   PIRP                    Irp,
   VOID                    *Context)
{
/*    PIO_STACK_LOCATION      IrpSp = NULL;
    ULONG                   IoControlCode = 0;*/
    PtrUDFIrpContext       PtrIrpContext = (PtrUDFIrpContext)Context;

    KdPrint(("UDFDevIoctlCompletion Irp %x, ctx %x\n", Irp, Context));
    if (Irp->PendingReturned) {
        KdPrint(("  IoMarkIrpPending\n"));
        IoMarkIrpPending(Irp);
    }

    UDFReleaseIrpContext(PtrIrpContext);
/*    if(Irp->IoStatus.Status == STATUS_SUCCESS) {
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

        switch(IoControlCode) {
        case IOCTL_CDRW_RESET_DRIVER: {
            Vcb->MediaLockCount = 0;
        }
        }
    }*/

    return STATUS_SUCCESS;
} // end UDFDevIoctlCompletion()


/*************************************************************************
*
* Function: UDFHandleQueryPath()
*
* Description:
*   Handle the MUP request.
*   
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS
*
*************************************************************************/
/*NTSTATUS UDFHandleQueryPath(
VOID           *BufferPointer)
{
    NTSTATUS                    RC = STATUS_SUCCESS;
    PQUERY_PATH_REQUEST RequestBuffer = (PQUERY_PATH_REQUEST)BufferPointer;
    PQUERY_PATH_RESPONSE    ReplyBuffer = (PQUERY_PATH_RESPONSE)BufferPointer;
    ULONG                       LengthOfNameToBeMatched = RequestBuffer->PathNameLength;
    ULONG                       LengthOfMatchedName = 0;
    WCHAR                *NameToBeMatched = RequestBuffer->FilePathName;

    KdPrint(("UDFHandleQueryPath\n"));
    // So here we are. Simply check the name supplied.
    // We can use whatever algorithm we like to determine whether the
    // sent in name is acceptable.
    // The first character in the name is always a "\"
    // If we like the name sent in (probably, we will like a subset
    // of the name), set the matching length value in LengthOfMatchedName.

    // if (FoundMatch) {
    //      ReplyBuffer->LengthAccepted = LengthOfMatchedName;
    // } else {
    //      RC = STATUS_OBJECT_NAME_NOT_FOUND;
    // }

    return(RC);
}*/

NTSTATUS
UDFGetFileAllocModeFromICB(
    PtrUDFIrpContext IrpContext,
    PIRP             Irp
    )
{
    PEXTENDED_IO_STACK_LOCATION IrpSp =
        (PEXTENDED_IO_STACK_LOCATION)IoGetCurrentIrpStackLocation( Irp );

//    PVCB Vcb;
    PtrUDFFCB Fcb;
    PtrUDFCCB Ccb;
    PUDF_GET_FILE_ALLOCATION_MODE_OUT OutputBuffer;

    KdPrint(("UDFGetFileAllocModeFromICB\n"));

    // Decode the file object, the only type of opens we accept are
    // user volume opens.
    Ccb = (PtrUDFCCB)(IrpSp->FileObject->FsContext2);
    Fcb = Ccb->Fcb;
//    Vcb = Fcb->Vcb;

    Irp->IoStatus.Information = 0;
    if(IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(UDF_GET_FILE_ALLOCATION_MODE_OUT))
        return STATUS_BUFFER_TOO_SMALL;

    OutputBuffer = (PUDF_GET_FILE_ALLOCATION_MODE_OUT)(Irp->AssociatedIrp.SystemBuffer);
    if(!OutputBuffer)
        return STATUS_INVALID_USER_BUFFER;

    OutputBuffer->AllocMode = UDFGetFileICBAllocMode__(Fcb->FileInfo);
    Irp->IoStatus.Information = sizeof(UDF_GET_FILE_ALLOCATION_MODE_OUT);

    return STATUS_SUCCESS;
} // end UDFGetFileAllocModeFromICB()

#ifndef UDF_READ_ONLY_BUILD
NTSTATUS
UDFSetFileAllocModeFromICB(
    PtrUDFIrpContext IrpContext,
    PIRP             Irp
    )
{
    PEXTENDED_IO_STACK_LOCATION IrpSp =
        (PEXTENDED_IO_STACK_LOCATION)IoGetCurrentIrpStackLocation( Irp );

    PVCB Vcb;
    PtrUDFFCB Fcb;
    PtrUDFCCB Ccb;
    PUDF_SET_FILE_ALLOCATION_MODE_IN InputBuffer;
    NTSTATUS RC;
    UCHAR AllocMode;

    KdPrint(("UDFSetFileAllocModeFromICB\n"));

    Ccb = (PtrUDFCCB)(IrpSp->FileObject->FsContext2);
    Fcb = Ccb->Fcb;
    Vcb = Fcb->Vcb;

    Irp->IoStatus.Information = 0;
    if(IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof(UDF_SET_FILE_ALLOCATION_MODE_IN))
        return STATUS_BUFFER_TOO_SMALL;

    InputBuffer = (PUDF_SET_FILE_ALLOCATION_MODE_IN)(Irp->AssociatedIrp.SystemBuffer);
    if(!InputBuffer)
        return STATUS_INVALID_USER_BUFFER;

    UDFFlushAFile(Fcb, Ccb, &(Irp->IoStatus), 0);
    RC = Irp->IoStatus.Status;
    if(!NT_SUCCESS(RC))
        return RC;

    if(InputBuffer->AllocMode != ICB_FLAG_AD_IN_ICB) {
        AllocMode = UDFGetFileICBAllocMode__(Fcb->FileInfo);
        if(AllocMode == ICB_FLAG_AD_IN_ICB) {
            RC = UDFConvertFEToNonInICB(Vcb, Fcb->FileInfo, InputBuffer->AllocMode);
        } else
        if(AllocMode != InputBuffer->AllocMode) {
            RC = STATUS_INVALID_PARAMETER;
        } else {
            RC = STATUS_SUCCESS;
        }
    } else {
        RC = STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
} // end UDFSetFileAllocModeFromICB()
#endif //UDF_READ_ONLY_BUILD

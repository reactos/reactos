/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    diskwmi.c

Abstract:

    SCSI disk class driver - WMI support routines

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "disk.h"

#include <wmistr.h>

NTSTATUS
NTAPI
DiskSendFailurePredictIoctl(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    PSTORAGE_PREDICT_FAILURE checkFailure
    );

NTSTATUS
NTAPI
DiskGetIdentifyInfo(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    PBOOLEAN SupportSmart
    );

NTSTATUS
NTAPI
DiskDetectFailurePrediction(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    PFAILURE_PREDICTION_METHOD FailurePredictCapability
    );

NTSTATUS
NTAPI
DiskReadFailurePredictThresholds(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    PSTORAGE_FAILURE_PREDICT_THRESHOLDS DiskSmartThresholds
    );

NTSTATUS
NTAPI
DiskReadSmartLog(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN UCHAR SectorCount,
    IN UCHAR LogAddress,
    OUT PUCHAR Buffer
    );

NTSTATUS
NTAPI
DiskWriteSmartLog(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN UCHAR SectorCount,
    IN UCHAR LogAddress,
    IN PUCHAR Buffer
    );

void NTAPI DiskReregWorker(
    IN PVOID Context
    );

//
// WMI reregistration globals
//
// Since it will take too long to do a mode sense on some drive, we
// need a good way to effect the mode sense for the info exceptions
// mode page so that we can determine if SMART is supported and enabled
// for the drive. So the strategy is to do an asynchronous mode sense
// when the device starts and then look at the info exceptions mode
// page within the completion routine. Now within the completion
// routine we cannot call IoWMIRegistrationControl since we are at DPC
// level, so we create a stack of device objects that will be processed
// by a single work item that is fired off only when the stack
// transitions from empty to non empty.
//
WORK_QUEUE_ITEM DiskReregWorkItem;
SINGLE_LIST_ENTRY DiskReregHead;
KSPIN_LOCK DiskReregSpinlock;
LONG DiskReregWorkItems;

GUIDREGINFO DiskWmiFdoGuidList[] =
{
    {
        WMI_DISK_GEOMETRY_GUID,
        1,
        0
    },

    {
        WMI_STORAGE_FAILURE_PREDICT_STATUS_GUID,
        1,
        WMIREG_FLAG_EXPENSIVE
    },
    
    {
        WMI_STORAGE_FAILURE_PREDICT_DATA_GUID,
        1,
        WMIREG_FLAG_EXPENSIVE
    },

    {
        WMI_STORAGE_FAILURE_PREDICT_FUNCTION_GUID,
        1,
        WMIREG_FLAG_EXPENSIVE
    },

    {
        WMI_STORAGE_PREDICT_FAILURE_EVENT_GUID,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },

    {
        WMI_STORAGE_FAILURE_PREDICT_THRESHOLDS_GUID,
        1,
        WMIREG_FLAG_EXPENSIVE
    },

    {
        WMI_STORAGE_SCSI_INFO_EXCEPTIONS_GUID,
        1,
        0
    },

    
};


GUID DiskPredictFailureEventGuid = WMI_STORAGE_PREDICT_FAILURE_EVENT_GUID;

#define DiskGeometryGuid           0
#define SmartStatusGuid            1
#define SmartDataGuid              2
#define SmartPerformFunction       3
    #define AllowDisallowPerformanceHit                 1
    #define EnableDisableHardwareFailurePrediction      2
    #define EnableDisableFailurePredictionPolling       3
    #define GetFailurePredictionCapability              4
    #define EnableOfflineDiags                          5

#define SmartEventGuid             4
#define SmartThresholdsGuid        5
#define ScsiInfoExceptionsGuid     6

#if 0
    //
    // Enable this to add WMI support for PDOs
GUIDREGINFO DiskWmiPdoGuidList[] =
{
    {
        // {25007F51-57C2-11d1-A528-00A0C9062910}
        { 0x25007f52, 0x57c2, 0x11d1,
                       { 0xa5, 0x28, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0x10 } },
        0
    },

};

ULONG DiskDummyData[4] = { 1, 2, 3, 4};
#endif

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, DiskWmiFunctionControl)
#pragma alloc_text(PAGE, DiskFdoQueryWmiRegInfo)
#pragma alloc_text(PAGE, DiskFdoQueryWmiDataBlock)
#pragma alloc_text(PAGE, DiskFdoSetWmiDataBlock)
#pragma alloc_text(PAGE, DiskFdoSetWmiDataItem)
#pragma alloc_text(PAGE, DiskFdoExecuteWmiMethod)

#pragma alloc_text(PAGE, DiskDetectFailurePrediction)
#pragma alloc_text(PAGE, DiskEnableDisableFailurePrediction)
#pragma alloc_text(PAGE, DiskEnableDisableFailurePredictPolling)
#pragma alloc_text(PAGE, DiskReadFailurePredictStatus)
#pragma alloc_text(PAGE, DiskReadFailurePredictData)
#pragma alloc_text(PAGE, DiskReadFailurePredictThresholds)
#pragma alloc_text(PAGE, DiskGetIdentifyInfo)
#pragma alloc_text(PAGE, DiskReadSmartLog)
#pragma alloc_text(PAGE, DiskWriteSmartLog)

#pragma alloc_text(PAGE, DiskPerformSmartCommand)

#pragma alloc_text(PAGE, DiskSendFailurePredictIoctl)

#pragma alloc_text(PAGE, DiskReregWorker)
#pragma alloc_text(PAGE, DiskInitializeReregistration)

#endif


//
// SMART/IDE specific routines

//
// Read SMART data attributes.
// SrbControl should be sizeof(SRB_IO_CONTROL) +
//                      (sizeof(SENDCMDINPARAMS)-1) +
//                      READ_ATTRIBUTE_BUFFER_SIZE
// Attribute data returned at &SendCmdOutParams->bBuffer[0]
//
#define DiskReadSmartData(FdoExtension, \
                          SrbControl, \
                          BufferSize) \
    DiskPerformSmartCommand(FdoExtension, \
                            IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS,  \
                            SMART_CMD, \
                            READ_ATTRIBUTES, \
                            0, \
                            0, \
                            (SrbControl), \
                            (BufferSize))


//
// Read SMART data thresholds.
// SrbControl should be sizeof(SRB_IO_CONTROL) +
//                      (sizeof(SENDCMDINPARAMS)-1) +
//                      READ_THRESHOLD_BUFFER_SIZE
// Attribute data returned at &SendCmdOutParams->bBuffer[0]
//
#define DiskReadSmartThresholds(FdoExtension, \
                          SrbControl, \
                          BufferSize) \
    DiskPerformSmartCommand(FdoExtension, \
                            IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS, \
                            SMART_CMD, \
                            READ_THRESHOLDS, \
                            0, \
                            0, \
                            (SrbControl), \
                            (BufferSize))


//
// Read SMART status
// SrbControl should be sizeof(SRB_IO_CONTROL) +
//                      (sizeof(SENDCMDINPARAMS)-1) +
//                      sizeof(IDEREGS)
// Failure predicted if cmdOutParameters[3] == 0xf4 and [4] == 0x2c
//
#define DiskReadSmartStatus(FdoExtension, \
                          SrbControl, \
                          BufferSize) \
    DiskPerformSmartCommand(FdoExtension, \
                            IOCTL_SCSI_MINIPORT_RETURN_STATUS, \
                            SMART_CMD, \
                            RETURN_SMART_STATUS, \
                            0, \
                            0, \
                            (SrbControl), \
                            (BufferSize))


//
// Read disks IDENTIFY data
// SrbControl should be sizeof(SRB_IO_CONTROL) +
//                      (sizeof(SENDCMDINPARAMS)-1) +
//                      sizeof(IDENTIFY_BUFFER_SIZE)
// Identify data returned at &cmdOutParams.bBuffer[0]
//
#define DiskGetIdentifyData(FdoExtension, \
                          SrbControl, \
                          BufferSize) \
    DiskPerformSmartCommand(FdoExtension, \
                            IOCTL_SCSI_MINIPORT_IDENTIFY, \
                            ID_CMD, \
                            0, \
                            0, \
                            0, \
                            (SrbControl), \
                            (BufferSize))


//
// Enable SMART
//
#ifdef __REACTOS__
static
#endif
__inline NTSTATUS
DiskEnableSmart(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    UCHAR srbControl[sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS)];
    ULONG bufferSize = sizeof(srbControl);

    return DiskPerformSmartCommand(FdoExtension,
                                   IOCTL_SCSI_MINIPORT_ENABLE_SMART,
                                   SMART_CMD,
                                   ENABLE_SMART,
                                   0,
                                   0,
                                   (PSRB_IO_CONTROL)srbControl,
                                   &bufferSize);
}

//
// Disable SMART
//
#ifdef __REACTOS__
static
#endif
__inline NTSTATUS
DiskDisableSmart(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    UCHAR srbControl[sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS)];
    ULONG bufferSize = sizeof(srbControl);
    return DiskPerformSmartCommand(FdoExtension,
                                   IOCTL_SCSI_MINIPORT_DISABLE_SMART,
                                   SMART_CMD,
                                   DISABLE_SMART,
                                   0,
                                   0,
                                   (PSRB_IO_CONTROL)srbControl,
                                   &bufferSize);
}

//
// Enable Attribute Autosave
//
__inline NTSTATUS
DiskEnableSmartAttributeAutosave(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    UCHAR srbControl[sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS)];
    ULONG bufferSize = sizeof(srbControl);
    return DiskPerformSmartCommand(FdoExtension,
                                   IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE,
                                   SMART_CMD,
                                   ENABLE_DISABLE_AUTOSAVE,
                                   0xf1,
                                   0,
                                   (PSRB_IO_CONTROL)srbControl,
                                   &bufferSize);
}

//
// Disable Attribute Autosave
//
__inline NTSTATUS
DiskDisableSmartAttributeAutosave(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    UCHAR srbControl[sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS)];
    ULONG bufferSize = sizeof(srbControl);
    return DiskPerformSmartCommand(FdoExtension,
                                   IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE,
                                   SMART_CMD,
                                   ENABLE_DISABLE_AUTOSAVE,
                                   0x00,
                                   0,
                                   (PSRB_IO_CONTROL)srbControl,
                                   &bufferSize);
}

//
// Initialize execution of SMART online diagnostics
//
#ifdef __REACTOS__
static
#endif
__inline NTSTATUS
DiskExecuteSmartDiagnostics(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    UCHAR Subcommand
    )
{
    UCHAR srbControl[sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS)];
    ULONG bufferSize = sizeof(srbControl);
    return DiskPerformSmartCommand(FdoExtension,
                                   IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS,
                                   SMART_CMD,
                                   EXECUTE_OFFLINE_DIAGS,
                                   0,
                                   Subcommand,
                                   (PSRB_IO_CONTROL)srbControl,
                                   &bufferSize);
}


NTSTATUS
NTAPI
DiskReadSmartLog(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN UCHAR SectorCount,
    IN UCHAR LogAddress,
    OUT PUCHAR Buffer
    )
{
    PSRB_IO_CONTROL srbControl;
    NTSTATUS status;
    PSENDCMDOUTPARAMS sendCmdOutParams;
    ULONG logSize, bufferSize;

    PAGED_CODE();

    logSize = SectorCount * SMART_LOG_SECTOR_SIZE;
    bufferSize = sizeof(SRB_IO_CONTROL) +  sizeof(SENDCMDINPARAMS) - 1 +
                 logSize;

    srbControl = ExAllocatePoolWithTag(NonPagedPool,
                                       bufferSize,
                                       DISK_TAG_SMART);
    
    if (srbControl != NULL)
    {
        status = DiskPerformSmartCommand(FdoExtension,
                                         IOCTL_SCSI_MINIPORT_READ_SMART_LOG,
                                         SMART_CMD,
                                         SMART_READ_LOG,
                                         SectorCount,
                                         LogAddress,
                                         srbControl,
                                         &bufferSize);

        if (NT_SUCCESS(status))
        {
            sendCmdOutParams = (PSENDCMDOUTPARAMS)((PUCHAR)srbControl +
                                                   sizeof(SRB_IO_CONTROL));
            RtlCopyMemory(Buffer,
                          &sendCmdOutParams->bBuffer[0],
                          logSize);
        }
        
        ExFreePool(srbControl);
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return(status);
}


NTSTATUS
NTAPI
DiskWriteSmartLog(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN UCHAR SectorCount,
    IN UCHAR LogAddress,
    IN PUCHAR Buffer
    )
{
    PSRB_IO_CONTROL srbControl;
    NTSTATUS status;
    PSENDCMDINPARAMS sendCmdInParams;
    ULONG logSize, bufferSize;

    PAGED_CODE();

    logSize = SectorCount * SMART_LOG_SECTOR_SIZE;
    bufferSize = sizeof(SRB_IO_CONTROL) +  sizeof(SENDCMDINPARAMS) - 1 +
                 logSize;

    srbControl = ExAllocatePoolWithTag(NonPagedPool,
                                       bufferSize,
                                       DISK_TAG_SMART);
    
    if (srbControl != NULL)
    {
        sendCmdInParams = (PSENDCMDINPARAMS)((PUCHAR)srbControl +
                                               sizeof(SRB_IO_CONTROL));
        RtlCopyMemory(&sendCmdInParams->bBuffer[0],
                      Buffer,
                      logSize);
        status = DiskPerformSmartCommand(FdoExtension,
                                         IOCTL_SCSI_MINIPORT_WRITE_SMART_LOG,
                                         SMART_CMD,
                                         SMART_WRITE_LOG,
                                         SectorCount,
                                         LogAddress,
                                         srbControl,
                                         &bufferSize);

        ExFreePool(srbControl);
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return(status);
}

NTSTATUS
NTAPI
DiskPerformSmartCommand(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN ULONG SrbControlCode,
    IN UCHAR Command,
    IN UCHAR Feature,
    IN UCHAR SectorCount,
    IN UCHAR SectorNumber,
    IN OUT PSRB_IO_CONTROL SrbControl,
    OUT PULONG BufferSize
    )
/*++

Routine Description:

    This routine will perform some SMART command

Arguments:

    FdoExtension is the FDO device extension

    SrbControlCode is the SRB control code to use for the request

    Command is the SMART command to be executed. It may be SMART_CMD or
        ID_CMD.

    Feature is the value to place in the IDE feature register.

    SectorCount is the value to place in the IDE SectorCount register

    SrbControl is the buffer used to build the SRB_IO_CONTROL and pass
        any input parameters. It also returns the output parameters.

    *BufferSize on entry has total size of SrbControl and on return has
        the size used in SrbControl.



Return Value:

    status

--*/
{
    PCOMMON_DEVICE_EXTENSION commonExtension = (PCOMMON_DEVICE_EXTENSION)FdoExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    PUCHAR buffer;
    PSENDCMDINPARAMS cmdInParameters;
    //PSENDCMDOUTPARAMS cmdOutParameters;
    //ULONG outBufferSize;
    NTSTATUS status;
    ULONG availableBufferSize;
    KEVENT event;
    PIRP irp;
    IO_STATUS_BLOCK ioStatus;
    SCSI_REQUEST_BLOCK      srb;
    LARGE_INTEGER           startingOffset;
    ULONG length;
    PIO_STACK_LOCATION      irpStack;

    PAGED_CODE();

    //
    // Point to the 'buffer' portion of the SRB_CONTROL and compute how
    // much room we have left in the srb control
    //

    buffer = (PUCHAR)SrbControl;
    buffer +=  sizeof(SRB_IO_CONTROL);

    cmdInParameters = (PSENDCMDINPARAMS)buffer;
    //cmdOutParameters = (PSENDCMDOUTPARAMS)buffer;

    availableBufferSize = *BufferSize - sizeof(SRB_IO_CONTROL);

#if DBG
    //
    // Ensure control codes and buffer lengths passed are correct
    //
    {
        ULONG controlCode;
        ULONG lengthNeeded = sizeof(SENDCMDINPARAMS) - 1;

        if (Command == SMART_CMD)
        {
            switch (Feature)
            {

                case ENABLE_SMART:
                {
                    controlCode = IOCTL_SCSI_MINIPORT_ENABLE_SMART;

                    break;
                }

                case DISABLE_SMART:
                {
                    controlCode = IOCTL_SCSI_MINIPORT_DISABLE_SMART;
                    break;
                }

                case  RETURN_SMART_STATUS:
                {
                     //
                    // Ensure bBuffer is at least 2 bytes (to hold the values of
                    // cylinderLow and cylinderHigh).
                    //

                    lengthNeeded = sizeof(SENDCMDINPARAMS) - 1 + sizeof(IDEREGS);

                    controlCode = IOCTL_SCSI_MINIPORT_RETURN_STATUS;
                    break;
                }

                case ENABLE_DISABLE_AUTOSAVE:
                {
                    controlCode = IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE;
                    break;
                }

                case SAVE_ATTRIBUTE_VALUES:
                {
                    controlCode = IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES;
                    break;
                }


                case EXECUTE_OFFLINE_DIAGS:
                {
                    controlCode = IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS;
                    break;
                }

                case READ_ATTRIBUTES:
                {
                    controlCode = IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS;
                    lengthNeeded = READ_ATTRIBUTE_BUFFER_SIZE + sizeof(SENDCMDOUTPARAMS) - 1;
                    break;
                }

                case READ_THRESHOLDS:
                {
                    controlCode = IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS;
                    lengthNeeded = READ_THRESHOLD_BUFFER_SIZE + sizeof(SENDCMDOUTPARAMS) - 1;
                    break;
                }

                case SMART_READ_LOG:
                {
                    controlCode = IOCTL_SCSI_MINIPORT_READ_SMART_LOG;
                    lengthNeeded = (SectorCount * SMART_LOG_SECTOR_SIZE) +
                                   sizeof(SENDCMDINPARAMS) - 1;
                    break;
                }

                case SMART_WRITE_LOG:
                {
                    controlCode = IOCTL_SCSI_MINIPORT_WRITE_SMART_LOG;
                    lengthNeeded = (SectorCount * SMART_LOG_SECTOR_SIZE) +
                                   sizeof(SENDCMDINPARAMS) - 1;
                    break;
                }
                
                default:
                    controlCode = 0;
                    break;
                
            }
        } else if (Command == ID_CMD) {
            controlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
            lengthNeeded = IDENTIFY_BUFFER_SIZE + sizeof(SENDCMDOUTPARAMS) -1;
        } else {
            controlCode = 0;
            ASSERT(FALSE);
        }

        ASSERT(controlCode == SrbControlCode);
        ASSERT(availableBufferSize >= lengthNeeded);
    }
#endif

    //
    // Build SrbControl and input to SMART command
    //

    SrbControl->HeaderLength = sizeof(SRB_IO_CONTROL);
    RtlMoveMemory (SrbControl->Signature, "SCSIDISK", 8);
    SrbControl->Timeout = FdoExtension->TimeOutValue;
    SrbControl->Length = availableBufferSize;

    SrbControl->ControlCode = SrbControlCode;

    cmdInParameters->cBufferSize = sizeof(SENDCMDINPARAMS);
    cmdInParameters->bDriveNumber = diskData->ScsiAddress.TargetId;
    cmdInParameters->irDriveRegs.bFeaturesReg = Feature;
    cmdInParameters->irDriveRegs.bSectorCountReg = SectorCount;
    cmdInParameters->irDriveRegs.bSectorNumberReg = SectorNumber;
    cmdInParameters->irDriveRegs.bCylLowReg = SMART_CYL_LOW;
    cmdInParameters->irDriveRegs.bCylHighReg = SMART_CYL_HI;
    cmdInParameters->irDriveRegs.bCommandReg = Command;


    //
    // Create and send irp
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    startingOffset.QuadPart = (LONGLONG) 1;

    length = SrbControl->HeaderLength + SrbControl->Length;

    irp = IoBuildSynchronousFsdRequest(
                IRP_MJ_SCSI,
                commonExtension->LowerDeviceObject,
                SrbControl,
                length,
                &startingOffset,
                &event,
                &ioStatus);

    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpStack = IoGetNextIrpStackLocation(irp);

    //
    // Set major and minor codes.
    //

    irpStack->MajorFunction = IRP_MJ_SCSI;
    irpStack->MinorFunction = 1;

    //
    // Fill in SRB fields.
    //

    irpStack->Parameters.Others.Argument1 = &srb;

    //
    // Zero out the srb.
    //

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    srb.PathId = diskData->ScsiAddress.PathId;
    srb.TargetId = diskData->ScsiAddress.TargetId;
    srb.Lun = diskData->ScsiAddress.Lun;

    srb.Function = SRB_FUNCTION_IO_CONTROL;
    srb.Length = sizeof(SCSI_REQUEST_BLOCK);

    srb.SrbFlags = FdoExtension->SrbFlags;
    SET_FLAG(srb.SrbFlags, SRB_FLAGS_DATA_IN);
    SET_FLAG(srb.SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);
    SET_FLAG(srb.SrbFlags, SRB_FLAGS_NO_KEEP_AWAKE);

    srb.QueueAction = SRB_SIMPLE_TAG_REQUEST;
    srb.QueueTag = SP_UNTAGGED;

    srb.OriginalRequest = irp;

    //
    // Set timeout to requested value.
    //

    srb.TimeOutValue = SrbControl->Timeout;

    //
    // Set the data buffer.
    //

    srb.DataBuffer = SrbControl;
    srb.DataTransferLength = length;

    //
    // Flush the data buffer for output. This will insure that the data is
    // written back to memory.  Since the data-in flag is the the port driver
    // will flush the data again for input which will ensure the data is not
    // in the cache.
    //

    KeFlushIoBuffers(irp->MdlAddress, FALSE, TRUE);

    //
    // Call port driver to handle this request.
    //

    status = IoCallDriver(commonExtension->LowerDeviceObject, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    return status;
}

NTSTATUS
NTAPI
DiskGetIdentifyInfo(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    PBOOLEAN SupportSmart
    )
{
    UCHAR outBuffer[sizeof(SRB_IO_CONTROL) + (sizeof(SENDCMDINPARAMS)-1) + IDENTIFY_BUFFER_SIZE];
    ULONG outBufferSize = sizeof(outBuffer);
    NTSTATUS status;

    PAGED_CODE();

    status = DiskGetIdentifyData(FdoExtension,
                                 (PSRB_IO_CONTROL)outBuffer,
                                 &outBufferSize);

    if (NT_SUCCESS(status))
    {
        PUSHORT identifyData = (PUSHORT)&(outBuffer[sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDOUTPARAMS)-1]);
        USHORT commandSetSupported = identifyData[82];

        *SupportSmart = ((commandSetSupported != 0xffff) &&
                         (commandSetSupported != 0) &&
                         ((commandSetSupported & 1) == 1));
    } else {
        *SupportSmart = FALSE;
    }

    DebugPrint((3, "DiskGetIdentifyInfo: SMART %s supported for device %p, status %lx\n",
                   *SupportSmart ? "is" : "is not",
                   FdoExtension->DeviceObject,
                   status));

    return status;
}

//
// FP Ioctl specific routines
//

NTSTATUS
NTAPI
DiskSendFailurePredictIoctl(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    PSTORAGE_PREDICT_FAILURE checkFailure
    )
{
    KEVENT event;
    PDEVICE_OBJECT deviceObject;
    IO_STATUS_BLOCK ioStatus;
    PIRP irp;
    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    deviceObject = IoGetAttachedDeviceReference(FdoExtension->DeviceObject);

    irp = IoBuildDeviceIoControlRequest(
                    IOCTL_STORAGE_PREDICT_FAILURE,
                    deviceObject,
                    NULL,
                    0,
                    checkFailure,
                    sizeof(STORAGE_PREDICT_FAILURE),
                    FALSE,
                    &event,
                    &ioStatus);

    if (irp != NULL)
    {
        status = IoCallDriver(deviceObject, irp);
        if (status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    ObDereferenceObject(deviceObject);

    return status;
}

//
// FP type independent routines
//

NTSTATUS
NTAPI
DiskEnableDisableFailurePrediction(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    BOOLEAN Enable
    )
/*++

Routine Description:

    Enable or disable failure prediction at the hardware level

Arguments:

    FdoExtension

    Enable

Return Value:

    NT Status

--*/
{
    NTSTATUS status;
    PCOMMON_DEVICE_EXTENSION commonExtension = &(FdoExtension->CommonExtension);
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);

    PAGED_CODE();

    switch(diskData->FailurePredictionCapability)
    {
        case FailurePredictionSmart:
        {

            if (Enable)
            {
                status = DiskEnableSmart(FdoExtension);
            } else {
                status = DiskDisableSmart(FdoExtension);
            }

            break;
        }

        case  FailurePredictionSense:
        case  FailurePredictionIoctl:
        {
            //
            // We assume that the drive is already setup properly for
            // failure prediction
            //
            status = STATUS_SUCCESS;
            break;
        }

        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }
    return status;
}

NTSTATUS
NTAPI
DiskEnableDisableFailurePredictPolling(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    BOOLEAN Enable,
    ULONG PollTimeInSeconds
    )
/*++

Routine Description:

    Enable or disable polling for hardware failure detection

Arguments:

    FdoExtension

    Enable

    PollTimeInSeconds - if 0 then no change to current polling timer

Return Value:

    NT Status

--*/
{
    NTSTATUS status;
    PCOMMON_DEVICE_EXTENSION commonExtension = (PCOMMON_DEVICE_EXTENSION)FdoExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);

    PAGED_CODE();

    if (Enable)
    {
        status = DiskEnableDisableFailurePrediction(FdoExtension,
                                           Enable);
    } else {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status))
    {
        status = ClassSetFailurePredictionPoll(FdoExtension,
                        Enable ? diskData->FailurePredictionCapability :
                                 FailurePredictionNone,
                                     PollTimeInSeconds);

        //
        // Even if this failed we do not want to disable FP on the
        // hardware. FP is only ever disabled on the hardware by
        // specific command of the user.
        //
    }

    return status;
}

NTSTATUS
NTAPI
DiskReadFailurePredictStatus(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    PSTORAGE_FAILURE_PREDICT_STATUS DiskSmartStatus
    )
/*++

Routine Description:

    Obtains current failure prediction status

Arguments:

    FdoExtension

    DiskSmartStatus

Return Value:

    NT Status

--*/
{
    PCOMMON_DEVICE_EXTENSION commonExtension = (PCOMMON_DEVICE_EXTENSION)FdoExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    NTSTATUS status;

    PAGED_CODE();

    DiskSmartStatus->PredictFailure = FALSE;

    switch(diskData->FailurePredictionCapability)
    {
        case FailurePredictionSmart:
        {
            UCHAR outBuffer[sizeof(SRB_IO_CONTROL) + (sizeof(SENDCMDINPARAMS) - 1 + sizeof(IDEREGS))];
            ULONG outBufferSize = sizeof(outBuffer);
            PSENDCMDOUTPARAMS cmdOutParameters;

            status = DiskReadSmartStatus(FdoExtension,
                                     (PSRB_IO_CONTROL)outBuffer,
                                     &outBufferSize);

            if (NT_SUCCESS(status))
            {
                cmdOutParameters = (PSENDCMDOUTPARAMS)(outBuffer +
                                               sizeof(SRB_IO_CONTROL));

                DiskSmartStatus->Reason = 0; // Unknown;
                DiskSmartStatus->PredictFailure = ((((PUCHAR)cmdOutParameters->bBuffer)[3] == 0xf4) &&
                                                   (((PUCHAR)cmdOutParameters->bBuffer)[4] == 0x2c));
            }
            break;
        }

        case FailurePredictionSense:
        {
            DiskSmartStatus->Reason = FdoExtension->FailureReason;
            DiskSmartStatus->PredictFailure = FdoExtension->FailurePredicted;
            status = STATUS_SUCCESS;
            break;
        }

        case FailurePredictionIoctl:
        case FailurePredictionNone:
        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    return status;
}

NTSTATUS
NTAPI
DiskReadFailurePredictData(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    PSTORAGE_FAILURE_PREDICT_DATA DiskSmartData
    )
/*++

Routine Description:

    Obtains current failure prediction data. Not available for
    FAILURE_PREDICT_SENSE types.

Arguments:

    FdoExtension

    DiskSmartData

Return Value:

    NT Status

--*/
{
    PCOMMON_DEVICE_EXTENSION commonExtension = (PCOMMON_DEVICE_EXTENSION)FdoExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    NTSTATUS status;

    PAGED_CODE();

    switch(diskData->FailurePredictionCapability)
    {
        case FailurePredictionSmart:
        {
            PUCHAR outBuffer;
            ULONG outBufferSize;
            PSENDCMDOUTPARAMS cmdOutParameters;

            outBufferSize = sizeof(SRB_IO_CONTROL) +
                            (sizeof(SENDCMDOUTPARAMS)-1) +
                            READ_ATTRIBUTE_BUFFER_SIZE;

            outBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                              outBufferSize,
                                              DISK_TAG_SMART);

            if (outBuffer != NULL)
            {
                status = DiskReadSmartData(FdoExtension,
                                           (PSRB_IO_CONTROL)outBuffer,
                                           &outBufferSize);

                if (NT_SUCCESS(status))
                {
                    cmdOutParameters = (PSENDCMDOUTPARAMS)(outBuffer +
                                                    sizeof(SRB_IO_CONTROL));

                    DiskSmartData->Length = READ_ATTRIBUTE_BUFFER_SIZE;
                    RtlCopyMemory(DiskSmartData->VendorSpecific,
                                  cmdOutParameters->bBuffer,
                                  READ_ATTRIBUTE_BUFFER_SIZE);
                }
                ExFreePool(outBuffer);
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            break;
        }

        case FailurePredictionSense:
        {
            DiskSmartData->Length = sizeof(ULONG);
            *((PULONG)DiskSmartData->VendorSpecific) = FdoExtension->FailureReason;

            status = STATUS_SUCCESS;
            break;
        }

        case FailurePredictionIoctl:
        case FailurePredictionNone:
        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    return status;
}

NTSTATUS
NTAPI
DiskReadFailurePredictThresholds(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    PSTORAGE_FAILURE_PREDICT_THRESHOLDS DiskSmartThresholds
    )
/*++

Routine Description:

    Obtains current failure prediction thresholds. Not available for
    FAILURE_PREDICT_SENSE types.

Arguments:

    FdoExtension

    DiskSmartData

Return Value:

    NT Status

--*/
{
    PCOMMON_DEVICE_EXTENSION commonExtension = (PCOMMON_DEVICE_EXTENSION)FdoExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    NTSTATUS status;

    PAGED_CODE();

    switch(diskData->FailurePredictionCapability)
    {
        case FailurePredictionSmart:
        {
            PUCHAR outBuffer;
            PSENDCMDOUTPARAMS cmdOutParameters;
            ULONG outBufferSize;

            outBufferSize = sizeof(SRB_IO_CONTROL) +
                            (sizeof(SENDCMDOUTPARAMS)-1) +
                            READ_THRESHOLD_BUFFER_SIZE;
            
            outBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                              outBufferSize,
                                              DISK_TAG_SMART);

            if (outBuffer != NULL)
            {
                status = DiskReadSmartThresholds(FdoExtension,
                                                (PSRB_IO_CONTROL)outBuffer,
                                                &outBufferSize);

                if (NT_SUCCESS(status))
                {
                    cmdOutParameters = (PSENDCMDOUTPARAMS)(outBuffer +
                                           sizeof(SRB_IO_CONTROL));

                    RtlCopyMemory(DiskSmartThresholds->VendorSpecific,
                                  cmdOutParameters->bBuffer,
                                  READ_THRESHOLD_BUFFER_SIZE);
                }
                ExFreePool(outBuffer);
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            break;
        }

        case FailurePredictionSense:
        case FailurePredictionIoctl:
        case FailurePredictionNone:
        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    return status;
}

void NTAPI DiskReregWorker(
    IN PVOID Context
    )
{
    PDISKREREGREQUEST reregRequest;
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PIRP irp;

    PAGED_CODE();
    
    do
    {
        reregRequest = (PDISKREREGREQUEST)ExInterlockedPopEntryList(
            &DiskReregHead,
            &DiskReregSpinlock);

        deviceObject = reregRequest->DeviceObject;
        irp = reregRequest->Irp;
        
        status = IoWMIRegistrationControl(deviceObject,
                                          WMIREG_ACTION_UPDATE_GUIDS);

        if (! NT_SUCCESS(status))
        {
            DebugPrint((1, "DiskReregWorker: Reregistration failed %x\n",
                        status));
        }

        //
        // Release remove lock and free irp, now that we are done
        // processing this
        //
        ClassReleaseRemoveLock(deviceObject, irp);

        IoFreeMdl(irp->MdlAddress);
        IoFreeIrp(irp);
        
        ExFreePool(reregRequest);
        
    } while (InterlockedDecrement(&DiskReregWorkItems));

    
}

NTSTATUS NTAPI DiskInitializeReregistration(
    void
    )
{
    PAGED_CODE();
    
    //
    // Initialize the global work item and spinlock used to manage the
    // list of disks reregistering their guids
    //
    ExInitializeWorkItem( &DiskReregWorkItem,
                          DiskReregWorker,
                          NULL );

    KeInitializeSpinLock(&DiskReregSpinlock);

    return(STATUS_SUCCESS);
}

NTSTATUS NTAPI DiskPostReregisterRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PDISKREREGREQUEST reregRequest;
    NTSTATUS status;
    
    reregRequest = ExAllocatePoolWithTag(NonPagedPool,
                                         sizeof(DISKREREGREQUEST),
                                         DISK_TAG_SMART);

    if (reregRequest != NULL)
    {
        //
        // add the disk that needs reregistration to the stack of disks
        // to reregister. If the list is transitioning from empty to
        // non empty then also kick off the work item so that the
        // reregistration worker can do the reregister.
        //
        reregRequest->DeviceObject = DeviceObject;
        reregRequest->Irp = Irp;
        ExInterlockedPushEntryList(
                                   &DiskReregHead,
                                   &reregRequest->Next,
                                   &DiskReregSpinlock);
    
        if (InterlockedIncrement(&DiskReregWorkItems) == 1)
        {
            ExQueueWorkItem( &DiskReregWorkItem, DelayedWorkQueue );
        }
        status = STATUS_SUCCESS;
    } else {
        DebugPrint((1, "DiskPostReregisterRequest: could not allocate reregRequest for %p\n",
                    DeviceObject));
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return(status);
}

NTSTATUS NTAPI DiskInfoExceptionComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = Context;
    NTSTATUS status;
    BOOLEAN retry;
    ULONG retryInterval;
    ULONG srbStatus;
    BOOLEAN freeLockAndIrp = TRUE;
    //KIRQL oldIrql;

    ASSERT(fdoExtension->CommonExtension.IsFdo);

    srbStatus = SRB_STATUS(srb->SrbStatus);

    //
    // Check SRB status for success of completing request.
    // SRB_STATUS_DATA_OVERRUN also indicates success.
    //
    if ((srbStatus != SRB_STATUS_SUCCESS) &&
        (srbStatus != SRB_STATUS_DATA_OVERRUN))
    {    
        DebugPrint((2, "DiskInfoExceptionComplete: IRP %p, SRB %p\n", Irp, srb));

        retry = ClassInterpretSenseInfo(
                    DeviceObject,
                    srb,
                    irpStack->MajorFunction,
                     0,
                    MAXIMUM_RETRIES -
                        ((ULONG)(ULONG_PTR)irpStack->Parameters.Others.Argument4),
                    &status,
                    &retryInterval);

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (TEST_FLAG(irpStack->Flags, SL_OVERRIDE_VERIFY_VOLUME) &&
            status == STATUS_VERIFY_REQUIRED)
        {
            status = STATUS_IO_DEVICE_ERROR;
            retry = TRUE;
        }

        if (retry && irpStack->Parameters.Others.Argument4)
        {
            irpStack->Parameters.Others.Argument4 =
                (PVOID)((ULONG_PTR)irpStack->Parameters.Others.Argument4 - 1);

            //
            // Retry request.
            //

            DebugPrint((1, "DiskInfoExceptionComplete: Retry request %p\n", Irp));
            
            ASSERT(srb->DataBuffer == MmGetMdlVirtualAddress(Irp->MdlAddress));
            
            //
            // Reset byte count of transfer in SRB Extension.
            //
            srb->DataTransferLength = Irp->MdlAddress->ByteCount;
            
            //
            // Zero SRB statuses.
            //

            srb->SrbStatus = srb->ScsiStatus = 0;

            //
            // Set the no disconnect flag, disable synchronous data transfers and
            // disable tagged queuing. This fixes some errors.
            //

            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_DISCONNECT);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            CLEAR_FLAG(srb->SrbFlags, SRB_FLAGS_QUEUE_ACTION_ENABLE);

            srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
            srb->QueueTag = SP_UNTAGGED;

            //
            // Set up major SCSI function.
            //

            nextIrpStack->MajorFunction = IRP_MJ_SCSI;

            //
            // Save SRB address in next stack for port driver.
            //

            nextIrpStack->Parameters.Scsi.Srb = srb;


            IoSetCompletionRoutine(Irp,
                                   DiskInfoExceptionComplete,
                                   srb,
                                   TRUE, TRUE, TRUE);
            
            (VOID)IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
    
    } else {

        //
        // Get the results from the mode sense
        //
        PMODE_INFO_EXCEPTIONS pageData;
        PMODE_PARAMETER_HEADER modeData;
        ULONG modeDataLength;

        modeData = srb->DataBuffer;
        modeDataLength = srb->DataTransferLength;
        
        pageData = ClassFindModePage((PUCHAR) modeData,
                                     modeDataLength,
                                     MODE_PAGE_FAULT_REPORTING,
                                     TRUE);
        if (pageData != NULL)
        {
            DebugPrint((1, "DiskInfoExceptionComplete: %p supports SMART\n",
                        DeviceObject));

            if (pageData->Dexcpt == 0)
            {
                diskData->FailurePredictionCapability = FailurePredictionSense;
                status = DiskPostReregisterRequest(DeviceObject, Irp);

                if (NT_SUCCESS(status))
                {
                    //
                    // Make sure we won't free the remove lock and the irp
                    // since we need to keep these until after the work
                    // item has completed running
                    //
                    freeLockAndIrp = FALSE;
                }
            } else {
                DebugPrint((1, "DiskInfoExceptionComplete: %p is not enabled for SMART\n",
                        DeviceObject));
            
            }
            
        } else {
            DebugPrint((1, "DiskInfoExceptionComplete: %p does not supports SMART\n",
                        DeviceObject));
            
        }
        
        //
        // Set status for successful request
        //

        status = STATUS_SUCCESS;

    } // end if (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_SUCCESS)

    //
    // Free the srb
    //
    ExFreePool(srb->SenseInfoBuffer);
    ExFreePool(srb->DataBuffer);
    ExFreePool(srb);
        
    if (freeLockAndIrp)
    {
        //
        // Set status in completing IRP.
        //

        Irp->IoStatus.Status = status;

        //
        // If pending has be returned for this irp then mark the current stack as
        // pending.
        //

        if (Irp->PendingReturned) {
            IoMarkIrpPending(Irp);
        }

        ClassReleaseRemoveLock(DeviceObject, Irp);
        IoFreeMdl(Irp->MdlAddress);
        IoFreeIrp(Irp);
    }
    
    return(STATUS_MORE_PROCESSING_REQUIRED);
    
}

NTSTATUS NTAPI DiskInfoExceptionCheck(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PUCHAR modeData;
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    PVOID senseInfoBuffer;
    ULONG isRemoved;
    
    modeData = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                     MODE_DATA_SIZE,
                                     DISK_TAG_INFO_EXCEPTION);
    if (modeData == NULL)
    {
        DebugPrint((1, "DiskInfoExceptionCheck: Can't allocate mode data "
                        "buffer\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
        
    srb = ExAllocatePoolWithTag(NonPagedPool,
                                SCSI_REQUEST_BLOCK_SIZE,
                                DISK_TAG_SRB);
    if (srb == NULL)
    {
        ExFreePool(modeData);
        DebugPrint((1, "DiskInfoExceptionCheck: Can't allocate srb "
                        "buffer\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);      
    }
    
    //
    // Build the MODE SENSE CDB.
    //
    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    
    cdb = (PCDB)srb->Cdb;
    srb->CdbLength = 6;
    cdb = (PCDB)srb->Cdb;
    
    //
    // Set timeout value from device extension.
    //
    srb->TimeOutValue = FdoExtension->TimeOutValue;

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.PageCode = MODE_PAGE_FAULT_REPORTING;
    cdb->MODE_SENSE.AllocationLength = MODE_DATA_SIZE;
    
    //
    // Write length to SRB.
    //
    srb->Length = SCSI_REQUEST_BLOCK_SIZE;

    //
    // Set SCSI bus address.
    //

    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    //
    // Enable auto request sense.
    //  
    
    srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

    //
    // Sense buffer is in aligned nonpaged pool.
    //

    senseInfoBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                     SENSE_BUFFER_SIZE,
                                     '7CcS');

    if (senseInfoBuffer == NULL)
    {
        ExFreePool(srb);
        ExFreePool(modeData);
        DebugPrint((1, "DiskInfoExceptionCheck: Can't allocate request sense "
                        "buffer\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    srb->SenseInfoBuffer = senseInfoBuffer;
    srb->DataBuffer = modeData;
    
    srb->SrbFlags = FdoExtension->SrbFlags;


    SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);
    
    //
    // Disable synchronous transfer for these requests.
    //
    SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);

    //
    // Don't freeze the queue on an error
    //  
    SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);

    srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
    srb->QueueTag = SP_UNTAGGED;

        
    //
    // Build device I/O control request with METHOD_NEITHER data transfer.
    // We'll queue a completion routine to cleanup the MDL's and such ourself.
    //

    irp = IoAllocateIrp(
            (CCHAR) (FdoExtension->CommonExtension.LowerDeviceObject->StackSize + 1),
            FALSE);

    if (irp == NULL)
    {
        ExFreePool(senseInfoBuffer);
        ExFreePool(srb);
        ExFreePool(modeData);
        DebugPrint((1, "DiskInfoExceptionCheck: Can't allocate Irp\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    isRemoved = ClassAcquireRemoveLock(FdoExtension->DeviceObject, irp);

    if (isRemoved)
    {
        ClassReleaseRemoveLock(FdoExtension->DeviceObject, irp);
        IoFreeIrp(irp);
        ExFreePool(senseInfoBuffer);
        ExFreePool(srb);
        ExFreePool(modeData);
        DebugPrint((1, "DiskInfoExceptionCheck: RemoveLock says isRemoved\n"));
        return(STATUS_DEVICE_DOES_NOT_EXIST);
    }
    
    //
    // Get next stack location.
    //

    IoSetNextIrpStackLocation(irp);
    irpStack = IoGetCurrentIrpStackLocation(irp);
    irpStack->DeviceObject = FdoExtension->DeviceObject;

    //
    // Save retry count in current Irp stack.
    //
    irpStack->Parameters.Others.Argument4 = (PVOID)MAXIMUM_RETRIES;
    
    
    irpStack = IoGetNextIrpStackLocation(irp);

    //
    // Set up SRB for execute scsi request. Save SRB address in next stack
    // for the port driver.
    //

    irpStack->MajorFunction = IRP_MJ_SCSI;
    irpStack->Parameters.Scsi.Srb = srb;

    IoSetCompletionRoutine(irp,
                           DiskInfoExceptionComplete,
                           srb,
                           TRUE,
                           TRUE,
                           TRUE);

    irp->MdlAddress = IoAllocateMdl( modeData,
                                     MODE_DATA_SIZE,
                                     FALSE,
                                     FALSE,
                                     irp );
    if (irp->MdlAddress == NULL)
    {
        ClassReleaseRemoveLock(FdoExtension->DeviceObject, irp);
        ExFreePool(srb);
        ExFreePool(modeData);
        ExFreePool(senseInfoBuffer);
        IoFreeIrp( irp );
        DebugPrint((1, "DiskINfoExceptionCheck: Can't allocate MDL\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool(irp->MdlAddress);
        
    //
    // Set the transfer length.
    //
    srb->DataTransferLength = MODE_DATA_SIZE;

    //
    // Zero out status.
    //
    srb->ScsiStatus = srb->SrbStatus = 0;
    srb->NextSrb = 0;

    //
    // Set up IRP Address.
    //
    srb->OriginalRequest = irp;
    
    //
    // Call the port driver with the request and wait for it to complete.
    //

    IoMarkIrpPending(irp);
    IoCallDriver(FdoExtension->CommonExtension.LowerDeviceObject,
                          irp);

    return(STATUS_PENDING);
}

NTSTATUS
NTAPI
DiskDetectFailurePrediction(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    PFAILURE_PREDICTION_METHOD FailurePredictCapability
    )
/*++

Routine Description:

    Detect if device has any failure prediction capabilities. First we
    check for IDE SMART capability. This is done by sending the drive an
    IDENTIFY command and checking if the SMART command set bit is set.

    Next we check if SCSI SMART (aka Information Exception Control Page,
    X3T10/94-190 Rev 4). This is done by querying for the Information
    Exception mode page.

    Lastly we check if the device has IOCTL failure prediction. This mechanism
    a filter driver implements IOCTL_STORAGE_PREDICT_FAILURE and will respond
    with the information in the IOCTL. We do this by sending the ioctl and
    if the status returned is STATUS_SUCCESS we assume that it is supported.

Arguments:

    FdoExtension

    *FailurePredictCapability

Return Value:

    NT Status

--*/
{
    PCOMMON_DEVICE_EXTENSION commonExtension = (PCOMMON_DEVICE_EXTENSION)FdoExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    BOOLEAN supportFP;
    NTSTATUS status;
    STORAGE_PREDICT_FAILURE checkFailure;
    STORAGE_FAILURE_PREDICT_STATUS diskSmartStatus;
    //BOOLEAN logErr;

    PAGED_CODE();

    //
    // Assume no failure predict mechanisms
    //
    *FailurePredictCapability = FailurePredictionNone;

    //
    // See if this is an IDE drive that supports SMART. If so enable SMART
    // and then ensure that it supports the SMART READ STATUS command
    //
    status = DiskGetIdentifyInfo(FdoExtension,
                                 &supportFP);

    if (supportFP)
    {
        status = DiskEnableSmart(FdoExtension);
        if (NT_SUCCESS(status))
        {
            *FailurePredictCapability = FailurePredictionSmart;

            status = DiskReadFailurePredictStatus(FdoExtension,
                                                  &diskSmartStatus);

            DebugPrint((1, "Disk: Device %p %s IDE SMART\n",
                       FdoExtension->DeviceObject,
                       NT_SUCCESS(status) ? "does" : "does not"));

            if (! NT_SUCCESS(status))
            {
                *FailurePredictCapability = FailurePredictionNone;
            }
        }
        return(status);
    }

    //
    // See if there is a a filter driver to intercept
    // IOCTL_STORAGE_PREDICT_FAILURE
    //
    status = DiskSendFailurePredictIoctl(FdoExtension,
                                         &checkFailure);

    DebugPrint((1, "Disk: Device %p %s IOCTL_STORAGE_FAILURE_PREDICT\n",
                       FdoExtension->DeviceObject,
                       NT_SUCCESS(status) ? "does" : "does not"));

    if (NT_SUCCESS(status))
    {
        *FailurePredictCapability = FailurePredictionIoctl;
        if (checkFailure.PredictFailure)
        {
            checkFailure.PredictFailure = 512;
            ClassNotifyFailurePredicted(FdoExtension,
                                            (PUCHAR)&checkFailure,
                                            sizeof(checkFailure),
                                            (BOOLEAN)(FdoExtension->FailurePredicted == FALSE),
                                            0x11,
                                            diskData->ScsiAddress.PathId,
                                            diskData->ScsiAddress.TargetId,
                                            diskData->ScsiAddress.Lun);

            FdoExtension->FailurePredicted = TRUE;
        }
        return(status);
    }
    
    //
    // Finally we assume it will not be a scsi smart drive. but
    // we'll also send off an asynchronous mode sense so that if
    // it is SMART we'll reregister the device object
    //

    DiskInfoExceptionCheck(FdoExtension);
    
    *FailurePredictCapability = FailurePredictionNone;

    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
DiskWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN CLASSENABLEDISABLEFUNCTION Function,
    IN BOOLEAN Enable
    )
/*++

Routine Description:

    This routine is a callback into the driver to enabled or disable event
    generation or data block collection. A device should only expect a
    single enable when the first event or data consumer enables events or
    data collection and a single disable when the last event or data
    consumer disables events or data collection. Data blocks will only
    receive collection enable/disable if they were registered as requiring
    it.


    When NT boots, failure prediction is not automatically enabled, although
    it may have been persistently enabled on a previous boot. Polling is also
    not automatically enabled. When the first data block that accesses SMART
    such as SmartStatusGuid, SmartDataGuid, SmartPerformFunction, or
    SmartEventGuid is accessed then SMART is automatically enabled in the
    hardware. Polling is enabled when SmartEventGuid is enabled and disabled
    when it is disabled. Hardware SMART is only disabled when the DisableSmart
    method is called. Polling is also disabled when this is called regardless
    of the status of the other guids or events.

Arguments:

    DeviceObject is the device whose data block is being queried

    GuidIndex is the index into the list of guids provided when the
        device registered

    Function specifies which functionality is being enabled or disabled

    Enable is TRUE then the function is being enabled else disabled

Return Value:

    status

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    //PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    //PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    //ULONG enableCount;

    PAGED_CODE();

    if ((Function == DataBlockCollection) && Enable)
    {
        if ((GuidIndex == SmartStatusGuid) ||
            (GuidIndex == SmartDataGuid) ||
            (GuidIndex == SmartThresholdsGuid) ||
            (GuidIndex == SmartPerformFunction))
        {
            status = DiskEnableDisableFailurePrediction(fdoExtension,
                                                        TRUE);
            DebugPrint((3, "Disk: DeviceObject %p, Irp %p Enable -> %lx\n",
                       DeviceObject,
                       Irp,
                       status));

        } else {
            DebugPrint((3, "Disk: DeviceObject %p, Irp %p, GuidIndex %d %s for Collection\n",
                      DeviceObject, Irp,
                      GuidIndex,
                      Enable ? "Enabled" : "Disabled"));        }
    } else if (Function == EventGeneration) {
        DebugPrint((3, "Disk: DeviceObject %p, Irp %p, GuidIndex %d %s for Event Generation\n",
                  DeviceObject, Irp,
                  GuidIndex,
                  Enable ? "Enabled" : "Disabled"));


        if ((GuidIndex == SmartEventGuid) && Enable)
        {
            status = DiskEnableDisableFailurePredictPolling(fdoExtension,
                                                   Enable,
                                                   0);
            DebugPrint((3, "Disk: DeviceObject %p, Irp %p %s -> %lx\n",
                       DeviceObject,
                       Irp,
                       Enable ? "DiskEnableSmartPolling" : "DiskDisableSmartPolling",
                       status));
        }

#if DBG
    } else {
        DebugPrint((3, "Disk: DeviceObject %p, Irp %p, GuidIndex %d %s for function %d\n",
                  DeviceObject, Irp,
                  GuidIndex,
                  Enable ? "Enabled" : "Disabled",
                  Function));
#endif
    }

    status = ClassWmiCompleteRequest(DeviceObject,
                                     Irp,
                                     status,
                                     0,
                                     IO_NO_INCREMENT);
    return status;
}

NTSTATUS
NTAPI
DiskFdoQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve the list of
    guids or data blocks that the driver wants to register with WMI. This
    routine may not pend or block. Driver should NOT call
    ClassWmiCompleteRequest.

Arguments:

    DeviceObject is the device whose data block is being queried

    *RegFlags returns with a set of flags that describe the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.


Return Value:

    status

--*/
{
    //PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    //NTSTATUS status;

    PAGED_CODE();

    SET_FLAG(DiskWmiFdoGuidList[SmartThresholdsGuid].Flags,  WMIREG_FLAG_REMOVE_GUID);
    SET_FLAG(DiskWmiFdoGuidList[ScsiInfoExceptionsGuid].Flags,  WMIREG_FLAG_REMOVE_GUID);
    
    switch (diskData->FailurePredictionCapability)
    {
        case FailurePredictionSmart:
        {
            CLEAR_FLAG(DiskWmiFdoGuidList[SmartThresholdsGuid].Flags,  WMIREG_FLAG_REMOVE_GUID);
            //
            // Fall Through
            //
        }
        case FailurePredictionIoctl:
        {
            CLEAR_FLAG(DiskWmiFdoGuidList[SmartStatusGuid].Flags,      WMIREG_FLAG_REMOVE_GUID);
            CLEAR_FLAG(DiskWmiFdoGuidList[SmartDataGuid].Flags,        WMIREG_FLAG_REMOVE_GUID);
            CLEAR_FLAG(DiskWmiFdoGuidList[SmartEventGuid].Flags,       WMIREG_FLAG_REMOVE_GUID);
            CLEAR_FLAG(DiskWmiFdoGuidList[SmartPerformFunction].Flags, WMIREG_FLAG_REMOVE_GUID);
                  
            break;
        }

        case FailurePredictionSense:
        {
            CLEAR_FLAG(DiskWmiFdoGuidList[SmartStatusGuid].Flags,      WMIREG_FLAG_REMOVE_GUID);
            CLEAR_FLAG(DiskWmiFdoGuidList[SmartEventGuid].Flags,       WMIREG_FLAG_REMOVE_GUID);
            CLEAR_FLAG(DiskWmiFdoGuidList[SmartPerformFunction].Flags, WMIREG_FLAG_REMOVE_GUID);
            CLEAR_FLAG(DiskWmiFdoGuidList[ScsiInfoExceptionsGuid].Flags,  WMIREG_FLAG_REMOVE_GUID);
            SET_FLAG  (DiskWmiFdoGuidList[SmartDataGuid].Flags,        WMIREG_FLAG_REMOVE_GUID);
            break;
        }


        default:
        {
            SET_FLAG  (DiskWmiFdoGuidList[SmartStatusGuid].Flags,      WMIREG_FLAG_REMOVE_GUID);
            SET_FLAG  (DiskWmiFdoGuidList[SmartDataGuid].Flags,        WMIREG_FLAG_REMOVE_GUID);
            SET_FLAG  (DiskWmiFdoGuidList[SmartEventGuid].Flags,       WMIREG_FLAG_REMOVE_GUID);
            SET_FLAG  (DiskWmiFdoGuidList[SmartPerformFunction].Flags, WMIREG_FLAG_REMOVE_GUID);
            break;
        }
    }

    //
    // Use devnode for FDOs
    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DiskFdoQueryWmiRegInfoEx(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING MofName                      
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve the list of
    guids or data blocks that the driver wants to register with WMI. This
    routine may not pend or block. Driver should NOT call
    ClassWmiCompleteRequest.

Arguments:

    DeviceObject is the device whose data block is being queried

    *RegFlags returns with a set of flags that describe the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    MofName returns initialized with the mof resource name for the
        binary mof resource attached to the driver's image file. If the
        driver does not have a mof resource then it should leave this
        parameter untouched.

Return Value:

    status

--*/
{
    NTSTATUS status;
    
    status = DiskFdoQueryWmiRegInfo(DeviceObject,
                                    RegFlags,
                                    InstanceName);

    //
    // Leave MofName alone since disk doesn't have one
    //
    return(status);
}

NTSTATUS
NTAPI
DiskFdoQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    BufferAvail on has the maximum size available to write the data
        block.

    Buffer on return is filled with the returned data block


Return Value:

    status

--*/
{
    NTSTATUS status;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    ULONG sizeNeeded;

    PAGED_CODE();

    DebugPrint((3, "Disk: DiskQueryWmiDataBlock, Device %p, Irp %p, GuiIndex %d\n"
             "      BufferAvail %lx Buffer %lx\n",
             DeviceObject, Irp,
             GuidIndex, BufferAvail, Buffer));

    switch (GuidIndex)
    {
        case DiskGeometryGuid:
        {
            sizeNeeded = sizeof(DISK_GEOMETRY);
            if (BufferAvail >= sizeNeeded)
            {
                if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
                {
                    //
                    // Issue ReadCapacity to update device extension
                    // with information for current media.
                    status = DiskReadDriveCapacity(commonExtension->PartitionZeroExtension->DeviceObject);

                    //
                    // Note whether the drive is ready.
                    diskData->ReadyStatus = status;

                    if (!NT_SUCCESS(status))
                    {
                        break;
                    }
                }

                //
                // Copy drive geometry information from device extension.
                RtlMoveMemory(Buffer,
                              &(fdoExtension->DiskGeometry),
                              sizeof(DISK_GEOMETRY));

                status = STATUS_SUCCESS;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        }

        case SmartStatusGuid:
        {
            PSTORAGE_FAILURE_PREDICT_STATUS diskSmartStatus;

            ASSERT(diskData->FailurePredictionCapability != FailurePredictionNone);


            sizeNeeded = sizeof(STORAGE_FAILURE_PREDICT_STATUS);
            if (BufferAvail >= sizeNeeded)
            {
                STORAGE_PREDICT_FAILURE checkFailure;

                diskSmartStatus = (PSTORAGE_FAILURE_PREDICT_STATUS)Buffer;

                status = DiskSendFailurePredictIoctl(fdoExtension,
                                                     &checkFailure);

                if (NT_SUCCESS(status))
                {
                    if (diskData->FailurePredictionCapability ==
                                                      FailurePredictionSense)
                    {
                        diskSmartStatus->Reason =  *((PULONG)checkFailure.VendorSpecific);
                    } else {
                        diskSmartStatus->Reason =  0; // unknown
                    }

                    diskSmartStatus->PredictFailure = (checkFailure.PredictFailure != 0);
                }
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        }

        case SmartDataGuid:
        {
            PSTORAGE_FAILURE_PREDICT_DATA diskSmartData;

            ASSERT((diskData->FailurePredictionCapability ==
                                                  FailurePredictionSmart) ||
                   (diskData->FailurePredictionCapability ==
                                                  FailurePredictionIoctl));

            sizeNeeded = sizeof(STORAGE_FAILURE_PREDICT_DATA);
            if (BufferAvail >= sizeNeeded)
            {
                PSTORAGE_PREDICT_FAILURE checkFailure = (PSTORAGE_PREDICT_FAILURE)Buffer;

                diskSmartData = (PSTORAGE_FAILURE_PREDICT_DATA)Buffer;

                status = DiskSendFailurePredictIoctl(fdoExtension,
                                                     checkFailure);

                if (NT_SUCCESS(status))
                {
                    diskSmartData->Length = 512;
                }
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }

            break;
        }
        
        case SmartThresholdsGuid:
        {
            PSTORAGE_FAILURE_PREDICT_THRESHOLDS diskSmartThresholds;

            ASSERT((diskData->FailurePredictionCapability ==
                                                  FailurePredictionSmart));

            sizeNeeded = sizeof(STORAGE_FAILURE_PREDICT_THRESHOLDS);
            if (BufferAvail >= sizeNeeded)
            {
                diskSmartThresholds = (PSTORAGE_FAILURE_PREDICT_THRESHOLDS)Buffer;
                status = DiskReadFailurePredictThresholds(fdoExtension,
                                                          diskSmartThresholds);
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }

            break;
        }

        case SmartPerformFunction:
        {
            sizeNeeded = 0;
            status = STATUS_SUCCESS;
            break;
        }

        case ScsiInfoExceptionsGuid:
        {
            PSTORAGE_SCSI_INFO_EXCEPTIONS infoExceptions;
            MODE_INFO_EXCEPTIONS modeInfo;

            ASSERT((diskData->FailurePredictionCapability ==
                                                  FailurePredictionSense));
            
            sizeNeeded = sizeof(STORAGE_SCSI_INFO_EXCEPTIONS);
            if (BufferAvail >= sizeNeeded)
            {
                infoExceptions = (PSTORAGE_SCSI_INFO_EXCEPTIONS)Buffer;
                status = DiskGetInfoExceptionInformation(fdoExtension,
                                                         &modeInfo);

                if (NT_SUCCESS(status))
                {
                    infoExceptions->PageSavable = modeInfo.PSBit;
                    infoExceptions->Flags = modeInfo.Flags;
                    infoExceptions->MRIE = modeInfo.ReportMethod;
                    infoExceptions->Padding = 0;
                    REVERSE_BYTES(&infoExceptions->IntervalTimer,
                                  &modeInfo.IntervalTimer);
                    REVERSE_BYTES(&infoExceptions->ReportCount,
                                  &modeInfo.ReportCount)
                }
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }

            break;
        }

        default:
        {
            sizeNeeded = 0;
            status = STATUS_WMI_GUID_NOT_FOUND;
        }
    }
    DebugPrint((3, "Disk: DiskQueryWmiDataBlock Device %p, Irp %p returns %lx\n",
             DeviceObject, Irp, status));

    status = ClassWmiCompleteRequest(DeviceObject,
                                     Irp,
                                     status,
                                     sizeNeeded,
                                     IO_NO_INCREMENT);

    return status;
}

NTSTATUS
NTAPI
DiskFdoSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    BufferSize has the size of the data block passed

    Buffer has the new values for the data block


Return Value:

    status

--*/
{
    NTSTATUS status;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);

    PAGED_CODE();

    DebugPrint((3, "Disk: DiskSetWmiDataBlock, Device %p, Irp %p, GuiIndex %d\n"
             "      BufferSize %#x Buffer %p\n",
             DeviceObject, Irp,
             GuidIndex, BufferSize, Buffer));

    if (GuidIndex == ScsiInfoExceptionsGuid)
    {
        PSTORAGE_SCSI_INFO_EXCEPTIONS infoExceptions;
        MODE_INFO_EXCEPTIONS modeInfo;

        if (BufferSize >= sizeof(STORAGE_SCSI_INFO_EXCEPTIONS))
        {
            infoExceptions = (PSTORAGE_SCSI_INFO_EXCEPTIONS)Buffer;

            modeInfo.PageCode = MODE_PAGE_FAULT_REPORTING;
            modeInfo.PageLength = sizeof(MODE_INFO_EXCEPTIONS) - 2;

            modeInfo.PSBit = 0;
            modeInfo.Flags = infoExceptions->Flags;
            
            modeInfo.ReportMethod = infoExceptions->MRIE;

            REVERSE_BYTES(&modeInfo.IntervalTimer[0],
                          &infoExceptions->IntervalTimer);
            
            REVERSE_BYTES(&modeInfo.ReportCount[0],
                          &infoExceptions->ReportCount);

            if (modeInfo.Perf == 1)
            {
                diskData->AllowFPPerfHit = FALSE;
            } else {
                diskData->AllowFPPerfHit = TRUE;
            }
            
            status = DiskSetInfoExceptionInformation(fdoExtension,
                                                     &modeInfo);
        } else {
            status = STATUS_INVALID_PARAMETER;
        }
        
    } else if (GuidIndex <= SmartEventGuid)
    {
        status = STATUS_WMI_READ_ONLY;
    } else {
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    DebugPrint((3, "Disk: DiskSetWmiDataBlock Device %p, Irp %p returns %lx\n",
             DeviceObject, Irp, status));

    status = ClassWmiCompleteRequest(DeviceObject,
                                     Irp,
                                     status,
                                     0,
                                     IO_NO_INCREMENT);

    return status;
}

NTSTATUS
NTAPI
DiskFdoSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

    status

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    DebugPrint((3, "Disk: DiskSetWmiDataItem, Device %p, Irp %p, GuiIndex %d, DataId %d\n"
             "      BufferSize %#x Buffer %p\n",
             DeviceObject, Irp,
             GuidIndex, DataItemId, BufferSize, Buffer));

    if (GuidIndex <= SmartEventGuid)
    {
        status = STATUS_WMI_READ_ONLY;
    } else {
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    DebugPrint((3, "Disk: DiskSetWmiDataItem Device %p, Irp %p returns %lx\n",
             DeviceObject, Irp, status));

    status = ClassWmiCompleteRequest(DeviceObject,
                                     Irp,
                                     status,
                                     0,
                                     IO_NO_INCREMENT);

    return status;
}

NTSTATUS
NTAPI
DiskFdoExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to execute a method. When the
    driver has finished filling the data block it must call
    ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    MethodId has the id of the method being called

    InBufferSize has the size of the data block passed in as the input to
        the method.

    OutBufferSize on entry has the maximum size available to write the
        returned data block.

    Buffer is filled with the returned data block


Return Value:

    status

--*/
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    ULONG sizeNeeded;
    NTSTATUS status;

    PAGED_CODE();

    DebugPrint((3, "Disk: DiskExecuteWmiMethod, DeviceObject %p, Irp %p, Guid Id %d, MethodId %d\n"
             "      InBufferSize %#x, OutBufferSize %#x, Buffer %p\n",
             DeviceObject, Irp,
             GuidIndex, MethodId, InBufferSize, OutBufferSize, Buffer));

    switch(GuidIndex)
    {
        case SmartPerformFunction:
        {

            ASSERT((diskData->FailurePredictionCapability ==
                                                  FailurePredictionSmart) ||
                   (diskData->FailurePredictionCapability ==
                                                  FailurePredictionIoctl) ||
                   (diskData->FailurePredictionCapability ==
                                                  FailurePredictionSense));


            switch(MethodId)
            {
                //
                // void AllowPerformanceHit([in] boolean Allow)
                //
                case AllowDisallowPerformanceHit:
                {
                    BOOLEAN allowPerfHit;

                    sizeNeeded = 0;
                    if (InBufferSize >= sizeof(BOOLEAN))
                    {
                        status = STATUS_SUCCESS;

                        allowPerfHit = *((PBOOLEAN)Buffer);
                        if (diskData->AllowFPPerfHit !=  allowPerfHit)
                        {
                            diskData->AllowFPPerfHit = allowPerfHit;
                            if (diskData->FailurePredictionCapability ==
                                FailurePredictionSense)
                            {
                                MODE_INFO_EXCEPTIONS modeInfo;

                                status = DiskGetInfoExceptionInformation(fdoExtension,
                                                                         &modeInfo);
                                if (NT_SUCCESS(status))
                                {
                                    modeInfo.Perf = allowPerfHit ? 0 : 1;
                                    status = DiskSetInfoExceptionInformation(fdoExtension,
                                                                             &modeInfo);
                                }
                            }
                        }

                        DebugPrint((3, "DiskFdoWmiExecuteMethod: AllowPerformanceHit %x for device %p --> %lx\n",
                                    allowPerfHit,
                                    fdoExtension->DeviceObject,
                                    status));
                    } else {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    break;
                }

                //
                // void EnableDisableHardwareFailurePrediction([in] boolean Enable)
                //
                case EnableDisableHardwareFailurePrediction:
                {
                    BOOLEAN enable;

                    sizeNeeded = 0;
                    if (InBufferSize >= sizeof(BOOLEAN))
                    {
                        status = STATUS_SUCCESS;
                        enable = *((PBOOLEAN)Buffer);
                        if (! enable)
                        {
                            //
                            // If we are disabling we need to also disable
                            // polling
                            //
                            DiskEnableDisableFailurePredictPolling(
                                                               fdoExtension,
                                                               enable,
                                                               0);
                        }

                        status = DiskEnableDisableFailurePrediction(
                                                           fdoExtension,
                                                           enable);

                        DebugPrint((3, "DiskFdoWmiExecuteMethod: EnableDisableHardwareFailurePrediction: %x for device %p --> %lx\n",
                                    enable,
                                    fdoExtension->DeviceObject,
                                    status));
                    } else {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    break;
                }

                //
                // void EnableDisableFailurePredictionPolling(
                //                               [in] uint32 Period,
                //                               [in] boolean Enable)
                //
                case EnableDisableFailurePredictionPolling:
                {
                    BOOLEAN enable;
                    ULONG period;

                    sizeNeeded = 0;
                    if (InBufferSize >= (sizeof(ULONG) + sizeof(BOOLEAN)))
                    {
                        period = *((PULONG)Buffer);
                        Buffer += sizeof(ULONG);
                        enable = *((PBOOLEAN)Buffer);

                           status = DiskEnableDisableFailurePredictPolling(
                                                               fdoExtension,
                                                               enable,
                                                               period);

                        DebugPrint((3, "DiskFdoWmiExecuteMethod: EnableDisableFailurePredictionPolling: %x %x for device %p --> %lx\n",
                                    enable,
                                    period,
                                    fdoExtension->DeviceObject,
                                    status));
                    } else {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    break;
                }

                //
                // void GetFailurePredictionCapability([out] uint32 Capability)
                //
                case GetFailurePredictionCapability:
                {
                    sizeNeeded = sizeof(ULONG);
                    if (OutBufferSize >= sizeNeeded)
                    {
                        status = STATUS_SUCCESS;
                        *((PFAILURE_PREDICTION_METHOD)Buffer) = diskData->FailurePredictionCapability;
                        DebugPrint((3, "DiskFdoWmiExecuteMethod: GetFailurePredictionCapability: %x for device %p --> %lx\n",
                                    *((PFAILURE_PREDICTION_METHOD)Buffer),
                                    fdoExtension->DeviceObject,
                                    status));
                    } else {
                        status = STATUS_BUFFER_TOO_SMALL;
                    }
                    break;
                }

                //
                // void EnableOfflineDiags([out] boolean Success);
                //
                case EnableOfflineDiags:
                {
                    sizeNeeded = sizeof(BOOLEAN);
                    if (OutBufferSize >= sizeNeeded)
                    {
                        if (diskData->FailurePredictionCapability ==
                                  FailurePredictionSmart)
                        {
                            //
                            // Initiate or resume offline diagnostics.
                            // This may cause a loss of performance
                            // to the disk, but mayincrease the amount
                            // of disk checking.
                            //
                            status = DiskExecuteSmartDiagnostics(fdoExtension,
                                                                0);

                        } else {
                            status = STATUS_INVALID_DEVICE_REQUEST;
                        }

                        *((PBOOLEAN)Buffer) = NT_SUCCESS(status);

                        DebugPrint((3, "DiskFdoWmiExecuteMethod: EnableOfflineDiags for device %p --> %lx\n",
                                    fdoExtension->DeviceObject,
                                    status));
                    } else {
                        status = STATUS_BUFFER_TOO_SMALL;
                    }
                    break;
                }

                //
                //    void ReadLogSectors([in] uint8 LogAddress,
                //        [in] uint8 SectorCount,
                //        [out] uint32 Length,
                //        [out, WmiSizeIs("Length")] uint8 LogSectors[]
                //       );
                //
                case ReadLogSectors:
                {
                    sizeNeeded = 0;
                    if (diskData->FailurePredictionCapability ==
                                  FailurePredictionSmart)
                    {
                        if (InBufferSize >= sizeof(READ_LOG_SECTORS_IN))
                        {
                            PREAD_LOG_SECTORS_IN inParams;
                            PREAD_LOG_SECTORS_OUT outParams;
                            ULONG readSize;

                            inParams = (PREAD_LOG_SECTORS_IN)Buffer;
                            readSize = inParams->SectorCount * SMART_LOG_SECTOR_SIZE;
                            sizeNeeded = FIELD_OFFSET(READ_LOG_SECTORS_OUT,
                                                  LogSectors) + readSize;
                                                  
                            if (OutBufferSize >= sizeNeeded)
                            {
                                outParams = (PREAD_LOG_SECTORS_OUT)Buffer;
                                status = DiskReadSmartLog(fdoExtension,
                                                        inParams->SectorCount,
                                                        inParams->LogAddress,
                                                        outParams->LogSectors);

                                if (NT_SUCCESS(status))
                                {
                                    outParams->Length = readSize;
                                } else {
                                    //
                                    // SMART command failure is
                                    // indicated by successful
                                    // execution, but no data returned
                                    //
                                    outParams->Length = 0;
                                    status = STATUS_SUCCESS;
                                }
                            } else {
                                status = STATUS_BUFFER_TOO_SMALL;
                            }
                            
                        } else {
                            status = STATUS_INVALID_PARAMETER;
                        }
                    } else {
                        status = STATUS_INVALID_DEVICE_REQUEST;
                    }
                    break;
                }

                //    void WriteLogSectors([in] uint8 LogAddress,
                //        [in] uint8 SectorCount,
                //        [in] uint32 Length,
                //        [in, WmiSizeIs("Length")] uint8 LogSectors[],
                //        [out] boolean Success
                //       );
                case WriteLogSectors:
                {
                    sizeNeeded = 0;
                    if (diskData->FailurePredictionCapability ==
                                  FailurePredictionSmart)
                    {
                        if (InBufferSize >= FIELD_OFFSET(WRITE_LOG_SECTORS_IN,
                                                        LogSectors))
                        {
                            PWRITE_LOG_SECTORS_IN inParams;
                            PWRITE_LOG_SECTORS_OUT outParams;
                            ULONG writeSize;

                            inParams = (PWRITE_LOG_SECTORS_IN)Buffer;
                            writeSize = inParams->SectorCount * SMART_LOG_SECTOR_SIZE;
                            if (InBufferSize >= (FIELD_OFFSET(WRITE_LOG_SECTORS_IN,
                                                             LogSectors) +
                                                 writeSize))
                            {
                                sizeNeeded = sizeof(WRITE_LOG_SECTORS_OUT);
                                                  
                                if (OutBufferSize >= sizeNeeded)
                                {
                                    outParams = (PWRITE_LOG_SECTORS_OUT)Buffer;
                                    status = DiskWriteSmartLog(fdoExtension,
                                                        inParams->SectorCount,
                                                        inParams->LogAddress,
                                                        inParams->LogSectors);

                                    if (NT_SUCCESS(status))
                                    {
                                        outParams->Success = TRUE;
                                    } else {
                                        outParams->Success = FALSE;
                                        status = STATUS_SUCCESS;
                                    }                                   
                                } else {
                                    status = STATUS_BUFFER_TOO_SMALL;
                                }
                            } else {
                                status = STATUS_INVALID_PARAMETER;
                            }                           
                        } else {
                            status = STATUS_INVALID_PARAMETER;
                        }
                    } else {
                        status = STATUS_INVALID_DEVICE_REQUEST;
                    }
                    break;
                }

                //    void ExecuteSelfTest([in] uint8 Subcommand,
                //         [out, 
                //          Values{"0", "1", "2"},
                //          ValueMap{"Successful Completion",
                //                   "Captive Mode Required",
                //                   "Unsuccessful Completion"}
                //         ]
                //         uint32 ReturnCode);
                case ExecuteSelfTest:
                {
                    sizeNeeded = 0;
                    if (diskData->FailurePredictionCapability ==
                              FailurePredictionSmart)
                    {
                        if (InBufferSize >= sizeof(EXECUTE_SELF_TEST_IN))
                        {
                            sizeNeeded = sizeof(EXECUTE_SELF_TEST_OUT);
                            if (OutBufferSize >= sizeNeeded)
                            {
                                PEXECUTE_SELF_TEST_IN inParam;
                                PEXECUTE_SELF_TEST_OUT outParam;

                                inParam = (PEXECUTE_SELF_TEST_IN)Buffer;
                                outParam = (PEXECUTE_SELF_TEST_OUT)Buffer;

                                if (DiskIsValidSmartSelfTest(inParam->Subcommand))
                                {
                                   status = DiskExecuteSmartDiagnostics(fdoExtension,
                                                            inParam->Subcommand);
                                   if (NT_SUCCESS(status))
                                   {
                                       //
                                       // Return self test executed
                                       // without a problem
                                       //
                                       outParam->ReturnCode = 0;
                                   } else {
                                       //
                                       // Return Self test execution
                                       // failed status
                                       //
                                       outParam->ReturnCode = 2;
                                       status = STATUS_SUCCESS;
                                   }
                                } else {
                                    //
                                    // If self test subcommand requires
                                    // captive mode then return that
                                    // status
                                    //
                                    outParam->ReturnCode = 1;
                                    status = STATUS_SUCCESS;
                                }

                            } else {
                                status = STATUS_BUFFER_TOO_SMALL;
                            }
                            
                        } else {
                            status = STATUS_INVALID_PARAMETER;
                        }
                    } else {
                        status = STATUS_INVALID_DEVICE_REQUEST;
                    }
                            
                    break;
                }

                default :
                {
                    sizeNeeded = 0;
                    status = STATUS_WMI_ITEMID_NOT_FOUND;
                    break;
                }
            }

            break;
        }

        case DiskGeometryGuid:
        case SmartStatusGuid:
        case SmartDataGuid:
        {

            sizeNeeded = 0;
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        default:
        {
            sizeNeeded = 0;
            status = STATUS_WMI_GUID_NOT_FOUND;
        }
    }

    DebugPrint((3, "Disk: DiskExecuteMethod Device %p, Irp %p returns %lx\n",
             DeviceObject, Irp, status));

    status = ClassWmiCompleteRequest(DeviceObject,
                                     Irp,
                                     status,
                                     sizeNeeded,
                                     IO_NO_INCREMENT);

    return status;
}

#if 0
//
// Enable this to add WMI support for PDOs
NTSTATUS
NTAPI
DiskPdoQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve the list of
    guids or data blocks that the driver wants to register with WMI. This
    routine may not pend or block. Driver should NOT call
    ClassWmiCompleteRequest.

Arguments:

    DeviceObject is the device whose data block is being queried

    *RegFlags returns with a set of flags that describe the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.


Return Value:

    status

--*/
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION parentFunctionalExtension;
    ANSI_STRING ansiString;
    CHAR name[256];
    NTSTATUS status;

    //
    // We need to pick a name for PDOs since they do not have a devnode
    parentFunctionalExtension = commonExtension->PartitionZeroExtension;
    sprintf(name,
                "Disk(%d)_Partition(%d)_Start(%#I64x)_Length(%#I64x)",
                parentFunctionalExtension->DeviceNumber,
                commonExtension->PartitionNumber,
                commonExtension->StartingOffset.QuadPart,
                commonExtension->PartitionLength.QuadPart);
    RtlInitAnsiString(&ansiString,
                          name);

    status = RtlAnsiStringToUnicodeString(InstanceName,
                                     &ansiString,
                                     TRUE);

    return status;
}

NTSTATUS
NTAPI
DiskPdoQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    BufferAvail on has the maximum size available to write the data
        block.

    Buffer on return is filled with the returned data block


Return Value:

    status

--*/
{
    NTSTATUS status;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    ULONG sizeNeeded;

    DebugPrint((3, "Disk: DiskQueryWmiDataBlock, Device %p, Irp %p, GuiIndex %d\n"
             "      BufferAvail %#x Buffer %p\n",
             DeviceObject, Irp,
             GuidIndex, BufferAvail, Buffer));

    switch (GuidIndex)
    {
        case 0:
        {
            sizeNeeded = 4 * sizeof(ULONG);
            if (BufferAvail >= sizeNeeded)
            {
                RtlCopyMemory(Buffer, DiskDummyData, sizeNeeded);
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        }

        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }
    }

    DebugPrint((3, "Disk: DiskQueryWmiDataBlock Device %p, Irp %p returns %lx\n",
             DeviceObject, Irp, status));

    status = ClassWmiCompleteRequest(DeviceObject,
                                     Irp,
                                     status,
                                     sizeNeeded,
                                     IO_NO_INCREMENT);

    return status;
}

NTSTATUS
NTAPI
DiskPdoSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    BufferSize has the size of the data block passed

    Buffer has the new values for the data block


Return Value:

    status

--*/
{
    NTSTATUS status;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    ULONG sizeNeeded;

    DebugPrint((3, "Disk: DiskSetWmiDataBlock, Device %p, Irp %p, GuiIndex %d\n"
             "      BufferSize %#x Buffer %p\n",
             DeviceObject, Irp,
             GuidIndex, BufferSize, Buffer));

    switch(GuidIndex)
    {
        case 0:
        {
            sizeNeeded = 4 * sizeof(ULONG);
            if (BufferSize == sizeNeeded)
              {
                RtlCopyMemory(DiskDummyData, Buffer, sizeNeeded);
                status = STATUS_SUCCESS;
               } else {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;
        }

        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }
    }

    DebugPrint((3, "Disk: DiskSetWmiDataBlock Device %p, Irp %p returns %lx\n",
             DeviceObject, Irp, status));

    status = ClassWmiCompleteRequest(DeviceObject,
                                     Irp,
                                     status,
                                     0,
                                     IO_NO_INCREMENT);

    return status;
}

NTSTATUS
NTAPI
DiskPdoSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

    status

--*/
{
    NTSTATUS status;

    DebugPrint((3, "Disk: DiskSetWmiDataItem, Device %p, Irp %p, GuiIndex %d, DataId %d\n"
             "      BufferSize %#x Buffer %p\n",
             DeviceObject, Irp,
             GuidIndex, DataItemId, BufferSize, Buffer));

    switch(GuidIndex)
    {
        case 0:
        {
            if ((BufferSize == sizeof(ULONG)) &&
                (DataItemId <= 3))
              {
                  DiskDummyData[DataItemId] = *((PULONG)Buffer);
                   status = STATUS_SUCCESS;
               } else {
                   status = STATUS_INVALID_DEVICE_REQUEST;
               }
            break;
        }

        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }
    }


    DebugPrint((3, "Disk: DiskSetWmiDataItem Device %p, Irp %p returns %lx\n",
             DeviceObject, Irp, status));

    status = ClassWmiCompleteRequest(DeviceObject,
                                     Irp,
                                     status,
                                     0,
                                     IO_NO_INCREMENT);

    return status;
}

NTSTATUS
NTAPI
DiskPdoExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to execute a method. When the
    driver has finished filling the data block it must call
    ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    MethodId has the id of the method being called

    InBufferSize has the size of the data block passed in as the input to
        the method.

    OutBufferSize on entry has the maximum size available to write the
        returned data block.

    Buffer is filled with the returned data block


Return Value:

    status

--*/
{
    ULONG sizeNeeded = 4 * sizeof(ULONG);
    NTSTATUS status;
    ULONG tempData[4];

    DebugPrint((3, "Disk: DiskExecuteWmiMethod, DeviceObject %p, Irp %p, Guid Id %d, MethodId %d\n"
             "      InBufferSize %#x, OutBufferSize %#x, Buffer %p\n",
             DeviceObject, Irp,
             GuidIndex, MethodId, InBufferSize, OutBufferSize, Buffer));

    switch(GuidIndex)
    {
        case 0:
        {
            if (MethodId == 1)
            {
                if (OutBufferSize >= sizeNeeded)
                {

                    if (InBufferSize == sizeNeeded)
                    {
                        RtlCopyMemory(tempData, Buffer, sizeNeeded);
                        RtlCopyMemory(Buffer, DiskDummyData, sizeNeeded);
                        RtlCopyMemory(DiskDummyData, tempData, sizeNeeded);

                        status = STATUS_SUCCESS;
                    } else {
                        status = STATUS_INVALID_DEVICE_REQUEST;
                    }
                } else {
                    status = STATUS_BUFFER_TOO_SMALL;
                }
            } else {
                   status = STATUS_INVALID_DEVICE_REQUEST;
            }
            break;
        }

        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }
    }

    DebugPrint((3, "Disk: DiskExecuteMethod Device %p, Irp %p returns %lx\n",
             DeviceObject, Irp, status));

    status = ClassWmiCompleteRequest(DeviceObject,
                                     Irp,
                                     status,
                                     0,
                                     IO_NO_INCREMENT);

    return status;
}
#endif

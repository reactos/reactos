/*--

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    sense.c

Abstract:

    This file contains the methods needed to accurately
    determine how to retry requests on CDROM device types.

Environment:

    kernel mode only

Revision History:

--*/

#include "stddef.h"
#include "string.h"

#include "ntddk.h"
#include "ntddstor.h"
#include "cdrom.h"
#include "ntstrsafe.h"


#ifdef DEBUG_USE_WPP
#include "sense.tmh"
#endif

// Forward declarations
VOID
SenseInfoInterpretRefineByIoControl(
    _In_      PCDROM_DEVICE_EXTENSION   DeviceExtension,
    _In_      ULONG                     IoControlCode,
    _In_      BOOLEAN                   OverrideVerifyVolume,
    _Inout_   BOOLEAN*                  Retry,
    _Inout_   NTSTATUS*                 Status
    );


#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, SenseInfoInterpretRefineByIoControl)

#endif


//
// FROM CLASSPNP\CLASSP.H
//  Lots of retries of synchronized SCSI commands that devices may not
//  even support really slows down the system (especially while booting).
//  (Even GetDriveCapacity may be failed on purpose if an external disk is powered off).
//  If a disk cannot return a small initialization buffer at startup
//  in two attempts (with delay interval) then we cannot expect it to return
//  data consistently with four retries.
//  So don't set the retry counts as high here as for data SRBs.
//
//  If we find that these requests are failing consecutively,
//  despite the retry interval, on otherwise reliable media,
//  then we should either increase the retry interval for
//  that failure or (by all means) increase these retry counts as appropriate.
//

#define TOTAL_COUNT_RETRY_DEFAULT       4
#define TOTAL_COUNT_RETRY_LOCK_MEDIA    1
#define TOTAL_COUNT_RETRY_MODESENSE     1
#define TOTAL_COUNT_RETRY_READ_CAPACITY 1


#define TOTAL_SECONDS_RETRY_TIME_WRITE          160
#define TOTAL_SECONDS_RETRY_TIME_MEDIUM_REMOVAL 120

typedef struct _ERROR_LOG_CONTEXT {
    BOOLEAN     LogError;
    BOOLEAN     ErrorUnhandled;
    NTSTATUS    ErrorCode;
    ULONG       UniqueErrorValue;
    ULONG       BadSector;
} ERROR_LOG_CONTEXT, *PERROR_LOG_CONTEXT;

NTSTATUS
DeviceErrorHandlerForMmc(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK      Srb,
    _Inout_ PNTSTATUS             Status,
    _Inout_ PBOOLEAN              Retry
    )
/*++

Routine Description:

    this routine will be used for error handler for all MMC devices.
    it's invoked by DeviceErrorHandler()that invoked by SenseInfoInterpret() or GESN

    This routine just checks for media change sense/asc/ascq and
    also for other events, such as bus resets.  this is used to
    determine if the device behaviour has changed, to allow for
    read and write operations to be allowed and/or disallowed.

Arguments:

    DeviceExtension - device context
    Srb - SRB structure for analyze

Return Value:

    NTSTATUS
    Status - 
    Retry - 

--*/
{
    BOOLEAN mediaChange = FALSE;
    PCDB    cdb = (PCDB)Srb->Cdb;

    if (TEST_FLAG(Srb->SrbStatus, SRB_STATUS_AUTOSENSE_VALID)) 
    {
        PSENSE_DATA senseBuffer = Srb->SenseInfoBuffer;

        // the following sense keys could indicate a change in capabilities.

        // we used to expect this to be serialized, and only hit from our
        // own routine. we now allow some requests to continue during our
        // processing of the capabilities update in order to allow
        // IoReadPartitionTable() to succeed.
        switch (senseBuffer->SenseKey & 0xf) 
        {

        case SCSI_SENSE_NOT_READY: 
        {
            if (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_NO_MEDIA_IN_DEVICE) 
            {
                if (DeviceExtension->DeviceAdditionalData.Mmc.WriteAllowed) 
                {
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                               "DeviceErrorHandlerForMmc: media removed, writes will be "
                               "failed until new media detected\n"));
                }

                // NOTE - REF #0002
                DeviceExtension->DeviceAdditionalData.Mmc.WriteAllowed = FALSE;
            } 
            else if (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY) 
            {
                if (senseBuffer->AdditionalSenseCodeQualifier == SCSI_SENSEQ_BECOMING_READY) 
                {
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                               "DeviceErrorHandlerForMmc: media becoming ready, "
                               "SHOULD notify shell of change time by sending "
                               "GESN request immediately!\n"));
                } 
                else if (((senseBuffer->AdditionalSenseCodeQualifier == SCSI_SENSEQ_OPERATION_IN_PROGRESS) ||
                            (senseBuffer->AdditionalSenseCodeQualifier == SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS)
                            ) &&
                           ((Srb->Cdb[0] == SCSIOP_READ) ||
                            (Srb->Cdb[0] == SCSIOP_READ6) ||
                            (Srb->Cdb[0] == SCSIOP_READ_CAPACITY) ||
                            (Srb->Cdb[0] == SCSIOP_READ_CD) ||
                            (Srb->Cdb[0] == SCSIOP_READ_CD_MSF) ||
                            (Srb->Cdb[0] == SCSIOP_READ_TOC) ||
                            (Srb->Cdb[0] == SCSIOP_WRITE) ||
                            (Srb->Cdb[0] == SCSIOP_WRITE6) ||
                            (Srb->Cdb[0] == SCSIOP_READ_TRACK_INFORMATION) ||
                            (Srb->Cdb[0] == SCSIOP_READ_DISK_INFORMATION)
                            )
                           ) 
                {
                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,
                               "DeviceErrorHandlerForMmc: LONG_WRITE or "
                               "OP_IN_PROGRESS for limited subset of cmds -- "
                               "setting retry to TRUE\n"));
                    *Retry = TRUE;
                    *Status = STATUS_DEVICE_BUSY;
                }
            }
            break;
        } // end SCSI_SENSE_NOT_READY

        case SCSI_SENSE_UNIT_ATTENTION: 
        {
            switch (senseBuffer->AdditionalSenseCode) 
            {
            case SCSI_ADSENSE_MEDIUM_CHANGED: 
            {
                // always update if the medium may have changed

                // NOTE - REF #0002
                DeviceExtension->DeviceAdditionalData.Mmc.WriteAllowed = FALSE;
                DeviceExtension->DeviceAdditionalData.Mmc.UpdateState = CdromMmcUpdateRequired;

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                           "DeviceErrorHandlerForMmc: media change detected, need to "
                           "update drive capabilities\n"));
                mediaChange = TRUE;
                break;

            } // end SCSI_ADSENSE_MEDIUM_CHANGED

            case SCSI_ADSENSE_BUS_RESET:
            {
                // NOTE - REF #0002
                DeviceExtension->DeviceAdditionalData.Mmc.WriteAllowed = FALSE;
                DeviceExtension->DeviceAdditionalData.Mmc.UpdateState = CdromMmcUpdateRequired;

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                           "DeviceErrorHandlerForMmc: bus reset detected, need to "
                           "update drive capabilities\n"));
                break;

            } // end SCSI_ADSENSE_BUS_RESET

            case SCSI_ADSENSE_OPERATOR_REQUEST: 
            {

                BOOLEAN b = FALSE;

                switch (senseBuffer->AdditionalSenseCodeQualifier)
                {
                case SCSI_SENSEQ_MEDIUM_REMOVAL: 
                {
                    // eject notification currently handled by classpnp
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                               "DeviceErrorHandlerForMmc: Eject requested by user\n"));
                    *Retry = TRUE;
                    *Status = STATUS_DEVICE_BUSY;
                    break;
                }

                case SCSI_SENSEQ_WRITE_PROTECT_DISABLE:
                    b = TRUE;
                case SCSI_SENSEQ_WRITE_PROTECT_ENABLE: 
                {
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                               "DeviceErrorHandlerForMmc: Write protect %s requested "
                               "by user\n",
                               (b ? "disable" : "enable")));
                    *Retry = TRUE;
                    *Status = STATUS_DEVICE_BUSY;
                    // NOTE - REF #0002
                    DeviceExtension->DeviceAdditionalData.Mmc.WriteAllowed = FALSE;
                    DeviceExtension->DeviceAdditionalData.Mmc.UpdateState = CdromMmcUpdateRequired;

                    break;
                }

                } // end of AdditionalSenseCodeQualifier switch


                break;

            } // end SCSI_ADSENSE_OPERATOR_REQUEST

            default: 
            {
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,
                           "DeviceErrorHandlerForMmc: Unit attention %02x/%02x\n",
                           senseBuffer->AdditionalSenseCode,
                           senseBuffer->AdditionalSenseCodeQualifier));
                break;
            }

            } // end of AdditionSenseCode switch
            break;

        } // end SCSI_SENSE_UNIT_ATTENTION

        case SCSI_SENSE_ILLEGAL_REQUEST: 
        {
            if (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_WRITE_PROTECT) 
            {
                if (DeviceExtension->DeviceAdditionalData.Mmc.WriteAllowed)
                {
                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,
                               "DeviceErrorHandlerForMmc: media was writable, but "
                               "failed request with WRITE_PROTECT error...\n"));
                }
                // NOTE - REF #0002
                // do not update all the capabilities just because
                // we can't write to the disc.
                DeviceExtension->DeviceAdditionalData.Mmc.WriteAllowed = FALSE;
            }
            break;
        } // end SCSI_SENSE_ILLEGAL_REQUEST

        } // end of SenseKey switch

        // Check if we failed to set the DVD region key and send appropriate error
        if (cdb->CDB16.OperationCode == SCSIOP_SEND_KEY) 
        {
            if (cdb->SEND_KEY.KeyFormat == DvdSetRpcKey) 
            {
                if (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_NO_MEDIA_IN_DEVICE) 
                {
                    // media of appropriate region required
                    *Status = STATUS_NO_MEDIA_IN_DEVICE;
                    *Retry = FALSE;
                } 
                else if ((senseBuffer->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST) &&
                           (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_COPY_PROTECTION_FAILURE) &&
                           (senseBuffer->AdditionalSenseCodeQualifier == SCSI_SENSEQ_MEDIA_CODE_MISMATCHED_TO_LOGICAL_UNIT)) 
                {
                    // media of appropriate region required
                    *Status = STATUS_CSS_REGION_MISMATCH;
                    *Retry = FALSE;
                } 
                else if ((senseBuffer->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST) &&
                           (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_INVALID_MEDIA) &&
                           (senseBuffer->AdditionalSenseCodeQualifier == SCSI_SENSEQ_INCOMPATIBLE_FORMAT)) 
                {
                    // media of appropriate region required
                    *Status = STATUS_CSS_REGION_MISMATCH;
                    *Retry = FALSE;
                }
            }
        }
    } // end of SRB_STATUS_AUTOSENSE_VALID

    // On media change, if device speed should be reset to default then
    // queue a workitem to send the commands to the device. Do this on
    // media arrival as some device will fail this command if no media
    // is present. Ignore the fake media change from classpnp driver.
    if ((mediaChange == TRUE) && (*Status != STATUS_MEDIA_CHANGED)) 
    {
        if (DeviceExtension->DeviceAdditionalData.RestoreDefaults == TRUE) 
        {
            NTSTATUS                status = STATUS_SUCCESS;
            WDF_OBJECT_ATTRIBUTES   attributes;
            WDF_WORKITEM_CONFIG     workitemConfig;
            WDFWORKITEM             workItem;

            WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
            attributes.ParentObject = DeviceExtension->Device;

            WDF_WORKITEM_CONFIG_INIT(&workitemConfig,
                                     DeviceRestoreDefaultSpeed);

            status = WdfWorkItemCreate(&workitemConfig,
                                       &attributes,
                                       &workItem);
            if (!NT_SUCCESS(status))
            {
                return STATUS_SUCCESS;
            }

            WdfWorkItemEnqueue(workItem);

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, 
                        "DeviceErrorHandlerForMmc: Restore device default speed for %p\n",
                        DeviceExtension->DeviceObject));
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
DeviceErrorHandlerForHitachiGD2000(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK      Srb,
    _Inout_ PNTSTATUS             Status,
    _Inout_ PBOOLEAN              Retry
    )
/*++

Routine Description:

   error handler for HITACHI CDR-1750S, CDR-3650/1650S

   This routine checks the type of error.  If the error suggests that the
   drive has spun down and cannot reinitialize itself, send a
   START_UNIT or READ to the device.  This will force the drive to spin
   up.  This drive also loses the AGIDs it has granted when it spins down,
   which may result in playback failure the first time around.

Arguments:

    DeviceExtension - the device object.

    Srb - Supplies a pointer to the failing Srb.

    Status - return the final status for this command?

    Retry - return if the command should be retried.

Return Value:

    None.

--*/
{
    PSENSE_DATA senseBuffer = Srb->SenseInfoBuffer;

    if (!TEST_FLAG(Srb->SrbStatus, SRB_STATUS_AUTOSENSE_VALID)) 
    {
        return STATUS_SUCCESS; //nobody cares about this return value yet.
    }

    if (((senseBuffer->SenseKey & 0xf) == SCSI_SENSE_HARDWARE_ERROR) &&
        (senseBuffer->AdditionalSenseCode == 0x44)) 
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                    "DeviceErrorHandlerForHitachiGD2000 (%p) => Internal Target "
                    "Failure Detected -- spinning up drive\n", DeviceExtension->Device));

        // the request should be retried because the device isn't ready
        *Retry = TRUE;
        *Status = STATUS_DEVICE_NOT_READY;

        // send a START_STOP unit to spin up the drive
        // NOTE: this temporarily violates the StartIo serialization
        //       mechanism, but the completion routine on this will NOT
        //       call StartNextPacket(), so it's a temporary disruption
        //       of the serialization only.
        DeviceSendStartUnit(DeviceExtension->Device);
    }

    return STATUS_SUCCESS;
}


VOID
SenseInfoRequestGetInformation(
    _In_  WDFREQUEST  Request,
    _Out_ UCHAR*      MajorFunctionCode,
    _Out_ ULONG*      IoControlCode,
    _Out_ BOOLEAN*    OverrideVerifyVolume,
    _Out_ ULONGLONG*  Total100nsSinceFirstSend
    )
{
    PCDROM_REQUEST_CONTEXT requestContext = RequestGetContext(Request);

    *MajorFunctionCode = 0;
    *IoControlCode = 0;
    *OverrideVerifyVolume = FALSE;
    *Total100nsSinceFirstSend = 0;

    if (requestContext->OriginalRequest != NULL)
    {
        PIO_STACK_LOCATION originalIrpStack = NULL;
        
        PIRP originalIrp = WdfRequestWdmGetIrp(requestContext->OriginalRequest);

        if (originalIrp != NULL)
        {
            originalIrpStack = IoGetCurrentIrpStackLocation(originalIrp);
        }

        if (originalIrpStack != NULL)
        {
            *MajorFunctionCode = originalIrpStack->MajorFunction;

            if (*MajorFunctionCode == IRP_MJ_DEVICE_CONTROL)
            {
                *IoControlCode = originalIrpStack->Parameters.DeviceIoControl.IoControlCode;
            }

            *OverrideVerifyVolume = TEST_FLAG(originalIrpStack->Flags, SL_OVERRIDE_VERIFY_VOLUME);
        }
    }

    // Calculate time past since the request was first time sent.
    if (requestContext->TimeSentDownFirstTime.QuadPart > 0)
    {
        LARGE_INTEGER tmp;
        KeQueryTickCount(&tmp);
        tmp.QuadPart -= requestContext->TimeSentDownFirstTime.QuadPart;
        tmp.QuadPart *= KeQueryTimeIncrement();
        *Total100nsSinceFirstSend = tmp.QuadPart;
    }
    else
    {
        // set to -1 if field TimeSentDownFirstTime not set.
        *Total100nsSinceFirstSend = (ULONGLONG) -1;
    }

    return;
}

BOOLEAN
SenseInfoInterpretByAdditionalSenseCode(
    _In_      PCDROM_DEVICE_EXTENSION   DeviceExtension,
    _In_      PSCSI_REQUEST_BLOCK       Srb,
    _In_      UCHAR                     AdditionalSenseCode,
    _In_      UCHAR                     AdditionalSenseCodeQual,
    _Inout_   NTSTATUS*                 Status,
    _Inout_   BOOLEAN*                  Retry,
    _Out_ _Deref_out_range_(0,100) ULONG*         RetryIntervalInSeconds,  
    _Inout_   PERROR_LOG_CONTEXT        LogContext
    )
/*
    This function will interpret error based on ASC/ASCQ. 

    If the error code is not processed in this function, e.g. return value is TRUE, 
    caller needs to call SenseInfoInterpretBySenseKey() for further interpret.
*/
{
    BOOLEAN     needFurtherInterpret = TRUE;
    PSENSE_DATA senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

    // set default values for retry fields.
    *Status = STATUS_IO_DEVICE_ERROR;
    *Retry = TRUE;
    *RetryIntervalInSeconds = 0;

    switch (AdditionalSenseCode) 
    {
    case SCSI_ADSENSE_LUN_NOT_READY: 
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, 
                        "SenseInfoInterpretByAdditionalSenseCode: Lun not ready\n"));

            //
            //  Many non-WHQL certified drives (mostly CD-RW) return
            //  2/4/0 when they have no media instead of the obvious choice of:
            //
            //      SCSI_SENSE_NOT_READY/SCSI_ADSENSE_NO_MEDIA_IN_DEVICE
            //
            //  These drives should not pass WHQL certification due to this discrepency.
            //
            //  However, we have to retry on 2/4/0 (Not ready, LUN not ready, no info) 
            //  and also 3/2/0 (no seek complete).
            //
            //  These conditions occur when the shell tries to examine an
            //  injected CD (e.g. for autoplay) before the CD is spun up.
            //
            //  The drive should be returning an ASCQ of SCSI_SENSEQ_BECOMING_READY
            //  (0x01) in order to comply with WHQL standards.
            //
            //  The default retry timeout of one second is acceptable to balance
            //  these discrepencies.  don't modify the status, though....
            //
            
            switch (AdditionalSenseCodeQual) 
            {
            case SCSI_SENSEQ_OPERATION_IN_PROGRESS: 
                {
                    DEVICE_EVENT_BECOMING_READY notReady = {0};

                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, 
                                "SenseInfoInterpretByAdditionalSenseCode: Operation In Progress\n"));

                    needFurtherInterpret = FALSE;
            
                    *Retry = TRUE;
                    *RetryIntervalInSeconds = NOT_READY_RETRY_INTERVAL;
                    *Status = STATUS_DEVICE_NOT_READY;

                    notReady.Version = 1;
                    notReady.Reason = 2;
                    notReady.Estimated100msToReady = *RetryIntervalInSeconds * 10;
                    DeviceSendNotification(DeviceExtension,
                                           &GUID_IO_DEVICE_BECOMING_READY,
                                           sizeof(DEVICE_EVENT_BECOMING_READY),
                                           &notReady);

                    break;
                }

            case SCSI_SENSEQ_BECOMING_READY: 
                {
                    DEVICE_EVENT_BECOMING_READY notReady = {0};

                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                                "SenseInfoInterpretByAdditionalSenseCode: In process of becoming ready\n"));

                    needFurtherInterpret = FALSE;
            
                    *Retry = TRUE;
                    *RetryIntervalInSeconds = NOT_READY_RETRY_INTERVAL;
                    *Status = STATUS_DEVICE_NOT_READY;

                    notReady.Version = 1;
                    notReady.Reason = 1;
                    notReady.Estimated100msToReady = *RetryIntervalInSeconds * 10;
                    DeviceSendNotification(DeviceExtension,
                                           &GUID_IO_DEVICE_BECOMING_READY,
                                           sizeof(DEVICE_EVENT_BECOMING_READY),
                                           &notReady);

                    break;
                }

            case SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS: 
                {
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, 
                                "SenseInfoInterpretByAdditionalSenseCode: Long write in progress\n"));

                    needFurtherInterpret = FALSE;
            
                    // This has been seen as a transcient failure on some drives
                    *Status = STATUS_DEVICE_NOT_READY;
                    *Retry = TRUE;
                    // Set retry interval to be 0 as the drive can be ready at anytime.
                    *RetryIntervalInSeconds = 0;

                    break;
                }

            case SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED: 
                {
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, 
                                "SenseInfoInterpretByAdditionalSenseCode: Manual intervention required\n"));

                    needFurtherInterpret = FALSE;

                    *Status = STATUS_NO_MEDIA_IN_DEVICE;
                    *Retry = FALSE;
                    *RetryIntervalInSeconds = NOT_READY_RETRY_INTERVAL;

                    break;
                }

            case SCSI_SENSEQ_FORMAT_IN_PROGRESS: 
                {
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                                "SenseInfoInterpretByAdditionalSenseCode: Format in progress\n"));
            
                    needFurtherInterpret = FALSE;

                    *Status = STATUS_DEVICE_NOT_READY;
                    *Retry = FALSE;
                    *RetryIntervalInSeconds = NOT_READY_RETRY_INTERVAL;

                    break;
                }

            case SCSI_SENSEQ_CAUSE_NOT_REPORTABLE: 
            case SCSI_SENSEQ_INIT_COMMAND_REQUIRED:
            default:
                {
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, 
                                "SenseInfoInterpretByAdditionalSenseCode: Initializing command required\n"));

                    needFurtherInterpret = FALSE;

                    *Status = STATUS_DEVICE_NOT_READY;
                    *Retry = TRUE;
                    *RetryIntervalInSeconds = 0;

                    // This sense code/additional sense code combination may indicate 
                    // that the device needs to be started. 
                    if (TEST_FLAG(DeviceExtension->DeviceFlags, DEV_SAFE_START_UNIT) &&
                        !TEST_FLAG(Srb->SrbFlags, SRB_CLASS_FLAGS_LOW_PRIORITY))
                    {
                        DeviceSendStartUnit(DeviceExtension->Device);
                    }

                    break;
                }
            } // end switch (AdditionalSenseCodeQual)
            break;

        } // end case (SCSI_ADSENSE_LUN_NOT_READY)

    case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE: 
        {
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_GENERAL, 
                        "SenseInfoInterpretByAdditionalSenseCode: No Media in device.\n"));

            needFurtherInterpret = FALSE;

            *Status = STATUS_NO_MEDIA_IN_DEVICE;
            *Retry = FALSE;

            if (AdditionalSenseCodeQual == 0xCC)
            {
                //  The IMAPIv1 filter returns this ASCQ value while it is burning CD media, and we want
                //  to preserve this functionality for compatibility reasons.
                //  We want to indicate that the media is not present to most applications;
                //  but RSM has to know that the media is still in the drive (i.e. the drive is not free).
                DeviceSetMediaChangeStateEx(DeviceExtension, MediaUnavailable, NULL);
            }
            else 
            {
                DeviceSetMediaChangeStateEx(DeviceExtension, MediaNotPresent, NULL);
            }

            break;
        } // end case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE

    case SCSI_ADSENSE_INVALID_MEDIA: 
        {
        switch (AdditionalSenseCodeQual) 
        {

        case SCSI_SENSEQ_UNKNOWN_FORMAT: 
            {
                needFurtherInterpret = FALSE;
            
                // Log error only if this is a paging request
                *Status = STATUS_UNRECOGNIZED_MEDIA;
                *Retry = FALSE;

                LogContext->LogError = TEST_FLAG(Srb->SrbFlags, SRB_CLASS_FLAGS_PAGING);
                LogContext->UniqueErrorValue = 256;
                LogContext->ErrorCode = IO_ERR_BAD_BLOCK;

                break;
            }

        case SCSI_SENSEQ_INCOMPATIBLE_FORMAT: 
            {
                needFurtherInterpret = FALSE;
            
                *Status = STATUS_UNRECOGNIZED_MEDIA;
                *Retry = FALSE;

                LogContext->LogError = FALSE;

                break;
            }   

        case SCSI_SENSEQ_CLEANING_CARTRIDGE_INSTALLED:
            {
                needFurtherInterpret = FALSE;
            
                *Status = STATUS_CLEANER_CARTRIDGE_INSTALLED;
                *Retry = FALSE;

                LogContext->LogError = FALSE;
                LogContext->UniqueErrorValue = 256;
                LogContext->ErrorCode = IO_ERR_BAD_BLOCK;
                break;
            }

        default:
            {
                needFurtherInterpret = TRUE;
                break;
            }
        } // end case AdditionalSenseCodeQual

        break;
        } // end case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE

    case SCSI_ADSENSE_NO_SEEK_COMPLETE: 
        {
        switch (AdditionalSenseCodeQual) 
        {

        case 0x00: 
            {
                needFurtherInterpret = FALSE;
            
                *Status = STATUS_DEVICE_DATA_ERROR;
                *Retry = TRUE;
                *RetryIntervalInSeconds = 0;
                LogContext->LogError = TRUE;
                LogContext->UniqueErrorValue = 256;
                LogContext->ErrorCode = IO_ERR_BAD_BLOCK;
                break;
            }

        default:
            {
                needFurtherInterpret = TRUE;
                break;
            }
        }

        break;
        } // end case SCSI_ADSENSE_NO_SEEK_COMPLETE

    case SCSI_ADSENSE_LUN_COMMUNICATION: 
        {
        switch (AdditionalSenseCodeQual) 
        {

        case SCSI_SESNEQ_COMM_CRC_ERROR: 
            {
                needFurtherInterpret = FALSE;
            
                *Status = STATUS_IO_DEVICE_ERROR;
                *Retry = TRUE;
                *RetryIntervalInSeconds = 1;
                LogContext->LogError = TRUE;
                LogContext->UniqueErrorValue = 257;
                LogContext->ErrorCode = IO_ERR_CONTROLLER_ERROR;
                break;
            }

        default:
            {
                needFurtherInterpret = TRUE;
                break;
            }
        }

        break;
        } // end case SCSI_ADSENSE_LUN_COMMUNICATION

    case SCSI_ADSENSE_ILLEGAL_BLOCK: 
        {
            needFurtherInterpret = FALSE;

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, 
                        "SenseInfoInterpretByAdditionalSenseCode: Illegal block address\n"));
            *Status = STATUS_NONEXISTENT_SECTOR;
            *Retry = FALSE;
            break;
        }

    case SCSI_ADSENSE_INVALID_LUN: 
        {
            needFurtherInterpret = FALSE;

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,  
                        "SenseInfoInterpretByAdditionalSenseCode: Invalid LUN\n"));
            *Status = STATUS_NO_SUCH_DEVICE;
            *Retry = FALSE;
            break;
        }

    case SCSI_ADSENSE_COPY_PROTECTION_FAILURE: 
        {
            needFurtherInterpret = FALSE;

            *Retry = FALSE;

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, 
                        "SenseInfoInterpretByAdditionalSenseCode: Key - Copy protection failure\n"));

            switch (AdditionalSenseCodeQual) 
            {
            case SCSI_SENSEQ_AUTHENTICATION_FAILURE:
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                            "SenseInfoInterpretByAdditionalSenseCode: Authentication failure\n"));
                *Status = STATUS_CSS_AUTHENTICATION_FAILURE;
                break;
            case SCSI_SENSEQ_KEY_NOT_PRESENT:
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                            "SenseInfoInterpretByAdditionalSenseCode: Key not present\n"));
                *Status = STATUS_CSS_KEY_NOT_PRESENT;
                break;
            case SCSI_SENSEQ_KEY_NOT_ESTABLISHED:
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                            "SenseInfoInterpretByAdditionalSenseCode: Key not established\n"));
                *Status = STATUS_CSS_KEY_NOT_ESTABLISHED;
                break;
            case SCSI_SENSEQ_READ_OF_SCRAMBLED_SECTOR_WITHOUT_AUTHENTICATION:
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                            "SenseInfoInterpretByAdditionalSenseCode: Read of scrambled sector w/o authentication\n"));
                *Status = STATUS_CSS_SCRAMBLED_SECTOR;
                break;
            case SCSI_SENSEQ_MEDIA_CODE_MISMATCHED_TO_LOGICAL_UNIT:
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                            "SenseInfoInterpretByAdditionalSenseCode: Media region does not logical unit region\n"));
                *Status = STATUS_CSS_REGION_MISMATCH;
                break;
            case SCSI_SENSEQ_LOGICAL_UNIT_RESET_COUNT_ERROR:
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                            "SenseInfoInterpretByAdditionalSenseCode: Region set error -- region may be permanent\n"));
                *Status = STATUS_CSS_RESETS_EXHAUSTED;
                break;

            default:
                *Status = STATUS_COPY_PROTECTION_FAILURE;
                break;
            } // end switch of ASCQ for COPY_PROTECTION_FAILURE

            break;
        }

    case SCSI_ADSENSE_INVALID_CDB: 
        {
            needFurtherInterpret = FALSE;

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,  
                        "SenseInfoInterpretByAdditionalSenseCode: Key - Invalid CDB\n"));

            *Status = STATUS_INVALID_DEVICE_REQUEST;
            *Retry = FALSE;

            // Note: the retry interval is not typically used.
            // it is set here only because a ClassErrorHandler
            // cannot set the RetryIntervalInSeconds, and the error may
            // require a few commands to be sent to clear whatever
            // caused this condition (i.e. disk clears the write
            // cache, requiring at least two commands)
            //
            // hopefully, this shortcoming can be changed for blackcomb.
            *RetryIntervalInSeconds = 3;

            break;
        }

    case SCSI_ADSENSE_MEDIUM_CHANGED: 
        {
            needFurtherInterpret = FALSE;
            *RetryIntervalInSeconds = 0;

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_MCN,  
                        "SenseInfoInterpretByAdditionalSenseCode: Media changed\n"));

            DeviceSetMediaChangeStateEx(DeviceExtension, MediaPresent, NULL);

            // special process for Media Change
            if (IsVolumeMounted(DeviceExtension->DeviceObject))
            {
                // Set bit to indicate that media may have changed and volume needs verification.
                SET_FLAG(DeviceExtension->DeviceObject->Flags, DO_VERIFY_VOLUME);

                *Status = STATUS_VERIFY_REQUIRED;
                *Retry = FALSE;
            }
            else 
            {
                *Status = STATUS_IO_DEVICE_ERROR;
                *Retry = TRUE;
            }
            break;
        }

    case SCSI_ADSENSE_OPERATOR_REQUEST: 
        {
            switch (AdditionalSenseCodeQual) 
            {
            case SCSI_SENSEQ_MEDIUM_REMOVAL: 
                {
                    needFurtherInterpret = FALSE;
                    *RetryIntervalInSeconds = 0;
            
                    InterlockedIncrement((PLONG)&DeviceExtension->MediaChangeCount);
                    
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  
                        "SenseInfoInterpretByAdditionalSenseCode: Ejection request received!\n"));
                    //Send eject notification.
                    DeviceSendNotification(DeviceExtension,
                                           &GUID_IO_MEDIA_EJECT_REQUEST,
                                           0,
                                           NULL);
                    // special process for Media Change
                    if (IsVolumeMounted(DeviceExtension->DeviceObject))
                    {
                        // Set bit to indicate that media may have changed and volume needs verification.
                        SET_FLAG(DeviceExtension->DeviceObject->Flags, DO_VERIFY_VOLUME);

                        *Status = STATUS_VERIFY_REQUIRED;
                        *Retry = FALSE;
                    }
                    else 
                    {
                        *Status = STATUS_IO_DEVICE_ERROR;
                        *Retry = TRUE;
                    }
                    break;
                }
            default:
                {
                    needFurtherInterpret = TRUE;
                    break;
                }
            }
            break;
        }

    case SCSI_ADSENSE_OPERATING_CONDITIONS_CHANGED: 
        {
            needFurtherInterpret = FALSE;
            *RetryIntervalInSeconds = 5;

            InterlockedIncrement((PLONG)&DeviceExtension->MediaChangeCount);
                    
            // Device information has changed, we need to rescan the
            // bus for changed information such as the capacity.
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  
                        "SenseInfoInterpretByAdditionalSenseCode: Device information changed. Invalidate the bus\n"));

            IoInvalidateDeviceRelations(DeviceExtension->LowerPdo, BusRelations);

            // special process for Media Change
            if (IsVolumeMounted(DeviceExtension->DeviceObject))
            {
                // Set bit to indicate that media may have changed and volume needs verification.
                SET_FLAG(DeviceExtension->DeviceObject->Flags, DO_VERIFY_VOLUME);

                *Status = STATUS_VERIFY_REQUIRED;
                *Retry = FALSE;
            }
            else 
            {
                *Status = STATUS_IO_DEVICE_ERROR;
                *Retry = TRUE;
            }
            break;
        } //end Case SCSI_ADSENSE_OPERATING_CONDITIONS_CHANGED


    case SCSI_ADSENSE_REC_DATA_NOECC:
    case SCSI_ADSENSE_REC_DATA_ECC: 
        {
            needFurtherInterpret = FALSE;

            *Status = STATUS_SUCCESS;
            *Retry = FALSE;
            LogContext->LogError = TRUE;
            LogContext->UniqueErrorValue = 258;
            LogContext->ErrorCode = IO_RECOVERED_VIA_ECC;

            if (senseBuffer->IncorrectLength) 
            {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, 
                            "SenseInfoInterpretByAdditionalSenseCode: Incorrect length detected.\n"));
                *Status = STATUS_INVALID_BLOCK_LENGTH ;
            }
            break;
        }

    case SCSI_ADSENSE_FAILURE_PREDICTION_THRESHOLD_EXCEEDED: 
        {
            UCHAR wmiEventData[sizeof(ULONG)+sizeof(UCHAR)] = {0};

            *((PULONG)wmiEventData) = sizeof(UCHAR);
            wmiEventData[sizeof(ULONG)] = AdditionalSenseCodeQual;

            needFurtherInterpret = FALSE;

            // Don't log another eventlog if we have already logged once
            // NOTE: this should have been interlocked, but the structure
            //       was publicly defined to use a BOOLEAN (char).  Since
            //       media only reports these errors once per X minutes,
            //       the potential race condition is nearly non-existant.
            //       the worst case is duplicate log entries, so ignore.

            *Status = STATUS_SUCCESS;
            *Retry = FALSE;
            LogContext->UniqueErrorValue = 258;
            LogContext->LogError = TRUE;
            LogContext->ErrorCode = IO_WRN_FAILURE_PREDICTED;

            if (senseBuffer->IncorrectLength) 
            {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, 
                            "SenseInfoInterpretByAdditionalSenseCode: Incorrect length detected.\n"));
                *Status = STATUS_INVALID_BLOCK_LENGTH ;
            }
            break;
        }

    case 0x57: 
        {
            // UNABLE_TO_RECOVER_TABLE_OF_CONTENTS
            // the Matshita CR-585 returns this for all read commands
            // on blank CD-R and CD-RW media, and we need to handle
            // this for READ_CD detection ability.
            switch (AdditionalSenseCodeQual) 
            {
            case 0x00: 
                {
                    needFurtherInterpret = FALSE;

                    *Status = STATUS_UNRECOGNIZED_MEDIA;
                    *Retry = FALSE;
                    break;
                }
            default:
                {
                    needFurtherInterpret = TRUE;
                    break;
                }
            }
            break;
        }   //end case Matshita specific error 0x57

    default:
        {
            needFurtherInterpret = TRUE;
            break;
        }
    }

    return needFurtherInterpret;
}

VOID
SenseInfoInterpretBySenseKey(
    _In_      PCDROM_DEVICE_EXTENSION   DeviceExtension,
    _In_      PSENSE_DATA               SenseData,
    _In_      UCHAR                     SenseKey,
    _Inout_   NTSTATUS*                 Status,
    _Inout_   BOOLEAN*                  Retry,
    _Out_ _Deref_out_range_(0,100) ULONG*         RetryIntervalInSeconds,
    _Inout_   PERROR_LOG_CONTEXT        LogContext
    )
{
    // set default values for retry fields.
    *Status = STATUS_IO_DEVICE_ERROR;
    *Retry = TRUE;
    *RetryIntervalInSeconds = 0;

    switch (SenseKey)
    {
    case SCSI_SENSE_NOT_READY:
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                        "SenseInfoInterpretBySenseKey: Key - Not Ready (bad block)\n"));

            *Status = STATUS_DEVICE_NOT_READY;
            *Retry = TRUE;

            // for unprocessed "not ready" codes, retry the command immediately.
            *RetryIntervalInSeconds = 0;
            break;
        }

    case SCSI_SENSE_DATA_PROTECT:
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,
                        "SenseInfoInterpretBySenseKey: Key - Media write protected\n"));
            *Status = STATUS_MEDIA_WRITE_PROTECTED;
            *Retry = FALSE;
            break;
        } 

    case SCSI_SENSE_MEDIUM_ERROR:
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                        "SenseInfoInterpretBySenseKey: Key - Medium Error (bad block)\n"));

            *Status = STATUS_DEVICE_DATA_ERROR;
            *Retry = FALSE;
            LogContext->LogError = TRUE;
            LogContext->UniqueErrorValue = 256;
            LogContext->ErrorCode = IO_ERR_BAD_BLOCK;

            break;
        } // end SCSI_SENSE_MEDIUM_ERROR

    case SCSI_SENSE_HARDWARE_ERROR:
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                        "SenseInfoInterpretBySenseKey: Key - Hardware error\n"));

            *Status = STATUS_IO_DEVICE_ERROR;
            *Retry = TRUE;
            LogContext->LogError = TRUE;
            LogContext->UniqueErrorValue = 257;
            LogContext->ErrorCode = IO_ERR_CONTROLLER_ERROR;

            break;
        } // end SCSI_SENSE_HARDWARE_ERROR

    case SCSI_SENSE_ILLEGAL_REQUEST:
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                        "SenseInfoInterpretBySenseKey: Key - Illegal SCSI request\n"));
            *Status = STATUS_INVALID_DEVICE_REQUEST;
            *Retry = FALSE;

            break;
        } // end SCSI_SENSE_ILLEGAL_REQUEST

    case SCSI_SENSE_UNIT_ATTENTION:
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                        "SenseInfoInterpretBySenseKey: Key - Unit Attention\n"));

            // A media change may have occured so increment the change
            // count for the physical device
            InterlockedIncrement((PLONG)&DeviceExtension->MediaChangeCount);

            if (IsVolumeMounted(DeviceExtension->DeviceObject))
            {
                // Set bit to indicate that media may have changed
                // and volume needs verification.
                SET_FLAG(DeviceExtension->DeviceObject->Flags, DO_VERIFY_VOLUME);

                *Status = STATUS_VERIFY_REQUIRED;
                *Retry = FALSE;
            }
            else
            {
                *Status = STATUS_IO_DEVICE_ERROR;
                *Retry = TRUE;
            }

            break;

        } // end SCSI_SENSE_UNIT_ATTENTION

    case SCSI_SENSE_ABORTED_COMMAND:
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                        "SenseInfoInterpretBySenseKey: Command aborted\n"));
            *Status = STATUS_IO_DEVICE_ERROR;
            *Retry = TRUE;
            *RetryIntervalInSeconds = 1;
            break;
        } // end SCSI_SENSE_ABORTED_COMMAND

    case SCSI_SENSE_BLANK_CHECK:
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,
                        "SenseInfoInterpretBySenseKey: Media blank check\n"));
            *Retry = FALSE;
            *Status = STATUS_NO_DATA_DETECTED;
            break;
        } // end SCSI_SENSE_BLANK_CHECK

    case SCSI_SENSE_RECOVERED_ERROR:
        {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                        "SenseInfoInterpretBySenseKey: Recovered error\n"));
            *Status = STATUS_SUCCESS;
            *Retry = FALSE;
            LogContext->LogError = TRUE;
            LogContext->UniqueErrorValue = 258;
            LogContext->ErrorCode = IO_ERR_CONTROLLER_ERROR;

            if (SenseData->IncorrectLength)
            {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                            "SenseInfoInterpretBySenseKey: Incorrect length detected.\n"));
                *Status = STATUS_INVALID_BLOCK_LENGTH ;
            }

            break;
        } // end SCSI_SENSE_RECOVERED_ERROR

    case SCSI_SENSE_NO_SENSE:
        {
            // Check other indicators.
            if (SenseData->IncorrectLength)
            {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                            "SenseInfoInterpretBySenseKey: Incorrect length detected.\n"));
                *Status = STATUS_INVALID_BLOCK_LENGTH ;
                *Retry   = FALSE;
            }
            else
            {
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,
                            "SenseInfoInterpretBySenseKey: No specific sense key\n"));
                *Status = STATUS_IO_DEVICE_ERROR;
                *Retry = TRUE;
            }

            break;
        } // end SCSI_SENSE_NO_SENSE

    default:
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                        "SenseInfoInterpretBySenseKey: Unrecognized sense code\n"));
            *Status = STATUS_IO_DEVICE_ERROR;
            *Retry = TRUE;
            *RetryIntervalInSeconds = 0;

            break;
        }

    } // end switch (SenseKey)

    return;
}

VOID
SenseInfoInterpretBySrbStatus(
    _In_      PCDROM_DEVICE_EXTENSION   DeviceExtension,
    _In_      PSCSI_REQUEST_BLOCK       Srb,
    _In_ ULONG                          RetriedCount,
    _Inout_   NTSTATUS*                 Status,
    _Inout_   BOOLEAN*                  Retry,
    _Out_ _Deref_out_range_(0,100) ULONG*         RetryIntervalInSeconds,
    _Inout_   PERROR_LOG_CONTEXT        LogContext
    )
{
    BOOLEAN incrementErrorCount = FALSE;

    // set default values for retry fields.
    *Status = STATUS_IO_DEVICE_ERROR;
    *Retry = TRUE;
    *RetryIntervalInSeconds = 0;

    switch (SRB_STATUS(Srb->SrbStatus)) 
    {
    case SRB_STATUS_INVALID_LUN:
    case SRB_STATUS_INVALID_TARGET_ID:
    case SRB_STATUS_NO_DEVICE:
    case SRB_STATUS_NO_HBA:
    case SRB_STATUS_INVALID_PATH_ID: 
    {
        *Status = STATUS_NO_SUCH_DEVICE;
        *Retry = FALSE;
        break;
    }

    case SRB_STATUS_COMMAND_TIMEOUT:
    case SRB_STATUS_TIMEOUT: 
    {
        // Update the error count for the device.
        *Status = STATUS_IO_TIMEOUT;
        *Retry = TRUE;
        *RetryIntervalInSeconds = 0;
        incrementErrorCount = TRUE;
        break;
    }

    case SRB_STATUS_ABORTED:
    {
        // Update the error count for the device.
        *Status = STATUS_IO_TIMEOUT;
        *Retry = TRUE;
        *RetryIntervalInSeconds = 1;
        incrementErrorCount = TRUE;
        break;
    }

    case SRB_STATUS_SELECTION_TIMEOUT: 
    {
        *Status = STATUS_DEVICE_NOT_CONNECTED;
        *Retry = FALSE;
        *RetryIntervalInSeconds = 2;
        LogContext->LogError = TRUE;
        LogContext->ErrorCode = IO_ERR_NOT_READY;
        LogContext->UniqueErrorValue = 260;
        break;
    }

    case SRB_STATUS_DATA_OVERRUN:
    {
        *Status = STATUS_DATA_OVERRUN;
        *Retry = FALSE;
        break;
    }

    case SRB_STATUS_PHASE_SEQUENCE_FAILURE: 
    {
        // Update the error count for the device.
        incrementErrorCount = TRUE;
        *Status = STATUS_IO_DEVICE_ERROR;

        // If there was phase sequence error then limit the number of retries.
        *Retry = (RetriedCount <= 1);

        break;
    }

    case SRB_STATUS_REQUEST_FLUSHED: 
    {
        // If the status needs verification bit is set.  Then set
        // the status to need verification and no retry; otherwise,
        // just retry the request.
        if (TEST_FLAG(DeviceExtension->DeviceObject->Flags, DO_VERIFY_VOLUME)) 
        {
            *Status = STATUS_VERIFY_REQUIRED;
            *Retry = FALSE;
        } 
        else 
        {
            *Status = STATUS_IO_DEVICE_ERROR;
            *Retry = TRUE;
        }

        break;
    }

    case SRB_STATUS_INVALID_REQUEST: 
    {
        *Status = STATUS_INVALID_DEVICE_REQUEST;
        *Retry = FALSE;
        break;
    }

    case SRB_STATUS_UNEXPECTED_BUS_FREE:
    case SRB_STATUS_PARITY_ERROR:
        // Update the error count for the device and fall through to below
        incrementErrorCount = TRUE;

    case SRB_STATUS_BUS_RESET:
    {
        *Status = STATUS_IO_DEVICE_ERROR;
        *Retry = TRUE;
        break;
    }

    case SRB_STATUS_ERROR: 
    {
        *Status = STATUS_IO_DEVICE_ERROR;
        *Retry = TRUE;

        if (Srb->ScsiStatus == 0) 
        {
            // This is some strange return code.  Update the error
            // count for the device.
            incrementErrorCount = TRUE;
        } 

        if (Srb->ScsiStatus == SCSISTAT_BUSY) 
        {
            *Status = STATUS_DEVICE_NOT_READY;
        }

        break;
    }

    default: 
    {
        *Status = STATUS_IO_DEVICE_ERROR;
        *Retry = TRUE;
        LogContext->LogError = TRUE;
        LogContext->ErrorCode = IO_ERR_CONTROLLER_ERROR;
        LogContext->UniqueErrorValue = 259;
        LogContext->ErrorUnhandled = TRUE;
        break;
    }

    } //end of (SRB_STATUS(Srb->SrbStatus))  
    
    if (incrementErrorCount) 
    {
        // if any error count occurred, delay the retry of this io by
        // at least one second, if caller supports it.
        if (*RetryIntervalInSeconds == 0) 
        {
            *RetryIntervalInSeconds = 1;
        }

        DevicePerfIncrementErrorCount(DeviceExtension);
    }
 
    return;
}

VOID
SenseInfoLogError(
    _In_    PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_    PSCSI_REQUEST_BLOCK     Srb,
    _In_    UCHAR                   MajorFunctionCode,
    _In_    ULONG                   IoControlCode,
    _In_    ULONG                   RetriedCount,
    _In_    NTSTATUS*               Status,
    _In_    BOOLEAN*                Retry,
    _Inout_ PERROR_LOG_CONTEXT      LogContext
    )
{
    //      Always log the error in our internal log.
    //      If logError is set, also log the error in the system log.
    PSENSE_DATA          senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;
    ULONG                totalSize = 0;
    ULONG                senseBufferSize = 0;
    IO_ERROR_LOG_PACKET  staticErrLogEntry = {0};
    CDROM_ERROR_LOG_DATA staticErrLogData = {0};

    // Calculate the total size of the error log entry.
    // add to totalSize in the order that they are used.
    // the advantage to calculating all the sizes here is
    // that we don't have to do a bunch of extraneous checks
    // later on in this code path.
    totalSize = sizeof(IO_ERROR_LOG_PACKET)     // required
              + sizeof(CDROM_ERROR_LOG_DATA);   // struct for ease

    // also save any available extra sense data, up to the maximum errlog
    // packet size .  WMI should be used for real-time analysis.
    // the event log should only be used for post-mortem debugging.
    if (TEST_FLAG(Srb->SrbStatus, SRB_STATUS_AUTOSENSE_VALID))
    {
        ULONG       validSenseBytes;
        BOOLEAN     validSense;

        // make sure we can at least access the AdditionalSenseLength field
        validSense = RTL_CONTAINS_FIELD(senseBuffer,
                                        Srb->SenseInfoBufferLength,
                                        AdditionalSenseLength);
        if (validSense) 
        {
            // if extra info exists, copy the maximum amount of available
            // sense data that is safe into the the errlog.
            validSenseBytes = senseBuffer->AdditionalSenseLength
                                + offsetof(SENSE_DATA, AdditionalSenseLength);

            // this is invalid because it causes overflow!
            // whoever sent this type of request would cause
            // a system crash.
            NT_ASSERT(validSenseBytes < MAX_ADDITIONAL_SENSE_BYTES);

            // set to save the most sense buffer possible
            senseBufferSize = max(validSenseBytes, sizeof(SENSE_DATA));
            senseBufferSize = min(senseBufferSize, Srb->SenseInfoBufferLength);
        }
        else 
        {
            // it's smaller than required to read the total number of
            // valid bytes, so just use the SenseInfoBufferLength field.
            senseBufferSize = Srb->SenseInfoBufferLength;
        }

        //  Bump totalSize by the number of extra senseBuffer bytes
        //  (beyond the default sense buffer within CDROM_ERROR_LOG_DATA).
        //  Make sure to never allocate more than ERROR_LOG_MAXIMUM_SIZE.
        if (senseBufferSize > sizeof(SENSE_DATA))
        {
            totalSize += senseBufferSize-sizeof(SENSE_DATA);
            if (totalSize > ERROR_LOG_MAXIMUM_SIZE)
            {
                senseBufferSize -= totalSize-ERROR_LOG_MAXIMUM_SIZE;
                totalSize = ERROR_LOG_MAXIMUM_SIZE;
            }
        }
    }

    // If we've used up all of our retry attempts, set the final status to
    // reflect the appropriate result.
    //
    // ISSUE: the test below should also check RetriedCount to determine if we will actually retry,
    //            but there is no easy test because we'd have to consider the original retry count
    //            for the op; besides, InterpretTransferPacketError sometimes ignores the retry
    //            decision returned by this function.  So just ErrorRetried to be true in the majority case.
    //
    if (*Retry)
    {
        staticErrLogEntry.FinalStatus = STATUS_SUCCESS;
        staticErrLogData.ErrorRetried = TRUE;
    } 
    else 
    {
        staticErrLogEntry.FinalStatus = *Status;
    }

    // Don't log generic IO_WARNING_PAGING_FAILURE message if either the
    // I/O is retried, or it completed successfully.
    if ((LogContext->ErrorCode == IO_WARNING_PAGING_FAILURE) &&
        (*Retry || NT_SUCCESS(*Status)) ) 
    {
        LogContext->LogError = FALSE;
    }

    if (TEST_FLAG(Srb->SrbFlags, SRB_CLASS_FLAGS_PAGING)) 
    {
        staticErrLogData.ErrorPaging = TRUE;
    }

    staticErrLogData.ErrorUnhandled = LogContext->ErrorUnhandled;

    // Calculate the device offset if there is a geometry.
    staticErrLogEntry.DeviceOffset.QuadPart = (LONGLONG)LogContext->BadSector;
    staticErrLogEntry.DeviceOffset.QuadPart *= (LONGLONG)DeviceExtension->DiskGeometry.BytesPerSector;

    if (LogContext->ErrorCode == -1)
    {
        staticErrLogEntry.ErrorCode = STATUS_IO_DEVICE_ERROR;
    }
    else 
    {
        staticErrLogEntry.ErrorCode = LogContext->ErrorCode;
    }

    //  The dump data follows the IO_ERROR_LOG_PACKET
    staticErrLogEntry.DumpDataSize = (USHORT)totalSize - sizeof(IO_ERROR_LOG_PACKET);

    staticErrLogEntry.SequenceNumber = 0;
    staticErrLogEntry.MajorFunctionCode = MajorFunctionCode;
    staticErrLogEntry.IoControlCode = IoControlCode;
    staticErrLogEntry.RetryCount = (UCHAR)RetriedCount;
    staticErrLogEntry.UniqueErrorValue = LogContext->UniqueErrorValue;

    KeQueryTickCount(&staticErrLogData.TickCount);
    staticErrLogData.PortNumber = (ULONG)-1;

    //  Save the entire contents of the SRB.
    staticErrLogData.Srb = *Srb;

    //  For our private log, save just the default length of the SENSE_DATA.
    if (senseBufferSize != 0)
    {
        RtlCopyMemory(&staticErrLogData.SenseData, senseBuffer, min(senseBufferSize, sizeof(SENSE_DATA)));
    }

    // Save the error log in our context.
    // We only save the default sense buffer length.
    {
        KIRQL                oldIrql;
        KeAcquireSpinLock(&DeviceExtension->PrivateFdoData->SpinLock, &oldIrql);
        DeviceExtension->PrivateFdoData->ErrorLogs[DeviceExtension->PrivateFdoData->ErrorLogNextIndex] = staticErrLogData;
        DeviceExtension->PrivateFdoData->ErrorLogNextIndex++;
        DeviceExtension->PrivateFdoData->ErrorLogNextIndex %= NUM_ERROR_LOG_ENTRIES;
        KeReleaseSpinLock(&DeviceExtension->PrivateFdoData->SpinLock, oldIrql);
    }

    //  If logError is set, also save this log in the system's error log.
    //  But make sure we don't log TUR failures over and over
    //  (e.g. if an external drive was switched off and we're still sending TUR's to it every second).
    if (LogContext->LogError)
    {
        // We do not want to log certain system events repetitively
        switch (((PCDB)Srb->Cdb)->CDB10.OperationCode)
        {
            case SCSIOP_TEST_UNIT_READY:
            {
                if (DeviceExtension->PrivateFdoData->LoggedTURFailureSinceLastIO)
                {
                    LogContext->LogError = FALSE;
                }
                else
                {
                    DeviceExtension->PrivateFdoData->LoggedTURFailureSinceLastIO = TRUE;
                }

                break;
            }

            case SCSIOP_SYNCHRONIZE_CACHE:
            {
                if (DeviceExtension->PrivateFdoData->LoggedSYNCFailure)
                {
                    LogContext->LogError = FALSE;
                }
                else
                {
                    DeviceExtension->PrivateFdoData->LoggedSYNCFailure = TRUE;
                }

                break;
            }
        }

        // Do not log 5/21/00 LOGICAL BLOCK ADDRESS OUT OF RANGE if the disc is blank,
        // it is known to litter the Event Log with repetitive errors
        // Do not log this error for READ, as it's known that File System mount process reads different sectors from media.
        if (senseBufferSize > RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseCodeQualifier) &&
            senseBuffer->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST &&
            senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_ILLEGAL_BLOCK &&
            senseBuffer->AdditionalSenseCodeQualifier == 0 &&
            IS_SCSIOP_READ(((PCDB)Srb->Cdb)->CDB10.OperationCode))
        {
            LogContext->LogError = FALSE;
        }
    }

    //  Write the error log packet to the system error logging thread.
    if (LogContext->LogError)
    {
        PIO_ERROR_LOG_PACKET    errorLogEntry;
        PCDROM_ERROR_LOG_DATA   errlogData;

        errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(DeviceExtension->DeviceObject, (UCHAR)totalSize);
        if (errorLogEntry)
        {
            errlogData = (PCDROM_ERROR_LOG_DATA)errorLogEntry->DumpData;

            *errorLogEntry = staticErrLogEntry;
            *errlogData = staticErrLogData;

            //  For the system log, copy as much of the sense buffer as possible.
            if (senseBufferSize != 0) 
            {
                RtlCopyMemory(&errlogData->SenseData, senseBuffer, senseBufferSize);
            }

            //  errorLogEntry - It will be freed by the kernel.
            IoWriteErrorLogEntry(errorLogEntry);
        }
    }

    return;
}

VOID
SenseInfoInterpretRefineByScsiCommand(
    _In_      PCDROM_DEVICE_EXTENSION   DeviceExtension,
    _In_      PSCSI_REQUEST_BLOCK       Srb,
    _In_      ULONG                     RetriedCount,
    _In_      LONGLONG                  Total100nsSinceFirstSend,
    _In_      BOOLEAN                   OverrideVerifyVolume,
    _Inout_   BOOLEAN*                  Retry,
    _Inout_   NTSTATUS*                 Status,
    _Inout_   _Deref_out_range_(0, MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
              LONGLONG*                 RetryIntervalIn100ns
    )
/*++

Routine Description:

    Based on SCSI command, modify the interpretion result.

Arguments:

    DeviceExtension - device extension.
    Srb - Supplies the scsi request block which failed.
    RetriedCount - retried count.
    Total100nsUnitsSinceFirstSend - time spent after the request was sent down first time.
    OverrideVerifyVolume - should override verify volume request.

Return Value:

    Retry - the reques should be retried or not.
    Status - Returns the status for the request.
    RetryInterval - waiting time (in 100ns) before the request should be retried.
                    Zero indicates the request should be immediately retried.

--*/
{
    UCHAR const opCode = Srb->Cdb[0];
    CDB const*  cdb = (CDB const*)(Srb->Cdb);
    PSENSE_DATA senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

    if (opCode == SCSIOP_MEDIUM_REMOVAL)
    {
        if (( cdb->AsByte[1]         == 0) &&
            ( cdb->AsByte[2]         == 0) &&
            ( cdb->AsByte[3]         == 0) &&
            ((cdb->AsByte[4] & 0xFC) == 0)
            )
        {
            // byte[4] == 0x3 or byte[4] == 0x1  ==  UNLOCK OF MEDIA
            if ((cdb->AsByte[4] & 0x01) == 0)
            {
                if (RetriedCount < TOTAL_COUNT_RETRY_DEFAULT)
                {
                    // keep retrying unlock operation for several times
                    *Retry = TRUE;
                }
            }
            else // LOCK REQUEST
            {
                // do not retry LOCK requests more than once (per CLASSPNP code)
                if (RetriedCount > TOTAL_COUNT_RETRY_LOCK_MEDIA)
                {
                    *Retry = FALSE;
                }
            }
        }

        // want a minimum time to retry of 2 seconds
        if ((*Status == STATUS_DEVICE_NOT_READY) &&
            (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY))
        {
            *RetryIntervalIn100ns = max(*RetryIntervalIn100ns, SECONDS_TO_100NS_UNITS(2));
        }
        else if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SELECTION_TIMEOUT)
        {
            *RetryIntervalIn100ns = max(*RetryIntervalIn100ns, SECONDS_TO_100NS_UNITS(2));
        }
    }
    else if ((opCode == SCSIOP_MODE_SENSE) || (opCode == SCSIOP_MODE_SENSE10))
    {
        // want a minimum time to retry of 2 seconds
        if ((*Status == STATUS_DEVICE_NOT_READY) &&
            (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY))
        {
            *RetryIntervalIn100ns = max(*RetryIntervalIn100ns, SECONDS_TO_100NS_UNITS(2));
        }
        else if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SELECTION_TIMEOUT)
        {
            *RetryIntervalIn100ns = max(*RetryIntervalIn100ns, SECONDS_TO_100NS_UNITS(2));
        }

        // Want to ignore a STATUS_VERIFY_REQUIRED error because it either
        // doesn't make sense or is required to satisfy the VERIFY.
        if (*Status == STATUS_VERIFY_REQUIRED)
        {
            *Retry = TRUE;
        }
        else if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN)
        {
            /*
             *  This is a HACK.
             *  Atapi returns SRB_STATUS_DATA_OVERRUN when it really means
             *  underrun (i.e. success, and the buffer is longer than needed).
             *  So treat this as a success.
             *  When the caller of this function sees that the status was changed to success,
             *  it will add the transferred length to the original irp.
             */
            *Status = STATUS_SUCCESS;
            *Retry = FALSE;
        }

        // limit the count of retries
        if (RetriedCount > TOTAL_COUNT_RETRY_MODESENSE)
        {
            *Retry = FALSE;
        }
    }
    else if ((opCode == SCSIOP_READ_CAPACITY) || (opCode == SCSIOP_READ_CAPACITY16))
    {
        // Want to ignore a STATUS_VERIFY_REQUIRED error because it either
        // doesn't make sense or is required to satisfy the VERIFY.
        if (*Status == STATUS_VERIFY_REQUIRED)
        {
            *Retry = TRUE;
        }

        if (RetriedCount > TOTAL_COUNT_RETRY_READ_CAPACITY)
        {
            *Retry = FALSE;
        }
    }
    else if ((opCode == SCSIOP_RESERVE_UNIT) || (opCode == SCSIOP_RELEASE_UNIT))
    {
        // The RESERVE(6) / RELEASE(6) commands are optional. 
        // So if they aren't supported, try the 10-byte equivalents
        if (*Status == STATUS_INVALID_DEVICE_REQUEST)
        {
            PCDB tempCdb = (PCDB)Srb->Cdb;

            Srb->CdbLength = 10;
            tempCdb->CDB10.OperationCode = (tempCdb->CDB6GENERIC.OperationCode == SCSIOP_RESERVE_UNIT) 
                                            ? SCSIOP_RESERVE_UNIT10 
                                            : SCSIOP_RELEASE_UNIT10;

            SET_FLAG(DeviceExtension->PrivateFdoData->HackFlags, FDO_HACK_NO_RESERVE6);
            *Retry = TRUE;
        }
    }
    else if (IS_SCSIOP_READWRITE(opCode))
    {
        // Retry if still verifying a (possibly) reloaded disk/cdrom.
        if (OverrideVerifyVolume && (*Status == STATUS_VERIFY_REQUIRED))
        {
            *Status = STATUS_IO_DEVICE_ERROR;
            *Retry = TRUE;
        }

        // Special case for streaming READ/WRITE commands
        if (((opCode == SCSIOP_READ12) && (cdb->READ12.Streaming == 1)) ||
            ((opCode == SCSIOP_WRITE12) && (cdb->WRITE12.Streaming == 1)))
        {
            // We've got a failure while performing a streaming operation and now need to guess if
            // it's likely to be a permanent error because the drive does not support streaming at all
            // (in which case we're going to fall back to normal reads/writes), or a transient error
            // (in which case we quickly fail the request but contrinue to use streaming).
            //
            // We analyze the sense information to make that decision. Bus resets and device timeouts
            // are treated as permanent errors, because some non-compliant devices may even hang when
            // they get a command that they do not expect.

            BOOLEAN     disableStreaming = FALSE;

            if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_TIMEOUT || 
                SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_COMMAND_TIMEOUT ||
                SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SELECTION_TIMEOUT ||
                SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_BUS_RESET)
            {
                disableStreaming = TRUE;
            }
            else if ((senseBuffer->SenseKey &0xf) == SCSI_SENSE_UNIT_ATTENTION)
            {
                if (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_BUS_RESET ||
                    senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_INSUFFICIENT_TIME_FOR_OPERATION)
                {
                    disableStreaming = TRUE;
                }
            }
            else if ((senseBuffer->SenseKey &0xf) == SCSI_SENSE_ILLEGAL_REQUEST)
            {
                // LBA Out of Range is an exception, as it's more likely to be caused by 
                // upper layers attempting to read/write a wrong LBA.
                if (senseBuffer->AdditionalSenseCode != SCSI_ADSENSE_ILLEGAL_BLOCK)
                {
                    disableStreaming = TRUE;
                }
            }

            if (disableStreaming)
            {
                // if the failure looks permanent, we disable streaming for all future reads/writes
                // and retry the command immediately
                SET_FLAG(DeviceExtension->PrivateFdoData->HackFlags, FDO_HACK_NO_STREAMING);
                *Retry = TRUE;
                *RetryIntervalIn100ns = 0;
            }
            else
            {
                // if the failure looks transient, we simply fail the current request without retries
                // to minimize the time of processing
                *Retry = FALSE;
            }
        }

        // Special-case handling of READ/WRITE commands.  These commands now have a 120 second timeout, 
        // but the preferred behavior (and that taken by many drives) is to immediately report 2/4/x 
        // on OPC and similar commands.  Thus, retries must occur for at least 160 seconds 
        // (120 seconds + four 10 second retries) as a conservative guess.
        // Note: 160s retry time is also a result of discussion with OEMs for case of 2/4/7 and 2/4/8. 
        if (*Retry)
        {
            if ((Total100nsSinceFirstSend < 0) ||
                (((senseBuffer->SenseKey &0xf) == SCSI_SENSE_HARDWARE_ERROR) && (senseBuffer->AdditionalSenseCode == 0x09)))
            {
                // time information is not valid. use default retry count.
                // or if it's SERVO FAILURE, use retry count instead of 160s retry.
                *Retry = (RetriedCount <= TOTAL_COUNT_RETRY_DEFAULT);
            }
            else if (Total100nsSinceFirstSend > SECONDS_TO_100NS_UNITS(TOTAL_SECONDS_RETRY_TIME_WRITE))
            {
                *Retry = FALSE;
            }

            // How long should we request a delay for during writing?  This depends entirely on 
            // the current write speed of the drive.  If we request retries too quickly,
            // we can overload the processor on the drive (resulting in garbage being written),
            // but too slowly results in lesser performance.
            //
            *RetryIntervalIn100ns = DeviceExtension->DeviceAdditionalData.ReadWriteRetryDelay100nsUnits;

        } // end retry for 160 seconds modification
    }
    else if (opCode == SCSIOP_GET_PERFORMANCE)
    {
        if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN)
        {
            //  This is a HACK.
            //  Atapi returns SRB_STATUS_DATA_OVERRUN when it really means
            //  underrun (i.e. success, and the buffer is longer than needed).
            //  So treat this as a success.
            //  When the caller of this function sees that the status was changed to success,
            //  it will add the transferred length to the original irp.
            *Status = STATUS_SUCCESS;
            *Retry = FALSE;
        }

        if ((Srb->SenseInfoBufferLength < RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA,AdditionalSenseCodeQualifier)) ||
            !TEST_FLAG(Srb->SrbStatus, SRB_STATUS_AUTOSENSE_VALID)) 
        {
            // If get configuration command is failing and if the request type is TYPE ONE
            // then most likely the device does not support this request type. Set the
            // flag so that the TYPE ONE requests will be tried as TYPE ALL requets.
            if ((SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS) &&
                (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_DATA_OVERRUN) &&
                (((PCDB)Srb->Cdb)->GET_CONFIGURATION.RequestType == SCSI_GET_CONFIGURATION_REQUEST_TYPE_ONE)) 
            {

                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,
                           "TYPE ONE GetConfiguration failed. Set hack flag and retry.\n"));
                SET_FLAG(DeviceExtension->DeviceAdditionalData.HackFlags, CDROM_HACK_BAD_TYPE_ONE_GET_CONFIG);
                *Retry = TRUE;
            }
        }

        // limit retries to GET_PERFORMANCE commands to default retry count
        if (RetriedCount > TOTAL_COUNT_RETRY_DEFAULT)
        {
            *Retry = FALSE;
        }
    }
    else // default handler -- checks for retry count only.
    {
        if (RetriedCount > TOTAL_COUNT_RETRY_DEFAULT)
        {
            *Retry = FALSE;
        }
    }

    return;
}


VOID
SenseInfoInterpretRefineByIoControl(
    _In_      PCDROM_DEVICE_EXTENSION   DeviceExtension,
    _In_      ULONG                     IoControlCode,
    _In_      BOOLEAN                   OverrideVerifyVolume,
    _Inout_   BOOLEAN*                  Retry,
    _Inout_   NTSTATUS*                 Status
    )
/*++

Routine Description:

    Based on IOCTL code, modify the interpretion result.

Arguments:

    Device - Supplies the device object associated with this request.
    OriginalRequest - the irp that error occurs on.
    Srb - Supplies the scsi request block which failed.
    MajorFunctionCode - Supplies the function code to be used for logging.
    IoDeviceCode - Supplies the device code to be used for logging.
    PreviousRetryCount - retried count.
    RequestHistory_DoNotUse - the history list

Return Value:

    BOOLEAN TRUE: Drivers should retry this request.
            FALSE: Drivers should not retry this request.
    Status - Returns the status for the request.
    RetryInterval - Number of seconds before the request should be retried.
                    Zero indicates the request should be immediately retried.

--*/
{
    PAGED_CODE();

    if ((IoControlCode == IOCTL_CDROM_GET_LAST_SESSION) ||
        (IoControlCode == IOCTL_CDROM_READ_TOC)         ||
        (IoControlCode == IOCTL_CDROM_READ_TOC_EX)      ||
        (IoControlCode == IOCTL_CDROM_GET_CONFIGURATION)||
        (IoControlCode == IOCTL_CDROM_GET_VOLUME)) 
    {
        if (*Status == STATUS_DATA_OVERRUN) 
        {
            *Status = STATUS_SUCCESS;
            *Retry = FALSE;
        }
    }

    if (IoControlCode == IOCTL_CDROM_READ_Q_CHANNEL) 
    {
        PLAY_ACTIVE(DeviceExtension) = FALSE;
    }

    // If the status is verified required and the this request
    // should bypass verify required then retry the request.
    if (OverrideVerifyVolume && (*Status == STATUS_VERIFY_REQUIRED)) 
    {
        // note: status gets overwritten here
        *Status = STATUS_IO_DEVICE_ERROR;
        *Retry = TRUE;

        if ((IoControlCode == IOCTL_CDROM_CHECK_VERIFY) ||
            (IoControlCode == IOCTL_STORAGE_CHECK_VERIFY) ||
            (IoControlCode == IOCTL_STORAGE_CHECK_VERIFY2) ||
            (IoControlCode == IOCTL_DISK_CHECK_VERIFY)
           ) 
        {
            // Update the geometry information, as the media could have changed.
            (VOID) MediaReadCapacity(DeviceExtension->Device);
        } // end of ioctls to update capacity
    }

    if (!NT_SUCCESS(*Status) && (IoControlCode == IOCTL_CDROM_SET_SPEED))
    {
        // If set speed request fails then we should disable the restore speed option.
        // Otherwise we will try to restore to default speed on next media change,
        // if requested by the caller.
        DeviceExtension->DeviceAdditionalData.RestoreDefaults = FALSE;
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "Disable restore default\n"));
    }

    return;
}

BOOLEAN
SenseInfoInterpret(
    _In_    PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_    WDFREQUEST              Request, 
    _In_    PSCSI_REQUEST_BLOCK     Srb,
    _In_    ULONG                   RetriedCount,
    _Out_   NTSTATUS*               Status,
    _Out_ _Deref_out_range_(0, MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
              LONGLONG*             RetryIntervalIn100ns
    )
/*++

SenseInfoInterpret()

Routine Description:

    This routine interprets the data returned from the SCSI request sense. 
    It determines the status to return in the IRP 
    and whether this request can be retried.

Arguments:

    Device - Supplies the device object associated with this request.
    Srb - Supplies the scsi request block which failed.
    MajorFunctionCode - Supplies the function code to be used for logging.
    IoDeviceCode - Supplies the device code to be used for logging.

Return Value:

    BOOLEAN TRUE: Drivers should retry this request.
            FALSE: Drivers should not retry this request.
    Status - Returns the status for the request.
    RetryInterval - Number of seconds before the request should be retried.
                    Zero indicates the request should be immediately retried.

--*/
{
    ULONG               retryIntervalInSeconds = 0;
    BOOLEAN             retry = TRUE;
    PSENSE_DATA         senseBuffer = Srb->SenseInfoBuffer;
    ULONG               readSector = 0;
    ERROR_LOG_CONTEXT   logContext;

    UCHAR                   majorFunctionCode = 0;
    ULONG                   ioControlCode = 0;
    BOOLEAN                 overrideVerifyVolume = FALSE;
    ULONGLONG               total100nsSinceFirstSend = 0;
    PZERO_POWER_ODD_INFO    zpoddInfo = DeviceExtension->ZeroPowerODDInfo;

    //
    *Status = STATUS_IO_DEVICE_ERROR;

    RtlZeroMemory(&logContext, sizeof(ERROR_LOG_CONTEXT));
    logContext.ErrorCode = -1;

    // Get Original Request related information
    SenseInfoRequestGetInformation(Request,
                                   &majorFunctionCode,
                                   &ioControlCode,
                                   &overrideVerifyVolume,
                                   &total100nsSinceFirstSend);

    if(TEST_FLAG(Srb->SrbFlags, SRB_CLASS_FLAGS_PAGING)) 
    {
        // Log anything remotely incorrect about paging i/o
        logContext.LogError = TRUE;
        logContext.UniqueErrorValue = 301;
        logContext.ErrorCode = IO_WARNING_PAGING_FAILURE;
    }

    // must handle the SRB_STATUS_INTERNAL_ERROR case first,
    // as it has all the flags set.
    if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_INTERNAL_ERROR)
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                    "SenseInfoInterpret: Internal Error code is %x\n",
                    Srb->InternalStatus));

        retry = FALSE;
        *Status = Srb->InternalStatus;
    } 
    else if (Srb->ScsiStatus == SCSISTAT_RESERVATION_CONFLICT) 
    {
        retry = FALSE;
        *Status = STATUS_DEVICE_BUSY;
        logContext.LogError = FALSE;
    } 
    else if ((Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
             (Srb->SenseInfoBufferLength >= RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength))) 
    {
        UCHAR   senseKey = (UCHAR)(senseBuffer->SenseKey & 0x0f);
        UCHAR   additionalSenseCode = 0;
        UCHAR   additionalSenseCodeQual = 0;

        // Zero the additional sense code and additional sense code qualifier
        // if they were not returned by the device.
        readSector = senseBuffer->AdditionalSenseLength + offsetof(SENSE_DATA, AdditionalSenseLength);
        if (readSector > Srb->SenseInfoBufferLength) 
        {
            readSector = Srb->SenseInfoBufferLength;
        }

        additionalSenseCode = (readSector >= RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseCode)) ?
                               senseBuffer->AdditionalSenseCode : 0;
        additionalSenseCodeQual = (readSector >= RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseCodeQualifier)) ?
                                   senseBuffer->AdditionalSenseCodeQualifier : 0;

        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, 
                    "SCSI Error - \n"
                    "\tcdb: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n"
                    "\tsrb status: %X; sense: %02X/%02X/%02X; Retried count: %d\n\n",
                    Srb->Cdb[0], Srb->Cdb[1], Srb->Cdb[2], Srb->Cdb[3], Srb->Cdb[4], Srb->Cdb[5], 
                    Srb->Cdb[6], Srb->Cdb[7], Srb->Cdb[8], Srb->Cdb[9], Srb->Cdb[10], Srb->Cdb[11], 
                    Srb->Cdb[12], Srb->Cdb[13], Srb->Cdb[14], Srb->Cdb[15], 
                    SRB_STATUS(Srb->SrbStatus), 
                    senseKey, 
                    additionalSenseCode, 
                    additionalSenseCodeQual,
                    RetriedCount));

        if (senseKey == SCSI_SENSE_UNIT_ATTENTION)
        {
            ULONG   mediaChangeCount;

            // A media change may have occured so increment the change count for the physical device
            mediaChangeCount = InterlockedIncrement((PLONG)&DeviceExtension->MediaChangeCount);
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_MCN,  
                       "SenseInfoInterpret: Media change count for device %d incremented to %#lx\n",
                       DeviceExtension->DeviceNumber, mediaChangeCount));
        }

        if ((zpoddInfo != NULL) &&
            (((PCDB)Srb->Cdb)->CDB6GENERIC.OperationCode == SCSIOP_TEST_UNIT_READY))
        {
            // This sense code is in response to the Test Unit Ready sent during delayed power down
            // request. Copy the sense data into the zpoddInfo structure for later processing.
            zpoddInfo->SenseKey = senseKey;
            zpoddInfo->AdditionalSenseCode = additionalSenseCode;
            zpoddInfo->AdditionalSenseCodeQualifier = additionalSenseCodeQual;
        }

        // Interpret error by specific ASC & ASCQ first,
        // If the error is not handled, interpret using f
        {
            BOOLEAN notHandled = FALSE;
            notHandled = SenseInfoInterpretByAdditionalSenseCode(DeviceExtension,
                                                                 Srb,
                                                                 additionalSenseCode, 
                                                                 additionalSenseCodeQual,
                                                                 Status,
                                                                 &retry,
                                                                 &retryIntervalInSeconds,
                                                                 &logContext);
            
            if (notHandled)
            {
                SenseInfoInterpretBySenseKey(DeviceExtension,
                                             senseBuffer,
                                             senseKey,
                                             Status,
                                             &retry,
                                             &retryIntervalInSeconds,
                                             &logContext);
            }
        }

        // Try to determine the bad sector from the inquiry data.
        if ((IS_SCSIOP_READWRITE(((PCDB)Srb->Cdb)->CDB10.OperationCode)) ||
            (((PCDB)Srb->Cdb)->CDB10.OperationCode == SCSIOP_VERIFY)     ||
            (((PCDB)Srb->Cdb)->CDB10.OperationCode == SCSIOP_VERIFY16)) 
        {
            ULONG index;
            readSector = 0;

            for (index = 0; index < 4; index++) 
            {
                logContext.BadSector = (logContext.BadSector << 8) | senseBuffer->Information[index];
            }

            for (index = 0; index < 4; index++) 
            {
                readSector = (readSector << 8) | Srb->Cdb[index+2];
            }

            index = (((PCDB)Srb->Cdb)->CDB10.TransferBlocksMsb << 8) |
                    ((PCDB)Srb->Cdb)->CDB10.TransferBlocksLsb;

            // Make sure the bad sector is within the read sectors.
            if (!(logContext.BadSector >= readSector && logContext.BadSector < (readSector + index)))
            {
                logContext.BadSector = readSector;
            }
        }
    } 
    else 
    {
        // Request sense buffer not valid. No sense information
        // to pinpoint the error. Return general request fail.
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,  
                    "SCSI Error - \n"
                    "\tcdb: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n"
                    "\tsrb status: %X; sense info not valid; Retried count: %d\n\n",
                    Srb->Cdb[0], Srb->Cdb[1], Srb->Cdb[2], Srb->Cdb[3], Srb->Cdb[4], Srb->Cdb[5], 
                    Srb->Cdb[6], Srb->Cdb[7], Srb->Cdb[8], Srb->Cdb[9], Srb->Cdb[10], Srb->Cdb[11], 
                    Srb->Cdb[12], Srb->Cdb[13], Srb->Cdb[14], Srb->Cdb[15], 
                    SRB_STATUS(Srb->SrbStatus),
                    RetriedCount));

        SenseInfoInterpretBySrbStatus(DeviceExtension,
                                      Srb, 
                                      RetriedCount,
                                      Status,
                                      &retry,
                                      &retryIntervalInSeconds,
                                      &logContext);
    }

    // all functions using unit - seconds for retry Interval already be called.
    *RetryIntervalIn100ns = SECONDS_TO_100NS_UNITS(retryIntervalInSeconds);

    // call the device specific error handler if it has one.
    // DeviceErrorHandlerForMmmc() for all MMC devices
    // or DeviceErrorHandlerForHitachiGD2000() for HITACHI GD-2000, HITACHI DVD-ROM GD-2000
    if (DeviceExtension->DeviceAdditionalData.ErrorHandler) 
    {
        DeviceExtension->DeviceAdditionalData.ErrorHandler(DeviceExtension, Srb, Status, &retry);
    }

    // Refine retry based on SCSI command
    SenseInfoInterpretRefineByScsiCommand(DeviceExtension,
                                          Srb,
                                          RetriedCount,
                                          total100nsSinceFirstSend,
                                          overrideVerifyVolume,
                                          &retry,
                                          Status,
                                          RetryIntervalIn100ns);

    // Refine retry based on IOCTL code. 
    if (majorFunctionCode == IRP_MJ_DEVICE_CONTROL)
    {
        SenseInfoInterpretRefineByIoControl(DeviceExtension,
                                            ioControlCode, 
                                            overrideVerifyVolume,
                                            &retry,
                                            Status);
    }
    
    // LOG the error:
    //  Always log the error in our internal log.
    //  If logError is set, also log the error in the system log.
    SenseInfoLogError(DeviceExtension,
                      Srb,
                      majorFunctionCode,
                      ioControlCode,
                      RetriedCount,
                      Status,
                      &retry,
                      &logContext);

    // all process about the error done. check if the irp was cancelled.
    if ((!NT_SUCCESS(*Status)) && retry)
    {
        PCDROM_REQUEST_CONTEXT requestContext = RequestGetContext(Request);

        if ((requestContext->OriginalRequest != NULL) &&
            WdfRequestIsCanceled(requestContext->OriginalRequest)
            )
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,  
                       "Request %p was cancelled when it would have been retried\n",
                       requestContext->OriginalRequest));
            
            *Status = STATUS_CANCELLED;
            retry = FALSE;
            *RetryIntervalIn100ns = 0;
        }
    }

    // now, all decisions are made. display trace information.
    if (retry)
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  
                   "Command shall be retried in %2I64d.%03I64d seconds\n",
                   (*RetryIntervalIn100ns / UNIT_100NS_PER_SECOND),
                   (*RetryIntervalIn100ns / 10000) % 1000
                   ));
    }
    else
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,  
                   "Will not retry; Sense/ASC/ASCQ of %02x/%02x/%02x\n",
                   senseBuffer->SenseKey,
                   senseBuffer->AdditionalSenseCode,
                   senseBuffer->AdditionalSenseCodeQualifier
                   ));
    }

    return retry;

} // end SenseInfoInterpret()


BOOLEAN
SenseInfoInterpretForZPODD(
    _In_    PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_    PSCSI_REQUEST_BLOCK     Srb,
    _Out_   NTSTATUS*               Status,
    _Out_ _Out_range_(0, MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
              LONGLONG*             RetryIntervalIn100ns
    )
/*++

SenseInfoInterpretForZPODD()

Routine Description:

    This routine interprets the data returned from the SCSI request sense. 
    It determines the status to return in the IRP 
    and whether this request can be retried.

Arguments:

    Device - Supplies the device object associated with this request.
    Srb - Supplies the scsi request block which failed.

Return Value:

    BOOLEAN TRUE: Drivers should retry this request.
            FALSE: Drivers should not retry this request.
    Status - Returns the status for the request.
    RetryInterval - Number of seconds before the request should be retried.
                    Zero indicates the request should be immediately retried.

--*/
{
    BOOLEAN                 retry = FALSE;
    PSENSE_DATA             senseBuffer = Srb->SenseInfoBuffer;
    ULONG                   readSector = 0;
    PZERO_POWER_ODD_INFO    zpoddInfo = DeviceExtension->ZeroPowerODDInfo;

    *Status = STATUS_IO_DEVICE_ERROR;
    *RetryIntervalIn100ns = 0;

    if (zpoddInfo->RetryFirstCommand != FALSE)
    {
        // The first command to the logical unit after power resumed will be terminated
        // with CHECK CONDITION Status, 6/29/00 POWER ON, RESET, OR BUS DEVICE RESET OCCURRED

        // We have observed some devices return a different sense code, and thus as long as
        // the first command after power resume fails, we just retry one more time.
        zpoddInfo->RetryFirstCommand = FALSE;

        retry = TRUE;
    }
    else if ((Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
             (Srb->SenseInfoBufferLength >= RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength))) 
    {
        UCHAR   senseKey = (UCHAR)(senseBuffer->SenseKey & 0x0f);
        UCHAR   additionalSenseCode = 0;
        UCHAR   additionalSenseCodeQual = 0;

        // Zero the additional sense code and additional sense code qualifier
        // if they were not returned by the device.
        readSector = senseBuffer->AdditionalSenseLength + offsetof(SENSE_DATA, AdditionalSenseLength);
        if (readSector > Srb->SenseInfoBufferLength) 
        {
            readSector = Srb->SenseInfoBufferLength;
        }

        additionalSenseCode = (readSector >= RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseCode)) ?
                               senseBuffer->AdditionalSenseCode : 0;
        additionalSenseCodeQual = (readSector >= RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseCodeQualifier)) ?
                                   senseBuffer->AdditionalSenseCodeQualifier : 0;

        // If sense code is 2/4/1, device is becoming ready from ZPODD mode. According to Mt Fuji, device
        // could take up to 800msec to be fully operational.
        if ((senseKey == SCSI_SENSE_NOT_READY) &&
            (additionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY) &&
            (additionalSenseCodeQual == SCSI_SENSEQ_BECOMING_READY))
        {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                        "SenseInfoInterpretForZPODD: In process of becoming ready\n"));

            zpoddInfo->BecomingReadyRetryCount--;

            if (zpoddInfo->BecomingReadyRetryCount > 0)
            {
                DEVICE_EVENT_BECOMING_READY notReady = {0};

                retry = TRUE;
                *Status = STATUS_DEVICE_NOT_READY;
                *RetryIntervalIn100ns = BECOMING_READY_RETRY_INTERNVAL_IN_100NS;

                notReady.Version = 1;
                notReady.Reason = 1;
                notReady.Estimated100msToReady = (ULONG) *RetryIntervalIn100ns / (1000 * 1000);
                DeviceSendNotification(DeviceExtension,
                                       &GUID_IO_DEVICE_BECOMING_READY,
                                       sizeof(DEVICE_EVENT_BECOMING_READY),
                                       &notReady);
            }
        }
    }

    // now, all decisions are made. display trace information.
    if (retry)
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  
                   "Command shall be retried in %2I64d.%03I64d seconds\n",
                   (*RetryIntervalIn100ns / UNIT_100NS_PER_SECOND),
                   (*RetryIntervalIn100ns / 10000) % 1000
                   ));
    }
    else
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,  
                   "Will not retry; Sense/ASC/ASCQ of %02x/%02x/%02x\n",
                   senseBuffer->SenseKey,
                   senseBuffer->AdditionalSenseCode,
                   senseBuffer->AdditionalSenseCodeQualifier
                   ));
    }

    return retry;

} // end SenseInfoInterpret()


BOOLEAN
RequestSenseInfoInterpret(
    _In_      PCDROM_DEVICE_EXTENSION   DeviceExtension,
    _In_      WDFREQUEST                Request, 
    _In_      PSCSI_REQUEST_BLOCK       Srb,
    _In_      ULONG                     RetriedCount,
    _Out_     NTSTATUS*                 Status,
    _Out_opt_ _Deref_out_range_(0, MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
              LONGLONG*                 RetryIntervalIn100ns
    )
/*++

Routine Description:

Interpret the error, process it.
    1. Release device queue if it's frozen.
    2. Interpret and process the error.

Arguments:

    DeviceExtension - Supplies the device object associated with this request.
    Request - the Request that error occurs on.
    Srb - Supplies the scsi request block which failed.
    RetriedCount - retried count.

Return Value:

    BOOLEAN TRUE:  Drivers should retry this request.
            FALSE: Drivers should not retry this request.
    Status - Returns the status for the request.
    RetryIntervalIn100nsUnits - Number of 100ns before the request should be retried.
                                Zero indicates the request should be immediately retried.

--*/
{
    BOOLEAN                 retry = FALSE;
    LONGLONG                retryIntervalIn100ns = 0;
    PZERO_POWER_ODD_INFO    zpoddInfo = DeviceExtension->ZeroPowerODDInfo;

    if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS)
    {
        // request succeeded. 
        if ((zpoddInfo != NULL) &&
            (zpoddInfo->BecomingReadyRetryCount > 0))
        {
            zpoddInfo->BecomingReadyRetryCount = 0;
        }

        *Status = STATUS_SUCCESS;
        retry = FALSE;
    }
    else
    {
        // request failed. We need to process the error.
        
        // 1. Release the queue if it is frozen.
        if (Srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) 
        {
            DeviceReleaseQueue(DeviceExtension->Device);
        }

        if ((zpoddInfo != NULL) &&
            ((zpoddInfo->RetryFirstCommand != FALSE) || (zpoddInfo->BecomingReadyRetryCount > 0)))
        {
            retry = SenseInfoInterpretForZPODD(DeviceExtension,
                                               Srb,
                                               Status,
                                               &retryIntervalIn100ns);
        }

        if (retry == FALSE)
        {
            // 2. Error Processing
            if ((zpoddInfo != NULL) &&
                (zpoddInfo->BecomingReadyRetryCount > 0))
            {
                zpoddInfo->BecomingReadyRetryCount = 0;
            }

            retry = SenseInfoInterpret(DeviceExtension,
                                       Request,
                                       Srb,
                                       RetriedCount,
                                       Status,
                                       &retryIntervalIn100ns);
        }
    }

    if (RetryIntervalIn100ns != NULL)
    {
        *RetryIntervalIn100ns = retryIntervalIn100ns;
    }

    return retry;
}


BOOLEAN
RequestSenseInfoInterpretForScratchBuffer(
    _In_    PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_    ULONG                   RetriedCount,
    _Out_   NTSTATUS*               Status,
    _Out_ _Deref_out_range_(0, MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
            LONGLONG*               RetryIntervalIn100ns
    )
/*++

Routine Description:

    to analyze the error occurred and set the status, retry interval and decide to retry or not.

Arguments:

    DeviceExtension - device extension
    RetriedCount - already retried count.

Return Value:

    BOOLEAN - TRUE (should retry)
    Status - NTSTATUS
    RetryIntervalIn100nsUnits - retry interval

--*/
{
    NT_ASSERT(DeviceExtension->ScratchContext.ScratchInUse != 0);

    return RequestSenseInfoInterpret(DeviceExtension,
                                     DeviceExtension->ScratchContext.ScratchRequest,
                                     DeviceExtension->ScratchContext.ScratchSrb,
                                     RetriedCount,
                                     Status,
                                     RetryIntervalIn100ns);
}



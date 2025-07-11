/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device identification
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

static
BOOLEAN
AtaDeviceIdentifyDataEqual(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData1,
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData2)
{
    if (!RtlEqualMemory(IdentifyData1->SerialNumber,
                        IdentifyData2->SerialNumber,
                        RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, SerialNumber)))
    {
        return FALSE;
    }

    if (!RtlEqualMemory(IdentifyData1->FirmwareRevision,
                        IdentifyData2->FirmwareRevision,
                        RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, FirmwareRevision) +
                        RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, ModelNumber)))
    {
        return FALSE;
    }

    return TRUE;
}

VOID
AtaDeviceSendIdentify(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ UCHAR Command)
{
    PATA_DEVICE_REQUEST Request = Context->InternalRequest;
    PATAPORT_DEVICE_EXTENSION DevExt = Context->DevExt;

    TRACE("PORT %lu: Send %s identify tid %u\n",
          GET_PORTDATA(Context)->PortNumber,
          Command == IDE_COMMAND_IDENTIFY ? "ATA" : "ATAPI",
          DevExt->Device.AtaScsiAddress.TargetId);

    /*
     * For PATA devices, disable interrupts for the identify command and use polling instead.
     *
     * On some single device 1 configurations or non-existent IDE channels
     * the status register stuck permanently at a value of 0,
     * and we incorrectly assume that the device is present.
     * This will result in a taskfile timeout
     * which must be avoided as it would cause hangs at boot time.
     */
    Request->Flags = REQUEST_FLAG_DATA_IN | REQUEST_FLAG_HAS_LOCAL_BUFFER;
    if (!IS_AHCI(&DevExt->Device))
        Request->Flags |= REQUEST_FLAG_POLL;
    Request->TimeOut = 10;
    Request->DataTransferLength = sizeof(DevExt->IdentifyDeviceData);

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = Command;

    AtaFsmIssueCommand(Context);
}

static
VOID
AtaDeviceSendInquiry(
    _In_ PATA_WORKER_CONTEXT Context)
{
    PATA_DEVICE_REQUEST Request = Context->InternalRequest;
    PATAPORT_DEVICE_EXTENSION DevExt = Context->DevExt;
    PCDB Cdb;

    TRACE("PORT %lu: Send inquiry tid %u\n",
          GET_PORTDATA(Context)->PortNumber,
          DevExt->Device.AtaScsiAddress.TargetId);

    Request->Flags = REQUEST_FLAG_DATA_IN |
                     REQUEST_FLAG_PACKET_COMMAND |
                     REQUEST_FLAG_HAS_LOCAL_BUFFER;
    Request->TimeOut = 3;
    Request->DataTransferLength = INQUIRYDATABUFFERSIZE;

    RtlZeroMemory(Request->Cdb, sizeof(Request->Cdb));
    Cdb = (PCDB)&Request->Cdb;
    Cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
    Cdb->CDB6INQUIRY.LogicalUnitNumber = DevExt->Device.AtaScsiAddress.Lun;
    Cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

    AtaFsmIssueCommand(Context);
}

static
BOOLEAN
AtaDeviceIsXboxDrive(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    static const PCSTR AtapBrokenInquiryDrive[] =
    {
        "THOMSON-DVD",
        "PHILIPS XBOX DVD DRIVE",
        "PHILIPS J5 3235C",
        "SAMSUNG DVD-ROM SDG-605B"
        // TODO: Xbox 360 drives are also affected.
    };
    CHAR ModelNumber[26];
    PUCHAR End;
    ULONG i;

    End = AtaCopyIdStringUnsafe((PUCHAR)ModelNumber,
                                DevExt->IdentifyDeviceData.ModelNumber,
                                sizeof(ModelNumber));
    *End = ANSI_NULL;

    for (i = 0; i < RTL_NUMBER_OF(AtapBrokenInquiryDrive); ++i)
    {
        if (strcmp(ModelNumber, AtapBrokenInquiryDrive[i]) == 0)
            return TRUE;
    }

    return FALSE;
}

static
VOID
AtaCreateAtapiStandardInquiryData(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DevExt->IdentifyDeviceData;
    PINQUIRYDATA InquiryData = &DevExt->InquiryData;

    InquiryData->DeviceType = READ_ONLY_DIRECT_ACCESS_DEVICE;
    InquiryData->RemovableMedia = AtaDevIsRemovable(IdentifyData);
    InquiryData->Versions = 0x04; // SPC-2
    InquiryData->ResponseDataFormat = 2; // This means "Complies to this standard"
    InquiryData->AdditionalLength =
        INQUIRYDATABUFFERSIZE - RTL_SIZEOF_THROUGH_FIELD(INQUIRYDATA, AdditionalLength);

    /* Copy ModelNumber from a byte-swapped ATA string */
    AtaCopyIdStringSafe((PCHAR)InquiryData->VendorId,
                        IdentifyData->ModelNumber,
                        sizeof(IdentifyData->ModelNumber),
                        ' ');
    AtaSwapIdString(InquiryData->VendorId, sizeof(IdentifyData->ModelNumber) / 2);

    /* Copy FirmwareRevision from a byte-swapped ATA string */
    AtaCopyIdStringSafe((PCHAR)InquiryData->ProductRevisionLevel,
                        IdentifyData->FirmwareRevision,
                        sizeof(InquiryData->ProductRevisionLevel),
                        ' ');
    AtaSwapIdString(InquiryData->ProductRevisionLevel,
                    sizeof(InquiryData->ProductRevisionLevel) / 2);
}

static
VOID
AtaDeviceSpinUp(
    _In_ PATA_WORKER_CONTEXT Context)
{
    PATA_DEVICE_REQUEST Request = Context->InternalRequest;

    TRACE("PORT %lu: Spin-up tid %u\n",
          GET_PORTDATA(Context)->PortNumber,
          Context->DevExt->Device.AtaScsiAddress.TargetId);

    Request->Flags = 0;
    Request->TimeOut = 20;

    Request->TaskFile.Command = IDE_COMMAND_SET_FEATURE;
    Request->TaskFile.Feature = IDE_FEATURE_PUIS_SPIN_UP;

    AtaFsmIssueCommand(Context);
}

static
VOID
AtaPortHandleIdIdentifyTargetDevice(
    _In_ PATA_WORKER_CONTEXT Context)
{
    PATAPORT_DEVICE_EXTENSION DevExt = Context->DevExt;
    BOOLEAN StartFromAtapi;

    /*
     * Try the known device type first.
     * This may speed up device detection by eliminating I/O errors.
     */
    if (DevExt->Device.DeviceFlags & DEVICE_UNINITIALIZED)
    {
        if (IS_AHCI(&DevExt->Device))
        {
            ULONG Signature = AHCI_PORT_READ(GET_PORTDATA(Context)->Ahci.IoBase, PxSignature);

            StartFromAtapi = (Signature == AHCI_PXSIG_ATAPI);
        }
        else
        {
            StartFromAtapi = (DevExt->DeviceClass == DEV_ATAPI);
        }
    }
    else
    {
        StartFromAtapi = IS_ATAPI(&DevExt->Device);
    }

    if (Context->IdentifyAttempt != 0)
        StartFromAtapi = !StartFromAtapi;

    if (StartFromAtapi)
        AtaDeviceSendIdentify(Context, IDE_COMMAND_ATAPI_IDENTIFY);
    else
        AtaDeviceSendIdentify(Context, IDE_COMMAND_IDENTIFY);
    AtaFsmSetLocalState(Context, PORT_STATE_ID_IDENTIFY_RESULT);
}

static
VOID
AtaPortHandleIdIdentifyTargetDeviceResult(
    _In_ PATA_WORKER_CONTEXT Context)
{
    PATA_DEVICE_REQUEST Request = Context->InternalRequest;
    PATAPORT_DEVICE_EXTENSION DevExt = Context->DevExt;
    PIDENTIFY_DEVICE_DATA IdentifyData = DevExt->Device.LocalBuffer;
    ULONG DeviceClass;
    BOOLEAN IsSameDevice;

    if (Request->SrbStatus != SRB_STATUS_SUCCESS)
    {
        /*
         * We do not check the device signature here,
         * because the NEC CDR-260 drive reports an ATA signature.
         */
        if (Context->IdentifyAttempt++ == 0)
            AtaFsmSetLocalState(Context, PORT_STATE_ID_IDENTIFY);
        else
            AtaFsmCompleteDeviceEnumEvent(Context, DevExt, DEV_STATUS_NO_DEVICE);
        return;
    }

    /*
     * The drive needs to be spun up
     * or the device was unable to return complete identity data.
     */
    if (AtaDevInPuisState(IdentifyData) || AtaDevIsIdentifyDataIncomplete(IdentifyData))
    {
        if (!(DevExt->Worker.Flags & DEV_WORKER_FLAG_SPIN_UP))
        {
            DevExt->Worker.Flags |= DEV_WORKER_FLAG_SPIN_UP;

            AtaDeviceSpinUp(Context);
            AtaFsmSetLocalState(Context, PORT_STATE_ID_IDENTIFY);
        }
        else
        {
            ERR("Failed to spin-up device\n");
            AtaFsmCompleteDeviceEnumEvent(Context, DevExt, DEV_STATUS_NO_DEVICE);
        }
        return;
    }

    /* Verify the checksum */
    if (!AtaDevIsIdentifyDataValid(IdentifyData))
    {
        ERR("Identify data CRC error\n");
        AtaFsmCompleteDeviceEnumEvent(Context, DevExt, DEV_STATUS_NO_DEVICE);
        return;
    }

    if (Request->TaskFile.Command == IDE_COMMAND_ATAPI_IDENTIFY)
        DeviceClass = DEV_ATAPI;
    else
        DeviceClass = DEV_ATA;

    if (DevExt->Device.DeviceFlags & DEVICE_UNINITIALIZED)
    {
        IsSameDevice = FALSE;
    }
    else
    {
        IsSameDevice = (DeviceClass == DevExt->DeviceClass) &&
                       (AtaDeviceIdentifyDataEqual(&DevExt->IdentifyDeviceData,
                                                   DevExt->Device.LocalBuffer));
    }

    /* Update identify data */
    RtlCopyMemory(&DevExt->IdentifyDeviceData,
                  DevExt->Device.LocalBuffer,
                  sizeof(DevExt->IdentifyDeviceData));

    DevExt->DeviceClass = DeviceClass;

    if (IsSameDevice)
    {
        AtaFsmCompleteDeviceEnumEvent(Context, DevExt, DEV_STATUS_SAME_DEVICE);
        return;
    }

    if (DeviceClass == DEV_ATAPI)
    {
        DevExt->Device.DeviceFlags |= DEVICE_IS_ATAPI;

        DevExt->Device.CdbSize = AtaDevCdbSizeInWords(&DevExt->IdentifyPacketData);

        if (AtaDevHasCdbInterrupt(&DevExt->IdentifyPacketData))
            DevExt->Device.DeviceFlags |= DEVICE_HAS_CDB_INTERRUPT;

        if (AtaDevIsDmaDirectionRequired(&DevExt->IdentifyPacketData))
            DevExt->Device.DeviceFlags |= DEVICE_NEED_DMA_DIRECTION;
        else
            DevExt->Device.DeviceFlags &= ~DEVICE_NEED_DMA_DIRECTION;

        /* The NEC CDR-260 string is not byteswapped */
        if (DevExt->IdentifyPacketData.ModelNumber[0] == 'N' &&
            DevExt->IdentifyPacketData.ModelNumber[1] == 'E' &&
            DevExt->IdentifyPacketData.ModelNumber[2] == 'C')
        {
            DevExt->Device.DeviceFlags |= DEVICE_IS_NEC_CDR260;
        }

        AtaDeviceSendInquiry(Context);
        AtaFsmSetLocalState(Context, PORT_STATE_ID_INQUIRY_RESULT);
        return;
    }

    DevExt->Device.DeviceFlags &= ~(DEVICE_IS_ATAPI |
                                    DEVICE_HAS_CDB_INTERRUPT |
                                    DEVICE_NEED_DMA_DIRECTION |
                                    DEVICE_IS_NEC_CDR260);

    AtaFsmCompleteDeviceEnumEvent(Context, DevExt, DEV_STATUS_NEW_DEVICE);
}

static
VOID
AtaPortHandleIdIdentifyLogicalDeviceResult(
    _In_ PATA_WORKER_CONTEXT Context)
{
    PATA_DEVICE_REQUEST Request = Context->InternalRequest;
    PATAPORT_DEVICE_EXTENSION DevExt = Context->DevExt;
    ATA_DEVICE_STATUS Status;

    if (Request->SrbStatus != SRB_STATUS_SUCCESS)
    {
        if (DevExt->Device.AtaScsiAddress.Lun == 0 && AtaDeviceIsXboxDrive(DevExt))
        {
            /*
            * The INQUIRY command is mandatory to be implemented by ATAPI devices,
            * but some Xbox drives violate this.
            */
            AtaCreateAtapiStandardInquiryData(DevExt);
            Status = DEV_STATUS_NEW_DEVICE;
        }
        else
        {
            Status = DEV_STATUS_NO_DEVICE;
        }
    }
    else
    {
        /* Update the inquiry data */
        RtlCopyMemory(&DevExt->InquiryData,
                      DevExt->Device.LocalBuffer,
                      sizeof(DevExt->InquiryData));

        Status = DEV_STATUS_NEW_DEVICE;
    }

    AtaFsmCompleteDeviceEnumEvent(Context, DevExt, Status);
}

VOID
AtaDeviceIdRunStateMachine(
    _In_ PATA_WORKER_CONTEXT Context)
{
    switch (Context->LocalState)
    {
        case PORT_STATE_ID_IDENTIFY:
            AtaPortHandleIdIdentifyTargetDevice(Context);
            break;

        case PORT_STATE_ID_IDENTIFY_RESULT:
            AtaPortHandleIdIdentifyTargetDeviceResult(Context);
            break;

        case PORT_STATE_ID_INQUIRY_RESULT:
            AtaPortHandleIdIdentifyLogicalDeviceResult(Context);
            break;

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }
}

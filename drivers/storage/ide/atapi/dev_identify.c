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
                        RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, FirmwareRevision)))
    {
        return FALSE;
    }

    if (!RtlEqualMemory(IdentifyData1->ModelNumber,
                        IdentifyData2->ModelNumber,
                        RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, ModelNumber)))
    {
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
AtaDeviceSendIdentify(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ UCHAR Command)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;

    TRACE("CH %lu: Send %s identify device %u\n",
          PortData->PortNumber,
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
    Request->Flags = REQUEST_FLAG_POLL;

    Request->Flags |= REQUEST_FLAG_DATA_IN | REQUEST_FLAG_HAS_LOCAL_BUFFER;
    Request->TimeOut = 10;
    Request->DataTransferLength = sizeof(DevExt->IdentifyDeviceData);

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = Command;

    return AtaPortSendRequest(PortData, DevExt);
}

static
NTSTATUS
AtaDeviceSpinUp(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;

    TRACE("CH %lu: Spin-up device %u\n",
          PortData->PortNumber,
          DevExt->Device.AtaScsiAddress.TargetId);

    Request->Flags = 0;
    Request->TimeOut = 20;

    Request->TaskFile.Command = IDE_COMMAND_SET_FEATURE;
    Request->TaskFile.Feature = IDE_FEATURE_PUIS_SPIN_UP;

    return AtaPortSendRequest(PortData, DevExt);
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
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData,
    _Out_ PINQUIRYDATA InquiryData)
{
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
NTSTATUS
AtaDeviceSendInquiry(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;
    NTSTATUS Status;
    PCDB Cdb;

    TRACE("CH %lx: Send inquiry device %u\n",
          PortData->PortNumber,
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

    Status = AtaPortSendRequest(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
        return Status;

    if (!NT_SUCCESS(Status))
    {
        if (DevExt->Device.AtaScsiAddress.Lun == 0 && AtaDeviceIsXboxDrive(DevExt))
        {
            /*
            * The INQUIRY command is mandatory to be implemented by ATAPI devices,
            * but some Xbox drives violate this.
            */
            AtaCreateAtapiStandardInquiryData(&DevExt->IdentifyDeviceData,
                                              DevExt->Device.LocalBuffer);
            Status = DEV_STATUS_NEW_DEVICE;
        }
        else
        {
            Status = DEV_STATUS_NO_DEVICE;
        }
    }

    return STATUS_SUCCESS;
}

ATA_DEVICE_STATUS
AtaPortIdentifyDevice(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = DevExt->Device.LocalBuffer;
    ATA_CONNECTION_STATUS ConnectionStatus;
    NTSTATUS Status;
    BOOLEAN StartFromAtapi, IsSameDevice;
    ULONG Attempt, RetryCount;
    ATA_DEVICE_TYPE DeviceType;

    _InterlockedOr(&PortData->InterruptFlags, PORT_INT_FLAG_IGNORE_LINK_IRQ);

    ConnectionStatus = PortData->IdentifyDevice(PortData->ChannelContext,
                                                DevExt->Device.AtaScsiAddress.TargetId);

    _InterlockedAnd(&PortData->InterruptFlags, ~PORT_INT_FLAG_IGNORE_LINK_IRQ);

    if (ConnectionStatus == CONN_STATUS_FAILURE)
        return DEV_STATUS_FAILED;

    if (ConnectionStatus == CONN_STATUS_NO_DEVICE)
        return DEV_STATUS_NO_DEVICE;

    /* Power up the device if needed */
    Status = AtaPortCheckDevicePowerState(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
        return DEV_STATUS_FAILED;

    /*
     * Try the known device type first.
     * This may speed up device detection by eliminating I/O errors.
     */
    if (DevExt->Device.DeviceFlags & DEVICE_UNINITIALIZED)
        StartFromAtapi = (ConnectionStatus == CONN_STATUS_DEV_ATAPI);
    else
        StartFromAtapi = IS_ATAPI(&DevExt->Device);

    /* Look for ATA/ATAPI devices */
    for (RetryCount = 0; RetryCount < 2; ++RetryCount)
    {
        /* Send the identify command */
        for (Attempt = 0; Attempt < 2; ++Attempt)
        {
            if (StartFromAtapi)
                DeviceType = DEV_ATAPI;
            else
                DeviceType = DEV_ATA;

            /* Swap device types */
            if (Attempt != 0)
                DeviceType ^= (DEV_ATAPI ^ DEV_ATA);

            Status = AtaDeviceSendIdentify(PortData,
                                           DevExt,
                                           DeviceType == DEV_ATAPI ?
                                           IDE_COMMAND_ATAPI_IDENTIFY : IDE_COMMAND_IDENTIFY);
            if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
                return DEV_STATUS_FAILED;

            if (NT_SUCCESS(Status))
                break;
        }
        if (!NT_SUCCESS(Status))
            return DEV_STATUS_NO_DEVICE;

        /*
         * The drive needs to be spun up
         * or the device was unable to return complete identity data.
         */
        if (AtaDevInPuisState(IdentifyData) || AtaDevIsIdentifyDataIncomplete(IdentifyData))
        {

            Status = AtaDeviceSpinUp(PortData, DevExt);
            if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
                return DEV_STATUS_FAILED;

            if (!NT_SUCCESS(Status))
                ERR("Failed to spin-up device\n");
            continue;
        }
    }

    /* Verify the checksum */
    if (!AtaDevIsIdentifyDataValid(IdentifyData))
    {
        ERR("Identify data CRC error\n");
        return DEV_STATUS_FAILED;
    }

    if (DevExt->Device.DeviceFlags & DEVICE_UNINITIALIZED)
    {
        IsSameDevice = FALSE;
    }
    else
    {
        IsSameDevice = (DeviceType == DevExt->DeviceType) &&
                       (AtaDeviceIdentifyDataEqual(&DevExt->IdentifyDeviceData,
                                                   DevExt->Device.LocalBuffer));
    }

    /* Update identify data */
    RtlCopyMemory(&DevExt->IdentifyDeviceData,
                  DevExt->Device.LocalBuffer,
                  sizeof(DevExt->IdentifyDeviceData));
    DevExt->DeviceType = DeviceType;

    if (IsSameDevice)
        return DEV_STATUS_SAME_DEVICE;

    DevExt->Device.TransportFlags &= ~(DEVICE_IS_ATAPI |
                                       DEVICE_HAS_CDB_INTERRUPT |
                                       DEVICE_NEED_DMA_DIRECTION |
                                       DEVICE_IS_NEC_CDR260);

    if (DeviceType == DEV_ATAPI)
    {
        DevExt->Device.TransportFlags |= DEVICE_IS_ATAPI;

        DevExt->Device.CdbSize = AtaDevCdbSizeInWords(&DevExt->IdentifyPacketData);
        TRACE("Device has CDB size of %u bytes\n", DevExt->Device.CdbSize * 2);

        if (AtaDevHasCdbInterrupt(&DevExt->IdentifyPacketData))
        {
            TRACE("Device has CDB interrupt\n");
            DevExt->Device.TransportFlags |= DEVICE_HAS_CDB_INTERRUPT;
        }

        if (AtaDevIsDmaDirectionRequired(&DevExt->IdentifyPacketData))
        {
            TRACE("Device needs DMA DIR\n");
            DevExt->Device.TransportFlags |= DEVICE_NEED_DMA_DIRECTION;
        }

        /* The NEC CDR-260 string is not byteswapped */
        if (DevExt->IdentifyPacketData.ModelNumber[0] == 'N' &&
            DevExt->IdentifyPacketData.ModelNumber[1] == 'E' &&
            DevExt->IdentifyPacketData.ModelNumber[2] == 'C')
        {
            TRACE("Device is a NEC CDR-260 drive\n");
            DevExt->Device.TransportFlags |= DEVICE_IS_NEC_CDR260;
        }

        Status = AtaDeviceSendInquiry(PortData, DevExt);
        if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
            return DEV_STATUS_FAILED;

        /* Update the inquiry data */
        RtlCopyMemory(&DevExt->InquiryData,
                      DevExt->Device.LocalBuffer,
                      sizeof(DevExt->InquiryData));
    }

    return DEV_STATUS_NEW_DEVICE;
}

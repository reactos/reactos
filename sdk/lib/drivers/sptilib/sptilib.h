/*
 * PROJECT:     ReactOS Storage Stack
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Public header of the ATA and SCSI Pass Through Interface for storage drivers
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

/**
 * ATA Protocol field definitions for the ATA passthrough commands
 * (SCSIOP_ATA_PASSTHROUGH12 and SCSIOP_ATA_PASSTHROUGH16).
 */
/*@{*/
#define ATA_PASSTHROUGH_PROTOCOL_HARDWARE_RESET      0x0
#define ATA_PASSTHROUGH_PROTOCOL_SOFTWARE_RESET      0x1
#define ATA_PASSTHROUGH_PROTOCOL_NON_DATA            0x3
#define ATA_PASSTHROUGH_PROTOCOL_PIO_DATA_IN         0x4
#define ATA_PASSTHROUGH_PROTOCOL_PIO_DATA_OUT        0x5
#define ATA_PASSTHROUGH_PROTOCOL_DMA                 0x6
#define ATA_PASSTHROUGH_PROTOCOL_DEVICE_DIAG         0x8
#define ATA_PASSTHROUGH_PROTOCOL_DEVICE_RESET        0x9
#define ATA_PASSTHROUGH_PROTOCOL_UDMA_DATA_IN        0xA
#define ATA_PASSTHROUGH_PROTOCOL_UDMA_DATA_OUT       0xB
#define ATA_PASSTHROUGH_PROTOCOL_NCQ                 0xC
#define ATA_PASSTHROUGH_PROTOCOL_RETURN_RESPONSE     0xF
/*@}*/

/**
 * Additional sense code for the successfully completed ATA passthrough commands
 * with check condition enabled.
 */
#define SCSI_SENSEQ_ATA_PASS_THROUGH_INFORMATION_AVAILABLE   0x1D

/**
 * @brief
 * Handler for the IOCTL_ATA_PASS_THROUGH and IOCTL_ATA_PASS_THROUGH_DIRECT requests.
 *
 * @param[in]       DeviceObject
 * PDO device object.
 *
 * @param[in,out]   Irp
 * Pointer to the IOCTL request.
 *
 * @param[in]       MaximumTransferLength
 * Maximum size of data transfer for a device.
 *
 * @param[in]       MaximumPhysicalPages
 * Maximum number of physical pages per data transfer for a device.
 *
 * @return Status.
 */
CODE_SEG("PAGE")
NTSTATUS
SptiHandleAtaPassthru(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_ ULONG MaximumTransferLength,
    _In_ ULONG MaximumPhysicalPages);

/**
 * @brief
 * Handler for the IOCTL_SCSI_PASS_THROUGH and IOCTL_SCSI_PASS_THROUGH_DIRECT requests.
 *
 * @param[in]       DeviceObject
 * PDO device object.
 *
 * @param[in,out]   Irp
 * Pointer to the IOCTL request.
 *
 * @param[in]       MaximumTransferLength
 * Maximum size of data transfer for a device.
 *
 * @param[in]       MaximumPhysicalPages
 * Maximum number of physical pages per data transfer for a device.
 *
 * @return Status.
 */
CODE_SEG("PAGE")
NTSTATUS
SptiHandleScsiPassthru(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_ ULONG MaximumTransferLength,
    _In_ ULONG MaximumPhysicalPages);

/*
 * PROJECT:     ReactOS Storage Stack / SCSIPORT storage port library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     IOCTL handlers
 * COPYRIGHT:   Eric Kohl (eric.kohl@reactos.org)
 *              Aleksey Bragin (aleksey@reactos.org)
 */

#include "scsiport.h"

#define NDEBUG
#include <debug.h>


static
NTSTATUS
SpiGetInquiryData(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP Irp)
{
    ULONG InquiryDataSize;
    PSCSI_LUN_INFO LunInfo;
    ULONG BusCount, LunCount, Length;
    PIO_STACK_LOCATION IrpStack;
    PSCSI_ADAPTER_BUS_INFO AdapterBusInfo;
    PSCSI_INQUIRY_DATA InquiryData;
    PSCSI_BUS_DATA BusData;
    ULONG Bus;
    PUCHAR Buffer;

    DPRINT("SpiGetInquiryData() called\n");

    /* Get pointer to the buffer */
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    /* Initialize bus and LUN counters */
    BusCount = DeviceExtension->BusesConfig->NumberOfBuses;
    LunCount = 0;

    /* Calculate total number of LUNs */
    for (Bus = 0; Bus < BusCount; Bus++)
        LunCount += DeviceExtension->BusesConfig->BusScanInfo[Bus]->LogicalUnitsCount;

    /* Calculate size of inquiry data, rounding up to sizeof(ULONG) */
    InquiryDataSize =
        ((sizeof(SCSI_INQUIRY_DATA) - 1 + INQUIRYDATABUFFERSIZE +
        sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1));

    /* Calculate data size */
    Length = sizeof(SCSI_ADAPTER_BUS_INFO) + (BusCount - 1) * sizeof(SCSI_BUS_DATA);

    Length += InquiryDataSize * LunCount;

    /* Check, if all data is going to fit into provided buffer */
    if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < Length)
    {
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Store data size in the IRP */
    Irp->IoStatus.Information = Length;

    DPRINT("Data size: %lu\n", Length);

    AdapterBusInfo = (PSCSI_ADAPTER_BUS_INFO)Buffer;

    AdapterBusInfo->NumberOfBuses = (UCHAR)BusCount;

    /* Point InquiryData to the corresponding place inside Buffer */
    InquiryData = (PSCSI_INQUIRY_DATA)(Buffer + sizeof(SCSI_ADAPTER_BUS_INFO) +
                    (BusCount - 1) * sizeof(SCSI_BUS_DATA));

    /* Loop each bus */
    for (Bus = 0; Bus < BusCount; Bus++)
    {
        BusData = &AdapterBusInfo->BusData[Bus];

        /* Calculate and save an offset of the inquiry data */
        BusData->InquiryDataOffset = (ULONG)((PUCHAR)InquiryData - Buffer);

        /* Get a pointer to the LUN information structure */
        LunInfo = DeviceExtension->BusesConfig->BusScanInfo[Bus]->LunInfo;

        /* Store Initiator Bus Id */
        BusData->InitiatorBusId =
            DeviceExtension->BusesConfig->BusScanInfo[Bus]->BusIdentifier;

        /* Store LUN count */
        BusData->NumberOfLogicalUnits =
            DeviceExtension->BusesConfig->BusScanInfo[Bus]->LogicalUnitsCount;

        /* Loop all LUNs */
        while (LunInfo != NULL)
        {
            DPRINT("(Bus %lu Target %lu Lun %lu)\n",
                   Bus, LunInfo->TargetId, LunInfo->Lun);

            /* Fill InquiryData with values */
            InquiryData->PathId = LunInfo->PathId;
            InquiryData->TargetId = LunInfo->TargetId;
            InquiryData->Lun = LunInfo->Lun;
            InquiryData->InquiryDataLength = INQUIRYDATABUFFERSIZE;
            InquiryData->DeviceClaimed = LunInfo->DeviceClaimed;
            InquiryData->NextInquiryDataOffset =
                (ULONG)((PUCHAR)InquiryData + InquiryDataSize - Buffer);

            /* Copy data in it */
            RtlCopyMemory(InquiryData->InquiryData,
                          LunInfo->InquiryData,
                          INQUIRYDATABUFFERSIZE);

            /* Move to the next LUN */
            LunInfo = LunInfo->Next;
            InquiryData = (PSCSI_INQUIRY_DATA) ((PCHAR)InquiryData + InquiryDataSize);
        }

        /* Either mark the end, or set offset to 0 */
        if (BusData->NumberOfLogicalUnits != 0)
            ((PSCSI_INQUIRY_DATA) ((PCHAR)InquiryData - InquiryDataSize))->NextInquiryDataOffset = 0;
        else
            BusData->InquiryDataOffset = 0;
    }

    /* Finish with success */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

/**********************************************************************
 * NAME                         INTERNAL
 *  ScsiPortDeviceControl
 *
 * DESCRIPTION
 *  Answer requests for device control calls
 *
 * RUN LEVEL
 *  PASSIVE_LEVEL
 *
 * ARGUMENTS
 *  Standard dispatch arguments
 *
 * RETURNS
 *  NTSTATUS
 */

NTSTATUS
NTAPI
ScsiPortDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PDUMP_POINTERS DumpPointers;
    NTSTATUS Status;

    DPRINT("ScsiPortDeviceControl()\n");

    Irp->IoStatus.Information = 0;

    Stack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = DeviceObject->DeviceExtension;

    switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {
      case IOCTL_SCSI_GET_DUMP_POINTERS:
        DPRINT("  IOCTL_SCSI_GET_DUMP_POINTERS\n");

        if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DUMP_POINTERS))
        {
          Status = STATUS_BUFFER_OVERFLOW;
          Irp->IoStatus.Information = sizeof(DUMP_POINTERS);
          break;
        }

        DumpPointers = Irp->AssociatedIrp.SystemBuffer;
        DumpPointers->DeviceObject = DeviceObject;
        /* More data.. ? */

        Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(DUMP_POINTERS);
        break;

      case IOCTL_SCSI_GET_CAPABILITIES:
        DPRINT("  IOCTL_SCSI_GET_CAPABILITIES\n");
        if (Stack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(PVOID))
        {
            *((PVOID *)Irp->AssociatedIrp.SystemBuffer) = &DeviceExtension->PortCapabilities;

            Irp->IoStatus.Information = sizeof(PVOID);
            Status = STATUS_SUCCESS;
            break;
        }

        if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(IO_SCSI_CAPABILITIES))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                      &DeviceExtension->PortCapabilities,
                      sizeof(IO_SCSI_CAPABILITIES));

        Irp->IoStatus.Information = sizeof(IO_SCSI_CAPABILITIES);
        Status = STATUS_SUCCESS;
        break;

      case IOCTL_SCSI_GET_INQUIRY_DATA:
          DPRINT("  IOCTL_SCSI_GET_INQUIRY_DATA\n");

          /* Copy inquiry data to the port device extension */
          Status = SpiGetInquiryData(DeviceExtension, Irp);
          break;

      case IOCTL_SCSI_MINIPORT:
          DPRINT1("IOCTL_SCSI_MINIPORT unimplemented!\n");
          Status = STATUS_NOT_IMPLEMENTED;
          break;

      case IOCTL_SCSI_PASS_THROUGH:
          DPRINT1("IOCTL_SCSI_PASS_THROUGH unimplemented!\n");
          Status = STATUS_NOT_IMPLEMENTED;
          break;

      default:
          if (DEVICE_TYPE_FROM_CTL_CODE(Stack->Parameters.DeviceIoControl.IoControlCode) == MOUNTDEVCONTROLTYPE)
          {
            switch (Stack->Parameters.DeviceIoControl.IoControlCode)
            {
            case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
                DPRINT1("Got unexpected IOCTL_MOUNTDEV_QUERY_DEVICE_NAME\n");
                break;
            case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
                DPRINT1("Got unexpected IOCTL_MOUNTDEV_QUERY_UNIQUE_ID\n");
                break;
            default:
                DPRINT("  got ioctl intended for the mount manager: 0x%lX\n", Stack->Parameters.DeviceIoControl.IoControlCode);
                break;
            }
          } else {
            DPRINT1("  unknown ioctl code: 0x%lX\n", Stack->Parameters.DeviceIoControl.IoControlCode);
          }
          Status = STATUS_NOT_IMPLEMENTED;
          break;
    }

    /* Complete the request with the given status */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

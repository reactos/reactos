/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/common.c
 * PURPOSE:     Common operations in FDO/PDO.
 * PROGRAMMERS:
 *              Michael Martin
 */

#define INITGUID
#include "usbehci.h"
#include <wdmguid.h>
#include <stdio.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC AND PRIVATE FUNCTIONS ***********************************************/

NTSTATUS NTAPI
GetBusInterface(PDEVICE_OBJECT DeviceObject, PBUS_INTERFACE_STANDARD busInterface)
{
    KEVENT Event;
    NTSTATUS Status;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION Stack;

    if ((!DeviceObject) || (!busInterface))
        return STATUS_UNSUCCESSFUL;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       DeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);

    if (Irp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Stack=IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = IRP_MJ_PNP;
    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    Stack->Parameters.QueryInterface.InterfaceType = (LPGUID)&GUID_BUS_INTERFACE_STANDARD;
    Stack->Parameters.QueryInterface.Version = 1;
    Stack->Parameters.QueryInterface.Interface = (PINTERFACE)busInterface;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;
    Irp->IoStatus.Status=STATUS_NOT_SUPPORTED ;

    Status=IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        Status=IoStatus.Status;
    }

    return Status;
}

NTSTATUS NTAPI
ForwardAndWaitCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PKEVENT Event)
{
    if (Irp->PendingReturned)
    {
        KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS NTAPI
ForwardAndWait(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PFDO_DEVICE_EXTENSION DeviceExtensions;
    KEVENT Event;
    NTSTATUS Status;


    DeviceExtensions = DeviceObject->DeviceExtension; 
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE)ForwardAndWaitCompletionRoutine, &Event, TRUE, TRUE, TRUE);
    Status = IoCallDriver(DeviceExtensions->LowerDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    return Status;
}

NTSTATUS NTAPI
ForwardIrpAndForget(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice;

    LowerDevice = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
DPRINT1("DeviceObject %x, LowerDevice %x\n", DeviceObject, LowerDevice);
    ASSERT(LowerDevice);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(LowerDevice, Irp);
}

/* Copied fom trunk PCI drivers */
NTSTATUS
DuplicateUnicodeString(ULONG Flags, PCUNICODE_STRING SourceString, PUNICODE_STRING DestinationString)
{
    if (SourceString == NULL || DestinationString == NULL
     || SourceString->Length > SourceString->MaximumLength
     || (SourceString->Length == 0 && SourceString->MaximumLength > 0 && SourceString->Buffer == NULL)
     || Flags == RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING || Flags >= 4)
    {
        return STATUS_INVALID_PARAMETER;
    }


    if ((SourceString->Length == 0)
     && (Flags != (RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
                   RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING)))
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;
    }
    else
    {
        USHORT DestMaxLength = SourceString->Length;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestMaxLength += sizeof(UNICODE_NULL);

        DestinationString->Buffer = ExAllocatePool(PagedPool, DestMaxLength);
        if (DestinationString->Buffer == NULL)
            return STATUS_NO_MEMORY;

        RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
        DestinationString->Length = SourceString->Length;
        DestinationString->MaximumLength = DestMaxLength;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestinationString->Buffer[DestinationString->Length / sizeof(WCHAR)] = 0;
    }

    return STATUS_SUCCESS;
}


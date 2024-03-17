/*
 * PROJECT:     ReactOS Universal Serial Bus Human Interface Device Driver
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     HID Class Driver
 * COPYRIGHT:   Copyright  Michael Martin <michael.martin@reactos.org>
 *              Copyright  Johannes Anderwald <johannes.anderwald@reactos.org>
 *              Copyright 2022 Roman Masanin <36927roma@gmail.com>
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

static LPWSTR ClientIdentificationAddress = L"HIDCLASS";
static LONG HidClassDeviceNumber = 0;

DRIVER_DISPATCH HidClassDispatch;
DRIVER_UNLOAD HidClassDriverUnload;
DRIVER_ADD_DEVICE HidClassAddDevice;

NTSTATUS
NTAPI
DllInitialize(
    IN PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DllUnload(VOID)
{
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HidClassAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    WCHAR CharDeviceName[64];
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT NewDeviceObject;
    PHIDCLASS_FDO_EXTENSION FDODeviceExtension;
    ULONG DeviceExtensionSize;
    PHIDCLASS_DRIVER_EXTENSION DriverExtension;

    /* increment device number */
    UINT32 hidclassDevNum = InterlockedIncrement(&HidClassDeviceNumber) & 0xFFFFFFFF;

    /* construct device name */
    swprintf(CharDeviceName, L"\\Device\\_HID%08x", hidclassDevNum);

    /* initialize device name */
    RtlInitUnicodeString(&DeviceName, CharDeviceName);

    /* get driver object extension */
    DriverExtension = IoGetDriverObjectExtension(DriverObject, ClientIdentificationAddress);
    if (!DriverExtension)
    {
        /* device removed */
        ASSERT(FALSE);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    /* calculate device extension size */
    DeviceExtensionSize = sizeof(HIDCLASS_FDO_EXTENSION) + DriverExtension->DeviceExtensionSize;

    /* now create the device */
    Status = IoCreateDevice(DriverObject, DeviceExtensionSize, &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &NewDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* failed to create device object */
        ASSERT(FALSE);
        return Status;
    }

    /* get device extension */
    FDODeviceExtension = NewDeviceObject->DeviceExtension;

    /* zero device extension */
    RtlZeroMemory(FDODeviceExtension, sizeof(HIDCLASS_FDO_EXTENSION));

    /* initialize device extension */
    FDODeviceExtension->Common.IsFDO = TRUE;
    FDODeviceExtension->Common.DriverExtension = DriverExtension;
    FDODeviceExtension->Common.HidDeviceExtension.PhysicalDeviceObject = PhysicalDeviceObject;
    FDODeviceExtension->Common.HidDeviceExtension.MiniDeviceExtension = (PVOID)((ULONG_PTR)FDODeviceExtension + sizeof(HIDCLASS_FDO_EXTENSION));
    FDODeviceExtension->Common.HidDeviceExtension.NextDeviceObject = IoAttachDeviceToDeviceStack(NewDeviceObject, PhysicalDeviceObject);
    if (FDODeviceExtension->Common.HidDeviceExtension.NextDeviceObject == NULL)
    {
        /* no PDO */
        IoDeleteDevice(NewDeviceObject);
        DPRINT1("[HIDCLASS] failed to attach to device stack\n");
        return STATUS_DEVICE_REMOVED;
    }

    /* sanity check */
    ASSERT(FDODeviceExtension->Common.HidDeviceExtension.NextDeviceObject);

    /* increment stack size */
    NewDeviceObject->StackSize++;

    /* init device object */
    NewDeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
    NewDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    /* now call driver provided add device routine */
    ASSERT(DriverExtension->AddDevice != 0);
    Status = DriverExtension->AddDevice(DriverObject, NewDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        DPRINT1("HIDCLASS: AddDevice failed with %x\n", Status);
        IoDetachDevice(FDODeviceExtension->Common.HidDeviceExtension.NextDeviceObject);
        IoDeleteDevice(NewDeviceObject);
        return Status;
    }

    /* succeeded */
    return Status;
}

VOID
NTAPI
HidClassDriverUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
HidClass_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PHIDCLASS_COMMON_DEVICE_EXTENSION CommonDeviceExtension;

    DPRINT("[HIDCLASS] HidClass_Create\n");

    //
    // get device extension
    //
    CommonDeviceExtension = DeviceObject->DeviceExtension;
    if (CommonDeviceExtension->IsFDO)
    {
        DPRINT("[HIDCLASS] HidClass_Create fdo\n");
         //
         // only supported for PDO
         //
         Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         return STATUS_UNSUCCESSFUL;
    }

    //
    // must be a PDO
    //
    ASSERT(CommonDeviceExtension->IsFDO == FALSE);

    //
    // get stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("ShareAccess %x\n", IoStack->Parameters.Create.ShareAccess);
    DPRINT("Options %x\n", IoStack->Parameters.Create.Options);
    DPRINT("DesiredAccess %x\n", IoStack->Parameters.Create.SecurityContext->DesiredAccess);

    //
    // done
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HidClass_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHIDCLASS_COMMON_DEVICE_EXTENSION CommonDeviceExtension;

    //
    // get device extension
    //
    CommonDeviceExtension = DeviceObject->DeviceExtension;

    //
    // is it a FDO request
    //
    if (CommonDeviceExtension->IsFDO)
    {
        //
        // how did the request get there
        //
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER_1;
    }

    //
    // complete request
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

BOOLEAN HidClass_QueueIrp(PHIDCLASS_PDO_DEVICE_EXTENSION deviceContext, PIRP Irp)
{
    PDRIVER_CANCEL oldCancelRoutine;

    IoMarkIrpPending(Irp);

    // check if canceled
    oldCancelRoutine = IoSetCancelRoutine(Irp, HidClassPDO_IrpCancelRoutine);
    ASSERT(oldCancelRoutine == NULL);

    if (Irp->Cancel == TRUE)
    {
        // Is cancel routine called?
        oldCancelRoutine = IoSetCancelRoutine(Irp, NULL);
        if (oldCancelRoutine != NULL)
        {
            // cancel routine not called
            // IRP must be completed
            return FALSE;
        }
        else
        {
            // cancel routine is called
            // IRP will be closed by it
            return TRUE;
        }
    }
    else
    {
        InsertTailList(&deviceContext->PendingIRPList, &Irp->Tail.Overlay.ListEntry);
    }

    return TRUE;
}

VOID
NTAPI
HidClassPDO_IrpCancelRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PHIDCLASS_PDO_DEVICE_EXTENSION deviceContext = DeviceObject->DeviceExtension;
    KIRQL  oldIrql;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    KeAcquireSpinLock(&deviceContext->ReadLock, &oldIrql);

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    KeReleaseSpinLock(&deviceContext->ReadLock, oldIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return;
}

NTSTATUS
NTAPI
HidClass_Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN isCompletionRequired = TRUE;
    PHIDCLASS_PDO_DEVICE_EXTENSION PDODeviceExtension;
    PUCHAR Address;
    KIRQL CurrentIRQL;

    //DPRINT("[HIDCLASS] HidClass_Read\n");

    //
    // get device extension
    //
    PDODeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    KeAcquireSpinLock(&PDODeviceExtension->ReadLock, &CurrentIRQL);
    Address = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
    if (HidClass_CyclicBufferGet(&PDODeviceExtension->InputBuffer, Address) == TRUE)
    {
        Irp->IoStatus.Information = HidClassPDO_GetCollectionDescription(&PDODeviceExtension->Common.DeviceDescription, PDODeviceExtension->CollectionNumber)->InputLength;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        status = STATUS_SUCCESS;
        isCompletionRequired = TRUE;
    }
    else
    {
        if (HidClass_QueueIrp(PDODeviceExtension, Irp) == TRUE)
        {
            status = STATUS_PENDING;
            isCompletionRequired = FALSE;
        }
        else
        {
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_CANCELLED;
            status = STATUS_PENDING;
            isCompletionRequired = TRUE;
        }
    }
    KeReleaseSpinLock(&PDODeviceExtension->ReadLock, CurrentIRQL);

    // complete request outside spinLock, if required
    if (isCompletionRequired == TRUE)
    {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;
}
NTSTATUS
NTAPI
HidClass_Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PHIDCLASS_COMMON_DEVICE_EXTENSION CommonDeviceExtension;
    PIRP SubIrp;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    HID_XFER_PACKET XferPacket;
    NTSTATUS Status;
    ULONG Length;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Length = IoStack->Parameters.Write.Length;
    if (Length < 1)
    {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&XferPacket, sizeof(XferPacket));
    XferPacket.reportBufferLen = Length;
    XferPacket.reportBuffer = Irp->UserBuffer;
    XferPacket.reportId = XferPacket.reportBuffer[0];

    CommonDeviceExtension = DeviceObject->DeviceExtension;
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
    SubIrp = IoBuildDeviceIoControlRequest(
        IOCTL_HID_WRITE_REPORT,
        CommonDeviceExtension->HidDeviceExtension.NextDeviceObject,
        NULL, 0,
        NULL, 0,
        TRUE,
        &Event,
        &IoStatusBlock);
    if (!SubIrp)
    {
        Irp->IoStatus.Status = STATUS_NO_MEMORY;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_IMPLEMENTED;
    }
    SubIrp->UserBuffer = &XferPacket;
    Status = IoCallDriver(CommonDeviceExtension->HidDeviceExtension.NextDeviceObject, SubIrp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
HidClass_DeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PHIDCLASS_COMMON_DEVICE_EXTENSION CommonDeviceExtension;
    PHID_COLLECTION_INFORMATION CollectionInformation;
    PHIDP_COLLECTION_DESC CollectionDescription;
    PHIDCLASS_PDO_DEVICE_EXTENSION PDODeviceExtension;

    //
    // get device extension
    //
    CommonDeviceExtension = DeviceObject->DeviceExtension;

    //
    // only PDO are supported
    //
    if (CommonDeviceExtension->IsFDO)
    {
        //
        // invalid request
        //
        DPRINT1("[HIDCLASS] DeviceControl Irp for FDO arrived\n");
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER_1;
    }

    ASSERT(CommonDeviceExtension->IsFDO == FALSE);

    //
    // get pdo device extension
    //
    PDODeviceExtension = DeviceObject->DeviceExtension;

    //
    // get stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_HID_GET_COLLECTION_INFORMATION:
        {
            //
            // check if output buffer is big enough
            //
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(HID_COLLECTION_INFORMATION))
            {
                //
                // invalid buffer size
                //
                Irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_BUFFER_SIZE;
            }

            //
            // get output buffer
            //
            CollectionInformation = Irp->AssociatedIrp.SystemBuffer;
            ASSERT(CollectionInformation);

            //
            // get collection description
            //
            CollectionDescription = HidClassPDO_GetCollectionDescription(&CommonDeviceExtension->DeviceDescription,
                                                                         PDODeviceExtension->CollectionNumber);
            ASSERT(CollectionDescription);

            //
            // init result buffer
            //
            CollectionInformation->DescriptorSize = CollectionDescription->PreparsedDataLength;
            CollectionInformation->Polled = CommonDeviceExtension->DriverExtension->DevicesArePolled;
            CollectionInformation->VendorID = CommonDeviceExtension->Attributes.VendorID;
            CollectionInformation->ProductID = CommonDeviceExtension->Attributes.ProductID;
            CollectionInformation->VersionNumber = CommonDeviceExtension->Attributes.VersionNumber;

            //
            // complete request
            //
            Irp->IoStatus.Information = sizeof(HID_COLLECTION_INFORMATION);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }
        case IOCTL_HID_GET_COLLECTION_DESCRIPTOR:
        {
            //
            // get collection description
            //
            CollectionDescription = HidClassPDO_GetCollectionDescription(&CommonDeviceExtension->DeviceDescription,
                                                                         PDODeviceExtension->CollectionNumber);
            ASSERT(CollectionDescription);

            //
            // check if output buffer is big enough
            //
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < CollectionDescription->PreparsedDataLength)
            {
                //
                // invalid buffer size
                //
                Irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_BUFFER_SIZE;
            }

            //
            // copy result
            //
            ASSERT(Irp->UserBuffer);
            RtlCopyMemory(Irp->UserBuffer, CollectionDescription->PreparsedData, CollectionDescription->PreparsedDataLength);

            //
            // complete request
            //
            Irp->IoStatus.Information = CollectionDescription->PreparsedDataLength;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }
        case IOCTL_HID_GET_FEATURE:
        {
            PIRP SubIrp;
            KEVENT Event;
            IO_STATUS_BLOCK IoStatusBlock;
            HID_XFER_PACKET XferPacket;
            NTSTATUS Status;
            PHIDP_REPORT_IDS ReportDescription;

            if (IoStack->Parameters.DeviceIoControl.InputBufferLength < 1)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_PARAMETER;
            }
            ReportDescription = HidClassPDO_GetReportDescriptionByReportID(&PDODeviceExtension->Common.DeviceDescription, ((PUCHAR)Irp->AssociatedIrp.SystemBuffer)[0]);
            if (!ReportDescription || IoStack->Parameters.DeviceIoControl.OutputBufferLength < ReportDescription->FeatureLength)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_PARAMETER;
            }

            RtlZeroMemory(&XferPacket, sizeof(XferPacket));
            XferPacket.reportBufferLen = ReportDescription->FeatureLength;
            XferPacket.reportBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
            XferPacket.reportId = ((PUCHAR)Irp->AssociatedIrp.SystemBuffer)[0];
            if (!XferPacket.reportBuffer)
            {
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
            SubIrp = IoBuildDeviceIoControlRequest(
                IOCTL_HID_GET_FEATURE,
                CommonDeviceExtension->HidDeviceExtension.NextDeviceObject,
                NULL, 0,
                NULL, 0,
                TRUE,
                &Event,
                &IoStatusBlock);
            if (!SubIrp)
            {
                Irp->IoStatus.Status = STATUS_NO_MEMORY;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_NOT_IMPLEMENTED;
            }
            SubIrp->UserBuffer = &XferPacket;
            Status = IoCallDriver(CommonDeviceExtension->HidDeviceExtension.NextDeviceObject, SubIrp);
            if (Status == STATUS_PENDING)
            {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                Status = IoStatusBlock.Status;
            }
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
        case IOCTL_HID_SET_FEATURE:
        {
            PIRP SubIrp;
            KEVENT Event;
            IO_STATUS_BLOCK IoStatusBlock;
            HID_XFER_PACKET XferPacket;
            NTSTATUS Status;
            PHIDP_REPORT_IDS ReportDescription;

            if (IoStack->Parameters.DeviceIoControl.InputBufferLength < 1)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_PARAMETER;
            }
            ReportDescription = HidClassPDO_GetReportDescriptionByReportID(&PDODeviceExtension->Common.DeviceDescription, ((PUCHAR)Irp->AssociatedIrp.SystemBuffer)[0]);
            if (!ReportDescription || IoStack->Parameters.DeviceIoControl.InputBufferLength < ReportDescription->FeatureLength)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_PARAMETER;
            }

            RtlZeroMemory(&XferPacket, sizeof(XferPacket));
            XferPacket.reportBufferLen = ReportDescription->FeatureLength;
            XferPacket.reportBuffer = Irp->AssociatedIrp.SystemBuffer;
            XferPacket.reportId = XferPacket.reportBuffer[0];

            KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
            SubIrp = IoBuildDeviceIoControlRequest(
                IOCTL_HID_SET_FEATURE,
                CommonDeviceExtension->HidDeviceExtension.NextDeviceObject,
                NULL, 0,
                NULL, 0,
                TRUE,
                &Event,
                &IoStatusBlock);
            if (!SubIrp)
            {
                Irp->IoStatus.Status = STATUS_NO_MEMORY;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_NOT_IMPLEMENTED;
            }
            SubIrp->UserBuffer = &XferPacket;
            Status = IoCallDriver(CommonDeviceExtension->HidDeviceExtension.NextDeviceObject, SubIrp);
            if (Status == STATUS_PENDING)
            {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                Status = IoStatusBlock.Status;
            }
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
        case IOCTL_HID_GET_MANUFACTURER_STRING:
        case IOCTL_HID_GET_PRODUCT_STRING:
        case IOCTL_HID_GET_SERIALNUMBER_STRING:
        {
            NTSTATUS Status;
            UINT_PTR StringId;
            UINT_PTR Lang = 0;
            PIO_STACK_LOCATION SubIoStack;
            PIRP SubIrp;

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < 1)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_PARAMETER;
            }

            switch (IoStack->Parameters.DeviceIoControl.IoControlCode)
            {
                case IOCTL_HID_GET_MANUFACTURER_STRING:
                    StringId = HID_STRING_ID_IMANUFACTURER;
                    break;
                case IOCTL_HID_GET_SERIALNUMBER_STRING:
                    StringId = HID_STRING_ID_ISERIALNUMBER;
                    break;
                default: //IOCTL_HID_GET_PRODUCT_STRING
                    StringId = HID_STRING_ID_IPRODUCT;
                    break;
            }

            SubIrp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
            if (!SubIrp)
            {
                Irp->IoStatus.Status = STATUS_NO_MEMORY;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            // Get next stack location
            SubIoStack = IoGetNextIrpStackLocation(SubIrp);

            // Setup request
            SubIoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            SubIoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_GET_STRING;
            SubIoStack->Parameters.DeviceIoControl.OutputBufferLength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
            SubIoStack->Parameters.DeviceIoControl.InputBufferLength = 0;
            SubIoStack->Parameters.DeviceIoControl.Type3InputBuffer = (PVOID)((Lang << 16) | StringId);
            SubIrp->UserBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

            Status = HidClassFDO_DispatchRequestSynchronous(DeviceObject, SubIrp);

            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = SubIrp->IoStatus.Information;
            IoFreeIrp(SubIrp);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
        default:
        {
            DPRINT1("[HIDCLASS] DeviceControl IoControlCode 0x%x not implemented\n", IoStack->Parameters.DeviceIoControl.IoControlCode);
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_IMPLEMENTED;
        }
    }
}

NTSTATUS
NTAPI
HidClass_InternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    UNIMPLEMENTED;
    ASSERT(FALSE);
    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidClass_Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHIDCLASS_COMMON_DEVICE_EXTENSION CommonDeviceExtension;
    CommonDeviceExtension = DeviceObject->DeviceExtension;

    if (CommonDeviceExtension->IsFDO)
    {
        IoCopyCurrentIrpStackLocationToNext(Irp);
        return HidClassFDO_DispatchRequest(DeviceObject, Irp);
    }
    else
    {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }
}

NTSTATUS
NTAPI
HidClass_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHIDCLASS_COMMON_DEVICE_EXTENSION CommonDeviceExtension;

    //
    // get common device extension
    //
    CommonDeviceExtension = DeviceObject->DeviceExtension;

    //
    // check type of device object
    //
    if (CommonDeviceExtension->IsFDO)
    {
        //
        // handle request
        //
        return HidClassFDO_PnP(DeviceObject, Irp);
    }
    else
    {
        //
        // handle request
        //
        return HidClassPDO_PnP(DeviceObject, Irp);
    }
}

NTSTATUS
NTAPI
HidClass_DispatchDefault(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHIDCLASS_COMMON_DEVICE_EXTENSION CommonDeviceExtension;

    //
    // get common device extension
    //
    CommonDeviceExtension = DeviceObject->DeviceExtension;

    //
    // FIXME: support PDO
    //
    ASSERT(CommonDeviceExtension->IsFDO == TRUE);

    //
    // skip current irp stack location
    //
    IoSkipCurrentIrpStackLocation(Irp);

    //
    // dispatch to lower device object
    //
    return IoCallDriver(CommonDeviceExtension->HidDeviceExtension.NextDeviceObject, Irp);
}

NTSTATUS
NTAPI
HidClassDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DPRINT("[HIDCLASS] Dispatch Major %x Minor %x\n", IoStack->MajorFunction, IoStack->MinorFunction);

    //
    // dispatch request based on major function
    //
    switch (IoStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            return HidClass_Create(DeviceObject, Irp);
        case IRP_MJ_CLOSE:
            return HidClass_Close(DeviceObject, Irp);
        case IRP_MJ_READ:
            return HidClass_Read(DeviceObject, Irp);
        case IRP_MJ_WRITE:
            return HidClass_Write(DeviceObject, Irp);
        case IRP_MJ_DEVICE_CONTROL:
            return HidClass_DeviceControl(DeviceObject, Irp);
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
           return HidClass_InternalDeviceControl(DeviceObject, Irp);
        case IRP_MJ_POWER:
            return HidClass_Power(DeviceObject, Irp);
        case IRP_MJ_PNP:
            return HidClass_PnP(DeviceObject, Irp);
        default:
            return HidClass_DispatchDefault(DeviceObject, Irp);
    }
}

NTSTATUS
NTAPI
HidRegisterMinidriver(
    IN PHID_MINIDRIVER_REGISTRATION MinidriverRegistration)
{
    NTSTATUS Status;
    PHIDCLASS_DRIVER_EXTENSION DriverExtension;

    /* check if the version matches */
    if (MinidriverRegistration->Revision > HID_REVISION)
    {
        /* revision mismatch */
        ASSERT(FALSE);
        return STATUS_REVISION_MISMATCH;
    }

    /* now allocate the driver object extension */
    Status = IoAllocateDriverObjectExtension(MinidriverRegistration->DriverObject,
                                             ClientIdentificationAddress,
                                             sizeof(HIDCLASS_DRIVER_EXTENSION),
                                             (PVOID *)&DriverExtension);
    if (!NT_SUCCESS(Status))
    {
        /* failed to allocate driver extension */
        ASSERT(FALSE);
        return Status;
    }

    /* zero driver extension */
    RtlZeroMemory(DriverExtension, sizeof(HIDCLASS_DRIVER_EXTENSION));

    /* init driver extension */
    DriverExtension->DriverObject = MinidriverRegistration->DriverObject;
    DriverExtension->DeviceExtensionSize = MinidriverRegistration->DeviceExtensionSize;
    DriverExtension->DevicesArePolled = MinidriverRegistration->DevicesArePolled;
    DriverExtension->AddDevice = MinidriverRegistration->DriverObject->DriverExtension->AddDevice;
    DriverExtension->DriverUnload = MinidriverRegistration->DriverObject->DriverUnload;

    /* copy driver dispatch routines */
    RtlCopyMemory(DriverExtension->MajorFunction,
                  MinidriverRegistration->DriverObject->MajorFunction,
                  sizeof(PDRIVER_DISPATCH) * (IRP_MJ_MAXIMUM_FUNCTION + 1));

    /* initialize lock */
    KeInitializeSpinLock(&DriverExtension->Lock);

    /* now replace dispatch routines */
    DriverExtension->DriverObject->DriverExtension->AddDevice = HidClassAddDevice;
    DriverExtension->DriverObject->DriverUnload = HidClassDriverUnload;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_CREATE] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_CLOSE] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_READ] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_WRITE] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_POWER] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_PNP] = HidClassDispatch;

    /* done */
    return STATUS_SUCCESS;
}

/*
 * PROJECT:     ReactOS Universal Serial Bus Human Interface Device Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/hid/hidclass/fdo.c
 * PURPOSE:     HID Class Driver
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

#include <wdmguid.h>

#define NDEBUG
#include <debug.h>

PHIDP_COLLECTION_DESC
HidClassPDO_GetCollectionDescription(
    PHIDP_DEVICE_DESC DeviceDescription,
    ULONG CollectionNumber)
{
    ULONG Index;

    for(Index = 0; Index < DeviceDescription->CollectionDescLength; Index++)
    {
        if (DeviceDescription->CollectionDesc[Index].CollectionNumber == CollectionNumber)
        {
            //
            // found collection
            //
            return &DeviceDescription->CollectionDesc[Index];
        }
    }

    //
    // failed to find collection
    //
    DPRINT1("[HIDCLASS] GetCollectionDescription CollectionNumber %x not found\n", CollectionNumber);
    ASSERT(FALSE);
    return NULL;
}

PHIDP_REPORT_IDS
HidClassPDO_GetReportDescription(
    PHIDP_DEVICE_DESC DeviceDescription,
    ULONG CollectionNumber)
{
    ULONG Index;

    for (Index = 0; Index < DeviceDescription->ReportIDsLength; Index++)
    {
        if (DeviceDescription->ReportIDs[Index].CollectionNumber == CollectionNumber)
        {
            //
            // found collection
            //
            return &DeviceDescription->ReportIDs[Index];
        }
    }

    //
    // failed to find collection
    //
    DPRINT1("[HIDCLASS] GetReportDescription CollectionNumber %x not found\n", CollectionNumber);
    ASSERT(FALSE);
    return NULL;
}

NTSTATUS
HidClassPDO_HandleQueryDeviceId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    LPWSTR Buffer;
    LPWSTR NewBuffer, Ptr;
    ULONG Length;

    //
    // copy current stack location
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // call mini-driver
    //
    Status = HidClassFDO_DispatchRequestSynchronous(DeviceObject, Irp);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        return Status;
    }

    //
    // get buffer
    //
    Buffer = (LPWSTR)Irp->IoStatus.Information;
    Length = wcslen(Buffer);

    //
    // allocate new buffer
    //
    NewBuffer = ExAllocatePoolWithTag(NonPagedPool, (Length + 1) * sizeof(WCHAR), HIDCLASS_TAG);
    if (!NewBuffer)
    {
        //
        // failed to allocate buffer
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // replace bus
    //
    wcscpy(NewBuffer, L"HID\\");

    //
    // get offset to first '\\'
    //
    Ptr = wcschr(Buffer, L'\\');
    if (Ptr)
    {
        //
        // append result
        //
        wcscat(NewBuffer, Ptr + 1);
    }

    //
    // free old buffer
    //
    ExFreePoolWithTag(Buffer, 0);

    //
    // store result
    //
    DPRINT("NewBuffer %S\n", NewBuffer);
    Irp->IoStatus.Information = (ULONG_PTR)NewBuffer;
    return STATUS_SUCCESS;
}

NTSTATUS
HidClassPDO_HandleQueryHardwareId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    PHIDCLASS_PDO_DEVICE_EXTENSION PDODeviceExtension;
    WCHAR Buffer[200];
    ULONG Offset = 0;
    LPWSTR Ptr;
    PHIDP_COLLECTION_DESC CollectionDescription;

    //
    // get device extension
    //
    PDODeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    //
    // copy current stack location
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // call mini-driver
    //
    Status = HidClassFDO_DispatchRequestSynchronous(DeviceObject, Irp);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        return Status;
    }

    if (PDODeviceExtension->Common.DeviceDescription.CollectionDescLength > 1)
    {
        //
        // multi-tlc device
        //
        Offset = swprintf(&Buffer[Offset], L"HID\\Vid_%04x&Pid_%04x&Rev_%04x&Col%02x", PDODeviceExtension->Common.Attributes.VendorID, PDODeviceExtension->Common.Attributes.ProductID, PDODeviceExtension->Common.Attributes.VersionNumber, PDODeviceExtension->CollectionNumber) + 1;
        Offset += swprintf(&Buffer[Offset], L"HID\\Vid_%04x&Pid_%04x&Col%02x", PDODeviceExtension->Common.Attributes.VendorID, PDODeviceExtension->Common.Attributes.ProductID, PDODeviceExtension->CollectionNumber) + 1;
    }
    else
    {
        //
        // single tlc device
        //
        Offset = swprintf(&Buffer[Offset], L"HID\\Vid_%04x&Pid_%04x&Rev_%04x", PDODeviceExtension->Common.Attributes.VendorID, PDODeviceExtension->Common.Attributes.ProductID, PDODeviceExtension->Common.Attributes.VersionNumber) + 1;
        Offset += swprintf(&Buffer[Offset], L"HID\\Vid_%04x&Pid_%04x", PDODeviceExtension->Common.Attributes.VendorID, PDODeviceExtension->Common.Attributes.ProductID) + 1;
    }

    //
    // get collection description
    //
    CollectionDescription = HidClassPDO_GetCollectionDescription(&PDODeviceExtension->Common.DeviceDescription, PDODeviceExtension->CollectionNumber);
    ASSERT(CollectionDescription);

    if (CollectionDescription->UsagePage == HID_USAGE_PAGE_GENERIC)
    {
        switch (CollectionDescription->Usage)
        {
            case HID_USAGE_GENERIC_POINTER:
            case HID_USAGE_GENERIC_MOUSE:
                //
                // Pointer / Mouse
                //
                Offset += swprintf(&Buffer[Offset], L"HID_DEVICE_SYSTEM_MOUSE") + 1;
                break;
            case HID_USAGE_GENERIC_GAMEPAD:
            case HID_USAGE_GENERIC_JOYSTICK:
                //
                // Joystick / Gamepad
                //
                Offset += swprintf(&Buffer[Offset], L"HID_DEVICE_SYSTEM_GAME") + 1;
                break;
            case HID_USAGE_GENERIC_KEYBOARD:
            case HID_USAGE_GENERIC_KEYPAD:
                //
                // Keyboard / Keypad
                //
                Offset += swprintf(&Buffer[Offset], L"HID_DEVICE_SYSTEM_KEYBOARD") + 1;
                break;
            case HID_USAGE_GENERIC_SYSTEM_CTL:
                //
                // System Control
                //
                Offset += swprintf(&Buffer[Offset], L"HID_DEVICE_SYSTEM_CONTROL") + 1;
                break;
        }
    }
    else if (CollectionDescription->UsagePage  == HID_USAGE_PAGE_CONSUMER && CollectionDescription->Usage == HID_USAGE_CONSUMERCTRL)
    {
        //
        // Consumer Audio Control
        //
        Offset += swprintf(&Buffer[Offset], L"HID_DEVICE_SYSTEM_CONSUMER") + 1;
    }

    //
    // add HID_DEVICE_UP:0001_U:0002'
    //
    Offset += swprintf(&Buffer[Offset], L"HID_DEVICE_UP:%04x_U:%04x", CollectionDescription->UsagePage, CollectionDescription->Usage) + 1;

    //
    // add HID
    //
    Offset +=swprintf(&Buffer[Offset], L"HID_DEVICE") + 1;

    //
    // free old buffer
    //
    ExFreePoolWithTag((PVOID)Irp->IoStatus.Information, 0);

    //
    // allocate buffer
    //
    Ptr = ExAllocatePoolWithTag(NonPagedPool, (Offset + 1) * sizeof(WCHAR), HIDCLASS_TAG);
    if (!Ptr)
    {
        //
        // no memory
        //
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // copy buffer
    //
    RtlCopyMemory(Ptr, Buffer, Offset * sizeof(WCHAR));
    Ptr[Offset] = UNICODE_NULL;

    //
    // store result
    //
    Irp->IoStatus.Information = (ULONG_PTR)Ptr;
    return STATUS_SUCCESS;
}

NTSTATUS
HidClassPDO_HandleQueryInstanceId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    LPWSTR Buffer;
    PHIDCLASS_PDO_DEVICE_EXTENSION PDODeviceExtension;

    //
    // get device extension
    //
    PDODeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    //
    // allocate buffer
    //
    Buffer = ExAllocatePoolWithTag(NonPagedPool, 5 * sizeof(WCHAR), HIDCLASS_TAG);
    if (!Buffer)
    {
        //
        // failed
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // write device id
    //
    swprintf(Buffer, L"%04x", PDODeviceExtension->CollectionNumber);
    Irp->IoStatus.Information = (ULONG_PTR)Buffer;

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
HidClassPDO_HandleQueryCompatibleId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    LPWSTR Buffer;

    Buffer = ExAllocatePoolWithTag(NonPagedPool, 2 * sizeof(WCHAR), HIDCLASS_TAG);
    if (!Buffer)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // zero buffer
    //
    Buffer[0] = 0;
    Buffer[1] = 0;

    //
    // store result
    //
    Irp->IoStatus.Information = (ULONG_PTR)Buffer;
    return STATUS_SUCCESS;
}

NTSTATUS
HidClassPDO_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHIDCLASS_PDO_DEVICE_EXTENSION PDODeviceExtension;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PPNP_BUS_INFORMATION BusInformation;
    PDEVICE_RELATIONS DeviceRelation;
    ULONG Index, bFound;

    //
    // get device extension
    //
    PDODeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // handle request
    //
    switch (IoStack->MinorFunction)
    {
        case IRP_MN_QUERY_ID:
        {
            if (IoStack->Parameters.QueryId.IdType == BusQueryDeviceID)
            {
                //
                // handle query device id
                //
                Status = HidClassPDO_HandleQueryDeviceId(DeviceObject, Irp);
                break;
            }
            else if (IoStack->Parameters.QueryId.IdType == BusQueryHardwareIDs)
            {
                //
                // handle instance id
                //
                Status = HidClassPDO_HandleQueryHardwareId(DeviceObject, Irp);
                break;
            }
            else if (IoStack->Parameters.QueryId.IdType == BusQueryInstanceID)
            {
                //
                // handle instance id
                //
                Status = HidClassPDO_HandleQueryInstanceId(DeviceObject, Irp);
                break;
            }
            else if (IoStack->Parameters.QueryId.IdType == BusQueryCompatibleIDs)
            {
                //
                // handle instance id
                //
                Status = HidClassPDO_HandleQueryCompatibleId(DeviceObject, Irp);
                break;
            }

            DPRINT1("[HIDCLASS]: IRP_MN_QUERY_ID IdType %x unimplemented\n", IoStack->Parameters.QueryId.IdType);
            Status = STATUS_NOT_SUPPORTED;
            Irp->IoStatus.Information = 0;
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            if (IoStack->Parameters.DeviceCapabilities.Capabilities == NULL)
            {
                //
                // invalid request
                //
                Status = STATUS_DEVICE_CONFIGURATION_ERROR;
                break;
            }

            //
            // copy capabilities
            //
            RtlCopyMemory(IoStack->Parameters.DeviceCapabilities.Capabilities,
                          &PDODeviceExtension->Capabilities,
                          sizeof(DEVICE_CAPABILITIES));
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            //
            //
            //
            BusInformation = ExAllocatePoolWithTag(NonPagedPool, sizeof(PNP_BUS_INFORMATION), HIDCLASS_TAG);

            //
            // fill in result
            //
            RtlCopyMemory(&BusInformation->BusTypeGuid, &GUID_BUS_TYPE_HID, sizeof(GUID));
            BusInformation->LegacyBusType = PNPBus;
            BusInformation->BusNumber = 0; //FIXME

            //
            // store result
            //
            Irp->IoStatus.Information = (ULONG_PTR)BusInformation;
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
        {
            //
            // FIXME set flags when driver fails / disabled
            //
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            //
            // only target relations are supported
            //
            if (IoStack->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation)
            {
                //
                // not supported
                //
                Status = Irp->IoStatus.Status;
                break;
            }

            //
            // allocate device relations
            //
            DeviceRelation = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEVICE_RELATIONS), HIDCLASS_TAG);
            if (!DeviceRelation)
            {
                //
                // no memory
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            //
            // init device relation
            //
            DeviceRelation->Count = 1;
            DeviceRelation->Objects[0] = DeviceObject;
            ObReferenceObject(DeviceRelation->Objects[0]);

            //
            // store result
            //
            Irp->IoStatus.Information = (ULONG_PTR)DeviceRelation;
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_START_DEVICE:
        {
            //
            // FIXME: support polled devices
            //
            ASSERT(PDODeviceExtension->Common.DriverExtension->DevicesArePolled == FALSE);

            //
            // now register the device interface
            //
            Status = IoRegisterDeviceInterface(PDODeviceExtension->Common.HidDeviceExtension.PhysicalDeviceObject,
                                               &GUID_DEVINTERFACE_HID,
                                               NULL,
                                               &PDODeviceExtension->DeviceInterface);
            DPRINT("[HIDCLASS] IoRegisterDeviceInterfaceState Status %x\n", Status);
            if (NT_SUCCESS(Status))
            {
                //
                // enable device interface
                //
                Status = IoSetDeviceInterfaceState(&PDODeviceExtension->DeviceInterface, TRUE);
                DPRINT("[HIDCLASS] IoSetDeviceInterFaceState %x\n", Status);
            }

            //
            // done
            //
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            /* Disable the device interface */
            if (PDODeviceExtension->DeviceInterface.Length != 0)
                IoSetDeviceInterfaceState(&PDODeviceExtension->DeviceInterface, FALSE);

            //
            // remove us from the fdo's pdo list
            //
            bFound = FALSE;
            for (Index = 0; Index < PDODeviceExtension->FDODeviceExtension->DeviceRelations->Count; Index++)
            {
                if (PDODeviceExtension->FDODeviceExtension->DeviceRelations->Objects[Index] == DeviceObject)
                {
                    //
                    // remove us
                    //
                    bFound = TRUE;
                    PDODeviceExtension->FDODeviceExtension->DeviceRelations->Objects[Index] = NULL;
                    break;
                }
            }

            /* Complete the IRP */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            if (bFound)
            {
                /* Delete our device object*/
                IoDeleteDevice(DeviceObject);
            }

            return STATUS_SUCCESS;
        }
        case IRP_MN_QUERY_INTERFACE:
        {
            DPRINT1("[HIDCLASS] PDO IRP_MN_QUERY_INTERFACE not implemented\n");

            //
            // do nothing
            //
            Status = Irp->IoStatus.Status;
            break;
        }
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        {
            //
            // no/op
            //
#if 0
            Status = STATUS_SUCCESS;
#else
            DPRINT1("Denying removal of HID device due to IRP cancellation bugs\n");
            Status = STATUS_UNSUCCESSFUL;
#endif
            break;
        }
        default:
        {
            //
            // do nothing
            //
            Status = Irp->IoStatus.Status;
            break;
        }
    }

    //
    // complete request
    //
    if (Status != STATUS_PENDING)
    {
        //
        // store result
        //
        Irp->IoStatus.Status = Status;

        //
        // complete request
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    //
    // done processing
    //
    return Status;
}

NTSTATUS
HidClassPDO_CreatePDO(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDEVICE_RELATIONS *OutDeviceRelations)
{
    PHIDCLASS_FDO_EXTENSION FDODeviceExtension;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT PDODeviceObject;
    PHIDCLASS_PDO_DEVICE_EXTENSION PDODeviceExtension;
    ULONG Index;
    PDEVICE_RELATIONS DeviceRelations;
    ULONG Length;

    //
    // get device extension
    //
    FDODeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // first allocate device relations
    //
    Length = FIELD_OFFSET(DEVICE_RELATIONS, Objects) + sizeof(PDEVICE_OBJECT) * FDODeviceExtension->Common.DeviceDescription.CollectionDescLength;
    DeviceRelations = ExAllocatePoolWithTag(PagedPool, Length, HIDCLASS_TAG);
    if (!DeviceRelations)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // zero device relations
    //
    RtlZeroMemory(DeviceRelations, Length);

    //
    // let's create a PDO for top level collection
    //
    Index = 0;
    while (Index < FDODeviceExtension->Common.DeviceDescription.CollectionDescLength)
    {
        //
        // let's create the device object
        //
        Status = IoCreateDevice(FDODeviceExtension->Common.DriverExtension->DriverObject,
                                sizeof(HIDCLASS_PDO_DEVICE_EXTENSION),
                                NULL,
                                FILE_DEVICE_UNKNOWN,
                                FILE_AUTOGENERATED_DEVICE_NAME,
                                FALSE,
                                &PDODeviceObject);
        if (!NT_SUCCESS(Status))
        {
            //
            // failed to create device
            //
            DPRINT1("[HIDCLASS] Failed to create PDO %x\n", Status);
            break;
        }

        //
        // patch stack size
        //
        PDODeviceObject->StackSize = DeviceObject->StackSize + 1;

        //
        // get device extension
        //
        PDODeviceExtension = PDODeviceObject->DeviceExtension;

        //
        // init device extension
        //
        PDODeviceExtension->Common.HidDeviceExtension.MiniDeviceExtension = FDODeviceExtension->Common.HidDeviceExtension.MiniDeviceExtension;
        PDODeviceExtension->Common.HidDeviceExtension.NextDeviceObject = FDODeviceExtension->Common.HidDeviceExtension.NextDeviceObject;
        PDODeviceExtension->Common.HidDeviceExtension.PhysicalDeviceObject = FDODeviceExtension->Common.HidDeviceExtension.PhysicalDeviceObject;
        PDODeviceExtension->Common.IsFDO = FALSE;
        PDODeviceExtension->FDODeviceExtension = FDODeviceExtension;
        PDODeviceExtension->FDODeviceObject = DeviceObject;
        PDODeviceExtension->Common.DriverExtension = FDODeviceExtension->Common.DriverExtension;
        PDODeviceExtension->CollectionNumber = FDODeviceExtension->Common.DeviceDescription.CollectionDesc[Index].CollectionNumber;

        //
        // copy device data
        //
        RtlCopyMemory(&PDODeviceExtension->Common.Attributes, &FDODeviceExtension->Common.Attributes, sizeof(HID_DEVICE_ATTRIBUTES));
        RtlCopyMemory(&PDODeviceExtension->Common.DeviceDescription, &FDODeviceExtension->Common.DeviceDescription, sizeof(HIDP_DEVICE_DESC));
        RtlCopyMemory(&PDODeviceExtension->Capabilities, &FDODeviceExtension->Capabilities, sizeof(DEVICE_CAPABILITIES));

        //
        // set device flags
        //
        PDODeviceObject->Flags |= DO_MAP_IO_BUFFER;

        //
        // device is initialized
        //
        PDODeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        //
        // store device object in device relations
        //
        DeviceRelations->Objects[Index] = PDODeviceObject;
        DeviceRelations->Count++;

        //
        // move to next
        //
        Index++;

    }


    //
    // check if creating succeeded
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        for (Index = 0; Index < DeviceRelations->Count; Index++)
        {
            //
            // delete device
            //
            IoDeleteDevice(DeviceRelations->Objects[Index]);
        }

        //
        // free device relations
        //
        ExFreePoolWithTag(DeviceRelations, HIDCLASS_TAG);
        return Status;
    }

    //
    // store device relations
    //
    *OutDeviceRelations = DeviceRelations;

    //
    // done
    //
    return STATUS_SUCCESS;
}

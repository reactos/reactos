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
    NewBuffer = (LPWSTR)ExAllocatePool(NonPagedPool, (Length + 1) * sizeof(WCHAR));
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
    ExFreePool(Buffer);

    //
    // store result
    //
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
    WCHAR Buffer[100];
    ULONG Offset = 0;
    LPWSTR Ptr;

    //
    // get device extension
    //
   PDODeviceExtension = (PHIDCLASS_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
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

    //
    // store hardware ids
    //
    Offset = swprintf(&Buffer[Offset], L"HID\\Vid_%04x&Pid_%04x&Rev_%04x", PDODeviceExtension->Common.Attributes.VendorID, PDODeviceExtension->Common.Attributes.ProductID, PDODeviceExtension->Common.Attributes.VersionNumber) + 1;
    Offset += swprintf(&Buffer[Offset], L"HID\\Vid_%04x&Pid_%04x", PDODeviceExtension->Common.Attributes.VendorID, PDODeviceExtension->Common.Attributes.ProductID) + 1;

    if (PDODeviceExtension->Common.DeviceDescription.CollectionDesc[PDODeviceExtension->CollectionIndex].UsagePage == HID_USAGE_PAGE_GENERIC)
    {
        switch(PDODeviceExtension->Common.DeviceDescription.CollectionDesc[PDODeviceExtension->CollectionIndex].Usage)
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
    else if (PDODeviceExtension->Common.DeviceDescription.CollectionDesc[PDODeviceExtension->CollectionIndex].UsagePage == HID_USAGE_PAGE_CONSUMER && PDODeviceExtension->Common.DeviceDescription.CollectionDesc[PDODeviceExtension->CollectionIndex].Usage == HID_USAGE_CONSUMERCTRL)
    {
        //
        // Consumer Audio Control
        //
        Offset += swprintf(&Buffer[Offset], L"HID_DEVICE_SYSTEM_CONSUMER") + 1;
    }

    //
    // FIXME: add 'HID_DEVICE_UP:0001_U:0002'
    //

    //
    // add HID
    //
    Offset +=swprintf(&Buffer[Offset], L"HID_DEVICE") + 1;

    //
    // free old buffer
    //
    ExFreePool((PVOID)Irp->IoStatus.Information);

    //
    // allocate buffer
    //
    Ptr = (LPWSTR)ExAllocatePool(NonPagedPool, (Offset +1)* sizeof(WCHAR));
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
    NTSTATUS Status;

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
    DPRINT1("HidClassPDO_HandleQueryInstanceId Buffer %S\n", Irp->IoStatus.Information);
    //
    //TODO implement instance id
    // example:
    // HID\VID_045E&PID_0047\8&1A0700BC&0&0000
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
HidClassPDO_HandleQueryCompatibleId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;

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
    // FIXME: implement me
    //
    return STATUS_NOT_IMPLEMENTED;
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

    //
    // get device extension
    //
    PDODeviceExtension = (PHIDCLASS_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // handle request
    //
    switch(IoStack->MinorFunction)
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
            }

            //
            // copy capabilities
            //
            RtlCopyMemory(IoStack->Parameters.DeviceCapabilities.Capabilities, &PDODeviceExtension->Capabilities, sizeof(DEVICE_CAPABILITIES));
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            //
            //
            //
            BusInformation = (PPNP_BUS_INFORMATION)ExAllocatePool(NonPagedPool, sizeof(PNP_BUS_INFORMATION));
 
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
            DeviceRelation = (PDEVICE_RELATIONS)ExAllocatePool(NonPagedPool, sizeof(DEVICE_RELATIONS));
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
            Status = IoRegisterDeviceInterface(PDODeviceExtension->Common.HidDeviceExtension.PhysicalDeviceObject, &GUID_DEVINTERFACE_HID, NULL, &PDODeviceExtension->DeviceInterface);
            if (NT_SUCCESS(Status))
            {
                //
                // enable device interface
                //
                Status = IoSetDeviceInterfaceState(&PDODeviceExtension->DeviceInterface, TRUE);
            }
            ASSERT(Status == STATUS_SUCCESS);

            //
            // break
            //
            break;
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            DPRINT1("[HIDCLASS] PDO IRP_MN_REMOVE_DEVICE not implemented\n");
            ASSERT(FALSE);

            //
            // do nothing
            //
            Status = STATUS_SUCCESS; //Irp->IoStatus.Status;
            break;
        }
        case IRP_MN_QUERY_INTERFACE:
        {
            DPRINT1("[HIDCLASS] PDO IRP_MN_QUERY_INTERFACE not implemented\n");
            ASSERT(FALSE);

            //
            // do nothing
            //
            Status = Irp->IoStatus.Status;
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
    IN PDEVICE_OBJECT DeviceObject)
{
    PHIDCLASS_FDO_EXTENSION FDODeviceExtension;
    NTSTATUS Status;
    PDEVICE_OBJECT PDODeviceObject;
    PHIDCLASS_PDO_DEVICE_EXTENSION PDODeviceExtension;

    //
    // get device extension
    //
    FDODeviceExtension = (PHIDCLASS_FDO_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // lets create the device object
    //
    Status = IoCreateDevice(FDODeviceExtension->Common.DriverExtension->DriverObject, sizeof(HIDCLASS_PDO_DEVICE_EXTENSION), NULL, FILE_DEVICE_UNKNOWN, FILE_AUTOGENERATED_DEVICE_NAME, FALSE, &PDODeviceObject);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to create device
        //
        DPRINT1("[HIDCLASS] Failed to create device %x\n", Status);
        return Status;
    }

    //
    // patch stack size
    //
    PDODeviceObject->StackSize = DeviceObject->StackSize + 1;

    //
    // get device extension
    //
    PDODeviceExtension = (PHIDCLASS_PDO_DEVICE_EXTENSION)PDODeviceObject->DeviceExtension;

    //
    // init device extension
    //
    PDODeviceExtension->Common.HidDeviceExtension.MiniDeviceExtension = FDODeviceExtension->Common.HidDeviceExtension.MiniDeviceExtension;
    PDODeviceExtension->Common.HidDeviceExtension.NextDeviceObject = FDODeviceExtension->Common.HidDeviceExtension.NextDeviceObject;
    PDODeviceExtension->Common.HidDeviceExtension.PhysicalDeviceObject = FDODeviceExtension->Common.HidDeviceExtension.PhysicalDeviceObject;
    PDODeviceExtension->Common.IsFDO = FALSE;
    PDODeviceExtension->FDODeviceObject = DeviceObject;
    PDODeviceExtension->Common.DriverExtension = FDODeviceExtension->Common.DriverExtension;
    RtlCopyMemory(&PDODeviceExtension->Common.Attributes, &FDODeviceExtension->Common.Attributes, sizeof(HID_DEVICE_ATTRIBUTES));
    RtlCopyMemory(&PDODeviceExtension->Common.DeviceDescription, &FDODeviceExtension->Common.DeviceDescription, sizeof(HIDP_DEVICE_DESC));
    RtlCopyMemory(&PDODeviceExtension->Capabilities, &FDODeviceExtension->Capabilities, sizeof(DEVICE_CAPABILITIES));

    //
    // FIXME: support composite devices
    //
    PDODeviceExtension->CollectionIndex = 0;
    ASSERT(PDODeviceExtension->Common.DeviceDescription.CollectionDescLength == 1);

    //
    // store in device relations struct
    //
    FDODeviceExtension->DeviceRelations.Count = 1;
    FDODeviceExtension->DeviceRelations.Objects[0] = PDODeviceObject;

    //
    // set device flags
    //
    PDODeviceObject->Flags |= DO_MAP_IO_BUFFER;

    //
    // device is initialized
    //
    PDODeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    ObReferenceObject(PDODeviceObject);

    //
    // done
    //
    return STATUS_SUCCESS;
}
/*
 * PROJECT:     ReactOS USB Port Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBPort I/O control functions
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbport.h"

#define NDEBUG
#include <debug.h>

VOID
NTAPI
USBPORT_UserGetHcName(IN PDEVICE_OBJECT FdoDevice,
                      IN PUSBUSER_CONTROLLER_UNICODE_NAME ControllerName,
                      IN PUSB_UNICODE_NAME UnicodeName)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    ULONG Length;
    NTSTATUS Status;
    ULONG ResultLength;

    DPRINT("USBPORT_UserGetHcName: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;

    Length = ControllerName->Header.RequestBufferLength -
             sizeof(USBUSER_CONTROLLER_UNICODE_NAME);

    RtlZeroMemory(UnicodeName, Length);

    Status = IoGetDeviceProperty(FdoExtension->CommonExtension.LowerPdoDevice,
                                 DevicePropertyDriverKeyName,
                                 Length,
                                 UnicodeName->String,
                                 &ResultLength);

    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            ControllerName->Header.UsbUserStatusCode = UsbUserBufferTooSmall;
        }
        else
        {
            ControllerName->Header.UsbUserStatusCode = UsbUserInvalidParameter;
        }
    }
    else
    {
        ControllerName->Header.UsbUserStatusCode = UsbUserSuccess;
        UnicodeName->Length = ResultLength + sizeof(UNICODE_NULL);
    }

    ControllerName->Header.ActualBufferLength = sizeof(USBUSER_CONTROLLER_UNICODE_NAME) +
                                                ResultLength;
}

NTSTATUS
NTAPI
USBPORT_GetSymbolicName(IN PDEVICE_OBJECT RootHubPdo,
                        IN PUNICODE_STRING DestinationString)
{
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PUNICODE_STRING RootHubName;
    PWCHAR Buffer;
    SIZE_T LengthName;
    SIZE_T Length;
    PWSTR SourceString;
    WCHAR Character;

    DPRINT("USBPORT_GetSymbolicName: ... \n");

    PdoExtension = RootHubPdo->DeviceExtension;
    RootHubName = &PdoExtension->CommonExtension.SymbolicLinkName;
    Buffer = RootHubName->Buffer;

    if (!Buffer)
    {
        return STATUS_UNSUCCESSFUL;
    }

    LengthName = RootHubName->Length;

    SourceString = ExAllocatePoolWithTag(PagedPool, LengthName, USB_PORT_TAG);

    if (!SourceString)
    {
        RtlInitUnicodeString(DestinationString, NULL);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(SourceString, LengthName);

    if (*Buffer == L'\\')
    {
         Buffer += 1;

        if (*Buffer == L'\\')
        {
            Buffer += 1;
            goto Exit;
        }

        Character = *Buffer;

        do
        {
            if (Character == UNICODE_NULL)
            {
                break;
            }

            Buffer += 1;
            Character = *Buffer;
        }
        while (*Buffer != L'\\');

        if (*Buffer == L'\\')
        {
            Buffer += 1;
        }

Exit:
        Length = (ULONG_PTR)Buffer - (ULONG_PTR)RootHubName->Buffer;
    }
    else
    {
        Length = 0;
    }

    RtlCopyMemory(SourceString,
                  (PVOID)((ULONG_PTR)RootHubName->Buffer + Length),
                  RootHubName->Length - Length);

    RtlInitUnicodeString(DestinationString, SourceString);

    DPRINT("USBPORT_RegisterDeviceInterface: DestinationString  - %wZ\n",
           DestinationString);

    return STATUS_SUCCESS;
}

VOID
NTAPI
USBPORT_UserGetRootHubName(IN PDEVICE_OBJECT FdoDevice,
                           IN PUSBUSER_CONTROLLER_UNICODE_NAME RootHubName,
                           IN PUSB_UNICODE_NAME UnicodeName)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    UNICODE_STRING UnicodeString;
    ULONG Length;
    ULONG ResultLength = 0;
    NTSTATUS Status;

    DPRINT("USBPORT_UserGetRootHubName: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;

    Length = RootHubName->Header.RequestBufferLength -
             sizeof(USBUSER_CONTROLLER_UNICODE_NAME);

    RtlZeroMemory(UnicodeName, Length);

    Status = USBPORT_GetSymbolicName(FdoExtension->RootHubPdo, &UnicodeString);

    if (NT_SUCCESS(Status))
    {
        ResultLength = UnicodeString.Length;

        if (UnicodeString.Length > Length)
        {
            UnicodeString.Length = Length;
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        if (UnicodeString.Length)
        {
            RtlCopyMemory(UnicodeName->String,
                          UnicodeString.Buffer,
                          UnicodeString.Length);
        }

        RtlFreeUnicodeString(&UnicodeString);
    }

    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            RootHubName->Header.UsbUserStatusCode = UsbUserBufferTooSmall;
        }
        else
        {
            RootHubName->Header.UsbUserStatusCode = UsbUserInvalidParameter;
        }
    }
    else
    {
        RootHubName->Header.UsbUserStatusCode = UsbUserSuccess;
        UnicodeName->Length = ResultLength + sizeof(UNICODE_NULL);
    }

    RootHubName->Header.ActualBufferLength = sizeof(USBUSER_CONTROLLER_UNICODE_NAME) +
                                             ResultLength;
}

NTSTATUS
NTAPI
USBPORT_GetUnicodeName(IN PDEVICE_OBJECT FdoDevice,
                       IN PIRP Irp,
                       IN PULONG_PTR Information)
{
    PUSB_HCD_DRIVERKEY_NAME DriverKey;
    PIO_STACK_LOCATION IoStack;
    ULONG OutputBufferLength;
    ULONG IoControlCode;
    ULONG Length;
    PUSBUSER_CONTROLLER_UNICODE_NAME ControllerName;
    PUSB_UNICODE_NAME UnicodeName;
    ULONG ActualLength;

    DPRINT("USBPORT_GetUnicodeName: ... \n");

    *Information = 0;
    DriverKey = Irp->AssociatedIrp.SystemBuffer;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    OutputBufferLength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
    IoControlCode = IoStack->Parameters.DeviceIoControl.IoControlCode;

    if (OutputBufferLength < sizeof(USB_UNICODE_NAME))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    Length = sizeof(USBUSER_CONTROLLER_UNICODE_NAME);

    while (TRUE)
    {
        ControllerName = ExAllocatePoolWithTag(PagedPool,
                                               Length,
                                               USB_PORT_TAG);

        if (!ControllerName)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(ControllerName, Length);

        ControllerName->Header.RequestBufferLength = Length;
        UnicodeName = &ControllerName->UnicodeName;

        if (IoControlCode == IOCTL_GET_HCD_DRIVERKEY_NAME)
        {
            ControllerName->Header.UsbUserRequest = USBUSER_GET_CONTROLLER_DRIVER_KEY;
            USBPORT_UserGetHcName(FdoDevice, ControllerName, UnicodeName);
        }
        else
        {
            ControllerName->Header.UsbUserRequest = USBUSER_GET_ROOTHUB_SYMBOLIC_NAME;
            USBPORT_UserGetRootHubName(FdoDevice, ControllerName, UnicodeName);
        }

        if (ControllerName->Header.UsbUserStatusCode != UsbUserBufferTooSmall)
        {
            break;
        }

        Length = ControllerName->Header.ActualBufferLength;

        ExFreePoolWithTag(ControllerName, USB_PORT_TAG);
    }

    if (ControllerName->Header.UsbUserStatusCode != UsbUserSuccess)
    {
        ExFreePoolWithTag(ControllerName, USB_PORT_TAG);
        return STATUS_UNSUCCESSFUL;
    }

    ActualLength = sizeof(ULONG) + ControllerName->UnicodeName.Length;

    DriverKey->ActualLength = ActualLength;

    if (OutputBufferLength < ActualLength)
    {
        DriverKey->DriverKeyName[0] = UNICODE_NULL;
        *Information = sizeof(USB_UNICODE_NAME);
    }
    else
    {
        RtlCopyMemory(DriverKey->DriverKeyName,
                      ControllerName->UnicodeName.String,
                      ControllerName->UnicodeName.Length);

        *Information = DriverKey->ActualLength;
    }

    ExFreePoolWithTag(ControllerName, USB_PORT_TAG);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBPORT_PdoDeviceControl(IN PDEVICE_OBJECT PdoDevice,
                         IN PIRP Irp)
{
    DPRINT1("USBPORT_PdoDeviceControl: UNIMPLEMENTED. FIXME. \n");
    return 0;
}

NTSTATUS
NTAPI
USBPORT_PdoInternalDeviceControl(IN PDEVICE_OBJECT PdoDevice,
                                 IN PIRP Irp)
{
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PIO_STACK_LOCATION IoStack;
    ULONG IoCtl;
    NTSTATUS Status;

    PdoExtension = PdoDevice->DeviceExtension;
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    IoCtl = IoStack->Parameters.DeviceIoControl.IoControlCode;

    //DPRINT("USBPORT_PdoInternalDeviceControl: PdoDevice - %p, Irp - %p, IoCtl - %x\n",
    //       PdoDevice,
    //       Irp,
    //       IoCtl);

    if (IoCtl == IOCTL_INTERNAL_USB_SUBMIT_URB)
    {
        return USBPORT_HandleSubmitURB(PdoDevice, Irp, URB_FROM_IRP(Irp));
    }

    if (IoCtl == IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO)
    {
        DPRINT("USBPORT_PdoInternalDeviceControl: IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO\n");

        if (IoStack->Parameters.Others.Argument1)
            *(PVOID *)IoStack->Parameters.Others.Argument1 = PdoDevice;

        if (IoStack->Parameters.Others.Argument2)
            *(PVOID *)IoStack->Parameters.Others.Argument2 = PdoDevice;

        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if (IoCtl == IOCTL_INTERNAL_USB_GET_HUB_COUNT)
    {
        DPRINT("USBPORT_PdoInternalDeviceControl: IOCTL_INTERNAL_USB_GET_HUB_COUNT\n");

        if (IoStack->Parameters.Others.Argument1)
        {
            ++*(PULONG)IoStack->Parameters.Others.Argument1;
        }

        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if (IoCtl == IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE)
    {
        DPRINT("USBPORT_PdoInternalDeviceControl: IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE\n");

        if (IoStack->Parameters.Others.Argument1)
        {
            *(PVOID *)IoStack->Parameters.Others.Argument1 = &PdoExtension->DeviceHandle;
        }

        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if (IoCtl == IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION)
    {
        DPRINT("USBPORT_PdoInternalDeviceControl: IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION\n");
        return USBPORT_IdleNotification(PdoDevice, Irp);
    }

    DPRINT("USBPORT_PdoInternalDeviceControl: INVALID INTERNAL DEVICE CONTROL\n");
    Status = STATUS_INVALID_DEVICE_REQUEST;

Exit:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
USBPORT_FdoDeviceControl(IN PDEVICE_OBJECT FdoDevice,
                         IN PIRP Irp)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PIO_STACK_LOCATION IoStack;
    ULONG ControlCode;
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;
    ULONG_PTR Information = 0;

    DPRINT("USBPORT_FdoDeviceControl: Irp - %p\n", Irp);

    FdoExtension = FdoDevice->DeviceExtension;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ControlCode = IoStack->Parameters.DeviceIoControl.IoControlCode;

    switch (ControlCode)
    {
        case IOCTL_USB_DIAGNOSTIC_MODE_ON:
            DPRINT("USBPORT_FdoDeviceControl: IOCTL_USB_DIAGNOSTIC_MODE_ON\n");
            FdoExtension->Flags |= USBPORT_FLAG_DIAGNOSTIC_MODE;
            break;

        case IOCTL_USB_DIAGNOSTIC_MODE_OFF:
            DPRINT("USBPORT_FdoDeviceControl: IOCTL_USB_DIAGNOSTIC_MODE_OFF\n");
            FdoExtension->Flags &= ~USBPORT_FLAG_DIAGNOSTIC_MODE;
            break;

        case IOCTL_USB_GET_NODE_INFORMATION:
            DPRINT1("USBPORT_FdoDeviceControl: IOCTL_USB_GET_NODE_INFORMATION\n");
            Status = USBPORT_GetUnicodeName(FdoDevice, Irp, &Information);
            break;

        case IOCTL_GET_HCD_DRIVERKEY_NAME:
            DPRINT1("USBPORT_FdoDeviceControl: IOCTL_GET_HCD_DRIVERKEY_NAME\n");
            Status = USBPORT_GetUnicodeName(FdoDevice, Irp, &Information);
            break;

        case IOCTL_USB_USER_REQUEST:
            DPRINT1("USBPORT_FdoDeviceControl: IOCTL_USB_USER_REQUEST UNIMPLEMENTED. FIXME\n");
            break;

        default:
            DPRINT1("USBPORT_FdoDeviceControl: Not supported IoControlCode - %x\n",
                    ControlCode);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
NTAPI
USBPORT_FdoInternalDeviceControl(IN PDEVICE_OBJECT FdoDevice,
                                 IN PIRP Irp)
{
    DPRINT1("USBPORT_FdoInternalDeviceControl: UNIMPLEMENTED. FIXME. \n");
    return 0;
}

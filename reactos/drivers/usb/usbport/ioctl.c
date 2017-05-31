#include "usbport.h"

//#define NDEBUG
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

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

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
    PUSHORT Buffer;
    SIZE_T LengthName;
    SIZE_T Length;
    PWSTR SourceString;
    USHORT Symbol;

    DPRINT("USBPORT_GetSymbolicName: ... \n");

    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)RootHubPdo->DeviceExtension;
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

    if (*Buffer == 0x005C) // '\'
    {
         Buffer += 1;

        if (*Buffer == 0x005C)
        {
            Buffer += 1;
            goto Exit;
        }

        Symbol = *Buffer;

        do
        {
            if (Symbol == 0)
            {
                break;
            }

            Buffer += 1;
            Symbol = *Buffer;
        }
        while (*Buffer != 0x005C);

        if (*Buffer == 0x005C)
        {
            Buffer += 1;
        }

Exit:
        Length = (ULONG)Buffer - (ULONG)RootHubName->Buffer;
    }
    else
    {
        Length = 0;
    }

    RtlCopyMemory(SourceString,
                  (PVOID)((ULONG)RootHubName->Buffer + Length),
                  RootHubName->Length - Length);

    RtlInitUnicodeString(DestinationString, (PCWSTR)SourceString);
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
    ULONG ResultLength;
    NTSTATUS Status;

    DPRINT("USBPORT_UserGetRootHubName: ... \n");

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

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
                       IN PULONG Information)
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
    DriverKey = (PUSB_HCD_DRIVERKEY_NAME)Irp->AssociatedIrp.SystemBuffer;

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
        ControllerName = ExAllocatePoolWithTag(PagedPool, Length, USB_PORT_TAG);

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

        ExFreePool(ControllerName);
    }

    if (ControllerName->Header.UsbUserStatusCode != UsbUserSuccess)
    {
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

    ExFreePool(ControllerName);

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
USBPORT_FdoDeviceControl(IN PDEVICE_OBJECT FdoDevice,
                         IN PIRP Irp)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PIO_STACK_LOCATION IoStack;
    ULONG ControlCode;
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;
    ULONG Information = 0;

    DPRINT("USBPORT_FdoDeviceControl: Irp - %p\n", Irp);

    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ControlCode = IoStack->Parameters.DeviceIoControl.IoControlCode;

    switch (ControlCode)
    {
        case IOCTL_USB_DIAGNOSTIC_MODE_ON: // 220400
            DPRINT("USBPORT_FdoDeviceControl: IOCTL_USB_DIAGNOSTIC_MODE_ON\n");
            FdoExtension->Flags |= USBPORT_FLAG_DIAGNOSTIC_MODE;
            break;

        case IOCTL_USB_DIAGNOSTIC_MODE_OFF: // 0x220404
            DPRINT("USBPORT_FdoDeviceControl: IOCTL_USB_DIAGNOSTIC_MODE_OFF\n");
            FdoExtension->Flags &= ~USBPORT_FLAG_DIAGNOSTIC_MODE;
            break;

        case IOCTL_USB_GET_NODE_INFORMATION: // 0x220408
            DPRINT1("USBPORT_FdoDeviceControl: IOCTL_USB_GET_NODE_INFORMATION\n");
            Status = USBPORT_GetUnicodeName(FdoDevice, Irp, &Information);
            break;

        case IOCTL_GET_HCD_DRIVERKEY_NAME: // 0x220424
            DPRINT1("USBPORT_FdoDeviceControl: IOCTL_GET_HCD_DRIVERKEY_NAME\n");
            Status = USBPORT_GetUnicodeName(FdoDevice, Irp, &Information);
            break;

        case IOCTL_USB_USER_REQUEST: // 0x220438
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
USBPORT_FdoScsi(IN PDEVICE_OBJECT FdoDevice,
                IN PIRP Irp)
{
    DPRINT1("USBPORT_FdoScsi: UNIMPLEMENTED. FIXME. \n");
    return 0;
}

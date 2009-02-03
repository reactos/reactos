/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/main.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */
#include "wdmaud.h"

const GUID KSCATEGORY_SYSAUDIO = {0xA7C7A5B1L, 0x5AF3, 0x11D1, {0x9C, 0xED, 0x00, 0xA0, 0x24, 0xBF, 0x04, 0x07}};

NTSTATUS
NTAPI
WdmAudAddDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PDEVICE_OBJECT  PhysicalDeviceObject)
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT NextDeviceObject;
    NTSTATUS Status;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    DPRINT("WdmAudAddDevice called\n");

    Status = IoCreateDevice(DriverObject,
                            sizeof(WDMAUD_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_KS,
                            0,
                            FALSE,
                            &DeviceObject);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateDevice failed with %x\n", Status);
        return Status;
    }

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    RtlZeroMemory(DeviceExtension, sizeof(WDMAUD_DEVICE_EXTENSION));

    InitializeListHead(&DeviceExtension->SysAudioDeviceList);

    Status = KsAllocateDeviceHeader(&DeviceExtension->DeviceHeader, 0, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsAllocateDeviceHeader failed with %x\n", Status);
        IoDeleteDevice(DeviceObject);
        return Status;
    }

    NextDeviceObject = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
    if (NextDeviceObject)
    {
        /// FIXME
        /// KsSetDevicePnpAndBaseObject((KSDEVICE_HEADER)DeviceObject->DeviceExtension, NextDeviceObject, DeviceObject);
    }


    DeviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;
    DeviceObject->Flags &= ~ DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

VOID
NTAPI
WdmAudUnload(
    IN PDRIVER_OBJECT driver)
{
    DPRINT("WdmAudUnload called\n");
}

NTSTATUS
NTAPI
WdmAudPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IrpStack;

    DPRINT("WdmAudPnp called\n");

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    if (IrpStack->MinorFunction == IRP_MN_QUERY_PNP_DEVICE_STATE)
    {
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
        return KsDefaultDispatchPnp(DeviceObject, Irp);
    }
    return KsDefaultDispatchPnp(DeviceObject, Irp);
}

NTSTATUS
WdmAudOpenSysAudioDevice(
    IN LPWSTR DeviceName,
    OUT PHANDLE Handle)
{
    UNICODE_STRING SymbolicLink;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    RtlInitUnicodeString(&SymbolicLink, DeviceName);
    InitializeObjectAttributes(&ObjectAttributes, &SymbolicLink, OBJ_OPENIF | OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = IoCreateFile(Handle,
                          SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          0,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          IO_NO_PARAMETER_CHECKING | IO_FORCE_ACCESS_CHECK);

    return Status;
}

NTSTATUS
NTAPI
DeviceInterfaceChangeCallback(
    IN PVOID NotificationStructure,
    IN PVOID Context)
{
    DEVICE_INTERFACE_CHANGE_NOTIFICATION * Event = (DEVICE_INTERFACE_CHANGE_NOTIFICATION*)NotificationStructure;

    DPRINT1("DeviceInterfaceChangeCallback called %p\n", Event);
    return STATUS_SUCCESS;
}

NTSTATUS
WdmAudOpenSysAudioDeviceInterfaces(
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension,
    IN LPWSTR SymbolicLinkList)
{
    NTSTATUS Status;
    HANDLE Handle;
    SYSAUDIO_ENTRY * Entry;
    UINT Length;
    PFILE_OBJECT FileObject;
    ULONG Result;
    ULONG BytesReturned;
    KSPROPERTY KsPropset = {{STATIC_KSPROPSETID_Sysaudio}, KSPROPERTY_SYSAUDIO_DEVICE_DEFAULT, KSPROPERTY_TYPE_SET};

    while(*SymbolicLinkList)
    {
        Length = wcslen(SymbolicLinkList) + 1;
        Status = WdmAudOpenSysAudioDevice(SymbolicLinkList, &Handle);
        if (NT_SUCCESS(Status))
        {
            Status = ObReferenceObjectByHandle(Handle, 
                                               FILE_READ_DATA | FILE_WRITE_DATA,
                                               IoFileObjectType,
                                               KernelMode,
                                               (PVOID*)&FileObject,
                                               NULL);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("ObReferenceObjectByHandle failed with %x\n", Status);
                ZwClose(Handle);
            }

            Entry = (SYSAUDIO_ENTRY*)ExAllocatePool(NonPagedPool, sizeof(SYSAUDIO_ENTRY) + Length * sizeof(WCHAR));
            if (!Entry)
            {
                ZwClose(Handle);
                ObDereferenceObject((PVOID)FileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Entry->Handle = Handle;
            Entry->SymbolicLink.Length = Length * sizeof(WCHAR);
            Entry->SymbolicLink.MaximumLength = Length * sizeof(WCHAR);
            Entry->SymbolicLink.Buffer = (LPWSTR) (Entry + 1);
            Entry->FileObject = FileObject;
            wcscpy(Entry->SymbolicLink.Buffer, SymbolicLinkList);

            InsertTailList(&DeviceExtension->SysAudioDeviceList, &Entry->Entry);

            /* set device as default device */
            KsSynchronousIoControlDevice(FileObject, 
                                         KernelMode,
                                         IOCTL_KS_PROPERTY,
                                         (PVOID)&KsPropset, 
                                         sizeof(KSPROPERTY),
                                         (PVOID)&Result,
                                         sizeof(ULONG),
                                         &BytesReturned);

            DeviceExtension->NumSysAudioDevices++;
        }
        SymbolicLinkList += Length;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
WdmAudCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;
    LPWSTR SymbolicLinkList;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    DPRINT1("WdmAudCreate\n");


    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

#if 0
    Status = KsReferenceSoftwareBusObject((KSDEVICE_HEADER)DeviceObject->DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsReferenceSoftwareBusObject failed with %x\n", Status);
        return Status;
    }
#endif

    Status = IoGetDeviceInterfaces(&KSCATEGORY_SYSAUDIO,
                                   NULL,
                                   0,
                                   &SymbolicLinkList);

    if (NT_SUCCESS(Status))
    {
        WdmAudOpenSysAudioDeviceInterfaces(DeviceExtension, SymbolicLinkList);
        ExFreePool(SymbolicLinkList);
    }


    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&KSCATEGORY_SYSAUDIO,
                                            DeviceObject->DriverObject,
                                            DeviceInterfaceChangeCallback,
                                            (PVOID)DeviceExtension,
                                            &DeviceExtension->SysAudioNotification);


    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
NTAPI
WdmAudClose(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    DPRINT1("WdmAudClose\n");

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

#if 0
    Status = KsDereferenceSoftwareBusObject(DeviceExtension->DeviceHeader);
#endif

    if (NT_SUCCESS(Status))
    {
        Status = IoUnregisterPlugPlayNotification(DeviceExtension->SysAudioNotification);
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
NTAPI
WdmAudDeviceControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
WdmAudCleanup(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}



NTSTATUS NTAPI
DriverEntry(
    IN PDRIVER_OBJECT Driver,
    IN PUNICODE_STRING Registry_path
)
{
    DPRINT("Wdmaud.sys loaded\n");

    Driver->DriverExtension->AddDevice = WdmAudAddDevice;
    Driver->DriverUnload = WdmAudUnload;


    Driver->MajorFunction[IRP_MJ_CREATE] = WdmAudCreate;
    Driver->MajorFunction[IRP_MJ_CLOSE] = WdmAudClose;
    Driver->MajorFunction[IRP_MJ_PNP] = WdmAudPnp;
    Driver->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp; 
    Driver->MajorFunction[IRP_MJ_CLEANUP] = WdmAudCleanup;
    Driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WdmAudDeviceControl;
    Driver->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;


    return STATUS_SUCCESS;
}

/*
    ReactOS Sound System
    Sound Blaster / SB Pro / SB 16 driver
    NT4 driver model

    Author:
        Andrew Greenwood (andrew.greenwood@silverblade.co.uk)

    History:
        25 May 2008 - Created
*/

#include <ntddk.h>
#include <ntddsnd.h>
#include <debug.h>

typedef struct _SOUND_BLASTER_EXTENSION
{
    ULONG NothingHereYet;
} SOUND_BLASTER_EXTENSION, *PSOUND_BLASTER_EXTENSION;


NTSTATUS STDCALL
CreateSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
CloseSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
CleanupSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
ControlSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;}

NTSTATUS STDCALL
WriteSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;}

VOID STDCALL
UnloadSoundBlaster(
    IN  PDRIVER_OBJECT DriverObject)
{
    INFO_(IHVAUDIO, "Sound Blaster driver being unloaded");
}

NTSTATUS STDCALL
DriverEntry(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;

    INFO_(IHVAUDIO, "Sound Blaster driver (NT4 model) by Silver Blade");

    DriverObject->Flags = 0;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = CleanupSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ControlSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = WriteSoundBlaster;
    DriverObject->DriverUnload = UnloadSoundBlaster;

    return STATUS_SUCCESS;
}


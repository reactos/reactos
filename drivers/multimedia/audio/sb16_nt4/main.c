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

#include <sbdsp.h>
#include <devname.h>

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

/* This is to be moved into the sound library later */
#define SOUND_PARAMETERS_KEYNAME_W      L"Parameters"
#define SOUND_DEVICES_KEYNAME_W         L"Devices"
#define SOUND_DEVICE_KEYNAME_PREFIX_W   L"Device"

/* NT4 */
#if 0
ULONG
GetSoundDeviceCount(
    IN  PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE KeyHandle;
    ULONG DeviceCount = 0;
    PCWSTR RegistryPathBuffer;
    UNICODE_STRING FullRegistryPath;
    ULONG PathLength;

    //PathLength = RegistryPath.Length +;

    /* TODO */
    /*RegistryPathBuffer = ExAllocatePoolWithTag(PAGED_POOL,*/

    InitializeObjectAttributes(&Attributes,
                               RegistryPath,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&KeyHandle,
                       KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                       &Attributes);

    if ( ! NT_SUCCESS(Status) )
    {
        return 0;
    }

    //ZwEnumerateKey(Key
}
#endif


/*
    This is purely for testing the sound library at present. Eventually this
    should be re-formed to consider that some (admittedly slightly crazy)
    users might have more than one sound blaster card, or perhaps might have
    a different configuration.

    ie, this is pretty much as non-PnP as you can get!
*/
STDCALL
NTSTATUS
ThisIsSparta(IN PDRIVER_OBJECT DriverObject)
{
    DEVICE_OBJECT device;
    BOOLEAN result;
    CreateSoundDeviceWithDefaultName(DriverObject, 0, 69, 0, &device);

    INFO_(IHVAUDIO, "Resetting SB...");
    result = ResetSoundBlasterDSP((PUCHAR)0x220, 10000);
    INFO_(IHVAUDIO, "SB reset returns %d", result);

    return STATUS_SUCCESS;
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

    /* Hax */
    return ThisIsSparta(DriverObject);
}


/*
 *
 * COPYRIGHT:           See COPYING in the top level directory
 * PROJECT:             ReactOS Sound System
 * FILE:                drivers/multimedia/audio/sndblst/sndblst.c
 * PURPOSE:             Sound Blaster / Pro / 16 driver
 * PROGRAMMER:          Andrew Greenwood (silverblade@reactos.org)
 *
 * UPDATE HISTORY:      Feb 25, 2009: New rewrite started
 *
 */

/* DEFINES AND INCLUDES ******************************************************/

#include <ntddk.h>
#include <windef.h>
#include <mmsystem.h>
#include <debug.h>

#define CompleteIrpAndReturn(irp, status) \
    irp->IoStatus.Status = status; \
    irp->IoStatus.Information = 0; \
    IoCompleteRequest(Irp, IO_NO_INCREMENT); \
    return status;


/* FORWARD DECLARATIONS *****************************************************/

static VOID NTAPI
UnloadSoundBlaster(PDRIVER_OBJECT DriverObject);


/* DEVICE "DISCOVERY" *******************************************************/
/* Nb: These need to go in the helper lib */

typedef NTSTATUS (*SOUNDDEVICEENUMERATIONCALLBACK)(
    IN  PUNICODE_STRING DeviceRegistryPath);

NTSTATUS
EnumerateSoundDevices(
    IN  PUNICODE_STRING RegistryPath,
    IN  PWSTR RegistrySubKey,
    IN  SOUNDDEVICEENUMERATIONCALLBACK Callback)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES RegAttributes;
    HKEY MainKeyHandle, ChildKeyHandle;
    UNICODE_STRING UnicodeSubkeyName, DeviceKeyName;
    KEY_BASIC_INFORMATION KeyInfo, *FinalKeyInfo;
    ULONG i = 0, NeededDataLength = 0, FinalDataLength = 0, NameLength = 0;

    /* Turn the subkey name into a Unicode string */
    RtlInitUnicodeString(&UnicodeSubkeyName, RegistrySubKey);

    /* Open the registry key for the service */
    InitializeObjectAttributes(&RegAttributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    Status = ZwOpenKey(&MainKeyHandle, KEY_READ, &RegAttributes);

    if ( ! NT_SUCCESS(Status) )
    {
        DPRINT("Failed to open registry key\n");
        return Status;
    }

    /* Open the subkey usually named "Parameters" */
    InitializeObjectAttributes(&RegAttributes,
                               &UnicodeSubkeyName,
                               OBJ_CASE_INSENSITIVE,
                               MainKeyHandle,
                               (PSECURITY_DESCRIPTOR) NULL);

    Status = ZwOpenKey(&ChildKeyHandle, KEY_ENUMERATE_SUB_KEYS, &RegAttributes);

    /* We're done with the main key now */
    ZwClose(MainKeyHandle);

    if ( ! NT_SUCCESS(Status) )
    {
        DPRINT("Failed to open registry subkeys for enumeration\n");
        return Status;
    }

    /* Enumerate through the device keys */
    while ( ( Status = ZwEnumerateKey(ChildKeyHandle,
                                      i,
                                      KeyBasicInformation,
                                      &KeyInfo,
                                      sizeof(KEY_BASIC_INFORMATION),
                                      &NeededDataLength) ) != STATUS_NO_MORE_ENTRIES )
    {
        PWSTR EnumeratedKeyName, StartOfEnumeratedKeyName;

        DPRINT("Found subkey %d\n", i);

        FinalDataLength = NeededDataLength + FIELD_OFFSET(KEY_BASIC_INFORMATION, Name[0]);
        DPRINT("Allocating %d bytes\n", FinalDataLength);

        FinalKeyInfo = (PKEY_BASIC_INFORMATION) ExAllocatePool(PagedPool, FinalDataLength);

        if ( ! FinalKeyInfo )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        /* This time we get the real info */
        Status = ZwEnumerateKey(ChildKeyHandle,
                                i,
                                KeyBasicInformation,
                                FinalKeyInfo,
                                FinalDataLength,
                                &NeededDataLength);

        if ( ! NT_SUCCESS(Status) )
        {
            DPRINT("FAILED to enumerate key!\n");
        }
        else
        {
            NameLength = RegistryPath->Length + sizeof(WCHAR) +
                         UnicodeSubkeyName.Length + sizeof(WCHAR) +
                         FinalKeyInfo->NameLength + sizeof(UNICODE_NULL);

            DPRINT("Allocating memory for name (%d bytes)\n", NameLength);

            EnumeratedKeyName = (PWSTR) ExAllocatePool(PagedPool, NameLength);

            if ( ! EnumeratedKeyName )
            {
                ExFreePool((PVOID)FinalKeyInfo);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            StartOfEnumeratedKeyName = EnumeratedKeyName;

            /* Start building the registry path using the service key */
            RtlCopyMemory(EnumeratedKeyName,
                          RegistryPath->Buffer,
                          RegistryPath->Length);

            EnumeratedKeyName += RegistryPath->Length / sizeof(WCHAR);
            EnumeratedKeyName[0] = '\\';
            ++ EnumeratedKeyName;

            /* Append the parameters subkey */
            RtlCopyMemory(EnumeratedKeyName,
                          RegistrySubKey,
                          UnicodeSubkeyName.Length);

            EnumeratedKeyName += UnicodeSubkeyName.Length / sizeof(WCHAR);
            EnumeratedKeyName[0] = '\\';
            ++ EnumeratedKeyName;

            /* And finally append the enumerated key name */
            RtlCopyMemory(EnumeratedKeyName,
                          FinalKeyInfo->Name,
                          FinalKeyInfo->NameLength);

            EnumeratedKeyName += FinalKeyInfo->NameLength / sizeof(WCHAR);
            EnumeratedKeyName[0] = UNICODE_NULL;

            /* Reset pointer */
            EnumeratedKeyName = StartOfEnumeratedKeyName;

            /* Convert into a Unicode string for the callback */
            RtlInitUnicodeString(&DeviceKeyName, EnumeratedKeyName);

            Callback(&DeviceKeyName);

            /* No longer need the key name */
            ExFreePool((PVOID)EnumeratedKeyName);
            EnumeratedKeyName = NULL;
        }

        /* No longer need the key info */
        ExFreePool((PVOID)FinalKeyInfo);
        FinalKeyInfo = NULL;

        ++ i;
    }

    /* We're done with enumeration so close this */
    ZwClose(ChildKeyHandle);

    /* This isn't an error */
    if ( Status == STATUS_NO_MORE_ENTRIES )
    {
        Status = STATUS_SUCCESS;
    }

    /* No devices configured? */
    if ( i == 0 && Status == STATUS_NO_MORE_ENTRIES )
    {
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    return Status;
}

NTSTATUS
PublishWaveOutDevice(
    IN  DWORD HardwareDeviceIndex,
    IN  PWSTR BaseDeviceName,
    IN  DWORD DeviceIndex,
    IN  LPWAVEOUTCAPS Capabilities)
{
    return STATUS_SUCCESS;
}


typedef struct _SOUND_BLASTER_DEVICE
{
    DWORD   BasePort;
    DWORD   MidiUartBasePort;

    DWORD   Irq;

    DWORD   DmaChannel_8;
    DWORD   DmaChannel_16;

    DWORD   DspVersion;

    DWORD   ActualDmaBufferSize;
    DWORD   DmaBufferSize;
} SOUND_BLASTER_DEVICE;


NTSTATUS
AllocateSoundBlasterStructure(OUT SOUND_BLASTER_DEVICE* SoundBlasterDevice)
{
    return STATUS_NOT_IMPLEMENTED;
}

/* callback */
/*
    Configuration options within the registry:
    REG_DWORD   Actual Dma Buffer Size      0x00004000
    REG_DWORD   Configuration Error         0xffffffff
    REG_DWORD   Dma Buffer Size             0x00004000
    REG_DWORD   DmaChannel                  0x00000001
    REG_DWORD   DmaChannel16                0x00000005
    REG_DWORD   DSP Version                 0x00000405
    REG_DWORD   Interrupt                   0x00000005
    REG_DWORD   Load Type                   0x00000000
    REG_BINARY  Mixer Settings              ??
    REG_DWORD   MPU401 Port                 0xffffffff
    REG_DWORD   Port                        0x00000220
*/

NTSTATUS
ConfigureSoundBlasterDevice(IN PUNICODE_STRING DeviceRegistryPath)
{
    OBJECT_ATTRIBUTES RegAttributes;
    NTSTATUS Status = STATUS_SUCCESS;
    HKEY ConfigKeyHandle;

    DPRINT("Configuring Sound Blaster (config at %S)\n", DeviceRegistryPath->Buffer);

    if ( ! DeviceRegistryPath )
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Open the registry key */
    InitializeObjectAttributes(&RegAttributes,
                               DeviceRegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    Status = ZwOpenKey(&ConfigKeyHandle, KEY_READ, &RegAttributes);

    if ( ! NT_SUCCESS(Status) )
    {
        DPRINT("Failed to open config registry key\n");
        return Status;
    }

    /* Read configuration */
    DPRINT("Reading configuration\n");

    //Status = ZwQueryValueKey(ConfigKeyHandle,

    return Status;
}


/* IRP DISPATCHERS **********************************************************/

static NTSTATUS NTAPI
CreateSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT("Sound Blaster driver received IRP_MJ_CREATE\n");

    CompleteIrpAndReturn(Irp, STATUS_SUCCESS);
}

static NTSTATUS NTAPI
CloseSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT("Sound Blaster driver received IRP_MJ_CLOSE\n");

    CompleteIrpAndReturn(Irp, STATUS_SUCCESS);
}

static NTSTATUS NTAPI
CleanupSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT("Sound Blaster driver received IRP_MJ_CLEANUP\n");

    CompleteIrpAndReturn(Irp, STATUS_SUCCESS);
}

static NTSTATUS NTAPI
ControlSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT("Sound Blaster driver received IRP_MJ_CONTROL\n");

    CompleteIrpAndReturn(Irp, STATUS_SUCCESS);
}

static NTSTATUS NTAPI
WriteToSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT("Sound Blaster driver received IRP_MJ_WRITE\n");

    CompleteIrpAndReturn(Irp, STATUS_SUCCESS);
}


/* DRIVER ENTRYPOINT ********************************************************/

NTSTATUS NTAPI
DriverEntry(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("Sound Blaster driver by silverblade\n");

    DriverObject->Flags = 0;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = CleanupSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ControlSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = WriteToSoundBlaster;
    DriverObject->DriverUnload = UnloadSoundBlaster;

    EnumerateSoundDevices(RegistryPath, L"Parameters", ConfigureSoundBlasterDevice);

    return Status;
}

static VOID NTAPI
UnloadSoundBlaster(IN PDRIVER_OBJECT DriverObject)
{
    DPRINT("Sound Blaster driver is being unloaded\n");
}

/*
    ReactOS Sound System
    Device naming & creation helper routines

    Author:
        Andrew Greenwood (andrew.greenwood@silverblade.co.uk)

    History:
        25 May 2008 - Created
*/

#include <ntddk.h>
#include <ntddsnd.h>
#include <debug.h>

/*
    Default device names

    Just to keep things tidy, we define a structure to hold both the \\Device
    and \\DosDevices names, and then fill this structure with the default
    device names that can be found in NTDDSND.H
*/

typedef struct _DEVICE_NAME_GROUP
{
    PCWSTR DeviceName;
    PCWSTR DosDeviceName;
} DEVICE_NAME_GROUP;

DEVICE_NAME_GROUP SoundDeviceNameBodies[6] =
{
    {
        DD_WAVE_IN_DEVICE_NAME_U,
        DD_WAVE_IN_DOS_DEVICE_NAME_U
    },
    {
        DD_WAVE_OUT_DEVICE_NAME_U,
        DD_WAVE_OUT_DOS_DEVICE_NAME_U
    },
    {
        DD_MIDI_IN_DEVICE_NAME_U,
        DD_MIDI_IN_DOS_DEVICE_NAME_U
    },
    {
        DD_MIDI_OUT_DEVICE_NAME_U,
        DD_MIDI_OUT_DOS_DEVICE_NAME_U
    },
    {
        DD_MIX_DEVICE_NAME_U,
        DD_MIX_DOS_DEVICE_NAME_U
    },
    {
        DD_AUX_DEVICE_NAME_U,
        DD_AUX_DOS_DEVICE_NAME_U
    }
};


/*
    ConstructDeviceName

    This takes a wide-character string containing the device name body (for
    example, "\\Device\\WaveOut") and appends the device index, forming a
    string like "\\Device\\WaveOut0", and so on.

    The resulting device name is a unicode string.
*/

NTSTATUS
ConstructDeviceName(
    IN  PCWSTR Path,
    IN  UCHAR Index,
    OUT PUNICODE_STRING DeviceName)
{
    UNICODE_STRING UnicodePath;
    UNICODE_STRING UnicodeIndex;
    WCHAR IndexStringBuffer[5];
    USHORT Size;
    USHORT LastCharacterIndex;

    /* Check for NULL parameters */
    if ( ( ! Path ) || ( ! DeviceName ) )
    {
        ERR_(IHVAUDIO, "Unexpected NULL parameter");
        return STATUS_INVALID_PARAMETER;
    }

    /* Range-check */
    if ( Index >= SOUND_MAX_DEVICES )
    {
        ERR_(IHVAUDIO, "Device index %d out of range", Index);
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialise the unicode path string */
    RtlInitUnicodeString(&UnicodePath, Path);

    /* Calculate the length to hold the full string */
    Size = UnicodePath.Length +
           sizeof(IndexStringBuffer) +
           sizeof(UNICODE_NULL);

    /* Allocate memory for DeviceName */
    DeviceName->Buffer = ExAllocatePool(PagedPool, Size);
    DeviceName->MaximumLength = Size;

    if ( ! DeviceName->Buffer )
    {
        ERR_(IHVAUDIO, "Couldn't allocate memory for device name string");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy the path */
    RtlCopyUnicodeString(DeviceName, &UnicodePath);

    /* Convert Index to string and append */
    UnicodeIndex.Buffer = IndexStringBuffer;
    UnicodeIndex.MaximumLength = sizeof(IndexStringBuffer);

    RtlIntegerToUnicodeString((ULONG)Index, 10, &UnicodeIndex);
    RtlAppendUnicodeStringToString(DeviceName, &UnicodeIndex);

    /* Terminate the string */
    LastCharacterIndex = DeviceName->Length / sizeof(UNICODE_NULL);
    DeviceName->Buffer[LastCharacterIndex] = UNICODE_NULL;

    return STATUS_SUCCESS;
}


/*
    FreeUnicodeStringBuffer

    A small helper routine to free a unicode string buffer, nullify the
    buffer and reset the lengths to zero.
*/

VOID
FreeUnicodeStringBuffer(IN PUNICODE_STRING String)
{
    ASSERT(String != NULL);
    ASSERT(String->Buffer != NULL);

    ExFreePool(String->Buffer);

    String->Buffer = NULL;
    String->Length = 0;
    String->MaximumLength = 0;
}


/*
    GetDefaultSoundDeviceNameBodies

    Simply accesses the SoundDeviceNameBodies struct defined earlier and
    fills the DeviceNameBody and DosDeviceNameBody parameters accordingly.

    Basically a "safe" way to access the array and perform two assignments
    with one call, as this will assign the name and DOS name if a valid
    DeviceType is passed, otherwise it will fail with STATUS_INVALID_PARAMETER.
*/

NTSTATUS
GetDefaultSoundDeviceNameBodies(
    IN  UCHAR DeviceType,
    OUT PCWSTR* DeviceNameBody,
    OUT PCWSTR* DosDeviceNameBody)
{
    ASSERT(DeviceNameBody != NULL);
    ASSERT(DosDeviceNameBody != NULL);

    if ( DeviceType > MAX_DEVICE_TYPE )
    {
        ERR_(IHVAUDIO, "Invalid device type");
        return STATUS_INVALID_PARAMETER;
    }

    *DeviceNameBody = SoundDeviceNameBodies[DeviceType].DeviceName;
    *DosDeviceNameBody = SoundDeviceNameBodies[DeviceType].DosDeviceName;

    return STATUS_SUCCESS;
}


/*
    ConstructSoundDeviceNames

    Given two wide-character strings and a device index, convert these into
    two unicode strings with the index appended to the end.

    This is intended for converting a device name and a DOS device name at
    the same time.
*/

NTSTATUS
ConstructSoundDeviceNames(
    IN  PCWSTR DeviceNameBody,
    IN  PCWSTR DosDeviceNameBody,
    IN  UCHAR Index,
    OUT PUNICODE_STRING FullDeviceName,
    OUT PUNICODE_STRING FullDosDeviceName)
{
    NTSTATUS Status;

    /* Check for NULL parameters */
    if ( ( ! DeviceNameBody ) || ( ! DosDeviceNameBody ) ||
         ( ! FullDeviceName ) || ( ! FullDosDeviceName ) )
    {
        ERR_(IHVAUDIO, "Unexpected NULL parameter");
        return STATUS_INVALID_PARAMETER;
    }

    /* Range-check */
    if ( Index >= SOUND_MAX_DEVICES )
    {
        ERR_(IHVAUDIO, "Device %d exceeds maximum", Index);
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConstructDeviceName(DeviceNameBody, Index, FullDeviceName);

    if ( ! NT_SUCCESS(Status) )
    {
        /* No need to clean up on failure here */
        return Status;
    }

    Status = ConstructDeviceName(DosDeviceNameBody, Index, FullDosDeviceName);

    if ( ! NT_SUCCESS(Status) )
    {
        /* We need to free the string we successfully got earlier */
        FreeUnicodeStringBuffer(FullDeviceName);
        return Status;
    }

    return STATUS_SUCCESS;
}


/*
    CreateSoundDevice

    Creates a device and symbolically-links a DOS device to this. Use this
    when you want to specify alternative device names to the defaults
    (eg: "\\Device\\MySoundDev" rather than "\\Device\\WaveOut")
*/

NTSTATUS
CreateSoundDevice(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PCWSTR WideDeviceName,
    IN  PCWSTR WideDosDeviceName,
    IN  UCHAR Index,
    IN  ULONG ExtensionSize,
    OUT PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;

    UNICODE_STRING DeviceName;
    UNICODE_STRING DosDeviceName;

    /* Check for NULL parameters */
    if ( ( ! DriverObject ) || ( ! DeviceObject ) ||
         ( ! WideDeviceName ) || ( ! WideDosDeviceName ) )
    {
        ERR_(IHVAUDIO, "Unexpected NULL parameter");
        return STATUS_INVALID_PARAMETER;
    }

    /* Range-check */
    if ( Index >= SOUND_MAX_DEVICES )
    {
        ERR_(IHVAUDIO, "Device index %d exceeds maximum", Index);
        return STATUS_INVALID_PARAMETER;
    }

    /* Construct the device and DOS device names */
    Status = ConstructSoundDeviceNames(WideDeviceName,
                                       WideDosDeviceName,
                                       Index,
                                       &DeviceName,
                                       &DosDeviceName);

    if ( ! NT_SUCCESS(Status) )
    {
        return Status;
    }

    /* Now create the device */
    Status = IoCreateDevice(DriverObject,
                            ExtensionSize,
                            &DeviceName,
                            FILE_DEVICE_SOUND,
                            0,
                            FALSE,
                            &DeviceObject);

    if ( ! NT_SUCCESS(Status) )
    {
        /* These will have been allocated by ConstructSoundDeviceNames */
        FreeUnicodeStringBuffer(&DeviceName);
        FreeUnicodeStringBuffer(&DosDeviceName);

        return Status;
    }

    /* Create a symbolic link for the DOS deviec name */
    Status = IoCreateSymbolicLink(&DosDeviceName, &DeviceName);

    if ( ! NT_SUCCESS(Status) )
    {
        /* IoDeleteDevice( --- What are we deleting? */

        /* These will have been allocated by ConstructSoundDeviceNames */
        FreeUnicodeStringBuffer(&DeviceName);
        FreeUnicodeStringBuffer(&DosDeviceName);

        return Status;
    }

    return STATUS_SUCCESS;
}


/*
    CreateSoundDeviceWithDefaultName

    Similar to CreateSoundDevice, except this uses the default device names
    ("\\Device\\WaveOut" etc.) based on the DeviceType parameter.
*/

NTSTATUS
CreateSoundDeviceWithDefaultName(
    IN  PDRIVER_OBJECT DriverObject,
    IN  UCHAR DeviceType,
    IN  UCHAR Index,
    IN  ULONG ExtensionSize,
    OUT PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PCWSTR WideDeviceName = NULL;
    PCWSTR WideDosDeviceName = NULL;

    /* Check for NULL parameters */
    if ( ( ! DriverObject ) || ( ! DeviceObject ) )
    {
        ERR_(IHVAUDIO, "Unexpected NULL parameter");
        return STATUS_INVALID_PARAMETER;
    }

    /* Range-check */
    if ( Index >= SOUND_MAX_DEVICES )
    {
        ERR_(IHVAUDIO, "Device index %d exceeds maximum", Index);
        return STATUS_INVALID_PARAMETER;
    }

    /* Look-up the default name based on the device type */
    Status = GetDefaultSoundDeviceNameBodies(DeviceType,
                                             &WideDeviceName,
                                             &WideDosDeviceName);

    if ( ! NT_SUCCESS(Status) )
    {
        return Status;
    }

    /* Go create the device! */
    Status = CreateSoundDevice(DriverObject,
                               WideDeviceName,
                               WideDosDeviceName,
                               Index,
                               ExtensionSize,
                               DeviceObject);

    if ( ! NT_SUCCESS(Status) )
    {
        /* No clean-up to do */
        return Status;
    }

    return STATUS_SUCCESS;
}

/*
    ReactOS Sound System
    Device naming & creation helper routines

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        25 May 2008 - Created
*/

#ifndef ROS_DEVNAME
#define ROS_DEVNAME

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
    OUT PUNICODE_STRING DeviceName);


/*
    FreeUnicodeStringBuffer

    A small helper routine to free a unicode string buffer, nullify the
    buffer and reset the lengths to zero.
*/

VOID
FreeUnicodeStringBuffer(IN PUNICODE_STRING String);


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
    OUT PCWSTR* DosDeviceNameBody);


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
    OUT PUNICODE_STRING FullDosDeviceName);


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
    OUT PDEVICE_OBJECT* DeviceObject);


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
    OUT PDEVICE_OBJECT* DeviceObject);


/*
    DestroySoundDevice

    Destroy a device and its symbolic link
*/
NTSTATUS
DestroySoundDevice(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PCWSTR WideDosDeviceName,
    IN  UCHAR Index);


/*
    DestroySoundDeviceWithDefaultName

    Similar to DestroySoundDevice, but operating on one of the
    default device names.
*/
NTSTATUS
DestroySoundDeviceWithDefaultName(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  UCHAR DeviceType,
    IN  UCHAR Index);

#endif

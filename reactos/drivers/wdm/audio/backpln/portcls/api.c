#include "private.h"

/*
 * @implemented
 */
NTSTATUS
NTAPI
PcGetDeviceProperty(
    IN  PVOID DeviceObject,
    IN  DEVICE_REGISTRY_PROPERTY DeviceProperty,
    IN  ULONG BufferLength,
    OUT PVOID PropertyBuffer,
    OUT PULONG ResultLength)
{
    return IoGetDeviceProperty(DeviceObject, DeviceProperty, BufferLength, PropertyBuffer, ResultLength);
}

/*
 * @implemented
 */
ULONGLONG
NTAPI
PcGetTimeInterval(
    IN  ULONGLONG Since)
{
    LARGE_INTEGER CurrentTime;

    KeQuerySystemTime(&CurrentTime);

    return (CurrentTime.QuadPart - Since);
}


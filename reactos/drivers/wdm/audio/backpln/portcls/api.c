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

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcRegisterIoTimeout(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  PIO_TIMER_ROUTINE pTimerRoutine,
    IN  PVOID pContext)
{
    NTSTATUS Status;


    /* FIXME 
     * check if timer is already used 
     */

    Status = IoInitializeTimer(pDeviceObject, pTimerRoutine, pContext);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IoInitializeTimer failed with %x\n", Status);
        return Status;
    }

    IoStartTimer(pDeviceObject);
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcUnregisterIoTimeout(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  PIO_TIMER_ROUTINE pTimerRoutine,
    IN  PVOID pContext)
{
    /* FIXME 
     * check if timer is already used 
     */

    IoStopTimer(pDeviceObject);
    return STATUS_SUCCESS;
}

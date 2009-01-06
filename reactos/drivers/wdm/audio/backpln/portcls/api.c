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


NTSTATUS
NTAPI
PcDmaMasterDescription(
    IN PRESOURCELIST ResourceList OPTIONAL,
    IN BOOLEAN ScatterGather,
    IN BOOLEAN Dma32BitAddresses,
    IN BOOLEAN IgnoreCount,
    IN BOOLEAN Dma64BitAddresses,
    IN DMA_WIDTH DmaWidth,
    IN DMA_SPEED DmaSpeed,
    IN ULONG MaximumLength,
    IN ULONG DmaPort,
    OUT PDEVICE_DESCRIPTION DeviceDescription)
{

    RtlZeroMemory(DeviceDescription, sizeof(DEVICE_DESCRIPTION));

    DeviceDescription->Master = TRUE;
    DeviceDescription->ScatterGather= ScatterGather;
    DeviceDescription->Dma32BitAddresses = Dma32BitAddresses;
    DeviceDescription->IgnoreCount = IgnoreCount;
    DeviceDescription->Dma64BitAddresses = Dma64BitAddresses;
    DeviceDescription->DmaWidth = DmaWidth;
    DeviceDescription->DmaSpeed = DmaSpeed;
    DeviceDescription->MaximumLength = MaximumLength;
    DeviceDescription->DmaPort = DmaPort;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PcDmaSlaveDescription(
    IN PRESOURCELIST  ResourceList OPTIONAL,
    IN ULONG DmaIndex,
    IN BOOL DemandMode,
    IN ULONG AutoInitialize,
    IN DMA_SPEED DmaSpeed
    IN ULONG MaximumLength,
    IN ULONG DmaPort,
    OUT PDEVICE_DESCRIPTION DeviceDescription)
{

    RtlZeroMemory(DeviceDescription, sizeof(DEVICE_DESCRIPTION));

    DeviceDescription->DemandMode = DemandMode;
    DeviceDescription->AutoInitialize = AutoInitialize;
    DeviceDescription->DmaSpeed = DmaSpeed;
    DeviceDescription->MaximumLength = MaximumLength;
    DeviceDescription->DmaPort = DmaPort;

    return STATUS_SUCCESS;
}
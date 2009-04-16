/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/api.c
 * PURPOSE:         Port api functions
 * PROGRAMMER:      Johannes Anderwald
 */

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
        DPRINT1("IoInitializeTimer failed with %x\n", Status);
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


/*
 * @implemented
 */
NTSTATUS
NTAPI
PcCompletePendingPropertyRequest(
    IN  PPCPROPERTY_REQUEST PropertyRequest,
    IN  NTSTATUS NtStatus)
{
    /* sanity checks */

    if (!PropertyRequest)
        return STATUS_INVALID_PARAMETER;

    ASSERT(PropertyRequest->Irp);
    ASSERT(NtStatus != STATUS_PENDING);

    /* set the final status code */
    PropertyRequest->Irp->IoStatus.Status = NtStatus;

    /* complete the irp */
    IoCompleteRequest(PropertyRequest->Irp, IO_SOUND_INCREMENT);

    /* free the property request */
    ExFreePool(PropertyRequest);

    /* return success */
    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
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
    DeviceDescription->Version = DEVICE_DESCRIPTION_VERSION1;
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
/*
 * @implemented
 */
NTSTATUS
NTAPI
PcDmaSlaveDescription(
    IN PRESOURCELIST  ResourceList OPTIONAL,
    IN ULONG DmaIndex,
    IN BOOL DemandMode,
    IN ULONG AutoInitialize,
    IN DMA_SPEED DmaSpeed,
    IN ULONG MaximumLength,
    IN ULONG DmaPort,
    OUT PDEVICE_DESCRIPTION DeviceDescription)
{

    RtlZeroMemory(DeviceDescription, sizeof(DEVICE_DESCRIPTION));

    DeviceDescription->Version = DEVICE_DESCRIPTION_VERSION1;
    DeviceDescription->DemandMode = DemandMode;
    DeviceDescription->AutoInitialize = AutoInitialize;
    DeviceDescription->DmaSpeed = DmaSpeed;
    DeviceDescription->MaximumLength = MaximumLength;
    DeviceDescription->DmaPort = DmaPort;

    return STATUS_SUCCESS;
}


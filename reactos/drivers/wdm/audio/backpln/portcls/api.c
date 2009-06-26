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
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
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
 * @implemented
 */

VOID
NTAPI
PcIoTimerRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID  Context)
{
    PPCLASS_DEVICE_EXTENSION DeviceExtension;
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    PTIMER_CONTEXT CurContext;

    if (!DeviceObject || !DeviceObject->DeviceExtension)
        return;

    DeviceExtension = (PPCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    KeAcquireSpinLock(&DeviceExtension->TimerListLock, &OldIrql);

    ListEntry = DeviceExtension->TimerList.Flink;
    while(ListEntry != &DeviceExtension->TimerList)
    {
        CurContext = (PTIMER_CONTEXT)CONTAINING_RECORD(ListEntry, TIMER_CONTEXT, Entry);

        CurContext->pTimerRoutine(DeviceObject, CurContext->Context);
        ListEntry = ListEntry->Flink;
    }

    KeReleaseSpinLock(&DeviceExtension->TimerListLock, OldIrql);
}

/*
 * @implemented
 */
NTSTATUS NTAPI
PcRegisterIoTimeout(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  PIO_TIMER_ROUTINE pTimerRoutine,
    IN  PVOID pContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTIMER_CONTEXT TimerContext, CurContext;
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    BOOLEAN bFound;
    PPCLASS_DEVICE_EXTENSION DeviceExtension;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!pDeviceObject || !pDeviceObject->DeviceExtension)
        return STATUS_INVALID_PARAMETER;

    DeviceExtension = (PPCLASS_DEVICE_EXTENSION)pDeviceObject->DeviceExtension;

    TimerContext = AllocateItem(NonPagedPool, sizeof(TIMER_CONTEXT), TAG_PORTCLASS);
    if (!TimerContext)
    {
        DPRINT1("Failed to allocate memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeAcquireSpinLock(&DeviceExtension->TimerListLock, &OldIrql);

    ListEntry = DeviceExtension->TimerList.Flink;
    bFound = FALSE;
    while(ListEntry != &DeviceExtension->TimerList)
    {
        CurContext = (PTIMER_CONTEXT)CONTAINING_RECORD(ListEntry, TIMER_CONTEXT, Entry);

        if (CurContext->Context == pContext && CurContext->pTimerRoutine == pTimerRoutine)
        {
            bFound = TRUE;
            Status = STATUS_UNSUCCESSFUL;
            ExFreePool(TimerContext);
            break;
        }
        ListEntry = ListEntry->Flink;
    }

    if (!bFound)
    {
        TimerContext->Context = pContext;
        TimerContext->pTimerRoutine = pTimerRoutine;
        InsertTailList(&DeviceExtension->TimerList, &TimerContext->Entry);
    }

    KeReleaseSpinLock(&DeviceExtension->TimerListLock, OldIrql);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PcUnregisterIoTimeout(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  PIO_TIMER_ROUTINE pTimerRoutine,
    IN  PVOID pContext)
{
    PTIMER_CONTEXT CurContext;
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    BOOLEAN bFound;
    PPCLASS_DEVICE_EXTENSION DeviceExtension;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!pDeviceObject || !pDeviceObject->DeviceExtension)
        return STATUS_INVALID_PARAMETER;


    DeviceExtension = (PPCLASS_DEVICE_EXTENSION)pDeviceObject->DeviceExtension;


    KeAcquireSpinLock(&DeviceExtension->TimerListLock, &OldIrql);

    ListEntry = DeviceExtension->TimerList.Flink;
    bFound = FALSE;

    while(ListEntry != &DeviceExtension->TimerList)
    {
        CurContext = (PTIMER_CONTEXT)CONTAINING_RECORD(ListEntry, TIMER_CONTEXT, Entry);

        if (CurContext->Context == pContext && CurContext->pTimerRoutine == pTimerRoutine)
        {
            bFound = TRUE;
            RemoveEntryList(&CurContext->Entry);
            ExFreePool(CurContext);
            break;
        }
        ListEntry = ListEntry->Flink;
    }

    KeReleaseSpinLock(&DeviceExtension->TimerListLock, OldIrql);

    if (bFound)
        return STATUS_SUCCESS;
    else
        return STATUS_NOT_FOUND;
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
    ASSERT_IRQL(DISPATCH_LEVEL);

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


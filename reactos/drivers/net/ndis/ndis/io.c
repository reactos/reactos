/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/io.c
 * PURPOSE:     I/O related routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>
#include <miniport.h>


VOID HandleDeferredProcessing(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2)
/*
 * FUNCTION: Deferred interrupt processing routine
 * ARGUMENTS:
 *     Dpc             = Pointer to DPC object
 *     DeferredContext = Pointer to context information (LOGICAL_ADAPTER)
 *     SystemArgument1 = Unused
 *     SystemArgument2 = Unused
 */
{
    PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(DeferredContext);

    /* Call the deferred interrupt service handler for this adapter */
    (*Adapter->Miniport->Chars.HandleInterruptHandler)(Adapter);
}


BOOLEAN ServiceRoutine(
    IN  PKINTERRUPT Interrupt,
    IN  PVOID       ServiceContext)
/*
 * FUNCTION: Interrupt service routine
 * ARGUMENTS:
 *     Interrupt      = Pointer to interrupt object
 *     ServiceContext = Pointer to context information (LOGICAL_ADAPTER)
 * RETURNS
 *     TRUE if our device generated the interrupt
 */
{
    BOOLEAN InterruptRecognized;
    BOOLEAN QueueMiniportHandleInterrupt;
    PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(ServiceContext);

    /* FIXME: Support shared interrupts */

    (*Adapter->Miniport->Chars.ISRHandler)(&InterruptRecognized,
        &QueueMiniportHandleInterrupt, Adapter);

    if (QueueMiniportHandleInterrupt) {
        KeInsertQueueDpc(&Adapter->InterruptObject->InterruptDpc, NULL, NULL);
    }

    return TRUE;
}


VOID
EXPORT
NdisCompleteDmaTransfer(
    OUT PNDIS_STATUS    Status,
    IN  PNDIS_HANDLE    NdisDmaHandle,
    IN  PNDIS_BUFFER    Buffer,
    IN  ULONG           Offset,
    IN  ULONG           Length,
    IN  BOOLEAN         WriteToDevice)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisFlushBuffer(
    IN  PNDIS_BUFFER    Buffer,
    IN  BOOLEAN         WriteToDevice)
{
    UNIMPLEMENTED
}


ULONG
EXPORT
NdisGetCacheFillSize(
    VOID)
{
    UNIMPLEMENTED

	return 0;
}


VOID
EXPORT
NdisImmediateReadPortUchar(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    OUT PUCHAR      Data)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisImmediateReadPortUlong(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    OUT PULONG      Data)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisImmediateReadPortUshort(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    OUT PUSHORT     Data)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisImmediateWritePortUchar(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    IN  UCHAR       Data)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisImmediateWritePortUlong(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    IN  ULONG       Data)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisImmediateWritePortUshort(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    IN  USHORT      Data)
{
    UNIMPLEMENTED
}


NDIS_STATUS
EXPORT
NdisMAllocateMapRegisters(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  UINT        DmaChannel,
    IN  BOOLEAN     Dma32BitAddresses,
    IN  ULONG       PhysicalMapRegistersNeeded,
    IN  ULONG       MaximumPhysicalMapping)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisMCompleteDmaTransfer(
    OUT PNDIS_STATUS    Status,
    IN  PNDIS_HANDLE    MiniportDmaHandle,
    IN  PNDIS_BUFFER    Buffer,
    IN  ULONG           Offset,
    IN  ULONG           Length,
    IN  BOOLEAN         WriteToDevice)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMDeregisterDmaChannel(
    IN  PNDIS_HANDLE    MiniportDmaHandle)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMDeregisterInterrupt(
    IN  PNDIS_MINIPORT_INTERRUPT    Interrupt)
/*
 * FUNCTION: Releases an interrupt vector
 * ARGUMENTS:
 *     Interrupt = Pointer to interrupt object
 */
{
    IoDisconnectInterrupt(Interrupt->InterruptObject);
}


VOID
EXPORT
NdisMDeregisterIoPortRange(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  UINT        InitialPort,
    IN  UINT        NumberOfPorts,
    IN  PVOID       PortOffset)
/*
 * FUNCTION: Releases a register mapping to I/O ports
 * ARGUMENTS:
 *     MiniportAdapterHandle = Specifies handle input to MiniportInitialize
 *     InitialPort           = Bus-relative base port address of a range to be mapped
 *     NumberOfPorts         = Specifies number of ports to be mapped
 *     PortOffset            = Pointer to mapped base port address
 */
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMFreeMapRegisters(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    UNIMPLEMENTED
}


NDIS_STATUS
EXPORT
NdisMMapIoSpace(
    OUT PVOID                   *VirtualAddress,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress,
    IN  UINT                    Length)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


VOID
EXPORT
NdisMQueryInformationComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status)
{
    UNIMPLEMENTED
}


ULONG
EXPORT
NdisMReadDmaCounter(
    IN  NDIS_HANDLE MiniportDmaHandle)
{
    UNIMPLEMENTED

	return 0;
}


NDIS_STATUS
EXPORT
NdisMRegisterDmaChannel(
    OUT PNDIS_HANDLE            MiniportDmaHandle,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  UINT                    DmaChannel,
    IN  BOOLEAN                 Dma32BitAddresses,
    IN  PNDIS_DMA_DESCRIPTION   DmaDescription,
    IN  ULONG                   MaximumLength)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


NDIS_STATUS
EXPORT
NdisMRegisterInterrupt(
    OUT PNDIS_MINIPORT_INTERRUPT    Interrupt,
    IN  NDIS_HANDLE                 MiniportAdapterHandle,
    IN  UINT                        InterruptVector,
    IN  UINT                        InterruptLevel,
    IN  BOOLEAN	                    RequestIsr,
    IN  BOOLEAN                     SharedInterrupt,
    IN  NDIS_INTERRUPT_MODE         InterruptMode)
/*
 * FUNCTION: Claims access to an interrupt vector
 * ARGUMENTS:
 *     Interrupt             = Address of interrupt object to initialize
 *     MiniportAdapterHandle = Specifies handle input to MiniportInitialize
 *     InterruptVector       = Specifies bus-relative vector to register
 *     InterruptLevel        = Specifies bus-relative DIRQL vector for interrupt
 *     RequestIsr            = TRUE if MiniportISR should always be called
 *     SharedInterrupt       = TRUE if other devices may use tha same interrupt
 *     InterruptMode         = Specifies type of interrupt
 * RETURNS:
 *     Status of operation
 */
{
    NTSTATUS NtStatus;
    ULONG MappedIRQ;
    KIRQL DIrql;
    KAFFINITY Affinity = 0xFFFFFFFF;
    PLOGICAL_ADAPTER Adapter  = GET_LOGICAL_ADAPTER(MiniportAdapterHandle);

    RtlZeroMemory(Interrupt, sizeof(NDIS_MINIPORT_INTERRUPT));

    KeInitializeSpinLock(&Interrupt->DpcCountLock);

    KeInitializeDpc(&Interrupt->InterruptDpc, HandleDeferredProcessing, Adapter);

    KeInitializeEvent(&Interrupt->DpcsCompletedEvent,
        NotificationEvent, FALSE);

    Interrupt->SharedInterrupt = SharedInterrupt;

    Adapter->InterruptObject = Interrupt;

    MappedIRQ = HalGetInterruptVector(Adapter->AdapterType, 0,
        InterruptLevel, InterruptVector, &DIrql, &Affinity);

    NtStatus = IoConnectInterrupt(&Interrupt->InterruptObject, ServiceRoutine, Adapter,
        &Interrupt->DpcCountLock, MappedIRQ, DIrql, DIrql, InterruptMode,
        SharedInterrupt, Affinity, FALSE);

	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
EXPORT
NdisMRegisterIoPortRange(
    OUT PVOID       *PortOffset,
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  UINT        InitialPort,
    IN  UINT        NumberOfPorts)
/*
 * FUNCTION: Sets up driver access to device I/O ports
 * ARGUMENTS:
 *     PortOffset            = Address of buffer to place mapped base port address
 *     MiniportAdapterHandle = Specifies handle input to MiniportInitialize
 *     InitialPort           = Bus-relative base port address of a range to be mapped
 *     NumberOfPorts         = Specifies number of ports to be mapped
 * RETURNS:
 *     Status of operation
 */
{
#if 0
    NTSTATUS NtStatus;
    BOOLEAN ConflictDetected;
    PLOGICAL_ADAPTER Adapter  = GET_LOGICAL_ADAPTER(MiniportAdapterHandle);
    PMINIPORT_DRIVER Miniport = Adapter->Miniport;

    /* Non-PnP hardware. NT5 function */
    NtStatus = IoReportResourceForDetection(
        Miniport->DriverObject,
        NULL,
        0,
        NULL,
        NULL,
        0,
        &ConflictDetected);
    return NDIS_STATUS_FAILURE;
#else
    /* It's yours! */
    *PortOffset = (PVOID)InitialPort;

    return NDIS_STATUS_SUCCESS;
#endif
}


VOID
EXPORT
NdisMSetInformationComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMSetupDmaTransfer(
    OUT	PNDIS_STATUS    Status,
    IN	PNDIS_HANDLE    MiniportDmaHandle,
    IN	PNDIS_BUFFER    Buffer,
    IN	ULONG           Offset,
    IN	ULONG           Length,
    IN	BOOLEAN         WriteToDevice)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMTransferDataComplete(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status,
    IN  UINT            BytesTransferred)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisMUnmapIoSpace(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  PVOID       VirtualAddress,
    IN  UINT        Length)
{
    UNIMPLEMENTED
}

/* EOF */

/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    hanfnc.c

Abstract:

    default handlers for hal functions which don't get handlers
    installed by the hal

Author:

    Ken Reneris (kenr) 19-July-1994

Revision History:

--*/

#include "ntos.h"
#include "haldisp.h"

HAL_DISPATCH HalDispatchTable = {
    HAL_DISPATCH_VERSION,
    xHalQuerySystemInformation,
    xHalSetSystemInformation,
    xHalQueryBusSlots,
    0,
    xHalExamineMBR,
    xHalIoAssignDriveLetters,
    xHalIoReadPartitionTable,
    xHalIoSetPartitionInformation,
    xHalIoWritePartitionTable,
    xHalHandlerForBus,                  // HalReferenceHandlerByBus
    xHalReferenceHandler,               // HalReferenceBusHandler
    xHalReferenceHandler,               // HalDereferenceBusHandler
    xHalInitPnpDriver,
    xHalInitPowerManagement,
    0,
    xHalGetInterruptTranslator
    };


HAL_PRIVATE_DISPATCH HalPrivateDispatchTable = {
    HAL_PRIVATE_DISPATCH_VERSION,
    xHalHandlerForBus,
    xHalHandlerForBus,
    xHalLocateHiberRanges,
    xHalRegisterBusHandler,
    xHalSetWakeEnable,
    xHalSetWakeAlarm,
    xHalTranslateBusAddress,
    xHalAssignSlotResources,
    xHalHaltSystem,
    (NULL),                             // HalFindBusAddressTranslation
    (NULL)                              // HalResetDisplay
    };

#if 0
DMA_OPERATIONS HalPrivateDmaOperations = {
    sizeof(DMA_OPERATIONS),
    xHalPutDmaAdapter,
    xHalAllocateCommonBuffer,
    xHalFreeCommonBuffer,
    xHalAllocateAdapterChannel,
    xHalFlushAdapterBuffers,
    xHalFreeAdapterChannel,
    xHalFreeMapRegisters,
    xHalMapTransfer,
    xHalGetDmaAlignment,
    xHalReadDmaCounter,
    xHalGetScatterGatherList,
    xHalPutScatterGatherList
    };
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,   xHalLocateHiberRanges)
#pragma alloc_text(PAGE,   xHalQuerySystemInformation)
#pragma alloc_text(PAGE,   xHalSetSystemInformation)
#pragma alloc_text(PAGE,   xHalQueryBusSlots)
#pragma alloc_text(PAGE,   xHalRegisterBusHandler)
#pragma alloc_text(PAGELK, xHalSetWakeEnable)
#pragma alloc_text(PAGELK, xHalSetWakeAlarm)
#endif


//
// Global dispatch table for HAL apis
//


//
// Stub handlers for hals which don't provide the above functions
//

NTSTATUS
xHalQuerySystemInformation(
    IN HAL_QUERY_INFORMATION_CLASS InformationClass,
    IN ULONG     BufferSize,
    OUT PVOID    Buffer,
    OUT PULONG   ReturnedLength
    )
{
    PAGED_CODE ();
    return STATUS_INVALID_LEVEL;
}

NTSTATUS
xHalSetSystemInformation(
    IN HAL_SET_INFORMATION_CLASS InformationClass,
    IN ULONG     BufferSize,
    OUT PVOID    Buffer
    )
{
    PAGED_CODE ();
    return STATUS_INVALID_LEVEL;
}

NTSTATUS
xHalQueryBusSlots(
    IN PBUS_HANDLER         BusHandler,
    IN ULONG                BufferSize,
    OUT PULONG              SlotNumbers,
    OUT PULONG              ReturnedLength
    )
{
    PAGED_CODE ();
    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
xHalRegisterBusHandler(
    IN INTERFACE_TYPE          InterfaceType,
    IN BUS_DATA_TYPE           ConfigurationSpace,
    IN ULONG                   BusNumber,
    IN INTERFACE_TYPE          ParentBusType,
    IN ULONG                   ParentBusNumber,
    IN ULONG                   SizeofBusExtensionData,
    IN PINSTALL_BUS_HANDLER    InstallBusHandler,
    OUT PBUS_HANDLER           *BusHandler
    )
{
    PAGED_CODE ();
    return STATUS_NOT_SUPPORTED;
}


VOID
xHalSetWakeEnable(
    IN BOOLEAN              Enable
    )
{
}


VOID
xHalSetWakeAlarm(
    IN ULONGLONG        WakeTime,
    IN PTIME_FIELDS     WakeTimeFields
    )
{
}

VOID
xHalLocateHiberRanges (
    IN PVOID MemoryMap
    )
{
}

PBUS_HANDLER
FASTCALL
xHalHandlerForBus (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    )
{
    return NULL;
}

VOID
FASTCALL
xHalReferenceHandler (
    IN PBUS_HANDLER     Handler
    )
{
}
NTSTATUS
xHalInitPnpDriver(
    VOID
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
xHalInitPowerManagement(
    IN PPM_DISPATCH_TABLE  PmDriverDispatchTable,
    IN OUT PPM_DISPATCH_TABLE *PmHalDispatchTable
    )
{
    return STATUS_NOT_SUPPORTED;
}

#if 0
PDMA_ADAPTER
xHalGetDmaAdapter (
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    )
{
    PADAPTER_OBJECT AdapterObject;

    AdapterObject = ExAllocatePoolWithTag( NonPagedPool,
                                           sizeof( ADAPTER_OBJECT ),
                                           ' laH');

    if (AdapterObject == NULL) {
        return NULL;
    }

    AdapterObject->DmaAdapter.Size = sizeof( ADAPTER_OBJECT );
    AdapterObject->DmaAdapter.Version = 1;
    AdapterObject->DmaAdapter.DmaOperations = &HalPrivateDmaOperations;
    AdapterObject->RealAdapterObject = HalGetAdapter( DeviceDescriptor,
                                                      NumberOfMapRegisters );

    if (AdapterObject->RealAdapterObject == NULL) {

        //
        // No adapter object was returned.  Just return NULL to the caller.
        //

        ExFreePool( AdapterObject );
        return NULL;
    }

    return &AdapterObject->DmaAdapter;
}

VOID
xHalPutDmaAdapter (
    PDMA_ADAPTER DmaAdapter
    )
{
    ExFreePool( DmaAdapter );
}

PVOID
xHalAllocateCommonBuffer (
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    )
{
    return HalAllocateCommonBuffer( ((PADAPTER_OBJECT) DmaAdapter)->RealAdapterObject,
                                    Length,
                                    LogicalAddress,
                                    CacheEnabled );

}

VOID
xHalFreeCommonBuffer (
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    )
{
    HalFreeCommonBuffer( ((PADAPTER_OBJECT) DmaAdapter)->RealAdapterObject,
                         Length,
                         LogicalAddress,
                         VirtualAddress,
                         CacheEnabled );

}

NTSTATUS
xHalAllocateAdapterChannel (
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    )
{
    return IoAllocateAdapterChannel( ((PADAPTER_OBJECT) DmaAdapter)->RealAdapterObject,
                                     DeviceObject,
                                     NumberOfMapRegisters,
                                     ExecutionRoutine,
                                     Context );

}

BOOLEAN
xHalFlushAdapterBuffers (
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    )
{
    return IoFlushAdapterBuffers( ((PADAPTER_OBJECT) DmaAdapter)->RealAdapterObject,
                                  Mdl,
                                  MapRegisterBase,
                                  CurrentVa,
                                  Length,
                                  WriteToDevice );

}

VOID
xHalFreeAdapterChannel (
    IN PDMA_ADAPTER DmaAdapter
    )
{
    IoFreeAdapterChannel( ((PADAPTER_OBJECT) DmaAdapter)->RealAdapterObject );
}

VOID
xHalFreeMapRegisters (
    IN PDMA_ADAPTER DmaAdapter,
    PVOID MapRegisterBase,
    ULONG NumberOfMapRegisters
    )

{
    IoFreeMapRegisters( ((PADAPTER_OBJECT) DmaAdapter)->RealAdapterObject,
                        MapRegisterBase,
                        NumberOfMapRegisters );
}

PHYSICAL_ADDRESS
xHalMapTransfer (
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    )
{
    return IoMapTransfer( ((PADAPTER_OBJECT) DmaAdapter)->RealAdapterObject,
                           Mdl,
                           MapRegisterBase,
                           CurrentVa,
                           Length,
                           WriteToDevice );
}

ULONG
xHalGetDmaAlignment (
    IN PDMA_ADAPTER DmaAdapter
    )
{
    return HalGetDmaAlignmentRequirement();
}

ULONG
xHalReadDmaCounter (
    IN PDMA_ADAPTER DmaAdapter
    )
{
    return HalReadDmaCounter( ((PADAPTER_OBJECT) DmaAdapter)->RealAdapterObject );
}


NTSTATUS
xHalGetScatterGatherList (
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    )
/*++

Routine Description:

    This routine allocates the adapter channel specified by the adapter
    object.  Next a scatter/gather list is built based on the MDL, the
    CurrentVa and the requested Length.  Finally the driver?s execution
    function is called with the scatter/gather list.  The adapter is
    released when after the execution function returns.

    The scatter/gather list is freed by calling PutScatterGatherList.

Arguments:

    DmaAdapter - Pointer to the adapter control object to allocate to the
        driver.

    DeviceObject - Pointer to the device object that is allocating the
        adapter.

    Mdl - Pointer to the MDL that describes the pages of memory that are being
        read or written.

    CurrentVa - Current virtual address in the buffer described by the MDL
        that the transfer is being done to or from.

    Length - Supplies the length of the transfer.

    ExecutionRoutine - The address of the driver's execution routine that is
        invoked once the adapter channel (and possibly map registers) have been
        allocated.

    Context - An untyped longword context parameter passed to the driver's
        execution routine.

    WriteToDevice - Supplies the value that indicates whether this is a
        write to the device from memory (TRUE), or vice versa.

Return Value:

    Returns STATUS_SUCESS unless too many map registers are requested or
    memory for the scatter/gather list could not be allocated.

Notes:

    Note that this routine MUST be invoked at DISPATCH_LEVEL or above.

    The data in the buffer cannot be accessed until the put scatter/gather function has been called.

--*/

{
    PXHAL_WAIT_CONTEXT_BLOCK WaitBlock;
    PWAIT_CONTEXT_BLOCK Wcb;
    PMDL TempMdl;
    ULONG NumberOfMapRegisters;
    ULONG ContextSize;
    ULONG TransferLength;
    ULONG MdlLength;
    ULONG MdlCount;
    PUCHAR MdlVa;
    NTSTATUS Status;

    MdlVa = MmGetMdlVirtualAddress(Mdl);

    //
    // Calculate the number of required map registers.
    //

    TempMdl = Mdl;
    TransferLength = TempMdl->ByteCount - (ULONG)((PUCHAR) CurrentVa - MdlVa);
    MdlLength = TransferLength;

    MdlVa = (PUCHAR) BYTE_OFFSET(CurrentVa);
    NumberOfMapRegisters = 0;
    MdlCount = 1;

    //
    // Loop through the any chained MDLs accumulating the the required
    // number of map registers.
    //

    while (TransferLength < Length && TempMdl->Next != NULL) {

        NumberOfMapRegisters += (ULONG)(((ULONG_PTR) MdlVa + MdlLength + PAGE_SIZE - 1) >>
                                    PAGE_SHIFT);

        TempMdl = TempMdl->Next;
        MdlVa = (PUCHAR) TempMdl->ByteOffset;
        MdlLength = TempMdl->ByteCount;
        TransferLength += MdlLength;
        MdlCount++;
    }

    if (TransferLength + PAGE_SIZE < (ULONG_PTR)(Length + MdlVa) ) {
        ASSERT(TransferLength >= Length);
        return(STATUS_BUFFER_TOO_SMALL);
    }

    //
    // Calculate the last number of map registers base on the requested
    // length not the length of the last MDL.
    //

    ASSERT( TransferLength <= MdlLength + Length );

    NumberOfMapRegisters += (ULONG)(((ULONG_PTR) MdlVa + Length + MdlLength - TransferLength +
                             PAGE_SIZE - 1) >> PAGE_SHIFT);

    //
    // Calculate how much memory is required for context structure.  This
    // this actually layed out as follows:
    //
    //   XHAL_WAIT_CONTEXT_BLOCK;
    //   MapRegisterBase[ MdlCount ];
    //   union {
    //      WAIT_CONTEXT_BLOCK[ MdlCount ];
    //      SCATTER_GATER_LIST [ NumberOfMapRegisters ];
    //   };
    //

    ContextSize = NumberOfMapRegisters * sizeof( SCATTER_GATHER_ELEMENT ) +
                  sizeof( SCATTER_GATHER_LIST );

    //
    // For each Mdl a separate Wcb is required since a separate map
    // registers base must be allocated.
    //

    if (ContextSize < sizeof( WAIT_CONTEXT_BLOCK ) * MdlCount) {

        ContextSize = sizeof( WAIT_CONTEXT_BLOCK ) * MdlCount;
    }

    ContextSize += sizeof( XHAL_WAIT_CONTEXT_BLOCK ) +
                    MdlCount * sizeof( PVOID );
    WaitBlock = ExAllocatePoolWithTag( NonPagedPool, ContextSize, ' laH' );

    if (WaitBlock == NULL) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    //
    // Store the wait context block at the end of our block.
    // All of the information in wait block can be over written
    // by the scatter/gather list.
    //

    Wcb = (PWAIT_CONTEXT_BLOCK) ((PVOID *) (WaitBlock + 1) + MdlCount);

    //
    // Save the interesting data in the wait block.
    //

    WaitBlock->Mdl = Mdl;
    WaitBlock->CurrentVa = CurrentVa;
    WaitBlock->Length = Length;
    WaitBlock->RealAdapterObject = ((PADAPTER_OBJECT) DmaAdapter)->RealAdapterObject;
    WaitBlock->DriverExecutionRoutine = ExecutionRoutine;
    WaitBlock->DriverContext = Context;
    WaitBlock->CurrentIrp = DeviceObject->CurrentIrp;
    WaitBlock->MapRegisterLock = MdlCount;
    WaitBlock->WriteToDevice = WriteToDevice;
    WaitBlock->MdlCount = (UCHAR) MdlCount;

    //
    // Loop through each of the required MDL's calling
    // IoAllocateAdapterChannel.
    //

    MdlCount = 0;

    TempMdl = Mdl;
    TransferLength = Length;
    MdlLength = TempMdl->ByteCount - (ULONG)((PUCHAR) CurrentVa - (PUCHAR) MmGetMdlVirtualAddress(Mdl));

    MdlVa = (PUCHAR) BYTE_OFFSET(CurrentVa);
    NumberOfMapRegisters = 0;

    //
    // Loop through the any chained MDLs accumulating the the required
    // number of map registers.
    //

    while (TransferLength > 0) {

        if (MdlLength > TransferLength) {

            MdlLength = TransferLength;
        }

        TransferLength -= MdlLength;

        NumberOfMapRegisters = (ULONG)(((ULONG_PTR) MdlVa + MdlLength + PAGE_SIZE - 1) >>
                                    PAGE_SHIFT);

        Wcb->DeviceContext = WaitBlock;
        Wcb->DeviceObject = DeviceObject;

        //
        // Store the map register index in IRP pointer.
        //

        Wcb->CurrentIrp = (PVOID) MdlCount;

        //
        // Call the HAL to allocate the adapter channel.
        // xHalpAllocateAdapterCallback will fill in the scatter/gather list.
        //

        Status = HalAllocateAdapterChannel( ((PADAPTER_OBJECT) DmaAdapter)->RealAdapterObject,
                                            Wcb,
                                            NumberOfMapRegisters,
                                            xHalpAllocateAdapterCallback );

        if (TempMdl->Next == NULL) {
            break;
        }

        //
        // Advance to next MDL.
        //

        TempMdl = TempMdl->Next;
        MdlVa = (PUCHAR) TempMdl->ByteOffset;
        MdlLength = TempMdl->ByteCount;
        MdlCount++;
        Wcb++;
    }

    //
    // If HalAllocateAdapterChannel failed then free the wait block.
    //

    if (!NT_SUCCESS( Status)) {
        ExFreePool( WaitBlock );
    }

    return( Status );
}



VOID
xHalPutScatterGatherList (
    IN PDMA_ADAPTER DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN BOOLEAN WriteToDevice
    )
{
    PXHAL_WAIT_CONTEXT_BLOCK WaitBlock = (PVOID) ScatterGather->Reserved;
    ULONG TransferLength;
    ULONG MdlLength;
    ULONG MdlCount = 0;
    PMDL Mdl;
    PUCHAR CurrentVa;

    //
    // Setup for the first MDL.  We expect the MDL pointer to be pointing
    // at the first used mdl.
    //

    Mdl = WaitBlock->Mdl;
    CurrentVa = WaitBlock->CurrentVa;
    ASSERT( CurrentVa >= (PUCHAR) MmGetMdlVirtualAddress(Mdl) && CurrentVa < (PUCHAR) MmGetMdlVirtualAddress(Mdl) + Mdl->ByteCount );

    MdlLength = Mdl->ByteCount - (ULONG)(CurrentVa - (PUCHAR) MmGetMdlVirtualAddress(Mdl));
    TransferLength = WaitBlock->Length;

    //
    // Loop through the used MDLs call IoFlushAdapterBuffers.
    //

    while (TransferLength >  0) {

        if (MdlLength > TransferLength) {

            MdlLength = TransferLength;
        }

        TransferLength -= MdlLength;

        IoFlushAdapterBuffers( WaitBlock->RealAdapterObject,
                                Mdl,
                                WaitBlock->MapRegisterBase[MdlCount],
                                CurrentVa,
                                MdlLength,
                                WriteToDevice );


        if (Mdl->Next == NULL) {
            break;
        }

        //
        // Advance to the next MDL.  Update the current VA and the MdlLength.
        //

        Mdl = Mdl->Next;
        CurrentVa = MmGetMdlVirtualAddress(Mdl);
        MdlLength = Mdl->ByteCount;
        MdlCount++;
    }

    ExFreePool( WaitBlock );

}

IO_ALLOCATION_ACTION
xHalpAllocateAdapterCallback (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the adapter object and map registers are
    available for the data transfer. This routines saves the map register
    base away.  If all of the required bases have not been saved then it
    returns. Otherwise it routine builds the entire scatter/gather
    list by calling IoMapTransfer.  After the list is build it is passed to
    the driver.

Arguments:

    DeviceObject - Pointer to the device object that is allocating the
        adapter.

    Irp - Supplies the map register offset assigned for this callback.

    MapRegisterBase - Supplies the map register base for use by the adapter
        routines.

    Context - Supplies a pointer to the xhal wait contorl block.

Return Value:

    Returns DeallocateObjectKeepRegisters.


--*/
{
    PXHAL_WAIT_CONTEXT_BLOCK WaitBlock = Context;
    PVOID *MapRegisterBasePtr;
    ULONG TransferLength;
    LONG MdlLength;
    PMDL Mdl;
    PUCHAR CurrentVa;
    PSCATTER_GATHER_LIST ScatterGather;
    PSCATTER_GATHER_ELEMENT Element;

    //
    // Save the map register base in the appropriate slot.
    //

    WaitBlock->MapRegisterBase[ (ULONG_PTR) Irp ] = MapRegisterBase;

    //
    // See if this is the last callback;
    //

    if (InterlockedDecrement( &WaitBlock->MapRegisterLock ) != 0) {

        //
        // More to come, wait for the rest.
        //

        return( DeallocateObjectKeepRegisters );

    }

    //
    // Put the scatter gatther list after wait block. Add a back pointer to
    // the begining of the wait block.
    //

    MapRegisterBasePtr = (PVOID *) (WaitBlock + 1);
    ScatterGather = (PSCATTER_GATHER_LIST) (MapRegisterBasePtr +
                        WaitBlock->MdlCount);
    ScatterGather->Reserved = (ULONG_PTR) WaitBlock;
    Element = ScatterGather->Elements;

    //
    // Setup for the first MDL.  We expect the MDL pointer to be pointing
    // at the first used MDL.
    //

    Mdl = WaitBlock->Mdl;
    CurrentVa = WaitBlock->CurrentVa;
    ASSERT( CurrentVa >= (PUCHAR) MmGetMdlVirtualAddress(Mdl) && CurrentVa < (PUCHAR) MmGetMdlVirtualAddress(Mdl) + Mdl->ByteCount );

    MdlLength = Mdl->ByteCount - (ULONG)(CurrentVa - (PUCHAR) MmGetMdlVirtualAddress(Mdl));
    TransferLength = WaitBlock->Length;

    //
    // Loop build the list for each MDL.
    //

    while (TransferLength >  0) {

        if ((ULONG) MdlLength > TransferLength) {

            MdlLength = TransferLength;
        }

        TransferLength -= MdlLength;

        //
        // Loop building the list for the elments within and MDL.
        //

        while (MdlLength > 0) {

            Element->Length = MdlLength;
            Element->Address = IoMapTransfer( WaitBlock->RealAdapterObject,
                                            Mdl,
                                            *MapRegisterBasePtr,
                                            CurrentVa,
                                            &Element->Length,
                                            WaitBlock->WriteToDevice );

            ASSERT( (ULONG) MdlLength >= Element->Length );
            MdlLength -= Element->Length;
            CurrentVa += Element->Length;
            Element++;
        }

        if (Mdl->Next == NULL) {

            //
            // There is a few cases where the buffer described by the MDL
            // is less than the transfer length.  This occurs when the
            // file system transfering the last page of file and MM defines
            // the MDL to be file size and the file system round the write
            // up to a sector.  This extra should never cross a page
            // bountry. Add this extra to the length of the last element.
            //

            ASSERT(((Element - 1)->Length & (PAGE_SIZE - 1)) + TransferLength <= PAGE_SIZE );
            (Element - 1)->Length += TransferLength;

            break;
        }

        //
        // Advance to the next MDL.  Update the current VA and the MdlLength.
        //

        Mdl = Mdl->Next;
        CurrentVa = MmGetMdlVirtualAddress(Mdl);
        MdlLength = Mdl->ByteCount;
        MapRegisterBasePtr++;

    }

    //
    // Set the number of elements actually used.
    //

    ScatterGather->NumberOfElements = (ULONG)(Element - ScatterGather->Elements);

    //
    // Call the driver with the scatter/gather list.
    //

    WaitBlock->DriverExecutionRoutine( DeviceObject,
                                       WaitBlock->CurrentIrp,
                                       ScatterGather,
                                       WaitBlock->DriverContext );

    return( DeallocateObjectKeepRegisters );
}
#endif
BOOLEAN
xHalTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )
{
    //
    // If the HAL fails to override this function, then
    // the HAL has clearly failed to initialize.
    //

    KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0, 0, 0, 7);
    return FALSE;
}

NTSTATUS
xHalAssignSlotResources (
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    )
{
    //
    // If the HAL fails to override this function, then
    // the HAL has clearly failed to initialize.
    //

    KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0, 0, 0, 7);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
xHalHaltSystem(
    VOID
    )
{
    for (;;) ;
}

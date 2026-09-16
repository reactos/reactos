/*
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            hal/halx86/generic/dma.c
 * PURPOSE:         DMA functions
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Filip Navara (navaraf@reactos.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/**
 * @page DMA Implementation Notes
 *
 * Concepts:
 *
 * - Map register
 *
 *   Abstract encapsulation of physically contiguous buffer that resides
 *   in memory accessible by both the DMA device / controller and the system.
 *   The map registers are allocated and distributed on demand and are
 *   scarce resource.
 *
 *   The actual use of map registers is to allow transfers from/to buffer
 *   located in physical memory at address inaccessible by the DMA device /
 *   controller directly. For such transfers the map register buffers
 *   are used as intermediate data storage.
 *
 * - Master adapter
 *
 *   A container for map registers (typically corresponding to one physical
 *   bus connection type). There can be master adapters for 24-bit address
 *   ranges, 32-bit address ranges, etc. Every time a new DMA adapter is
 *   created it's associated with a corresponding master adapter that
 *   is used for any map register allocation requests.
 *
 * - Bus-master / Slave DMA
 *
 *   Slave DMA is term used for DMA transfers done by the system (E)ISA
 *   controller as opposed to transfers mastered by the device itself
 *   (hence the name).
 *
 *   For slave DMA special care is taken to actually access the system
 *   controller and handle the transfers. The relevant code is in
 *   HalpDmaInitializeEisaAdapter, HalReadDmaCounter, IoFlushAdapterBuffers
 *   and IoMapTransfer.
 *
 * Implementation:
 *
 * - Allocation of map registers
 *
 *   Initial set of map registers is allocated on the system start to
 *   ensure that low memory won't get filled up later. Additional map
 *   registers are allocated as needed by HalpGrowMapBuffers. This
 *   routine is called on two places:
 *
 *   - HalGetAdapter, since we're at PASSIVE_LEVEL and it's known that
 *     more map registers will probably be needed.
 *   - IoAllocateAdapterChannel (indirectly using HalpGrowMapBufferWorker
 *     since we're at DISPATCH_LEVEL and call HalpGrowMapBuffers directly)
 *     when no more map registers are free.
 *
 *   Note that even if no more map registers can be allocated it's not
 *   the end of the world. The adapters waiting for free map registers
 *   are queued in the master adapter's queue and once one driver hands
 *   back it's map registers (using IoFreeMapRegisters or indirectly using
 *   the execution routine callback in IoAllocateAdapterChannel) the
 *   queue gets processed and the map registers are reassigned.
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#include <suppress.h>

#define NDEBUG
#include <debug.h>

#ifndef _MINIHAL_
static KEVENT HalpDmaLock;
static KSPIN_LOCK HalpDmaAdapterListLock;
static LIST_ENTRY HalpDmaAdapterList;
static PADAPTER_OBJECT HalpEisaAdapter[8];
#endif
static BOOLEAN HalpEisaDma;
#ifndef _MINIHAL_
static PADAPTER_OBJECT HalpMasterAdapter;
#endif

static const ULONG_PTR HalpEisaPortPage[8] = {
   FIELD_OFFSET(DMA_PAGE, Channel0),
   FIELD_OFFSET(DMA_PAGE, Channel1),
   FIELD_OFFSET(DMA_PAGE, Channel2),
   FIELD_OFFSET(DMA_PAGE, Channel3),
   0,
   FIELD_OFFSET(DMA_PAGE, Channel5),
   FIELD_OFFSET(DMA_PAGE, Channel6),
   FIELD_OFFSET(DMA_PAGE, Channel7)
};

#ifndef _MINIHAL_
NTSTATUS
NTAPI
HalCalculateScatterGatherListSize(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl OPTIONAL,
    IN PVOID CurrentVa,
    IN ULONG Length,
    OUT PULONG ScatterGatherListSize,
    OUT PULONG pNumberOfMapRegisters);

NTSTATUS
NTAPI
HalBuildScatterGatherList(
    IN PADAPTER_OBJECT AdapterObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice,
    IN PVOID ScatterGatherBuffer,
    IN ULONG ScatterGatherLength);

NTSTATUS
NTAPI
HalBuildMdlFromScatterGatherList(
    IN PDMA_ADAPTER DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PMDL OriginalMdl,
    OUT PMDL *TargetMdl);


static DMA_OPERATIONS HalpDmaOperations = {
   sizeof(DMA_OPERATIONS),
   (PPUT_DMA_ADAPTER)HalPutDmaAdapter,
   (PALLOCATE_COMMON_BUFFER)HalAllocateCommonBuffer,
   (PFREE_COMMON_BUFFER)HalFreeCommonBuffer,
   NULL, /* Initialized in HalpInitDma() */
   NULL, /* Initialized in HalpInitDma() */
   NULL, /* Initialized in HalpInitDma() */
   NULL, /* Initialized in HalpInitDma() */
   NULL, /* Initialized in HalpInitDma() */
   (PGET_DMA_ALIGNMENT)HalpDmaGetDmaAlignment,
   (PREAD_DMA_COUNTER)HalReadDmaCounter,
   (PGET_SCATTER_GATHER_LIST)HalGetScatterGatherList,
   (PPUT_SCATTER_GATHER_LIST)HalPutScatterGatherList,
   (PCALCULATE_SCATTER_GATHER_LIST_SIZE)HalCalculateScatterGatherListSize,
   (PBUILD_SCATTER_GATHER_LIST)HalBuildScatterGatherList,
   (PBUILD_MDL_FROM_SCATTER_GATHER_LIST)HalBuildMdlFromScatterGatherList
};

static
NTSTATUS
NTAPI
HalGetDmaAdapterInfo(
    _In_ PADAPTER_OBJECT AdapterObject,
    _Inout_ PDMA_ADAPTER_INFO AdapterInfo);

static
NTSTATUS
NTAPI
HalGetDmaTransferInfo(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PMDL Mdl,
    _In_ ULONGLONG Offset,
    _In_ ULONG Length,
    _In_ BOOLEAN WriteOnly,
    _Inout_ PDMA_TRANSFER_INFO TransferInfo);

static
NTSTATUS
NTAPI
HalInitializeDmaTransferContext(
    _In_ PADAPTER_OBJECT AdapterObject,
    _Out_ PVOID DmaTransferContext);

static
PVOID
NTAPI
HalAllocateCommonBufferEx(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_opt_ PPHYSICAL_ADDRESS MaximumAddress,
    _In_ ULONG Length,
    _Out_ PPHYSICAL_ADDRESS LogicalAddress,
    _In_ BOOLEAN CacheEnabled,
    _In_ NODE_REQUIREMENT PreferredNode);

static
NTSTATUS
NTAPI
HalAllocateAdapterChannelEx(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID DmaTransferContext,
    _In_ ULONG NumberOfMapRegisters,
    _In_ ULONG Flags,
    _In_opt_ PDRIVER_CONTROL ExecutionRoutine,
    _In_opt_ PVOID ExecutionContext,
    _Out_opt_ PVOID *MapRegisterBase);

static
NTSTATUS
NTAPI
HalConfigureAdapterChannel(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ ULONG FunctionNumber,
    _In_ PVOID Context);

static
BOOLEAN
NTAPI
HalCancelAdapterChannel(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID DmaTransferContext);

static
NTSTATUS
NTAPI
HalMapTransferEx(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PMDL Mdl,
    _In_ PVOID MapRegisterBase,
    _In_ ULONGLONG Offset,
    _In_ ULONG DeviceOffset,
    _Inout_ PULONG Length,
    _In_ BOOLEAN WriteToDevice,
    _Out_writes_bytes_opt_(ScatterGatherBufferLength) PSCATTER_GATHER_LIST ScatterGatherBuffer,
    _In_ ULONG ScatterGatherBufferLength,
    _In_opt_ PDMA_COMPLETION_ROUTINE DmaCompletionRoutine,
    _In_opt_ PVOID CompletionContext);

static
NTSTATUS
NTAPI
HalGetScatterGatherListEx(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID DmaTransferContext,
    _In_ PMDL Mdl,
    _In_ ULONGLONG Offset,
    _In_ ULONG Length,
    _In_ ULONG Flags,
    _In_opt_ PDRIVER_LIST_CONTROL ExecutionRoutine,
    _In_opt_ PVOID Context,
    _In_ BOOLEAN WriteToDevice,
    _In_opt_ PDMA_COMPLETION_ROUTINE DmaCompletionRoutine,
    _In_opt_ PVOID CompletionContext,
    _Out_opt_ PSCATTER_GATHER_LIST *ScatterGatherList);

static
NTSTATUS
NTAPI
HalBuildScatterGatherListEx(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID DmaTransferContext,
    _In_ PMDL Mdl,
    _In_ ULONGLONG Offset,
    _In_ ULONG Length,
    _In_ ULONG Flags,
    _In_opt_ PDRIVER_LIST_CONTROL ExecutionRoutine,
    _In_opt_ PVOID Context,
    _In_ BOOLEAN WriteToDevice,
    _In_ PVOID ScatterGatherBuffer,
    _In_ ULONG ScatterGatherBufferLength,
    _In_opt_ PDMA_COMPLETION_ROUTINE DmaCompletionRoutine,
    _In_opt_ PVOID CompletionContext,
    _Out_opt_ PSCATTER_GATHER_LIST *ScatterGatherList);

static
NTSTATUS
NTAPI
HalFlushAdapterBuffersEx(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PMDL Mdl,
    _In_ PVOID MapRegisterBase,
    _In_ ULONGLONG Offset,
    _In_ ULONG Length,
    _In_ BOOLEAN WriteToDevice);

static
VOID
NTAPI
HalFreeAdapterObject(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ IO_ALLOCATION_ACTION AllocationAction);

static
NTSTATUS
NTAPI
HalCancelMappedTransfer(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PVOID DmaTransferContext);

/* Operations of DEVICE_DESCRIPTION_VERSION3 adapters, filled in HalpInitDma() */
static DMA_OPERATIONS HalpDmaOperationsV3;
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
#define HalpGetDeviceDmaAddressWidth(Description) ((Description)->DmaAddressWidth)
#else
/* Fields a DEVICE_DESCRIPTION_VERSION3 caller adds after DmaPort */
typedef struct _HALP_DEVICE_DESCRIPTION_V3
{
    DEVICE_DESCRIPTION Description;
    ULONG DmaAddressWidth;
    ULONG DmaControllerInstance;
    ULONG DmaRequestLine;
    PHYSICAL_ADDRESS DeviceAddress;
} HALP_DEVICE_DESCRIPTION_V3, *PHALP_DEVICE_DESCRIPTION_V3;

#define HalpGetDeviceDmaAddressWidth(Description) \
    (((PHALP_DEVICE_DESCRIPTION_V3)(Description))->DmaAddressWidth)
#endif

#define MAX_MAP_REGISTERS 64

#define TAG_DMA ' AMD'

/* FUNCTIONS *****************************************************************/

#if defined(SARCH_PC98)
/*
 * Disable I/O for safety.
 * FIXME: Add support for PC-98 DMA controllers.
 */
#undef WRITE_PORT_UCHAR
#undef READ_PORT_UCHAR

#define WRITE_PORT_UCHAR(Port, Data) \
    do { \
        UNIMPLEMENTED; \
        (Port); \
        (Data); \
    } while (0)

#define READ_PORT_UCHAR(Port) 0x00
#endif

#ifndef _MINIHAL_
CODE_SEG("INIT")
VOID
HalpInitDma(VOID)
{
    /*
     * Initialize the DMA Operation table
     */
    HalpDmaOperations.AllocateAdapterChannel = (PALLOCATE_ADAPTER_CHANNEL)IoAllocateAdapterChannel;
    HalpDmaOperations.FlushAdapterBuffers = (PFLUSH_ADAPTER_BUFFERS)IoFlushAdapterBuffers;
    HalpDmaOperations.FreeAdapterChannel = (PFREE_ADAPTER_CHANNEL)IoFreeAdapterChannel;
    HalpDmaOperations.FreeMapRegisters = (PFREE_MAP_REGISTERS)IoFreeMapRegisters;
    HalpDmaOperations.MapTransfer = (PMAP_TRANSFER)IoMapTransfer;

    /* Version 3 adapters add the transfer context based operations */
    HalpDmaOperationsV3 = HalpDmaOperations;
    HalpDmaOperationsV3.Size = FIELD_OFFSET(DMA_OPERATIONS, AllocateDomainCommonBuffer);
    HalpDmaOperationsV3.GetDmaAdapterInfo = (PGET_DMA_ADAPTER_INFO)HalGetDmaAdapterInfo;
    HalpDmaOperationsV3.GetDmaTransferInfo = (PGET_DMA_TRANSFER_INFO)HalGetDmaTransferInfo;
    HalpDmaOperationsV3.InitializeDmaTransferContext =
        (PINITIALIZE_DMA_TRANSFER_CONTEXT)HalInitializeDmaTransferContext;
    HalpDmaOperationsV3.AllocateCommonBufferEx =
        (PALLOCATE_COMMON_BUFFER_EX)HalAllocateCommonBufferEx;
    HalpDmaOperationsV3.AllocateAdapterChannelEx =
        (PALLOCATE_ADAPTER_CHANNEL_EX)HalAllocateAdapterChannelEx;
    HalpDmaOperationsV3.ConfigureAdapterChannel =
        (PCONFIGURE_ADAPTER_CHANNEL)HalConfigureAdapterChannel;
    HalpDmaOperationsV3.CancelAdapterChannel = (PCANCEL_ADAPTER_CHANNEL)HalCancelAdapterChannel;
    HalpDmaOperationsV3.MapTransferEx = (PMAP_TRANSFER_EX)HalMapTransferEx;
    HalpDmaOperationsV3.GetScatterGatherListEx =
        (PGET_SCATTER_GATHER_LIST_EX)HalGetScatterGatherListEx;
    HalpDmaOperationsV3.BuildScatterGatherListEx =
        (PBUILD_SCATTER_GATHER_LIST_EX)HalBuildScatterGatherListEx;
    HalpDmaOperationsV3.FlushAdapterBuffersEx = (PFLUSH_ADAPTER_BUFFERS_EX)HalFlushAdapterBuffersEx;
    HalpDmaOperationsV3.FreeAdapterObject = (PFREE_ADAPTER_OBJECT)HalFreeAdapterObject;
    HalpDmaOperationsV3.CancelMappedTransfer = (PCANCEL_MAPPED_TRANSFER)HalCancelMappedTransfer;

    if (HalpBusType == MACHINE_TYPE_EISA)
    {
        /*
        * Check if Extended DMA is available. We're just going to do a random
        * read and write.
        */
        WRITE_PORT_UCHAR(UlongToPtr(FIELD_OFFSET(EISA_CONTROL, DmaController2Pages.Channel2)), 0x2A);
        if (READ_PORT_UCHAR(UlongToPtr(FIELD_OFFSET(EISA_CONTROL, DmaController2Pages.Channel2))) == 0x2A)
        {
            DPRINT1("Machine supports EISA DMA. Bus type: %lu\n", HalpBusType);
            HalpEisaDma = TRUE;
        }
    }

    /*
     * Intialize all the global variables and allocate master adapter with
     * first map buffers.
     */
    InitializeListHead(&HalpDmaAdapterList);
    KeInitializeSpinLock(&HalpDmaAdapterListLock);
    KeInitializeEvent(&HalpDmaLock, NotificationEvent, TRUE);
    HalpMasterAdapter = HalpDmaAllocateMasterAdapter();

    /*
     * Setup the HalDispatchTable callback for creating PnP DMA adapters. It's
     * used by IoGetDmaAdapter in the kernel.
     */
    HalGetDmaAdapter = HalpGetDmaAdapter;
}
#endif

/**
 * @name HalpGetAdapterMaximumPhysicalAddress
 *
 * Get the maximum physical address acceptable by the device represented
 * by the passed DMA adapter.
 */
PHYSICAL_ADDRESS
NTAPI
HalpGetAdapterMaximumPhysicalAddress(IN PADAPTER_OBJECT AdapterObject)
{
    PHYSICAL_ADDRESS HighestAddress;

    if (AdapterObject->MasterDevice)
    {
        if (AdapterObject->DmaAddressWidth >= 64)
        {
            HighestAddress.QuadPart = 0xFFFFFFFFFFFFFFFFULL;
            return HighestAddress;
        }
        else if (AdapterObject->DmaAddressWidth > 24)
        {
            HighestAddress.QuadPart = (1ULL << AdapterObject->DmaAddressWidth) - 1;
            return HighestAddress;
        }
    }

    HighestAddress.QuadPart = 0xFFFFFF;
    return HighestAddress;
}

#ifndef _MINIHAL_
/**
 * @name HalpGrowMapBuffers
 *
 * Allocate initial, or additional, map buffers for DMA master adapter.
 *
 * @param MasterAdapter
 *        DMA master adapter to allocate buffers for.
 * @param SizeOfMapBuffers
 *        Size of the map buffers to allocate (not including the size
 *        already allocated).
 */
BOOLEAN
NTAPI
HalpGrowMapBuffers(IN PADAPTER_OBJECT AdapterObject,
                  IN ULONG SizeOfMapBuffers)
{
    PVOID VirtualAddress;
    PHYSICAL_ADDRESS PhysicalAddress;
    PHYSICAL_ADDRESS HighestAcceptableAddress;
    PHYSICAL_ADDRESS LowestAcceptableAddress;
    PHYSICAL_ADDRESS BoundryAddressMultiple;
    KIRQL OldIrql;
    ULONG MapRegisterCount;

    /* Check if enough map register slots are available. */
    MapRegisterCount = BYTES_TO_PAGES(SizeOfMapBuffers);
    if (MapRegisterCount + AdapterObject->NumberOfMapRegisters > MAX_MAP_REGISTERS)
    {
        DPRINT("No more map register slots available! (Current: %d | Requested: %d | Limit: %d)\n",
               AdapterObject->NumberOfMapRegisters,
               MapRegisterCount,
               MAX_MAP_REGISTERS);
        return FALSE;
    }

    /*
     * Allocate memory for the new map registers. For 32-bit adapters we use
     * two passes in order not to waste scare resource (low memory).
     */
    HighestAcceptableAddress = HalpGetAdapterMaximumPhysicalAddress(AdapterObject);
    LowestAcceptableAddress.HighPart = 0;
    LowestAcceptableAddress.LowPart = HighestAcceptableAddress.LowPart == 0xFFFFFFFF ? 0x1000000 : 0;
    BoundryAddressMultiple.QuadPart = 0;

    VirtualAddress = MmAllocateContiguousMemorySpecifyCache(MapRegisterCount << PAGE_SHIFT,
                                                            LowestAcceptableAddress,
                                                            HighestAcceptableAddress,
                                                            BoundryAddressMultiple,
                                                            MmNonCached);
    if (!(VirtualAddress) && (LowestAcceptableAddress.LowPart))
    {
        LowestAcceptableAddress.LowPart = 0;
        VirtualAddress = MmAllocateContiguousMemorySpecifyCache(MapRegisterCount << PAGE_SHIFT,
                                                                LowestAcceptableAddress,
                                                                HighestAcceptableAddress,
                                                                BoundryAddressMultiple,
                                                                MmNonCached);
    }

    if (!VirtualAddress) return FALSE;

    PhysicalAddress = MmGetPhysicalAddress(VirtualAddress);

    /*
     * All the following must be done with the master adapter lock held
     * to prevent corruption.
     */
    KeAcquireSpinLock(&AdapterObject->SpinLock, &OldIrql);

    /*
     * Setup map register entries for the buffer allocated. Each entry has
     * a virtual and physical address and corresponds to PAGE_SIZE large
     * buffer.
     */
    if (MapRegisterCount > 0)
    {
        PROS_MAP_REGISTER_ENTRY CurrentEntry, PreviousEntry;

        CurrentEntry = AdapterObject->MapRegisterBase + AdapterObject->NumberOfMapRegisters;
        do
        {
            /*
             * Leave one entry free for every non-contiguous memory region
             * in the map register bitmap. This ensures that we can search
             * using RtlFindClearBits for contiguous map register regions.
             *
             * Also for non-EISA DMA leave one free entry for every 64Kb
             * break, because the DMA controller can handle only coniguous
             * 64Kb regions.
             */
            if (CurrentEntry != AdapterObject->MapRegisterBase)
            {
                PreviousEntry = CurrentEntry - 1;
                if ((PreviousEntry->PhysicalAddress.LowPart + PAGE_SIZE) == PhysicalAddress.LowPart)
                {
                    if (!HalpEisaDma)
                    {
                        if ((PreviousEntry->PhysicalAddress.LowPart ^ PhysicalAddress.LowPart) & 0xFFFF0000)
                        {
                            CurrentEntry++;
                            AdapterObject->NumberOfMapRegisters++;
                        }
                    }
                }
                else
                {
                    CurrentEntry++;
                    AdapterObject->NumberOfMapRegisters++;
                }
            }

            RtlClearBit(AdapterObject->MapRegisters,
                        (ULONG)(CurrentEntry - AdapterObject->MapRegisterBase));
            CurrentEntry->VirtualAddress = VirtualAddress;
            CurrentEntry->PhysicalAddress = PhysicalAddress;

            PhysicalAddress.LowPart += PAGE_SIZE;
            VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + PAGE_SIZE);

            CurrentEntry++;
            AdapterObject->NumberOfMapRegisters++;
            MapRegisterCount--;
        } while (MapRegisterCount);
    }

    KeReleaseSpinLock(&AdapterObject->SpinLock, OldIrql);

    return TRUE;
}

/**
 * @name HalpDmaAllocateMasterAdapter
 *
 * Helper routine to allocate and initialize master adapter object and it's
 * associated map register buffers.
 *
 * @see HalpInitDma
 */
PADAPTER_OBJECT
NTAPI
HalpDmaAllocateMasterAdapter(VOID)
{
    PADAPTER_OBJECT MasterAdapter;
    ULONG Size, SizeOfBitmap;

    SizeOfBitmap = MAX_MAP_REGISTERS;
    Size = sizeof(ADAPTER_OBJECT);
    Size += sizeof(RTL_BITMAP);
    Size += (SizeOfBitmap + 7) >> 3;

    MasterAdapter = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_DMA);
    if (!MasterAdapter) return NULL;

    RtlZeroMemory(MasterAdapter, Size);

    KeInitializeSpinLock(&MasterAdapter->SpinLock);
    InitializeListHead(&MasterAdapter->AdapterQueue);

    MasterAdapter->MapRegisters = (PVOID)(MasterAdapter + 1);
    RtlInitializeBitMap(MasterAdapter->MapRegisters,
                        (PULONG)(MasterAdapter->MapRegisters + 1),
                        SizeOfBitmap);
    RtlSetAllBits(MasterAdapter->MapRegisters);
    MasterAdapter->NumberOfMapRegisters = 0;
    MasterAdapter->CommittedMapRegisters = 0;

    MasterAdapter->MapRegisterBase = ExAllocatePoolWithTag(NonPagedPool,
                                                           SizeOfBitmap *
                                                           sizeof(ROS_MAP_REGISTER_ENTRY),
                                                           TAG_DMA);
    if (!MasterAdapter->MapRegisterBase)
    {
        ExFreePool(MasterAdapter);
        return NULL;
    }

    RtlZeroMemory(MasterAdapter->MapRegisterBase,
                  SizeOfBitmap * sizeof(ROS_MAP_REGISTER_ENTRY));
    if (!HalpGrowMapBuffers(MasterAdapter, 0x10000))
    {
        ExFreePool(MasterAdapter);
        return NULL;
    }

    return MasterAdapter;
}

/**
 * @name HalpDmaAllocateChildAdapter
 *
 * Helper routine of HalGetAdapter. Allocate child adapter object and
 * fill out some basic fields.
 *
 * @see HalGetAdapter
 */
PADAPTER_OBJECT
NTAPI
HalpDmaAllocateChildAdapter(IN ULONG NumberOfMapRegisters,
                            IN PDEVICE_DESCRIPTION DeviceDescription)
{
    PADAPTER_OBJECT AdapterObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE Handle;

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE | OBJ_PERMANENT,
                               NULL,
                               NULL);

    Status = ObCreateObject(KernelMode,
                            IoAdapterObjectType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(ADAPTER_OBJECT),
                            0,
                            0,
                            (PVOID)&AdapterObject);
    if (!NT_SUCCESS(Status)) return NULL;

    RtlZeroMemory(AdapterObject, sizeof(ADAPTER_OBJECT));

    Status = ObInsertObject(AdapterObject,
                            NULL,
                            FILE_READ_DATA | FILE_WRITE_DATA,
                            0,
                            NULL,
                            &Handle);
    if (!NT_SUCCESS(Status)) return NULL;

    ObReferenceObject(AdapterObject);

    ZwClose(Handle);

    AdapterObject->DmaHeader.Version = (USHORT)DeviceDescription->Version;
    AdapterObject->DmaHeader.Size = sizeof(ADAPTER_OBJECT);
    if (DeviceDescription->Version >= DEVICE_DESCRIPTION_VERSION3)
        AdapterObject->DmaHeader.DmaOperations = &HalpDmaOperationsV3;
    else
        AdapterObject->DmaHeader.DmaOperations = &HalpDmaOperations;
    AdapterObject->MapRegistersPerChannel = 1;
    AdapterObject->Dma32BitAddresses = DeviceDescription->Dma32BitAddresses;
    AdapterObject->ChannelNumber = 0xFF;
    AdapterObject->MasterAdapter = HalpMasterAdapter;
    KeInitializeDeviceQueue(&AdapterObject->ChannelWaitQueue);

    return AdapterObject;
}
#endif

/**
 * @name HalpDmaInitializeEisaAdapter
 *
 * Setup DMA modes and extended modes for (E)ISA DMA adapter object.
 */
BOOLEAN
NTAPI
HalpDmaInitializeEisaAdapter(IN PADAPTER_OBJECT AdapterObject,
                             IN PDEVICE_DESCRIPTION DeviceDescription)
{
    UCHAR Controller;
    DMA_MODE DmaMode = {{0 }};
    DMA_EXTENDED_MODE ExtendedMode = {{ 0 }};
    PVOID AdapterBaseVa;

    Controller = (DeviceDescription->DmaChannel & 4) ? 2 : 1;

    if (Controller == 1)
    {
        AdapterBaseVa = UlongToPtr(FIELD_OFFSET(EISA_CONTROL, DmaController1));
    }
    else
    {
        AdapterBaseVa = UlongToPtr(FIELD_OFFSET(EISA_CONTROL, DmaController2));
    }

    AdapterObject->AdapterNumber = Controller;
    AdapterObject->ChannelNumber = (UCHAR)(DeviceDescription->DmaChannel & 3);
    AdapterObject->PagePort = (PUCHAR)HalpEisaPortPage[DeviceDescription->DmaChannel];
    AdapterObject->Width16Bits = FALSE;
    AdapterObject->AdapterBaseVa = AdapterBaseVa;

    if (HalpEisaDma)
    {
        ExtendedMode.ChannelNumber = AdapterObject->ChannelNumber;

        switch (DeviceDescription->DmaSpeed)
        {
            case Compatible: ExtendedMode.TimingMode = COMPATIBLE_TIMING; break;
            case TypeA: ExtendedMode.TimingMode = TYPE_A_TIMING; break;
            case TypeB: ExtendedMode.TimingMode = TYPE_B_TIMING; break;
            case TypeC: ExtendedMode.TimingMode = BURST_TIMING; break;
            default:
                return FALSE;
        }

        switch (DeviceDescription->DmaWidth)
        {
            case Width8Bits: ExtendedMode.TransferSize = B_8BITS; break;
            case Width16Bits: ExtendedMode.TransferSize = B_16BITS; break;
            case Width32Bits: ExtendedMode.TransferSize = B_32BITS; break;
            default:
                return FALSE;
        }

        if (Controller == 1)
        {
            WRITE_PORT_UCHAR(UlongToPtr(FIELD_OFFSET(EISA_CONTROL, DmaExtendedMode1)),
                            ExtendedMode.Byte);
        }
        else
        {
            WRITE_PORT_UCHAR(UlongToPtr(FIELD_OFFSET(EISA_CONTROL, DmaExtendedMode2)),
                            ExtendedMode.Byte);
        }
    }
    else
    {
        /*
         * Validate setup for non-busmaster DMA adapter. Secondary controller
         * supports only 16-bit transfers and main controller supports only
         * 8-bit transfers. Anything else is invalid.
         */
        if (!DeviceDescription->Master)
        {
            if ((Controller == 2) && (DeviceDescription->DmaWidth == Width16Bits))
            {
                AdapterObject->Width16Bits = TRUE;
            }
            else if ((Controller != 1) || (DeviceDescription->DmaWidth != Width8Bits))
            {
                return FALSE;
            }
        }
    }

    DmaMode.Channel = AdapterObject->ChannelNumber;
    DmaMode.AutoInitialize = DeviceDescription->AutoInitialize;

    /*
     * Set the DMA request mode.
     *
     * For (E)ISA bus master devices just unmask (enable) the DMA channel
     * and set it to cascade mode. Otherwise just select the right one
     * bases on the passed device description.
     */
    if (DeviceDescription->Master)
    {
        DmaMode.RequestMode = CASCADE_REQUEST_MODE;
        if (Controller == 1)
        {
            /* Set the Request Data */
            _PRAGMA_WARNING_SUPPRESS(__WARNING_DEREF_NULL_PTR)
            WRITE_PORT_UCHAR(&((PDMA1_CONTROL)AdapterBaseVa)->Mode, DmaMode.Byte);

            /* Unmask DMA Channel */
            WRITE_PORT_UCHAR(&((PDMA1_CONTROL)AdapterBaseVa)->SingleMask,
                             AdapterObject->ChannelNumber | DMA_CLEARMASK);
        }
        else
        {
            /* Set the Request Data */
            WRITE_PORT_UCHAR(&((PDMA2_CONTROL)AdapterBaseVa)->Mode, DmaMode.Byte);

            /* Unmask DMA Channel */
            WRITE_PORT_UCHAR(&((PDMA2_CONTROL)AdapterBaseVa)->SingleMask,
                             AdapterObject->ChannelNumber | DMA_CLEARMASK);
        }
    }
    else
    {
        if (DeviceDescription->DemandMode)
        {
            DmaMode.RequestMode = DEMAND_REQUEST_MODE;
        }
        else
        {
            DmaMode.RequestMode = SINGLE_REQUEST_MODE;
        }
    }

    AdapterObject->AdapterMode = DmaMode;

    return TRUE;
}

#ifndef _MINIHAL_
/**
 * @name HalGetAdapter
 *
 * Allocate an adapter object for DMA device.
 *
 * @param DeviceDescription
 *        Structure describing the attributes of the device.
 * @param NumberOfMapRegisters
 *        On return filled with the maximum number of map registers the
 *        device driver can allocate for DMA transfer operations.
 *
 * @return The DMA adapter on success, NULL otherwise.
 *
 * @implemented
 */
PADAPTER_OBJECT
NTAPI
HalGetAdapter(IN PDEVICE_DESCRIPTION DeviceDescription,
              OUT PULONG NumberOfMapRegisters)
{
    PADAPTER_OBJECT AdapterObject = NULL;
    BOOLEAN EisaAdapter;
    ULONG MapRegisters;
    ULONG MaximumLength;
    KIRQL OldIrql;

    /* Validate parameters in device description */
    if (DeviceDescription->Version > DEVICE_DESCRIPTION_VERSION3) return NULL;

    /* Version 3 slave devices name a DMA controller request line, and none are registered */
    if ((DeviceDescription->Version == DEVICE_DESCRIPTION_VERSION3) &&
        !(DeviceDescription->Master))
    {
        return NULL;
    }

    /*
     * See if we're going to use ISA/EISA DMA adapter. These adapters are
     * special since they're reused.
     *
     * Also note that we check for channel number since there are only 8 DMA
     * channels on ISA, so any request above this requires new adapter.
     */
    if (((DeviceDescription->InterfaceType == Eisa) ||
         (DeviceDescription->InterfaceType == Isa)) || !(DeviceDescription->Master))
    {
        if (((DeviceDescription->InterfaceType == Isa) ||
             (DeviceDescription->InterfaceType == Eisa)) &&
            (DeviceDescription->DmaChannel >= 8))
        {
            EisaAdapter = FALSE;
        }
        else
        {
            EisaAdapter = TRUE;
        }
    }
    else
    {
        EisaAdapter = FALSE;
    }

    /*
     * Disallow creating adapter for ISA/EISA DMA channel 4 since it's used
     * for cascading the controllers and it's not available for software use.
     */
    if ((EisaAdapter) && (DeviceDescription->DmaChannel == 4)) return NULL;

    /*
     * Calculate the number of map registers.
     *
     * - For EISA and PCI scatter/gather no map registers are needed.
     * - For ISA slave scatter/gather one map register is needed.
     * - For all other cases the number of map registers depends on
     *   DeviceDescription->MaximumLength.
     */
    MaximumLength = DeviceDescription->MaximumLength & MAXLONG;
    if ((DeviceDescription->ScatterGather) &&
        ((DeviceDescription->InterfaceType == Eisa) ||
         (DeviceDescription->InterfaceType == PCIBus)))
    {
        MapRegisters = 0;
    }
    else if ((DeviceDescription->ScatterGather) && !(DeviceDescription->Master))
    {
        MapRegisters = 1;
    }
    else
    {
        /*
         * In the equation below the additional map register added by
         * the "+1" accounts for the case when a transfer does not start
         * at a page-aligned address.
         */
        MapRegisters = BYTES_TO_PAGES(MaximumLength) + 1;
        if (MapRegisters > 16) MapRegisters = 16;
    }

    /*
     * Acquire the DMA lock that is used to protect the EISA adapter array.
     */
    KeWaitForSingleObject(&HalpDmaLock, Executive, KernelMode, FALSE, NULL);

    /*
     * Now we must get ahold of the adapter object. For first eight ISA/EISA
     * channels there are static adapter objects that are reused and updated
     * on succesive HalGetAdapter calls. In other cases a new adapter object
     * is always created and it's to the DMA adapter list (HalpDmaAdapterList).
     */
    if (EisaAdapter)
    {
        AdapterObject = HalpEisaAdapter[DeviceDescription->DmaChannel];
        if (AdapterObject)
        {
            if ((AdapterObject->NeedsMapRegisters) &&
                (MapRegisters > AdapterObject->MapRegistersPerChannel))
            {
                AdapterObject->MapRegistersPerChannel = MapRegisters;
            }
        }
    }

    if (AdapterObject == NULL)
    {
        AdapterObject = HalpDmaAllocateChildAdapter(MapRegisters, DeviceDescription);
        if (AdapterObject == NULL)
        {
            KeSetEvent(&HalpDmaLock, 0, 0);
            return NULL;
        }

        if (EisaAdapter)
        {
            HalpEisaAdapter[DeviceDescription->DmaChannel] = AdapterObject;
        }

        if (MapRegisters > 0)
        {
            AdapterObject->NeedsMapRegisters = TRUE;
            AdapterObject->MapRegistersPerChannel = MapRegisters;
        }
        else
        {
            AdapterObject->NeedsMapRegisters = FALSE;
            if (DeviceDescription->Master)
            {
                AdapterObject->MapRegistersPerChannel = BYTES_TO_PAGES(MaximumLength) + 1;
            }
            else
            {
                AdapterObject->MapRegistersPerChannel = 1;
            }
        }
    }

    /*
     * Release the DMA lock. HalpEisaAdapter will no longer be touched,
     * so we don't need it.
     */
    KeSetEvent(&HalpDmaLock, 0, 0);

    if (!EisaAdapter)
    {
        /* If it's not one of the static adapters, add it to the list */
        KeAcquireSpinLock(&HalpDmaAdapterListLock, &OldIrql);
        InsertTailList(&HalpDmaAdapterList, &AdapterObject->AdapterList);
        KeReleaseSpinLock(&HalpDmaAdapterListLock, OldIrql);
    }

    /*
     * Setup the values in the adapter object that are common for all
     * types of buses.
     */
    if (DeviceDescription->Version >= DEVICE_DESCRIPTION_VERSION1)
    {
        AdapterObject->IgnoreCount = DeviceDescription->IgnoreCount;
    }
    else
    {
        AdapterObject->IgnoreCount = 0;
    }

    if (DeviceDescription->Version == DEVICE_DESCRIPTION_VERSION3)
    {
        /* Only the address width describes what a version 3 device can reach */
        AdapterObject->DmaAddressWidth = HalpGetDeviceDmaAddressWidth(DeviceDescription);
        AdapterObject->Dma32BitAddresses = (AdapterObject->DmaAddressWidth >= 32);
        AdapterObject->Dma64BitAddresses = (AdapterObject->DmaAddressWidth >= 64);
    }
    else
    {
        AdapterObject->Dma32BitAddresses = DeviceDescription->Dma32BitAddresses;
        AdapterObject->Dma64BitAddresses = DeviceDescription->Dma64BitAddresses;

        if (DeviceDescription->Master && DeviceDescription->Dma64BitAddresses)
            AdapterObject->DmaAddressWidth = 64;
        else if (DeviceDescription->Master && DeviceDescription->Dma32BitAddresses)
            AdapterObject->DmaAddressWidth = 32;
        else
            AdapterObject->DmaAddressWidth = 24;
    }
    AdapterObject->ScatterGather = DeviceDescription->ScatterGather;
    AdapterObject->MasterDevice = DeviceDescription->Master;
    *NumberOfMapRegisters = AdapterObject->MapRegistersPerChannel;

    /*
     * For non-(E)ISA adapters we have already done all the work. On the
     * other hand for (E)ISA adapters we must still setup the DMA modes
     * and prepare the controller.
     */
    if (EisaAdapter)
    {
        if (!HalpDmaInitializeEisaAdapter(AdapterObject, DeviceDescription))
        {
            ObDereferenceObject(AdapterObject);
            return NULL;
        }
    }

    return AdapterObject;
}

/**
 * @name HalpGetDmaAdapter
 *
 * Internal routine to allocate PnP DMA adapter object. It's exported through
 * HalDispatchTable and used by IoGetDmaAdapter.
 *
 * @see HalGetAdapter
 */
PDMA_ADAPTER
NTAPI
HalpGetDmaAdapter(IN PVOID Context,
                  IN PDEVICE_DESCRIPTION DeviceDescription,
                  OUT PULONG NumberOfMapRegisters)
{
    return &HalGetAdapter(DeviceDescription, NumberOfMapRegisters)->DmaHeader;
}

/**
 * @name HalPutDmaAdapter
 *
 * Internal routine to free DMA adapter and resources for reuse. It's exported
 * using the DMA_OPERATIONS interface by HalGetAdapter.
 *
 * @see HalGetAdapter
 */
VOID
NTAPI
HalPutDmaAdapter(IN PADAPTER_OBJECT AdapterObject)
{
    KIRQL OldIrql;
    if (AdapterObject->ChannelNumber == 0xFF)
    {
        KeAcquireSpinLock(&HalpDmaAdapterListLock, &OldIrql);
        RemoveEntryList(&AdapterObject->AdapterList);
        KeReleaseSpinLock(&HalpDmaAdapterListLock, OldIrql);
    }

    ObDereferenceObject(AdapterObject);
}

/**
 * @name HalAllocateCommonBuffer
 *
 * Allocates memory that is visible to both the processor(s) and the DMA
 * device.
 *
 * @param AdapterObject
 *        Adapter object representing the bus master or system dma controller.
 * @param Length
 *        Number of bytes to allocate.
 * @param LogicalAddress
 *        Logical address the driver can use to access the buffer.
 * @param CacheEnabled
 *        Specifies if the memory can be cached.
 *
 * @return The base virtual address of the memory allocated or NULL on failure.
 *
 * @remarks
 *    On real NT x86 systems the CacheEnabled parameter is ignored, we honour
 *    it. If it proves to cause problems change it.
 *
 * @see HalFreeCommonBuffer
 *
 * @implemented
 */
PVOID
NTAPI
HalAllocateCommonBuffer(IN PADAPTER_OBJECT AdapterObject,
                        IN ULONG Length,
                        IN PPHYSICAL_ADDRESS LogicalAddress,
                        IN BOOLEAN CacheEnabled)
{
    PHYSICAL_ADDRESS LowestAcceptableAddress;
    PHYSICAL_ADDRESS HighestAcceptableAddress;
    PHYSICAL_ADDRESS BoundryAddressMultiple;
    PVOID VirtualAddress;

    LowestAcceptableAddress.QuadPart = 0;
    HighestAcceptableAddress = HalpGetAdapterMaximumPhysicalAddress(AdapterObject);
    BoundryAddressMultiple.QuadPart = 0;

    /*
     * For bus-master DMA devices the buffer mustn't cross 4Gb boundary. For
     * slave DMA devices the 64Kb boundary mustn't be crossed since the
     * controller wouldn't be able to handle it.
     */
    if (AdapterObject->MasterDevice)
    {
        BoundryAddressMultiple.HighPart = 1;
    }
    else
    {
        BoundryAddressMultiple.LowPart = 0x10000;
    }

    VirtualAddress = MmAllocateContiguousMemorySpecifyCache(Length,
                                                            LowestAcceptableAddress,
                                                            HighestAcceptableAddress,
                                                            BoundryAddressMultiple,
                                                            CacheEnabled ? MmCached :
                                                            MmNonCached);
    if (VirtualAddress == NULL) return NULL;

    *LogicalAddress = MmGetPhysicalAddress(VirtualAddress);

    return VirtualAddress;
}

/**
 * @name HalFreeCommonBuffer
 *
 * Free common buffer allocated with HalAllocateCommonBuffer.
 *
 * @see HalAllocateCommonBuffer
 *
 * @implemented
 */
VOID
NTAPI
HalFreeCommonBuffer(IN PADAPTER_OBJECT AdapterObject,
                    IN ULONG Length,
                    IN PHYSICAL_ADDRESS LogicalAddress,
                    IN PVOID VirtualAddress,
                    IN BOOLEAN CacheEnabled)
{
    MmFreeContiguousMemorySpecifyCache(VirtualAddress,
                                       Length,
                                       CacheEnabled ? MmCached : MmNonCached);
}

typedef struct _SCATTER_GATHER_CONTEXT {
    BOOLEAN UsingUserBuffer;
    PADAPTER_OBJECT AdapterObject;
    PMDL Mdl;
    ULONGLONG Offset;
    ULONG Length;
    PDRIVER_LIST_CONTROL AdapterListControlRoutine;
    PVOID AdapterListControlContext, MapRegisterBase;
    ULONG MapRegisterCount;
    ULONG ElementCount;
    BOOLEAN WriteToDevice;
    WAIT_CONTEXT_BLOCK Wcb;
} SCATTER_GATHER_CONTEXT, *PSCATTER_GATHER_CONTEXT;

/* The list follows its context in the same buffer */
#define SCATTER_GATHER_BUFFER_SIZE(Count) \
    (sizeof(SCATTER_GATHER_CONTEXT) + FIELD_OFFSET(SCATTER_GATHER_LIST, Elements) + \
     (Count) * sizeof(SCATTER_GATHER_ELEMENT))

/* HAL layout of a DMA_TRANSFER_CONTEXT */
typedef struct _HALP_DMA_TRANSFER
{
    ULONG Version;
    LONG State;
    PDRIVER_CONTROL ExecutionRoutine;
    PVOID ExecutionContext;
    PVOID MapRegisterBase;
    PVOID PoolBuffer;
    WAIT_CONTEXT_BLOCK Wcb;
} HALP_DMA_TRANSFER, *PHALP_DMA_TRANSFER;

C_ASSERT(sizeof(HALP_DMA_TRANSFER) <= DMA_TRANSFER_CONTEXT_SIZE_V1);

#define HALP_DMA_TRANSFER_GRANTED   0x1
#define HALP_DMA_TRANSFER_CANCELED  0x2

static
NTSTATUS
HalpAllocateAdapterChannel(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PWAIT_CONTEXT_BLOCK WaitContextBlock,
    _In_ ULONG NumberOfMapRegisters,
    _In_ PDRIVER_CONTROL ExecutionRoutine,
    _In_ BOOLEAN Synchronous);

/* Returns the MDL of a chain holding Offset and makes Offset relative to it */
static
PMDL
HalpFindTransferMdl(
    _In_opt_ PMDL Mdl,
    _Inout_ PULONGLONG Offset)
{
    while ((Mdl != NULL) && (*Offset >= MmGetMdlByteCount(Mdl)))
    {
        *Offset -= MmGetMdlByteCount(Mdl);
        Mdl = Mdl->Next;
    }

    return Mdl;
}

/* Tells whether IoMapTransfer maps two neighboring MDL pages as one run */
static
BOOLEAN
HalpIsSameTransferRun(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PFN_NUMBER Page,
    _In_ PFN_NUMBER NextPage)
{
    if (Page + 1 != NextPage)
        return FALSE;

    if (!AdapterObject->NeedsMapRegisters)
        return !((Page ^ NextPage) & ~0xFFFFF);

    return HalpEisaDma || !((Page ^ NextPage) & ~0xF);
}

/* Counts the map registers and list elements a transfer takes */
static
NTSTATUS
HalpGetTransferCounts(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_opt_ PMDL Mdl,
    _In_ ULONGLONG Offset,
    _In_ ULONG Length,
    _Out_ PULONG MapRegisterCount,
    _Out_ PULONG ElementCount)
{
    PPFN_NUMBER Pages;
    ULONG ByteOffset, Chunk, PageCount, Index;

    *MapRegisterCount = 0;
    *ElementCount = 0;

    Mdl = HalpFindTransferMdl(Mdl, &Offset);
    if (Mdl == NULL)
        return STATUS_INVALID_PARAMETER;

    for (; (Mdl != NULL) && (Length != 0); Mdl = Mdl->Next, Offset = 0)
    {
        Chunk = min(MmGetMdlByteCount(Mdl) - (ULONG)Offset, Length);
        if (Chunk == 0)
            continue;

        Length -= Chunk;
        ByteOffset = MmGetMdlByteOffset(Mdl) + (ULONG)Offset;
        Pages = MmGetMdlPfnArray(Mdl) + (ByteOffset >> PAGE_SHIFT);
        PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(BYTE_OFFSET(ByteOffset), Chunk);

        *MapRegisterCount += PageCount;
        (*ElementCount)++;

        /* Without scatter/gather support the map registers gather the whole piece */
        if (AdapterObject->NeedsMapRegisters && !AdapterObject->ScatterGather)
            continue;

        for (Index = 1; Index < PageCount; Index++)
        {
            if (!HalpIsSameTransferRun(AdapterObject, Pages[Index - 1], Pages[Index]))
                (*ElementCount)++;
        }
    }

    return STATUS_SUCCESS;
}

/* Maps a transfer, possibly crossing several MDLs of a chain, into list elements */
static
VOID
HalpMapTransferElements(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PMDL Mdl,
    _In_opt_ PVOID MapRegisterBase,
    _In_ ULONGLONG Offset,
    _Inout_ PULONG Length,
    _In_ BOOLEAN WriteToDevice,
    _Out_ PSCATTER_GATHER_LIST List,
    _In_ ULONG MaximumElements)
{
    PSCATTER_GATHER_ELEMENT Element = List->Elements;
    ULONG Remaining = *Length;
    ULONG Chunk, Piece;
    PUCHAR CurrentVa;

    List->NumberOfElements = 0;
    List->Reserved = 0;

    Mdl = HalpFindTransferMdl(Mdl, &Offset);
    for (; (Mdl != NULL) && (Remaining != 0); Mdl = Mdl->Next, Offset = 0)
    {
        Chunk = min(MmGetMdlByteCount(Mdl) - (ULONG)Offset, Remaining);
        CurrentVa = (PUCHAR)MmGetMdlVirtualAddress(Mdl) + (ULONG)Offset;

        while (Chunk != 0)
        {
            if (List->NumberOfElements == MaximumElements)
                goto Done;

            Piece = Chunk;
            Element->Address = IoMapTransfer(AdapterObject,
                                             Mdl,
                                             MapRegisterBase,
                                             CurrentVa,
                                             &Piece,
                                             WriteToDevice);
            if (Piece == 0)
                goto Done;

            Element->Length = Piece;
            Element->Reserved = 0;
            Element++;
            List->NumberOfElements++;

            CurrentVa += Piece;
            Chunk -= Piece;
            Remaining -= Piece;
        }
    }

Done:
    *Length -= Remaining;
}

/* Flushes a transfer mapped by HalpMapTransferElements */
static
VOID
HalpFlushTransferElements(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PMDL Mdl,
    _In_opt_ PVOID MapRegisterBase,
    _In_ ULONGLONG Offset,
    _In_ ULONG Length,
    _In_ BOOLEAN WriteToDevice)
{
    ULONG Chunk;

    Mdl = HalpFindTransferMdl(Mdl, &Offset);
    for (; (Mdl != NULL) && (Length != 0); Mdl = Mdl->Next, Offset = 0)
    {
        Chunk = min(MmGetMdlByteCount(Mdl) - (ULONG)Offset, Length);
        IoFlushAdapterBuffers(AdapterObject,
                              Mdl,
                              MapRegisterBase,
                              (PUCHAR)MmGetMdlVirtualAddress(Mdl) + (ULONG)Offset,
                              Chunk,
                              WriteToDevice);
        Length -= Chunk;
    }
}

/* Fills the list that follows a scatter/gather context */
static
PSCATTER_GATHER_LIST
HalpFillScatterGatherList(
    _In_ PSCATTER_GATHER_CONTEXT AdapterControlContext,
    _In_opt_ PVOID MapRegisterBase)
{
    PSCATTER_GATHER_LIST ScatterGatherList = (PSCATTER_GATHER_LIST)(AdapterControlContext + 1);
    ULONG Length = AdapterControlContext->Length;

    /* Store the map register base for later in HalPutScatterGatherList */
    AdapterControlContext->MapRegisterBase = MapRegisterBase;

    HalpMapTransferElements(AdapterControlContext->AdapterObject,
                            AdapterControlContext->Mdl,
                            MapRegisterBase,
                            AdapterControlContext->Offset,
                            &Length,
                            AdapterControlContext->WriteToDevice,
                            ScatterGatherList,
                            AdapterControlContext->ElementCount);
    if (Length != AdapterControlContext->Length)
    {
        DPRINT1("Scatter/gather list construction failed!\n");
        return NULL;
    }

    ScatterGatherList->Reserved = (ULONG_PTR)AdapterControlContext;

    DPRINT("Initiating S/G DMA with %lu element(s)\n", ScatterGatherList->NumberOfElements);
    return ScatterGatherList;
}

IO_ALLOCATION_ACTION
NTAPI
HalpScatterGatherAdapterControl(IN PDEVICE_OBJECT DeviceObject,
                                IN PIRP Irp,
                                IN PVOID MapRegisterBase,
                                IN PVOID Context)
{
    PSCATTER_GATHER_CONTEXT AdapterControlContext = Context;
    PSCATTER_GATHER_LIST ScatterGatherList;

    ScatterGatherList = HalpFillScatterGatherList(AdapterControlContext, MapRegisterBase);
    if (ScatterGatherList == NULL)
        return DeallocateObject;

    if (AdapterControlContext->AdapterListControlRoutine)
    {
        AdapterControlContext->AdapterListControlRoutine(DeviceObject,
                                                         Irp,
                                                         ScatterGatherList,
                                                         AdapterControlContext->
                                                             AdapterListControlContext);
    }

    return DeallocateObjectKeepRegisters;
}

/**
 * @name HalGetScatterGatherList
 *
 * Creates a scatter-gather list to be using in scatter/gather DMA
 *
 * @param AdapterObject
 *        Adapter object representing the bus master or system dma controller.
 * @param DeviceObject
 *        The device target for DMA.
 * @param Mdl
 *        The MDL that describes the buffer to be mapped.
 * @param CurrentVa
 *        The current VA in the buffer to be mapped for transfer.
 * @param Length
 *        Specifies the length of data in bytes to be mapped.
 * @param ExecutionRoutine
 *        A caller supplied AdapterListControl routine to be called when DMA is available.
 * @param Context
 *        Context passed to the AdapterListControl routine.
 * @param WriteToDevice
 *        Indicates direction of DMA operation.
 *
 * @return The status of the operation.
 *
 * @see HalBuildScatterGatherList
 *
 * @implemented
 */
 NTSTATUS
 NTAPI
 HalGetScatterGatherList(IN PADAPTER_OBJECT AdapterObject,
                         IN PDEVICE_OBJECT DeviceObject,
                         IN PMDL Mdl,
                         IN PVOID CurrentVa,
                         IN ULONG Length,
                         IN PDRIVER_LIST_CONTROL ExecutionRoutine,
                         IN PVOID Context,
                         IN BOOLEAN WriteToDevice)
{
    return HalBuildScatterGatherList(AdapterObject,
                                     DeviceObject,
                                     Mdl,
                                     CurrentVa,
                                     Length,
                                     ExecutionRoutine,
                                     Context,
                                     WriteToDevice,
                                     NULL,
                                     0);
}

/**
 * @name HalPutScatterGatherList
 *
 * Frees a scatter-gather list allocated from HalBuildScatterGatherList
 *
 * @param AdapterObject
 *        Adapter object representing the bus master or system dma controller.
 * @param ScatterGather
 *        The scatter/gather list to be freed.
 * @param WriteToDevice
 *        Indicates direction of DMA operation.
 *
 * @return None
 *
 * @see HalBuildScatterGatherList
 *
 * @implemented
 */
 VOID
 NTAPI
 HalPutScatterGatherList(IN PADAPTER_OBJECT AdapterObject,
                         IN PSCATTER_GATHER_LIST ScatterGather,
                         IN BOOLEAN WriteToDevice)
{
    PSCATTER_GATHER_CONTEXT AdapterControlContext = (PSCATTER_GATHER_CONTEXT)ScatterGather->Reserved;

    HalpFlushTransferElements(AdapterObject,
                              AdapterControlContext->Mdl,
                              AdapterControlContext->MapRegisterBase,
                              AdapterControlContext->Offset,
                              AdapterControlContext->Length,
                              AdapterControlContext->WriteToDevice);

    IoFreeMapRegisters(AdapterObject,
                       AdapterControlContext->MapRegisterBase,
                       AdapterControlContext->MapRegisterCount);

    /* The list is part of the context buffer. If this is our buffer, release it */
    if (!AdapterControlContext->UsingUserBuffer)
        ExFreePoolWithTag(AdapterControlContext, TAG_DMA);

    DPRINT("S/G DMA has finished!\n");
}

NTSTATUS
NTAPI
HalCalculateScatterGatherListSize(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl OPTIONAL,
    IN PVOID CurrentVa,
    IN ULONG Length,
    OUT PULONG ScatterGatherListSize,
    OUT PULONG pNumberOfMapRegisters)
{
    ULONG NumberOfMapRegisters, ElementCount;
    ULONG_PTR Offset;
    NTSTATUS Status;

    if (Mdl == NULL)
    {
        /* Without the pages every page counts as an element */
        NumberOfMapRegisters = ADDRESS_AND_SIZE_TO_SPAN_PAGES(CurrentVa, Length);
        ElementCount = NumberOfMapRegisters;
    }
    else
    {
        Offset = (ULONG_PTR)CurrentVa - (ULONG_PTR)MmGetMdlVirtualAddress(Mdl);
        if (Offset >= MmGetMdlByteCount(Mdl))
            return STATUS_INVALID_PARAMETER;

        Status = HalpGetTransferCounts(AdapterObject,
                                       Mdl,
                                       Offset,
                                       Length,
                                       &NumberOfMapRegisters,
                                       &ElementCount);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    if (AdapterObject->NeedsMapRegisters &&
        (NumberOfMapRegisters > AdapterObject->MapRegistersPerChannel))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *ScatterGatherListSize = SCATTER_GATHER_BUFFER_SIZE(ElementCount);
    if (pNumberOfMapRegisters) *pNumberOfMapRegisters = NumberOfMapRegisters;

    return STATUS_SUCCESS;
}

static
NTSTATUS
HalpAllocateTransferChannel(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PHALP_DMA_TRANSFER Transfer,
    _In_ ULONG NumberOfMapRegisters,
    _In_ BOOLEAN Synchronous,
    _In_opt_ PDRIVER_CONTROL ExecutionRoutine,
    _In_opt_ PVOID ExecutionContext);

/* Builds a list for an offset into an MDL chain, optionally tied to a transfer context */
static
NTSTATUS
HalpBuildScatterGatherList(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PHALP_DMA_TRANSFER Transfer,
    _In_opt_ PMDL Mdl,
    _In_ ULONGLONG Offset,
    _In_ ULONG Length,
    _In_ BOOLEAN Synchronous,
    _In_opt_ PDRIVER_LIST_CONTROL ExecutionRoutine,
    _In_opt_ PVOID Context,
    _In_ BOOLEAN WriteToDevice,
    _In_opt_ PVOID ScatterGatherBuffer,
    _In_ ULONG ScatterGatherBufferLength,
    _Out_opt_ PSCATTER_GATHER_LIST *ScatterGatherList)
{
    PSCATTER_GATHER_CONTEXT ScatterGatherContext;
    PSCATTER_GATHER_LIST List;
    ULONG NumberOfMapRegisters, ElementCount, SgSize;
    NTSTATUS Status;

    if (ScatterGatherList)
        *ScatterGatherList = NULL;

    Status = HalpGetTransferCounts(AdapterObject,
                                   Mdl,
                                   Offset,
                                   Length,
                                   &NumberOfMapRegisters,
                                   &ElementCount);
    if (!NT_SUCCESS(Status)) return Status;

    SgSize = SCATTER_GATHER_BUFFER_SIZE(ElementCount);
    if (ScatterGatherBuffer)
    {
        /* Checking if user buffer is enough */
        if (ScatterGatherBufferLength < SgSize)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }
        ScatterGatherContext = ScatterGatherBuffer;
        ScatterGatherContext->UsingUserBuffer = TRUE;
    }
    else
    {
        ScatterGatherContext = ExAllocatePoolWithTag(NonPagedPool, SgSize, TAG_DMA);
        if (!ScatterGatherContext)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        ScatterGatherContext->UsingUserBuffer = FALSE;
    }

    /* Fill the scatter-gather context */
    ScatterGatherContext->AdapterObject = AdapterObject;
    ScatterGatherContext->Mdl = Mdl;
    ScatterGatherContext->Offset = Offset;
    ScatterGatherContext->Length = Length;
    ScatterGatherContext->MapRegisterCount = NumberOfMapRegisters;
    ScatterGatherContext->ElementCount = ElementCount;
    ScatterGatherContext->AdapterListControlRoutine = ExecutionRoutine;
    ScatterGatherContext->AdapterListControlContext = Context;
    ScatterGatherContext->WriteToDevice = WriteToDevice;
    ScatterGatherContext->MapRegisterBase = NULL;

    List = (PSCATTER_GATHER_LIST)(ScatterGatherContext + 1);
    List->Reserved = 0;

    /* Without map registers there is nothing to wait for */
    if (!AdapterObject->NeedsMapRegisters)
    {
        if (Transfer)
            InterlockedOr(&Transfer->State, HALP_DMA_TRANSFER_GRANTED);

        if (HalpFillScatterGatherList(ScatterGatherContext, NULL) == NULL)
        {
            if (!ScatterGatherContext->UsingUserBuffer)
                ExFreePoolWithTag(ScatterGatherContext, TAG_DMA);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (ScatterGatherList)
            *ScatterGatherList = List;

        if (ExecutionRoutine)
            ExecutionRoutine(DeviceObject, DeviceObject->CurrentIrp, List, Context);

        return STATUS_SUCCESS;
    }

    if (Transfer)
    {
        Transfer->PoolBuffer = ScatterGatherContext->UsingUserBuffer ? NULL : ScatterGatherContext;
        Status = HalpAllocateTransferChannel(AdapterObject,
                                             DeviceObject,
                                             Transfer,
                                             NumberOfMapRegisters,
                                             Synchronous,
                                             HalpScatterGatherAdapterControl,
                                             ScatterGatherContext);
    }
    else
    {
        ScatterGatherContext->Wcb.DeviceObject = DeviceObject;
        ScatterGatherContext->Wcb.DeviceContext = (PVOID)ScatterGatherContext;
        ScatterGatherContext->Wcb.CurrentIrp = DeviceObject->CurrentIrp;

        Status = HalAllocateAdapterChannel(AdapterObject,
                                           &ScatterGatherContext->Wcb,
                                           NumberOfMapRegisters,
                                           HalpScatterGatherAdapterControl);
    }

    /* A synchronous request without a routine has its list by now. With a
       routine the list may already be freed, so the context is left alone */
    if (NT_SUCCESS(Status) && !ExecutionRoutine &&
        (List->Reserved != (ULONG_PTR)ScatterGatherContext))
        Status = STATUS_INSUFFICIENT_RESOURCES;

    if (!NT_SUCCESS(Status))
    {
        if (Transfer)
            Transfer->PoolBuffer = NULL;
        if (!ScatterGatherContext->UsingUserBuffer)
            ExFreePoolWithTag(ScatterGatherContext, TAG_DMA);
        return Status;
    }

    if (ScatterGatherList)
        *ScatterGatherList = List;

    return STATUS_SUCCESS;
}

/**
 * @name HalBuildScatterGatherList
 *
 * Creates a scatter-gather list to be using in scatter/gather DMA
 *
 * @param AdapterObject
 *        Adapter object representing the bus master or system dma controller.
 * @param DeviceObject
 *        The device target for DMA.
 * @param Mdl
 *        The MDL that describes the buffer to be mapped.
 * @param CurrentVa
 *        The current VA in the buffer to be mapped for transfer.
 * @param Length
 *        Specifies the length of data in bytes to be mapped.
 * @param ExecutionRoutine
 *        A caller supplied AdapterListControl routine to be called when DMA is available.
 * @param Context
 *        Context passed to the AdapterListControl routine.
 * @param WriteToDevice
 *        Indicates direction of DMA operation.
 *
 * @param ScatterGatherBuffer
 *        User buffer for the scatter-gather list
 *
 * @param ScatterGatherBufferLength
 *        Buffer length
 *
 * @return The status of the operation.
 *
 * @see HalPutScatterGatherList
 *
 * @implemented
 */
NTSTATUS
NTAPI
HalBuildScatterGatherList(
    IN PADAPTER_OBJECT AdapterObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice,
    IN PVOID ScatterGatherBuffer,
    IN ULONG ScatterGatherBufferLength)
{
    return HalpBuildScatterGatherList(AdapterObject,
                                      DeviceObject,
                                      NULL,
                                      Mdl,
                                      (ULONG_PTR)CurrentVa - (ULONG_PTR)MmGetMdlVirtualAddress(Mdl),
                                      Length,
                                      FALSE,
                                      ExecutionRoutine,
                                      Context,
                                      WriteToDevice,
                                      ScatterGatherBuffer,
                                      ScatterGatherBufferLength,
                                      NULL);
}

NTSTATUS
NTAPI
HalBuildMdlFromScatterGatherList(
    IN PDMA_ADAPTER DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PMDL OriginalMdl,
    OUT PMDL *TargetMdl)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* DMA VERSION 3 *************************************************************/

static
IO_ALLOCATION_ACTION
NTAPI
HalpTransferChannelControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PVOID MapRegisterBase,
    _In_ PVOID Context)
{
    PHALP_DMA_TRANSFER Transfer = Context;

    /* A canceled request hands the channel straight back */
    if (InterlockedOr(&Transfer->State, HALP_DMA_TRANSFER_GRANTED) & HALP_DMA_TRANSFER_CANCELED)
        return DeallocateObject;

    Transfer->MapRegisterBase = MapRegisterBase;

    if (Transfer->ExecutionRoutine == NULL)
        return KeepObject;

    return Transfer->ExecutionRoutine(DeviceObject,
                                      Irp,
                                      MapRegisterBase,
                                      Transfer->ExecutionContext);
}

static
NTSTATUS
HalpAllocateTransferChannel(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PHALP_DMA_TRANSFER Transfer,
    _In_ ULONG NumberOfMapRegisters,
    _In_ BOOLEAN Synchronous,
    _In_opt_ PDRIVER_CONTROL ExecutionRoutine,
    _In_opt_ PVOID ExecutionContext)
{
    NTSTATUS Status;
    KIRQL OldIrql;

    Transfer->State = 0;
    Transfer->ExecutionRoutine = ExecutionRoutine;
    Transfer->ExecutionContext = ExecutionContext;
    Transfer->MapRegisterBase = NULL;
    Transfer->Wcb.DeviceObject = DeviceObject;
    Transfer->Wcb.CurrentIrp = DeviceObject->CurrentIrp;
    Transfer->Wcb.DeviceContext = Transfer;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    Status = HalpAllocateAdapterChannel(AdapterObject,
                                        &Transfer->Wcb,
                                        NumberOfMapRegisters,
                                        HalpTransferChannelControl,
                                        Synchronous);
    KeLowerIrql(OldIrql);

    return Status;
}

static
NTSTATUS
NTAPI
HalGetDmaAdapterInfo(
    _In_ PADAPTER_OBJECT AdapterObject,
    _Inout_ PDMA_ADAPTER_INFO AdapterInfo)
{
    if (AdapterInfo->Version > DMA_ADAPTER_INFO_VERSION1)
        return STATUS_NOT_SUPPORTED;

    AdapterInfo->V1.DmaAddressWidth = AdapterObject->DmaAddressWidth;
    AdapterInfo->V1.MinimumTransferUnit = 1;

    if (AdapterObject->MasterDevice)
    {
        AdapterInfo->V1.ReadDmaCounterAvailable = FALSE;
        AdapterInfo->V1.ScatterGatherLimit = MAXULONG;
        AdapterInfo->V1.Flags =
            AdapterObject->NeedsMapRegisters ? 0 : ADAPTER_INFO_SYNCHRONOUS_CALLBACK;
    }
    else
    {
        AdapterInfo->V1.ReadDmaCounterAvailable = TRUE;
        AdapterInfo->V1.ScatterGatherLimit = 1;
        AdapterInfo->V1.Flags = 0;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
HalGetDmaTransferInfo(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PMDL Mdl,
    _In_ ULONGLONG Offset,
    _In_ ULONG Length,
    _In_ BOOLEAN WriteOnly,
    _Inout_ PDMA_TRANSFER_INFO TransferInfo)
{
    ULONG MapRegisterCount, ElementCount;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(WriteOnly);

    if (TransferInfo->Version > DMA_TRANSFER_INFO_VERSION1)
        return STATUS_NOT_SUPPORTED;

    Status = HalpGetTransferCounts(AdapterObject, Mdl, Offset, Length,
                                   &MapRegisterCount, &ElementCount);
    if (!NT_SUCCESS(Status))
        return Status;

    TransferInfo->V1.MapRegisterCount = MapRegisterCount;
    TransferInfo->V1.ScatterGatherElementCount = ElementCount;
    TransferInfo->V1.ScatterGatherListSize = SCATTER_GATHER_BUFFER_SIZE(ElementCount);

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
HalInitializeDmaTransferContext(
    _In_ PADAPTER_OBJECT AdapterObject,
    _Out_ PVOID DmaTransferContext)
{
    PHALP_DMA_TRANSFER Transfer = DmaTransferContext;

    UNREFERENCED_PARAMETER(AdapterObject);

    RtlZeroMemory(Transfer, DMA_TRANSFER_CONTEXT_SIZE_V1);
    Transfer->Version = DMA_TRANSFER_CONTEXT_VERSION1;

    return STATUS_SUCCESS;
}

static
PVOID
NTAPI
HalAllocateCommonBufferEx(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_opt_ PPHYSICAL_ADDRESS MaximumAddress,
    _In_ ULONG Length,
    _Out_ PPHYSICAL_ADDRESS LogicalAddress,
    _In_ BOOLEAN CacheEnabled,
    _In_ NODE_REQUIREMENT PreferredNode)
{
    PHYSICAL_ADDRESS LowestAcceptableAddress;
    PHYSICAL_ADDRESS HighestAcceptableAddress;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    PVOID VirtualAddress;

    UNREFERENCED_PARAMETER(PreferredNode);

    LowestAcceptableAddress.QuadPart = 0;
    HighestAcceptableAddress = HalpGetAdapterMaximumPhysicalAddress(AdapterObject);
    if (MaximumAddress &&
        ((ULONGLONG)MaximumAddress->QuadPart < (ULONGLONG)HighestAcceptableAddress.QuadPart))
    {
        HighestAcceptableAddress = *MaximumAddress;
    }

    /* Only devices below 32 bits of addressing are kept inside 64Kb blocks */
    BoundaryAddressMultiple.QuadPart = 0;
    if (AdapterObject->MasterDevice || (AdapterObject->DmaAddressWidth >= 32))
        BoundaryAddressMultiple.HighPart = 1;
    else
        BoundaryAddressMultiple.LowPart = 0x10000;

    VirtualAddress = MmAllocateContiguousMemorySpecifyCache(Length,
                                                            LowestAcceptableAddress,
                                                            HighestAcceptableAddress,
                                                            BoundaryAddressMultiple,
                                                            CacheEnabled ? MmCached : MmNonCached);
    if (VirtualAddress == NULL) return NULL;

    *LogicalAddress = MmGetPhysicalAddress(VirtualAddress);

    return VirtualAddress;
}

static
NTSTATUS
NTAPI
HalAllocateAdapterChannelEx(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID DmaTransferContext,
    _In_ ULONG NumberOfMapRegisters,
    _In_ ULONG Flags,
    _In_opt_ PDRIVER_CONTROL ExecutionRoutine,
    _In_opt_ PVOID ExecutionContext,
    _Out_opt_ PVOID *MapRegisterBase)
{
    PHALP_DMA_TRANSFER Transfer = DmaTransferContext;
    BOOLEAN Synchronous = !!(Flags & DMA_SYNCHRONOUS_CALLBACK);
    NTSTATUS Status;

    if ((Transfer == NULL) || (Transfer->Version != DMA_TRANSFER_CONTEXT_VERSION1))
        return STATUS_INVALID_PARAMETER;

    /* Without a routine the map registers can only be handed back synchronously */
    if (Synchronous)
    {
        if ((ExecutionRoutine == NULL) && (MapRegisterBase == NULL))
            return STATUS_INVALID_PARAMETER;
    }
    else if ((ExecutionRoutine == NULL) || (MapRegisterBase != NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (MapRegisterBase)
        *MapRegisterBase = NULL;

    Transfer->PoolBuffer = NULL;
    Status = HalpAllocateTransferChannel(AdapterObject,
                                         DeviceObject,
                                         Transfer,
                                         NumberOfMapRegisters,
                                         Synchronous,
                                         ExecutionRoutine,
                                         ExecutionContext);

    if (NT_SUCCESS(Status) && MapRegisterBase)
        *MapRegisterBase = Transfer->MapRegisterBase;

    return Status;
}

static
NTSTATUS
NTAPI
HalConfigureAdapterChannel(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ ULONG FunctionNumber,
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(AdapterObject);
    UNREFERENCED_PARAMETER(FunctionNumber);
    UNREFERENCED_PARAMETER(Context);

    /* Only DMA controller channels can be configured */
    return STATUS_NOT_IMPLEMENTED;
}

static
BOOLEAN
NTAPI
HalCancelAdapterChannel(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID DmaTransferContext)
{
    PHALP_DMA_TRANSFER Transfer = DmaTransferContext;
    PADAPTER_OBJECT MasterAdapter = AdapterObject->MasterAdapter;
    PLIST_ENTRY Entry;
    BOOLEAN WaitingForRegisters = FALSE;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(DeviceObject);

    /* Too late once the channel went to the request */
    if (InterlockedOr(&Transfer->State, HALP_DMA_TRANSFER_CANCELED) & HALP_DMA_TRANSFER_GRANTED)
        return FALSE;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    if (!KeRemoveEntryDeviceQueue(&AdapterObject->ChannelWaitQueue,
                                  &Transfer->Wcb.WaitQueueEntry) &&
        (MasterAdapter != NULL))
    {
        /* The request may own the channel while it waits for map registers */
        KeAcquireSpinLockAtDpcLevel(&MasterAdapter->SpinLock);
        for (Entry = MasterAdapter->AdapterQueue.Flink;
             Entry != &MasterAdapter->AdapterQueue;
             Entry = Entry->Flink)
        {
            if ((Entry == &AdapterObject->AdapterQueue) &&
                (AdapterObject->CurrentWcb == &Transfer->Wcb))
            {
                RemoveEntryList(Entry);
                WaitingForRegisters = TRUE;
                break;
            }
        }
        KeReleaseSpinLockFromDpcLevel(&MasterAdapter->SpinLock);

        if (WaitingForRegisters)
        {
            AdapterObject->NumberOfMapRegisters = 0;
            IoFreeAdapterChannel(AdapterObject);
        }
    }

    KeLowerIrql(OldIrql);

    if (Transfer->PoolBuffer)
    {
        ExFreePoolWithTag(Transfer->PoolBuffer, TAG_DMA);
        Transfer->PoolBuffer = NULL;
    }

    return TRUE;
}

static
NTSTATUS
NTAPI
HalMapTransferEx(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PMDL Mdl,
    _In_ PVOID MapRegisterBase,
    _In_ ULONGLONG Offset,
    _In_ ULONG DeviceOffset,
    _Inout_ PULONG Length,
    _In_ BOOLEAN WriteToDevice,
    _Out_writes_bytes_opt_(ScatterGatherBufferLength) PSCATTER_GATHER_LIST ScatterGatherBuffer,
    _In_ ULONG ScatterGatherBufferLength,
    _In_opt_ PDMA_COMPLETION_ROUTINE DmaCompletionRoutine,
    _In_opt_ PVOID CompletionContext)
{
    /* The device offset and completion routine only apply to DMA controller channels */
    UNREFERENCED_PARAMETER(DeviceOffset);
    UNREFERENCED_PARAMETER(DmaCompletionRoutine);
    UNREFERENCED_PARAMETER(CompletionContext);

    if (*Length == 0)
    {
        if ((ScatterGatherBuffer == NULL) ||
            (ScatterGatherBufferLength < FIELD_OFFSET(SCATTER_GATHER_LIST, Elements)))
        {
            return AdapterObject->MasterDevice ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        }

        ScatterGatherBuffer->NumberOfElements = 0;
        ScatterGatherBuffer->Reserved = 0;
        return STATUS_SUCCESS;
    }

    if (HalpFindTransferMdl(Mdl, &Offset) == NULL)
        return STATUS_INVALID_PARAMETER;

    if ((ScatterGatherBuffer == NULL) ||
        (ScatterGatherBufferLength < FIELD_OFFSET(SCATTER_GATHER_LIST, Elements) +
                                     sizeof(SCATTER_GATHER_ELEMENT)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    HalpMapTransferElements(AdapterObject,
                            Mdl,
                            MapRegisterBase,
                            Offset,
                            Length,
                            WriteToDevice,
                            ScatterGatherBuffer,
                            (ScatterGatherBufferLength -
                             FIELD_OFFSET(SCATTER_GATHER_LIST, Elements)) /
                            sizeof(SCATTER_GATHER_ELEMENT));

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
HalGetScatterGatherListEx(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID DmaTransferContext,
    _In_ PMDL Mdl,
    _In_ ULONGLONG Offset,
    _In_ ULONG Length,
    _In_ ULONG Flags,
    _In_opt_ PDRIVER_LIST_CONTROL ExecutionRoutine,
    _In_opt_ PVOID Context,
    _In_ BOOLEAN WriteToDevice,
    _In_opt_ PDMA_COMPLETION_ROUTINE DmaCompletionRoutine,
    _In_opt_ PVOID CompletionContext,
    _Out_opt_ PSCATTER_GATHER_LIST *ScatterGatherList)
{
    BOOLEAN Synchronous = !!(Flags & DMA_SYNCHRONOUS_CALLBACK);

    UNREFERENCED_PARAMETER(DmaCompletionRoutine);
    UNREFERENCED_PARAMETER(CompletionContext);

    if (DmaTransferContext == NULL)
        return STATUS_INVALID_PARAMETER;

    if ((ExecutionRoutine == NULL) && (!Synchronous || (ScatterGatherList == NULL)))
        return STATUS_INVALID_PARAMETER;

    return HalpBuildScatterGatherList(AdapterObject,
                                      DeviceObject,
                                      DmaTransferContext,
                                      Mdl,
                                      Offset,
                                      Length,
                                      Synchronous,
                                      ExecutionRoutine,
                                      Context,
                                      WriteToDevice,
                                      NULL,
                                      0,
                                      ScatterGatherList);
}

static
NTSTATUS
NTAPI
HalBuildScatterGatherListEx(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID DmaTransferContext,
    _In_ PMDL Mdl,
    _In_ ULONGLONG Offset,
    _In_ ULONG Length,
    _In_ ULONG Flags,
    _In_opt_ PDRIVER_LIST_CONTROL ExecutionRoutine,
    _In_opt_ PVOID Context,
    _In_ BOOLEAN WriteToDevice,
    _In_ PVOID ScatterGatherBuffer,
    _In_ ULONG ScatterGatherBufferLength,
    _In_opt_ PDMA_COMPLETION_ROUTINE DmaCompletionRoutine,
    _In_opt_ PVOID CompletionContext,
    _Out_opt_ PSCATTER_GATHER_LIST *ScatterGatherList)
{
    BOOLEAN Synchronous = !!(Flags & DMA_SYNCHRONOUS_CALLBACK);

    UNREFERENCED_PARAMETER(DmaCompletionRoutine);
    UNREFERENCED_PARAMETER(CompletionContext);

    if ((DmaTransferContext == NULL) || (ScatterGatherBuffer == NULL))
        return STATUS_INVALID_PARAMETER;

    if ((ExecutionRoutine == NULL) && (!Synchronous || (ScatterGatherList == NULL)))
        return STATUS_INVALID_PARAMETER;

    return HalpBuildScatterGatherList(AdapterObject,
                                      DeviceObject,
                                      DmaTransferContext,
                                      Mdl,
                                      Offset,
                                      Length,
                                      Synchronous,
                                      ExecutionRoutine,
                                      Context,
                                      WriteToDevice,
                                      ScatterGatherBuffer,
                                      ScatterGatherBufferLength,
                                      ScatterGatherList);
}

static
NTSTATUS
NTAPI
HalFlushAdapterBuffersEx(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PMDL Mdl,
    _In_ PVOID MapRegisterBase,
    _In_ ULONGLONG Offset,
    _In_ ULONG Length,
    _In_ BOOLEAN WriteToDevice)
{
    if (Length == 0)
        return STATUS_SUCCESS;

    if (HalpFindTransferMdl(Mdl, &Offset) == NULL)
        return STATUS_INVALID_PARAMETER;

    HalpFlushTransferElements(AdapterObject, Mdl, MapRegisterBase, Offset, Length, WriteToDevice);

    return STATUS_SUCCESS;
}

static
VOID
NTAPI
HalFreeAdapterObject(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ IO_ALLOCATION_ACTION AllocationAction)
{
    KIRQL OldIrql;

    if (AllocationAction == DeallocateObjectKeepRegisters)
        AdapterObject->NumberOfMapRegisters = 0;
    else if (AllocationAction != DeallocateObject)
        return;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    IoFreeAdapterChannel(AdapterObject);
    KeLowerIrql(OldIrql);
}

static
NTSTATUS
NTAPI
HalCancelMappedTransfer(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PVOID DmaTransferContext)
{
    UNREFERENCED_PARAMETER(DmaTransferContext);

    /* Bus masters run their own transfers, and there are no DMA controller channels */
    if (AdapterObject->MasterDevice)
        return STATUS_INVALID_PARAMETER;

    return STATUS_NOT_SUPPORTED;
}
#endif

/**
 * @name HalpDmaGetDmaAlignment
 *
 * Internal routine to return the DMA alignment requirement. It's exported
 * using the DMA_OPERATIONS interface by HalGetAdapter.
 *
 * @see HalGetAdapter
 */
ULONG
NTAPI
HalpDmaGetDmaAlignment(IN PADAPTER_OBJECT AdapterObject)
{
    return 1;
}

/*
 * @name HalReadDmaCounter
 *
 * Read DMA operation progress counter.
 *
 * @implemented
 */
ULONG
NTAPI
HalReadDmaCounter(IN PADAPTER_OBJECT AdapterObject)
{
    KIRQL OldIrql;
    ULONG Count, OldCount;

    ASSERT(!AdapterObject->MasterDevice);

    /*
     * Acquire the master adapter lock since we're going to mess with the
     * system DMA controller registers and we really don't want anyone
     * to do the same at the same time.
     */
    KeAcquireSpinLock(&AdapterObject->MasterAdapter->SpinLock, &OldIrql);

    /* Send the request to the specific controller. */
    if (AdapterObject->AdapterNumber == 1)
    {
        PDMA1_CONTROL DmaControl1 = AdapterObject->AdapterBaseVa;

        Count = 0xffff00;
        do
        {
            OldCount = Count;

            /* Send Reset */
            WRITE_PORT_UCHAR(&DmaControl1->ClearBytePointer, 0);

            /* Read Count */
            Count = READ_PORT_UCHAR(&DmaControl1->DmaAddressCount
                                    [AdapterObject->ChannelNumber].DmaBaseCount);
            Count |= READ_PORT_UCHAR(&DmaControl1->DmaAddressCount
                                     [AdapterObject->ChannelNumber].DmaBaseCount) << 8;
        } while (0xffff00 & (OldCount ^ Count));
    }
    else
    {
        PDMA2_CONTROL DmaControl2 = AdapterObject->AdapterBaseVa;

        Count = 0xffff00;
        do
        {
            OldCount = Count;

            /* Send Reset */
            WRITE_PORT_UCHAR(&DmaControl2->ClearBytePointer, 0);

            /* Read Count */
            Count = READ_PORT_UCHAR(&DmaControl2->DmaAddressCount
                                    [AdapterObject->ChannelNumber].DmaBaseCount);
            Count |= READ_PORT_UCHAR(&DmaControl2->DmaAddressCount
                                     [AdapterObject->ChannelNumber].DmaBaseCount) << 8;
        } while (0xffff00 & (OldCount ^ Count));
    }

    KeReleaseSpinLock(&AdapterObject->MasterAdapter->SpinLock, OldIrql);

    Count++;
    Count &= 0xffff;
    if (AdapterObject->Width16Bits) Count *= 2;

    return Count;
}

#ifndef _MINIHAL_
/**
 * @name HalpGrowMapBufferWorker
 *
 * Helper routine of HalAllocateAdapterChannel for allocating map registers
 * at PASSIVE_LEVEL in work item.
 */
VOID
NTAPI
HalpGrowMapBufferWorker(IN PVOID DeferredContext)
{
    PGROW_WORK_ITEM WorkItem = (PGROW_WORK_ITEM)DeferredContext;
    KIRQL OldIrql;
    BOOLEAN Succeeded;

    /*
     * Try to allocate new map registers for the adapter.
     *
     * NOTE: The NT implementation actually tries to allocate more map
     * registers than needed as an optimization.
     */
    KeWaitForSingleObject(&HalpDmaLock, Executive, KernelMode, FALSE, NULL);
    Succeeded = HalpGrowMapBuffers(WorkItem->AdapterObject->MasterAdapter,
                                   WorkItem->NumberOfMapRegisters << PAGE_SHIFT);
    KeSetEvent(&HalpDmaLock, 0, 0);

    if (Succeeded)
    {
        /*
         * Flush the adapter queue now that new map registers are ready. The
         * easiest way to do that is to call IoFreeMapRegisters to not free
         * any registers. Note that we use the magic (PVOID)2 map register
         * base to bypass the parameter checking.
         */
        OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
        IoFreeMapRegisters(WorkItem->AdapterObject, (PVOID)2, 0);
        KfLowerIrql(OldIrql);
    }

    ExFreePool(WorkItem);
}

/**
 * @name HalAllocateAdapterChannel
 *
 * Setup map registers for an adapter object.
 *
 * @param AdapterObject
 *        Pointer to an ADAPTER_OBJECT to set up.
 * @param WaitContextBlock
 *        Context block to be used with ExecutionRoutine.
 * @param NumberOfMapRegisters
 *        Number of map registers requested.
 * @param ExecutionRoutine
 *        Callback to call when map registers are allocated.
 *
 * @return
 *    If not enough map registers can be allocated then
 *    STATUS_INSUFFICIENT_RESOURCES is returned. If the function
 *    succeeds or the callback is queued for later delivering then
 *    STATUS_SUCCESS is returned.
 *
 * @see IoFreeAdapterChannel
 *
 * @implemented
 */
NTSTATUS
NTAPI
HalAllocateAdapterChannel(IN PADAPTER_OBJECT AdapterObject,
                          IN PWAIT_CONTEXT_BLOCK WaitContextBlock,
                          IN ULONG NumberOfMapRegisters,
                          IN PDRIVER_CONTROL ExecutionRoutine)
{
    return HalpAllocateAdapterChannel(AdapterObject,
                                      WaitContextBlock,
                                      NumberOfMapRegisters,
                                      ExecutionRoutine,
                                      FALSE);
}

/*
 * A synchronous request never waits: it fails with STATUS_INSUFFICIENT_RESOURCES
 * when the channel or the map registers are not free right away.
 */
static
NTSTATUS
HalpAllocateAdapterChannel(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ PWAIT_CONTEXT_BLOCK WaitContextBlock,
    _In_ ULONG NumberOfMapRegisters,
    _In_ PDRIVER_CONTROL ExecutionRoutine,
    _In_ BOOLEAN Synchronous)
{
    PADAPTER_OBJECT MasterAdapter;
    PGROW_WORK_ITEM WorkItem;
    ULONG Index = MAXULONG;
    ULONG Result;
    KIRQL OldIrql;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* Set up the wait context block in case we can't run right away. */
    WaitContextBlock->DeviceRoutine = ExecutionRoutine;
    WaitContextBlock->NumberOfMapRegisters = NumberOfMapRegisters;

    if (Synchronous)
    {
        KeAcquireSpinLockAtDpcLevel(&AdapterObject->ChannelWaitQueue.Lock);
        if (AdapterObject->ChannelWaitQueue.Busy)
        {
            KeReleaseSpinLockFromDpcLevel(&AdapterObject->ChannelWaitQueue.Lock);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        AdapterObject->ChannelWaitQueue.Busy = TRUE;
        KeReleaseSpinLockFromDpcLevel(&AdapterObject->ChannelWaitQueue.Lock);
    }
    /* Returns true if queued, else returns false and sets the queue to busy */
    else if (KeInsertDeviceQueue(&AdapterObject->ChannelWaitQueue,
                                 &WaitContextBlock->WaitQueueEntry))
    {
        return STATUS_SUCCESS;
    }

    MasterAdapter = AdapterObject->MasterAdapter;

    AdapterObject->NumberOfMapRegisters = NumberOfMapRegisters;
    AdapterObject->CurrentWcb = WaitContextBlock;

    if ((NumberOfMapRegisters) && (AdapterObject->NeedsMapRegisters))
    {
        if (NumberOfMapRegisters > AdapterObject->MapRegistersPerChannel)
        {
            AdapterObject->NumberOfMapRegisters = 0;
            IoFreeAdapterChannel(AdapterObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /*
         * Get the map registers. This is partly complicated by the fact
         * that new map registers can only be allocated at PASSIVE_LEVEL
         * and we're currently at DISPATCH_LEVEL. The following code has
         * two code paths:
         *
         * - If there is no adapter queued for map register allocation,
         *   try to see if enough contiguous map registers are present.
         *   In case they're we can just get them and proceed further.
         *
         * - If some adapter is already present in the queue we must
         *   respect the order of adapters asking for map registers and
         *   so the fast case described above can't take place.
         *   This case is also entered if not enough coniguous map
         *   registers are present.
         *
         *   A work queue item is allocated and queued, the adapter is
         *   also queued into the master adapter queue. The worker
         *   routine does the job of allocating the map registers at
         *   PASSIVE_LEVEL and calling the ExecutionRoutine.
         */

        KeAcquireSpinLock(&MasterAdapter->SpinLock, &OldIrql);

        if (IsListEmpty(&MasterAdapter->AdapterQueue))
        {
            Index = RtlFindClearBitsAndSet(MasterAdapter->MapRegisters, NumberOfMapRegisters, 0);
            if (Index != MAXULONG)
            {
                AdapterObject->MapRegisterBase = MasterAdapter->MapRegisterBase + Index;
                if (!AdapterObject->ScatterGather)
                {
                    AdapterObject->MapRegisterBase = (PROS_MAP_REGISTER_ENTRY)((ULONG_PTR)AdapterObject->MapRegisterBase | MAP_BASE_SW_SG);
                }
            }
        }

        if (Index == MAXULONG)
        {
            if (!Synchronous)
                InsertTailList(&MasterAdapter->AdapterQueue, &AdapterObject->AdapterQueue);

            WorkItem = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(GROW_WORK_ITEM),
                                             TAG_DMA);
            if (WorkItem)
            {
                ExInitializeWorkItem(&WorkItem->WorkQueueItem, HalpGrowMapBufferWorker, WorkItem);
                WorkItem->AdapterObject = AdapterObject;
                WorkItem->NumberOfMapRegisters = NumberOfMapRegisters;

                ExQueueWorkItem(&WorkItem->WorkQueueItem, DelayedWorkQueue);
            }

            KeReleaseSpinLock(&MasterAdapter->SpinLock, OldIrql);

            if (Synchronous)
            {
                /* The registers are being grown, but this request does not wait for them */
                AdapterObject->NumberOfMapRegisters = 0;
                IoFreeAdapterChannel(AdapterObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            return STATUS_SUCCESS;
        }

        KeReleaseSpinLock(&MasterAdapter->SpinLock, OldIrql);
    }
    else
    {
        AdapterObject->MapRegisterBase = NULL;
        AdapterObject->NumberOfMapRegisters = 0;
    }

    AdapterObject->CurrentWcb = WaitContextBlock;

    Result = ExecutionRoutine(WaitContextBlock->DeviceObject,
                              WaitContextBlock->CurrentIrp,
                              AdapterObject->MapRegisterBase,
                              WaitContextBlock->DeviceContext);

    /*
     * Possible return values:
     *
     * - KeepObject
     *   Don't free any resources, the ADAPTER_OBJECT is still in use and
     *   the caller will call IoFreeAdapterChannel later.
     *
     * - DeallocateObject
     *   Deallocate the map registers and release the ADAPTER_OBJECT, so
     *   someone else can use it.
     *
     * - DeallocateObjectKeepRegisters
     *   Release the ADAPTER_OBJECT, but hang on to the map registers. The
     *   client will later call IoFreeMapRegisters.
     *
     * NOTE:
     * IoFreeAdapterChannel runs the queue, so it must be called unless
     * the adapter object is not to be freed.
     */
    if (Result == DeallocateObject)
    {
        IoFreeAdapterChannel(AdapterObject);
    }
    else if (Result == DeallocateObjectKeepRegisters)
    {
        AdapterObject->NumberOfMapRegisters = 0;
        IoFreeAdapterChannel(AdapterObject);
    }

    return STATUS_SUCCESS;
}

/**
 * @name IoFreeAdapterChannel
 *
 * Free DMA resources allocated by IoAllocateAdapterChannel.
 *
 * @param AdapterObject
 *        Adapter object with resources to free.
 *
 * @remarks
 *    This function releases map registers registers assigned to the DMA
 *    adapter. After releasing the adapter, it checks the adapter's queue
 *    and runs each queued device object in series until the queue is
 *    empty. This is the only way the device queue is emptied.
 *
 * @see IoAllocateAdapterChannel
 *
 * @implemented
 */
VOID
NTAPI
IoFreeAdapterChannel(IN PADAPTER_OBJECT AdapterObject)
{
    PADAPTER_OBJECT MasterAdapter;
    PKDEVICE_QUEUE_ENTRY DeviceQueueEntry;
    PWAIT_CONTEXT_BLOCK WaitContextBlock;
    ULONG Index = MAXULONG;
    ULONG Result;
    KIRQL OldIrql;

    MasterAdapter = AdapterObject->MasterAdapter;

    for (;;)
    {
        /*
         * To keep map registers, call here with AdapterObject->
         * NumberOfMapRegisters set to zero. This trick is used in
         * HalAllocateAdapterChannel for example.
         */
        if (AdapterObject->NumberOfMapRegisters)
        {
            IoFreeMapRegisters(AdapterObject,
                               AdapterObject->MapRegisterBase,
                               AdapterObject->NumberOfMapRegisters);
        }

        DeviceQueueEntry = KeRemoveDeviceQueue(&AdapterObject->ChannelWaitQueue);
        if (!DeviceQueueEntry) break;

        WaitContextBlock = CONTAINING_RECORD(DeviceQueueEntry,
                                             WAIT_CONTEXT_BLOCK,
                                             WaitQueueEntry);

        AdapterObject->CurrentWcb = WaitContextBlock;
        AdapterObject->NumberOfMapRegisters = WaitContextBlock->NumberOfMapRegisters;

        if ((WaitContextBlock->NumberOfMapRegisters) && (AdapterObject->MasterAdapter))
        {
            KeAcquireSpinLock(&MasterAdapter->SpinLock, &OldIrql);

            if (IsListEmpty(&MasterAdapter->AdapterQueue))
            {
                Index = RtlFindClearBitsAndSet(MasterAdapter->MapRegisters,
                                               WaitContextBlock->NumberOfMapRegisters,
                                               0);
                if (Index != MAXULONG)
                {
                    AdapterObject->MapRegisterBase = MasterAdapter->MapRegisterBase + Index;
                    if (!AdapterObject->ScatterGather)
                    {
                        AdapterObject->MapRegisterBase =(PROS_MAP_REGISTER_ENTRY)((ULONG_PTR)AdapterObject->MapRegisterBase | MAP_BASE_SW_SG);
                    }
                }
            }

            if (Index == MAXULONG)
            {
                InsertTailList(&MasterAdapter->AdapterQueue, &AdapterObject->AdapterQueue);
                KeReleaseSpinLock(&MasterAdapter->SpinLock, OldIrql);
                break;
            }

            KeReleaseSpinLock(&MasterAdapter->SpinLock, OldIrql);
        }
        else
        {
            AdapterObject->MapRegisterBase = NULL;
            AdapterObject->NumberOfMapRegisters = 0;
        }

        /* Call the adapter control routine. */
        Result = ((PDRIVER_CONTROL)WaitContextBlock->DeviceRoutine)(WaitContextBlock->DeviceObject,
                                                                    WaitContextBlock->CurrentIrp,
                                                                    AdapterObject->MapRegisterBase,
                                                                    WaitContextBlock->DeviceContext);
        switch (Result)
        {
            case KeepObject:
                /*
                 * We're done until the caller manually calls IoFreeAdapterChannel
                 * or IoFreeMapRegisters.
                 */
                return;

            case DeallocateObjectKeepRegisters:
                /*
                 * Hide the map registers so they aren't deallocated next time
                 * around.
                 */
                AdapterObject->NumberOfMapRegisters = 0;
                break;

            default:
                break;
        }
    }
}

/**
 * @name IoFreeMapRegisters
 *
 * Free map registers reserved by the system for a DMA.
 *
 * @param AdapterObject
 *        DMA adapter to free map registers on.
 * @param MapRegisterBase
 *        Handle to map registers to free.
 * @param NumberOfRegisters
 *        Number of map registers to be freed.
 *
 * @implemented
 */
VOID
NTAPI
IoFreeMapRegisters(IN PADAPTER_OBJECT AdapterObject,
                   IN PVOID MapRegisterBase,
                   IN ULONG NumberOfMapRegisters)
{
    PADAPTER_OBJECT MasterAdapter = AdapterObject->MasterAdapter;
    PLIST_ENTRY ListEntry;
    KIRQL OldIrql;
    ULONG Index;
    ULONG Result;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (!(MasterAdapter) || !(MapRegisterBase)) return;

    KeAcquireSpinLock(&MasterAdapter->SpinLock, &OldIrql);

    if (NumberOfMapRegisters != 0)
    {
        PROS_MAP_REGISTER_ENTRY RealMapRegisterBase;

        RealMapRegisterBase = (PROS_MAP_REGISTER_ENTRY)((ULONG_PTR)MapRegisterBase & ~MAP_BASE_SW_SG);
        RtlClearBits(MasterAdapter->MapRegisters,
                     (ULONG)(RealMapRegisterBase - MasterAdapter->MapRegisterBase),
                     NumberOfMapRegisters);
    }

    /*
     * Now that we freed few map registers it's time to look at the master
     * adapter queue and see if there is someone waiting for map registers.
     */
    while (!IsListEmpty(&MasterAdapter->AdapterQueue))
    {
        ListEntry = RemoveHeadList(&MasterAdapter->AdapterQueue);
        AdapterObject = CONTAINING_RECORD(ListEntry, struct _ADAPTER_OBJECT, AdapterQueue);

        Index = RtlFindClearBitsAndSet(MasterAdapter->MapRegisters,
                                       AdapterObject->NumberOfMapRegisters,
                                       0);
        if (Index == MAXULONG)
        {
            InsertHeadList(&MasterAdapter->AdapterQueue, ListEntry);
            break;
        }

        KeReleaseSpinLock(&MasterAdapter->SpinLock, OldIrql);

        AdapterObject->MapRegisterBase = MasterAdapter->MapRegisterBase + Index;
        if (!AdapterObject->ScatterGather)
        {
            AdapterObject->MapRegisterBase =
                (PROS_MAP_REGISTER_ENTRY)((ULONG_PTR)AdapterObject->MapRegisterBase | MAP_BASE_SW_SG);
        }

        Result = ((PDRIVER_CONTROL)AdapterObject->CurrentWcb->DeviceRoutine)(AdapterObject->CurrentWcb->DeviceObject,
                                                                             AdapterObject->CurrentWcb->CurrentIrp,
                                                                             AdapterObject->MapRegisterBase,
                                                                             AdapterObject->CurrentWcb->DeviceContext);
        switch (Result)
        {
            case DeallocateObjectKeepRegisters:
                AdapterObject->NumberOfMapRegisters = 0;
                /* fall through */

            case DeallocateObject:
                if (AdapterObject->NumberOfMapRegisters)
                {
                    KeAcquireSpinLock(&MasterAdapter->SpinLock, &OldIrql);
                    RtlClearBits(MasterAdapter->MapRegisters,
                                 (ULONG)(AdapterObject->MapRegisterBase -
                                         MasterAdapter->MapRegisterBase),
                                 AdapterObject->NumberOfMapRegisters);
                    KeReleaseSpinLock(&MasterAdapter->SpinLock, OldIrql);
                }

                IoFreeAdapterChannel(AdapterObject);
                break;

            default:
                break;
        }

        KeAcquireSpinLock(&MasterAdapter->SpinLock, &OldIrql);
    }

    KeReleaseSpinLock(&MasterAdapter->SpinLock, OldIrql);
}

/**
 * @name HalpCopyBufferMap
 *
 * Helper function for copying data from/to map register buffers.
 *
 * @see IoFlushAdapterBuffers, IoMapTransfer
 */
VOID
NTAPI
HalpCopyBufferMap(IN PMDL Mdl,
                  IN PROS_MAP_REGISTER_ENTRY MapRegisterBase,
                  IN PVOID CurrentVa,
                  IN ULONG Length,
                  IN BOOLEAN WriteToDevice)
{
    ULONG CurrentLength;
    ULONG_PTR CurrentAddress;
    ULONG ByteOffset;
    PVOID VirtualAddress;

    VirtualAddress = MmGetSystemAddressForMdlSafe(Mdl, HighPagePriority);
    if (!VirtualAddress)
    {
        /*
         * NOTE: On real NT a mechanism with reserved pages is implemented
         * to handle this case in a slow, but graceful non-fatal way.
         */
         KeBugCheckEx(HAL_MEMORY_ALLOCATION, PAGE_SIZE, 0, (ULONG_PTR)__FILE__, 0);
    }

    CurrentAddress = (ULONG_PTR)VirtualAddress +
                     (ULONG_PTR)CurrentVa -
                     (ULONG_PTR)MmGetMdlVirtualAddress(Mdl);

    while (Length > 0)
    {
        ByteOffset = BYTE_OFFSET(CurrentAddress);
        CurrentLength = PAGE_SIZE - ByteOffset;
        if (CurrentLength > Length) CurrentLength = Length;

        if (WriteToDevice)
        {
            RtlCopyMemory((PVOID)((ULONG_PTR)MapRegisterBase->VirtualAddress + ByteOffset),
                          (PVOID)CurrentAddress,
                          CurrentLength);
        }
        else
        {
            RtlCopyMemory((PVOID)CurrentAddress,
                          (PVOID)((ULONG_PTR)MapRegisterBase->VirtualAddress + ByteOffset),
                          CurrentLength);
        }

        Length -= CurrentLength;
        CurrentAddress += CurrentLength;
        MapRegisterBase++;
    }
}

/**
 * @name IoFlushAdapterBuffers
 *
 * Flush any data remaining in the DMA controller's memory into the host
 * memory.
 *
 * @param AdapterObject
 *        The adapter object to flush.
 * @param Mdl
 *        Original MDL to flush data into.
 * @param MapRegisterBase
 *        Map register base that was just used by IoMapTransfer, etc.
 * @param CurrentVa
 *        Offset into Mdl to be flushed into, same as was passed to
 *        IoMapTransfer.
 * @param Length
 *        Length of the buffer to be flushed into.
 * @param WriteToDevice
 *        TRUE if it's a write, FALSE if it's a read.
 *
 * @return TRUE in all cases.
 *
 * @remarks
 *    This copies data from the map register-backed buffer to the user's
 *    target buffer. Data are not in the user buffer until this function
 *    is called.
 *    For slave DMA transfers the controller channel is masked effectively
 *    stopping the current transfer.
 *
 * @unimplemented.
 */
BOOLEAN
NTAPI
IoFlushAdapterBuffers(IN PADAPTER_OBJECT AdapterObject,
                      IN PMDL Mdl,
                      IN PVOID MapRegisterBase,
                      IN PVOID CurrentVa,
                      IN ULONG Length,
                      IN BOOLEAN WriteToDevice)
{
    BOOLEAN SlaveDma = FALSE;
    PROS_MAP_REGISTER_ENTRY RealMapRegisterBase;
    PHYSICAL_ADDRESS HighestAcceptableAddress;
    PHYSICAL_ADDRESS PhysicalAddress;
    PPFN_NUMBER MdlPagesPtr;

    /* Sanity checks */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    ASSERT(AdapterObject);

    if (!AdapterObject->MasterDevice)
    {
        /* Mask out (disable) the DMA channel. */
        if (AdapterObject->AdapterNumber == 1)
        {
            PDMA1_CONTROL DmaControl1 = AdapterObject->AdapterBaseVa;
            WRITE_PORT_UCHAR(&DmaControl1->SingleMask,
                             AdapterObject->ChannelNumber | DMA_SETMASK);
        }
        else
        {
            PDMA2_CONTROL DmaControl2 = AdapterObject->AdapterBaseVa;
            WRITE_PORT_UCHAR(&DmaControl2->SingleMask,
                             AdapterObject->ChannelNumber | DMA_SETMASK);
        }
        SlaveDma = TRUE;
    }

    /* This can happen if the device supports hardware scatter/gather. */
    if (MapRegisterBase == NULL) return TRUE;

    RealMapRegisterBase = (PROS_MAP_REGISTER_ENTRY)((ULONG_PTR)MapRegisterBase & ~MAP_BASE_SW_SG);

    if (!WriteToDevice)
    {
        if ((ULONG_PTR)MapRegisterBase & MAP_BASE_SW_SG)
        {
            if (RealMapRegisterBase->Counter != MAXULONG)
            {
                if ((SlaveDma) && !(AdapterObject->IgnoreCount))
                {
                    Length -= HalReadDmaCounter(AdapterObject);
                }
            }
            HalpCopyBufferMap(Mdl,
                              RealMapRegisterBase,
                              CurrentVa,
                              Length,
                              FALSE);
        }
        else
        {
            MdlPagesPtr = MmGetMdlPfnArray(Mdl);
            MdlPagesPtr += ((ULONG_PTR)CurrentVa - (ULONG_PTR)Mdl->StartVa) >> PAGE_SHIFT;

            PhysicalAddress.QuadPart = *MdlPagesPtr << PAGE_SHIFT;
            PhysicalAddress.QuadPart += BYTE_OFFSET(CurrentVa);

            HighestAcceptableAddress = HalpGetAdapterMaximumPhysicalAddress(AdapterObject);
            if ((PhysicalAddress.QuadPart + Length) > HighestAcceptableAddress.QuadPart)
            {
                HalpCopyBufferMap(Mdl,
                                  RealMapRegisterBase,
                                  CurrentVa,
                                  Length,
                                  FALSE);
            }
        }
    }

    RealMapRegisterBase->Counter = 0;

    return TRUE;
}

/**
 * @name IoMapTransfer
 *
 * Map a DMA for transfer and do the DMA if it's a slave.
 *
 * @param AdapterObject
 *        Adapter object to do the DMA on. Bus-master may pass NULL.
 * @param Mdl
 *        Locked-down user buffer to DMA in to or out of.
 * @param MapRegisterBase
 *        Handle to map registers to use for this dma.
 * @param CurrentVa
 *        Index into Mdl to transfer into/out of.
 * @param Length
 *        Length of transfer. Number of bytes actually transferred on
 *        output.
 * @param WriteToDevice
 *        TRUE if it's an output DMA, FALSE otherwise.
 *
 * @return
 *    A logical address that can be used to program a DMA controller, it's
 *    not meaningful for slave DMA device.
 *
 * @remarks
 *    This function does a copyover to contiguous memory <16MB represented
 *    by the map registers if needed. If the buffer described by MDL can be
 *    used as is no copyover is done.
 *    If it's a slave transfer, this function actually performs it.
 *
 * @implemented
 */
PHYSICAL_ADDRESS
NTAPI
IoMapTransfer(IN PADAPTER_OBJECT AdapterObject,
              IN PMDL Mdl,
              IN PVOID MapRegisterBase,
              IN PVOID CurrentVa,
              IN OUT PULONG Length,
              IN BOOLEAN WriteToDevice)
{
    PPFN_NUMBER MdlPagesPtr;
    PFN_NUMBER MdlPage1, MdlPage2;
    ULONG ByteOffset;
    ULONG TransferOffset;
    ULONG TransferLength;
    BOOLEAN UseMapRegisters;
    PROS_MAP_REGISTER_ENTRY RealMapRegisterBase;
    PHYSICAL_ADDRESS PhysicalAddress;
    PHYSICAL_ADDRESS HighestAcceptableAddress;
    ULONG Counter;
    DMA_MODE AdapterMode;
    KIRQL OldIrql;

    /*
     * Precalculate some values that are used in all cases.
     *
     * ByteOffset is offset inside the page at which the transfer starts.
     * MdlPagesPtr is pointer inside the MDL page chain at the page where the
     *             transfer start.
     * PhysicalAddress is physical address corresponding to the transfer
     *                 start page and offset.
     * TransferLength is the initial length of the transfer, which is reminder
     *                of the first page. The actual value is calculated below.
     *
     * Note that all the variables can change during the processing which
     * takes place below. These are just initial values.
     */
    ByteOffset = BYTE_OFFSET(CurrentVa);

    MdlPagesPtr = MmGetMdlPfnArray(Mdl);
    MdlPagesPtr += ((ULONG_PTR)CurrentVa - (ULONG_PTR)Mdl->StartVa) >> PAGE_SHIFT;

    PhysicalAddress.QuadPart = *MdlPagesPtr << PAGE_SHIFT;
    PhysicalAddress.QuadPart += ByteOffset;

    TransferLength = PAGE_SIZE - ByteOffset;

    /*
     * Special case for bus master adapters with S/G support. We can directly
     * use the buffer specified by the MDL, so not much work has to be done.
     *
     * Just return the passed VA's corresponding physical address and update
     * length to the number of physically contiguous bytes found. Also
     * pages crossing the 4Gb boundary aren't considered physically contiguous.
     */
    if (MapRegisterBase == NULL)
    {
        while (TransferLength < *Length)
        {
            MdlPage1 = *MdlPagesPtr;
            MdlPage2 = *(MdlPagesPtr + 1);
            if (MdlPage1 + 1 != MdlPage2) break;
            if ((MdlPage1 ^ MdlPage2) & ~0xFFFFF) break;
            TransferLength += PAGE_SIZE;
            MdlPagesPtr++;
        }

        if (TransferLength < *Length) *Length = TransferLength;

        return PhysicalAddress;
    }

    /*
     * The code below applies to slave DMA adapters and bus master adapters
     * without hardward S/G support.
     */
    RealMapRegisterBase = (PROS_MAP_REGISTER_ENTRY)((ULONG_PTR)MapRegisterBase & ~MAP_BASE_SW_SG);

    /*
     * Try to calculate the size of the transfer. We can only transfer
     * pages that are physically contiguous and that don't cross the
     * 64Kb boundary (this limitation applies only for ISA controllers).
     */
    while (TransferLength < *Length)
    {
        MdlPage1 = *MdlPagesPtr;
        MdlPage2 = *(MdlPagesPtr + 1);
        if (MdlPage1 + 1 != MdlPage2) break;
        if (!HalpEisaDma && ((MdlPage1 ^ MdlPage2) & ~0xF)) break;
        TransferLength += PAGE_SIZE;
        MdlPagesPtr++;
    }

    if (TransferLength > *Length) TransferLength = *Length;

    /*
     * If we're about to simulate software S/G and not all the pages are
     * physically contiguous then we must use the map registers to store
     * the data and allow the whole transfer to proceed at once.
     */
    if (((ULONG_PTR)MapRegisterBase & MAP_BASE_SW_SG) && (TransferLength < *Length))
    {
        UseMapRegisters = TRUE;
        PhysicalAddress = RealMapRegisterBase->PhysicalAddress;
        PhysicalAddress.QuadPart += ByteOffset;
        TransferLength = *Length;
        RealMapRegisterBase->Counter = MAXULONG;
        Counter = 0;
    }
    else
    {
        /*
         * This is ordinary DMA transfer, so just update the progress
         * counters. These are used by IoFlushAdapterBuffers to track
         * the transfer progress.
         */
        UseMapRegisters = FALSE;
        Counter = RealMapRegisterBase->Counter;
        RealMapRegisterBase->Counter += BYTES_TO_PAGES(ByteOffset + TransferLength);

        /*
         * Check if the buffer doesn't exceed the highest physical address
         * limit of the device. In that case we must use the map registers to
         * store the data.
         */
        HighestAcceptableAddress = HalpGetAdapterMaximumPhysicalAddress(AdapterObject);
        if ((PhysicalAddress.QuadPart + TransferLength) > HighestAcceptableAddress.QuadPart)
        {
            UseMapRegisters = TRUE;
            PhysicalAddress = RealMapRegisterBase[Counter].PhysicalAddress;
            PhysicalAddress.QuadPart += ByteOffset;
            if ((ULONG_PTR)MapRegisterBase & MAP_BASE_SW_SG)
            {
                RealMapRegisterBase->Counter = MAXULONG;
                Counter = 0;
            }
        }
    }

    /*
     * If we decided to use the map registers (see above) and we're about
     * to transfer data to the device then copy the buffers into the map
     * register memory.
     */
    if ((UseMapRegisters) && (WriteToDevice))
    {
        HalpCopyBufferMap(Mdl,
                          RealMapRegisterBase + Counter,
                          CurrentVa,
                          TransferLength,
                          WriteToDevice);
    }

    /*
     * Return the length of transfer that actually takes place.
     */
    *Length = TransferLength;

    /*
     * If we're doing slave (system) DMA then program the (E)ISA controller
     * to actually start the transfer.
     */
    if ((AdapterObject) && !(AdapterObject->MasterDevice))
    {
        AdapterMode = AdapterObject->AdapterMode;

        if (WriteToDevice)
        {
            AdapterMode.TransferType = WRITE_TRANSFER;
        }
        else
        {
            AdapterMode.TransferType = READ_TRANSFER;
            if (AdapterObject->IgnoreCount)
            {
                RtlZeroMemory((PUCHAR)RealMapRegisterBase[Counter].VirtualAddress + ByteOffset,
                              TransferLength);
            }
        }

        TransferOffset = PhysicalAddress.LowPart & 0xFFFF;
        if (AdapterObject->Width16Bits)
        {
            TransferLength >>= 1;
            TransferOffset >>= 1;
        }

        KeAcquireSpinLock(&AdapterObject->MasterAdapter->SpinLock, &OldIrql);

        if (AdapterObject->AdapterNumber == 1)
        {
            PDMA1_CONTROL DmaControl1 = AdapterObject->AdapterBaseVa;

            /* Reset Register */
            WRITE_PORT_UCHAR(&DmaControl1->ClearBytePointer, 0);

            /* Set the Mode */
            WRITE_PORT_UCHAR(&DmaControl1->Mode, AdapterMode.Byte);

            /* Set the Offset Register */
            WRITE_PORT_UCHAR(&DmaControl1->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseAddress,
                             (UCHAR)(TransferOffset));
            WRITE_PORT_UCHAR(&DmaControl1->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseAddress,
                             (UCHAR)(TransferOffset >> 8));

            /* Set the Page Register */
            WRITE_PORT_UCHAR(AdapterObject->PagePort + FIELD_OFFSET(EISA_CONTROL, DmaController1Pages),
                             (UCHAR)(PhysicalAddress.LowPart >> 16));
            if (HalpEisaDma)
            {
                WRITE_PORT_UCHAR(AdapterObject->PagePort + FIELD_OFFSET(EISA_CONTROL, DmaController2Pages),
                                 0);
            }

            /* Set the Length */
            WRITE_PORT_UCHAR(&DmaControl1->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseCount,
                             (UCHAR)(TransferLength - 1));
            WRITE_PORT_UCHAR(&DmaControl1->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseCount,
                             (UCHAR)((TransferLength - 1) >> 8));

            /* Unmask the Channel */
            WRITE_PORT_UCHAR(&DmaControl1->SingleMask, AdapterObject->ChannelNumber | DMA_CLEARMASK);
        }
        else
        {
            PDMA2_CONTROL DmaControl2 = AdapterObject->AdapterBaseVa;

            /* Reset Register */
            WRITE_PORT_UCHAR(&DmaControl2->ClearBytePointer, 0);

            /* Set the Mode */
            WRITE_PORT_UCHAR(&DmaControl2->Mode, AdapterMode.Byte);

            /* Set the Offset Register */
            WRITE_PORT_UCHAR(&DmaControl2->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseAddress,
                             (UCHAR)(TransferOffset));
            WRITE_PORT_UCHAR(&DmaControl2->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseAddress,
                             (UCHAR)(TransferOffset >> 8));

            /* Set the Page Register */
            WRITE_PORT_UCHAR(AdapterObject->PagePort + FIELD_OFFSET(EISA_CONTROL, DmaController1Pages),
                             (UCHAR)(PhysicalAddress.u.LowPart >> 16));
            if (HalpEisaDma)
            {
                WRITE_PORT_UCHAR(AdapterObject->PagePort + FIELD_OFFSET(EISA_CONTROL, DmaController2Pages),
                                 0);
            }

            /* Set the Length */
            WRITE_PORT_UCHAR(&DmaControl2->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseCount,
                             (UCHAR)(TransferLength - 1));
            WRITE_PORT_UCHAR(&DmaControl2->DmaAddressCount[AdapterObject->ChannelNumber].DmaBaseCount,
                             (UCHAR)((TransferLength - 1) >> 8));

            /* Unmask the Channel */
            WRITE_PORT_UCHAR(&DmaControl2->SingleMask,
                             AdapterObject->ChannelNumber | DMA_CLEARMASK);
        }

        KeReleaseSpinLock(&AdapterObject->MasterAdapter->SpinLock, OldIrql);
    }

    /*
     * Return physical address of the buffer with data that is used for the
     * transfer. It can either point inside the Mdl that was passed by the
     * caller or into the map registers if the Mdl buffer can't be used
     * directly.
     */
     return PhysicalAddress;
}
#endif

/**
 * @name HalFlushCommonBuffer
 *
 * @implemented
 */
BOOLEAN
NTAPI
HalFlushCommonBuffer(IN PADAPTER_OBJECT AdapterObject,
                     IN ULONG Length,
                     IN PHYSICAL_ADDRESS LogicalAddress,
                     IN PVOID VirtualAddress)
{
    /* Function always returns true */
    return TRUE;
}

/*
 * @implemented
 */
PVOID
NTAPI
HalAllocateCrashDumpRegisters(IN PADAPTER_OBJECT AdapterObject,
                              IN OUT PULONG NumberOfMapRegisters)
{
    PADAPTER_OBJECT MasterAdapter = AdapterObject->MasterAdapter;
    ULONG MapRegisterNumber;

    /* Check if it needs map registers */
    if (AdapterObject->NeedsMapRegisters)
    {
        /* Check if we have enough */
        if (*NumberOfMapRegisters > AdapterObject->MapRegistersPerChannel)
        {
            /* We don't, fail */
            AdapterObject->NumberOfMapRegisters = 0;
            return NULL;
        }

        /* Try to find free map registers */
        MapRegisterNumber = RtlFindClearBitsAndSet(MasterAdapter->MapRegisters,
                                                   *NumberOfMapRegisters,
                                                   0);

        /* Check if nothing was found */
        if (MapRegisterNumber == MAXULONG)
        {
            /* No free registers found, so use the base registers */
            RtlSetBits(MasterAdapter->MapRegisters,
                       0,
                       *NumberOfMapRegisters);
            MapRegisterNumber = 0;
        }

        /* Calculate the new base */
        AdapterObject->MapRegisterBase =
            (PROS_MAP_REGISTER_ENTRY)(MasterAdapter->MapRegisterBase +
                                      MapRegisterNumber);

        /* Check if scatter gather isn't supported */
        if (!AdapterObject->ScatterGather)
        {
            /* Set the flag */
            AdapterObject->MapRegisterBase =
                (PROS_MAP_REGISTER_ENTRY)
                ((ULONG_PTR)AdapterObject->MapRegisterBase | MAP_BASE_SW_SG);
        }
    }
    else
    {
        AdapterObject->MapRegisterBase = NULL;
        AdapterObject->NumberOfMapRegisters = 0;
    }

    /* Return the base */
    return AdapterObject->MapRegisterBase;
}

/* EOF */

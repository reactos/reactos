/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/fstub/halstub.c
* PURPOSE:         I/O Stub HAL Routines
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
                   Pierre Schweitzer (pierre.schweitzer@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

HAL_DISPATCH HalDispatchTable =
{
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
    xHalHandlerForBus,
    xHalReferenceHandler,
    xHalReferenceHandler,
    xHalInitPnpDriver,
    xHalInitPowerManagement,
    (pHalGetDmaAdapter) NULL,
    xHalGetInterruptTranslator,
    xHalStartMirroring,
    xHalEndMirroring,
    xHalMirrorPhysicalMemory,
    xHalEndOfBoot,
    xHalMirrorPhysicalMemory
};

HAL_PRIVATE_DISPATCH HalPrivateDispatchTable =
{
    HAL_PRIVATE_DISPATCH_VERSION,
    xHalHandlerForBus,
    (pHalHandlerForConfigSpace)xHalHandlerForBus,
    xHalLocateHiberRanges,
    xHalRegisterBusHandler,
    xHalSetWakeEnable,
    xHalSetWakeAlarm,
    xHalTranslateBusAddress,
    (pHalAssignSlotResources)xHalTranslateBusAddress,
    xHalHaltSystem,
    (pHalFindBusAddressTranslation)NULL,
    (pHalResetDisplay)NULL,
    xHalAllocateMapRegisters,
    xKdSetupPciDeviceForDebugging,
    xKdReleasePciDeviceForDebugging,
    xKdGetAcpiTablePhase,
    (pKdCheckPowerButton)xHalReferenceHandler,
    xHalVectorToIDTEntry,
    (pKdMapPhysicalMemory64)MatchAll,
    (pKdUnmapVirtualAddress)xKdUnmapVirtualAddress
};

/* FUNCTIONS *****************************************************************/

UCHAR
NTAPI
xHalVectorToIDTEntry(IN ULONG Vector)
{
    /* Return the vector */
    return (UCHAR)Vector;
}

VOID
NTAPI
xHalHaltSystem(VOID)
{
    /* Halt execution */
    while (TRUE);
}

VOID
NTAPI
xHalEndOfBoot(VOID)
{
    PAGED_CODE();

    /* Nothing */
    return;
}

VOID
NTAPI
xHalSetWakeEnable(IN BOOLEAN Enable)
{
    /* Nothing */
    return;
}

PBUS_HANDLER
FASTCALL
xHalHandlerForBus(IN INTERFACE_TYPE InterfaceType,
                  IN ULONG BusNumber)
{
    return NULL;
}

VOID
FASTCALL
xHalReferenceHandler(IN PBUS_HANDLER BusHandler)
{
    /* Nothing */
    return;
}

NTSTATUS
NTAPI
xHalInitPnpDriver(VOID)
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
xHalInitPowerManagement(IN PPM_DISPATCH_TABLE PmDriverDispatchTable,
                        OUT PPM_DISPATCH_TABLE *PmHalDispatchTable)
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
xHalStartMirroring(VOID)
{
    PAGED_CODE();

    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
xHalEndMirroring(IN ULONG PassNumber)
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
xHalMirrorPhysicalMemory(IN PHYSICAL_ADDRESS PhysicalAddress,
                         IN LARGE_INTEGER NumberOfBytes)
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
xHalQueryBusSlots(IN PBUS_HANDLER BusHandler,
                  IN ULONG BufferSize,
                  OUT PULONG SlotNumbers,
                  OUT PULONG ReturnedLength)
{
    PAGED_CODE();

    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
xHalSetSystemInformation(IN HAL_SET_INFORMATION_CLASS InformationClass,
                         IN ULONG BufferSize,
                         IN PVOID Buffer)
{
    PAGED_CODE();

    return STATUS_INVALID_LEVEL;
}

NTSTATUS
NTAPI
xHalQuerySystemInformation(IN HAL_QUERY_INFORMATION_CLASS InformationClass,
                           IN ULONG BufferSize,
                           IN OUT PVOID Buffer,
                           OUT PULONG ReturnedLength)
{
    PAGED_CODE();

    return STATUS_INVALID_LEVEL;
}

VOID
NTAPI
xHalLocateHiberRanges(IN PVOID MemoryMap)
{
    /* Nothing */
    return;
}

NTSTATUS
NTAPI
xHalRegisterBusHandler(IN INTERFACE_TYPE InterfaceType,
                       IN BUS_DATA_TYPE ConfigSpace,
                       IN ULONG BusNumber,
                       IN INTERFACE_TYPE ParentInterfaceType,
                       IN ULONG ParentBusNumber,
                       IN ULONG ContextSize,
                       IN PINSTALL_BUS_HANDLER InstallCallback,
                       OUT PBUS_HANDLER *BusHandler)
{
    PAGED_CODE();

    return STATUS_NOT_SUPPORTED;
}

VOID
NTAPI
xHalSetWakeAlarm(IN ULONGLONG AlartTime,
                 IN PTIME_FIELDS TimeFields)
{
    /* Nothing */
    return;
}

BOOLEAN
NTAPI
xHalTranslateBusAddress(IN INTERFACE_TYPE InterfaceType,
                        IN ULONG BusNumber,
                        IN PHYSICAL_ADDRESS BusAddress,
                        IN OUT PULONG AddressSpace,
                        OUT PPHYSICAL_ADDRESS TranslatedAddress)
{
    KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0, 0, 0, 0);

    return FALSE;
}

NTSTATUS
NTAPI
xHalAllocateMapRegisters(IN PADAPTER_OBJECT AdapterObject,
                         IN ULONG Unknown,
                         IN ULONG Unknown2,
                         PMAP_REGISTER_ENTRY Registers)
{
    PAGED_CODE();

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
xKdSetupPciDeviceForDebugging(IN PVOID LoaderBlock OPTIONAL,
                              IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
xKdReleasePciDeviceForDebugging(IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice)
{
    return STATUS_NOT_IMPLEMENTED;
}

PVOID
NTAPI
xKdGetAcpiTablePhase(IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
                     IN ULONG Signature)
{
    return NULL;
}

PVOID
NTAPI
MatchAll(IN PHYSICAL_ADDRESS PhysicalAddress,
         IN ULONG NumberPages,
         IN BOOLEAN FlushCurrentTLB)
{
    return NULL;
}

VOID
NTAPI
xKdUnmapVirtualAddress(IN PVOID VirtualAddress,
                       IN ULONG NumberPages,
                       IN BOOLEAN FlushCurrentTLB)
{
    /* Nothing */
    return;
}

/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    haltypes.h

Abstract:

    Type definitions for the HAL.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _HALTYPES_H
#define _HALTYPES_H

//
// Dependencies
//
#include <umtypes.h>

#ifndef NTOS_MODE_USER

//
// HalShutdownSystem Types
//
typedef enum _FIRMWARE_REENTRY
{
    HalHaltRoutine,
    HalPowerDownRoutine,
    HalRestartRoutine,
    HalRebootRoutine,
    HalInteractiveModeRoutine,
    HalMaximumRoutine
} FIRMWARE_REENTRY, *PFIRMWARE_REENTRY;

//
// HAL Private function Types
//
typedef
PBUS_HANDLER
(NTAPI *pHalHandlerForConfigSpace)(
    IN BUS_DATA_TYPE ConfigSpace,
    IN ULONG BusNumber
);

typedef
NTSTATUS
(NTAPI *PINSTALL_BUS_HANDLER)(
    IN PBUS_HANDLER Bus
);

typedef
NTSTATUS
(NTAPI *pHalRegisterBusHandler)(
    IN INTERFACE_TYPE InterfaceType,
    IN BUS_DATA_TYPE ConfigSpace,
    IN ULONG BusNumber,
    IN INTERFACE_TYPE ParentInterfaceType,
    IN ULONG ParentBusNumber,
    IN ULONG ContextSize,
    IN PINSTALL_BUS_HANDLER InstallCallback,
    OUT PBUS_HANDLER *BusHandler
);

typedef
VOID
(NTAPI *pHalSetWakeEnable)(
    IN BOOLEAN Enable
);

typedef
VOID
(NTAPI *pHalSetWakeAlarm)(
    IN ULONGLONG AlartTime,
    IN PTIME_FIELDS TimeFields
);

typedef
VOID
(NTAPI *pHalLocateHiberRanges)(
    IN PVOID MemoryMap
);

typedef
BOOLEAN
(NTAPI *pHalAllocateMapRegisters)(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Unknown,
    IN ULONG Unknown2,
    PMAP_REGISTER_ENTRY Registers
);

//
// HAL Bus Handler Callback Types
//
typedef
NTSTATUS
(NTAPI *pAdjustResourceList)(
    IN PBUS_HANDLER BusHandler,
    IN ULONG BusNumber,
    IN OUT PCM_RESOURCE_LIST Resources
);

typedef
NTSTATUS
(NTAPI *pAssignSlotResources)(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
);

typedef
ULONG
(NTAPI *pGetSetBusData)(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PCI_SLOT_NUMBER SlotNumber,
    OUT PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

typedef
ULONG
(NTAPI *pGetInterruptVector)(
    IN PBUS_HANDLER BusHandler,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
);

typedef
ULONG
(NTAPI *pTranslateBusAddress)(
    IN PBUS_HANDLER BusHandler,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
);

//
// Hal Private dispatch Table
//
#define HAL_PRIVATE_DISPATCH_VERSION        2
typedef struct _HAL_PRIVATE_DISPATCH
{
    ULONG Version;
    pHalHandlerForBus HalHandlerForBus;
    pHalHandlerForConfigSpace HalHandlerForConfigSpace;
    pHalLocateHiberRanges HalLocateHiberRanges;
    pHalRegisterBusHandler HalRegisterBusHandler;
    pHalSetWakeEnable HalSetWakeEnable;
    pHalSetWakeAlarm HalSetWakeAlarm;
    pHalTranslateBusAddress HalPciTranslateBusAddress;
    pHalAssignSlotResources HalPciAssignSlotResources;
    pHalHaltSystem HalHaltSystem;
    pHalFindBusAddressTranslation HalFindBusAddressTranslation;
    pHalResetDisplay HalResetDisplay;
    pHalAllocateMapRegisters HalAllocateMapRegisters;
    pKdSetupPciDeviceForDebugging KdSetupPciDeviceForDebugging;
    pKdReleasePciDeviceForDebugging KdReleasePciDeviceforDebugging;
    pKdGetAcpiTablePhase0 KdGetAcpiTablePhase0;
    pKdCheckPowerButton KdCheckPowerButton;
    pHalVectorToIDTEntry HalVectorToIDTEntry;
    pKdMapPhysicalMemory64 KdMapPhysicalMemory64;
    pKdUnmapVirtualAddress KdUnmapVirtualAddress;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    pKdGetPciDataByOffset KdGetPciDataByOffset;
    pKdSetPciDataByOffset KdSetPciDataByOffset;
    PVOID HalGetInterruptVectorOverride;
    PVOID HalGetVectorInputOverride;
#endif
} HAL_PRIVATE_DISPATCH, *PHAL_PRIVATE_DISPATCH;

//
// HAL Bus Handler
//
typedef struct _BUS_HANDLER
{
    ULONG Version;
    INTERFACE_TYPE InterfaceType;
    BUS_DATA_TYPE ConfigurationType;
    ULONG BusNumber;
    PDEVICE_OBJECT DeviceObject;
    struct _BUS_HANDLER *ParentHandler;
    PVOID BusData;
    ULONG DeviceControlExtensionSize;
    //PSUPPORTED_RANGES BusAddresses;
    ULONG Reserved[4];
    pGetSetBusData GetBusData;
    pGetSetBusData SetBusData;
    pAdjustResourceList AdjustResourceList;
    pAssignSlotResources AssignSlotResources;
    pGetInterruptVector GetInterruptVector;
    pTranslateBusAddress TranslateBusAddress;
} BUS_HANDLER;

//
// Kernel Exports
//
#if defined(_NTDRIVER_) || defined(_NTHAL_)
extern NTSYSAPI PHAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#define HALPRIVATEDISPATCH ((PHAL_PRIVATE_DISPATCH)&HalPrivateDispatchTable)
#else
extern NTSYSAPI HAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#define HALPRIVATEDISPATCH (&HalPrivateDispatchTable)
#endif

//
// HAL Exports
//
#ifndef _NTHAL_
extern NTHALAPI PUCHAR *KdComPortInUse;
#endif

#endif
#endif




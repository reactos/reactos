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
(*pHalHandlerForConfigSpace)(
    IN BUS_DATA_TYPE  ConfigSpace,
    IN ULONG BusNumber
);

typedef
NTSTATUS
(*PINSTALL_BUS_HANDLER)(
    IN PBUS_HANDLER Bus
);

typedef
NTSTATUS
(*pHalRegisterBusHandler)(
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
(*pHalSetWakeEnable)(
    IN BOOLEAN Enable
);


typedef
VOID
(*pHalSetWakeAlarm)(
    IN ULONGLONG AlartTime,
    IN PTIME_FIELDS TimeFields
);

typedef
VOID
(*pHalLocateHiberRanges)(
    IN PVOID MemoryMap
);

typedef
BOOLEAN
(*pHalAllocateMapRegisters)(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Unknown,
    IN ULONG Unknown2,
    PMAP_REGISTER_ENTRY Registers
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
// Kernel Exports
//
#ifndef _NTOSKRNL_
extern PHAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#else
extern HAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#endif

//
// HAL Exports
//
#ifndef _NTHAL_
extern PUCHAR *KdComPortInUse;
#endif

#endif
#endif


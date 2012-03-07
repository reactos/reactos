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
(FASTCALL *pHalHandlerForConfigSpace)(
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
NTSTATUS
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
(NTAPI *PADJUSTRESOURCELIST)(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *Resources
);

typedef
NTSTATUS
(NTAPI *PASSIGNSLOTRESOURCES)(
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
(NTAPI *PGETSETBUSDATA)(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG SlotNumber,
    OUT PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

typedef
ULONG
(NTAPI *PGETINTERRUPTVECTOR)(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
);

typedef
BOOLEAN
(NTAPI *PTRANSLATEBUSADDRESS)(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
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
// HAL Supported Range
//
#define HAL_SUPPORTED_RANGE_VERSION 1
typedef struct _SUPPORTED_RANGE
{
    struct _SUPPORTED_RANGE *Next;
    ULONG SystemAddressSpace;
    LONGLONG SystemBase;
    LONGLONG Base;
    LONGLONG Limit;
} SUPPORTED_RANGE, *PSUPPORTED_RANGE;

typedef struct _SUPPORTED_RANGES 
{
    USHORT Version;
    BOOLEAN Sorted;
    UCHAR Reserved;
    ULONG NoIO;
    SUPPORTED_RANGE IO;
    ULONG NoMemory;
    SUPPORTED_RANGE Memory;
    ULONG NoPrefetchMemory;
    SUPPORTED_RANGE PrefetchMemory;
    ULONG NoDma;
    SUPPORTED_RANGE Dma;
} SUPPORTED_RANGES, *PSUPPORTED_RANGES;

//
// HAL Bus Handler
//
#define HAL_BUS_HANDLER_VERSION 1
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
    PSUPPORTED_RANGES BusAddresses;
    ULONG Reserved[4];
    PGETSETBUSDATA GetBusData;
    PGETSETBUSDATA SetBusData;
    PADJUSTRESOURCELIST AdjustResourceList;
    PASSIGNSLOTRESOURCES AssignSlotResources;
    PGETINTERRUPTVECTOR GetInterruptVector;
    PTRANSLATEBUSADDRESS TranslateBusAddress;
    PVOID Spare1;
    PVOID Spare2;
    PVOID Spare3;
    PVOID Spare4;
    PVOID Spare5;
    PVOID Spare6;
    PVOID Spare7;
    PVOID Spare8;
} BUS_HANDLER;

//
// HAL Chip Hacks
//
#define HAL_PCI_CHIP_HACK_BROKEN_ACPI_TIMER        0x01
#define HAL_PCI_CHIP_HACK_DISABLE_HIBERNATE        0x02
#define HAL_PCI_CHIP_HACK_DISABLE_ACPI_IRQ_ROUTING 0x04
#define HAL_PCI_CHIP_HACK_USB_SMI_DISABLE          0x08

//
// Kernel Exports
//
#if (defined(_NTDRIVER_) || defined(_NTHAL_)) && !defined(_BLDR_)
extern NTSYSAPI PHAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#define HALPRIVATEDISPATCH ((PHAL_PRIVATE_DISPATCH)&HalPrivateDispatchTable)
#else
extern NTSYSAPI HAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#define HALPRIVATEDISPATCH (&HalPrivateDispatchTable)
#endif

//
// HAL Exports
//
extern PUCHAR NTHALAPI KdComPortInUse;

//
// HAL Constants
//
#define HAL_IRQ_TRANSLATOR_VERSION 0x0

#endif
#endif




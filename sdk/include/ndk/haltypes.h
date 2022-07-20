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
    _In_ BUS_DATA_TYPE ConfigSpace,
    _In_ ULONG BusNumber
);

typedef
NTSTATUS
(NTAPI *PINSTALL_BUS_HANDLER)(
    _In_ PBUS_HANDLER Bus
);

typedef
NTSTATUS
(NTAPI *pHalRegisterBusHandler)(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ BUS_DATA_TYPE ConfigSpace,
    _In_ ULONG BusNumber,
    _In_ INTERFACE_TYPE ParentInterfaceType,
    _In_ ULONG ParentBusNumber,
    _In_ ULONG ContextSize,
    _In_ PINSTALL_BUS_HANDLER InstallCallback,
    _Out_ PBUS_HANDLER *BusHandler
);

typedef
VOID
(NTAPI *pHalSetWakeEnable)(
    _In_ BOOLEAN Enable
);

typedef
VOID
(NTAPI *pHalSetWakeAlarm)(
    _In_ ULONGLONG AlartTime,
    _In_ PTIME_FIELDS TimeFields
);

typedef
VOID
(NTAPI *pHalLocateHiberRanges)(
    _In_ PVOID MemoryMap
);

typedef
NTSTATUS
(NTAPI *pHalAllocateMapRegisters)(
    _In_ PADAPTER_OBJECT AdapterObject,
    _In_ ULONG Unknown,
    _In_ ULONG Unknown2,
    PMAP_REGISTER_ENTRY Registers
);

//
// HAL Bus Handler Callback Types
//
typedef
NTSTATUS
(NTAPI *PADJUSTRESOURCELIST)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _Inout_ PIO_RESOURCE_REQUIREMENTS_LIST *Resources
);

typedef
NTSTATUS
(NTAPI *PASSIGNSLOTRESOURCES)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ PUNICODE_STRING RegistryPath,
    _In_ PUNICODE_STRING DriverClassName,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG SlotNumber,
    _Inout_ PCM_RESOURCE_LIST *AllocatedResources
);

typedef
ULONG
(NTAPI *PGETSETBUSDATA)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ ULONG SlotNumber,
    _Out_ PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length
);

typedef
ULONG
(NTAPI *PGETINTERRUPTVECTOR)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ ULONG BusInterruptLevel,
    _In_ ULONG BusInterruptVector,
    _Out_ PKIRQL Irql,
    _Out_ PKAFFINITY Affinity
);

typedef
BOOLEAN
(NTAPI *PTRANSLATEBUSADDRESS)(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PBUS_HANDLER RootHandler,
    _In_ PHYSICAL_ADDRESS BusAddress,
    _Inout_ PULONG AddressSpace,
    _Out_ PPHYSICAL_ADDRESS TranslatedAddress
);

//
// HAL Private dispatch Table
//
// See Version table at:
// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/ntos/hal/hal_private_dispatch.htm
//
#if (NTDDI_VERSION < NTDDI_WINXP)
#define HAL_PRIVATE_DISPATCH_VERSION        1
#elif (NTDDI_VERSION < NTDDI_LONGHORN)
#define HAL_PRIVATE_DISPATCH_VERSION        2
#elif (NTDDI_VERSION >= NTDDI_LONGHORN)
#define HAL_PRIVATE_DISPATCH_VERSION        5
#else
/* Not yet defined */
#endif
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
#if !defined(_NTSYSTEM_) && (defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_))
extern NTSYSAPI PHAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#define HALPRIVATEDISPATCH ((PHAL_PRIVATE_DISPATCH)&HalPrivateDispatchTable)
#else
extern NTSYSAPI HAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#define HALPRIVATEDISPATCH (&HalPrivateDispatchTable)
#endif

//
// HAL Exports
//
extern NTHALAPI PUCHAR KdComPortInUse;

//
// HAL Constants
//
#define HAL_IRQ_TRANSLATOR_VERSION 0x0

//
// BIOS call structure
//
#ifdef _M_AMD64

typedef struct _X86_BIOS_REGISTERS
{
    ULONG Eax;
    ULONG Ecx;
    ULONG Edx;
    ULONG Ebx;
    ULONG Ebp;
    ULONG Esi;
    ULONG Edi;
    USHORT SegDs;
    USHORT SegEs;
} X86_BIOS_REGISTERS, *PX86_BIOS_REGISTERS;

#endif // _M_AMD64

#endif
#endif




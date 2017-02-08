/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    halfuncs.h

Abstract:

    Function definitions for the HAL.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _HALFUNCS_H
#define _HALFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <haltypes.h>
#include <ketypes.h>

#ifndef NTOS_MODE_USER

//
// Private HAL Callbacks
//
#define HalHandlerForBus                HALPRIVATEDISPATCH->HalHandlerForBus
#define HalHandlerForConfigSpace        HALPRIVATEDISPATCH->HalHandlerForConfigSpace
#define HalLocateHiberRanges            HALPRIVATEDISPATCH->HalLocateHiberRanges
#define HalRegisterBusHandler           HALPRIVATEDISPATCH->HalRegisterBusHandler
#define HalSetWakeEnable                HALPRIVATEDISPATCH->HalSetWakeEnable
#define HalSetWakeAlarm                 HALPRIVATEDISPATCH->HalSetWakeAlarm
#define HalPciTranslateBusAddress       HALPRIVATEDISPATCH->HalPciTranslateBusAddress
#define HalPciAssignSlotResources       HALPRIVATEDISPATCH->HalPciAssignSlotResources
#define HalHaltSystem                   HALPRIVATEDISPATCH->HalHaltSystem
#define HalFindBusAddressTranslation    HALPRIVATEDISPATCH->HalFindBusAddressTranslation
#define HalResetDisplay                 HALPRIVATEDISPATCH->HalResetDisplay
#define HalAllocateMapRegisters         HALPRIVATEDISPATCH->HalAllocateMapRegisters
#define KdSetupPciDeviceForDebugging    HALPRIVATEDISPATCH->KdSetupPciDeviceForDebugging
#define KdReleasePciDeviceforDebugging  HALPRIVATEDISPATCH->KdReleasePciDeviceforDebugging
#define KdGetAcpiTablePhase0            HALPRIVATEDISPATCH->KdGetAcpiTablePhase0
#define KdCheckPowerButton              HALPRIVATEDISPATCH->KdCheckPowerButton
#define HalVectorToIDTEntry             HALPRIVATEDISPATCH->HalVectorToIDTEntry
#define KdMapPhysicalMemory64           HALPRIVATEDISPATCH->KdMapPhysicalMemory64
#define KdUnmapVirtualAddress           HALPRIVATEDISPATCH->KdUnmapVirtualAddress

//
// Display Functions
//
NTHALAPI
VOID
NTAPI
HalDisplayString(
    _In_ PCHAR String
);

//
// Initialization Functions
//
NTHALAPI
BOOLEAN
NTAPI
HalAllProcessorsStarted(
    VOID
);

#ifdef _ARC_
NTHALAPI
VOID
NTAPI
HalInitializeProcessor(
    _In_ ULONG ProcessorNumber,
    _In_ struct _LOADER_PARAMETER_BLOCK *LoaderBlock
);

NTHALAPI
BOOLEAN
NTAPI
HalInitSystem(
    _In_ ULONG BootPhase,
    _In_ struct _LOADER_PARAMETER_BLOCK *LoaderBlock
);

NTHALAPI
BOOLEAN
NTAPI
HalStartNextProcessor(
    _In_ struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    _In_ PKPROCESSOR_STATE ProcessorState
);

#endif

NTHALAPI
VOID
NTAPI
HalReturnToFirmware(
    _In_ FIRMWARE_REENTRY Action
);

//
// CPU Routines
//
NTHALAPI
VOID
NTAPI
HalProcessorIdle(
    VOID
);

//
// Interrupt Functions
//
NTHALAPI
BOOLEAN
NTAPI
HalBeginSystemInterrupt(
    _In_ KIRQL Irql,
    _In_ ULONG Vector,
    _Out_ PKIRQL OldIrql
);

VOID
FASTCALL
HalClearSoftwareInterrupt(
    _In_ KIRQL Request
);

NTHALAPI
VOID
NTAPI
HalDisableSystemInterrupt(
    _In_ ULONG Vector,
    _In_ KIRQL Irql
);

NTHALAPI
BOOLEAN
NTAPI
HalEnableSystemInterrupt(
    _In_ ULONG Vector,
    _In_ KIRQL Irql,
    _In_ KINTERRUPT_MODE InterruptMode
);

#ifdef __REACTOS__
NTHALAPI
VOID
NTAPI
HalEndSystemInterrupt(
    _In_ KIRQL Irql,
    _In_ PKTRAP_FRAME TrapFrame
);
#else
NTHALAPI
VOID
NTAPI
HalEndSystemInterrupt(
    _In_ KIRQL Irql,
    _In_ UCHAR Vector
);
#endif

#ifdef _ARM_ // FIXME: ndk/arm? armddk.h?
ULONG
HalGetInterruptSource(
    VOID
);
#endif

NTHALAPI
VOID
NTAPI
HalReportResourceUsage(
    VOID
);

NTHALAPI
VOID
FASTCALL
HalRequestSoftwareInterrupt(
    _In_ KIRQL SoftwareInterruptRequested
);

NTHALAPI
VOID
NTAPI
HalRequestIpi(
    _In_ KAFFINITY TargetSet
);

NTHALAPI
VOID
NTAPI
HalHandleNMI(
    PVOID NmiInfo
);

NTHALAPI
UCHAR
FASTCALL
HalSystemVectorDispatchEntry(
    _In_ ULONG Vector,
    _Out_ PKINTERRUPT_ROUTINE **FlatDispatch,
    _Out_ PKINTERRUPT_ROUTINE *NoConnection
);

//
// Bus Functions
//
NTHALAPI
NTSTATUS
NTAPI
HalAdjustResourceList(
    _Inout_ PIO_RESOURCE_REQUIREMENTS_LIST *pResourceList
);

//
// Environment Functions
//
#ifdef _ARC_
NTHALAPI
ARC_STATUS
NTAPI
HalSetEnvironmentVariable(
    _In_ PCH Name,
    _In_ PCH Value
);

NTHALAPI
ARC_STATUS
NTAPI
HalGetEnvironmentVariable(
    _In_ PCH Variable,
    _In_ USHORT Length,
    _Out_ PCH Buffer
);
#endif

//
// Profiling Functions
//
VOID
NTAPI
HalStartProfileInterrupt(
    _In_ KPROFILE_SOURCE ProfileSource
);

NTHALAPI
VOID
NTAPI
HalStopProfileInterrupt(
    _In_ KPROFILE_SOURCE ProfileSource
);

NTHALAPI
ULONG_PTR
NTAPI
HalSetProfileInterval(
    _In_ ULONG_PTR Interval
);

//
// Time Functions
//
NTHALAPI
BOOLEAN
NTAPI
HalQueryRealTimeClock(
    _In_ PTIME_FIELDS RtcTime
);

NTHALAPI
BOOLEAN
NTAPI
HalSetRealTimeClock(
    _In_ PTIME_FIELDS RtcTime
);

NTHALAPI
ULONG
NTAPI
HalSetTimeIncrement(
    _In_ ULONG Increment
);

#endif
#endif

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
// The DDK steals these away from you.
//
#ifdef _MSC_VER
//void _enable(void);
//void _disable(void);
#pragma intrinsic(_enable)
#pragma intrinsic(_disable)
#endif

//
// Display Functions
//
NTHALAPI
VOID
NTAPI
HalDisplayString(
    IN PCHAR String
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
    ULONG ProcessorNumber,
    struct _LOADER_PARAMETER_BLOCK *LoaderBlock
);

NTHALAPI
BOOLEAN
NTAPI
HalInitSystem(
    ULONG BootPhase,
    struct _LOADER_PARAMETER_BLOCK *LoaderBlock
);

NTHALAPI
BOOLEAN
NTAPI
HalStartNextProcessor(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN PKPROCESSOR_STATE ProcessorState
);

#endif

NTHALAPI
VOID
NTAPI
HalReturnToFirmware(
    FIRMWARE_REENTRY Action
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
    KIRQL Irql,
    ULONG Vector,
    PKIRQL OldIrql
);

NTHALAPI
BOOLEAN
NTAPI
HalDisableSystemInterrupt(
    ULONG Vector,
    KIRQL Irql
);

NTHALAPI
BOOLEAN
NTAPI
HalEnableSystemInterrupt(
    ULONG Vector,
    KIRQL Irql,
    KINTERRUPT_MODE InterruptMode
);

NTHALAPI
VOID
NTAPI
HalEndSystemInterrupt(
    KIRQL Irql,
    ULONG Vector
);

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
    KIRQL SoftwareInterruptRequested
);

NTHALAPI
VOID
NTAPI
HalRequestIpi(
    KAFFINITY TargetSet
);

NTHALAPI
VOID
NTAPI
HalHandleNMI(
    PVOID NmiInfo
);

//
// Environment Functions
//
#ifdef _ARC_
NTHALAPI
ARC_STATUS
NTAPI
HalSetEnvironmentVariable(
    IN PCH Name,
    IN PCH Value
);

NTHALAPI
ARC_STATUS
NTAPI
HalGetEnvironmentVariable(
    IN PCH Variable,
    IN USHORT Length,
    OUT PCH Buffer
);
#endif

//
// Time Functions
//
NTHALAPI
BOOLEAN
NTAPI
HalQueryRealTimeClock(
    IN PTIME_FIELDS RtcTime
);

NTHALAPI
BOOLEAN
NTAPI
HalSetRealTimeClock(
    IN PTIME_FIELDS RtcTime
);

#endif
#endif

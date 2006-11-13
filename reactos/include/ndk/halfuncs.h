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

#ifndef NTOS_MODE_USER

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
#endif

NTHALAPI
VOID
NTAPI
HalReturnToFirmware(
    FIRMWARE_REENTRY Action
);

NTHALAPI
BOOLEAN
NTAPI
HalStartNextProcessor(
    ULONG Unknown1,
    ULONG Unknown2
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
BOOLEAN
NTAPI
HalGetEnvironmentVariable(
    PCH Variable,
    USHORT Length,
    PCH Buffer
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
NTHALAPI
BOOLEAN
NTAPI
HalSetEnvironmentVariable(
    IN PCH Name,
    IN PCH Value
);

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
VOID
NTAPI
HalSetRealTimeClock(
    IN PTIME_FIELDS RtcTime
);

#endif
#endif

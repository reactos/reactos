/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    halfuncs.h

Abstract:

    Function definitions for the HAL.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

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
VOID
_enable(
    VOID
);
VOID
_disable(
    VOID
);
#ifdef _MSC_VER
#pragma intrinsic(_enable)
#pragma intrinsic(_disable)
#endif

//
// Display Functions
//
NTHALAPI
BOOLEAN
NTAPI
HalQueryDisplayOwnership(
    VOID
);

NTHALAPI
VOID
NTAPI
HalDisplayString(
    IN PCHAR String
);

NTHALAPI
BOOLEAN
NTAPI
HalQueryDisplayOwnership(
    VOID
);

NTHALAPI
VOID
NTAPI
HalReleaseDisplayOwnership(
    VOID
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

NTHALAPI
VOID
NTAPI
HalInitializeProcessor(
    ULONG ProcessorNumber,
    PLOADER_PARAMETER_BLOCK LoaderBlock
);

NTHALAPI
BOOLEAN
NTAPI
HalInitSystem(
    ULONG BootPhase,
    PLOADER_PARAMETER_BLOCK LoaderBlock
);

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
// Interrupt Functions
//
NTHALAPI
BOOLEAN
NTAPI
HalBeginSystemInterrupt(
    ULONG Vector,
    KIRQL Irql,
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
    PCH Name,
    PCH Value,
    USHORT ValueLength
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
    ULONG Unknown
);

NTHALAPI
VOID
NTAPI
HalHandleNMI(
    ULONG Unknown
);

//
// I/O Functions
//
NTHALAPI
VOID
NTAPI
IoAssignDriveLetters(
    struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    PSTRING NtDeviceName,
    PUCHAR NtSystemPath,
    PSTRING NtSystemPathString
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

#endif
#endif

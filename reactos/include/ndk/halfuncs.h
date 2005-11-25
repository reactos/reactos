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
// Display Functions
//
BOOLEAN
NTAPI
HalQueryDisplayOwnership(
    VOID
);

VOID
NTAPI
HalDisplayString(
    IN PCHAR String
);

BOOLEAN
NTAPI
HalQueryDisplayOwnership(
    VOID
);

VOID
NTAPI
HalReleaseDisplayOwnership(
    VOID
);

//
// Initialization Functions
//
BOOLEAN
NTAPI
HalAllProcessorsStarted(
    VOID
);

VOID
NTAPI
HalInitializeProcessor(
    ULONG ProcessorNumber,
    PVOID ProcessorStack
);

BOOLEAN
NTAPI
HalInitSystem(
    ULONG BootPhase,
    PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
HalReturnToFirmware(
    FIRMWARE_REENTRY Action
);

BOOLEAN
NTAPI
HalStartNextProcessor(
    ULONG Unknown1,
    ULONG Unknown2
);

//
// Interrupt Functions
//
BOOLEAN
NTAPI
HalBeginSystemInterrupt(
    ULONG Vector,
    KIRQL Irql,
    PKIRQL OldIrql
);

BOOLEAN
NTAPI
HalDisableSystemInterrupt(
    ULONG Vector,
    KIRQL Irql
);

BOOLEAN
NTAPI
HalEnableSystemInterrupt(
    ULONG Vector,
    KIRQL Irql,
    KINTERRUPT_MODE InterruptMode
);

VOID
NTAPI
HalEndSystemInterrupt(
    KIRQL Irql,
    ULONG Vector
);

BOOLEAN
NTAPI
HalGetEnvironmentVariable(
    PCH Name,
    PCH Value,
    USHORT ValueLength
);

VOID
NTAPI
HalReportResourceUsage(
    VOID
);

VOID
FASTCALL
HalRequestSoftwareInterrupt(
    KIRQL SoftwareInterruptRequested
);

VOID
NTAPI
HalRequestIpi(
    ULONG Unknown
);

//
// I/O Functions
//
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
BOOLEAN
NTAPI
HalSetEnvironmentVariable(
    IN PCH Name,
    IN PCH Value
);

#endif
#endif

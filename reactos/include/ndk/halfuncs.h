/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/halfuncs.h
 * PURPOSE:         Prototypes for exported HAL Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _HALFUNCS_H
#define _HALFUNCS_H

/* DEPENDENCIES **************************************************************/
#include "haltypes.h"

/* FUNCTION TYPES ************************************************************/

/* PROTOTYPES ****************************************************************/

BOOLEAN
NTAPI
HalQueryDisplayOwnership(VOID);

BOOLEAN
NTAPI
HalAllProcessorsStarted(VOID);

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

VOID
NTAPI
HalDisplayString (
    IN PCHAR String
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

BOOLEAN
NTAPI
HalQueryDisplayOwnership(VOID);

VOID
NTAPI
HalReportResourceUsage(VOID);

VOID
FASTCALL
HalRequestSoftwareInterrupt(
    KIRQL SoftwareInterruptRequested
);

VOID
NTAPI
HalReleaseDisplayOwnership(VOID);

VOID
NTAPI
HalReturnToFirmware(
    FIRMWARE_REENTRY Action
);

VOID
NTAPI
HalRequestIpi(
    ULONG Unknown
);

BOOLEAN
NTAPI
HalSetEnvironmentVariable(
    IN PCH Name,
    IN PCH Value
);

BOOLEAN
NTAPI
HalStartNextProcessor(
    ULONG Unknown1,
    ULONG Unknown2
);

VOID
NTAPI
IoAssignDriveLetters(
    struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    PSTRING NtDeviceName,
    PUCHAR NtSystemPath,
    PSTRING NtSystemPathString
);

#endif

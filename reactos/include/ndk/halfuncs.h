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

VOID
STDCALL
HalAcquireDisplayOwnership(
    IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters
);

BOOLEAN
STDCALL
HalAllProcessorsStarted(VOID);

NTSTATUS
STDCALL
HalAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PWAIT_CONTEXT_BLOCK WaitContextBlock,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine
);

BOOLEAN
STDCALL
HalBeginSystemInterrupt(
    ULONG Vector,
    KIRQL Irql,
    PKIRQL OldIrql
);

BOOLEAN
STDCALL
HalDisableSystemInterrupt(
    ULONG Vector,
    KIRQL Irql
);

VOID
STDCALL
HalDisplayString (
    IN PCHAR String
);

BOOLEAN
STDCALL
HalEnableSystemInterrupt(
    ULONG Vector,
    KIRQL Irql,
    KINTERRUPT_MODE InterruptMode
);

VOID
STDCALL
HalEndSystemInterrupt(
    KIRQL Irql,
    ULONG Vector
);

BOOLEAN
STDCALL
HalGetEnvironmentVariable(
    PCH Name,
    PCH Value,
    USHORT ValueLength
);

VOID
STDCALL
HalInitializeProcessor(
    ULONG ProcessorNumber,
    PVOID ProcessorStack
);

BOOLEAN
STDCALL
HalInitSystem(
    ULONG BootPhase,
    PLOADER_PARAMETER_BLOCK LoaderBlock
);

BOOLEAN
STDCALL
HalQueryDisplayOwnership(VOID);

VOID
STDCALL
HalReportResourceUsage(VOID);

VOID
FASTCALL
HalRequestSoftwareInterrupt(
    KIRQL SoftwareInterruptRequested
);

VOID
STDCALL
HalReleaseDisplayOwnership(VOID);

VOID
STDCALL
HalReturnToFirmware(
    FIRMWARE_REENTRY Action
);

VOID
STDCALL
HalRequestIpi(
    ULONG Unknown
);

BOOLEAN
STDCALL
HalSetEnvironmentVariable(
    IN PCH Name,
    IN PCH Value
);

BOOLEAN
STDCALL
HalStartNextProcessor(
    ULONG Unknown1,
    ULONG Unknown2
);

VOID
STDCALL
IoAssignDriveLetters(
    struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    PSTRING NtDeviceName,
    PUCHAR NtSystemPath,
    PSTRING NtSystemPathString
);

#endif

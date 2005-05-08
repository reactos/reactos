/* $Id: kefuncs.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
 *
 *  ReactOS Headers
 *  Copyright (C) 1998-2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/kefuncs.h
 * PURPOSE:         Prototypes for Kernel Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _KEFUNCS_H
#define _KEFUNCS_H

#include "ketypes.h"

VOID 
STDCALL
KeInitializeApc(
	IN PKAPC  Apc,
	IN PKTHREAD  Thread,
	IN KAPC_ENVIRONMENT  TargetEnvironment,
	IN PKKERNEL_ROUTINE  KernelRoutine,
	IN PKRUNDOWN_ROUTINE  RundownRoutine OPTIONAL,
	IN PKNORMAL_ROUTINE  NormalRoutine,
	IN KPROCESSOR_MODE  Mode,
	IN PVOID  Context
);	

VOID
STDCALL
KeEnterKernelDebugger(VOID);

VOID
FASTCALL
KiAcquireSpinLock(
	PKSPIN_LOCK SpinLock
);

VOID
FASTCALL
KiReleaseSpinLock(
	PKSPIN_LOCK SpinLock
);

VOID
STDCALL
KiDeliverApc(
	IN KPROCESSOR_MODE  PreviousMode,
	IN PVOID  Reserved,
	IN PKTRAP_FRAME  TrapFrame
);

VOID
STDCALL
KiDispatchInterrupt(VOID);


BOOLEAN
STDCALL
KeAreApcsDisabled(
	VOID
	);

VOID
STDCALL
KeFlushQueuedDpcs(
	VOID
	);

ULONG
STDCALL
KeGetRecommendedSharedDataAlignment(
	VOID
	);

ULONG
STDCALL
KeQueryRuntimeThread(
	IN PKTHREAD Thread,
	OUT PULONG UserTime
	);    

BOOLEAN
STDCALL
KeSetKernelStackSwapEnable(
	IN BOOLEAN Enable
	);

BOOLEAN
STDCALL
KeDeregisterBugCheckReasonCallback(
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord
    );

BOOLEAN
STDCALL
KeRegisterBugCheckReasonCallback(
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
    IN PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PUCHAR Component
    );

VOID 
STDCALL
KeTerminateThread(
	IN KPRIORITY   	 Increment  	 
);

BOOLEAN
STDCALL
KeIsExecutingDpc(
	VOID
);

VOID
STDCALL
KeSetEventBoostPriority(
	IN PKEVENT Event,
	IN PKTHREAD *Thread OPTIONAL
);

PVOID
STDCALL
KeFindConfigurationEntry(
    IN PVOID Unknown,
    IN ULONG Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG RegKey
);

PVOID
STDCALL
KeFindConfigurationNextEntry(
    IN PVOID Unknown,
    IN ULONG Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG RegKey,
    IN PVOID *NextLink
);

VOID
STDCALL
KeFlushEntireTb(
    IN BOOLEAN Unknown,
    IN BOOLEAN CurrentCpuOnly
);

VOID
STDCALL
KeRevertToUserAffinityThread(
    VOID
);

VOID
STDCALL
KiCoprocessorError(
    VOID
);

VOID
STDCALL
KiUnexpectedInterrupt(
    VOID
);

VOID
STDCALL
KeSetDmaIoCoherency(
    IN ULONG Coherency
);

VOID
STDCALL
KeSetProfileIrql(
    IN KIRQL ProfileIrql
);

NTSTATUS
STDCALL
KeSetAffinityThread(
    PKTHREAD Thread,
    KAFFINITY Affinity
);
            
VOID
STDCALL
KeSetSystemAffinityThread(
    IN KAFFINITY Affinity
);

NTSTATUS
STDCALL
KeUserModeCallback(
    IN ULONG	FunctionID,
    IN PVOID	InputBuffer,
    IN ULONG	InputLength,
    OUT PVOID	*OutputBuffer,
    OUT PULONG	OutputLength
);

VOID
STDCALL
KeSetTimeIncrement(
    IN ULONG MaxIncrement,
    IN ULONG MinIncrement
);

VOID
STDCALL
KeInitializeInterrupt(
	PKINTERRUPT InterruptObject,
	PKSERVICE_ROUTINE ServiceRoutine,
	PVOID ServiceContext,
	PKSPIN_LOCK SpinLock,
	ULONG Vector,
	KIRQL Irql,
	KIRQL SynchronizeIrql,
	KINTERRUPT_MODE InterruptMode,
	BOOLEAN ShareVector,
	CHAR ProcessorNumber,
	BOOLEAN FloatingSave
);
	
BOOLEAN
STDCALL 
KeConnectInterrupt(
	PKINTERRUPT InterruptObject
);

VOID 
STDCALL 
KeDisconnectInterrupt(
	PKINTERRUPT InterruptObject
);

struct _KPROCESS* 
KeGetCurrentProcess(
    VOID
);

VOID
KeSetGdtSelector(
    ULONG Entry,
    ULONG Value1,
    ULONG Value2
);

LONG
STDCALL
KeReadStateMutant(
    IN PKMUTANT Mutant
);

VOID
STDCALL
KeInitializeMutant(
    IN PKMUTANT Mutant,
    IN BOOLEAN InitialOwner
);
                
LONG
STDCALL
KeReleaseMutant(
    IN PKMUTANT Mutant,
    IN KPRIORITY Increment,
    IN BOOLEAN Abandon,
    IN BOOLEAN Wait
);

NTSTATUS
STDCALL
KeRaiseUserException(
    IN NTSTATUS ExceptionCode
    );
    
#endif

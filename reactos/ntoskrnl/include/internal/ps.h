/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
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
/* $Id: ps.h,v 1.42 2002/09/07 15:12:51 chorns Exp $
 *
 * FILE:            ntoskrnl/ke/kthread.c
 * PURPOSE:         Process manager definitions
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

#ifndef __INCLUDE_INTERNAL_PS_H
#define __INCLUDE_INTERNAL_PS_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifndef AS_INVOKED

#include <internal/mm.h>

/* Forward declarations. */

struct _KTHREAD;
struct _KTRAPFRAME;

#endif /* !AS_INVOKED */

#include <internal/arch/ps.h>

#ifndef AS_INVOKED

#include <internal/mm.h>

extern HANDLE SystemProcessHandle;
extern LCID PsDefaultThreadLocaleId;
extern LCID PsDefaultSystemLocaleId;

VOID PiInitDefaultLocale(VOID);
VOID PiInitProcessManager(VOID);
VOID PiShutdownProcessManager(VOID);
VOID PsInitThreadManagment(VOID);
VOID PsInitProcessManagment(VOID);
VOID PsInitIdleThread(VOID);
VOID PsDispatchThreadNoLock(ULONG NewThreadStatus);
VOID PiTerminateProcessThreads(PEPROCESS Process, NTSTATUS ExitStatus);
VOID PsTerminateOtherThread(PETHREAD Thread, NTSTATUS ExitStatus);
VOID PsReleaseThread(PETHREAD Thread);
VOID PsBeginThread(PKSTART_ROUTINE StartRoutine, PVOID StartContext);
VOID PsBeginThreadWithContextInternal(VOID);
VOID PiKillMostProcesses(VOID);
NTSTATUS STDCALL PiTerminateProcess(PEPROCESS Process, NTSTATUS ExitStatus);
VOID PiInitApcManagement(VOID);
VOID STDCALL PiDeleteThread(PVOID ObjectBody);
VOID PsReapThreads(VOID);
NTSTATUS 
PsInitializeThread(HANDLE ProcessHandle,
		   PETHREAD* ThreadPtr,
		   PHANDLE ThreadHandle,
		   ACCESS_MASK DesiredAccess,
		   POBJECT_ATTRIBUTES ObjectAttributes,
		   BOOLEAN First);

PIACCESS_TOKEN PsReferenceEffectiveToken(PETHREAD Thread,
					PTOKEN_TYPE TokenType,
					PUCHAR b,
					PSECURITY_IMPERSONATION_LEVEL Level);

NTSTATUS PsOpenTokenOfProcess(HANDLE ProcessHandle,
			      PIACCESS_TOKEN* Token);

NTSTATUS PsSuspendThread(PETHREAD Thread, PULONG PreviousCount);
NTSTATUS PsResumeThread(PETHREAD Thread, PULONG PreviousCount);


#define THREAD_STATE_INITIALIZED  (0)
#define THREAD_STATE_READY        (1)
#define THREAD_STATE_RUNNING      (2)
#define THREAD_STATE_SUSPENDED    (3)
#define THREAD_STATE_FROZEN       (4)
#define THREAD_STATE_TERMINATED_1 (5)
#define THREAD_STATE_TERMINATED_2 (6)
#define THREAD_STATE_BLOCKED      (7)
#define THREAD_STATE_MAX          (8)


/*
 * Internal thread priorities, added by Phillip Susi
 * TODO: rebalence these to make use of all priorities... the ones above 16 
 * can not all be used right now
 */
#define PROCESS_PRIO_IDLE			3
#define PROCESS_PRIO_NORMAL			8
#define PROCESS_PRIO_HIGH			13
#define PROCESS_PRIO_RT				18


VOID 
KeInitializeThread(PKPROCESS Process, PKTHREAD Thread, BOOLEAN First);
NTSTATUS KeReleaseThread(PETHREAD Thread);
VOID STDCALL PiDeleteProcess(PVOID ObjectBody);
VOID PsReapThreads(VOID);
VOID PsUnfreezeOtherThread(PETHREAD Thread);
VOID PsFreezeOtherThread(PETHREAD Thread);
VOID PsFreezeProcessThreads(PEPROCESS Process);
VOID PsUnfreezeProcessThreads(PEPROCESS Process);
PEPROCESS PsGetNextProcess(PEPROCESS OldProcess);
VOID
PsBlockThread(PNTSTATUS Status, UCHAR Alertable, ULONG WaitMode, 
	      BOOLEAN DispatcherLock, KIRQL WaitIrql);
VOID
PsUnblockThread(PETHREAD Thread, PNTSTATUS WaitStatus);
VOID
PsApplicationProcessorInit(VOID);
VOID
PsPrepareForApplicationProcessorInit(ULONG Id);
VOID STDCALL
PsIdleThreadMain(PVOID Context);

VOID STDCALL
PiSuspendThreadRundownRoutine(PKAPC Apc);
VOID STDCALL
PiSuspendThreadKernelRoutine(PKAPC Apc,
			     PKNORMAL_ROUTINE* NormalRoutine,
			     PVOID* NormalContext,
			     PVOID* SystemArgument1,
			     PVOID* SystemArguemnt2);
VOID STDCALL
PiSuspendThreadNormalRoutine(PVOID NormalContext,
			     PVOID SystemArgument1,
			     PVOID SystemArgument2);
VOID STDCALL
PsDispatchThread(ULONG NewThreadStatus);
VOID
PsInitialiseSuspendImplementation(VOID);

extern ULONG PiNrThreadsAwaitingReaping;

#endif /* !AS_INVOKED */

#endif /* __INCLUDE_INTERNAL_PS_H */

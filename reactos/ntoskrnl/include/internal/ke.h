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

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_KE_H

/* INCLUDES *****************************************************************/

#ifndef __ASM__
#include <ddk/ntddk.h>

#include <stdarg.h>
#endif /* not __ASM__ */

#include <internal/arch/ke.h>

/* INTERNAL KERNEL FUNCTIONS ************************************************/

#ifndef __ASM__

struct _KTHREAD;

VOID KiUpdateSystemTime (KIRQL oldIrql, ULONG Eip);

VOID KeAcquireDispatcherDatabaseLock(BOOLEAN Wait);
VOID KeReleaseDispatcherDatabaseLock(BOOLEAN Wait);
BOOLEAN KeDispatcherObjectWake(DISPATCHER_HEADER* hdr);
VOID STDCALL KeExpireTimers(PKDPC Apc,
			    PVOID Arg1,
			    PVOID Arg2,
			    PVOID Arg3);
VOID KeInitializeDispatcherHeader(DISPATCHER_HEADER* Header, ULONG Type,
				  ULONG Size, ULONG SignalState);
VOID KeDumpStackFrames(PULONG Frame);
BOOLEAN KiTestAlert(VOID);
VOID KeRemoveAllWaitsThread(struct _ETHREAD* Thread, NTSTATUS WaitStatus);
PULONG KeGetStackTopThread(struct _ETHREAD* Thread);
VOID KeContextToTrapFrame(PCONTEXT Context,
			  PKTRAP_FRAME TrapFrame);
VOID KeReleaseDispatcherDatabaseLockAtDpcLevel(BOOLEAN Wait);
VOID
KiDeliverNormalApc(VOID);

BOOLEAN STDCALL KeRemoveQueueApc (PKAPC Apc);

/* INITIALIZATION FUNCTIONS *************************************************/

VOID KeInitExceptions(VOID);
VOID KeInitInterrupts(VOID);
VOID KeInitTimer(VOID);
VOID KeInitDpc(VOID);
VOID KeInitDispatcher(VOID);
VOID KeInitializeDispatcher(VOID);
VOID KeInitializeTimerImpl(VOID);
VOID KeInitializeBugCheck(VOID);
VOID Phase1Initialization(PVOID Context);

VOID KeInit1(VOID);
VOID KeInit2(VOID);

BOOLEAN KiDeliverUserApc(PKTRAP_FRAME TrapFrame);
VOID
KiAddProfileEvent(KPROFILE_SOURCE Source, ULONG Pc);
VOID 
KiDispatchException(PEXCEPTION_RECORD ExceptionRecord,
		    PCONTEXT Context,
		    PKTRAP_FRAME Tf,
		    KPROCESSOR_MODE PreviousMode,
		    BOOLEAN SearchFrames);
VOID KeTrapFrameToContext(PKTRAP_FRAME TrapFrame,
			  PCONTEXT Context);
VOID
KeApplicationProcessorInit();
VOID
KePrepareForApplicationProcessorInit(ULONG id);
ULONG
KiUserTrapHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr, PVOID Cr2);
VOID STDCALL
KePushAndStackSwitchAndSysRet(ULONG Push, PVOID NewStack);
VOID STDCALL
KeStackSwitchAndRet(PVOID NewStack);
VOID STDCALL
KeBugCheckWithTf(ULONG BugCheckCode, 	     
		 ULONG BugCheckParameter1,
		 ULONG BugCheckParameter2,
		 ULONG BugCheckParameter3,
		 ULONG BugCheckParameter4,
		 PKTRAP_FRAME Tf);
VOID
KiDumpTrapFrame(PKTRAP_FRAME Tf, ULONG ExceptionNr, ULONG cr2);

#endif /* not __ASM__ */

#define MAXIMUM_PROCESSORS      32

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_KE_H */

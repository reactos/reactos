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

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_KERNEL_H
#define __NTOSKRNL_INCLUDE_INTERNAL_KERNEL_H

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <stdarg.h>

/* INTERNAL KERNEL FUNCTIONS ************************************************/

struct _KTHREAD;

typedef struct _KTRAP_FRAME
{
   PVOID DebugEbp;
   PVOID DebugEip;
   PVOID DebugArgMark;
   PVOID DebugPointer;
   PVOID TempCs;
   PVOID TempEip;
   PVOID Dr0;
   PVOID Dr1;
   PVOID Dr2;
   PVOID Dr3;
   PVOID Dr6;
   PVOID Dr7;
   USHORT Gs;
   USHORT Reserved1;
   USHORT Es;
   USHORT Reserved2;
   USHORT Ds;
   USHORT Reserved3;
   ULONG Edx;
   ULONG Ecx;
   ULONG Eax;
   ULONG PreviousMode;
   PVOID ExceptionList;
   USHORT Fs;
   USHORT Reserved4;
   ULONG Edi;
   ULONG Esi;
   ULONG Ebx;
   ULONG Ebp;
   ULONG ErrorCode;
   ULONG Eip;
   ULONG Cs;
   ULONG Eflags;
   ULONG Esp;
   USHORT Ss;
   USHORT Reserved5;
   USHORT V86_Es;
   USHORT Reserved6;
   USHORT V86_Ds;
   USHORT Reserved7;
   USHORT V86_Fs;
   USHORT Reserved8;
   USHORT V86_Gs;
   USHORT Reserved9;
} KTRAP_FRAME, *PKTRAP_FRAME;

VOID KiUpdateSystemTime (VOID);

VOID KeAcquireDispatcherDatabaseLock(BOOLEAN Wait);
VOID KeReleaseDispatcherDatabaseLock(BOOLEAN Wait);
BOOLEAN KeDispatcherObjectWake(DISPATCHER_HEADER* hdr);

#if 0
VOID KiInterruptDispatch(ULONG irq);
#endif
VOID KeExpireTimers(VOID);
NTSTATUS KeAddThreadTimeout(struct _KTHREAD* Thread, 
			    PLARGE_INTEGER Interval);
VOID KeInitializeDispatcherHeader(DISPATCHER_HEADER* Header, ULONG Type,
				  ULONG Size, ULONG SignalState);

VOID KeDumpStackFrames(PVOID Stack, ULONG NrFrames);
ULONG KeAllocateGdtSelector(ULONG Desc[2]);
VOID KeFreeGdtSelector(ULONG Entry);
BOOLEAN KiTestAlert(VOID);
VOID KeRemoveAllWaitsThread(struct _ETHREAD* Thread, NTSTATUS WaitStatus);
PULONG KeGetStackTopThread(struct _ETHREAD* Thread);
VOID KeContextToTrapFrame(PCONTEXT Context,
			  PKTRAP_FRAME TrapFrame);

/* INITIALIZATION FUNCTIONS *************************************************/

VOID KeInitExceptions(VOID);
VOID KeInitInterrupts(VOID);
VOID KeInitTimer(VOID);
VOID KeInitDpc(VOID);
VOID KeInitDispatcher(VOID);
VOID KeInitializeDispatcher(VOID);
VOID KeInitializeTimerImpl(VOID);
VOID KeInitializeBugCheck(VOID);

VOID KeInit1(VOID);
VOID KeInit2(VOID);

BOOLEAN KiDeliverUserApc(PKTRAP_FRAME TrapFrame);
VOID
NtEarlyInitVdm(VOID);

#endif

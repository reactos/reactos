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
#include <ddk/ntifs.h>
#include <stdarg.h>
#endif /* not __ASM__ */

#include "arch/ke.h"

/* INTERNAL KERNEL FUNCTIONS ************************************************/

#ifdef __USE_W32API
struct _KPROCESS* KeGetCurrentProcess(VOID);
VOID KeSetGdtSelector(ULONG Entry, ULONG Value1, ULONG Value2);
#endif

#ifndef __ASM__

struct _KTHREAD;
struct _KIRQ_TRAPFRAME;
struct _KPCR;
struct _KPRCB;
struct _KEXCEPTION_FRAME;

#define IPI_REQUEST_FUNCTIONCALL    0
#define IPI_REQUEST_APC		    1
#define IPI_REQUEST_DPC		    2

/* ipi.c ********************************************************************/

BOOLEAN STDCALL 
KiIpiServiceRoutine(IN PKTRAP_FRAME TrapFrame, 
		    IN struct _KEXCEPTION_FRAME* ExceptionFrame);

VOID  
KiIpiSendRequest(ULONG TargetSet, 
		 ULONG IpiRequest);

VOID  
KeIpiGenericCall(VOID (STDCALL *WorkerRoutine)(PVOID), 
		 PVOID Argument);

/* next file ***************************************************************/

typedef struct _KPROCESS_PROFILE
/*
 * List of the profile data structures associated with a process.
 */
{
  LIST_ENTRY ProfileListHead;
  LIST_ENTRY ListEntry;
  HANDLE Pid;
} KPROCESS_PROFILE, *PKPROCESS_PROFILE;

typedef struct _KPROFILE
/*
 * Describes a contiguous region of process memory that is being profiled.
 */
{
  CSHORT Type;
  CSHORT Name;

  /* Entry in the list of profile data structures for this process. */
  LIST_ENTRY ListEntry; 

  /* Base of the region being profiled. */
  PVOID Base;

  /* Size of the region being profiled. */
  ULONG Size;

  /* Shift of offsets from the region to buckets in the profiling buffer. */
  ULONG BucketShift;

  /* MDL which described the buffer that receives profiling data. */
  struct _MDL *BufferMdl;

  /* System alias for the profiling buffer. */
  PULONG Buffer;

  /* Size of the buffer for profiling data. */
  ULONG BufferSize;

  /* 
   * Mask of processors for which profiling data should be collected. 
   * Currently unused.
   */
  ULONG ProcessorMask;

  /* TRUE if profiling has been started for this region. */
  BOOLEAN Started;

  /* Pointer (and reference) to the process which is being profiled. */
  struct _EPROCESS *Process;
} KPROFILE, *PKPROFILE;

/* Cached modules from the loader block */
typedef enum _CACHED_MODULE_TYPE {
    AnsiCodepage,
    OemCodepage,
    UnicodeCasemap,
    SystemRegistry,
    HardwareRegistry,
    MaximumCachedModuleType,
} CACHED_MODULE_TYPE, *PCACHED_MODULE_TYPE;
extern PLOADER_MODULE CachedModules[MaximumCachedModuleType];

VOID STDCALL 
DbgBreakPointNoBugCheck(VOID);

VOID
STDCALL
KeProfileInterrupt(
    PKTRAP_FRAME TrapFrame
);

VOID
STDCALL
KeProfileInterruptWithSource(
	IN PKTRAP_FRAME   		TrapFrame,
	IN KPROFILE_SOURCE		Source
);

VOID KiAddProfileEventToProcess(PLIST_ENTRY ListHead, PVOID Eip);
VOID KiAddProfileEvent(KPROFILE_SOURCE Source, ULONG Eip);
VOID KiInsertProfileIntoProcess(PLIST_ENTRY ListHead, PKPROFILE Profile);
VOID KiInsertProfile(PKPROFILE Profile);
VOID KiRemoveProfile(PKPROFILE Profile);
VOID STDCALL KiDeleteProfile(PVOID ObjectBody);


VOID STDCALL KeUpdateSystemTime(PKTRAP_FRAME TrapFrame, KIRQL Irql);
VOID STDCALL KeUpdateRunTime(PKTRAP_FRAME TrapFrame, KIRQL Irql);

VOID STDCALL KiExpireTimers(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

KIRQL KeAcquireDispatcherDatabaseLock(VOID);
VOID KeAcquireDispatcherDatabaseLockAtDpcLevel(VOID);
VOID KeReleaseDispatcherDatabaseLock(KIRQL Irql);
VOID KeReleaseDispatcherDatabaseLockFromDpcLevel(VOID);

BOOLEAN KiDispatcherObjectWake(DISPATCHER_HEADER* hdr, KPRIORITY increment);
VOID STDCALL KeExpireTimers(PKDPC Apc,
			    PVOID Arg1,
			    PVOID Arg2,
			    PVOID Arg3);
VOID KeInitializeDispatcherHeader(DISPATCHER_HEADER* Header, ULONG Type,
				  ULONG Size, ULONG SignalState);
VOID KeDumpStackFrames(PULONG Frame);
BOOLEAN KiTestAlert(VOID);

BOOLEAN KiAbortWaitThread(struct _KTHREAD* Thread, NTSTATUS WaitStatus);

PULONG KeGetStackTopThread(struct _ETHREAD* Thread);
VOID KeContextToTrapFrame(PCONTEXT Context, PKTRAP_FRAME TrapFrame);
VOID STDCALL KiDeliverApc(KPROCESSOR_MODE PreviousMode,
                  PVOID Reserved,
                  PKTRAP_FRAME TrapFrame);
		  
VOID KiInitializeUserApc(IN PVOID Reserved,
			 IN PKTRAP_FRAME TrapFrame,
			 IN PKNORMAL_ROUTINE NormalRoutine,
			 IN PVOID NormalContext,
			 IN PVOID SystemArgument1,
			 IN PVOID SystemArgument2);

VOID STDCALL KiAttachProcess(struct _KTHREAD *Thread, struct _KPROCESS *Process, KIRQL ApcLock, struct _KAPC_STATE *SavedApcState);

VOID STDCALL KiSwapProcess(struct _KPROCESS *NewProcess, struct _KPROCESS *OldProcess);

BOOLEAN
STDCALL
KeTestAlertThread(IN KPROCESSOR_MODE AlertMode);

BOOLEAN STDCALL KeRemoveQueueApc (PKAPC Apc);
PLIST_ENTRY STDCALL KeRundownQueue(IN PKQUEUE Queue);

extern LARGE_INTEGER SystemBootTime;

/* INITIALIZATION FUNCTIONS *************************************************/

VOID KeInitExceptions(VOID);
VOID KeInitInterrupts(VOID);
VOID KeInitTimer(VOID);
VOID KeInitDpc(struct _KPRCB* Prcb);
VOID KeInitDispatcher(VOID);
VOID KeInitializeDispatcher(VOID);
VOID KiInitializeSystemClock(VOID);
VOID KiInitializeBugCheck(VOID);
VOID Phase1Initialization(PVOID Context);

VOID KeInit1(PCHAR CommandLine, PULONG LastKernelAddress);
VOID KeInit2(VOID);

BOOLEAN KiDeliverUserApc(PKTRAP_FRAME TrapFrame);

VOID
STDCALL
KiMoveApcState (PKAPC_STATE OldState,
		PKAPC_STATE NewState);

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
KeApplicationProcessorInit(VOID);
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
#define KEBUGCHECKWITHTF(a,b,c,d,e,f) DbgPrint("KeBugCheckWithTf at %s:%i\n",__FILE__,__LINE__), KeBugCheckWithTf(a,b,c,d,e,f)
VOID
KiDumpTrapFrame(PKTRAP_FRAME Tf, ULONG ExceptionNr, ULONG cr2);

VOID
STDCALL
KeFlushCurrentTb(VOID);

VOID
KiSetSystemTime(PLARGE_INTEGER NewSystemTime);

#endif /* not __ASM__ */

#define MAXIMUM_PROCESSORS      32

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_KE_H */

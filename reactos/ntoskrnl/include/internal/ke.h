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
#define IPI_REQUEST_FREEZE	    3

/* threadsch.c ********************************************************************/

/* Thread Scheduler Functions */

/* Readies a Thread for Execution. */
VOID 
STDCALL
KiDispatchThreadNoLock(ULONG NewThreadStatus);

/* Readies a Thread for Execution. */
VOID 
STDCALL
KiDispatchThread(ULONG NewThreadStatus);

/* Puts a Thread into a block state. */
VOID
STDCALL
KiBlockThread(PNTSTATUS Status, 
              UCHAR Alertable, 
              ULONG WaitMode,
              UCHAR WaitReason);
    
/* Removes a thread out of a block state. */        
VOID
STDCALL
KiUnblockThread(PKTHREAD Thread, 
                PNTSTATUS WaitStatus, 
                KPRIORITY Increment);

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

typedef struct _KPROFILE_SOURCE_OBJECT {
    KPROFILE_SOURCE Source;
    LIST_ENTRY ListEntry;
} KPROFILE_SOURCE_OBJECT, *PKPROFILE_SOURCE_OBJECT;

typedef struct _KPROFILE {
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY ListEntry;
    PVOID RegionStart;
    PVOID RegionEnd;
    ULONG BucketShift;
    PVOID Buffer;
    CSHORT Source;
    ULONG Affinity;
    BOOLEAN Active;
    struct _KPROCESS *Process;
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

STDCALL
VOID
KeInitializeProfile(struct _KPROFILE* Profile,
                    struct _KPROCESS* Process,
                    PVOID ImageBase,
                    ULONG ImageSize,
                    ULONG BucketSize,
                    KPROFILE_SOURCE ProfileSource,
                    KAFFINITY Affinity);

STDCALL
VOID
KeStartProfile(struct _KPROFILE* Profile,
               PVOID Buffer);

STDCALL
VOID
KeStopProfile(struct _KPROFILE* Profile);

STDCALL
ULONG
KeQueryIntervalProfile(KPROFILE_SOURCE ProfileSource);

STDCALL
VOID
KeSetIntervalProfile(KPROFILE_SOURCE ProfileSource,
                     ULONG Interval);

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


VOID STDCALL KeUpdateSystemTime(PKTRAP_FRAME TrapFrame, KIRQL Irql);
VOID STDCALL KeUpdateRunTime(PKTRAP_FRAME TrapFrame, KIRQL Irql);

VOID STDCALL KiExpireTimers(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

KIRQL inline FASTCALL KeAcquireDispatcherDatabaseLock(VOID);
VOID inline FASTCALL KeAcquireDispatcherDatabaseLockAtDpcLevel(VOID);
VOID inline FASTCALL KeReleaseDispatcherDatabaseLock(KIRQL Irql);
VOID inline FASTCALL KeReleaseDispatcherDatabaseLockFromDpcLevel(VOID);

VOID 
STDCALL
KeInitializeThread(struct _KPROCESS* Process, PKTHREAD Thread, BOOLEAN First);

VOID
STDCALL
KeRundownThread(VOID);

NTSTATUS KeReleaseThread(PKTHREAD Thread);

VOID
STDCALL
KeStackAttachProcess (
    IN struct _KPROCESS* Process,
    OUT PKAPC_STATE ApcState
    );

VOID
STDCALL
KeUnstackDetachProcess (
    IN PKAPC_STATE ApcState
    );

BOOLEAN KiDispatcherObjectWake(DISPATCHER_HEADER* hdr, KPRIORITY increment);
VOID STDCALL KeExpireTimers(PKDPC Apc,
			    PVOID Arg1,
			    PVOID Arg2,
			    PVOID Arg3);
VOID inline FASTCALL KeInitializeDispatcherHeader(DISPATCHER_HEADER* Header, ULONG Type,
 				  ULONG Size, ULONG SignalState);
VOID KeDumpStackFrames(PULONG Frame);
BOOLEAN KiTestAlert(VOID);

VOID
FASTCALL 
KiAbortWaitThread(PKTHREAD Thread, 
                  NTSTATUS WaitStatus,
                  KPRIORITY Increment);
                  
ULONG
STDCALL
KeForceResumeThread(IN PKTHREAD Thread);
 
BOOLEAN STDCALL KiInsertTimer(PKTIMER Timer, LARGE_INTEGER DueTime);

VOID inline FASTCALL KiSatisfyObjectWait(PDISPATCHER_HEADER Object, PKTHREAD Thread);

BOOLEAN inline FASTCALL KiIsObjectSignaled(PDISPATCHER_HEADER Object, PKTHREAD Thread);

VOID inline FASTCALL KiSatisifyMultipleObjectWaits(PKWAIT_BLOCK WaitBlock);

VOID FASTCALL KiWaitTest(PDISPATCHER_HEADER Object, KPRIORITY Increment);

PULONG KeGetStackTopThread(struct _ETHREAD* Thread);
VOID KeContextToTrapFrame(PCONTEXT Context, PKTRAP_FRAME TrapFrame);
VOID STDCALL KiDeliverApc(KPROCESSOR_MODE PreviousMode,
                  PVOID Reserved,
                  PKTRAP_FRAME TrapFrame);

LONG 
STDCALL 
KiInsertQueue(IN PKQUEUE Queue, 
              IN PLIST_ENTRY Entry, 
              BOOLEAN Head);
   
ULONG
STDCALL
KeSetProcess(struct _KPROCESS* Process, 
             KPRIORITY Increment);
             
                            
VOID STDCALL KeInitializeEventPair(PKEVENT_PAIR EventPair);

VOID STDCALL KiInitializeUserApc(IN PVOID Reserved,
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
VOID FASTCALL KiWakeQueue(IN PKQUEUE Queue);
PLIST_ENTRY STDCALL KeRundownQueue(IN PKQUEUE Queue);

extern LARGE_INTEGER SystemBootTime;

/* INITIALIZATION FUNCTIONS *************************************************/

VOID KeInitExceptions(VOID);
VOID KeInitInterrupts(VOID);
VOID KeInitTimer(VOID);
VOID KeInitDpc(struct _KPRCB* Prcb);
VOID KeInitDispatcher(VOID);
VOID inline FASTCALL KeInitializeDispatcher(VOID);
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

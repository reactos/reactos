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

#include "arch/ke.h"

/* INTERNAL KERNEL TYPES ****************************************************/

#ifndef __ASM__

#ifndef __USE_W32API

typedef struct _KPROCESS *PKPROCESS;
typedef struct _DISPATCHER_HEADER *PDISPATCHER_HEADER;

#endif /* __USE_W32API */

typedef struct _HARDWARE_PTE_X86 {
    ULONG Valid             : 1;
    ULONG Write             : 1;
    ULONG Owner             : 1;
    ULONG WriteThrough      : 1;
    ULONG CacheDisable      : 1;
    ULONG Accessed          : 1;
    ULONG Dirty             : 1;
    ULONG LargePage         : 1;
    ULONG Global            : 1;
    ULONG CopyOnWrite       : 1;
    ULONG Prototype         : 1;
    ULONG reserved          : 1;
    ULONG PageFrameNumber   : 20;
} HARDWARE_PTE_X86, *PHARDWARE_PTE_X86;

typedef struct _WOW64_PROCESS
{
  PVOID Wow64;
} WOW64_PROCESS, *PWOW64_PROCESS;

#include <pshpack1.h>

typedef struct _KTHREAD
{
   /* For waiting on thread exit */
   DISPATCHER_HEADER DispatcherHeader;    /* 00 */

   /* List of mutants owned by the thread */
   LIST_ENTRY        MutantListHead;      /* 10 */
   PVOID             InitialStack;        /* 18 */
   ULONG_PTR         StackLimit;          /* 1C */

   /* Pointer to the thread's environment block in user memory */
   struct _TEB       *Teb;                /* 20 */

   /* Pointer to the thread's TLS array */
   PVOID             TlsArray;            /* 24 */
   PVOID             KernelStack;         /* 28 */
   UCHAR             DebugActive;         /* 2C */

   /* Thread state (one of THREAD_STATE_xxx constants below) */
   UCHAR             State;               /* 2D */
   BOOLEAN           Alerted[2];          /* 2E */
   UCHAR             Iopl;                /* 30 */
   UCHAR             NpxState;            /* 31 */
   CHAR              Saturation;          /* 32 */
   CHAR              Priority;            /* 33 */
   KAPC_STATE        ApcState;            /* 34 */
   ULONG             ContextSwitches;     /* 4C */
   LONG              WaitStatus;          /* 50 */
   KIRQL             WaitIrql;            /* 54 */
   CHAR              WaitMode;            /* 55 */
   UCHAR             WaitNext;            /* 56 */
   UCHAR             WaitReason;          /* 57 */
   union {                                /* 58 */
      PKWAIT_BLOCK   WaitBlockList;       /* 58 */
      PKGATE         GateObject;          /* 58 */
   };                                     /* 58 */
   LIST_ENTRY        WaitListEntry;       /* 5C */
   ULONG             WaitTime;            /* 64 */
   CHAR              BasePriority;        /* 68 */
   UCHAR             DecrementCount;      /* 69 */
   UCHAR             PriorityDecrement;   /* 6A */
   CHAR              Quantum;             /* 6B */
   KWAIT_BLOCK       WaitBlock[4];        /* 6C */
   PVOID             LegoData;            /* CC */
   union {
          struct {
              USHORT KernelApcDisable;
              USHORT SpecialApcDisable;
          };
          ULONG      CombinedApcDisable;  /* D0 */
   };
   KAFFINITY         UserAffinity;        /* D4 */
   UCHAR             SystemAffinityActive;/* D8 */
   UCHAR             PowerState;          /* D9 */
   UCHAR             NpxIrql;             /* DA */
   UCHAR             Pad[1];              /* DB */
   PVOID             ServiceTable;        /* DC */
   PKQUEUE           Queue;               /* E0 */
   KSPIN_LOCK        ApcQueueLock;        /* E4 */
   KTIMER            Timer;               /* E8 */
   LIST_ENTRY        QueueListEntry;      /* 110 */
   KAFFINITY         Affinity;            /* 118 */
   UCHAR             Preempted;           /* 11C */
   UCHAR             ProcessReadyQueue;   /* 11D */
   UCHAR             KernelStackResident; /* 11E */
   UCHAR             NextProcessor;       /* 11F */
   PVOID             CallbackStack;       /* 120 */
   struct _W32THREAD *Win32Thread;        /* 124 */
   struct _KTRAP_FRAME *TrapFrame;        /* 128 */
   PKAPC_STATE       ApcStatePointer[2];  /* 12C */
   UCHAR             EnableStackSwap;     /* 134 */
   UCHAR             LargeStack;          /* 135 */
   UCHAR             ResourceIndex;       /* 136 */
   UCHAR             PreviousMode;        /* 137 */
   ULONG             KernelTime;          /* 138 */
   ULONG             UserTime;            /* 13C */
   KAPC_STATE        SavedApcState;       /* 140 */
   UCHAR             Alertable;           /* 158 */
   UCHAR             ApcStateIndex;       /* 159 */
   UCHAR             ApcQueueable;        /* 15A */
   UCHAR             AutoAlignment;       /* 15B */
   PVOID             StackBase;           /* 15C */
   KAPC              SuspendApc;          /* 160 */
   KSEMAPHORE        SuspendSemaphore;    /* 190 */
   LIST_ENTRY        ThreadListEntry;     /* 1A4 */
   CHAR              FreezeCount;         /* 1AC */
   UCHAR             SuspendCount;        /* 1AD */
   UCHAR             IdealProcessor;      /* 1AE */
   UCHAR             DisableBoost;        /* 1AF */
   UCHAR             QuantumReset;        /* 1B0 */
} KTHREAD;

#include <poppack.h>

typedef struct _KEXECUTE_OPTIONS
{
    UCHAR ExecuteDisable:1;
    UCHAR ExecuteEnable:1;
    UCHAR DisableThunkEmulation:1;
    UCHAR Permanent:1;
    UCHAR ExecuteDispatchEnable:1;
    UCHAR ImageDispatchEnable:1;
    UCHAR Spare:2;
} KEXECUTE_OPTIONS, *PKEXECUTE_OPTIONS;

/*
 * NAME:           KPROCESS
 * DESCRIPTION:    Internal Kernel Process Structure.
 * PORTABILITY:    Architecture Dependent.
 * KERNEL VERSION: 5.2
 * DOCUMENTATION:  http://reactos.com/wiki/index.php/KPROCESS
 */
typedef struct _KPROCESS
{
    DISPATCHER_HEADER     Header;                    /* 000 */
    LIST_ENTRY            ProfileListHead;           /* 010 */
    PHYSICAL_ADDRESS      DirectoryTableBase;        /* 018 */
    KGDTENTRY             LdtDescriptor;             /* 020 */
    KIDTENTRY             Int21Descriptor;           /* 028 */
    USHORT                IopmOffset;                /* 030 */
    UCHAR                 Iopl;                      /* 032 */
    UCHAR                 Unused;                    /* 033 */
    ULONG                 ActiveProcessors;          /* 034 */
    ULONG                 KernelTime;                /* 038 */
    ULONG                 UserTime;                  /* 03C */
    LIST_ENTRY            ReadyListHead;             /* 040 */
    LIST_ENTRY            SwapListEntry;             /* 048 */
    PVOID                 VdmTrapcHandler;           /* 04C */
    LIST_ENTRY            ThreadListHead;            /* 050 */
    KSPIN_LOCK            ProcessLock;               /* 058 */
    KAFFINITY             Affinity;                  /* 05C */
    union {
        struct {
            ULONG         AutoAlignment:1;           /* 060.0 */
            ULONG         DisableBoost:1;            /* 060.1 */
            ULONG         DisableQuantum:1;          /* 060.2 */
            ULONG         ReservedFlags:29;          /* 060.3 */
        };
        ULONG             ProcessFlags;              /* 060 */
    };
    CHAR                  BasePriority;              /* 064 */
    CHAR                  QuantumReset;              /* 065 */
    UCHAR                 State;                     /* 066 */
    UCHAR                 ThreadSeed;                /* 067 */
    UCHAR                 PowerState;                /* 068 */
    UCHAR                 IdealNode;                 /* 069 */
    UCHAR                 Visited;                   /* 06A */
    KEXECUTE_OPTIONS      Flags;                     /* 06B */
    ULONG                 StackCount;                /* 06C */
    LIST_ENTRY            ProcessListEntry;          /* 070 */
} KPROCESS;

/* INTERNAL KERNEL FUNCTIONS ************************************************/

struct _KIRQ_TRAPFRAME;
struct _KPCR;
struct _KPRCB;
struct _KEXCEPTION_FRAME;

#define IPI_REQUEST_FUNCTIONCALL    0
#define IPI_REQUEST_APC		    1
#define IPI_REQUEST_DPC		    2
#define IPI_REQUEST_FREEZE	    3

#ifndef __USE_W32API
typedef enum _KTHREAD_STATE {
    Initialized,
    Ready,
    Running,
    Standby,
    Terminated,
    Waiting,
    Transition,
    DeferredReady,
} THREAD_STATE, *PTHREAD_STATE;
#endif

/* MACROS *************************************************************************/

#define KeEnterCriticalRegion(X) \
{ \
    PKTHREAD _Thread = KeGetCurrentThread(); \
    if (_Thread) _Thread->KernelApcDisable--; \
}

#define KeLeaveCriticalRegion(X) \
{ \
    PKTHREAD _Thread = KeGetCurrentThread(); \
    if((_Thread) && (++_Thread->KernelApcDisable == 0)) \
    { \
        if (!IsListEmpty(&_Thread->ApcState.ApcListHead[KernelMode])) \
        { \
            KiKernelApcDeliveryCheck(); \
        } \
    } \
}

#ifndef __USE_W32API
#define KeGetCurrentProcessorNumber() (KeGetCurrentKPCR()->Number)
#endif

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

NTSTATUS
STDCALL
KeSuspendThread(PKTHREAD Thread);

NTSTATUS
FASTCALL
KiSwapContext(PKTHREAD NewThread);

/* gmutex.c ********************************************************************/

VOID
FASTCALL
KiAcquireGuardedMutexContented(PKGUARDED_MUTEX GuardedMutex);

/* gate.c **********************************************************************/

VOID
FASTCALL
KeInitializeGate(PKGATE Gate);

VOID
FASTCALL
KeSignalGateBoostPriority(PKGATE Gate);

VOID
FASTCALL
KeWaitForGate(PKGATE Gate,
              KWAIT_REASON WaitReason,
              KPROCESSOR_MODE WaitMode);

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

BOOLEAN
STDCALL
KiRosPrintAddress(PVOID Address);

VOID STDCALL KeUpdateSystemTime(PKTRAP_FRAME TrapFrame, KIRQL Irql);
VOID STDCALL KeUpdateRunTime(PKTRAP_FRAME TrapFrame, KIRQL Irql);

VOID STDCALL KiExpireTimers(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

KIRQL inline FASTCALL KeAcquireDispatcherDatabaseLock(VOID);
VOID inline FASTCALL KeAcquireDispatcherDatabaseLockAtDpcLevel(VOID);
VOID inline FASTCALL KeReleaseDispatcherDatabaseLock(KIRQL Irql);
VOID inline FASTCALL KeReleaseDispatcherDatabaseLockFromDpcLevel(VOID);

VOID
STDCALL
KeInitializeThread(struct _KPROCESS* Process,
                   PKTHREAD Thread,
                   PKSYSTEM_ROUTINE SystemRoutine,
                   PKSTART_ROUTINE StartRoutine,
                   PVOID StartContext,
                   PCONTEXT Context,
                   PVOID Teb,
                   PVOID KernelStack);

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

VOID
STDCALL
KeInitializeProcess(struct _KPROCESS *Process,
                    KPRIORITY Priority,
                    KAFFINITY Affinity,
                    LARGE_INTEGER DirectoryTableBase);

ULONG
STDCALL
KeForceResumeThread(IN PKTHREAD Thread);

BOOLEAN STDCALL KiInsertTimer(PKTIMER Timer, LARGE_INTEGER DueTime);

VOID inline FASTCALL KiSatisfyObjectWait(PDISPATCHER_HEADER Object, PKTHREAD Thread);

BOOLEAN inline FASTCALL KiIsObjectSignaled(PDISPATCHER_HEADER Object, PKTHREAD Thread);

VOID inline FASTCALL KiSatisifyMultipleObjectWaits(PKWAIT_BLOCK WaitBlock);

VOID FASTCALL KiWaitTest(PDISPATCHER_HEADER Object, KPRIORITY Increment);

PULONG KeGetStackTopThread(struct _ETHREAD* Thread);
BOOLEAN STDCALL KeContextToTrapFrame(PCONTEXT Context, PKTRAP_FRAME TrapFrame);
VOID STDCALL KiDeliverApc(KPROCESSOR_MODE PreviousMode,
                  PVOID Reserved,
                  PKTRAP_FRAME TrapFrame);
VOID
STDCALL
KiKernelApcDeliveryCheck(VOID);
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

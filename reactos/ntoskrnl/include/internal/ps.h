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
/*
 * FILE:            ntoskrnl/ke/kthread.c
 * PURPOSE:         Process manager definitions
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

#ifndef __INCLUDE_INTERNAL_PS_H
#define __INCLUDE_INTERNAL_PS_H

/*
 * Defines for accessing KPCR and KTHREAD structure members
 */
#define KTHREAD_KERNEL_STACK      0x28
#define KTHREAD_PREVIOUS_MODE     0x137
#define KTHREAD_TRAP_FRAME        0x128

#define KPCR_BASE                 0xFFDFF000

#define KPCR_EXCEPTION_LIST       0x0
#define KPCR_CURRENT_THREAD       0x124	

#ifndef __ASM__

#include <internal/hal.h>
#include <internal/mm.h>

struct _KTHREAD;
struct _KTRAPFRAME;

/*
 * Processor Control Region
 */
typedef struct _KPCR
{
   PVOID ExceptionList;               /* 00 */
   PVOID StackBase;                   /* 04 */
   PVOID StackLimit;                  /* 08 */
   PVOID SubSystemTib;                /* 0C */
   PVOID Reserved1;                   /* 10 */
   PVOID ArbitraryUserPointer;        /* 14 */
   struct _KPCR* Self;                /* 18 */
   UCHAR Reserved2[0x108];            /* 1C */
   struct _KTHREAD* CurrentThread;    /* 124 */
} KPCR, *PKPCR;

#define CURRENT_KPCR ((PKPCR)KPCR_BASE)

extern HANDLE SystemProcessHandle;

typedef struct _KAPC_STATE
{
   LIST_ENTRY ApcListHead[2];
   struct _KPROCESS* Process;
   UCHAR KernelApcInProgress;
   UCHAR KernelApcPending;
   USHORT UserApcPending;
} __attribute__((packed)) KAPC_STATE, *PKAPC_STATE;

typedef struct _KTHREAD
{
   DISPATCHER_HEADER DispatcherHeader;    /* 00 */
   LIST_ENTRY        MutantListHead;      /* 10 */
   PVOID             InitialStack;        /* 18 */
   ULONG             StackLimit;          /* 1C */
   NT_TEB*           Teb;                 /* 20 */
   PVOID             TlsArray;            /* 24 */
   PVOID             KernelStack;         /* 28 */
   UCHAR             DebugActive;         /* 2C */
   UCHAR             State;               /* 2D */
   UCHAR             Alerted[2];          /* 2E */
   UCHAR             Iopl;                /* 30 */
   UCHAR             NpxState;            /* 31 */
   UCHAR             Saturation;          /* 32 */
   CHAR              Priority;            /* 33 */
   KAPC_STATE        ApcState;            /* 34 */
   ULONG             ContextSwitches;     /* 4C */
   ULONG             WaitStatus;          /* 50 */
   KIRQL             WaitIrql;            /* 54 */
   UCHAR             WaitMode;            /* 55 */
   UCHAR             WaitNext;            /* 56 */
   UCHAR             WaitReason;          /* 57 */
   PKWAIT_BLOCK      WaitBlockList;       /* 58 */
   LIST_ENTRY        WaitListEntry;       /* 5C */
   ULONG             WaitTime;            /* 64 */
   CHAR              BasePriority;        /* 68 */
   UCHAR             DecrementCount;      /* 69 */
   UCHAR             PriorityDecrement;   /* 6A */
   UCHAR             Quantum;             /* 6B */
   KWAIT_BLOCK       WaitBlock[4];        /* 6C */
   PVOID             LegoData;            /* CC */
   LONG              KernelApcDisable;    /* D0 */
   KAFFINITY         UserAffinity;        /* D4 */
   UCHAR             SystemAffinityActive;/* D8 */
   UCHAR             Pad[7];              /* D9 */
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
   BOOL              Win32Thread;         /* 124 */
   struct _KTRAP_FRAME*      TrapFrame;   /* 128 */
   PVOID             ApcStatePointer[2];  /* 12C */
   UCHAR             EnableStackSwap;     /* 134 */
   UCHAR             LargeStack;          /* 135 */
   UCHAR             ResourceIndex;       /* 136 */
   UCHAR             PreviousMode;        /* 137 */
   TIME              KernelTime;          /* 138 */
   TIME              UserTime;            /* 13C */
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
   
   /*
    * Below here are thread structure members that are specific to ReactOS
    */
   
   /* Added by Phillip Susi for list of threads in a process */
   LIST_ENTRY        ProcessThreadListEntry;        

   /* Provisionally added by David Welch */
   hal_thread_state                   Context;
   /* Added by Phillip Susi for internal KeAddThreadTimeout() implementation */
   KDPC              TimerDpc;		       
} __attribute__((packed)) KTHREAD, *PKTHREAD;

// According to documentation the stack should have a commited [ 1 page ] and
// a reserved part [ 1 M ] but can be specified otherwise in the image file.







// TopLevelIrp can be one of the following values:
// FIXME I belong somewhere else

#define 	FSRTL_FSP_TOP_LEVEL_IRP			(0x01)
#define 	FSRTL_CACHE_TOP_LEVEL_IRP		(0x02)
#define 	FSRTL_MOD_WRITE_TOP_LEVEL_IRP		(0x03)
#define		FSRTL_FAST_IO_TOP_LEVEL_IRP		(0x04)
#define		FSRTL_MAX_TOP_LEVEL_IRP_FLAG		(0x04)

typedef struct _TOP_LEVEL_IRP
{
	PIRP TopLevelIrp;
	ULONG TopLevelIrpConst;
} TOP_LEVEL_IRP;

typedef struct
{
   PACCESS_TOKEN Token;                              // 0x0
   UCHAR Unknown1;                                   // 0x4
   UCHAR Unknown2;                                   // 0x5
   UCHAR Pad[2];                                     // 0x6
   SECURITY_IMPERSONATION_LEVEL Level;               // 0x8
} PS_IMPERSONATION_INFO, *PPS_IMPERSONATION_INFO;

struct _WIN32THREADDATA;

typedef struct _ETHREAD 
{
   KTHREAD Tcb;
   TIME	CreateTime;
   TIME	ExitTime;
   NTSTATUS ExitStatus;
   LIST_ENTRY PostBlockList;
   LIST_ENTRY TerminationPortList;  
   KSPIN_LOCK ActiveTimerListLock;
   PVOID ActiveTimerListHead;
   CLIENT_ID Cid;
   PLARGE_INTEGER LpcReplySemaphore;
   PVOID LpcReplyMessage;
   PLARGE_INTEGER LpcReplyMessageId;
   PPS_IMPERSONATION_INFO ImpersonationInfo;
   LIST_ENTRY IrpList;
   TOP_LEVEL_IRP TopLevelIrp;
   ULONG ReadClusterSize;
   UCHAR ForwardClusterOnly;
   UCHAR DisablePageFaultClustering;
   UCHAR DeadThread;
   UCHAR HasTerminated;
   ACCESS_MASK GrantedAccess;
   struct _EPROCESS* ThreadsProcess;
   PKSTART_ROUTINE StartAddress;
   LPTHREAD_START_ROUTINE Win32StartAddress; 
   UCHAR LpcExitThreadCalled;
   UCHAR HardErrorsAreDisabled;
   UCHAR LpcReceivedMsgIdValid;
   UCHAR ActiveImpersonationInfo;
   ULONG PerformanceCountHigh;

   /*
    * Added by David Welch (welch@cwcom.net)
    */
   struct _EPROCESS* OldProcess;
   struct _WIN32THREADDATA *Win32ThreadData; // Pointer to win32 private thread data

} ETHREAD, *PETHREAD;


typedef struct _KPROCESS 
{
   DISPATCHER_HEADER 	DispatcherHeader;
   PVOID		PageTableDirectory; // FIXME: I should point to a PTD
   TIME			ElapsedTime;
   TIME			KernelTime;
   TIME			UserTime;
   LIST_ENTRY		InMemoryList;  
   LIST_ENTRY		SwappedOutList;   	
   KSPIN_LOCK		SpinLock;
   KAFFINITY		Affinity;
   ULONG		StackCount;
   KPRIORITY		BasePriority;
   ULONG		DefaultThreadQuantum;
   UCHAR		ProcessState;
   ULONG		ThreadSeed;
   UCHAR		DisableBoost;
} KPROCESS, *PKPROCESS;

struct _WIN32PROCESSDATA;

typedef struct _EPROCESS
{
   KPROCESS Pcb;
   NTSTATUS ExitStatus;
   KEVENT LockEvent;
   ULONG LockCount;
   TIME CreateTime;
   TIME ExitTime;
   PVOID LockOwner;
   ULONG UniqueProcessId;
   LIST_ENTRY ActiveProcessLinks;
   ULONG QuotaPeakPoolUsage[2];
   ULONG QuotaPoolUsage[2];
   ULONG PagefileUsage;
   ULONG CommitCharge;
   ULONG PeakPagefileUsage;
   ULONG PeakVirtualUsage;
   LARGE_INTEGER VirtualSize;
   PVOID Vm;                // Actually 48 bytes
   PVOID LastProtoPteFault;
   struct _EPORT* DebugPort;
   struct _EPORT* ExceptionPort;
   PVOID ObjectTable;
   PVOID Token;
   KMUTEX WorkingSetLock;
   PVOID WorkingSetPage;
   UCHAR ProcessOutswapEnabled;
   UCHAR ProcessOutswapped;
   UCHAR AddressSpaceInitialized;
   UCHAR AddressSpaceDeleted;
   KMUTEX AddressCreationLock;
   PVOID ForkInProgress;
   PVOID VmOperation;
   PKEVENT VmOperationEvent;
   PVOID PageDirectoryPte;
   LARGE_INTEGER LastFaultCount;
   PVOID VadRoot;
   PVOID VadHint;
   PVOID CloneRoot;
   ULONG NumberOfPrivatePages;
   ULONG NumberOfLockedPages;
   UCHAR ForkWasSuccessFul;
   UCHAR ExitProcessCalled;
   UCHAR CreateProcessReported;
   HANDLE SectionHandle;
   PPEB Peb;
   PVOID SectionBaseAddress;
   PVOID QuotaBlock;
   NTSTATUS LastThreadExitStatus;
   LARGE_INTEGER WorkingSetWatch;         //
   ULONG InheritedFromUniqueProcessId;
   ACCESS_MASK GrantedAccess;
   ULONG DefaultHardErrorProcessing;
   PVOID LdtInformation;
   ULONG VadFreeHint;
   PVOID VdmObjects;
   KMUTANT ProcessMutant;
   CHAR ImageFileName[16];
   LARGE_INTEGER VmTrimFaultValue;
   struct _WIN32PROCESSDATA *Win32Process;
   
   /*
    * Added by David Welch (welch@mcmail.com)
    */
   MADDRESS_SPACE       AddressSpace;
   HANDLE_TABLE         HandleTable;
   LIST_ENTRY           ProcessListEntry;
   
   /*
    * Added by Philip Susi for list of threads in process
    */
   LIST_ENTRY           ThreadListHead;        
} EPROCESS, *PEPROCESS;

#define PROCESS_STATE_TERMINATED (1)
#define PROCESS_STATE_ACTIVE     (2)

VOID PiInitProcessManager(VOID);
VOID PiShutdownProcessManager(VOID);
VOID PsInitThreadManagment(VOID);
VOID PsInitProcessManagment(VOID);
VOID PsInitIdleThread(VOID);
VOID PsDispatchThread(ULONG NewThreadStatus);
VOID PsDispatchThreadNoLock(ULONG NewThreadStatus);
VOID PiTerminateProcessThreads(PEPROCESS Process, NTSTATUS ExitStatus);
VOID PsTerminateOtherThread(PETHREAD Thread, NTSTATUS ExitStatus);
VOID PsReleaseThread(PETHREAD Thread);
VOID PsBeginThread(PKSTART_ROUTINE StartRoutine, PVOID StartContext);
VOID PsBeginThreadWithContextInternal(VOID);
VOID PiKillMostProcesses(VOID);
NTSTATUS STDCALL PiTerminateProcess(PEPROCESS Process, NTSTATUS ExitStatus);
ULONG PsUnfreezeThread(PETHREAD Thread, PNTSTATUS WaitStatus);
ULONG PsFreezeThread(PETHREAD Thread, PNTSTATUS WaitStatus,
		     UCHAR Alertable, ULONG WaitMode);
VOID PiInitApcManagement(VOID);
VOID PiDeleteThread(PVOID ObjectBody);
VOID PiCloseThread(PVOID ObjectBody, ULONG HandleCount);
VOID PsReapThreads(VOID);
NTSTATUS PsInitializeThread(HANDLE ProcessHandle,
			    PETHREAD* ThreadPtr,
			    PHANDLE ThreadHandle,
			    ACCESS_MASK DesiredAccess,
			    POBJECT_ATTRIBUTES ObjectAttributes);

PACCESS_TOKEN PsReferenceEffectiveToken(PETHREAD Thread,
					PTOKEN_TYPE TokenType,
					PUCHAR b,
					PSECURITY_IMPERSONATION_LEVEL Level);

NTSTATUS PsOpenTokenOfProcess(HANDLE ProcessHandle,
			      PACCESS_TOKEN* Token);

ULONG PsSuspendThread(PETHREAD Thread,
		      PNTSTATUS WaitStatus,
		      UCHAR Alertable,
		      ULONG WaitMode);
ULONG PsResumeThread(PETHREAD Thread,
		     PNTSTATUS WaitStatus);


#define THREAD_STATE_INVALID      (0)
#define THREAD_STATE_RUNNABLE     (1)
#define THREAD_STATE_RUNNING      (2)
#define THREAD_STATE_SUSPENDED    (3)
#define THREAD_STATE_FROZEN       (4)
#define THREAD_STATE_TERMINATED_1 (5)
#define THREAD_STATE_TERMINATED_2 (6)
#define THREAD_STATE_MAX          (7)


/*
 * Internal thread priorities, added by Phillip Susi
 * TODO: rebalence these to make use of all priorities... the ones above 16 can not all be used right now
 */

#define PROCESS_PRIO_IDLE			3
#define PROCESS_PRIO_NORMAL			8
#define PROCESS_PRIO_HIGH			13
#define PROCESS_PRIO_RT				18

/*
 * Functions the HAL must provide
 */

VOID 
KeInitializeThread(PKPROCESS Process, PKTHREAD Thread);

void HalInitFirstTask(PETHREAD thread);
NTSTATUS HalInitTask(PETHREAD thread, PKSTART_ROUTINE fn, PVOID StartContext);
void HalTaskSwitch(PKTHREAD thread);
NTSTATUS HalInitTaskWithContext(PETHREAD Thread, PCONTEXT Context);
NTSTATUS HalReleaseTask(PETHREAD Thread);
VOID PiDeleteProcess(PVOID ObjectBody);
VOID PsReapThreads(VOID);
VOID PsUnfreezeOtherThread(PETHREAD Thread);
VOID PsFreezeOtherThread(PETHREAD Thread);
VOID PsFreezeProcessThreads(PEPROCESS Process);
VOID PsUnfreezeProcessThreads(PEPROCESS Process);
PEPROCESS PsGetNextProcess(PEPROCESS OldProcess);

#endif /* ASSEMBLER */

#endif /* __INCLUDE_INTERNAL_PS_H */

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
/* $Id$
 *
 * FILE:            ntoskrnl/ke/kthread.c
 * PURPOSE:         Process manager definitions
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

#ifndef __INCLUDE_INTERNAL_PS_H
#define __INCLUDE_INTERNAL_PS_H

#ifndef __ASM__

/* Forward declarations. */
struct _KTHREAD;
struct _KTRAPFRAME;
struct _EJOB;

#endif /* __ASM__ */

#include <internal/arch/ps.h>

#ifndef __ASM__

#include <internal/mm.h>
#include <napi/teb.h>

#ifndef KeGetCurrentProcessorNumber
#define KeGetCurrentProcessorNumber() (KeGetCurrentKPCR()->ProcessorNumber)
#endif

extern LCID PsDefaultThreadLocaleId;
extern LCID PsDefaultSystemLocaleId;

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
   PTEB              Teb;                 /* 20 */
   
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
   PKWAIT_BLOCK      WaitBlockList;       /* 58 */
   LIST_ENTRY        WaitListEntry;       /* 5C */
   ULONG             WaitTime;            /* 64 */
   CHAR              BasePriority;        /* 68 */
   UCHAR             DecrementCount;      /* 69 */
   UCHAR             PriorityDecrement;   /* 6A */
   CHAR              Quantum;             /* 6B */
   KWAIT_BLOCK       WaitBlock[4];        /* 6C */
   PVOID             LegoData;            /* CC */
   ULONG             KernelApcDisable;    /* D0 */
   KAFFINITY         UserAffinity;        /* D4 */
   UCHAR             SystemAffinityActive;/* D8 */
   UCHAR             PowerState;          /* D9 */
   UCHAR             NpxIrql;             /* DA */
   UCHAR             Pad[1];              /* DB */
   SSDT_ENTRY        *ServiceTable;       /* DC */
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
} KTHREAD;

#include <poppack.h>

/* Top level irp definitions. */
#define	FSRTL_FSP_TOP_LEVEL_IRP			(0x01)
#define	FSRTL_CACHE_TOP_LEVEL_IRP		(0x02)
#define	FSRTL_MOD_WRITE_TOP_LEVEL_IRP		(0x03)
#define	FSRTL_FAST_IO_TOP_LEVEL_IRP		(0x04)
#define	FSRTL_MAX_TOP_LEVEL_IRP_FLAG		(0x04)

#ifndef __USE_W32API
typedef struct
{
    PACCESS_TOKEN                   Token;
    BOOLEAN                         CopyOnOpen;
    BOOLEAN                         EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL    ImpersonationLevel;
} PS_IMPERSONATION_INFORMATION, *PPS_IMPERSONATION_INFORMATION;
#endif

#include <pshpack1.h>

typedef struct _ETHREAD
{
  KTHREAD Tcb;
  union {
  	LARGE_INTEGER CreateTime;
  	UCHAR NestedFaultCount:2;
  	UCHAR ApcNeeded:1;
  };
  LARGE_INTEGER ExitTime;
  LIST_ENTRY LpcReplyChain;
  NTSTATUS ExitStatus;
  PVOID OfsChain;
  LIST_ENTRY PostBlockList;
  LIST_ENTRY TerminationPortList;
  KSPIN_LOCK ActiveTimerListLock;
  LIST_ENTRY ActiveTimerListHead;
  CLIENT_ID Cid;
  KSEMAPHORE LpcReplySemaphore;
  PVOID LpcReplyMessage;
  ULONG LpcReplyMessageId;
  ULONG PerformanceCountLow;
  PPS_IMPERSONATION_INFORMATION ImpersonationInfo;
  LIST_ENTRY IrpList;
  PIRP TopLevelIrp;
  PDEVICE_OBJECT DeviceToVerify;
  ULONG ReadClusterSize;
  UCHAR ForwardClusterOnly;
  UCHAR DisablePageFaultClustering;
  UCHAR DeadThread;
  UCHAR HideFromDebugger;
  ULONG HasTerminated;
#ifdef _ENABLE_THRDEVTPAIR
  PVOID EventPair;
#endif /* _ENABLE_THRDEVTPAIR */
  ACCESS_MASK GrantedAccess;
  struct _EPROCESS *ThreadsProcess;
  PKSTART_ROUTINE StartAddress;
  LPTHREAD_START_ROUTINE Win32StartAddress;
  ULONG LpcReceivedMessageId;
  UCHAR LpcExitThreadCalled;
  UCHAR HardErrorsAreDisabled;
  UCHAR LpcReceivedMsgIdValid;
  UCHAR ActiveImpersonationInfo;
  ULONG PerformanceCountHigh;
  LIST_ENTRY ThreadListEntry;
} ETHREAD;

#include <poppack.h>


#ifndef __USE_W32API

typedef struct _ETHREAD *PETHREAD;

#endif /* __USE_W32API */


typedef struct _KPROCESS 
{
  /* So it's possible to wait for the process to terminate */
  DISPATCHER_HEADER 	DispatcherHeader;             /* 000 */
  /* 
   * Presumably a list of profile objects associated with this process,
   * currently unused.
   */
  LIST_ENTRY            ProfileListHead;              /* 010 */
  /*
   * We use the first member of this array to hold the physical address of
   * the page directory for this process.
   */
  PHYSICAL_ADDRESS      DirectoryTableBase;           /* 018 */
  /*
   * Presumably a descriptor for the process's LDT, currently unused.
   */
  ULONG                 LdtDescriptor[2];             /* 020 */
  /*
   * Virtual Dos Machine flag.
   */
  ULONG                 NtVdmFlag;                    /* 028 */
  ULONG                 VdmUnused;                    /* 02C */
  /* Is the i/o permission map enabled for the process. */
  USHORT                IopmOffset;                   /* 030 */
  /* 
   * Presumably I/O privilege level to be used for this process, currently
   * unused.
   */
  UCHAR                 Iopl;                         /* 032 */
  /* Set if this process is a virtual dos machine? */
  UCHAR                 VdmFlag;                      /* 033 */
  /* Bitmask of the processors being used by this process's threads? */
  ULONG                 ActiveProcessors;             /* 034 */
  /* Aggregate of the time this process's threads have spent in kernel mode? */
  ULONG                 KernelTime;                   /* 038 */
  /* Aggregate of the time this process's threads have spent in user mode? */
  ULONG                 UserTime;                     /* 03C */
  /* List of this process's threads that are ready for execution? */
  LIST_ENTRY            ReadyListHead;                /* 040 */
  /* List of this process's threads that have their stacks swapped out? */
  LIST_ENTRY            SwapListEntry;                /* 048 */
  /* List of this process's threads? */
  LIST_ENTRY            ThreadListHead;               /* 050 */
  /* Maybe a lock for this data structure, the type is assumed. */
  KSPIN_LOCK            ProcessLock;                  /* 058 */
  /* Default affinity mask for this process's threads? */
  ULONG                 Affinity;                     /* 05C */
  /* Count of the stacks allocated for this process's threads? */
  USHORT                StackCount;                   /* 060 */
  /* Base priority for this process's threads? */
  KPRIORITY             BasePriority;                 /* 062 */
  /* Default quantum for this process's threads */
  UCHAR		        ThreadQuantum;                /* 063 */
  /* Unknown. */
  UCHAR                 AutoAlignment;                /* 064 */
  /* Process execution state, currently either active or terminated. */
  UCHAR		        State;                        /* 065 */
  /* Seed for generating thread ids for this process's threads? */
  UCHAR		        ThreadSeed;                   /* 066 */
  /* Disable priority boosts? */
  UCHAR		        DisableBoost;                 /* 067 */
} KPROCESS;

#ifndef __USE_W32API

typedef struct _KPROCESS *PKPROCESS;

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

#endif /* __USE_W32API */

struct _EPROCESS
{
  /* Microkernel specific process state. */
  KPROCESS              Pcb;                          /* 000 */
  /* Exit status of the process. */
  NTSTATUS              ExitStatus;                   /* 068 */
  /* Unknown. */
  KEVENT                LockEvent;                    /* 06C */
  /* Unknown. */
  ULONG                 LockCount;                    /* 07C */

  /* Time of process creation. */
  LARGE_INTEGER         CreateTime;                   /* 080 */

  /* Time of process exit. */
  LARGE_INTEGER         ExitTime;                     /* 088 */
  /* Unknown. */
  PKTHREAD              LockOwner;                    /* 090 */
  /* Process id. */
  HANDLE                UniqueProcessId;              /* 094 */
  /* Unknown. */
  LIST_ENTRY            ActiveProcessLinks;           /* 098 */
  /* Unknown. */
  ULONG                 QuotaPeakPoolUsage[2];        /* 0A0 */
  /* Unknown. */
  ULONG                 QuotaPoolUsage[2];            /* 0A8 */
  /* Unknown. */
  ULONG                 PagefileUsage;                /* 0B0 */
  /* Unknown. */
  ULONG                 CommitCharge;                 /* 0B4 */
  /* Unknown. */
  ULONG                 PeakPagefileUsage;            /* 0B8 */
  /* Unknown. */
  ULONG                 PeakVirtualSize;              /* 0BC */
  /* Unknown. */
  LARGE_INTEGER         VirtualSize;                  /* 0C0 */

  MMSUPPORT             Vm;
  LIST_ENTRY            SessionProcessLinks;
  struct _EPORT         *DebugPort;
  struct _EPORT         *ExceptionPort;
  PHANDLE_TABLE         ObjectTable;
  PVOID                 Token;
  FAST_MUTEX            WorkingSetLock;
  ULONG                 WorkingSetPage;
  UCHAR                 ProcessOutswapEnabled;
  UCHAR                 ProcessOutswapped;
  UCHAR                 AddressSpaceInitialized;
  UCHAR                 AddressSpaceDeleted;
  FAST_MUTEX            AddressCreationLock;
  KSPIN_LOCK            HyperSpaceLock;
  PETHREAD              ForkInProgress;
  USHORT                VmOperation;
  UCHAR                 ForkWasSuccessful;
  UCHAR                 MmAgressiveWsTrimMask;
  PKEVENT               VmOperationEvent;
  PVOID                 PaeTop;
  ULONG                 LastFaultCount;
  ULONG                 ModifiedPageCount;
  PVOID                 VadRoot;
  PVOID                 VadHint;
  PVOID                 CloneRoot;
  ULONG                 NumberOfPrivatePages;
  ULONG                 NumberOfLockedPages;
  USHORT                NextPageColor;
  UCHAR                 ExitProcessCalled;
  UCHAR                 CreateProcessReported;
  HANDLE                SectionHandle;
  PPEB                  Peb;
  PVOID                 SectionBaseAddress;
  PEPROCESS_QUOTA_BLOCK QuotaBlock;
  NTSTATUS              LastThreadExitStatus;
  PPAGEFAULT_HISTORY    WorkingSetWatch;
  HANDLE                Win32WindowStation;
  HANDLE                InheritedFromUniqueProcessId;
  ULONG                 GrantedAccess;
  ULONG                 DefaultHardErrorProcessing;
  PVOID                 LdtInformation;
  PVOID                 VadFreeHint;
  PVOID                 VdmObjects;
  PVOID                 DeviceObjects;
  ULONG                 SessionId;
  LIST_ENTRY            PhysicalVadList;
  HARDWARE_PTE_X86      PageDirectoryPte;
  ULONGLONG             Filler;
  ULONG                 PaePageDirectoryPage;
  CHAR                  ImageFileName[16];
  ULONG                 VmTrimFaultValue;
  UCHAR                 SetTimerResolution;
  UCHAR                 PriorityClass;
  UCHAR                 SubSystemMinorVersion;
  UCHAR                 SubSystemMajorVersion;
  USHORT                SubSystemVersion;
  struct _W32PROCESS    *Win32Process;
  struct _EJOB          *Job;
  ULONG                 JobStatus;
  LIST_ENTRY            JobLinks;
  PVOID                 LockedPagesList;
  struct _EPORT         *SecurityPort;
  PWOW64_PROCESS        Wow64;
  LARGE_INTEGER         ReadOperationCount;
  LARGE_INTEGER         WriteOperationCount;
  LARGE_INTEGER         OtherOperationCount;
  LARGE_INTEGER         ReadTransferCount;
  LARGE_INTEGER         WriteTransferCount;
  LARGE_INTEGER         OtherTransferCount;
  ULONG                 CommitChargeLimit;
  ULONG                 CommitChargePeak;
  LIST_ENTRY            ThreadListHead;
  PRTL_BITMAP           VadPhysicalPagesBitMap;
  ULONG                 VadPhysicalPages;
  KSPIN_LOCK            AweLock;
  ULONG                 Cookie;

  /*
   * FIXME - ReactOS specified - remove the following fields ASAP!!!
   */
  MADDRESS_SPACE        AddressSpace;
  LIST_ENTRY            ProcessListEntry;
  FAST_MUTEX            TebLock;
  PVOID                 TebBlock;
  PVOID                 TebLastAllocated;
};

#define PROCESS_STATE_TERMINATED (1)
#define PROCESS_STATE_ACTIVE     (2)

VOID PiInitDefaultLocale(VOID);
VOID PiInitProcessManager(VOID);
VOID PiShutdownProcessManager(VOID);
VOID PsInitThreadManagment(VOID);
VOID PsInitProcessManagment(VOID);
VOID PsInitIdleThread(VOID);
VOID PsDispatchThreadNoLock(ULONG NewThreadStatus);
VOID PiTerminateProcessThreads(PEPROCESS Process, NTSTATUS ExitStatus);
VOID PsTerminateCurrentThread(NTSTATUS ExitStatus);
VOID PsTerminateOtherThread(PETHREAD Thread, NTSTATUS ExitStatus);
VOID PsReleaseThread(PETHREAD Thread);
VOID PsBeginThread(PKSTART_ROUTINE StartRoutine, PVOID StartContext);
VOID PsBeginThreadWithContextInternal(VOID);
VOID PiKillMostProcesses(VOID);
NTSTATUS STDCALL PiTerminateProcess(PEPROCESS Process, NTSTATUS ExitStatus);
VOID PiInitApcManagement(VOID);
VOID STDCALL PiDeleteThread(PVOID ObjectBody);
VOID PsReapThreads(VOID);
VOID PsInitializeThreadReaper(VOID);
VOID PsQueueThreadReap(PETHREAD Thread);
NTSTATUS 
PsInitializeThread(PEPROCESS Process,
		   PETHREAD* ThreadPtr,
		   POBJECT_ATTRIBUTES ObjectAttributes,
		   KPROCESSOR_MODE AccessMode,
		   BOOLEAN First);

PACCESS_TOKEN PsReferenceEffectiveToken(PETHREAD Thread,
					PTOKEN_TYPE TokenType,
					PUCHAR b,
					PSECURITY_IMPERSONATION_LEVEL Level);

NTSTATUS PsOpenTokenOfProcess(HANDLE ProcessHandle,
			      PACCESS_TOKEN* Token);

NTSTATUS PsSuspendThread(PETHREAD Thread, PULONG PreviousCount);
NTSTATUS PsResumeThread(PETHREAD Thread, PULONG PreviousCount);

VOID STDCALL PsExitSpecialApc(PKAPC Apc, 
		      PKNORMAL_ROUTINE *NormalRoutine,
		      PVOID *NormalContext,
		      PVOID *SystemArgument1,
		      PVOID *SystemArgument2);

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


VOID STDCALL
KeInitializeThread(PKPROCESS Process, PKTHREAD Thread, BOOLEAN First);
NTSTATUS KeReleaseThread(PKTHREAD Thread);

VOID
STDCALL
KeStackAttachProcess (
    IN PKPROCESS Process,
    OUT PKAPC_STATE ApcState
    );

VOID
STDCALL
KeUnstackDetachProcess (
    IN PKAPC_STATE ApcState
    );

VOID STDCALL PiDeleteProcess(PVOID ObjectBody);
VOID PsReapThreads(VOID);
VOID PsInitializeThreadReaper(VOID);
VOID PsQueueThreadReap(PETHREAD Thread);
VOID PsUnfreezeOtherThread(PETHREAD Thread);
VOID PsFreezeOtherThread(PETHREAD Thread);
VOID PsFreezeProcessThreads(PEPROCESS Process);
VOID PsUnfreezeProcessThreads(PEPROCESS Process);
ULONG PsEnumThreadsByProcess(PEPROCESS Process);
PEPROCESS PsGetNextProcess(PEPROCESS OldProcess);
VOID
STDCALL
PsBlockThread(PNTSTATUS Status, 
              UCHAR Alertable, 
              ULONG WaitMode,
              UCHAR WaitReason);
VOID
PsUnblockThread(PETHREAD Thread, PNTSTATUS WaitStatus, KPRIORITY Increment);
VOID
PsApplicationProcessorInit(VOID);
VOID
PsPrepareForApplicationProcessorInit(ULONG Id);
VOID
PsInitReaperThread(VOID);
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

extern LONG PiNrThreadsAwaitingReaping;

NTSTATUS
PsInitWin32Thread (PETHREAD Thread);

VOID
PsTerminateWin32Process (PEPROCESS Process);

VOID
PsTerminateWin32Thread (PETHREAD Thread);

VOID
PsInitialiseW32Call(VOID);

VOID
STDCALL
PspRunCreateThreadNotifyRoutines(PETHREAD, BOOLEAN);

VOID
STDCALL
PspRunCreateProcessNotifyRoutines(PEPROCESS, BOOLEAN);

#include <pshpack1.h>
typedef struct _PS_JOB_TOKEN_FILTER
{
  UINT CapturedSidCount;
  PSID_AND_ATTRIBUTES CapturedSids;
  UINT CapturedSidsLength;
  UINT CapturedGroupCount;
  PSID_AND_ATTRIBUTES CapturedGroups;
  UINT CapturedGroupsLength;
  UINT CapturedPrivilegeCount;
  PLUID_AND_ATTRIBUTES CapturedPrivileges;
  UINT CapturedPrivilegesLength;
} PS_JOB_TOKEN_FILTER, *PPS_JOB_TOKEN_FILTER;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _EJOB
{
  KEVENT Event;
  LIST_ENTRY JobLinks;
  LIST_ENTRY ProcessListHead;
  ERESOURCE JobLock;
  LARGE_INTEGER TotalUserTime;
  LARGE_INTEGER TotalKernelTime;
  LARGE_INTEGER ThisPeriodTotalUserTime;
  LARGE_INTEGER ThisPeriodTotalKernelTime;
  UINT TotalPageFaultCount;
  UINT TotalProcesses;
  UINT ActiveProcesses;
  UINT TotalTerminatedProcesses;
  LARGE_INTEGER PerProcessUserTimeLimit;
  LARGE_INTEGER PerJobUserTimeLimit;
  UINT LimitFlags;
  UINT MinimumWorkingSetSize;
  UINT MaximumWorkingSetSize;
  UINT ActiveProcessLimit;
  UINT Affinity;
  BYTE PriorityClass;
  UINT UIRestrictionsClass;
  UINT SecurityLimitFlags;
  PVOID Token;
  PPS_JOB_TOKEN_FILTER Filter;
  UINT EndOfJobTimeAction;
  PVOID CompletionPort;
  PVOID CompletionKey;
  UINT SessionId;
  UINT SchedulingClass;
  ULONGLONG ReadOperationCount;
  ULONGLONG WriteOperationCount;
  ULONGLONG OtherOperationCount;
  ULONGLONG ReadTransferCount;
  ULONGLONG WriteTransferCount;
  ULONGLONG OtherTransferCount;
  IO_COUNTERS IoInfo;
  UINT ProcessMemoryLimit;
  UINT JobMemoryLimit;
  UINT PeakProcessMemoryUsed;
  UINT PeakJobMemoryUsed;
  UINT CurrentJobMemoryUsed;
  FAST_MUTEX MemoryLimitsLock;
} EJOB;
#include <poppack.h>

VOID INIT_FUNCTION PsInitJobManagment(VOID);

/* CLIENT ID */

NTSTATUS PsCreateCidHandle(PVOID Object, POBJECT_TYPE ObjectType, PHANDLE Handle);
NTSTATUS PsDeleteCidHandle(HANDLE CidHandle, POBJECT_TYPE ObjectType);
PHANDLE_TABLE_ENTRY PsLookupCidHandle(HANDLE CidHandle, POBJECT_TYPE ObjectType, PVOID *Object);
VOID PsUnlockCidHandle(PHANDLE_TABLE_ENTRY CidEntry);
NTSTATUS PsLockProcess(PEPROCESS Process, BOOL Timeout);
VOID PsUnlockProcess(PEPROCESS Process);

#define ETHREAD_TO_KTHREAD(pEThread) (&(pEThread)->Tcb)
#define KTHREAD_TO_ETHREAD(pKThread) (CONTAINING_RECORD((pKThread), ETHREAD, Tcb))
#define EPROCESS_TO_KPROCESS(pEProcess) (&(pEProcess)->Pcb)
#define KPROCESS_TO_EPROCESS(pKProcess) (CONTAINING_RECORD((pKProcess), EPROCESS, Pcb))

#endif /* ASSEMBLER */

#endif /* __INCLUDE_INTERNAL_PS_H */

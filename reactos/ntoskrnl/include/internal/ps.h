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
/* $Id: ps.h,v 1.48 2003/06/20 16:20:34 ekohl Exp $
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

#endif /* __ASM__ */

#include <internal/arch/ps.h>

#ifndef __ASM__

#include <internal/mm.h>
#include <napi/teb.h>

#define KeGetCurrentProcessorNumber() (KeGetCurrentKPCR()->ProcessorNumber)

extern HANDLE SystemProcessHandle;

extern LCID PsDefaultThreadLocaleId;
extern LCID PsDefaultSystemLocaleId;

#ifndef __USE_W32API

typedef struct _KAPC_STATE
{
   LIST_ENTRY ApcListHead[2];
   struct _KPROCESS* Process;
   UCHAR KernelApcInProgress;
   UCHAR KernelApcPending;
   USHORT UserApcPending;
} __attribute__((packed)) KAPC_STATE, *PKAPC_STATE;

#endif /* __USE_W32API */

typedef struct _KTHREAD
{
   /* For waiting on thread exit */
   DISPATCHER_HEADER DispatcherHeader;    /* 00 */
   
   /* List of mutants owned by the thread */
   LIST_ENTRY        MutantListHead;      /* 10 */
   PVOID             InitialStack;        /* 18 */
   ULONG             StackLimit;          /* 1C */
   
   /* Pointer to the thread's environment block in user memory */
   PTEB              Teb;                 /* 20 */
   
   /* Pointer to the thread's TLS array */
   PVOID             TlsArray;            /* 24 */
   PVOID             KernelStack;         /* 28 */
   UCHAR             DebugActive;         /* 2C */
   
   /* Thread state (one of THREAD_STATE_xxx constants below) */
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
   UCHAR             PowerState;          /* D9 */
   UCHAR             NpxIrql;             /* DA */
   UCHAR             Pad;                 /* DB */
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
   BOOL              Win32Thread;         /* 124 */
   struct _KTRAP_FRAME*      TrapFrame;   /* 128 */
   PVOID             ApcStatePointer[2];  /* 12C */
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
   
   /*
    * Below here are thread structure members that are specific to ReactOS
    */
   
   /* Added by Phillip Susi for list of threads in a process */
   LIST_ENTRY        ProcessThreadListEntry;         /* 1B0 */
} __attribute__((packed)) KTHREAD;

/* Top level irp definitions. */
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

typedef struct _ETHREAD
{
  KTHREAD Tcb;                                      /* 000 */
  TIME CreateTime;                                  /* 1B0/1B8 */
  union
  {
    TIME ExitTime;                                  /* 1B8/1E4 */
    LIST_ENTRY LpcReplyChain;                       /* 1B8/1E4 */
  } u1;
  NTSTATUS ExitStatus;                              /* 1C0/1EC */
  LIST_ENTRY PostBlockList;                         /* 1C4/1F0 */
  LIST_ENTRY TerminationPortList;                   /* 1CC/1F8 */
  KSPIN_LOCK ActiveTimerListLock;                   /* 1D4/200 */
  LIST_ENTRY ActiveTimerListHead;                   /* 1D8/204 */
  CLIENT_ID Cid;                                    /* 1E0/20C */
  KSEMAPHORE LpcReplySemaphore;                     /* 1E8/214 */
  PVOID LpcReplyMessage;                            /* 1FC/228 */
  PLARGE_INTEGER LpcReplyMessageId;                 /* 200/22C */
  ULONG PerformanceCounterLow;                      /* 204/230 */
  PPS_IMPERSONATION_INFO ImpersonationInfo;         /* 208/234 */
  LIST_ENTRY IrpList;                               /* 20C/238 */
  TOP_LEVEL_IRP* TopLevelIrp;                       /* 214/240 */
  PDEVICE_OBJECT DeviceToVerify;                    /* 218/244 */
  ULONG ReadClusterSize;                            /* 21C/248 */
  UCHAR ForwardClusterOnly;                         /* 220/24C */
  UCHAR DisablePageFaultClustering;                 /* 221/24D */
  UCHAR DeadThread;                                 /* 222/24E */
  UCHAR HasTerminated;                              /* 223/24F */
  PVOID EventPair;                                  /* 224/250 */
  ACCESS_MASK GrantedAccess;                        /* 228/254 */
  struct _EPROCESS* ThreadsProcess;                 /* 22C/258 */
  PKSTART_ROUTINE StartAddress;                     /* 230/25C */
  union
  {
    LPTHREAD_START_ROUTINE Win32StartAddress;       /* 234/260 */
    ULONG LpcReceiveMessageId;                      /* 234/260 */
  } u2;
  UCHAR LpcExitThreadCalled;                        /* 238/264 */
  UCHAR HardErrorsAreDisabled;                      /* 239/265 */
  UCHAR LpcReceivedMsgIdValid;                      /* 23A/266 */
  UCHAR ActiveImpersonationInfo;                    /* 23B/267 */
  ULONG PerformanceCountHigh;                       /* 23C/268 */

  /*
   * Added by David Welch (welch@cwcom.net)
   */
  struct _EPROCESS* OldProcess;                     /* 240/26C */

  struct _W32THREAD* Win32Thread;
  
} __attribute__((packed)) ETHREAD;

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
   * Presumably for processing int 0x21 from V86 mode DOS, currently
   * unused.
   */
  ULONG                 Int21Descriptor[2];           /* 028 */
  /* Don't know. */
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
  TIME                  CreateTime;                   /* 080 */
  /* Time of process exit. */
  TIME                  ExitTime;                     /* 088 */
  /* Unknown. */
  PVOID                 LockOwner;                    /* 090 */
  /* Process id. */
  ULONG                 UniqueProcessId;              /* 094 */
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
  struct
  {
    ULONG               LastTrimTime;
    ULONG               LastTrimFaultCount;
    ULONG               PageFaultCount;
    ULONG               PeakWorkingSetSize;
    ULONG               WorkingSetSize;
    ULONG               MinimumWorkingSetSize;
    ULONG               MaximumWorkingSetSize;
    ULONG               VmWorkingSetList;
    LIST_ENTRY          WorkingSetExpansionList;
    UCHAR               AllowWorkingSetAdjustment;
    UCHAR               AddressSpaceBeingDeleted;
    UCHAR               ForegroundPrioritySwitch;
    UCHAR               MemoryPriority;
  } Vm;
  PVOID                 LastProtoPteFault;
  struct _EPORT*        DebugPort;
  struct _EPORT*        ExceptionPort;
  PVOID                 ObjectTable;
  PVOID                 Token;
  /*  FAST_MUTEX            WorkingSetLock; */
  KMUTEX                WorkingSetLock;
  PVOID                 WorkingSetPage;
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
  PVOID                 PageDirectoryPte;
  ULONG                 LastFaultCount;
  PVOID                 VadRoot;
  PVOID                 VadHint;
  PVOID                 CloneRoot;
  ULONG                 NumberOfPrivatePages;
  ULONG                 NumberOfLockedPages;
  USHORT                NextProcessColour;
  UCHAR                 ExitProcessCalled;
  UCHAR                 CreateProcessReported;
  HANDLE                SectionHandle;
  PPEB                  Peb;
  PVOID                 SectionBaseAddress;
  PVOID                 QuotaBlock;
  NTSTATUS              LastThreadExitStatus;
  PVOID                 WorkingSetWatch;
  HANDLE                InheritedFromUniqueProcessId;
  ACCESS_MASK           GrantedAccess;
  ULONG                 DefaultHardErrorProcessing;
  PVOID                 LdtInformation;
  ULONG                 VadFreeHint;
  PVOID                 VdmObjects;
  KMUTANT               ProcessMutant;
  CHAR                  ImageFileName[16];
  ULONG                 VmTrimFaultValue;
  UCHAR                 SetTimerResolution;
  UCHAR                 PriorityClass;
  UCHAR                 SubSystemMinorVersion;
  UCHAR                 SubSystemMajorVersion;
  USHORT                SubSystemVersion;
  struct _W32PROCESS*   Win32Process;
  HANDLE                Win32WindowStation;
   
   /*
    * Added by David Welch (welch@mcmail.com)
    */
  HANDLE                Win32Desktop;
  MADDRESS_SPACE        AddressSpace;
  HANDLE_TABLE          HandleTable;
  LIST_ENTRY            ProcessListEntry;
   
   /*
    * Added by Philip Susi for list of threads in process
    */
  LIST_ENTRY           ThreadListHead;
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

PACCESS_TOKEN PsReferenceEffectiveToken(PETHREAD Thread,
					PTOKEN_TYPE TokenType,
					PUCHAR b,
					PSECURITY_IMPERSONATION_LEVEL Level);

NTSTATUS PsOpenTokenOfProcess(HANDLE ProcessHandle,
			      PACCESS_TOKEN* Token);

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
	      BOOLEAN DispatcherLock, KIRQL WaitIrql, UCHAR WaitReason);
VOID
PsUnblockThread(PETHREAD Thread, PNTSTATUS WaitStatus);
VOID
PsApplicationProcessorInit(VOID);
VOID
PsPrepareForApplicationProcessorInit(ULONG Id);
NTSTATUS STDCALL
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


NTSTATUS
PsInitWin32Thread (PETHREAD Thread);

VOID
PsTerminateWin32Process (PEPROCESS Process);

VOID
PsTerminateWin32Thread (PETHREAD Thread);

#endif /* ASSEMBLER */

#endif /* __INCLUDE_INTERNAL_PS_H */

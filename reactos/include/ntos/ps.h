/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/ntos/ps.h
 * PURPOSE:      Process/thread declarations used by all the parts of the 
 *               system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */


#ifndef NTOS_PS_H
#define NTOS_PS_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#include "mm.h"

/* Thread access rights */
#define THREAD_SET_THREAD_TOKEN		(0x0080L)
#define THREAD_IMPERSONATE		(0x0100L)
#define THREAD_DIRECT_IMPERSONATION	(0x0200L)

#define THREAD_ALL_ACCESS		(0x1f03ffL)
#define THREAD_READ			(0x020048L)
#define THREAD_WRITE			(0x020037L)
#define THREAD_EXECUTE			(0x120000L)

/* Process access rights */
#define PROCESS_TERMINATE		(0x0001L)
#define PROCESS_CREATE_THREAD		(0x0002L)
#define PROCESS_SET_SESSIONID		(0x0004L)
#define PROCESS_VM_OPERATION		(0x0008L)
#define PROCESS_VM_READ			(0x0010L)
#define PROCESS_VM_WRITE		(0x0020L)
#define PROCESS_DUP_HANDLE		(0x0040L)
#define PROCESS_CREATE_PROCESS		(0x0080L)
#define PROCESS_SET_QUOTA		(0x0100L)
#define PROCESS_SET_INFORMATION		(0x0200L)
#define PROCESS_QUERY_INFORMATION	(0x0400L)

#define PROCESS_ALL_ACCESS		(0x1f0fffL)
#define PROCESS_READ			(0x020410L)
#define PROCESS_WRITE			(0x020bebL)
#define PROCESS_EXECUTE			(0x120000L)

/* Thread priorities */
#define THREAD_PRIORITY_ABOVE_NORMAL	(1)
#define THREAD_PRIORITY_BELOW_NORMAL	(-1)
#define THREAD_PRIORITY_HIGHEST	(2)
#define THREAD_PRIORITY_IDLE	(-15)
#define THREAD_PRIORITY_LOWEST	(-2)
#define THREAD_PRIORITY_NORMAL	(0)
#define THREAD_PRIORITY_TIME_CRITICAL	(15)
#define THREAD_PRIORITY_ERROR_RETURN	(2147483647)

/* CreateProcess */
#define CREATE_DEFAULT_ERROR_MODE	(67108864)
#define CREATE_NEW_CONSOLE	(16)
#define CREATE_NEW_PROCESS_GROUP	(512)
#define CREATE_SEPARATE_WOW_VDM	(2048)
#define CREATE_SUSPENDED	(4)
#define CREATE_UNICODE_ENVIRONMENT	(1024)
#define DEBUG_PROCESS	(1)
#define DEBUG_ONLY_THIS_PROCESS	(2)
#define DETACHED_PROCESS	(8)
#define HIGH_PRIORITY_CLASS	(128)
#define IDLE_PRIORITY_CLASS	(64)
#define NORMAL_PRIORITY_CLASS	(32)
#define REALTIME_PRIORITY_CLASS	(256)

/* ResumeThread / SuspendThread */
#define MAXIMUM_SUSPEND_COUNT	(0x7f)

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
   UCHAR             WaitMode;            /* 58 */
   UCHAR             WaitNext;            /* 59 */
   UCHAR             WaitReason;          /* 5A */
   UCHAR             Pad;                 /* 5B */
   PKWAIT_BLOCK      WaitBlockList;       /* 5C */
   LIST_ENTRY        WaitListEntry;       /* 60 */
   ULONG             WaitTime;            /* 68 */
   CHAR              BasePriority;        /* 69 */
   UCHAR             DecrementCount;      /* 6A */
   UCHAR             PriorityDecrement;   /* 6B */
   UCHAR             Quantum;             /* 6C */
   KWAIT_BLOCK       WaitBlock[4];        /* 70 */
   PVOID             LegoData;            /* D0 */
   LONG              KernelApcDisable;    /* D4 */
   KAFFINITY         UserAffinity;        /* D8 */
   UCHAR             SystemAffinityActive;/* DC */
   UCHAR             Pad2[7];             /* DD */
   PKQUEUE           Queue;               /* E4 */
   KSPIN_LOCK        ApcQueueLock;        /* E8 */
   KTIMER            Timer;               /* EC */
   LIST_ENTRY        QueueListEntry;      /* 114 */
   KAFFINITY         Affinity;            /* 11C */
   UCHAR             Preempted;           /* 120 */
   UCHAR             ProcessReadyQueue;   /* 121 */
   UCHAR             KernelStackResident; /* 122 */
   UCHAR             NextProcessor;       /* 123 */
   PVOID             CallbackStack;       /* 124 */
   BOOLEAN           Win32Thread;         /* 128 */
   UCHAR             Pad3[3];             /* 129 */
   struct _KTRAP_FRAME*      TrapFrame;   /* 12C */
   PVOID             ApcStatePointer[2];  /* 130 */
   UCHAR             EnableStackSwap;     /* 138 */
   UCHAR             LargeStack;          /* 139 */
   UCHAR             ResourceIndex;       /* 13A */
   UCHAR             PreviousMode;        /* 13B */
   ULONG             KernelTime;          /* 13C */
   ULONG             UserTime;            /* 140 */
   KAPC_STATE        SavedApcState;       /* 144 */
   UCHAR             Alertable;           /* 15C */
   UCHAR             ApcStateIndex;       /* 15D */
   UCHAR             ApcQueueable;        /* 15E */
   UCHAR             AutoAlignment;       /* 15F */
   PVOID             StackBase;           /* 160 */
   KAPC              SuspendApc;          /* 164 */
   KSEMAPHORE        SuspendSemaphore;    /* 194 */
   LIST_ENTRY        ThreadListEntry;     /* 1A8 */
   CHAR              FreezeCount;         /* 1B0 */
   UCHAR             SuspendCount;        /* 1B1 */
   UCHAR             IdealProcessor;      /* 1B2 */
   UCHAR             DisableBoost;        /* 1B3 */
   
   /*
    * Below here are thread structure members that are specific to ReactOS
    */
   
   /* Added by Phillip Susi for list of threads in a process */
   LIST_ENTRY        ProcessThreadListEntry;         /* 1B4 */
   ULONG             Padding[3*4+1];
} __attribute__((packed)) KTHREAD, *PKTHREAD;


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


typedef struct _ETHREAD
{
  KTHREAD Tcb;                                      /* 000 */
  TIME CreateTime;                                  /* 1F0 */
  union
  {
    TIME ExitTime;                                  /* 1F8 */
    LIST_ENTRY LpcReplyChain;                       /* 1F8 */
  } u1;
  NTSTATUS ExitStatus;                              /* 200 */
  LIST_ENTRY PostBlockList;                         /* 204 */
  LIST_ENTRY TerminationPortList;                   /* 20C */
  KSPIN_LOCK ActiveTimerListLock;                   /* 214 */
  LIST_ENTRY ActiveTimerListHead;                   /* 218 */
  CLIENT_ID Cid;                                    /* 220 */
  KSEMAPHORE LpcReplySemaphore;                     /* 228 */
  PVOID LpcReplyMessage;                            /* 23C */
  PLARGE_INTEGER LpcReplyMessageId;                 /* 240 */
  ULONG PerformanceCounterLow;                      /* 244 */
  PPS_IMPERSONATION_INFORMATION ImpersonationInfo;  /* 248 */
  LIST_ENTRY IrpList;                               /* 24C */
  TOP_LEVEL_IRP* TopLevelIrp;                       /* 254 */
  PDEVICE_OBJECT DeviceToVerify;                    /* 258 */
  ULONG ReadClusterSize;                            /* 25C */
  UCHAR ForwardClusterOnly;                         /* 260 */
  UCHAR DisablePageFaultClustering;                 /* 261 */
  UCHAR DeadThread;                                 /* 262 */
  UCHAR HasTerminated;                              /* 263 */
  PVOID EventPair;                                  /* 264 */
  ACCESS_MASK GrantedAccess;                        /* 268 */
  struct _EPROCESS* ThreadsProcess;                 /* 26C */
  PKSTART_ROUTINE StartAddress;                     /* 270 */
  union
  {
    LPTHREAD_START_ROUTINE Win32StartAddress;       /* 274 */
    ULONG LpcReceiveMessageId;                      /* 274 */
  } u2;
  UCHAR LpcExitThreadCalled;                        /* 278 */
  UCHAR HardErrorsAreDisabled;                      /* 279 */
  UCHAR LpcReceivedMsgIdValid;                      /* 27A */
  UCHAR ActiveImpersonationInfo;                    /* 27B */
  ULONG PerformanceCountHigh;                       /* 27C */

  /*
   * Added by David Welch (welch@cwcom.net)
   */
  struct _EPROCESS* OldProcess;                     /* 280 */

  struct _W32THREAD* Win32Thread;                   /* 284 */
  
} __attribute__((packed)) ETHREAD, *PETHREAD;

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
} KPROCESS, *PKPROCESS;

typedef struct _EPROCESS
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
  //  FAST_MUTEX            WorkingSetLock;
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
   HANDLE               Win32Desktop;
   MADDRESS_SPACE       AddressSpace;
   ROS_HANDLE_TABLE     HandleTable;
   LIST_ENTRY           ProcessListEntry;
   
   /*
    * Added by Philip Susi for list of threads in process
    */
   LIST_ENTRY           ThreadListHead;
} EPROCESS, *PEPROCESS;

#define PROCESS_STATE_TERMINATED (1)
#define PROCESS_STATE_ACTIVE     (2)

#endif /* NTOS_PS_H */

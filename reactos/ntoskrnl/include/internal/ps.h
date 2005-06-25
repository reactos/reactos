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

extern LCID PsDefaultThreadLocaleId;
extern LCID PsDefaultSystemLocaleId;

/* Top level irp definitions. */
#define	FSRTL_FSP_TOP_LEVEL_IRP			(0x01)
#define	FSRTL_CACHE_TOP_LEVEL_IRP		(0x02)
#define	FSRTL_MOD_WRITE_TOP_LEVEL_IRP		(0x03)
#define	FSRTL_FAST_IO_TOP_LEVEL_IRP		(0x04)
#define	FSRTL_MAX_TOP_LEVEL_IRP_FLAG		(0x04)

typedef struct _W32_OBJECT_CALLBACK {
    OB_OPEN_METHOD WinStaCreate;
    OB_PARSE_METHOD  WinStaParse;
    OB_DELETE_METHOD  WinStaDelete;
    OB_FIND_METHOD  WinStaFind;
    OB_CREATE_METHOD  DesktopCreate;
    OB_DELETE_METHOD  DesktopDelete;
} W32_OBJECT_CALLBACK, *PW32_OBJECT_CALLBACK;

#ifndef __USE_W32API
typedef struct
{
    PACCESS_TOKEN                   Token;
    BOOLEAN                         CopyOnOpen;
    BOOLEAN                         EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL    ImpersonationLevel;
} PS_IMPERSONATION_INFORMATION, *PPS_IMPERSONATION_INFORMATION;
#endif

#include <pshpack4.h>
/*
 * NAME:           ETHREAD
 * DESCRIPTION:    Internal Executive Thread Structure.
 * PORTABILITY:    Architecture Independent.
 * KERNEL VERSION: 5.2
 * DOCUMENTATION:  http://reactos.com/wiki/index.php/ETHREAD
 */
typedef struct _ETHREAD
{
    KTHREAD                        Tcb;                         /* 1C0 */
    LARGE_INTEGER                  CreateTime;                  /* 1C0 */
    LARGE_INTEGER                  ExitTime;                    /* 1C0 */
    union {
        LIST_ENTRY                 LpcReplyChain;               /* 1C0 */
        LIST_ENTRY                 KeyedWaitChain;              /* 1C0 */
    };
    union {
        NTSTATUS                   ExitStatus;                  /* 1C8 */
        PVOID                      OfsChain;                    /* 1C8 */
    };
    LIST_ENTRY                     PostBlockList;               /* 1CC */
    union {
        struct _TERMINATION_PORT   *TerminationPort;            /* 1D4 */
        struct _ETHREAD            *ReaperLink;                 /* 1D4 */
        PVOID                      KeyedWaitValue;              /* 1D4 */
    };
    KSPIN_LOCK                     ActiveTimerListLock;         /* 1D8 */
    LIST_ENTRY                     ActiveTimerListHead;         /* 1D8 */
    CLIENT_ID                      Cid;                         /* 1E0 */
    union {
        KSEMAPHORE                 LpcReplySemaphore;           /* 1E4 */
        KSEMAPHORE                 KeyedReplySemaphore;         /* 1E4 */
    };
    union {
        PVOID                      LpcReplyMessage;             /* 200 */
        PVOID                      LpcWaitingOnPort;            /* 200 */
    };
    PPS_IMPERSONATION_INFORMATION  ImpersonationInfo;           /* 204 */
    LIST_ENTRY                     IrpList;                     /* 208 */
    ULONG                          TopLevelIrp;                 /* 210 */
    PDEVICE_OBJECT                 DeviceToVerify;              /* 214 */
    struct _EPROCESS               *ThreadsProcess;             /* 218 */
    PKSTART_ROUTINE                StartAddress;                /* 21C */
    union {
        PTHREAD_START_ROUTINE      Win32StartAddress;           /* 220 */
        ULONG                      LpcReceivedMessageId;        /* 220 */
    };
    LIST_ENTRY                     ThreadListEntry;             /* 224 */
    EX_RUNDOWN_REF                 RundownProtect;              /* 22C */
    EX_PUSH_LOCK                   ThreadLock;                  /* 230 */
    ULONG                          LpcReplyMessageId;           /* 234 */
    ULONG                          ReadClusterSize;             /* 238 */
    ACCESS_MASK                    GrantedAccess;               /* 23C */
    union {
        struct {
           ULONG                   Terminated:1;
           ULONG                   DeadThread:1;
           ULONG                   HideFromDebugger:1;
           ULONG                   ActiveImpersonationInfo:1;
           ULONG                   SystemThread:1;
           ULONG                   HardErrorsAreDisabled:1;
           ULONG                   BreakOnTermination:1;
           ULONG                   SkipCreationMsg:1;
           ULONG                   SkipTerminationMsg:1;
        };
        ULONG                      CrossThreadFlags;            /* 240 */
    };
    union {
        struct {
           ULONG                   ActiveExWorker:1;
           ULONG                   ExWorkerCanWaitUser:1;
           ULONG                   MemoryMaker:1;
           ULONG                   KeyedEventInUse:1;
        };
        ULONG                      SameThreadPassiveFlags;      /* 244 */
    };
    union {
        struct {
           ULONG                   LpcReceivedMsgIdValid:1;
           ULONG                   LpcExitThreadCalled:1;
           ULONG                   AddressSpaceOwner:1;
           ULONG                   OwnsProcessWorkingSetExclusive:1;
           ULONG                   OwnsProcessWorkingSetShared:1;
           ULONG                   OwnsSystemWorkingSetExclusive:1;
           ULONG                   OwnsSystemWorkingSetShared:1;
           ULONG                   OwnsSessionWorkingSetExclusive:1;
           ULONG                   OwnsSessionWorkingSetShared:1;
           ULONG                   ApcNeeded:1;
        };
        ULONG                      SameThreadApcFlags;          /* 248 */
    };
    UCHAR                          ForwardClusterOnly;          /* 24C */
    UCHAR                          DisablePageFaultClustering;  /* 24D */
    UCHAR                          ActiveFaultCount;            /* 24E */
} ETHREAD;

#include <poppack.h>

#ifndef __USE_W32API

typedef struct _ETHREAD *PETHREAD;

#endif /* __USE_W32API */

#include <pshpack4.h>
/*
 * NAME:           EPROCESS
 * DESCRIPTION:    Internal Executive Process Structure.
 * PORTABILITY:    Architecture Independent.
 * KERNEL VERSION: 5.2
 * DOCUMENTATION:  http://reactos.com/wiki/index.php/EPROCESS
 */
typedef struct _EPROCESS
{
    KPROCESS              Pcb;                          /* 000 */
    EX_PUSH_LOCK          ProcessLock;                  /* 078 */
    LARGE_INTEGER         CreateTime;                   /* 080 */
    LARGE_INTEGER         ExitTime;                     /* 088 */
    EX_RUNDOWN_REF        RundownProtect;               /* 090 */
    HANDLE                UniqueProcessId;              /* 094 */
    LIST_ENTRY            ActiveProcessLinks;           /* 098 */
    ULONG                 QuotaUsage[3];                /* 0A0 */
    ULONG                 QuotaPeak[3];                 /* 0AC */
    ULONG                 CommitCharge;                 /* 0B8 */
    ULONG                 PeakVirtualSize;              /* 0BC */
    ULONG                 VirtualSize;                  /* 0C0 */
    LIST_ENTRY            SessionProcessLinks;          /* 0C4 */
    PVOID                 DebugPort;                    /* 0CC */
    PVOID                 ExceptionPort;                /* 0D0 */
    PHANDLE_TABLE         ObjectTable;                  /* 0D4 */
    EX_FAST_REF           Token;                        /* 0D8 */
    ULONG                 WorkingSetPage;               /* 0DC */
    KGUARDED_MUTEX        AddressCreationLock;          /* 0E0 */
    KSPIN_LOCK            HyperSpaceLock;               /* 100 */
    PETHREAD              ForkInProgress;               /* 104 */
    ULONG                 HardwareTrigger;              /* 108 */
    MM_AVL_TABLE          PhysicalVadroot;              /* 10C */
    PVOID                 CloneRoot;                    /* 110 */
    ULONG                 NumberOfPrivatePages;         /* 114 */
    ULONG                 NumberOfLockedPages;          /* 118 */
    PVOID                 *Win32Process;                /* 11C */
    struct _EJOB          *Job;                         /* 120 */
    PVOID                 SectionObject;                /* 124 */
    PVOID                 SectionBaseAddress;           /* 128 */
    PEPROCESS_QUOTA_BLOCK QuotaBlock;                   /* 12C */
    PPAGEFAULT_HISTORY    WorkingSetWatch;              /* 130 */
    PVOID                 Win32WindowStation;           /* 134 */
    HANDLE                InheritedFromUniqueProcessId; /* 138 */
    PVOID                 LdtInformation;               /* 13C */
    PVOID                 VadFreeHint;                  /* 140 */
    PVOID                 VdmObjects;                   /* 144 */
    PVOID                 DeviceMap;                    /* 148 */
    PVOID                 Spare0[3];                    /* 14C */
    union {
        HARDWARE_PTE_X86  PagedirectoryPte;             /* 158 */
        ULONGLONG         Filler;                       /* 158 */
    };
    ULONG                 Session;                      /* 160 */
    CHAR                  ImageFileName[16];            /* 164 */
    LIST_ENTRY            JobLinks;                     /* 174 */
    PVOID                 LockedPagesList;              /* 17C */
    LIST_ENTRY            ThreadListHead;               /* 184 */
    PVOID                 SecurityPort;                 /* 188 */
    PVOID                 PaeTop;                       /* 18C */
    ULONG                 ActiveThreds;                 /* 190 */
    ACCESS_MASK           GrantedAccess;                /* 194 */
    ULONG                 DefaultHardErrorProcessing;   /* 198 */
    NTSTATUS              LastThreadExitStatus;         /* 19C */
    struct _PEB*          Peb;                          /* 1A0 */
    EX_FAST_REF           PrefetchTrace;                /* 1A4 */
    LARGE_INTEGER         ReadOperationCount;           /* 1A8 */
    LARGE_INTEGER         WriteOperationCount;          /* 1B0 */
    LARGE_INTEGER         OtherOperationCount;          /* 1B8 */
    LARGE_INTEGER         ReadTransferCount;            /* 1C0 */
    LARGE_INTEGER         WriteTransferCount;           /* 1C8 */
    LARGE_INTEGER         OtherTransferCount;           /* 1D0 */
    ULONG                 CommitChargeLimit;            /* 1D8 */
    ULONG                 CommitChargePeak;             /* 1DC */
    PVOID                 AweInfo;                      /* 1E0 */
    SE_AUDIT_PROCESS_CREATION_INFO SeAuditProcessCreationInfo; /* 1E4 */
    MMSUPPORT             Vm;                           /* 1E8 */
    LIST_ENTRY            MmProcessLinks;               /* 230 */
    ULONG                 ModifiedPageCount;            /* 238 */
    ULONG                 JobStatus;                    /* 23C */
    union {
        struct {
            ULONG         CreateReported:1;
            ULONG         NoDebugInherit:1;
            ULONG         ProcessExiting:1;
            ULONG         ProcessDelete:1;
            ULONG         Wow64SplitPages:1;
            ULONG         VmDeleted:1;
            ULONG         OutswapEnabled:1;
            ULONG         Outswapped:1;
            ULONG         ForkFailed:1;
            ULONG         Wow64VaSpace4Gb:1;
            ULONG         AddressSpaceInitialized:2;
            ULONG         SetTimerResolution:1;
            ULONG         BreakOnTermination:1;
            ULONG         SessionCreationUnderway:1;
            ULONG         WriteWatch:1;
            ULONG         ProcessInSession:1;
            ULONG         OverrideAddressSpace:1;
            ULONG         HasAddressSpace:1;
            ULONG         LaunchPrefetched:1;
            ULONG         InjectInpageErrors:1;
            ULONG         VmTopDown:1;
            ULONG         ImageNotifyDone:1;
            ULONG         PdeUpdateNeeded:1;
            ULONG         VdmAllowed:1;
            ULONG         SmapAllowed:1;
            ULONG         CreateFailed:1;
            ULONG         DefaultIoPriority:3;
            ULONG         Spare1:1;
            ULONG         Spare2:1;
        };
        ULONG             Flags;                        /* 240 */
    };

    NTSTATUS              ExitStatus;                   /* 244 */
    USHORT                NextPageColor;                /* 248 */
    union {
        struct {
            UCHAR         SubSystemMinorVersion;        /* 24A */
            UCHAR         SubSystemMajorVersion;        /* 24B */
        };
        USHORT            SubSystemVersion;             /* 24A */
    };
    UCHAR                 PriorityClass;                /* 24C */
    MM_AVL_TABLE          VadRoot;                      /* 250 */
    ULONG                 Cookie;                       /* 270 */

/***************************************************************
 *                REACTOS SPECIFIC START
 ***************************************************************/
    /* FIXME WILL BE DEPRECATED WITH PUSHLOCK SUPPORT IN 0.3.0 */
    KEVENT                LockEvent;                    /* 274 */
    ULONG                 LockCount;                    /* 284 */
    struct _KTHREAD       *LockOwner;                   /* 288 */

    /* FIXME MOVE TO AVL TREES                                 */
    MADDRESS_SPACE        AddressSpace;                 /* 28C */
} EPROCESS;
#include <poppack.h>

#define PROCESS_STATE_TERMINATED (1)
#define PROCESS_STATE_ACTIVE     (2)

VOID PiInitDefaultLocale(VOID);
VOID PiInitProcessManager(VOID);
VOID PiShutdownProcessManager(VOID);
VOID PsInitThreadManagment(VOID);
VOID PsInitProcessManagment(VOID);
VOID PsInitIdleThread(VOID);
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

PACCESS_TOKEN STDCALL PsReferenceEffectiveToken(PETHREAD Thread,
					PTOKEN_TYPE TokenType,
					PUCHAR b,
					PSECURITY_IMPERSONATION_LEVEL Level);

NTSTATUS STDCALL PsOpenTokenOfProcess(HANDLE ProcessHandle,
			      PACCESS_TOKEN* Token);
VOID
STDCALL
PspTerminateProcessThreads(PEPROCESS Process,
                           NTSTATUS ExitStatus);
NTSTATUS PsSuspendThread(PETHREAD Thread, PULONG PreviousCount);
NTSTATUS PsResumeThread(PETHREAD Thread, PULONG PreviousCount);
NTSTATUS
STDCALL
PspAssignPrimaryToken(PEPROCESS Process,
                      HANDLE TokenHandle);
VOID STDCALL PsExitSpecialApc(PKAPC Apc,
		      PKNORMAL_ROUTINE *NormalRoutine,
		      PVOID *NormalContext,
		      PVOID *SystemArgument1,
		      PVOID *SystemArgument2);

NTSTATUS
STDCALL
PspInitializeProcessSecurity(PEPROCESS Process,
                             PEPROCESS Parent OPTIONAL);


VOID
STDCALL
PspSystemThreadStartup(PKSTART_ROUTINE StartRoutine,
                       PVOID StartContext);

NTSTATUS
PsInitializeIdleOrFirstThread (
    PEPROCESS Process,
    PETHREAD* ThreadPtr,
    PKSTART_ROUTINE StartRoutine,
    KPROCESSOR_MODE AccessMode,
    BOOLEAN First);
/*
 * Internal thread priorities, added by Phillip Susi
 * TODO: rebalence these to make use of all priorities... the ones above 16
 * can not all be used right now
 */
#define PROCESS_PRIO_IDLE			3
#define PROCESS_PRIO_NORMAL			8
#define PROCESS_PRIO_HIGH			13
#define PROCESS_PRIO_RT				18


VOID STDCALL PiDeleteProcess(PVOID ObjectBody);

VOID
STDCALL
PspReapRoutine(PVOID Context);

VOID
STDCALL
PspExitThread(NTSTATUS ExitStatus);

extern LIST_ENTRY PspReaperListHead;
extern WORK_QUEUE_ITEM PspReaperWorkItem;
extern BOOLEAN PspReaping;
extern PEPROCESS PsInitialSystemProcess;
extern PEPROCESS PsIdleProcess;
extern LIST_ENTRY PsActiveProcessHead;
extern FAST_MUTEX PspActiveProcessMutex;
extern LARGE_INTEGER ShortPsLockDelay, PsLockTimeout;

VOID
STDCALL
PspTerminateThreadByPointer(PETHREAD Thread,
                            NTSTATUS ExitStatus);

VOID PsUnfreezeOtherThread(PETHREAD Thread);
VOID PsFreezeOtherThread(PETHREAD Thread);
VOID PsFreezeProcessThreads(PEPROCESS Process);
VOID PsUnfreezeProcessThreads(PEPROCESS Process);
ULONG PsEnumThreadsByProcess(PEPROCESS Process);
PEPROCESS STDCALL PsGetNextProcess(PEPROCESS OldProcess);
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
VOID
PsInitialiseSuspendImplementation(VOID);
NTSTATUS
STDCALL
PspExitProcess(PEPROCESS Process);

VOID
STDCALL
PspDeleteProcess(PVOID ObjectBody);

VOID
STDCALL
PspDeleteThread(PVOID ObjectBody);

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
} EJOB, *PEJOB;
#include <poppack.h>

VOID INIT_FUNCTION PsInitJobManagment(VOID);

/* CLIENT ID */

NTSTATUS PsCreateCidHandle(PVOID Object, POBJECT_TYPE ObjectType, PHANDLE Handle);
NTSTATUS PsDeleteCidHandle(HANDLE CidHandle, POBJECT_TYPE ObjectType);
PHANDLE_TABLE_ENTRY PsLookupCidHandle(HANDLE CidHandle, POBJECT_TYPE ObjectType, PVOID *Object);
VOID PsUnlockCidHandle(PHANDLE_TABLE_ENTRY CidEntry);
NTSTATUS PsLockProcess(PEPROCESS Process, BOOLEAN Timeout);
VOID PsUnlockProcess(PEPROCESS Process);

#define ETHREAD_TO_KTHREAD(pEThread) (&(pEThread)->Tcb)
#define KTHREAD_TO_ETHREAD(pKThread) (CONTAINING_RECORD((pKThread), ETHREAD, Tcb))
#define EPROCESS_TO_KPROCESS(pEProcess) (&(pEProcess)->Pcb)
#define KPROCESS_TO_EPROCESS(pKProcess) (CONTAINING_RECORD((pKProcess), EPROCESS, Pcb))

#define MAX_PROCESS_NOTIFY_ROUTINE_COUNT    8
#define MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT  8

#endif /* ASSEMBLER */

#endif /* __INCLUDE_INTERNAL_PS_H */

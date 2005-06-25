/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/pstypes.h
 * PURPOSE:         Defintions for Process Manager Types not documented in DDK/IFS.
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _PSTYPES_H
#define _PSTYPES_H

/* DEPENDENCIES **************************************************************/
#include "ldrtypes.h"
#include "rtltypes.h"

/* EXPORTED DATA *************************************************************/

extern NTOSAPI struct _EPROCESS* PsInitialSystemProcess;
extern NTOSAPI POBJECT_TYPE PsProcessType;
extern NTOSAPI POBJECT_TYPE PsThreadType;

/* CONSTANTS *****************************************************************/

/* These are not exposed to drivers normally */
#ifndef _NTOS_MODE_USER
    #define JOB_OBJECT_ASSIGN_PROCESS    (1)
    #define JOB_OBJECT_SET_ATTRIBUTES    (2)
    #define JOB_OBJECT_QUERY    (4)
    #define JOB_OBJECT_TERMINATE    (8)
    #define JOB_OBJECT_SET_SECURITY_ATTRIBUTES    (16)
    #define JOB_OBJECT_ALL_ACCESS    (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|31)
#endif

/* FIXME: This was changed in XP... Ask ThomasW about it */
#define PROCESS_SET_PORT 0x800

#define THREAD_ALERT 0x4

#define USER_SHARED_DATA (0x7FFE0000)

/* Process priority classes */
#define PROCESS_PRIORITY_CLASS_HIGH	(4) /* FIXME */
#define PROCESS_PRIORITY_CLASS_IDLE	(0) /* FIXME */
#define PROCESS_PRIORITY_CLASS_NORMAL	(2) /* FIXME */
#define PROCESS_PRIORITY_CLASS_REALTIME	(5) /* FIXME */
#define PROCESS_PRIORITY_CLASS_BELOW_NORMAL (1) /* FIXME */
#define PROCESS_PRIORITY_CLASS_ABOVE_NORMAL (3) /* FIXME */

/* Global Flags */
#define FLG_STOP_ON_EXCEPTION          0x00000001
#define FLG_SHOW_LDR_SNAPS             0x00000002
#define FLG_DEBUG_INITIAL_COMMAND      0x00000004
#define FLG_STOP_ON_HUNG_GUI           0x00000008
#define FLG_HEAP_ENABLE_TAIL_CHECK     0x00000010
#define FLG_HEAP_ENABLE_FREE_CHECK     0x00000020
#define FLG_HEAP_VALIDATE_PARAMETERS   0x00000040
#define FLG_HEAP_VALIDATE_ALL          0x00000080
#define FLG_POOL_ENABLE_TAIL_CHECK     0x00000100
#define FLG_POOL_ENABLE_FREE_CHECK     0x00000200
#define FLG_POOL_ENABLE_TAGGING        0x00000400
#define FLG_HEAP_ENABLE_TAGGING        0x00000800
#define FLG_USER_STACK_TRACE_DB        0x00001000
#define FLG_KERNEL_STACK_TRACE_DB      0x00002000
#define FLG_MAINTAIN_OBJECT_TYPELIST   0x00004000
#define FLG_HEAP_ENABLE_TAG_BY_DLL     0x00008000
#define FLG_IGNORE_DEBUG_PRIV          0x00010000
#define FLG_ENABLE_CSRDEBUG            0x00020000
#define FLG_ENABLE_KDEBUG_SYMBOL_LOAD  0x00040000
#define FLG_DISABLE_PAGE_KERNEL_STACKS 0x00080000
#define FLG_HEAP_ENABLE_CALL_TRACING   0x00100000
#define FLG_HEAP_DISABLE_COALESCING    0x00200000
#define FLG_ENABLE_CLOSE_EXCEPTIONS    0x00400000
#define FLG_ENABLE_EXCEPTION_LOGGING   0x00800000
#define FLG_ENABLE_HANDLE_TYPE_TAGGING 0x01000000
#define FLG_HEAP_PAGE_ALLOCS           0x02000000
#define FLG_DEBUG_INITIAL_COMMAND_EX   0x04000000

/* ENUMERATIONS **************************************************************/

/* FUNCTION TYPES ************************************************************/
typedef VOID (STDCALL *PPEBLOCKROUTINE)(PVOID);

typedef NTSTATUS 
(STDCALL *PW32_PROCESS_CALLBACK)(
    struct _EPROCESS *Process,
    BOOLEAN Create
);

typedef NTSTATUS
(STDCALL *PW32_THREAD_CALLBACK)(
    struct _ETHREAD *Thread,
    BOOLEAN Create
);

/* TYPES *********************************************************************/

struct _ETHREAD;

typedef struct _CURDIR 
{
    UNICODE_STRING DosPath;
    PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct _DESCRIPTOR_TABLE_ENTRY
{
    ULONG Selector;
    LDT_ENTRY Descriptor;
} DESCRIPTOR_TABLE_ENTRY, *PDESCRIPTOR_TABLE_ENTRY;

typedef struct _PEB_FREE_BLOCK 
{
    struct _PEB_FREE_BLOCK* Next;
    ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

typedef struct _PEB
{
    UCHAR InheritedAddressSpace;                     /* 00h */
    UCHAR ReadImageFileExecOptions;                  /* 01h */
    UCHAR BeingDebugged;                             /* 02h */
    BOOLEAN SpareBool;                               /* 03h */
    HANDLE Mutant;                                   /* 04h */
    PVOID ImageBaseAddress;                          /* 08h */
    PPEB_LDR_DATA Ldr;                               /* 0Ch */
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;  /* 10h */
    PVOID SubSystemData;                             /* 14h */
    PVOID ProcessHeap;                               /* 18h */
    PVOID FastPebLock;                               /* 1Ch */
    PPEBLOCKROUTINE FastPebLockRoutine;              /* 20h */
    PPEBLOCKROUTINE FastPebUnlockRoutine;            /* 24h */
    ULONG EnvironmentUpdateCount;                    /* 28h */
    PVOID* KernelCallbackTable;                      /* 2Ch */
    PVOID EventLogSection;                           /* 30h */
    PVOID EventLog;                                  /* 34h */
    PPEB_FREE_BLOCK FreeList;                        /* 38h */
    ULONG TlsExpansionCounter;                       /* 3Ch */
    PVOID TlsBitmap;                                 /* 40h */
    ULONG TlsBitmapBits[0x2];                        /* 44h */
    PVOID ReadOnlySharedMemoryBase;                  /* 4Ch */
    PVOID ReadOnlySharedMemoryHeap;                  /* 50h */
    PVOID* ReadOnlyStaticServerData;                 /* 54h */
    PVOID AnsiCodePageData;                          /* 58h */
    PVOID OemCodePageData;                           /* 5Ch */
    PVOID UnicodeCaseTableData;                      /* 60h */
    ULONG NumberOfProcessors;                        /* 64h */
    ULONG NtGlobalFlag;                              /* 68h */
    LARGE_INTEGER CriticalSectionTimeout;            /* 70h */
    ULONG HeapSegmentReserve;                        /* 78h */
    ULONG HeapSegmentCommit;                         /* 7Ch */
    ULONG HeapDeCommitTotalFreeThreshold;            /* 80h */
    ULONG HeapDeCommitFreeBlockThreshold;            /* 84h */
    ULONG NumberOfHeaps;                             /* 88h */
    ULONG MaximumNumberOfHeaps;                      /* 8Ch */
    PVOID* ProcessHeaps;                             /* 90h */
    PVOID GdiSharedHandleTable;                      /* 94h */
    PVOID ProcessStarterHelper;                      /* 98h */
    PVOID GdiDCAttributeList;                        /* 9Ch */
    PVOID LoaderLock;                                /* A0h */
    ULONG OSMajorVersion;                            /* A4h */
    ULONG OSMinorVersion;                            /* A8h */
    USHORT OSBuildNumber;                            /* ACh */
    USHORT OSCSDVersion;                             /* AEh */
    ULONG OSPlatformId;                              /* B0h */
    ULONG ImageSubSystem;                            /* B4h */
    ULONG ImageSubSystemMajorVersion;                /* B8h */
    ULONG ImageSubSystemMinorVersion;                /* BCh */
    ULONG ImageProcessAffinityMask;                  /* C0h */
    ULONG GdiHandleBuffer[0x22];                     /* C4h */
    PVOID PostProcessInitRoutine;                    /* 14Ch */
    PVOID *TlsExpansionBitmap;                       /* 150h */
    ULONG TlsExpansionBitmapBits[0x20];              /* 154h */
    ULONG SessionId;                                 /* 1D4h */
    PVOID AppCompatInfo;                             /* 1D8h */
    UNICODE_STRING CSDVersion;                       /* 1DCh */
} PEB;

typedef struct _GDI_TEB_BATCH 
{
    ULONG Offset;
    ULONG HDC;
    ULONG Buffer[0x136];
} GDI_TEB_BATCH, *PGDI_TEB_BATCH;

typedef struct _INITIAL_TEB
{
  PVOID StackBase;
  PVOID StackLimit;
  PVOID StackCommit;
  PVOID StackCommitMax;
  PVOID StackReserved;
} INITIAL_TEB, *PINITIAL_TEB;

typedef struct _TEB 
{
    NT_TIB Tib;                         /* 00h */
    PVOID EnvironmentPointer;           /* 1Ch */
    CLIENT_ID Cid;                      /* 20h */
    PVOID ActiveRpcInfo;                /* 28h */
    PVOID ThreadLocalStoragePointer;    /* 2Ch */
    struct _PEB *Peb;                   /* 30h */
    ULONG LastErrorValue;               /* 34h */
    ULONG CountOfOwnedCriticalSections; /* 38h */
    PVOID CsrClientThread;              /* 3Ch */
    struct _W32THREAD* Win32ThreadInfo; /* 40h */
    ULONG Win32ClientInfo[0x1F];        /* 44h */
    PVOID WOW32Reserved;                /* C0h */
    LCID CurrentLocale;                 /* C4h */
    ULONG FpSoftwareStatusRegister;     /* C8h */
    PVOID SystemReserved1[0x36];        /* CCh */
    PVOID Spare1;                       /* 1A4h */
    LONG ExceptionCode;                 /* 1A8h */
    UCHAR SpareBytes1[0x28];            /* 1ACh */
    PVOID SystemReserved2[0xA];         /* 1D4h */
    GDI_TEB_BATCH GdiTebBatch;          /* 1FCh */
    ULONG gdiRgn;                       /* 6DCh */
    ULONG gdiPen;                       /* 6E0h */
    ULONG gdiBrush;                     /* 6E4h */
    CLIENT_ID RealClientId;             /* 6E8h */
    PVOID GdiCachedProcessHandle;       /* 6F0h */
    ULONG GdiClientPID;                 /* 6F4h */
    ULONG GdiClientTID;                 /* 6F8h */
    PVOID GdiThreadLocaleInfo;          /* 6FCh */
    PVOID UserReserved[5];              /* 700h */
    PVOID glDispatchTable[0x118];       /* 714h */
    ULONG glReserved1[0x1A];            /* B74h */
    PVOID glReserved2;                  /* BDCh */
    PVOID glSectionInfo;                /* BE0h */
    PVOID glSection;                    /* BE4h */
    PVOID glTable;                      /* BE8h */
    PVOID glCurrentRC;                  /* BECh */
    PVOID glContext;                    /* BF0h */
    NTSTATUS LastStatusValue;           /* BF4h */
    UNICODE_STRING StaticUnicodeString; /* BF8h */
    WCHAR StaticUnicodeBuffer[0x105];   /* C00h */
    PVOID DeallocationStack;            /* E0Ch */
    PVOID TlsSlots[0x40];               /* E10h */
    LIST_ENTRY TlsLinks;                /* F10h */
    PVOID Vdm;                          /* F18h */
    PVOID ReservedForNtRpc;             /* F1Ch */
    PVOID DbgSsReserved[0x2];           /* F20h */
    ULONG HardErrorDisabled;            /* F28h */
    PVOID Instrumentation[0x10];        /* F2Ch */
    PVOID WinSockData;                  /* F6Ch */
    ULONG GdiBatchCount;                /* F70h */
    USHORT _Spare2;                     /* F74h */
    BOOLEAN IsFiber;                    /* F76h */
    UCHAR Spare3;                       /* F77h */
    ULONG _Spare4;                      /* F78h */
    ULONG _Spare5;                      /* F7Ch */
    PVOID ReservedForOle;               /* F80h */
    ULONG WaitingOnLoaderLock;          /* F84h */
    ULONG _Unknown[11];                 /* F88h */
    PVOID FlsSlots;                     /* FB4h */
    PVOID WineDebugInfo;                /* Needed for WINE DLL's  */
} TEB, *PTEB;

/* KERNEL MODE ONLY **********************************************************/
#ifndef NTOS_MODE_USER

#include "mmtypes.h"
#include "extypes.h"
#include "setypes.h"

/* FIXME: see note in mmtypes.h */
#ifdef _NTOSKRNL_
#include <internal/mm.h>
#endif

typedef struct _EPROCESS_QUOTA_ENTRY
{
    ULONG Usage;
    ULONG Limit;
    ULONG Peak;
    ULONG Return;
} EPROCESS_QUOTA_ENTRY, *PEPROCESS_QUOTA_ENTRY;

typedef struct _EPROCESS_QUOTA_BLOCK
{
    EPROCESS_QUOTA_ENTRY QuotaEntry[3];
    LIST_ENTRY QuotaList;
    ULONG ReferenceCount;
    ULONG ProcessCount;
} EPROCESS_QUOTA_BLOCK, *PEPROCESS_QUOTA_BLOCK;

typedef struct _PAGEFAULT_HISTORY
{
    ULONG CurrentIndex;
    ULONG MapIndex;
    KSPIN_LOCK SpinLock;
    PVOID Reserved;
    PROCESS_WS_WATCH_INFORMATION WatchInfo[1];
} PAGEFAULT_HISTORY, *PPAGEFAULT_HISTORY;

typedef struct _PS_IMPERSONATION_INFORMATION
{
    PACCESS_TOKEN                   Token;
    BOOLEAN                         CopyOnOpen;
    BOOLEAN                         EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL    ImpersonationLevel;
} PS_IMPERSONATION_INFORMATION, *PPS_IMPERSONATION_INFORMATION;

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
    KGUARDED_MUTEX MemoryLimitsLock;
    ULONG MemberLevel;
    ULONG JobFlags;
} EJOB, *PEJOB;
#include <poppack.h>

#endif

#endif

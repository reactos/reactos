/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    pstypes.h

Abstract:

    Type definitions for the Process Manager

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _PSTYPES_H
#define _PSTYPES_H

//
// Dependencies
//
#include <umtypes.h>
#include <ldrtypes.h>
#include <mmtypes.h>
#include <obtypes.h>
#ifndef NTOS_MODE_USER
#include <extypes.h>
#include <setypes.h>
#endif

//
// KUSER_SHARED_DATA location in User Mode
//
#define USER_SHARED_DATA                        (0x7FFE0000)

//
// Global Flags
//
#define FLG_STOP_ON_EXCEPTION                   0x00000001
#define FLG_SHOW_LDR_SNAPS                      0x00000002
#define FLG_DEBUG_INITIAL_COMMAND               0x00000004
#define FLG_STOP_ON_HUNG_GUI                    0x00000008
#define FLG_HEAP_ENABLE_TAIL_CHECK              0x00000010
#define FLG_HEAP_ENABLE_FREE_CHECK              0x00000020
#define FLG_HEAP_VALIDATE_PARAMETERS            0x00000040
#define FLG_HEAP_VALIDATE_ALL                   0x00000080
#define FLG_POOL_ENABLE_TAIL_CHECK              0x00000100
#define FLG_POOL_ENABLE_FREE_CHECK              0x00000200
#define FLG_POOL_ENABLE_TAGGING                 0x00000400
#define FLG_HEAP_ENABLE_TAGGING                 0x00000800
#define FLG_USER_STACK_TRACE_DB                 0x00001000
#define FLG_KERNEL_STACK_TRACE_DB               0x00002000
#define FLG_MAINTAIN_OBJECT_TYPELIST            0x00004000
#define FLG_HEAP_ENABLE_TAG_BY_DLL              0x00008000
#define FLG_IGNORE_DEBUG_PRIV                   0x00010000
#define FLG_ENABLE_CSRDEBUG                     0x00020000
#define FLG_ENABLE_KDEBUG_SYMBOL_LOAD           0x00040000
#define FLG_DISABLE_PAGE_KERNEL_STACKS          0x00080000
#define FLG_HEAP_ENABLE_CALL_TRACING            0x00100000
#define FLG_HEAP_DISABLE_COALESCING             0x00200000
#define FLG_ENABLE_CLOSE_EXCEPTIONS             0x00400000
#define FLG_ENABLE_EXCEPTION_LOGGING            0x00800000
#define FLG_ENABLE_HANDLE_TYPE_TAGGING          0x01000000
#define FLG_HEAP_PAGE_ALLOCS                    0x02000000
#define FLG_DEBUG_INITIAL_COMMAND_EX            0x04000000
#define FLG_VALID_BITS                          0x07FFFFFF

//
// Process priority classes
//
#define PROCESS_PRIORITY_CLASS_INVALID          0
#define PROCESS_PRIORITY_CLASS_IDLE             1
#define PROCESS_PRIORITY_CLASS_NORMAL           2
#define PROCESS_PRIORITY_CLASS_HIGH             3
#define PROCESS_PRIORITY_CLASS_REALTIME         4
#define PROCESS_PRIORITY_CLASS_BELOW_NORMAL     5
#define PROCESS_PRIORITY_CLASS_ABOVE_NORMAL     6

//
// NtCreateProcessEx flags
//
#define PS_REQUEST_BREAKAWAY                    1
#define PS_NO_DEBUG_INHERIT                     2
#define PS_INHERIT_HANDLES                      4
#define PS_UNKNOWN_VALUE                        8
#define PS_ALL_FLAGS                            (PS_REQUEST_BREAKAWAY | \
                                                 PS_NO_DEBUG_INHERIT  | \
                                                 PS_INHERIT_HANDLES   | \
                                                 PS_UNKNOWN_VALUE)

//
// Process base priorities
//
#define PROCESS_PRIORITY_IDLE                   3
#define PROCESS_PRIORITY_NORMAL                 8
#define PROCESS_PRIORITY_NORMAL_FOREGROUND      9

//
// Process Priority Separation Values (OR)
//
#define PSP_VARIABLE_QUANTUMS                   4
#define PSP_LONG_QUANTUMS                       16

#ifndef NTOS_MODE_USER

//
// Thread Access Types
//
#define THREAD_QUERY_INFORMATION                0x0040
#define THREAD_SET_THREAD_TOKEN                 0x0080
#define THREAD_IMPERSONATE                      0x0100
#define THREAD_DIRECT_IMPERSONATION             0x0200

//
// Process Access Types
//
#define PROCESS_TERMINATE                       0x0001
#define PROCESS_CREATE_THREAD                   0x0002
#define PROCESS_SET_SESSIONID                   0x0004
#define PROCESS_VM_OPERATION                    0x0008
#define PROCESS_VM_READ                         0x0010
#define PROCESS_VM_WRITE                        0x0020
#define PROCESS_CREATE_PROCESS                  0x0080
#define PROCESS_SET_QUOTA                       0x0100
#define PROCESS_SET_INFORMATION                 0x0200
#define PROCESS_QUERY_INFORMATION               0x0400
#define PROCESS_SUSPEND_RESUME                  0x0800
#define PROCESS_QUERY_LIMITED_INFORMATION       0x1000
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
#define PROCESS_ALL_ACCESS                      (STANDARD_RIGHTS_REQUIRED | \
                                                 SYNCHRONIZE | \
                                                 0xFFFF)
#else
#define PROCESS_ALL_ACCESS                      (STANDARD_RIGHTS_REQUIRED | \
                                                 SYNCHRONIZE | \
                                                 0xFFF)

//
// Thread Base Priorities
//
#define THREAD_BASE_PRIORITY_LOWRT              15
#define THREAD_BASE_PRIORITY_MAX                2
#define THREAD_BASE_PRIORITY_MIN                -2
#define THREAD_BASE_PRIORITY_IDLE               -15

//
// TLS Slots
//
#define TLS_MINIMUM_AVAILABLE                   64
#endif

//
// Job Access Types
//
#define JOB_OBJECT_ASSIGN_PROCESS               0x1
#define JOB_OBJECT_SET_ATTRIBUTES               0x2
#define JOB_OBJECT_QUERY                        0x4
#define JOB_OBJECT_TERMINATE                    0x8
#define JOB_OBJECT_SET_SECURITY_ATTRIBUTES      0x10
#define JOB_OBJECT_ALL_ACCESS                   (STANDARD_RIGHTS_REQUIRED | \
                                                 SYNCHRONIZE | \
                                                 31)

//
// Cross Thread Flags
//
#define CT_TERMINATED_BIT                       0x1
#define CT_DEAD_THREAD_BIT                      0x2
#define CT_HIDE_FROM_DEBUGGER_BIT               0x4
#define CT_ACTIVE_IMPERSONATION_INFO_BIT        0x8
#define CT_SYSTEM_THREAD_BIT                    0x10
#define CT_HARD_ERRORS_ARE_DISABLED_BIT         0x20
#define CT_BREAK_ON_TERMINATION_BIT             0x40
#define CT_SKIP_CREATION_MSG_BIT                0x80
#define CT_SKIP_TERMINATION_MSG_BIT             0x100

//
// Same Thread Passive Flags
//
#define STP_ACTIVE_EX_WORKER_BIT                0x1
#define STP_EX_WORKER_CAN_WAIT_USER_BIT         0x2
#define STP_MEMORY_MAKER_BIT                    0x4
#define STP_KEYED_EVENT_IN_USE_BIT              0x8

//
// Same Thread APC Flags
//
#define STA_LPC_RECEIVED_MSG_ID_VALID_BIT       0x1
#define STA_LPC_EXIT_THREAD_CALLED_BIT          0x2
#define STA_ADDRESS_SPACE_OWNER_BIT             0x4
#endif

#define TLS_EXPANSION_SLOTS                     1024
//
// Process Flags
//
#define PSF_CREATE_REPORTED_BIT                 0x1
#define PSF_NO_DEBUG_INHERIT_BIT                0x2
#define PSF_PROCESS_EXITING_BIT                 0x4
#define PSF_PROCESS_DELETE_BIT                  0x8
#define PSF_WOW64_SPLIT_PAGES_BIT               0x10
#define PSF_VM_DELETED_BIT                      0x20
#define PSF_OUTSWAP_ENABLED_BIT                 0x40
#define PSF_OUTSWAPPED_BIT                      0x80
#define PSF_FORK_FAILED_BIT                     0x100
#define PSF_WOW64_VA_SPACE_4GB_BIT              0x200
#define PSF_ADDRESS_SPACE_INITIALIZED_BIT       0x400
#define PSF_SET_TIMER_RESOLUTION_BIT            0x1000
#define PSF_BREAK_ON_TERMINATION_BIT            0x2000
#define PSF_SESSION_CREATION_UNDERWAY_BIT       0x4000
#define PSF_WRITE_WATCH_BIT                     0x8000
#define PSF_PROCESS_IN_SESSION_BIT              0x10000
#define PSF_OVERRIDE_ADDRESS_SPACE_BIT          0x20000
#define PSF_HAS_ADDRESS_SPACE_BIT               0x40000
#define PSF_LAUNCH_PREFETCHED_BIT               0x80000
#define PSF_INJECT_INPAGE_ERRORS_BIT            0x100000
#define PSF_VM_TOP_DOWN_BIT                     0x200000
#define PSF_IMAGE_NOTIFY_DONE_BIT               0x400000
#define PSF_PDE_UPDATE_NEEDED_BIT               0x800000
#define PSF_VDM_ALLOWED_BIT                     0x1000000
#define PSF_SWAP_ALLOWED_BIT                    0x2000000
#define PSF_CREATE_FAILED_BIT                   0x4000000
#define PSF_DEFAULT_IO_PRIORITY_BIT             0x8000000

//
// Vista Process Flags
//
#define PSF2_PROTECTED_BIT                      0x800

#ifdef NTOS_MODE_USER
//
// Current Process/Thread built-in 'special' handles
//
#define NtCurrentProcess()                      ((HANDLE)(LONG_PTR)-1)
#define ZwCurrentProcess()                      NtCurrentProcess()
#define NtCurrentThread()                       ((HANDLE)(LONG_PTR)-2)
#define ZwCurrentThread()                       NtCurrentThread()

//
// Process/Thread/Job Information Classes for NtQueryInformationProcess/Thread/Job
//
typedef enum _PROCESSINFOCLASS
{
    ProcessBasicInformation,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessDeviceMap,
    ProcessSessionInformation,
    ProcessForegroundInformation,
    ProcessWow64Information,
    ProcessImageFileName,
    ProcessLUIDDeviceMapsEnabled,
    ProcessBreakOnTermination,
    ProcessDebugObjectHandle,
    ProcessDebugFlags,
    ProcessHandleTracing,
    ProcessIoPriority,
    ProcessExecuteFlags,
    ProcessTlsInformation,
    ProcessCookie,
    ProcessImageInformation,
    ProcessCycleTime,
    ProcessPagePriority,
    ProcessInstrumentationCallback,
    MaxProcessInfoClass
} PROCESSINFOCLASS;

typedef enum _THREADINFOCLASS
{
    ThreadBasicInformation,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending,
    ThreadHideFromDebugger,
    ThreadBreakOnTermination,
    ThreadSwitchLegacyState,
    ThreadIsTerminated,
    ThreadLastSystemCall,
    ThreadIoPriority,
    ThreadCycleTime,
    ThreadPagePriority,
    ThreadActualBasePriority,
    ThreadTebInformation,
    ThreadCSwitchMon,
    MaxThreadInfoClass
} THREADINFOCLASS;

#else

typedef enum _PSPROCESSPRIORITYMODE
{
    PsProcessPriorityForeground,
    PsProcessPriorityBackground,
    PsProcessPrioritySpinning
} PSPROCESSPRIORITYMODE;

typedef enum _JOBOBJECTINFOCLASS
{
    JobObjectBasicAccountingInformation = 1,
    JobObjectBasicLimitInformation,
    JobObjectBasicProcessIdList,
    JobObjectBasicUIRestrictions,
    JobObjectSecurityLimitInformation,
    JobObjectEndOfJobTimeInformation,
    JobObjectAssociateCompletionPortInformation,
    JobObjectBasicAndIoAccountingInformation,
    JobObjectExtendedLimitInformation,
    JobObjectJobSetInformation,
    MaxJobObjectInfoClass
} JOBOBJECTINFOCLASS;

//
// Power Event Events for Win32K Power Event Callback
//
typedef enum _PSPOWEREVENTTYPE
{
    PsW32FullWake = 0,
    PsW32EventCode = 1,
    PsW32PowerPolicyChanged = 2,
    PsW32SystemPowerState = 3,
    PsW32SystemTime = 4,
    PsW32DisplayState = 5,
    PsW32CapabilitiesChanged = 6,
    PsW32SetStateFailed = 7,
    PsW32GdiOff = 8,
    PsW32GdiOn = 9,
    PsW32GdiPrepareResumeUI = 10,
    PsW32GdiOffRequest = 11,
    PsW32MonitorOff = 12,
} PSPOWEREVENTTYPE;

//
// Power State Tasks for Win32K Power State Callback
//
typedef enum _POWERSTATETASK
{
    PowerState_BlockSessionSwitch = 0,
    PowerState_Init = 1,
    PowerState_QueryApps = 2,
    PowerState_QueryServices = 3,
    PowerState_QueryAppsFailed = 4,
    PowerState_QueryServicesFailed = 5,
    PowerState_SuspendApps = 6,
    PowerState_SuspendServices = 7,
    PowerState_ShowUI = 8,
    PowerState_NotifyWL = 9,
    PowerState_ResumeApps = 10,
    PowerState_ResumeServices = 11,
    PowerState_UnBlockSessionSwitch = 12,
    PowerState_End = 13,
    PowerState_BlockInput = 14,
    PowerState_UnblockInput = 15,
} POWERSTATETASK;

//
// Win32K Job Callback Types
//
typedef enum _PSW32JOBCALLOUTTYPE
{
   PsW32JobCalloutSetInformation = 0,
   PsW32JobCalloutAddProcess = 1,
   PsW32JobCalloutTerminate = 2,
} PSW32JOBCALLOUTTYPE;

//
// Win32K Thread Callback Types
//
typedef enum _PSW32THREADCALLOUTTYPE
{
    PsW32ThreadCalloutInitialize,
    PsW32ThreadCalloutExit,
} PSW32THREADCALLOUTTYPE;

//
// Declare empty structure definitions so that they may be referenced by
// routines before they are defined
//
struct _W32THREAD;
struct _W32PROCESS;
//struct _ETHREAD;
struct _WIN32_POWEREVENT_PARAMETERS;
struct _WIN32_POWERSTATE_PARAMETERS;
struct _WIN32_JOBCALLOUT_PARAMETERS;
struct _WIN32_OPENMETHOD_PARAMETERS;
struct _WIN32_OKAYTOCLOSEMETHOD_PARAMETERS;
struct _WIN32_CLOSEMETHOD_PARAMETERS;
struct _WIN32_DELETEMETHOD_PARAMETERS;
struct _WIN32_PARSEMETHOD_PARAMETERS;

//
// Win32K Process and Thread Callbacks
//
typedef
NTSTATUS
(NTAPI *PKWIN32_PROCESS_CALLOUT)(
    struct _EPROCESS *Process,
    BOOLEAN Create
);

typedef
NTSTATUS
(NTAPI *PKWIN32_THREAD_CALLOUT)(
    struct _ETHREAD *Thread,
    PSW32THREADCALLOUTTYPE Type
);

typedef
NTSTATUS
(NTAPI *PKWIN32_GLOBALATOMTABLE_CALLOUT)(
    VOID
);

typedef
NTSTATUS
(NTAPI *PKWIN32_POWEREVENT_CALLOUT)(
    struct _WIN32_POWEREVENT_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PKWIN32_POWERSTATE_CALLOUT)(
    struct _WIN32_POWERSTATE_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PKWIN32_JOB_CALLOUT)(
    struct _WIN32_JOBCALLOUT_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PGDI_BATCHFLUSH_ROUTINE)(
    VOID
);

typedef
NTSTATUS
(NTAPI *PKWIN32_OPENMETHOD_CALLOUT)(
    struct _WIN32_OPENMETHOD_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PKWIN32_OKTOCLOSEMETHOD_CALLOUT)(
    struct _WIN32_OKAYTOCLOSEMETHOD_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PKWIN32_CLOSEMETHOD_CALLOUT)(
    struct _WIN32_CLOSEMETHOD_PARAMETERS *Parameters
);

typedef
VOID
(NTAPI *PKWIN32_DELETEMETHOD_CALLOUT)(
    struct _WIN32_DELETEMETHOD_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PKWIN32_PARSEMETHOD_CALLOUT)(
    struct _WIN32_PARSEMETHOD_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PKWIN32_WIN32DATACOLLECTION_CALLOUT)(
    struct _EPROCESS *Process,
    PVOID Callback,
    PVOID Context
);

//
// Lego Callback
//
typedef
VOID
(NTAPI *PLEGO_NOTIFY_ROUTINE)(
    IN PKTHREAD Thread
);

#endif

typedef NTSTATUS
(NTAPI *PPOST_PROCESS_INIT_ROUTINE)(
    VOID
);

//
// Descriptor Table Entry Definition
//
#define _DESCRIPTOR_TABLE_ENTRY_DEFINED
typedef struct _DESCRIPTOR_TABLE_ENTRY
{
    ULONG Selector;
    LDT_ENTRY Descriptor;
} DESCRIPTOR_TABLE_ENTRY, *PDESCRIPTOR_TABLE_ENTRY;

//
// PEB Lock Routine
//
typedef VOID
(NTAPI *PPEBLOCKROUTINE)(
    PVOID PebLock
);

//
// PEB Free Block Descriptor
//
typedef struct _PEB_FREE_BLOCK
{
    struct _PEB_FREE_BLOCK* Next;
    ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

//
// Process Environment Block (PEB)
//
typedef struct _PEB
{
    UCHAR InheritedAddressSpace;
    UCHAR ReadImageFileExecOptions;
    UCHAR BeingDebugged;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    struct
    {
        UCHAR ImageUsesLargePages:1;
        UCHAR IsProtectedProcess:1;
        UCHAR IsLegacyProcess:1;
        UCHAR SpareBits:5;
    };
#else
    BOOLEAN SpareBool;
#endif
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    PPEB_LDR_DATA Ldr;
    struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters;
    PVOID SubSystemData;
    PVOID ProcessHeap;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    struct _RTL_CRITICAL_SECTION *FastPebLock;
    PVOID AltThunkSListPtr;
    PVOID IFEOKey;
    ULONG Spare;
    union
    {
        PVOID* KernelCallbackTable;
        PVOID UserSharedInfoPtr;
    };
    ULONG SystemReserved[1];
    ULONG SpareUlong;
#else
    PVOID FastPebLock;
    PPEBLOCKROUTINE FastPebLockRoutine;
    PPEBLOCKROUTINE FastPebUnlockRoutine;
    ULONG EnvironmentUpdateCount;
    PVOID* KernelCallbackTable;
    PVOID EventLogSection;
    PVOID EventLog;
#endif
    PPEB_FREE_BLOCK FreeList;
    ULONG TlsExpansionCounter;
    PVOID TlsBitmap;
    ULONG TlsBitmapBits[0x2];
    PVOID ReadOnlySharedMemoryBase;
    PVOID ReadOnlySharedMemoryHeap;
    PVOID* ReadOnlyStaticServerData;
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCaseTableData;
    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;
    LARGE_INTEGER CriticalSectionTimeout;
    ULONG HeapSegmentReserve;
    ULONG HeapSegmentCommit;
    ULONG HeapDeCommitTotalFreeThreshold;
    ULONG HeapDeCommitFreeBlockThreshold;
    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    PVOID* ProcessHeaps;
    PVOID GdiSharedHandleTable;
    PVOID ProcessStarterHelper;
    PVOID GdiDCAttributeList;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    struct _RTL_CRITICAL_SECTION *LoaderLock;
#else
    PVOID LoaderLock;
#endif
    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    USHORT OSBuildNumber;
    USHORT OSCSDVersion;
    ULONG OSPlatformId;
    ULONG ImageSubSystem;
    ULONG ImageSubSystemMajorVersion;
    ULONG ImageSubSystemMinorVersion;
    ULONG ImageProcessAffinityMask;
    ULONG GdiHandleBuffer[0x22];
    PPOST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
    struct _RTL_BITMAP *TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[0x20];
    ULONG SessionId;
#if (NTDDI_VERSION >= NTDDI_WINXP)
    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    PVOID pShimData;
    PVOID AppCompatInfo;
    UNICODE_STRING CSDVersion;
    struct _ACTIVATION_CONTEXT_DATA *ActivationContextData;
    struct _ASSEMBLY_STORAGE_MAP *ProcessAssemblyStorageMap;
    struct _ACTIVATION_CONTEXT_DATA *SystemDefaultActivationContextData;
    struct _ASSEMBLY_STORAGE_MAP *SystemAssemblyStorageMap;
    ULONG MinimumStackCommit;
#endif
#if (NTDDI_VERSION >= NTDDI_WS03)
    PVOID *FlsCallback;
    LIST_ENTRY FlsListHead;
    struct _RTL_BITMAP *FlsBitmap;
    ULONG FlsBitmapBits[4];
    ULONG FlsHighIndex;
#endif
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PVOID WerRegistrationData;
    PVOID WerShipAssertPtr;
#endif
} PEB, *PPEB;

//
// GDI Batch Descriptor
//
typedef struct _GDI_TEB_BATCH
{
    ULONG Offset;
    ULONG HDC;
    ULONG Buffer[0x136];
} GDI_TEB_BATCH, *PGDI_TEB_BATCH;

//
// Window Client Information structure
//
typedef struct _W32CLTINFO_TEB
{
    ULONG Win32ClientInfo0[2];
    ULONG ulWindowsVersion;
    ULONG ulAppCompatFlags;
    ULONG ulAppCompatFlags2;    
    ULONG Win32ClientInfo1[5];
    HWND  hWND;
    PVOID pvWND;
    ULONG Win32ClientInfo2[50];
} W32CLTINFO_TEB, *PW32CLTINFO_TEB;

//
// Initial TEB
//
typedef struct _INITIAL_TEB
{
    PVOID PreviousStackBase;
    PVOID PreviousStackLimit;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID AllocatedStackBase;
} INITIAL_TEB, *PINITIAL_TEB;

//
// TEB Active Frame Structures
//
typedef struct _TEB_ACTIVE_FRAME_CONTEXT 
{
    ULONG Flags;
    LPSTR FrameName;
} TEB_ACTIVE_FRAME_CONTEXT, *PTEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME
{
    ULONG Flags;
    struct _TEB_ACTIVE_FRAME *Previous;
    PTEB_ACTIVE_FRAME_CONTEXT Context;
} TEB_ACTIVE_FRAME, *PTEB_ACTIVE_FRAME;

//
// Thread Environment Block (TEB)
//
typedef struct _TEB
{
    NT_TIB Tib;
    PVOID EnvironmentPointer;
    CLIENT_ID Cid;
    PVOID ActiveRpcHandle;
    PVOID ThreadLocalStoragePointer;
    struct _PEB *ProcessEnvironmentBlock;
    ULONG LastErrorValue;
    ULONG CountOfOwnedCriticalSections;
    PVOID CsrClientThread;
    struct _W32THREAD* Win32ThreadInfo;
    ULONG User32Reserved[0x1A];
    ULONG UserReserved[5];
    PVOID WOW32Reserved;
    LCID CurrentLocale;
    ULONG FpSoftwareStatusRegister;
    PVOID SystemReserved1[0x36];
    LONG ExceptionCode;
    struct _ACTIVATION_CONTEXT_STACK *ActivationContextStackPointer;
    UCHAR SpareBytes1[0x24];
    ULONG TxFsContext;
    GDI_TEB_BATCH GdiTebBatch;
    CLIENT_ID RealClientId;
    PVOID GdiCachedProcessHandle;
    ULONG GdiClientPID;
    ULONG GdiClientTID;
    PVOID GdiThreadLocalInfo;
    W32CLTINFO_TEB Win32ClientInfo;
    PVOID glDispatchTable[0xE9];
    ULONG glReserved1[0x1D];
    PVOID glReserved2;
    PVOID glSectionInfo;
    PVOID glSection;
    PVOID glTable;
    PVOID glCurrentRC;
    PVOID glContext;
    NTSTATUS LastStatusValue;
    UNICODE_STRING StaticUnicodeString;
    WCHAR StaticUnicodeBuffer[0x105];
    PVOID DeallocationStack;
    PVOID TlsSlots[0x40];
    LIST_ENTRY TlsLinks;
    PVOID Vdm;
    PVOID ReservedForNtRpc;
    PVOID DbgSsReserved[0x2];
    ULONG HardErrorDisabled;
    PVOID Instrumentation[9];
    GUID ActivityId;
    PVOID SubProcessTag;
    PVOID EtwTraceData;
    PVOID WinSockData;
    ULONG GdiBatchCount;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    BOOLEAN SpareBool0;
    BOOLEAN SpareBool1;
    BOOLEAN SpareBool2;
#else
    BOOLEAN InDbgPrint;
    BOOLEAN FreeStackOnTermination;
    BOOLEAN HasFiberData;
#endif
    UCHAR IdealProcessor;
    ULONG GuaranteedStackBytes;
    PVOID ReservedForPerf;
    PVOID ReservedForOle;
    ULONG WaitingOnLoaderLock;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PVOID SavedPriorityState;
#else
    ULONG SparePointer1;
#endif
    ULONG SoftPatchPtr1;
    ULONG SoftPatchPtr2;
    PVOID *TlsExpansionSlots;
    ULONG ImpersonationLocale;
    ULONG IsImpersonating;
    PVOID NlsCache;
    PVOID pShimData;
    ULONG HeapVirualAffinity;
    PVOID CurrentTransactionHandle;
    PTEB_ACTIVE_FRAME ActiveFrame;
#if (NTDDI_VERSION >= NTDDI_WS03)
    PVOID FlsData;
#endif
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PVOID PreferredLangauges;
    PVOID UserPrefLanguages;
    PVOID MergedPrefLanguages;
    ULONG MuiImpersonation;
    union
    {
        struct
        {
            USHORT SpareCrossTebFlags:16;
        };
        USHORT CrossTebFlags;
    };
    union
    {
        struct
        {
            USHORT DbgSafeThunkCall:1;
            USHORT DbgInDebugPrint:1;
            USHORT DbgHasFiberData:1;
            USHORT DbgSkipThreadAttach:1;
            USHORT DbgWerInShipAssertCode:1;
            USHORT DbgIssuedInitialBp:1;
            USHORT DbgClonedThread:1;
            USHORT SpareSameTebBits:9;
        };
        USHORT SameTebFlags;
    };
    PVOID TxnScopeEntercallback;
    PVOID TxnScopeExitCAllback;
    PVOID TxnScopeContext;
    ULONG LockCount;
    ULONG ProcessRundown;
    ULONGLONG LastSwitchTime;
    ULONGLONG TotalSwitchOutTime;
    LARGE_INTEGER WaitReasonBitMap;
#else
    UCHAR SafeThunkCall;
    UCHAR BooleanSpare[3];
#endif
} TEB, *PTEB;

#ifdef NTOS_MODE_USER

//
// Process Information Structures for NtQueryProcessInformation
//
typedef struct _PROCESS_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    KPRIORITY BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION,*PPROCESS_BASIC_INFORMATION;

typedef struct _PROCESS_ACCESS_TOKEN
{
    HANDLE Token;
    HANDLE Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

typedef struct _PROCESS_DEVICEMAP_INFORMATION
{
    union
    {
        struct
        {
            HANDLE DirectoryHandle;
        } Set;
        struct
        {
            ULONG DriveMap;
            UCHAR DriveType[32];
        } Query;
    };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

typedef struct _KERNEL_USER_TIMES
{
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

typedef struct _PROCESS_SESSION_INFORMATION
{
    ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

#endif

typedef struct _PROCESS_PRIORITY_CLASS
{
    BOOLEAN Foreground;
    UCHAR PriorityClass;
} PROCESS_PRIORITY_CLASS, *PPROCESS_PRIORITY_CLASS;

//
// Thread Information Structures for NtQueryProcessInformation
//
typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PVOID TebBaseAddress;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

#ifndef NTOS_MODE_USER

//
// Job Set Array
//
typedef struct _JOB_SET_ARRAY
{
    HANDLE JobHandle;
    ULONG MemberLevel;
    ULONG Flags;
} JOB_SET_ARRAY, *PJOB_SET_ARRAY;

//
// EPROCESS Quota Structures
//
typedef struct _EPROCESS_QUOTA_ENTRY
{
    SIZE_T Usage;
    SIZE_T Limit;
    SIZE_T Peak;
    SIZE_T Return;
} EPROCESS_QUOTA_ENTRY, *PEPROCESS_QUOTA_ENTRY;

typedef struct _EPROCESS_QUOTA_BLOCK
{
    EPROCESS_QUOTA_ENTRY QuotaEntry[3];
    LIST_ENTRY QuotaList;
    ULONG ReferenceCount;
    ULONG ProcessCount;
} EPROCESS_QUOTA_BLOCK, *PEPROCESS_QUOTA_BLOCK;

//
// Process Pagefault History
//
typedef struct _PAGEFAULT_HISTORY
{
    ULONG CurrentIndex;
    ULONG MapIndex;
    KSPIN_LOCK SpinLock;
    PVOID Reserved;
    PROCESS_WS_WATCH_INFORMATION WatchInfo[1];
} PAGEFAULT_HISTORY, *PPAGEFAULT_HISTORY;

//
// Process Impersonation Information
//
typedef struct _PS_IMPERSONATION_INFORMATION
{
    PACCESS_TOKEN Token;
    BOOLEAN CopyOnOpen;
    BOOLEAN EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
} PS_IMPERSONATION_INFORMATION, *PPS_IMPERSONATION_INFORMATION;

//
// Process Termination Port
//
typedef struct _TERMINATION_PORT
{
    struct _TERMINATION_PORT *Next;
    PVOID Port;
} TERMINATION_PORT, *PTERMINATION_PORT;

//
// Per-Process APC Rate Limiting
//
typedef struct _PSP_RATE_APC
{
    union
    {
        SINGLE_LIST_ENTRY NextApc;
        ULONGLONG ExcessCycles;
    };
    ULONGLONG TargetGEneration;
    KAPC RateApc;
} PSP_RATE_APC, *PPSP_RATE_APC;

//
// Executive Thread (ETHREAD)
//
typedef struct _ETHREAD
{
    KTHREAD Tcb;
    PVOID Padding;
    LARGE_INTEGER CreateTime;
    union
    {
        LARGE_INTEGER ExitTime;
        LIST_ENTRY LpcReplyChain;
        LIST_ENTRY KeyedWaitChain;
    };
    union
    {
        NTSTATUS ExitStatus;
        PVOID OfsChain;
    };
    LIST_ENTRY PostBlockList;
    union
    {
        struct _TERMINATION_PORT *TerminationPort;
        struct _ETHREAD *ReaperLink;
        PVOID KeyedWaitValue;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
        PVOID Win32StartParameter;
#endif
    };
    KSPIN_LOCK ActiveTimerListLock;
    LIST_ENTRY ActiveTimerListHead;
    CLIENT_ID Cid;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    KSEMAPHORE KeyedWaitSemaphore;
#else
    union
    {
        KSEMAPHORE LpcReplySemaphore;
        KSEMAPHORE KeyedReplySemaphore;
    };
    union
    {
        PVOID LpcReplyMessage;
        PVOID LpcWaitingOnPort;
    };
#endif
    PPS_IMPERSONATION_INFORMATION ImpersonationInfo;
    LIST_ENTRY IrpList;
    ULONG TopLevelIrp;
    PDEVICE_OBJECT DeviceToVerify;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PPSP_RATE_APC RateControlApc;
#else
    struct _EPROCESS *ThreadsProcess;
#endif
    PVOID Win32StartAddress;
    union
    {
        PKSTART_ROUTINE StartAddress;
        ULONG LpcReceivedMessageId;
    };
    LIST_ENTRY ThreadListEntry;
    EX_RUNDOWN_REF RundownProtect;
    EX_PUSH_LOCK ThreadLock;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    ULONG LpcReplyMessageId;
#endif
    ULONG ReadClusterSize;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG SpareUlong0;
#else
    ACCESS_MASK GrantedAccess;
#endif
    union
    {
        struct
        {
           ULONG Terminated:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
           ULONG ThreadInserted:1;
#else
           ULONG DeadThread:1;
#endif
           ULONG HideFromDebugger:1;
           ULONG ActiveImpersonationInfo:1;
           ULONG SystemThread:1;
           ULONG HardErrorsAreDisabled:1;
           ULONG BreakOnTermination:1;
           ULONG SkipCreationMsg:1;
           ULONG SkipTerminationMsg:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
           ULONG CreateMsgSent:1;
           ULONG ThreadIoPriority:3;
           ULONG ThreadPagePriority:3;
           ULONG PendingRatecontrol:1;
#endif
        };
        ULONG CrossThreadFlags;
    };
    union
    {
        struct
        {
           ULONG ActiveExWorker:1;
           ULONG ExWorkerCanWaitUser:1;
           ULONG MemoryMaker:1;
           ULONG KeyedEventInUse:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
           ULONG RateApcState:2;
#endif
        };
        ULONG SameThreadPassiveFlags;
    };
    union
    {
        struct
        {
           ULONG LpcReceivedMsgIdValid:1;
           ULONG LpcExitThreadCalled:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
           ULONG Spare:1;
#else
           ULONG AddressSpaceOwner:1;
#endif
           ULONG OwnsProcessWorkingSetExclusive:1;
           ULONG OwnsProcessWorkingSetShared:1;
           ULONG OwnsSystemWorkingSetExclusive:1;
           ULONG OwnsSystemWorkingSetShared:1;
           ULONG OwnsSessionWorkingSetExclusive:1;
           ULONG OwnsSessionWorkingSetShared:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
           ULONG SupressSymbolLoad:1;
           ULONG Spare1:3;
           ULONG PriorityRegionActive:4;
#else
           ULONG ApcNeeded:1;
#endif
        };
        ULONG SameThreadApcFlags;
    };
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    UCHAR CacheManagerActive;
#else
    UCHAR ForwardClusterOnly;
#endif
    UCHAR DisablePageFaultClustering;
    UCHAR ActiveFaultCount;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG AlpcMessageId;
    union
    {
        PVOID AlpcMessage;
        ULONG AlpcReceiveAttributeSet;
    };
    LIST_ENTRY AlpcWaitListEntry;
    KSEMAPHORE AlpcWaitSemaphore;
    ULONG CacheManagerCount;
#endif
} ETHREAD, *PETHREAD;

//
// Executive Process (EPROCESS)
//
typedef struct _EPROCESS
{
    KPROCESS Pcb;
    EX_PUSH_LOCK ProcessLock;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    EX_RUNDOWN_REF RundownProtect;
    HANDLE UniqueProcessId;
    LIST_ENTRY ActiveProcessLinks;
    ULONG QuotaUsage[3];
    ULONG QuotaPeak[3];
    ULONG CommitCharge;
    ULONG PeakVirtualSize;
    ULONG VirtualSize;
    LIST_ENTRY SessionProcessLinks;
    PVOID DebugPort;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    union
    {
        PVOID ExceptionPortData;
        ULONG ExceptionPortValue;
        UCHAR ExceptionPortState:3;
    };
#else
    PVOID ExceptionPort;
#endif
    PHANDLE_TABLE ObjectTable;
    EX_FAST_REF Token;
    ULONG WorkingSetPage;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    EX_PUSH_LOCK AddressCreationLock;
    PETHREAD RotateInProgress;
#else
    FAST_MUTEX AddressCreationLock; // FIXME: FAST_MUTEX for XP, KGUARDED_MUTEX for 2K3
    KSPIN_LOCK HyperSpaceLock;
#endif
    PETHREAD ForkInProgress;
    ULONG HardwareTrigger;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PMM_AVL_TABLE PhysicalVadroot;
#else
    MM_AVL_TABLE PhysicalVadroot;
#endif
    PVOID CloneRoot;
    ULONG NumberOfPrivatePages;
    ULONG NumberOfLockedPages;
    PVOID *Win32Process;
    struct _EJOB *Job;
    PVOID SectionObject;
    PVOID SectionBaseAddress;
    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    PPAGEFAULT_HISTORY WorkingSetWatch;
    PVOID Win32WindowStation;
    HANDLE InheritedFromUniqueProcessId;
    PVOID LdtInformation;
    PVOID VadFreeHint;
    PVOID VdmObjects;
    PVOID DeviceMap;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PVOID EtwDataSource;
    PVOID FreeTebHint;
#else
    PVOID Spare0[3];
#endif
    union
    {
        HARDWARE_PTE PagedirectoryPte;
        ULONGLONG Filler;
    };
    ULONG Session;
    CHAR ImageFileName[16];
    LIST_ENTRY JobLinks;
    PVOID LockedPagesList;
    LIST_ENTRY ThreadListHead;
    PVOID SecurityPort;
    PVOID PaeTop;
    ULONG ActiveThreads;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG ImagePathHash;
#else
    ACCESS_MASK GrantedAccess;
#endif
    ULONG DefaultHardErrorProcessing;
    NTSTATUS LastThreadExitStatus;
    struct _PEB* Peb;
    EX_FAST_REF PrefetchTrace;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
    ULONG CommitChargeLimit;
    ULONG CommitChargePeak;
    PVOID AweInfo;
    SE_AUDIT_PROCESS_CREATION_INFO SeAuditProcessCreationInfo;
    MMSUPPORT Vm;
    LIST_ENTRY MmProcessLinks;
    ULONG ModifiedPageCount;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    union
    {
        struct
        {
            ULONG JobNotReallyActive:1;
            ULONG AccountingFolded:1;
            ULONG NewProcessReported:1;
            ULONG ExitProcessReported:1;
            ULONG ReportCommitChanges:1;
            ULONG LastReportMemory:1;
            ULONG ReportPhysicalPageChanges:1;
            ULONG HandleTableRundown:1;
            ULONG NeedsHandleRundown:1;
            ULONG RefTraceEnabled:1;
            ULONG NumaAware:1;
            ULONG ProtectedProcess:1;
            ULONG DefaultPagePriority:3;
            ULONG ProcessDeleteSelf:1;
            ULONG ProcessVerifierTarget:1;
        };
        ULONG Flags2;
    };
#else
    ULONG JobStatus;
#endif
    union
    {
        struct
        {
            ULONG CreateReported:1;
            ULONG NoDebugInherit:1;
            ULONG ProcessExiting:1;
            ULONG ProcessDelete:1;
            ULONG Wow64SplitPages:1;
            ULONG VmDeleted:1;
            ULONG OutswapEnabled:1;
            ULONG Outswapped:1;
            ULONG ForkFailed:1;
            ULONG Wow64VaSpace4Gb:1;
            ULONG AddressSpaceInitialized:2;
            ULONG SetTimerResolution:1;
            ULONG BreakOnTermination:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
            ULONG DeprioritizeViews:1;
#else
            ULONG SessionCreationUnderway:1;
#endif
            ULONG WriteWatch:1;
            ULONG ProcessInSession:1;
            ULONG OverrideAddressSpace:1;
            ULONG HasAddressSpace:1;
            ULONG LaunchPrefetched:1;
            ULONG InjectInpageErrors:1;
            ULONG VmTopDown:1;
            ULONG ImageNotifyDone:1;
            ULONG PdeUpdateNeeded:1;
            ULONG VdmAllowed:1;
            ULONG SmapAllowed:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
            ULONG ProcessInserted:1;
#else
            ULONG CreateFailed:1;
#endif
            ULONG DefaultIoPriority:3;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
            ULONG SparePsFlags1:2;
#else
            ULONG Spare1:1;
            ULONG Spare2:1;
#endif
        };
        ULONG Flags;
    };
    NTSTATUS ExitStatus;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    USHORT Spare7;
#else
    USHORT NextPageColor;
#endif
    union
    {
        struct
        {
            UCHAR SubSystemMinorVersion;
            UCHAR SubSystemMajorVersion;
        };
        USHORT SubSystemVersion;
    };
    UCHAR PriorityClass;
    MM_AVL_TABLE VadRoot;
    ULONG Cookie;
} EPROCESS, *PEPROCESS;

//
// Job Token Filter Data
//
#include <pshpack1.h>
typedef struct _PS_JOB_TOKEN_FILTER
{
    ULONG CapturedSidCount;
    PSID_AND_ATTRIBUTES CapturedSids;
    ULONG CapturedSidsLength;
    ULONG CapturedGroupCount;
    PSID_AND_ATTRIBUTES CapturedGroups;
    ULONG CapturedGroupsLength;
    ULONG CapturedPrivilegeCount;
    PLUID_AND_ATTRIBUTES CapturedPrivileges;
    ULONG CapturedPrivilegesLength;
} PS_JOB_TOKEN_FILTER, *PPS_JOB_TOKEN_FILTER;

//
// Executive Job (EJOB)
//
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
    ULONG TotalPageFaultCount;
    ULONG TotalProcesses;
    ULONG ActiveProcesses;
    ULONG TotalTerminatedProcesses;
    LARGE_INTEGER PerProcessUserTimeLimit;
    LARGE_INTEGER PerJobUserTimeLimit;
    ULONG LimitFlags;
    ULONG MinimumWorkingSetSize;
    ULONG MaximumWorkingSetSize;
    ULONG ActiveProcessLimit;
    ULONG Affinity;
    UCHAR PriorityClass;
    ULONG UIRestrictionsClass;
    ULONG SecurityLimitFlags;
    PVOID Token;
    PPS_JOB_TOKEN_FILTER Filter;
    ULONG EndOfJobTimeAction;
    PVOID CompletionPort;
    PVOID CompletionKey;
    ULONG SessionId;
    ULONG SchedulingClass;
    ULONGLONG ReadOperationCount;
    ULONGLONG WriteOperationCount;
    ULONGLONG OtherOperationCount;
    ULONGLONG ReadTransferCount;
    ULONGLONG WriteTransferCount;
    ULONGLONG OtherTransferCount;
    IO_COUNTERS IoInfo;
    ULONG ProcessMemoryLimit;
    ULONG JobMemoryLimit;
    ULONG PeakProcessMemoryUsed;
    ULONG PeakJobMemoryUsed;
    ULONG CurrentJobMemoryUsed;
#if (NTDDI_VERSION == NTDDI_WINXP)
    FAST_MUTEX MemoryLimitsLock;
#elif (NTDDI_VERSION == NTDDI_WS03)
    KGUARDED_MUTEX MemoryLimitsLock;
#elif (NTDDI_VERSION >= NTDDI_LONGHORN)
    EX_PUSH_LOCK MemoryLimitsLock;
#endif
    LIST_ENTRY JobSetLinks;
    ULONG MemberLevel;
    ULONG JobFlags;
} EJOB, *PEJOB;
#include <poppack.h>

//
// Win32K Callback Registration Data
//
typedef struct _WIN32_POWEREVENT_PARAMETERS
{
    PSPOWEREVENTTYPE EventNumber;
    ULONG Code;
} WIN32_POWEREVENT_PARAMETERS, *PWIN32_POWEREVENT_PARAMETERS;

typedef struct _WIN32_POWERSTATE_PARAMETERS
{
    UCHAR Promotion;
    POWER_ACTION SystemAction;
    SYSTEM_POWER_STATE MinSystemState;
    ULONG Flags;
    POWERSTATETASK PowerStateTask;
} WIN32_POWERSTATE_PARAMETERS, *PWIN32_POWERSTATE_PARAMETERS;

typedef struct _WIN32_JOBCALLOUT_PARAMETERS
{
    PVOID Job;
    PSW32JOBCALLOUTTYPE CalloutType;
    PVOID Data;
} WIN32_JOBCALLOUT_PARAMETERS, *PWIN32_JOBCALLOUT_PARAMETERS;

typedef struct _WIN32_OPENMETHOD_PARAMETERS
{
    OB_OPEN_REASON OpenReason;
    PEPROCESS Process;
    PVOID Object;
    ULONG GrantedAccess;
    ULONG HandleCount;
} WIN32_OPENMETHOD_PARAMETERS, *PWIN32_OPENMETHOD_PARAMETERS;

typedef struct _WIN32_OKAYTOCLOSEMETHOD_PARAMETERS
{
    PEPROCESS Process;
    PVOID Object;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
} WIN32_OKAYTOCLOSEMETHOD_PARAMETERS, *PWIN32_OKAYTOCLOSEMETHOD_PARAMETERS;

typedef struct _WIN32_CLOSEMETHOD_PARAMETERS
{
    PEPROCESS Process;
    PVOID Object;
    ACCESS_MASK AccessMask;
    ULONG ProcessHandleCount;
    ULONG SystemHandleCount;
} WIN32_CLOSEMETHOD_PARAMETERS, *PWIN32_CLOSEMETHOD_PARAMETERS;

typedef struct _WIN32_DELETEMETHOD_PARAMETERS
{
    PVOID Object;
} WIN32_DELETEMETHOD_PARAMETERS, *PWIN32_DELETEMETHOD_PARAMETERS;

typedef struct _WIN32_PARSEMETHOD_PARAMETERS
{
    PVOID ParseObject;
    PVOID ObjectType;
    PACCESS_STATE AccessState;
    KPROCESSOR_MODE AccessMode;
    ULONG Attributes;
    OUT PUNICODE_STRING CompleteName;
    PUNICODE_STRING RemainingName;
    PVOID Context;
    PSECURITY_QUALITY_OF_SERVICE SecurityQos;
    PVOID *Object;
} WIN32_PARSEMETHOD_PARAMETERS, *PWIN32_PARSEMETHOD_PARAMETERS;

typedef struct _WIN32_CALLOUTS_FPNS
{
    PKWIN32_PROCESS_CALLOUT ProcessCallout;
    PKWIN32_THREAD_CALLOUT ThreadCallout;
    PKWIN32_GLOBALATOMTABLE_CALLOUT GlobalAtomTableCallout;
    PKWIN32_POWEREVENT_CALLOUT PowerEventCallout;
    PKWIN32_POWERSTATE_CALLOUT PowerStateCallout;
    PKWIN32_JOB_CALLOUT JobCallout;
    PGDI_BATCHFLUSH_ROUTINE BatchFlushRoutine;
    PKWIN32_OPENMETHOD_CALLOUT DesktopOpenProcedure;
    PKWIN32_OKTOCLOSEMETHOD_CALLOUT DesktopOkToCloseProcedure;
    PKWIN32_CLOSEMETHOD_CALLOUT DesktopCloseProcedure;
    PKWIN32_DELETEMETHOD_CALLOUT DesktopDeleteProcedure;
    PKWIN32_OKTOCLOSEMETHOD_CALLOUT WindowStationOkToCloseProcedure;
    PKWIN32_CLOSEMETHOD_CALLOUT WindowStationCloseProcedure;
    PKWIN32_DELETEMETHOD_CALLOUT WindowStationDeleteProcedure;
    PKWIN32_PARSEMETHOD_CALLOUT WindowStationParseProcedure;
    PKWIN32_OPENMETHOD_CALLOUT WindowStationOpenProcedure;
    PKWIN32_WIN32DATACOLLECTION_CALLOUT Win32DataCollectionProcedure;
} WIN32_CALLOUTS_FPNS, *PWIN32_CALLOUTS_FPNS;

#endif // !NTOS_MODE_USER

#endif // _PSTYPES_H

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
#include <rtltypes.h>
#ifndef NTOS_MODE_USER
#include <extypes.h>
#include <setypes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NTOS_MODE_USER

//
// Kernel Exported Object Types
//
extern POBJECT_TYPE NTSYSAPI PsJobType;

#endif // !NTOS_MODE_USER

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
#define FLG_APPLICATION_VERIFIER                0x00000100
#define FLG_POOL_ENABLE_TAGGING                 0x00000400
#define FLG_HEAP_ENABLE_TAGGING                 0x00000800
#define FLG_USER_STACK_TRACE_DB                 0x00001000
#define FLG_KERNEL_STACK_TRACE_DB               0x00002000
#define FLG_MAINTAIN_OBJECT_TYPELIST            0x00004000
#define FLG_HEAP_ENABLE_TAG_BY_DLL              0x00008000
#define FLG_DISABLE_STACK_EXTENSION             0x00010000
#define FLG_ENABLE_CSRDEBUG                     0x00020000
#define FLG_ENABLE_KDEBUG_SYMBOL_LOAD           0x00040000
#define FLG_DISABLE_PAGE_KERNEL_STACKS          0x00080000
#if (NTDDI_VERSION < NTDDI_WINXP)
#define FLG_HEAP_ENABLE_CALL_TRACING            0x00100000
#else
#define FLG_ENABLE_SYSTEM_CRIT_BREAKS           0x00100000
#endif
#define FLG_HEAP_DISABLE_COALESCING             0x00200000
#define FLG_ENABLE_CLOSE_EXCEPTIONS             0x00400000
#define FLG_ENABLE_EXCEPTION_LOGGING            0x00800000
#define FLG_ENABLE_HANDLE_TYPE_TAGGING          0x01000000
#define FLG_HEAP_PAGE_ALLOCS                    0x02000000
#define FLG_DEBUG_INITIAL_COMMAND_EX            0x04000000
#define FLG_DISABLE_DEBUG_PROMPTS               0x08000000 // ReactOS-specific
#define FLG_VALID_BITS                          0x0FFFFFFF

//
// Flags for NtCreateProcessEx
//
#define PROCESS_CREATE_FLAGS_BREAKAWAY              0x00000001
#define PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT       0x00000002
#define PROCESS_CREATE_FLAGS_INHERIT_HANDLES        0x00000004
#define PROCESS_CREATE_FLAGS_OVERRIDE_ADDRESS_SPACE 0x00000008
#define PROCESS_CREATE_FLAGS_LARGE_PAGES            0x00000010
#define PROCESS_CREATE_FLAGS_ALL_LARGE_PAGE_FLAGS   PROCESS_CREATE_FLAGS_LARGE_PAGES
#define PROCESS_CREATE_FLAGS_LEGAL_MASK             (PROCESS_CREATE_FLAGS_BREAKAWAY | \
                                                     PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT | \
                                                     PROCESS_CREATE_FLAGS_INHERIT_HANDLES | \
                                                     PROCESS_CREATE_FLAGS_OVERRIDE_ADDRESS_SPACE | \
                                                     PROCESS_CREATE_FLAGS_ALL_LARGE_PAGE_FLAGS)

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
// Process base priorities
//
#define PROCESS_PRIORITY_IDLE                   3
#define PROCESS_PRIORITY_NORMAL                 8
#define PROCESS_PRIORITY_NORMAL_FOREGROUND      9

//
// Process memory priorities
//
#define MEMORY_PRIORITY_BACKGROUND             0
#define MEMORY_PRIORITY_UNKNOWN                1
#define MEMORY_PRIORITY_FOREGROUND             2

//
// Process Priority Separation Values (OR)
//
#define PSP_DEFAULT_QUANTUMS                    0x00
#define PSP_VARIABLE_QUANTUMS                   0x04
#define PSP_FIXED_QUANTUMS                      0x08
#define PSP_LONG_QUANTUMS                       0x10
#define PSP_SHORT_QUANTUMS                      0x20

//
// Process Handle Tracing Values
//
#define PROCESS_HANDLE_TRACE_TYPE_OPEN          1
#define PROCESS_HANDLE_TRACE_TYPE_CLOSE         2
#define PROCESS_HANDLE_TRACE_TYPE_BADREF        3
#define PROCESS_HANDLE_TRACING_MAX_STACKS       16

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
#endif

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

//
// TEB Active Frame Flags
//
#define TEB_ACTIVE_FRAME_CONTEXT_FLAG_EXTENDED 	0x1

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
// Job Limit Flags
//
#define JOB_OBJECT_LIMIT_WORKINGSET             0x1
#define JOB_OBJECT_LIMIT_PROCESS_TIME           0x2
#define JOB_OBJECT_LIMIT_JOB_TIME               0x4
#define JOB_OBJECT_LIMIT_ACTIVE_PROCESS         0x8
#define JOB_OBJECT_LIMIT_AFFINITY               0x10
#define JOB_OBJECT_LIMIT_PRIORITY_CLASS         0x20
#define JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME      0x40
#define JOB_OBJECT_LIMIT_SCHEDULING_CLASS       0x80
#define JOB_OBJECT_LIMIT_PROCESS_MEMORY         0x100
#define JOB_OBJECT_LIMIT_JOB_MEMORY             0x200
#define JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION 0x400
#define JOB_OBJECT_LIMIT_BREAKAWAY_OK           0x800
#define JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK    0x1000
#define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE      0x2000

//
// Job Security Limit Flags
//
#define JOB_OBJECT_SECURITY_NO_ADMIN            0x0001
#define JOB_OBJECT_SECURITY_RESTRICTED_TOKEN    0x0002
#define JOB_OBJECT_SECURITY_ONLY_TOKEN          0x0004
#define JOB_OBJECT_SECURITY_FILTER_TOKENS       0x0008

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
#define STA_OWNS_WORKING_SET_BITS               0x1F8

//
// Kernel Process flags (maybe in ketypes.h?)
//
#define KPSF_AUTO_ALIGNMENT_BIT                 0
#define KPSF_DISABLE_BOOST_BIT                  1

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
#endif

//
// TLS/FLS Defines
//
#define TLS_EXPANSION_SLOTS                     1024

#ifdef NTOS_MODE_USER
//
// Thread Native Base Priorities
//
#define LOW_PRIORITY                            0
#define LOW_REALTIME_PRIORITY                   16
#define HIGH_PRIORITY                           31
#define MAXIMUM_PRIORITY                        32

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
    ProcessThreadStackAllocation,
    ProcessWorkingSetWatchEx,
    ProcessImageFileNameWin32,
    ProcessImageFileMapping,
    ProcessAffinityUpdateMode,
    ProcessMemoryAllocationMode,
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
    _In_ struct _EPROCESS *Process,
    _In_ BOOLEAN Create
);

typedef
NTSTATUS
(NTAPI *PKWIN32_THREAD_CALLOUT)(
    _In_ struct _ETHREAD *Thread,
    _In_ PSW32THREADCALLOUTTYPE Type
);

typedef
NTSTATUS
(NTAPI *PKWIN32_GLOBALATOMTABLE_CALLOUT)(
    VOID
);

typedef
NTSTATUS
(NTAPI *PKWIN32_POWEREVENT_CALLOUT)(
    _In_ struct _WIN32_POWEREVENT_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PKWIN32_POWERSTATE_CALLOUT)(
    _In_ struct _WIN32_POWERSTATE_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PKWIN32_JOB_CALLOUT)(
    _In_ struct _WIN32_JOBCALLOUT_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PGDI_BATCHFLUSH_ROUTINE)(
    VOID
);

typedef
NTSTATUS
(NTAPI *PKWIN32_OPENMETHOD_CALLOUT)(
    _In_ struct _WIN32_OPENMETHOD_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PKWIN32_OKTOCLOSEMETHOD_CALLOUT)(
    _In_ struct _WIN32_OKAYTOCLOSEMETHOD_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PKWIN32_CLOSEMETHOD_CALLOUT)(
    _In_ struct _WIN32_CLOSEMETHOD_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PKWIN32_DELETEMETHOD_CALLOUT)(
    _In_ struct _WIN32_DELETEMETHOD_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PKWIN32_PARSEMETHOD_CALLOUT)(
    _In_ struct _WIN32_PARSEMETHOD_PARAMETERS *Parameters
);

typedef
NTSTATUS
(NTAPI *PKWIN32_SESSION_CALLOUT)(
    _In_ PVOID Parameter
);

#if (NTDDI_VERSION >= NTDDI_LONGHORN)
typedef
NTSTATUS
(NTAPI *PKWIN32_WIN32DATACOLLECTION_CALLOUT)(
    _In_ struct _EPROCESS *Process,
    _In_ PVOID Callback,
    _In_ PVOID Context
);
#endif

//
// Lego Callback
//
typedef
VOID
(NTAPI *PLEGO_NOTIFY_ROUTINE)(
    _In_ PKTHREAD Thread
);

#endif

typedef NTSTATUS
(NTAPI *PPOST_PROCESS_INIT_ROUTINE)(
    VOID
);

//
// Descriptor Table Entry Definition
//
#if (_M_IX86)
#define _DESCRIPTOR_TABLE_ENTRY_DEFINED
typedef struct _DESCRIPTOR_TABLE_ENTRY
{
    ULONG Selector;
    LDT_ENTRY Descriptor;
} DESCRIPTOR_TABLE_ENTRY, *PDESCRIPTOR_TABLE_ENTRY;
#endif

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
// Initial PEB
//
typedef struct _INITIAL_PEB
{
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    union
    {
        BOOLEAN BitField;
#if (NTDDI_VERSION >= NTDDI_WS03)
        struct
        {
            BOOLEAN ImageUsesLargePages:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
            BOOLEAN IsProtectedProcess:1;
            BOOLEAN IsLegacyProcess:1;
            BOOLEAN SpareBits:5;
#else
            BOOLEAN SpareBits:7;
#endif
        };
#else
        BOOLEAN SpareBool;
#endif
    };
    HANDLE Mutant;
} INITIAL_PEB, *PINITIAL_PEB;

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
typedef const struct _TEB_ACTIVE_FRAME_CONTEXT *PCTEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME_CONTEXT_EX
{
    TEB_ACTIVE_FRAME_CONTEXT BasicContext;
    PCSTR SourceLocation;
} TEB_ACTIVE_FRAME_CONTEXT_EX, *PTEB_ACTIVE_FRAME_CONTEXT_EX;
typedef const struct _TEB_ACTIVE_FRAME_CONTEXT_EX *PCTEB_ACTIVE_FRAME_CONTEXT_EX;

typedef struct _TEB_ACTIVE_FRAME
{
    ULONG Flags;
    struct _TEB_ACTIVE_FRAME *Previous;
    PCTEB_ACTIVE_FRAME_CONTEXT Context;
} TEB_ACTIVE_FRAME, *PTEB_ACTIVE_FRAME;
typedef const struct _TEB_ACTIVE_FRAME *PCTEB_ACTIVE_FRAME;

typedef struct _TEB_ACTIVE_FRAME_EX
{
    TEB_ACTIVE_FRAME BasicFrame;
    PVOID ExtensionIdentifier;
} TEB_ACTIVE_FRAME_EX, *PTEB_ACTIVE_FRAME_EX;
typedef const struct _TEB_ACTIVE_FRAME_EX *PCTEB_ACTIVE_FRAME_EX;

typedef struct _CLIENT_ID32
{
    ULONG UniqueProcess;
    ULONG UniqueThread;
} CLIENT_ID32, *PCLIENT_ID32;

typedef struct _CLIENT_ID64
{
    ULONG64 UniqueProcess;
    ULONG64 UniqueThread;
} CLIENT_ID64, *PCLIENT_ID64;

#if (NTDDI_VERSION < NTDDI_WS03)
typedef struct _Wx86ThreadState
{
    PULONG  CallBx86Eip;
    PVOID   DeallocationCpu;
    BOOLEAN UseKnownWx86Dll;
    CHAR    OleStubInvoked;
} Wx86ThreadState, *PWx86ThreadState;
#endif

//
// PEB.AppCompatFlags
// Tag FLAG_MASK_KERNEL
//
typedef enum _APPCOMPAT_FLAGS
{
    GetShortPathNameNT4 = 0x1,
    GetDiskFreeSpace2GB = 0x8,
    FTMFromCurrentAPI = 0x20,
    DisallowCOMBindingNotifications = 0x40,
    Ole32ValidatePointers = 0x80,
    DisableCicero = 0x100,
    Ole32EnableAsyncDocFile = 0x200,
    EnableLegacyExceptionHandlinginOLE = 0x400,
    DisableAdvanceRPCClientHardening = 0x800,
    DisableMaybeNULLSizeisConsistencycheck = 0x1000,
    DisableAdvancedRPCrangeCheck = 0x4000,
    EnableLegacyExceptionHandlingInRPC = 0x8000,
    EnableLegacyNTFSFlagsForDocfileOpens = 0x10000,
    DisableNDRIIDConsistencyCheck = 0x20000,
    UserDisableForwarderPatch = 0x40000,
    DisableNewWMPAINTDispatchInOLE = 0x100000,
    DoNotAddToCache = 0x80000000,
} APPCOMPAT_FLAGS;


//
// PEB.AppCompatFlagsUser.LowPart
// Tag FLAG_MASK_USER
//
typedef enum _APPCOMPAT_USERFLAGS
{
    DisableAnimation = 0x1,
    DisableKeyboardCues = 0x2,
    No50StylebitsInSetWindowLong = 0x4,
    DisableDrawPatternRect = 0x8,
    MSShellDialog = 0x10,
    NoDDETerminateDuringDestroy = 0x20,
    GiveupForeground = 0x40,
    AlwaysActiveMenus = 0x80,
    NoMouseHideInEdit = 0x100,
    NoGdiBatching = 0x200,
    FontSubstitution = 0x400,
    No50StylebitsInCreateWindow = 0x800,
    NoCustomPaperSizes = 0x1000,
    AllTheDdeHacks = 0x2000,
    UseDefaultCharset = 0x4000,
    NoCharDeadKey = 0x8000,
    NoTryExceptForWindowProc = 0x10000,
    NoInitInsertReplaceFlags = 0x20000,
    NoDdeSync = 0x40000,
    NoGhost = 0x80000,
    NoDdeAsyncReg = 0x100000,
    StrictLLHook = 0x200000,
    NoShadow = 0x400000,
    NoTimerCallbackProtection = 0x1000000,
    HighDpiAware = 0x2000000,
    OpenGLEmfAware = 0x4000000,
    EnableTransparantBltMirror = 0x8000000,
    NoPaddedBorder = 0x10000000,
    ForceLegacyResizeCM = 0x20000000,
    HardwareAudioMixer = 0x40000000,
    DisableSWCursorOnMoveSize = 0x80000000,
#if 0
    DisableWindowArrangement = 0x100000000,
    ReorderWaveForCommunications = 0x200000000,
    NoGdiHwAcceleration = 0x400000000,
#endif
} APPCOMPAT_USERFLAGS;

//
// PEB.AppCompatFlagsUser.HighPart
// Tag FLAG_MASK_USER
//
typedef enum _APPCOMPAT_USERFLAGS_HIGHPART
{
    DisableWindowArrangement = 0x1,
    ReorderWaveForCommunications = 0x2,
    NoGdiHwAcceleration = 0x4,
} APPCOMPAT_USERFLAGS_HIGHPART;

//
// Process Environment Block (PEB)
// Thread Environment Block (TEB)
//
#include "peb_teb.h"

#ifdef _WIN64
//
// Explicit 32 bit PEB/TEB
//
#define EXPLICIT_32BIT
#include "peb_teb.h"
#undef EXPLICIT_32BIT

//
// Explicit 64 bit PEB/TEB
//
#define EXPLICIT_64BIT
#include "peb_teb.h"
#undef EXPLICIT_64BIT
#endif

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
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

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

typedef struct _POOLED_USAGE_AND_LIMITS
{
    SIZE_T PeakPagedPoolUsage;
    SIZE_T PagedPoolUsage;
    SIZE_T PagedPoolLimit;
    SIZE_T PeakNonPagedPoolUsage;
    SIZE_T NonPagedPoolUsage;
    SIZE_T NonPagedPoolLimit;
    SIZE_T PeakPagefileUsage;
    SIZE_T PagefileUsage;
    SIZE_T PagefileLimit;
} POOLED_USAGE_AND_LIMITS, *PPOOLED_USAGE_AND_LIMITS;

typedef struct _PROCESS_WS_WATCH_INFORMATION
{
    PVOID FaultingPc;
    PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

typedef struct _PROCESS_SESSION_INFORMATION
{
    ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

typedef struct _PROCESS_HANDLE_TRACING_ENTRY
{
    HANDLE Handle;
    CLIENT_ID ClientId;
    ULONG Type;
    PVOID Stacks[PROCESS_HANDLE_TRACING_MAX_STACKS];
} PROCESS_HANDLE_TRACING_ENTRY, *PPROCESS_HANDLE_TRACING_ENTRY;

typedef struct _PROCESS_HANDLE_TRACING_QUERY
{
    HANDLE Handle;
    ULONG TotalTraces;
    PROCESS_HANDLE_TRACING_ENTRY HandleTrace[ANYSIZE_ARRAY];
} PROCESS_HANDLE_TRACING_QUERY, *PPROCESS_HANDLE_TRACING_QUERY;

#endif

typedef struct _PROCESS_LDT_INFORMATION
{
    ULONG Start;
    ULONG Length;
    LDT_ENTRY LdtEntries[ANYSIZE_ARRAY];
} PROCESS_LDT_INFORMATION, *PPROCESS_LDT_INFORMATION;

typedef struct _PROCESS_LDT_SIZE
{
    ULONG Length;
} PROCESS_LDT_SIZE, *PPROCESS_LDT_SIZE;

typedef struct _PROCESS_PRIORITY_CLASS
{
    BOOLEAN Foreground;
    UCHAR PriorityClass;
} PROCESS_PRIORITY_CLASS, *PPROCESS_PRIORITY_CLASS;

// Compatibility with windows, see CORE-16757, CORE-17106, CORE-17247
C_ASSERT(sizeof(PROCESS_PRIORITY_CLASS) == 2);

typedef struct _PROCESS_FOREGROUND_BACKGROUND
{
    BOOLEAN Foreground;
} PROCESS_FOREGROUND_BACKGROUND, *PPROCESS_FOREGROUND_BACKGROUND;

//
// Apphelp SHIM Cache
//
typedef enum _APPHELPCACHESERVICECLASS
{
    ApphelpCacheServiceLookup = 0,
    ApphelpCacheServiceRemove = 1,
    ApphelpCacheServiceUpdate = 2,
    ApphelpCacheServiceFlush = 3,
    ApphelpCacheServiceDump = 4,

    ApphelpDBGReadRegistry = 0x100,
    ApphelpDBGWriteRegistry = 0x101,
} APPHELPCACHESERVICECLASS;


typedef struct _APPHELP_CACHE_SERVICE_LOOKUP
{
    UNICODE_STRING ImageName;
    HANDLE ImageHandle;
} APPHELP_CACHE_SERVICE_LOOKUP, *PAPPHELP_CACHE_SERVICE_LOOKUP;


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
// Process Quota Type
//
typedef enum _PS_QUOTA_TYPE
{
    PsNonPagedPool = 0,
    PsPagedPool,
    PsPageFile,
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PsWorkingSet,
#endif
#if (NTDDI_VERSION == NTDDI_LONGHORN)
    PsCpuRate,
#endif
    PsQuotaTypes
} PS_QUOTA_TYPE;

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
    EPROCESS_QUOTA_ENTRY QuotaEntry[PsQuotaTypes];
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
        KSEMAPHORE KeyedWaitSemaphore;
    };
    union
    {
        PVOID LpcReplyMessage;
        PVOID LpcWaitingOnPort;
    };
#endif
    PPS_IMPERSONATION_INFORMATION ImpersonationInfo;
    LIST_ENTRY IrpList;
    ULONG_PTR TopLevelIrp;
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
           ULONG SuppressSymbolLoad:1;
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
} ETHREAD;

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
    SIZE_T QuotaUsage[PsQuotaTypes];
    SIZE_T QuotaPeak[PsQuotaTypes];
    SIZE_T CommitCharge;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
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
    PFN_NUMBER WorkingSetPage;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    EX_PUSH_LOCK AddressCreationLock;
    PETHREAD RotateInProgress;
#else
    KGUARDED_MUTEX AddressCreationLock;
    KSPIN_LOCK HyperSpaceLock;
#endif
    PETHREAD ForkInProgress;
    ULONG_PTR HardwareTrigger;
    PMM_AVL_TABLE PhysicalVadRoot;
    PVOID CloneRoot;
    PFN_NUMBER NumberOfPrivatePages;
    PFN_NUMBER NumberOfLockedPages;
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
        HARDWARE_PTE PageDirectoryPte;
        ULONGLONG Filler;
    };
    PVOID Session;
    CHAR ImageFileName[16];
    LIST_ENTRY JobLinks;
    PVOID LockedPagesList;
    LIST_ENTRY ThreadListHead;
    PVOID SecurityPort;
#ifdef _M_AMD64
    struct _WOW64_PROCESS *Wow64Process;
#else
    PVOID PaeTop;
#endif
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
    SIZE_T CommitChargeLimit;
    SIZE_T CommitChargePeak;
    PVOID AweInfo;
    SE_AUDIT_PROCESS_CREATION_INFO SeAuditProcessCreationInfo;
    MMSUPPORT Vm;
#ifdef _M_AMD64
    ULONG Spares[2];
#else
    LIST_ENTRY MmProcessLinks;
#endif
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
} EPROCESS;

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
#if (NTDDI_VERSION >= NTDDI_WINXP) && (NTDDI_VERSION < NTDDI_WS03)
    FAST_MUTEX MemoryLimitsLock;
#elif (NTDDI_VERSION >= NTDDI_WS03) && (NTDDI_VERSION < NTDDI_LONGHORN)
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
// Job Information Structures for NtQueryInformationJobObject
//

typedef struct _JOBOBJECT_BASIC_ACCOUNTING_INFORMATION
{
    LARGE_INTEGER TotalUserTime;
    LARGE_INTEGER TotalKernelTime;
    LARGE_INTEGER ThisPeriodTotalUserTime;
    LARGE_INTEGER ThisPeriodTotalKernelTime;
    ULONG TotalPageFaultCount;
    ULONG TotalProcesses;
    ULONG ActiveProcesses;
    ULONG TotalTerminatedProcesses;
} JOBOBJECT_BASIC_ACCOUNTING_INFORMATION, *PJOBOBJECT_BASIC_ACCOUNTING_INFORMATION;

typedef struct _JOBOBJECT_BASIC_LIMIT_INFORMATION
{
    LARGE_INTEGER PerProcessUserTimeLimit;
    LARGE_INTEGER PerJobUserTimeLimit;
    ULONG LimitFlags;
    SIZE_T MinimumWorkingSetSize;
    SIZE_T MaximumWorkingSetSize;
    ULONG ActiveProcessLimit;
    ULONG_PTR Affinity;
    ULONG PriorityClass;
    ULONG SchedulingClass;
} JOBOBJECT_BASIC_LIMIT_INFORMATION, *PJOBOBJECT_BASIC_LIMIT_INFORMATION;

typedef struct _JOBOBJECT_BASIC_PROCESS_ID_LIST
{
    ULONG NumberOfAssignedProcesses;
    ULONG NumberOfProcessIdsInList;
    ULONG_PTR ProcessIdList[1];
} JOBOBJECT_BASIC_PROCESS_ID_LIST, *PJOBOBJECT_BASIC_PROCESS_ID_LIST;

typedef struct _JOBOBJECT_BASIC_UI_RESTRICTIONS
{
    ULONG UIRestrictionsClass;
} JOBOBJECT_BASIC_UI_RESTRICTIONS, *PJOBOBJECT_BASIC_UI_RESTRICTIONS;

typedef struct _JOBOBJECT_SECURITY_LIMIT_INFORMATION
{
    ULONG SecurityLimitFlags;
    HANDLE JobToken;
    PTOKEN_GROUPS SidsToDisable;
    PTOKEN_PRIVILEGES PrivilegesToDelete;
    PTOKEN_GROUPS RestrictedSids;
} JOBOBJECT_SECURITY_LIMIT_INFORMATION, *PJOBOBJECT_SECURITY_LIMIT_INFORMATION;

typedef struct _JOBOBJECT_END_OF_JOB_TIME_INFORMATION
{
    ULONG EndOfJobTimeAction;
} JOBOBJECT_END_OF_JOB_TIME_INFORMATION, PJOBOBJECT_END_OF_JOB_TIME_INFORMATION;

typedef struct _JOBOBJECT_ASSOCIATE_COMPLETION_PORT
{
    PVOID CompletionKey;
    HANDLE CompletionPort;
} JOBOBJECT_ASSOCIATE_COMPLETION_PORT, *PJOBOBJECT_ASSOCIATE_COMPLETION_PORT;

typedef struct JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION
{
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION BasicInfo;
    IO_COUNTERS IoInfo;
} JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION, *PJOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION;

typedef struct _JOBOBJECT_EXTENDED_LIMIT_INFORMATION
{
    JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
    IO_COUNTERS IoInfo;
    SIZE_T ProcessMemoryLimit;
    SIZE_T JobMemoryLimit;
    SIZE_T PeakProcessMemoryUsed;
    SIZE_T PeakJobMemoryUsed;
} JOBOBJECT_EXTENDED_LIMIT_INFORMATION, *PJOBOBJECT_EXTENDED_LIMIT_INFORMATION;


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
    _Out_ PUNICODE_STRING CompleteName;
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
    PKWIN32_SESSION_CALLOUT DesktopOpenProcedure;
    PKWIN32_SESSION_CALLOUT DesktopOkToCloseProcedure;
    PKWIN32_SESSION_CALLOUT DesktopCloseProcedure;
    PKWIN32_SESSION_CALLOUT DesktopDeleteProcedure;
    PKWIN32_SESSION_CALLOUT WindowStationOkToCloseProcedure;
    PKWIN32_SESSION_CALLOUT WindowStationCloseProcedure;
    PKWIN32_SESSION_CALLOUT WindowStationDeleteProcedure;
    PKWIN32_SESSION_CALLOUT WindowStationParseProcedure;
    PKWIN32_SESSION_CALLOUT WindowStationOpenProcedure;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PKWIN32_WIN32DATACOLLECTION_CALLOUT Win32DataCollectionProcedure;
#endif
} WIN32_CALLOUTS_FPNS, *PWIN32_CALLOUTS_FPNS;

#endif // !NTOS_MODE_USER

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _PSTYPES_H

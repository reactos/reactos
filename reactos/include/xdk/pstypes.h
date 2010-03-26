/******************************************************************************
 *                           Process Manager Types                            *
 ******************************************************************************/
$if (_WDMDDK_)

#define QUOTA_LIMITS_HARDWS_MIN_ENABLE  0x00000001
#define QUOTA_LIMITS_HARDWS_MIN_DISABLE 0x00000002
#define QUOTA_LIMITS_HARDWS_MAX_ENABLE  0x00000004
#define QUOTA_LIMITS_HARDWS_MAX_DISABLE 0x00000008
#define QUOTA_LIMITS_USE_DEFAULT_LIMITS 0x00000010

/* Thread Access Rights */
#define THREAD_TERMINATE                 0x0001
#define THREAD_SUSPEND_RESUME            0x0002
#define THREAD_ALERT                     0x0004
#define THREAD_GET_CONTEXT               0x0008
#define THREAD_SET_CONTEXT               0x0010
#define THREAD_SET_INFORMATION           0x0020
#define THREAD_SET_LIMITED_INFORMATION   0x0400
#define THREAD_QUERY_LIMITED_INFORMATION 0x0800

#define PROCESS_DUP_HANDLE                 (0x0040)

#if (NTDDI_VERSION >= NTDDI_VISTA)
#define PROCESS_ALL_ACCESS  (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF)
#else
#define PROCESS_ALL_ACCESS  (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFF)
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)
#define THREAD_ALL_ACCESS   (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF)
#else
#define THREAD_ALL_ACCESS   (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3FF)
#endif

#define LOW_PRIORITY                      0
#define LOW_REALTIME_PRIORITY             16
#define HIGH_PRIORITY                     31
#define MAXIMUM_PRIORITY                  32

$endif /* _WDMDDK_ */
$if (_NTDDK_)

#define QUOTA_LIMITS_HARDWS_MIN_ENABLE  0x00000001
#define QUOTA_LIMITS_HARDWS_MIN_DISABLE 0x00000002
#define QUOTA_LIMITS_HARDWS_MAX_ENABLE  0x00000004
#define QUOTA_LIMITS_HARDWS_MAX_DISABLE 0x00000008
#define QUOTA_LIMITS_USE_DEFAULT_LIMITS 0x00000010

typedef struct _QUOTA_LIMITS {
  SIZE_T PagedPoolLimit;
  SIZE_T NonPagedPoolLimit;
  SIZE_T MinimumWorkingSetSize;
  SIZE_T MaximumWorkingSetSize;
  SIZE_T PagefileLimit;
  LARGE_INTEGER TimeLimit;
} QUOTA_LIMITS, *PQUOTA_LIMITS;

typedef union _RATE_QUOTA_LIMIT {
  ULONG RateData;
  struct {
    ULONG RatePercent:7;
    ULONG Reserved0:25;
  } DUMMYSTRUCTNAME;
} RATE_QUOTA_LIMIT, *PRATE_QUOTA_LIMIT;

typedef struct _QUOTA_LIMITS_EX {
  SIZE_T PagedPoolLimit;
  SIZE_T NonPagedPoolLimit;
  SIZE_T MinimumWorkingSetSize;
  SIZE_T MaximumWorkingSetSize;
  SIZE_T PagefileLimit;
  LARGE_INTEGER TimeLimit;
  SIZE_T WorkingSetLimit;
  SIZE_T Reserved2;
  SIZE_T Reserved3;
  SIZE_T Reserved4;
  ULONG Flags;
  RATE_QUOTA_LIMIT CpuRateLimit;
} QUOTA_LIMITS_EX, *PQUOTA_LIMITS_EX;

typedef struct _IO_COUNTERS {
  ULONGLONG ReadOperationCount;
  ULONGLONG WriteOperationCount;
  ULONGLONG OtherOperationCount;
  ULONGLONG ReadTransferCount;
  ULONGLONG WriteTransferCount;
  ULONGLONG OtherTransferCount;
} IO_COUNTERS, *PIO_COUNTERS;

typedef struct _VM_COUNTERS {
  SIZE_T PeakVirtualSize;
  SIZE_T VirtualSize;
  ULONG PageFaultCount;
  SIZE_T PeakWorkingSetSize;
  SIZE_T WorkingSetSize;
  SIZE_T QuotaPeakPagedPoolUsage;
  SIZE_T QuotaPagedPoolUsage;
  SIZE_T QuotaPeakNonPagedPoolUsage;
  SIZE_T QuotaNonPagedPoolUsage;
  SIZE_T PagefileUsage;
  SIZE_T PeakPagefileUsage;
} VM_COUNTERS, *PVM_COUNTERS;

typedef struct _VM_COUNTERS_EX {
  SIZE_T PeakVirtualSize;
  SIZE_T VirtualSize;
  ULONG PageFaultCount;
  SIZE_T PeakWorkingSetSize;
  SIZE_T WorkingSetSize;
  SIZE_T QuotaPeakPagedPoolUsage;
  SIZE_T QuotaPagedPoolUsage;
  SIZE_T QuotaPeakNonPagedPoolUsage;
  SIZE_T QuotaNonPagedPoolUsage;
  SIZE_T PagefileUsage;
  SIZE_T PeakPagefileUsage;
  SIZE_T PrivateUsage;
} VM_COUNTERS_EX, *PVM_COUNTERS_EX;

#define MAX_HW_COUNTERS 16
#define THREAD_PROFILING_FLAG_DISPATCH  0x00000001

typedef enum _HARDWARE_COUNTER_TYPE {
  PMCCounter,
  MaxHardwareCounterType
} HARDWARE_COUNTER_TYPE, *PHARDWARE_COUNTER_TYPE;

typedef struct _HARDWARE_COUNTER {
  HARDWARE_COUNTER_TYPE Type;
  ULONG Reserved;
  ULONG64 Index;
} HARDWARE_COUNTER, *PHARDWARE_COUNTER;

typedef struct _POOLED_USAGE_AND_LIMITS {
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

typedef struct _PROCESS_ACCESS_TOKEN {
  HANDLE Token;
  HANDLE Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

#define PROCESS_EXCEPTION_PORT_ALL_STATE_BITS     0x00000003UL
#define PROCESS_EXCEPTION_PORT_ALL_STATE_FLAGS    ((ULONG_PTR)((1UL << PROCESS_EXCEPTION_PORT_ALL_STATE_BITS) - 1))

typedef struct _PROCESS_EXCEPTION_PORT {
  IN HANDLE ExceptionPortHandle;
  IN OUT ULONG StateFlags;
} PROCESS_EXCEPTION_PORT, *PPROCESS_EXCEPTION_PORT;

typedef VOID
(NTAPI *PCREATE_PROCESS_NOTIFY_ROUTINE)(
  IN HANDLE ParentId,
  IN HANDLE ProcessId,
  IN BOOLEAN Create);

typedef struct _PS_CREATE_NOTIFY_INFO {
  IN SIZE_T Size;
  union {
    IN ULONG Flags;
    struct {
      IN ULONG FileOpenNameAvailable:1;
      IN ULONG Reserved:31;
    };
  };
  IN HANDLE ParentProcessId;
  IN CLIENT_ID CreatingThreadId;
  IN OUT struct _FILE_OBJECT *FileObject;
  IN PCUNICODE_STRING ImageFileName;
  IN PCUNICODE_STRING CommandLine OPTIONAL;
  IN OUT NTSTATUS CreationStatus;
} PS_CREATE_NOTIFY_INFO, *PPS_CREATE_NOTIFY_INFO;

typedef VOID
(NTAPI *PCREATE_PROCESS_NOTIFY_ROUTINE_EX)(
  IN OUT PEPROCESS Process,
  IN HANDLE ProcessId,
  IN PPS_CREATE_NOTIFY_INFO CreateInfo OPTIONAL);

typedef VOID
(NTAPI *PCREATE_THREAD_NOTIFY_ROUTINE)(
  IN HANDLE ProcessId,
  IN HANDLE ThreadId,
  IN BOOLEAN Create);

#define IMAGE_ADDRESSING_MODE_32BIT       3

typedef struct _IMAGE_INFO {
  _ANONYMOUS_UNION union {
    ULONG Properties;
    _ANONYMOUS_STRUCT struct {
      ULONG ImageAddressingMode:8;
      ULONG SystemModeImage:1;
      ULONG ImageMappedToAllPids:1;
      ULONG ExtendedInfoPresent:1;
      ULONG Reserved:21;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  PVOID ImageBase;
  ULONG ImageSelector;
  SIZE_T ImageSize;
  ULONG ImageSectionNumber;
} IMAGE_INFO, *PIMAGE_INFO;

typedef struct _IMAGE_INFO_EX {
  SIZE_T Size;
  IMAGE_INFO ImageInfo;
  struct _FILE_OBJECT *FileObject;
} IMAGE_INFO_EX, *PIMAGE_INFO_EX;

typedef VOID
(NTAPI *PLOAD_IMAGE_NOTIFY_ROUTINE)(
  IN PUNICODE_STRING FullImageName,
  IN HANDLE ProcessId,
  IN PIMAGE_INFO ImageInfo);

#define THREAD_CSWITCH_PMU_DISABLE  FALSE
#define THREAD_CSWITCH_PMU_ENABLE   TRUE

#define PROCESS_LUID_DOSDEVICES_ONLY 0x00000001

#define PROCESS_HANDLE_TRACING_MAX_STACKS 16

typedef struct _NT_TIB {
  struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
  PVOID StackBase;
  PVOID StackLimit;
  PVOID SubSystemTib;
  _ANONYMOUS_UNION union {
    PVOID FiberData;
    ULONG Version;
  } DUMMYUNIONNAME;
  PVOID ArbitraryUserPointer;
  struct _NT_TIB *Self;
} NT_TIB, *PNT_TIB;

typedef struct _NT_TIB32 {
  ULONG ExceptionList;
  ULONG StackBase;
  ULONG StackLimit;
  ULONG SubSystemTib;
  __GNU_EXTENSION union {
    ULONG FiberData;
    ULONG Version;
  };
  ULONG ArbitraryUserPointer;
  ULONG Self;
} NT_TIB32,*PNT_TIB32;

typedef struct _NT_TIB64 {
  ULONG64 ExceptionList;
  ULONG64 StackBase;
  ULONG64 StackLimit;
  ULONG64 SubSystemTib;
  __GNU_EXTENSION union {
    ULONG64 FiberData;
    ULONG Version;
  };
  ULONG64 ArbitraryUserPointer;
  ULONG64 Self;
} NT_TIB64,*PNT_TIB64;

typedef enum _PROCESSINFOCLASS {
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
  ProcessGroupInformation,
  ProcessTokenVirtualizationEnabled,
  ProcessConsoleHostProcess,
  ProcessWindowInformation,
  MaxProcessInfoClass
} PROCESSINFOCLASS;

typedef enum _THREADINFOCLASS {
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
  ThreadCSwitchPmu,
  ThreadWow64Context,
  ThreadGroupInformation,
  ThreadUmsInformation,
  ThreadCounterProfiling,
  ThreadIdealProcessorEx,
  MaxThreadInfoClass
} THREADINFOCLASS;

typedef struct _PAGE_PRIORITY_INFORMATION {
  ULONG PagePriority;
} PAGE_PRIORITY_INFORMATION, *PPAGE_PRIORITY_INFORMATION;

typedef struct _PROCESS_WS_WATCH_INFORMATION {
  PVOID FaultingPc;
  PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION {
  NTSTATUS ExitStatus;
  struct _PEB *PebBaseAddress;
  ULONG_PTR AffinityMask;
  KPRIORITY BasePriority;
  ULONG_PTR UniqueProcessId;
  ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION,*PPROCESS_BASIC_INFORMATION;

typedef struct _PROCESS_EXTENDED_BASIC_INFORMATION {
  SIZE_T Size;
  PROCESS_BASIC_INFORMATION BasicInfo;
  union {
    ULONG Flags;
    struct {
      ULONG IsProtectedProcess:1;
      ULONG IsWow64Process:1;
      ULONG IsProcessDeleting:1;
      ULONG IsCrossSessionCreate:1;
      ULONG SpareBits:28;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
} PROCESS_EXTENDED_BASIC_INFORMATION, *PPROCESS_EXTENDED_BASIC_INFORMATION;

typedef struct _PROCESS_DEVICEMAP_INFORMATION {
  __GNU_EXTENSION union {
    struct {
      HANDLE DirectoryHandle;
    } Set;
    struct {
      ULONG DriveMap;
      UCHAR DriveType[32];
    } Query;
  };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

typedef struct _PROCESS_DEVICEMAP_INFORMATION_EX {
  union {
    struct {
      HANDLE DirectoryHandle;
    } Set;
    struct {
      ULONG DriveMap;
      UCHAR DriveType[32];
    } Query;
  } DUMMYUNIONNAME;
  ULONG Flags;
} PROCESS_DEVICEMAP_INFORMATION_EX, *PPROCESS_DEVICEMAP_INFORMATION_EX;

typedef struct _PROCESS_SESSION_INFORMATION {
  ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

typedef struct _PROCESS_HANDLE_TRACING_ENABLE {
  ULONG Flags;
} PROCESS_HANDLE_TRACING_ENABLE, *PPROCESS_HANDLE_TRACING_ENABLE;

typedef struct _PROCESS_HANDLE_TRACING_ENABLE_EX {
  ULONG Flags;
  ULONG TotalSlots;
} PROCESS_HANDLE_TRACING_ENABLE_EX, *PPROCESS_HANDLE_TRACING_ENABLE_EX;

typedef struct _PROCESS_HANDLE_TRACING_ENTRY {
  HANDLE Handle;
  CLIENT_ID ClientId;
  ULONG Type;
  PVOID Stacks[PROCESS_HANDLE_TRACING_MAX_STACKS];
} PROCESS_HANDLE_TRACING_ENTRY, *PPROCESS_HANDLE_TRACING_ENTRY;

typedef struct _PROCESS_HANDLE_TRACING_QUERY {
  HANDLE Handle;
  ULONG TotalTraces;
  PROCESS_HANDLE_TRACING_ENTRY HandleTrace[1];
} PROCESS_HANDLE_TRACING_QUERY, *PPROCESS_HANDLE_TRACING_QUERY;

extern NTKERNELAPI PEPROCESS PsInitialSystemProcess;

$endif /* _NTDDK_ */


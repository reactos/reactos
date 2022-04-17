/******************************************************************************
 *                              Kernel Types                                  *
 ******************************************************************************/
$if (_WDMDDK_)

typedef UCHAR KIRQL, *PKIRQL;
typedef CCHAR KPROCESSOR_MODE;
typedef LONG KPRIORITY;

typedef enum _MODE {
  KernelMode,
  UserMode,
  MaximumMode
} MODE;

#define CACHE_FULLY_ASSOCIATIVE 0xFF
#define MAXIMUM_SUSPEND_COUNT   MAXCHAR

#define EVENT_QUERY_STATE (0x0001)
#define EVENT_MODIFY_STATE (0x0002)
#define EVENT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3)

#define LTP_PC_SMT 0x1

#if (NTDDI_VERSION < NTDDI_WIN7) || defined(_X86_) || !defined(NT_PROCESSOR_GROUPS)
#define SINGLE_GROUP_LEGACY_API        1
#endif

#define SEMAPHORE_QUERY_STATE (0x0001)
#define SEMAPHORE_MODIFY_STATE (0x0002)
#define SEMAPHORE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3)

$endif(_WDMDDK_)
$if(_WDMDDK_ || _WINNT_)

typedef struct _PROCESSOR_GROUP_INFO {
  UCHAR MaximumProcessorCount;
  UCHAR ActiveProcessorCount;
  UCHAR Reserved[38];
  KAFFINITY ActiveProcessorMask;
} PROCESSOR_GROUP_INFO, *PPROCESSOR_GROUP_INFO;

typedef enum _PROCESSOR_CACHE_TYPE {
  CacheUnified,
  CacheInstruction,
  CacheData,
  CacheTrace
} PROCESSOR_CACHE_TYPE;

typedef struct _CACHE_DESCRIPTOR {
  UCHAR Level;
  UCHAR Associativity;
  USHORT LineSize;
  ULONG Size;
  PROCESSOR_CACHE_TYPE Type;
} CACHE_DESCRIPTOR, *PCACHE_DESCRIPTOR;

typedef struct _NUMA_NODE_RELATIONSHIP {
  ULONG NodeNumber;
  UCHAR Reserved[20];
  GROUP_AFFINITY GroupMask;
} NUMA_NODE_RELATIONSHIP, *PNUMA_NODE_RELATIONSHIP;

typedef struct _CACHE_RELATIONSHIP {
  UCHAR Level;
  UCHAR Associativity;
  USHORT LineSize;
  ULONG CacheSize;
  PROCESSOR_CACHE_TYPE Type;
  UCHAR Reserved[20];
  GROUP_AFFINITY GroupMask;
} CACHE_RELATIONSHIP, *PCACHE_RELATIONSHIP;

typedef struct _GROUP_RELATIONSHIP {
  USHORT MaximumGroupCount;
  USHORT ActiveGroupCount;
  UCHAR Reserved[20];
  PROCESSOR_GROUP_INFO GroupInfo[ANYSIZE_ARRAY];
} GROUP_RELATIONSHIP, *PGROUP_RELATIONSHIP;

typedef enum _LOGICAL_PROCESSOR_RELATIONSHIP {
  RelationProcessorCore,
  RelationNumaNode,
  RelationCache,
  RelationProcessorPackage,
  RelationGroup,
  RelationAll = 0xffff
} LOGICAL_PROCESSOR_RELATIONSHIP;

typedef struct _PROCESSOR_RELATIONSHIP {
  UCHAR Flags;
  UCHAR Reserved[21];
  USHORT GroupCount;
  _Field_size_(GroupCount) GROUP_AFFINITY GroupMask[ANYSIZE_ARRAY];
} PROCESSOR_RELATIONSHIP, *PPROCESSOR_RELATIONSHIP;

typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION {
  ULONG_PTR ProcessorMask;
  LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
  _ANONYMOUS_UNION union {
    struct {
      UCHAR Flags;
    } ProcessorCore;
    struct {
      ULONG NodeNumber;
    } NumaNode;
    CACHE_DESCRIPTOR Cache;
    ULONGLONG Reserved[2];
  } DUMMYUNIONNAME;
} SYSTEM_LOGICAL_PROCESSOR_INFORMATION, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION;

typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX {
  LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
  ULONG Size;
  _ANONYMOUS_UNION union {
    PROCESSOR_RELATIONSHIP Processor;
    NUMA_NODE_RELATIONSHIP NumaNode;
    CACHE_RELATIONSHIP Cache;
    GROUP_RELATIONSHIP Group;
  } DUMMYUNIONNAME;
} SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX;

$endif(_WDMDDK_ || _WINNT_)
$if(_WDMDDK_)

/* Processor features */
#define PF_FLOATING_POINT_PRECISION_ERRATA       0
#define PF_FLOATING_POINT_EMULATED               1
#define PF_COMPARE_EXCHANGE_DOUBLE               2
#define PF_MMX_INSTRUCTIONS_AVAILABLE            3
#define PF_PPC_MOVEMEM_64BIT_OK                  4
#define PF_ALPHA_BYTE_INSTRUCTIONS               5
#define PF_XMMI_INSTRUCTIONS_AVAILABLE           6
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE          7
#define PF_RDTSC_INSTRUCTION_AVAILABLE           8
#define PF_PAE_ENABLED                           9
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE        10
#define PF_SSE_DAZ_MODE_AVAILABLE               11
#define PF_NX_ENABLED                           12
#define PF_SSE3_INSTRUCTIONS_AVAILABLE          13
#define PF_COMPARE_EXCHANGE128                  14
#define PF_COMPARE64_EXCHANGE128                15
#define PF_CHANNELS_ENABLED                     16
#define PF_XSAVE_ENABLED                        17
#define PF_ARM_VFP_32_REGISTERS_AVAILABLE       18
#define PF_ARM_NEON_INSTRUCTIONS_AVAILABLE      19
#define PF_SECOND_LEVEL_ADDRESS_TRANSLATION     20
#define PF_VIRT_FIRMWARE_ENABLED                21
#define PF_RDWRFSGSBASE_AVAILABLE               22
#define PF_FASTFAIL_AVAILABLE                   23
#define PF_ARM_DIVIDE_INSTRUCTION_AVAILABLE     24
#define PF_ARM_64BIT_LOADSTORE_ATOMIC           25
#define PF_ARM_EXTERNAL_CACHE_AVAILABLE         26
#define PF_ARM_FMAC_INSTRUCTIONS_AVAILABLE      27
#define PF_RDRAND_INSTRUCTION_AVAILABLE         28
#define PF_ARM_V8_INSTRUCTIONS_AVAILABLE        29
#define PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE 30
#define PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE  31

#define MAXIMUM_WAIT_OBJECTS              64

#define ASSERT_APC(Object) \
    NT_ASSERT((Object)->Type == ApcObject)

#define ASSERT_DPC(Object) \
    NT_ASSERT(((Object)->Type == 0) || \
              ((Object)->Type == DpcObject) || \
              ((Object)->Type == ThreadedDpcObject))

#define ASSERT_GATE(Object) \
    NT_ASSERT((((Object)->Header.Type & KOBJECT_TYPE_MASK) == GateObject) || \
              (((Object)->Header.Type & KOBJECT_TYPE_MASK) == EventSynchronizationObject))

#define ASSERT_DEVICE_QUEUE(Object) \
    NT_ASSERT((Object)->Type == DeviceQueueObject)

#define ASSERT_TIMER(Object) \
    NT_ASSERT(((Object)->Header.Type == TimerNotificationObject) || \
              ((Object)->Header.Type == TimerSynchronizationObject))

#define ASSERT_MUTANT(Object) \
    NT_ASSERT((Object)->Header.Type == MutantObject)

#define ASSERT_SEMAPHORE(Object) \
    NT_ASSERT((Object)->Header.Type == SemaphoreObject)

#define ASSERT_EVENT(Object) \
    NT_ASSERT(((Object)->Header.Type == NotificationEvent) || \
              ((Object)->Header.Type == SynchronizationEvent))

#define DPC_NORMAL 0
#define DPC_THREADED 1

#define GM_LOCK_BIT          0x1
#define GM_LOCK_BIT_V        0x0
#define GM_LOCK_WAITER_WOKEN 0x2
#define GM_LOCK_WAITER_INC   0x4

#define LOCK_QUEUE_WAIT_BIT               0
#define LOCK_QUEUE_OWNER_BIT              1

#define LOCK_QUEUE_WAIT                   1
#define LOCK_QUEUE_OWNER                  2
#define LOCK_QUEUE_TIMER_LOCK_SHIFT       4
#define LOCK_QUEUE_TIMER_TABLE_LOCKS (1 << (8 - LOCK_QUEUE_TIMER_LOCK_SHIFT))

#define PROCESSOR_FEATURE_MAX 64

#define DBG_STATUS_CONTROL_C              1
#define DBG_STATUS_SYSRQ                  2
#define DBG_STATUS_BUGCHECK_FIRST         3
#define DBG_STATUS_BUGCHECK_SECOND        4
#define DBG_STATUS_FATAL                  5
#define DBG_STATUS_DEBUG_CONTROL          6
#define DBG_STATUS_WORKER                 7

#if defined(_WIN64)
#define MAXIMUM_PROC_PER_GROUP 64
#else
#define MAXIMUM_PROC_PER_GROUP 32
#endif
#define MAXIMUM_PROCESSORS          MAXIMUM_PROC_PER_GROUP

#define EXCEPTION_DIVIDED_BY_ZERO       0
#define EXCEPTION_DEBUG                 1
#define EXCEPTION_NMI                   2
#define EXCEPTION_INT3                  3
#define EXCEPTION_BOUND_CHECK           5
#define EXCEPTION_INVALID_OPCODE        6
#define EXCEPTION_NPX_NOT_AVAILABLE     7
#define EXCEPTION_DOUBLE_FAULT          8
#define EXCEPTION_NPX_OVERRUN           9
#define EXCEPTION_INVALID_TSS           0x0A
#define EXCEPTION_SEGMENT_NOT_PRESENT   0x0B
#define EXCEPTION_STACK_FAULT           0x0C
#define EXCEPTION_GP_FAULT              0x0D
#define EXCEPTION_RESERVED_TRAP         0x0F
#define EXCEPTION_NPX_ERROR             0x010
#define EXCEPTION_ALIGNMENT_CHECK       0x011

typedef enum _KBUGCHECK_CALLBACK_REASON {
  KbCallbackInvalid,
  KbCallbackReserved1,
  KbCallbackSecondaryDumpData,
  KbCallbackDumpIo,
  KbCallbackAddPages
} KBUGCHECK_CALLBACK_REASON;

struct _KBUGCHECK_REASON_CALLBACK_RECORD;

_Function_class_(KBUGCHECK_REASON_CALLBACK_ROUTINE)
_IRQL_requires_same_
typedef VOID
(NTAPI KBUGCHECK_REASON_CALLBACK_ROUTINE)(
    _In_ KBUGCHECK_CALLBACK_REASON Reason,
    _In_ struct _KBUGCHECK_REASON_CALLBACK_RECORD *Record,
    _Inout_ PVOID ReasonSpecificData,
    _In_ ULONG ReasonSpecificDataLength);
typedef KBUGCHECK_REASON_CALLBACK_ROUTINE *PKBUGCHECK_REASON_CALLBACK_ROUTINE;

typedef struct _KBUGCHECK_ADD_PAGES {
  _Inout_ PVOID Context;
  _Inout_ ULONG Flags;
  _In_ ULONG BugCheckCode;
  _Out_ ULONG_PTR Address;
  _Out_ ULONG_PTR Count;
} KBUGCHECK_ADD_PAGES, *PKBUGCHECK_ADD_PAGES;

typedef struct _KBUGCHECK_SECONDARY_DUMP_DATA {
  _In_ PVOID InBuffer;
  _In_ ULONG InBufferLength;
  _In_ ULONG MaximumAllowed;
  _Out_ GUID Guid;
  _Out_ PVOID OutBuffer;
  _Out_ ULONG OutBufferLength;
} KBUGCHECK_SECONDARY_DUMP_DATA, *PKBUGCHECK_SECONDARY_DUMP_DATA;

typedef enum _KBUGCHECK_DUMP_IO_TYPE {
  KbDumpIoInvalid,
  KbDumpIoHeader,
  KbDumpIoBody,
  KbDumpIoSecondaryData,
  KbDumpIoComplete
} KBUGCHECK_DUMP_IO_TYPE;

typedef struct _KBUGCHECK_DUMP_IO {
  _In_ ULONG64 Offset;
  _In_ PVOID Buffer;
  _In_ ULONG BufferLength;
  _In_ KBUGCHECK_DUMP_IO_TYPE Type;
} KBUGCHECK_DUMP_IO, *PKBUGCHECK_DUMP_IO;

#define KB_ADD_PAGES_FLAG_VIRTUAL_ADDRESS         0x00000001UL
#define KB_ADD_PAGES_FLAG_PHYSICAL_ADDRESS        0x00000002UL
#define KB_ADD_PAGES_FLAG_ADDITIONAL_RANGES_EXIST 0x80000000UL

typedef struct _KBUGCHECK_REASON_CALLBACK_RECORD {
  LIST_ENTRY Entry;
  PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine;
  PUCHAR Component;
  ULONG_PTR Checksum;
  KBUGCHECK_CALLBACK_REASON Reason;
  UCHAR State;
} KBUGCHECK_REASON_CALLBACK_RECORD, *PKBUGCHECK_REASON_CALLBACK_RECORD;

typedef enum _KBUGCHECK_BUFFER_DUMP_STATE {
  BufferEmpty,
  BufferInserted,
  BufferStarted,
  BufferFinished,
  BufferIncomplete
} KBUGCHECK_BUFFER_DUMP_STATE;

_Function_class_(KBUGCHECK_CALLBACK_ROUTINE)
_IRQL_requires_same_
typedef VOID
(NTAPI KBUGCHECK_CALLBACK_ROUTINE)(
  IN PVOID Buffer,
  IN ULONG Length);
typedef KBUGCHECK_CALLBACK_ROUTINE *PKBUGCHECK_CALLBACK_ROUTINE;

typedef struct _KBUGCHECK_CALLBACK_RECORD {
  LIST_ENTRY Entry;
  PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine;
  _Field_size_bytes_opt_(Length) PVOID Buffer;
  ULONG Length;
  PUCHAR Component;
  ULONG_PTR Checksum;
  UCHAR State;
} KBUGCHECK_CALLBACK_RECORD, *PKBUGCHECK_CALLBACK_RECORD;

_Function_class_(NMI_CALLBACK)
_IRQL_requires_same_
typedef BOOLEAN
(NTAPI NMI_CALLBACK)(
  _In_opt_ PVOID Context,
  _In_ BOOLEAN Handled);
typedef NMI_CALLBACK *PNMI_CALLBACK;

typedef enum _KE_PROCESSOR_CHANGE_NOTIFY_STATE {
  KeProcessorAddStartNotify = 0,
  KeProcessorAddCompleteNotify,
  KeProcessorAddFailureNotify
} KE_PROCESSOR_CHANGE_NOTIFY_STATE;

typedef struct _KE_PROCESSOR_CHANGE_NOTIFY_CONTEXT {
  KE_PROCESSOR_CHANGE_NOTIFY_STATE State;
  ULONG NtNumber;
  NTSTATUS Status;
#if (NTDDI_VERSION >= NTDDI_WIN7)
  PROCESSOR_NUMBER ProcNumber;
#endif
} KE_PROCESSOR_CHANGE_NOTIFY_CONTEXT, *PKE_PROCESSOR_CHANGE_NOTIFY_CONTEXT;

_IRQL_requires_same_
_Function_class_(PROCESSOR_CALLBACK_FUNCTION)
typedef VOID
(NTAPI PROCESSOR_CALLBACK_FUNCTION)(
  _In_ PVOID CallbackContext,
  _In_ PKE_PROCESSOR_CHANGE_NOTIFY_CONTEXT ChangeContext,
  _Inout_ PNTSTATUS OperationStatus);
typedef PROCESSOR_CALLBACK_FUNCTION *PPROCESSOR_CALLBACK_FUNCTION;

#define KE_PROCESSOR_CHANGE_ADD_EXISTING         1

#define INVALID_PROCESSOR_INDEX     0xffffffff

typedef enum _KINTERRUPT_POLARITY {
  InterruptPolarityUnknown,
  InterruptActiveHigh,
  InterruptActiveLow
} KINTERRUPT_POLARITY, *PKINTERRUPT_POLARITY;

typedef enum _KPROFILE_SOURCE {
  ProfileTime,
  ProfileAlignmentFixup,
  ProfileTotalIssues,
  ProfilePipelineDry,
  ProfileLoadInstructions,
  ProfilePipelineFrozen,
  ProfileBranchInstructions,
  ProfileTotalNonissues,
  ProfileDcacheMisses,
  ProfileIcacheMisses,
  ProfileCacheMisses,
  ProfileBranchMispredictions,
  ProfileStoreInstructions,
  ProfileFpInstructions,
  ProfileIntegerInstructions,
  Profile2Issue,
  Profile3Issue,
  Profile4Issue,
  ProfileSpecialInstructions,
  ProfileTotalCycles,
  ProfileIcacheIssues,
  ProfileDcacheAccesses,
  ProfileMemoryBarrierCycles,
  ProfileLoadLinkedIssues,
  ProfileMaximum
} KPROFILE_SOURCE;

typedef enum _KWAIT_REASON {
  Executive,
  FreePage,
  PageIn,
  PoolAllocation,
  DelayExecution,
  Suspended,
  UserRequest,
  WrExecutive,
  WrFreePage,
  WrPageIn,
  WrPoolAllocation,
  WrDelayExecution,
  WrSuspended,
  WrUserRequest,
  WrEventPair,
  WrQueue,
  WrLpcReceive,
  WrLpcReply,
  WrVirtualMemory,
  WrPageOut,
  WrRendezvous,
  WrKeyedEvent,
  WrTerminated,
  WrProcessInSwap,
  WrCpuRateControl,
  WrCalloutStack,
  WrKernel,
  WrResource,
  WrPushLock,
  WrMutex,
  WrQuantumEnd,
  WrDispatchInt,
  WrPreempted,
  WrYieldExecution,
  WrFastMutex,
  WrGuardedMutex,
  WrRundown,
  MaximumWaitReason
} KWAIT_REASON;

typedef struct _KWAIT_BLOCK {
  LIST_ENTRY WaitListEntry;
#if (NTDDI_VERSION >= NTDDI_WIN8)
  UCHAR WaitType;
  volatile UCHAR BlockState;
  USHORT WaitKey;
#ifdef _WIN64
  LONG SpareLong;
#endif
  union {
    struct _KTHREAD *Thread;
    struct _KQUEUE *NotificationQueue;
  };
  PVOID Object;
  PVOID SparePtr;
#else
  struct _KTHREAD *Thread;
  PVOID Object;
  struct _KWAIT_BLOCK *NextWaitBlock;
  USHORT WaitKey;
  UCHAR WaitType;
#if (NTDDI_VERSION >= NTDDI_WIN7)
  volatile UCHAR BlockState;
#else
  UCHAR SpareByte;
#endif
#if defined(_WIN64)
  LONG SpareLong;
#endif
#endif
} KWAIT_BLOCK, *PKWAIT_BLOCK, *PRKWAIT_BLOCK;

typedef enum _KINTERRUPT_MODE {
  LevelSensitive,
  Latched
} KINTERRUPT_MODE;

#define THREAD_WAIT_OBJECTS 3

_IRQL_requires_same_
_Function_class_(KSTART_ROUTINE)
typedef VOID
(NTAPI KSTART_ROUTINE)(
  _In_ PVOID StartContext);
typedef KSTART_ROUTINE *PKSTART_ROUTINE;

typedef VOID
(NTAPI *PKINTERRUPT_ROUTINE)(
  VOID);

_Function_class_(KSERVICE_ROUTINE)
_IRQL_requires_(HIGH_LEVEL)
_IRQL_requires_same_
typedef BOOLEAN
(NTAPI KSERVICE_ROUTINE)(
  _In_ struct _KINTERRUPT *Interrupt,
  _In_ PVOID ServiceContext);
typedef KSERVICE_ROUTINE *PKSERVICE_ROUTINE;

_Function_class_(KMESSAGE_SERVICE_ROUTINE)
_IRQL_requires_same_
typedef BOOLEAN
(NTAPI KMESSAGE_SERVICE_ROUTINE)(
  _In_ struct _KINTERRUPT *Interrupt,
  _In_ PVOID ServiceContext,
  _In_ ULONG MessageID);
typedef KMESSAGE_SERVICE_ROUTINE *PKMESSAGE_SERVICE_ROUTINE;

typedef enum _KD_OPTION {
  KD_OPTION_SET_BLOCK_ENABLE,
} KD_OPTION;

#ifdef _NTSYSTEM_
typedef VOID
(NTAPI *PKNORMAL_ROUTINE)(
  IN PVOID NormalContext OPTIONAL,
  IN PVOID SystemArgument1 OPTIONAL,
  IN PVOID SystemArgument2 OPTIONAL);

typedef VOID
(NTAPI *PKRUNDOWN_ROUTINE)(
  IN struct _KAPC *Apc);

typedef VOID
(NTAPI *PKKERNEL_ROUTINE)(
  IN struct _KAPC *Apc,
  IN OUT PKNORMAL_ROUTINE *NormalRoutine OPTIONAL,
  IN OUT PVOID *NormalContext OPTIONAL,
  IN OUT PVOID *SystemArgument1 OPTIONAL,
  IN OUT PVOID *SystemArgument2 OPTIONAL);
#endif

typedef struct _KAPC {
  UCHAR Type;
  UCHAR SpareByte0;
  UCHAR Size;
  UCHAR SpareByte1;
  ULONG SpareLong0;
  struct _KTHREAD *Thread;
  LIST_ENTRY ApcListEntry;
#ifdef _NTSYSTEM_
  PKKERNEL_ROUTINE KernelRoutine;
  PKRUNDOWN_ROUTINE RundownRoutine;
  PKNORMAL_ROUTINE NormalRoutine;
#else
  PVOID Reserved[3];
#endif
  PVOID NormalContext;
  PVOID SystemArgument1;
  PVOID SystemArgument2;
  CCHAR ApcStateIndex;
  KPROCESSOR_MODE ApcMode;
  BOOLEAN Inserted;
} KAPC, *PKAPC, *RESTRICTED_POINTER PRKAPC;

#define KAPC_OFFSET_TO_SPARE_BYTE0 FIELD_OFFSET(KAPC, SpareByte0)
#define KAPC_OFFSET_TO_SPARE_BYTE1 FIELD_OFFSET(KAPC, SpareByte1)
#define KAPC_OFFSET_TO_SPARE_LONG FIELD_OFFSET(KAPC, SpareLong0)
#define KAPC_OFFSET_TO_SYSTEMARGUMENT1 FIELD_OFFSET(KAPC, SystemArgument1)
#define KAPC_OFFSET_TO_SYSTEMARGUMENT2 FIELD_OFFSET(KAPC, SystemArgument2)
#define KAPC_OFFSET_TO_APCSTATEINDEX FIELD_OFFSET(KAPC, ApcStateIndex)
#define KAPC_ACTUAL_LENGTH (FIELD_OFFSET(KAPC, Inserted) + sizeof(BOOLEAN))

typedef struct _KDEVICE_QUEUE_ENTRY {
  LIST_ENTRY DeviceListEntry;
  ULONG SortKey;
  BOOLEAN Inserted;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY,
*RESTRICTED_POINTER PRKDEVICE_QUEUE_ENTRY;

typedef PVOID PKIPI_CONTEXT;

typedef VOID
(NTAPI *PKIPI_WORKER)(
  IN OUT PKIPI_CONTEXT PacketContext,
  IN PVOID Parameter1 OPTIONAL,
  IN PVOID Parameter2 OPTIONAL,
  IN PVOID Parameter3 OPTIONAL);

typedef struct _KIPI_COUNTS {
  ULONG Freeze;
  ULONG Packet;
  ULONG DPC;
  ULONG APC;
  ULONG FlushSingleTb;
  ULONG FlushMultipleTb;
  ULONG FlushEntireTb;
  ULONG GenericCall;
  ULONG ChangeColor;
  ULONG SweepDcache;
  ULONG SweepIcache;
  ULONG SweepIcacheRange;
  ULONG FlushIoBuffers;
  ULONG GratuitousDPC;
} KIPI_COUNTS, *PKIPI_COUNTS;

_IRQL_requires_same_
_Function_class_(KIPI_BROADCAST_WORKER)
_IRQL_requires_(IPI_LEVEL)
typedef ULONG_PTR
(NTAPI KIPI_BROADCAST_WORKER)(
  _In_ ULONG_PTR Argument);
typedef KIPI_BROADCAST_WORKER *PKIPI_BROADCAST_WORKER;

typedef ULONG_PTR KSPIN_LOCK, *PKSPIN_LOCK;

typedef struct _KSPIN_LOCK_QUEUE {
  struct _KSPIN_LOCK_QUEUE *volatile Next;
  PKSPIN_LOCK volatile Lock;
} KSPIN_LOCK_QUEUE, *PKSPIN_LOCK_QUEUE;

typedef struct _KLOCK_QUEUE_HANDLE {
  KSPIN_LOCK_QUEUE LockQueue;
  KIRQL OldIrql;
} KLOCK_QUEUE_HANDLE, *PKLOCK_QUEUE_HANDLE;

#if defined(_AMD64_)

typedef ULONG64 KSPIN_LOCK_QUEUE_NUMBER;

#define LockQueueDispatcherLock 0
#define LockQueueExpansionLock 1
#define LockQueuePfnLock 2
#define LockQueueSystemSpaceLock 3
#define LockQueueVacbLock 4
#define LockQueueMasterLock 5
#define LockQueueNonPagedPoolLock 6
#define LockQueueIoCancelLock 7
#define LockQueueWorkQueueLock 8
#define LockQueueIoVpbLock 9
#define LockQueueIoDatabaseLock 10
#define LockQueueIoCompletionLock 11
#define LockQueueNtfsStructLock 12
#define LockQueueAfdWorkQueueLock 13
#define LockQueueBcbLock 14
#define LockQueueMmNonPagedPoolLock 15
#define LockQueueUnusedSpare16 16
#define LockQueueTimerTableLock 17
#define LockQueueMaximumLock (LockQueueTimerTableLock + LOCK_QUEUE_TIMER_TABLE_LOCKS)

#else

typedef enum _KSPIN_LOCK_QUEUE_NUMBER {
  LockQueueDispatcherLock,
  LockQueueExpansionLock,
  LockQueuePfnLock,
  LockQueueSystemSpaceLock,
  LockQueueVacbLock,
  LockQueueMasterLock,
  LockQueueNonPagedPoolLock,
  LockQueueIoCancelLock,
  LockQueueWorkQueueLock,
  LockQueueIoVpbLock,
  LockQueueIoDatabaseLock,
  LockQueueIoCompletionLock,
  LockQueueNtfsStructLock,
  LockQueueAfdWorkQueueLock,
  LockQueueBcbLock,
  LockQueueMmNonPagedPoolLock,
  LockQueueUnusedSpare16,
  LockQueueTimerTableLock,
  LockQueueMaximumLock = LockQueueTimerTableLock + LOCK_QUEUE_TIMER_TABLE_LOCKS
} KSPIN_LOCK_QUEUE_NUMBER, *PKSPIN_LOCK_QUEUE_NUMBER;

#endif /* defined(_AMD64_) */

_Function_class_(KDEFERRED_ROUTINE)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
typedef VOID
(NTAPI KDEFERRED_ROUTINE)(
  _In_ struct _KDPC *Dpc,
  _In_opt_ PVOID DeferredContext,
  _In_opt_ PVOID SystemArgument1,
  _In_opt_ PVOID SystemArgument2);
typedef KDEFERRED_ROUTINE *PKDEFERRED_ROUTINE;

typedef enum _KDPC_IMPORTANCE {
  LowImportance,
  MediumImportance,
  HighImportance,
  MediumHighImportance
} KDPC_IMPORTANCE;

typedef struct _KDPC {
  UCHAR Type;
  UCHAR Importance;
  volatile USHORT Number;
  LIST_ENTRY DpcListEntry;
  PKDEFERRED_ROUTINE DeferredRoutine;
  PVOID DeferredContext;
  PVOID SystemArgument1;
  PVOID SystemArgument2;
  volatile PVOID DpcData;
} KDPC, *PKDPC, *RESTRICTED_POINTER PRKDPC;

typedef struct _KDPC_WATCHDOG_INFORMATION {
  ULONG DpcTimeLimit;
  ULONG DpcTimeCount;
  ULONG DpcWatchdogLimit;
  ULONG DpcWatchdogCount;
  ULONG Reserved;
} KDPC_WATCHDOG_INFORMATION, *PKDPC_WATCHDOG_INFORMATION;

typedef struct _KDEVICE_QUEUE {
  CSHORT Type;
  CSHORT Size;
  LIST_ENTRY DeviceListHead;
  KSPIN_LOCK Lock;
# if defined(_AMD64_)
  _ANONYMOUS_UNION union {
    BOOLEAN Busy;
    _ANONYMOUS_STRUCT struct {
      LONG64 Reserved:8;
      LONG64 Hint:56;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
# else
  BOOLEAN Busy;
# endif
} KDEVICE_QUEUE, *PKDEVICE_QUEUE, *RESTRICTED_POINTER PRKDEVICE_QUEUE;

#define TIMER_EXPIRED_INDEX_BITS        6
#define TIMER_PROCESSOR_INDEX_BITS      5

typedef struct _DISPATCHER_HEADER {
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      UCHAR Type;
      _ANONYMOUS_UNION union {
        _ANONYMOUS_UNION union {
          UCHAR TimerControlFlags;
          _ANONYMOUS_STRUCT struct {
            UCHAR Absolute:1;
            UCHAR Coalescable:1;
            UCHAR KeepShifting:1;
            UCHAR EncodedTolerableDelay:5;
          } DUMMYSTRUCTNAME;
        } DUMMYUNIONNAME;
        UCHAR Abandoned;
#if (NTDDI_VERSION < NTDDI_WIN7)
        UCHAR NpxIrql;
#endif
        BOOLEAN Signalling;
      } DUMMYUNIONNAME;
      _ANONYMOUS_UNION union {
        _ANONYMOUS_UNION union {
          UCHAR ThreadControlFlags;
          _ANONYMOUS_STRUCT struct {
            UCHAR CpuThrottled:1;
            UCHAR CycleProfiling:1;
            UCHAR CounterProfiling:1;
            UCHAR Reserved:5;
          } DUMMYSTRUCTNAME;
        } DUMMYUNIONNAME;
        UCHAR Size;
        UCHAR Hand;
      } DUMMYUNIONNAME2;
      _ANONYMOUS_UNION union {
#if (NTDDI_VERSION >= NTDDI_WIN7)
        _ANONYMOUS_UNION union {
          UCHAR TimerMiscFlags;
          _ANONYMOUS_STRUCT struct {
#if !defined(_X86_)
            UCHAR Index:TIMER_EXPIRED_INDEX_BITS;
#else
            UCHAR Index:1;
            UCHAR Processor:TIMER_PROCESSOR_INDEX_BITS;
#endif
            UCHAR Inserted:1;
            volatile UCHAR Expired:1;
          } DUMMYSTRUCTNAME;
        } DUMMYUNIONNAME;
#else
        /* Pre Win7 compatibility fix to latest WDK */
        UCHAR Inserted;
#endif
        _ANONYMOUS_UNION union {
          BOOLEAN DebugActive;
          _ANONYMOUS_STRUCT struct {
            BOOLEAN ActiveDR7:1;
            BOOLEAN Instrumented:1;
            BOOLEAN Reserved2:4;
            BOOLEAN UmsScheduled:1;
            BOOLEAN UmsPrimary:1;
          } DUMMYSTRUCTNAME;
        } DUMMYUNIONNAME; /* should probably be DUMMYUNIONNAME2, but this is what WDK says */
        BOOLEAN DpcActive;
      } DUMMYUNIONNAME3;
    } DUMMYSTRUCTNAME;
    volatile LONG Lock;
  } DUMMYUNIONNAME;
  LONG SignalState;
  LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER, *PDISPATCHER_HEADER;

typedef struct _KEVENT {
  DISPATCHER_HEADER Header;
} KEVENT, *PKEVENT, *RESTRICTED_POINTER PRKEVENT;

typedef struct _KSEMAPHORE {
  DISPATCHER_HEADER Header;
  LONG Limit;
} KSEMAPHORE, *PKSEMAPHORE, *RESTRICTED_POINTER PRKSEMAPHORE;

#define KSEMAPHORE_ACTUAL_LENGTH (FIELD_OFFSET(KSEMAPHORE, Limit) + sizeof(LONG))

typedef struct _KGATE {
  DISPATCHER_HEADER Header;
} KGATE, *PKGATE, *RESTRICTED_POINTER PRKGATE;

typedef struct _KGUARDED_MUTEX {
  volatile LONG Count;
  PKTHREAD Owner;
  ULONG Contention;
  KGATE Gate;
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      SHORT KernelApcDisable;
      SHORT SpecialApcDisable;
    } DUMMYSTRUCTNAME;
    ULONG CombinedApcDisable;
  } DUMMYUNIONNAME;
} KGUARDED_MUTEX, *PKGUARDED_MUTEX;

typedef struct _KMUTANT {
  DISPATCHER_HEADER Header;
  LIST_ENTRY MutantListEntry;
  struct _KTHREAD *RESTRICTED_POINTER OwnerThread;
  BOOLEAN Abandoned;
  UCHAR ApcDisable;
} KMUTANT, *PKMUTANT, *RESTRICTED_POINTER PRKMUTANT, KMUTEX, *PKMUTEX, *RESTRICTED_POINTER PRKMUTEX;

#define TIMER_TABLE_SIZE 512
#define TIMER_TABLE_SHIFT 9

typedef struct _KTIMER {
  DISPATCHER_HEADER Header;
  ULARGE_INTEGER DueTime;
  LIST_ENTRY TimerListEntry;
  struct _KDPC *Dpc;
#if (NTDDI_VERSION >= NTDDI_WIN7) && !defined(_X86_)
  ULONG Processor;
#endif
  ULONG Period;
} KTIMER, *PKTIMER, *RESTRICTED_POINTER PRKTIMER;

typedef enum _LOCK_OPERATION {
  IoReadAccess,
  IoWriteAccess,
  IoModifyAccess
} LOCK_OPERATION;

#define KTIMER_ACTUAL_LENGTH (FIELD_OFFSET(KTIMER, Period) + sizeof(LONG))

_Function_class_(KSYNCHRONIZE_ROUTINE)
_IRQL_requires_same_
typedef BOOLEAN
(NTAPI KSYNCHRONIZE_ROUTINE)(
  _In_ PVOID SynchronizeContext);
typedef KSYNCHRONIZE_ROUTINE *PKSYNCHRONIZE_ROUTINE;

typedef enum _POOL_TYPE {
  NonPagedPool,
  PagedPool,
  NonPagedPoolMustSucceed,
  DontUseThisType,
  NonPagedPoolCacheAligned,
  PagedPoolCacheAligned,
  NonPagedPoolCacheAlignedMustS,
  MaxPoolType,

  NonPagedPoolBase = 0,
  NonPagedPoolBaseMustSucceed = NonPagedPoolBase + 2,
  NonPagedPoolBaseCacheAligned = NonPagedPoolBase + 4,
  NonPagedPoolBaseCacheAlignedMustS = NonPagedPoolBase + 6,

  NonPagedPoolSession = 32,
  PagedPoolSession,
  NonPagedPoolMustSucceedSession,
  DontUseThisTypeSession,
  NonPagedPoolCacheAlignedSession,
  PagedPoolCacheAlignedSession,
  NonPagedPoolCacheAlignedMustSSession
} POOL_TYPE;

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE {
  StandardDesign,
  NEC98x86,
  EndAlternatives
} ALTERNATIVE_ARCHITECTURE_TYPE;

/* Correctly define these run-time definitions for non X86 machines */
#ifndef _X86_

#ifndef IsNEC_98
#define IsNEC_98 (FALSE)
#endif

#ifndef IsNotNEC_98
#define IsNotNEC_98 (TRUE)
#endif

#ifndef SetNEC_98
#define SetNEC_98
#endif

#ifndef SetNotNEC_98
#define SetNotNEC_98
#endif

#endif

typedef struct _KSYSTEM_TIME {
  ULONG LowPart;
  LONG High1Time;
  LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

typedef struct DECLSPEC_ALIGN(16) _M128A {
  ULONGLONG Low;
  LONGLONG High;
} M128A, *PM128A;

typedef struct DECLSPEC_ALIGN(16) _XSAVE_FORMAT {
  USHORT ControlWord;
  USHORT StatusWord;
  UCHAR TagWord;
  UCHAR Reserved1;
  USHORT ErrorOpcode;
  ULONG ErrorOffset;
  USHORT ErrorSelector;
  USHORT Reserved2;
  ULONG DataOffset;
  USHORT DataSelector;
  USHORT Reserved3;
  ULONG MxCsr;
  ULONG MxCsr_Mask;
  M128A FloatRegisters[8];
#if defined(_WIN64)
  M128A XmmRegisters[16];
  UCHAR Reserved4[96];
#else
  M128A XmmRegisters[8];
  UCHAR Reserved4[192];
  ULONG StackControl[7];
  ULONG Cr0NpxState;
#endif
} XSAVE_FORMAT, *PXSAVE_FORMAT;

typedef struct DECLSPEC_ALIGN(8) _XSAVE_AREA_HEADER {
  ULONG64 Mask;
  ULONG64 Reserved[7];
} XSAVE_AREA_HEADER, *PXSAVE_AREA_HEADER;

typedef struct DECLSPEC_ALIGN(16) _XSAVE_AREA {
  XSAVE_FORMAT LegacyState;
  XSAVE_AREA_HEADER Header;
} XSAVE_AREA, *PXSAVE_AREA;

typedef struct _XSTATE_CONTEXT {
  ULONG64 Mask;
  ULONG Length;
  ULONG Reserved1;
  _Field_size_bytes_opt_(Length) PXSAVE_AREA Area;
#if defined(_X86_)
  ULONG Reserved2;
#endif
  PVOID Buffer;
#if defined(_X86_)
  ULONG Reserved3;
#endif
} XSTATE_CONTEXT, *PXSTATE_CONTEXT;

typedef struct _XSTATE_SAVE {
#if defined(_AMD64_)
  struct _XSTATE_SAVE* Prev;
  struct _KTHREAD* Thread;
  UCHAR Level;
  XSTATE_CONTEXT XStateContext;
#elif defined(_IA64_) || defined(_ARM_) || defined(_ARM64_)
  ULONG Dummy;
#elif defined(_X86_)
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      LONG64 Reserved1;
      ULONG Reserved2;
      struct _XSTATE_SAVE* Prev;
      PXSAVE_AREA Reserved3;
      struct _KTHREAD* Thread;
      PVOID Reserved4;
      UCHAR Level;
    } DUMMYSTRUCTNAME;
    XSTATE_CONTEXT XStateContext;
  } DUMMYUNIONNAME;
#endif
} XSTATE_SAVE, *PXSTATE_SAVE;

#ifdef _X86_

#define MAXIMUM_SUPPORTED_EXTENSION  512

#if !defined(__midl) && !defined(MIDL_PASS)
C_ASSERT(sizeof(XSAVE_FORMAT) == MAXIMUM_SUPPORTED_EXTENSION);
#endif

#endif /* _X86_ */

#define XSAVE_ALIGN                    64
#define MINIMAL_XSTATE_AREA_LENGTH     sizeof(XSAVE_AREA)

#if !defined(__midl) && !defined(MIDL_PASS)
C_ASSERT((sizeof(XSAVE_FORMAT) & (XSAVE_ALIGN - 1)) == 0);
C_ASSERT((FIELD_OFFSET(XSAVE_AREA, Header) & (XSAVE_ALIGN - 1)) == 0);
C_ASSERT(MINIMAL_XSTATE_AREA_LENGTH == 512 + 64);
#endif

typedef struct _CONTEXT_CHUNK {
  LONG Offset;
  ULONG Length;
} CONTEXT_CHUNK, *PCONTEXT_CHUNK;

typedef struct _CONTEXT_EX {
  CONTEXT_CHUNK All;
  CONTEXT_CHUNK Legacy;
  CONTEXT_CHUNK XState;
} CONTEXT_EX, *PCONTEXT_EX;

#define CONTEXT_EX_LENGTH         ALIGN_UP_BY(sizeof(CONTEXT_EX), STACK_ALIGN)

#if (NTDDI_VERSION >= NTDDI_VISTA)
extern NTSYSAPI volatile CCHAR KeNumberProcessors;
#elif (NTDDI_VERSION >= NTDDI_WINXP)
extern NTSYSAPI CCHAR KeNumberProcessors;
#else
__CREATE_NTOS_DATA_IMPORT_ALIAS(KeNumberProcessors)
extern PCCHAR KeNumberProcessors;
#endif

$endif (_WDMDDK_)
$if (_NTDDK_)

typedef struct _EXCEPTION_REGISTRATION_RECORD
{
  struct _EXCEPTION_REGISTRATION_RECORD *Next;
  PEXCEPTION_ROUTINE Handler;
} EXCEPTION_REGISTRATION_RECORD, *PEXCEPTION_REGISTRATION_RECORD;

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
  _ANONYMOUS_UNION union {
    ULONG FiberData;
    ULONG Version;
  } DUMMYUNIONNAME;
  ULONG ArbitraryUserPointer;
  ULONG Self;
} NT_TIB32,*PNT_TIB32;

typedef struct _NT_TIB64 {
  ULONG64 ExceptionList;
  ULONG64 StackBase;
  ULONG64 StackLimit;
  ULONG64 SubSystemTib;
  _ANONYMOUS_UNION union {
    ULONG64 FiberData;
    ULONG Version;
  } DUMMYUNIONNAME;
  ULONG64 ArbitraryUserPointer;
  ULONG64 Self;
} NT_TIB64,*PNT_TIB64;

_IRQL_requires_same_
_Function_class_(EXPAND_STACK_CALLOUT)
typedef VOID
(NTAPI EXPAND_STACK_CALLOUT)(
  _In_opt_ PVOID Parameter);
typedef EXPAND_STACK_CALLOUT *PEXPAND_STACK_CALLOUT;

typedef VOID
(NTAPI *PTIMER_APC_ROUTINE)(
  _In_ PVOID TimerContext,
  _In_ ULONG TimerLowValue,
  _In_ LONG TimerHighValue);

typedef enum _TIMER_SET_INFORMATION_CLASS {
  TimerSetCoalescableTimer,
  MaxTimerInfoClass
} TIMER_SET_INFORMATION_CLASS;

#if (NTDDI_VERSION >= NTDDI_WIN7)
typedef struct _TIMER_SET_COALESCABLE_TIMER_INFO {
  _In_ LARGE_INTEGER DueTime;
  _In_opt_ PTIMER_APC_ROUTINE TimerApcRoutine;
  _In_opt_ PVOID TimerContext;
  _In_opt_ struct _COUNTED_REASON_CONTEXT *WakeContext;
  _In_opt_ ULONG Period;
  _In_ ULONG TolerableDelay;
  _Out_opt_ PBOOLEAN PreviousState;
} TIMER_SET_COALESCABLE_TIMER_INFO, *PTIMER_SET_COALESCABLE_TIMER_INFO;
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#define XSTATE_LEGACY_FLOATING_POINT        0
#define XSTATE_LEGACY_SSE                   1
#define XSTATE_GSSE                         2

#define XSTATE_MASK_LEGACY_FLOATING_POINT   (1LL << (XSTATE_LEGACY_FLOATING_POINT))
#define XSTATE_MASK_LEGACY_SSE              (1LL << (XSTATE_LEGACY_SSE))
#define XSTATE_MASK_LEGACY                  (XSTATE_MASK_LEGACY_FLOATING_POINT | XSTATE_MASK_LEGACY_SSE)
#define XSTATE_MASK_GSSE                    (1LL << (XSTATE_GSSE))

#define MAXIMUM_XSTATE_FEATURES             64

typedef struct _XSTATE_FEATURE {
  ULONG Offset;
  ULONG Size;
} XSTATE_FEATURE, *PXSTATE_FEATURE;

typedef struct _XSTATE_CONFIGURATION {
  ULONG64 EnabledFeatures;
  ULONG Size;
  ULONG OptimizedSave:1;
  XSTATE_FEATURE Features[MAXIMUM_XSTATE_FEATURES];
} XSTATE_CONFIGURATION, *PXSTATE_CONFIGURATION;

#define MAX_WOW64_SHARED_ENTRIES 16

//
// Flags for NXSupportPolicy
//
#if (NTDDI_VERSION >= NTDDI_WINXPSP2)
#define NX_SUPPORT_POLICY_ALWAYSOFF 0
#define NX_SUPPORT_POLICY_ALWAYSON  1
#define NX_SUPPORT_POLICY_OPTIN     2
#define NX_SUPPORT_POLICY_OPTOUT    3
#endif

typedef struct _KUSER_SHARED_DATA {
  ULONG TickCountLowDeprecated;
  ULONG TickCountMultiplier;
  volatile KSYSTEM_TIME InterruptTime;
  volatile KSYSTEM_TIME SystemTime;
  volatile KSYSTEM_TIME TimeZoneBias;
  USHORT ImageNumberLow;
  USHORT ImageNumberHigh;
  WCHAR NtSystemRoot[260];
  ULONG MaxStackTraceDepth;
  ULONG CryptoExponent;
  ULONG TimeZoneId;
  ULONG LargePageMinimum;
  ULONG Reserved2[7];
  NT_PRODUCT_TYPE NtProductType;
  BOOLEAN ProductTypeIsValid;
  ULONG NtMajorVersion;
  ULONG NtMinorVersion;
  BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];
  ULONG Reserved1;
  ULONG Reserved3;
  volatile ULONG TimeSlip;
  ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
  ULONG AltArchitecturePad[1];
  LARGE_INTEGER SystemExpirationDate;
  ULONG SuiteMask;
  BOOLEAN KdDebuggerEnabled;
#if (NTDDI_VERSION >= NTDDI_WINXPSP2)
  UCHAR NXSupportPolicy;
#endif
  volatile ULONG ActiveConsoleId;
  volatile ULONG DismountCount;
  ULONG ComPlusPackage;
  ULONG LastSystemRITEventTickCount;
  ULONG NumberOfPhysicalPages;
  BOOLEAN SafeBootMode;
#if (NTDDI_VERSION >= NTDDI_WIN7)
  _ANONYMOUS_UNION union {
    UCHAR TscQpcData;
    _ANONYMOUS_STRUCT struct {
      UCHAR TscQpcEnabled:1;
      UCHAR TscQpcSpareFlag:1;
      UCHAR TscQpcShift:6;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  UCHAR TscQpcPad[2];
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
  _ANONYMOUS_UNION union {
    ULONG SharedDataFlags;
    _ANONYMOUS_STRUCT struct {
      ULONG DbgErrorPortPresent:1;
      ULONG DbgElevationEnabled:1;
      ULONG DbgVirtEnabled:1;
      ULONG DbgInstallerDetectEnabled:1;
      ULONG DbgSystemDllRelocated:1;
      ULONG DbgDynProcessorEnabled:1;
      ULONG DbgSEHValidationEnabled:1;
      ULONG SpareBits:25;
    } DUMMYSTRUCTNAME2;
  } DUMMYUNIONNAME2;
#else
  ULONG TraceLogging;
#endif
  ULONG DataFlagsPad[1];
  ULONGLONG TestRetInstruction;
  ULONG SystemCall;
  ULONG SystemCallReturn;
  ULONGLONG SystemCallPad[3];
  _ANONYMOUS_UNION union {
    volatile KSYSTEM_TIME TickCount;
    volatile ULONG64 TickCountQuad;
    _ANONYMOUS_STRUCT struct {
      ULONG ReservedTickCountOverlay[3];
      ULONG TickCountPad[1];
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME3;
  ULONG Cookie;
  ULONG CookiePad[1];
#if (NTDDI_VERSION >= NTDDI_WS03)
  LONGLONG ConsoleSessionForegroundProcessId;
  ULONG Wow64SharedInformation[MAX_WOW64_SHARED_ENTRIES];
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
#if (NTDDI_VERSION >= NTDDI_WIN7)
  USHORT UserModeGlobalLogger[16];
#else
  USHORT UserModeGlobalLogger[8];
  ULONG HeapTracingPid[2];
  ULONG CritSecTracingPid[2];
#endif
  ULONG ImageFileExecutionOptions;
#if (NTDDI_VERSION >= NTDDI_VISTASP1)
  ULONG LangGenerationCount;
#else
  /* 4 bytes padding */
#endif
  ULONGLONG Reserved5;
  volatile ULONG64 InterruptTimeBias;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN7)
  volatile ULONG64 TscQpcBias;
  volatile ULONG ActiveProcessorCount;
  volatile USHORT ActiveGroupCount;
  USHORT Reserved4;
  volatile ULONG AitSamplingValue;
  volatile ULONG AppCompatFlag;
  ULONGLONG SystemDllNativeRelocation;
  ULONG SystemDllWowRelocation;
  ULONG XStatePad[1];
  XSTATE_CONFIGURATION XState;
#endif
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

#if (NTDDI_VERSION >= NTDDI_VISTA)
extern NTSYSAPI volatile CCHAR KeNumberProcessors;
#elif (NTDDI_VERSION >= NTDDI_WINXP)
extern NTSYSAPI CCHAR KeNumberProcessors;
#else
extern PCCHAR KeNumberProcessors;
#endif

$endif (_NTDDK_)
$if (_NTIFS_)
typedef struct _KAPC_STATE {
  LIST_ENTRY ApcListHead[MaximumMode];
  PKPROCESS Process;
  BOOLEAN KernelApcInProgress;
  BOOLEAN KernelApcPending;
  BOOLEAN UserApcPending;
} KAPC_STATE, *PKAPC_STATE, *RESTRICTED_POINTER PRKAPC_STATE;

#define KAPC_STATE_ACTUAL_LENGTH (FIELD_OFFSET(KAPC_STATE, UserApcPending) + sizeof(BOOLEAN))

#define ASSERT_QUEUE(Q) ASSERT(((Q)->Header.Type & KOBJECT_TYPE_MASK) == QueueObject);

typedef struct _KQUEUE {
  DISPATCHER_HEADER Header;
  LIST_ENTRY EntryListHead;
  volatile ULONG CurrentCount;
  ULONG MaximumCount;
  LIST_ENTRY ThreadListHead;
} KQUEUE, *PKQUEUE, *RESTRICTED_POINTER PRKQUEUE;

$endif (_NTIFS_)


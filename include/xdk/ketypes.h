/******************************************************************************
 *                              Kernel Types                                  *
 ******************************************************************************/

typedef UCHAR KIRQL, *PKIRQL;
typedef CCHAR KPROCESSOR_MODE;
typedef LONG KPRIORITY;

typedef ULONG EXECUTION_STATE;

typedef enum _MODE {
  KernelMode,
  UserMode,
  MaximumMode
} MODE;

/* Processor features */
#define PF_FLOATING_POINT_PRECISION_ERRATA  0
#define PF_FLOATING_POINT_EMULATED          1
#define PF_COMPARE_EXCHANGE_DOUBLE          2
#define PF_MMX_INSTRUCTIONS_AVAILABLE       3
#define PF_PPC_MOVEMEM_64BIT_OK             4
#define PF_ALPHA_BYTE_INSTRUCTIONS          5
#define PF_XMMI_INSTRUCTIONS_AVAILABLE      6
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE     7
#define PF_RDTSC_INSTRUCTION_AVAILABLE      8
#define PF_PAE_ENABLED                      9
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE   10
#define PF_SSE_DAZ_MODE_AVAILABLE          11
#define PF_NX_ENABLED                      12
#define PF_SSE3_INSTRUCTIONS_AVAILABLE     13
#define PF_COMPARE_EXCHANGE128             14
#define PF_COMPARE64_EXCHANGE128           15
#define PF_CHANNELS_ENABLED                16
#define PF_XSAVE_ENABLED                   17

#define MAXIMUM_WAIT_OBJECTS              64

#define ASSERT_APC(Object) \
    ASSERT((Object)->Type == ApcObject)

#define ASSERT_DPC(Object) \
    ASSERT(((Object)->Type == 0) || \
           ((Object)->Type == DpcObject) || \
           ((Object)->Type == ThreadedDpcObject))

#define ASSERT_GATE(object) \
    ASSERT((((object)->Header.Type & KOBJECT_TYPE_MASK) == GateObject) || \
           (((object)->Header.Type & KOBJECT_TYPE_MASK) == EventSynchronizationObject))

#define ASSERT_DEVICE_QUEUE(Object) \
    ASSERT((Object)->Type == DeviceQueueObject)

#define ASSERT_TIMER(E) \
    ASSERT(((E)->Header.Type == TimerNotificationObject) || \
           ((E)->Header.Type == TimerSynchronizationObject))

#define ASSERT_MUTANT(E) \
    ASSERT((E)->Header.Type == MutantObject)

#define ASSERT_SEMAPHORE(E) \
    ASSERT((E)->Header.Type == SemaphoreObject)

#define ASSERT_EVENT(E) \
    ASSERT(((E)->Header.Type == NotificationEvent) || \
           ((E)->Header.Type == SynchronizationEvent))

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

/* Exception Records */
#define EXCEPTION_NONCONTINUABLE 1
#define EXCEPTION_MAXIMUM_PARAMETERS 15

typedef struct _EXCEPTION_RECORD {
  NTSTATUS ExceptionCode;
  ULONG ExceptionFlags;
  struct _EXCEPTION_RECORD *ExceptionRecord;
  PVOID ExceptionAddress;
  ULONG NumberParameters;
  ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

typedef struct _EXCEPTION_RECORD32 {
  NTSTATUS ExceptionCode;
  ULONG ExceptionFlags;
  ULONG ExceptionRecord;
  ULONG ExceptionAddress;
  ULONG NumberParameters;
  ULONG ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD32, *PEXCEPTION_RECORD32;

typedef struct _EXCEPTION_RECORD64 {
  NTSTATUS ExceptionCode;
  ULONG ExceptionFlags;
  ULONG64 ExceptionRecord;
  ULONG64 ExceptionAddress;
  ULONG NumberParameters;
  ULONG __unusedAlignment;
  ULONG64 ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD64, *PEXCEPTION_RECORD64;

typedef struct _EXCEPTION_POINTERS {
  PEXCEPTION_RECORD ExceptionRecord;
  PCONTEXT ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;

typedef enum _KBUGCHECK_CALLBACK_REASON {
  KbCallbackInvalid,
  KbCallbackReserved1,
  KbCallbackSecondaryDumpData,
  KbCallbackDumpIo,
  KbCallbackAddPages
} KBUGCHECK_CALLBACK_REASON;

struct _KBUGCHECK_REASON_CALLBACK_RECORD;

typedef VOID
(NTAPI *PKBUGCHECK_REASON_CALLBACK_ROUTINE)(
  IN KBUGCHECK_CALLBACK_REASON Reason,
  IN struct _KBUGCHECK_REASON_CALLBACK_RECORD *Record,
  IN OUT PVOID ReasonSpecificData,
  IN ULONG ReasonSpecificDataLength);

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

typedef VOID
(NTAPI *PKBUGCHECK_CALLBACK_ROUTINE)(
  IN PVOID Buffer,
  IN ULONG Length);

typedef struct _KBUGCHECK_CALLBACK_RECORD {
  LIST_ENTRY Entry;
  PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine;
  PVOID Buffer;
  ULONG Length;
  PUCHAR Component;
  ULONG_PTR Checksum;
  UCHAR State;
} KBUGCHECK_CALLBACK_RECORD, *PKBUGCHECK_CALLBACK_RECORD;

typedef BOOLEAN
(NTAPI *PNMI_CALLBACK)(
  IN PVOID Context,
  IN BOOLEAN Handled);

typedef enum _TRACE_INFORMATION_CLASS {
  TraceIdClass,
  TraceHandleClass,
  TraceEnableFlagsClass,
  TraceEnableLevelClass,
  GlobalLoggerHandleClass,
  EventLoggerHandleClass,
  AllLoggerHandlesClass,
  TraceHandleByNameClass,
  LoggerEventsLostClass,
  TraceSessionSettingsClass,
  LoggerEventsLoggedClass,
  MaxTraceInformationClass
} TRACE_INFORMATION_CLASS;

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
  struct _KTHREAD *Thread;
  PVOID Object;
  struct _KWAIT_BLOCK *NextWaitBlock;
  USHORT WaitKey;
  UCHAR WaitType;
  volatile UCHAR BlockState;
#if defined(_WIN64)
  LONG SpareLong;
#endif
} KWAIT_BLOCK, *PKWAIT_BLOCK, *PRKWAIT_BLOCK;

typedef enum _KINTERRUPT_MODE {
  LevelSensitive,
  Latched
} KINTERRUPT_MODE;

#define THREAD_WAIT_OBJECTS 3

typedef VOID
(NTAPI *PKINTERRUPT_ROUTINE)(
  VOID);

typedef enum _KD_OPTION {
  KD_OPTION_SET_BLOCK_ENABLE,
} KD_OPTION;

typedef VOID
(NTAPI *PKNORMAL_ROUTINE)(
  IN PVOID NormalContext,
  IN PVOID SystemArgument1,
  IN PVOID SystemArgument2);

typedef VOID
(NTAPI *PKRUNDOWN_ROUTINE)(
  IN struct _KAPC *Apc);

typedef VOID
(NTAPI *PKKERNEL_ROUTINE)(
  IN struct _KAPC *Apc,
  IN OUT PKNORMAL_ROUTINE *NormalRoutine,
  IN OUT PVOID *NormalContext,
  IN OUT PVOID *SystemArgument1,
  IN OUT PVOID *SystemArgument2);

typedef struct _KAPC {
  UCHAR Type;
  UCHAR SpareByte0;
  UCHAR Size;
  UCHAR SpareByte1;
  ULONG SpareLong0;
  struct _KTHREAD *Thread;
  LIST_ENTRY ApcListEntry;
  PKKERNEL_ROUTINE KernelRoutine;
  PKRUNDOWN_ROUTINE RundownRoutine;
  PKNORMAL_ROUTINE NormalRoutine;
  PVOID NormalContext;
  PVOID SystemArgument1;
  PVOID SystemArgument2;
  CCHAR ApcStateIndex;
  KPROCESSOR_MODE ApcMode;
  BOOLEAN Inserted;
} KAPC, *PKAPC, *RESTRICTED_POINTER PRKAPC;

typedef struct _KDEVICE_QUEUE_ENTRY {
  LIST_ENTRY DeviceListEntry;
  ULONG SortKey;
  BOOLEAN Inserted;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY,
*RESTRICTED_POINTER PRKDEVICE_QUEUE_ENTRY;

typedef PVOID PKIPI_CONTEXT;

typedef VOID
(NTAPI *PKIPI_WORKER)(
  IN PKIPI_CONTEXT PacketContext,
  IN PVOID Parameter1,
  IN PVOID Parameter2,
  IN PVOID Parameter3);

typedef ULONG_PTR
(NTAPI *PKIPI_BROADCAST_WORKER)(
  IN ULONG_PTR Argument);

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

typedef VOID
(NTAPI *PKDEFERRED_ROUTINE)(
  IN struct _KDPC *Dpc,
  IN PVOID DeferredContext,
  IN PVOID SystemArgument1,
  IN PVOID SystemArgument2);

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
  volatile PVOID  DpcData;
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
  #if defined(_AMD64_)
  union {
    BOOLEAN Busy;
    struct {
      LONG64 Reserved:8;
      LONG64 Hint:56;
    };
  };
  #else
  BOOLEAN Busy;
  #endif
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

typedef struct _KGATE {
  DISPATCHER_HEADER Header;
} KGATE, *PKGATE, *RESTRICTED_POINTER PRKGATE;

typedef struct _KGUARDED_MUTEX {
  volatile LONG Count;
  PKTHREAD Owner;
  ULONG Contention;
  KGATE Gate;
  __GNU_EXTENSION union {
    __GNU_EXTENSION struct {
      SHORT KernelApcDisable;
      SHORT SpecialApcDisable;
    };
    ULONG CombinedApcDisable;
  };
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
  #if !defined(_X86_)
  ULONG Processor;
  #endif
  ULONG Period;
} KTIMER, *PKTIMER, *RESTRICTED_POINTER PRKTIMER;

typedef BOOLEAN
(NTAPI *PKSYNCHRONIZE_ROUTINE)(
  IN PVOID SynchronizeContext);

typedef enum _POOL_TYPE {
  NonPagedPool,
  PagedPool,
  NonPagedPoolMustSucceed,
  DontUseThisType,
  NonPagedPoolCacheAligned,
  PagedPoolCacheAligned,
  NonPagedPoolCacheAlignedMustS,
  MaxPoolType,
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

typedef struct _KSYSTEM_TIME {
  ULONG LowPart;
  LONG High1Time;
  LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

typedef struct _PNP_BUS_INFORMATION {
  GUID BusTypeGuid;
  INTERFACE_TYPE LegacyBusType;
  ULONG BusNumber;
} PNP_BUS_INFORMATION, *PPNP_BUS_INFORMATION;

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
  PXSAVE_AREA Area;
#if defined(_X86_)
  ULONG Reserved2;
#endif
  PVOID Buffer;
#if defined(_X86_)
  ULONG Reserved3;
#endif
} XSTATE_CONTEXT, *PXSTATE_CONTEXT;

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
extern PCCHAR KeNumberProcessors;
#endif


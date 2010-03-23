/******************************************************************************
 *                            Executive Types                                 *
 ******************************************************************************/
$if (_WDMDDK_)

#define EX_RUNDOWN_ACTIVE                 0x1
#define EX_RUNDOWN_COUNT_SHIFT            0x1
#define EX_RUNDOWN_COUNT_INC              (1 << EX_RUNDOWN_COUNT_SHIFT)

typedef struct _FAST_MUTEX {
  volatile LONG Count;
  PKTHREAD Owner;
  ULONG Contention;
  KEVENT Event;
  ULONG OldIrql;
} FAST_MUTEX, *PFAST_MUTEX;

typedef enum _SUITE_TYPE {
  SmallBusiness,
  Enterprise,
  BackOffice,
  CommunicationServer,
  TerminalServer,
  SmallBusinessRestricted,
  EmbeddedNT,
  DataCenter,
  SingleUserTS,
  Personal,
  Blade,
  EmbeddedRestricted,
  SecurityAppliance,
  StorageServer,
  ComputeServer,
  WHServer,
  MaxSuiteType
} SUITE_TYPE;

typedef enum _EX_POOL_PRIORITY {
  LowPoolPriority,
  LowPoolPrioritySpecialPoolOverrun = 8,
  LowPoolPrioritySpecialPoolUnderrun = 9,
  NormalPoolPriority = 16,
  NormalPoolPrioritySpecialPoolOverrun = 24,
  NormalPoolPrioritySpecialPoolUnderrun = 25,
  HighPoolPriority = 32,
  HighPoolPrioritySpecialPoolOverrun = 40,
  HighPoolPrioritySpecialPoolUnderrun = 41
} EX_POOL_PRIORITY;

#if !defined(_WIN64) && (defined(_NTDDK_) || defined(_NTIFS_) || defined(_NDIS_))
#define LOOKASIDE_ALIGN
#else
#define LOOKASIDE_ALIGN /* FIXME: DECLSPEC_CACHEALIGN */
#endif

typedef struct _LOOKASIDE_LIST_EX *PLOOKASIDE_LIST_EX;

typedef PVOID
(NTAPI *PALLOCATE_FUNCTION)(
  IN POOL_TYPE PoolType,
  IN SIZE_T NumberOfBytes,
  IN ULONG Tag);

typedef PVOID
(NTAPI *PALLOCATE_FUNCTION_EX)(
  IN POOL_TYPE PoolType,
  IN SIZE_T NumberOfBytes,
  IN ULONG Tag,
  IN OUT PLOOKASIDE_LIST_EX Lookaside);

typedef VOID
(NTAPI *PFREE_FUNCTION)(
  IN PVOID Buffer);

typedef VOID
(NTAPI *PFREE_FUNCTION_EX)(
  IN PVOID Buffer,
  IN OUT PLOOKASIDE_LIST_EX Lookaside);

typedef VOID
(NTAPI CALLBACK_FUNCTION)(
  IN PVOID CallbackContext OPTIONAL,
  IN PVOID Argument1 OPTIONAL,
  IN PVOID Argument2 OPTIONAL);
typedef CALLBACK_FUNCTION *PCALLBACK_FUNCTION;

#define GENERAL_LOOKASIDE_LAYOUT                \
    union {                                     \
        SLIST_HEADER ListHead;                  \
        SINGLE_LIST_ENTRY SingleListHead;       \
    } DUMMYUNIONNAME;                           \
    USHORT Depth;                               \
    USHORT MaximumDepth;                        \
    ULONG TotalAllocates;                       \
    union {                                     \
        ULONG AllocateMisses;                   \
        ULONG AllocateHits;                     \
    } DUMMYUNIONNAME2;                          \
                                                \
    ULONG TotalFrees;                           \
    union {                                     \
        ULONG FreeMisses;                       \
        ULONG FreeHits;                         \
    } DUMMYUNIONNAME3;                          \
                                                \
    POOL_TYPE Type;                             \
    ULONG Tag;                                  \
    ULONG Size;                                 \
    union {                                     \
        PALLOCATE_FUNCTION_EX AllocateEx;       \
        PALLOCATE_FUNCTION Allocate;            \
    } DUMMYUNIONNAME4;                          \
                                                \
    union {                                     \
        PFREE_FUNCTION_EX FreeEx;               \
        PFREE_FUNCTION Free;                    \
    } DUMMYUNIONNAME5;                          \
                                                \
    LIST_ENTRY ListEntry;                       \
    ULONG LastTotalAllocates;                   \
    union {                                     \
        ULONG LastAllocateMisses;               \
        ULONG LastAllocateHits;                 \
    } DUMMYUNIONNAME6;                          \
    ULONG Future[2];

typedef struct LOOKASIDE_ALIGN _GENERAL_LOOKASIDE {
  GENERAL_LOOKASIDE_LAYOUT
} GENERAL_LOOKASIDE, *PGENERAL_LOOKASIDE;

typedef struct _GENERAL_LOOKASIDE_POOL {
  GENERAL_LOOKASIDE_LAYOUT
} GENERAL_LOOKASIDE_POOL, *PGENERAL_LOOKASIDE_POOL;

#define LOOKASIDE_CHECK(f)  \
    C_ASSERT(FIELD_OFFSET(GENERAL_LOOKASIDE,f) == FIELD_OFFSET(GENERAL_LOOKASIDE_POOL,f))

LOOKASIDE_CHECK(TotalFrees);
LOOKASIDE_CHECK(Tag);
LOOKASIDE_CHECK(Future);

typedef struct _PAGED_LOOKASIDE_LIST {
  GENERAL_LOOKASIDE L;
#if !defined(_AMD64_) && !defined(_IA64_)
  FAST_MUTEX Lock__ObsoleteButDoNotDelete;
#endif
} PAGED_LOOKASIDE_LIST, *PPAGED_LOOKASIDE_LIST;

typedef struct LOOKASIDE_ALIGN _NPAGED_LOOKASIDE_LIST {
  GENERAL_LOOKASIDE L;
#if !defined(_AMD64_) && !defined(_IA64_)
  KSPIN_LOCK Lock__ObsoleteButDoNotDelete;
#endif
} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;

#define LOOKASIDE_MINIMUM_BLOCK_SIZE (RTL_SIZEOF_THROUGH_FIELD (SLIST_ENTRY, Next))

typedef struct _LOOKASIDE_LIST_EX {
  GENERAL_LOOKASIDE_POOL L;
} LOOKASIDE_LIST_EX;

#if (NTDDI_VERSION >= NTDDI_VISTA)

#define EX_LOOKASIDE_LIST_EX_FLAGS_RAISE_ON_FAIL 0x00000001UL
#define EX_LOOKASIDE_LIST_EX_FLAGS_FAIL_NO_RAISE 0x00000002UL

#define EX_MAXIMUM_LOOKASIDE_DEPTH_BASE          256
#define EX_MAXIMUM_LOOKASIDE_DEPTH_LIMIT         1024

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

typedef struct _EX_RUNDOWN_REF {
  __GNU_EXTENSION union {
    volatile ULONG_PTR Count;
    volatile PVOID Ptr;
  };
} EX_RUNDOWN_REF, *PEX_RUNDOWN_REF;

typedef struct _EX_RUNDOWN_REF_CACHE_AWARE *PEX_RUNDOWN_REF_CACHE_AWARE;

typedef enum _WORK_QUEUE_TYPE {
  CriticalWorkQueue,
  DelayedWorkQueue,
  HyperCriticalWorkQueue,
  MaximumWorkQueue
} WORK_QUEUE_TYPE;

typedef VOID
(NTAPI WORKER_THREAD_ROUTINE)(
  IN PVOID Parameter);
typedef WORKER_THREAD_ROUTINE *PWORKER_THREAD_ROUTINE;

typedef struct _WORK_QUEUE_ITEM {
  LIST_ENTRY List;
  PWORKER_THREAD_ROUTINE WorkerRoutine;
  volatile PVOID Parameter;
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;

typedef ULONG_PTR ERESOURCE_THREAD, *PERESOURCE_THREAD;

typedef struct _OWNER_ENTRY {
  ERESOURCE_THREAD OwnerThread;
  union {
    struct {
      ULONG IoPriorityBoosted:1;
      ULONG OwnerReferenced:1;
      ULONG OwnerCount:30;
    };
    ULONG TableSize;
  };
} OWNER_ENTRY, *POWNER_ENTRY;

typedef struct _ERESOURCE {
  LIST_ENTRY SystemResourcesList;
  POWNER_ENTRY OwnerTable;
  SHORT ActiveCount;
  USHORT Flag;
  volatile PKSEMAPHORE SharedWaiters;
  volatile PKEVENT ExclusiveWaiters;
  OWNER_ENTRY OwnerEntry;
  ULONG ActiveEntries;
  ULONG ContentionCount;
  ULONG NumberOfSharedWaiters;
  ULONG NumberOfExclusiveWaiters;
#if defined(_WIN64)
  PVOID Reserved2;
#endif
  __GNU_EXTENSION union {
    PVOID Address;
    ULONG_PTR CreatorBackTraceIndex;
  };
  KSPIN_LOCK SpinLock;
} ERESOURCE, *PERESOURCE;

/* ERESOURCE.Flag */
#define ResourceNeverExclusive            0x0010
#define ResourceReleaseByOtherThread      0x0020
#define ResourceOwnedExclusive            0x0080

#define RESOURCE_HASH_TABLE_SIZE          64

typedef struct _RESOURCE_HASH_ENTRY {
  LIST_ENTRY ListEntry;
  PVOID Address;
  ULONG ContentionCount;
  ULONG Number;
} RESOURCE_HASH_ENTRY, *PRESOURCE_HASH_ENTRY;

typedef struct _RESOURCE_PERFORMANCE_DATA {
  ULONG ActiveResourceCount;
  ULONG TotalResourceCount;
  ULONG ExclusiveAcquire;
  ULONG SharedFirstLevel;
  ULONG SharedSecondLevel;
  ULONG StarveFirstLevel;
  ULONG StarveSecondLevel;
  ULONG WaitForExclusive;
  ULONG OwnerTableExpands;
  ULONG MaximumTableExpand;
  LIST_ENTRY HashTable[RESOURCE_HASH_TABLE_SIZE];
} RESOURCE_PERFORMANCE_DATA, *PRESOURCE_PERFORMANCE_DATA;

/* Global debug flag */
#if DEVL
extern ULONG NtGlobalFlag;
#define IF_NTOS_DEBUG(FlagName) if (NtGlobalFlag & (FLG_##FlagName))
#else
#define IF_NTOS_DEBUG(FlagName) if(FALSE)
#endif

$endif /* _WDMDDK_ */
$if (_NTDDK_)

typedef struct _ZONE_SEGMENT_HEADER {
  SINGLE_LIST_ENTRY SegmentList;
  PVOID Reserved;
} ZONE_SEGMENT_HEADER, *PZONE_SEGMENT_HEADER;

typedef struct _ZONE_HEADER {
  SINGLE_LIST_ENTRY FreeList;
  SINGLE_LIST_ENTRY SegmentList;
  ULONG BlockSize;
  ULONG TotalSegmentSize;
} ZONE_HEADER, *PZONE_HEADER;

#define PROTECTED_POOL                    0x80000000

$endif /* _NTDDK_ */


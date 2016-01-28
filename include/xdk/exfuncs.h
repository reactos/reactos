/******************************************************************************
 *                          Executive Functions                               *
 ******************************************************************************/

$if (_WDMDDK_)
#define ExInterlockedIncrementLong(Addend,Lock) Exfi386InterlockedIncrementLong(Addend)
#define ExInterlockedDecrementLong(Addend,Lock) Exfi386InterlockedDecrementLong(Addend)
#define ExInterlockedExchangeUlong(Target, Value, Lock) Exfi386InterlockedExchangeUlong(Target, Value)

#define ExAcquireSpinLock(Lock, OldIrql) KeAcquireSpinLock((Lock), (OldIrql))
#define ExReleaseSpinLock(Lock, OldIrql) KeReleaseSpinLock((Lock), (OldIrql))
#define ExAcquireSpinLockAtDpcLevel(Lock) KeAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock) KeReleaseSpinLockFromDpcLevel(Lock)

#define ExInitializeSListHead InitializeSListHead

#if defined(_NTHAL_) && defined(_X86_)

NTKERNELAPI
VOID
FASTCALL
ExiAcquireFastMutex(
  IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
VOID
FASTCALL
ExiReleaseFastMutex(
  IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
BOOLEAN
FASTCALL
ExiTryToAcquireFastMutex(
    IN OUT PFAST_MUTEX FastMutex);

#define ExAcquireFastMutex ExiAcquireFastMutex
#define ExReleaseFastMutex ExiReleaseFastMutex
#define ExTryToAcquireFastMutex ExiTryToAcquireFastMutex

#else

#if (NTDDI_VERSION >= NTDDI_WIN2K)

_IRQL_raises_(APC_LEVEL)
_IRQL_saves_global_(OldIrql, FastMutex)
NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutex(
  _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_)
    PFAST_MUTEX FastMutex);

_IRQL_requires_(APC_LEVEL)
_IRQL_restores_global_(OldIrql, FastMutex)
NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutex(
  _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_)
    PFAST_MUTEX FastMutex);

_Must_inspect_result_
_Success_(return!=FALSE)
_IRQL_raises_(APC_LEVEL)
_IRQL_saves_global_(OldIrql, FastMutex)
NTKERNELAPI
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex(
  _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_)
    PFAST_MUTEX FastMutex);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#endif /* defined(_NTHAL_) && defined(_X86_) */

#if defined(_X86_)
#define ExInterlockedAddUlong ExfInterlockedAddUlong
#define ExInterlockedInsertHeadList ExfInterlockedInsertHeadList
#define ExInterlockedInsertTailList ExfInterlockedInsertTailList
#define ExInterlockedRemoveHeadList ExfInterlockedRemoveHeadList
#define ExInterlockedPopEntryList ExfInterlockedPopEntryList
#define ExInterlockedPushEntryList ExfInterlockedPushEntryList
#endif /* defined(_X86_) */

#ifdef _X86_

#ifdef _WIN2K_COMPAT_SLIST_USAGE

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
ExInterlockedPushEntrySList(
    _Inout_ PSLIST_HEADER SListHead,
    _Inout_ __drv_aliasesMem PSLIST_ENTRY SListEntry,
    _Inout_opt_ _Requires_lock_not_held_(*_Curr_) PKSPIN_LOCK Lock);

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
ExInterlockedPopEntrySList(
    _Inout_ PSLIST_HEADER SListHead,
    _Inout_opt_ _Requires_lock_not_held_(*_Curr_) PKSPIN_LOCK Lock);

#else /* !_WIN2K_COMPAT_SLIST_USAGE */

#define ExInterlockedPushEntrySList(SListHead, SListEntry, Lock) \
    InterlockedPushEntrySList(SListHead, SListEntry)

#define ExInterlockedPopEntrySList(SListHead, Lock) \
    InterlockedPopEntrySList(SListHead)

#endif /* _WIN2K_COMPAT_SLIST_USAGE */

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
ExInterlockedFlushSList(
    _Inout_ PSLIST_HEADER SListHead);

#ifdef NONAMELESSUNION
#define ExQueryDepthSList(SListHead) (SListHead)->s.Depth
#else
#define ExQueryDepthSList(SListHead) (SListHead)->Depth
#endif

#else /* !_X86_ */

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedPushEntrySList(
    _Inout_ PSLIST_HEADER SListHead,
    _Inout_ __drv_aliasesMem PSLIST_ENTRY SListEntry);

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedPopEntrySList(
    _Inout_ PSLIST_HEADER SListHead);

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedFlushSList(
    _Inout_ PSLIST_HEADER SListHead);

#if !defined(_NTSYSTEM_) && (defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_) || defined(_NTOSP_))
NTKERNELAPI
USHORT
ExQueryDepthSList(_In_ PSLIST_HEADER SListHead);
#else
FORCEINLINE
USHORT
ExQueryDepthSList(_In_ PSLIST_HEADER SListHead)
{
#ifdef _WIN64
    return (USHORT)(SListHead->Alignment & 0xffff);
#else /* !_WIN64 */
    return (USHORT)SListHead->Depth;
#endif /* _WIN64 */
}
#endif

#define ExInterlockedPushEntrySList(SListHead, SListEntry, Lock) \
    ExpInterlockedPushEntrySList(SListHead, SListEntry)

#define ExInterlockedPopEntrySList(SListHead, Lock) \
    ExpInterlockedPopEntrySList(SListHead)

#define ExInterlockedFlushSList(SListHead) \
    ExpInterlockedFlushSList(SListHead)

#endif /* _X86_ */


#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PVOID
NTAPI
ExAllocateFromPagedLookasideList(
  _Inout_ PPAGED_LOOKASIDE_LIST Lookaside);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExFreeToPagedLookasideList(
  _Inout_ PPAGED_LOOKASIDE_LIST Lookaside,
  _In_ PVOID Entry);

#else /* !_WIN2K_COMPAT_SLIST_USAGE */

_IRQL_requires_max_(APC_LEVEL)
static __inline
PVOID
ExAllocateFromPagedLookasideList(
  _Inout_ PPAGED_LOOKASIDE_LIST Lookaside)
{
  PVOID Entry;

  Lookaside->L.TotalAllocates++;
#ifdef NONAMELESSUNION
  Entry = InterlockedPopEntrySList(&Lookaside->L.u.ListHead);
  if (Entry == NULL) {
    Lookaside->L.u2.AllocateMisses++;
    Entry = (Lookaside->L.u4.Allocate)(Lookaside->L.Type,
                                       Lookaside->L.Size,
                                       Lookaside->L.Tag);
  }
#else /* NONAMELESSUNION */
  Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
  if (Entry == NULL) {
    Lookaside->L.AllocateMisses++;
    Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                    Lookaside->L.Size,
                                    Lookaside->L.Tag);
  }
#endif /* NONAMELESSUNION */
  return Entry;
}

_IRQL_requires_max_(APC_LEVEL)
static __inline
VOID
ExFreeToPagedLookasideList(
  _Inout_ PPAGED_LOOKASIDE_LIST Lookaside,
  _In_ PVOID Entry)
{
  Lookaside->L.TotalFrees++;
#ifdef NONAMELESSUNION
  if (ExQueryDepthSList(&Lookaside->L.u.ListHead) >= Lookaside->L.Depth) {
    Lookaside->L.u3.FreeMisses++;
    (Lookaside->L.u5.Free)(Entry);
  } else {
    InterlockedPushEntrySList(&Lookaside->L.u.ListHead, (PSLIST_ENTRY)Entry);
  }
#else /* NONAMELESSUNION */
  if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
    Lookaside->L.FreeMisses++;
    (Lookaside->L.Free)(Entry);
  } else {
    InterlockedPushEntrySList(&Lookaside->L.ListHead, (PSLIST_ENTRY)Entry);
  }
#endif /* NONAMELESSUNION */
}

#endif /* _WIN2K_COMPAT_SLIST_USAGE */


/* ERESOURCE_THREAD
 * ExGetCurrentResourceThread(
 *     VOID);
 */
#define ExGetCurrentResourceThread() ((ULONG_PTR)PsGetCurrentThread())

#define ExReleaseResource(R) (ExReleaseResourceLite(R))

/* VOID
 * ExInitializeWorkItem(
 *     IN PWORK_QUEUE_ITEM Item,
 *     IN PWORKER_THREAD_ROUTINE Routine,
 *     IN PVOID Context)
 */
#define ExInitializeWorkItem(Item, Routine, Context) \
{ \
  (Item)->WorkerRoutine = Routine; \
  (Item)->Parameter = Context; \
  (Item)->List.Flink = NULL; \
}

FORCEINLINE
VOID
ExInitializeFastMutex(
  _Out_ PFAST_MUTEX FastMutex)
{
  FastMutex->Count = FM_LOCK_BIT;
  FastMutex->Owner = NULL;
  FastMutex->Contention = 0;
  KeInitializeEvent(&FastMutex->Event, SynchronizationEvent, FALSE);
  return;
}

$endif (_WDMDDK_)
$if (_NTDDK_)
static __inline PVOID
ExAllocateFromZone(
  IN PZONE_HEADER Zone)
{
  PVOID Result = (PVOID)Zone->FreeList.Next;
  if (Zone->FreeList.Next)
    Zone->FreeList.Next = Zone->FreeList.Next->Next;
  return Result;
}

static __inline PVOID
ExFreeToZone(
  IN PZONE_HEADER Zone,
  IN PVOID Block)
{
  ((PSINGLE_LIST_ENTRY) Block)->Next = Zone->FreeList.Next;
  Zone->FreeList.Next = ((PSINGLE_LIST_ENTRY) Block);
  return ((PSINGLE_LIST_ENTRY) Block)->Next;
}

/*
 * PVOID
 * ExInterlockedAllocateFromZone(
 *   IN PZONE_HEADER  Zone,
 *   IN PKSPIN_LOCK  Lock)
 */
#define ExInterlockedAllocateFromZone(Zone, Lock) \
    ((PVOID) ExInterlockedPopEntryList(&Zone->FreeList, Lock))

/* PVOID
 * ExInterlockedFreeToZone(
 *  IN PZONE_HEADER  Zone,
 *  IN PVOID  Block,
 *  IN PKSPIN_LOCK  Lock);
 */
#define ExInterlockedFreeToZone(Zone, Block, Lock) \
    ExInterlockedPushEntryList(&(Zone)->FreeList, (PSINGLE_LIST_ENTRY)(Block), Lock)

/*
 * BOOLEAN
 * ExIsFullZone(
 *  IN PZONE_HEADER  Zone)
 */
#define ExIsFullZone(Zone) \
  ((Zone)->FreeList.Next == (PSINGLE_LIST_ENTRY) NULL)

/* BOOLEAN
 * ExIsObjectInFirstZoneSegment(
 *     IN PZONE_HEADER Zone,
 *     IN PVOID Object);
 */
#define ExIsObjectInFirstZoneSegment(Zone,Object) \
    ((BOOLEAN)( ((PUCHAR)(Object) >= (PUCHAR)(Zone)->SegmentList.Next) && \
                ((PUCHAR)(Object) <  (PUCHAR)(Zone)->SegmentList.Next + \
                         (Zone)->TotalSegmentSize)) )

#define ExAcquireResourceExclusive ExAcquireResourceExclusiveLite
#define ExAcquireResourceShared ExAcquireResourceSharedLite
#define ExConvertExclusiveToShared ExConvertExclusiveToSharedLite
#define ExDeleteResource ExDeleteResourceLite
#define ExInitializeResource ExInitializeResourceLite
#define ExIsResourceAcquiredExclusive ExIsResourceAcquiredExclusiveLite
#define ExIsResourceAcquiredShared ExIsResourceAcquiredSharedLite
#define ExIsResourceAcquired ExIsResourceAcquiredSharedLite
#define ExReleaseResourceForThread ExReleaseResourceForThreadLite

#ifndef _M_IX86
#define RESULT_ZERO     0
#define RESULT_NEGATIVE 1
#define RESULT_POSITIVE 2
#endif

typedef enum _INTERLOCKED_RESULT {
  ResultNegative = RESULT_NEGATIVE,
  ResultZero = RESULT_ZERO,
  ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

#ifdef _X86_

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedIncrementLong(
  _Inout_ _Interlocked_operand_ LONG volatile *Addend);

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedDecrementLong(
  _Inout_ _Interlocked_operand_ PLONG Addend);

NTKERNELAPI
ULONG
FASTCALL
Exfi386InterlockedExchangeUlong(
  _Inout_ _Interlocked_operand_ PULONG Target,
  _In_ ULONG Value);

#endif

$endif (_NTDDK_)
$if (_NTIFS_)

#define ExDisableResourceBoost ExDisableResourceBoostLite

VOID
NTAPI
ExInitializePushLock(
  _Out_ PEX_PUSH_LOCK PushLock);
$endif (_NTIFS_)

#if (NTDDI_VERSION >= NTDDI_WIN2K)
$if (_WDMDDK_)
_IRQL_requires_max_(APC_LEVEL)
_Requires_lock_held_(_Global_critical_region_)
NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutexUnsafe(
  _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_)
    PFAST_MUTEX FastMutex);

_IRQL_requires_max_(APC_LEVEL)
_Requires_lock_held_(_Global_critical_region_)
NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutexUnsafe(
  _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_)
    PFAST_MUTEX FastMutex);

_Requires_lock_held_(_Global_critical_region_)
_Requires_lock_not_held_(*Resource)
_When_(Wait!=0, _Acquires_exclusive_lock_(*Resource))
_IRQL_requires_max_(APC_LEVEL)
_When_(Wait!=0, _Post_satisfies_(return == 1))
_When_(Wait==0, _Post_satisfies_(return == 0 || return == 1) _Must_inspect_result_)
NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireResourceExclusiveLite(
  _Inout_ PERESOURCE Resource,
  _In_ _Literal_ BOOLEAN Wait);

_IRQL_requires_max_(APC_LEVEL)
_Requires_lock_held_(_Global_critical_region_)
_When_(Wait!=0, _Post_satisfies_(return == 1))
_When_(Wait==0, _Post_satisfies_(return == 0 || return == 1) _Must_inspect_result_)
NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireResourceSharedLite(
  _Inout_ _Requires_lock_not_held_(*_Curr_)
  _When_(return!=0, _Acquires_shared_lock_(*_Curr_))
    PERESOURCE Resource,
  _In_ BOOLEAN Wait);

_IRQL_requires_max_(APC_LEVEL)
_Requires_lock_held_(_Global_critical_region_)
_When_(Wait!=0, _Post_satisfies_(return == 1))
_When_(Wait==0, _Post_satisfies_(return == 0 || return == 1) _Must_inspect_result_)
NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireSharedStarveExclusive(
  _Inout_ _Requires_lock_not_held_(*_Curr_)
  _When_(return!=0, _Acquires_shared_lock_(*_Curr_))
    PERESOURCE Resource,
  _In_ BOOLEAN Wait);

_IRQL_requires_max_(APC_LEVEL)
_Requires_lock_held_(_Global_critical_region_)
_When_(Wait!=0, _Post_satisfies_(return == 1))
_When_(Wait==0, _Post_satisfies_(return == 0 || return == 1) _Must_inspect_result_)
NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireSharedWaitForExclusive(
  _Inout_ _Requires_lock_not_held_(*_Curr_)
  _When_(return!=0, _Acquires_lock_(*_Curr_))
    PERESOURCE Resource,
  _In_ BOOLEAN Wait);

__drv_preferredFunction("ExAllocatePoolWithTag",
                        "No tag interferes with debugging.")
__drv_allocatesMem(Mem)
_When_((PoolType & PagedPool) != 0, _IRQL_requires_max_(APC_LEVEL))
_When_((PoolType & PagedPool) == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_((PoolType & NonPagedPoolMustSucceed) != 0,
  __drv_reportError("Must succeed pool allocations are forbidden. "
                    "Allocation failures cause a system crash"))
_When_((PoolType & (NonPagedPoolMustSucceed |
                    POOL_RAISE_IF_ALLOCATION_FAILURE)) == 0,
  _Post_maybenull_ _Must_inspect_result_)
_When_((PoolType & (NonPagedPoolMustSucceed |
                    POOL_RAISE_IF_ALLOCATION_FAILURE)) != 0,
  _Post_notnull_)
_Post_writable_byte_size_(NumberOfBytes)
NTKERNELAPI
PVOID
NTAPI
ExAllocatePool(
  __drv_strictTypeMatch(__drv_typeExpr) _In_ POOL_TYPE PoolType,
  _In_ SIZE_T NumberOfBytes);

__drv_preferredFunction("ExAllocatePoolWithQuotaTag",
                        "No tag interferes with debugging.")
__drv_allocatesMem(Mem)
_When_((PoolType & PagedPool) != 0, _IRQL_requires_max_(APC_LEVEL))
_When_((PoolType & PagedPool) == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_((PoolType & NonPagedPoolMustSucceed) != 0,
  __drv_reportError("Must succeed pool allocations are forbidden. "
                    "Allocation failures cause a system crash"))
_When_((PoolType & POOL_QUOTA_FAIL_INSTEAD_OF_RAISE) != 0,
  _Post_maybenull_ _Must_inspect_result_)
_When_((PoolType & POOL_QUOTA_FAIL_INSTEAD_OF_RAISE) == 0, _Post_notnull_)
_Post_writable_byte_size_(NumberOfBytes)
NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithQuota(
  __drv_strictTypeMatch(__drv_typeExpr) _In_ POOL_TYPE PoolType,
  _In_ SIZE_T NumberOfBytes);

__drv_allocatesMem(Mem)
_When_((PoolType & PagedPool) != 0, _IRQL_requires_max_(APC_LEVEL))
_When_((PoolType & PagedPool) == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_((PoolType & NonPagedPoolMustSucceed) != 0,
  __drv_reportError("Must succeed pool allocations are forbidden. "
                    "Allocation failures cause a system crash"))
_When_((PoolType & POOL_QUOTA_FAIL_INSTEAD_OF_RAISE) != 0,
  _Post_maybenull_ _Must_inspect_result_)
_When_((PoolType & POOL_QUOTA_FAIL_INSTEAD_OF_RAISE) == 0, _Post_notnull_)
_Post_writable_byte_size_(NumberOfBytes)
NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithQuotaTag(
  _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE PoolType,
  _In_ SIZE_T NumberOfBytes,
  _In_ ULONG Tag);

#ifndef POOL_TAGGING
#define ExAllocatePoolWithQuotaTag(a,b,c) ExAllocatePoolWithQuota(a,b)
#endif

__drv_allocatesMem(Mem)
_When_((PoolType & PagedPool) != 0, _IRQL_requires_max_(APC_LEVEL))
_When_((PoolType & PagedPool) == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_((PoolType & NonPagedPoolMustSucceed) != 0,
  __drv_reportError("Must succeed pool allocations are forbidden. "
                    "Allocation failures cause a system crash"))
_When_((PoolType & (NonPagedPoolMustSucceed | POOL_RAISE_IF_ALLOCATION_FAILURE)) == 0,
  _Post_maybenull_ _Must_inspect_result_)
_When_((PoolType & (NonPagedPoolMustSucceed | POOL_RAISE_IF_ALLOCATION_FAILURE)) != 0,
  _Post_notnull_)
_Post_writable_byte_size_(NumberOfBytes)
_Function_class_(ALLOCATE_FUNCTION)
NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTag(
  _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE PoolType,
  _In_ SIZE_T NumberOfBytes,
  _In_ ULONG Tag);

#ifndef POOL_TAGGING
#define ExAllocatePoolWithTag(a,b,c) ExAllocatePool(a,b)
#endif

__drv_allocatesMem(Mem)
_When_((PoolType & PagedPool) != 0, _IRQL_requires_max_(APC_LEVEL))
_When_((PoolType & PagedPool) == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_((PoolType & NonPagedPoolMustSucceed) != 0,
  __drv_reportError("Must succeed pool allocations are forbidden. "
                    "Allocation failures cause a system crash"))
_When_((PoolType & (NonPagedPoolMustSucceed | POOL_RAISE_IF_ALLOCATION_FAILURE)) == 0,
  _Post_maybenull_ _Must_inspect_result_)
_When_((PoolType & (NonPagedPoolMustSucceed | POOL_RAISE_IF_ALLOCATION_FAILURE)) != 0,
  _Post_notnull_)
_Post_writable_byte_size_(NumberOfBytes)
NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTagPriority(
  _In_ __drv_strictTypeMatch(__drv_typeCond) POOL_TYPE PoolType,
  _In_ SIZE_T NumberOfBytes,
  _In_ ULONG Tag,
  _In_ __drv_strictTypeMatch(__drv_typeExpr) EX_POOL_PRIORITY Priority);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExConvertExclusiveToSharedLite(
  _Inout_ _Requires_lock_held_(*_Curr_) PERESOURCE Resource);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
ExCreateCallback(
  _Outptr_ PCALLBACK_OBJECT *CallbackObject,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ BOOLEAN Create,
  _In_ BOOLEAN AllowMultipleCallbacks);

NTKERNELAPI
VOID
NTAPI
ExDeleteNPagedLookasideList(
  _Inout_ PNPAGED_LOOKASIDE_LIST Lookaside);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExDeletePagedLookasideList(
  _Inout_ PPAGED_LOOKASIDE_LIST Lookaside);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
ExDeleteResourceLite(
  _Inout_ PERESOURCE Resource);

_IRQL_requires_max_(DISPATCH_LEVEL)
_Function_class_(FREE_FUNCTION)
NTKERNELAPI
VOID
NTAPI
ExFreePool(
  _Pre_notnull_ __drv_freesMem(Mem) PVOID P);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExFreePoolWithTag(
  _Pre_notnull_ __drv_freesMem(Mem) PVOID P,
  _In_ ULONG Tag);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
ULONG
NTAPI
ExGetExclusiveWaiterCount(
  _In_ PERESOURCE Resource);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
KPROCESSOR_MODE
NTAPI
ExGetPreviousMode(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
ULONG
NTAPI
ExGetSharedWaiterCount(
  _In_ PERESOURCE Resource);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExInitializeNPagedLookasideList(
  _Out_ PNPAGED_LOOKASIDE_LIST Lookaside,
  _In_opt_ PALLOCATE_FUNCTION Allocate,
  _In_opt_ PFREE_FUNCTION Free,
  _In_ ULONG Flags,
  _In_ SIZE_T Size,
  _In_ ULONG Tag,
  _In_ USHORT Depth);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExInitializePagedLookasideList(
  _Out_ PPAGED_LOOKASIDE_LIST Lookaside,
  _In_opt_ PALLOCATE_FUNCTION Allocate,
  _In_opt_ PFREE_FUNCTION Free,
  _In_ ULONG Flags,
  _In_ SIZE_T Size,
  _In_ ULONG Tag,
  _In_ USHORT Depth);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
ExInitializeResourceLite(
  _Out_ PERESOURCE Resource);

NTKERNELAPI
LARGE_INTEGER
NTAPI
ExInterlockedAddLargeInteger(
  _Inout_ PLARGE_INTEGER Addend,
  _In_ LARGE_INTEGER Increment,
  _Inout_ _Requires_lock_not_held_(*_Curr_) PKSPIN_LOCK Lock);

#if !defined(_M_IX86)
#define ExInterlockedAddLargeStatistic(Addend, Increment) \
    (VOID)InterlockedAdd64(&(Addend)->QuadPart, Increment)
#else
#define ExInterlockedAddLargeStatistic(Addend, Increment) \
    (VOID)_InterlockedAddLargeStatistic((PLONGLONG)&(Addend)->QuadPart, Increment)
#endif

NTKERNELAPI
ULONG
FASTCALL
ExInterlockedAddUlong(
  _Inout_ PULONG Addend,
  _In_ ULONG Increment,
  _Inout_ _Requires_lock_not_held_(*_Curr_) PKSPIN_LOCK Lock);

#if defined(_M_IX86)

NTKERNELAPI
LONGLONG
FASTCALL
ExfInterlockedCompareExchange64(
  _Inout_ _Interlocked_operand_ LONGLONG volatile *Destination,
  _In_ PLONGLONG Exchange,
  _In_ PLONGLONG Comperand);

#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    ExfInterlockedCompareExchange64(Destination, Exchange, Comperand)

#else

#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    InterlockedCompareExchange64(Destination, *(Exchange), *(Comperand))

#endif /* defined(_M_IX86) */

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertHeadList(
  _Inout_ PLIST_ENTRY ListHead,
  _Inout_ __drv_aliasesMem PLIST_ENTRY ListEntry,
  _Inout_ _Requires_lock_not_held_(*_Curr_) PKSPIN_LOCK Lock);

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertTailList(
  _Inout_ PLIST_ENTRY ListHead,
  _Inout_ __drv_aliasesMem PLIST_ENTRY ListEntry,
  _Inout_ _Requires_lock_not_held_(*_Curr_) PKSPIN_LOCK Lock);

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPopEntryList(
  _Inout_ PSINGLE_LIST_ENTRY ListHead,
  _Inout_ _Requires_lock_not_held_(*_Curr_) PKSPIN_LOCK Lock);

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPushEntryList(
  _Inout_ PSINGLE_LIST_ENTRY ListHead,
  _Inout_ __drv_aliasesMem PSINGLE_LIST_ENTRY ListEntry,
  _Inout_ _Requires_lock_not_held_(*_Curr_) PKSPIN_LOCK Lock);

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedRemoveHeadList(
  _Inout_ PLIST_ENTRY ListHead,
  _Inout_ _Requires_lock_not_held_(*_Curr_) PKSPIN_LOCK Lock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
ExIsProcessorFeaturePresent(
  _In_ ULONG ProcessorFeature);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
ExIsResourceAcquiredExclusiveLite(
  _In_ PERESOURCE Resource);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
ULONG
NTAPI
ExIsResourceAcquiredSharedLite(
  _In_ PERESOURCE Resource);

#define ExIsResourceAcquiredLite ExIsResourceAcquiredSharedLite

NTKERNELAPI
VOID
NTAPI
ExLocalTimeToSystemTime(
  _In_ PLARGE_INTEGER LocalTime,
  _Out_ PLARGE_INTEGER SystemTime);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExNotifyCallback(
  _In_ PCALLBACK_OBJECT CallbackObject,
  _In_opt_ PVOID Argument1,
  _In_opt_ PVOID Argument2);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExQueueWorkItem(
  _Inout_ __drv_aliasesMem PWORK_QUEUE_ITEM WorkItem,
  __drv_strictTypeMatch(__drv_typeExpr) _In_ WORK_QUEUE_TYPE QueueType);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseStatus(
  _In_ NTSTATUS Status);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PVOID
NTAPI
ExRegisterCallback(
  _Inout_ PCALLBACK_OBJECT CallbackObject,
  _In_ PCALLBACK_FUNCTION CallbackFunction,
  _In_opt_ PVOID CallbackContext);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
ExReinitializeResourceLite(
  _Inout_ PERESOURCE Resource);

_IRQL_requires_max_(DISPATCH_LEVEL)
_Requires_lock_held_(_Global_critical_region_)
NTKERNELAPI
VOID
NTAPI
ExReleaseResourceForThreadLite(
  _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_)
    PERESOURCE Resource,
  _In_ ERESOURCE_THREAD ResourceThreadId);

_Requires_lock_held_(_Global_critical_region_)
_Requires_lock_held_(*Resource)
_Releases_lock_(*Resource)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
FASTCALL
ExReleaseResourceLite(
  _Inout_ PERESOURCE Resource);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExSetResourceOwnerPointer(
  _Inout_ PERESOURCE Resource,
  _In_ PVOID OwnerPointer);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
ULONG
NTAPI
ExSetTimerResolution(
  _In_ ULONG DesiredTime,
  _In_ BOOLEAN SetResolution);

NTKERNELAPI
VOID
NTAPI
ExSystemTimeToLocalTime(
  _In_ PLARGE_INTEGER SystemTime,
  _Out_ PLARGE_INTEGER LocalTime);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExUnregisterCallback(
  _Inout_ PVOID CbRegistration);

$endif (_WDMDDK_)
$if (_NTDDK_)
NTKERNELAPI
NTSTATUS
NTAPI
ExExtendZone(
  _Inout_ PZONE_HEADER Zone,
  _Inout_ PVOID Segment,
  _In_ ULONG SegmentSize);

NTKERNELAPI
NTSTATUS
NTAPI
ExInitializeZone(
  _Out_ PZONE_HEADER Zone,
  _In_ ULONG BlockSize,
  _Inout_ PVOID InitialSegment,
  _In_ ULONG InitialSegmentSize);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
ExInterlockedExtendZone(
  _Inout_ PZONE_HEADER Zone,
  _Inout_ PVOID Segment,
  _In_ ULONG SegmentSize,
  _Inout_ _Requires_lock_not_held_(*_Curr_) PKSPIN_LOCK Lock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
ExUuidCreate(
  _Out_ UUID *Uuid);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseAccessViolation(VOID);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseDatatypeMisalignment(VOID);

$endif (_NTDDK_)
$if (_NTIFS_)

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
SIZE_T
NTAPI
ExQueryPoolBlockSize(
  _In_ PVOID PoolBlock,
  _Out_ PBOOLEAN QuotaCharged);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
ExAdjustLookasideDepth(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExDisableResourceBoostLite(
  _In_ PERESOURCE Resource);
$endif (_NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

$if (_WDMDDK_ || _NTIFS_)
#if (NTDDI_VERSION >= NTDDI_WINXP)
$endif (_WDMDDK_ || _NTIFS_)
$if (_WDMDDK_)

_Must_inspect_result_
_Post_satisfies_(return == 0 || return == 1)
NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtection(
  _Inout_ PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
VOID
FASTCALL
ExInitializeRundownProtection(
  _Out_ PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
VOID
FASTCALL
ExReInitializeRundownProtection(
  _Inout_ PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtection(
  _Inout_ PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
VOID
FASTCALL
ExRundownCompleted(
  _Out_ PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
BOOLEAN
NTAPI
ExVerifySuite(
  __drv_strictTypeMatch(__drv_typeExpr) _In_ SUITE_TYPE SuiteType);

NTKERNELAPI
VOID
FASTCALL
ExWaitForRundownProtectionRelease(
  _Inout_ PEX_RUNDOWN_REF RunRef);
$endif (_WDMDDK_)
$if (_NTIFS_)

PSLIST_ENTRY
FASTCALL
InterlockedPushListSList(
  _Inout_ PSLIST_HEADER ListHead,
  _Inout_ __drv_aliasesMem PSLIST_ENTRY List,
  _Inout_ PSLIST_ENTRY ListEnd,
  _In_ ULONG Count);
$endif (_NTIFS_)
$if (_WDMDDK_ || _NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */
$endif (_WDMDDK_ || _NTIFS_)

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WINXPSP2)

_Must_inspect_result_
_Post_satisfies_(return == 0 || return == 1)
NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtectionEx(
  _Inout_ PEX_RUNDOWN_REF RunRef,
  _In_ ULONG Count);

NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtectionEx(
  _Inout_ PEX_RUNDOWN_REF RunRef,
  _In_ ULONG Count);

#endif /* (NTDDI_VERSION >= NTDDI_WINXPSP2) */

#if (NTDDI_VERSION >= NTDDI_WS03SP1)

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PEX_RUNDOWN_REF_CACHE_AWARE
NTAPI
ExAllocateCacheAwareRundownProtection(
  __drv_strictTypeMatch(__drv_typeExpr) _In_ POOL_TYPE PoolType,
  _In_ ULONG PoolTag);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
SIZE_T
NTAPI
ExSizeOfRundownProtectionCacheAware(VOID);

_IRQL_requires_max_(APC_LEVEL)
_Acquires_lock_(_Global_critical_region_)
NTKERNELAPI
PVOID
NTAPI
ExEnterCriticalRegionAndAcquireResourceShared(
  _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_shared_lock_(*_Curr_)
    PERESOURCE Resource);

_IRQL_requires_max_(APC_LEVEL)
_Acquires_lock_(_Global_critical_region_)
NTKERNELAPI
PVOID
NTAPI
ExEnterCriticalRegionAndAcquireResourceExclusive(
  _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_exclusive_lock_(*_Curr_)
    PERESOURCE Resource);

_IRQL_requires_max_(APC_LEVEL)
_Acquires_lock_(_Global_critical_region_)
NTKERNELAPI
PVOID
NTAPI
ExEnterCriticalRegionAndAcquireSharedWaitForExclusive(
  _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_)
    PERESOURCE Resource);

_IRQL_requires_max_(DISPATCH_LEVEL)
_Releases_lock_(_Global_critical_region_)
NTKERNELAPI
VOID
FASTCALL
ExReleaseResourceAndLeaveCriticalRegion(
  _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_)
    PERESOURCE Resource);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExInitializeRundownProtectionCacheAware(
  _Out_ PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware,
  _In_ SIZE_T RunRefSize);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExFreeCacheAwareRundownProtection(
  _Inout_ PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware);

_Must_inspect_result_
_Post_satisfies_(return == 0 || return == 1)
NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtectionCacheAware(
  _Inout_ PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware);

NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtectionCacheAware(
  _Inout_ PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware);

_Must_inspect_result_
_Post_satisfies_(return == 0 || return == 1)
NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtectionCacheAwareEx(
  _Inout_ PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware,
  _In_ ULONG Count);

NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtectionCacheAwareEx(
  _Inout_ PEX_RUNDOWN_REF_CACHE_AWARE RunRef,
  _In_ ULONG Count);

NTKERNELAPI
VOID
FASTCALL
ExWaitForRundownProtectionReleaseCacheAware(
  IN OUT PEX_RUNDOWN_REF_CACHE_AWARE RunRef);

NTKERNELAPI
VOID
FASTCALL
ExReInitializeRundownProtectionCacheAware(
  _Inout_ PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware);

NTKERNELAPI
VOID
FASTCALL
ExRundownCompletedCacheAware(
  _Inout_ PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware);

#endif /* (NTDDI_VERSION >= NTDDI_WS03SP1) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
ExInitializeLookasideListEx(
  _Out_ PLOOKASIDE_LIST_EX Lookaside,
  _In_opt_ PALLOCATE_FUNCTION_EX Allocate,
  _In_opt_ PFREE_FUNCTION_EX Free,
  _In_ POOL_TYPE PoolType,
  _In_ ULONG Flags,
  _In_ SIZE_T Size,
  _In_ ULONG Tag,
  _In_ USHORT Depth);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExDeleteLookasideListEx(
  _Inout_ PLOOKASIDE_LIST_EX Lookaside);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExFlushLookasideListEx(
  _Inout_ PLOOKASIDE_LIST_EX Lookaside);

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:__WARNING_MEMORY_NOT_ACQUIRED)
#endif

__drv_allocatesMem(Mem)
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
PVOID
ExAllocateFromLookasideListEx(
  _Inout_ PLOOKASIDE_LIST_EX Lookaside)
{
  PVOID Entry;

  Lookaside->L.TotalAllocates += 1;
#ifdef NONAMELESSUNION
  Entry = InterlockedPopEntrySList(&Lookaside->L.u.ListHead);
  if (Entry == NULL) {
    Lookaside->L.u2.AllocateMisses += 1;
    Entry = (Lookaside->L.u4.AllocateEx)(Lookaside->L.Type,
                                         Lookaside->L.Size,
                                         Lookaside->L.Tag,
                                         Lookaside);
  }
#else /* NONAMELESSUNION */
  Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
  if (Entry == NULL) {
    Lookaside->L.AllocateMisses += 1;
    Entry = (Lookaside->L.AllocateEx)(Lookaside->L.Type,
                                      Lookaside->L.Size,
                                      Lookaside->L.Tag,
                                      Lookaside);
  }
#endif /* NONAMELESSUNION */
  return Entry;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
VOID
ExFreeToLookasideListEx(
  _Inout_ PLOOKASIDE_LIST_EX Lookaside,
  _In_ __drv_freesMem(Entry) PVOID Entry)
{
  Lookaside->L.TotalFrees += 1;
  if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
    Lookaside->L.FreeMisses += 1;
    (Lookaside->L.FreeEx)(Entry, Lookaside);
  } else {
    InterlockedPushEntrySList(&Lookaside->L.ListHead, (PSLIST_ENTRY)Entry);
  }
  return;
}

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExSetResourceOwnerPointerEx(
  _Inout_ PERESOURCE Resource,
  _In_ PVOID OwnerPointer,
  _In_ ULONG Flags);

#define FLAG_OWNER_POINTER_IS_THREAD 0x1

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

__drv_allocatesMem(Mem)
_IRQL_requires_max_(DISPATCH_LEVEL)
_Ret_maybenull_
_Post_writable_byte_size_(Lookaside->L.Size)
static __inline
PVOID
ExAllocateFromNPagedLookasideList(
  _Inout_ PNPAGED_LOOKASIDE_LIST Lookaside)
{
  PVOID Entry;

  Lookaside->L.TotalAllocates++;
#ifdef NONAMELESSUNION
#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)
  Entry = ExInterlockedPopEntrySList(&Lookaside->L.u.ListHead,
                                     &Lookaside->Lock__ObsoleteButDoNotDelete);
#else
  Entry = InterlockedPopEntrySList(&Lookaside->L.u.ListHead);
#endif
  if (Entry == NULL) {
    Lookaside->L.u2.AllocateMisses++;
    Entry = (Lookaside->L.u4.Allocate)(Lookaside->L.Type,
                                       Lookaside->L.Size,
                                       Lookaside->L.Tag);
  }
#else /* NONAMELESSUNION */
#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)
  Entry = ExInterlockedPopEntrySList(&Lookaside->L.ListHead,
                                     &Lookaside->Lock__ObsoleteButDoNotDelete);
#else
  Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
#endif
  if (Entry == NULL) {
    Lookaside->L.AllocateMisses++;
    Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                    Lookaside->L.Size,
                                    Lookaside->L.Tag);
  }
#endif /* NONAMELESSUNION */
  return Entry;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
static __inline
VOID
ExFreeToNPagedLookasideList(
  _Inout_ PNPAGED_LOOKASIDE_LIST Lookaside,
  _In_ __drv_freesMem(Mem) PVOID Entry)
{
  Lookaside->L.TotalFrees++;
#ifdef NONAMELESSUNION
  if (ExQueryDepthSList(&Lookaside->L.u.ListHead) >= Lookaside->L.Depth) {
    Lookaside->L.u3.FreeMisses++;
    (Lookaside->L.u5.Free)(Entry);
  } else {
#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)
      ExInterlockedPushEntrySList(&Lookaside->L.u.ListHead,
                                  (PSLIST_ENTRY)Entry,
                                  &Lookaside->Lock__ObsoleteButDoNotDelete);
#else
      InterlockedPushEntrySList(&Lookaside->L.u.ListHead, (PSLIST_ENTRY)Entry);
#endif
   }
#else /* NONAMELESSUNION */
  if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
    Lookaside->L.FreeMisses++;
    (Lookaside->L.Free)(Entry);
  } else {
#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)
      ExInterlockedPushEntrySList(&Lookaside->L.ListHead,
                                  (PSLIST_ENTRY)Entry,
                                  &Lookaside->Lock__ObsoleteButDoNotDelete);
#else
      InterlockedPushEntrySList(&Lookaside->L.ListHead, (PSLIST_ENTRY)Entry);
#endif
   }
#endif /* NONAMELESSUNION */
}

$endif (_WDMDDK_)

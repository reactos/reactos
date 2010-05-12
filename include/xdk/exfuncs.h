/******************************************************************************
 *                          Executive Functions                               *
 ******************************************************************************/
$if (_NTDDK_)
static __inline PVOID
ExAllocateFromZone(
  IN PZONE_HEADER Zone)
{
  if (Zone->FreeList.Next)
    Zone->FreeList.Next = Zone->FreeList.Next->Next;
  return (PVOID) Zone->FreeList.Next;
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

#ifdef _X86_

typedef enum _INTERLOCKED_RESULT {
  ResultNegative = RESULT_NEGATIVE,
  ResultZero = RESULT_ZERO,
  ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedIncrementLong(
  IN OUT LONG volatile *Addend);

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedDecrementLong(
  IN PLONG  Addend);

NTKERNELAPI
ULONG
FASTCALL
Exfi386InterlockedExchangeUlong(
  IN PULONG  Target,
  IN ULONG  Value);
#endif

$endif

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

NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutex(
  IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutex(
  IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex(
  IN OUT PFAST_MUTEX FastMutex);

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

#if defined(_WIN64)

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || \
    defined(_NTHAL_) || defined(_NTOSP_)
NTKERNELAPI
USHORT
ExQueryDepthSList(IN PSLIST_HEADER ListHead);
#else
FORCEINLINE
USHORT
ExQueryDepthSList(IN PSLIST_HEADER ListHead)
{
  return (USHORT)(ListHead->Alignment & 0xffff);
}
#endif

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedFlushSList(
  PSLIST_HEADER ListHead);

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedPopEntrySList(
  PSLIST_HEADER ListHead);

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedPushEntrySList(
  PSLIST_HEADER ListHead,
  PSLIST_ENTRY ListEntry);

#define ExInterlockedFlushSList(Head) \
    ExpInterlockedFlushSList(Head)
#define ExInterlockedPopEntrySList(Head, Lock) \
    ExpInterlockedPopEntrySList(Head)
#define ExInterlockedPushEntrySList(Head, Entry, Lock) \
    ExpInterlockedPushEntrySList(Head, Entry)

#else /* !defined(_WIN64) */

#define ExQueryDepthSList(listhead) (listhead)->Depth

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedFlushSList(
  IN OUT PSLIST_HEADER ListHead);

#endif /* !defined(_WIN64) */

#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)

NTKERNELAPI
PSINGLE_LIST_ENTRY 
FASTCALL
ExInterlockedPopEntrySList(
  IN PSLIST_HEADER ListHead,
  IN PKSPIN_LOCK Lock);

NTKERNELAPI
PSINGLE_LIST_ENTRY 
FASTCALL
ExInterlockedPushEntrySList(
  IN PSLIST_HEADER ListHead,
  IN PSINGLE_LIST_ENTRY ListEntry,
  IN PKSPIN_LOCK Lock);

NTKERNELAPI
PVOID
NTAPI
ExAllocateFromPagedLookasideList(
  IN OUT PPAGED_LOOKASIDE_LIST Lookaside);

NTKERNELAPI
VOID
NTAPI
ExFreeToPagedLookasideList(
  IN OUT PPAGED_LOOKASIDE_LIST Lookaside,
  IN PVOID Entry);

#else /* !_WIN2K_COMPAT_SLIST_USAGE */

#if !defined(_WIN64)
#define ExInterlockedPopEntrySList(_ListHead, _Lock) \
    InterlockedPopEntrySList(_ListHead)
#define ExInterlockedPushEntrySList(_ListHead, _ListEntry, _Lock) \
    InterlockedPushEntrySList(_ListHead, _ListEntry)
#endif

static __inline
PVOID
ExAllocateFromPagedLookasideList(
  IN OUT PPAGED_LOOKASIDE_LIST Lookaside)
{
  PVOID Entry;

  Lookaside->L.TotalAllocates++;
  Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
  if (Entry == NULL) {
    Lookaside->L.AllocateMisses++;
    Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                    Lookaside->L.Size,
                                    Lookaside->L.Tag);
  }
  return Entry;
}

static __inline
VOID
ExFreeToPagedLookasideList(
  IN OUT PPAGED_LOOKASIDE_LIST Lookaside,
  IN PVOID Entry)
{
  Lookaside->L.TotalFrees++;
  if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
    Lookaside->L.FreeMisses++;
    (Lookaside->L.Free)(Entry);
  } else {
    InterlockedPushEntrySList(&Lookaside->L.ListHead, (PSLIST_ENTRY)Entry);
  }
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
  OUT PFAST_MUTEX FastMutex)
{
  FastMutex->Count = FM_LOCK_BIT;
  FastMutex->Owner = NULL;
  FastMutex->Contention = 0;
  KeInitializeEvent(&FastMutex->Event, SynchronizationEvent, FALSE);
  return;
}
$endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)
$if (_NTDDK_)
NTKERNELAPI
NTSTATUS
NTAPI
ExExtendZone(
  IN OUT PZONE_HEADER Zone,
  IN OUT PVOID Segment,
  IN ULONG SegmentSize);

NTKERNELAPI
NTSTATUS
NTAPI
ExInitializeZone(
  OUT PZONE_HEADER Zone,
  IN ULONG BlockSize,
  IN OUT PVOID InitialSegment,
  IN ULONG InitialSegmentSize);

NTKERNELAPI
NTSTATUS
NTAPI
ExInterlockedExtendZone(
  IN OUT PZONE_HEADER Zone,
  IN OUT PVOID Segment,
  IN ULONG SegmentSize,
  IN OUT PKSPIN_LOCK Lock);

NTKERNELAPI
NTSTATUS
NTAPI
ExUuidCreate(
  OUT UUID *Uuid);

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseAccessViolation(VOID);

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseDatatypeMisalignment(VOID);
$endif

$if (_WDMDDK_)
NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutexUnsafe(
  IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutexUnsafe(
  IN OUT PFAST_MUTEX FastMutex);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireResourceExclusiveLite(
  IN OUT PERESOURCE Resource,
  IN BOOLEAN Wait);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireResourceSharedLite(
  IN OUT PERESOURCE Resource,
  IN BOOLEAN Wait);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireSharedStarveExclusive(
  IN OUT PERESOURCE Resource,
  IN BOOLEAN Wait);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireSharedWaitForExclusive(
  IN OUT PERESOURCE Resource,
  IN BOOLEAN Wait);

NTKERNELAPI
PVOID
NTAPI
ExAllocatePool(
  IN POOL_TYPE PoolType,
  IN SIZE_T NumberOfBytes);

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithQuota(
  IN POOL_TYPE PoolType,
  IN SIZE_T NumberOfBytes);

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithQuotaTag(
  IN POOL_TYPE PoolType,
  IN SIZE_T NumberOfBytes,
  IN ULONG Tag);

#ifndef POOL_TAGGING
#define ExAllocatePoolWithQuotaTag(a,b,c) ExAllocatePoolWithQuota(a,b)
#endif

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTag(
  IN POOL_TYPE PoolType,
  IN SIZE_T NumberOfBytes,
  IN ULONG Tag);

#ifndef POOL_TAGGING
#define ExAllocatePoolWithTag(a,b,c) ExAllocatePool(a,b)
#endif

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTagPriority(
  IN POOL_TYPE PoolType,
  IN SIZE_T NumberOfBytes,
  IN ULONG Tag,
  IN EX_POOL_PRIORITY Priority);

NTKERNELAPI
VOID
NTAPI
ExConvertExclusiveToSharedLite(
  IN OUT PERESOURCE Resource);

NTKERNELAPI
NTSTATUS
NTAPI
ExCreateCallback(
  OUT PCALLBACK_OBJECT *CallbackObject,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  IN BOOLEAN Create,
  IN BOOLEAN AllowMultipleCallbacks);

NTKERNELAPI
VOID
NTAPI
ExDeleteNPagedLookasideList(
  IN OUT PNPAGED_LOOKASIDE_LIST Lookaside);

NTKERNELAPI
VOID
NTAPI
ExDeletePagedLookasideList(
  IN PPAGED_LOOKASIDE_LIST Lookaside);

NTKERNELAPI
NTSTATUS
NTAPI
ExDeleteResourceLite(
  IN OUT PERESOURCE Resource);

NTKERNELAPI
VOID
NTAPI
ExFreePool(
  IN PVOID P);

NTKERNELAPI
VOID
NTAPI
ExFreePoolWithTag(
  IN PVOID P,
  IN ULONG Tag);

NTKERNELAPI
ULONG
NTAPI
ExGetExclusiveWaiterCount(
  IN PERESOURCE Resource);

NTKERNELAPI
KPROCESSOR_MODE
NTAPI
ExGetPreviousMode(VOID);

NTKERNELAPI
ULONG
NTAPI
ExGetSharedWaiterCount(
  IN PERESOURCE Resource);

NTKERNELAPI
VOID
NTAPI
ExInitializeNPagedLookasideList(
  IN PNPAGED_LOOKASIDE_LIST Lookaside,
  IN PALLOCATE_FUNCTION Allocate OPTIONAL,
  IN PFREE_FUNCTION Free OPTIONAL,
  IN ULONG Flags,
  IN SIZE_T Size,
  IN ULONG Tag,
  IN USHORT Depth);

NTKERNELAPI
VOID
NTAPI
ExInitializePagedLookasideList(
  IN PPAGED_LOOKASIDE_LIST Lookaside,
  IN PALLOCATE_FUNCTION Allocate OPTIONAL,
  IN PFREE_FUNCTION Free OPTIONAL,
  IN ULONG Flags,
  IN SIZE_T Size,
  IN ULONG Tag,
  IN USHORT Depth);

NTKERNELAPI
NTSTATUS
NTAPI
ExInitializeResourceLite(
  OUT PERESOURCE Resource);

NTKERNELAPI
LARGE_INTEGER
NTAPI
ExInterlockedAddLargeInteger(
  IN PLARGE_INTEGER Addend,
  IN LARGE_INTEGER Increment,
  IN PKSPIN_LOCK Lock);

#if defined(_WIN64)
#define ExInterlockedAddLargeStatistic(Addend, Increment) \
    (VOID)InterlockedAdd64(&(Addend)->QuadPart, Increment)
#else
#define ExInterlockedAddLargeStatistic(Addend, Increment) \
    _InterlockedAddLargeStatistic((PLONGLONG)&(Addend)->QuadPart, Increment)
#endif

NTKERNELAPI
ULONG
FASTCALL
ExInterlockedAddUlong(
  IN PULONG Addend,
  IN ULONG Increment,
  IN OUT PKSPIN_LOCK Lock);

#if defined(_AMD64_) || defined(_IA64_)

#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    InterlockedCompareExchange64(Destination, *(Exchange), *(Comperand))

#elif defined(_X86_)

NTKERNELAPI
LONGLONG
FASTCALL
ExfInterlockedCompareExchange64(
  IN OUT LONGLONG volatile *Destination,
  IN PLONGLONG Exchange,
  IN PLONGLONG Comperand);

#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    ExfInterlockedCompareExchange64(Destination, Exchange, Comperand)

#else

NTKERNELAPI
LONGLONG
FASTCALL
ExInterlockedCompareExchange64(
  IN OUT LONGLONG volatile *Destination,
  IN PLONGLONG Exchange,
  IN PLONGLONG Comparand,
  IN PKSPIN_LOCK Lock);

#endif /* defined(_AMD64_) || defined(_IA64_) */

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertHeadList(
  IN OUT PLIST_ENTRY ListHead,
  IN OUT PLIST_ENTRY ListEntry,
  IN OUT PKSPIN_LOCK Lock);

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertTailList(
  IN OUT PLIST_ENTRY ListHead,
  IN OUT PLIST_ENTRY ListEntry,
  IN OUT PKSPIN_LOCK Lock);

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPopEntryList(
  IN OUT PSINGLE_LIST_ENTRY ListHead,
  IN OUT PKSPIN_LOCK Lock);

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPushEntryList(
  IN OUT PSINGLE_LIST_ENTRY ListHead,
  IN OUT PSINGLE_LIST_ENTRY ListEntry,
  IN OUT PKSPIN_LOCK Lock);

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedRemoveHeadList(
  IN OUT PLIST_ENTRY ListHead,
  IN OUT PKSPIN_LOCK Lock);

NTKERNELAPI
BOOLEAN
NTAPI
ExIsProcessorFeaturePresent(
  IN ULONG ProcessorFeature);

NTKERNELAPI
BOOLEAN
NTAPI
ExIsResourceAcquiredExclusiveLite(
  IN PERESOURCE Resource);

NTKERNELAPI
ULONG
NTAPI
ExIsResourceAcquiredSharedLite(
  IN PERESOURCE Resource);

#define ExIsResourceAcquiredLite ExIsResourceAcquiredSharedLite

NTKERNELAPI
VOID
NTAPI
ExLocalTimeToSystemTime(
  IN PLARGE_INTEGER LocalTime,
  OUT PLARGE_INTEGER SystemTime);

NTKERNELAPI
VOID
NTAPI
ExNotifyCallback(
  IN PCALLBACK_OBJECT CallbackObject,
  IN PVOID Argument1 OPTIONAL,
  IN PVOID Argument2 OPTIONAL);

NTKERNELAPI
VOID
NTAPI
ExQueueWorkItem(
  IN OUT PWORK_QUEUE_ITEM WorkItem,
  IN WORK_QUEUE_TYPE QueueType);

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseStatus(
  IN NTSTATUS Status);

NTKERNELAPI
PVOID
NTAPI
ExRegisterCallback(
  IN PCALLBACK_OBJECT CallbackObject,
  IN PCALLBACK_FUNCTION CallbackFunction,
  IN PVOID CallbackContext OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
ExReinitializeResourceLite(
  IN OUT PERESOURCE Resource);

NTKERNELAPI
VOID
NTAPI
ExReleaseResourceForThreadLite(
  IN OUT PERESOURCE Resource,
  IN ERESOURCE_THREAD ResourceThreadId);

NTKERNELAPI
VOID
FASTCALL
ExReleaseResourceLite(
  IN OUT PERESOURCE Resource);

NTKERNELAPI
VOID
NTAPI
ExSetResourceOwnerPointer(
  IN OUT PERESOURCE Resource,
  IN PVOID OwnerPointer);

NTKERNELAPI
ULONG
NTAPI
ExSetTimerResolution(
  IN ULONG DesiredTime,
  IN BOOLEAN SetResolution);

NTKERNELAPI
VOID
NTAPI
ExSystemTimeToLocalTime(
  IN PLARGE_INTEGER SystemTime,
  OUT PLARGE_INTEGER LocalTime);

NTKERNELAPI
VOID
NTAPI
ExUnregisterCallback(
  IN OUT PVOID CbRegistration);
$endif

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtection(
  IN OUT PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
VOID
FASTCALL
ExInitializeRundownProtection(
  OUT PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
VOID
FASTCALL
ExReInitializeRundownProtection(
  IN OUT PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtection(
  IN OUT PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
VOID
FASTCALL
ExRundownCompleted(
  OUT PEX_RUNDOWN_REF RunRef);

NTKERNELAPI
BOOLEAN
NTAPI
ExVerifySuite(
  IN SUITE_TYPE SuiteType);

NTKERNELAPI
VOID
FASTCALL
ExWaitForRundownProtectionRelease(
  IN OUT PEX_RUNDOWN_REF RunRef);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WINXPSP2)

NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtectionEx(
  IN OUT PEX_RUNDOWN_REF RunRef,
  IN ULONG Count);

NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtectionEx(
  IN OUT PEX_RUNDOWN_REF RunRef,
  IN ULONG Count);

#endif /* (NTDDI_VERSION >= NTDDI_WINXPSP2) */

#if (NTDDI_VERSION >= NTDDI_WS03SP1)

NTKERNELAPI
PEX_RUNDOWN_REF_CACHE_AWARE
NTAPI
ExAllocateCacheAwareRundownProtection(
  IN POOL_TYPE PoolType,
  IN ULONG PoolTag);

NTKERNELAPI
SIZE_T
NTAPI
ExSizeOfRundownProtectionCacheAware(VOID);

NTKERNELAPI
PVOID
NTAPI
ExEnterCriticalRegionAndAcquireResourceShared(
  IN OUT PERESOURCE Resource);

NTKERNELAPI
PVOID
NTAPI
ExEnterCriticalRegionAndAcquireResourceExclusive(
  IN OUT PERESOURCE Resource);

NTKERNELAPI
PVOID
NTAPI
ExEnterCriticalRegionAndAcquireSharedWaitForExclusive(
  IN OUT PERESOURCE Resource);

NTKERNELAPI
VOID
FASTCALL
ExReleaseResourceAndLeaveCriticalRegion(
  IN OUT PERESOURCE Resource);

NTKERNELAPI
VOID
NTAPI
ExInitializeRundownProtectionCacheAware(
  OUT PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware,
  IN SIZE_T RunRefSize);

NTKERNELAPI
VOID
NTAPI
ExFreeCacheAwareRundownProtection(
  IN OUT PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware);

NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtectionCacheAware(
  IN OUT PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware);

NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtectionCacheAware(
  IN OUT PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware);

NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtectionCacheAwareEx(
  IN OUT PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware,
  IN ULONG Count);

NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtectionCacheAwareEx(
  IN OUT PEX_RUNDOWN_REF_CACHE_AWARE RunRef,
  IN ULONG Count);

NTKERNELAPI
VOID
FASTCALL
ExWaitForRundownProtectionReleaseCacheAware(
  IN OUT PEX_RUNDOWN_REF_CACHE_AWARE RunRef);

NTKERNELAPI
VOID
FASTCALL
ExReInitializeRundownProtectionCacheAware(
  IN OUT PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware);

NTKERNELAPI
VOID
FASTCALL
ExRundownCompletedCacheAware(
  IN OUT PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware);

#endif /* (NTDDI_VERSION >= NTDDI_WS03SP1) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTKERNELAPI
NTSTATUS
NTAPI
ExInitializeLookasideListEx(
  OUT PLOOKASIDE_LIST_EX Lookaside,
  IN PALLOCATE_FUNCTION_EX Allocate OPTIONAL,
  IN PFREE_FUNCTION_EX Free OPTIONAL,
  IN POOL_TYPE PoolType,
  IN ULONG Flags,
  IN SIZE_T Size,
  IN ULONG Tag,
  IN USHORT Depth);

NTKERNELAPI
VOID
NTAPI
ExDeleteLookasideListEx(
  IN OUT PLOOKASIDE_LIST_EX Lookaside);

NTKERNELAPI
VOID
NTAPI
ExFlushLookasideListEx(
  IN OUT PLOOKASIDE_LIST_EX Lookaside);

FORCEINLINE
PVOID
ExAllocateFromLookasideListEx(
  IN OUT PLOOKASIDE_LIST_EX Lookaside)
{
  PVOID Entry;

  Lookaside->L.TotalAllocates += 1;
  Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
  if (Entry == NULL) {
    Lookaside->L.AllocateMisses += 1;
    Entry = (Lookaside->L.AllocateEx)(Lookaside->L.Type,
                                      Lookaside->L.Size,
                                      Lookaside->L.Tag,
                                      Lookaside);
  }
  return Entry;
}

FORCEINLINE
VOID
ExFreeToLookasideListEx(
  IN OUT PLOOKASIDE_LIST_EX Lookaside,
  IN PVOID Entry)
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

NTKERNELAPI
VOID
NTAPI
ExSetResourceOwnerPointerEx(
  IN OUT PERESOURCE Resource,
  IN PVOID OwnerPointer,
  IN ULONG Flags);

#define FLAG_OWNER_POINTER_IS_THREAD 0x1

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

static __inline PVOID
ExAllocateFromNPagedLookasideList(
  IN OUT PNPAGED_LOOKASIDE_LIST Lookaside)
{
  PVOID Entry;

  Lookaside->L.TotalAllocates++;
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
  return Entry;
}

static __inline VOID
ExFreeToNPagedLookasideList(
  IN OUT PNPAGED_LOOKASIDE_LIST Lookaside,
  IN PVOID Entry)
{
  Lookaside->L.TotalFrees++;
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
}

$endif


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

#if defined(_X86_)
#if defined(_NTHAL_)
#define ExAcquireFastMutex ExiAcquireFastMutex
#define ExReleaseFastMutex ExiReleaseFastMutex
#define ExTryToAcquireFastMutex ExiTryToAcquireFastMutex
#endif
#define ExInterlockedAddUlong ExfInterlockedAddUlong
#define ExInterlockedInsertHeadList ExfInterlockedInsertHeadList
#define ExInterlockedInsertTailList ExfInterlockedInsertTailList
#define ExInterlockedRemoveHeadList ExfInterlockedRemoveHeadList
#define ExInterlockedPopEntryList ExfInterlockedPopEntryList
#define ExInterlockedPushEntryList ExfInterlockedPushEntryList
#endif

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

#else // !defined(_WIN64)

#define ExQueryDepthSList(listhead) (listhead)->Depth

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedFlushSList(
    IN PSLIST_HEADER ListHead);

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
#else
#define ExInterlockedPopEntrySList(_ListHead, _Lock) \
    InterlockedPopEntrySList(_ListHead)
#define ExInterlockedPushEntrySList(_ListHead, _ListEntry, _Lock) \
    InterlockedPushEntrySList(_ListHead, _ListEntry)
#endif // _WIN2K_COMPAT_SLIST_USAGE

#endif // !defined(_WIN64)

/* ERESOURCE_THREAD
 * ExGetCurrentResourceThread(
 *     VOID);
 */
#define ExGetCurrentResourceThread() ((ERESOURCE_THREAD)PsGetCurrentThread())

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
    IN PERESOURCE Resource,
    IN BOOLEAN Wait);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireResourceSharedLite(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireSharedStarveExclusive(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait);

NTKERNELAPI
BOOLEAN
NTAPI
ExAcquireSharedWaitForExclusive(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait);

NTKERNELAPI
PVOID
NTAPI
ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes);

#ifdef POOL_TAGGING
#define ExAllocatePool(p,n) ExAllocatePoolWithTag(p,n,' kdD')
#endif /* POOL_TAGGING */

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes);

#ifdef POOL_TAGGING
#define ExAllocatePoolWithQuota(p,n) ExAllocatePoolWithQuotaTag(p,n,' kdD')
#endif /* POOL_TAGGING */

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithQuotaTag(
  IN POOL_TYPE  PoolType,
  IN SIZE_T  NumberOfBytes,
  IN ULONG  Tag);

#ifndef POOL_TAGGING
#define ExAllocatePoolWithQuotaTag(a,b,c) ExAllocatePoolWithQuota(a,b)
#endif /* POOL_TAGGING */

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag);

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
    IN PERESOURCE Resource);

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
    IN PNPAGED_LOOKASIDE_LIST Lookaside);

NTKERNELAPI
VOID
NTAPI
ExDeletePagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside);

NTKERNELAPI
NTSTATUS
NTAPI
ExDeleteResourceLite(
    IN PERESOURCE Resource);

NTKERNELAPI
VOID
NTAPI
ExFreePool(
    IN PVOID P);

#ifdef POOL_TAGGING
#define ExFreePool(P) ExFreePoolWithTag(P, 0)
#endif

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
ExGetPreviousMode(
    VOID);

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
    IN PERESOURCE Resource);

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
    PKSPIN_LOCK Lock);

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
#endif

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertHeadList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock);

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertTailList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock);

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPopEntryList(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PKSPIN_LOCK Lock);

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPushEntryList(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock);

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedRemoveHeadList(
    IN PLIST_ENTRY ListHead,
    IN PKSPIN_LOCK Lock);

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
    IN PVOID Argument1,
    IN PVOID Argument2);

NTKERNELAPI
VOID
NTAPI
ExQueueWorkItem(
    IN PWORK_QUEUE_ITEM WorkItem,
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
    IN PVOID CallbackContext);

NTKERNELAPI
NTSTATUS
NTAPI
ExReinitializeResourceLite(
    IN PERESOURCE Resource);

NTKERNELAPI
VOID
NTAPI
ExReleaseResourceForThreadLite(
    IN PERESOURCE Resource,
    IN ERESOURCE_THREAD ResourceThreadId);

NTKERNELAPI
VOID
FASTCALL
ExReleaseResourceLite(
    IN PERESOURCE Resource);

NTKERNELAPI
VOID
NTAPI
ExSetResourceOwnerPointer(
    IN PERESOURCE Resource,
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
    IN PVOID CbRegistration);


#endif // (NTDDI_VERSION >= NTDDI_WIN2K)

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
    OUT PEX_RUNDOWN_REF RunRef);

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

#endif // (NTDDI_VERSION >= NTDDI_WINXP)

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
#endif // (NTDDI_VERSION >= NTDDI_WINXPSP2)

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
#endif // (NTDDI_VERSION >= NTDDI_WS03SP1)

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
#endif

#if !defined(MIDL_PASS)

static __inline PVOID
ExAllocateFromNPagedLookasideList(
    IN PNPAGED_LOOKASIDE_LIST Lookaside)
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

static __inline PVOID
ExAllocateFromPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside)
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

static __inline VOID
ExFreeToNPagedLookasideList(
    IN PNPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID  Entry)
{
    Lookaside->L.TotalFrees++;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses++;
        (Lookaside->L.Free)(Entry);
    } else {
        InterlockedPushEntrySList(&Lookaside->L.ListHead, (PSLIST_ENTRY)Entry);
    }
}

static __inline VOID
ExFreeToPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID  Entry)
{
    Lookaside->L.TotalFrees++;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses++;
        (Lookaside->L.Free)(Entry);
    } else {
        InterlockedPushEntrySList(&Lookaside->L.ListHead, (PSLIST_ENTRY)Entry);
    }
}

#endif // !defined(MIDL_PASS)

$endif


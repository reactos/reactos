#ifndef __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H

/* GLOBAL VARIABLES *********************************************************/

extern TIME_ZONE_INFORMATION ExpTimeZoneInfo;
extern LARGE_INTEGER ExpTimeZoneBias;
extern ULONG ExpTimeZoneId;
extern ULONG ExpTickCountMultiplier;
extern POBJECT_TYPE ExEventPairObjectType;
extern ULONG NtBuildNumber;
extern ULONG NtMajorVersion;
extern ULONG NtMinorVersion;
extern FAST_MUTEX ExpEnvironmentLock;
extern ERESOURCE ExpFirmwareTableResource;
extern LIST_ENTRY ExpFirmwareTableProviderListHead;
ULONG ExpAnsiCodePageDataOffset, ExpOemCodePageDataOffset;
ULONG ExpUnicodeCaseTableDataOffset;
PVOID ExpNlsSectionPointer;

#define MAX_FAST_REFS           7

#define EX_OBJ_TO_HDR(eob) ((POBJECT_HEADER)((ULONG_PTR)(eob) &                \
  ~(EX_HANDLE_ENTRY_PROTECTFROMCLOSE | EX_HANDLE_ENTRY_INHERITABLE |           \
  EX_HANDLE_ENTRY_AUDITONCLOSE)))
#define EX_HTE_TO_HDR(hte) ((POBJECT_HEADER)((ULONG_PTR)((hte)->Object) &   \
  ~(EX_HANDLE_ENTRY_PROTECTFROMCLOSE | EX_HANDLE_ENTRY_INHERITABLE |           \
  EX_HANDLE_ENTRY_AUDITONCLOSE)))

/* Note: we only use a spinlock on SMP. On UP, we cli/sti intead */
#ifndef CONFIG_SMP
#define ExAcquireResourceLock(l, i) { \
    (void)i; \
    Ke386DisableInterrupts(); \
}
#define ExReleaseResourceLock(l, i) Ke386EnableInterrupts();
#else
#define ExAcquireResourceLock(l, i) KeAcquireSpinLock(l, i);
#define ExReleaseResourceLock(l, i) KeReleaseSpinLock(l, i);
#endif

#define ExAcquireRundownProtection                      _ExAcquireRundownProtection
#define ExReleaseRundownProtection                      _ExReleaseRundownProtection
#define ExInitializeRundownProtection                   _ExInitializeRundownProtection
#define ExWaitForRundownProtectionRelease               _ExWaitForRundownProtectionRelease
#define ExRundownCompleted                              _ExRundownCompleted

/* INITIALIZATION FUNCTIONS *************************************************/

VOID
NTAPI
ExpWin32kInit(VOID);

VOID
NTAPI
ExInit2(VOID);

VOID
NTAPI
ExPhase2Init(
    IN PVOID Context
);

VOID
NTAPI
ExpInitTimeZoneInfo(VOID);

VOID
NTAPI
ExpInitializeWorkerThreads(VOID);

VOID
NTAPI
ExpInitLookasideLists(VOID);

VOID
NTAPI
ExInitializeSystemLookasideList(
    IN PGENERAL_LOOKASIDE List,
    IN POOL_TYPE Type,
    IN ULONG Size,
    IN ULONG Tag,
    IN USHORT MaximumDepth,
    IN PLIST_ENTRY ListHead
);

VOID
NTAPI
ExpInitializeCallbacks(VOID);

VOID
NTAPI
ExpInitUuids(VOID);

VOID
NTAPI
ExpInitializeExecutive(
    IN ULONG Cpu,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
ExpInitializeEventImplementation(VOID);

VOID
NTAPI
ExpInitializeEventImplementation(VOID);

VOID
NTAPI
ExpInitializeEventPairImplementation(VOID);

VOID
NTAPI
ExpInitializeSemaphoreImplementation(VOID);

VOID
NTAPI
ExpInitializeMutantImplementation(VOID);

VOID
NTAPI
ExpInitializeTimerImplementation(VOID);

VOID
NTAPI
ExpInitializeProfileImplementation(VOID);

VOID
NTAPI
ExpResourceInitialization(VOID);

VOID
NTAPI
ExInitPoolLookasidePointers(VOID);

/* Rundown Functions ********************************************************/

VOID
FASTCALL
ExfInitializeRundownProtection(
     OUT PEX_RUNDOWN_REF RunRef
);

VOID
FASTCALL
ExfReInitializeRundownProtection(
     OUT PEX_RUNDOWN_REF RunRef
);

BOOLEAN
FASTCALL
ExfAcquireRundownProtection(
     IN OUT PEX_RUNDOWN_REF RunRef
);

BOOLEAN
FASTCALL
ExfAcquireRundownProtectionEx(
     IN OUT PEX_RUNDOWN_REF RunRef,
     IN ULONG Count
);

VOID
FASTCALL
ExfReleaseRundownProtection(
     IN OUT PEX_RUNDOWN_REF RunRef
);

VOID
FASTCALL
ExfReleaseRundownProtectionEx(
     IN OUT PEX_RUNDOWN_REF RunRef,
     IN ULONG Count
);

VOID
FASTCALL
ExfRundownCompleted(
     OUT PEX_RUNDOWN_REF RunRef
);

VOID
FASTCALL
ExfWaitForRundownProtectionRelease(
     IN OUT PEX_RUNDOWN_REF RunRef
);

/* HANDLE TABLE FUNCTIONS ***************************************************/

#define EX_HANDLE_ENTRY_LOCKED (1 << ((sizeof(PVOID) * 8) - 1))
#define EX_HANDLE_ENTRY_PROTECTFROMCLOSE (1 << 0)
#define EX_HANDLE_ENTRY_INHERITABLE (1 << 1)
#define EX_HANDLE_ENTRY_AUDITONCLOSE (1 << 2)

#define EX_HANDLE_TABLE_CLOSING 0x1

#define EX_HANDLE_ENTRY_FLAGSMASK (EX_HANDLE_ENTRY_LOCKED |                    \
                                   EX_HANDLE_ENTRY_PROTECTFROMCLOSE |          \
                                   EX_HANDLE_ENTRY_INHERITABLE |               \
                                   EX_HANDLE_ENTRY_AUDITONCLOSE)

typedef VOID (NTAPI PEX_SWEEP_HANDLE_CALLBACK)(
    PHANDLE_TABLE_ENTRY HandleTableEntry,
    HANDLE Handle,  
    PVOID Context
);

typedef BOOLEAN (NTAPI PEX_DUPLICATE_HANDLE_CALLBACK)(
    PHANDLE_TABLE HandleTable, 
    PHANDLE_TABLE_ENTRY HandleTableEntry, 
    PVOID Context
);

typedef BOOLEAN (NTAPI PEX_CHANGE_HANDLE_CALLBACK)(
    PHANDLE_TABLE HandleTable, 
    PHANDLE_TABLE_ENTRY HandleTableEntry, 
    PVOID Context
);

VOID
ExpInitializeHandleTables(VOID);

PHANDLE_TABLE
ExCreateHandleTable(IN PEPROCESS QuotaProcess  OPTIONAL);

VOID
ExDestroyHandleTable(
    IN PHANDLE_TABLE HandleTable
);

VOID
ExSweepHandleTable(
    IN PHANDLE_TABLE HandleTable,
    IN PEX_SWEEP_HANDLE_CALLBACK SweepHandleCallback  OPTIONAL,
    IN PVOID Context  OPTIONAL
);

PHANDLE_TABLE
ExDupHandleTable(
    IN PEPROCESS QuotaProcess  OPTIONAL,
    IN PEX_DUPLICATE_HANDLE_CALLBACK DuplicateHandleCallback  OPTIONAL,
    IN PVOID Context  OPTIONAL,
    IN PHANDLE_TABLE SourceHandleTable
);

BOOLEAN
ExLockHandleTableEntry(
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY Entry
);

VOID
ExUnlockHandleTableEntry(
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY Entry
);

HANDLE
ExCreateHandle(
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY Entry
);

BOOLEAN
ExDestroyHandle(
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle
);

VOID
ExDestroyHandleByEntry(
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY Entry,
    IN HANDLE Handle
);

PHANDLE_TABLE_ENTRY
ExMapHandleToPointer(
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle
);

BOOLEAN
ExChangeHandle(
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN PEX_CHANGE_HANDLE_CALLBACK ChangeHandleCallback,
    IN PVOID Context
);

/* PSEH EXCEPTION HANDLING **************************************************/

LONG
NTAPI
ExSystemExceptionFilter(VOID);

static __inline _SEH_FILTER(_SEH_ExSystemExceptionFilter)
{
    return ExSystemExceptionFilter();
}

/* RUNDOWN *******************************************************************/

#ifdef _WIN64
#define ExpChangeRundown(x, y, z) InterlockedCompareExchange64((PLONGLONG)x, y, z)
#define ExpSetRundown(x, y) InterlockedExchange64((PLONGLONG)x, y)
#else
#define ExpChangeRundown(x, y, z) InterlockedCompareExchange((PLONG)x, PtrToLong(y), PtrToLong(z))
#define ExpSetRundown(x, y) InterlockedExchange((PLONG)x, y)
#endif

/*++
 * @name ExfAcquireRundownProtection
 * INTERNAL MACRO
 *
 *     The ExfAcquireRundownProtection routine acquires rundown protection for
 *     the specified descriptor.
 *
 * @param RunRef
 *        Pointer to a rundown reference descriptor.
 *
 * @return TRUE if access to the protected structure was granted, FALSE otherwise.
 *
 * @remarks This is the internal macro for system use only.In case the rundown
 *          was active, then the slow-path will be called through the exported
 *          function.
 *
 *--*/
BOOLEAN
FORCEINLINE
_ExAcquireRundownProtection(IN PEX_RUNDOWN_REF RunRef)
{
    ULONG_PTR Value, NewValue, OldValue;

    /* Get the current value and mask the active bit */
    Value = RunRef->Count &~ EX_RUNDOWN_ACTIVE;

    /* Add a reference */
    NewValue = Value + EX_RUNDOWN_COUNT_INC;

    /* Change the value */
    OldValue = ExpChangeRundown(RunRef, NewValue, Value);
    if (OldValue != Value)
    {
        /* Rundown was active, use long path */
        return ExfAcquireRundownProtection(RunRef);
    }

    /* Success */
    return TRUE;
}

/*++
 * @name ExReleaseRundownProtection
 * INTERNAL MACRO
 *
 *     The ExReleaseRundownProtection routine releases rundown protection for
 *     the specified descriptor.
 *
 * @param RunRef
 *        Pointer to a rundown reference descriptor.
 *
 * @return TRUE if access to the protected structure was granted, FALSE otherwise.
 *
 * @remarks This is the internal macro for system use only.In case the rundown
 *          was active, then the slow-path will be called through the exported
 *          function.
 *
 *--*/
VOID
FORCEINLINE
_ExReleaseRundownProtection(IN PEX_RUNDOWN_REF RunRef)
{
    ULONG_PTR Value, NewValue, OldValue;

    /* Get the current value and mask the active bit */
    Value = RunRef->Count &~ EX_RUNDOWN_ACTIVE;

    /* Remove a reference */
    NewValue = Value - EX_RUNDOWN_COUNT_INC;

    /* Change the value */
    OldValue = ExpChangeRundown(RunRef, NewValue, Value);

    /* Check if the rundown was active */
    if (OldValue != Value)
    {
        /* Rundown was active, use long path */
        ExfReleaseRundownProtection(RunRef);
    }
    else
    {
        /* Sanity check */
        ASSERT((Value >= EX_RUNDOWN_COUNT_INC) || (KeNumberProcessors > 1));
    }
}

/*++
 * @name ExInitializeRundownProtection
 * INTERNAL MACRO
 *
 *     The ExInitializeRundownProtection routine initializes a rundown
 *     protection descriptor.
 *
 * @param RunRef
 *        Pointer to a rundown reference descriptor.
 *
 * @return None.
 *
 * @remarks This is the internal macro for system use only.
 *
 *--*/
VOID
FORCEINLINE
_ExInitializeRundownProtection(IN PEX_RUNDOWN_REF RunRef)
{
    /* Set the count to zero */
    RunRef->Count = 0;
}

/*++
 * @name ExWaitForRundownProtectionRelease
 * INTERNAL MACRO
 *
 *     The ExWaitForRundownProtectionRelease routine waits until the specified
 *     rundown descriptor has been released.
 *
 * @param RunRef
 *        Pointer to a rundown reference descriptor.
 *
 * @return None.
 *
 * @remarks This is the internal macro for system use only. If a wait is actually
 *          necessary, then the slow path is taken through the exported function.
 *
 *--*/
VOID
FORCEINLINE
_ExWaitForRundownProtectionRelease(IN PEX_RUNDOWN_REF RunRef)
{
    ULONG_PTR Value;

    /* Set the active bit */
    Value = ExpChangeRundown(RunRef, EX_RUNDOWN_ACTIVE, 0);
    if ((Value) || (Value != EX_RUNDOWN_ACTIVE))
    {
        /* If the the rundown wasn't already active, then take the long path */
        ExfWaitForRundownProtectionRelease(RunRef);
    }
}

/*++
 * @name ExRundownCompleted
 * INTERNAL MACRO
 *
 *     The ExRundownCompleted routine completes the rundown of the specified
 *     descriptor by setting the active bit.
 *
 * @param RunRef
 *        Pointer to a rundown reference descriptor.
 *
 * @return None.
 *
 * @remarks This is the internal macro for system use only.
 *
 *--*/
VOID
FORCEINLINE
_ExRundownCompleted(IN PEX_RUNDOWN_REF RunRef)
{
    /* Sanity check */
    ASSERT((RunRef->Count & EX_RUNDOWN_ACTIVE) != 0);

    /* Mark the counter as active */
    ExpSetRundown(&RunRef->Count, EX_RUNDOWN_ACTIVE);
}

/* PUSHLOCKS *****************************************************************/

/*++
 * @name ExAcquirePushLockExclusive
 * INTERNAL MACRO
 *
 *     The ExAcquirePushLockExclusive macro exclusively acquires a PushLock.
 *
 * @params PushLock
 *         Pointer to the pushlock which is to be acquired.
 *
 * @return None.
 *
 * @remarks The function attempts the quickest route to acquire the lock, which is
 *          to simply set the lock bit.
 *          However, if the pushlock is already shared, the slower path is taken.
 *
 *          Callers of ExAcquirePushLockShared must be running at IRQL <= APC_LEVEL.
 *          This macro should usually be paired up with KeAcquireCriticalRegion.
 *
 *--*/
VOID
FORCEINLINE
ExAcquirePushLockExclusive(PEX_PUSH_LOCK PushLock)
{
    /* Try acquiring the lock */
    if (InterlockedBitTestAndSet((PLONG)PushLock, EX_PUSH_LOCK_LOCK_V))
    {
        /* Someone changed it, use the slow path */
        DbgPrint("%s - Contention!\n", __FUNCTION__);
        ExfAcquirePushLockExclusive(PushLock);
    }

    /* Sanity check */
    ASSERT(PushLock->Locked);
}

/*++
 * @name ExAcquirePushLockShared
 * INTERNAL MACRO
 *
 *     The ExAcquirePushLockShared macro acquires a shared PushLock.
 *
 * @params PushLock
 *         Pointer to the pushlock which is to be acquired.
 *
 * @return None.
 *
 * @remarks The function attempts the quickest route to acquire the lock, which is
 *          to simply set the lock bit and set the share count to one.
 *          However, if the pushlock is already shared, the slower path is taken.
 *
 *          Callers of ExAcquirePushLockShared must be running at IRQL <= APC_LEVEL.
 *          This macro should usually be paired up with KeAcquireCriticalRegion.
 *
 *--*/
VOID
FORCEINLINE
ExAcquirePushLockShared(PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK NewValue;

    /* Try acquiring the lock */
    NewValue.Value = EX_PUSH_LOCK_LOCK | EX_PUSH_LOCK_SHARE_INC;
    if (InterlockedCompareExchangePointer(PushLock, NewValue.Ptr, 0))
    {
        /* Someone changed it, use the slow path */
        DbgPrint("%s - Contention!\n", __FUNCTION__);
        ExfAcquirePushLockShared(PushLock);
    }

    /* Sanity checks */
    ASSERT(PushLock->Locked);
    ASSERT(PushLock->Waiting || PushLock->Shared > 0);
}

/*++
 * @name ExWaitOnPushLock
 * INTERNAL MACRO
 *
 *     The ExWaitOnPushLock macro acquires and instantly releases a pushlock.
 *
 * @params PushLock
 *         Pointer to a pushlock.
 *
 * @return None.
 *
 * @remarks The function attempts to get any exclusive waiters out of their slow
 *          path by forcing an instant acquire/release operation.
 *
 *          Callers of ExWaitOnPushLock must be running at IRQL <= APC_LEVEL.
 *
 *--*/
VOID
FORCEINLINE
ExWaitOnPushLock(PEX_PUSH_LOCK PushLock)
{
    /* Acquire the lock */
    ExfAcquirePushLockExclusive(PushLock);
    ASSERT(PushLock->Locked);

    /* Release it */
    ExfReleasePushLockExclusive(PushLock);
}

/*++
 * @name ExReleasePushLockShared
 * INTERNAL MACRO
 *
 *     The ExReleasePushLockShared macro releases a previously acquired PushLock.
 *
 * @params PushLock
 *         Pointer to a previously acquired pushlock.
 *
 * @return None.
 *
 * @remarks The function attempts the quickest route to release the lock, which is 
 *          to simply decrease the share count and remove the lock bit.
 *          However, if the pushlock is being waited on then the long path is taken.
 *
 *          Callers of ExReleasePushLockShared must be running at IRQL <= APC_LEVEL.
 *          This macro should usually be paired up with KeLeaveCriticalRegion.
 *
 *--*/
VOID
FORCEINLINE
ExReleasePushLockShared(PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK OldValue;

    /* Sanity checks */
    ASSERT(PushLock->Locked);
    ASSERT(PushLock->Waiting || PushLock->Shared > 0);

    /* Try to clear the pushlock */
    OldValue.Value = EX_PUSH_LOCK_LOCK | EX_PUSH_LOCK_SHARE_INC;
    if (InterlockedCompareExchangePointer(PushLock, 0, OldValue.Ptr) !=
        OldValue.Ptr)
    {
        /* There are still other people waiting on it */
        DbgPrint("%s - Contention!\n", __FUNCTION__);
        ExfReleasePushLockShared(PushLock);
    }
}

/*++
 * @name ExReleasePushLockExclusive
 * INTERNAL MACRO
 *
 *     The ExReleasePushLockExclusive macro releases a previously
 *     exclusively acquired PushLock.
 *
 * @params PushLock
 *         Pointer to a previously acquired pushlock.
 *
 * @return None.
 *
 * @remarks The function attempts the quickest route to release the lock, which is
 *          to simply clear the locked bit.
 *          However, if the pushlock is being waited on, the slow path is taken
 *          in an attempt to wake up the lock.
 *
 *          Callers of ExReleasePushLockExclusive must be running at IRQL <= APC_LEVEL.
 *          This macro should usually be paired up with KeLeaveCriticalRegion.
 *
 *--*/
VOID
FORCEINLINE
ExReleasePushLockExclusive(PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK OldValue;

    /* Sanity checks */
    ASSERT(PushLock->Locked);
    ASSERT(PushLock->Waiting || PushLock->Shared == 0);

    /* Unlock the pushlock */
    OldValue.Value = InterlockedExchangeAddSizeT((PLONG)PushLock, -1);

    /* Sanity checks */
    ASSERT(OldValue.Locked);
    ASSERT(OldValue.Waiting || OldValue.Shared == 0);

    /* Check if anyone is waiting on it and it's not already waking*/
    if ((OldValue.Waiting) && !(OldValue.Waking))
    {
        /* Wake it up */
        DbgPrint("%s - Contention!\n", __FUNCTION__);
        ExfTryToWakePushLock(PushLock);
    }
}

/*++
 * @name ExReleasePushLock
 * INTERNAL MACRO
 *
 *     The ExReleasePushLock macro releases a previously acquired PushLock.
 *
 * @params PushLock
 *         Pointer to a previously acquired pushlock.
 *
 * @return None.
 *
 * @remarks The function attempts the quickest route to release the lock, which is 
 *          to simply clear all the fields and decrease the share count if required.
 *          However, if the pushlock is being waited on then the long path is taken.
 *
 *          Callers of ExReleasePushLock must be running at IRQL <= APC_LEVEL.
 *          This macro should usually be paired up with KeLeaveCriticalRegion.
 *
 *--*/
VOID
FORCEINLINE
ExReleasePushLock(PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK OldValue = *PushLock;
    EX_PUSH_LOCK NewValue;

    /* Sanity checks */
    ASSERT(OldValue.Locked);

    /* Check if the pushlock is shared */
    if (OldValue.Shared > 1)
    {
        /* Decrease the share count */
        NewValue.Value = OldValue.Value &~ EX_PUSH_LOCK_SHARE_INC;
    }
    else
    {
        /* Clear the pushlock entirely */
        NewValue.Value = 0;
    }

    /* Check if nobody is waiting on us and try clearing the lock here */
    if ((OldValue.Waiting) ||
        (InterlockedCompareExchangePointer(PushLock, NewValue.Ptr, OldValue.Ptr) ==
         OldValue.Ptr))
    {
        /* We have waiters, use the long path */
        DbgPrint("%s - Contention!\n", __FUNCTION__);
        ExfReleasePushLock(PushLock);
    }
}

/* OTHER FUNCTIONS **********************************************************/

LONGLONG
FASTCALL
ExfpInterlockedExchange64(
    LONGLONG volatile * Destination,
    PLONGLONG Exchange
);

NTSTATUS
ExpSetTimeZoneInformation(PTIME_ZONE_INFORMATION TimeZoneInformation);

NTSTATUS
NTAPI
ExpAllocateLocallyUniqueId(OUT LUID *LocallyUniqueId);

VOID
NTAPI
ExTimerRundown(VOID);

#define InterlockedDecrementUL(Addend) \
   (ULONG)InterlockedDecrement((PLONG)(Addend))

#define InterlockedIncrementUL(Addend) \
   (ULONG)InterlockedIncrement((PLONG)(Addend))

#define InterlockedExchangeUL(Target, Value) \
   (ULONG)InterlockedExchange((PLONG)(Target), (LONG)(Value))

#define InterlockedExchangeAddUL(Addend, Value) \
   (ULONG)InterlockedExchangeAdd((PLONG)(Addend), (LONG)(Value))

#define InterlockedCompareExchangeUL(Destination, Exchange, Comperand) \
   (ULONG)InterlockedCompareExchange((PLONG)(Destination), (LONG)(Exchange), (LONG)(Comperand))

#define ExfInterlockedCompareExchange64UL(Destination, Exchange, Comperand) \
   (ULONGLONG)ExfInterlockedCompareExchange64((PLONGLONG)(Destination), (PLONGLONG)(Exchange), (PLONGLONG)(Comperand))

#define ExfpInterlockedExchange64UL(Target, Value) \
   (ULONGLONG)ExfpInterlockedExchange64((PLONGLONG)(Target), (PLONGLONG)(Value))

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H */

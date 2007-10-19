#ifndef __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H

/* GLOBAL VARIABLES *********************************************************/

extern TIME_ZONE_INFORMATION ExpTimeZoneInfo;
extern LARGE_INTEGER ExpTimeZoneBias;
extern ULONG ExpTimeZoneId;
extern ULONG ExpTickCountMultiplier;
extern ULONG ExpLastTimeZoneBias;
extern POBJECT_TYPE ExEventPairObjectType;
extern POBJECT_TYPE _ExEventObjectType, _ExSemaphoreObjectType;
extern ULONG NtBuildNumber;
extern ULONG NtMajorVersion;
extern ULONG NtMinorVersion;
extern FAST_MUTEX ExpEnvironmentLock;
extern ERESOURCE ExpFirmwareTableResource;
extern LIST_ENTRY ExpFirmwareTableProviderListHead;
extern BOOLEAN ExpIsWinPEMode;
extern LIST_ENTRY ExpSystemResourcesList;
ULONG ExpAnsiCodePageDataOffset, ExpOemCodePageDataOffset;
ULONG ExpUnicodeCaseTableDataOffset;
PVOID ExpNlsSectionPointer;
extern CHAR NtBuildLab[];
extern ULONG CmNtCSDVersion;
extern ULONG NtGlobalFlag;
extern ULONG ExpInitializationPhase;
extern ULONG ExpAltTimeZoneBias;

typedef struct _EXHANDLE
{
    union
    {
        struct
        {
            ULONG TagBits:2;
            ULONG Index:30;
        };
        HANDLE GenericHandleOverlay;
        ULONG_PTR Value;
    };
} EXHANDLE, *PEXHANDLE;

typedef struct _ETIMER
{
    KTIMER KeTimer;
    KAPC TimerApc;
    KDPC TimerDpc;
    LIST_ENTRY ActiveTimerListEntry;
    KSPIN_LOCK Lock;
    LONG Period;
    BOOLEAN ApcAssociated;
    BOOLEAN WakeTimer;
    LIST_ENTRY WakeTimerListEntry;
} ETIMER, *PETIMER;

typedef struct
{
    PCALLBACK_OBJECT *CallbackObject;
    PWSTR Name;
} SYSTEM_CALLBACKS;

#define MAX_FAST_REFS           7

#define ExAcquireRundownProtection                      _ExAcquireRundownProtection
#define ExReleaseRundownProtection                      _ExReleaseRundownProtection
#define ExInitializeRundownProtection                   _ExInitializeRundownProtection
#define ExWaitForRundownProtectionRelease               _ExWaitForRundownProtectionRelease
#define ExRundownCompleted                              _ExRundownCompleted
#define ExGetPreviousMode                               KeGetPreviousMode


//
// Various bits tagged on the handle or handle table
//
#define EXHANDLE_TABLE_ENTRY_LOCK_BIT    1
#define FREE_HANDLE_MASK                -1

//
// Number of entries in each table level
//
#define LOW_LEVEL_ENTRIES   (PAGE_SIZE / sizeof(HANDLE_TABLE_ENTRY))
#define MID_LEVEL_ENTRIES   (PAGE_SIZE / sizeof(PHANDLE_TABLE_ENTRY))
#define HIGH_LEVEL_ENTRIES  (16777216 / (LOW_LEVEL_ENTRIES * MID_LEVEL_ENTRIES))

//
// Maximum index in each table level before we need another table
//
#define MAX_LOW_INDEX       LOW_LEVEL_ENTRIES
#define MAX_MID_INDEX       (MID_LEVEL_ENTRIES * LOW_LEVEL_ENTRIES)
#define MAX_HIGH_INDEX      (MID_LEVEL_ENTRIES * MID_LEVEL_ENTRIES * LOW_LEVEL_ENTRIES)

//
// Detect GCC
//
#ifdef __GNUC__

#define DEFINE_WAIT_BLOCK(x)                                \
    struct _AlignHack                                       \
    {                                                       \
        UCHAR Hack[15];                                     \
        EX_PUSH_LOCK_WAIT_BLOCK UnalignedBlock;             \
    } WaitBlockBuffer;                                      \
    PEX_PUSH_LOCK_WAIT_BLOCK x = (PEX_PUSH_LOCK_WAIT_BLOCK) \
        ((ULONG_PTR)&WaitBlockBuffer.UnalignedBlock &~ 0xF);

#else

//
// This is only for compatibility; the compiler will optimize the extra
// local variable (the actual pointer) away, so we don't take any perf hit
// by doing this.
//
#define DEFINE_WAIT_BLOCK(x)                                \
    EX_PUSH_LOCK_WAIT_BLOCK WaitBlockBuffer;                \
    PEX_PUSH_LOCK_WAIT_BLOCK x = &WaitBlockBuffer;

#endif

/* INITIALIZATION FUNCTIONS *************************************************/

VOID
NTAPI
ExpWin32kInit(VOID);

VOID
NTAPI
ExInit2(VOID);

VOID
NTAPI
Phase1Initialization(
    IN PVOID Context
);

VOID
NTAPI
ExpInitializePushLocks(VOID);

BOOLEAN
NTAPI
ExRefreshTimeZoneInformation(
    IN PLARGE_INTEGER SystemBootTime
);

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

BOOLEAN
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

/* Callback Functions ********************************************************/

VOID
NTAPI
ExInitializeCallBack(
    IN OUT PEX_CALLBACK Callback
);

PEX_CALLBACK_ROUTINE_BLOCK
NTAPI
ExAllocateCallBack(
    IN PEX_CALLBACK_FUNCTION Function,
    IN PVOID Context
);

VOID
NTAPI
ExFreeCallBack(
    IN PEX_CALLBACK_ROUTINE_BLOCK CallbackRoutineBlock
);

BOOLEAN
NTAPI
ExCompareExchangeCallBack (
    IN OUT PEX_CALLBACK CallBack,
    IN PEX_CALLBACK_ROUTINE_BLOCK NewBlock,
    IN PEX_CALLBACK_ROUTINE_BLOCK OldBlock
);

PEX_CALLBACK_ROUTINE_BLOCK
NTAPI
ExReferenceCallBackBlock(
    IN OUT PEX_CALLBACK CallBack
);

VOID
NTAPI
ExDereferenceCallBackBlock(
    IN OUT PEX_CALLBACK CallBack,
    IN PEX_CALLBACK_ROUTINE_BLOCK CallbackRoutineBlock
);

PEX_CALLBACK_FUNCTION
NTAPI
ExGetCallBackBlockRoutine(
    IN PEX_CALLBACK_ROUTINE_BLOCK CallbackRoutineBlock
);

PVOID
NTAPI
ExGetCallBackBlockContext(
    IN PEX_CALLBACK_ROUTINE_BLOCK CallbackRoutineBlock
);

VOID
NTAPI
ExWaitForCallBacks(
    IN PEX_CALLBACK_ROUTINE_BLOCK CallbackRoutineBlock
);

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

typedef BOOLEAN
(NTAPI *PEX_SWEEP_HANDLE_CALLBACK)(
    PHANDLE_TABLE_ENTRY HandleTableEntry,
    HANDLE Handle,
    PVOID Context
);

typedef BOOLEAN
(NTAPI *PEX_DUPLICATE_HANDLE_CALLBACK)(
    IN PEPROCESS Process,
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry,
    IN PHANDLE_TABLE_ENTRY NewEntry
);

typedef BOOLEAN
(NTAPI *PEX_CHANGE_HANDLE_CALLBACK)(
    PHANDLE_TABLE_ENTRY HandleTableEntry,
    ULONG_PTR Context
);

VOID
NTAPI
ExpInitializeHandleTables(
    VOID
);

PHANDLE_TABLE
NTAPI
ExCreateHandleTable(
    IN PEPROCESS Process OPTIONAL
);

VOID
NTAPI
ExUnlockHandleTableEntry(
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry
);

HANDLE
NTAPI
ExCreateHandle(
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry
);

VOID
NTAPI
ExDestroyHandleTable(
    IN PHANDLE_TABLE HandleTable,
    IN PVOID DestroyHandleProcedure OPTIONAL
);

BOOLEAN
NTAPI
ExDestroyHandle(
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry OPTIONAL
);

PHANDLE_TABLE_ENTRY
NTAPI
ExMapHandleToPointer(
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle
);

PHANDLE_TABLE
NTAPI
ExDupHandleTable(
    IN PEPROCESS Process,
    IN PHANDLE_TABLE HandleTable,
    IN PEX_DUPLICATE_HANDLE_CALLBACK DupHandleProcedure,
    IN ULONG_PTR Mask
);

BOOLEAN
NTAPI
ExChangeHandle(
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN PEX_CHANGE_HANDLE_CALLBACK ChangeRoutine,
    IN ULONG_PTR Context
);

VOID
NTAPI
ExSweepHandleTable(
    IN PHANDLE_TABLE HandleTable,
    IN PEX_SWEEP_HANDLE_CALLBACK EnumHandleProcedure,
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

/* CALLBACKS *****************************************************************/

VOID
FORCEINLINE
ExDoCallBack(IN OUT PEX_CALLBACK Callback,
             IN PVOID Context,
             IN PVOID Argument1,
             IN PVOID Argument2)
{
    PEX_CALLBACK_ROUTINE_BLOCK CallbackRoutineBlock;
    PEX_CALLBACK_FUNCTION Function;

    /* Reference the block */
    CallbackRoutineBlock = ExReferenceCallBackBlock(Callback);
    if (CallbackRoutineBlock)
    {
        /* Get the function */
        Function = ExGetCallBackBlockRoutine(CallbackRoutineBlock);

        /* Do the callback */
        Function(Context, Argument1, Argument2);

        /* Now dereference it */
        ExDereferenceCallBackBlock(Callback, CallbackRoutineBlock);
    }
}

/* RUNDOWN *******************************************************************/

#ifdef _WIN64
#define ExpChangeRundown(x, y, z) InterlockedCompareExchange64((PLONGLONG)x, y, z)
#define ExpSetRundown(x, y) InterlockedExchange64((PLONGLONG)x, y)
#else
#define ExpChangeRundown(x, y, z) InterlockedCompareExchange((PLONG)x, PtrToLong(y), PtrToLong(z))
#define ExpChangePushlock(x, y, z) LongToPtr(InterlockedCompareExchange((PLONG)x, PtrToLong(y), PtrToLong(z)))
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
    ULONG_PTR Value, NewValue;

    /* Get the current value and mask the active bit */
    Value = RunRef->Count &~ EX_RUNDOWN_ACTIVE;

    /* Add a reference */
    NewValue = Value + EX_RUNDOWN_COUNT_INC;

    /* Change the value */
    NewValue = ExpChangeRundown(RunRef, NewValue, Value);
    if (NewValue != Value)
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
    ULONG_PTR Value, NewValue;

    /* Get the current value and mask the active bit */
    Value = RunRef->Count &~ EX_RUNDOWN_ACTIVE;

    /* Remove a reference */
    NewValue = Value - EX_RUNDOWN_COUNT_INC;

    /* Change the value */
    NewValue = ExpChangeRundown(RunRef, NewValue, Value);

    /* Check if the rundown was active */
    if (NewValue != Value)
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
    if ((Value) && (Value != EX_RUNDOWN_ACTIVE))
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

/* FIXME: VERIFY THESE! */

VOID
FASTCALL
ExBlockPushLock(
    IN PEX_PUSH_LOCK PushLock,
    IN PVOID WaitBlock
);

VOID
FASTCALL
ExfUnblockPushLock(
    IN PEX_PUSH_LOCK PushLock,
    IN PVOID CurrentWaitBlock
);

VOID
FASTCALL
ExWaitForUnblockPushLock(
    IN PEX_PUSH_LOCK PushLock,
    IN PVOID WaitBlock
);

/*++
 * @name ExInitializePushLock
 * INTERNAL MACRO
 *
 *     The ExInitializePushLock macro initializes a PushLock.
 *
 * @params PushLock
 *         Pointer to the pushlock which is to be initialized.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
FORCEINLINE
ExInitializePushLock(IN PULONG_PTR PushLock)
{
    /* Set the value to 0 */
    *PushLock = 0;
}

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
        // DbgPrint("%s - Contention!\n", __FUNCTION__);
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
    if (ExpChangePushlock(PushLock, NewValue.Ptr, 0))
    {
        /* Someone changed it, use the slow path */
        // DbgPrint("%s - Contention!\n", __FUNCTION__);
        ExfAcquirePushLockShared(PushLock);
    }

    /* Sanity checks */
    ASSERT(PushLock->Locked);
    ASSERT(PushLock->Waiting || PushLock->Shared > 0);
}

/*++
 * @name ExConvertPushLockSharedToExclusive
 * INTERNAL MACRO
 *
 *     The ExConvertPushLockSharedToExclusive macro converts an exclusive
 *     pushlock to a shared pushlock.
 *
 * @params PushLock
 *         Pointer to the pushlock which is to be converted.
 *
 * @return FALSE if conversion failed, TRUE otherwise.
 *
 * @remarks The function attempts the quickest route to convert the lock, which is
 *          to simply set the lock bit and remove any other bits.
 *
 *--*/
BOOLEAN
FORCEINLINE
ExConvertPushLockSharedToExclusive(IN PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK OldValue;

    /* Set the expected old value */
    OldValue.Value = EX_PUSH_LOCK_LOCK | EX_PUSH_LOCK_SHARE_INC;

    /* Try converting the lock */
    if (ExpChangePushlock(PushLock, EX_PUSH_LOCK_LOCK, OldValue.Value) !=
        OldValue.Ptr)
    {
        /* Conversion failed */
        return FALSE;
    }

    /* Sanity check */
    ASSERT(PushLock->Locked);
    return TRUE;
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
    /* Check if we're locked */
    if (PushLock->Locked)
    {
        /* Acquire the lock */
        ExfAcquirePushLockExclusive(PushLock);
        ASSERT(PushLock->Locked);

        /* Release it */
        ExfReleasePushLockExclusive(PushLock);
    }
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
    if (ExpChangePushlock(PushLock, 0, OldValue.Ptr) != OldValue.Ptr)
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
    OldValue.Value = InterlockedExchangeAddSizeT((PLONG)PushLock,
                                                 -(LONG)EX_PUSH_LOCK_LOCK);

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
        NewValue.Value = OldValue.Value - EX_PUSH_LOCK_SHARE_INC;
    }
    else
    {
        /* Clear the pushlock entirely */
        NewValue.Value = 0;
    }

    /* Check if nobody is waiting on us and try clearing the lock here */
    if ((OldValue.Waiting) ||
        (ExpChangePushlock(PushLock, NewValue.Ptr, OldValue.Ptr) !=
         OldValue.Ptr))
    {
        /* We have waiters, use the long path */
        // DbgPrint("%s - Contention!\n", __FUNCTION__);
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

BOOLEAN
NTAPI
ExAcquireTimeRefreshLock(BOOLEAN Wait);

VOID
NTAPI
ExReleaseTimeRefreshLock(VOID);

VOID
NTAPI
ExUpdateSystemTimeFromCmos(IN BOOLEAN UpdateInterruptTime,
                           IN ULONG MaxSepInSeconds);

NTSTATUS
NTAPI
ExpAllocateLocallyUniqueId(OUT LUID *LocallyUniqueId);

VOID
NTAPI
ExTimerRundown(VOID);

VOID
NTAPI
HeadlessInit(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
XIPInit(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

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

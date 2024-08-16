#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/* GLOBAL VARIABLES *********************************************************/

extern RTL_TIME_ZONE_INFORMATION ExpTimeZoneInfo;
extern LARGE_INTEGER ExpTimeZoneBias;
extern ULONG ExpTimeZoneId;
extern ULONG ExpTickCountMultiplier;
extern ULONG ExpLastTimeZoneBias;
extern POBJECT_TYPE ExEventPairObjectType;
extern POBJECT_TYPE _ExEventObjectType, _ExSemaphoreObjectType;
extern FAST_MUTEX ExpEnvironmentLock;
extern ERESOURCE ExpFirmwareTableResource;
extern ERESOURCE ExpTimeRefreshLock;
extern LIST_ENTRY ExpFirmwareTableProviderListHead;
extern BOOLEAN ExpIsWinPEMode;
extern ULONG ExpResourceTimeoutCount;
extern LIST_ENTRY ExpSystemResourcesList;
extern ULONG ExpAnsiCodePageDataOffset, ExpOemCodePageDataOffset;
extern ULONG ExpUnicodeCaseTableDataOffset;
extern PVOID ExpNlsSectionPointer;
extern ULONG NtGlobalFlag;
extern UNICODE_STRING NtSystemRoot;
extern ULONG ExpInitializationPhase;
extern ULONG ExpAltTimeZoneBias;
extern LIST_ENTRY ExSystemLookasideListHead;
extern PCALLBACK_OBJECT PowerStateCallback;
extern LIST_ENTRY ExPoolLookasideListHead;
extern LIST_ENTRY ExpNonPagedLookasideListHead;
extern LIST_ENTRY ExpPagedLookasideListHead;
extern KSPIN_LOCK ExpNonPagedLookasideListLock;
extern KSPIN_LOCK ExpPagedLookasideListLock;
extern ULONG ExCriticalWorkerThreads;
extern ULONG ExDelayedWorkerThreads;

extern PVOID ExpDefaultErrorPort;
extern PEPROCESS ExpDefaultErrorPortProcess;

/*
 * NT/Cm Version Info variables
 */
extern ULONG NtMajorVersion;
extern ULONG NtMinorVersion;
extern ULONG NtBuildNumber;
extern ULONG CmNtSpBuildNumber;
extern ULONG CmNtCSDVersion;
extern ULONG CmNtCSDReleaseType;
extern UNICODE_STRING CmVersionString;
extern UNICODE_STRING CmCSDVersionString;
extern CHAR NtBuildLab[];

/*
 * WinDBG Debugger Worker State Machine data (see dbgctrl.c)
 */
typedef enum _WINKD_WORKER_STATE
{
    WinKdWorkerReady = 0,
    WinKdWorkerStart,
    WinKdWorkerInitialized
} WINKD_WORKER_STATE;

extern WORK_QUEUE_ITEM ExpDebuggerWorkItem;
extern WINKD_WORKER_STATE ExpDebuggerWork;
extern PEPROCESS ExpDebuggerProcessAttach;
extern PEPROCESS ExpDebuggerProcessKill;
extern ULONG_PTR ExpDebuggerPageIn;

VOID
NTAPI
ExpDebuggerWorker(
    _In_ PVOID Context);

#ifdef _WIN64
#define HANDLE_LOW_BITS     (PAGE_SHIFT - 4)
#define HANDLE_HIGH_BITS    (PAGE_SHIFT - 3)
#else
#define HANDLE_LOW_BITS     (PAGE_SHIFT - 3)
#define HANDLE_HIGH_BITS    (PAGE_SHIFT - 2)
#endif
#define HANDLE_TAG_BITS     2
#define HANDLE_INDEX_BITS   (HANDLE_LOW_BITS + 2 * HANDLE_HIGH_BITS)
#define KERNEL_FLAG_BITS    (sizeof(ULONG_PTR) * 8 - HANDLE_INDEX_BITS - HANDLE_TAG_BITS)

typedef union _EXHANDLE
{
    struct
    {
        ULONG_PTR TagBits:      HANDLE_TAG_BITS;
        ULONG_PTR Index:        HANDLE_INDEX_BITS;
        ULONG_PTR KernelFlag:   KERNEL_FLAG_BITS;
    };
    struct
    {
        ULONG_PTR TagBits2:     HANDLE_TAG_BITS;
        ULONG_PTR LowIndex:     HANDLE_LOW_BITS;
        ULONG_PTR MidIndex:     HANDLE_HIGH_BITS;
        ULONG_PTR HighIndex:    HANDLE_HIGH_BITS;
        ULONG_PTR KernelFlag2:  KERNEL_FLAG_BITS;
    };
    HANDLE GenericHandleOverlay;
    ULONG_PTR Value;
    ULONG AsULONG;
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

typedef struct _HARDERROR_USER_PARAMETERS
{
    ULONG_PTR Parameters[MAXIMUM_HARDERROR_PARAMETERS];
    UNICODE_STRING Strings[MAXIMUM_HARDERROR_PARAMETERS];
    WCHAR Buffer[ANYSIZE_ARRAY];
} HARDERROR_USER_PARAMETERS, *PHARDERROR_USER_PARAMETERS;

#define MAX_FAST_REFS           7

#define ExAcquireRundownProtection          _ExAcquireRundownProtection
#define ExReleaseRundownProtection          _ExReleaseRundownProtection
#define ExInitializeRundownProtection       _ExInitializeRundownProtection
#define ExWaitForRundownProtectionRelease   _ExWaitForRundownProtectionRelease
#define ExRundownCompleted                  _ExRundownCompleted
#define ExGetPreviousMode                   KeGetPreviousMode


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

#define ExpChangeRundown(x, y, z)   (ULONG_PTR)InterlockedCompareExchangePointer(&(x)->Ptr, (PVOID)(y), (PVOID)(z))
#define ExpChangePushlock(x, y, z)  InterlockedCompareExchangePointer((PVOID*)(x), (PVOID)(y), (PVOID)(z))
#define ExpSetRundown(x, y)         InterlockedExchangePointer(&(x)->Ptr, (PVOID)(y))

NTSTATUS
NTAPI
ExGetPoolTagInfo(
    IN PSYSTEM_POOLTAG_INFORMATION SystemInformation,
    IN ULONG SystemInformationLength,
    IN OUT PULONG ReturnLength OPTIONAL
);

typedef struct _UUID_CACHED_VALUES_STRUCT
{
    ULONGLONG Time;
    LONG AllocatedCount;
    union
    {
        struct
        {
            UCHAR ClockSeqHiAndReserved;
            UCHAR ClockSeqLow;
            UCHAR NodeId[6 /*SEED_BUFFER_SIZE*/];
        };
        UCHAR GuidInit[8]; /* Match GUID.Data4 */
    };
} UUID_CACHED_VALUES_STRUCT, *PUUID_CACHED_VALUES_STRUCT;

C_ASSERT(RTL_FIELD_SIZE(UUID_CACHED_VALUES_STRUCT, GuidInit) == RTL_FIELD_SIZE(UUID, Data4));

/* INITIALIZATION FUNCTIONS *************************************************/

CODE_SEG("INIT")
BOOLEAN
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

CODE_SEG("INIT")
VOID
NTAPI
ExpInitializePushLocks(VOID);

BOOLEAN
NTAPI
ExRefreshTimeZoneInformation(
    IN PLARGE_INTEGER SystemBootTime
);

CODE_SEG("INIT")
VOID
NTAPI
ExpInitializeWorkerThreads(VOID);

VOID
NTAPI
ExSwapinWorkerThreads(IN BOOLEAN AllowSwap);

CODE_SEG("INIT")
VOID
NTAPI
ExpInitLookasideLists(VOID);

CODE_SEG("INIT")
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

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitializeCallbacks(VOID);

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpUuidInitialization(VOID);

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExLuidInitialization(VOID);

CODE_SEG("INIT")
VOID
NTAPI
ExpInitializeExecutive(
    IN ULONG Cpu,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
NTAPI
ExShutdownSystem(VOID);

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitializeEventImplementation(VOID);

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitializeKeyedEventImplementation(VOID);

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitializeEventPairImplementation(VOID);

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitializeSemaphoreImplementation(VOID);

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitializeMutantImplementation(VOID);

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitializeTimerImplementation(VOID);

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitializeProfileImplementation(VOID);

CODE_SEG("INIT")
VOID
NTAPI
ExpResourceInitialization(VOID);

CODE_SEG("INIT")
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

CODE_SEG("INIT")
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

PHANDLE_TABLE_ENTRY
NTAPI
ExpLookupHandleTableEntry(
    IN PHANDLE_TABLE HandleTable,
    IN EXHANDLE Handle
);

BOOLEAN
NTAPI
ExpLockHandleTableEntry(
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry
);

/* PSEH EXCEPTION HANDLING **************************************************/

LONG
NTAPI
ExSystemExceptionFilter(VOID);

/* CALLBACKS *****************************************************************/

FORCEINLINE
VOID
ExDoCallBack(IN OUT PEX_CALLBACK Callback,
             IN PVOID Context,
             IN PVOID Argument1,
             IN PVOID Argument2)
{
    PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock;
    PEX_CALLBACK_FUNCTION Function;

    /* Reference the block */
    CallbackBlock = ExReferenceCallBackBlock(Callback);
    if (CallbackBlock)
    {
        /* Get the function */
        Function = ExGetCallBackBlockRoutine(CallbackBlock);

        /* Do the callback */
        Function(Context, Argument1, Argument2);

        /* Now dereference it */
        ExDereferenceCallBackBlock(Callback, CallbackBlock);
    }
}

/* FAST REFS ******************************************************************/

FORCEINLINE
PVOID
ExGetObjectFastReference(IN EX_FAST_REF FastRef)
{
    /* Return the unbiased pointer */
    return (PVOID)(FastRef.Value & ~MAX_FAST_REFS);
}

FORCEINLINE
ULONG
ExGetCountFastReference(IN EX_FAST_REF FastRef)
{
    /* Return the reference count */
    return (ULONG)FastRef.RefCnt;
}

FORCEINLINE
VOID
ExInitializeFastReference(OUT PEX_FAST_REF FastRef,
                          IN OPTIONAL PVOID Object)
{
    /* Sanity check */
    ASSERT((((ULONG_PTR)Object) & MAX_FAST_REFS) == 0);

    /* Check if an object is being set */
    if (!Object)
    {
        /* Clear the field */
        FastRef->Object = NULL;
    }
    else
    {
        /* Otherwise, we assume the object was referenced and is ready */
        FastRef->Value = (ULONG_PTR)Object | MAX_FAST_REFS;
    }
}

FORCEINLINE
EX_FAST_REF
ExAcquireFastReference(IN OUT PEX_FAST_REF FastRef)
{
    EX_FAST_REF OldValue, NewValue;

    /* Start reference loop */
    for (;;)
    {
        /* Get the current reference count */
        OldValue = *FastRef;
        if (OldValue.RefCnt)
        {
            /* Increase the reference count */
            NewValue.Value = OldValue.Value - 1;
            NewValue.Object = ExpChangePushlock(&FastRef->Object,
                                                NewValue.Object,
                                                OldValue.Object);
            if (NewValue.Object != OldValue.Object) continue;
        }

        /* We are done */
        break;
    }

    /* Return the old value */
    return OldValue;
}

FORCEINLINE
BOOLEAN
ExInsertFastReference(IN OUT PEX_FAST_REF FastRef,
                      IN PVOID Object)
{
    EX_FAST_REF OldValue, NewValue;

    /* Sanity checks */
    ASSERT(!(((ULONG_PTR)Object) & MAX_FAST_REFS));

    /* Start update loop */
    for (;;)
    {
        /* Get the current reference count */
        OldValue = *FastRef;

        /* Check if the current count is too high or if the pointer changed */
        if (((OldValue.RefCnt + MAX_FAST_REFS) > MAX_FAST_REFS) ||
            ((OldValue.Value &~ MAX_FAST_REFS) != (ULONG_PTR)Object))
        {
            /* Fail */
            return FALSE;
        }

        /* Update the reference count */
        NewValue.Value = OldValue.Value + MAX_FAST_REFS;
        NewValue.Object = ExpChangePushlock(&FastRef->Object,
                                            NewValue.Object,
                                            OldValue.Object);
        if (NewValue.Object != OldValue.Object) continue;

        /* We are done */
        break;
    }

    /* Return success */
    return TRUE;
}

FORCEINLINE
BOOLEAN
ExReleaseFastReference(IN PEX_FAST_REF FastRef,
                       IN PVOID Object)
{
    EX_FAST_REF OldValue, NewValue;

    /* Sanity checks */
    ASSERT(Object != NULL);
    ASSERT(!(((ULONG_PTR)Object) & MAX_FAST_REFS));

    /* Start reference loop */
    for (;;)
    {
        /* Get the current reference count */
        OldValue = *FastRef;

        /* Check if we're full if if the pointer changed */
        if ((OldValue.Value ^ (ULONG_PTR)Object) >= MAX_FAST_REFS) return FALSE;

        /* Decrease the reference count */
        NewValue.Value = OldValue.Value + 1;
        NewValue.Object = ExpChangePushlock(&FastRef->Object,
                                            NewValue.Object,
                                            OldValue.Object);
        if (NewValue.Object != OldValue.Object) continue;

        /* We are done */
        break;
    }

    /* Return success */
    return TRUE;
}

FORCEINLINE
EX_FAST_REF
ExSwapFastReference(IN PEX_FAST_REF FastRef,
                    IN PVOID Object)
{
    EX_FAST_REF NewValue, OldValue;

    /* Sanity check */
    ASSERT((((ULONG_PTR)Object) & MAX_FAST_REFS) == 0);

    /* Check if an object is being set */
    if (!Object)
    {
        /* Clear the field */
        NewValue.Object = NULL;
    }
    else
    {
        /* Otherwise, we assume the object was referenced and is ready */
        NewValue.Value = (ULONG_PTR)Object | MAX_FAST_REFS;
    }

    /* Update the object */
    OldValue.Object = InterlockedExchangePointer(&FastRef->Object, NewValue.Object);
    return OldValue;
}

FORCEINLINE
EX_FAST_REF
ExCompareSwapFastReference(IN PEX_FAST_REF FastRef,
                           IN PVOID Object,
                           IN PVOID OldObject)
{
    EX_FAST_REF OldValue, NewValue;

    /* Sanity check and start swap loop */
    ASSERT(!(((ULONG_PTR)Object) & MAX_FAST_REFS));
    for (;;)
    {
        /* Get the current value */
        OldValue = *FastRef;

        /* Make sure there's enough references to swap */
        if (!((OldValue.Value ^ (ULONG_PTR)OldObject) <= MAX_FAST_REFS)) break;

        /* Check if we have an object to swap */
        if (Object)
        {
            /* Set up the value with maximum fast references */
            NewValue.Value = (ULONG_PTR)Object | MAX_FAST_REFS;
        }
        else
        {
            /* Write the object address itself (which is empty) */
            NewValue.Value = (ULONG_PTR)Object;
        }

        /* Do the actual compare exchange */
        NewValue.Object = ExpChangePushlock(&FastRef->Object,
                                            NewValue.Object,
                                            OldValue.Object);
        if (NewValue.Object != OldValue.Object) continue;

        /* All done */
        break;
    }

    /* Return the old value */
    return OldValue;
}

/* RUNDOWN *******************************************************************/

FORCEINLINE
PEX_RUNDOWN_REF
ExGetRunRefForGivenProcessor(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware,
                             IN ULONG ProcNumber)
{
    return (PEX_RUNDOWN_REF)((ULONG_PTR)RunRefCacheAware->RunRefs +
                             RunRefCacheAware->RunRefSize *
                             (ProcNumber % RunRefCacheAware->Number));
}

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
FORCEINLINE
BOOLEAN
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
FORCEINLINE
VOID
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
FORCEINLINE
VOID
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
FORCEINLINE
VOID
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
FORCEINLINE
VOID
_ExRundownCompleted(IN PEX_RUNDOWN_REF RunRef)
{
    /* Sanity check */
    ASSERT((RunRef->Count & EX_RUNDOWN_ACTIVE) != 0);

    /* Mark the counter as active */
    ExpSetRundown(RunRef, EX_RUNDOWN_ACTIVE);
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
 * @name _ExInitializePushLock
 * INTERNAL MACRO
 *
 *     The _ExInitializePushLock macro initializes a PushLock.
 *
 * @params PushLock
 *         Pointer to the pushlock which is to be initialized.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
FORCEINLINE
VOID
_ExInitializePushLock(OUT PEX_PUSH_LOCK PushLock)
{
    /* Set the value to 0 */
    PushLock->Ptr = 0;
}
#define ExInitializePushLock _ExInitializePushLock

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
FORCEINLINE
VOID
ExAcquirePushLockExclusive(PEX_PUSH_LOCK PushLock)
{
    /* Try acquiring the lock */
    if (InterlockedBitTestAndSet((PLONG)PushLock, EX_PUSH_LOCK_LOCK_V))
    {
        /* Someone changed it, use the slow path */
        ExfAcquirePushLockExclusive(PushLock);
    }

    /* Sanity check */
    ASSERT(PushLock->Locked);
}

/*++
* @name ExTryToAcquirePushLockExclusive
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
FORCEINLINE
BOOLEAN
ExTryToAcquirePushLockExclusive(PEX_PUSH_LOCK PushLock)
{
    /* Try acquiring the lock */
    if (InterlockedBitTestAndSet((PLONG)PushLock, EX_PUSH_LOCK_LOCK_V))
    {
        /* Can't acquire */
        return FALSE;
    }

    /* Got acquired */
    ASSERT (PushLock->Locked);
    return TRUE;
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
FORCEINLINE
VOID
ExAcquirePushLockShared(PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK NewValue;

    /* Try acquiring the lock */
    NewValue.Value = EX_PUSH_LOCK_LOCK | EX_PUSH_LOCK_SHARE_INC;
    if (ExpChangePushlock(PushLock, NewValue.Ptr, 0))
    {
        /* Someone changed it, use the slow path */
        ExfAcquirePushLockShared(PushLock);
    }

    /* Sanity checks */
    ASSERT(PushLock->Locked);
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
FORCEINLINE
BOOLEAN
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
FORCEINLINE
VOID
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
FORCEINLINE
VOID
ExReleasePushLockShared(PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK OldValue;

    /* Sanity checks */
    ASSERT(PushLock->Locked);

    /* Try to clear the pushlock */
    OldValue.Value = EX_PUSH_LOCK_LOCK | EX_PUSH_LOCK_SHARE_INC;
    if (ExpChangePushlock(PushLock, 0, OldValue.Ptr) != OldValue.Ptr)
    {
        /* There are still other people waiting on it */
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
FORCEINLINE
VOID
ExReleasePushLockExclusive(PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK OldValue;

    /* Sanity checks */
    ASSERT(PushLock->Locked);

    /* Unlock the pushlock */
    OldValue.Value = InterlockedExchangeAddSizeT((PSIZE_T)PushLock,
                                                 -(SSIZE_T)EX_PUSH_LOCK_LOCK);

    /* Sanity checks */
    ASSERT(OldValue.Locked);
    ASSERT(OldValue.Waiting || OldValue.Shared == 0);

    /* Check if anyone is waiting on it and it's not already waking*/
    if ((OldValue.Waiting) && !(OldValue.Waking))
    {
        /* Wake it up */
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
FORCEINLINE
VOID
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
        ExfReleasePushLock(PushLock);
    }
}

/* FAST MUTEX INLINES *********************************************************/

FORCEINLINE
VOID
_ExAcquireFastMutexUnsafe(IN PFAST_MUTEX FastMutex)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* Sanity check */
    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (Thread->CombinedApcDisable != 0) ||
           (Thread->Teb == NULL) ||
           (Thread->Teb >= (PTEB)MM_SYSTEM_RANGE_START));
    ASSERT(FastMutex->Owner != Thread);

    /* Decrease the count */
    if (InterlockedDecrement(&FastMutex->Count))
    {
        /* Someone is still holding it, use slow path */
        KiAcquireFastMutex(FastMutex);
    }

    /* Set the owner */
    FastMutex->Owner = Thread;
}

FORCEINLINE
VOID
_ExReleaseFastMutexUnsafe(IN OUT PFAST_MUTEX FastMutex)
{
    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (KeGetCurrentThread()->CombinedApcDisable != 0) ||
           (KeGetCurrentThread()->Teb == NULL) ||
           (KeGetCurrentThread()->Teb >= (PTEB)MM_SYSTEM_RANGE_START));
    ASSERT(FastMutex->Owner == KeGetCurrentThread());

    /* Erase the owner */
    FastMutex->Owner = NULL;

    /* Increase the count */
    if (InterlockedIncrement(&FastMutex->Count) <= 0)
    {
        /* Someone was waiting for it, signal the waiter */
        KeSetEventBoostPriority(&FastMutex->Event, NULL);
    }
}

FORCEINLINE
VOID
_ExAcquireFastMutex(IN PFAST_MUTEX FastMutex)
{
    KIRQL OldIrql;
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    /* Raise IRQL to APC */
    KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Decrease the count */
    if (InterlockedDecrement(&FastMutex->Count))
    {
        /* Someone is still holding it, use slow path */
        KiAcquireFastMutex(FastMutex);
    }

    /* Set the owner and IRQL */
    FastMutex->Owner = KeGetCurrentThread();
    FastMutex->OldIrql = OldIrql;
}

FORCEINLINE
VOID
_ExReleaseFastMutex(IN OUT PFAST_MUTEX FastMutex)
{
    KIRQL OldIrql;
    ASSERT(KeGetCurrentIrql() == APC_LEVEL);

    /* Erase the owner */
    FastMutex->Owner = NULL;
    OldIrql = (KIRQL)FastMutex->OldIrql;

    /* Increase the count */
    if (InterlockedIncrement(&FastMutex->Count) <= 0)
    {
        /* Someone was waiting for it, signal the waiter */
        KeSetEventBoostPriority(&FastMutex->Event, NULL);
    }

    /* Lower IRQL back */
    KeLowerIrql(OldIrql);
}

FORCEINLINE
BOOLEAN
_ExTryToAcquireFastMutex(IN OUT PFAST_MUTEX FastMutex)
{
    KIRQL OldIrql;
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    /* Raise to APC_LEVEL */
    KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Check if we can quickly acquire it */
    if (InterlockedCompareExchange(&FastMutex->Count, 0, 1) == 1)
    {
        /* We have, set us as owners */
        FastMutex->Owner = KeGetCurrentThread();
        FastMutex->OldIrql = OldIrql;
        return TRUE;
    }
    else
    {
        /* Acquire attempt failed */
        KeLowerIrql(OldIrql);
        YieldProcessor();
        return FALSE;
    }
}

FORCEINLINE
VOID
_ExEnterCriticalRegionAndAcquireFastMutexUnsafe(IN OUT PFAST_MUTEX FastMutex)
{
    /* Enter the Critical Region */
    KeEnterCriticalRegion();

    /* Acquire the mutex unsafely */
    _ExAcquireFastMutexUnsafe(FastMutex);
}

FORCEINLINE
VOID
_ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(IN OUT PFAST_MUTEX FastMutex)
{
    /* Release the mutex unsafely */
    _ExReleaseFastMutexUnsafe(FastMutex);

    /* Leave the critical region */
    KeLeaveCriticalRegion();
}

/* OTHER FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
ExTryToAcquireResourceExclusiveLite(
    IN PERESOURCE Resource
);

NTSTATUS
ExpSetTimeZoneInformation(
    IN PRTL_TIME_ZONE_INFORMATION TimeZoneInformation
);

BOOLEAN
NTAPI
ExAcquireTimeRefreshLock(
    IN BOOLEAN Wait
);

VOID
NTAPI
ExReleaseTimeRefreshLock(
    VOID
);

VOID
NTAPI
ExUpdateSystemTimeFromCmos(
    IN BOOLEAN UpdateInterruptTime,
    IN ULONG MaxSepInSeconds
);

VOID
NTAPI
ExAllocateLocallyUniqueId(
    OUT LUID *LocallyUniqueId
);

VOID
NTAPI
ExTimerRundown(
    VOID
);

VOID
NTAPI
ExUnlockUserBuffer(PMDL Mdl);

NTSTATUS
NTAPI
ExLockUserBuffer(
    PVOID BaseAddress,
    ULONG Length,
    KPROCESSOR_MODE AccessMode,
    LOCK_OPERATION Operation,
    PVOID *MappedSystemVa,
    PMDL *OutMdl);

CODE_SEG("INIT")
VOID
NTAPI
HeadlessInit(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

CODE_SEG("INIT")
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

#define InterlockedCompareExchangeSizeT(Destination, Exchange, Comperand) \
   (SIZE_T)InterlockedCompareExchangePointer((PVOID*)(Destination), (PVOID)(SIZE_T)(Exchange), (PVOID)(SIZE_T)(Comperand))

#ifdef _WIN64
#define InterlockedExchangeSizeT(Target, Value) \
    (SIZE_T)InterlockedExchange64((PLONG64)(Target), (LONG64)(Value))
#else
#define InterlockedExchangeSizeT(Target, Value) \
    (SIZE_T)InterlockedExchange((PLONG)(Target), (LONG)(Value))
#endif

#define ExfInterlockedCompareExchange64UL(Destination, Exchange, Comperand) \
   (ULONGLONG)ExfInterlockedCompareExchange64((PLONGLONG)(Destination), (PLONGLONG)(Exchange), (PLONGLONG)(Comperand))

#ifdef __cplusplus
} // extern "C"
#endif


CROSSNT_DECL(
HANDLE,__stdcall,
PsGetCurrentProcessId,(),())

CROSSNT_DECL(
HANDLE,__stdcall,
PsGetCurrentThreadId,(),())

CROSSNT_DECL(
BOOLEAN,
__fastcall,
KeTestSpinLock,(
    IN PKSPIN_LOCK SpinLock
    ),
    (
    SpinLock
    ))
#if 0
CROSSNT_DECL(
LONG,
__fastcall,
InterlockedIncrement,(
    IN OUT PLONG Addend
    ),
    (
    IN OUT PLONG Addend
    ))

CROSSNT_DECL(
LONG,
__fastcall,
InterlockedDecrement,(
    IN OUT PLONG Addend
    ),
    (
    IN OUT PLONG Addend
    ))

CROSSNT_DECL(
LONG,
__fastcall,
InterlockedExchangeAdd,(
    IN OUT PLONG Addend,
    IN LONG Increment
    ),
    (
    IN OUT PLONG Addend,
    IN LONG Increment
    ))

CROSSNT_DECL(
PVOID,
__fastcall,
InterlockedCompareExchange,(
    IN OUT PVOID *Destination,
    IN PVOID ExChange,
    IN PVOID Comperand
    ),
    (
    IN OUT PVOID *Destination,
    IN PVOID ExChange,
    IN PVOID Comperand
    ))
#endif

#define CrNtInterlockedIncrement InterlockedIncrement
#define CrNtInterlockedDecrement InterlockedDecrement
#define CrNtInterlockedExchangeAdd InterlockedExchangeAdd
#define CrNtInterlockedCompareExchange InterlockedCompareExchange

CROSSNT_DECL_EX("HAL.DLL",
KIRQL,__stdcall,
KeRaiseIrqlToDpcLevel,(),())

CROSSNT_DECL_EX("HAL.DLL",
KIRQL,__stdcall,
KeRaiseIrqlToSynchLevel,(),())

CROSSNT_DECL_EX("NDIS.SYS",
VOID,
__stdcall,
NdisInitializeReadWriteLock,(
    IN PNDIS_RW_LOCK Lock
    ),
    (
    Lock
    ))

CROSSNT_DECL_EX("NDIS.SYS",
VOID,
__stdcall,
NdisAcquireReadWriteLock,(
    IN PNDIS_RW_LOCK Lock,
    IN BOOLEAN       fWrite,
    IN PLOCK_STATE   LockState
    ),
    (
    Lock,
    fWrite,
    LockState
    ))

CROSSNT_DECL_EX("NDIS.SYS",
VOID,
__stdcall,
NdisReleaseReadWriteLock,(
    IN PNDIS_RW_LOCK Lock,
    IN PLOCK_STATE   LockState
    ),
    (
    Lock,
    LockState
    ))


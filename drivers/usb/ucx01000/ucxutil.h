/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Small C++ helpers shared by every part of the driver
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Pool tag for everything ucx01000 allocates itself ("UHCD" in a pool dump) */
#define UCX_POOL_TAG 'DCHU'

/* Objects live in WDF context memory; this builds them in place */
inline
PVOID
operator new(
    _In_ size_t Size,
    _In_ PVOID Where) noexcept
{
    UNREFERENCED_PARAMETER(Size);
    return Where;
}

inline
VOID
operator delete(
    _In_ PVOID Memory,
    _In_ PVOID Where) noexcept
{
    UNREFERENCED_PARAMETER(Memory);
    UNREFERENCED_PARAMETER(Where);
}

/** Holds a spin lock for the lifetime of the guard. */
class SpinLockGuard
{
public:
    explicit
    SpinLockGuard(
        _Inout_ PKSPIN_LOCK Lock)
        : m_Lock(Lock)
    {
        KeAcquireSpinLock(m_Lock, &m_OldIrql);
    }

    ~SpinLockGuard()
    {
        KeReleaseSpinLock(m_Lock, m_OldIrql);
    }

    SpinLockGuard(const SpinLockGuard&) = delete;
    SpinLockGuard& operator=(const SpinLockGuard&) = delete;

private:
    PKSPIN_LOCK m_Lock;
    KIRQL m_OldIrql;
};

/** Raises to DISPATCH_LEVEL for the lifetime of the guard. */
class DispatchLevelGuard
{
public:
    DispatchLevelGuard()
    {
        KeRaiseIrql(DISPATCH_LEVEL, &m_OldIrql);
    }

    ~DispatchLevelGuard()
    {
        KeLowerIrql(m_OldIrql);
    }

    DispatchLevelGuard(const DispatchLevelGuard&) = delete;
    DispatchLevelGuard& operator=(const DispatchLevelGuard&) = delete;

private:
    KIRQL m_OldIrql;
};

/* A LIST_ENTRY with NULL links means "not on any list" throughout this driver */
FORCEINLINE
VOID
NTAPI
UcxClearListEntry(
    _Out_ PLIST_ENTRY Entry)
{
    Entry->Flink = NULL;
    Entry->Blink = NULL;
}

FORCEINLINE
BOOLEAN
NTAPI
UcxIsListEntryLinked(
    _In_ const LIST_ENTRY* Entry)
{
    return Entry->Flink != NULL;
}

/** The four Parameters.Others arguments of the request's current stack location. */
struct UcxRequestArgs
{
    PVOID Arg1;
    PVOID Arg2;
    PVOID Arg4;

    explicit
    UcxRequestArgs(
        _In_ WDFREQUEST Request)
    {
        WDF_REQUEST_PARAMETERS Params;

        WDF_REQUEST_PARAMETERS_INIT(&Params);
        WdfRequestGetParameters(Request, &Params);

        Arg1 = Params.Parameters.Others.Arg1;
        Arg2 = Params.Parameters.Others.Arg2;
        Arg4 = Params.Parameters.Others.Arg4;
    }
};

/** Creates a WDF object with a primary set of attributes and an optional second context. */
_Must_inspect_result_
NTSTATUS
NTAPI
UcxCreateObjectWithTwoContexts(
    _In_ PWDF_OBJECT_ATTRIBUTES Primary,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Secondary,
    _Out_ WDFOBJECT* Object);

/* Kernel routines resolved at load time; NULL when the kernel lacks them */
typedef
BOOLEAN
(NTAPI *PFN_UCX_IO_TRY_QUEUE_WORKITEM)(
    _Inout_ PIO_WORKITEM IoWorkItem,
    _In_ PIO_WORKITEM_ROUTINE_EX WorkerRoutine,
    _In_ WORK_QUEUE_TYPE QueueType,
    _In_opt_ PVOID Context);

/** Driver wide state, one instance, set up in DriverEntry. */
struct UcxDriverState
{
    WDFDRIVER Driver;
    WDFDEVICE ControlDevice;
    UNICODE_STRING ControlDeviceName;
    WCHAR ControlDeviceNameBuffer[100];

    /* Every live controller, for diagnostics */
    LIST_ENTRY ControllerList;
    KSPIN_LOCK ControllerListLock;
    ULONG ControllerCount;

    /* usbflags Allow64KLowOrFullSpeedControlTransfers == 1 */
    BOOLEAN Allow64KLowOrFullSpeedControl;

    PFN_UCX_IO_TRY_QUEUE_WORKITEM IoTryQueueWorkItem;

    /* Seed for verifier fault injection */
    ULONG RandomSeed;
};

extern UcxDriverState UcxDriver;

/** Stops in the debugger on a controller driver bug, only while verifying and attached. */
FORCEINLINE
VOID
NTAPI
UcxVerifierBreak(
    _In_ BOOLEAN Verifying)
{
    if (Verifying && !KdRefreshDebuggerNotPresent())
        DbgBreakPoint();
}

/* Verifier fault injection helpers, safe at DISPATCH_LEVEL */

ULONG
NTAPI
UcxRandom(VOID);

BOOLEAN
NTAPI
UcxVerifierWantsFailure(
    _In_ ULONG Setting);

ULONG
NTAPI
UcxRandomInRange(
    _In_ ULONG Max);

NTSTATUS
NTAPI
UcxRandomErrorStatus(VOID);

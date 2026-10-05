/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Small C++ helpers shared by every part of the driver
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Pool tags, as they show up in a pool dump */
#define HUB_TAG_DRIVER 'w3HU'
#define HUB_TAG_HUB    'h3HU'
#define HUB_TAG_DEVICE 'd3HU'
#define HUB_TAG_PORT   'p3HU'

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
class HubSpinLockGuard
{
public:
    explicit
    HubSpinLockGuard(
        _Inout_ PKSPIN_LOCK Lock)
        : m_Lock(Lock)
    {
        KeAcquireSpinLock(m_Lock, &m_OldIrql);
    }

    ~HubSpinLockGuard()
    {
        KeReleaseSpinLock(m_Lock, m_OldIrql);
    }

    HubSpinLockGuard(const HubSpinLockGuard&) = delete;
    HubSpinLockGuard& operator=(const HubSpinLockGuard&) = delete;

private:
    PKSPIN_LOCK m_Lock;
    KIRQL m_OldIrql;
};

/** Holds a WDF wait lock for the lifetime of the guard. PASSIVE_LEVEL only. */
class HubWaitLockGuard
{
public:
    explicit
    HubWaitLockGuard(
        _In_ WDFWAITLOCK Lock)
        : m_Lock(Lock)
    {
        WdfWaitLockAcquire(m_Lock, NULL);
    }

    ~HubWaitLockGuard()
    {
        WdfWaitLockRelease(m_Lock);
    }

    HubWaitLockGuard(const HubWaitLockGuard&) = delete;
    HubWaitLockGuard& operator=(const HubWaitLockGuard&) = delete;

private:
    WDFWAITLOCK m_Lock;
};

/* One shot state machine timer built on a KTIMER and DPC */
class HubTimer
{
public:
    typedef VOID (NTAPI *PFN_HUB_TIMER_FIRED)(_In_ PVOID Context);

    VOID
    Initialize(
        _In_ PFN_HUB_TIMER_FIRED Fired,
        _In_ PVOID Context)
    {
        m_Fired = Fired;
        m_Context = Context;
        KeInitializeTimer(&m_Timer);
        KeInitializeDpc(&m_Dpc, DpcRoutine, this);
    }

    VOID
    Start(
        _In_ ULONG Milliseconds)
    {
        LARGE_INTEGER DueTime;

        DueTime.QuadPart = -10000LL * Milliseconds;
        KeSetTimer(&m_Timer, DueTime, &m_Dpc);
    }

    /** FALSE when the timer already fired and its callback is coming. */
    BOOLEAN
    Cancel()
    {
        return KeCancelTimer(&m_Timer);
    }

private:
    static
    VOID
    NTAPI
    DpcRoutine(
        _In_ PKDPC Dpc,
        _In_opt_ PVOID Context,
        _In_opt_ PVOID Argument1,
        _In_opt_ PVOID Argument2)
    {
        HubTimer* Self = (HubTimer*)Context;

        UNREFERENCED_PARAMETER(Dpc);
        UNREFERENCED_PARAMETER(Argument1);
        UNREFERENCED_PARAMETER(Argument2);

        Self->m_Fired(Self->m_Context);
    }

    KTIMER m_Timer;
    KDPC m_Dpc;
    PFN_HUB_TIMER_FIRED m_Fired;
    PVOID m_Context;
};

/* A LIST_ENTRY with NULL links means "not on any list" throughout this driver */
FORCEINLINE
VOID
NTAPI
HubClearListEntry(
    _Out_ PLIST_ENTRY Entry)
{
    Entry->Flink = NULL;
    Entry->Blink = NULL;
}

FORCEINLINE
BOOLEAN
NTAPI
HubIsListEntryLinked(
    _In_ const LIST_ENTRY* Entry)
{
    return Entry->Flink != NULL;
}

/**
 * Waits on a PnP handshake event with no timeout, printing a watchdog line
 * every minute so a stuck machine is visible in the debugger.
 */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWaitForPnpEvent(
    _In_ PKEVENT Event,
    _In_ PCSTR What,
    _In_ WDFOBJECT Object);

/** Drops a reference at DISPATCH_LEVEL so a last release never waits on its own work item. */
FORCEINLINE
VOID
NTAPI
HubDereferenceDeferred(
    _In_ WDFOBJECT Object)
{
    KIRQL Irql;

    KeRaiseIrql(DISPATCH_LEVEL, &Irql);
    WdfObjectDereference(Object);
    KeLowerIrql(Irql);
}

/** WdfObjectDelete with the same deferral as HubDereferenceDeferred. */
FORCEINLINE
VOID
NTAPI
HubDeleteDeferred(
    _In_ WDFOBJECT Object)
{
    KIRQL Irql;

    KeRaiseIrql(DISPATCH_LEVEL, &Irql);
    WdfObjectDelete(Object);
    KeLowerIrql(Irql);
}

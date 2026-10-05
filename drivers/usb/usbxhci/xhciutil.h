/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Small C++ helpers shared by every part of the driver
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Pool tags, as they show up in a pool dump */
#define XHCI_TAG_DRIVER      'dHCX'
#define XHCI_TAG_CONTROLLER  'cHCX'
#define XHCI_TAG_BUFFER      'bHCX'
#define XHCI_TAG_COMMAND     'mHCX'
#define XHCI_TAG_INTERRUPTER 'iHCX'
#define XHCI_TAG_ROOTHUB     'rHCX'
#define XHCI_TAG_SLOT        'sHCX'

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
class XhciSpinLockGuard
{
public:
    explicit
    XhciSpinLockGuard(
        _Inout_ PKSPIN_LOCK Lock)
        : m_Lock(Lock)
    {
        KeAcquireSpinLock(m_Lock, &m_OldIrql);
    }

    ~XhciSpinLockGuard()
    {
        KeReleaseSpinLock(m_Lock, m_OldIrql);
    }

    XhciSpinLockGuard(const XhciSpinLockGuard&) = delete;
    XhciSpinLockGuard& operator=(const XhciSpinLockGuard&) = delete;

private:
    PKSPIN_LOCK m_Lock;
    KIRQL m_OldIrql;
};

/** Holds a WDF spin lock for the lifetime of the guard. */
class XhciWdfLockGuard
{
public:
    explicit
    XhciWdfLockGuard(
        _In_ WDFSPINLOCK Lock)
        : m_Lock(Lock)
    {
        WdfSpinLockAcquire(m_Lock);
    }

    ~XhciWdfLockGuard()
    {
        WdfSpinLockRelease(m_Lock);
    }

    XhciWdfLockGuard(const XhciWdfLockGuard&) = delete;
    XhciWdfLockGuard& operator=(const XhciWdfLockGuard&) = delete;

private:
    WDFSPINLOCK m_Lock;
};

/** Holds a WDF wait lock for the lifetime of the guard. PASSIVE_LEVEL only. */
class XhciWaitLockGuard
{
public:
    explicit
    XhciWaitLockGuard(
        _In_ WDFWAITLOCK Lock)
        : m_Lock(Lock)
    {
        WdfWaitLockAcquire(m_Lock, NULL);
    }

    ~XhciWaitLockGuard()
    {
        WdfWaitLockRelease(m_Lock);
    }

    XhciWaitLockGuard(const XhciWaitLockGuard&) = delete;
    XhciWaitLockGuard& operator=(const XhciWaitLockGuard&) = delete;

private:
    WDFWAITLOCK m_Lock;
};

/* A LIST_ENTRY with NULL links means "not on any list" throughout this driver */
FORCEINLINE
VOID
XhciClearListEntry(
    _Out_ PLIST_ENTRY Entry)
{
    Entry->Flink = NULL;
    Entry->Blink = NULL;
}

FORCEINLINE
BOOLEAN
XhciIsListEntryLinked(
    _In_ const LIST_ENTRY* Entry)
{
    return Entry->Flink != NULL;
}

/** Milliseconds to a relative KeSetTimer/KeDelayExecutionThread interval. */
FORCEINLINE
LARGE_INTEGER
XhciRelativeMs(
    _In_ ULONG Milliseconds)
{
    LARGE_INTEGER Interval;

    Interval.QuadPart = -10000LL * Milliseconds;
    return Interval;
}

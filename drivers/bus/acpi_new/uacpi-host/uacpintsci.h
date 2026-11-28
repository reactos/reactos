/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Private definitions shared by the uACPI host files
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#include <uacpi/kernel_api.h>

/* Allocations made on behalf of the interpreter */
#define UACPINT_HOST_POOL_TAG   'HpcA'

typedef enum _UACPINT_HOST_DEFER_KIND
{
    UacpiNtHostDeferWork = 0,
    UacpiNtHostDeferSignal
} UACPINT_HOST_DEFER_KIND;

/*
 * ISR to DPC hand off, embedded first in every object that can be deferred.
 * SLIST entries need 16 byte alignment on 64 bit.
 */
typedef struct _UACPINT_HOST_DEFER
{
    SLIST_ENTRY             Link;
    UACPINT_HOST_DEFER_KIND Kind;
} UACPINT_HOST_DEFER, *PUACPINT_HOST_DEFER;

/*
 * Counting semaphore handed to uACPI for both its locks and its events
 */
typedef struct _UACPINT_HOST_SEMAPHORE
{
    UACPINT_HOST_DEFER Defer;
    KSEMAPHORE         Semaphore;
    LONG               Limit;
    volatile LONG      DeferredUnits;   ///< given above DISPATCH_LEVEL, released by the DPC
    volatile PVOID     Owner;           ///< last taker, reported for stuck lock waits
} UACPINT_HOST_SEMAPHORE, *PUACPINT_HOST_SEMAPHORE;

/*
 * SCI handler installed by uACPI, connected later by UacpiNtHostConnectSci
 */
typedef struct _UACPINT_HOST_INTERRUPT
{
    PKINTERRUPT             InterruptObject;
    KDPC                    Dpc;
    uacpi_interrupt_handler Handler;
    uacpi_handle            Context;
    ULONG                   Vector;
    KIRQL                   Irql;
    BOOLEAN                 LevelTriggered;
    ULONG                   UnclaimedCount;      ///< consecutive unclaimed SCIs
} UACPINT_HOST_INTERRUPT, *PUACPINT_HOST_INTERRUPT;

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

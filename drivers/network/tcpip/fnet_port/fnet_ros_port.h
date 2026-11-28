/*
 * PROJECT:     ReactOS TCP/IP protocol driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FNET kernel port: heap, timer, worker and upcall threads
 * COPYRIGHT:   Copyright 2026 Muhammad Yahya Bin Ehsan Qureshi <yahyauni54@gmail.com>
 */

#pragma once

#define FNET_PORT_TAG               'TENF'

/* Heap is RAM / divisor clamped to [MIN, MAX]; REG_DWORD Tcpip\Parameters\FnetHeapSize overrides it */
#define FNET_PORT_HEAP_RAM_DIVISOR  64
#define FNET_PORT_HEAP_MIN          (4 * 1024 * 1024)
#define FNET_PORT_HEAP_MAX          (64 * 1024 * 1024)
#define FNET_PORT_HEAP_OVERRIDE_MIN (1 * 1024 * 1024)
#define FNET_PORT_HEAP_OVERRIDE_MAX (512 * 1024 * 1024)

/* Longest gap between two fnet_poll() calls; FNET timers have this granularity */
#define FNET_PORT_POLL_MS           20

typedef VOID (*PFNET_PORT_WORK_ROUTINE)(PVOID Context);

/* Owned by the caller; must stay valid until the routine has run */
typedef struct _FNET_PORT_WORK_ITEM
{
    LIST_ENTRY ListEntry;
    PFNET_PORT_WORK_ROUTINE Routine;
    PVOID Context;
} FNET_PORT_WORK_ITEM, *PFNET_PORT_WORK_ITEM;

/* PASSIVE_LEVEL */
NTSTATUS
FnetPortInitialize(VOID);

/* PASSIVE_LEVEL */
VOID
FnetPortShutdown(VOID);

/* IRQL <= DISPATCH_LEVEL. Routine runs on the worker thread, where it may call FNET. */
VOID
FnetPortQueueWork(
    _Inout_ PFNET_PORT_WORK_ITEM Item,
    _In_ PFNET_PORT_WORK_ROUTINE Routine,
    _In_opt_ PVOID Context);

/* IRQL <= APC_LEVEL. Runs Routine on the calling thread inside FNET; nests when already inside. */
VOID
FnetPortRunSync(
    _In_ PFNET_PORT_WORK_ROUTINE Routine,
    _In_opt_ PVOID Context);

/* Inside FNET only. Runs after the current FNET call, before FNET is left. */
VOID
FnetPortQueueLocal(
    _Inout_ PFNET_PORT_WORK_ITEM Item,
    _In_ PFNET_PORT_WORK_ROUTINE Routine,
    _In_opt_ PVOID Context);

/* Inside FNET only. Routine runs later, in order, outside FNET; only upcalls may complete IRPs. */
VOID
FnetPortDeferUpcall(
    _Inout_ PFNET_PORT_WORK_ITEM Item,
    _In_ PFNET_PORT_WORK_ROUTINE Routine,
    _In_opt_ PVOID Context);

/* Runs pending upcalls on the calling thread. Call with no tcpip locks held; does nothing above PASSIVE_LEVEL. */
VOID
FnetPortDrainUpcalls(VOID);

BOOLEAN
FnetPortInsideFnet(VOID);

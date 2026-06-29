/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Portable processor related routines
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

KAFFINITY KeActiveProcessors = 0;

/* Number of processors */
CCHAR KeNumberProcessors = 0;

#ifdef CONFIG_SMP

/* Theoretical maximum number of processors that can be handled.
 * Set once at run-time. Returned by KeQueryMaximumProcessorCount(). */
ULONG KeMaximumProcessors = MAXIMUM_PROCESSORS;

/* Maximum number of logical processors that can be started
 * (including dynamically) at run-time. If 0: do not perform checks. */
ULONG KeNumprocSpecified = 0;

/* Maximum number of logical processors that can be started
 * at boot-time. If 0: do not perform checks. */
ULONG KeBootprocSpecified = 0;

#endif // CONFIG_SMP

/* FUNCTIONS *****************************************************************/

KAFFINITY
NTAPI
KeQueryActiveProcessors(VOID)
{
    return KeActiveProcessors;
}

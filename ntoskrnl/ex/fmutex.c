/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/fmutex.c
 * PURPOSE:         Implements fast mutexes
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* Undefine some macros we implement here */
#undef ExEnterCriticalRegionAndAcquireFastMutexUnsafe
#undef ExReleaseFastMutexUnsafeAndLeaveCriticalRegion
#undef ExAcquireFastMutex
#undef ExReleaseFastMutex
#undef ExAcquireFastMutexUnsafe
#undef ExReleaseFastMutexUnsafe
#undef ExTryToAcquireFastMutex

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
FASTCALL
ExEnterCriticalRegionAndAcquireFastMutexUnsafe(IN OUT PFAST_MUTEX FastMutex)
{
    /* Call the inline */
	_ExEnterCriticalRegionAndAcquireFastMutexUnsafe(FastMutex);
}

/*
 * @implemented
 */
VOID
FASTCALL
ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(IN OUT PFAST_MUTEX FastMutex)
{
    /* Call the inline */
	_ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(FastMutex);
}

/*
 * @implemented
 */
VOID
FASTCALL
ExAcquireFastMutex(IN OUT PFAST_MUTEX FastMutex)
{
    /* Call the inline */
    _ExAcquireFastMutex(FastMutex);
}

/*
 * @implemented
 */
VOID
FASTCALL
ExReleaseFastMutex(IN OUT PFAST_MUTEX FastMutex)
{
    /* Call the inline */
    _ExReleaseFastMutex(FastMutex);
}

/*
 * @implemented
 */
VOID
FASTCALL
ExAcquireFastMutexUnsafe(IN OUT PFAST_MUTEX FastMutex)
{
    /* Acquire the mutex unsafely */
    _ExAcquireFastMutexUnsafe(FastMutex);
}

/*
 * @implemented
 */
VOID
FASTCALL
ExReleaseFastMutexUnsafe(IN OUT PFAST_MUTEX FastMutex)
{
    /* Release the mutex unsafely */
    _ExReleaseFastMutexUnsafe(FastMutex);
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex(IN OUT PFAST_MUTEX FastMutex)
{
    /* Call the inline */
    return _ExTryToAcquireFastMutex(FastMutex);
}

/* EOF */

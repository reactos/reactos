/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/gmutex.c
 * PURPOSE:         Implements Guarded Mutex
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Filip Navara (navaraf@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* Undefine some macros we implement here */
#undef KeInitializeGuardedMutex
#undef KeAcquireGuardedMutex
#undef KeReleaseGuardedMutex
#undef KeAcquireGuardedMutexUnsafe
#undef KeReleaseGuardedMutexUnsafe
#undef KeTryToAcquireGuardedMutex

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
FASTCALL
KeInitializeGuardedMutex(OUT PKGUARDED_MUTEX GuardedMutex)
{
    /* Call the inline */
    _KeInitializeGuardedMutex(GuardedMutex);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeAcquireGuardedMutex(IN PKGUARDED_MUTEX GuardedMutex)
{
    /* Call the inline */
    _KeAcquireGuardedMutex(GuardedMutex);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeReleaseGuardedMutex(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    /* Call the inline */
    _KeReleaseGuardedMutex(GuardedMutex);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeAcquireGuardedMutexUnsafe(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    /* Call the inline */
    _KeAcquireGuardedMutexUnsafe(GuardedMutex);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeReleaseGuardedMutexUnsafe(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    /* Call the inline */
    _KeReleaseGuardedMutexUnsafe(GuardedMutex);
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
KeTryToAcquireGuardedMutex(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    /* Call the inline */
    return _KeTryToAcquireGuardedMutex(GuardedMutex);
}

/**
 * @name KeEnterGuardedRegion
 *
 * Enters a guarded region. This causes all (incl. special kernel) APCs
 * to be disabled.
 */
VOID
NTAPI
_KeEnterGuardedRegion(VOID)
{
    /* Use the inlined version */
    KeEnterGuardedRegion();
}

/**
 * @name KeLeaveGuardedRegion
 *
 * Leaves a guarded region and delivers pending APCs if possible.
 */
VOID
NTAPI
_KeLeaveGuardedRegion(VOID)
{
    /* Use the inlined version */
    KeLeaveGuardedRegion();
}

/* EOF */

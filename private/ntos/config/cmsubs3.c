/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmsubs3.c

Abstract:

    This module contains locking support routines for the configuration manager.

Author:

    Bryan M. Willman (bryanwi) 30-Mar-1992

Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpUnlockRegistry)

#if DBG
#pragma alloc_text(PAGE,CmpTestRegistryLock)
#pragma alloc_text(PAGE,CmpTestRegistryLockExclusive)
#endif

#endif


//
// Global registry lock
//

ERESOURCE CmpRegistryLock;

PVOID       CmpCaller;
PVOID       CmpCallerCaller;

VOID
CmpLockRegistry(
    VOID
    )
/*++

Routine Description:

    Lock the registry for shared (read-only) access

Arguments:

    None.

Return Value:

    None, the registry lock will be held for shared access upon return.

--*/
{
    #if DBG
    PVOID       Caller;
    PVOID       CallerCaller;
    #endif

    KeEnterCriticalRegion();
    ExAcquireResourceShared(&CmpRegistryLock, TRUE);

    #if DBG
    RtlGetCallersAddress(&Caller, &CallerCaller);
    CMLOG(CML_FLOW, CMS_LOCKING) {
        KdPrint(("CmpLockRegistry: c, cc: %08lx  %08lx\n", Caller, CallerCaller));
    }
    #endif

}

VOID
CmpLockRegistryExclusive(
    VOID
    )
/*++

Routine Description:

    Lock the registry for exclusive (write) access.

Arguments:

    CanWait - Supplies whether or not the call should wait
              for the resource or return immediately.

              If CanWait is TRUE, this will always return
              TRUE.

Return Value:

    TRUE - Lock was acquired exclusively

    FALSE - Lock is owned by another thread.

--*/
{
    BOOLEAN Status;


    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&CmpRegistryLock,TRUE);

    RtlGetCallersAddress(&CmpCaller, &CmpCallerCaller);
}

VOID
CmpUnlockRegistry(
    )
/*++

Routine Description:

    Unlock the registry.

--*/
{
    ASSERT_CM_LOCK_OWNED();
    ExReleaseResource(&CmpRegistryLock);
    KeLeaveCriticalRegion();
}


#if DBG

BOOLEAN
CmpTestRegistryLock(VOID)
{
    BOOLEAN rc;

    rc = TRUE;
    if (ExIsResourceAcquiredShared(&CmpRegistryLock) == 0) {
        rc = FALSE;
    }
    return rc;
}

BOOLEAN
CmpTestRegistryLockExclusive(VOID)
{
    if (ExIsResourceAcquiredExclusive(&CmpRegistryLock) == 0) {
        return(FALSE);
    }
    return(TRUE);
}

#endif

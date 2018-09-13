/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    cmgquota.c

Abstract:

    The module contains CM routines to support Global Quota

    Global Quota has little to do with NT's standard per-process/user
    quota system.  Global Quota is waying of controlling the aggregate
    resource usage of the entire registry.  It is used to manage space
    consumption by objects which user apps create, but which are persistent
    and therefore cannot be assigned to the quota of a user app.

    Global Quota prevents the registry from consuming all of paged
    pool, and indirectly controls how much disk it can consume.
    Like the release 1 file systems, a single app can fill all the
    space in the registry, but at least it cannot kill the system.

    Memory objects used for known short times and protected by
    serialization, or billable as quota objects, are not counted
    in the global quota.

Author:

    Bryan M. Willman (bryanwi) 13-Jan-1993

Revision History:

--*/

#include "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpClaimGlobalQuota)
#pragma alloc_text(PAGE,CmpReleaseGlobalQuota)
#pragma alloc_text(PAGE,CmpSetGlobalQuotaAllowed)
#pragma alloc_text(PAGE,CmpQuotaWarningWorker)
#pragma alloc_text(PAGE,CmQueryRegistryQuotaInformation)
#pragma alloc_text(PAGE,CmSetRegistryQuotaInformation)
#pragma alloc_text(INIT,CmpComputeGlobalQuotaAllowed)
#endif

//
// Registry control values
//
#define CM_DEFAULT_RATIO            (3)
#define CM_LIMIT_RATIO(x)           ((x / 10) * 8)
#define CM_MINIMUM_GLOBAL_QUOTA     (16 *1024 * 1024)

//
// Percent of used registry quota that triggers a hard error
// warning popup.
//
#define CM_REGISTRY_WARNING_LEVEL   (95)

extern ULONG CmRegistrySizeLimit;
extern ULONG CmRegistrySizeLimitLength;
extern ULONG CmRegistrySizeLimitType;

extern ULONG MmSizeOfPagedPoolInBytes;

//
// Maximum number of bytes of Global Quota the registry may use.
// Set to largest positive number for use in boot.  Will be set down
// based on pool and explicit registry values.
//
extern ULONG   CmpGlobalQuota;
extern ULONG   CmpGlobalQuotaAllowed;

//
// Mark that will trigger the low-on-quota popup
//
extern ULONG   CmpGlobalQuotaWarning;

//
// Indicate whether the popup has been triggered yet or not.
//
extern BOOLEAN CmpQuotaWarningPopupDisplayed;

//
// GQ actually in use
//
extern ULONG   CmpGlobalQuotaUsed;


VOID
CmQueryRegistryQuotaInformation(
    IN PSYSTEM_REGISTRY_QUOTA_INFORMATION RegistryQuotaInformation
    )

/*++

Routine Description:

    Returns the registry quota information

Arguments:

    RegistryQuotaInformation - Supplies pointer to buffer that will return
        the registry quota information.

Return Value:

    None.

--*/

{
    RegistryQuotaInformation->RegistryQuotaAllowed  = CmpGlobalQuota;
    RegistryQuotaInformation->RegistryQuotaUsed     = CmpGlobalQuotaUsed;
    RegistryQuotaInformation->PagedPoolSize         = MmSizeOfPagedPoolInBytes;
}


VOID
CmSetRegistryQuotaInformation(
    IN PSYSTEM_REGISTRY_QUOTA_INFORMATION RegistryQuotaInformation
    )

/*++

Routine Description:

    Sets the registry quota information.  The caller is assumed to have
    completed the necessary security checks already.

Arguments:

    RegistryQuotaInformation - Supplies pointer to buffer that provides
        the new registry quota information.

Return Value:

    None.

--*/

{
    CmpGlobalQuota = RegistryQuotaInformation->RegistryQuotaAllowed;

    //
    // Sanity checks against insane values
    //
    if (CmpGlobalQuota > CM_WRAP_LIMIT) {
        CmpGlobalQuota = CM_WRAP_LIMIT;
    }
    if (CmpGlobalQuota < CM_MINIMUM_GLOBAL_QUOTA) {
        CmpGlobalQuota = CM_MINIMUM_GLOBAL_QUOTA;
    }

    //
    // Recompute the warning level
    //
    CmpGlobalQuotaWarning = CM_REGISTRY_WARNING_LEVEL * (CmpGlobalQuota / 100);

    CmpGlobalQuotaAllowed = CmpGlobalQuota;
}


VOID
CmpQuotaWarningWorker(
    IN PVOID WorkItem
    )

/*++

Routine Description:

    Displays hard error popup that indicates the registry quota is
    running out.

Arguments:

    WorkItem - Supplies pointer to the work item. This routine will
               free the work item.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ULONG Response;

    ExFreePool(WorkItem);

    Status = ExRaiseHardError(STATUS_REGISTRY_QUOTA_LIMIT,
                              0,
                              0,
                              NULL,
                              OptionOk,
                              &Response);
}


BOOLEAN
CmpClaimGlobalQuota(
    IN ULONG    Size
    )
/*++

Routine Description:

    If CmpGlobalQuotaUsed + Size >= CmpGlobalQuotaAllowed, return
    false.  Otherwise, increment CmpGlobalQuotaUsed, in effect claiming
    the requested GlobalQuota.

Arguments:

    Size - number of bytes of GlobalQuota caller wants to claim

Return Value:

    TRUE - Claim succeeded, and has been counted in Used GQ

    FALSE - Claim failed, nothing counted in GQ.

--*/
{
    LONG   available;
    PWORK_QUEUE_ITEM WorkItem;

    //
    // compute available space, then see if size <.  This prevents overflows.
    // Note that this must be signed. Since quota is not enforced until logon,
    // it is possible for the available bytes to be negative.
    //

    available = (LONG)CmpGlobalQuotaAllowed - (LONG)CmpGlobalQuotaUsed;

    if ((LONG)Size < available) {
        CmpGlobalQuotaUsed += Size;
        if ((CmpGlobalQuotaUsed > CmpGlobalQuotaWarning) &&
            (!CmpQuotaWarningPopupDisplayed) &&
            (ExReadyForErrors)) {

            //
            // Queue work item to display popup
            //
            WorkItem = ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
            if (WorkItem != NULL) {

                CmpQuotaWarningPopupDisplayed = TRUE;
                ExInitializeWorkItem(WorkItem,
                                     CmpQuotaWarningWorker,
                                     WorkItem);
                ExQueueWorkItem(WorkItem, DelayedWorkQueue);
            }
        }
        return TRUE;
    } else {
        return FALSE;
    }
}


VOID
CmpReleaseGlobalQuota(
    IN ULONG    Size
    )
/*++

Routine Description:

    If Size <= CmpGlobalQuotaUsed, then decrement it.  Else BugCheck.

Arguments:

    Size - number of bytes of GlobalQuota caller wants to release

Return Value:

    NONE.

--*/
{
    if (Size > CmpGlobalQuotaUsed) {
        KeBugCheckEx(REGISTRY_ERROR,2,1,0,0);
    }

    CmpGlobalQuotaUsed -= Size;
}


VOID
CmpComputeGlobalQuotaAllowed(
    VOID
    )

/*++

Routine Description:

    Compute CmpGlobalQuota based on:
        (a) Size of paged pool
        (b) Explicit user registry commands to set registry GQ

Return Value:

    NONE.

--*/

{
    ULONG   PagedLimit;

    PagedLimit = CM_LIMIT_RATIO(MmSizeOfPagedPoolInBytes);

    if ((CmRegistrySizeLimitLength != 4) ||
        (CmRegistrySizeLimitType != REG_DWORD) ||
        (CmRegistrySizeLimit == 0))
    {
        //
        // If no value at all, or value of wrong type, or set to
        // zero, use internally computed default
        //
        CmpGlobalQuota = MmSizeOfPagedPoolInBytes / CM_DEFAULT_RATIO;

    } else if (CmRegistrySizeLimit >= PagedLimit) {
        //
        // If more than computed upper bound, use computed upper bound
        //
        CmpGlobalQuota = PagedLimit;

    } else {
        //
        // Use the set size
        //
        CmpGlobalQuota = CmRegistrySizeLimit;

    }

    if (CmpGlobalQuota > CM_WRAP_LIMIT) {
        CmpGlobalQuota = CM_WRAP_LIMIT;
    }
    if (CmpGlobalQuota < CM_MINIMUM_GLOBAL_QUOTA) {
        CmpGlobalQuota = CM_MINIMUM_GLOBAL_QUOTA;
    }

    CmpGlobalQuotaWarning = CM_REGISTRY_WARNING_LEVEL * (CmpGlobalQuota / 100);

    return;
}


VOID
CmpSetGlobalQuotaAllowed(
    VOID
    )
/*++

Routine Description:

    Enables registry quota

    NOTE:   Do NOT put this in init segment, we call it after
            that code has been freed!

Return Value:

    NONE.

--*/
{
     CmpGlobalQuotaAllowed = CmpGlobalQuota;
}

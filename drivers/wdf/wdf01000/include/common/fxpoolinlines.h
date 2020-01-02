#ifndef _FXPOOLINLINES_H_
#define _FXPOOLINLINES_H_

#include "common/fxpool.h"

VOID
__inline
FxPoolRemoveNonPagedAllocateTracker(
    __in PFX_POOL_TRACKER  Tracker
    )
/*++

Routine Description:

    Decommission a Tracker for a NonPaged allocation.

Arguments:

    Tracker - Pointer to the formatted FX_POOL_TRACKER structure.

Returns:

    VOID

--*/
{
    KIRQL irql;

    Tracker->Pool->NonPagedLock.Acquire(&irql);

    RemoveEntryList(&Tracker->Link);

    Tracker->Pool->NonPagedBytes -= Tracker->Size;
    Tracker->Pool->NonPagedAllocations--;

    Tracker->Pool->NonPagedLock.Release(irql);
}

VOID
__inline
FxPoolRemovePagedAllocateTracker(
    __in PFX_POOL_TRACKER  Tracker
    )
/*++

Routine Description:

    Decommission a Tracker for a Paged allocation.

Arguments:

    Tracker - Pointer to the formatted FX_POOL_TRACKER structure.

Returns:

    VOID

--*/
{
    Tracker->Pool->PagedLock.Acquire();

    RemoveEntryList(&Tracker->Link);

    Tracker->Pool->PagedBytes -= Tracker->Size;
    Tracker->Pool->PagedAllocations--;

    Tracker->Pool->PagedLock.Release();
}

#endif //_FXPOOLINLINES_H_
#include "common/fxverifierlock.h"
#include "common/fxglobals.h"
#include "common/fxpool.h"
#include "common/dbgtrace.h"


//
// Called at Driver Frameworks init time
//
extern "C"
void
FxVerifierLockInitialize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    if( FxDriverGlobals->FxVerifierLock )
    {
    
        FxDriverGlobals->ThreadTableLock.Initialize();

        FxVerifierLock::AllocateThreadTable(FxDriverGlobals);
    }

    return;
}

void
FxVerifierLock::AllocateThreadTable(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    KIRQL       oldIrql;
    ULONG       newEntries;
    PLIST_ENTRY newTable;

    FxDriverGlobals->ThreadTableLock.Acquire(&oldIrql);

    if( FxDriverGlobals->ThreadTable != NULL )
    {
        FxDriverGlobals->ThreadTableLock.Release(oldIrql);
        return;
    }

    // Table must be kept as a power of 2 for hash algorithm
    newEntries = VERIFIER_THREAD_HASHTABLE_SIZE;

    newTable = (PLIST_ENTRY) FxPoolAllocateWithTag(
        FxDriverGlobals,
        NonPagedPool,
        sizeof(LIST_ENTRY) * newEntries,
        FxDriverGlobals->Tag);

    if( newTable == NULL )
    {
        FxDriverGlobals->ThreadTableLock.Release(oldIrql);
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "No Memory to allocate thread table");
        return;
    }

    for(ULONG index=0; index < newEntries; index++ )
    {
        InitializeListHead(&newTable[index]);
    }

    FxDriverGlobals->ThreadTable     = newTable;

    FxDriverGlobals->ThreadTableLock.Release(oldIrql);

    return;
}

//
// Called at Driver Frameworks is unloading
//
extern "C"
void
FxVerifierLockDestroy(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{    
    if( FxDriverGlobals->FxVerifierLock )
    {
        FxVerifierLock::FreeThreadTable(FxDriverGlobals);
        
        FxDriverGlobals->ThreadTableLock.Uninitialize();
    }
    
    return;
}

void
FxVerifierLock::FreeThreadTable(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(FxDriverGlobals);

    FxDriverGlobals->ThreadTableLock.Acquire(&oldIrql);

    if( FxDriverGlobals->ThreadTable == NULL )
    {
        FxDriverGlobals->ThreadTableLock.Release(oldIrql);
        return;
    }

    FxPoolFree(FxDriverGlobals->ThreadTable);

    FxDriverGlobals->ThreadTable = NULL;

    FxDriverGlobals->ThreadTableLock.Release(oldIrql);

    return;
}
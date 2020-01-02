#include "common/fxverifierlock.h"
#include "common/fxglobals.h"
#include "common/fxpool.h"

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
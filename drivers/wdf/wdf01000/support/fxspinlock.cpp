#include "common/fxspinlock.h"


FxSpinLock::FxSpinLock(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ExtraSize
    ) :
    FxObject(FX_TYPE_SPIN_LOCK, COMPUTE_OBJECT_SIZE(sizeof(FxSpinLock), ExtraSize), FxDriverGlobals)
{
    FX_SPIN_LOCK_HISTORY* pHistory;

    m_Irql = 0;
    m_InterruptLock = FALSE;

    pHistory = GetHistory();

    if (pHistory != NULL)
    {
        RtlZeroMemory(pHistory, sizeof(FX_SPIN_LOCK_HISTORY));

        pHistory->CurrentHistory = &pHistory->History[0];
    }
}

__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FxSpinLock::AcquireLock(
    __in PVOID CallersAddress
    )
{
    PFX_SPIN_LOCK_HISTORY pHistory;
    KIRQL irql;

    m_SpinLock.Acquire(&irql);

    m_Irql = irql;

    pHistory = GetHistory();

    if (pHistory != NULL)
    {
        PFX_SPIN_LOCK_HISTORY_ENTRY pCur;

        //
        // This assert should never fire here, but this helps track ownership
        // in the case of a release without an acquire.
        //
        ASSERT(pHistory->OwningThread == NULL);
        pHistory->OwningThread = Mx::MxGetCurrentThread();

        pCur = pHistory->CurrentHistory;

        Mx::MxQueryTickCount(&pCur->AcquiredAtTime);
        pCur->CallersAddress = CallersAddress;
    }
}
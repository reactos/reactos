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
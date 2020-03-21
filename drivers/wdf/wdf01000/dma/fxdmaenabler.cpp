#include "common/fxdmaenabler.h"
#include "common/dbgtrace.h"


// ----------------------------------------------------------------------------
// ------------------------ Pnp/Power notification -----------------------------
// ----------------------------------------------------------------------------

_Must_inspect_result_
NTSTATUS
FxDmaEnabler::PowerUp(
    VOID
    )
{
    NTSTATUS              status          = STATUS_SUCCESS;
    PFX_DRIVER_GLOBALS    pFxDriverGlobals = GetDriverGlobals();
    WDFDMAENABLER         handle          = GetHandle();
    FxDmaEnablerCallbacks tag             = FxEvtDmaEnablerInvalid;

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, 0,//TRACINGDMA,
                        "WDFDMAENABLER %p: PowerUp notification", GetHandle());

    do
    {
        if (m_EvtDmaEnablerFill.m_Method)
        {
            status = m_EvtDmaEnablerFill.Invoke( handle );

            if (!NT_SUCCESS(status))
            {
                m_DmaEnablerFillFailed = TRUE;
                tag = FxEvtDmaEnablerFill;
                break;
            }
        }

        if (m_EvtDmaEnablerEnable.m_Method)
        {
            status = m_EvtDmaEnablerEnable.Invoke( handle );

            if (!NT_SUCCESS(status))
            {
                m_DmaEnablerEnableFailed = TRUE;
                tag = FxEvtDmaEnablerEnable;
                break;
            }
        }

        if (m_EvtDmaEnablerSelfManagedIoStart.m_Method)
        {
            status = m_EvtDmaEnablerSelfManagedIoStart.Invoke( handle );

            if (!NT_SUCCESS(status))
            {
                m_DmaEnablerSelfManagedIoStartFailed = TRUE;
                tag = FxEvtDmaEnablerSelfManagedIoStart;
                break;
            }
        }

    } while (0);

    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, 0,//TRACINGDMA,
                            "WDFDMAENABLER %p: PowerUp: "
                            "%!WdfDmaEnablerCallback! failed %!STATUS!",
                            GetHandle(), tag, status);
    }

    return status;
}

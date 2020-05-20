/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     DMA enabler object
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */



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

_Must_inspect_result_
NTSTATUS
FxDmaEnabler::PowerDown(
    VOID
    )
{
    NTSTATUS              status          = STATUS_SUCCESS;
    NTSTATUS              localStatus;
    PFX_DRIVER_GLOBALS    pFxDriverGlobals = GetDriverGlobals();
    WDFDMAENABLER         handle          = GetHandle();
    FxDmaEnablerCallbacks tag             = FxEvtDmaEnablerInvalid;

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, 0,//TRACINGDMA,
                        "WDFDMAENABLER %p: PowerDown notification", GetHandle());

    do
    {
        if (m_EvtDmaEnablerSelfManagedIoStop.m_Method)
        {
            localStatus = m_EvtDmaEnablerSelfManagedIoStop.Invoke( handle );

            if (!NT_SUCCESS(localStatus))
            {
                tag = FxEvtDmaEnablerSelfManagedIoStop;
                status = (NT_SUCCESS(status)) ? localStatus : status;
            }
        }

        if (m_EvtDmaEnablerDisable.m_Method &&
            m_DmaEnablerFillFailed == FALSE)
        {
            localStatus = m_EvtDmaEnablerDisable.Invoke( handle );

            if (!NT_SUCCESS(localStatus))
            {
                tag = FxEvtDmaEnablerDisable;
                status = (NT_SUCCESS(status)) ? localStatus : status;
            }
        }

        if (m_EvtDmaEnablerFlush.m_Method     &&
            m_DmaEnablerFillFailed   == FALSE &&
            m_DmaEnablerEnableFailed == FALSE)
        {
            localStatus = m_EvtDmaEnablerFlush.Invoke( handle );

            if (!NT_SUCCESS(localStatus))
            {
                tag = FxEvtDmaEnablerFlush;
                status = (NT_SUCCESS(status)) ? localStatus : status;
            }
        }

    } while (0);

    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, 0,//TRACINGDMA,
                            "WDFDMAENABLER %p: PowerDown: "
                            "%!WdfDmaEnablerCallback! failed %!STATUS!",
                            GetHandle(), tag, status);
    }

    return status;
}

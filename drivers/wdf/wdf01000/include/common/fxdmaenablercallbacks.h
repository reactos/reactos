#ifndef _FXDMAENABLERCALLBACKS_H
#define _FXDMAENABLERCALLBACKS_H

#include "common/fxcallback.h"


enum FxDmaEnablerCallbacks {

    FxEvtDmaEnablerInvalid,
    FxEvtDmaEnablerFill,
    FxEvtDmaEnablerFlush,
    FxEvtDmaEnablerEnable,
    FxEvtDmaEnablerDisable,
    FxEvtDmaEnablerSelfManagedIoStart,
    FxEvtDmaEnablerSelfManagedIoStop,

};

//
// EvtDmaEnablerFill callback delegate
//
class FxEvtDmaEnablerFillCallback : public FxCallback {

public:
    PFN_WDF_DMA_ENABLER_FILL  m_Method;

    NTSTATUS  m_Status;

    FxEvtDmaEnablerFillCallback(
        VOID
        ) :
        FxCallback(),
        m_Method(NULL),
        m_Status(STATUS_SUCCESS)
    {
    }

    NTSTATUS
    Invoke(
        __in WDFDMAENABLER Handle
        )
    {
        if (m_Method)
        {
            CallbackStart();
            m_Status = m_Method( Handle );
            CallbackEnd();
        }
        else
        {
            m_Status = STATUS_SUCCESS;
        }
        return m_Status;
    }
};

//
// EvtDmaEnablerEnable callback delegate
//
class FxEvtDmaEnablerEnableCallback : public FxCallback {

public:
    PFN_WDF_DMA_ENABLER_ENABLE  m_Method;

    NTSTATUS  m_Status;

    FxEvtDmaEnablerEnableCallback(
        VOID
        ) :
        FxCallback(),
        m_Method(NULL),
        m_Status(STATUS_SUCCESS)
    {
    }

    NTSTATUS
    Invoke(
        __in WDFDMAENABLER Handle
        )
    {
        if (m_Method)
        {
            CallbackStart();
            m_Status = m_Method( Handle );
            CallbackEnd();
        }
        else
        {
            m_Status = STATUS_SUCCESS;
        }
        return m_Status;
    }
};

//
// EvtDmaEnablerSelfManagedIoStart callback delegate
//
class FxEvtDmaEnablerSelfManagedIoStartCallback : public FxCallback {

public:
    PFN_WDF_DMA_ENABLER_SELFMANAGED_IO_START m_Method;

    NTSTATUS  m_Status;

    FxEvtDmaEnablerSelfManagedIoStartCallback(
        VOID
        ) :
        FxCallback(),
        m_Method(NULL),
        m_Status(STATUS_SUCCESS)
    {
    }

    NTSTATUS
    Invoke(
        __in WDFDMAENABLER Handle
        )
    {
        if (m_Method)
        {
            CallbackStart();
            m_Status = m_Method( Handle );
            CallbackEnd();
        }
        else
        {
            m_Status = STATUS_SUCCESS;
        }
        return m_Status;
    }
};

#endif //_FXDMAENABLERCALLBACKS_H
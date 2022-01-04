/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDmaEnablerCallbacks.hpp

Abstract:

    This module implements the FxDmaEnabler object callbacks

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _FXDMAENABLERCALLBACKS_HPP
#define _FXDMAENABLERCALLBACKS_HPP

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
        if (m_Method) {
            CallbackStart();
            m_Status = m_Method( Handle );
            CallbackEnd();
        }
        else {
            m_Status = STATUS_SUCCESS;
        }
        return m_Status;
    }
};

//
// EvtDmaEnablerFlush callback delegate
//
class FxEvtDmaEnablerFlushCallback : public FxCallback {

public:
    PFN_WDF_DMA_ENABLER_FLUSH  m_Method;

    NTSTATUS  m_Status;

    FxEvtDmaEnablerFlushCallback(
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
        if (m_Method) {
            CallbackStart();
            m_Status = m_Method( Handle );
            CallbackEnd();
        }
        else {
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
        if (m_Method) {
            CallbackStart();
            m_Status = m_Method( Handle );
            CallbackEnd();
        }
        else {
            m_Status = STATUS_SUCCESS;
        }
        return m_Status;
    }
};

//
// EvtDmaEnablerDisable callback delegate
//
class FxEvtDmaEnablerDisableCallback : public FxCallback {

public:
    PFN_WDF_DMA_ENABLER_DISABLE  m_Method;

    NTSTATUS  m_Status;

    FxEvtDmaEnablerDisableCallback(
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
        if (m_Method) {
            CallbackStart();
            m_Status = m_Method( Handle );
            CallbackEnd();
        }
        else {
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
        if (m_Method) {
            CallbackStart();
            m_Status = m_Method( Handle );
            CallbackEnd();
        }
        else {
            m_Status = STATUS_SUCCESS;
        }
        return m_Status;
    }
};


//
// EvtDmaEnablerSelfManagedIoStop callback delegate
//
class FxEvtDmaEnablerSelfManagedIoStopCallback : public FxCallback {

public:
    PFN_WDF_DMA_ENABLER_SELFMANAGED_IO_STOP m_Method;

    NTSTATUS  m_Status;

    FxEvtDmaEnablerSelfManagedIoStopCallback(
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
        if (m_Method) {
            CallbackStart();
            m_Status = m_Method( Handle );
            CallbackEnd();
        }
        else {
            m_Status = STATUS_SUCCESS;
        }
        return m_Status;
    }
};


#endif // _FXDMAENABLERCALLBACKS_HPP

/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIoQueueCallbacks.h

Abstract:

    This module implements the I/O package queue object callbacks

Author:




Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXREQUESTCALLBACKS_H_
#define _FXREQUESTCALLBACKS_H_


//
// Delegate which contains EvtRequestCancel
//
class FxRequestCancelCallback : public FxCallback {

public:
    PFN_WDF_REQUEST_CANCEL m_Cancel;

    FxRequestCancelCallback(
        VOID
        )
    {
        m_Cancel = NULL;
    }

    void
    InvokeCancel(
        __in FxCallbackLock* Lock,
        __in WDFREQUEST Request
        )
    {
        if (m_Cancel != NULL) {
            PFN_WDF_REQUEST_CANCEL pMethod;
            KIRQL irql;

            //
            // Satisfy W4 warning, even though it is technically not necessary to
            // assign an initial value.
            //
            irql = PASSIVE_LEVEL;

            if (Lock != NULL) {
                Lock->Lock(&irql);
            }

            //
            // Clear the value before invoking the routine since the assignment
            // is invalidated when the routine is run.
            //
            pMethod = m_Cancel;
            m_Cancel = NULL;

            pMethod(Request);

            if (Lock != NULL) {
                Lock->Unlock(irql);
            }
        }
    }
};

//
// Delegate which contains EvtRequestCompletion
//
class FxRequestCompletionCallback : public FxCallback {

public:
    PFN_WDF_REQUEST_COMPLETION_ROUTINE m_Completion;

    FxRequestCompletionCallback(
        VOID
        )
    {
        m_Completion = NULL;
    }
};


#endif // _FXREQUESTCALLBACKS_H_


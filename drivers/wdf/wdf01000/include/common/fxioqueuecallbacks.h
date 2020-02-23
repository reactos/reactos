#ifndef _FXIOQUEUECALLBACKS_H_
#define _FXIOQUEUECALLBACKS_H_

#include "common/fxcallback.h"


//
// EvtIoQueueStatus callback delegate
//
class FxIoQueueIoState : public FxLockedCallback {

public:
    PFN_WDF_IO_QUEUE_STATE Method;

    FxIoQueueIoState(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    void
    Invoke(
        __in WDFQUEUE   Queue,
        __in WDFCONTEXT Context
        )
    {
        if (Method != NULL)
        {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(Queue, Context);
            CallbackEnd(irql);
        }
    }
};

class FxIoQueueForwardProgressAllocateResources : public FxCallback {

public:
    PFN_WDF_IO_ALLOCATE_REQUEST_RESOURCES   Method;

    FxIoQueueForwardProgressAllocateResources(
        VOID
        )   :
       FxCallback()
    {
        Method = NULL;
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFQUEUE   Queue,
        __in WDFREQUEST Request
        )
    {
        ASSERT(Method != NULL);
        return Method(Queue, Request);
    }
};

//
// EvtIoStop callback delegate
//
class FxIoQueueIoStop : public FxLockedCallback {

public:
    PFN_WDF_IO_QUEUE_IO_STOP Method;

    FxIoQueueIoStop(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    void
    Invoke(
        __in WDFQUEUE   Queue,
        __in WDFREQUEST Request,
        __in ULONG ActionFlags
        )
    {
        if (Method != NULL)
        {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(Queue, Request, ActionFlags);
            CallbackEnd(irql);
        }
    }
};

//
// EvtIoResume callback delegate
//
class FxIoQueueIoResume : public FxLockedCallback {

public:
    PFN_WDF_IO_QUEUE_IO_RESUME Method;

    FxIoQueueIoResume(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    void
    Invoke(
        __in WDFQUEUE   Queue,
        __in WDFREQUEST Request
        )
    {
        if (Method != NULL)
        {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(Queue, Request);
            CallbackEnd(irql);
        }
    }
};

class FxIoQueueIoCanceledOnQueue : public FxLockedCallback {

public:
    PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE Method;

    FxIoQueueIoCanceledOnQueue(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    void
    Invoke(
        __in WDFQUEUE   Queue,
        __in WDFREQUEST Request
        )
    {
        if (Method != NULL)
        {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(Queue, Request);
            CallbackEnd(irql);
        }
    }
};

//
// EvtIoRead callback delegate
//
class FxIoQueueIoRead : public FxLockedCallback {

public:
    PFN_WDF_IO_QUEUE_IO_READ Method;

    FxIoQueueIoRead(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    void
    Invoke(
        __in WDFQUEUE   Queue,
        __in WDFREQUEST Request,
        __in ULONG      Length
        )
    {
        if (Method != NULL)
        {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(Queue, Request, Length);
            CallbackEnd(irql);
        }
    }
};

//
// EvtIoWrite callback delegate
//
class FxIoQueueIoWrite : public FxLockedCallback {

public:
    PFN_WDF_IO_QUEUE_IO_WRITE Method;

    FxIoQueueIoWrite(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    void
    Invoke(
        __in WDFQUEUE   Queue,
        __in WDFREQUEST Request,
        __in ULONG      Length
        )
    {
        if (Method != NULL)
        {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(Queue, Request, Length);
            CallbackEnd(irql);
        }
    }
};

//
// EvtIoIoctl callback delegate
//
class FxIoQueueIoDeviceControl : public FxLockedCallback {

public:
    PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL Method;

    FxIoQueueIoDeviceControl(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    void
    Invoke(
        __in WDFQUEUE   Queue,
        __in WDFREQUEST Request,
        __in ULONG      OutputBufferLength,
        __in ULONG      InputBufferLength,
        __in ULONG      IoControlCode
        )
    {
        if (Method != NULL)
        {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(
                Queue,
                Request,
                OutputBufferLength,
                InputBufferLength,
                IoControlCode
                );
            CallbackEnd(irql);
        }
    }
};

//
// EvtIoInternalIoctl callback delegate
//
class FxIoQueueIoInternalDeviceControl : public FxLockedCallback {

public:
    PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL Method;

    FxIoQueueIoInternalDeviceControl(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    void
    Invoke(
        __in WDFQUEUE   Queue,
        __in WDFREQUEST Request,
        __in ULONG      OutputBufferLength,
        __in ULONG      InputBufferLength,
        __in ULONG      IoInternalControlCode
        )
    {
        if (Method != NULL)
        {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(
                Queue,
                Request,
                OutputBufferLength,
                InputBufferLength,
                IoInternalControlCode
                );
            CallbackEnd(irql);
        }
    }
};

//
// EvtIoDefault callback delegate
//
class FxIoQueueIoDefault : public FxLockedCallback {

public:
    PFN_WDF_IO_QUEUE_IO_DEFAULT Method;

    FxIoQueueIoDefault(
        VOID
        ) :
        FxLockedCallback()
    {
        Method = NULL;
    }

    void
    Invoke(
        __in WDFQUEUE                Queue,
        __in WDFREQUEST              Request
        )
    {
        if (Method != NULL)
        {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(Queue, Request);
            CallbackEnd(irql);
        }
    }
};

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
        if (m_Cancel != NULL)
        {
            PFN_WDF_REQUEST_CANCEL pMethod;
            KIRQL irql;

            //
            // Satisfy W4 warning, even though it is technically not necessary to
            // assign an initial value.
            //
            irql = PASSIVE_LEVEL;

            if (Lock != NULL)
            {
                Lock->Lock(&irql);
            }

            //
            // Clear the value before invoking the routine since the assignment
            // is invalidated when the routine is run.
            //
            pMethod = m_Cancel;
            m_Cancel = NULL;

            pMethod(Request);

            if (Lock != NULL)
            {
                Lock->Unlock(irql);
            }
        }
    }
};

class FxIoQueueForwardProgressExamineIrp : public FxCallback {
public:
    PFN_WDF_IO_WDM_IRP_FOR_FORWARD_PROGRESS Method;

    FxIoQueueForwardProgressExamineIrp(
        VOID
        )   :
        FxCallback()
    {   
        Method = NULL;
    }

    WDF_IO_FORWARD_PROGRESS_ACTION
    Invoke(
        __in WDFQUEUE   Queue,
        __in PIRP       Irp
        )
    {
        ASSERT(Method != NULL);
        return Method(Queue, Irp);
    }
};

#endif //_FXIOQUEUECALLBACKS_H_

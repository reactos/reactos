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

#ifndef _FXIOQUEUECALLBACKS_H_
#define _FXIOQUEUECALLBACKS_H_

//
// These delegates are in a seperate file since there are many
//

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
        if (Method != NULL) {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(Queue, Request);
            CallbackEnd(irql);
        }
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
        if (Method != NULL) {
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
        if (Method != NULL) {
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
        if (Method != NULL) {
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
        if (Method != NULL) {
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
        if (Method != NULL) {
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
        if (Method != NULL) {
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
        if (Method != NULL) {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(Queue, Context);
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
        if (Method != NULL) {
            KIRQL irql = 0;

            CallbackStart(&irql);
            Method(Queue, Request);
            CallbackEnd(irql);
        }
    }
};


class FxIoQueueForwardProgressAllocateResourcesReserved : public FxCallback {

public:
    PFN_WDF_IO_ALLOCATE_RESOURCES_FOR_RESERVED_REQUEST  Method;

    FxIoQueueForwardProgressAllocateResourcesReserved(
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



#endif // _FXIOQUEUECALLBACKS_H_

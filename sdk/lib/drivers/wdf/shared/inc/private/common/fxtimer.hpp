/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTimer.hpp

Abstract:

    This module implements a frameworks managed TIMER that
    can synchrononize with driver frameworks object locks.

Author:




Environment:

    Both kernel and user mode

Revision History:


--*/

#ifndef _FXTIMER_H_
#define _FXTIMER_H_

//
// Driver Frameworks TIMER Design:
//
// The driver frameworks provides an optional TIMER wrapper object that allows
// a reference counted TIMER object to be created that can synchronize
// automatically with certain frameworks objects.
//
// This provides automatic synchronization between the TIMER's execution, and the
// frameworks objects event callbacks into the device driver.
//

class FxTimer : public FxNonPagedObject {

private:

    //
    // Kernel Timer Object
    //
    MxTimer             m_Timer;

    //
    // This is the Framework object who is associated with the TIMER
    // if supplied
    //
    FxObject*           m_Object;

    //
    // This is the supplied Period to WdfTimerCreate
    //
    ULONG               m_Period;

    //
    // Optional tolerance for the timer in milliseconds
    //
    ULONG               m_TolerableDelay;

    //
    // This indicates whether a high resolution attribute is set
    // for the timer
    //
    BOOLEAN             m_UseHighResolutionTimer;

    //
    // This is the callback lock for the object this TIMER will
    // synchronize with
    //
    FxCallbackLock*     m_CallbackLock;

    //
    // This is the object whose reference count actually controls
    // the lifetime of the m_CallbackLock
    //
    FxObject*           m_CallbackLockObject;

    //
    // This is the user supplied callback function
    //
    PFN_WDF_TIMER       m_Callback;

    //
    // This workitem object will be used to queue workitem from the timer
    // dpc callback if the caller requests passive callback.
    //
    FxSystemWorkItem*   m_SystemWorkItem;

    //
    // This is a pointer to thread object that invoked our dpc callback
    // callback. This value will be used to avoid deadlock when we try
    // to stop or delete the timer.
    //
    volatile POINTER_ALIGNMENT MxThread   m_CallbackThread;

    //
    // This is a pointer to the Stop thread object when driver invokes the
    // timer's WdfTimerStop function.
    //
    MxThread            m_StopThread;

    //
    // TRUE if the timer was restarted while stopping.
    //
    BOOLEAN             m_StopAgain;

    //
    // TRUE if a start operation was aborted b/c stop was in progress.
    //
    BOOLEAN             m_StartAborted;

    //
    // Ensures only one of either Delete or Cleanup runsdown the object
    //
    BOOLEAN             m_RunningDown;

public:
    static
    _Must_inspect_result_
    NTSTATUS
    _Create(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PWDF_TIMER_CONFIG Config,
        __in PWDF_OBJECT_ATTRIBUTES Attributes,
        __in FxObject* ParentObject,
        __out WDFTIMER* Timer
        );

    FxTimer(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    virtual
    ~FxTimer(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PWDF_OBJECT_ATTRIBUTES Attributes,
        __in PWDF_TIMER_CONFIG Config,
        __in FxObject* ParentObject,
        __out WDFTIMER* Timer
        );

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    BOOLEAN
    Start(
        __in LARGE_INTEGER DueTime
        );

    BOOLEAN
    Stop(
        __in BOOLEAN  Wait
        );

    VOID
    TimerHandler(
        VOID
        );

    WDFOBJECT
    GetObject(
        VOID
        )
    {
        if( m_Object != NULL ) {
            return m_Object->GetObjectHandle();
        }
        else {
            return NULL;
        }
    }

    WDFTIMER
    GetHandle(
        VOID
        )
    {
        return (WDFTIMER) GetObjectHandle();
    }

private:

    //
    // Called from Dispose, or cleanup list to perform final flushing of any
    // outstanding DPC's and dereferencing of objects.
    //
    VOID
    FlushAndRundown(
        VOID
        );

    static
    FN_WDF_SYSTEMWORKITEM
    _FxTimerWorkItemCallback;

    static
    MdDeferredRoutineType
    _FxTimerDpcThunk;

    static
    MdExtCallbackType
    _FxTimerExtCallbackThunk;
};

#endif // _FXTIMER_H


/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxTimerUm.h

Abstract:

    User mode implementation of timer defined in
    MxTimer.h

Author:


Revision History:



--*/

#pragma once

typedef struct _MdTimer {
    //
    // Callback function to be invoked upon timer expiration and the context to
    // be passed in to the callback function
    //
    MdDeferredRoutine m_TimerCallback;
    PVOID m_TimerContext;

    //
    // The timer period
    //
    LONG m_Period;

    //
    // Handle to the timer object
    //
    PTP_TIMER m_TimerHandle;

    //
    // Work object to be executed by threadpool upon timer expiration
    //
    PTP_WORK m_WorkObject;

    //
    // Flag to indicate that the timer callback has started running
    //
    BOOL m_CallbackStartedRunning;

    //
    // Flag to indicate that the timer was started
    // since it was created or since it was last stopped.
    //
    BOOL m_TimerWasStarted;

    _MdTimer(
        VOID
        )
    {
        m_TimerHandle = NULL;
        m_WorkObject = NULL;
        m_TimerCallback = NULL;
        m_TimerContext = NULL;
        m_Period = 0;
        m_CallbackStartedRunning = FALSE;
        m_TimerWasStarted = FALSE;
    }

    ~_MdTimer(
        VOID
        )
    {
        //
        // Release the timer object
        //
        if (m_TimerHandle)
        {
            CloseThreadpoolTimer(m_TimerHandle);
            m_TimerHandle = NULL;
        }

        //
        // Release the work object
        //
        if (m_WorkObject)
        {
            CloseThreadpoolWork(m_WorkObject);
            m_WorkObject = NULL;
        }
    }

    BOOLEAN
    IsInSystemQueue(
        VOID
        )
    {
        //
        // Timer was not started since it was created or since
        // it was last stopped, so it can't be in the system timer queue.
        //
        if (!m_TimerWasStarted) {
            return FALSE;
        }

        //
        // Periodic timers are always in the system timer queue.
        //
        if (m_Period != 0) {
            return TRUE;
        }

        //
        // Non-periodic timer:
        //
        // At this point, the timer callback function has either been canceled or
        // has finished running. Examine the m_CallbackStartedRunning value to see
        // which one of these happened.
        //
        if (m_CallbackStartedRunning)
        {
            //
            // Timer cancellation was too late. Timer callback already started
            // running and the timer was removed from the system timer queue.
            //
            return FALSE;
        }
        else
        {
            //
            // Timer cancellation happened on time and prevented the timer callback
            // from running.
            //
            return TRUE;
        }
    }

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in_opt PVOID TimerContext,
        __in MdDeferredRoutine TimerCallback,
        __in LONG Period
        )
    {
        NTSTATUS ntStatus = STATUS_SUCCESS;

        m_TimerCallback = TimerCallback;
        m_TimerContext = TimerContext;
        m_Period = Period;

        //
        // Create the timer object
        //
        m_TimerHandle = CreateThreadpoolTimer(_MdTimer::s_MdTimerCallback,
                                              this,
                                              NULL);
        if (NULL == m_TimerHandle)
        {
            ntStatus = WinErrorToNtStatus(GetLastError());
        }

        //
        // Create the work object
        //
        if (NT_SUCCESS(ntStatus))
        {
            m_WorkObject = CreateThreadpoolWork(_MdTimer::s_MdWorkCallback,
                                                this,
                                                NULL);
            if (NULL == m_WorkObject)
            {
                ntStatus = WinErrorToNtStatus(GetLastError());
            }
        }

        return ntStatus;
    }

    BOOLEAN
    Start(
        __in LARGE_INTEGER DueTime,
        __in ULONG TolerableDelay
        )
    {
        BOOLEAN bRetVal;
        FILETIME dueFileTime;

        if (m_TimerWasStarted) {
            //
            // Cancel the previously pended timer callback,
            // we want it to execute after a full period elapsed.
            //
            WaitForThreadpoolTimerCallbacks(m_TimerHandle, TRUE);
        }

        //
        // Return TRUE if the timer is in the system timer queue.
        //
        bRetVal = IsInSystemQueue();

        //
        // This is a fresh start for the timer, so clear the flag that the
        // timer callback function may have previously set.
        //
        m_CallbackStartedRunning = FALSE;

        //
        // Set the timer started flag.
        //
        m_TimerWasStarted = TRUE;

        //
        // Copy the due time into a FILETIME structure
        //
        dueFileTime.dwLowDateTime = DueTime.LowPart;
        dueFileTime.dwHighDateTime = (DWORD) DueTime.HighPart;

        //
        // Start the timer
        //
        SetThreadpoolTimer(m_TimerHandle,
                           &dueFileTime,
                           (DWORD) m_Period,
                           TolerableDelay);

        return bRetVal;
    }

    _Must_inspect_result_
    BOOLEAN
    Stop(
        VOID
        )
    {
        BOOLEAN bRetVal;

        bRetVal = IsInSystemQueue();

        //
        // Stop the timer
        //
        SetThreadpoolTimer(m_TimerHandle,
                           NULL, // pftDueTime
                           0, // msPeriod
                           0 // msWindowLength
                           );

        //
        // Cancel pending callbacks that have not yet started to execute and wait
        // for outstanding callbacks to complete.
        //
        WaitForThreadpoolTimerCallbacks(m_TimerHandle,
                                        TRUE // cancel pending callbacks
                                        );

        //
        // Reset the timer started flag.
        //
        m_TimerWasStarted = FALSE;

        return bRetVal;
    }

    VOID
    TimerCallback(
        VOID
        )
    {
        //
        // Invoke the user's callback function
        //
        m_TimerCallback(NULL, /* Reserved1 */
                        m_TimerContext,
                        NULL, /* Reserved2 */
                        NULL /* Reserved3 */
                        );

        return;
    }

    static
    VOID CALLBACK
    s_MdWorkCallback(
        __inout PTP_CALLBACK_INSTANCE Instance,
        __inout_opt PVOID Context,
        __inout PTP_WORK Work
        )
    {
        struct _MdTimer *pThis = NULL;

        UNREFERENCED_PARAMETER(Instance);
        UNREFERENCED_PARAMETER(Work);

        pThis = (struct _MdTimer*) Context;
        pThis->TimerCallback();

        return;
    }

    static
    VOID CALLBACK
    s_MdTimerCallback(
        __inout PTP_CALLBACK_INSTANCE Instance,
        __inout_opt PVOID Context,
        __inout PTP_TIMER Timer
        )
    {
        struct _MdTimer *pThis = NULL;

        UNREFERENCED_PARAMETER(Instance);
        UNREFERENCED_PARAMETER(Timer);

        pThis = (struct _MdTimer*) Context;

        //
        // First, indicate that the callback has started running
        //
        pThis->m_CallbackStartedRunning = TRUE;

        //
        // Post a work object to execute the callback function supplied by the
        // user of MxTimer.
        //
        // We do not execute the user-supplied callback here because we could
        // run into a deadlock if the user is trying to cancel the timer by
        // calling MxTimer::Stop. MxTimer::Stop actually blocks waiting for
        // MdTimer::s_MdTimerCallback to finish executing, so that it can know
        // where its attempt to cancel the timer was successful. If we were to
        // execute the user's callback in MdTimer::s_MdTimerCallback, the user
        // would have to be careful not to call MxTimer::Stop while holding a
        // lock that the user's callback tries to acquire. In order to avoid
        // imposing such a restriction on the user, we allow
        // MdTimer::s_MdTimerCallback to return immediately after posting a
        // work object to run the user's callback.
        //
        SubmitThreadpoolWork(pThis->m_WorkObject);

        return;
    }

    BOOLEAN
    StartWithReturn(
        __in LARGE_INTEGER DueTime,
        __in ULONG TolerableDelay
        )
    {
        return Start(DueTime, TolerableDelay);
    }
} MdTimer;

#include "MxTimer.h"

//
// Implementation of MxTimer functions
//
MxTimer::MxTimer(
    VOID
    )
{
}

MxTimer::~MxTimer(
    VOID
    )
{
}

_Must_inspect_result_
NTSTATUS
MxTimer::Initialize(
    __in_opt PVOID TimerContext,
    __in MdDeferredRoutine TimerCallback,
    __in LONG Period
    )
/*++
Routine description:
    Initializes the MxTimer object.

Arguments:
    TimerContext - Context information that will be passed in to the timer
        callback function.

    TimerCallback - The timer callback function.

        *** IMPORTANT NOTE ***
        MxTimer object must not be freed inside the timer callback function
        because in the pre-Vista, user mode implementation of MxTimer, the
        destructor blocks waiting for all callback functions to finish
        executing. Hence freeing the MxTimer object inside the callback
        function will result in a deadlock.

    Period - The period of the timer in milliseconds.

Return value:
    An NTSTATUS value that indicates whether or not we succeeded in
    initializing the MxTimer
--*/
{
    NTSTATUS ntStatus;

    ntStatus = m_Timer.Initialize(TimerContext,
                                  TimerCallback,
                                  Period);

    return ntStatus;
}

_Must_inspect_result_
NTSTATUS
MxTimer::InitializeEx(
    __in_opt PVOID TimerContext,
    __in MdExtCallback TimerCallback,
    __in LONG Period,
    __in ULONG TolerableDelay,
    __in BOOLEAN UseHighResolutionTimer
    )
{

    UNREFERENCED_PARAMETER(TolerableDelay);
    UNREFERENCED_PARAMETER(UseHighResolutionTimer);
    UNREFERENCED_PARAMETER(TimerCallback);
    UNREFERENCED_PARAMETER(TimerContext);
    UNREFERENCED_PARAMETER(Period);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
    return STATUS_NOT_IMPLEMENTED;

}

VOID
MxTimer::Start(
    __in LARGE_INTEGER DueTime,
    __in ULONG TolerableDelay
    )
{
    m_Timer.Start(DueTime, TolerableDelay);

    return;
}

_Must_inspect_result_
BOOLEAN
MxTimer::Stop(
    VOID
    )
{
    BOOLEAN bRetVal;

    bRetVal = m_Timer.Stop();

    return bRetVal;
}

_Must_inspect_result_
BOOLEAN
MxTimer::StartWithReturn(
    __in LARGE_INTEGER DueTime,
    __in ULONG TolerableDelay
    )
{
    BOOLEAN bRetVal = TRUE;

    bRetVal = m_Timer.StartWithReturn(DueTime, TolerableDelay);

    return bRetVal;
}

VOID
MxTimer::FlushQueuedDpcs(
    VOID
    )
{
    WaitForThreadpoolWorkCallbacks(m_Timer.m_WorkObject,
                                   TRUE // cancel pending callbacks
                                   );
}


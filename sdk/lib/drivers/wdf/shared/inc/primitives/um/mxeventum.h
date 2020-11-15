/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxEventUm.h

Abstract:

    User mode implementation of event
    class defined in MxEvent.h

Author:



Revision History:



--*/

#pragma once

typedef struct {
    HANDLE Event;
#if DBG
    EVENT_TYPE Type; //tracked to allow ReadState only for notification events
#endif
} MdEvent;

#include "DbgMacros.h"
#include "MxEvent.h"

__inline
MxEvent::MxEvent()
{
    CLEAR_DBGFLAG_INITIALIZED;

    m_Event.Event = NULL;
}

__inline
MxEvent::~MxEvent()
{
    //
    // PLEASE NOTE: shared code must not rely of d'tor uninitializing the
    // event. d'tor may not be invoked if the event is used in a structure
    // which is allocated/deallocated using MxPoolAllocate/Free instead of
    // new/delete
    //
    Uninitialize();
}

_Must_inspect_result_
__inline
NTSTATUS
MxEvent::Initialize(
    __in EVENT_TYPE Type,
    __in BOOLEAN InitialState
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE event;
    BOOL bManualReset;

    if (NotificationEvent == Type)
    {
        bManualReset = TRUE;
    }
    else
    {
        bManualReset = FALSE;
    }

    event = CreateEvent(
        NULL,
        bManualReset,
        InitialState ? TRUE : FALSE,
        NULL
        );

    if (NULL == event) {
        DWORD err = GetLastError();
        status = WinErrorToNtStatus(err);
        goto exit;
    }

    m_Event.Event = event;

#if DBG
    m_Event.Type = Type;
#endif

    SET_DBGFLAG_INITIALIZED;

exit:
    return status;
}

__inline
PVOID
MxEvent::GetEvent(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    return m_Event.Event;
}

__inline
VOID
MxEvent::Set(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    SetEvent(m_Event.Event);
}

__inline
VOID
MxEvent::SetWithIncrement(
    __in KPRIORITY Priority
    )
{
    UNREFERENCED_PARAMETER(Priority);

    ASSERT_DBGFLAG_INITIALIZED;

    Set();
}

__inline
VOID
MxEvent::Clear(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    ResetEvent(m_Event.Event);
}

__drv_when(Timeout != NULL, _Must_inspect_result_)
__inline
NTSTATUS
MxEvent::WaitFor(
    __in     KWAIT_REASON  WaitReason,
    __in     KPROCESSOR_MODE  WaitMode,
    __in     BOOLEAN  Alertable,
    __in_opt PLARGE_INTEGER  Timeout
    )
/*++

Routine Description:
    Waits for the event

Arguments:
    WaitReason  - Unused (only there to match km definition)

    WaitMode    - Unuses (only there to match km definition)

    Altertable  - Whether the wait is alertable

    Timout      - Timeout in 100 ns units, MUST BE NEGATIVE
                  (negative implies relative timeout)

Return Value:
    Status corresponding to return value of WaitForSingleObjectEx

  --*/
{
    ASSERT_DBGFLAG_INITIALIZED;

    DWORD retVal;

    UNREFERENCED_PARAMETER(WaitReason);
    UNREFERENCED_PARAMETER(WaitMode);

    LONGLONG relativeTimeOut = 0;
    LONGLONG timeoutInMs = 0;
    DWORD dwTimeout = 0;

    if (NULL != Timeout)
    {
        //
        // Make sure that timeout is 0 or -ve (which implies relative timeout)
        //
        if (Timeout->QuadPart > 0)
        {
            Mx::MxAssertMsg(
                "Absolute wait not supported in user mode",
                FALSE
                );

            return STATUS_INVALID_PARAMETER;
        }

        //
        // Remove the -ve sign
        //
        if (Timeout->QuadPart < 0)
        {
            relativeTimeOut = -1 * Timeout->QuadPart;
        }

        //
        // Convert from 100ns units to milliseconds
        //
        timeoutInMs = (relativeTimeOut / (10 * 1000));

        if (timeoutInMs > ULONG_MAX)
        {
            Mx::MxAssertMsg("Timeout too large", FALSE);

            return STATUS_INVALID_PARAMETER;
        }
        else
        {
            dwTimeout = (DWORD) timeoutInMs;
        }
    }

    retVal = WaitForSingleObjectEx(
                    m_Event.Event,
                    (NULL == Timeout) ? INFINITE : dwTimeout,
                    Alertable
                    );

    switch(retVal)
    {
        case WAIT_ABANDONED:
            return STATUS_ABANDONED;
        case WAIT_OBJECT_0:
            return STATUS_SUCCESS;
        case WAIT_TIMEOUT:
            return STATUS_TIMEOUT;
        case WAIT_FAILED:
        {
            DWORD err = GetLastError();
            return WinErrorToNtStatus(err);
        }
        default:
        {
            //
            // We shoudn't get here
            //
            Mx::MxAssert(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
    }
}

LONG
__inline
MxEvent::ReadState(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

#if DBG
    Mx::MxAssert(m_Event.Type == NotificationEvent);
#endif

    if (WAIT_OBJECT_0 == WaitForSingleObject(
                m_Event.Event,
                0
                )) {
        return 1;
    }
    else {
        return 0;
    }
}


__inline
VOID
MxEvent::Uninitialize(
    )
{
    if (NULL != m_Event.Event) {
        CloseHandle(m_Event.Event);
        m_Event.Event = NULL;
    }

    CLEAR_DBGFLAG_INITIALIZED;
}

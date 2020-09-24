/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxEvent.h

Abstract:

    Kernel mode implementation of event
    class defined in MxEvent.h

Author:



Revision History:



--*/

#pragma once

typedef KEVENT MdEvent;

#include "MxEvent.h"

__inline
MxEvent::MxEvent()
{
    //
    // Make sure that m_Event is the first member. That way if someone passes
    // address of MxEvent to Ke*Event functions, it would still work.
    //
    // If this statement causes compilation failure, check if you added a field
    // before m_Event.
    //
    C_ASSERT(FIELD_OFFSET(MxEvent, m_Event) == 0);

    CLEAR_DBGFLAG_INITIALIZED;
}

__inline
MxEvent::~MxEvent()
{
}

__inline
NTSTATUS
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "_Must_inspect_result_ not needed in kernel mode as the function always succeeds");
MxEvent::Initialize(
    __in EVENT_TYPE Type,
    __in BOOLEAN InitialState
    )
{
    KeInitializeEvent(&m_Event, Type, InitialState);

    SET_DBGFLAG_INITIALIZED;

    return STATUS_SUCCESS;
}

__inline
PVOID
MxEvent::GetEvent(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    return &m_Event;
}


__inline
VOID
MxEvent::SetWithIncrement(
    __in KPRIORITY Priority
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    KeSetEvent(&m_Event, Priority, FALSE);
}

__inline
VOID
MxEvent::Set(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    KeSetEvent(&m_Event, IO_NO_INCREMENT, FALSE);
}


__inline
VOID
MxEvent::Clear(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    KeClearEvent(&m_Event);
}

__drv_when(Timeout == NULL && Alertable == FALSE, __drv_valueIs(==0))
__drv_when(Timeout != NULL && Alertable == FALSE, __drv_valueIs(==0;==258))
__drv_when(Timeout != NULL || Alertable == TRUE, _Must_inspect_result_)
__inline
NTSTATUS
MxEvent::WaitFor(
    __in     KWAIT_REASON  WaitReason,
    __in     KPROCESSOR_MODE  WaitMode,
    __in     BOOLEAN  Alertable,
    __in_opt PLARGE_INTEGER  Timeout
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    return KeWaitForSingleObject(
                    &m_Event,
                    WaitReason,
                    WaitMode,
                    Alertable,
                    Timeout
                    );
}

LONG
__inline
MxEvent::ReadState(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    return KeReadStateEvent(&m_Event);
}


__inline
VOID
MxEvent::Uninitialize(
    )
{
    CLEAR_DBGFLAG_INITIALIZED;
}

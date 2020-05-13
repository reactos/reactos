#ifndef _MXEVENT_H_
#define _MXEVENT_H_

#include <ntddk.h>
#include "dbgmacros.h"

typedef KEVENT MdEvent;

class MxEvent {

private:
    //
    // MdEvent is typedef'ed to appropriate type for the mode
    // in the mode specific file
    //
    MdEvent m_Event;

    DECLARE_DBGFLAG_INITIALIZED;

public:

    __inline
    MxEvent(
        )
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
    ~MxEvent(
        )
    {
    }

    __inline
    NTSTATUS
    #ifndef __GNUC__
    #pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "_Must_inspect_result_ not needed in kernel mode as the function always succeeds");
    #endif
    Initialize(
    __in EVENT_TYPE Type,
    __in BOOLEAN InitialState
        ) 
    {
        KeInitializeEvent(&m_Event, Type, InitialState);

        SET_DBGFLAG_INITIALIZED;

        return STATUS_SUCCESS;
    }

    __inline
    VOID
    Clear(
        )
    {
        ASSERT_DBGFLAG_INITIALIZED;

        KeClearEvent(&m_Event);
    }

    __inline
    VOID
    Set(
        )
    {
        ASSERT_DBGFLAG_INITIALIZED;

        KeSetEvent(&m_Event, IO_NO_INCREMENT, FALSE);
    }

    __inline
    VOID
    SetWithIncrement(
        __in KPRIORITY Priority
        )
    {
        ASSERT_DBGFLAG_INITIALIZED;

        KeSetEvent(&m_Event, Priority, FALSE);
    }

    MxEvent*
    GetSelfPointer(
        VOID
        )
    {
        //
        // Since operator& is hidden, we still need to be able to get a pointer
        // to this object, so we must make it an explicit method.
        //
        return this;
    }

    __drv_when(Timeout == NULL && Alertable == FALSE, __drv_valueIs(==0))
    __drv_when(Timeout != NULL && Alertable == FALSE, __drv_valueIs(==0;==258))
    __drv_when(Timeout != NULL || Alertable == TRUE, _Must_inspect_result_)
    __inline
    NTSTATUS
    WaitFor(
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

    //
    // GetEvent will return the underlying primitive event
    // PKEVENT in case of kernel mode and event HANDLE in case of user-mode
    //
    __inline
    PVOID
    GetEvent(
        )
    {
        ASSERT_DBGFLAG_INITIALIZED;

        return &m_Event;
    }

    //
    // NOTE: In user mode this function can only be called
    // for a notification event (and not for a
    // synchronization event)
    //
    __inline
    LONG
    ReadState(
        )
    {
        ASSERT_DBGFLAG_INITIALIZED;

        return KeReadStateEvent(&m_Event);
    }
};

#endif //_MXEVENT_H_

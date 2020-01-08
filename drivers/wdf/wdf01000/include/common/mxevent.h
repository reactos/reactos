#ifndef _MXEVENT_H_
#define _MXEVENT_H_

#include <ntddk.h>
#include "common/dbgmacros.h"

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
    #pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "_Must_inspect_result_ not needed in kernel mode as the function always succeeds");
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
};

#endif //_MXEVENT_H_

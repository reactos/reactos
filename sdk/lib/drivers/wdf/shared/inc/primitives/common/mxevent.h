/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxEvent.h

Abstract:

    Mode agnostic definition of event

    See MxEventKm.h and MxEventUm.h for mode
    specific implementations

Author:



Revision History:



--*/

#pragma once

class MxEvent
{

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
        );

    __inline
    ~MxEvent(
        );

    CHECK_RETURN_IF_USER_MODE
    __inline
    NTSTATUS
    Initialize(
        __in EVENT_TYPE Type,
        __in BOOLEAN InitialState
        );

    //
    // GetEvent will return the underlying primitive event
    // PKEVENT in case of kernel mode and event HANDLE in case of user-mode
    //
    __inline
    PVOID
    GetEvent(
        );

    __inline
    VOID
    Set(
        );

    __inline
    VOID
    SetWithIncrement(
        __in KPRIORITY Priority
        );

    __inline
    VOID
    Clear(
        );

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
        );

    //
    // NOTE: In user mode this function can only be called
    // for a notification event (and not for a
    // synchronization event)
    //
    __inline
    LONG
    ReadState(
        );

    __inline
    VOID
    Uninitialize(
        );

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

private:
    MxEvent* operator&(
        VOID
        )
    {
        //
        // By making the address of operator private, we make it harder
        // to accidentally use this object in an improper fashion, ie
        // something like this is prevented:
        //
        // MxEvent event;
        // KeWaitForSingleObject(&event, ...);
        //
        // However please note that in some cases event pointer is needed when
        // event is passed around, in which case, GetSelfPointer() is used and
        // this protection is circumvented. Calling code should be careful
        // not to pass the pointer to KeWaitForSingleObject in such cases.
        //
        ASSERT(FALSE);
        return NULL;
    }

};

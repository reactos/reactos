#ifndef _FXWAITLOCK_H_
#define _FXWAITLOCK_H_

#include "common/mxevent.h"


struct FxCREvent {

    FxCREvent(
        __in BOOLEAN InitialState = FALSE
        )
    {
    //
    // For kernel mode letting c'tor do the initialization to not churn the
    // non-shared code
    //
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
        m_Event.Initialize(SynchronizationEvent, InitialState);
#else
        UNREFERENCED_PARAMETER(InitialState);
#endif
    }

    FxCREvent(
        __in EVENT_TYPE Type,
        __in BOOLEAN InitialState
        )
    {
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
        m_Event.Initialize(Type, InitialState);
#else
        UNREFERENCED_PARAMETER(Type);
        UNREFERENCED_PARAMETER(InitialState);
#endif
    }

    VOID
    Clear(
        VOID
        )
    {
        m_Event.Clear();
    }

    VOID
    Set(
        VOID
        )
    {
        m_Event.Set();
    }

private:
    MxEvent m_Event;
};

#endif // _FXWAITLOCK_H_

/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxInterruptThreadpoolUm.hpp

Abstract:

    This file contains the class definition for the threadpool for interrupt object.

Author:



Environment:

    User mode only

Revision History:



--*/
#pragma once

#define MINIMUM_THREAD_COUNT_DEFAULT   (1)
class FxInterrupt;

class FxInterruptThreadpool  : FxGlobalsStump
{

private:

    //
    // Pointer to structure representing thread pool
    //
    PTP_POOL m_Pool;

    //
    // Structure representing callback environment for thread pool
    //
    TP_CALLBACK_ENVIRON m_CallbackEnvironment;

    //
    // Minimum thread pool thread count
    //
    ULONG m_MinimumThreadCount;


    HRESULT
    Initialize(
        );

public:

    FxInterruptThreadpool(
        PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    ~FxInterruptThreadpool();

    static
    HRESULT
    _CreateAndInit(
        _In_ PFX_DRIVER_GLOBALS DriverGlobals,
        _Out_ FxInterruptThreadpool** ppThreadpool
        );

    PTP_CALLBACK_ENVIRON
    GetCallbackEnvironment(
        VOID
        )
    {
        return &m_CallbackEnvironment;
    }

    PTP_WAIT
    CreateThreadpoolWait(
        __in         PTP_WAIT_CALLBACK pfnwa,
        __inout_opt  PVOID Context
        )
    {
        return ::CreateThreadpoolWait(pfnwa, Context, &m_CallbackEnvironment);
    }

    HRESULT
    UpdateThreadPoolThreadLimits(
        _In_ ULONG InterruptCount
        );
};

class FxInterruptWaitblock : FxGlobalsStump
{

private:

    //
    // threadpool wait block
    //
    PTP_WAIT m_Wait;

    //
    // auto reset event
    //
    HANDLE m_Event;


    HRESULT
    Initialize(
        __in FxInterruptThreadpool* Threadpool,
        __in FxInterrupt* Interrupt,
        __in PTP_WAIT_CALLBACK WaitCallback
        );

public:

    static
    HRESULT
    _CreateAndInit(
        _In_ FxInterruptThreadpool* Threadpool,
        _In_ FxInterrupt* Interrupt,
        _In_ PTP_WAIT_CALLBACK WaitCallback,
        _Out_ FxInterruptWaitblock** Waitblock
        );

    FxInterruptWaitblock(
        PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxGlobalsStump(FxDriverGlobals),
        m_Wait(NULL),
        m_Event(NULL)
    {
    }

    ~FxInterruptWaitblock();

    VOID
    CloseThreadpoolWait(
        VOID
        )
    {
        ::CloseThreadpoolWait(m_Wait);
    }

    VOID
    SetThreadpoolWait(
        VOID
        )
    {
        //
        // associates event with wait block and queues it
        // to thread pool queue.
        //
        ::SetThreadpoolWait(m_Wait, m_Event, NULL);
    }

    VOID
    ClearThreadpoolWait(
        VOID
        )
    {
        //
        // Passing a NULL handle clears the wait
        //
        ::SetThreadpoolWait(m_Wait, NULL, NULL);
    }

    VOID
    WaitForOutstandingCallbackToComplete(
        VOID
        )
    {
        ::WaitForThreadpoolWaitCallbacks(m_Wait, FALSE);
    }

    HANDLE
    GetEventHandle(
        VOID
        )
    {
        return m_Event;
    }

    VOID
    ResetEvent(
        VOID
        )
    {
        ::ResetEvent(m_Event);
    }

};


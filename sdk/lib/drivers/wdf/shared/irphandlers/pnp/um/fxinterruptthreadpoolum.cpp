/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxInterruptThreadpoolUm.cpp

Abstract:

    Threadpool functions for interrupt handling

Author:




Environment:

    User mode only

Revision History:

--*/

#include "fxmin.hpp"
#include "FxInterruptThreadpoolUm.hpp"

extern "C" {
#include "FxInterruptThreadpoolUm.tmh"
}

#define STRSAFE_LIB
#include <strsafe.h>

FxInterruptThreadpool::FxInterruptThreadpool(
    PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxGlobalsStump(FxDriverGlobals),
    m_Pool(NULL),
    m_MinimumThreadCount(MINIMUM_THREAD_COUNT_DEFAULT)
{
    InitializeThreadpoolEnvironment(&m_CallbackEnvironment);
}

FxInterruptThreadpool::~FxInterruptThreadpool()
{
    //
    // close pool
    //
    if (m_Pool != NULL) {
        CloseThreadpool(m_Pool);
        m_Pool = NULL;
    }

    DestroyThreadpoolEnvironment(&m_CallbackEnvironment);
}

HRESULT
FxInterruptThreadpool::_CreateAndInit(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals,
    _Out_ FxInterruptThreadpool** ppThreadpool
    )
{
    HRESULT hr;
    FxInterruptThreadpool* pool = NULL;

    FX_VERIFY_WITH_NAME(INTERNAL, CHECK_NOT_NULL(ppThreadpool),
        DriverGlobals->Public.DriverName);

    *ppThreadpool = NULL;

    pool = new (DriverGlobals) FxInterruptThreadpool(DriverGlobals);
    if (pool == NULL)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        DoTraceLevelMessage(DriverGlobals,
                    TRACE_LEVEL_ERROR, TRACINGPNP,
                    "FxInterruptThreadpool creation failed, "
                    "%!hresult!", hr);
        return hr;
    }

    hr = pool->Initialize();

    if (SUCCEEDED(hr))
    {
        *ppThreadpool = pool;
    }
    else {
        delete pool;
    }

    return hr;
}

HRESULT
FxInterruptThreadpool::Initialize(
    )
{
    HRESULT hr = S_OK;
    DWORD error;
    BOOL bRet;

    //
    // Create a thread pool using win32 APIs
    //
    m_Pool = CreateThreadpool(NULL);
    if (m_Pool == NULL)
    {
        error = GetLastError();
        hr = HRESULT_FROM_WIN32(error);
        DoTraceLevelMessage(GetDriverGlobals(),
                    TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Threadpool creation failed, %!winerr!", error);
        return hr;
    }

    //
    // Set maximum thread count to equal the total number of interrupts.
    // Set minimum thread count (persistent threads) to equal the lower of
    // number of interrupt and number of processors.
    //
    // We want minimum number of persistent threads to be at least equal to the
    // number of interrupt objects so that there is no delay in servicing the
    // interrupt if there are processors available (the delay can come due to
    // thread pool delay in allocating and initializing a new thread). However,
    // there is not much benefit in having more persistent threads than
    // processors, as threads would have to wait for processor to become
    // available anyways. Therefore, we chose to have the number of persistent
    // equal to the Min(number of interrupts, number of processors).
    //
    // In the current design, if there are more interrupts than
    // processors, as soon as one thread finishes servicing the interrupt, both
    // the thread and processor will be available to service the next queued
    // interrupt. Note that thread pool will queue all the waits and will
    // satisfy them in a FIFO manner.
    //
    // Since we don't know the number of interrupts in the beginning, we will
    // start with one and update it when we know the actual number of interrupts
    // after processing driver's OnPrepareHardware callback.
    //
    // Note on interrupt servicing:
    // When fx connects the interrupt, it queues an event-based wait block
    // to thread pool for each interrupt, so that the interrupt can get serviced
    // using one of the thread pool threads when the auto-reset event shared
    // with reflector is set by reflector in the DpcForIsr routine queued by Isr.
    // While the interrupt is being serviced by one of the threads, no wait block
    // is queued to thread pool for that interrupt, so arrival of same interrupt
    // signals the event in DpcForIsr but doesn't cause new threads to pick up
    // the servicing. Note that other interrupts still have their wait blocks
    // queued so they will be serviced as they arrive.
    //
    // When previous servicing is over (i.e. the ISR callback has been
    // invoked and it has returned), Fx queues another wait block to thread pool
    // for that interrupt. If the event is already signalled, it would result in
    // the same thread (or another) picking up servicing immediately.
    //
    // This means the interrupt ISR routine can never run concurrently
    // for same interrupt. Therefore, there is no need to have more than one
    // thread for each interrupt.
    //
    SetThreadpoolThreadMaximum(m_Pool, 1);

    //
    // Create one persistent thread since atleast one interrupt is the most
    // likely scenario. We will update this number to have either the same
    // number of threads as there are interrupts, or the number of
    // processors on the machine.
    //
    bRet = SetThreadpoolThreadMinimum(m_Pool, m_MinimumThreadCount);
    if (bRet == FALSE) {
        error = GetLastError();
        hr = HRESULT_FROM_WIN32(error);
        DoTraceLevelMessage(GetDriverGlobals(),
                    TRACE_LEVEL_ERROR, TRACINGPNP,
                    "%!FUNC!: Failed to set minimum threads (%d) in threadpool,"
                    " %!winerr!", m_MinimumThreadCount, error);
        goto cleanup;
    }

    //
    // Associate thread pool with callback environment.
    //
    SetThreadpoolCallbackPool(&m_CallbackEnvironment, m_Pool);

cleanup:

    if (FAILED(hr)) {
        CloseThreadpool(m_Pool);
        m_Pool = NULL;
    }

    return hr;
}

HRESULT
FxInterruptThreadpool::UpdateThreadPoolThreadLimits(
    _In_ ULONG InterruptCount
    )
{
    BOOL bRet;
    HRESULT hr = S_OK;
    SYSTEM_INFO sysInfo;
    ULONG minThreadCount = 0;
    ULONG procs;
    DWORD error;

    FX_VERIFY(INTERNAL, CHECK_NOT_NULL(m_Pool));

    //
    // if there are more than one interrupts then we need to update minimum
    // thread count.
    //
    if (m_MinimumThreadCount >= InterruptCount) {
        //
        // nothing to do
        //
        return S_OK;
    }

    //
    // We want to have number of minimum persistent threads
    // = Min(number of interrupts, number of processors).
    // See comments in Initialize routine for details.
    //
    GetSystemInfo(&sysInfo);
    procs = sysInfo.dwNumberOfProcessors;

    minThreadCount = min(InterruptCount, procs);

    if (m_MinimumThreadCount < minThreadCount) {
        //
        // Set threadpool min
        //
        bRet = SetThreadpoolThreadMinimum(m_Pool, minThreadCount);
        if (bRet == FALSE) {
            error = GetLastError();
            hr = HRESULT_FROM_WIN32(error);
            DoTraceLevelMessage(GetDriverGlobals(),
                        TRACE_LEVEL_ERROR, TRACINGPNP,
                        "Failed to set minimum threads in threadpool,"
                        " TP_POOL 0x%p to %d %!winerr!", m_Pool, minThreadCount,
                        error);
            return hr;
        }

        m_MinimumThreadCount = minThreadCount;
    }

    //
    // set thread pool max to max number of interrupts
    //
    SetThreadpoolThreadMaximum(m_Pool, InterruptCount);

    DoTraceLevelMessage(GetDriverGlobals(),
                TRACE_LEVEL_ERROR, TRACINGPNP,
                "Threads in thread pool TP_POOL 0x%p updated"
                " to Max %d Min %d threads", m_Pool,
                InterruptCount, minThreadCount);

    return hr;
}

FxInterruptWaitblock::~FxInterruptWaitblock(
    VOID
    )
{
    //
    // close the thread pool wait structure
    //
    if (m_Wait) {
        //
        // Make sure no event is registered.
        //
        ClearThreadpoolWait();

        //
        // Wait for all the callbacks to finish.
        //
        WaitForOutstandingCallbackToComplete();

        //
        // close the wait
        //
        CloseThreadpoolWait();

        m_Wait = NULL;
    }

    //
    // close event handle
    //
    if (m_Event) {
        CloseHandle(m_Event);
        m_Event = NULL;
    }
}

HRESULT
FxInterruptWaitblock::_CreateAndInit(
    _In_ FxInterruptThreadpool* Threadpool,
    _In_ FxInterrupt* Interrupt,
    _In_ PTP_WAIT_CALLBACK WaitCallback,
    _Out_ FxInterruptWaitblock** Waitblock
    )
{
    HRESULT hr = S_OK;
    FxInterruptWaitblock* waitblock = NULL;
    PFX_DRIVER_GLOBALS driverGlobals;

    FX_VERIFY(INTERNAL, CHECK_NOT_NULL(Waitblock));
    *Waitblock = NULL;
    driverGlobals = Interrupt->GetDriverGlobals();

    //
    // create an instance of interrupt wait block
    //
    waitblock = new (driverGlobals) FxInterruptWaitblock(driverGlobals);
    if (waitblock == NULL) {
        hr = E_OUTOFMEMORY;
        DoTraceLevelMessage(driverGlobals,
                    TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Waitblock creation failed %!hresult!", hr);
        goto exit;
    }

    hr = waitblock->Initialize(Threadpool,
                               Interrupt,
                               WaitCallback);
    if (SUCCEEDED(hr)) {
        *Waitblock = waitblock;
    }
    else {
        DoTraceLevelMessage(driverGlobals,
                    TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Waitblock init failed %!hresult!", hr);
    }

exit:

    if(FAILED(hr) && waitblock != NULL) {
        delete waitblock;
    }

    return hr;
}

HRESULT
FxInterruptWaitblock::Initialize(
    __in FxInterruptThreadpool* Threadpool,
    __in FxInterrupt* Interrupt,
    __in PTP_WAIT_CALLBACK WaitCallback
    )
{
    HRESULT hr = S_OK;
    DWORD error;

    //
    // create a per-interrupt auto-reset event, non-signalled to begin with.
    //
    m_Event = CreateEvent(
                          NULL,  // LPSECURITY_ATTRIBUTES lpEventAttributes,
                          FALSE, // BOOL bManualReset,
                          FALSE, // BOOL bInitialState,
                          NULL   // LPCTSTR lpName
                          );

    if (m_Event == NULL) {
        error = GetLastError();
        hr = HRESULT_FROM_WIN32(error);
        DoTraceLevelMessage(GetDriverGlobals(),
                    TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Event creation failed for FxInterrupt object"
                    " %!winerr!", error);
        goto exit;
    }

    //
    // create a per-interrupt thread pool wait structure. This wait structure is
    // needed to associate an event-based wait callback with threadpool.
    //
    m_Wait = Threadpool->CreateThreadpoolWait(WaitCallback,
                                              Interrupt);
    if (m_Wait == NULL) {
        error = GetLastError();
        hr = HRESULT_FROM_WIN32(error);
        DoTraceLevelMessage(GetDriverGlobals(),
                    TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Event creation failed for FxInterrupt object"
                    " %!winerr!", error);
        goto exit;
    }

exit:

    return hr;
}



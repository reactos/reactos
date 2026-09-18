// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Definitions for the partition manager.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

#define TIMER_INTERVAL 10


MtDefine(CMediaControl, MILRender, "CMediaControl");

//
// This is the global partition manager.
//

CPartitionManager *g_pPartitionManager = NULL;

MtDefine(CPartitionManager, MILRender, "CPartitionManager");


#if ENABLE_PARTITION_MANAGER_LOG

//
// The partition manager log.
// 

DWORD g_PartitionManagerLog[1024] = { 0 };

volatile LONG g_PartitionManagerLogIndex = -1;


//+-----------------------------------------------------------------------------
//
//    Member:
//        CPartitionManager::LogEvent
// 
//    Synopsis:
//        Adds an entry to the partition manager's log
// 
//------------------------------------------------------------------------------

/* static */ 
void CPartitionManager::LogEvent(
    PartitionManagerEvent::FlagsEnum event,
    DWORD value // Masked with PartitionManagerEvent::Mask
    )
{
    LONG nCurrentIndex, nNextIndex;

    // Increment the current index in a thread-safe manner
    do
    {
        // Obtain the current index
        nCurrentIndex = g_PartitionManagerLogIndex;

        // Calculate the next index
        nNextIndex = (nCurrentIndex + 1) % ARRAY_SIZE(g_PartitionManagerLog);

        // Don't exchange unless g_PartitionManagerLogIndex is still nCurrentIndex
        //
        // Attempt an atomic assignment of the next index, until the current index 
        // hasn't changed since it was read
    } while (nCurrentIndex != InterlockedCompareExchange(
                                  &g_PartitionManagerLogIndex, 
                                  nNextIndex, 
                                  nCurrentIndex   
                                  ));

    g_PartitionManagerLog[nNextIndex] = 
        static_cast<DWORD>(event) | (value & PartitionManagerEvent::Mask);
}

#endif /* ENABLE_PARTITION_MANAGER_LOG */


//+-----------------------------------------------------------------------------
//
//    Member:
//        CPartitionManager constructor
// 
//------------------------------------------------------------------------------

CPartitionManager::CPartitionManager()
{
#if ENABLE_PARTITION_MANAGER_LOG
    LogEvent(PartitionManagerEvent::PartitionManagerCtor, static_cast<DWORD>(reinterpret_cast<UINT_PTR>(this)));
#endif /* ENABLE_PARTITION_MANAGER_LOG */

    InitializeListHead(&m_PartitionList);
    m_fShutdown = false;
    m_hevWork = NULL;
    m_hevBeat = NULL;
    m_cEvents = 0;
    m_nWorkerThreadPriority = THREAD_PRIORITY_ERROR_RETURN;
}


//+-----------------------------------------------------------------------
//
//  Member: CParititonMangager::~CPartitionManager 
//
//  Synopsis: Destructor
//
//------------------------------------------------------------------------

CPartitionManager::~CPartitionManager()
{
#if ENABLE_PARTITION_MANAGER_LOG
    LogEvent(PartitionManagerEvent::PartitionManagerDtor, static_cast<DWORD>(reinterpret_cast<UINT_PTR>(this)));
#endif /* ENABLE_PARTITION_MANAGER_LOG */

    ReleaseSchedulerResources();
}


//+-----------------------------------------------------------------------
//
//  Member: CPartitionManager::Create 
//
//  Synopsis:  Creates the global partition manager.  The supplied
//             handle is used to trigger the worker threads to wakeup
//             If no handle is supplied a waitable timer with a 
//             period of 10ms is used.
//
//  Returns: S_OK if succeeds
//
//------------------------------------------------------------------------
HRESULT CPartitionManager::Create(
    int nPriority,
    __deref_out_ecount(1) CPartitionManager **ppm
    )
{
    HRESULT hr = S_OK;
    CPartitionManager *pm = new CPartitionManager;
    IFCOOM(pm);
    
    IFC(pm->Initialize(nPriority));

    *ppm = pm;
    pm = NULL;
    
  Cleanup:
    delete pm;
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member: CPartitionManager::GetComposedEventId 
//
//  Synopsis:  Gets the id used to create the named event singled
//             after each compose pass
//
//------------------------------------------------------------------------
HRESULT CPartitionManager::GetComposedEventId(
    __out_ecount(1) UINT *pcEventId
    )
{
    HRESULT hr = S_OK;
    Lock();
    if (m_rgpThread.GetCount())
    {
        IFC(m_rgpThread[0]->GetComposedEventId(pcEventId));
    }
    else
    {
        IFC(WGXERR_NOTINITIALIZED);
    }
Cleanup:
    Unlock();
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CPartitionManager::Initialize
//
//    Synopsis:
//        Prepares the partition manager for first use.
//
//------------------------------------------------------------------------------

HRESULT 
CPartitionManager::Initialize(
    int nPriority
    )
{
    Assert(m_hevWork == NULL);
    HRESULT hr = S_OK;
    DWORD fEnableDebugControl = 0;
    HKEY hRegAvalonGraphics = NULL;

    g_pMediaControl = NULL;

    //
    // Check the registry key for enabling the control center.
    //

    if (SUCCEEDED(GetAvalonRegistrySettingsKey(
            &hRegAvalonGraphics, 
            FALSE /* HKEY LOCAL MACHINE */)))
    {
        RegReadDWORD(hRegAvalonGraphics,
            _T("EnableDebugControl"),
            &fEnableDebugControl);
    }

    if (fEnableDebugControl)
    {
#if DBG==1
        CSetDefaultMeter mtDefault(Mt(CMediaControl));
#endif
        WCHAR wszBuffer [64];
        DWORD pid = GetCurrentProcessId();

        //
        // *** ATTENTION    ATTENTION    ATTENTION    ATTENTION    ATTENTION ***
        //
        //  The next side-by-side release of this product needs to rename this
        //  again to prevent two different versions of this DLL from conflicting
        //  when creating the mapping!
        //
        //  This also needs updating in core\control\dll\exports.cs
        //
        // *** ATTENTION    ATTENTION    ATTENTION    ATTENTION    ATTENTION ***
        //
        
        IFC(StringCchPrintfW(wszBuffer, ARRAYSIZE(wszBuffer), L"wpfgfx_v0400-%d", pid));

        CPerformanceCounter::Initialize();

        IFC(CMediaControl::Create(
            wszBuffer,
            /*out*/ &g_pMediaControl
            ));
    }

    IFC(m_cs.Init());


    IFC(UpdateSchedulerSettings(nPriority));


Cleanup:
    if (hRegAvalonGraphics != NULL)
    {
        RegCloseKey(hRegAvalonGraphics);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CPartitionManager::ReleaseSchedulerResources
//
//    Synopsis:
//        Cleans up scheduler related resources (wait handles, etc.).
//
//------------------------------------------------------------------------------

void
CPartitionManager::ReleaseSchedulerResources()
{
    if (m_hevBeat)
    {
        CloseHandle(m_hevBeat);
        m_hevBeat = NULL;
    }
    
    if (m_hevWork)
    {   
        CloseHandle(m_hevWork);
        m_hevWork = NULL;
    }

    m_cEvents = 0;
}


//+-----------------------------------------------------------------------
//
//  Member:CPartitionManager::CreateWorkerThread 
//
//  Synopsis:  Creates a worker thread object and associated thread
//
//  Returns: S_OK if succeeds
//
//------------------------------------------------------------------------
HRESULT CPartitionManager::CreateWorkerThread(int nPriority)
{
    HRESULT hr = S_OK;
    CPartitionThread *pThread = NULL;
    Lock();

    if (!m_fShutdown)
    {
        pThread = new CPartitionThread(this, nPriority);
        IFCOOM(pThread);
            
        IFC(pThread->Initialize());

        IFC(pThread->StartThread());

        m_rgpThread.Add(pThread);
    }
  Cleanup:
    if (FAILED(hr))
    {
        delete pThread;
    }
    Unlock();
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:
//      CPartitionManager::ScheduleBatchProcessing
//
//  Synopsis:
//      Requires worker thread to execute composition pass
//      in order to accept command batch[es].
//      Executing of this request can follow ASAP or can be
//      deferred till next frame, depending on worker thread
//      implementation.
//
//      The scheduling operation is atomically accompanied with
//      call to pPartition->EnqueueBatch() that hooks up the batch
//      to partition batch queue.
//
//      Also atomically, partition state is first of all checked
//      against zombie state. If the partitions is zombified,
//      the work is not scheduled, EnqueueBatch() is not being called,
//      and "false" value is returned. Caller is responsible for
//      detecting this and freeing the batch if necessary.
//
//  Returns:
//      "true": all done;
//      "false": partition is in zombie state, nothing done.
//
//------------------------------------------------------------------------
bool
CPartitionManager::ScheduleBatchProcessing(
    __inout_ecount(1) Partition * pPartition,
    __inout_ecount(1) CMilCommandBatch *pBatch
    )
{
    bool fResult = true;

    Lock();

    //
    // In the zombie state we ignore the PartitionCommandBatch types.
    // We schedule all other command batches so we can properly manipulate the channel tables in the partitions.
    //

    if ((pPartition->IsZombie()) && (pBatch->m_commandType == PartitionCommandBatch))
    {
        fResult = false;
    }
    else
    {
        pPartition->EnqueueBatch(pBatch);

        SetPartitionState(pPartition, PartitionStateNull, PartitionNeedsBatchProcessing);

        //
        // Set the event to signalled state to awake worker thread.
        //
        SetEvent(m_hevWork);
    }

    Unlock();

    return fResult;
}


//+-----------------------------------------------------------------------
//
//  Member:
//      CPartitionManager::ScheduleCompositionPass
//
//  Synopsis:
//      Requires worker thread to execute throttled composition pass.
//
//     This method can be called during composition pass
//     thus requesting next one. Worker thread implementation
//     should guarantee not falling into forever loop
//     either by waiting for next VBlank or sleeping.
//     
//     The delay is not guaranteed however: ScheduleBatchProcessing()
//     might cause immediate composition pass; when it happens
//     this request is considered executed. Callers should be
//     prepared for it and supply more ScheduleCompositionPass calls
//     if necessary.
//------------------------------------------------------------------------
void
CPartitionManager::ScheduleCompositionPass(
    __inout_ecount(1) Partition * pPartition
    )
{
    SetPartitionState(pPartition, PartitionStateNull, PartitionNeedsCompositionPass);

    //
    // Set the event to signalled state to awake worker thread.
    //

    SetEvent(m_hevWork);
}


//+-----------------------------------------------------------------------
//
//  Member:
//      CPartitionManager::ScheduleRenderingPass
//
//  Synopsis:
//      Requires worker thread to execute composition pass.
//      This requiest does not assume any delay on worker thread.
//
//------------------------------------------------------------------------
void
CPartitionManager::ScheduleRenderingPass(
    __inout_ecount(1) Partition * pPartition
    )
{
    SetPartitionState(pPartition, PartitionStateNull, PartitionNeedsRender);

    //
    // Set the event to signalled state to awake worker thread.
    //
    SetEvent(m_hevWork);
}


//+-----------------------------------------------------------------------
//
//  Member:
//      CPartitionManager::SchedulePresentAndCompleteProcessing
//
//  Synopsis:
//      By this call the caller declares that the partition has been
//      rendered and needs presenting.
//      Ensure that the partition is included in managers attention list,
//      mark the state flags correspondingly.
//
//------------------------------------------------------------------------
void
CPartitionManager::SchedulePresentAndCompleteProcessing(
    __inout_ecount(1) Partition * pPartition
    )
{
    // This routine should be called from worker thread.
    Assert(CurrentThreadIsWorkerThread());

    Assert(!pPartition->IsZombie());

    SetPartitionState(pPartition, PartitionIsBeingProcessed, PartitionNeedsPresent);
}

//+-----------------------------------------------------------------------
//
//  Member:
//      CPartitionManager::CompleteProcessing 
//
//  Synopsis:
//      Completes processing of a partition and updates its state.
//      If there is no more work for this partition, it will be removed
//      from the list.
//
//------------------------------------------------------------------------
void
CPartitionManager::CompleteProcessing(
    __inout_ecount(1) Partition * pPartition
    )
{
    // This routine should be called from worker thread.
    Assert(CurrentThreadIsWorkerThread());

    SetPartitionState(pPartition, PartitionIsBeingProcessed, PartitionStateNull);
}


//+-----------------------------------------------------------------------
//
//  Member: 
//       CPartitionManager::ZombifyPartitionAndCompleteProcessing
//
//  Synopsis:  
//       Puts a partition into zombie state.
//
//------------------------------------------------------------------------
void
CPartitionManager::ZombifyPartitionAndCompleteProcessing(
    __in_ecount(1) Partition *pPartition,
    HRESULT hrFailureCode
    )
{
    // This routine should be called from worker thread.
    Assert(CurrentThreadIsWorkerThread());

    //
    // We only return OOM or OOVM back to through the back channel.
    // For all other failures, just return a generic render thread failure
    // failure since we don't want to give out the details
    // for security reasons.
    //
    if (hrFailureCode == D3DERR_OUTOFVIDEOMEMORY)
    {
        pPartition->m_hrZombieNotificationFailureReason = D3DERR_OUTOFVIDEOMEMORY;
    }
    else if (IsOOM(hrFailureCode))
    {
        pPartition->m_hrZombieNotificationFailureReason = E_OUTOFMEMORY;
    }
    else
    {
        //
        // Note: This failure is not as a result in a problem in this code, something
        //       happened in the render thread that resulted in us zombifying the 
        //       partition. Look at stack backtrace capture to determine the root
        //       cause of the failure.
        // 
        MilUnexpectedError(hrFailureCode, TEXT("The render thread failed unexpectedly."));

        pPartition->m_hrZombieNotificationFailureReason = WGXERR_UCE_RENDERTHREADFAILURE;
    }

    SetPartitionState(
        pPartition,
        PartitionZombifyClearFlags,
        PartitionZombifySetFlags
        );
}

//+-----------------------------------------------------------------------
//
//  Member:
//      CPartitionManager::SetPartitionState
//
//  Synopsis:
//      Private helper to change partition state.
//      Sets and/or resets required flags, then includes or excludes
//      the partition to/from partition list in correspondence with
//      partition state.
//
//------------------------------------------------------------------------
void
CPartitionManager::SetPartitionState(
    __inout_ecount(1) Partition * pPartition,
    PartitionState flagsToClear,
    PartitionState flagsToSet
    )
{
    Lock();

#if ENABLE_PARTITION_MANAGER_LOG
    pPartition->AddRef(); // keep the partition alive if dequeuing releases the last reference
#endif /* ENABLE_PARTITION_MANAGER_LOG */

    pPartition->ClearStateFlags(flagsToClear);
    pPartition->SetStateFlags(flagsToSet);

    if (pPartition->NeedsAttention())
    {
        if (!pPartition->IsEnqueued())
        {
            pPartition->AddRef();
            InsertTailList(&m_PartitionList, pPartition);
            pPartition->SetStateFlags(PartitionIsEnqueued);

#if ENABLE_PARTITION_MANAGER_LOG
            LogEvent(PartitionManagerEvent::EnqueuedPartition, static_cast<DWORD>(reinterpret_cast<UINT_PTR>(pPartition)));
#endif /* ENABLE_PARTITION_MANAGER_LOG */
        }
    }
    else
    {
        if (pPartition->IsEnqueued())
        {
            pPartition->ClearStateFlags(PartitionIsEnqueued);
            RemoveEntryList(pPartition);
            pPartition->Release();

#if ENABLE_PARTITION_MANAGER_LOG
            LogEvent(PartitionManagerEvent::DequeuedPartition, static_cast<DWORD>(reinterpret_cast<UINT_PTR>(pPartition)));
#endif /* ENABLE_PARTITION_MANAGER_LOG */
        }
    }

#if ENABLE_PARTITION_MANAGER_LOG
    LogEvent(PartitionManagerEvent::EffectiveFlags, static_cast<DWORD>(pPartition->m_state));
    pPartition->Release(); // release the reference acquired at the beginning of this method
#endif /* ENABLE_PARTITION_MANAGER_LOG */

    Unlock();
}


//+-----------------------------------------------------------------------
//
//  Member:
//      CPartitionManager::ActivateDeferredPartitions 
//
//  Synopsis:
//      Start processing new frame.
//      In other words, declare timeout passed so that partitions
//      marked with PartitionNeedsDeferredPass obtain
//      PartitionNeedsCompositionPass flag.
//
//------------------------------------------------------------------------
void
CPartitionManager::ActivateDeferredPartitions(
    PartitionState flags
    )
{
    Lock();
    
    for (LIST_ENTRY *p = m_PartitionList.Flink; p != &m_PartitionList; p = p->Flink)
    {
        Partition *pPartition = static_cast<Partition*>(p);

        if(pPartition->IsBeingProcessed())
            continue;

        if (pPartition->HasAnyFlag(flags))
        {
            pPartition->SetStateFlags(PartitionNeedsRender);
        }
    }

    Unlock();
}


//+-----------------------------------------------------------------------
//
//  Member:
//      CPartitionManager::HandleZombiePartition
//
//  Synopsis:
//      Attempts to perform pending zombie notifications
//
//------------------------------------------------------------------------

void
CPartitionManager::HandleZombiePartition(
    __in_ecount(1) Partition *pPartition
    )
{
    Assert(pPartition->NeedsZombieNotification());

    //
    // We will keep attempting to send zombie notifications until we succeed.
    // 
    // If this happens, we will remove the last flags hinting that the partition
    // needs partition manager's attention which will make SetPartitionState
    // release it and remove it from the partitions list.
    //

    PartitionState flagsToClear = SUCCEEDED(pPartition->NotifyPartitionIsZombie())
        ? PartitionNeedsAttention
        : PartitionIsBeingProcessed;

    SetPartitionState(
        pPartition,
        flagsToClear,
        PartitionStateNull // flagsToSet
        );
}


//
// Move everything from the pending list to the ready list. Mark everything
// moved as PartitionReady
//

VOID CPartitionManager::ReleasePartitions()
{
    Lock();
    
    while (!IsListEmpty(&m_PartitionList))
    {
        Partition *p = static_cast<Partition*>(RemoveHeadList(&m_PartitionList));
        p->Release();
    }

    Unlock();
}

#if DBG_ANALYSIS
//+-----------------------------------------------------------------------------
//
//    Member:
//        CPartitionManager::CurrentThreadIsWorkerThread
//
//    Synopsis:
//        Checks whether the method is called from worker thread.
//
//------------------------------------------------------------------------------
bool
CPartitionManager::CurrentThreadIsWorkerThread()
{
    DWORD dwTid = GetCurrentThreadId();
    bool fReply = false;

    Lock();
    for (UINT i = 0, n = m_rgpThread.GetCount(); i < n; i++)
    {
        const CPartitionThread *pThread = m_rgpThread[i];
        if (pThread && pThread->GetThreadId() == dwTid)
        {
            fReply = true;
            break;
        }
    }
    Unlock();

    return fReply;
}
#endif //DBG_ANALYSIS

//+-----------------------------------------------------------------------------
//
//    Member:
//        CPartitionManager::StopWorkerThreads
//
//    Synopsis:
//        Shuts down all the worker threads.
//
//------------------------------------------------------------------------------

void
CPartitionManager::StopWorkerThreads()
{
    HANDLE hWorkerThread = INVALID_HANDLE_VALUE;

    {
        //
        // Set the shutdown flag and determine if we have to wait for threads
        // to shut down. Do this under the critical section so that we don't
        // have threading problems with CreateWorkerThread().
        //
        
        Lock();
        
        //
        // Flag that we're shutting down.
        //
        
        m_fShutdown = true;
            
        //
        // Build the list of thread handles which will be used later to wait
        // for the threads to shut down. We need to do it here, because each 
        // thread will delete its entry before exiting.
        //
        // This logic currently supports at
        //  most one worker thread. Locking and special logic will need to
        //  be added to support multiple worker threads.
        //

        Assert(m_rgpThread.GetCount() <= 1);

        if (m_rgpThread.GetCount() == 1) 
        {
            hWorkerThread = m_rgpThread[0]->GetHandle();
        }
        
        Unlock();
    }

    //
    // Trigger the worker threads to wake on the next heartbeat and shutdown.
    //    
    
    SetEvent(m_hevWork);
    
    //
    // Wait for the threads to shut down outside of the critical section. If
    // we held the critical section for this wait, we'd deadlock on the 
    // worker threads which are also taking the CS.
    //
    
    if (hWorkerThread != INVALID_HANDLE_VALUE)
    {
        ::WaitForSingleObject(hWorkerThread, INFINITE);

        ::CloseHandle(hWorkerThread);
    }
    
    //
    // Flag that we're done shutting down.
    //
    
    m_fShutdown = false;
    
    Assert(m_rgpThread.GetCount() == 0);
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CPartitionManager::Shutdown
//
//    Synopsis:
//        Shutdown the partition manager and all the worker threads.
//
//------------------------------------------------------------------------------

void 
CPartitionManager::Shutdown()
{
    // Stop all the worker threads
    StopWorkerThreads();

    // Clean up the partitions
    ReleasePartitions();
    SAFE_DELETE(g_pMediaControl);
}


//+-----------------------------------------------------------------------
//
//  Member:
//      CPartitionManager::GetWork 
//
//  Synopsis:
//      Waits for working item, retrieves a partition that needs
//      composition pass or presenting.
//
//------------------------------------------------------------------------
WorkType
CPartitionManager::GetWork(
    __deref_out_ecount(1) Partition **ppPartition
    )
{
    *ppPartition = NULL;
    WorkType workType = WorkType_None;

    //
    // Set the state of m_hevWork to nonsignaled.
    // Do it before inspecting partition list so that
    // if the work will arrive during this procedure
    // it would not be missed.
    //
    ResetEvent(m_hevWork);


    while (!m_fShutdown)
    {
        Partition *pPartitionToRender = NULL;
        Partition *pPartitionToPresent = NULL;
        Partition *pPartitionToZombie = NULL;
        bool fNeedsBatchProcessing = false;
        bool fNeedsCompositionPass = false;

        //
        // Look thru partition list.
        //

        Lock();
        for (LIST_ENTRY *p = m_PartitionList.Flink; p != &m_PartitionList; p = p->Flink)
        {
            Partition *pPartition = static_cast<Partition*>(p);

            //
            // If this partition is already processed by another thread,
            // don't touch it.
            //
            if (pPartition->IsBeingProcessed())
                continue;

            if (pPartition->NeedsPresent())
            {
                // this partition is needs presenting that should
                // be done before executing rendering requests
                if (pPartitionToPresent == NULL)
                    pPartitionToPresent = pPartition;
            }
            else if (pPartition->NeedsRender())
            {
                // this partition is ready for rendering
                if (pPartitionToRender == NULL)
                    pPartitionToRender = pPartition;
            }
            else if (pPartition->NeedsBatchProcessing())
            {
                // partition has no immediate work to do, but
                // has queued batch processing request
                fNeedsBatchProcessing = true;
            }
            else if (pPartition->NeedsCompositionPass())
            {
                // partition has no immediate work to do, but
                // has queued composition pass request
                fNeedsCompositionPass = true;
            }
            else if (pPartition->NeedsZombieNotification())
            {
                // partition has been zombied, we need to send the notification to the UI thread
                if (pPartitionToZombie == NULL) 
                {
                    pPartitionToZombie = pPartition;
                }
            }
            else
            {
                AssertMsg(false, "Partition stays under manager's attention without reason.");
            }
        }

        // Choose the work. We want to handle the zombie partitions first,
        // then process partitions that need to render and finally present.
        if (pPartitionToZombie != NULL) 
        {
            pPartitionToZombie->SetStateFlags(PartitionIsBeingProcessed);

            *ppPartition = pPartitionToZombie;
            workType = WorkType_Zombie;
        }
        else if (pPartitionToRender)
        {
            //
            // Clear all flags related to rendering requests
            // before rendering is actually done, so that if new job will
            // appear during processing it would not be missed.
            //
            pPartitionToRender->ClearStateFlags(PartitionRenderClearFlags);

            pPartitionToRender->SetStateFlags(PartitionIsBeingProcessed);
            *ppPartition = pPartitionToRender;
            workType = WorkType_Render;
        }
        else if (pPartitionToPresent)
        {
            pPartitionToPresent->ClearStateFlags(PartitionNeedsPresent);

            pPartitionToPresent->SetStateFlags(PartitionIsBeingProcessed);
            *ppPartition = pPartitionToPresent;
            workType = WorkType_Present;
        }

        // Now we have PartitionIsBeingProcessed flag set so we can unlock.
        Unlock();

        // If we have found the work then we are done
        if (*ppPartition != NULL)
            break;

        //
        // There is no immediate work to do but there might be deferred requests.
        // Process throttling in accordance with given throttling mode.
        //

        //
        // For sleep-throttled thread, we handle requests ASAP but
        // take care of possible forever loop. The latter might appear
        // if during rendering pass ScheduleCompositionPass is called
        // thus causing another rendering pass. 
        // 

        if (fNeedsBatchProcessing)
        {
            ActivateDeferredPartitions(PartitionNeedsBatchProcessing);
        }
        else
        {
            DWORD dwTimeout = fNeedsCompositionPass ? 16 : INFINITE;
            WaitForSingleObject(m_hevWork, dwTimeout);

            //
            // Activate deferred partitions.
            // DO NOT check for WAIT_TIMEOUT result from WaitForSingleObject().
            // The frequency of scheduling events may be high
            // so that we'll never get WAIT_TIMEOUT.
            //
            ActivateDeferredPartitions(PartitionNeedsCompositionPass);
        }

    }

    return workType;
}

//+-----------------------------------------------------------------------
//
//  Member: CPartitionManager::ThreadStopped
//
//  Synopsis:  Called by each worker thread as it exits to
//             notify the manager it is stopping
//
//------------------------------------------------------------------------
void CPartitionManager::ThreadStopped(CPartitionThread *pThread)
{
    Lock();
    
    Verify(m_rgpThread.Remove(pThread));

    delete pThread;
    
    Unlock();
}

//+-----------------------------------------------------------------------------
//
//    Function:
//        UpdateSchedulerSettings
//
//    Synopsis:
//        Causes a scheduler type change. This will trigger shutdown of any 
//        existing worker threads.  Scheduler will then be re-created and 
//        new worker threads will be started.
//
//------------------------------------------------------------------------------

HRESULT CPartitionManager::UpdateSchedulerSettings(
    int nPriority
    )
{
    HRESULT hr = S_OK;
    CGuard<CCriticalSection> oGuard(g_csCompositionEngine);

    if (GetWorkerThreadPriority() != nPriority) 
    {
        C_ASSERT(NUM_WORKER_THREADS == 1);

        Assert(GetWorkerThreadCount() == 0);
        
        if (m_hevWork != NULL)
        {
            //
            // Stop the worker threads and clean up scheduler related resources.
            //
            
            StopWorkerThreads();
            
            ReleaseSchedulerResources();
        }

        //
        // Create the work event.
        //
        IFCW32(m_hevWork = CreateEvent(NULL, FALSE, FALSE, NULL));

        //
        // Put event handle in array for waiting
        //
        m_rgEvents[m_cEvents++] = m_hevWork;
        m_hevBeat = NULL;

        IFC(CreateWorkerThread(nPriority));
    }

Cleanup:
    RRETURN(hr);
}




// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_composition
//      $Keywords:  composition
//
//  $Description:
//      The cross-thread composition device that allows for deferred execution
//      of the partition commands.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CCrossThreadComposition, MILRender, "CCrossThreadComposition");


//+-----------------------------------------------------------------------------
// 
//    Member:
//        CCrossThreadComposition constructor
// 
//------------------------------------------------------------------------------       

CCrossThreadComposition::CCrossThreadComposition(__in MilMarshalType::Enum marshalType) 
    : CComposition(marshalType)
{
    // Zero-initialized by DECLARE_METERHEAP_CLEAR
    Assert(m_activeBatches == NULL);

    // Initialize the batch queue.
    InitializeSListHead(&m_enqueuedBatches);

    // Initialize debugging support for video playback
    DbgInitialize();
}


//+-----------------------------------------------------------------------------
// 
//    Member:
//        CCrossThreadComposition destructor
// 
//------------------------------------------------------------------------------       

CCrossThreadComposition::~CCrossThreadComposition()
{
    // Release any pending batches.
    ReleasePendingBatches();
}



//+-----------------------------------------------------------------------------
// 
//    Member:
//        CCrossThreadComposition::Create
// 
//    Synopsis:
//        Creates a new instance of the CCrossThreadComposition class.
// 
//------------------------------------------------------------------------------       

/* static */ HRESULT 
CCrossThreadComposition::Create(
    __in MilMarshalType::Enum marshalType,
    __out_ecount(1) CCrossThreadComposition **ppCrossThreadComposition
    )
{
    HRESULT hr = S_OK;

    Assert(marshalType != MilMarshalType::SameThread);

    CCrossThreadComposition *pCrossThreadComposition = 
        new CCrossThreadComposition(marshalType);

    IFCOOM(pCrossThreadComposition);

    IFC(pCrossThreadComposition->Initialize()); //NOTE: calls CComposition implementation of Initialize

    SetInterface(*ppCrossThreadComposition, pCrossThreadComposition);

Cleanup:
    ReleaseInterfaceNoNULL(pCrossThreadComposition);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CCrossThreadComposition::SubmitBatch
//
//  Synopsis:
//      Called by the server channel to enqueue a batch. If the partition 
//      is in zombie state, the batch is immediately released and the sync 
//      flush event is signaled.
//
//------------------------------------------------------------------------------

HRESULT 
CCrossThreadComposition::SubmitBatch(
    __in CMilCommandBatch *pBatch
    )
{
    HRESULT hr = S_OK;

#if ENABLE_PARTITION_MANAGER_LOG
    CPartitionManager::LogEvent(PartitionManagerEvent::SubmittingBatch, static_cast<DWORD>(reinterpret_cast<UINT_PTR>(pBatch)));
#endif /* ENABLE_PARTITION_MANAGER_LOG */

    //
    // Enqueue the batch and request processing it by worker thread.
    //

    bool fSuccess = g_pPartitionManager->ScheduleBatchProcessing(this, pBatch);

    if (!fSuccess)
    {
        //
        // Partition is in zombie state and will not accept any more batches.
        //
        // Signal that the work has been finished in case anybody's waiting
        // for the sync flush event.
        //

        TraceTag((tagMILVerbose,
                  "CCrossThreadComposition::SubmitBatch: partition is in zombie state, releasing the batch."
                  ));

        if (pBatch->GetChannelPtr() != NULL) 
        {
            //
            // If we are trying to submit a batch to a zombied partition post
            // a zombie message
            //

            MIL_MESSAGE msg = { MilMessageClass::PartitionIsZombie };
            msg.partitionIsZombieData.hrFailureCode = m_hrZombieNotificationFailureReason;               
            MIL_THR(pBatch->GetChannelPtr()->PostMessageToChannel(&msg));

            //
            // Signal the sync flush requests to avoid blocking the clients
            // indefinitely. 
            // 
            // Note that the channel pointer could be NULL when opening and
            // closing the channel. The later case is particularly interesting
            // as the lack of the NULL check could lead to an AV on attempt
            // to close a channel attached to a zombied partition.
            //

            pBatch->GetChannelPtr()->SignalFinishedFlush(m_hrZombieNotificationFailureReason);
            pBatch->SetChannelPtr(NULL);
        }

        delete pBatch;
    }


    RRETURN(hr);
}



//+-----------------------------------------------------------------------
//
//  Member:
//      CCrossThreadComposition::EnqueueBatch
//
//  Synopsis:
//      Enqueue the batch for processing by worker thread.
//
//------------------------------------------------------------------------

void
CCrossThreadComposition::EnqueueBatch(
    __inout_ecount(1) CMilCommandBatch *pBatch
    )
{
    InterlockedPushEntrySList(&m_enqueuedBatches, &(pBatch->m_link));

#if ENABLE_PARTITION_MANAGER_LOG
    CPartitionManager::LogEvent(PartitionManagerEvent::PushedBatch, static_cast<DWORD>(reinterpret_cast<UINT_PTR>(pBatch)));
#endif /* ENABLE_PARTITION_MANAGER_LOG */
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CCrossThreadComposition::ScheduleCompositionPass
// 
//  Synopsis:
//      Ensures that a composition pass will be scheduled in the nearest 
//      future even though no work might be available for the compositor.
// 
//  Notes:
//       investigate if schedule manager is able 
//          to handle the job of tracking work instead of partition manager.
// 
//------------------------------------------------------------------------------

/* override */ void 
CCrossThreadComposition::ScheduleCompositionPass()
{
    g_pPartitionManager->ScheduleCompositionPass(this);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CCrossThreadComposition::OnBeginComposition
// 
//  Synopsis:
//      Called by ProcessComposition after ensuring the display set.
// 
//------------------------------------------------------------------------------

/* override */ HRESULT
CCrossThreadComposition::OnBeginComposition()
{
    HRESULT hr = S_OK;

    if (g_pMediaControl && m_fQPCSupported)
    {
        m_dbgCompositionStartTime.QuadPart = 0;

        QueryPerformanceCounter(&m_dbgCompositionStartTime);
    }

    //
    // Transfer all the pending batches to the local device list so that we
    // can process them without worrying about request threads adding new
    // batches and breaking our ordering. This function is atomic so other
    // threads cannot interleave the batch order.
    //

    GetPendingBatches();

    IFC(ProcessBatches(true));

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CCrossThreadComposition::OnEndComposition
// 
//  Synopsis:
//      Called by ProcessComposition after the composition pass is over.
// 
//------------------------------------------------------------------------------

/* override */ HRESULT
CCrossThreadComposition::OnEndComposition()
{
    //
    // Get time when next tick is needed in the case of internal
    // animations. This allows the rendering thread to wake up even
    // if there are no updates to the composition. In this case we
    // will wake the thread, skip updating the composition (nothing to
    // do), run the animations and draw.
    //
    // Internal animations are the animations not asked from outside.
    // We need them to maintain device dependent smooth pixel grid
    // snapping (subpixel animation).
    //

    DWORD dwTimeout = m_scheduleManager.GetNextActivityTimeout();
    if (dwTimeout != INFINITE)
    {
        g_pPartitionManager->ScheduleCompositionPass(this);
    }

    DbgEndPerformanceDataCollection(m_dbgCompositionStartTime);

    RRETURN(S_OK);
}


//+-----------------------------------------------------------------------------
// 
//  Member:
//      CCrossThreadComposition::OnShutdownComposition
//
//  Synopsis:
//      Called by the composition device on shutdown.
// 
//------------------------------------------------------------------------------

/* override */ void
CCrossThreadComposition::OnShutdownComposition()
{
    ReleasePendingBatches();
}


//+-----------------------------------------------------------------------------
// 
//  Member:
//      CCrossThreadComposition::OnZombieComposition
//
//  Synopsis:
//      Called by Compose after the partition has been zombied.
// 
//------------------------------------------------------------------------------

/* override */ HRESULT
CCrossThreadComposition::OnZombieComposition()
{
    HRESULT hr = S_OK;

    Assert(IsZombie());

    //
    // If the partition is zombied, only process the attach/detach commands.
    //

    GetPendingBatches();

    IFC(ProcessBatches(false));

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
// 
//  Member:
//      CCrossThreadComposition::GetPendingBatches
//
//  Synopsis:
//      Transfers all the pending batches to the local device list so that we
//      can process them without worrying about request threads adding new
//      batches and breaking our ordering. This function is atomic so other
//      threads cannot interleave the batch order.
// 
//------------------------------------------------------------------------------

void 
CCrossThreadComposition::GetPendingBatches()
{
    //
    // Make sure that we have processed the entire list before
    //

    Assert(m_activeBatches == NULL);

    //
    // Get the list of pending batches.
    //

    m_activeBatches = RtlInterlockedFlushSList(&m_enqueuedBatches);

#if ENABLE_PARTITION_MANAGER_LOG
    CPartitionManager::LogEvent(m_activeBatches == NULL ? PartitionManagerEvent::BatchesFlushedNull : PartitionManagerEvent::BatchesFlushedNonNull, 0);        
#endif /* ENABLE_PARTITION_MANAGER_LOG */

    //
    // Reverse the list to get the order in which we will process the batches.
    //

    ReverseSList(&m_activeBatches);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CCrossThreadComposition::ReleasePendingBatches
//
//  Synopsis:
//      Releases all batches that have been queued for processing.
//
//-----------------------------------------------------------------------------

void 
CCrossThreadComposition::ReleasePendingBatches()
{
    //
    // The batches are stored in two lists, client and device. Batches are
    // enqueued on the client list and moved to and subsequently processed
    // on the device list. Clean up both lists.
    //

    ProcessBatches(false);
    GetPendingBatches();
    ProcessBatches(false);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CCrossThreadComposition::ProcessBatches
//
//  Synopsis:
//      Walks the frames device list and processes partition commands.
//
//-----------------------------------------------------------------------------

HRESULT 
CCrossThreadComposition::ProcessBatches(
    bool fProcessBatchCommands
    )
{
    HRESULT hr = S_OK;

    //
    // An input frame is a set of batches. m_InputFramesDevice are the
    // downstream (server to client) batches
    //

    CMilCommandBatch *pBatch = NULL;

    EventWriteWClientUceProcessQueueBegin((UINT64)this);
    
    while (m_activeBatches != NULL)
    {
        pBatch = CONTAINING_RECORD(m_activeBatches, CMilCommandBatch, m_link);

#if ENABLE_PARTITION_MANAGER_LOG
        CPartitionManager::LogEvent(PartitionManagerEvent::ProcessingBatch, static_cast<DWORD>(reinterpret_cast<UINT_PTR>(pBatch)));
#endif /* ENABLE_PARTITION_MANAGER_LOG */

        //
        // Advance the list before processing the batch, because the batch
        // processing will result in the batch being pushed on the lookaside
        // list when done.
        //

        m_activeBatches = m_activeBatches->Next;
        
        IFC(ProcessPartitionCommand(
            pBatch,
            fProcessBatchCommands
            ));
    }

    EventWriteWClientUceProcessQueueEnd((UINT64)this);

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CCrossThreadComposition::DbgInitialize
// 
//  Synopsis:
//      Initializes extra debugging support for video playback.
// 
//------------------------------------------------------------------------------

void
CCrossThreadComposition::DbgInitialize()
{
    m_dbgFrameCount = 0;
    m_dbgStartTime.QuadPart = 0;

    if (g_pMediaControl && m_fQPCSupported)
    {
        QueryPerformanceCounter(&m_dbgStartTime);
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CCrossThreadComposition::DbgEndPerformanceDataCollection
// 
//  Synopsis:
//      Called for every frame rendered, updates the state of the video
//      playback debugging support.
// 
//------------------------------------------------------------------------------

void
CCrossThreadComposition::DbgEndPerformanceDataCollection(LARGE_INTEGER compositionStartTime)
{
    if (g_pMediaControl && m_fQPCSupported)
    {
        m_dbgFrameCount++;

        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        LARGE_INTEGER passedTime;
        passedTime.QuadPart = (currentTime.QuadPart - m_dbgStartTime.QuadPart) * 1000 / m_qpcFrequency.QuadPart;

        LARGE_INTEGER compositionTime;
        compositionTime.QuadPart = (currentTime.QuadPart - compositionStartTime.QuadPart) * 1000 / m_qpcFrequency.QuadPart;

        m_dbgAccumulatedCompositionTime.QuadPart += compositionTime.QuadPart;

        if (passedTime.QuadPart > 1000)  // update every second
        {
            CMediaControlFile* pFile = g_pMediaControl->GetDataPtr();
            DWORD frameRate = (m_dbgFrameCount * 1000) / static_cast<DWORD>(passedTime.QuadPart);
            InterlockedExchange(reinterpret_cast<LONG *>(&pFile->FrameRate), frameRate);

            // Figure % of elapsed time spent in composition
            DWORD percent =
                static_cast<DWORD>((m_dbgAccumulatedCompositionTime.QuadPart * 100) / passedTime.QuadPart);
            InterlockedExchange(reinterpret_cast<LONG *>(&pFile->PercentElapsedTimeForComposition),
                                percent);

            m_dbgStartTime = currentTime;
            m_dbgFrameCount = 0;
            m_dbgAccumulatedCompositionTime.QuadPart = 0;
        }
    }

    CDirtyRegion2::UpdatePerFrameStatistics();
}




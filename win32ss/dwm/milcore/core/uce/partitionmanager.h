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

#pragma once

#include "compatsettings.h"

//+-----------------------------------------------------------------------------
//
//    Enumeration:
//        WorkType
// 
//    Synopsis:
//        CPartitionManager::GetWork() return values.
//
//------------------------------------------------------------------------------

enum WorkType
{
    WorkType_None,
    WorkType_Render,
    WorkType_Present,
    WorkType_Zombie,
};


#define NUM_WORKER_THREADS 1


#if ENABLE_PARTITION_MANAGER_LOG

//+-----------------------------------------------------------------------------
//
//    Enumeration:
//        PartitionManagerEvent
// 
//    Synopsis:
//        Partition manager log contains information about a partition's 
//        state changes. This includes flags cleared and set and batch 
//        s-list operations.
// 
// adding partition manager log event as instrumentation to help narrow down this bug.
//    
//------------------------------------------------------------------------------

BEGIN_MILFLAGENUM( PartitionManagerEvent )

    PartitionManagerCtor            = 0x10000000,
    PartitionManagerDtor            = 0x20000000,
    PartitionManagerChangeScheduler = 0x30000000,
    ClearedFlags                    = 0x40000000,
    SetFlags                        = 0x50000000,
    EffectiveFlags                  = 0x60000000,
    EnqueuedPartition               = 0x70000000,
    DequeuedPartition               = 0x80000000,
    PushedBatch                     = 0x90000000,
    ExecutedSameThreadBatch         = 0xA0000000,
    BatchesFlushedNull              = 0xB0000000,
    BatchesFlushedNonNull           = 0xC0000000,
    SubmittingBatch                 = 0xD0000000,
    Composing                       = 0xE0000000,
    ProcessingBatch                 = 0xF0000000,

    Mask                            = 0x0FFFFFFF,

END_MILFLAGENUM

#endif /* ENABLE_PARTITION_MANAGER_LOG */

//
// This is the global partition manager.
//

extern class CPartitionManager *g_pPartitionManager;

MtExtern(CPartitionManager);

//
// Forward declarations.
//

class CPartitionThread;

//+------------------------------------------------------------------------
//
//  Class:
//      CPartitionManager
//
//  Synopsis:
//      This class serves as a dispatcher that organizes execution
//  process of the whole composition/rendering machine.
//
//  There is only one instance of CPartitionManager.
//  It holds a pool of working items that should be executed by worker
//  threads. Working items are supplied by UI thread. Worker threads fetch
//  items from the pool and execute them. During execution, worker threads
//  also can generate new working items and put them into the pool, so that
//  they will be executed later.
//
//  There is no separate struct or class for working item.
//  Instead, CPartitionManager operates with instances of class Partition.
//  The Partition has a flag word named m_state that describes what
//  particularly should be done with this partition. When several flags
//  in m_state are set, this means that several working items are
//  associated with this partition.
//
//  Storing working items in a form of partitions and flag words has
//  another benefit which is important for multi-thread and multi-partition
//  scenario. We need to avoid inter-thread collisions; the key rule here is
//  that the partition can not be accessed from several threads at a time.
//
//  Worker threads perform their job cyclically, like following:
//  ThreadRoutine()
//  {
//      for(;;)
//      {
//          // Apply to partition manager for work:
//          Partition *pPartition;
//          CPartitionManager::GetWork(&pPartition);
//                  // At this moment partition manager
//                  // locks the partition so that other threads
//                  // will not get it.
//
//          // Do the work in accordance with pPartition->m_state:
//          ProcessWork(pPartition);
//                  // This can be pretty long procedure.
//                  // While executing it can clear some flags in
//                  // pPartition->m_state thus declaring that
//                  // the work has been executed, and also can
//                  // set some flags in order to schedule some
//                  // work to be executed later.
//
//          // Update partition state and complete partition processing:
//          CPartitionManager::CompleteProcessing(pPartition);
//                  // At this moment partition manager
//                  // unlocks the partition so that it can play
//                  // in consequent GetWork calls.
//      }
//  }
//  
//  The values used in Partition::m_state are described in partition.h.
//
//--------------------------------------------------------------------
//
//  Following is typical sequence of m_state changes:
//
//  [partition is idle, m_state = 0]
//
//  1.  UI thread submits batch and calls ScheduleBatchProcessing().
//
//  [m_state has flag PartitionNeedsBatchProcessing]
//
//  2.  Worker thread calls GetWork().
//
//  [PartitionNeedsBatchProcessing is cleared, PartitionIsBeingProcessed set]
//
//  3.  Worker thread executes composition pass, schedules presenting and
//      stops handling the partition by calling
//      SchedulePresentAndCompleteProcessing().
//
//  [PartitionIsBeingProcessed cleared, PartitionNeedsPresent set]
//
//  4.  Worker thread calls GetWork().
//
//  [PartitionNeedsPresent is cleared, PartitionIsBeingProcessed set]
//
//  5.  Worker thread executes presenting and stops handling the partition
//      by calling CompleteProcessing().
//
//  [PartitionIsBeingProcessed cleared]
//      at this moment the parttion goes away from managers list unless
//      some flags appeared while steps above were processed. This might
//      happen if another batch will arrive. Or, working thread called
//      ScheduleCompositionPass to force redoing step 3.
//
//-------------------------------------------------------------------------
class CPartitionManager
{

public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CPartitionManager));


    CPartitionManager();
    ~CPartitionManager();
    
    static HRESULT Create(
        int nPriority,
        __deref_out_ecount(1) CPartitionManager **ppm
        );

    HRESULT GetComposedEventId(
        __out_ecount(1) UINT *pcEventId
        );

    bool ScheduleBatchProcessing(
        __inout_ecount(1) Partition * pPartition,
        __inout_ecount(1) CMilCommandBatch *pBatch
        );

    void ScheduleCompositionPass(
        __inout_ecount(1) Partition * pPartition
        );
        
    void ScheduleRenderingPass(
        __inout_ecount(1) Partition * pPartition
        );
        
    void SchedulePresentAndCompleteProcessing(
        __inout_ecount(1) Partition * pPartition
        );

    // Stops partition processing and puts a partition in zombie state.
    void ZombifyPartitionAndCompleteProcessing(
        __in_ecount(1) Partition *pPartition,
        HRESULT hrFailureCode
        );

    HRESULT UpdateSchedulerSettings(
        int nPriority
        );

    //
    // Create a new thread to service the partitions.
    //
    
    HRESULT CreateWorkerThread(int nPriority);


    // Returns the current worker thread priority setting
    int GetWorkerThreadPriority() const { return m_nWorkerThreadPriority; }

    //
    // Shutdown the partition manager and all the worker threads.
    //
    
    VOID Shutdown();

    //
    //  Called by worker thread to notify the manager it it stopping
    //
    
    void ThreadStopped(CPartitionThread *pThread);

    //
    // Waits for working item, retrieves a partition that needs
    // composition pass or presenting
    //
    WorkType GetWork(
        __deref_out_ecount(1) Partition **ppPartition
        );

    //
    // Called by the partition thread
    // after it completes processing a partition
    //
    void CompleteProcessing(
        __inout_ecount(1) Partition *pPartition
        );

    // Attempts to perform pending zombie notifications
    void HandleZombiePartition(
        __in_ecount(1) Partition *pPartition
        );

    //
    // Get the number of worker threads.
    //
    
    UINT GetWorkerThreadCount() const { return m_rgpThread.GetCount(); }

    inline CCompatSettings& GetCompatSettings() { return m_compatSettings; }

#if ENABLE_PARTITION_MANAGER_LOG
    // Adds an entry to the partition manager's log
    static void LogEvent(
        PartitionManagerEvent::FlagsEnum event,
        DWORD value // Masked with PartitionManagerEvent::Mask
        );
#endif /* ENABLE_PARTITION_MANAGER_LOG */
private:

    //
    // Prepares the partition manager for first use.
    //

    HRESULT Initialize(
        int nPriority
        );    

    // 
    // Shuts down all the worker threads.
    //

    void StopWorkerThreads();

    //
    // Releases the resources used by the scheduler.
    //

    void ReleaseSchedulerResources();

    //
    // Releases the partitions managed by this object.
    //

    VOID ReleasePartitions();

    void SetPartitionState(
        __inout_ecount(1) Partition * pPartition,
        PartitionState flagsToClear,
        PartitionState flagsToSet
        );

    void ActivateDeferredPartitions(PartitionState flags);
    
#if DBG_ANALYSIS
    bool CurrentThreadIsWorkerThread();
#endif

private:
    //
    // List of partitions that need manager's attention.
    //
    LIST_ENTRY m_PartitionList;

    //
    // The partition manager is accessed from multiple threads. It is protected
    // by this critical section. Specifically the ready list is protected.
    //

    CCriticalSection m_cs;

    //
    // This flag is signalled when the system is shutting down. It's used by 
    // the worker threads to indicate they should stop processing.
    //
    
    volatile bool m_fShutdown;
    
    // Keep track of the threads
    DynArray<CPartitionThread *> m_rgpThread;

    //    
    // Worker threads wake up when both the pending work event and the heartbeat
    // timer are signalled. Currently the heartbeat event is a simple 10ms
    // OS timer event, but it should be hooked up to the V-Blank event when
    // this becomes available from LDDM.
    //
    
    //
    // Heartbeat timer. The worker threads will wake up on this timer in order
    // to process the work in the ready queue.
    //
    
    HANDLE m_hevBeat;
    
    //
    // Work event. This is signalled anytime anything is added to the pending
    // list. 
    //
    
    HANDLE m_hevWork;

     // Array of events for waiting
    HANDLE m_rgEvents[2];    
    DWORD m_cEvents;

    // The current worker thread priority
    int m_nWorkerThreadPriority;


    // Compat settings
    CCompatSettings m_compatSettings;

    //  Locking methods to protect the pending and ready list
    //  Locking methods are exposed because the
    //  CPartitionThread class must use them
    //
    inline void Lock()
    {
        m_cs.Enter();
    }

    inline void Unlock()
    {
        m_cs.Leave();
    }

#if DBG
    inline BOOL Locked()
    {
        return (HandleToUlong(m_cs.OwningThread()) == GetCurrentThreadId());
    }
#endif
};


// MilPerfInstrumentationFlags serve to control instrumentation
// for rendering performance measurement.

extern UINT g_uMilPerfInstrumentationFlags;

enum MilPerfInstrumentationFlags
{
    // When MilPerfInstrumentation_DisableThrottling set, worker threads do not
    // wait for heartbeat event and so will provide maximum throughput.
    MilPerfInstrumentation_DisableThrottling = 1,


    // When MilPerfInstrumentation_SignalPresent set, CSlaveHWndRenderTarget::Present()
    // posts WM_USER message that can be caught in test to detect frame rendering completing.
    MilPerfInstrumentation_SignalPresent = 2,
};


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

MtExtern(CCrossThreadComposition);


//+-----------------------------------------------------------------------------
//
//  Class:
//      CCrossThreadComposition
// 
//  Synopsis:
//      The cross-thread compositor.
// 
//------------------------------------------------------------------------------

class CCrossThreadComposition : public CComposition
{
protected:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CCrossThreadComposition));

    CCrossThreadComposition(__in MilMarshalType::Enum marshalType);

    virtual ~CCrossThreadComposition();

public:
    DEFINE_REF_COUNT_BASE;

    // Creates a new instance of the CCrossThreadComposition class.
    static HRESULT Create(
        __in MilMarshalType::Enum marshalType,
        __out_ecount(1) CCrossThreadComposition **ppCrossThreadComposition
        );

    //
    // Submit a batch for processing on this device. Note that ownership of
    // the batch memory is transferred from the caller to the composition
    // device and on success the caller's batch object will be NULL to avoid
    // accidental modification. The composition device is responsible for
    // releasing the batch and putting it on the appropriate lookaside when
    // it's done processing it.
    //
    override HRESULT SubmitBatch(
        __in CMilCommandBatch *pBatch
        );

    // Enqueue the batch for processing by worker thread.
    override void EnqueueBatch(
        __inout_ecount(1) CMilCommandBatch *pBatch
        );

    // Ensures that an extra composition pass will be scheduled.
    override void ScheduleCompositionPass();

protected:
    // Called by ProcessComposition after ensuring the display set.
    override HRESULT OnBeginComposition();

    // Called by ProcessComposition after the composition pass is over.
    override HRESULT OnEndComposition();

    // Called by the composition device on shutdown.
    override void OnShutdownComposition();

    // Called by Compose after the partition has been zombied.
    override HRESULT OnZombieComposition();

    // Run on the rendering thread to retrieve the queued batches and
    // ensure they're ordered correctly.
    void GetPendingBatches();

    // Releases all batches that have been queued for processing.
    void ReleasePendingBatches();

    // Flushes the batch queue and executes every batch accumulated so far.
    HRESULT ProcessBatches(
        bool fProcessBatchCommands = true
        );

private:
    void DbgEndPerformanceDataCollection(LARGE_INTEGER compositionStartTime);

    void DbgInitialize();

private:
    // This s-list contains the batches accumulated so far.
    SLIST_HEADER m_enqueuedBatches;

    // This is the list of the most recently flushed batches.
    PSLIST_ENTRY m_activeBatches;

    UINT     m_lastNotifiedSysmemUsagePercent;

    UTC_TIME m_timeMemoryUsageLastChecked;

    UTC_TIME m_timeVideoMemoryUsageLastChecked;


    //+-------------------------------------------------------------------------
    //
    //  Debugging support
    //
    //--------------------------------------------------------------------------

    // Number of frames rendered by this composition device.
    UINT m_dbgFrameCount;

    // The QPC time of creation of this composition device.
    LARGE_INTEGER m_dbgStartTime;

    // The composition start time recorded for debugging purposes.
    LARGE_INTEGER m_dbgCompositionStartTime;

    // The total QPC time spent in composition so far.
    LARGE_INTEGER m_dbgAccumulatedCompositionTime;
};



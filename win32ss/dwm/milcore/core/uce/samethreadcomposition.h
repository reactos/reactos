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
//      The same-thread composition device that allows only for immediate
//      execution of the partition commands.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CSameThreadComposition);


//+-----------------------------------------------------------------------------
//
//    Class:
//        CSameThreadComposition
// 
//    Synopsis:
//        A specialized composition device that allows only for immediate
//        execution of the partition commands.
// 
//------------------------------------------------------------------------------

class CSameThreadComposition : public CComposition
{
protected:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CSameThreadComposition));

    CSameThreadComposition(__in MilMarshalType::Enum marshalType);

    virtual ~CSameThreadComposition();

public:
    // Creates a new instance of the CSameThreadComposition class.
    static HRESULT Create(
        __in MilMarshalType::Enum marshalType,
        __out_ecount(1) CSameThreadComposition **ppSynchronousComposition
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

protected:
    // Ensures that an extra composition pass will be scheduled.
    override void ScheduleCompositionPass();

    // Called by ProcessComposition after ensuring the display set.
    override HRESULT OnBeginComposition();

    // Called by ProcessComposition after the composition pass is over.
    override HRESULT OnEndComposition();

    // Called by the composition device on shutdown.
    override void OnShutdownComposition();

    // Called by Compose after the partition has been zombied.
    override HRESULT OnZombieComposition();
};



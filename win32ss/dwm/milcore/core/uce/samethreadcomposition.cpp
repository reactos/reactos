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

#include "precomp.hpp"

MtDefine(CSameThreadComposition, MILRender, "CSameThreadComposition");


//+-----------------------------------------------------------------------------
// 
//  Member:
//      CSameThreadComposition constructor
// 
//------------------------------------------------------------------------------       

CSameThreadComposition::CSameThreadComposition(
    __in MilMarshalType::Enum marshalType
    ) : CComposition(marshalType)
{
    // Zero-initialized by DECLARE_METERHEAP_CLEAR
}


//+-----------------------------------------------------------------------------
// 
//  Member:
//      CSameThreadComposition destructor
// 
//------------------------------------------------------------------------------       

CSameThreadComposition::~CSameThreadComposition()
{
}



//+-----------------------------------------------------------------------------
// 
//  Member:
//      CSameThreadComposition::Create
// 
//  Synopsis:
//      Creates a new instance of the CSameThreadComposition class.
// 
//------------------------------------------------------------------------------       

/* static */ HRESULT 
CSameThreadComposition::Create(
    __in MilMarshalType::Enum marshalType,
    __out_ecount(1) CSameThreadComposition **ppSynchronousComposition
    )
{
    HRESULT hr = S_OK;

    Assert(marshalType == MilMarshalType::SameThread);

    CSameThreadComposition *pSynchronousComposition = new CSameThreadComposition(marshalType);

    IFCOOM(pSynchronousComposition);

    IFC(pSynchronousComposition->Initialize()); //NOTE: calls CComposition implementation of Initialize

    SetInterface(*ppSynchronousComposition, pSynchronousComposition);

Cleanup:
    ReleaseInterfaceNoNULL(pSynchronousComposition);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSameThreadComposition::SubmitBatch
//
//  Synopsis:
//      Called by the packet player to enqueue a batch. If the partition is
//      in zombie state, the batch is immediately released and the sync flush
//      event is signaled.
//
//------------------------------------------------------------------------------

/* override */ HRESULT 
CSameThreadComposition::SubmitBatch(
    __in CMilCommandBatch* pBatch
    )
{
    HRESULT hr = S_OK;


#if ENABLE_PARTITION_MANAGER_LOG
    CPartitionManager::LogEvent(PartitionManagerEvent::SubmittingBatch, static_cast<DWORD>(reinterpret_cast<UINT_PTR>(pBatch)));
#endif /* ENABLE_PARTITION_MANAGER_LOG */

    //
    // Process batch on the current thread.
    //

    {
        CFloatFPU oGuard;

        MIL_THR(ProcessPartitionCommand(pBatch, true /* process command batches */));

        FlushChannels(false);
    }

#if ENABLE_PARTITION_MANAGER_LOG
    CPartitionManager::LogEvent(PartitionManagerEvent::ExecutedSameThreadBatch, static_cast<DWORD>(reinterpret_cast<UINT_PTR>(pBatch)));
#endif /* ENABLE_PARTITION_MANAGER_LOG */

    //
    // Zero the batch out, so that no one plays with it any more.
    //


    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member:
//      CSameThreadComposition::EnqueueBatch
//
//  Synopsis:
//      Enqueue the batch for processing by worker thread.
//
//------------------------------------------------------------------------

void
CSameThreadComposition::EnqueueBatch(
    __inout_ecount(1) CMilCommandBatch *pBatch
    )
{
    RIP("Should never call EnqueueBatch on a same-thread composition device.");
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSameThreadComposition::ScheduleCompositionPass
// 
//  Synopsis:
//      The synchronous compositor is inherently unscheduled -- therefore
//      this method is implemented as a no-op.
// 
//------------------------------------------------------------------------------

/* override */ void 
CSameThreadComposition::ScheduleCompositionPass()
{
    // No-op.
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSameThreadComposition::OnBeginComposition
// 
//  Synopsis:
//      Called by ProcessComposition after ensuring the display set. 
// 
//      The same-thread compositor performs only the core composition,
//      therefore this method is implemented as a no-op.
// 
//------------------------------------------------------------------------------

/* override */ HRESULT
CSameThreadComposition::OnBeginComposition()
{
    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSameThreadComposition::OnEndComposition
// 
//  Synopsis:
//      Called by ProcessComposition after the composition pass is over.
// 
//      The same-thread compositor performs only the core composition,
//      therefore this method is implemented as a no-op.
// 
//------------------------------------------------------------------------------

/* override */ HRESULT
CSameThreadComposition::OnEndComposition()
{
    return S_OK;
}


//+-----------------------------------------------------------------------------
// 
//  Member:
//      CSameThreadComposition::OnShutdownComposition
//
//  Synopsis:
//      Called by the composition device on shutdown.
// 
//      The same-thread compositor performs only the core composition,
//      therefore this method is implemented as a no-op.
// 
//------------------------------------------------------------------------------

/* override */ void
CSameThreadComposition::OnShutdownComposition()
{
    // No-op.
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSameThreadComposition::OnZombieComposition
// 
//  Synopsis:
//      Called by Compose after the partition has been zombied.
// 
//      The same-thread compositor performs only the core composition,
//      therefore this method is implemented as a no-op.
// 
//------------------------------------------------------------------------------

/* override */ HRESULT
CSameThreadComposition::OnZombieComposition()
{
    return S_OK;
}



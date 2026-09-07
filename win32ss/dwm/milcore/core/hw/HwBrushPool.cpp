// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_resourcemgmt
//      $Keywords:
//
//  $Description:
//      Contains CHwBrushPool and CHwBrushPoolManager implementations
//
//      Manages Hardware Brush Pool.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


MtDefine(CHwBrushPoolManager, MILRender, "CHwBrushPoolManager");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPoolManager::CHwBrushPoolManager
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CHwBrushPoolManager::CHwBrushPoolManager(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    )
{
    InitializeSListHead(&m_oUnusedListHead);
    InitializeListHead(&m_leAllHead);

    m_cReadyToUse = 0;
    m_pcpbHead = NULL;
    m_pcpbTail = NULL;
    m_cOutstandingBrushes = 0;

    // Not ref counted as this should be a satellite class of the device
    m_pDeviceNoRef = pDevice;

#if DBG
    m_fDbgReleased = FALSE;
#endif DBG
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPoolManager::~CHwBrushPoolManager
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CHwBrushPoolManager::~CHwBrushPoolManager()
{
    #if DBG
    Assert(m_fDbgReleased);
    #endif
    Assert(m_cOutstandingBrushes == -1);

    ReleaseUnusedBrushes();

    Assert(m_cReadyToUse == 0);
    Assert(QueryDepthSList(&m_oUnusedListHead) == 0);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPoolManager::Release
//
//  Synopsis:
//      Release this pool manager.  The only valid caller of this method is its
//      owner, which should be a generic brush pool.
//
//------------------------------------------------------------------------------
void CHwBrushPoolManager::Release()
{
#if DBG
    Assert(!m_fDbgReleased);

    m_fDbgReleased = TRUE;
#endif

    // This ReleaseUnusedBrushes isn't strictly required, but is useful
    // to get as much cleaned up now as possible.
    ReleaseUnusedBrushes();

    // Walk all (outstanding) brushes to mark them as invalid; so that they can
    // never call the device again, which after this will likely be invalid.
    // Note that we don't worry about threading since validity should only be
    // checked under the protected rendering context which is exactly when this
    // Release should be being called.
    MarkAllBrushesInvalid();

    // Decrement the outstanding brush count so that it may now reach
    // -1 signaling the need for object deletion.
    DecOutstanding();
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPoolManager::AddToList
//
//  Synopsis:
//      Add the specified object to tracking list
//

void
CHwBrushPoolManager::AddToList(
    __inout_ecount(1) CHwCacheablePoolBrush *pNewHwBrush
    )
{
    // Don't add a ref count since we don't want the pool manager to keep a
    // brush alive.

    Assert(IsListEmpty(&pNewHwBrush->m_leAll));
    if (IsListEmpty(&m_leAllHead))
    {
        Assert(m_cOutstandingBrushes == 0);
    }

    InsertHeadList(&m_leAllHead, &pNewHwBrush->m_leAll);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPoolManager::Remove
//
//  Synopsis:
//      Remove the specified object from the list
//
//------------------------------------------------------------------------------
void
CHwBrushPoolManager::Remove(
    __inout_ecount(1) CHwCacheablePoolBrush *pcpbOld
    )
{
    CHwCacheablePoolBrush **ppcpb = &m_pcpbHead;

    // While there is an item and it is not a match
    while (*ppcpb && *ppcpb != pcpbOld)
    {
        // Next
        ppcpb = &((*ppcpb)->m_pcpbNextInPool);
    }

    // Was there a match?
    if (*ppcpb)
    {
        Assert(*ppcpb == pcpbOld);

        // Remove it
        m_cReadyToUse--;
        *ppcpb = pcpbOld->m_pcpbNextInPool;

        // Check if the last item is being removed
        if (*ppcpb == NULL)
        {
            Assert(m_pcpbTail == pcpbOld);

            // Check for a previous item
            if (m_cReadyToUse > 0)
            {
                Assert(ppcpb != &m_pcpbHead);

                // Dereference previous item
                m_pcpbTail = CONTAINING_RECORD(
                    ppcpb,
                    CHwCacheablePoolBrush,
                    m_pcpbNextInPool
                    );
            }
            else
            {
                Assert(ppcpb == &m_pcpbHead);

                m_pcpbTail = NULL;
            }
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPoolManager::UnusedNotification
//
//  Synopsis:
//      Used to notify the manager that there are no outstanding uses and the
//      manager has full control.  Place the unused brush in the free list.
//
//      WARNING: This method may be called from any thread.
//
//------------------------------------------------------------------------------
void
CHwBrushPoolManager::UnusedNotification(
    __inout_ecount(1) CMILPoolResource *pUnused
    )
{
    CHwCacheablePoolBrush *pcpbUnused = DYNCAST(CHwCacheablePoolBrush, pUnused);
    Assert(pcpbUnused);

    InterlockedPushEntrySList(&m_oUnusedListHead, &(pcpbUnused->m_oListEntry));

    DecOutstanding();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPoolManager::UnusableNotification
//
//  Synopsis:
//      Used to notify the manager that the resource is no longer usable and
//      should be removed from the pool.
//
//------------------------------------------------------------------------------
void
CHwBrushPoolManager::UnusableNotification(
    __inout_ecount(1) CMILPoolResource *pUnusable
    )
{
    CHwCacheablePoolBrush *pcpbUnusable =
        DYNCAST(CHwCacheablePoolBrush, pUnusable);
    Assert(pcpbUnusable);
    Remove(pcpbUnusable);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPoolManager::ConsolidateUnusedLists
//
//  Synopsis:
//      Move all object on the synchronized unused list to the freely accessed
//      (device protection) ready to use list.
//
//------------------------------------------------------------------------------
void
CHwBrushPoolManager::ConsolidateUnusedLists()
{
    PSLIST_ENTRY pUnusedHead;

    pUnusedHead = InterlockedFlushSList(&m_oUnusedListHead);

    if (pUnusedHead)
    {
        //
        // Build a least recently used list from the most recently
        // used (technically most recently released) list and count
        // as this is done.
        //

        CHwCacheablePoolBrush *pcpbLRU = NULL;
        CHwCacheablePoolBrush *pcpbMRU =
            CONTAINING_RECORD(
                pUnusedHead,
                CHwCacheablePoolBrush,
                m_oListEntry
                );

        do
        {
            // Dereference brush from list entry
            CHwCacheablePoolBrush *pcpbUnused =
                CONTAINING_RECORD(
                    pUnusedHead,
                    CHwCacheablePoolBrush,
                    m_oListEntry
                    );

            // Get next brush on list
            pUnusedHead = pUnusedHead->Next;
            // Make this brush the head of the LRU list
            pcpbUnused->m_pcpbNextInPool = pcpbLRU;
            pcpbLRU = pcpbUnused;
            // Increment count
            m_cReadyToUse++;
        } while (pUnusedHead);

        //
        // Append new list to the current ready to use list
        //

        if (m_pcpbTail)
        {
            Assert(m_pcpbHead);
            m_pcpbTail->m_pcpbNextInPool = pcpbLRU;
        }
        else
        {
            Assert(!m_pcpbHead);
            m_pcpbHead = pcpbLRU;
        }
        m_pcpbTail = pcpbMRU;
        Assert(m_pcpbTail->m_pcpbNextInPool == NULL);
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPoolManager::ReleaseUnusedBrushes
//
//  Synopsis:
//      Release all of the brushes currently in the pool. (This does not include
//      the outstanding brushes.)
//
//------------------------------------------------------------------------------
void
CHwBrushPoolManager::ReleaseUnusedBrushes()
{
    ConsolidateUnusedLists();

    if (m_pcpbHead)
    {
        Assert(m_cReadyToUse > 0);

        CHwCacheablePoolBrush *pNext;

        do
        {
            pNext = m_pcpbHead->m_pcpbNextInPool;
            delete m_pcpbHead;
            m_pcpbHead = pNext;
        } while (m_pcpbHead);

        Assert(m_pcpbTail);
        m_pcpbTail = NULL;
        m_cReadyToUse = 0;
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPoolManager::MarkAllBrushesInvalid
//
//  Synopsis:
//      Walk entire list of brushes and mark them all as invalid
//

void
CHwBrushPoolManager::MarkAllBrushesInvalid()
{
    PLIST_ENTRY pListEntry = m_leAllHead.Flink;

    // Loop until list head
    while (pListEntry != &m_leAllHead)
    {
        // Decode CHwCacheablePoolBrush address
        CHwCacheablePoolBrush *pHwBrush =
            CONTAINING_RECORD(pListEntry,
                              CHwCacheablePoolBrush,
                              m_leAll);

        // Mark brush as invalid
        pHwBrush->m_fValid = FALSE;

        // Advance pListEntry
        pListEntry = pListEntry->Flink;
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPoolManager::AllocateHwBrush
//
//  Synopsis:
//      Find or create a hw brush for the given device independent brush and get
//      it realized.
//
//------------------------------------------------------------------------------
HRESULT
CHwBrushPoolManager::AllocateHwBrush(
    __inout_ecount(1) CMILBrush *pBrush,
    __in_ecount(1) const CHwBrushContext &hwBrushContext,
    __deref_out_ecount(1) CHwBrush ** const ppHwBrush
    )
{
    HRESULT hr;

    ConsolidateUnusedLists();

    // Try to find a usable brush in the unused list
    hr = E_FAIL;

    while (FAILED(hr) && m_pcpbHead)
    {
        CHwCacheablePoolBrush *pUnused = m_pcpbHead;

        //
        //  Instead of always just picking the LRU valid brush and having it
        //  realize this new brush, find a brush that already definitely has
        //  the required texels.  Furthermore, it might be good to avoid using
        //  a hw brush that has too many texels, which would waste a more
        //  widely usable resource and potentially incur unnecessary setup
        //  cost.
        //

        // Remove it from the list
        Remove(pUnused);

        Assert(pUnused->GetRefCount() == 0);

        if (pUnused->IsValid())
        {
            MIL_THR(pUnused->SetBrushAndContext(
                pBrush,
                hwBrushContext
                ));
        }

        if (SUCCEEDED(hr))
        {
            *ppHwBrush = pUnused;
            (*ppHwBrush)->AddRef();
        }
        else
        {
            delete pUnused;
        }
    }

    //
    //  Instead of keeping all unused brushs, pick some to be destroyed. This
    //  may be based on keeping a dynamic amount or that the resource just
    //  hasn't been reused in a long time.
    //

    // We were unsuccessful at reusing a once cached brush
    // so now try to create a new one.
    if (FAILED(hr))
    {
        MIL_THR(CreateHwBrush(
            pBrush,
            hwBrushContext,
            ppHwBrush
            ));
    }

    if (SUCCEEDED(hr))
    {
        Assert(*ppHwBrush);

        //
        // A new brush has been created or one has been pulled from
        // our unused list so increment the number of outstanding
        // brushes.
        //

        LONG cOutstanding;

        cOutstanding = InterlockedIncrement(&m_cOutstandingBrushes);

        Assert(cOutstanding > 0);
    }

    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwLinearGradientBrushPoolManager::CreateHwBrush
//
//  Synopsis:
//      Create a new HW brush for the given DI brush
//
//------------------------------------------------------------------------------
HRESULT
CHwLinearGradientBrushPoolManager::CreateHwBrush(
    __inout_ecount(1) CMILBrush *pBrush,
    __in_ecount(1) const CHwBrushContext &hwBrushContext,
    __deref_out_ecount(1) CHwBrush ** const ppHwBrush
    )
{
    HRESULT hr;

    *ppHwBrush = NULL;

    CHwLinearGradientBrush *pHwBrush;

    pHwBrush = new CHwLinearGradientBrush(this, m_pDeviceNoRef);
    IFCOOM(pHwBrush);
    // No AddRef here

    MIL_THR(pHwBrush->SetBrushAndContext(
        pBrush,
        hwBrushContext
        ));

    if (SUCCEEDED(hr))
    {
        //
        // Add to list
        //

        AddToList(pHwBrush);

        //
        // Return the new, referenced brush
        //

        *ppHwBrush = pHwBrush;
        (*ppHwBrush)->AddRef();
    }
    else
    {
        // If new brush creation failed then we need to just
        // delete the object.  Had we AddRef'ed it and then
        // Release'd it, it would end up on our unused list.
        delete pHwBrush;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwRadialGradientBrushPoolManager::CreateHwBrush
//
//  Synopsis:
//      Create a new HW brush for the given DI brush
//
//------------------------------------------------------------------------------
HRESULT
CHwRadialGradientBrushPoolManager::CreateHwBrush(
    __inout_ecount(1) CMILBrush *pBrush,
    __in_ecount(1) const CHwBrushContext &hwBrushContext,
    __deref_out_ecount(1) CHwBrush ** const ppHwBrush
    )
{
    HRESULT hr;

    *ppHwBrush = NULL;

    CHwRadialGradientBrush *pHwBrush;

    pHwBrush = new CHwRadialGradientBrush(this, m_pDeviceNoRef);
    IFCOOM(pHwBrush);
    // No AddRef here

    MIL_THR(pHwBrush->SetBrushAndContext(
        pBrush,
        hwBrushContext
        ));

    if (SUCCEEDED(hr))
    {
        //
        // Add to list
        //

        AddToList(pHwBrush);

        //
        // Return the new, referenced brush
        //

        *ppHwBrush = pHwBrush;
        (*ppHwBrush)->AddRef();
    }
    else
    {
        // If new brush creation failed then we need to just
        // delete the object.  Had we AddRef'ed it and then
        // Release'd it, it would end up on our unused list.
        delete pHwBrush;
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPool::CHwBrushPool
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CHwBrushPool::CHwBrushPool()
{
    m_psbScratch = NULL;
    m_pbpmGradientLinear = NULL;
    m_pbpmGradientRadial = NULL;

    m_pbbScratch = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPool::~CHwBrushPool
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CHwBrushPool::~CHwBrushPool()
{
    delete m_psbScratch;

    ReleaseInterfaceNoNULL(m_pbpmGradientLinear);
    ReleaseInterfaceNoNULL(m_pbpmGradientRadial);

    delete m_pbbScratch;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPool::Init
//
//  Synopsis:
//      Inits the pool
//
//------------------------------------------------------------------------------
HRESULT 
CHwBrushPool::Init(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    )
{
    HRESULT hr = S_OK;

    //
    // Initialize scratch brush and pool managers
    //

    m_psbScratch = new CHwSolidBrush(pDevice);
    IFCOOM(m_psbScratch);

    m_pbpmGradientLinear = new CHwLinearGradientBrushPoolManager(pDevice);
    IFCOOM(m_pbpmGradientLinear);

    m_pbpmGradientRadial = new CHwRadialGradientBrushPoolManager(pDevice);
    IFCOOM(m_pbpmGradientRadial);

    m_pbbScratch = new CHwBitmapBrush(pDevice);
    IFCOOM(m_pbbScratch);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushPool::GetHwBrush
//
//  Synopsis:
//      Locate a hw brush for the given device independent brush
//
//------------------------------------------------------------------------------
HRESULT 
CHwBrushPool::GetHwBrush(
    __inout_ecount(1) CMILBrush *pBrush,
    __in_ecount(1) const CHwBrushContext &hwBrushContext,
    __deref_out_ecount(1) CHwBrush ** const ppHwBrush
    )
{
    HRESULT hr = S_OK;

    Assert(m_psbScratch);
    Assert(m_pbpmGradientLinear);
    Assert(m_pbpmGradientRadial);
    Assert(m_pbbScratch);

    switch (pBrush->GetType())
    {
    case BrushSolid:
        {
            const CMILBrushSolid *pSolidBrush = DYNCAST(const CMILBrushSolid, pBrush);
            Assert(pSolidBrush);

            m_psbScratch->SetColor(pSolidBrush->m_SolidColor);

            m_psbScratch->AddRef();
            *ppHwBrush = m_psbScratch;
        }
        break;

    case BrushGradientLinear:
        IFC(m_pbpmGradientLinear->AllocateHwBrush(
            pBrush,
            hwBrushContext,
            ppHwBrush
            ));
        break;

    case BrushGradientRadial:
        IFC(m_pbpmGradientRadial->AllocateHwBrush(
            pBrush,
            hwBrushContext,
            ppHwBrush
            ));
        break;

    case BrushBitmap:
        {
            IFC(m_pbbScratch->SetBrushAndContext(
                pBrush,
                hwBrushContext
                ));
            *ppHwBrush = m_pbbScratch;
        }
        break;

    default:
        *ppHwBrush = NULL;
        IFC(E_NOTIMPL);
        break;
    }

Cleanup:
    RRETURN(hr);
}






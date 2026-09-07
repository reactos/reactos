// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//
//    class CMilSlaveGlyphCache implementation.
//    See comments in glyphcacheslave.h.
//

#include "precomp.hpp"

MtDefine(CMilSlaveGlyphCache, MILRender, "CMilSlaveGlyphCache");

#define IFC_NO_REPORT(x) if (FAILED(hr = (x))) goto Cleanup

//+------------------------------------------------------------------------
//
//  Member:     CMilSlaveGlyphCache::CMilSlaveGlyphCache()
//
//  Synopsis:   Constructor
//
//-------------------------------------------------------------------------
CMilSlaveGlyphCache::CMilSlaveGlyphCache(__in CComposition *pComposition)
{ 
    // Allow for cache to expand up to 1MB of glyph bitmaps
    m_cMaximumBitmapStorageSize = 1000000;
    // Then trim to 800k
    m_cBitmapTargetSize = 800000;
    // After the oldest is 100 frames or more old
    m_cFrameDelayBeforeCleanup = 100;

    m_pComposition = pComposition;
}

//+------------------------------------------------------------------------
//
//  Member:     CMilSlaveGlyphCache::Create()
//
//  Synopsis:   Create
//
//-------------------------------------------------------------------------
HRESULT
CMilSlaveGlyphCache::Create(__in CComposition *pComposition, __out CMilSlaveGlyphCache **ppGlyphCache)
{
    HRESULT hr = S_OK;
    IUnknown *pIUnknown = NULL;

    CMilSlaveGlyphCache *pGlyphCache = new CMilSlaveGlyphCache(pComposition);
    IFCOOM(pGlyphCache);

    IFC(g_DWriteLoader.DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED, 
            __uuidof(IDWriteFactory),
            &pIUnknown
            ));

    IFC(pIUnknown->QueryInterface(
            __uuidof(IDWriteFactory),
            reinterpret_cast<void**>(&(pGlyphCache->m_pDWriteFactory))
            ));                

    *ppGlyphCache = pGlyphCache;
    pGlyphCache = NULL;

Cleanup:
    ReleaseInterface(pIUnknown);
    delete pGlyphCache;
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CMilSlaveGlyphCache::~CMilSlaveGlyphCache()
//
//  Synopsis:   Destructor
//
//-------------------------------------------------------------------------
CMilSlaveGlyphCache::~CMilSlaveGlyphCache()
{
    ReleaseInterface(m_pDWriteFactory);
}

//+------------------------------------------------------------------------
//
//  Member:     CMilSlaveGlyphCache::TrimCache
//
//  Synopsis:   Trims bitmaps from the cache according to LRU.
//
//-------------------------------------------------------------------------
void CMilSlaveGlyphCache::TrimCache()
{
    if (m_totalGlyphBitmapStorageSize > m_cMaximumBitmapStorageSize)
    {
        if ((!m_realizationListNoRef.IsEmpty()) && (static_cast<LONG>(GetCurrentRealizationFrame() - m_realizationListNoRef.PeekAtHead()->LastUsedFrame()) > static_cast<LONG>(m_cFrameDelayBeforeCleanup)))
        {
#ifdef DBG            
            INT32 debugTotalGlyphStorageSizeBefore = m_totalGlyphBitmapStorageSize;
#endif
            INT32 sizeToLose = m_totalGlyphBitmapStorageSize - m_cBitmapTargetSize;
            INT32 sizeLost = 0;

            //
            // New items are inserted at the tail, so start at the head
            //
            CGlyphRunRealization *pCurrent = m_realizationListNoRef.PeekAtHead();
                
            while ((sizeLost < sizeToLose) && (pCurrent != NULL) && (static_cast<LONG>(GetCurrentRealizationFrame() - pCurrent->LastUsedFrame()) > static_cast<LONG>(m_cFrameDelayBeforeCleanup)))
            {
                CGlyphRunRealization *pNext = NULL;
                pNext = m_realizationListNoRef.PeekNext(pCurrent);
                sizeLost += pCurrent->GetTextureSize();

                // DeleteAlphaMap also calls back into CMilSlaveGlyphCache and removes that realization
                // from the linked list, so we don't need to do it here.
                pCurrent->DeleteAlphaMap();
                pCurrent = pNext;
            }
#ifdef DBG
            // Confirm that we trimmed exactly sizeLost amount of bytes from the cache.
            Assert(debugTotalGlyphStorageSizeBefore - m_totalGlyphBitmapStorageSize == sizeLost);
#endif 

            
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CMilSlaveGlyphCache::ProcessPendingAnimations()
//
//  Synopsis:   Call NotifyOnChanged on CMilSlaveGlyphRuns that have
//              requested it in a previous rendering pass. This
//              mechanism is used to allow the glyphs to produce new
//              realizations when an animation stops.
//
//-------------------------------------------------------------------------
void 
CMilSlaveGlyphCache::ProcessPendingAnimations()
{
    UTC_TIME currentRealizationFrame = GetCurrentRealizationFrame();
    UINT i = m_animatingGlyphRunArray.GetCount();

    while (i > 0) 
    {
        
        i--; // because of loop condition i will be >= 0 after this statement executed.
        

        // If we have lots of animations this loop can still be n^2 and a linked list data structure
        // would be more appropriate.
       
        if (currentRealizationFrame >= m_animatingGlyphRunArray[i].RequestedCallbackFrame)
        {
            m_animatingGlyphRunArray[i].pAnimatingGlyphRunNoRef->AnimationTimeoutCallback();
            m_animatingGlyphRunArray.RemoveAt(i);
        }
    }        

    if (m_animatingGlyphRunArray.GetCount() > 0)
    {
        // Still need another frame
        m_pComposition->ScheduleCompositionPass();
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CMilSlaveGlyphCache::GetCurrentRealizationFrame
//
//  Synopsis:   Gets the current realization frame #.  This number grows
//              with each distinct composition frame where we perform
//              text realization work.  Used for lifetime information.
//
//-------------------------------------------------------------------------
UTC_TIME
CMilSlaveGlyphCache::GetCurrentRealizationFrame()
{
    UTC_TIME latestCompositionFrame = CComposition::GetFrameLastComposed();
    if (latestCompositionFrame != m_lastCompositionFrame)
    {
        // We should not have any rollover with 64-bit UTC_TIME counter
        Assert(latestCompositionFrame > m_lastCompositionFrame);

        m_lastCompositionFrame = latestCompositionFrame;
        m_currentRealizationFrame++;
    }

    return m_currentRealizationFrame;
}

//+------------------------------------------------------------------------
//
//  Member:     CMilSlaveGlyphCache::RequestSubsequentPass
//
//  Synopsis:   Mechanism for CMilSlaveGlyphRun objects to ask for another 
//              rendering pass, to allow them to update their own realizations.
//
//-------------------------------------------------------------------------

HRESULT 
CMilSlaveGlyphCache::RequestSubsequentPass(__in CGlyphRunResource *pGlyphRunResource)
{
    HRESULT hr = S_OK;
    UINT index = FindAnimatingGlyphRunIndex(pGlyphRunResource);

    // Find old entry if it's in the callback request list already
    if (index != UINT_MAX)
    {
        m_animatingGlyphRunArray[index].RequestedCallbackFrame = GetCurrentRealizationFrame() + CGlyphRunResource::c_frameCountBeforeRealizationCallback;
    }
    else
    {
        AnimatingGlyphRunCallbackRequest newRequest;
        newRequest.RequestedCallbackFrame = GetCurrentRealizationFrame() + CGlyphRunResource::c_frameCountBeforeRealizationCallback;
        newRequest.pAnimatingGlyphRunNoRef = pGlyphRunResource;
        IFC(m_animatingGlyphRunArray.Add(newRequest));
    }
    m_pComposition->ScheduleCompositionPass();

Cleanup:
    RRETURN(hr);    
}

//+------------------------------------------------------------------------
//
//  Member:     CMilSlaveGlyphCache::UnRegisterForSubsequentPass
//
//  Sunopsis:   Method to unregister a CMilSlaveGlyphRun that has previously called
//              RequestSubsequentPass, in case it is destroyed before it gets the
//              chance to produce a new realization
//
//-------------------------------------------------------------------------
void
CMilSlaveGlyphCache::UnRegisterForSubsequentPass(__in const CGlyphRunResource *pGlyphRunResource)
{
    UINT index = FindAnimatingGlyphRunIndex(pGlyphRunResource);
    if (index != UINT_MAX)
    {
#ifdef DBG
        HRESULT hr = 
#endif
            m_animatingGlyphRunArray.RemoveAt(index);

#ifdef DBG
        AssertMsg(SUCCEEDED(hr), "Currently RemoveAt only fails if we pass an invalid index which we should never do.");
#endif
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CMilSlaveGlyphCache::AddRealization
//
//  Sunopsis:   Add a realization to the GlyphCache list for size tracking
//              It always gets added at the tail of the list for LRU management
//
//-------------------------------------------------------------------------
void 
CMilSlaveGlyphCache::AddRealization(__in CGlyphRunRealization *pRealization, UINT32 textureSize)
{
    m_totalGlyphBitmapStorageSize += textureSize;

    // This realization should not be in the list already
    Assert(pRealization->Flink == NULL);
    Assert(pRealization->Blink == NULL);
    m_realizationListNoRef.InsertAtTail(pRealization);
    Assert(pRealization->Flink != NULL);
    Assert(pRealization->Blink != NULL);
}

//+------------------------------------------------------------------------
//
//  Member:     CMilSlaveGlyphCache::RemoveRealization
//
//  Sunopsis:   Remove a realization to the GlyphCache list 
//
//-------------------------------------------------------------------------
void
CMilSlaveGlyphCache::RemoveRealization(__in CGlyphRunRealization *pRealization, UINT32 textureSize)
{
    if (pRealization)
    {
        m_totalGlyphBitmapStorageSize -= textureSize;
        Assert(m_totalGlyphBitmapStorageSize >= 0);
        m_realizationListNoRef.RemoveFromList(pRealization);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CMilSlaveGlyphCache::FindAnimatingGlyphRunIndex
//
//  Sunopsis:   Finds the index of a CGlyphRunResource in the animating glyph
//              runs array. 
//
//-------------------------------------------------------------------------
UINT
CMilSlaveGlyphCache::FindAnimatingGlyphRunIndex(__in const CGlyphRunResource *pGlyphRunResource) const
{
    for (UINT i = 0; i < m_animatingGlyphRunArray.GetCount(); i++)
    {   
        if (m_animatingGlyphRunArray[i].pAnimatingGlyphRunNoRef == pGlyphRunResource)
        {
            return i;
        }
    }
    return UINT_MAX;
} 



// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//
//    Originally stored bitmaps for individual glyphs, now this class
//    holds onto realizations which own bitmaps for entire glyph runs, 
//    and this class remembers their sizes, and if necessary
//    walks through them and trims bitmaps.
//
//------------------------------------------------------------------------

#pragma once

MtExtern(CMilSlaveGlyphCache);

class CMilSlaveGlyphCache
{
   
public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilSlaveGlyphCache));

    //
    // Construction/Destruction
    //
    static HRESULT Create(__in CComposition *pComposition, __out CMilSlaveGlyphCache **ppGlyphCache);
    ~CMilSlaveGlyphCache();
 
    //
    // Cache lifetime management
    //
    void TrimCache();

    void ValidateCache()
    {
        m_realizationListNoRef.ValidateList();
    }

    //
    // Walks through a list of CMilSlaveGlyphRun resources that are currently
    // in an animation state, and marks them dirty so that they can get another
    // rendering pass. This is required for when the animation stops, so that
    // they can produce a high quality realization in place of the animation 
    // quality one.
    //
    void ProcessPendingAnimations();

    UINT FindAnimatingGlyphRunIndex(__in const CGlyphRunResource *pGlyphRunResource) const;
    
    //
    // Get the current unique realization frame count
    //
    UTC_TIME GetCurrentRealizationFrame();    

    //
    // Mechanism for CMilSlaveGlyphRun objects to ask for another rendering
    // pass, to allow them to update their own realizations.
    //
    HRESULT RequestSubsequentPass(__in CGlyphRunResource *pGlyphRunResource);

    //
    // Method to unregister a CMilSlaveGlyphRun that has previously called
    // RequestSubsequentPass, in case it is destroyed before it gets the
    // chance to produce a new realization
    //
    void UnRegisterForSubsequentPass(__in const CGlyphRunResource *pGlyphRunResource);

    __out IDWriteFactory *GetDWriteFactoryNoRef()
    {
        return m_pDWriteFactory;        
    }

    void AddRealization(__in CGlyphRunRealization *pRealization, UINT32 textureSize);
    void RemoveRealization(__in CGlyphRunRealization *pRealization, UINT32 textureSize);
        
    static const UINT c_invalidHandleValue = (FontFaceHandle)(-1);

private:
    CMilSlaveGlyphCache(__in CComposition *pComposition);

    typedef struct {
        UTC_TIME RequestedCallbackFrame;
        CGlyphRunResource *pAnimatingGlyphRunNoRef;
    } AnimatingGlyphRunCallbackRequest;

    DynArray<AnimatingGlyphRunCallbackRequest> m_animatingGlyphRunArray;

    CComposition *m_pComposition;

    CDoubleLinkedList<CGlyphRunRealization> m_realizationListNoRef;  // Doubly-linked threaded list of bitmaps sorted by last access time
    
    INT32 m_totalGlyphBitmapStorageSize;

    // If glyph storage exceeds cMaximumBitmapStorageSize we'll trigger cleanup
    INT32 m_cMaximumBitmapStorageSize;
    // We'll keep cleaning up until we hit cBitmapTargetSize
    INT32 m_cBitmapTargetSize;

    // If we exceed the maximum storage size, if the delta between the current frame
    // and the oldest frame is less than this amount, we still won't cleanup.
    INT32 m_cFrameDelayBeforeCleanup;
    
    UTC_TIME m_lastCompositionFrame;     // For lifetime management: increments each time we compose
    UTC_TIME m_currentRealizationFrame;  // Increments each time we compose AND process realizations

    IDWriteFactory *m_pDWriteFactory;    
};


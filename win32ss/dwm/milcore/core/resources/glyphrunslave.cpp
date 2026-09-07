// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Classes:
//      CGlyphRunResource
//      CGlyphRunRealization
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CGlyphRunResource, MILRender, "CGlyphRunResource");
MtDefine(CGlyphRunRealization, CGlyphRunResource, "CGlyphRunRealization");

MtDefine(GlyphBitmapClearType, CGlyphRunRealization, "Glyph bitmap allocation: ClearType");
MtDefine(GlyphBitmapBiLevel, CGlyphRunRealization, "Glyph bitmap allocation: BiLevel");

MtDefine(GlyphBlendingParameters, CDisplaySet, "GlyphBlendingParameters"); 

const double CGlyphRunResource::c_minAnimationDetectionBar = 0.9;

#pragma warning ( default: 4700 )

//
// The time between composition passes that we request if there's no other work to do
// is a wait for approximately ~16ms. So 3 frames worth is about 48ms, a reasonable time
// to wait to see if an animation has terminated. Previously, when text animation completed 
// there was a noticeable delay before we snapped in high quality realizations, approximately
// 400ms. 50ms is less noticeable.
//
const int CGlyphRunResource::c_frameCountBeforeRealizationCallback = 2;

const int CGlyphRunResource::c_frameCountBeforeAnimationRealizationStale = 10;
const int CGlyphRunResource::c_frameCountBeforeDeleteHighQualityRealization = 20;

const int CGlyphRunResource::c_scaleGridSize = 7;

const float CGlyphRunResource::scaleGrid[c_scaleGridSize] = { 5.0f, 6.5f, 9.1f, 13.7f, 21.8f, 37.1f, 66.8f };


CGlyphRunResource::CDWriteFontFaceCache::FontFaceCacheEntry CGlyphRunResource::CDWriteFontFaceCache::m_cache[c_cacheSize] = { 0 };

UINT32 CGlyphRunResource::CDWriteFontFaceCache::m_cacheMRU = 0;

LONG CGlyphRunResource::CDWriteFontFaceCache::m_mutex = 0;

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::CGlyphRunResource
//
//  Synopsis:
//      Constructor
//
//  Arguments:
//      pHandleTable - which handle table should own this resource
//      hres - handle value for this resource
//
//------------------------------------------------------------------------------
CGlyphRunResource::CGlyphRunResource(__in_ecount(1) CComposition *pDevice)
{
    // DECLARE_METERHEAP_CLEAR zeroed everything
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::DeleteRealizationInArray
//
//  Synopsis:
//      Deletes all the realizations in the array and resets the array
//
//------------------------------------------------------------------------------
void 
CGlyphRunResource::DeleteRealizationInArray(__in DynArrayIA <CGlyphRunRealization*, 2> *pArray)
{
    UINT count = pArray->GetCount();
    
    for (UINT h = 0; h < count; h++)
    {
        CGlyphRunRealization* pRealization = (*pArray)[h];
        
        ReleaseInterface(pRealization);
    }       
    pArray->Reset(FALSE);
    
    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::~CGlyphRunResource()
//
//  Synopsis:
//      Destructor
//
//------------------------------------------------------------------------------
CGlyphRunResource::~CGlyphRunResource()
{    
    DeleteRealizationInArray(&m_prgHighQualityRealizationArray);
    DeleteRealizationInArray(&m_prgAnimationQualityRealizationArray);   
    DeleteRealizationInArray(&m_prgBiLevelRealizationArray);      

    m_pGlyphCache->UnRegisterForSubsequentPass(this);

    WPFFree(ProcessHeap, m_pGlyphBlendingParameters);
    m_pGlyphBlendingParameters = NULL;

    ReleaseInterface(m_pGeometry);
    m_pGlyphCache = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::Disable()
//
//  Synopsis:
//      Disables bitmap rendering by removing all the realizations. This routine
//      is called in responce to MILCMD_CHANNEL_DELETERESOURCE, as a workaround
//      for lifetime issues. See comment in
//      CComposition::ProcessDeleteResourceOnChannel().
//
//------------------------------------------------------------------------------
void
CGlyphRunResource::Disable()
{
    DeleteRealizationInArray(&m_prgHighQualityRealizationArray);
    DeleteRealizationInArray(&m_prgAnimationQualityRealizationArray);
    DeleteRealizationInArray(&m_prgBiLevelRealizationArray);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::ProcessCreate
//
//  Synopsis:
//      Execute the MILCMD_GLYPHRUN_CREATE command.
//
//------------------------------------------------------------------------------

HRESULT
CGlyphRunResource::ProcessCreate(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_GLYPHRUN_CREATE* pCmd,
    __in_bcount_opt(cbPayload) LPCVOID pPayload,
    UINT cbPayload
    )
{
    HRESULT hr = S_OK;

    //TODO: make InitStorage accept the pCmd/pPayload/cbPayload triplet
    IFC(InitStorage(
        pCmd,
        sizeof(MILCMD_GLYPHRUN_CREATE) + cbPayload
        ));

    m_pGlyphCache = pHandleTable->GetComposition()->GetGlyphCache();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::GetAvailableScale
//
//  Synopsis:
//      Render servicing. For a given pair of scale ratios, find the realization
//      that matches them best. If there isn't one with a sufficient quality
//      ratio, create one.
//
//  Notes:
//      This algorithm is relatively complex. Because of the way we handle 
//      text animation, and the complete lack of animation context at this low level
//      we have to use a lot of heuristics to determine when glyphs are animating or not
//      This ends up in the complex code here, and the worst case complexity is worse
//      than ideal. The optimization this gives by avoiding excessive requests for
//      the text rasterizer to produce new realizations is worth the cost, however.
//
//  Returns:
//      'true' on success, 'false' if no realizations are available and creation fails.
//
//------------------------------------------------------------------------------
bool
CGlyphRunResource::GetAvailableScale(
    __inout_ecount(1) float* pScaleX,
    __inout_ecount(1) float* pScaleY,
    __in_ecount(1) const DisplaySettings *pDisplaySettings,
    MilTextRenderingMode::Enum textRenderingMode,
    MilTextHintingMode::Enum textHintingMode,
    __out RenderingMode *pRecommendedRenderingMode,
    __out_ecount(1) CGlyphRunRealization** ppRealization,
    IDpiProvider const* pDpiProvider
    )
{
    HRESULT hr = S_OK;
 
    // Animation realization selection code    
    float requestedScaleX = *pScaleX;
    float requestedScaleY = *pScaleY;
    *ppRealization = NULL;

    CGlyphRunRealization *pRealization = NULL;

    bool fCreateNewRealization = false;
    bool fAnimationQuality = false;
 
    float realizationScaleX = FLOAT_QNAN;
    float realizationScaleY = FLOAT_QNAN;

    UTC_TIME currentRealizationFrame = m_pGlyphCache->GetCurrentRealizationFrame();

    Assert(pScaleX);
    Assert(pScaleY);
    Assert(ppRealization);

    if (!(*pScaleY > 0) || !(*pScaleX > 0))
    {
        IFC(WGXERR_INVALIDPARAMETER);  // We should only add realizations for scales below Geometry threshold
    }

    double matchScoreHighQuality = 0;
    bool fFoundExactMatch = false;
    bool fFoundMatch = false;
    UINT foundIndexHighQuality;
    UINT foundIndexAnimationQuality;

    RenderingMode blendMode;
    GetBlendMode(textRenderingMode, pDisplaySettings->DisplayRenderingMode, &blendMode);

    if (m_measuringMethod == DWRITE_MEASURING_MODE_NATURAL)
    {           
        if (textHintingMode == MilTextHintingMode::Fixed)
        {

            FindMatchingRealization(&m_prgHighQualityRealizationArray,
                                    false,  // match actual realization sizes
                                    requestedScaleX,
                                    requestedScaleY,
                                    &matchScoreHighQuality,
                                    &fFoundExactMatch,
                                    &fFoundMatch,
                                    &foundIndexHighQuality
                                    );


            if (!fFoundExactMatch)
            {
                fCreateNewRealization = true;
                fAnimationQuality = false;        
                realizationScaleX = requestedScaleX;
                realizationScaleY = requestedScaleY;
            }
            else
            {
                pRealization = m_prgHighQualityRealizationArray.At(foundIndexHighQuality);
                pRealization->AddRef();
            }
        }                       
        else if (textHintingMode == MilTextHintingMode::Animated)
        {
            float snappedScaleX = SnapToScaleGrid(requestedScaleX);
            float snappedScaleY = SnapToScaleGrid(requestedScaleY);                

            FindMatchingRealization(&m_prgAnimationQualityRealizationArray,
                                    false,  // match actual realization sizes
                                    snappedScaleX,
                                    snappedScaleY,
                                    &matchScoreHighQuality,
                                    &fFoundExactMatch,
                                    &fFoundMatch,
                                    &foundIndexAnimationQuality
                                    );

            if (!fFoundExactMatch)
            {
                fCreateNewRealization = true;
                fAnimationQuality = true;
                realizationScaleX = snappedScaleX;
                realizationScaleY = snappedScaleY;                
            }
            else
            {
                pRealization = m_prgAnimationQualityRealizationArray.At(foundIndexAnimationQuality);
                pRealization->AddRef();            
            }
        }
        else if (textHintingMode == MilTextHintingMode::Auto)
        {
            //
            // Pseudocode:
            //
            // 1. Get cached version of font file from glyphcache (currently not complete - no caching)
            // 2. Get cached font face (based on font file) (currently partially implemented)
            // 3. If cache font file and font face, is there a current analysis?
            //  - Yes: 4. Is it the right size?
            //     - Yes: 5. Does it have bitmaps?
            //         - Yes: Use, end
            //         - No: Request bitmaps (that were deleted by glyph cache trimming), store, Use, end
            //     - No: 6. Delete it, create a new one, go to 5. 
            //  - No: Go to 6.
            //

            //
            // Assumptions:
            // 1. The first time a glyphrun is displayed in a particular location,
            //    it is displayed as a high quality realization, even if it is 
            //    beginning an animation, because these two cases are indistinguishable
            //    at this point in the system (no knowledge of animations here).
            //
            //        
            // Initial search - look for an exact match high quality realization

            FindMatchingRealization(&m_prgHighQualityRealizationArray,
                                        false,  // match actual realization sizes
                                        requestedScaleX,
                                        requestedScaleY,
                                        &matchScoreHighQuality,
                                        &fFoundExactMatch,
                                        &fFoundMatch,
                                        &foundIndexHighQuality
                                        );

            if (fFoundMatch)
            {
                //
                // We're using this realization as our basis for comparison, even if we end up using an animation quality
                // realization, set this one to recently used so we don't go clean it up.
                //
                (m_prgHighQualityRealizationArray.At(foundIndexHighQuality))->UpdateLastUsedFrame();

                if (fFoundExactMatch)
                {
                    //
                    // Found a perfect match.
                    // This is either:
                    // 1. Non-animating text that has been previously realized
                    // 2. Animating text that is passing through the same scale as
                    //    text that has been previously produced for case 1.
                    //
                    // Since these two cases are indistinguishable, we routinely purge high
                    // quality realizations (in PurgeOldEntries) to ensure that a large number of 
                    // them are not present to cause animations to skip and jump regularly. 
                    // Further mitigating this factor is that even for a repeating scale animation which
                    // terminates once halfway through its operation, at say 20.5, if the animation
                    // repeats, it is relatively rare that the animation will pass exactly through 
                    // 20.5 as it continues further in the next iteration.
                    //
                    // The artifact that will occur when we misuse a high quality realization is a slight
                    // snapping to and away from that realization. This is not very noticeable unless it happens
                    // often. Indeed even in the regular case, there is slight snapping when we cross animation
                    // bands and have to re-realize at animation quality.
                    //
                    // More heuristics could be added to make this problem occur less frequently, but I believe
                    // what we have here is sufficient. More heuristics would further complicate the code, and
                    // would never be a complete solution as long as we allow graphness in the visual tree
                    // and have no context about animations during this realization step.
                    //

                    // Use this realization
                    fCreateNewRealization = false;
                    fAnimationQuality = false;

                    pRealization = m_prgHighQualityRealizationArray.At(foundIndexHighQuality);
                    pRealization->AddRef();

                    TraceTagText((tagMILWarning,
                      "CGlyphRunSlave::GetAvailableScale(): Found exact high quality match, using it"));
                }
                else
                {   
                    //
                    // Haven't found a perfect high quality match. This means either:
                    // 1. This is a new static display at this particular size and no realization has
                    //    yet been created
                    // 2. This glyph run is animating its scale.
                    //
                    // We assume 1 defaultly. 
                    // a. If the bestQuality score result from searching the high quality realization array 
                    //    is above a certain threshold, we assume 2, assuming that this indicates that
                    //    a glyphrun has just started animating (away from that close match. Thus if a realization 
                    //    is requested for a static glyphrun for which there is a close match it will appear as an animated
                    //    quality realization temporarily, before snapping into place in a few frames.
                    //
                    // b. If there is an animation quality realization whose 'last fulfilled scale' is within
                    //    a certain threshold, but not the same (indicating the animation has terminated),
                    //    we will also assume 2, taking this to mean that the scale is changing slightly. 
                    //    We then use the same animation quality realization and update its 'last fulfilled scale'
                    //    We must take care when the boundary snapping occurs. 
                    //

                    TraceTagText((tagMILWarning,
                      "CGlyphRunSlave::GetAvailableScale(): No exact high quality match"));

                    //
                    // Is the high quality a close match? 
                    // This is a precursor to any animation quality detection due to assumption 1
                    // listed above. 
                    //
                    if (matchScoreHighQuality > c_minAnimationDetectionBar)                
                    {
                        fAnimationQuality = true;

                        float snappedScaleX = SnapToScaleGrid(requestedScaleX);
                        float snappedScaleY = SnapToScaleGrid(requestedScaleY);                
                        
                        //
                        // Is the last fulfilled scale of a close animation quality realization within a certain
                        // threshold?
                        // This is case b.
                        //
                        double matchScoreAnimationQuality;
                        UINT foundIndexAnimationQuality;

                        FindMatchingRealization(
                            &m_prgAnimationQualityRealizationArray,
                            true,   // match last fulfilled scale
                            requestedScaleX,
                            requestedScaleY,
                            &matchScoreAnimationQuality,
                            &fFoundExactMatch,
                            &fFoundMatch,
                            &foundIndexAnimationQuality
                            );

                        if (!fFoundMatch)
                        {
                            //
                            // Found no animation quality realizations. We've already decided we want animation quality
                            // so create a new one
                            //
                            fCreateNewRealization = true;

                            realizationScaleX = snappedScaleX;
                            realizationScaleY = snappedScaleY;                   

                            TraceTagText((tagMILWarning,
                                  "CGlyphRunSlave::GetAvailableScale(): No animation quality match"));
                        }
                        else
                        {
                            // Have an animation quality realization, check its exactness and age
                            if (fFoundExactMatch &&
                                ((currentRealizationFrame - m_prgAnimationQualityRealizationArray.At(foundIndexAnimationQuality)->LastUsedFrame()) < c_frameCountBeforeAnimationRealizationStale))
                            {
                                TraceTagText((tagMILWarning,
                                  "CGlyphRunSlave::GetAvailableScale(): Found exact animation quality match that is recently used: animation terminating"));
                                //
                                // Reasons we may have an exact match:
                                // 1. We just used this animation quality realization last frame for the same
                                //    realization scale, indicating the animation has terminated.
                                // 2. It's an old realization that happens to have the same last fulfilled scale
                                //    that we are searching for.
                                //
                                // 2 is an edge case not worth optimizing for, so we combine the check for 1 with fFoundExactMatch
                                //
                                // Realization is recently used. This means we used it last frame and this animation has stopped.
                                fCreateNewRealization = true;
                                fAnimationQuality = false;
                                realizationScaleX = requestedScaleX;
                                realizationScaleY = requestedScaleY;
                            }
                            else
                            {                        
                                TraceTagText((tagMILWarning,
                                      "CGlyphRunSlave::GetAvailableScale(): Found inexact animation quality match"));

                                //
                                // Found inexact match, or exact match that is old. 
                                // No information on how inexact it is, and the match is to the last fulfilled scale,
                                // not the actual realized scale which we need to use (else we'd be stuck using the same
                                // animation quality realization forever).
                                //

                                FindMatchingRealization(
                                    &m_prgAnimationQualityRealizationArray,
                                    false,   // match realized size
                                    snappedScaleX,
                                    snappedScaleY,
                                    &matchScoreAnimationQuality,
                                    &fFoundExactMatch,
                                    &fFoundMatch,
                                    &foundIndexAnimationQuality
                                    );

                                Assert(fFoundMatch);
                                
                                if (fFoundExactMatch)
                                {
                                    // Already have the realization grid we want. Use it.
                                    fCreateNewRealization = false;
                                    fAnimationQuality = true;
                                    pRealization = m_prgAnimationQualityRealizationArray.At(foundIndexAnimationQuality);
                                    pRealization->AddRef();
                                }
                                else
                                {
                                    // Need a new one
                                    fCreateNewRealization = true;
                                    fAnimationQuality = true;
                                    realizationScaleX = snappedScaleX;
                                    realizationScaleY = snappedScaleY;
                                }
                            }
                        }
                    }
                    else
                    {
                        //
                        // Not animating - too far from a high quality realization
                        // If we are actually animating, then the next time around we'll find
                        // this high quality realization and produce an animation quality one
                        //
                        fCreateNewRealization = true;
                        fAnimationQuality = false;
                        realizationScaleX = requestedScaleX;
                        realizationScaleY = requestedScaleY;
                    }
                }
            }
            else
            {
                //
                // No realization found. Can't be animating if we have no previous
                // realizations (see assumption 1)
                //
                // Create a new one at full quality.
                //
                TraceTagText((tagMILWarning,
                      "CGlyphRunSlave::GetAvailableScale(): Found no high quality entries, creating new full quality realization"));
                fCreateNewRealization = true;
                realizationScaleX = requestedScaleX;
                realizationScaleY = requestedScaleY;                            
                fAnimationQuality = false;    
            }
                
            //
            // Do registration. Unregistration happens automatically and should only be used in case of 
            // object destruction. There's no way to know here if the same glyphrun at other scales hasn't
            // requested animation callbacks, so if we decided this particular instance didn't need them
            // and unregistered ourselves, we could potentially leave them dangling with an animation quality
            // realization after their animation had completed.
            //
            if (fAnimationQuality)
            {
                IFC(m_pGlyphCache->RequestSubsequentPass(this));
            }

            TraceTagText((tagMILWarning,
                          "CGlyphRunSlave::GetAvailableScale(): fCNR: %d, fAnimQ: %d, pX: %f, pY: %f, rX: %f, rY: %f",
                          fCreateNewRealization,
                          fAnimationQuality,
                          *pScaleX,
                          *pScaleY,
                          realizationScaleX,
                          realizationScaleY
                          ));
        }
    }
    else if (m_measuringMethod == DWRITE_MEASURING_MODE_GDI_CLASSIC)
    {
        const auto& primaryDisplayDpi = DpiScale::PrimaryDisplayDpi();

        // 
        // Since we ignore requestedScaleX and requestedScaleY which contain the DPI transform for
        // any high DPI modes, we need to add the DPI transform manually.
        //
        float dpiAdjustedRealizationScaleX = m_muSize * primaryDisplayDpi.DpiScaleX;
        float dpiAdjustedRealizationScaleY = m_muSize * primaryDisplayDpi.DpiScaleY;
        
        if (pDpiProvider != nullptr && pDpiProvider->IsPerMonitorDpiAware())
        {
            DpiScale dpi;
            if (SUCCEEDED(pDpiProvider->GetCurrentDpi(&dpi)))
            {
                dpiAdjustedRealizationScaleX = m_muSize * dpi.DpiScaleX;
                dpiAdjustedRealizationScaleY = m_muSize * dpi.DpiScaleY;
            }
        }
        
        DynArrayIA <CGlyphRunRealization*, 2> *pRealizationArray = (blendMode == BiLevel) ? 
                                                                        &m_prgBiLevelRealizationArray : 
                                                                        &m_prgHighQualityRealizationArray;
        
        FindMatchingRealization(pRealizationArray,
                                false,  // match actual realization sizes
                                dpiAdjustedRealizationScaleX,
                                dpiAdjustedRealizationScaleY,
                                &matchScoreHighQuality,
                                &fFoundExactMatch,
                                &fFoundMatch,
                                &foundIndexHighQuality
                                );

        if (!fFoundExactMatch)
        {
            fCreateNewRealization = true;
            fAnimationQuality = false;        
            realizationScaleX = dpiAdjustedRealizationScaleX;
            realizationScaleY = dpiAdjustedRealizationScaleY;
        }
        else
        {
            pRealization = pRealizationArray->At(foundIndexHighQuality);
            pRealization->AddRef();
        }                
    }
    else
    {
        // Should not be here. We don't support DWRITE_MEASURING_MODE_GDI_NATURAL.
        DebugBreak();
    }

    //
    // If we're creating new realizations, realizationScaleX and realizationScaleY should not have
    // been touched.
    // If we're not creating them, they should have been.
    //
    Assert(!fCreateNewRealization || ((realizationScaleX != FLOAT_QNAN) && (realizationScaleY != FLOAT_QNAN)));

    // If we're not creating a realization, we need to have set one
    Assert(fCreateNewRealization || (pRealization != NULL && 
                                     *reinterpret_cast<UINT*>(&realizationScaleX) == FLOAT_QNAN_UINT && 
                                     *reinterpret_cast<UINT*>(&realizationScaleY) == FLOAT_QNAN_UINT));


    if (fCreateNewRealization)
    {        
        Assert( (realizationScaleX > 0) && (realizationScaleY > 0) );

        IFC(CreateRealization(realizationScaleX,
                              realizationScaleY,
                              fAnimationQuality,
                              (blendMode == BiLevel),
                              pDisplaySettings,
                              textRenderingMode,
                              &pRealization
                              ));
    }
    else  // We already have a realization, but this does not guarantee we have all the bitmaps
    {        
        if (pRealization->IsAnimationQuality())
        {
            pRealization->SetLastFulfilledScale(requestedScaleX, requestedScaleY);
        }
        else
        {
            Assert(!fAnimationQuality);
        }

        //TraceTagText((tagMILWarning, "CGlyphRunSlave::GetAvailableScale(): Existing realization faceHandle = %d", faceHandle));
    }

    Assert(pRealization);

    pRealization->UpdateLastUsedFrame();

    if (m_pGlyphBlendingParameters == NULL)
    {
        m_pGlyphBlendingParameters = WPFAllocType(GlyphBlendingParameters *,
                                                  ProcessHeap,
                                                  Mt(GlyphBlendingParameters),
                                                  sizeof(GlyphBlendingParameters)
                                                  );
        IFCOOM(m_pGlyphBlendingParameters);
        
        IFC(CDisplaySet::CompileSettings(pDisplaySettings->pIDWriteRenderingParams, pDisplaySettings->PixelStructure, pRealization->GetAnalysisNoRef(), m_pGlyphBlendingParameters));
    }

    if (!pRealization->HasAlphaMaps())
    {
        EnhancedContrastTable *pECT = NULL;
        IFC(GetEnhancedContrastTable(m_pGlyphBlendingParameters->ContrastEnhanceFactor, &pECT));
        IFC(pRealization->EnsureValidAlphaMap(pECT));
    }

    *pScaleX = pRealization->GetScaleX();
    *pScaleY = pRealization->GetScaleY();

    *ppRealization = pRealization;
    pRealization = NULL;

    //
    // Aggressively delete old high quality realizations. This prevents us from building up a bunch of
    // high quality realizations at various scales that may then get picked up by scale animations 
    // as they pass through the same value. Since we do exact matching, matches don't happen that often,
    // but the problem could be compounded if we realize this glyph run at a lot of different scales.
    //
    // Delete all high quality realizations older than 20 realization frames
    //
    PurgeOldEntries(&m_prgHighQualityRealizationArray);
    
    TraceTagText((tagMILWarning,
                  "CGlyphRunSlave::GetAvailableScale(): fCNR: %d, fAnimQ: %d, pX: %f, pY: %f, rsX: %f, rsY: %f, Ptr: %p",
                  fCreateNewRealization,
                  fAnimationQuality,
                  *pScaleX,
                  *pScaleY,
                  requestedScaleX,
                  requestedScaleY,
                  *ppRealization
                  ));

    *pRecommendedRenderingMode = blendMode;

Cleanup:
    ReleaseInterface(pRealization);
    
    return SUCCEEDED(hr);
}

//============================================================================================================================
// 
// Determines what blending we want to use for this glyph bitmap given the WPF user requested rendering mode, Windows display 
// settings, and how this glyphrun was measured.
//
//============================================================================================================================

void 
CGlyphRunResource::GetBlendMode(MilTextRenderingMode::Enum textRenderingMode,
                                RenderingMode displayRenderingMode,
                                __out RenderingMode *pRecommendedBlendMode
                                )
{
    if (textRenderingMode == MilTextRenderingMode::Auto)
    {
        // Use display settings
        switch (displayRenderingMode)
        {
            case BiLevel:
                *pRecommendedBlendMode = (m_measuringMethod == DWRITE_MEASURING_MODE_NATURAL) ?
                                            // Can't display bilevel text with naturally measured text. Use closest available thing
                                            // instead, which is grayscale.
                                            Grayscale :
                                            BiLevel;
                return;                                            
            case Grayscale:
                *pRecommendedBlendMode = Grayscale;
                return;
            case ClearType:
            default:                
                *pRecommendedBlendMode = ClearType;
                return;
        }
    }
    // WPF developer settings override system text display settings
    switch (textRenderingMode)
    {
        case MilTextRenderingMode::Aliased:
            *pRecommendedBlendMode = (m_measuringMethod == DWRITE_MEASURING_MODE_NATURAL) ?
                                        // Can't display bilevel text with naturally measured text. Use closest available thing
                                        // instead, which is grayscale.
                                        Grayscale :
                                        BiLevel;
            return;
        case MilTextRenderingMode::Grayscale:
            *pRecommendedBlendMode = Grayscale;
            return;
        case MilTextRenderingMode::ClearType:
        default:
            *pRecommendedBlendMode = ClearType;
            return;
    }
}


//============================================================================================================================

HRESULT
CGlyphRunResource::CreateRealization(
    float scaleX,
    float scaleY,
    bool fAnimationQuality,
    bool fBiLevelRequested,
    __in const DisplaySettings *pDisplaySettings,
    MilTextRenderingMode::Enum textRenderingMode,
    __out CGlyphRunRealization **ppRealization
    )                    
{      
    HRESULT hr = S_OK;
    CGlyphRunRealization *pRealization = NULL;
    IDWriteGlyphRunAnalysis *pIDWriteGlyphRunAnalysis = NULL;
    IDWriteFontFace *pIDWriteFontFace = NULL;

    DWRITE_MATRIX scaleTransform;
    memset(&scaleTransform, 0, sizeof(DWRITE_MATRIX));

    // If we are creating a full quality realization, just set the scale transform.
    scaleTransform.m11 = scaleX / m_muSize;
    scaleTransform.m22 = scaleY / m_muSize;

    if (fAnimationQuality)
    {
        //
        // If we are creating an animation-quality realization, we need to disable hinting.
        // Since DWrite does not expose any direct means for doing this, we'll take advantage
        // of a trick that is guaranteed to be supported.  If we pass in a transform with a 
        // rotation component to their rasterizer, DWrite will disable hinting.  We can apply
        // a rotation small enough that it will disable hinting but will not be visible upon
        // rendering.
        //
        // DWrite converts the transform to fixed point and multiplies it by pixelsPerDip
        // and fontSize, so in order to disable hinting our transform must satisfy:
        // For all i,j
        // abs(transform[i,j]) > 1 / (pixelsPerDip * fontEmSize * 2^16)
        //
        
        // We multiply by 2 to be safe from rounding errors.  pixelsPerDip is hardcoded to 1.
        // We only need to set these two entries since the other two were set by the scale render
        // transform.
        float rotationComponent = 2 / (m_muSize * 65536); // 65526 == 2^16
        scaleTransform.m12 = scaleTransform.m21 = rotationComponent;
    }

    IFC(CDWriteFontFaceCache::GetFontFace(m_pIDWriteFont, &pIDWriteFontFace));
    
    DWRITE_GLYPH_RUN glyphRun;
    glyphRun.fontFace = pIDWriteFontFace;
    glyphRun.fontEmSize = m_muSize;
    glyphRun.glyphCount = m_usGlyphCount;
    glyphRun.glyphIndices = m_pGlyphIndices;
    glyphRun.glyphAdvances = m_pGlyphAdvances;

    //
    // glyphOffsets is an array of 
    // DWRITE_GLYPH_OFFSET, which is
    // defined as:
    //
    // struct DWRITE_GLYPH_OFFSET
    // {
    //    FLOAT advanceOffset;
    //    FLOAT ascenderOffset;
    // };
    //
    // Assert that this is always true so
    // that we don't have to remarshall all
    // our offset data, and can assume that
    // our encoding of [X0, Y0, X1, Y1, ...] 
    // matches an array of DWRITE_GLYPH_OFFSET
    //
    C_ASSERT(FIELD_OFFSET(DWRITE_GLYPH_OFFSET, advanceOffset) == 0);
    C_ASSERT(FIELD_OFFSET(DWRITE_GLYPH_OFFSET, ascenderOffset) == 4);

    glyphRun.glyphOffsets = reinterpret_cast<const DWRITE_GLYPH_OFFSET *>(m_pGlyphOffsets);

    glyphRun.bidiLevel = m_bidiLevel;
    glyphRun.isSideways = IsSideways();

    IDWriteFactory *pIDWriteFactoryNoRef = m_pGlyphCache->GetDWriteFactoryNoRef();
    Assert(pIDWriteFactoryNoRef != NULL);

    DWRITE_RENDERING_MODE dwriteRenderingMode;
    float scaleFactor = max(scaleX / m_muSize, scaleY / m_muSize);

    GetDWriteRenderingMode(pIDWriteFontFace, textRenderingMode, fAnimationQuality, scaleFactor, pDisplaySettings, &dwriteRenderingMode);
    
    // Passing outline to DWrite will invariably return a bad HR, since we should handle
    // all cases where we render geometric text separately (see ShouldUseGeometry).
    Assert(dwriteRenderingMode != DWRITE_RENDERING_MODE_OUTLINE);

    // NOTE: There is some inconsistency in argument passing here - we hard-code the
    // scale factor here to 1 and only pass the scale in via the transform argument.
    // This means that glyphs with different muSizes scaled to the same size on-screen
    // may be rendered differently by DWrite (different hinting, etc).
    IFC(pIDWriteFactoryNoRef->CreateGlyphRunAnalysis(
        &glyphRun,                              // glyphRun
        1,                                      // pixelsPerDip,
        &scaleTransform,                        // transform,
        dwriteRenderingMode,
        m_measuringMethod,
        0.0,                                    // baseline is handled by GlyphRunPainter
        0.0,
        &pIDWriteGlyphRunAnalysis               // glyphRunAnalysis
        ));

    // Create the realization 
    pRealization = new CGlyphRunRealization(scaleX, scaleY, fAnimationQuality, m_pGlyphCache);
    IFCOOM(pRealization);
    pRealization->AddRef();    
    pRealization->SetAnalysis(pIDWriteGlyphRunAnalysis);

    DynArrayIA <CGlyphRunRealization*, 2> *pArrayToCreateIn = NULL;
    if (fBiLevelRequested)
    {
        pArrayToCreateIn = &m_prgBiLevelRealizationArray;
    }
    else
    {
        pArrayToCreateIn = fAnimationQuality ? &m_prgAnimationQualityRealizationArray : 
                                               &m_prgHighQualityRealizationArray;        
    }

    // Find an unused array index in the appropriate array to store the new realization
    UINT realizationHandle = CMilSlaveGlyphCache::c_invalidHandleValue;
    UINT count = pArrayToCreateIn->GetCount();
    for (UINT n = 0; n < count; n++)
    {
        if ((*pArrayToCreateIn)[n] == NULL)
        {
            realizationHandle = n;
            break;
        }
    }
    
    if (realizationHandle == CMilSlaveGlyphCache::c_invalidHandleValue)  // No empty places, create a new element
    {
        realizationHandle = pArrayToCreateIn->GetCount();
        IFC( pArrayToCreateIn->Add(NULL) );
    }
    
    Assert(realizationHandle < pArrayToCreateIn->GetCount());

    // Include the realization into the list.
    Assert((*pArrayToCreateIn)[realizationHandle] == NULL);
    (*pArrayToCreateIn)[realizationHandle] = pRealization;
    pRealization->AddRef();
    

    // Returning realization - note that reference is transferred to out argument.
    *ppRealization = pRealization;
    pRealization = NULL; 

Cleanup:
    ReleaseInterface(pRealization);
    ReleaseInterface(pIDWriteGlyphRunAnalysis);
    ReleaseInterface(pIDWriteFontFace);
    
    RRETURN(hr);
}

//============================================================================================================================
//
// Determines which DWRITE_RENDERING_MODE we want to use when creating an IDWriteGlyphRunAnalysis. 
// The determination for which blend mode we will use happens separately.
//
// The decision of which DWRITE_RENDERING_MODE to use depends on TextOptions.TextFormattingMode and TextOptions.TextRenderingMode.
//
// TextRenderingMode | TextFormattingMode | DWRITE_RENDERING_MODE
// --------------------------------------------------------------
// Auto              | Display            | DWrite's Choice
// Auto              | Ideal              | DWrite's Choice
// Aliased           | Display            | DWRITE_RENDERING_MODE_ALIASED
// Aliased           | Ideal              | DWrite's Choice
// ClearType         | Display            | DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC
// ClearType         | Ideal              | DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC if DWrite chose symmetric, otherwise DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL
// Grayscale         | Display            | DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC
// Grayscale         | Ideal              | DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL
//
// The exception to the above is when fAnimationQuality is true and we are in Ideal mode.  In this case, DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC is used.
//============================================================================================================================
void
CGlyphRunResource::GetDWriteRenderingMode(__in IDWriteFontFace *pIDWriteFontFace,
                                          MilTextRenderingMode::Enum textRenderingMode,
                                          bool fAnimationQuality,
                                          float scaleFactor,
                                          __in const DisplaySettings *pDisplaySettings,                                    
                                          __out DWRITE_RENDERING_MODE *pDWriteRenderingMode
                                          )
{
    if (textRenderingMode == MilTextRenderingMode::Aliased 
        && IsDisplayMeasured())
    {
        *pDWriteRenderingMode = DWRITE_RENDERING_MODE_ALIASED;
    }
    else
    {
        if (fAnimationQuality && !IsDisplayMeasured())
        {
            // Force symmetric (VAA) CT for animation mode for Ideal/Natural measured text
            *pDWriteRenderingMode = DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC;
        }
        else
        {
            if (IsDisplayMeasured()
                && (textRenderingMode == MilTextRenderingMode::Grayscale
                    || textRenderingMode == MilTextRenderingMode::ClearType))
            {
                *pDWriteRenderingMode = DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC;
            }
            else
            {
                if (textRenderingMode == MilTextRenderingMode::Grayscale)
                {
                    *pDWriteRenderingMode = DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL;
                }
                else
                {
                    // We defer to DWrite for this decision in some cases
                    // The scaleFactor is used in conjunction with the muSize to calculate the
                    // actual rendered size of this glyph run.
                    if (FAILED((pIDWriteFontFace->GetRecommendedRenderingMode(
                        m_muSize,
                        scaleFactor,
                        m_measuringMethod,
                        pDisplaySettings->pIDWriteRenderingParams,
                        pDWriteRenderingMode
                        ))))
                    {
                        //
                        // Default to CT natural/ideal in failure case, since failure of this
                        // call is non fatal for our purposes.
                        //
                        *pDWriteRenderingMode = IsDisplayMeasured() ? DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC :
                            DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL;
                    }

                    if (textRenderingMode == MilTextRenderingMode::ClearType)
                    {
                        // If DWrite chose a symmetric anti-aliasing algorithm and the developer
                        // has explicitly chosen ClearType rendering, choose the corresponding
                        // symmetric ClearType algorithm.
                        if (*pDWriteRenderingMode == DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC
                            || *pDWriteRenderingMode == DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC)
                        {
                            *pDWriteRenderingMode = DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC;
                        }
                        else
                        {
                            *pDWriteRenderingMode = DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL;
                        }
                    }
                }
            }
        }
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::PurgeOldEntries
//
//  Synopsis:
//      Purges realizations from the array older than 
//      c_frameCountBeforeDeleteHighQualityRealization age
//      Note that this doesn't delete the bitmaps from the cache, just this 
//      particular realization record.
//
//  Returns:
//      void
//
//------------------------------------------------------------------------------
void 
CGlyphRunResource::PurgeOldEntries(__in DynArrayIA <CGlyphRunRealization*, 2> *pRealizationArray)
{
    const UINT minimumEntriesForPurge = 4;


    if (pRealizationArray->GetCount() > minimumEntriesForPurge)
    {
        UINT remainingCount = pRealizationArray->GetCount(); // Must be greater than 0 per loop condition.
        
        while (remainingCount > 0)
        {
            UINT current = remainingCount - 1; // Ok, because loop condition ensures that count > 0            
            CGlyphRunRealization* pRealization = (*pRealizationArray)[current];
            if (pRealization)
            {
                if (m_pGlyphCache->GetCurrentRealizationFrame() - pRealization->LastUsedFrame() >
                    c_frameCountBeforeDeleteHighQualityRealization)
                {
                    ReleaseInterface(pRealization);
                    pRealizationArray->RemoveAt(current); 
                }
            }
            else
            {
                // Remove empty slot
                pRealizationArray->RemoveAt(current);
            }
            
            remainingCount--;
        }    
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::FindMatchingRealization
//
//  Synopsis:
//      Searches pREalizationArray for a realization with a matching scale
//      factor. Returns score in pMatchQuality and fFoundExactMatch indicates
//      score was IsCloseReal() to 1.0.
//      fUseLastFulfilledScale indicates to search using GetLastFulfilledScaleX/Y
//      rather than GetScaleX/Y, which should be used only for animation quality
//      realization sets
//
//  Returns:
//      index of match, or -1 if there is no match (ie array is empty)
//
//------------------------------------------------------------------------------
void 
CGlyphRunResource::FindMatchingRealization(
    __in const DynArrayIA <CGlyphRunRealization*, 2> *pRealizationArray,
    bool fUseLastFulfilledScale,
    float desiredScaleX,
    float desiredScaleY,
    __out double *pMatchQuality,
    __out bool *pFoundExactMatch,
    __out bool *pFoundMatch,
    __out UINT *pFoundIndex
    )
{
    double matchQuality = 0.0;
    bool foundExactMatch = false;
    bool foundMatch = false;
    UINT foundIndex = 0;
    
    UINT count = pRealizationArray->GetCount();

 
    for (UINT h = 0; h < count; h++)
    {
        const CGlyphRunRealization *pRealizationTemp = (*pRealizationArray)[h];
        if (!pRealizationTemp) 
        {
            // Array should not be empty
            Assert(false);
            
            continue;
        }

        float scaleX; 
        float scaleY;

        if (fUseLastFulfilledScale)
        {
            Assert(pRealizationTemp->IsAnimationQuality());
            scaleX = pRealizationTemp->LastFulfilledScaleX();
            scaleY = pRealizationTemp->LastFulfilledScaleY();
        }
        else
        {
            scaleX = pRealizationTemp->GetScaleX();
            scaleY = pRealizationTemp->GetScaleY();
        }
        
        double quality = InspectScaleQuality(scaleX, desiredScaleX, scaleY, desiredScaleY);

        // Check for closest quality match         
        if (quality >= matchQuality)
        {
            foundIndex = h;
            foundMatch = true;
            matchQuality = quality;
        }

        // Early out for an exact match
        if (IsCloseReal(scaleX, desiredScaleX) && IsCloseReal(scaleY, desiredScaleY))
        {
            foundExactMatch = true;
            break;
        }
    }

    *pFoundExactMatch = foundExactMatch;
    *pMatchQuality = matchQuality;
    *pFoundMatch = foundMatch;
    *pFoundIndex = foundIndex;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      static CGlyphRunResource::InspectScaleQuality
//
//  Synopsis:
//      Helper for GetAvailableScale(). Detects how well two given pairs of
//      scale ratios match one another.
//
//  Returns:
//      the value in range [0..1]; the greater is the better
//
//------------------------------------------------------------------------------
double
CGlyphRunResource::InspectScaleQuality(
    __in double scaleX1,
    __in double scaleX2,
    __in double scaleY1,
    __in double scaleY2
    )
{
    // For given pair (a,b) we need a function that equals to 1
    // when a == b and is less than 1 when a != b.
    // Following formulas are used:
    // F = 1/(1 + R^2)
    // R = diff/sum
    // diff = a-b
    // sum = a+b

    double dx = scaleX1 - scaleX2;
    double sx = scaleX1 + scaleX2;

    double dy = scaleY1 - scaleY2;
    double sy = scaleY1 + scaleY2;

    dx *= dx;
    sx *= sx;
    dy *= dy;
    sy *= sy;

    return (sx*sy)/((sx+dx)*(sy+dy));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::ShouldUseGeometry
//
//  Synopsis:
//      For given transformation from local coordinates to device, check whether
//      geometry resource should be used.
//
//------------------------------------------------------------------------------
bool
CGlyphRunResource::ShouldUseGeometry(
    __in_ecount(1) CMultiOutSpaceMatrix<CoordinateSpace::LocalRendering> const * pWorldToDevice,
    __in_ecount(1) DisplaySettings const * pDisplaySettings
    )
{
    HRESULT hr;
    IDWriteFontFace *pIDWriteFontFace = NULL;

    // If the size is "big", then use geometry based rendering.
    // The sense of the word "big" is following.
    // Consider the glyph that is nothing but square,
    // with width and height equal to font face "Em" size.
    // The transformation will convert this square to parallelogramm.
    // The base of parallelogramm corresponds to the horizontal edge of square
    // (that's directed along text base line).
    // The size of parallelogramm base shows how much
    // the glyphs should be stretched horizontally.
    // Another thing to check is the height of the parallelogramm.
    // Its size shows how much the glyphs will be stretched vertically.
    // We pass the larger of the two to DWrite to determine whether our glyphs
    // are too big for bitmap-based rendering.
    //
    // Note: the scale calculation is done again in CGlyphPainter.Init() if we 
    // choose not to render with geometry.
    //
    float scaleFactorX = sqrtf(pWorldToDevice->_11*pWorldToDevice->_11
                              + pWorldToDevice->_12*pWorldToDevice->_12);
    float scaleFactorY = scaleFactorX == 0 ? 0
        : fabsf(pWorldToDevice->_11 * pWorldToDevice->_22
               - pWorldToDevice->_21 * pWorldToDevice->_12) / scaleFactorX;

    Assert(m_pIDWriteFont);
    DWRITE_RENDERING_MODE renderingMode;

    IFC(CDWriteFontFaceCache::GetFontFace(m_pIDWriteFont, &pIDWriteFontFace));

    IFC(pIDWriteFontFace->GetRecommendedRenderingMode(
                            m_muSize,
                            max(scaleFactorX, scaleFactorY),
                            m_measuringMethod,
                            pDisplaySettings->pIDWriteRenderingParams,
                            &renderingMode
                            ));

    bool fShouldUseGeometry = (renderingMode == DWRITE_RENDERING_MODE_OUTLINE);

    // If we can't get a geometry, just return false and try to render bitmaps,
    // else render nothing.
    if (fShouldUseGeometry)
    {
        EnsureGeometry();
    }

    // If glyph run size is big, then use geometry.
    // If it is small, use bitmaps 
    hr = (fShouldUseGeometry && (m_pGeometry != NULL)) ? S_OK : E_FAIL;

Cleanup:
    ReleaseInterface(pIDWriteFontFace);
    return SUCCEEDED(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::EnsureGeometry
//
//  Synopsis:
//      Ensures we have created a geometry realization of this glyphrun.
//
//------------------------------------------------------------------------------
void 
CGlyphRunResource::EnsureGeometry()
{
    HRESULT hr = S_OK;
    
    CGlyphRunGeometrySink *pGeometrySink = NULL;
    IDWriteFontFace *pIDWriteFontFace = NULL;

    if (!m_pGeometry)
    {
        //
        // glyphOffsets is an array of 
        // DWRITE_GLYPH_OFFSET, which is
        // defined as:
        //
        // struct DWRITE_GLYPH_OFFSET
        // {
        //    FLOAT advanceOffset;
        //    FLOAT ascenderOffset;
        // };
        //
        // Assert that this is always true so
        // that we don't have to remarshall all
        // our offset data, and can assume that
        // our encoding of [X0, Y0, X1, Y1, ...] 
        // matches an array of DWRITE_GLYPH_OFFSET
        //
        #if DBG
        {
            DWRITE_GLYPH_OFFSET offsetTest;
            const void * pXOffset = reinterpret_cast<const void *>(&offsetTest.advanceOffset);
            const void * pYOffset = reinterpret_cast<const void *>(&offsetTest.ascenderOffset);
            Assert (pYOffset > pXOffset );
            //+ sizeof(float));
            //Assert(&(*reinterpret_cast<VOID*>(&offsetTest.advanceOffset)) + sizeof(float) == &(*reinterpret_cast<VOID*>(&offsetTest.ascenderOffset)));
        }
        #endif 

        IFC(CGlyphRunGeometrySink::Create(&pGeometrySink));

        const DWRITE_GLYPH_OFFSET *pGlyphOffsets = reinterpret_cast<const DWRITE_GLYPH_OFFSET*>(m_pGlyphOffsets);

        Assert(m_pIDWriteFont);
        IFC(CDWriteFontFaceCache::GetFontFace(m_pIDWriteFont, &pIDWriteFontFace));

        IFC(pIDWriteFontFace->GetGlyphRunOutline(
            m_muSize,
            m_pGlyphIndices,
            m_pGlyphAdvances,
            pGlyphOffsets,
            m_usGlyphCount,
            !!IsSideways(),
            !!IsRightToLeft(),
            pGeometrySink
            ));

        // We now own the reference to the CMilGeometryDuce.
        IFC(pGeometrySink->ProduceGeometry(&m_origin, &m_pGeometry));
     }

    Cleanup:
    
        ReleaseInterface(pGeometrySink);
        ReleaseInterface(pIDWriteFontFace);
        
        // If we fail for some reason, we'll fall back to using bitmap text by ensuring
        // we do not have any text geometry.
        if (FAILED(hr))
        {
            ReleaseInterface(m_pGeometry);
        }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::GetGammaTable
//
//  Synopsis:
//      Gets a gamma table to use for gamma adjustment for this glyph run
//
//------------------------------------------------------------------------------
HRESULT 
CGlyphRunResource::GetGammaTable(__in_ecount(1) const DisplaySettings *pDisplaySettings, 
                                 __deref_out_ecount(1) GammaTable const* * ppGammaTable)
{
    HRESULT hr = S_OK;     
    
    UINT GammaIndex = pDisplaySettings->AllowGamma ? m_pGlyphBlendingParameters->GammaIndex : 0;

    const CDisplaySet *pDisplaySet = NULL;
    g_DisplayManager.GetCurrentDisplaySet(&pDisplaySet);

    const GammaTable *pGammaTable;
    IFC(pDisplaySet->GetGammaTable(GammaIndex, &pGammaTable));

    *ppGammaTable = pGammaTable;

Cleanup:
    ReleaseInterface(pDisplaySet);
    
    RRETURN(hr);        
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::GetEnhancedContrastTable
//
//  Synopsis:
//      Gets an enhanced contrast table to use for gamma adjustment for this glyph run
//
//------------------------------------------------------------------------------
HRESULT 
CGlyphRunResource::GetEnhancedContrastTable(float k, 
                                            __deref_out_ecount(1) EnhancedContrastTable **ppTable)
{
    HRESULT hr = S_OK; 
    *ppTable = NULL;

    // k value of 0 doesn't require contrast enhancement. It is expensive to perform, so
    // fast path this by returning ppTable = NULL.
    const CDisplaySet *pDisplaySet = NULL;
    if (k != 0.0f)
    {        
        g_DisplayManager.GetCurrentDisplaySet(&pDisplaySet);

        IFC(pDisplaySet->GetEnhancedContrastTable(k, ppTable));
    }
    
Cleanup:
    ReleaseInterface(pDisplaySet);
    
    RRETURN(hr);        
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::GetBounds
//
//  Synopsis:
//      For given transformation from local coordinates to device, calculate the
//      bounding rectangle needed for bitmap based rendering.
//
//      Returned rectangle is defined in local space.
//
//      This routine does not handle geometry based rendering.
//
//------------------------------------------------------------------------------
void
CGlyphRunResource::GetBounds(
    __out_ecount(1) CRectF<CoordinateSpace::LocalRendering> * prcBounds,
    __in_ecount(1) CBaseMatrix const * pWorldToDevice
    ) const
{
    // Use precomputed bounds from managed code.
    *prcBounds = m_boundingRect;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::SnapToScaleGrid
//
// Synopsis:
//      Choose the number in ScaleGrid closets to given one.
//      Ported from managed code
//
//------------------------------------------------------------------------------

float
CGlyphRunResource::SnapToScaleGrid(
    __in double x)
{
    // check for extreme values
    int a = 0;
    float va = scaleGrid[a];
    if (x <= va)
        return va;

    int b = c_scaleGridSize - 1;
    float vb = scaleGrid[b];
    if (x >= vb)
        return vb;

    // Binary search to detect the range
    // (ScaleGrid[a], ScaleGrid[a+1])
    // that contains given value.
    while ((b - a) > 1)
    {
        int c = (a + b) / 2;
        float vc = scaleGrid[c];
        if (x >= vc)
        {
            a = c;
            va = vc;
        }
        else
        {
            b = c;
            vb = vc;
        }
    }

    // Given x lays in between of (va,vb); choose
    // the best of them using logarithmic measure.
    return x*x > va*vb ? vb : va;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunRealization::CGlyphRunRealization
//
//  Synopsis:
//      Constructor.
//
//------------------------------------------------------------------------------
CGlyphRunRealization::CGlyphRunRealization(
    __in float scaleX,
    __in float scaleY,
    __in bool fAnimationQuality,
    __in CMilSlaveGlyphCache *pGlyphCacheSlave
    )
{
    m_scaleX     = scaleX;
    m_scaleY     = scaleY;

    m_lastFulfilledScaleX = scaleX;
    m_lastFulfilledScaleY = scaleY;

    m_fIsAnimationQuality = fAnimationQuality;

    m_pGlyphCacheNoRef = pGlyphCacheSlave;
    m_createdFrame = pGlyphCacheSlave->GetCurrentRealizationFrame();
    m_lastUsedFrame = pGlyphCacheSlave->GetCurrentRealizationFrame();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunRealization::SetAnalysis()
//
//------------------------------------------------------------------------------
void
CGlyphRunRealization::SetAnalysis(
    __in IDWriteGlyphRunAnalysis *pIDWriteGlyphRunAnalysis
    )
{
    Assert(m_pIDWriteGlyphRunAnalysis == NULL);
    
    m_pIDWriteGlyphRunAnalysis = pIDWriteGlyphRunAnalysis;
    pIDWriteGlyphRunAnalysis->AddRef();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunRealization::~CGlyphRunRealization()
//
//  Synopsis:
//      Destructor
//
//------------------------------------------------------------------------------
CGlyphRunRealization::~CGlyphRunRealization()
{
    DeleteAlphaMap();

    delete m_pSWGlyphRun;

    for (int i = m_pD3DGlyphRuns.GetCount(); --i >= 0;)
    {
        delete m_pD3DGlyphRuns[i];
    }

    ReleaseInterface(m_pIDWriteGlyphRunAnalysis);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunRealization::GetD3DGlyphRun
//
//  Synopsis:
//      Get the pointer to CD3DGlyphRun for given device index. Ensure that the
//      array that stores the pointers is large enough.
//
//------------------------------------------------------------------------------
HRESULT
CGlyphRunRealization::GetD3DGlyphRun(UINT index, CD3DGlyphRun** ppD3DGlyphRun)
{
    HRESULT hr = S_OK;

    Assert(ppD3DGlyphRun);

    if (index < (UINT)m_pD3DGlyphRuns.GetCount())
    {
        *ppD3DGlyphRun = m_pD3DGlyphRuns[index];
    }
    else
    {
        *ppD3DGlyphRun = 0;

        while (index >= (UINT)m_pD3DGlyphRuns.GetCount())
        {
            IFC( m_pD3DGlyphRuns.Add(0) );
        }
    }

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunRealization::SetD3DGlyphRun
//
//  Synopsis:
//      Store the pointer to CD3DGlyphRun for given device index.
//
//  Multithread notice 2:
//      Even in several rendering threads scenario there exist only one owner of
//      resource with particular index. So we need to protect only against
//      array's moving in memory, not against array's element content. If static
//      array will be used, no pretection will be needed.
//
//------------------------------------------------------------------------------
void
CGlyphRunRealization::SetD3DGlyphRun(UINT index, CD3DGlyphRun *pRun)
{

    // the array should be large enough due to preceeding
    // GetD3DGlyphRun() call
    Assert(index < (UINT)m_pD3DGlyphRuns.GetCount());
    Assert(!m_pD3DGlyphRuns[index]);
    Assert(pRun);
    m_pD3DGlyphRuns[index] = pRun;

}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunRealization::SetSWGlyphRun
//
//  Synopsis:
//      Store the pointer to CSWGlyphRun.
//
//  Multithread notice 3:
//      In several rendering threads scenario we need to rethink how can several
//      SW renderers share the single CSWGlyphRun.
//
//------------------------------------------------------------------------------
void
CGlyphRunRealization::SetSWGlyphRun(CSWGlyphRun *pRun)
{
    Assert(!m_pSWGlyphRun);
    m_pSWGlyphRun = pRun;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunRealization::EnsureValidAlphaMap
//
//  Synopsis:
//      Gets an alpha map for this realization using DWrite's IDWriteGlyphRunAnalysis
//
//------------------------------------------------------------------------------
HRESULT 
CGlyphRunRealization::EnsureValidAlphaMap(__in const EnhancedContrastTable *pECT)
{
    HRESULT hr = S_OK;

    if (!m_fHasAlphaMaps)
    {        
        FreAssert(m_pIDWriteGlyphRunAnalysis != NULL);

        UINT32 clearTypeTextureSize = 0, biLevelTextureSize = 0;
        RECT clearTypeAlphaMapBoundingBox = {0}, biLevelAlphaMapBoundingBox = {0};
        BYTE *pClearTypeAlphaMap = NULL, *pBiLevelAlphaMap = NULL;

        IFC(RealizeAlphaBoundsAndTextures(DWRITE_TEXTURE_CLEARTYPE_3x1, pECT, &clearTypeTextureSize, &clearTypeAlphaMapBoundingBox, &pClearTypeAlphaMap));
        IFC(RealizeAlphaBoundsAndTextures(DWRITE_TEXTURE_ALIASED_1x1, NULL, &biLevelTextureSize, &biLevelAlphaMapBoundingBox, &pBiLevelAlphaMap));

        m_fHasAlphaMaps = true;
        m_fIsBiLevelOnly = IsRectEmpty(clearTypeAlphaMapBoundingBox) && !IsRectEmpty(biLevelAlphaMapBoundingBox);        

        if (!IsRectEmpty(clearTypeAlphaMapBoundingBox) && !IsRectEmpty(biLevelAlphaMapBoundingBox))
        {
            //
            // If this is OK, optimize by picking larger rect to set first, then XORing the smaller rect into it in the correct
            // location. Also test for overlap.
            // Don't want to have to have the painting logic understand how to draw 2 separate bitmaps 
            // for one glyphrun. Complicates caching logic significantly and is an edge case. Instead 
            // combine into a CT surface and remove separate bilevel glyphs            
            //
            CMilRectL BLBbox(biLevelAlphaMapBoundingBox);
            CMilRectL CTBbox(clearTypeAlphaMapBoundingBox);
            CMilRectL UnionBBox(BLBbox);
            UnionBBox.Union(CTBbox);
            UINT32 textureSize = UnionBBox.Width() * UnionBBox.Height();
            BYTE *pCombinedAlphaMap = (BYTE*)WPFAlloc(ProcessHeap,
                                                       Mt(GlyphBitmapClearType),
                                                       textureSize
                                                       );

            TraceTagText((tagError, "CGlyphRunRealization::EnsureValidAlphaMap, allocated bytes: %d", textureSize));

            IFCOOM(pCombinedAlphaMap);
            memset(pCombinedAlphaMap, 0, textureSize);


            // Copy in bilevel glyphs
            const BYTE *pCurrentSource = pBiLevelAlphaMap;
            UINT destStride = UnionBBox.Width();
            UINT srcStride = BLBbox.Width();
            UINT destOffset = (BLBbox.top - UnionBBox.top) * destStride + (BLBbox.left - UnionBBox.left);
            BYTE *pCurrentDest = pCombinedAlphaMap + destOffset;
            for (int i = BLBbox.top; i < BLBbox.bottom; i++)
            {
                memcpy(pCurrentDest, pCurrentSource, srcStride);
                pCurrentDest += destStride;
                pCurrentSource += srcStride;
            }

            // Copy in CT glyphs
            pCurrentSource = pClearTypeAlphaMap;
            srcStride = CTBbox.Width();
            destOffset = (CTBbox.top - UnionBBox.top) * destStride + (CTBbox.left - UnionBBox.left);
            BYTE *pCurrentDestLine = pCurrentDest = pCombinedAlphaMap + destOffset;
            for (int i = CTBbox.top; i < CTBbox.bottom; i++)
            {
                for (int j = CTBbox.left; j < CTBbox.right; j++)                    
                {
                    *pCurrentDest++ |= *pCurrentSource++;
                }

                // Increment destination
                pCurrentDestLine += destStride;
                pCurrentDest = pCurrentDestLine;

                // Don't need to increment source line by line since we're copying all of it
            }

            WPFFree(ProcessHeap, pBiLevelAlphaMap);
            TraceTagText((tagError, "CGlyphRunRealization::EnsureValidAlphaMap, freed bytes: -%d", biLevelTextureSize));            
            pBiLevelAlphaMap = NULL;
            biLevelTextureSize = 0;
            
            WPFFree(ProcessHeap, pClearTypeAlphaMap);
            TraceTagText((tagError, "CGlyphRunRealization::EnsureValidAlphaMap, freed bytes: -%d", clearTypeTextureSize));
            pClearTypeAlphaMap = NULL;
            clearTypeTextureSize = 0;

            m_pAlphaMap = pCombinedAlphaMap;
            m_textureSize = textureSize;
            m_alphaMapBoundingBox.left = UnionBBox.left;
            m_alphaMapBoundingBox.top = UnionBBox.top;
            m_alphaMapBoundingBox.right = UnionBBox.right;
            m_alphaMapBoundingBox.bottom = UnionBBox.bottom;
        }
        else if (IsRectEmpty(clearTypeAlphaMapBoundingBox) || IsRectEmpty(biLevelAlphaMapBoundingBox))
        {
            if (!IsRectEmpty(clearTypeAlphaMapBoundingBox))
            {
                // Only CT bitmaps
                m_pAlphaMap = pClearTypeAlphaMap;
                pClearTypeAlphaMap = NULL;
                m_alphaMapBoundingBox = clearTypeAlphaMapBoundingBox;
                m_textureSize = clearTypeTextureSize;   
            }
            else
            {
                // Only bilevel
                m_pAlphaMap = pBiLevelAlphaMap;
                pBiLevelAlphaMap = NULL;
                m_alphaMapBoundingBox = biLevelAlphaMapBoundingBox;
                m_textureSize = biLevelTextureSize;                
            }
        }        
 
        m_pGlyphCacheNoRef->AddRealization(this, m_textureSize);
    }
Cleanup:
    RRETURN(hr);
}   

//+-----------------------------------------------------------------------------
//
// Deletes alpha map bitmaps, removes them from the CMilSlaveGlyphCache realization
// cache list, and marks the m_pSWGlyphRun and m_pD3DGlyphRuns device specific state
// as stale, so that they know to regenerate alpha bitmaps when they are next used.
//
//------------------------------------------------------------------------------

void 
CGlyphRunRealization::DeleteAlphaMap()
{
    if (m_fHasAlphaMaps)
    {
        WPFFree(ProcessHeap, m_pAlphaMap);
        m_pAlphaMap = NULL;

        TraceTagText((tagError, "CGlyphRunRealization::DeleteAlphaMap, freed bytes: -%d", m_textureSize));
         
        memset(&m_alphaMapBoundingBox, 0, sizeof(m_alphaMapBoundingBox));
        m_pGlyphCacheNoRef->RemoveRealization(this, GetTextureSize());
        m_textureSize = 0;
         m_fHasAlphaMaps = false;

        if (m_pSWGlyphRun)
        {
            m_pSWGlyphRun->DiscardAlphaArray();
        }

        UINT count = m_pD3DGlyphRuns.GetCount();
        for (UINT i = 0; i < count; i++)
        {
            if (m_pD3DGlyphRuns[i])
            {
                m_pD3DGlyphRuns[i]->DiscardAlphaArrayAndResources();
            }
        }
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunRealization::RealizeAlphaBoundsAndTextures
//
//  Synopsis:
//      Gets an alpha map for a particular texture type
//
//------------------------------------------------------------------------------
HRESULT
CGlyphRunRealization::RealizeAlphaBoundsAndTextures(
    DWRITE_TEXTURE_TYPE textureType, 
    __in_opt const EnhancedContrastTable *pECT,
    __out UINT32 *pTextureSize,
    __out RECT *pBoundingBox,
    __deref_out_ecount_opt(*pTextureSize) BYTE **pAlphaMap
    )
{
    HRESULT hr = S_OK;
    RECT boundingBox;
    BYTE *pAlphaValues = NULL;
    
    IFC(m_pIDWriteGlyphRunAnalysis->GetAlphaTextureBounds(
        textureType,
        &boundingBox
        ));

    if (IsRectEmpty(boundingBox))
    {
        *pTextureSize = 0;
        *pAlphaMap = NULL;
    }
    else
    {
        //
        // Calculate required buffer size considering the bounds
        //
        UINT32 width = boundingBox.right - boundingBox.left;
        UINT32 height = boundingBox.bottom - boundingBox.top;        
        UINT32 textureStride = width;
        if (textureType == DWRITE_TEXTURE_CLEARTYPE_3x1) 
        {
            // ClearType bitmaps (DWRITE_TEXTURE_CLEARTYPE_3x1) contain 3 bytes per pixel, 
            // Aliased bitmaps only contain 1.
            textureStride *= 3;
        }
        UINT32 textureSize = textureStride * height;
        
        pAlphaValues = (BYTE *)WPFAlloc(ProcessHeap,
                                        (textureType == DWRITE_TEXTURE_CLEARTYPE_3x1 ? Mt(GlyphBitmapClearType) : Mt(GlyphBitmapBiLevel)),
                                        textureSize);
        TraceTagText((tagError, "CGlyphRunRealization::RealizeAlphaBoundsAndTextures, allocated bytes: %d", textureSize));
        IFCOOM(pAlphaValues);

        IFC(m_pIDWriteGlyphRunAnalysis->CreateAlphaTexture(
            textureType,
            &boundingBox,
            pAlphaValues,
            textureSize
            ));

        //    
        // DWrite gives us bounding boxes in the pixel space rather than
        // the subpixel space we use to render the alpha bitmaps. Convert 
        // them here.
        //
        boundingBox.left *= 3;
        boundingBox.right *= 3; 

        if (textureType == DWRITE_TEXTURE_CLEARTYPE_3x1)
        {
            // Apply gamma correction, contrast enhancement and normalize from 0 - DWRITE_ALPHA_MAX to 0-255
            // pECT may be NULL if the contrast enhancement value is 0.
            if (pECT)
            {
                pECT->RenormalizeAndApplyContrast(pAlphaValues, boundingBox.right - boundingBox.left, boundingBox.bottom - boundingBox.top, textureStride, textureSize);
            }
        }
        else
        {
            // Future Consideration: probably shouldn't do this texture expansion. Need to write a different
            // shader and shrink the texture to benefit perf. Aliased text is relatively rare however, so it's 
            // not worth the investment at this point.
            BYTE *pNewAlphaValues = (BYTE *)WPFAlloc(ProcessHeap,
                                                     Mt(GlyphBitmapBiLevel),
                                                     textureSize * 3);
            TraceTagText((tagError, "CGlyphRunRealization::RealizeAlphaBoundsAndTextures, allocated bytes: %d", textureSize * 3));
            
            for (UINT i = 0; i < textureSize; i++)
            {
                pNewAlphaValues[i*3] = pAlphaValues[i];
                pNewAlphaValues[i*3+1] = pAlphaValues[i];
                pNewAlphaValues[i*3+2] = pAlphaValues[i];
            }

            WPFFree(ProcessHeap, pAlphaValues);
            TraceTagText((tagError, "CGlyphRunRealization::RealizeAlphaBoundsAndTextures, freed bytes: -%d", textureSize));

            pAlphaValues = pNewAlphaValues;

            textureSize *= 3;
        }
                                                    
        *pAlphaMap = pAlphaValues;
        pAlphaValues = NULL;
        *pTextureSize = textureSize;
        *pBoundingBox = boundingBox;
    }

Cleanup:
    if (FAILED(hr))
    {
        WPFFree(ProcessHeap, pAlphaValues);
        TraceTagText((tagError, "CGlyphRunRealization::RealizeAlphaBoundsAndTextures, freed bytes DUE TO FAILURE: unknown size"));
    }
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDWriteFontFaceCache::GetFontFace
//
//  Returns:
//      An IDWriteFontFace matching the given IDWriteFont.
//
//------------------------------------------------------------------------------
HRESULT CGlyphRunResource::CDWriteFontFaceCache::GetFontFace(
    __in IDWriteFont *pFont,
    __out IDWriteFontFace **ppFontFace
    )
{
    HRESULT hr = S_OK;

    *ppFontFace = NULL;

    if (InterlockedIncrement(&m_mutex) == 1)
    {
        // Try the fast path first -- is caller accessing exactly the mru entry?
        if (m_cache[m_cacheMRU].pFont == pFont)
        {
            m_cache[m_cacheMRU].pFontFace->AddRef();
            *ppFontFace = m_cache[m_cacheMRU].pFontFace;
        }
        else
        {
            // No luck, do a search through the cache.
            for (UINT32 i=0; i < c_cacheSize; i++)
            {
                if (m_cache[i].pFont == pFont)
                {
                    m_cache[i].pFontFace->AddRef();
                    *ppFontFace = m_cache[i].pFontFace;
                    m_cacheMRU = i;
                    break;
                }
            }
        }
    }
    InterlockedDecrement(&m_mutex);

    // If the cache was busy or did not contain this Font, create a new FontFace.
    if (NULL == *ppFontFace)
    {
        IFC(AddFontFaceToCache(pFont, ppFontFace));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDWriteFontFaceCache::Reset
//
//  Returns:
//      Clears the IDWriteFontFace cache, releasing all resources.
//
//------------------------------------------------------------------------------
void CGlyphRunResource::CDWriteFontFaceCache::Reset()
{
    // NB: If the cache is busy, we do nothing.
    if (InterlockedIncrement(&m_mutex) == 1)
    {
        for (UINT32 i=0; i < c_cacheSize; i++)
        {
            ReleaseInterface(m_cache[i].pFont);
            ReleaseInterface(m_cache[i].pFontFace);
        }

        m_cacheMRU = 0;
    }
    InterlockedDecrement(&m_mutex);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDWriteFontFaceCache::AddFontFaceToCache
//
//  Synopsis:
//      Adds a new IDWriteFontFace to the cache, discarding an older entry if necessary.
//
//------------------------------------------------------------------------------
HRESULT CGlyphRunResource::CDWriteFontFaceCache::AddFontFaceToCache(
    __in IDWriteFont *pFont,
    __out IDWriteFontFace **ppFontFace
    )
{
    HRESULT hr = S_OK;

    IFC(pFont->CreateFontFace(ppFontFace));

    // NB: if the cache is busy, we simply return the new FontFace
    // without bothering to cache it.
    if (InterlockedIncrement(&m_mutex) == 1)
    {
        // Default to a slot that is not the MRU.
        m_cacheMRU = (m_cacheMRU + 1) % c_cacheSize;

        // Look for an empty slot.
        for (UINT32 i=0; i < c_cacheSize; i++)
        {
            if (NULL == m_cache[i].pFont)
            {
                m_cacheMRU = i;
                break;
            }
        }

        ReleaseInterface(m_cache[m_cacheMRU].pFont);
        ReleaseInterface(m_cache[m_cacheMRU].pFontFace);

        pFont->AddRef();
        m_cache[m_cacheMRU].pFont = pFont;
        (*ppFontFace)->AddRef();
        m_cache[m_cacheMRU].pFontFace = *ppFontFace;
    }
    InterlockedDecrement(&m_mutex);

Cleanup:
    RRETURN(hr);
}



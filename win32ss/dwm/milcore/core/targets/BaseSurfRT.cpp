// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_targets
//      $Keywords:
//
//  $Description:
//      Define base surface render target class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


#include "precomp.hpp"


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseSurfaceRenderTarget::BeginLayer
//
//  Synopsis:
//      Begin accumulation of rendering into a layer.  Modifications to layer,
//      as specified in arguments, are handled and result is applied to render
//      target when the matching EndLayer call is made.
//
//      Calls to BeginLayer may be nested, but other calls that depend on the
//      current contents, such as Present, are not allowed until all
//      layers have been resolved with EndLayer.
//

template <typename TRenderTargetLayerData>
STDMETHODIMP
CBaseSurfaceRenderTarget<TRenderTargetLayerData>::BeginLayer(
    __in_ecount(1) MilRectF const &rcLayerBoundsF,
    MilAntiAliasMode::Enum AntiAliasMode,
    __in_ecount_opt(1) IShapeData const *pGeometricMask,
    __in_ecount_opt(1) CMILMatrix const *pGeometricMaskToTarget,
    FLOAT flAlphaScale,
    __in_ecount_opt(1) CBrushRealizer *pAlphaMask
    )
{
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "PREFix cannot determine that the m_LayerStack cannot have a null m_pData when Capacity isn't 0.")
    HRESULT hr = S_OK;

    CRenderTargetLayer *pNewLayer = NULL;

    CMILSurfaceRect rcLayerBounds;
    bool fClearTypeHintWasSaved = false;

    DbgAssertBoundsState();

    //
    // The case is supported so create a layer on our stack.  Since the stack
    // always maintains the layer object call push which will return the new
    // layer object already in a default initialized state.
    //

    IFC(m_LayerStack.Push(OUT pNewLayer));

    // Always need to remember current bounds to be restore at EndLayer.
    pNewLayer->rcPrevBounds = m_rcBounds;
    pNewLayer->AntiAliasMode = AntiAliasMode;

    //
    // Intersect with current surface bounds to find maximum layer to create.
    //
    // If intersection is empty or opacity is too small, all rendering to this
    // layer can be ignored.
    //
    if (   !IntersectAliasedBoundsRectFWithSurfaceRect(rcLayerBoundsF,
                                                       m_rcBounds,
                                                       &rcLayerBounds)
        || AlphaScaleEliminatesRenderOutput(flAlphaScale)
       )
    {
        // Set properties to make this layer consume rendering until EndLayer
        pNewLayer->rcLayerBounds.SetEmpty();
        pNewLayer->rAlpha = 0;
    }
    else
    {
        pNewLayer->rcLayerBounds = rcLayerBounds;
        pNewLayer->rAlpha = flAlphaScale;
        SetInterface(pNewLayer->pAlphaMaskBrush, pAlphaMask);

        BOOL fAlphaScalePreservesOpacity = AlphaScalePreservesOpacity(flAlphaScale);

        if (g_pMediaControl
            && g_pMediaControl->GetDataPtr()->AlphaEffectsDisabled)
        {
            fAlphaScalePreservesOpacity = TRUE;
        }

        //
        // Check if there is any need for a real layer at all
        //
        if (   !pGeometricMask
               // true if alpha scale has no effect - essentially opaque
            && fAlphaScalePreservesOpacity
            && !pAlphaMask
           )
        {
            // No fixup will be needed at EndLayer.  Currently this is
            // indicated by NULL m_SourceBitmapRef and m_pGeometricMask.
            Assert(pNewLayer->oTargetData.m_pSourceBitmap == NULL);
        }
        else
        {
            if (pGeometricMask)
            {
                //
                // Copy geometric mask for storage in layer data
                //

                pNewLayer->pGeometricMaskShape = new CShape;
                IFCOOM(pNewLayer->pGeometricMaskShape);

                IFC(pNewLayer->pGeometricMaskShape->AddShapeData(
                    *pGeometricMask,
                    pGeometricMaskToTarget
                    ));

                pNewLayer->pGeometricMaskShape->SetFillMode(
                    pGeometricMask->GetFillMode()
                    );
            }

            if (HasAlpha())
            {
                pNewLayer->fSavedClearTypeHint = m_forceClearType;
                m_forceClearType = false;
                fClearTypeHintWasSaved = true;
            }
            
            //
            // Call BeginLayerInternal to handle surface specific actions
            // like creating a copy of current surface for area of layer.
            //

            IFC(BeginLayerInternal(pNewLayer));

            if (MCGEN_ENABLE_CHECK(MICROSOFT_WINDOWS_WPF_PROVIDER_Context, WClientCreateIRT))
            {
                unsigned int effectType = 0L;

                if (pGeometricMask && !(flAlphaScale < 1))
                {
                    effectType = IRT_Clip;
                }
                else if (!pGeometricMask && flAlphaScale < 1)
                {
                    effectType = IRT_Opacity;
                }
                else if (pGeometricMask && flAlphaScale < 1)
                {
                    effectType = IRT_ClipAndOpacity;
                }

                EventWriteWClientCreateIRT(NULL, NULL, effectType);
            }
        }
    }

    // Update current bounds to those of the new layer
    m_rcBounds = pNewLayer->rcLayerBounds;

Cleanup:

    if (FAILED(hr))
    {
        // If we created/pushed a layer, pop it to destroy it
        if (pNewLayer)
        {
            // Reset ClearTypeHint to saved value.
            if (HasAlpha() && fClearTypeHintWasSaved)
            {
                m_forceClearType = pNewLayer->fSavedClearTypeHint;
            }
            
            m_LayerStack.Pop();
        }
    }

    DbgAssertBoundsState();

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseSurfaceRenderTarget::EndLayer
//
//  Synopsis:
//      End accumulation of rendering into current layer.  Modifications to
//      layer, as specified in BeginLayer arguments, are handled and result is
//      applied to render target.
//

template <typename TRenderTargetLayerData>
STDMETHODIMP
CBaseSurfaceRenderTarget<TRenderTargetLayerData>::EndLayer(
    )
{
    HRESULT hr = S_OK;

    DbgAssertBoundsState();

    CRenderTargetLayer const &layer = m_LayerStack.Top();

    Assert(layer.rcLayerBounds.left   == m_rcBounds.left  );
    Assert(layer.rcLayerBounds.top    == m_rcBounds.top   );
    Assert(layer.rcLayerBounds.right  == m_rcBounds.right );
    Assert(layer.rcLayerBounds.bottom == m_rcBounds.bottom);

    //
    // Check for no fixup case
    //

    if (!layer.oTargetData.m_pSourceBitmap)
    {
        goto Cleanup;
    }

    m_rcCurrentClip = layer.rcLayerBounds;

    //
    // Call EndLayerInternal to handle surface specific actions like creating
    // restoring portion of original surface for area of layer.
    //

    IFC(EndLayerInternal());

Cleanup:

    // Some failure HRESULTs should only cause the primitive
    // in question to not draw.
    IgnoreNoRenderHRESULTs(&hr);

    //
    // Clean up the RT state changes made by BeginLayer
    //

    // Restore previous surface bounds
    m_rcBounds = layer.rcPrevBounds;

    // Reset ClearTypeHint to saved value.
    if (HasAlpha())
    {
        m_forceClearType = layer.fSavedClearTypeHint;
    }
    
    // Cleanup the layer
    m_LayerStack.Pop();

    // State checks for check builds
    DbgAssertBoundsState();

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseSurfaceRenderTarget::EndAndIgnoreAllLayers
//
//  Synopsis:
//      End accumulation of rendering into all layers.  Mdifications to layers,
//      as specified in BeginLayer arguments, are ignored.
//

template <typename TRenderTargetLayerData>
STDMETHODIMP_(void)
CBaseSurfaceRenderTarget<TRenderTargetLayerData>::EndAndIgnoreAllLayers(
    )
{
    DbgAssertBoundsState();

    while (m_LayerStack.GetCount() > 0)
    {
        m_LayerStack.Pop();
    }

    m_rcBounds.left   = 0;
    m_rcBounds.top    = 0;
    m_rcBounds.right  = static_cast<INT>(m_uWidth);
    m_rcBounds.bottom = static_cast<INT>(m_uHeight);

    DbgAssertBoundsState();

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseSurfaceRenderTarget::ReadEnabledDisplays
//
//  Synopsis:
//      Return true for the display this surface is associated with.
//
//------------------------------------------------------------------------------

template <typename TRenderTargetLayerData>
STDMETHODIMP
CBaseSurfaceRenderTarget<TRenderTargetLayerData>::ReadEnabledDisplays (
    __inout DynArray<bool> *pEnabledDisplays
    )
{
    HRESULT hr = S_OK;

    const CDisplaySet *pDisplaySet;
    g_DisplayManager.GetCurrentDisplaySet(&pDisplaySet);

    // Mark the display we are associated with as true, all others false.
    // If our surf RT is not associated with a display, we will report no
    // displays enabled.
    UINT uDisplayIndex;
    if (m_associatedDisplay.IsNone())
    {
        uDisplayIndex = UINT_MAX;
    }
    else
    {
        IFC(pDisplaySet->GetDisplayIndexFromDisplayId(m_associatedDisplay, uDisplayIndex));
    }

    Assert(uDisplayIndex < pDisplaySet->GetDisplayCount() || uDisplayIndex == UINT_MAX);
    Assert(pDisplaySet->GetDisplayCount() == pEnabledDisplays->GetCount());
    for (UINT i = 0; i < pEnabledDisplays->GetCount(); i++)
    {       
        (*pEnabledDisplays)[i] = (i == uDisplayIndex);
    }

Cleanup:
    ReleaseInterface(pDisplaySet);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseSurfaceRenderTarget::GetPartialLayerCaptureRects
//
//  Synopsis:
//      Calculate a set of rectangles that can be used to capture a part of the
//      render target during BeginLayerInternal instead of the entire render
//      target
//
//------------------------------------------------------------------------------
template <typename TRenderTargetLayerData>
__success(return)
bool
CBaseSurfaceRenderTarget<TRenderTargetLayerData>::GetPartialLayerCaptureRects(
    __inout_ecount(1) CRenderTargetLayer *pNewLayer,
    __out_ecount_part(MAX_NUM_PARTIAL_LAYER_CAPTURE_RECTS, *pcCopyRects) CMILSurfaceRect *rgCopyRects,
    __deref_out_range(0,MAX_NUM_PARTIAL_LAYER_CAPTURE_RECTS) UINT *pcCopyRects
    )
{
    bool fPartialLayerCaptureAllowed = false;

    if (   pNewLayer->pGeometricMaskShape == NULL
        || !AlphaScalePreservesOpacity(pNewLayer->rAlpha)
        || !pNewLayer->pGeometricMaskShape->IsAxisAlignedRectangle()
           )
    {
        // none of these cases handled
        goto Cleanup;
    }

    {
        MilRectF rcGeometricMaskF;
        CMilRectL rcGeometricMaskInnerBounds;

        pNewLayer->pGeometricMaskShape->GetFigure(0).GetAsWellOrderedRectangle(rcGeometricMaskF);

        //
        // Note: We always round "in" here. For aliased rendering this may produce more area to
        // copy than we may technically need. Right now we do not expect that our mask has
        // fractional components when we render aliased, so we do not add extra code for this case.
        //
        rcGeometricMaskInnerBounds.left   = CFloatFPU::Ceiling(rcGeometricMaskF.left);
        rcGeometricMaskInnerBounds.top    = CFloatFPU::Ceiling(rcGeometricMaskF.top);
        rcGeometricMaskInnerBounds.right  = CFloatFPU::Floor(rcGeometricMaskF.right);
        rcGeometricMaskInnerBounds.bottom = CFloatFPU::Floor(rcGeometricMaskF.bottom);


        //
        // Side effect of if- inner bounds is intersected with the geometry.
        // Note: The IsEmpty call is necessary since rcGeometricMaskInnerBounds may not
        // be well-ordered and the Insersect call requires well-ordered input rectangles
        //
        if (rcGeometricMaskInnerBounds.IsEmpty() ||
            !rcGeometricMaskInnerBounds.Intersect(pNewLayer->rcLayerBounds)
            )
        {
            // we must capture the entire layer-
            // the geometry is empty or resides outside the layer
            goto Cleanup;
        }

        C_ASSERT(4 == MAX_NUM_PARTIAL_LAYER_CAPTURE_RECTS);

        *pcCopyRects =
            pNewLayer->rcLayerBounds.CalculateSubtractionRectangles(
                    rcGeometricMaskInnerBounds,
                    rgCopyRects,
                    MAX_NUM_PARTIAL_LAYER_CAPTURE_RECTS
                    );

        fPartialLayerCaptureAllowed = true;
    }

Cleanup:
    return fPartialLayerCaptureAllowed;
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseSurfaceRenderTarget::DbgAssertBoundsState
//
//  Synopsis:
//      Check bounds state against current layer state
//

template <typename TRenderTargetLayerData>
void
CBaseSurfaceRenderTarget<TRenderTargetLayerData>::DbgAssertBoundsState()
{
    if (m_LayerStack.GetCount() == 0)
    {
        Assert(m_rcBounds.left   == 0);
        Assert(m_rcBounds.top    == 0);
        Assert(m_rcBounds.right  == static_cast<INT>(m_uWidth));
        Assert(m_rcBounds.bottom == static_cast<INT>(m_uHeight));
    }
    else
    {
        Assert(m_rcBounds.IsWellOrdered());
        Assert(m_rcBounds.left   >= 0);
        Assert(m_rcBounds.top    >= 0);
        Assert(m_rcBounds.right  <= static_cast<INT>(m_uWidth));
        Assert(m_rcBounds.bottom <= static_cast<INT>(m_uHeight));
    }

    return;
}
#endif DBG


// Explicit template instantiation

#include "scanop\scanop.h"
#include "glyph\glyph.h"
#include "sw\sw.h"
#include "hw\hw.h"

template class CBaseSurfaceRenderTarget<CSwRenderTargetLayerData>;
template class CBaseSurfaceRenderTarget<CHwRenderTargetLayerData>;






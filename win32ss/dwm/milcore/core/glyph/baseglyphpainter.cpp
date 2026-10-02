// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//------------------------------------------------------------------------------

#include "precomp.h"

//==============================================================================

CBaseGlyphRunPainter::~CBaseGlyphRunPainter()
{
    ReleaseInterface(m_pRealization);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseGlyphRunPainter::Init
//
//  Synopsis:
//      Prepare to rendering: store painting arguments; check glyphrun
//      visibility (i.e. intersection with clip rect). Returns 'false' if
//      detected as invisible. If happened so, no more action are expected with
//      this class (initialization is left incomplete)
//
//------------------------------------------------------------------------------
bool
CBaseGlyphRunPainter::Init(
    __inout_ecount(1) CGlyphPainterMemory* pGlyphPainterMemory,
    __inout_ecount(1) CGlyphRunResource* pGlyphRunResource,
    __inout_ecount(1) CContextState* pContextState
    )
{
    m_pGlyphPainterMemory = pGlyphPainterMemory;
    m_pGlyphRunResource = pGlyphRunResource;
    m_pContextState = pContextState;

    {
        CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::Device> const
            &xfGlyphRun = pContextState->WorldToDevice;

        m_xfGlyphUR.Set(
            xfGlyphRun._11,
            xfGlyphRun._12,
            xfGlyphRun._21,
            xfGlyphRun._22,
            xfGlyphRun._41,
            xfGlyphRun._42
            );
    }
    m_pAlphaArray = 0;
    m_alphaArraySize = 0;

    {
        // Calculate combined transformation from glyph space to render space.
        // Include translation to position of the first glyph; further calculations
        // would use glyph positions relative to the first one.
        // Glyph space is the one where font height is 1.
        float factor = static_cast<float>(m_pGlyphRunResource->GetMUSize());
        MilPoint2F const& point0 = m_pGlyphRunResource->GetOrigin();

        MILMatrix3x2 xfGlyphGU(factor, 0, 0, factor, point0.X, point0.Y);
        m_xfGlyphGR.SetProduct(xfGlyphGU, m_xfGlyphUR);
    }

    if (m_xfGlyphGR.IsDegenerated())
    {   // glyph shape is degenerated to line or point, hence there
        // is nothing to draw
        return false;
    }

    //   Same scale calculation as in CGlyphRunResource::GetBounds?
    //  They appear to be about the same, but more directly calculated
    //  in CGlyphRunResource::GetBounds.  Why the difference?
    {
        bool fIsPixelAlignable = m_xfGlyphGR.m_01 == 0 &&
            //   What is the usefulness of this test?
            //  If m_xfGlyphGR.m_01 == 0, then m_scaleX will equal |GR.m_00|
            //  whether or not it is calculated with fabs or by
            //    sqrt(GR.m_00*GR.m_00 + 0*0)
            fabsf(m_xfGlyphGR.m_11) > fabsf(m_xfGlyphGR.m_10); // allow 45 degree skew

        // desired horizontal scaling ratio is X basis vector
        if (fIsPixelAlignable)
        {
            m_scaleX = fabsf(m_xfGlyphGR.m_00);
        }
        else
        {
            m_scaleX = sqrtf(m_xfGlyphGR.m_00*m_xfGlyphGR.m_00 + m_xfGlyphGR.m_01*m_xfGlyphGR.m_01);
        }
    }

    // desired vertical scaling ratio is Y basis vector projected to the normal of X basis vector
    m_scaleY = m_scaleX == 0 ? 0
        : fabsf(m_xfGlyphGR.GetDeterminant())/m_scaleX;

    {
        AssertMsg(m_pRealization == NULL, "This can only happen if Init has been called twice.");
       
        // Convert desired scaling ratios to available ones
        bool fRealizationAvailable = m_pGlyphRunResource->GetAvailableScale(
            &m_scaleX,
            &m_scaleY,
            m_pContextState->GetCurrentOrDefaultDisplaySettings(),
            m_pContextState->RenderState->TextRenderingMode,
            m_pContextState->RenderState->TextHintingMode,
            &m_recommendedBlendMode,
            &m_pRealization,
            m_pContextState->GetDpiProvider()
            );

        if (fRealizationAvailable)
        {
            //
            // Perform glyph run origin pixel snapping as necessary
            // We need to snap if:
            // - Text is display measured
            // - Text is ideal measured, but DWrite has provided embedded bilevel bitmaps in the
            //   realization (essentially making the text bilevel regardless of API request/system 
            //   system settings for font smoothing)
            //   
            if (m_pGlyphRunResource->IsDisplayMeasured() || m_pRealization->IsBiLevelOnly())
            {
                if (m_pContextState->WorldToDevice.IsTranslateOrScale())
                {
                    //
                    // If we're measuring using a display mode and the text is positioned rectilinearly, 
                    // we want to align the bitmaps with whole pixel boundaries for maximum contrast and clarity.
                    // To do this, we move the X offset of the m_xfGlyphGR transform to the nearest pixel.
                    // Baseline snapping will modify the Y component, if appropriate.
                    //
                    // Note that the above test implies that m_xfGlyphGR satisfies the same properties, since
                    // it is generated originally from WorldToDevice, then scaled and offset, preserving the properties
                    //
                    m_xfGlyphGR.m_20 = (float)CFloatFPU::Round(m_xfGlyphGR.m_20);
                }
                else if (m_pContextState->WorldToDevice.Is2DAxisAlignedPreservingApproximate())
                {
                    // If we're drawing text rotated 90 degrees, we want to snap the bitmap in the Y 
                    // direction to prevent stems from bleeding onto more than 1 scanline, which causes
                    // blurriness. We also snap the bitmap in the X direction to prevent the less problematic
                    // issue of stems being lengthened by bleeding.
                    //
                    // Note that the above test implies that m_xfGlyphGR satisfies the same properties, since
                    // it is generated originally from WorldToDevice, then scaled and offset, preserving the properties
                    //
                    m_xfGlyphGR.m_21 = (float)CFloatFPU::Round(m_xfGlyphGR.m_21);
                    m_xfGlyphGR.m_20 = (float)CFloatFPU::Round(m_xfGlyphGR.m_20);
                }
            }
        }
            
        return fRealizationAvailable;
        // 'false' can be returned when glyph run has no realizations,
        // which in turn can be caused by IMILGlyphSource implementation (CFontCacheReader)
        // who failed to create bitmap for some reason.
        // We treat this case soft way, skipping this glyph run rendering.
    }
}

static const UINT sc_uCriticalTime = 200; // msec
static const UINT sc_uWakeTime = 50; // msec
static const float sc_rAllowedStep = 0.05f; // pixel
static const float sc_rBigJumpThreshold = 3.0f; // pixels

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseGlyphRunPainter::PrepareTransforms
//
//  Synopsis:
//      Prepare to rendering given glyph run, either HW or SW
//
//------------------------------------------------------------------------------
HRESULT
CBaseGlyphRunPainter::PrepareTransforms()
{
    HRESULT hr = S_OK;

    //
    // Split m_xfGlyphGR into two transformations, from glyph to intermediate
    // work space (m_xfGlyphGW) and from work to render space (m_xfGlyphWR).
    // The first one should be scaling only, and the second should provide
    // good texture interpolation.
    //
    m_xfGlyphGW.SetScaling(m_scaleX, m_scaleY);

    MILMatrix3x2 xfGlyphWG;
    xfGlyphWG.SetInverse(m_xfGlyphGW);
    m_xfGlyphWR.SetProduct(xfGlyphWG, m_xfGlyphGR);

    // Check whether scaling ratios are too small.
    // Disable clear type if so, otherwise we'll get color fringes.
    m_fDisableClearType = (m_xfGlyphWR.GetDeterminant() < sc_uCriticalScaleDown*0.01f);

    //
    // Apply pixel snapping. It may change m_xfGlyphWR.
    //

    {
        Assert(m_pContextState);
        CSnappingFrame *pFrame = m_pContextState->m_pSnappingStack;
        if (pFrame && !pFrame->IsEmpty())
        {
            MilPoint2F point = {m_xfGlyphWR.m_20, m_xfGlyphWR.m_21};

            pFrame->SnapPoint(point);

            // use only Y offset, keep X
            m_xfGlyphWR.m_21 = point.Y;
        }
    }

    //
    // Finalize transformation splitting
    //
    m_xfGlyphRW.SetInverse(m_xfGlyphWR);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseGlyphRunPainter::MakeAlphaMap
//
//  Synopsis:
//      prepare data for rendering:
//       - bounding rectangle
//       - filtered alpha array
//
//------------------------------------------------------------------------------
void
CBaseGlyphRunPainter::MakeAlphaMap(
    __inout_ecount(1) CBaseGlyphRun *pRun
    )
{
    m_pRealization->GetAlphaMap(
        &m_pAlphaArray, 
        &m_alphaArraySize,
        &(pRun->m_rcFiltered));

    if (IsRectEmpty(&(pRun->m_rcFiltered)))
    {
        pRun->SetEmpty();
    }
}


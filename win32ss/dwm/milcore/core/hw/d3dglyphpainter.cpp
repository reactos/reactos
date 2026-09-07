// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      see comments in header file.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

CD3DGlyphRunPainter::CD3DGlyphRunPainter()
{
    m_data.m_pHwColorSource = NULL;
}

CD3DGlyphRunPainter::~CD3DGlyphRunPainter()
{
    ReleaseInterfaceNoNULL(m_data.m_pHwColorSource);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphRunPainter::Paint
//
//  Synopsis:
//      execute glyphrun rendering
//
//------------------------------------------------------------------------------
HRESULT
CD3DGlyphRunPainter::Paint(
    __in_ecount(1) DrawGlyphsParameters &pars,
    bool fTargetSupportsClearType,
    __inout_ecount(1) CD3DDeviceLevel1* pDevice,
    MilPixelFormat::Enum fmtTargetSurface
    )
{
    HRESULT hr = S_OK;
        
    m_pDevice = pDevice;

    BOOL fVisible = FALSE;
    MilPointAndSizeL rcClip;

    Assert(pars.pContextState);

    FLOAT flAlphaScale = pars.pBrushRealizer->GetOpacityFromRealizedBrush();

    m_pDevice->GetClipRect(&rcClip);

    {
        // Do a rough check for glyph run visibility.
        // We need it, at least, to protect against
        // overflows in rendering routines.
        CRectF<CoordinateSpace::Device> rcClipF(
            static_cast<float>(rcClip.X),
            static_cast<float>(rcClip.Y),
            static_cast<float>(rcClip.Width),
            static_cast<float>(rcClip.Height),
            XYWH_Parameters
            );

        if (!pars.rcBounds.Device().DoesIntersect(rcClipF))
            goto Cleanup;
    }

    fVisible = Init(
        m_pDevice->GetGlyphBank()->GetGlyphPainterMemory(),
        pars.pGlyphRun,
        pars.pContextState
        );

    if (!fVisible) goto Cleanup;

    IFC(ValidateGlyphRun());

    IFC(m_pGlyphRun->ValidateGeometry(this));

    if (m_pGlyphRun->IsEmpty())
    {
        // This is legal, for example if the glyph run is realized at such a small scale that
        // there is nothing to draw.
        goto Cleanup;
    }

    bool fClearType = (m_recommendedBlendMode == ClearType) && fTargetSupportsClearType;

    // Rendering preparation:
    // Choose rendering branch (set m_pfnDrawRectangle)
    // Set rendering state

    IFC(InspectBrush(pars, fmtTargetSurface));

    if (m_data.m_pHwColorSource == NULL)
    {   // solid brush

        // if the brush has zero alpha then skip drawing
        if (MIL_COLOR_GET_ALPHA(m_data.color) == 0)
            goto Cleanup;

        // We should not get here with alpha effect.
        // It should be combined with solid brush already.

        Assert(flAlphaScale == 1);

        if (fClearType)
        {
            m_pfnDrawRectangle = m_pDevice->CanDrawTextUsingPS20()
                ? sc_pfnDrawRectangle_CVertM1_CT_1Pass
                : sc_pfnDrawRectangle_CVertM3_1Pass;

            IFC(m_pDevice->SetRenderState_Text_ClearType_SolidBrush(m_data.color, pars.pGlyphRun->GetGammaIndex()));
        }
        else
        {
            m_pfnDrawRectangle = sc_pfnDrawRectangle_CVertM1_1Pass;
            IFC(m_pDevice->SetRenderState_Text_GreyScale_SolidBrush(m_data.color, pars.pGlyphRun->GetGammaIndex()));
        }
    }
    else
    {   // textured brush
        if (fClearType)
        {
            m_pfnDrawRectangle = sc_pfnDrawRectangle_CVertBM_3Pass;
            IFC(m_pDevice->SetRenderState_Text_ClearType_TextureBrush(pars.pGlyphRun->GetGammaIndex(), flAlphaScale));
        }
        else
        {
            m_pfnDrawRectangle = sc_pfnDrawRectangle_CVertBM_1Pass;
            IFC(m_pDevice->SetRenderState_Text_GreyScale_TextureBrush(pars.pGlyphRun->GetGammaIndex(), flAlphaScale));
        }
    }

    //
    // do rendering
    //

    for (m_pSubGlyph = m_pGlyphRun->GetFirstSubGlyph(); m_pSubGlyph; m_pSubGlyph = m_pSubGlyph->GetNext())
    {
        RECT const &r = m_pSubGlyph->GetFilteredRect();

        m_xMin = (float(r.left  ) + float(DX9_SUBGLYPH_OVERLAP_X) * .5f) * (1.f / 3);
        m_xMax = (float(r.right ) - float(DX9_SUBGLYPH_OVERLAP_X) * .5f) * (1.f / 3);

        m_yMin =  float(r.top   ) + float(DX9_SUBGLYPH_OVERLAP_Y) * .5f;
        m_yMax =  float(r.bottom) - float(DX9_SUBGLYPH_OVERLAP_Y) * .5f;

        if (m_pGlyphRun->IsBig() && IsSubglyphClippedOut(&rcClip)) continue;

        if (!m_pSubGlyph->IsAlphaMapValid())
        {
#if D3DLOG_ENABLED
            D3DLOG_INC(subglyphsRegenerated);
            D3DLOG_ADD(pixelsRegenerated, (r.right - r.left) * (r.bottom - r.top));
            if (m_pGlyphRun->IsPersistent())
            {
                D3DLOG_INC(persSubglyphsRegenerated);
            }
            if (m_pSubGlyph->WasEvicted())
            {
                D3DLOG_INC(subglyphsEvicted);
            }
#endif //D3DLOG_ENABLED

            IFC(m_pSubGlyph->ValidateAlphaMap(this));
        }
        else
        {
            D3DLOG_INC(subglyphsReused);
            D3DLOG_ADD(pixelsReused, (r.right - r.left) * (r.bottom - r.top));
        }

        m_data.pxfGlyphWR = &m_xfGlyphWR;

        float widTextureRc = m_pSubGlyph->GetWidTextureRc();
        float heiTextureRc = m_pSubGlyph->GetHeiTextureRc();
        SIZE const& offset = m_pSubGlyph->GetOffset();

        // scaling transform from work space to glyph texture space
        m_data.kxWT = widTextureRc * 3;
        m_data.kyWT = heiTextureRc;
        m_data.dxWT = widTextureRc * (float(offset.cx));
        m_data.dyWT = heiTextureRc * (float(offset.cy));

        m_data.pMaskTexture = m_pSubGlyph->GetTank()->GetTextureNoAddref();
        m_pSubGlyph->GetTank()->AddUsefulArea((r.right - r.left) * (r.bottom - r.top));

        float
            blueOffset = pars.pGlyphRun->BlueSubpixelOffset(),
            dxW = blueOffset * m_xfGlyphRW.m_00,
            dyW = blueOffset * m_xfGlyphRW.m_01;

        m_data.blueOffset = blueOffset;
        m_data.ds = dxW * m_data.kxWT;
        m_data.dt = dyW * m_data.kyWT;

        MIL_THR((this->*m_pfnDrawRectangle)());

        if (!m_pGlyphRun->IsPersistent())
        {
            m_pSubGlyph->FreeAlphaMap();
        }
        if (FAILED(hr)) goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphRunPainter::IsSubglyphClippedOut
//
//  Synopsis:
//      Do a fast "pre-clip" check.
//
//  Return values:
//      true:  Current subglyph is definitely outside the given clipping rectangle
//      false: Current subglyph might be visible
//
//------------------------------------------------------------------------------

MIL_FORCEINLINE bool
CD3DGlyphRunPainter::IsSubglyphClippedOut(MilPointAndSizeL const* prcClip) const
{
    float
        x0 = m_xMin*m_xfGlyphWR.m_00 + m_yMin*m_xfGlyphWR.m_10 + m_xfGlyphWR.m_20,
        y0 = m_xMin*m_xfGlyphWR.m_01 + m_yMin*m_xfGlyphWR.m_11 + m_xfGlyphWR.m_21,

        x1 = m_xMax*m_xfGlyphWR.m_00 + m_yMin*m_xfGlyphWR.m_10 + m_xfGlyphWR.m_20,
        y1 = m_xMax*m_xfGlyphWR.m_01 + m_yMin*m_xfGlyphWR.m_11 + m_xfGlyphWR.m_21,

        x2 = m_xMax*m_xfGlyphWR.m_00 + m_yMax*m_xfGlyphWR.m_10 + m_xfGlyphWR.m_20,
        y2 = m_xMax*m_xfGlyphWR.m_01 + m_yMax*m_xfGlyphWR.m_11 + m_xfGlyphWR.m_21,

        x3 = m_xMin*m_xfGlyphWR.m_00 + m_yMax*m_xfGlyphWR.m_10 + m_xfGlyphWR.m_20,
        y3 = m_xMin*m_xfGlyphWR.m_01 + m_yMax*m_xfGlyphWR.m_11 + m_xfGlyphWR.m_21,

        lef = float(prcClip->X                  ),
        rig = float(prcClip->X + prcClip->Width ),
        top = float(prcClip->Y                  ),
        bot = float(prcClip->Y + prcClip->Height);

    return
        x0 > rig && x1 > rig && x2 > rig && x3 > rig ||
        x0 < lef && x1 < lef && x2 < lef && x3 < lef ||
        y0 > bot && y1 > bot && y2 > bot && y3 > bot ||
        y0 < top && y1 < top && y2 < top && y3 < top;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphRunPainter::ValidateGlyphRun
//
//  Synopsis:
//      Create device specific CD3DGlyphRun if not yet created
//
//------------------------------------------------------------------------------
HRESULT
CD3DGlyphRunPainter::ValidateGlyphRun()
{
    HRESULT hr = S_OK;
    IMILResourceCache::ValidIndex uCacheIndex;

    IFC(m_pDevice->GetCacheIndex(&uCacheIndex));

    IFC(GetRealizationNoRef()->GetD3DGlyphRun(uCacheIndex, &m_pGlyphRun));

    if (!m_pGlyphRun)
    {
        m_pGlyphRun = new CD3DGlyphRun();
        IFCOOM(m_pGlyphRun);

        GetRealizationNoRef()->SetD3DGlyphRun(uCacheIndex, m_pGlyphRun);
    }
    else
    {
        m_pGlyphRun->SetPersistent();
    }

Cleanup:
    return hr;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphRunPainter::InspectBrush
//
//  Synopsis:
//      Inspect brush type and debug settings. Prepare either solid color value
//      in m_data.color or pointer to CHwTexturedColorSource in
//      m_data.m_pHwColorSource.
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE HRESULT
CD3DGlyphRunPainter::InspectBrush(
    __in_ecount(1) DrawGlyphsParameters &pars,
    MilPixelFormat::Enum fmtTargetSurface
    )
{
    HRESULT hr = S_OK;

    BOOL fUseBorder = FALSE;

    CContextState const *pContextState = pars.pContextState;
    CMILBrush* pMILBrush = pars.pBrushRealizer->GetRealizedBrushNoRef(false /* fConvertNULLToTransparent */);
    Assert(pMILBrush);

    switch (pMILBrush->GetType())
    {
    case BrushSolid:
        {
            const CMILBrushSolid* pMILBrushSolid = DYNCAST(CMILBrushSolid, pMILBrush);
            Assert(pMILBrushSolid);
            m_data.color = Convert_MilColorF_scRGB_To_MilColorB_sRGB(&pMILBrushSolid->m_SolidColor);
            break;
        }
    
    case BrushBitmap:
        {
            const CMILBrushBitmap *pBitmapBrush = DYNCAST(const CMILBrushBitmap, pMILBrush);
            Assert(pBitmapBrush);
            fUseBorder = pBitmapBrush->HasSourceClip();
            if (fUseBorder)
            {
                Assert(m_pDevice->SupportsBorderColor());
                Assert(pBitmapBrush->SourceClipIsEntireSource());
            }

            // fall thru to default
        }

        __fallthrough;
    
    default:
        {
            //
            // For 2D rendering, local rendering and world sampling spaces are identical
            //

            const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &
                matBaseSamplingToDevice =
                ReinterpretLocalRenderingAsBaseSampling(pContextState->WorldToDevice);

            CHwBrushContext hwBrushContext(
                pContextState,
                matBaseSamplingToDevice, // matWorld2DToSampleSpace -- In 2D the sample space is the same as the Device space.
                fmtTargetSurface,
                TRUE // fCanFallback
                );
            
            hwBrushContext.SetDeviceSamplingBounds(pars.rcBounds.Device());

            Assert(m_data.m_pHwColorSource == NULL);
    
            IFC(m_pDevice->DeriveHWTexturedColorSource(
                pMILBrush,
                hwBrushContext,
                &m_data.m_pHwColorSource
                ));
            IFC(m_data.m_pHwColorSource->Realize());

            if (fUseBorder)
            {
                m_data.m_pHwColorSource->ForceBorder();
            }

            m_data.xfBrushRT = m_data.m_pHwColorSource->GetDevicePointToTextureUV();
        }
    }

Cleanup:
    RRETURN(hr);
}

//==============================================================================

HRESULT
CD3DGlyphRunPainter::EnsureAlphaMap()
{  
    if (!HasAlphaArray()) 
    {
        // If we don't have an alpha map array, create one from the realization.
        Assert(m_pGlyphRun);

        MakeAlphaMap(m_pGlyphRun);
    }

     RRETURN(S_OK);
}


//+-----------------------------------------------------------------------------
//
//  Class:
//      CRenderFan1Pass
//
//  Synopsis:
//      Draws the accumulated vertex buffer as a fan primitive
//
//------------------------------------------------------------------------------
class CRenderFan1Pass
{
public:
    template<class TVertexBuffer>
    static HRESULT Draw(
        CD3DDeviceLevel1* pDev,
        TVertexBuffer *pBuffer,
        char* pvb,
        int stride,
        DWORD color,
        float blueOffset
        )
    {
        return pDev->EndPrimitiveFan(pBuffer);
    }
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CRenderFan3Pass
//
//  Synopsis:
//      Draw the accumulated vertex buffer as a fan primitive producing clear
//      type text rendering, with pixel shader that makes gamma correction.
//
//      The final image is generated in three passes, one per each color
//      component.
//
//------------------------------------------------------------------------------
class CRenderFan3Pass
{
public:
    template<class TVertexBuffer>
    static HRESULT Draw(
        CD3DDeviceLevel1* pDev,
        TVertexBuffer *pBuffer,
        char* pvb,
        int stride,
        DWORD color,
        float blueOffset
        )
    {
        HRESULT hr = S_OK;

        // Draw the green
        IFC( pDev->SetColorChannelGreen() );

        IFC( pDev->EndPrimitiveFan(pBuffer) );
        
        // Draw the red:
        // shift all the x-coordinates right by 1/3 pixel
        // (LCD display will shift them back as far as the red stripes
        //  lay in left part of the pixel consisting of R,G and B stripes)
        // For BGR display use oppose shift direction.
        // This choice is controlled by DisplaySettings::BlueSubpixelOffset
        // that is 1/3 for default RGB display and -1/3 for BGR.
        {
            float offset = blueOffset;
            *(float*)(pvb + stride*0) += offset;
            *(float*)(pvb + stride*1) += offset;
            *(float*)(pvb + stride*2) += offset;
            *(float*)(pvb + stride*3) += offset;
        }
        IFC( pDev->SetColorChannelRed() );

        IFC( pDev->EndPrimitiveFan(pBuffer) );

        // Draw the blue
        // Do the shift in opposite direction
        {
            float offset = 2*blueOffset;
            *(float*)(pvb + stride*0) -= offset;
            *(float*)(pvb + stride*1) -= offset;
            *(float*)(pvb + stride*2) -= offset;
            *(float*)(pvb + stride*3) -= offset;
        }
        IFC( pDev->SetColorChannelBlue() );

        IFC( pDev->EndPrimitiveFan(pBuffer) );

Cleanup:
        // restore the default settings
        {
            MIL_THR_SECONDARY(pDev->RestoreColorChannels());
        }
        RRETURN(hr);
    }
};


template<class TVertex, class TRender> HRESULT
CD3DGlyphRunPainter::TDrawRectangle()
{
    HRESULT hr = S_OK;

    TVertex* vertices = NULL;

    TVertex::buffer *pBuffer;
    IFC(m_pDevice->StartPrimitive(&pBuffer));
    IFC(pBuffer->GetNewVertices(
        4,
        (TVertex::base **)&vertices
        ));

    vertices[0].Set(m_xMin, m_yMin, &m_data);
    vertices[1].Set(m_xMax, m_yMin, &m_data);
    vertices[2].Set(m_xMax, m_yMax, &m_data);
    vertices[3].Set(m_xMin, m_yMax, &m_data);

    IFC( TVertex::SetTextures(m_pDevice, &m_data) );

    IFC( TRender::Draw(
        m_pDevice,
        pBuffer,
        (char*)vertices,
        sizeof(TVertex),
        m_data.color,
        m_data.blueOffset
        ) );

Cleanup:
    return hr;
}

//==============================================================================
// Vertex classes
// 1 mask
class CVertM1 : public CD3DVertexXYZDUV2
{
public:
    typedef CD3DVertexXYZDUV2 base;
    typedef CD3DVertexBufferDUV2 buffer;
    MIL_FORCEINLINE void
    Set(float xW, float yW, const VertexFillData* pData)
    {
        MILMatrix3x2 const &mWR = *pData->pxfGlyphWR;
        float xR = xW*mWR.m_00 + yW*mWR.m_10 + mWR.m_20,
              yR = xW*mWR.m_01 + yW*mWR.m_11 + mWR.m_21;

        float u0 = xW*pData->kxWT + pData->dxWT;
        float v0 = yW*pData->kyWT + pData->dyWT;

        SetXYUV0(xR, yR, u0, v0);
    }

    static MIL_FORCEINLINE HRESULT
    SetTextures(CD3DDeviceLevel1* pDevice, const VertexFillData* pData)
    {
        HRESULT hr = S_OK;
        IFC( pDevice->SetD3DTexture(0, pData->pMaskTexture));
        IFC( pDevice->DisableTextureTransform(0));
    Cleanup:
        return hr;
    }
};

class CVertM1_CT : public CVertM1
{
public:
    static MIL_FORCEINLINE HRESULT
    SetTextures(CD3DDeviceLevel1* pDevice, const VertexFillData* pData)
    {
        HRESULT hr = S_OK;
        IFC( pDevice->SetClearTypeOffsets(pData->ds, pData->dt) );
        IFC( pDevice->SetD3DTexture(0, pData->pMaskTexture) );
        IFC( pDevice->DisableTextureTransform(0) );
    Cleanup:
        return hr;
    }
};

// brush + mask
class CVertBM : public CD3DVertexXYZDUV2
{
public:
    typedef CD3DVertexXYZDUV2 base;
    typedef CD3DVertexBufferDUV2 buffer;
    MIL_FORCEINLINE void
    Set(float xW, float yW, const VertexFillData* pData)
    {
        MILMatrix3x2 const &mWR = *pData->pxfGlyphWR;

        float xR = xW*mWR.m_00 + yW*mWR.m_10 + mWR.m_20,
              yR = xW*mWR.m_01 + yW*mWR.m_11 + mWR.m_21;

        MILMatrix3x2 const &mRT = pData->xfBrushRT;

        SetXYUV1(xR,                                    // X
                 yR,                                    // Y
                 xR*mRT.m_00 + yR*mRT.m_10 + mRT.m_20,  // U0
                 xR*mRT.m_01 + yR*mRT.m_11 + mRT.m_21,  // V0
                 xW*pData->kxWT + pData->dxWT,          // U1
                 yW*pData->kyWT + pData->dyWT           // V1
                 );
    }

    static MIL_FORCEINLINE HRESULT
    SetTextures(CD3DDeviceLevel1* pDevice, const VertexFillData* pData)
    {
        HRESULT hr = S_OK;

        //
        // ResetForPipelineReuse must be called any time
        // SendDeviceStates is called outside the normal pipeline.
        //
        pData->m_pHwColorSource->ResetForPipelineReuse();
        IFC( pData->m_pHwColorSource->SendDeviceStates(0, 0) );
        IFC( pDevice->SetD3DTexture(1, pData->pMaskTexture));
        IFC( pDevice->DisableTextureTransform(1));
    Cleanup:
        return hr;
    }
};

// 3 masks
class CVertM3 : public CD3DVertexXYZDUV6
{
public:
    typedef CD3DVertexXYZDUV6 base;
    typedef CD3DVertexBufferDUV6 buffer;
    MIL_FORCEINLINE void
    Set(float xW, float yW, const VertexFillData* pData)
    {
        MILMatrix3x2 const &mWR = *pData->pxfGlyphWR;

        float xR = xW*mWR.m_00 + yW*mWR.m_10 + mWR.m_20,
              yR = xW*mWR.m_01 + yW*mWR.m_11 + mWR.m_21;

        float u1 = xW*pData->kxWT + pData->dxWT;  // green channel
        float v1 = yW*pData->kyWT + pData->dyWT;
        float u0 = u1 - pData->ds;                // red channel
        float v0 = v1 - pData->dt;
        float u2 = u1 + pData->ds;                // blue channel
        float v2 = v1 + pData->dt;

        SetXYUV2( xR,       // X
                  yR,       // Y
                  u0, v0,   // U0, V0
                  u1, v1,   // U1, V1
                  u2, v2    // U2, V2
                  );
    }

    static MIL_FORCEINLINE HRESULT
    SetTextures(CD3DDeviceLevel1* pDevice, const VertexFillData* pData)
    {
        HRESULT hr = S_OK;
        IFC( pDevice->SetD3DTexture(0, pData->pMaskTexture) );
        IFC( pDevice->DisableTextureTransform(0) );
        IFC( pDevice->SetD3DTexture(1, pData->pMaskTexture) );
        IFC( pDevice->DisableTextureTransform(1) );
        IFC( pDevice->SetD3DTexture(2, pData->pMaskTexture) );
        IFC( pDevice->DisableTextureTransform(2) );
Cleanup:
        return hr;
    }
};

//
// [pfx_parse] - workaround for PREfix parse problems
//
#ifndef _PREFIX_
HRESULT (CD3DGlyphRunPainter::*const CD3DGlyphRunPainter::sc_pfnDrawRectangle_CVertM1_1Pass)() = &CD3DGlyphRunPainter::TDrawRectangle<CVertM1, CRenderFan1Pass>;
HRESULT (CD3DGlyphRunPainter::*const CD3DGlyphRunPainter::sc_pfnDrawRectangle_CVertBM_1Pass)() = &CD3DGlyphRunPainter::TDrawRectangle<CVertBM, CRenderFan1Pass>;
HRESULT (CD3DGlyphRunPainter::*const CD3DGlyphRunPainter::sc_pfnDrawRectangle_CVertM3_1Pass)() = &CD3DGlyphRunPainter::TDrawRectangle<CVertM3, CRenderFan1Pass>;
HRESULT (CD3DGlyphRunPainter::*const CD3DGlyphRunPainter::sc_pfnDrawRectangle_CVertBM_3Pass)() = &CD3DGlyphRunPainter::TDrawRectangle<CVertBM, CRenderFan3Pass>;
HRESULT (CD3DGlyphRunPainter::*const CD3DGlyphRunPainter::sc_pfnDrawRectangle_CVertM1_CT_1Pass)() = &CD3DGlyphRunPainter::TDrawRectangle<CVertM1_CT, CRenderFan1Pass>;
#endif // !_PREFIX_





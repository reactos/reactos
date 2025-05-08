// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//

#include "precomp.hpp"

MtDefine(CHwSoftwareFallback, MILRender, "CHwSoftwareFallback");
MtDefine(MHwSoftwareFallbackScanlineBuffers, MILRawMemory, "MHwSoftwareFallbackScanlineBuffers");

//+------------------------------------------------------------------------
//
//  Function:   CHwSoftwareFallback::CHwSoftwareFallback
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------
CHwSoftwareFallback::CHwSoftwareFallback()
{
    m_pD3DDevice = NULL;

    m_uMaxTileWidth = 128;
    m_uMaxTileHeight = 128;

    m_pCurrentTile = m_tiles;

    m_fAlphaTexturesInited = FALSE;
}

//+------------------------------------------------------------------------
//
//  Function:   CHwSoftwareFallback::Init
//
//  Synopsis:   Associated a d3d device and create temporary textures
//              that we need here.
//
//-------------------------------------------------------------------------
HRESULT 
CHwSoftwareFallback::Init(
    __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice
    )
{
    HRESULT hr = S_OK;

    //
    // Set the D3DDevice
    //
    
    AssertMsg(m_pD3DDevice == NULL, "CHwSoftwareFallback::Init called twice");
    m_pD3DDevice = pD3DDevice;
    // Do not AddRef the device since this is a resource used by the device.
    //  => A reference here would be circular.

    //
    // Create the temporary textures we need 
    //

    D3DSURFACE_DESC d3dsd;

    ZeroMemory(&d3dsd, sizeof(d3dsd));
    d3dsd.Format = D3DFMT_A8R8G8B8;
    d3dsd.Type = D3DRTYPE_TEXTURE;
    d3dsd.Usage = 0;
    d3dsd.Pool = pD3DDevice->GetManagedPool();
    d3dsd.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dsd.MultiSampleQuality = 0;
    d3dsd.Width = m_uMaxTileWidth;
    d3dsd.Height = m_uMaxTileHeight;

    IFC(pD3DDevice->GetMinimalTextureDesc(
        &d3dsd,
        TRUE,
        GMTD_CHECK_ALL
        ));

    Assert(d3dsd.Format == D3DFMT_A8R8G8B8);

    // if our initial guess exceeds texture limits, we need to adjust it
    Assert(d3dsd.Width > 0);
    Assert(d3dsd.Height > 0);
    m_uMaxTileWidth = d3dsd.Width;
    m_uMaxTileHeight = d3dsd.Height;

    //
    // Create cached bitmap and clear the contents
    //

    for (int i = 0; i < NUM_FALLBACK_TILES; i++)
    {
        CD3DLockableTexture *pTexture = NULL;

        IFC(m_pD3DDevice->CreateLockableTexture(
            &d3dsd,
            &pTexture
            ));
        
        Assert(pTexture);

        m_tiles[i].InitMain(pTexture);
        pTexture->Release();
    }

    IFC(m_IntermediateBuffers.AllocateBuffers(
        Mt(MHwSoftwareFallbackScanlineBuffers),
        m_uMaxTileWidth
        ));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CHwSoftwareFallback::InitAlphaTextures
//
//  Synopsis:   Create temporary textures to store vector alpha data
//              that we need for text rendering in clear type mode.
//              Clear type requires 6 components per texel: three colors
//              and three alphas (separate alpha for each color component).
//              As far as DX allows only four components, we store
//              alphas separately in m_tiles[i].m_pAuxTexture, while
//              m_tiles[i].m_pMainTexture stores colors.
//-------------------------------------------------------------------------
HRESULT 
CHwSoftwareFallback::InitAlphaTextures()
{
    HRESULT hr = S_OK;

    Assert(!m_fAlphaTexturesInited);

    D3DSURFACE_DESC d3dsd;

    ZeroMemory(&d3dsd, sizeof(d3dsd));
    d3dsd.Format = D3DFMT_A8R8G8B8;
    d3dsd.Type = D3DRTYPE_TEXTURE;
    d3dsd.Usage = 0;
    d3dsd.Pool = m_pD3DDevice->GetManagedPool();
    d3dsd.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dsd.MultiSampleQuality = 0;
    d3dsd.Width = m_uMaxTileWidth;
    d3dsd.Height = m_uMaxTileHeight;

    IFC(m_pD3DDevice->GetMinimalTextureDesc(
        &d3dsd,
        TRUE,
        GMTD_CHECK_ALL
        ));

    Assert(d3dsd.Format == D3DFMT_A8R8G8B8);

    for (int i = 0; i < NUM_FALLBACK_TILES; i++)
    {
        CD3DLockableTexture *pTexture = NULL;

        IFC(m_pD3DDevice->CreateLockableTexture(
            &d3dsd,
            &pTexture
            ));
        
        Assert(pTexture);

        m_tiles[i].InitAux(pTexture);
        pTexture->Release();
    }

    m_fAlphaTexturesInited = TRUE;
Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CHwSoftwareFallback::ComputeFirstTile
//
//  Synopsis:   Takes in the shape bounds, the target size, and a tile bounds.
//              Intersects the shape bounds with the target size, sets the 
//              shape bounds and tile bounds to that intersection, and then
//              clamps the tile bounds to within MaxTileWidth and MaxTileHeight;
//
//  Returns:    FALSE if resulting rectangle is empty
//-------------------------------------------------------------------------
BOOL
CHwSoftwareFallback::ComputeFirstTile(UINT uTargetWidth, UINT uTargetHeight)
{
    MilPointAndSizeL rcSurfaceSize;
    MilPointAndSizeL rcIntersectionResult;
    
    Assert(m_uMaxTileWidth > 0);
    Assert(m_uMaxTileHeight > 0);
    
    rcSurfaceSize.X = 0;
    rcSurfaceSize.Y = 0;
    rcSurfaceSize.Width = static_cast<INT>(uTargetWidth);
    rcSurfaceSize.Height = static_cast<INT>(uTargetHeight);

    //
    // Calculate the intersection of the screensize and the shapebounds
    //

    IntersectRect(&rcIntersectionResult, &m_rcShapeBounds, &rcSurfaceSize);
    if (rcIntersectionResult.Width <= 0 || rcIntersectionResult.Height <= 0)
    {
        return FALSE;
    }

    m_rcShapeBounds = rcIntersectionResult;

    //
    // Copy the intersection to the tile, clamping the width and height to the maximum for
    // tiles.
    //

    m_rcCurrentTile.X = rcIntersectionResult.X;
    m_rcCurrentTile.Y = rcIntersectionResult.Y;
    m_rcCurrentTile.Width = min(static_cast<INT>(m_uMaxTileWidth), rcIntersectionResult.Width);
    m_rcCurrentTile.Height = min(static_cast<INT>(m_uMaxTileHeight), rcIntersectionResult.Height);

    return TRUE;
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSoftwareFallback::DrawBitmap
//
//  Synopsis:  Draw the bitmap using the software rastizer with our 
//             temporary surfaces.  Note that this is done by using
//             a system memory texture and copying to the surface
//             at some later point in time.  
//
//-------------------------------------------------------------------------
HRESULT 
CHwSoftwareFallback::DrawBitmap(
    __in_ecount(1) const CContextState *pContextState,
    __inout_ecount(1) IWGXBitmapSource *pIBitmap,
    __in_ecount_opt(1) IMILEffectList *pIEffect,
    UINT uTargetWidth,
    UINT uTargetHeight
    )
{
    HRESULT hr = S_OK;
    CRectF<CoordinateSpace::Device> rcTrueBounds;
    
    UINT uiImageWidth;
    UINT uiImageHeight;

    //
    // Check sw fallback mode
    //

    if (g_pMediaControl)
    {
        if (g_pMediaControl->GetDataPtr()->PrimitiveSoftwareFallbackDisabled)
        {
            goto Cleanup;
        }
    }

    //
    // Draw the primitive 
    //

    IFC(pIBitmap->GetSize(&uiImageWidth, &uiImageHeight));

    pContextState->WorldToDevice.Transform2DBounds(
        CRectF<CoordinateSpace::LocalRendering>(0.0f, 0.0f, (float)uiImageWidth, (float)uiImageHeight, XYWH_Parameters),
        rcTrueBounds
        );

    // Allow one more pixel for possible pixel snapping offset.
    // We should do it because rcTrueBounds here is not yet affected by pixel
    // snapping offset. 
    rcTrueBounds.Inflate(1, 1);

    // convert floats to ints
    IFC(InflateRectFToPointAndSizeL(rcTrueBounds, OUT m_rcShapeBounds));

    for (BOOL fHaveTile = ComputeFirstTile(uTargetWidth, uTargetHeight);
              fHaveTile;
              fHaveTile = ComputeNextTile())
    {
        //
        // We need the destruction of the lock before DrawCurrentTile,
        // therefore we need these brakets here
        //
        {
            CD3DLockableTexturePairLock lock(m_pCurrentTile);
            CRectClipper oClipper;
            oClipper.SetClip(CMILSurfaceRect(m_rcCurrentTile));

            // Lock and CLEAR the current tile
            IFC(lock.Lock(m_rcCurrentTile.Width, m_rcCurrentTile.Height, m_lockData));

            //
            // Call the software rasterizer
            //

            IFC(m_sr.DrawBitmap(
                this,
                &oClipper,
                pContextState, 
                pIBitmap, 
                pIEffect
                ));
        }
        //
        // Draw the current tile
        //

        IFC(DrawCurrentTile());
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSoftwareFallback::FillPath
//
//  Synopsis:  Draw the path using the software rasterizer with our 
//             temporary surfaces.  Note that this is done by using
//             a system memory texture and copying to the surface
//             at some later point in time.  
//
//-------------------------------------------------------------------------
HRESULT 
CHwSoftwareFallback::FillPath(
    __in_ecount(1) const CContextState *pContextState,
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice,
    __in_ecount(1) const IShapeData *pShape,
    __in_ecount(1) CMILBrush *pBrush,
    __in_ecount_opt(1) IMILEffectList *pIEffect,
    UINT uTargetWidth,
    UINT uTargetHeight
    )
{
    HRESULT hr = S_OK;
    CMilRectF rcBounds;

    //
    // Compute bounding box
    //
    IFC(pShape->GetTightBounds(OUT rcBounds,
                               NULL,
                               CMILMatrix::ReinterpretBase(pmatShapeToDevice)
                               ));

    // 
    // Early out if the tight bounds have zero size
    // 
    if (rcBounds.IsEmpty())
    {
        goto Cleanup;
    }
    
    //
    // For 2D rendering, local rendering and world sampling spaces are identical
    //

    const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &
        matBaseSamplingToDevice =
        ReinterpretLocalRenderingAsBaseSampling(pContextState->WorldToDevice);

    //
    // Check sw fallback mode/stats
    //

    if (g_pMediaControl)
    {
        if (g_pMediaControl->GetDataPtr()->PrimitiveSoftwareFallbackDisabled)
        {
            goto Cleanup;
        }
    }

    //
    // Draw the primitive 
    //

    // Allow one more pixel for possible pixel snapping offset.
    // We should do it because rcBounds here is not yet affected by pixel
    // snapping offset. 
    rcBounds.Inflate(1, 1);

    // convert floats to ints
    IFC(InflateRectFToPointAndSizeL(rcBounds, OUT m_rcShapeBounds));
    
    for (BOOL fHaveTile = ComputeFirstTile(uTargetWidth, uTargetHeight);
              fHaveTile;
              fHaveTile = ComputeNextTile())
    {
        //
        // We need the destruction of the lock before DrawCurrentTile,
        // therefore we need these brakets here
        //
        {
            CD3DLockableTexturePairLock lock(m_pCurrentTile);
            CRectClipper oClipper;
            oClipper.SetClip(CMILSurfaceRect(m_rcCurrentTile));
            
            // Lock and CLEAR the current tile
            IFC(lock.Lock(m_rcCurrentTile.Width, m_rcCurrentTile.Height, m_lockData));

            //
            // Call the software rasterizer
            //

            IFC(m_sr.FillPath(
                this,
                &oClipper,
                pContextState, 
                pShape, 
                pmatShapeToDevice, 
                pBrush,
                matBaseSamplingToDevice,
                pIEffect
                ));
        }
        
        //
        // Draw the current tile
        //
        IFC(DrawCurrentTile());
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSoftwareFallback::DrawGlyphs
//
//  Synopsis:  Draw the glyph run using the software rastizer with our 
//             temporary surfaces.  Note that this is done by using
//             a system memory texture and copying to the surface
//             at some later point in time.  
//
//-------------------------------------------------------------------------
HRESULT 
CHwSoftwareFallback::DrawGlyphs(
    __in_ecount(1) DrawGlyphsParameters &pars,
    bool fTargetSupportsClearType,
    __in_ecount(1) CMILBrush *pBrush,
    FLOAT flEffectAlpha,
    __in_ecount(1) CGlyphPainterMemory *pGlyphPainterMemory,
    UINT uTargetWidth,
    UINT uTargetHeight
    )
{
    HRESULT hr = S_OK;
    
    Assert(pars.pContextState);
    Assert(pars.pGlyphRun);
    Assert(pBrush);
    Assert(pGlyphPainterMemory);

    //
    // Check sw fallback mode/stats
    //

    if (g_pMediaControl)
    {
        if (g_pMediaControl->GetDataPtr()->PrimitiveSoftwareFallbackDisabled)
        {
            goto Cleanup;
        }
    }

    // calculate glyph run bounding rectangle
    {
        CRectF<CoordinateSpace::LocalRendering> rc;
        CRectF<CoordinateSpace::Device> rcTrueBounds;
        pars.pGlyphRun->GetBounds(&rc, &pars.pContextState->WorldToDevice);
        pars.pContextState->WorldToDevice.Transform2DBounds(
            rc,
            rcTrueBounds
            );

        // Allow one more pixel for possible pixel snapping offset.
        // We should do it because rcTrueBounds here is not yet affected by pixel
        // snapping offset. 
        rcTrueBounds.Inflate(1, 1);

        // convert floats to ints
        IFC(InflateRectFToPointAndSizeL(rcTrueBounds, OUT m_rcShapeBounds));
    }

    // initialize alpha textures it not yet
    if (fTargetSupportsClearType && !m_fAlphaTexturesInited)
    {
        IFC( InitAlphaTextures() );
    }

    for (BOOL fHaveTile = ComputeFirstTile(uTargetWidth, uTargetHeight);
              fHaveTile;
              fHaveTile = ComputeNextTile())
    {
        bool fClearTypeUsedToRender = false;
        //
        // We need the destruction of the lock before DrawCurrentTile,
        // therefore we need these brakets here
        //
        {
            CD3DLockableTexturePairLock lock(m_pCurrentTile);
            CRectClipper oClipper;
            oClipper.SetClip(CMILSurfaceRect(m_rcCurrentTile));

            // Lock and CLEAR the current tile
            //
            // Note that we may initialize the aux channels if fTargetSupportsClearType, but will not necessarily
            // use them if the out argument fClearTypeUsedToRender is false.
            //
            IFC(lock.Lock(m_rcCurrentTile.Width, m_rcCurrentTile.Height, m_lockData, fTargetSupportsClearType));

            //
            // Call the software rasterizer
            //

            IFC(m_sr.DrawGlyphRun(
                this,
                &oClipper,
                pars,
                pBrush,
                flEffectAlpha,
                pGlyphPainterMemory,
                fTargetSupportsClearType,
                &fClearTypeUsedToRender
                ));
        }
        //
        // Draw the current tile
        //

        IFC(DrawCurrentTile(fClearTypeUsedToRender));
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSoftwareFallback::DrawCurrentTile
//
//  Synopsis:  Draws the current tile
//
//-------------------------------------------------------------------------

HRESULT
CHwSoftwareFallback::DrawCurrentTile(bool fUseAux)
{
    HRESULT hr = S_OK;

    //
    // Draw the current tile
    //

    IFC( m_pCurrentTile->Draw(m_pD3DDevice, m_rcCurrentTile, fUseAux) );

    //
    // Use another tile next time
    // 

    if (++m_pCurrentTile == &m_tiles[NUM_FALLBACK_TILES])
    {
        m_pCurrentTile = m_tiles;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSoftwareFallback::AddDirtyRect, CSpanSink
//
//  Synopsis:  CHwSoftwareFallback does not care about dirty rects.
//
//-------------------------------------------------------------------------
void CHwSoftwareFallback::AddDirtyRect(
    __in_ecount(1) const MilPointAndSizeL *prcDirty
    )
{
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSoftwareFallback::OutputSpan, CSpanSink (COutputSpan)
//
//  Synopsis:  Output the span.
//
//             Note that we are writing to a tile, but all coordinates
//             here are relative to to the target surface.  GetNextScanline
//             is responsible for adjusting coordinates to hit the current 
//             tile.
//
//-------------------------------------------------------------------------
void 
CHwSoftwareFallback::OutputSpan(
    INT y, 
    INT xMin, 
    INT xMax
    )
{
    Assert(xMin < xMax);

    Assert(xMin >= m_rcCurrentTile.X);
    Assert(xMax <= m_rcCurrentTile.X + m_rcCurrentTile.Width);
    Assert(y >= m_rcCurrentTile.Y);
    Assert(y < m_rcCurrentTile.Y + m_rcCurrentTile.Height);

    UINT uWidth = xMax - xMin;

    UINT uOffset = CalculateScanlineOffset(xMin, y, uWidth);
    VOID *pvDest = m_lockData.pMainBits + uOffset;
    const VOID *pvAux = NULL;
    if (m_lockData.pAuxBits)
    {
        pvAux = m_lockData.pAuxBits + uOffset;
    }

    m_ScanPipeline.Run(
        pvDest,
        pvAux,
        uWidth,
        xMin,
        y
        DBG_ANALYSIS_COMMA_PARAM(uWidth * sizeof(ARGB))
        DBG_ANALYSIS_COMMA_PARAM(uWidth * sizeof(ARGB))
      );

    if (   g_pMediaControl != NULL
        && g_pMediaControl->GetDataPtr()->RecolorSoftwareRendering
        )
    {
        g_pMediaControl->TintARGBBitmap(
            static_cast<ARGB *>(pvDest),
            uWidth,
            1,
            m_lockData.uPitch
            );
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:  CHwSoftwareFallback::SetAntialiasedFiller, CSpanSink
//

VOID CHwSoftwareFallback::SetAntialiasedFiller(
    __in_ecount(1) CAntialiasedFiller *pFiller
    )
{
    m_ScanPipeline.SetAntialiasedFiller(pFiller);
}

//+-----------------------------------------------------------------------------
//
//  Function:  CHwSoftwareFallback::SetupPipeline, CSpanSink
//

HRESULT CHwSoftwareFallback::SetupPipeline(
    MilPixelFormat::Enum fmtColorData,
    __in_ecount(1) CColorSource *pColorSource,
    BOOL fPPAA,
    bool fComplementAlpha,
    MilCompositingMode::Enum eCompositingMode,
    __in_ecount(1) CSpanClipper *pSpanClipper,
    __in_ecount_opt(1) IMILEffectList *pIEffectList,
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Effect,CoordinateSpace::Device> *pmatEffectToDevice,
    __in_ecount(1) const CContextState *pContextState
    )
{
    HRESULT hr = S_OK;
    
    CMILSurfaceRect rcClipBounds;

    pSpanClipper->GetClipBounds(&rcClipBounds);

    // We initialize the pipeline to do MilCompositingMode::SourceCopy always.
    // This is because the hardware must do the blend operation.

    IFC(m_ScanPipeline.InitializeForRendering(
        m_IntermediateBuffers,
        MilPixelFormat::PBGRA32bpp,      // Tile texture format
        pColorSource,
        fPPAA,
        fComplementAlpha,
        MilCompositingMode::SourceCopy,  // See above
        rcClipBounds.Width(),
        pIEffectList,
        pmatEffectToDevice,
        pContextState));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:  CHwSoftwareFallback::SetupPipelineForText
//
//  Synopsis:
//      Recall m_ScanPipeline to prepare for text rendering
//
//------------------------------------------------------------------------------
HRESULT CHwSoftwareFallback::SetupPipelineForText(
    __in_ecount(1) CColorSource *pColorSource,
    MilCompositingMode::Enum eCompositingMode,
    __in_ecount(1) CSWGlyphRunPainter &painter,
    bool fNeedsAA
    )
{
    HRESULT hr = S_OK;
    
    // We initialize the pipeline to do MilCompositingMode::SourceCopy always.
    // This is because the hardware must do the blend operation.

    IFC( m_ScanPipeline.InitializeForTextRendering(
         m_IntermediateBuffers,
         MilPixelFormat::PBGRA32bpp,      // Tile texture format
         pColorSource,
         MilCompositingMode::SourceCopy,  // See above
         painter,
         fNeedsAA
        ) );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:  CHwSoftwareFallback::ReleaseExpensiveResources, CSpanSink
//

VOID CHwSoftwareFallback::ReleaseExpensiveResources()
{
    m_ScanPipeline.ReleaseExpensiveResources();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSoftwareFallback::CalculateScanlineOffset
//
//  Synopsis:
//      Returns an offset from beginning of locked texture area
//      to the scanline defined by given coordinates.
//
//------------------------------------------------------------------------------

UINT
CHwSoftwareFallback::CalculateScanlineOffset(
    INT x,
    INT y,
    UINT nWidth
    ) const
{
    // Calculate the destination offset for the scan.  Note that this is the point
    // we translate the data to fit in the tile coordinate system.  All other
    // code assumes the surface coordinate system.

    Assert(x >= m_rcCurrentTile.X);
    Assert(x + nWidth <= static_cast<UINT>(m_rcCurrentTile.X + m_rcCurrentTile.Width));
    Assert(y >= m_rcCurrentTile.Y);
    Assert(y < m_rcCurrentTile.Y + m_rcCurrentTile.Height);

    UINT xTileSpace = x - m_rcCurrentTile.X;
    UINT yTileSpace = y - m_rcCurrentTile.Y;

#if DBG_ANALYSIS
    Assert(xTileSpace < m_lockData.m_uDbgAnalysisLockedWidth);
    Assert(yTileSpace < m_lockData.m_uDbgAnalysisLockedHeight);
#endif

    UINT offset = yTileSpace * m_lockData.uPitch + xTileSpace * sizeof(ARGB);
    return offset;
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSoftwareFallback::ComputeNextTile
//
//  Synopsis:  Compute the next tile.  We iterate tiles first by row
//             then by column
//
//-------------------------------------------------------------------------
BOOL 
CHwSoftwareFallback::ComputeNextTile()
{
    int nCurrentTileRight = m_rcCurrentTile.X + m_rcCurrentTile.Width;
    int nShapeRight = m_rcShapeBounds.X + m_rcShapeBounds.Width;

    //
    // Advance in the X direction if we need to
    //
    
    if (nCurrentTileRight < nShapeRight)
    {
        m_rcCurrentTile.X = nCurrentTileRight;
        m_rcCurrentTile.Width = min(m_uMaxTileWidth, (UINT)nShapeRight-nCurrentTileRight);

        return TRUE;
    }
    
    //
    // Advance in the Y direction if we need to
    //

    int nCurrentTileBottom = m_rcCurrentTile.Y + m_rcCurrentTile.Height;
    int nShapeBottom = m_rcShapeBounds.Y + m_rcShapeBounds.Height;

    if (nCurrentTileBottom < nShapeBottom)
    {
        m_rcCurrentTile.X = m_rcShapeBounds.X;
        m_rcCurrentTile.Width = min(m_uMaxTileWidth, (UINT)m_rcShapeBounds.Width);
        m_rcCurrentTile.Y = nCurrentTileBottom;
        m_rcCurrentTile.Height = min(m_uMaxTileHeight, (UINT)nShapeBottom-nCurrentTileBottom);
        return TRUE;
    }

    return FALSE; // no more tiles, we're done
}





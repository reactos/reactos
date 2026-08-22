// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwSoftwareFallback which provides software fallback for the hw
//      render target.
//

class CD3DDeviceLevel1;
class CHwSurfaceRenderTarget;

MtExtern(CHwSoftwareFallback);

#define NUM_FALLBACK_TILES 2

//------------------------------------------------------------------------------
//
//  Class: CHwSoftwareFallback
//
//  Description:
//      Provides software fallback for the hw render target.  Note that this
//      is implemented by creating a small set of tiles in system memory that
//      we will render to using the software rasterizer and then draw using
//      the d3d device.  We need tiling to save memory and deal with
//      texture limits.
//
//------------------------------------------------------------------------------

class CHwSoftwareFallback
    : public CSpanSink
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwSoftwareFallback));

    //
    // CHwSoftwareFallback specific methods
    //

    CHwSoftwareFallback();
    virtual ~CHwSoftwareFallback()
    {
        // nothing
    };

    HRESULT Init(
        __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice
        );

    //
    // IRenderTargetInternal methods that need to be exposed
    //

    // Draw a surface.

    HRESULT DrawBitmap(
        __in_ecount(1) const CContextState *pContextState,
        __inout_ecount(1) IWGXBitmapSource *pIBitmap,
        __in_ecount_opt(1) IMILEffectList *pIEffect,
        UINT uTargetWidth,
        UINT uTargetHeight
        );

    // Draw a path.

    HRESULT FillPath(
        __in_ecount(1) const CContextState *pContextState,
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice,
        __in_ecount(1) const IShapeData *pShape,
        __in_ecount(1) CMILBrush *pBrush,
        __in_ecount_opt(1) IMILEffectList *pIEffect,
        UINT uTargetWidth,
        UINT uTargetHeight
        );

    // Draw a glyph run

    HRESULT DrawGlyphs(
        __in_ecount(1) DrawGlyphsParameters &pars,
        bool fTargetSupportsClearType,
        __in_ecount(1) CMILBrush *pBrush,
        FLOAT flEffectAlpha,
        __in_ecount(1) CGlyphPainterMemory *pGlyphPainterMemory,
        UINT uTargetWidth,
        UINT uTargetHeight
        );

    // CSpanSink interface

    override void OutputSpan(INT y, INT xMin, INT xMax);
    override void AddDirtyRect(__in_ecount(1) const MilPointAndSizeL *prcDirty);

    override HRESULT SetupPipeline(
        MilPixelFormat::Enum fmtColorData,
        __in_ecount(1) CColorSource *pColorSource,
        BOOL fPPAA,
        bool fComplementAlpha,        
        MilCompositingMode::Enum eCompositingMode,
        __in_ecount(1) CSpanClipper *pSpanClipper,
        __in_ecount_opt(1) IMILEffectList *pIEffectList,
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::Effect,CoordinateSpace::Device> *pmatEffectToDevice,
        __in_ecount(1) const CContextState *pContextState
        );

    override HRESULT SetupPipelineForText(
        __in_ecount(1) CColorSource *pColorSource,
        MilCompositingMode::Enum eCompositingMode,
        __in_ecount(1) CSWGlyphRunPainter &painter,
        bool fNeedsAA
        );

    override VOID ReleaseExpensiveResources();

    override VOID SetAntialiasedFiller(
        __in_ecount(1) CAntialiasedFiller *pFiller
        );

private:
    HRESULT InitAlphaTextures();
    UINT CalculateScanlineOffset(INT x, INT y, UINT nWidth) const;

    BOOL ComputeFirstTile(UINT uTargetWidth, UINT uTargetHeight );
    BOOL ComputeNextTile();

    HRESULT DrawCurrentTile(bool fUseAux = false);

private:
    //
    // D3D objects
    //

    CD3DDeviceLevel1 *m_pD3DDevice;

    //
    // Tile data
    //

    UINT             m_uMaxTileWidth;
    UINT             m_uMaxTileHeight;
    MilPointAndSizeL          m_rcShapeBounds;
    MilPointAndSizeL          m_rcCurrentTile;

    CD3DLockableTexturePair  m_tiles[NUM_FALLBACK_TILES];
    CD3DLockableTexturePair *m_pCurrentTile; // points to one of m_tiles

    CD3DLockableTexturePairLock::LockData m_lockData;

    //
    // Software rasterizer buffers
    //
    CSPIntermediateBuffers m_IntermediateBuffers;

    //
    // Data used for the scan pipeline
    //
    CScanPipelineRendering m_ScanPipeline;

    // Software rasterizer class itself

    CSoftwareRasterizer m_sr;

    // Flags for clear type text rendering
    BOOL m_fAlphaTexturesInited;
};





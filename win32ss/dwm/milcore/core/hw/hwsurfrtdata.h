// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      CHwSurfaceRenderTargetSharedData implementation.
//      Contains costly data that we want to share between hw surface render targets.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

class CHwBlurShader;
class CHwSingleSourceMultiPassBlurTechnique;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwSurfaceRenderTargetSharedData
//
//  Synopsis:
//      Stores costly data that we want to share for multiple render targets.
//      Currently, the CD3DDeviceLevel1 object holds on to this data.
//
//------------------------------------------------------------------------------

class CHwSurfaceRenderTargetSharedData
    // CMILResourceIndex should be the first base class so it may be destroyed
    // last, and definitely after any resources are released.
    : public CMILResourceIndex
{
protected:

    CHwSurfaceRenderTargetSharedData();
    ~CHwSurfaceRenderTargetSharedData();

    // Our shared data currently has device affinity
    HRESULT InitSharedData(
        __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice
        );

    HRESULT InitColorComponentSources();

public:

    // Index resources are to use for caching
    HRESULT GetCacheIndex(
        __out_ecount(1) IMILResourceCache::ValidIndex *puIndex
        ) const
    {
        if (m_uCacheIndex != CMILResourceCache::InvalidToken)
        {
            *puIndex = m_uCacheIndex;
            return S_OK;
        }

        return E_FAIL;
    }

    ULONG GetRealizationCacheIndex() const
    {
        return m_uCacheIndex;
    }

    // Fallback Software rasterizer
    HRESULT GetSoftwareFallback(
        __deref_out_ecount(1) CHwSoftwareFallback ** const ppISoftwareFallback,
        HRESULT hrReasonForFallback
        );

    void ResetPerPrimitiveResourceUsage();

    // Extract a cached HW brush or get a new one
    // Note: only one reference to a HW brush is allowed at one time
    HRESULT DeriveHWBrush(
        __in_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) CHwBrushContext const &hwBrushContext,
        __deref_out_ecount(1) CHwBrush **ppHwBrush
        );

    // Extract a cached HW textured color source or get a new one
    HRESULT DeriveHWTexturedColorSource(
        __in_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) CHwBrushContext const &hwBrushContext,
        __deref_out_ecount(1) CHwTexturedColorSource **ppHwTexturedColorSource
        );

    HRESULT GetSolidColorTexture(
        __in_ecount(1) MilColorF const &color,
        __deref_out_ecount(1) CHwSolidColorTextureSource ** const ppTexture
        )
    {
        return m_solidColorTextureSourcePool.RetrieveTexture(
            color,
            ppTexture
            );
    }

    void GetColorComponentSource(
        CHwColorComponentSource::VertexComponent eComponent,
        __deref_out_ecount(1) CHwColorComponentSource ** const ppColorComponentSource
        );

    HRESULT DeriveHWShader(
        __in_ecount(1) CMILShader *pShader,
        __in_ecount(1) CHwBrushContext const &hwBrushContext,
        __deref_out_ecount(1) CHwShader ** const ppHwShader
        );

    HRESULT DerivePipelineShader(
        __in_ecount(uNumPipelineItems) HwPipelineItem const * rgShaderPipelineItems,
        UINT uNumPipelineItems,
        __deref_out_ecount(1) CHwPipelineShader ** const ppHwShader
        );

    HRESULT GetHwShaderCache(
        __deref_out_ecount(1) CHwShaderCache ** const ppCache
        );

    HRESULT GetHwDestinationTexture(
        __in_ecount(1) CHwSurfaceRenderTarget *pHwSurfaceRenderTarget,
        __in_ecount(1) CMILSurfaceRect const &rcDestRect,
        __in_ecount_opt(crgSubDestCopyRects) CMILSurfaceRect const *prgSubDestCopyRects,
        UINT crgSubDestCopyRects,
        __deref_out_ecount(1) CHwDestinationTexture ** const ppHwDestinationTexture
        );

    HRESULT GetScratchHwBoxColorSource(
        __in_ecount(1) MILMatrix3x2 const *pMatXSpaceToSourceClip,
        __deref_out_ecount(1) CHwBoxColorSource ** const ppTextureSource
        );

    // Scratch storage for temp shapes while rendering
    __out_ecount(1)
    CShape *GetScratchFillShape()
    {
        return &m_shapeScratchFill;
    }

    // Scratch storage for temp shapes while rendering
    __out_ecount(1)
    CShape *GetScratchSnapShape()
    {
        return &m_shapeScratchSnap;
    }

    // Scratch storage for temp shapes while rendering
    __out_ecount(1)
    CShape *GetScratchWidenShape()
    {
        return &m_shapeScratchWiden;
    }

    HRESULT CHwSurfaceRenderTargetSharedData::GetScratchDrawBitmapBrushNoAddRef(
        __deref_out_ecount(1) CMILBrushBitmap ** const ppDrawBitmapScratchBrushNoAddRef
        );

    // Scratch points used during trapezoidal
    __out_ecount(1) DynArray<MilPoint2F> *GetScratchPoints()
    {
        return &m_rgPoints;
    }

    // Scratch types used during trapezoidal
    __out_ecount(1) DynArray<BYTE> *GetScratchTypes()
    {
        return &m_rgTypes;
    }

private:

    HRESULT GetCachedBrush(
        __in_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) CHwBrushContext const &hwBrushContext,
        __deref_out_ecount_opt(1) CHwBrush ** const ppHwCachedBrush
        );

private:
    // Brush Pool
    CHwBrushPool m_poolHwBrushes;
    CHwDestinationTexturePool *m_pHwDestinationTexturePoolBGR;
    CHwDestinationTexturePool *m_pHwDestinationTexturePoolPBGRA;

    CHwShaderCache *m_pHwShaderCache;

    DynArray<CHwColorComponentSource *> m_dynpColorComponentSources;

    // Scratch storage for temp shapes while rendering
    CShape m_shapeScratchFill;
    CShape m_shapeScratchSnap;
    CShape m_shapeScratchWiden;
    CMILBrushBitmap *m_pDrawBitmapScratchBrush;

    //
    // Points and types arrays, the hw rasterizer needs this input
    // which is provided by the geometry library.  Note that they
    // are cached here so that we keep our points buffer memory between
    // path rasterization.
    //

    DynArray<MilPoint2F> m_rgPoints;
    DynArray<BYTE>      m_rgTypes;

    // Fallback Software rasterizer
    CHwSoftwareFallback *m_pswFallback;

    CHwSolidColorTextureSourcePool m_solidColorTextureSourcePool;
    CHwBoxColorSource *m_pScratchHwBoxColorSource;

    // The d3d device that the data is associated with
    CD3DDeviceLevel1 *m_pD3DDevice;
};




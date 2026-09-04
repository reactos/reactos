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

MtExtern(CGlyphRunResource);
MtExtern(CGlyphRunRealization);

class CSWGlyphRun;  // cached data for software rendering
class CD3DGlyphRun; // cached data for D3D9 rendering
class CGlyphRunRealization;

//#define TEXT_DEBUG

#ifdef TEXT_DEBUG
#define TraceTagText(x)                         \
    do                                      \
    {                                       \
        if (DbgExTaggedTrace x) {                \
            AvalonDebugBreak();             \
        }                                   \
    } while (UNCONDITIONAL_EXPR(false))
#else
#define TraceTagText(x)
#endif

//#define TEXT_DEBUG2



class CGlyphRunResource
    : public CMilSlaveResource
    , public CGlyphRunStorage
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CGlyphRunResource));

    //
    // Construction/Destruction
    //

    CGlyphRunResource(__in_ecount(1) CComposition* pComposition);
    virtual ~CGlyphRunResource();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_GLYPHRUN;
    }

    void Disable();

    void GetBounds(
        __out_ecount(1) CRectF<CoordinateSpace::LocalRendering> * prcBounds,
        __in_ecount(1) CBaseMatrix const * pLocalToDevice
        ) const;

    bool GetAvailableScale(
        __inout_ecount(1) float* pScaleX,
        __inout_ecount(1) float* pScaleY,
        __in_ecount(1) const DisplaySettings *pDisplaySettings,
        MilTextRenderingMode::Enum textRenderingMode,
        MilTextHintingMode::Enum textHintingMode,
        __out RenderingMode *pRecommendedRenderingMode,
        __out_ecount(1) CGlyphRunRealization** ppRealization,
        IDpiProvider const* pDpiProvider
        );

    bool ShouldUseGeometry(
        __in_ecount(1) CMultiOutSpaceMatrix<CoordinateSpace::LocalRendering> const * pWorldToDevice,
        __in_ecount(1) DisplaySettings const * pDisplaySettings
        );

    void EnsureGeometry();

    CMilGeometryDuce *GetGeometryRes()
    {
        return m_pGeometry;            
    }

    float BlueSubpixelOffset()
    {
        return m_pGlyphBlendingParameters->BlueSubpixelOffset;
    }

    UINT GetGammaIndex()
    {        
        return m_pGlyphBlendingParameters->GammaIndex;
    }

    //
    // This returns true if our monitor is RGB, and we're rendering with 100% ClearType
    // level. This allows some optimizations in the software rasterization path.
    //
    bool IsRGBFullCleartype(__in_ecount(1) const DisplaySettings *pDisplaySettings)
    {        
        return (   (pDisplaySettings->PixelStructure == DWRITE_PIXEL_GEOMETRY_RGB)
            && (pDisplaySettings->DisplayRenderingMode == ClearType)
            && (IsCloseReal(m_pGlyphBlendingParameters->BlueSubpixelOffset, 1.0f/3.0f)));
    }
    
    HRESULT GetGammaTable(__in_ecount(1) const DisplaySettings *pDisplaySettings, 
                          __deref_out_ecount(1) GammaTable const* * ppGammaTable);
    
    HRESULT GetEnhancedContrastTable(float k, 
                                     __deref_out_ecount(1) EnhancedContrastTable **ppTable);

    // ------------------------------------------------------------------------
    //
    //   Command handlers
    //
    // ------------------------------------------------------------------------

    HRESULT ProcessCreate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_GLYPHRUN_CREATE* pCmd,
        __in_bcount_opt(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    //
    // Reset flag so we know we're not going to get called back again,
    // and need to re-register for a callback if we want another one.
    //
    void AnimationTimeoutCallback()
    {
        NotifyOnChanged(this);
    }

    static const int c_frameCountBeforeRealizationCallback;
    static const int c_frameCountBeforeAnimationRealizationStale;
    static const int c_frameCountBeforeDeleteHighQualityRealization;

private:    
    HRESULT CreateRealization(
        float scaleX,
        float scaleY,
        bool fAnimationQuality,
        bool fBiLevelRequested,
        __in const DisplaySettings *pDisplaySettings,
        MilTextRenderingMode::Enum textRenderingMode,
        __out CGlyphRunRealization **ppRealization
        );

    static void DeleteRealizationInArray(__in DynArrayIA <CGlyphRunRealization*, 2> *pArray);

    void PurgeOldEntries(__in DynArrayIA <CGlyphRunRealization*, 2> *pRealizationArray);

    void GetDWriteRenderingMode(
        __in IDWriteFontFace *pIDWriteFontFace,
        MilTextRenderingMode::Enum textRenderingMode,
        bool fAnimationQuality,
        float scaleFactor,
        __in const DisplaySettings *pDisplaySettings,
        __out DWRITE_RENDERING_MODE *pDWriteRenderingMode
        );

    void GetBlendMode(
        MilTextRenderingMode::Enum textRenderingMode,
        RenderingMode displayRenderingMode,
        __out RenderingMode *pRecommendedBlendMode
        );

    void FindMatchingRealization(
        __in const DynArrayIA <CGlyphRunRealization*, 2> *pRealizationArray,
        bool fUseLastFulfilledScale,
        float desiredScaleX,
        float desiredScaleY,
        __out double *pMatchQuality,
        __out bool *pFoundExactMatch,
        __out bool *pFoundMatch,
        __out UINT *pFoundIndex
        );

    static double InspectScaleQuality(
        __in double scaleX1,
        __in double scaleX2,
        __in double scaleY1,
        __in double scaleY2
        );

    static float SnapToScaleGrid(__in double x);

    // ------------------------------------------------------------------------
    //
    //  CDWriteFontFaceCache
    //
    //  Caches a small quantity of expensive IDWriteFontFace instances.
    //  The cache prevents us from exhausting available address space as each
    //  IDWriteFontFace typically maps in its corresponding file.
    //
    // ------------------------------------------------------------------------

public:
    static void ResetFontFaceCache() { CDWriteFontFaceCache::Reset(); }

private:
    class CDWriteFontFaceCache
    {
    public:
        
        static HRESULT GetFontFace(
            __in IDWriteFont *pFont,
            __out IDWriteFontFace **ppFontFace
            );

        static void Reset();

    private:

        static HRESULT AddFontFaceToCache(
            __in IDWriteFont *pFont,
            __out IDWriteFontFace **ppFontFace
            );

        struct FontFaceCacheEntry
        {
            IDWriteFont *pFont;
            IDWriteFontFace *pFontFace;
        };

        // A good cache size based upon measurements of the TextFormatter micro benchmarks
        // is 4. None of the tests allocate more than 3 IDWriteFontFaces on the render thread.
        // However, dwrite circa win7 has an issue aggressively consuming address space and
        // therefore we need to be conservative holding on to font references.
        static const UINT32 c_cacheSize = 1;

        // Cached IDwriteFontFace instances.
        static FontFaceCacheEntry m_cache[c_cacheSize];

        // Most recently used element in the FontFace cache.
        static UINT32 m_cacheMRU;

        // Guards access to m_cache.
        static LONG m_mutex;
    };

private:
    CMilSlaveGlyphCache *m_pGlyphCache;

    GlyphBlendingParameters *m_pGlyphBlendingParameters;

    //
    // The list of scale pairs that are known to be available
    // in glyph cache for this CGlyphRunResource.
    // Note that availability of a realization doesn't necessarily
    // imply that all the bitmaps are present - some may have been
    // cleaned up due to caching limits and will be re-requested
    // from the text rasterizer later.
    //
    DynArrayIA <CGlyphRunRealization*, 2> m_prgHighQualityRealizationArray;
    DynArrayIA <CGlyphRunRealization*, 2> m_prgAnimationQualityRealizationArray;
    DynArrayIA <CGlyphRunRealization*, 2> m_prgBiLevelRealizationArray;
    
    CMilGeometryDuce *m_pGeometry;

    static const double c_minAnimationDetectionBar;

    // ScaleGrid: allowed rasterization scales for scale animation.
    // Numbers are chosen heuristically.
    // Previous heuristic used powers of two (4, 8, 16, 32, 64).
    // It turned out that the row above does not provide desired quality.
    // Transition between neighbouring values on small scales are visible
    // as sudden "blur blast". To suppress it, we need to decrease grid
    // step. However we don't want to increase the burden of extra
    // rasterization on high end. The row below was obtained by following
    // formulas:
    // ScaleGrid[0] = 5;
    // ScaleGrid[i+1] = ScaleGrid[i] * (1.3 + 0.1*i);
    static const int c_scaleGridSize;
    static const float scaleGrid[];
};

//+-----------------------------------------------------------------------------
//
//  class:
//      CGlyphRunRealization
//
//  Synopsis:
//      Describes one of realizations available for glyph run.
//
//------------------------------------------------------------------------------

class CGlyphRunRealization :
    public LIST_ENTRY,
    public CMILCOMBase
{
public:

    DECLARE_COM_BASE

    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppvObject)
    {
        return E_NOTIMPL;
    }

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CGlyphRunRealization));

    CGlyphRunRealization(__in float scaleX,
                         __in float scaleY,
                         __in bool fAnimationQuality,
                         __in CMilSlaveGlyphCache *pGlyphCacheSlave
                         );
    virtual ~CGlyphRunRealization();

    void SetAnalysis(__in IDWriteGlyphRunAnalysis *pIDWriteGlyphRunAnalysis);

    float GetScaleX() const { return m_scaleX; }
    float GetScaleY() const { return m_scaleY; }
    CRectF<CoordinateSpace::LocalRendering> const& GetBoundingRect() const { return m_boundingRect; }

    // device dependent data accessors
    CSWGlyphRun* GetSWGlyphRun() const {return m_pSWGlyphRun;}
    void SetSWGlyphRun(CSWGlyphRun *pRun);

    HRESULT GetD3DGlyphRun(UINT index, CD3DGlyphRun**);
    void SetD3DGlyphRun(UINT index, CD3DGlyphRun *pRun);

    UTC_TIME CreatedFrame() const
    {
        return m_createdFrame;
    }

    UTC_TIME LastUsedFrame() const
    {
        return m_lastUsedFrame;
    }
    
    bool IsAnimationQuality() const
    {
        return m_fIsAnimationQuality;
    }

    float LastFulfilledScaleX() const
    {
        Assert(m_fIsAnimationQuality);
        return m_lastFulfilledScaleX;
    }

    float LastFulfilledScaleY() const
    {
        Assert(m_fIsAnimationQuality);
        return m_lastFulfilledScaleY;
    }

    void SetLastFulfilledScale(float x, float y)
    {
        Assert(m_fIsAnimationQuality);
        m_lastFulfilledScaleX = x;
        m_lastFulfilledScaleY = y;
    }

    void UpdateLastUsedFrame()
    {
        m_lastUsedFrame = m_pGlyphCacheNoRef->GetCurrentRealizationFrame();
        //
        // If we have a valid alpha map we are in the glyph cache realization list
        // and need to move ourselves to the head of the list to keep the list ordered
        // by last used frame. Easiest way to achieve this is to remove and readd ourselves.
        //
        if (m_fHasAlphaMaps)
        {
            m_pGlyphCacheNoRef->RemoveRealization(this, GetTextureSize());
            m_pGlyphCacheNoRef->AddRealization(this, GetTextureSize());
        }
    }

    HRESULT EnsureValidAlphaMap(__in const EnhancedContrastTable *pECT);
    bool HasAlphaMaps()
    {
        return m_fHasAlphaMaps;
    }

    void DeleteAlphaMap();

    void GetAlphaMap(
        __deref_out_ecount(*pAlphaMapSize) BYTE **ppAlphaMap, 
        __out UINT32 *pAlphaMapSize, 
        __out RECT *pBoundingBox
        )
    {        
        Assert(m_fHasAlphaMaps);
        *ppAlphaMap = m_pAlphaMap;
        *pBoundingBox = m_alphaMapBoundingBox;
        *pAlphaMapSize = m_textureSize;
    }

    UINT32 GetTextureSize() const
    {
        return m_textureSize;
    }

    bool IsBiLevelOnly() const
    {
        return m_fIsBiLevelOnly;
    }

    IDWriteGlyphRunAnalysis *GetAnalysisNoRef()
    {
        return m_pIDWriteGlyphRunAnalysis;
    }

private:
     HRESULT RealizeAlphaBoundsAndTextures(  
        DWRITE_TEXTURE_TYPE textureType, 
        __in_opt const EnhancedContrastTable *pECT,
        __out UINT32 *pTextureSize,
        __out RECT *pBoundingBox,
        __deref_out_ecount_opt(*pTextureSize) BYTE **pAlphaMap
        );

    float m_scaleX, m_scaleY;
    IDWriteGlyphRunAnalysis *m_pIDWriteGlyphRunAnalysis;

    //
    // If this glyph run has m_fIsAnimationQuality set, these
    // values represent the scales for which this realization 
    // was last used.
    //
    float m_lastFulfilledScaleX, m_lastFulfilledScaleY;

    bool m_fIsAnimationQuality;

    UTC_TIME m_createdFrame;
    UTC_TIME m_lastUsedFrame;

    // Bounding rectangle calculated for this realization.
    CRectF<CoordinateSpace::LocalRendering> m_boundingRect;

    //
    // Alpha map, size and bounding box. These are combined
    // for bi-level and CT glyphs. A single glyph run
    // can contain both simultaneously, though most will only 
    // contain one, usually CT. If there are only bilevel glyphs
    // in the run, m_fIsBiLevelOnly will be true. If there are only
    // ClearType glyphs, or a mix of ClearType and bilevel, it will
    // be false
    //
    BYTE *m_pAlphaMap;
    UINT32 m_textureSize;
    RECT m_alphaMapBoundingBox;

    bool m_fHasAlphaMaps;
    bool m_fIsBiLevelOnly;
    
    // device dependent data
    CSWGlyphRun* m_pSWGlyphRun;
    DynArrayIA<CD3DGlyphRun*, 2> m_pD3DGlyphRuns;

    CMilSlaveGlyphCache *m_pGlyphCacheNoRef;

};



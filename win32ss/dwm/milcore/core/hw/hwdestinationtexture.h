// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_effects
//      $Keywords:
//
//  $Description:
//      Contains the CHwDestinationTexture class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CHwDestinationTexture);
MtExtern(D3DResource_DestinationTexture);

class CHwDestinationTexturePool;

class CHwDestinationTexture :
    public CMILPoolResource,
    public CHwTexturedColorSource
{    
private:

    __checkReturn static HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) CHwDestinationTexturePool *pPoolManager,
        __deref_out_ecount(1) CHwDestinationTexture **ppHwDestinationTexture
        );

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwDestinationTexture));

    CHwDestinationTexture(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) CHwDestinationTexturePool *pPoolManager
        );

    ~CHwDestinationTexture();

public:

    void GetTextureSize(
        __out_ecount(1) UINT *puWidth,
        __out_ecount(1) UINT *puHeight
        ) const
    {
        m_pBackgroundTexture->GetTextureSize(
            puWidth,
            puHeight
            );
    }


    DEFINE_POOLRESOURCE_REF_COUNT_BASE

    //
    // CHwColorSource methods
    //

    override bool IsOpaque(
        ) const;

    override HRESULT Realize(
        );

    override HRESULT SendDeviceStates(
        DWORD dwStage,
        DWORD dwSampler
        );

public:

    //
    // Additional public methods
    //
    
    HRESULT Contains(
        __in_ecount(1) const CHwSurfaceRenderTarget *pHwTargetSurface,
        __in_ecount(1) const CMILSurfaceRect &rcDestRect,
        __out_ecount(1) bool *pDestinationTextureContainsNewTexture
        ) const;

    HRESULT SetContents(
        __inout_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface,
        __in_ecount(1) const CMILSurfaceRect &rcDestRect,
        __in_ecount_opt(crgSubDestCopyRects) const CMILSurfaceRect *prgSubDestCopyRects,
        UINT crgSubDestCopyRects
        );

    __out_ecount(1) CD3DVidMemOnlyTexture *GetTextureNoRef() const
        { return m_pBackgroundTexture; }

    HRESULT TransformDeviceSpaceBoundsToClippedDeviceSpaceBounds(
        __in_ecount(1) const CMILSurfaceRect &rcContentBoundsDeviceSpace,
        __out_ecount(1) CMILSurfaceRect *prcInflatedContentBoundsDeviceSpace
        ) const;

    void TransformDeviceSpaceBoundsToTextureSpaceBounds(
        __in_ecount(1) const CMILSurfaceRect &rcContentBoundsDeviceSpace,
        __out_ecount(1) CMILSurfaceRect *rcContentBoundsTextureSpace
        ) const;

    void TransformDeviceSpaceToTextureCoordinates(
        __in_ecount(1) const CMilRectL &rcBounds,
        __out_ecount(1) CMilRectF *prcTextureCoordinateBounds
        ) const;

private:


#if PERFMETER
    PERFMETERTAG GetPerfMeterTag() const
    {
        return Mt(D3DResource_DestinationTexture);
    }
#endif

    HRESULT UpdateSourceRect(
        __in_ecount(1) CMILSurfaceRect &rcSource,
        __in_ecount(1) CMILSurfaceRect &rcDest,
        __in_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface
        );

    static void CalculateSourceRect(
        UINT uRTWidth,
        UINT uRTHeight,
        __in_ecount(1) const CMILSurfaceRect &rcDestRect,
        __out_ecount(1) CMILSurfaceRect *pSourceRect
        );
        
#if DBG
    HRESULT DbgSetContentsInvalid(
        __inout_ecount(1) CD3DDeviceLevel1 *pDevice
        );
#endif

private:

    struct BackgroundTextureInfoType
    {
        dxlayer::vector2 textureSpaceMult;
        dxlayer::vector2 offsetDeviceSpace;
    };

    CHwSurfaceRenderTarget *m_pHwSurfaceRenderTarget;
    
    CD3DVidMemOnlyTexture *m_pBackgroundTexture;

    BackgroundTextureInfoType m_backgroundTextureInfo;

    MilPixelFormat::Enum m_fmtTexture;    // Precise pixel format inc. premul type
    UINT m_uTextureWidth;
    UINT m_uTextureHeight;

    bool m_fValidRealization;

    UINT m_uCopyWidthTextureSpace;
    UINT m_uCopyHeightTextureSpace;
    UINT m_uCopyOffsetXTextureSpace;
    UINT m_uCopyOffsetYTextureSpace;

    CMILSurfaceRect m_rcSource;

    LIST_ENTRY m_oUnusedPoolListEntry;

    friend class CHwDestinationTexturePool;
};




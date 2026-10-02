// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwTextureRenderTarget declaration
//

MtExtern(CHwTextureRenderTarget);

#if DBG
class CHwDisplayRenderTarget;
#endif DBG

class CDeviceBitmap;

class CHwTextureRenderTarget :
    public CMILCOMBase,
    public CHwSurfaceRenderTarget,
    public IMILRenderTargetBitmap    
{
public:

    static HRESULT Create(
        UINT uWidth,
        UINT uHeight,
        __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
        DisplayId associatedDisplay,
        BOOL fForBlending,
        __deref_out_ecount(1) CHwTextureRenderTarget ** const ppTextureRT
        DBG_STEP_RENDERING_COMMA_PARAM(__inout_ecount_opt(1) CHwDisplayRenderTarget *pDisplayRTParent)
        );

private:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwTextureRenderTarget));

    CHwTextureRenderTarget(
        __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        MilPixelFormat::Enum fmtTarget,
        D3DFORMAT d3dfmtTarget,
        DisplayId associatedDisplay
        );

    virtual ~CHwTextureRenderTarget();

    HRESULT Init(
        UINT uWidth,
        UINT uHeight
        DBG_STEP_RENDERING_COMMA_PARAM(__inout_ecount_opt(1) CHwDisplayRenderTarget *pDisplayRTParent)
        );

    HRESULT GetSurfaceDescription(
        UINT uWidth,
        UINT uHeight,
        __out_ecount(1) D3DSURFACE_DESC &sdLevel0
    ) const;

public:

    //
    // IUnknown methods
    //

    DECLARE_COM_BASE;
    STDMETHOD(HrFindInterface)(
        __in_ecount(1) REFIID riid,
        __deref_out void **ppv
        );

    //
    // IRenderTargetInternal methods
    //
    
   override STDMETHOD(DrawBitmap)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) IWGXBitmapSource *pIBitmap,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    override STDMETHOD(DrawMesh3D)(
        __inout_ecount(1) CContextState* pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) CMILMesh3D* pMesh3D,
        __inout_ecount_opt(1) CMILShader* pShader,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    override STDMETHOD(DrawPath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) IShapeData *pPath,
        __inout_ecount_opt(1) CPlainPen *pPen,
        __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
        __inout_ecount_opt(1) CBrushRealizer *pFillBrush
        );

    override STDMETHOD(DrawInfinitePath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) BrushContext *pBrushContext,
        __inout_ecount(1) CBrushRealizer *pFillBrush
        );
    
    override STDMETHOD(ComposeEffect)(
        __inout_ecount(1) CContextState *pContextState,
        __in_ecount(1) CMILMatrix *pScaleTransform,
        __inout_ecount(1) CMilEffectDuce* pEffect,
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        __in_opt IMILRenderTargetBitmap* pImplicitInput
        );

    override STDMETHOD(DrawGlyphs)(
        __inout_ecount(1) DrawGlyphsParameters &pars
        );

    override STDMETHOD(DrawVideo)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) IAVSurfaceRenderer *pSurfaceRenderer,
        __inout_ecount_opt(1) IWGXBitmapSource *pIBitmapSource,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    //
    // IMILRenderTarget methods
    //

    override STDMETHODIMP_(VOID) GetBounds(
        __out_ecount(1) MilRectF * const pBounds
        );

    override STDMETHOD(Clear)(
        __in_ecount_opt(1) const MilColorF *pColor,
        __in_ecount_opt(1) const CAliasedClip *pAliasedClip
        );

    override STDMETHOD(Begin3D)(
        __in_ecount(1) MilRectF const &rcBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        bool fUseZBuffer,
        FLOAT rZ
        );

    override STDMETHOD(End3D)();

    //
    // CHwSurfaceRenderTarget methods
    //

    override bool IsValid() const;

    CD3DVidMemOnlyTexture* GetTextureNoRef();

    //
    // IMILRenderTargetBitmap
    //

    STDMETHOD(GetBitmapSource)(
        __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
        );

    STDMETHOD(GetCacheableBitmapSource)(
        __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
        );    

    STDMETHOD(GetBitmap)(
        __deref_out_ecount(1) IWGXBitmap ** const ppIBitmap
        );

    override STDMETHOD(GetNumQueuedPresents)(
        __out_ecount(1) UINT *puNumQueuedPresents
        );

private:
    CD3DVidMemOnlyTexture *m_pVidMemOnlyTexture;

    CDeviceBitmap *m_pDeviceBitmap;
        // non-NULL after successful call to GetBitmapSource

    bool m_fInvalidContents;
};



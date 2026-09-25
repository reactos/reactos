// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwDisplayRenderTarget which implements IRenderTargetInternal
//      and IInternalRenderTargetHWND.
//

class CD3DSwapChain;
class CD3DDeviceLevel1;

ExternTag(tagDrawVideoPerf);

//------------------------------------------------------------------------------
//
//  Class: CHwDisplayRenderTarget
//
//  Description:
//      This object performs dirty rect management and stepped rendering support
//  for render targets that are presentable.
//
//------------------------------------------------------------------------------

class CHwDisplayRenderTarget :
    public CMILCOMBase,
    public CHwSurfaceRenderTarget,
    public IRenderTargetHWNDInternal
    DBG_STEP_RENDERING_COMMA_PARAM(public ISteppedRenderingDisplayRT)
{
public:

    static HRESULT Create(
        __in_ecount_opt(1) HWND hwnd,
        MilWindowLayerType::Enum eWindowLayerType,
        __in_ecount(1) CDisplay const *pDisplay,
        D3DDEVTYPE type,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) CHwDisplayRenderTarget **ppRenderTarget
        );

protected:

    CHwDisplayRenderTarget(
        __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        __in_ecount(1) D3DPRESENT_PARAMETERS const &D3DPresentParams,
        UINT AdapterOrdinalInGroup,
        DisplayId associatedDisplay
        );

    virtual ~CHwDisplayRenderTarget();

    virtual HRESULT Init(
        __in_ecount_opt(1) HWND hwnd,
        __in_ecount(1) CDisplay const *pDisplay,
        D3DDEVTYPE type,
        MilRTInitialization::Flags dwFlags
        );

public:

    //
    // CMILCOMBase overrides
    //

    DECLARE_COM_BASE;
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

    //
    // IInternalRenderTarget overrides
    //

    override STDMETHOD(DrawBitmap)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) IWGXBitmapSource *pIBitmap,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    override STDMETHOD(DrawMesh3D)(
        __inout_ecount(1) CContextState* pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) CMILMesh3D *pMesh3D,
        __inout_ecount_opt(1) CMILShader *pShader,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    override STDMETHOD(DrawPath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) IShapeData *pShape,
        __inout_ecount_opt(1) CPlainPen *pPen,
        __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
        __inout_ecount_opt(1) CBrushRealizer *pFillBrush
        );

    override STDMETHOD(DrawInfinitePath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) BrushContext *pBrushContext,
        __inout_ecount(1) CBrushRealizer *pFillBrush
        );

    STDMETHOD(DrawGlyphs)(
        __inout_ecount(1) DrawGlyphsParameters &pars
        );

    STDMETHOD(DrawVideo)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) IAVSurfaceRenderer *pSurfaceRenderer,
        __inout_ecount_opt(1) IWGXBitmapSource *pBitmapSource,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    //
    // IMILRenderTarget methods
    //

    STDMETHOD(Clear)(
        __in_ecount_opt(1) const MilColorF *pColor,
        __in_ecount_opt(1) const CAliasedClip *pAliasedClip
        );

    override STDMETHODIMP Begin3D(
        __in_ecount(1) MilRectF const &rcBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        bool fUseZBuffer,
        FLOAT rZ
        );

    override STDMETHODIMP End3D(
        );


    //
    // IRenderTargetHWNDInternal methods
    //

    STDMETHOD(Present)(
        __in_ecount(1) const RECT *pRect
        );

    STDMETHOD(ScrollBlt) (
        THIS_
        __in_ecount(1) const RECT *prcSource,
        __in_ecount(1) const RECT *prcDest
        ) PURE;    

    STDMETHOD(InvalidateRect)(
        __in_ecount(1) CMILSurfaceRect const *pRect
        );

    STDMETHOD(ClearInvalidatedRects)(
        )
    {
        return CBaseSurfaceRenderTarget<CHwRenderTargetLayerData>::ClearInvalidatedRects();
    }
    
    STDMETHOD(WaitForVBlank)();

    STDMETHOD_(VOID, AdvanceFrame)(
        UINT uFrameNumber
        );

protected:

    //
    // CHwSurfaceRenderTarget methods
    //

    //+------------------------------------------------------------------------
    //
    //  Member:    IsValid
    //
    //  Synopsis:  Returns FALSE when rendering with this render target or any
    //             use is no longer allowed.  Mode change is a common cause of
    //             of invalidation.
    //
    //-------------------------------------------------------------------------

    override bool IsValid() const;


protected:
    BOOL    m_fEnableRendering; // Rendering is disabled during resize
    CD3DSwapChain *m_pD3DSwapChain;
    D3DPRESENT_PARAMETERS m_D3DPresentParams;
    UINT const m_AdapterOrdinalInGroup;
    DWORD m_dwPresentFlags;

    // HRESULT indicating whether the display is invalid (on Resize/Present)
    HRESULT m_hrDisplayInvalid;

    CMILDeviceContext m_MILDC;

private:
    HRESULT PresentInternal(
        __in_ecount(1) CMILSurfaceRect const *prcSource,
        __in_ecount(1) CMILSurfaceRect const *prcDest,
        __in_ecount_opt(1) RGNDATA const *pDirtyRegion
        ) const;

#if DBG
protected:
    __forceinline void DbgSetValidContents() { m_fDbgInvalidContents = TRUE; }
    __forceinline void DbgSetInvalidContents() { m_fDbgInvalidContents = FALSE; }
    void DbgAssertNothingDirty();

private:
    BOOL m_fDbgInvalidContents;
#else
protected:
    MIL_FORCEINLINE void DbgSetValidContents() { }
    MIL_FORCEINLINE void DbgSetInvalidContents() { }
    MIL_FORCEINLINE void DbgAssertNothingDirty() { }
#endif

#if DBG_STEP_RENDERING
public:
    override void ShowSteppedRendering(
        __in LPCTSTR pszRenderDesc,
        __in_ecount(1) const ISteppedRenderingSurfaceRT *pRT
        );

private:
    HRESULT DbgStepCreateD3DSurfaceFromBitmapSource(
        __in_ecount(1) IWGXBitmapSource *pBitmap,
        __deref_out_ecount(1) CD3DSurface **ppD3DSurface
        );
        
    BOOL m_fDbgClearOnPresent;
#endif DBG_STEP_RENDERING
};



// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_meta
//      $Keywords:
//
//  $Description:
//      CMetaRenderTarget - This is a multiple or meta RenderTarget for
//      rendering on multiple devices. It handles enumerating the devices and
//      managing an array of sub-targets. If necessary it is able to hardware
//      accelerate and fall back to software RTs as appropriate.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


class CHwDisplayRenderTarget;
class CSwRenderTargetHWND;

extern DWORD g_dwCallNo;

//
// MetaData struct contains information for each sub-render target
//

struct MetaData
{
    IRenderTargetInternal *pInternalRT;
    BOOL fEnable;

     // offset to translate from meta rt space to internal rt space
    POINT ptInternalRTOffset;

    // Rendering bounds of device in local meta RT coordinate space.  It may
    // extend beyond virtual device bounds.  This rectangle is relative to the
    // meta RT origin.
    CMILSurfaceRect rcLocalDeviceRenderBounds;

    // Viable present bounds of device in local meta RT coordinate space.  It
    // may extend beyond virtual device bounds, but only for window RTs whose
    // windows can retain device specific offscreen contents.  See
    // CDesktopHWNDRenderTarget::ComputeRenderAndAdjustPresentBounds for
    // specifics.  This rectangle is relative to the meta RT origin.
    CMILSurfaceRect rcLocalDevicePresentBounds;

    // union of data specific to every type of MetaRT
    union {
        // Used by CDesktopRenderTarget
        struct {
            IRenderTargetHWNDInternal *pInternalRTHWND;
            CHwDisplayRenderTarget *pHwDisplayRT;
            CSwRenderTargetHWND *pSwHWNDRT;

            // Bounds of device in virtual coordinate space.  Often this is
            // virtual desktop space.
            CMILSurfaceRect rcVirtualDeviceBounds;

            // Bounds of target area that is expected to have some valid
            // content.  This rectangle is relative to the meta RT origin.
            CMILSurfaceRect rcLocalDeviceValidContentBounds;

        };

        // Used by CMetaBitmapRenderTarget
        struct {
            IMILRenderTargetBitmap *pIRTBitmap;
            UINT uIndexOfRealRTBitmap;
            ULONG cacheIndex; // For convenience of creating
        };
    };

    // When true no invalid render bounds were returned from GetInvalidRegions
    // because all bounds required for Present are already valid.
    WHEN_DBG_ANALYSIS(bool fDbgPresentBoundsAreValid);
};

class CMetaRenderTarget:
    public IRenderTargetInternal
{

protected:

    // internal methods.

    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

    CMetaRenderTarget(
        __out_ecount(cMaxRTs) MetaData *pMetaData,
        UINT cMaxRTs,
        __inout_ecount(1) CDisplaySet const *pDisplaySet
        );
    virtual ~CMetaRenderTarget();

    CDisplaySet const * DisplaySet() const {return m_pDisplaySet;}

    static void UpdateValidContentBounds(
        __inout_ecount(1) MetaData &oDevData,
        __in_ecount(1) CAliasedClip const &aliasedDeviceClip
        );

public:

    // IMILRenderTarget.

    STDMETHOD(Clear)(
        __in_ecount_opt(1) const MilColorF *pColor,
        __in_ecount_opt(1) const CAliasedClip *pAliasedClip
        );

    STDMETHOD(Begin3D)(
        __in_ecount(1) MilRectF const &rcBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        bool fUseZBuffer,
        FLOAT rZ
        ) override;

    STDMETHOD(End3D)(
        ) override;

    // IRenderTargetInternal.

    STDMETHOD_(__outro_ecount(1) const CMILMatrix *, GetDeviceTransform)() const;

    STDMETHOD(DrawBitmap)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) IWGXBitmapSource *pIBitmap,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    STDMETHOD(DrawMesh3D)(
        __inout_ecount(1) CContextState* pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) CMILMesh3D *pMesh3D,
        __inout_ecount_opt(1) CMILShader *pShader,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    STDMETHOD(DrawPath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) IShapeData *pShape,
        __inout_ecount_opt(1) CPlainPen *pPen,
        __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
        __inout_ecount_opt(1) CBrushRealizer *pFillBrush
        );

    STDMETHOD(DrawInfinitePath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) BrushContext *pBrushContext,
        __inout_ecount(1) CBrushRealizer *pFillBrush
        );

    STDMETHOD(ComposeEffect)(
        __inout_ecount(1) CContextState *pContextState,
        __in_ecount(1) CMILMatrix *pScaleTransform,
        __inout_ecount(1) CMilEffectDuce* pEffect,
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        __in_opt IMILRenderTargetBitmap* pImplicitInput
        );
    
    STDMETHOD(DrawGlyphs)(
        __inout_ecount(1) DrawGlyphsParameters &pars
        );

    STDMETHOD(CreateRenderTargetBitmap)(
        UINT width,
        UINT height,
        IntermediateRTUsage usageInfo,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
        __in_opt DynArray<bool> const *pActiveDisplays = NULL
        ) override;

    STDMETHOD(BeginLayer)(
        __in_ecount(1) MilRectF const &LayerBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        __in_ecount_opt(1) IShapeData const *pGeometricMask,
        __in_ecount_opt(1) CMILMatrix const *pGeometricMaskToTarget,
        FLOAT flAlphaScale,
        __in_ecount_opt(1) CBrushRealizer *pAlphaMask
        );

    STDMETHOD(EndLayer)(
        );

    STDMETHOD_(void, EndAndIgnoreAllLayers)(
        );
    
    STDMETHOD(ReadEnabledDisplays) (
        __inout DynArray<bool> *pEnabledDisplays
        ) override;
    
    // This method is used to determine if the render target is being
    // used to render, or if it's merely being used for bounds accumulation,
    // hit test, etc.
    STDMETHOD(GetType) (__out DWORD *pRenderTargetType);


    // This method is used to allow a developer to force ClearType use in
    // intermediate render targets with alpha channels.
    STDMETHOD(SetClearTypeHint) (
        __in bool forceClearType
        );

    UINT GetRealizationCacheIndex() override;

    STDMETHOD(DrawVideo)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) IAVSurfaceRenderer *pSurfaceRenderer,
        __inout_ecount(1) IWGXBitmapSource *pBitmapSource,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );
    
    bool HasEnabledDeviceIndex(IMILResourceCache::ValidIndex cacheIndex);

    __out_ecount_opt(1) CMetaRenderTarget *DynCastToMeta() override { return this; }

protected:

    MIL_FORCEINLINE BOOL FindFirstEnabledRT(
        __out_ecount(1) UINT *pidxFirstEnabledRT
        ) const;

    override STDMETHOD(GetNumQueuedPresents)(
        __out_ecount(1) UINT *puNumQueuedPresents
        );

protected:

    // internal data

    // Count of the render targets.
    UINT const m_cRT;

    // Internal render target arrays.
    __field_ecount_full(m_cRT) MetaData * const m_rgMetaData;

    bool m_fUseRTOffset;            // TRUE if ptInternalRTOffset is non-0 for
                                    // any device
    bool m_fAccumulateValidBounds;  // TRUE for meta RTs that track areas of
                                    // valid contents - currently HWND RT

    // friend classes declarations needed so that  meta data can be adjusted
    friend class CAdjustBrush;
    friend class CAdjustEffectList;
    friend class CAdjustBitmapSource;

    // NOTICE-2006/07/11-milesc  CMetaBitmapRenderTarget must be a friend of
    // CMetaRenderTarget so that WorksOnSameDeviceAs() may access the protected
    // meta data. In C++ you don't normally need to be a friend to access
    // protected members, but you do if those members are on an object other
    // than "this" *and* if that object is a superclass of a the class
    // containing the method which needs the protected member.
    friend class CMetaBitmapRenderTarget;

protected:
    // Current snap of display data
    CDisplaySet const * const m_pDisplaySet;
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaRenderTarget::FindFirstEnabledRT
//
//  Synopsis:
//      Finds the index of the first enabled RT.  Returns FALSE if there are no
//      enabled RTs
//
//------------------------------------------------------------------------------
BOOL CMetaRenderTarget::FindFirstEnabledRT(
    __out_ecount(1) UINT *pidxFirstEnabledRT
    ) const
{
    for (UINT i = 0; i < m_cRT; i++)
    {
        if (m_rgMetaData[i].fEnable)
        {
            *pidxFirstEnabledRT = i;
            return TRUE;
        }
    }

    return FALSE;
}




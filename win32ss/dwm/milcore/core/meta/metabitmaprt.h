// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_targets
//      $Keywords:
//
//  $Description:
//      Contains CMetaBitmapRenderTarget which implements IMILRenderTargetBitmap
//      and a limited IWGXBitmapSource.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMetaBitmapRenderTarget);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CMetaBitmapRenderTarget
//
//  Synopsis:
//      This is a multiple or meta Render Target for rendering on multiple
//      offscreen surfaces.  This is also a meta Bitmap Source that holds
//      references to IWGXBitmapSources specific to the sub Render Targets.
//
//------------------------------------------------------------------------------

class CMetaBitmapRenderTarget:
    public CMILCOMBase,
    public CMetaRenderTarget,
    public IMILRenderTargetBitmap,
    public IWGXBitmapSource
{

    // internal methods.

private:

    CMetaBitmapRenderTarget(
        UINT cMaxRTs,
        __inout_ecount(1) CDisplaySet const *pDisplaySet
        );

    inline void * __cdecl operator new(size_t cb, size_t cRTs);

    inline void __cdecl operator delete(void * pv) { WPFFree(ProcessHeap, pv); }

    inline void __cdecl operator delete(void * pv, size_t) { WPFFree(ProcessHeap, pv); }

protected:

    virtual ~CMetaBitmapRenderTarget();

    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

    HRESULT Init(
        UINT uWidth,
        UINT uHeight,
        IntermediateRTUsage usageInfo,
        MilRTInitialization::Flags dwFlags,
        __in_xcount(m_cRTs) MetaData const *pMetaData
        );


    // public methods

public:

    // This method creates an initialized and
    // referenced RT instance.

    static STDMETHODIMP Create(
        UINT uWidth,
        UINT uHeight,
        UINT cRTs,
        __in_ecount(cRTs) MetaData *pMetaData,
        __in_ecount(1) CDisplaySet const *pDisplaySet,
        IntermediateRTUsage usageInfo,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) CMetaBitmapRenderTarget **ppMetaRT
        );

    // IUnknown.

    DECLARE_COM_BASE;

    // IMILRenderTarget.

    STDMETHODIMP_(VOID) GetBounds(
        __out_ecount(1) MilRectF * const pBounds
        );

    override STDMETHODIMP Clear(
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

    // IMILRenderTargetBitmap.

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

    // IWGXBitmapSource.

    STDMETHOD(GetSize)(
        __out_ecount(1) UINT *puWidth,
        __out_ecount(1) UINT *puHeight
        );

    STDMETHOD(GetPixelFormat)(
        __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
        );

    STDMETHOD(GetResolution)(
        __out_ecount(1) double *pDpiX,
        __out_ecount(1) double *pDpiY
        );

    STDMETHOD(CopyPalette)(
        __inout_ecount(1) IWICPalette *pIPalette
        );

    STDMETHOD(CopyPixels)(
        __in_ecount_opt(1) const MILRect *prc,
        UINT cbStride,
        UINT cbBufferSize,
        __out_ecount(cbBufferSize) BYTE *pvPixels
        );


    // Additional methods

    HRESULT GetCompatibleSubRenderTargetNoRef(
        IMILResourceCache::ValidIndex uOptimalRealizationCacheIndex,
        DisplayId targetDestination,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetNoRef
        );

private:

    __out_ecount_opt(1) IMILRenderTargetBitmap *GetCompatibleSubRenderTargetNoRefInternal(
        IMILResourceCache::ValidIndex uOptimalRealizationCacheIndex,
        DisplayId targetDestination
        );

protected:

    // Dimensions of the bitmap(s)
    UINT m_uWidth;
    UINT m_uHeight;
};




// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_interop
//      $Keywords:
//
//  $Description:
//      Contains CHwDeviceBitmapColorSource declaration
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


MtExtern(CHwDeviceBitmapColorSource);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwDeviceBitmapColorSource
//
//  Synopsis:
//      Provides a shared bitmap color source for a HW device
//
//------------------------------------------------------------------------------

class CHwDeviceBitmapColorSource : public CHwBitmapColorSource
{
public:

    static HRESULT CreateForTexture(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) IWGXBitmap *pBitmap,
        MilPixelFormat::Enum fmt,
        __in_ecount(1) CMilRectU const &rcBoundsRequired,
        __inout_ecount(1) CD3DVidMemOnlyTexture *pVidMemTexture,
        __deref_out_ecount(1) CHwDeviceBitmapColorSource **ppHwBitmapCS
        );


    static HRESULT CreateWithSharedHandle(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) IWGXBitmap *pBitmap,
        MilPixelFormat::Enum fmt,
        __in_ecount(1) CMilRectU const &rcBoundsRequired,
        __deref_out_ecount(1) CHwDeviceBitmapColorSource **ppHwBitmapCS,
        __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle
        );

    override virtual HRESULT Realize(
        );

    //
    // Query methods
    //

    bool DoesContain(
        __in_ecount(1) CMilRectU const &rcBoundsRequired
        ) const
    {
        return m_rcPrefilteredBitmap.DoesContain(rcBoundsRequired);
    }

    bool IsAdapter(
        LUID luidAdapter
        ) const;


    //
    // Property setting methods
    //

    void UpdateValidBounds(
        __in_ecount(1) CMilRectU const &rcValid
        );


    //
    // Other methods
    //

    HRESULT CopyPixels(
        __in_ecount(1) const CMilRectU &rcCopy,
        UINT cClipRects,
        __in_ecount_opt(cClipRects) const CMilRectU *rgClipRects,
        MilPixelFormat::Enum fmtOut,
        DBG_ANALYSIS_PARAM_COMMA(UINT cbBufferOut)
        __out_bcount_full(cbBufferOut) BYTE * const pbBufferOut,
        UINT nStrideOut
        );

    virtual HRESULT UpdateSurface(
        __in UINT cDirtyRects,
        __in_ecount(cDirtyRects) const CMilRectU *prgDirtyRects,
        __in_ecount(1) IDirect3DSurface9 *pISrcSurface
        );
    
    virtual __out_opt CD3DSurface *GetValidTransferSurfaceNoRef();

protected:

    CHwDeviceBitmapColorSource(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_opt IWGXBitmap *pBitmap,
        MilPixelFormat::Enum fmt,
        __in_ecount(1) const D3DSURFACE_DESC &d3dsd,
        UINT uLevels
        );
    
    virtual ~CHwDeviceBitmapColorSource();
    
    static HRESULT GetRealizationDesc(
        __in IWGXBitmap *pBitmap,
        MilPixelFormat::Enum fmt,
        __in const CMilRectU &rcBoundsRequired,
        __out CacheParameters &oRealizationDesc
        );

    HRESULT Init(
        __in_ecount(1) IWGXBitmap *pBitmap,
        __in_ecount(1) CacheParameters const &oRealizationDesc,
        __inout_ecount_opt(1) CD3DVidMemOnlyTexture *pVidMemTexture,
        __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle
        );

    static HRESULT CreateCommon(
        __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) IWGXBitmap *pBitmap,
        MilPixelFormat::Enum fmt,
        __in_ecount(1) const CMilRectU &rcBoundsRequired,
        __in_ecount_opt(1) const CD3DVidMemOnlyTexture *pVidMemTexture,
        __out_ecount(1) CacheParameters &oRealizationDesc,
        __out_ecount(1) D3DSURFACE_DESC &d3dsd,
        __out_ecount(1) UINT &uLevels
        );

private:
    HRESULT Flush(
        __in_ecount(1) IDirect3DDevice9 *pID3DDevice,
        __in_ecount(1) IDirect3DSurface9 *pID3DSurface,
        __in_ecount(1) const D3DSURFACE_DESC &desc
        );

    HRESULT UpdateSurfaceSharedHandle(
        UINT cDirtyRects,
        __in_ecount(cDirtyRects) const CMilRectU *prgDirtyRects,
        __in_ecount(1) IDirect3DSurface9 *pISrcSurface
        );

    HRESULT UpdateSurfaceSoftware(
        UINT cDirtyRects,
        __in_ecount(cDirtyRects) const CMilRectU *prgDirtyRects,
        __in_ecount(1) IDirect3DSurface9 *pISrcSurface
        );

    static HRESULT CreateInternal(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) IWGXBitmap *pBitmap,
        MilPixelFormat::Enum fmt,
        __in_ecount(1) CMilRectU const &rcBoundsRequired,
        __inout_ecount_opt(1) CD3DVidMemOnlyTexture *pVidMemTexture,
        __deref_out_ecount(1) CHwDeviceBitmapColorSource **ppHwBitmapCS,
        __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle
        );

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwDeviceBitmapColorSource));
    
    override HRESULT GetPointerToValidSourceRects(
        __in_ecount_opt(1) IWGXBitmap *pBitmap,
        __out_ecount(1) UINT &cValidSourceRects,
        __deref_out_ecount_full(cValidSourceRects) CMilRectU const * &rgValidSourceRects
        ) const;

    // Can be NULL!
    HANDLE m_hSharedHandle;

    // System memory texture used for software updates in UpdateSurfaceSoftware
    CD3DLockableTexture *m_pSysMemTexture;
};




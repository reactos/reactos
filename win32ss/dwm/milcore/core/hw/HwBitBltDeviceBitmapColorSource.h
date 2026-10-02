// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//------------------------------------------------------------------------------

MtExtern(CHwBitBltDeviceBitmapColorSource);

class CHwBitBltDeviceBitmapColorSource : public CHwDeviceBitmapColorSource
{
public:
    
    override ~CHwBitBltDeviceBitmapColorSource();

    static HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) IWGXBitmap *pBitmap,
        MilPixelFormat::Enum fmt,
        __in_ecount(1) CMilRectU const &rcBoundsRequired,
        bool fIsDependent,
        __deref_out_ecount(1) CHwDeviceBitmapColorSource **ppHwBitBltDBCS
        );

    override HRESULT UpdateSurface(
        __in UINT cDirtyRects,
        __in_ecount(cDirtyRects) const CMilRectU *prgDirtyRects,
        __in_ecount(1) IDirect3DSurface9 *pISrcSurface
        );

    override HRESULT Realize(
        );

    override __out_opt CD3DSurface *GetValidTransferSurfaceNoRef();

private:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwBitBltDeviceBitmapColorSource));

    CHwBitBltDeviceBitmapColorSource(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_opt IWGXBitmap *pBitmap,
        MilPixelFormat::Enum fmt,
        __in_ecount(1) const D3DSURFACE_DESC &d3dsd,
        UINT uLevels
        );

    // Target of the cross-device BitBlt operation. This is then copied to the destination
    // texture. See comment on UpdateSurface.
    CD3DSurface *m_pTransferSurface;
};



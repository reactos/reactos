// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains D3D surface wrapper class declaration
//

MtExtern(CD3DSurface);
MtExtern(D3DResource_Surface);

class CD3DSurface : public CD3DResource
{
private:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CD3DSurface));

public:
    static HRESULT Create(
        __inout_ecount(1) CD3DResourceManager *pResourceManager,
        __inout_ecount(1) IDirect3DSurface9 *pD3DSurface,
        __deref_out_ecount(1) CD3DSurface **ppSurface
        );

    D3DSURFACE_DESC const &Desc() const { return m_d3dsd; }

    //
    // IDirect3DSurface methods
    //

    HRESULT LockRect(
        __out_ecount(1) D3DLOCKED_RECT *pLockedRect,
        __in_ecount(1) CONST RECT *pRect,
        DWORD Flags
        )
    {
        return ID3DSurface()->LockRect(pLockedRect, pRect, Flags);
    }

    HRESULT UnlockRect(
        )
    {
        return ID3DSurface()->UnlockRect();
    }


    __out_ecount_opt(1) IDirect3DSurface9 * GetD3DSurfaceNoAddRef() const
    {
        return m_pD3DSurface;
    }

    __out_ecount(1) IDirect3DSurface9 * ID3DSurface() const
    {
        Assert(m_pD3DSurface);
        return m_pD3DSurface;
    }

    void GetSurfaceSize(
        __out_ecount(1) UINT *puWidth,
        __out_ecount(1) UINT *puHeight
        ) const;

    HRESULT GetDC(
        __out_ecount(1) HDC *phdc
        );

    HRESULT ReleaseDC(
        HDC hdc
        )
    {
        return ID3DSurface()->ReleaseDC(hdc);
    }

    HRESULT ReadIntoSysMemBuffer(
        __in_ecount(1) const CMilRectU &rcSource,
        UINT cClipRects,
        __in_ecount_opt(cClipRects) const CMilRectU *rgClipRects,
        MilPixelFormat::Enum fmtBitmapOut,
        UINT nStrideOut,
        DBG_ANALYSIS_PARAM_COMMA(UINT cbBufferOut)
        __out_bcount_full(cbBufferOut) BYTE *pbBufferOut
        );

protected:
    CD3DSurface(__inout_ecount(1) IDirect3DSurface9 * pD3DSurface);
    ~CD3DSurface();

    HRESULT Init(
        __inout_ecount(1) CD3DResourceManager *pResourceManager
        );

#if PERFMETER
    virtual PERFMETERTAG GetPerfMeterTag() const
    {
        return Mt(D3DResource_Surface);
    }
#endif

private:

    // 
    // CD3DResource methods
    //

    virtual override void ReleaseD3DResources();

protected:

    // Pointer to the actual D3D resource.
    //  The pointer is constant to help enforce the modification restrictions
    //  of CD3DResource objects.
    IDirect3DSurface9 * const m_pD3DSurface;

    D3DSURFACE_DESC m_d3dsd;
};



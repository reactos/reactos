// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DSurface implementation
//
//      Provides basic abstraction of a D3D surface and tracks it as a D3D
//      resource.
//

#include "precomp.hpp"

MtDefine(CD3DSurface, MILRender, "CD3DSurface");
MtDefine(D3DResource_Surface, MILHwMetrics, "Approximate surface sizes");

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::Create
//
//  Synopsis:  Create a CD3DSurface object to wrap a D3D texture
//
//-------------------------------------------------------------------------
HRESULT 
CD3DSurface::Create(
    __inout_ecount(1) CD3DResourceManager *pResourceManager,
    __inout_ecount(1) IDirect3DSurface9 *pD3DSurface,
    __deref_out_ecount(1) CD3DSurface **ppSurface
    )
{
    HRESULT hr = S_OK;

    *ppSurface = NULL;

    //
    // Create the D3D surface wrapper
    //

    *ppSurface = new CD3DSurface(pD3DSurface);
    IFCOOM(*ppSurface);
    (*ppSurface)->AddRef(); // CD3DSurface::ctor sets ref count == 0

    //
    // Call init
    //

    IFC((*ppSurface)->Init(
        pResourceManager
        ));

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(*ppSurface);
    }
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::CD3DSurface
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CD3DSurface::CD3DSurface(
    __inout_ecount(1) IDirect3DSurface9 * const pD3DSurface
    )
    : m_pD3DSurface(pD3DSurface)
{
    m_pD3DSurface->AddRef();
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::~CD3DSurface
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CD3DSurface::~CD3DSurface()
{
    ReleaseInterfaceNoNULL(m_pD3DSurface);
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::Init
//
//  Synopsis:  Inits the surface wrapper
//
//-------------------------------------------------------------------------
HRESULT 
CD3DSurface::Init(
    __inout_ecount(1) CD3DResourceManager *pResourceManager
    )
{
    HRESULT hr = S_OK;
    UINT uResourceSize;

    //
    // Compute the size of the resource
    //

    IFC(m_pD3DSurface->GetDesc(&m_d3dsd));

    if (m_d3dsd.Pool == D3DPOOL_SYSTEMMEM)
    {
        uResourceSize = 0;
    }
    else
    {
        UINT uSamplesPerPixel = 1;
    
        if (m_d3dsd.MultiSampleType >= D3DMULTISAMPLE_2_SAMPLES)
        {
            C_ASSERT(D3DMULTISAMPLE_2_SAMPLES == 2);
            uSamplesPerPixel = static_cast<UINT>(m_d3dsd.MultiSampleType);
        }
    
        uResourceSize = D3DFormatSize(m_d3dsd.Format) * m_d3dsd.Width * m_d3dsd.Height * uSamplesPerPixel;
        Assert(uResourceSize > 0);
    }

    //
    // Init the base class
    //

    CD3DResource::Init(pResourceManager, uResourceSize);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::ReleaseD3DResources
//
//  Synopsis:  Release the surface.
//
//             This method may only be called by CD3DResourceManager because
//             there are various restrictions around when a call to
//             ReleaseD3DResources is okay.
//
//-------------------------------------------------------------------------
void 
CD3DSurface::ReleaseD3DResources()
{
    // This resource should have been marked invalid already or at least be out
    // of use.
    Assert(!m_fResourceValid || (m_cRef == 0));
    Assert(IsValid() == m_fResourceValid);

    if (m_d3dsd.Usage & D3DUSAGE_RENDERTARGET)
    {
        Device().ReleaseUseOfRenderTarget(this);
    }

    if (m_d3dsd.Usage & D3DUSAGE_DEPTHSTENCIL)
    {
        Device().ReleaseUseOfDepthStencilSurface(this);
    }

    // This context is protected so it is safe to release the D3D resource
    ReleaseInterface((*const_cast<IDirect3DSurface9 **>(&m_pD3DSurface)));

    return;
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::GetSurfaceSize
//
//  Synopsis:  Return the dimensions of the surface.
//
//-------------------------------------------------------------------------
void
CD3DSurface::GetSurfaceSize(
    __out_ecount(1) UINT *puWidth,
    __out_ecount(1) UINT *puHeight
    ) const
{
    Assert(IsValid());

    *puWidth = m_d3dsd.Width;
    *puHeight = m_d3dsd.Height;
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::GetDC
//
//  Synopsis:  Retrieve a DC wrapping this surface
//
//-------------------------------------------------------------------------
HRESULT 
CD3DSurface::GetDC(
    __out_ecount(1) HDC *phdc
    )
{
    HRESULT hr = S_OK;

    HDC hBackBuffer = NULL;

    *phdc = NULL;

    MIL_THR(m_pD3DSurface->GetDC(
        &hBackBuffer
        ));

    //
    // D3D isn't supposed to return E_FAIL to us.  We want to see if we can 
    // reinterpret it.
    //
    if (hr == E_FAIL)
    {
        //
        // Check to see if we're close to being out of available GDI handles.
        // If we are we'll evaluate that as E_OUTOFMEMORY, otherwise we'll
        // return D3DERR_DRIVERINTERNALERROR.
        //
        IFC(CheckGUIHandleQuota(
            GR_GDIOBJECTS,
            E_OUTOFMEMORY,
            D3DERR_DRIVERINTERNALERROR
            ));
    }

    //
    // If we've succeeded assign the Handle to return.
    //
    if (SUCCEEDED(hr))
    {
        *phdc = hBackBuffer;
        hBackBuffer = NULL;
    }

Cleanup:
    //
    // If we have a backbuffer handle at this point it means 
    // something failed.
    //
    if (hBackBuffer)
    {
        IGNORE_HR(m_pD3DSurface->ReleaseDC(hBackBuffer));
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DSurface::ReadIntoSysMemBuffer
//
//  Synopsis:  Reads the surface's contents in the specified source rectangle
//             and writes it into the provided buffer.
//
//             If cClipRects is non-zero, then writes are restricted to the
//             intersection of rcSource and rectangles in rgClipRects.  For
//             best performance clipping rectangles should not overlap one
//             another.
//
//             Note: the output buffer might be sparsely filled if nStrideOut
//                   is larger than the stride of the source in the given
//                   rectangle.
//
//             IMPORTANT: HwUtil.cpp's ReadRenderTargetIntoSysMemBuffer borrows 
//             heavily from this so it should be kept in sync with this 
//             implementation
//             
//-----------------------------------------------------------------------------
HRESULT
CD3DSurface::ReadIntoSysMemBuffer(
    __in_ecount(1) const CMilRectU &rcSource,
    UINT cClipRects,
    __in_ecount_opt(cClipRects) const CMilRectU *rgClipRects,
    MilPixelFormat::Enum fmtOut,
    UINT nStrideOut,
    DBG_ANALYSIS_PARAM_COMMA(UINT cbBufferOut)
    __out_bcount_full(cbBufferOut) BYTE *pbBufferOut
    )
{
    HRESULT hr = S_OK;
    IDirect3DSurface9 *pD3DLockableSurface = NULL;
    IDirect3DSurface9 *pTempSurface = NULL;
    
    bool fNeedToManuallyCopyBits = true;
    UINT nStrideCopy;

    UINT const uSourceWidth = rcSource.Width();
    UINT const uSourceHeight = rcSource.Height();

    RECT const rcDest = { 0, 0, uSourceWidth, uSourceHeight };

    BYTE const BitsPerPixel = GetPixelFormatSize(fmtOut);

    if (BitsPerPixel % BITS_PER_BYTE)
    {
        TraceTag((tagMILWarning,
                  "Call to CHwDeviceBitmapColorSource::CopyPixels requested fraction byte copy"));
        IFC(WGXERR_INVALIDPARAMETER);
    }

    IFC(HrCalcByteAlignedScanlineStride(uSourceWidth, BitsPerPixel, nStrideCopy));

#if DBG_ANALYSIS
    Assert(nStrideCopy <= nStrideOut);
    Assert(SUCCEEDED(HrCheckBufferSize(fmtOut, nStrideOut, uSourceWidth, uSourceHeight, cbBufferOut)));
#endif

    Assert(Device().IsInAUseContext());

    D3DFORMAT d3dfmtOut = PixelFormatToD3DFormat(fmtOut);

    if ((m_d3dsd.Pool == D3DPOOL_MANAGED) || (m_d3dsd.Pool == D3DPOOL_SYSTEMMEM))
    {
        if (d3dfmtOut != m_d3dsd.Format)
        {
            IFC(WGXERR_INVALIDCALL);
        }

        SetInterface(pD3DLockableSurface, ID3DSurface());
    }
    else
    {
        //
        // Create a lockable copy or wrapper around bit buffer and copy
        //

#if DBG
        {
            // Double check that we have the right device
            IDirect3DDevice9 *pID3DDevice = NULL;
            if (SUCCEEDED(ID3DSurface()->GetDevice(&pID3DDevice)))
            {
                Assert(pID3DDevice == Device().DbgGetID3DDevice9());
            }
            ReleaseInterface(pID3DDevice);
        }
#endif
        void* pvSysMemPixels = NULL;

        //
        // Shared surf create should take stride
        // CreateSysMemUpdateSurface with WDDM can provide a wrapper around a
        // system memory buffer owned by this module.  But surface width in
        // bytes must equal the stride.  Since GetRenderTargetData also has
        // surface size restrictions and nothing can be done about the out
        // buffer properties, check that out buffer meets requirements before
        // creating the wrapper.  If the check fails (or WDDM isn't available)
        // have D3D allocate system memory and do an extra system memory to
        // system memory transfer.
        //
        // Independent of the above, if there are clip rects, then handle
        // clipping with CPU copies rather than attempting several
        // GetRenderTargetData calls which probably still require a CPU
        // transfer.
        //
        if (   Device().IsLDDMDevice()
            && nStrideOut == nStrideCopy
            && cClipRects == 0)
        {
            fNeedToManuallyCopyBits = false;
            pvSysMemPixels = pbBufferOut;
        }        

        IFC(Device().CreateSysMemUpdateSurface(
            uSourceWidth, 
            uSourceHeight, 
            d3dfmtOut, 
            pvSysMemPixels, 
            &pD3DLockableSurface));

        //
        // The target of GetRenderTargetData has to be the same size and format
        // as the source.  If we need less that the full rendertarget or one of
        // a different format, we need to create a temporary rendertarget which
        // is the right size and format, copy into that, and then readback from
        // there.  If we need the whole thing and format matches, just use the
        // normal rendertarget.
        //
        if (   uSourceWidth != m_d3dsd.Width
            || uSourceHeight != m_d3dsd.Height
            || d3dfmtOut != m_d3dsd.Format)
        {
            IFC(Device().CheckRenderTargetFormat(m_d3dsd.Format, /* pphrTestGetDC = */ NULL));

            IFC(Device().CreateRenderTargetUntracked(
                uSourceWidth,
                uSourceHeight,
                m_d3dsd.Format,
                m_d3dsd.MultiSampleType,
                m_d3dsd.MultiSampleQuality,
                FALSE,
                &pTempSurface
                ));

            IFC(Device().StretchRect(
                this,
                reinterpret_cast<CMilRectL const *>(&rcSource),
                pTempSurface, 
                &rcDest,
                D3DTEXF_NONE
                ));
        }
        
        IFC(Device().GetRenderTargetData(
            pTempSurface ? pTempSurface : this->ID3DSurface(), 
            pD3DLockableSurface
            ));
    }

    //
    // Manually lock the surface and copy the bits to the destination if we
    // weren't able to get it done automatically by GetRenderTargetData.
    //
    if (fNeedToManuallyCopyBits)
    {
        D3DLOCKED_RECT d3dlr;
        
        IFC(pD3DLockableSurface->LockRect(&d3dlr, &rcDest, D3DLOCK_READONLY));

        UINT const BytesPerPixel = BitsPerPixel / BITS_PER_BYTE;

    #if DBG_ANALYSIS
        BYTE *pbAnalysisBufferOutOrig = pbBufferOut;
    #endif

        if (   cClipRects == 0
            || rgClipRects == NULL)
        {
            cClipRects = 1;
            rgClipRects = &rcSource;
        }

        do
        {
            CMilRectU rcCopy = *rgClipRects;
            if (rcCopy.Intersect(rcSource))
            {
                // Adjust pbBufferOut and locked D3D buffer pointer by amount
                // inset by intersection with source rect.  None of this math
                // should overflow since rcCopy is restricted to rcSource which
                // has already been validated as a good bounding rectangle.
                UINT cbBufferInset, cbLeftInset;

                IFC(MultiplyUINT(BytesPerPixel, rcCopy.left - rcSource.left, OUT cbLeftInset));

                IFC(MultiplyUINT(nStrideOut, rcCopy.top - rcSource.top, OUT cbBufferInset));
                IFC(AddUINT(cbBufferInset, cbLeftInset, OUT cbBufferInset));
                BYTE *pbOutBuffer = pbBufferOut + cbBufferInset;

                IFC(MultiplyUINT(d3dlr.Pitch, rcCopy.top - rcSource.top, OUT cbBufferInset));
                IFC(AddUINT(cbBufferInset, cbLeftInset, OUT cbBufferInset));
                BYTE *pbInBuffer = static_cast<BYTE *>(d3dlr.pBits) + cbBufferInset;

                IFC(MultiplyUINT(BytesPerPixel, rcCopy.Width(), OUT nStrideCopy));

                for (UINT i = rcCopy.top; i < rcCopy.bottom; i++)
                {
                    Assert(pbOutBuffer + nStrideCopy <= pbAnalysisBufferOutOrig + cbBufferOut);

                    GpMemcpy(pbOutBuffer, pbInBuffer, nStrideCopy);
                    pbOutBuffer += nStrideOut;
                    pbInBuffer += d3dlr.Pitch;
                }
            }
        } while (rgClipRects++, --cClipRects > 0);

        IGNORE_HR(pD3DLockableSurface->UnlockRect());
    }
    
Cleanup:
    ReleaseInterface(pD3DLockableSurface);
    ReleaseInterface(pTempSurface);
    
    RRETURN(hr);
}





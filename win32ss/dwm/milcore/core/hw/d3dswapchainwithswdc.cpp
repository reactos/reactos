// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DSwapChainWithSwDC implementation
//
//      This class overrides the GetDC method of CD3DSwapChain to implement
//      GetDC using GetRenderTargetData. This approach acheived phenominal perf
//      wins in WDDM.
//

#include "precomp.hpp"

//+----------------------------------------------------------------------------
//
//  Member:
//      CD3DSwapChainWithSwDC::CD3DSwapChainWithSwDC
//
//  Synopsis:
//      ctor
//

#pragma warning( push )
#pragma warning( disable : 4355 )

CD3DSwapChainWithSwDC::CD3DSwapChainWithSwDC(
    __in HDC hdcPresentVia,
    __in_range(>, 0) /*__out_range(==, this->m_cBackBuffers)*/ UINT cBackBuffers,
    __inout_ecount(1) IDirect3DSwapChain9 *pD3DSwapChain
    ) : CD3DSwapChain(
            pD3DSwapChain,
            cBackBuffers,
            reinterpret_cast<CD3DSurface **>(reinterpret_cast<BYTE *>(this) + sizeof(*this))
            ),
      m_hdcCopiedBackBuffer(hdcPresentVia),
      m_hbmpCopiedBackBuffer(NULL),
      m_pBuffer(NULL)
{
}

#pragma warning( pop )

//+----------------------------------------------------------------------------
//
//  Member:
//      CD3DSwapChainWithSwDC::~CD3DSwapChainWithSwDC
//
//  Synopsis:
//      dtor
//

CD3DSwapChainWithSwDC::~CD3DSwapChainWithSwDC()
{
    if (m_hdcCopiedBackBuffer)
    {
        DeleteDC(m_hdcCopiedBackBuffer);
    }

    if (m_hbmpCopiedBackBuffer)
    {
        DeleteObject(m_hbmpCopiedBackBuffer);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CD3DSwapChainWithSwDC::Init
//
//  Synopsis:
//      Inits the swap chain and creates the sysmem present surface
//
//-----------------------------------------------------------------------------
HRESULT
CD3DSwapChainWithSwDC::Init(
    __inout_ecount(1) CD3DResourceManager *pResourceManager
    )
{
    HRESULT hr = S_OK;

    // The base class must be initialized first
    IFC(CD3DSwapChain::Init(pResourceManager));

    Assert(m_cBackBuffers >= 1);
    D3DSURFACE_DESC const &surfDesc = m_rgBackBuffers[0]->Desc();

    // We don't handle anything else yet
    Assert(   surfDesc.Format == D3DFMT_A8R8G8B8
           || surfDesc.Format == D3DFMT_X8R8G8B8
          );

    BITMAPINFO bmi;

    bmi.bmiHeader.biSize = sizeof(bmi);
    bmi.bmiHeader.biWidth = surfDesc.Width;
    bmi.bmiHeader.biHeight = -INT(surfDesc.Height);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 10000;
    bmi.bmiHeader.biYPelsPerMeter = 10000;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;

    IFCW32_CHECKOOH(GR_GDIOBJECTS, m_hbmpCopiedBackBuffer = CreateDIBSection(
        m_hdcCopiedBackBuffer,
        &bmi,
        DIB_RGB_COLORS,
        &m_pBuffer,
        NULL,
        0
        ));

    MilPixelFormat::Enum milFormat = D3DFormatToPixelFormat(surfDesc.Format, TRUE);

    IFC(HrCalcDWordAlignedScanlineStride(surfDesc.Width, milFormat, OUT m_stride));

    IFC(HrGetRequiredBufferSize(
        milFormat,
        m_stride,
        surfDesc.Width,
        surfDesc.Height,
        &m_cbBuffer));

    IFCW32(SelectObject(
        m_hdcCopiedBackBuffer,
        m_hbmpCopiedBackBuffer
        ));
    
Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CD3DSwapChainWithSwDC::GetDC
//
//  Synopsis:
//      Gets a DC that refers to a system memory bitmap.
//
//      The system memory bitmap is updated during this call. The dirty rect is
//      used to determine how much of it needs updating.
//

HRESULT
CD3DSwapChainWithSwDC::GetDC(
    /*__in_range(<, this->m_cBackBuffers)*/ UINT iBackBuffer,
    __in_ecount(1) const CMilRectU& rcDirty,
    __deref_out HDC *phdcBackBuffer
    ) const
{
    HRESULT hr = S_OK;

    ENTER_USE_CONTEXT_FOR_SCOPE(Device());

    Assert(iBackBuffer < m_cBackBuffers);

    D3DSURFACE_DESC const &surfDesc = m_rgBackBuffers[iBackBuffer]->Desc();

    UINT cbBufferInset =
          m_stride * rcDirty.top
        + D3DFormatSize(surfDesc.Format) * rcDirty.left;

    BYTE *pbBuffer = reinterpret_cast<BYTE*>(m_pBuffer) + cbBufferInset;

    IFC(m_rgBackBuffers[iBackBuffer]->ReadIntoSysMemBuffer(
        rcDirty,
        0,
        NULL,
        D3DFormatToPixelFormat(surfDesc.Format, TRUE),
        m_stride,
        DBG_ANALYSIS_PARAM_COMMA(m_cbBuffer - cbBufferInset)
        pbBuffer
        ));

    *phdcBackBuffer = m_hdcCopiedBackBuffer;

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CD3DSwapChainWithSwDC::ReleaseDC
//
//  Synopsis:
//      Releases the DC returned by GetDC if necessary
//

HRESULT
CD3DSwapChainWithSwDC::ReleaseDC(
    /*__in_range(<, this->m_cBackBuffers)*/ UINT iBackBuffer,
    __in HDC hdcBackBuffer
    ) const
{
    //
    // Do nothing- GetDC just hands out the DC so release is not necessary.
    //
    UNREFERENCED_PARAMETER(iBackBuffer);
    UNREFERENCED_PARAMETER(hdcBackBuffer);

    return S_OK;
}





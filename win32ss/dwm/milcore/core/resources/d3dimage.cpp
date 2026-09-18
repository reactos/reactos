// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(D3DImageResource, MILRender, "D3DImage Resource");

MtDefine(CMilD3DImageDuce, D3DImageResource, "CMilD3DImageDuce");

//+------------------------------------------------------------------------
//
//  Member:
//      ctor
//
//-------------------------------------------------------------------------

CMilD3DImageDuce::CMilD3DImageDuce(__in_ecount(1) CComposition* pComposition)
    : m_pInteropDeviceBitmap(NULL), m_pISoftwareBitmap(NULL)
{

}

//+------------------------------------------------------------------------
//
//  Member:
//      dtor
//
//-------------------------------------------------------------------------

CMilD3DImageDuce::~CMilD3DImageDuce()
{
    UnRegisterNotifiers();

    ReleaseInterface(m_pInteropDeviceBitmap);
    ReleaseInterface(m_pISoftwareBitmap);
}

//+------------------------------------------------------------------------
//
//  Member:
//      CMilD3DImageDuce::GetResolution
//
//  Synopsis:
//      Returns the DPI of this image
//
//-------------------------------------------------------------------------

__override
HRESULT
CMilD3DImageDuce::GetResolution(
    __out_ecount(1) double *dDpiX,
    __out_ecount(1) double *dDpiY
    ) const
{
    HRESULT hr = S_OK;

    if (m_pInteropDeviceBitmap)
    {
        IFC(m_pInteropDeviceBitmap->GetResolution(dDpiX, dDpiY));
    }
    else
    {
        *dDpiX = *dDpiY = 96.0;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:
//      CMilD3DImageDuce::GetBitmapSource
//
//  Synopsis:
//      Returns the IWGXBitmapSource that will be drawn to the screen
//
//-------------------------------------------------------------------------

__override
HRESULT
CMilD3DImageDuce::GetBitmapSource(
    __deref_out_ecount_opt(1) IWGXBitmapSource **ppIWGXBitmapSource
    )
{
    IWGXBitmapSource *pIWGXBitmapSource = NULL;

    if (m_pISoftwareBitmap)
    {
        pIWGXBitmapSource = m_pISoftwareBitmap;
    }
    //
    // When the interop bitmap has been disabled, we need to start acting
    // like we don't have a bitmap because otherwise MIL will create
    // CHwBitmapColorSources for a dead bitmap until the user sets a new
    // back buffer. It's not the end of the world, but it's video memory
    // churn.
    //
    else if (m_pInteropDeviceBitmap && !m_pInteropDeviceBitmap->IsHwRenderingDisabled())
    {
        pIWGXBitmapSource = m_pInteropDeviceBitmap;
    }
    // else pIWGXBitmapSource is NULL

    SetInterface(*ppIWGXBitmapSource, pIWGXBitmapSource);

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:
//      CMilD3DImageDuce::GetBounds
//
//  Synopsis:
//      Obtains the bounds of the source image in device-independent
//      content units.
//
//-------------------------------------------------------------------------
__override
HRESULT
CMilD3DImageDuce::GetBounds(
    __in_ecount_opt(1) CContentBounder* pBounder,
    __out_ecount(1) CMilRectF *prcBounds
    )
{
    HRESULT hr = S_OK;

    if (m_pInteropDeviceBitmap)
    {
        IFC(::GetBitmapSourceBounds(m_pInteropDeviceBitmap, prcBounds));
    }
    else
    {
        *prcBounds = CMilRectF::sc_rcEmpty;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:
//      CMilD3DImageDuce::Draw
//
//  Synopsis:
//      Draws the bitmap with the given DC
//
//-------------------------------------------------------------------------
__override
HRESULT
CMilD3DImageDuce::Draw(
    __in_ecount(1) CDrawingContext *pDC,
    MilBitmapWrapMode::Enum wrapMode
    )
{
    RRETURN(pDC->DrawBitmap(this, wrapMode));
}

//+------------------------------------------------------------------------
//
//  Member:
//      CMilD3DImageDuce::ProcessUpdate
//
//  Synopsis:
//      Updates the resource with a new bitmap
//
//-------------------------------------------------------------------------

HRESULT
CMilD3DImageDuce::ProcessUpdate(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_D3DIMAGE* pCmd
    )
{
    HRESULT hr = S_OK;

    // Remove any pre-existing registered resources.
    UnRegisterNotifiers();

    ReleaseInterface(m_pInteropDeviceBitmap);
    ReleaseInterface(m_pISoftwareBitmap);

    // The bitmaps were AddRef'd before being put on the channel, so we'll steal a ref here...
    m_pInteropDeviceBitmap = reinterpret_cast<CInteropDeviceBitmap *>(pCmd->pInteropDeviceBitmap);

    if (pCmd->pSoftwareBitmap)
    {
        IFC(WrapInClosestBitmapInterface(
            reinterpret_cast<IWICBitmapSource *>(pCmd->pSoftwareBitmap),
            &m_pISoftwareBitmap
            ));
    }
    else if (m_pInteropDeviceBitmap != NULL)
    {
        // Fetch the last software copy of the user's surface. This will be NULL unless
        // both the front buffer is unavailable and software fallback has been enabled.
        IFC(m_pInteropDeviceBitmap->GetSoftwareBitmapSource(&m_pISoftwareBitmap));
    }

    // Register the new resources.
    IFC(RegisterNotifiers(pHandleTable));

Cleanup:

    if (FAILED(hr))
    {
        //
        // We have failed to process the update command. Performing unregistration
        // now guarantees that we leave the resource in a predictable state.
        //

        UnRegisterNotifiers();

        // Release the ref that preserved the bitmap across the channel since we failed
        ReleaseInterface(m_pInteropDeviceBitmap);
    }

    // Usually a render update only happens on present, which calls NotifyOnChanged,
    // but we need to be sure to update if null was passed for the bitmap
    NotifyOnChanged(this);

    // Release the ref that preserved the bitmap across the channel
    ReleaseInterfaceNoNULL(reinterpret_cast<IWICBitmapSource *>(pCmd->pSoftwareBitmap));

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:
//      CMilD3DImageDuce::ProcessPresent
//
//  Synopsis:
//      Forwards present to the bitmap and wakes the UI thread back up
//
//-------------------------------------------------------------------------

HRESULT
CMilD3DImageDuce::ProcessPresent(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_D3DIMAGE_PRESENT* pCmd
    )
{
    HRESULT hr = S_OK;
    HANDLE hEvent = UnwrapHandleFromUInt64(pCmd->hEvent);
    IWGXBitmapSource *pISoftwareBitmap;

    // On a synch channel, ProcessPresent will never happen. On the async
    // channel we should never have system memory bitmap unless software
    // fallback has been enabled.
    Assert(!m_pISoftwareBitmap || (m_pInteropDeviceBitmap != NULL &&
        m_pInteropDeviceBitmap->IsSoftwareFallbackEnabled()));

    if (m_pInteropDeviceBitmap)
    {
        IFC(m_pInteropDeviceBitmap->Present());

        // Fetch the last software copy of the user's surface. This will be NULL unless
        // both the front buffer is unavailable and software fallback has been enabled.
        IFC(m_pInteropDeviceBitmap->GetSoftwareBitmapSource(&pISoftwareBitmap));

        // Store it in instance member, getting refcounts right (DDVSO 170719)
        ReleaseInterface(m_pISoftwareBitmap);
        m_pISoftwareBitmap = pISoftwareBitmap;

        NotifyOnChanged(this);
    }

Cleanup:
    // Even if Present failed, we always want to wake up the UI thread
    MIL_TW32(SetEvent(hEvent));
    // The UI thread duplicated the handle to make sure it survived. Close it now.
    MIL_TW32(CloseHandle(hEvent));

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:
//      CMilD3DImageDuce::RegisterNotifiers
//
//  Synopsis:
//      We don't have any child resources, but it's abstract so we must
//      implement it
//
//-------------------------------------------------------------------------

HRESULT
CMilD3DImageDuce::RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable)
{
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:
//      CMilD3DImageDuce::UnRegisterNotifiers
//
//  Synopsis:
//      We don't have any child resources, but it's abstract so we must
//      implement it
//
//-------------------------------------------------------------------------

void
CMilD3DImageDuce::UnRegisterNotifiers()
{
}



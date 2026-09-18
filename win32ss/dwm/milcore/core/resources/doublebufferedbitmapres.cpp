// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//-----------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMilSlaveDoubleBufferedBitmap, MILImaging, "CMilSlaveDoubleBufferedBitmap");

//+-----------------------------------------------------------------------------
//  Constructor / Destructor
//------------------------------------------------------------------------------      

CMilSlaveDoubleBufferedBitmap::CMilSlaveDoubleBufferedBitmap(
    __in CComposition* comp
    ) :
    m_pDoubleBufferedBitmap(NULL)
{
}

CMilSlaveDoubleBufferedBitmap::~CMilSlaveDoubleBufferedBitmap()
{
    ReleaseInterfaceNoNULL(m_pDoubleBufferedBitmap);

    UnRegisterNotifiers();
}

//+-----------------------------------------------------------------------------
//
//  Method
//      Draw
//
//  Synopsis
//      Overridden from CMilImageSource, Draw the DoubleBufferedBitmap resource.
//
//------------------------------------------------------------------------------

HRESULT
CMilSlaveDoubleBufferedBitmap::Draw(
    __in_ecount(1) CDrawingContext *pDC,
    __in MilBitmapWrapMode::Enum wrapMode
    )
{
    HRESULT hr = S_OK;

    IFC(pDC->DrawBitmap(
        this,
        wrapMode
        ));

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method
//      GetBounds
//
//  Synopsis
//      Overridden from CMilImageSource, returns the ImageSource's
//      rectangular bounds.
//
//------------------------------------------------------------------------------

HRESULT
CMilSlaveDoubleBufferedBitmap::GetBounds(
    __in_ecount_opt(1) CContentBounder *pBounder,
    __out_ecount(1) CMilRectF *prcBounds
    )
{
    HRESULT hr = S_OK;
    IWGXBitmap *pFrontBuffer = NULL;

    if (m_pDoubleBufferedBitmap == NULL)
    {
        // We are concerned about callers who won't expect this
        // method to fail.
        *prcBounds = CMilRectF::sc_rcEmpty;
    }
    else
    {
        // The front and back buffers share the same bounds, so we don't have
        // to respect the m_useBackBuffer field.
        m_pDoubleBufferedBitmap->GetFrontBuffer(&pFrontBuffer);
        IFC(GetBitmapSourceBounds(pFrontBuffer, prcBounds));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pFrontBuffer);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method
//      GetResolution
//
//  Synopsis
//      Overridden from CMilImageSource, gets the ImageSource's resolution.
//
//------------------------------------------------------------------------------

HRESULT
CMilSlaveDoubleBufferedBitmap::GetResolution(
    __out_ecount(1) double *dDpiX,
    __out_ecount(1) double *dDpiY
    ) const
{
    HRESULT hr = S_OK;
    IWGXBitmap *pFrontBuffer = NULL;

    if (m_pDoubleBufferedBitmap == NULL)
    {
        IFC(WGXERR_NOTINITIALIZED);
    }

    // The front and back buffers share the same resolution, so we don't have
    // to respect the m_useBackBuffer field.
    m_pDoubleBufferedBitmap->GetFrontBuffer(&pFrontBuffer);
    IFC(pFrontBuffer->GetResolution(dDpiX, dDpiY));

Cleanup:

    ReleaseInterfaceNoNULL(pFrontBuffer);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method
//      GetBitmapSource
//
//  Synopsis
//      Overridden from CMilImageSource, returns the ImageSource's BitmapSource.
//
//------------------------------------------------------------------------------
HRESULT
CMilSlaveDoubleBufferedBitmap::GetBitmapSource(
    __deref_out_ecount_opt(1) IWGXBitmapSource **ppIWGXBitmapSource
    )
{
    if (m_pDoubleBufferedBitmap == NULL)
    {
        // The BitmapSource is an optional output, so we set it to
        // NULL in case we have not been initialized yet.
        *ppIWGXBitmapSource = NULL;
    }
    else
    {
        if (m_useBackBuffer)
        {
            m_pDoubleBufferedBitmap->GetPossiblyFormatConvertedBackBuffer(ppIWGXBitmapSource);
        }
        else
        {
            IWGXBitmap *pIBitmap;
            m_pDoubleBufferedBitmap->GetFrontBuffer(&pIBitmap);
            *ppIWGXBitmapSource = pIBitmap;
        }
    }

    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//  Method
//      ProcessUpdate
//
//  Synopsis:
//      Updates the CSwDoubleBufferedBitmap instance associated with this
//      resource.
//
//------------------------------------------------------------------------------

HRESULT CMilSlaveDoubleBufferedBitmap::ProcessUpdate(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_DOUBLEBUFFEREDBITMAP* pCmd
    )
{
    HRESULT hr = S_OK;

    if (pCmd->SwDoubleBufferedBitmap == 0)
    {
        IFC(E_INVALIDARG);
    }

    // Release the current double-buffered bitmap (if any).  Generally this
    // should not happen.
    ReleaseInterface(m_pDoubleBufferedBitmap);

    // Remove any pre-existing registered resources.
    UnRegisterNotifiers();

    //
    // Hold a reference to the CSwDoubleBufferedBitmap.
    //
    // NOTE: AddRef not needed.
    //
    // The managed code needed to keep pCmd->SwDoubleBufferedBitmap alive while on the
    // channel, so it already AddRef'ed it.  We'll will now take over that reference.
    //
    m_pDoubleBufferedBitmap = reinterpret_cast<CSwDoubleBufferedBitmap *>(pCmd->SwDoubleBufferedBitmap);

    m_useBackBuffer = (pCmd->UseBackBuffer != 0);

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
    }

    NotifyOnChanged(this);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method
//      ProcessCopyForward
//
//  Synopsis
//      ProcessCopyForward copies all dirty regions from the back buffer
//      to the front buffer, then releases the back buffer again for consumption
//      by the UI thread.
//
//------------------------------------------------------------------------------

HRESULT
CMilSlaveDoubleBufferedBitmap::ProcessCopyForward(
    __in_ecount(1) CMilSlaveHandleTable const *pHandleTable,
    __in_ecount(1) const MILCMD_DOUBLEBUFFEREDBITMAP_COPYFORWARD* pCmd
    )
{
    HRESULT hr = S_OK;
    HANDLE hCopyCompletedEvent = UnwrapHandleFromUInt64(pCmd->CopyCompletedEvent);

    // We should never receive a CopyForward command if the resource has
    // not been updated.
    if (m_pDoubleBufferedBitmap == NULL)
    {
        IFC(WGXERR_NOTINITIALIZED);
    }

    // We should never receive a CopyForward command if the resource was
    // told to use the back buffer.
    if (m_useBackBuffer)
    {
        IFC(E_FAIL);
    }

    IFC(m_pDoubleBufferedBitmap->CopyForwardDirtyRects());

    NotifyOnChanged(this);
   
Cleanup: 

    // In all cases, including failure, Signal the UI thread that the copy has
    // completed - otherwise we might block the UI thread.
    MIL_TW32(SetEvent(hCopyCompletedEvent));

    // The UI thread duplicated the handle to make sure it survived. Close it now.
    MIL_TW32(CloseHandle(hCopyCompletedEvent));

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method
//      RegisterNotifiers
//
//  Synopsis
//      Registers change listeners with each child resource.
//
//------------------------------------------------------------------------------

HRESULT
CMilSlaveDoubleBufferedBitmap::RegisterNotifiers(
    CMilSlaveHandleTable *pHandleTable
    )
{
    // We don't have any child resources.
    RRETURN(S_OK);
}

//+-----------------------------------------------------------------------------
//
//  Method
//      UnRegisterNotifiers
//
//  Synopsis
//      Unregisters change listeners from each child resource.
//
//------------------------------------------------------------------------------

void CMilSlaveDoubleBufferedBitmap::UnRegisterNotifiers()
{
    // We don't have any child resources.
}



// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Contents:  Implementation of CMILDeviceContext
//
//  Classes:   CMILDeviceContext
//
//------------------------------------------------------------------------

#include "precomp.hpp"

//+-----------------------------------------------------------------------
//
//  Member:     CMILDeviceContext::CMILDeviceContext
//
//  Synopsis:   Constructor, sets initial state
//
//------------------------------------------------------------------------
CMILDeviceContext::CMILDeviceContext()
{      
    m_hwnd = NULL;
    m_dwRTFlags = 0;

    m_ptWindowOrigin.x = m_ptWindowOrigin.y = 0;
    
    m_dwULWFlags = ULW_OPAQUE;
    m_blendULW.BlendOp = AC_SRC_OVER;
    m_blendULW.BlendFlags = 0;
    m_blendULW.SourceConstantAlpha = 255;
    m_blendULW.AlphaFormat = 0;
    m_colorKey = RGB(0,0,0);
}

//+-----------------------------------------------------------------------
//
//  Member:     CMILDeviceContext::~CMILDeviceContext
//
//  Synopsis:   Destructor, frees DC
//
//------------------------------------------------------------------------
CMILDeviceContext::~CMILDeviceContext() { }

//+-----------------------------------------------------------------------
//
//  Member:     CMILDeviceContext::Init
//
//  Synopsis:   Initializes the object with a HWND & fullscreen
//              flag, which are used to determine which type of
//              device context to use.
//
//------------------------------------------------------------------------
void
CMILDeviceContext::Init(
    __in_opt HWND hwnd,                       // Window handle to render to
    MilRTInitialization::Flags dwRTFlags     // RT initialization flags
    )
{
    // Only windowed rendering is supported
    Assert(hwnd != NULL);

    m_hwnd = hwnd;
    m_dwRTFlags = dwRTFlags;
}

//+-----------------------------------------------------------------------
//
//  Member:     CMILDeviceContext::BeginRendering
//
//  Synopsis:   Obtains a DC to render to
//
//  Returns:    HResult Success/Failure
//
//------------------------------------------------------------------------
HRESULT
CMILDeviceContext::BeginRendering(
    __deref_out HDC *pHDC       // Device Context to use
    ) const
{
    HRESULT hr = S_OK;

    *pHDC = NULL;

    //
    //  Mirror RTs access desktop.
    //  RTs for regular displays will too.  But this means that Presents
    //  will all be duplicated.  One Present to local display (Hw or Sw)
    //  and then another Present to the desktop from the mirror RT.
    //

    // Obtain a DC
    Assert(m_hwnd);
    HDC hdc;
    IFCW32_CHECKOOH(
        GR_GDIOBJECTS,
        hdc = (ShouldRenderFullWindow() ?
                    GetWindowDC(m_hwnd) :
                    GetDC(m_hwnd)));

    // When window has WS_EX_LAYOUTRTL flag in extended style,
    // new created hdc has LAYOUT_RTL flag that causes
    // blit operation to reflect everything from left to right.
    // This is not desired, and is suppressed by following call.
    //
    // Note no need to clear last error as prior IFCW32 handles that.
    if (TW32(0, SetLayout(hdc, 0) != GDI_ERROR))
    {
        // Success - return DC
        *pHDC = hdc;
    }
    else
    {
        //
        // Retrieve last/current error before making any other Win32 calls
        //
        const DWORD dwSetLayoutError = GetLastError();

        //
        // DC is useless so release it - possibily invalid, but there's no
        // harm releasing and invalid DC.  See next comment.
        //

        IGNORE_W32(0, ReleaseDC(m_hwnd, hdc));

        //
        // The HDC can become invalid, causing SetLayout to fail, when the
        // window has been destoyed.  User will release all DCs associated
        // with the window.  So if there was failure check for window
        // validity and return ERROR_INVALID_WINDOW_HANDLE which is better
        // handled by callers.
        //
        if (!IsWindow(m_hwnd))
        {
            IFC(HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE));
        }

        //
        // We don't expect to get here often, but just in case do our
        // regular Win32 error processing.
        //
        hr = HRESULT_FROM_WIN32(dwSetLayoutError);
        if (SUCCEEDED(hr))
        {
            // Return generic Win32 error in case SetLayout didn't specify
            // an error (and window is still valid.)
            hr = THR(WGXERR_WIN32ERROR);
        }
        MILINSTRUMENTATION_CALLHRESULTCHECKFUNCTION(hr);
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:     CMILDeviceContext::EndRendering
//
//  Synopsis:   Releases resources associated with the DC obtained from
//              BeginRendering
//
//------------------------------------------------------------------------
void
CMILDeviceContext::EndRendering(
    __in HDC hdc      // Device context to free
    ) const
{
    //
    // Try to clean up, but don't worry about a failure result.  It is
    // common in our testing that the window will be destroyed while we are
    // busy.  That will cause a failure here.  Furthermore we don't have
    // any alternative to properly clean up when this fails; so just eat
    // any and all errors.
    //
    IGNORE_W32(0, ReleaseDC(m_hwnd, hdc));

    return;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CMILDeviceContext::CreateCompatibleDC
//
//  Synopsis:
//      Create a DC (GDI) compatible to current device context.  Caller must
//      use DeleteDC to cleanup the created DC.
//
//-----------------------------------------------------------------------------
HRESULT
CMILDeviceContext::CreateCompatibleDC(
    __deref_out HDC *pHDC
    ) const
{
    HRESULT hr = S_OK;
    HDC targetDC = NULL;

    IFC(BeginRendering(&targetDC));
    {
        SET_MILINSTRUMENTATION_FLAGS(MILINSTRUMENTATIONFLAGS_BREAKANDCAPTURE | MILINSTRUMENTATIONFLAGS_BREAKINCLUDELIST);
        BEGIN_MILINSTRUMENTATION_HRESULT_LIST
            MILINSTRUMENTATION_DEFAULTOOMHRS
        END_MILINSTRUMENTATION_HRESULT_LIST
        IFCW32_CHECKOOH(GR_GDIOBJECTS, *pHDC = ::CreateCompatibleDC(targetDC));
    }// End Instrumentation

Cleanup:
    if (targetDC)
    {
        EndRendering(targetDC);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:     CMILDeviceContext::SetLayerProperties
//
//  Synopsis:   Updates the parameters we pass to UpdateLayeredWindow.
//
//------------------------------------------------------------------------
void
CMILDeviceContext::SetLayerProperties(
    MilTransparency::Flags transparencyFlags,
    BYTE constantAlpha,
    COLORREF colorKey,
    __in_ecount_opt(1) CDisplay const * const pDisplay
    )
{
    if (transparencyFlags == MilTransparency::Opaque)
    {
        m_dwULWFlags = ULW_OPAQUE;
        
        m_blendULW.SourceConstantAlpha = 255;
        m_blendULW.AlphaFormat = 0;
    }
    else
    {
        m_dwULWFlags =
            ((transparencyFlags & MilTransparency::ColorKey) ? ULW_COLORKEY : 0);

        //
        // Check alpha settings only if bit depth > 8.  At 8bbp or less GDI
        // ignores alpha anyway.
        //
        if (pDisplay == NULL || pDisplay->GetBitsPerPixel() > 8)
        {
            if (transparencyFlags & MilTransparency::ConstantAlpha)
            {
                m_dwULWFlags |= ULW_ALPHA;
            }

            //
            // Check per-pixel alpha settings
            //
            // Use per-pixel alpha if:
            //  1) caller requests per-pixel alpha
            //  2) present format has alpha
            //
            if (   (transparencyFlags & MilTransparency::PerPixelAlpha)
                && (m_dwRTFlags & MilRTInitialization::NeedDestinationAlpha))
            {
                m_dwULWFlags |= ULW_ALPHA;
                
                m_blendULW.AlphaFormat = AC_SRC_ALPHA;
            }
            else
            {
                m_blendULW.AlphaFormat = 0;
            }
        }

        //
        // If no other effect flags are specified then the window must be
        // opaque.  Specify opaque explicitly, because Win32k interprets 0 to
        // mean use previous flags.
        //
        if (m_dwULWFlags == 0)
        {
            m_dwULWFlags = ULW_OPAQUE;
        }

        m_blendULW.SourceConstantAlpha = constantAlpha;
        m_colorKey = colorKey;
    }
}





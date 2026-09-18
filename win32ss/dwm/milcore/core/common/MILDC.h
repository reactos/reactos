// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Contents:  Definition of CMILDeviceContext
//
//  Classes:   CMILDeviceContext
//
//------------------------------------------------------------------------

#pragma once

//+------------------------------------------------------------------
//
// Class:       CMILDeviceContext (MDC)
//
// Synopsis:    Contains logic specific to obtaining & releasing DCs
//              for either windowed or full-screen rendering via GDI.
//  
//              CSwPresenter32bppGDI requires all DC's
//              to have an origin of (0,0), irrespective of the
//              area of the virtual desktop they represent. 
//
//              The logic needed to handle all of these requirments
//              resides within this class.
//
//-------------------------------------------------------------------
class CMILDeviceContext
{
public:

    CMILDeviceContext();
    virtual ~CMILDeviceContext();
    
    void Init(
        __in_opt HWND hwnd,
        MilRTInitialization::Flags dwRTFlags
        );

    HRESULT BeginRendering(
        __deref_out HDC* pHDC
        ) const;

    void EndRendering(
        __in HDC hdc
        ) const;

    HRESULT CreateCompatibleDC(
        __deref_out HDC *pHDC
        ) const;

    HWND GetHWND() const
    {
        return m_hwnd;
    }

    MilRTInitialization::Flags GetRTInitializationFlags() const
    {
        return m_dwRTFlags;
    }

    bool PresentWithHAL() const
    { 
        return (m_dwRTFlags & MilRTInitialization::PresentUsingMask) == MilRTInitialization::PresentUsingHal; 
    }

    void SetPosition(POINT ptOrigin)
    {
        m_ptWindowOrigin = ptOrigin;
    }
    
    void SetLayerProperties(
        MilTransparency::Flags transparencyFlags,
        BYTE constantAlpha,
        COLORREF colorKey,
        __in_ecount_opt(1) CDisplay const * const pDisplay
        );

    const POINT& GetPosition() const {return m_ptWindowOrigin;}

	// ULW_EX_NORESIZE
	// Calling UpdateLayeredWindow can cause the window to resize to match the
	// contents of the bitmap.  This is undesirable since the render thread is
	// separate from the UI thread, and this can cause the window to be sized
	// incorrectly.  Worse, this can cause messages to be raised, including
	// WinEvents for accessibility, which are known to deadlock with the UI
	// thread in certain circumstances.  UpdateLayeredWindowIndirect, which is
	// available Vista+, accepts a ULW_EX_NORESIZE flag to avoid this problem.
	// Since we no longer support < Win7, we can now use this flag.
    DWORD GetULWFlags() const { return m_dwULWFlags | ULW_EX_NORESIZE; }

	const BLENDFUNCTION& GetBlendFunction() const { return m_blendULW; }
    DWORD GetColorKey() const { return m_colorKey; }

private:

    // Use GetWindowDC if rendering the full window
    MIL_FORCEINLINE BOOL ShouldRenderFullWindow() const
    { 
        return m_dwRTFlags & MilRTInitialization::RenderNonClient; 
    }

    HWND m_hwnd;
    MilRTInitialization::Flags m_dwRTFlags;

    POINT m_ptWindowOrigin;
    
    DWORD m_dwULWFlags;
    BLENDFUNCTION m_blendULW;
    COLORREF m_colorKey;

};



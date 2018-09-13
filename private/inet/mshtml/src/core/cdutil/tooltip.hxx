//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       tooltip.hxx
//
//  Contents:   Class definition for tooltip
//
//  Classes:    CTooltip
//
//
//----------------------------------------------------------------------------

#ifndef I_TOOLTIP_HXX_
#define I_TOOLTIP_HXX_
#pragma INCMSG("--- Beg 'tooltip.hxx'")

MtExtern(CTooltip)

//+-------------------------------------------------------------------------
//
//  Class:      CTooltip
//
//  Synopsis:   Tooltip window
//
//--------------------------------------------------------------------------

class CTooltip
{
friend void DeinitTooltip(THREADSTATE *   pts);

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTooltip))

    CTooltip();
    ~CTooltip();

    HRESULT     Init(HWND hwnd);

    void        Show(TCHAR * szText, HWND hwnd, MSG msg,  RECT * prc, DWORD_PTR dwCookie, BOOL fRTL);
    void        Hide();

private:

    HWND        _hwnd;
    HWND        _hwndOwner;
    DWORD_PTR   _dwCookie;
    CStr        _cstrText;

#ifndef UNIX
    TOOLINFO    _ti;
#else
    TOOLINFOA   _ti;
#endif

#ifdef WIN16
    THREAD_HANDLE _dwThreadId;
#endif
};

#pragma INCMSG("--- End 'tooltip.hxx'")
#else
#pragma INCMSG("*** Dup 'tooltip.hxx'")
#endif

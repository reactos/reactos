//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       wndclass.cxx
//
//  Contents:   Window class utilities
//
//----------------------------------------------------------------------------

#include "headers.hxx"

static int  s_catomWndClass;
static ATOM s_aatomWndClass[32];

//+---------------------------------------------------------------------------
//
//  Function:   Register window class
//
//  Synopsis:   Register a window class.
//
//  Arguments:  pstrClass   String to use as part of class name.
//              pfnWndProc  The window procedure.
//              style       Window style flags.
//              pstrBase    Base class name, can be null.
//              ppfnBase    Base class window proc, can be null.
//              patom       Name of registered class.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
RegisterWindowClass(
    TCHAR *   pstrClass,
    LRESULT   (CALLBACK *pfnWndProc)(HWND, UINT, WPARAM, LPARAM),
    UINT      style,
    TCHAR *   pstrBase,
    WNDPROC * ppfnBase,
    ATOM    * patom,
    HICON     hIconSm /* = NULL */ )
{
    TCHAR       achClass[64];
    WNDCLASSEX  wc;

    LOCK_GLOBALS;           // Guard access to globals (the passed atom and the atom array)

    // If another thread registered the class before this one obtained the global
    // lock, return immediately
    if (*patom)
        RRETURN(S_OK);

    Assert(*patom == 0);
    Assert(s_catomWndClass < ARRAY_SIZE(s_aatomWndClass));

    Verify(OK(Format(0,
            achClass,
            ARRAY_SIZE(achClass),
            _T("Internet Explorer_<0s>"),
            pstrClass)));

    if (pstrBase)
    {
        if (!GetClassInfoEx(NULL, pstrBase, &wc))
            goto Error;

        *ppfnBase = wc.lpfnWndProc;
    }
    else
    {
        memset(&wc, 0, sizeof(wc));
//        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    }

    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = (WNDPROC)pfnWndProc;
    wc.lpszClassName = achClass;
    wc.style |= style;
    wc.hInstance = g_hInstCore;
    wc.hIconSm = hIconSm;
#ifdef _MAC
    if (g_fJapanSystem)
    {
        wc.cbWndExtra = sizeof(HIMC);
    } 
#endif

    *patom = RegisterClassEx(&wc);
    if (!*patom)
        goto Error;

#if defined(WIN16) || defined(_MAC)
    *patom = GlobalAddAtom(achClass);
#endif
    s_aatomWndClass[s_catomWndClass++] = *patom;

    return S_OK;

Error:
    RRETURN(GetLastWin32Error());
}

//+---------------------------------------------------------------------------
//
//  Function:   DeinitWindowClasses
//
//  Synopsis:   Unregister any window classes we have registered.
//
//----------------------------------------------------------------------------

void
DeinitWindowClasses()
{
    while (--s_catomWndClass >= 0)
    {
#if !defined(WIN16) && !defined(_MAC)
        Verify(UnregisterClass((TCHAR *)(DWORD_PTR)s_aatomWndClass[s_catomWndClass], g_hInstCore));
#else
        TCHAR szClassName[255];
        Verify(GlobalGetAtomName(s_aatomWndClass[s_catomWndClass], szClassName, sizeof(szClassName)));
        Verify(UnregisterClass(szClassName, g_hInstCore));
        GlobalDeleteAtom(s_aatomWndClass[s_catomWndClass]);
#endif
    }
}

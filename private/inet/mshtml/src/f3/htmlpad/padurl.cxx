//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       padurl.cxx
//
//  Contents:   Ask the user for a URL.
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

static INT_PTR CALLBACK
GetURLProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PADTHREADSTATE * pts = GetThreadState();

    switch (msg)
    {
    case WM_INITDIALOG:
        if (pts->achURL[0] == 0)
        {
#if DBG==1
            _tcscpy(pts->achURL, TEXT("http://trident"));
#else
            _tcscpy(pts->achURL, TEXT("http://www.microsoft.com"));
#endif
        }
        SetDlgItemText(hwnd, IDI_ADDRESS, pts->achURL);
        return TRUE;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDOK:
                GetDlgItemText(hwnd, IDI_ADDRESS, pts->achURL, ARRAY_SIZE(pts->achURL));
                // Fall through.
            case IDCANCEL:
                EndDialog(hwnd, GET_WM_COMMAND_ID(wParam, lParam));
                return TRUE;
        }
        // Fall through

    default:
        return FALSE;
    }
}

// el cheapo file path recognition logic copied from shdocvw
BOOL PathIsFilePath(TCHAR *pchPath)
{
    if (pchPath[0]==_T('\\') || pchPath[0] && pchPath[1]==_T(':'))
        return TRUE;

    if (_tcsnipre(_T("file:"), 5, pchPath, -1))
        return TRUE;

    return FALSE;
}

BOOL
GetURL(HWND hwnd, TCHAR *pchURL, int cch)
{
    if (DialogBoxParam(
        g_hInstResource,
        MAKEINTRESOURCE(IDR_OPEN_URL),
        hwnd,
        &GetURLProc,
        NULL) == IDOK)
    {
        PADTHREADSTATE * pts = GetThreadState();
        ULONG cchUlong = cch;

        if (!PathIsFilePath(pts->achURL))
            return InternetCanonicalizeUrl(pts->achURL, pchURL, &cchUlong, 0);

        if (_tcsnipre(_T("file:"), 5, pts->achURL, -1))
        {
            TCHAR *pch = pts->achURL+5;

            if (_tcsnipre(_T("//"), 2, pch, -1))
                pch += 2;

            _tcsncpy(pchURL, pch, cch);
        }
        else
            _tcsncpy(pchURL, pts->achURL, cch);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



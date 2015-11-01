/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Clipboard Viewer
 * FILE:            base/applications/clipbrd/cliputils.c
 * PURPOSE:         Clipboard helper functions.
 * PROGRAMMERS:     Ricardo Hanke
 */

#include "precomp.h"

int GetPredefinedClipboardFormatName(HINSTANCE hInstance, UINT uFormat, LPWSTR lpszFormat, UINT cch)
{
    switch (uFormat)
    {
        case CF_TEXT:
        {
            return LoadStringW(hInstance, STRING_CF_TEXT, lpszFormat, cch);
        }

        case CF_BITMAP:
        {
            return LoadStringW(hInstance, STRING_CF_BITMAP, lpszFormat, cch);
        }

        case CF_OEMTEXT:
        {
            return LoadStringW(hInstance, STRING_CF_OEMTEXT, lpszFormat, cch);
        }

        case CF_UNICODETEXT:
        {
            return LoadStringW(hInstance, STRING_CF_UNICODETEXT, lpszFormat, cch);
        }

        case CF_DIB:
        {
            return LoadStringW(hInstance, STRING_CF_DIB, lpszFormat, cch);
        }

        case CF_LOCALE:
        {
            return LoadStringW(hInstance, STRING_CF_LOCALE, lpszFormat, cch);
        }

        case CF_ENHMETAFILE:
        {
            return LoadStringW(hInstance, STRING_CF_ENHMETAFILE, lpszFormat, cch);
        }

        case CF_METAFILEPICT:
        {
            return LoadStringW(hInstance, STRING_CF_METAFILEPICT, lpszFormat, cch);
        }

        case CF_PALETTE:
        {
            return LoadStringW(hInstance, STRING_CF_PALETTE, lpszFormat, cch);
        }

        case CF_DIBV5:
        {
            return LoadStringW(hInstance, STRING_CF_DIBV5, lpszFormat, cch);
        }

        case CF_SYLK:
        {
            return LoadStringW(hInstance, STRING_CF_SYLK, lpszFormat, cch);
        }

        case CF_DIF:
        {
            return LoadStringW(hInstance, STRING_CF_DIF, lpszFormat, cch);
        }

        case CF_HDROP:
        {
            return LoadStringW(hInstance, STRING_CF_HDROP, lpszFormat, cch);
        }

        default:
        {
            return 0;
        }
    }
}

void RetrieveClipboardFormatName(HINSTANCE hInstance, UINT uFormat, LPWSTR lpszFormat, UINT cch)
{
    if (!GetPredefinedClipboardFormatName(hInstance, uFormat, lpszFormat, cch))
    {
        if (!GetClipboardFormatName(uFormat, lpszFormat, cch))
        {
            LoadStringW(hInstance, STRING_CF_UNKNOWN, lpszFormat, cch);
        }
    }
}

void DeleteClipboardContent(void)
{
    if (!OpenClipboard(NULL))
    {
        ShowLastWin32Error(Globals.hMainWnd);
        return;
    }

    if (!EmptyClipboard())
    {
        ShowLastWin32Error(Globals.hMainWnd);
    }

    if (!CloseClipboard())
    {
        ShowLastWin32Error(Globals.hMainWnd);
    }
}

UINT GetAutomaticClipboardFormat(void)
{
    UINT uFormatList[] = {CF_UNICODETEXT, CF_ENHMETAFILE, CF_METAFILEPICT, CF_DIBV5, CF_DIB, CF_BITMAP};

    return GetPriorityClipboardFormat(uFormatList, 6);
}

BOOL IsClipboardFormatSupported(UINT uFormat)
{
    switch (uFormat)
    {
        case CF_UNICODETEXT:
        case CF_BITMAP:
        case CF_ENHMETAFILE:
        case CF_METAFILEPICT:
        case CF_DIB:
        case CF_DIBV5:
        {
            return TRUE;
        }

        default:
        {
            return FALSE;
        }
    }
}

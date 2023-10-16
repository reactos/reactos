/*
 * PROJECT:     ReactOS Clipboard Viewer
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Clipboard helper functions.
 * COPYRIGHT:   Copyright 2015-2018 Ricardo Hanke
 *              Copyright 2015-2018 Hermes Belusca-Maito
 */

#include "precomp.h"

LRESULT
SendClipboardOwnerMessage(
    IN BOOL bUnicode,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    HWND hwndOwner;

    hwndOwner = GetClipboardOwner();
    if (!hwndOwner)
        return GetLastError();

    if (bUnicode)
        return SendMessageW(hwndOwner, uMsg, wParam, lParam);
    else
        return SendMessageA(hwndOwner, uMsg, wParam, lParam);
}

static int
GetPredefinedClipboardFormatName(HINSTANCE hInstance,
                                 UINT uFormat,
                                 BOOL Unicode,
                                 PVOID lpszFormat,
                                 UINT cch)
{
    static
    struct FORMAT_NAME
    {
        UINT uFormat;
        UINT uResID;
    } uFormatList[] = {
    /* Table sorted in increasing order of CF_xxx values, please keep it this way! */
        {CF_TEXT        , STRING_CF_TEXT        },          // 1
        {CF_BITMAP      , STRING_CF_BITMAP      },          // 2
        {CF_METAFILEPICT, STRING_CF_METAFILEPICT},          // 3
        {CF_SYLK        , STRING_CF_SYLK        },          // 4
        {CF_DIF         , STRING_CF_DIF         },          // 5
        {CF_TIFF        , 0/*STRING_CF_TIFF*/        },          // 6
        {CF_OEMTEXT     , STRING_CF_OEMTEXT     },          // 7
        {CF_DIB         , STRING_CF_DIB         },          // 8
        {CF_PALETTE     , STRING_CF_PALETTE     },          // 9
        {CF_PENDATA     , 0/*STRING_CF_PENDATA*/     },          // 10
        {CF_RIFF        , 0/*STRING_CF_RIFF*/        },          // 11
        {CF_WAVE        , 0/*STRING_CF_WAVE*/        },          // 12
        {CF_UNICODETEXT , STRING_CF_UNICODETEXT },          // 13
        {CF_ENHMETAFILE , STRING_CF_ENHMETAFILE },          // 14
#if(WINVER >= 0x0400)
        {CF_HDROP       , STRING_CF_HDROP       },          // 15
        {CF_LOCALE      , STRING_CF_LOCALE      },          // 16
#endif
#if(WINVER >= 0x0500)
        {CF_DIBV5       , STRING_CF_DIBV5       },          // 17
#endif
    };

    switch (uFormat)
    {
        case CF_TEXT: case CF_BITMAP: case CF_METAFILEPICT:
        case CF_SYLK: case CF_DIF: // case CF_TIFF:
        case CF_OEMTEXT: case CF_DIB: case CF_PALETTE:
        // case CF_PENDATA: // case CF_RIFF: // case CF_WAVE:
        case CF_UNICODETEXT: case CF_ENHMETAFILE:
#if(WINVER >= 0x0400)
        case CF_HDROP: case CF_LOCALE:
#endif
#if(WINVER >= 0x0500)
        case CF_DIBV5:
#endif
        {
            if (Unicode)
                return LoadStringW(hInstance, uFormatList[uFormat-1].uResID, (LPWSTR)lpszFormat, cch);
            else
                return LoadStringA(hInstance, uFormatList[uFormat-1].uResID, (LPSTR)lpszFormat, cch);
        }

        default:
        {
            return 0;
        }
    }
}

void
RetrieveClipboardFormatName(HINSTANCE hInstance,
                            UINT uFormat,
                            BOOL Unicode,
                            PVOID lpszFormat,
                            UINT cch)
{
    ZeroMemory(lpszFormat, cch * (Unicode ? sizeof(WCHAR) : sizeof(CHAR)));

    /* Check for predefined clipboard format */
    if (GetPredefinedClipboardFormatName(hInstance, uFormat, Unicode, lpszFormat, cch) != 0)
        return;

    /* Check for owner-display format */
    if (uFormat == CF_OWNERDISPLAY)
    {
        if (SendClipboardOwnerMessage(Unicode, WM_ASKCBFORMATNAME,
                                      (WPARAM)cch, (LPARAM)lpszFormat) != 0)
        {
            if (Unicode)
                LoadStringW(hInstance, STRING_CF_UNKNOWN, (LPWSTR)lpszFormat, cch);
            else
                LoadStringA(hInstance, STRING_CF_UNKNOWN, (LPSTR)lpszFormat, cch);
        }
        return;
    }

    /* Fallback to registered clipboard format */
    if (Unicode)
    {
        if (!GetClipboardFormatNameW(uFormat, (LPWSTR)lpszFormat, cch))
            LoadStringW(hInstance, STRING_CF_UNKNOWN, (LPWSTR)lpszFormat, cch);
    }
    else
    {
        if (!GetClipboardFormatNameA(uFormat, (LPSTR)lpszFormat, cch))
            LoadStringA(hInstance, STRING_CF_UNKNOWN, (LPSTR)lpszFormat, cch);
    }
}

void DeleteClipboardContent(void)
{
    if (!OpenClipboard(Globals.hMainWnd))
    {
        ShowLastWin32Error(Globals.hMainWnd);
        return;
    }

    if (!EmptyClipboard())
    {
        ShowLastWin32Error(Globals.hMainWnd);
    }

    CloseClipboard();
}

UINT GetAutomaticClipboardFormat(void)
{
    static UINT uFormatList[] =
    {
        CF_OWNERDISPLAY,
        CF_UNICODETEXT,
        CF_TEXT,
        CF_OEMTEXT,
        CF_ENHMETAFILE,
        CF_METAFILEPICT,
        CF_DIBV5,
        CF_DIB,
        CF_BITMAP,
        CF_DSPTEXT,
        CF_DSPBITMAP,
        CF_DSPMETAFILEPICT,
        CF_DSPENHMETAFILE,
        CF_PALETTE,
        CF_HDROP
    };

    return GetPriorityClipboardFormat(uFormatList, ARRAYSIZE(uFormatList));
}

BOOL IsClipboardFormatSupported(UINT uFormat)
{
    switch (uFormat)
    {
        case CF_OWNERDISPLAY:
        case CF_UNICODETEXT:
        case CF_TEXT:
        case CF_OEMTEXT:
        case CF_BITMAP:
        case CF_ENHMETAFILE:
        case CF_METAFILEPICT:
        case CF_DIB:
        case CF_DIBV5:
        case CF_HDROP:
            return TRUE;

        default:
            break;
    }

    return (uFormat == Globals.uCFSTR_FILENAMEA ||
            uFormat == Globals.uCFSTR_FILENAMEW ||
            uFormat == Globals.uCF_HTML);
}

BOOL GetClipboardDataDimensions(UINT uFormat, PRECT pRc)
{
    BOOL ret = TRUE;
    SetRectEmpty(pRc);

    if (!OpenClipboard(Globals.hMainWnd))
    {
        return FALSE;
    }

    switch (uFormat)
    {
        case CF_NONE:
            break;

        case CF_DSPBITMAP:
        case CF_BITMAP:
        {
            HBITMAP hBitmap;
            BITMAP bmp;

            hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
            GetObjectW(hBitmap, sizeof(bmp), &bmp);
            SetRect(pRc, 0, 0, bmp.bmWidth, bmp.bmHeight);
            break;
        }

        case CF_ENHMETAFILE:
        case CF_METAFILEPICT:
        case CF_DSPMETAFILEPICT:
        case CF_DSPENHMETAFILE:
        {
            break;
        }

        case CF_DIB:
        case CF_DIBV5:
        {
            HGLOBAL hGlobal;
            LPBITMAPINFOHEADER lpInfoHeader;

            hGlobal = GetClipboardData(uFormat);
            if (!hGlobal)
                break;

            lpInfoHeader = GlobalLock(hGlobal);
            if (!lpInfoHeader)
                break;

            if (lpInfoHeader->biSize == sizeof(BITMAPCOREHEADER))
            {
                LPBITMAPCOREHEADER lpCoreHeader = (LPBITMAPCOREHEADER)lpInfoHeader;
                SetRect(pRc, 0, 0,
                        lpCoreHeader->bcWidth,
                        lpCoreHeader->bcHeight);
            }
            else if ((lpInfoHeader->biSize == sizeof(BITMAPINFOHEADER)) ||
                     (lpInfoHeader->biSize == sizeof(BITMAPV4HEADER))   ||
                     (lpInfoHeader->biSize == sizeof(BITMAPV5HEADER)))
            {
                SetRect(pRc, 0, 0,
                        lpInfoHeader->biWidth,
                        /* NOTE: biHeight < 0 for bottom-up DIBs, or > 0 for top-down DIBs */
                        (lpInfoHeader->biHeight > 0) ?  lpInfoHeader->biHeight
                                                     : -lpInfoHeader->biHeight);
            }
            else
            {
                /* Invalid format */
            }

            GlobalUnlock(hGlobal);
            break;
        }

        default:
            ret = FALSE;
            break;
    }

    CloseClipboard();

    return ret;
}

BOOL IsFormatText(UINT uFormat)
{
    switch (uFormat)
    {
        case CF_DSPTEXT:
        case CF_TEXT:
        case CF_OEMTEXT:
        case CF_UNICODETEXT:
        case CF_HDROP:
            return TRUE;

        default:
            break;
    }

    return (uFormat == Globals.uCFSTR_FILENAMEA ||
            uFormat == Globals.uCFSTR_FILENAMEW ||
            uFormat == Globals.uCF_HTML);
}

BOOL DoTextFromFormat(UINT uFormat, TEXTPROC fnCallback)
{
    HGLOBAL hGlobal;
    SIZE_T cbGlobal;

    if (!IsFormatText(uFormat))
        return FALSE;

    if (!OpenClipboard(Globals.hMainWnd))
        return FALSE;

    hGlobal = GetClipboardData(uFormat);
    cbGlobal = GlobalSize(hGlobal);
    if (uFormat == CF_HDROP)
    {
        HDROP hDrop = (HDROP)hGlobal;
        LPWSTR pszText = NULL;
        UINT iFile, cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
        WCHAR szFile[MAX_PATH + 2];
        for (iFile = 0; iFile < cFiles; ++iFile)
        {
            if (!DragQueryFile(hDrop, iFile, szFile, MAX_PATH))
                break;
            lstrcat(szFile, L"\r\n");
            pszText = AllocStrCat(pszText, szFile);
        }
        fnCallback(pszText, lstrlenW(pszText) * sizeof(WCHAR), ENCODING_WIDE);
        free(pszText);
    }
    else
    {
        LPVOID pvData = GlobalLock(hGlobal);
        if (pvData)
        {
            if (uFormat == CF_UNICODETEXT || uFormat == Globals.uCFSTR_FILENAMEW)
                fnCallback(pvData, cbGlobal, ENCODING_WIDE);
            else if (uFormat == Globals.uCF_HTML)
                fnCallback(pvData, cbGlobal, ENCODING_UTF8);
            else
                fnCallback(pvData, cbGlobal, ENCODING_ANSI);

            GlobalUnlock(hGlobal);
        }
        else if (cbGlobal == 0)
        {
            if (uFormat == CF_UNICODETEXT || uFormat == Globals.uCFSTR_FILENAMEW)
                fnCallback(L"", 0, ENCODING_WIDE);
            else if (uFormat == Globals.uCF_HTML)
                fnCallback("", 0, ENCODING_UTF8);
            else
                fnCallback("", 0, ENCODING_ANSI);
        }
    }

    CloseClipboard();
    return TRUE;
}

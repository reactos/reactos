/*
 * PROJECT:     ReactOS Clipboard Viewer
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Clipboard helper functions.
 * COPYRIGHT:   Copyright 2015-2018 Ricardo Hanke
 *              Copyright 2015-2018 Hermes Belusca-Maito
 *              Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
        {
            return TRUE;
        }

        default:
        {
            if (uFormat == Globals.uCFSTR_FILENAMEA ||
                uFormat == Globals.uCFSTR_FILENAMEW)
            {
                return TRUE;
            }

            return FALSE;
        }
    }
}

LPCWSTR FindNewLineW(LPCWSTR pszText, SIZE_T cch)
{
    SIZE_T ich;
    for (ich = 0; ich < cch; ++ich)
    {
        if (pszText[ich] == UNICODE_NULL || pszText[ich] == L'\n')
            break;
    }

    return &pszText[ich];
}

LPCSTR FindNewLineA(LPCSTR pszText, SIZE_T cch)
{
    SIZE_T ich;
    for (ich = 0; ich < cch; ++ich)
    {
        if (pszText[ich] == ANSI_NULL || pszText[ich] == '\n')
            break;
    }

    return &pszText[ich];
}

SIZE GetTextSizeW(HDC hDC, LPCWSTR pszText, SIZE_T cch)
{
    SIZE textSize = { 0, 0 };
    LPCWSTR pch0 = pszText, pch1;
    size_t ich, cchLine;
    DWORD dwSize;

    for (ich = 0; ich < cch; pch0 = pch1 + 1)
    {
        pch1 = FindNewLineW(pch0, cch - ich);
        cchLine = pch1 - pch0;

        if (cchLine > 0 && pch0[cchLine - 1] == L'\r')
            --cchLine;

        dwSize = GetTabbedTextExtentW(hDC, pch0, cchLine, 0, NULL);
        if (textSize.cx < LOWORD(dwSize))
            textSize.cx = LOWORD(dwSize);
        textSize.cy += HIWORD(dwSize);

        ich += pch1 - pch0 + 1;
    }

    return textSize;
}

SIZE GetTextSizeA(HDC hDC, LPCSTR pszText, SIZE_T cch)
{
    SIZE textSize = { 0, 0 };
    LPCSTR pch0 = pszText, pch1;
    size_t ich, cchLine;
    DWORD dwSize;

    for (ich = 0; ich < cch; pch0 = pch1 + 1)
    {
        pch1 = FindNewLineA(pch0, cch - ich);
        cchLine = pch1 - pch0;

        if (cchLine > 0 && pch0[cchLine - 1] == '\r')
            --cchLine;

        dwSize = GetTabbedTextExtentA(hDC, pch0, cchLine, 0, NULL);
        if (textSize.cx < LOWORD(dwSize))
            textSize.cx = LOWORD(dwSize);
        textSize.cy += HIWORD(dwSize);

        ich += pch1 - pch0 + 1;
    }

    return textSize;
}

BOOL DoText(UINT uFormat, DO_TEXT_PROC fnCallback, HDC hDC, LPRECT lpRect)
{
    LPWSTR pszW = NULL;
    LPSTR pszA;
    BOOL ret = FALSE;
    HGLOBAL hGlobal = GetClipboardData(uFormat);
    SIZE_T cbSize = GlobalSize(hGlobal);

    if (!hGlobal || !cbSize)
        return FALSE;

    switch (uFormat)
    {
        case CF_DSPTEXT:
        case CF_TEXT:
        case CF_OEMTEXT:
ansi_text:
            pszA = (LPSTR)GlobalLock(hGlobal);
            if (pszA)
            {
                ret = fnCallback(uFormat, pszA, (cbSize - 1) / sizeof(CHAR), TRUE, hDC, lpRect);
                GlobalUnlock(hGlobal);
            }
            break;

        case CF_UNICODETEXT:
wide_text:
            pszW = (LPWSTR)GlobalLock(hGlobal);
            if (pszW)
            {
                ret = fnCallback(uFormat, pszW, (cbSize - 1) / sizeof(WCHAR), FALSE, hDC, lpRect);
                GlobalUnlock(hGlobal);
            }
            break;

        case CF_HDROP:
        {
            WCHAR szFile[MAX_PATH + 2];
            UINT iFile, cFiles = DragQueryFileW((HDROP)hGlobal, 0xFFFFFFFF, NULL, 0);
            if (Globals.bTextCached)
            {
                fnCallback(uFormat, Globals.pszTextCache, Globals.cchTextCache, FALSE, hDC, lpRect);
                break;
            }
            for (iFile = 0; iFile < cFiles; ++iFile)
            {
                DragQueryFileW((HDROP)hGlobal, iFile, szFile, _countof(szFile) - 2);
                wcscat(szFile, L"\r\n");
                pszW = AllocStrCat(pszW, szFile);
            }
            if (pszW)
            {
                Globals.cchTextCache = wcslen(pszW);
                ret = fnCallback(uFormat, pszW, Globals.cchTextCache, FALSE, hDC, lpRect);
                free(Globals.pszTextCache);
                Globals.pszTextCache = pszW;
                Globals.bTextCached = TRUE;
            }
            break;
        }

        default:
        {
            if (uFormat == Globals.uCFSTR_FILENAMEA)
                goto ansi_text;
            else if (uFormat == Globals.uCFSTR_FILENAMEW)
                goto wide_text;
            break;
        }
    }

    return ret;
}

static BOOL CALLBACK
GetTextDimensionCallback(UINT uFormat, LPCVOID text, SIZE_T cch, BOOL bAnsi,
                         HDC hDC, LPRECT pRc)
{
    SIZE textSize;
    if (bAnsi)
        textSize = GetTextSizeA(hDC, (LPCSTR)text, cch);
    else
        textSize = GetTextSizeW(hDC, (LPCWSTR)text, cch);
    SetRect(pRc, 0, 0, textSize.cx, textSize.cy);
    return TRUE;
}

BOOL GetTextClipboardDimensions(UINT uFormat, PRECT pRc)
{
    HDC hDC = GetDC(Globals.hMainWnd);
    DoText(uFormat, GetTextDimensionCallback, hDC, pRc);
    ReleaseDC(Globals.hMainWnd, hDC);
    return TRUE;
}

BOOL GetClipboardDataDimensions(UINT uFormat, PRECT pRc)
{
    if (Globals.bExtentCached)
    {
        *pRc = Globals.rcExtentCache;
        return TRUE;
    }

    SetRectEmpty(pRc);

    if (!OpenClipboard(Globals.hMainWnd))
    {
        return FALSE;
    }

    switch (uFormat)
    {
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

        case CF_DSPTEXT:
        case CF_TEXT:
        case CF_OEMTEXT:
        case CF_UNICODETEXT:
        case CF_HDROP:
        {
            GetTextClipboardDimensions(uFormat, pRc);
            break;
        }

        default:
        {
            if (uFormat == Globals.uCFSTR_FILENAMEA ||
                uFormat == Globals.uCFSTR_FILENAMEW)
            {
                GetTextClipboardDimensions(uFormat, pRc);
            }
            break;
        }
    }

    CloseClipboard();

    Globals.rcExtentCache = *pRc;
    Globals.bExtentCached = TRUE;
    return TRUE;
}

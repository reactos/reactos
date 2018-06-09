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
        CF_PALETTE
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
            return FALSE;
        }
    }
}

SIZE_T
GetLineExtentW(
    IN LPCWSTR lpText,
    OUT LPCWSTR* lpNextLine)
{
    LPCWSTR ptr;

    /* Find the next line of text (lpText is NULL-terminated) */
    /* For newlines, focus only on '\n', not on '\r' */
    ptr = wcschr(lpText, L'\n'); // Find the end of this line.
    if (ptr)
    {
        /* We have the end of this line, go to the next line (ignore the endline in the count) */
        *lpNextLine = ptr + 1;
    }
    else
    {
        /* This line was the last one, go pointing to the terminating NULL */
        ptr = lpText + wcslen(lpText);
        *lpNextLine = ptr;
    }

    return (ptr - lpText);
}

SIZE_T
GetLineExtentA(
    IN LPCSTR lpText,
    OUT LPCSTR* lpNextLine)
{
    LPCSTR ptr;

    /* Find the next line of text (lpText is NULL-terminated) */
    /* For newlines, focus only on '\n', not on '\r' */
    ptr = strchr(lpText, '\n'); // Find the end of this line.
    if (ptr)
    {
        /* We have the end of this line, go to the next line (ignore the endline in the count) */
        *lpNextLine = ptr + 1;
    }
    else
    {
        /* This line was the last one, go pointing to the terminating NULL */
        ptr = lpText + strlen(lpText);
        *lpNextLine = ptr;
    }

    return (ptr - lpText);
}

BOOL GetClipboardDataDimensions(UINT uFormat, PRECT pRc)
{
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
        {
            HDC hDC;
            HGLOBAL hGlobal;
            PVOID lpText, ptr;
            DWORD dwSize;
            SIZE txtSize = {0, 0};
            SIZE_T lineSize;

            hGlobal = GetClipboardData(uFormat);
            if (!hGlobal)
                break;

            lpText = GlobalLock(hGlobal);
            if (!lpText)
                break;

            hDC = GetDC(Globals.hMainWnd);

            /* Find the size of the rectangle enclosing the text */
            for (;;)
            {
                if (uFormat == CF_UNICODETEXT)
                {
                    if (*(LPCWSTR)lpText == UNICODE_NULL)
                        break;
                    lineSize = GetLineExtentW(lpText, (LPCWSTR*)&ptr);
                    dwSize = GetTabbedTextExtentW(hDC, lpText, lineSize, 0, NULL);
                }
                else
                {
                    if (*(LPCSTR)lpText == ANSI_NULL)
                        break;
                    lineSize = GetLineExtentA(lpText, (LPCSTR*)&ptr);
                    dwSize = GetTabbedTextExtentA(hDC, lpText, lineSize, 0, NULL);
                }
                txtSize.cx = max(txtSize.cx, LOWORD(dwSize));
                txtSize.cy += HIWORD(dwSize);
                lpText = ptr;
            }

            ReleaseDC(Globals.hMainWnd, hDC);

            GlobalUnlock(hGlobal);

            SetRect(pRc, 0, 0, txtSize.cx, txtSize.cy);
            break;
        }

        default:
        {
            break;
        }
    }

    CloseClipboard();

    return TRUE;
}

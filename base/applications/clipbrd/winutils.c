/*
 * PROJECT:     ReactOS Clipboard Viewer
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Display helper functions.
 * COPYRIGHT:   Copyright 2015-2018 Ricardo Hanke
 *              Copyright 2015-2018 Hermes Belusca-Maito
 */

#include "precomp.h"

void ShowLastWin32Error(HWND hwndParent)
{
    DWORD dwError;
    LPWSTR lpMsgBuf = NULL;

    dwError = GetLastError();
    if (dwError == ERROR_SUCCESS)
        return;

    if (!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        dwError,
                        LANG_USER_DEFAULT,
                        (LPWSTR)&lpMsgBuf,
                        0, NULL))
    {
        return;
    }

    MessageBoxW(hwndParent, lpMsgBuf, NULL, MB_OK | MB_ICONERROR);
    LocalFree(lpMsgBuf);
}

void BringWindowToFront(HWND hWnd)
{
    if (IsIconic(hWnd))
    {
        ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
    }
    else
    {
        SetForegroundWindow(hWnd);
    }
}

int MessageBoxRes(HWND hWnd, HINSTANCE hInstance, UINT uText, UINT uCaption, UINT uType)
{
    MSGBOXPARAMSW mb;

    ZeroMemory(&mb, sizeof(mb));
    mb.cbSize = sizeof(mb);
    mb.hwndOwner = hWnd;
    mb.hInstance = hInstance;
    mb.lpszText = MAKEINTRESOURCEW(uText);
    mb.lpszCaption = MAKEINTRESOURCEW(uCaption);
    mb.dwStyle = uType;
    mb.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

    return MessageBoxIndirectW(&mb);
}

void DrawTextFromResource(HINSTANCE hInstance, UINT uID, HDC hDC, LPRECT lpRect, UINT uFormat)
{
    LPWSTR lpBuffer;
    int nCount;

    nCount = LoadStringW(hInstance, uID, (LPWSTR)&lpBuffer, 0);
    if (nCount)
        DrawTextW(hDC, lpBuffer, nCount, lpRect, uFormat);
}

void DrawTextFromClipboard(UINT uFormat, PAINTSTRUCT ps, SCROLLSTATE state)
{
    POINT ptOrg;
    HGLOBAL hGlobal;
    PVOID lpText, ptr;
    SIZE_T lineSize;
    INT FirstLine, LastLine;

    hGlobal = GetClipboardData(uFormat);
    if (!hGlobal)
        return;

    lpText = GlobalLock(hGlobal);
    if (!lpText)
        return;

    /* Find the first and last line indices to display (Note that CurrentX/Y are in pixels!) */
    FirstLine = max(0, (state.CurrentY + ps.rcPaint.top) / Globals.CharHeight);
    // LastLine = min(LINES - 1, (state.CurrentY + ps.rcPaint.bottom) / Globals.CharHeight);
    // NOTE: Can be less or greater than the actual number of lines in the text.
    LastLine = (state.CurrentY + ps.rcPaint.bottom) / Globals.CharHeight;

    /* Find the first text line to display */
    while (FirstLine > 0)
    {
        if (uFormat == CF_UNICODETEXT)
        {
            if (*(LPCWSTR)lpText == UNICODE_NULL)
                break;
            GetLineExtentW(lpText, (LPCWSTR*)&ptr);
        }
        else
        {
            if (*(LPCSTR)lpText == ANSI_NULL)
                break;
            GetLineExtentA(lpText, (LPCSTR*)&ptr);
        }

        --FirstLine;
        --LastLine;

        lpText = ptr;
    }

    ptOrg.x = ps.rcPaint.left;
    ptOrg.y = /* FirstLine */ max(0, (state.CurrentY + ps.rcPaint.top) / Globals.CharHeight)
                    * Globals.CharHeight - state.CurrentY;

    /* Display each line from the current one up to the last one */
    ++LastLine;
    while (LastLine >= 0)
    {
        if (uFormat == CF_UNICODETEXT)
        {
            if (*(LPCWSTR)lpText == UNICODE_NULL)
                break;
            lineSize = GetLineExtentW(lpText, (LPCWSTR*)&ptr);
            TabbedTextOutW(ps.hdc, /*ptOrg.x*/0 - state.CurrentX, ptOrg.y,
                           lpText, lineSize, 0, NULL,
                           /*ptOrg.x*/0 - state.CurrentX);
        }
        else
        {
            if (*(LPCSTR)lpText == ANSI_NULL)
                break;
            lineSize = GetLineExtentA(lpText, (LPCSTR*)&ptr);
            TabbedTextOutA(ps.hdc, /*ptOrg.x*/0 - state.CurrentX, ptOrg.y,
                           lpText, lineSize, 0, NULL,
                           /*ptOrg.x*/0 - state.CurrentX);
        }

        --LastLine;

        ptOrg.y += Globals.CharHeight;
        lpText = ptr;
    }

    GlobalUnlock(hGlobal);
}

void BitBltFromClipboard(PAINTSTRUCT ps, SCROLLSTATE state, DWORD dwRop)
{
    HDC hdcMem;
    HBITMAP hBitmap;
    BITMAP bmp;
    LONG bmWidth, bmHeight;

    hdcMem = CreateCompatibleDC(ps.hdc);
    if (!hdcMem)
        return;

    hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
    GetObjectW(hBitmap, sizeof(bmp), &bmp);

    SelectObject(hdcMem, hBitmap);

    bmWidth  = min(ps.rcPaint.right  - ps.rcPaint.left, bmp.bmWidth  - ps.rcPaint.left - state.CurrentX);
    bmHeight = min(ps.rcPaint.bottom - ps.rcPaint.top , bmp.bmHeight - ps.rcPaint.top  - state.CurrentY);

    BitBlt(ps.hdc,
           ps.rcPaint.left,
           ps.rcPaint.top,
           bmWidth,
           bmHeight,
           hdcMem,
           ps.rcPaint.left + state.CurrentX,
           ps.rcPaint.top  + state.CurrentY,
           dwRop);

    DeleteDC(hdcMem);
}

void SetDIBitsToDeviceFromClipboard(UINT uFormat, PAINTSTRUCT ps, SCROLLSTATE state, UINT fuColorUse)
{
    HGLOBAL hGlobal;
    LPBITMAPINFOHEADER lpInfoHeader;
    LPBYTE lpBits;
    LONG bmWidth, bmHeight;
    DWORD dwPalSize = 0;

    hGlobal = GetClipboardData(uFormat);
    if (!hGlobal)
        return;

    lpInfoHeader = GlobalLock(hGlobal);
    if (!lpInfoHeader)
        return;

    if (lpInfoHeader->biSize == sizeof(BITMAPCOREHEADER))
    {
        LPBITMAPCOREHEADER lpCoreHeader = (LPBITMAPCOREHEADER)lpInfoHeader;

        dwPalSize = 0;

        if (lpCoreHeader->bcBitCount <= 8)
        {
            dwPalSize = (1 << lpCoreHeader->bcBitCount);

            if (fuColorUse == DIB_RGB_COLORS)
                dwPalSize *= sizeof(RGBTRIPLE);
            else
                dwPalSize *= sizeof(WORD);
        }

        bmWidth  = lpCoreHeader->bcWidth;
        bmHeight = lpCoreHeader->bcHeight;
    }
    else if ((lpInfoHeader->biSize == sizeof(BITMAPINFOHEADER)) ||
             (lpInfoHeader->biSize == sizeof(BITMAPV4HEADER))   ||
             (lpInfoHeader->biSize == sizeof(BITMAPV5HEADER)))
    {
        dwPalSize = lpInfoHeader->biClrUsed;

        if ((dwPalSize == 0) && (lpInfoHeader->biBitCount <= 8))
            dwPalSize = (1 << lpInfoHeader->biBitCount);

        if (fuColorUse == DIB_RGB_COLORS)
            dwPalSize *= sizeof(RGBQUAD);
        else
            dwPalSize *= sizeof(WORD);

        if (/*(lpInfoHeader->biSize == sizeof(BITMAPINFOHEADER)) &&*/
            (lpInfoHeader->biCompression == BI_BITFIELDS))
        {
            dwPalSize += 3 * sizeof(DWORD);
        }

        /*
         * This is a (disabled) hack for Windows, when uFormat == CF_DIB
         * it needs yet another extra 3 DWORDs, in addition to the
         * ones already taken into account in via the compression.
         * This problem doesn't happen when uFormat == CF_DIBV5
         * (in that case, when compression is taken into account,
         * everything is nice).
         *
         * NOTE 1: This fix is only for us, because when one pastes DIBs
         * directly in apps, the bitmap offset problem is still present.
         *
         * NOTE 2: The problem can be seen with Windows' clipbrd.exe if
         * one copies a CF_DIB image in the clipboard. By default Windows'
         * clipbrd.exe works with CF_DIBV5 and CF_BITMAP, so the problem
         * is unseen, and the clipboard doesn't have to convert to CF_DIB.
         *
         * FIXME: investigate!!
         * ANSWER: this is a Windows bug; part of the answer is there:
         * https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/ac7ab3b5-8609-4478-b86a-976dab44c271/bug-clipboard-format-conversions-cfdib-cfdibv5-cfdib (DEAD_LINK)
         * May be related:
         * https://blog.talosintelligence.com/dangerous-clipboard/
         */
#if 0
        if ((lpInfoHeader->biSize == sizeof(BITMAPINFOHEADER)) &&
            (lpInfoHeader->biCompression == BI_BITFIELDS))
        {
            dwPalSize += 3 * sizeof(DWORD);
        }
#endif

        bmWidth  = lpInfoHeader->biWidth;
        /* NOTE: biHeight < 0 for bottom-up DIBs, or > 0 for top-down DIBs */
        bmHeight = lpInfoHeader->biHeight;
    }
    else
    {
        /* Invalid format */
        GlobalUnlock(hGlobal);
        return;
    }

    lpBits = (LPBYTE)lpInfoHeader + lpInfoHeader->biSize + dwPalSize;

    /*
     * The seventh parameter (YSrc) of SetDIBitsToDevice always designates
     * the Y-coordinate of the "lower-left corner" of the image, be the DIB
     * in bottom-up or top-down mode.
     */
    SetDIBitsToDevice(ps.hdc,
                      -state.CurrentX, // ps.rcPaint.left,
                      -state.CurrentY, // ps.rcPaint.top,
                      bmWidth,
                      bmHeight,
                      0, // ps.rcPaint.left + state.CurrentX,
                      0, // -(ps.rcPaint.top  + state.CurrentY),
                      0, // uStartScan,
                      bmHeight,
                      lpBits,
                      (LPBITMAPINFO)lpInfoHeader,
                      fuColorUse);

    GlobalUnlock(hGlobal);
}

void PlayMetaFileFromClipboard(HDC hdc, const RECT *lpRect)
{
    LPMETAFILEPICT mp;
    HGLOBAL hGlobal;

    hGlobal = GetClipboardData(CF_METAFILEPICT);
    if (!hGlobal)
        return;

    mp = (LPMETAFILEPICT)GlobalLock(hGlobal);
    if (!mp)
        return;

    SetMapMode(hdc, mp->mm);
    SetViewportExtEx(hdc, lpRect->right, lpRect->bottom, NULL);
    SetViewportOrgEx(hdc, lpRect->left, lpRect->top, NULL);
    PlayMetaFile(hdc, mp->hMF);
    GlobalUnlock(hGlobal);
}

void PlayEnhMetaFileFromClipboard(HDC hdc, const RECT *lpRect)
{
    HENHMETAFILE hEmf;

    hEmf = GetClipboardData(CF_ENHMETAFILE);
    PlayEnhMetaFile(hdc, hEmf, lpRect);
}

static LPWSTR AllocStrCat(LPWSTR psz, LPCWSTR cat)
{
    INT cch;
    LPWSTR pszNew;

    if (psz == NULL)
        return _wcsdup(cat);

    cch = lstrlenW(psz) + lstrlenW(cat) + 1;
    pszNew = realloc(psz, cch * sizeof(WCHAR));
    if (!pszNew)
        return psz;

    lstrcatW(pszNew, cat);
    return pszNew;
}

void HDropFromClipboard(HDC hdc, const RECT *lpRect)
{
    LPWSTR pszAlloc = NULL;
    WCHAR szFile[MAX_PATH + 2];
    HDROP hDrop = (HDROP)GetClipboardData(CF_HDROP);
    UINT iFile, cFiles = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
    RECT rc = *lpRect;

    FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));

    for (iFile = 0; iFile < cFiles; ++iFile)
    {
        DragQueryFileW(hDrop, iFile, szFile, _countof(szFile));
        lstrcatW(szFile, L"\r\n");
        pszAlloc = AllocStrCat(pszAlloc, szFile);
    }

    DrawTextW(hdc, pszAlloc, -1, &rc,
              DT_LEFT | DT_NOPREFIX | DT_EXTERNALLEADING | DT_WORD_ELLIPSIS);
    free(pszAlloc);
}

BOOL RealizeClipboardPalette(HDC hdc)
{
    BOOL Success;
    HPALETTE hPalette, hOldPalette;

    if (!IsClipboardFormatAvailable(CF_PALETTE))
        return FALSE;

    hPalette = GetClipboardData(CF_PALETTE);
    if (!hPalette)
        return FALSE;

    hOldPalette = SelectPalette(hdc, hPalette, FALSE);
    if (!hOldPalette)
        return FALSE;

    Success = (RealizePalette(hdc) != GDI_ERROR);

    SelectPalette(hdc, hOldPalette, FALSE);

    return Success;
}

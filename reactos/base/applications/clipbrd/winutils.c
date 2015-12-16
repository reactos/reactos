/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Clipboard Viewer
 * FILE:            base/applications/clipbrd/winutils.c
 * PURPOSE:         Miscellaneous helper functions.
 * PROGRAMMERS:     Ricardo Hanke
 */

#include "precomp.h"

void ShowLastWin32Error(HWND hwndParent)
{
    DWORD dwError;
    LPWSTR lpMsgBuf = NULL;

    dwError = GetLastError();

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL, dwError, 0, (LPWSTR)&lpMsgBuf, 0, NULL);
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

int DrawTextFromResource(HINSTANCE hInstance, UINT uID, HDC hDC, LPRECT lpRect, UINT uFormat)
{
    LPWSTR lpBuffer;
    int nCount;

    nCount = LoadStringW(hInstance, uID, (LPWSTR)&lpBuffer, 0);
    if (nCount)
    {
        return DrawTextW(hDC, lpBuffer, nCount, lpRect, uFormat);
    }
    else
    {
        return 0;
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

void DrawTextFromClipboard(HDC hDC, LPRECT lpRect, UINT uFormat)
{
    HGLOBAL hGlobal;
    LPWSTR lpchText;

    hGlobal = GetClipboardData(CF_UNICODETEXT);
    if (!hGlobal)
        return;

    lpchText = GlobalLock(hGlobal);
    if (!lpchText)
        return;

    DrawTextW(hDC, lpchText, -1, lpRect, uFormat);
    GlobalUnlock(hGlobal);
}

void BitBltFromClipboard(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, int nXSrc, int nYSrc, DWORD dwRop)
{
    HDC hdcMem;
    HBITMAP hbm;

    hdcMem = CreateCompatibleDC(hdcDest);
    if (hdcMem)
    {
        hbm = (HBITMAP)GetClipboardData(CF_BITMAP);
        SelectObject(hdcMem, hbm);
        BitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcMem, nXSrc, nYSrc, dwRop);
        DeleteDC(hdcMem);
    }
}

void SetDIBitsToDeviceFromClipboard(UINT uFormat, HDC hdc, int XDest, int YDest, int XSrc, int YSrc, UINT uStartScan, UINT fuColorUse)
{
    LPBITMAPINFOHEADER lpInfoHeader;
    LPBYTE lpBits;
    LONG bmWidth, bmHeight;
    DWORD dwPalSize = 0;
    HGLOBAL hGlobal;

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
         * http://go4answers.webhost4life.com/Help/bug-clipboard-format-conversions-28724.aspx
         * May be related:
         * http://blog.talosintel.com/2015/10/dangerous-clipboard.html
         */
#if 0
        if ((lpInfoHeader->biSize == sizeof(BITMAPINFOHEADER)) &&
            (lpInfoHeader->biCompression == BI_BITFIELDS))
        {
            dwPalSize += 3 * sizeof(DWORD);
        }
#endif

        bmWidth  = lpInfoHeader->biWidth;
        bmHeight = lpInfoHeader->biHeight;
    }
    else
    {
        /* Invalid format */
        GlobalUnlock(hGlobal);
        return;
    }

    lpBits = (LPBYTE)lpInfoHeader + lpInfoHeader->biSize + dwPalSize;

    SetDIBitsToDevice(hdc,
                      XDest, YDest,
                      bmWidth, bmHeight,
                      XSrc, YSrc,
                      uStartScan,
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

UINT RealizeClipboardPalette(HWND hWnd)
{
    HPALETTE hPalette;
    HPALETTE hOldPalette;
    UINT uResult;
    HDC hDevContext;

    if (!OpenClipboard(NULL))
    {
        return GDI_ERROR;
    }

    if (!IsClipboardFormatAvailable(CF_PALETTE))
    {
        CloseClipboard();
        return GDI_ERROR;
    }

    hPalette = GetClipboardData(CF_PALETTE);
    if (!hPalette)
    {
        CloseClipboard();
        return GDI_ERROR;
    }

    hDevContext = GetDC(hWnd);
    if (!hDevContext)
    {
        CloseClipboard();
        return GDI_ERROR;
    }

    hOldPalette = SelectPalette(hDevContext, hPalette, FALSE);
    if (!hOldPalette)
    {
        ReleaseDC(hWnd, hDevContext);
        CloseClipboard();
        return GDI_ERROR;
    }

    uResult = RealizePalette(hDevContext);

    SelectPalette(hDevContext, hOldPalette, FALSE);
    ReleaseDC(hWnd, hDevContext);

    CloseClipboard();

    return uResult;
}

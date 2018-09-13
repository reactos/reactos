/*  DBACKP.CPP
**
**  Copyright (C) Microsoft, 1997, All Rights Reserved.
**
**  window class to display a preview of the screen background,
**  complete with rudimentary palette handling and stretching
**  of bitmaps to fit the preview screen.
**
**  this can be replaced with a static bitmap control only
**  if palettes can also be handled by the control.
**
*/

#include "stdafx.h"
#pragma hdrstop
//#include "resource.h"
//#include "deskhtml.h"
//#include "dbackp.h"
//#include "dback.h"
//#include "deskstat.h"
//#include "dutil.h"

#ifdef POSTSPLIT

#define GWW_INFO        0

#define CXYDESKPATTERN 8

BOOL g_bInfoSet = FALSE;

HBITMAP g_hbmPreview = NULL;    // the bitmap used for previewing

HBITMAP  g_hbmDefault = NULL;   // default bitmap
HBITMAP  g_hbmWall = NULL;      // bitmap image of wallpaper
HBITMAP  g_hbmExternal = NULL;  // external wallpaper (from async html)
HDC      g_hdcWall = NULL;      // memory DC with g_hbmWall selected
HDC      g_hdcMemory = NULL;    // memory DC
HPALETTE g_hpalWall = NULL;     // palette that goes with hbmWall bitmap
HBRUSH   g_hbrBack = NULL;      // brush for the desktop background
IThumbnail *g_pthumb = NULL;    // Html to Bitmap converter
DWORD    g_dwWallpaperID = 0;   // ID to identify which bitmap we received

#define WM_HTML_BITMAP  (WM_USER + 100)

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
HPALETTE PaletteFromDS(HDC hdc)
{
    DWORD adw[257];
    int i,n;

    n = GetDIBColorTable(hdc, 0, 256, (LPRGBQUAD)&adw[1]);
    adw[0] = MAKELONG(0x300, n);

    for (i=1; i<=n; i++)
        adw[i] = RGB(GetBValue(adw[i]),GetGValue(adw[i]),GetRValue(adw[i]));

    if (n == 0)
        return NULL;
    else
        return CreatePalette((LPLOGPALETTE)&adw[0]);
}

/*--------------------------------------------------------------------
** Build the preview bitmap.
**
** both the pattern and the bitmap are drawn each time, but
** if the flags dictate the need, new pattern and bitmap
** globals are built as needed.
**--------------------------------------------------------------------*/
void NEAR PASCAL BuildPreviewBitmap(HWND hwnd, WPARAM flags)
{
    HBRUSH hbr = NULL;
    HBITMAP hbmOld;
    BITMAP bm;
    WORD patbits[CXYDESKPATTERN] = {0, 0, 0, 0, 0, 0, 0, 0};
    int     i;
    HCURSOR hcurOld = NULL;
    int     dxWall;          // size of wallpaper
    int     dyWall;


//    if( flags & BP_REINIT )
    {
        if( g_hbmPreview )
            DeleteObject( g_hbmPreview );

        g_hbmPreview = LoadMonitorBitmap();
    }

    hbmOld = (HBITMAP)SelectObject(g_hdcMemory, g_hbmPreview);

    WCHAR wszBuf[MAX_PATH];
    HBITMAP hbmTemp;
    COLORREF clrOldBk, clrOldText;
    /*
    ** first, fill in the pattern all over the bitmap
    */
//    if (flags & BP_NEWPAT)
    {
        // get rid of old brush if there was one
        if (g_hbrBack)
            DeleteObject(g_hbrBack);

        g_pActiveDesk->GetPattern(wszBuf, ARRAYSIZE(wszBuf), 0);
        if (wszBuf[0] != 0L)
        {
            LPTSTR   pszPatternBuf;
#ifndef UNICODE
            CHAR    szTemp[MAX_PATH];
            SHUnicodeToAnsi(wszBuf, szTemp, ARRAYSIZE(szTemp));
            pszPatternBuf = szTemp;
#else
            pszPatternBuf = wszBuf;
#endif
            PatternToWords(pszPatternBuf, patbits);    
            hbmTemp = CreateBitmap(8, 8, 1, 1, patbits);
            if (hbmTemp)
            {
                g_hbrBack = CreatePatternBrush(hbmTemp);
                DeleteObject(hbmTemp);
            }
        }
        else
        {
            g_hbrBack = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));
        }
        if (!g_hbrBack)
        {
            g_hbrBack = (HBRUSH)GetStockObject(BLACK_BRUSH);
        }
    }

    clrOldText = SetTextColor(g_hdcMemory, GetSysColor(COLOR_BACKGROUND));
    clrOldBk = SetBkColor(g_hdcMemory, GetSysColor(COLOR_WINDOWTEXT));

    hbr = (HBRUSH)SelectObject(g_hdcMemory, g_hbrBack);
    PatBlt(g_hdcMemory, MON_X, MON_Y, MON_DX, MON_DY, PATCOPY);
    SelectObject(g_hdcMemory, hbr);

    SetTextColor(g_hdcMemory, clrOldText);
    SetBkColor(g_hdcMemory, clrOldBk);

    /*
    ** now, position the wallpaper appropriately
    */
//    if (flags & BP_NEWWALL)
    {
//        g_bValidBitmap = TRUE;  // assume the new one is valid
        if (g_hbmWall)
        {
            SelectObject(g_hdcWall, g_hbmDefault);
            DeleteObject(g_hbmWall);
            g_hbmWall = NULL;

            if (g_hpalWall)
            {
                DeleteObject(g_hpalWall);
                g_hpalWall = NULL;
            }
        }

        WCHAR wszWallpaper[INTERNET_MAX_URL_LENGTH];
        LPTSTR pszWallpaper;

        g_pActiveDesk->GetWallpaper(wszWallpaper, ARRAYSIZE(wszWallpaper), 0);
#ifndef UNICODE
        CHAR  szWallpaper[ARRAYSIZE(wszWallpaper)];
        SHUnicodeToAnsi(wszWallpaper, szWallpaper, ARRAYSIZE(szWallpaper));
        pszWallpaper = szWallpaper;
#else
        pszWallpaper = wszWallpaper;
#endif

        g_dwWallpaperID++;

        if (!*pszWallpaper || !lstrcmpi(pszWallpaper, g_szNone))
            goto DonePreview;

        if (flags & BP_EXTERNALWALL)
        {
            g_hbmWall = g_hbmExternal;
            g_hbmExternal = NULL;
        }
        else
        {
            if (IsNormalWallpaper(pszWallpaper))
            {
                g_hbmWall = (HBITMAP)LoadImage(NULL, pszWallpaper, IMAGE_BITMAP, 0, 0,
                    LR_LOADFROMFILE|LR_CREATEDIBSECTION);
            }
            else
            {
                if(IsWallpaperPicture(pszWallpaper))
                {
                    // This is a picture (GIF, JPG etc.,)
                    // We need to generate a small HTML file that has this picture
                    // as the background image.
                    // 
                    // Compute the filename for the Temporary HTML file.
                    //
                    GetTempPath(ARRAYSIZE(wszWallpaper), pszWallpaper);
                    lstrcat(pszWallpaper, PREVIEW_PICTURE_FILENAME);
#ifndef UNICODE
                    SHAnsiToUnicode(szWallpaper, wszWallpaper, ARRAYSIZE(wszWallpaper));
#endif
                    //
                    // Generate the preview picture html file.
                    //
                    g_pActiveDesk->GenerateDesktopItemHtml(wszWallpaper, NULL, 0);
                }

                //
                // Will cause a WM_HTML_BITMAP to get sent to us.
                //
                g_pthumb->GetBitmap(wszWallpaper, g_dwWallpaperID, MON_DX, MON_DY);
            }
        }

        if (!g_hbmWall)
        {
//            g_bValidBitmap = FALSE;  // until we know it's not
            goto DonePreview;
        }

        SelectObject(g_hdcWall, g_hbmWall); // bitmap stays in this DC
        GetObject(g_hbmWall, sizeof(bm), &bm);
        TraceMsg(TF_ALWAYS, "for bitmap %08X we have bpp=%d and planes=%d", g_hbmWall, bm.bmBitsPixel, bm.bmPlanes);

        if (GetDeviceCaps(g_hdcMemory, RASTERCAPS) & RC_PALETTE)
        {
            if (bm.bmBitsPixel * bm.bmPlanes > 8)
                g_hpalWall = CreateHalftonePalette(g_hdcMemory);
            else if (bm.bmBitsPixel * bm.bmPlanes == 8)
                g_hpalWall = PaletteFromDS(g_hdcWall);
            else
                g_hpalWall = NULL;  //!!! assume 1 or 4bpp images dont have palettes
        }
    }

    if (g_hbmWall)
    {
        GetObject(g_hbmWall, sizeof(bm), &bm);

        if(flags & BP_EXTERNALWALL)
        {
            //For external wallpapers, we ask the image extractor to generate
            // bitmaps the size that we want to show (NOT the screen size).
            dxWall = MON_DX;
            dyWall = MON_DY;
        }
        else
        {
            dxWall = MulDiv(bm.bmWidth, MON_DX, GetDeviceCaps(g_hdcMemory, HORZRES));
            dyWall = MulDiv(bm.bmHeight, MON_DY, GetDeviceCaps(g_hdcMemory, VERTRES));
        }

        if (dxWall < 1) dxWall = 1;
        if (dyWall < 1) dyWall = 1;

        if (g_hpalWall)
        {
            SelectPalette(g_hdcMemory, g_hpalWall, TRUE);
            RealizePalette(g_hdcMemory);
        }

        IntersectClipRect(g_hdcMemory, MON_X, MON_Y, MON_X + MON_DX, MON_Y + MON_DY);
        SetStretchBltMode(g_hdcMemory, COLORONCOLOR);

        if (flags & BP_TILE)
        {
            StretchBlt(g_hdcMemory, MON_X, MON_Y, dxWall, dyWall,
                g_hdcWall, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

            for (i = MON_X+dxWall; i < (MON_X + MON_DX); i+= dxWall)
                BitBlt(g_hdcMemory, i, MON_Y, dxWall, dyWall, g_hdcMemory, MON_X, MON_Y, SRCCOPY);

            for (i = MON_Y; i < (MON_Y + MON_DY); i += dyWall)
                BitBlt(g_hdcMemory, MON_X, i, MON_DX, dyWall, g_hdcMemory, MON_X, MON_Y, SRCCOPY);
        }
        else
        {
            //We want to stretch the Bitmap to the preview monitor size ONLY for new platforms.
            if((flags & BP_STRETCH) && (g_bRunOnMemphis || g_bRunOnNT5))
            {
                //Stretch the bitmap to the whole preview monitor.
                dxWall = MON_DX;
                dyWall = MON_DY;
            }
            //Center the bitmap in the preview monitor
            StretchBlt(g_hdcMemory, MON_X + (MON_DX - dxWall)/2, MON_Y + (MON_DY - dyWall)/2,
                    dxWall, dyWall, g_hdcWall, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
        }

        // restore dc
        SelectPalette(g_hdcMemory, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE);
        SelectClipRgn(g_hdcMemory, NULL);
    }

DonePreview:
    SelectObject(g_hdcMemory, hbmOld);

    if (hcurOld)
        SetCursor(hcurOld);
}


BOOL NEAR PASCAL BP_CreateGlobals(HWND hwnd)
{
    HDC hdc;

    hdc = GetDC(NULL);
    g_hdcWall = CreateCompatibleDC(hdc);
    g_hdcMemory = CreateCompatibleDC(hdc);
    ReleaseDC(NULL, hdc);
    g_hbmPreview = LoadMonitorBitmap();

    HBITMAP hbm;
    hbm = CreateBitmap(1, 1, 1, 1, NULL);
    g_hbmDefault = (HBITMAP)SelectObject(g_hdcMemory, hbm);
    SelectObject(g_hdcMemory, g_hbmDefault);
    DeleteObject(hbm);

    HRESULT hr = E_FAIL;

    hr = CoCreateInstance(CLSID_Thumbnail, NULL, CLSCTX_INPROC_SERVER, IID_IThumbnail, (void **)&g_pthumb);
    if(SUCCEEDED(hr))
    {
        g_pthumb->Init(hwnd, WM_HTML_BITMAP);
    }

    
    if (!g_hdcWall || !g_hbmPreview || !SUCCEEDED(hr))
        return FALSE;
    else
        return TRUE;
}

void NEAR PASCAL BP_DestroyGlobals(void)
{
    if (g_hbmPreview)
    {
        DeleteObject(g_hbmPreview);
        g_hbmPreview = NULL;
    }
    if (g_hbmWall)
    {
        SelectObject(g_hdcWall, g_hbmDefault);
        DeleteObject(g_hbmWall);
        g_hbmWall = NULL;
    }
    if (g_hpalWall)
    {
        SelectPalette(g_hdcWall, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE);
        DeleteObject(g_hpalWall);
        g_hpalWall = NULL;
    }
    if (g_hdcWall)
    {
        DeleteDC(g_hdcWall);
        g_hdcWall = NULL;
    }
    if (g_hbrBack)
    {
        DeleteObject(g_hbrBack);
        g_hbrBack = NULL;
    }
    if (g_hdcMemory)
    {
        DeleteDC(g_hdcMemory);
        g_hdcMemory = NULL;
    }
    if (g_hbmDefault)
    {
        DeleteObject(g_hbmDefault);
        g_hbmDefault = NULL;
    }
    if (g_hbmExternal)
    {
        DeleteObject(g_hbmExternal);
        g_hbmExternal = NULL;
    }
    if (g_pthumb)
    {
        g_pthumb->Release();
        g_pthumb = NULL;
    }
}

void InvalidateBackPrevContents(HWND hwnd)
{
    BITMAP bm;
    RECT rc;

    //
    // Only invalidate the "screen" part of the monitor bitmap.
    //
    GetObject(g_hbmPreview, SIZEOF(bm), &bm);
    GetClientRect(hwnd, &rc);
    rc.left = ( rc.right - bm.bmWidth ) / 2 + MON_X;
    rc.top = ( rc.bottom - bm.bmHeight ) / 2 + MON_Y;
    rc.right = rc.left + MON_DX;
    rc.bottom = rc.top + MON_DY;

    InvalidateRect(hwnd, &rc, FALSE);
}

LONG CALLBACK  BackPreviewWndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT     ps;
    BITMAP          bm;
    RECT            rc;
    HBITMAP         hbmOld;
    HPALETTE        hpalOld;

    switch(message)
    {
        case WM_CREATE:
            if (!BP_CreateGlobals(hWnd))
                return -1L;
            break;

        case WM_DESTROY:
            MSG msg;
            BP_DestroyGlobals();
            while (PeekMessage(&msg, hWnd, WM_HTML_BITMAP, WM_HTML_BITMAP, PM_REMOVE))
            {
                if ( msg.lParam )
                    DeleteObject((HBITMAP)msg.lParam);
            }
            break;

        case WM_SETBACKINFO:
            if (g_hbmPreview)
            {
                BuildPreviewBitmap(hWnd, wParam);
                g_bInfoSet = TRUE;

                InvalidateBackPrevContents(hWnd);
            }
            break;

        case WM_HTML_BITMAP:
            // may come through with NULL if the image extraction failed....
            if (wParam == g_dwWallpaperID && lParam)
            {
                g_hbmExternal = (HBITMAP)lParam;
                BuildPreviewBitmap(hWnd, BP_EXTERNALWALL);
                InvalidateBackPrevContents(hWnd);
            }
            else
            {
                // Bitmap for something no longer selected
                DeleteObject((HBITMAP)lParam);
            }
            break;

        case WM_PALETTECHANGED:
            if ((HWND)wParam == hWnd)
                break;
            //fallthru
        case WM_QUERYNEWPALETTE:
            if (g_hpalWall)
                InvalidateRect(hWnd, NULL, FALSE);
            break;

        case WM_PAINT:
            BeginPaint(hWnd,&ps);
            if (g_hbmPreview && g_bInfoSet)
            {
                hbmOld = (HBITMAP)SelectObject(g_hdcMemory, g_hbmPreview);
                if (g_hpalWall)
                {
                    hpalOld = SelectPalette(ps.hdc, g_hpalWall, FALSE);
                    RealizePalette(ps.hdc);
                }

                GetObject(g_hbmPreview, sizeof(bm), &bm);
                GetClientRect(hWnd, &rc);
                rc.left = ( rc.right - bm.bmWidth ) / 2;
                rc.top = ( rc.bottom - bm.bmHeight ) / 2;
                BitBlt(ps.hdc, rc.left, rc.top, bm.bmWidth, bm.bmHeight, g_hdcMemory,
                    0, 0, SRCCOPY);

                if (g_hpalWall)
                {
                    SelectPalette(ps.hdc, hpalOld, TRUE);
                    RealizePalette(ps.hdc);
                }
                SelectObject(g_hdcMemory, hbmOld);
            }
            EndPaint(hWnd,&ps);
            return 0;
    }
    return DefWindowProc(hWnd,message,wParam,lParam);
}

BOOL RegisterBackPreviewClass()
{
    WNDCLASS wc;

    if (!GetClassInfo(HINST_THISDLL, c_szBackgroundPreview2, &wc)) {
        wc.style = 0;
        wc.lpfnWndProc = BackPreviewWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = HINST_THISDLL;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = c_szBackgroundPreview2;

        if (!RegisterClass(&wc))
            return FALSE;
    }

    return TRUE;
}
#endif

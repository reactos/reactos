/*  BACKPREV.C
**
**  Copyright (C) Microsoft, 1993, All Rights Reserved.
**
**  window class to display a preview of the screen background,
**  complete with rudimentary palette handling and stretching
**  of bitmaps to fit the preview screen.
**
**  this can be replaced with a static bitmap control only
**  if palettes can also be handled by the control.
**
*/
#include <windows.h>
#include "desk.h"
#include "deskid.h"

#define GWW_INFO        0

#define CXYDESKPATTERN 8

BOOL g_bInfoSet = FALSE;

HBITMAP g_hbmPreview = NULL;    // the bitmap used for previewing

HBITMAP  g_hbmWall = NULL;      // bitmap image of wallpaper
HDC      g_hdcWall = NULL;      // memory DC with g_hbmWall selected
HPALETTE g_hpalWall = NULL;     // palette that goes with hbmWall bitmap
HBRUSH   g_hbrBack = NULL;      // brush for the desktop background

//extern HPALETTE WINAPI CreateHalftonePalette(HDC hdc);

/*-------------------------------------------------------------
** given a pattern string from an ini file, return the pattern
** in a binary (ie useful) form.
**-------------------------------------------------------------*/
void FAR PASCAL ReadPattern(LPTSTR lpStr, WORD FAR *patbits)
{
  short i, val;

  /* Get eight groups of numbers seprated by non-numeric characters. */
  for (i = 0; i < CXYDESKPATTERN; i++)
    {
      val = 0;
      if (*lpStr != TEXT('\0'))
        {
          /* Skip over any non-numeric characters. */
          while (!(*lpStr >= TEXT('0') && *lpStr <= TEXT('9')))
              lpStr++;

          /* Get the next series of digits. */
          while (*lpStr >= TEXT('0') && *lpStr <= TEXT('9'))
              val = val*10 + *lpStr++ - TEXT('0');
         }

      patbits[i] = val;
    }
  return;
}


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
    HBITMAP hbmTemp;
    HBITMAP hbmOld;
    BITMAP bm;
    TCHAR szBuf[MAX_PATH];
    COLORREF clrOldBk, clrOldText;
    WORD patbits[CXYDESKPATTERN] = {0, 0, 0, 0, 0, 0, 0, 0};
    int     i;
    HCURSOR hcurOld = NULL;
    int     dxWall;          // size of wallpaper
    int     dyWall;


    if( flags & BP_REINIT )
    {
        if( g_hbmPreview )
            DeleteObject( g_hbmPreview );

        g_hbmPreview = LoadMonitorBitmap( TRUE );
    }

    hbmOld = SelectObject(g_hdcMem, g_hbmPreview);

    /*
    ** first, fill in the pattern all over the bitmap
    */
    if (flags & BP_NEWPAT)
    {
        // get rid of old brush if there was one
        if (g_hbrBack)
            DeleteObject(g_hbrBack);

        if (*g_szCurPattern && lstrcmpi(g_szCurPattern, g_szNone))
        {
            if (GetPrivateProfileString(g_szPatterns, g_szCurPattern, g_szNULL,
                                        szBuf, ARRAYSIZE(szBuf), g_szControlIni))
            {
                ReadPattern(szBuf, patbits);    
            }
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
            g_hbrBack = GetStockObject(BLACK_BRUSH);
    }

    clrOldText = SetTextColor(g_hdcMem, GetSysColor(COLOR_BACKGROUND));
    clrOldBk = SetBkColor(g_hdcMem, GetSysColor(COLOR_WINDOWTEXT));

    hbr = SelectObject(g_hdcMem, g_hbrBack);
    PatBlt(g_hdcMem, MON_X, MON_Y, MON_DX, MON_DY, PATCOPY);
    SelectObject(g_hdcMem, hbr);

    SetTextColor(g_hdcMem, clrOldText);
    SetBkColor(g_hdcMem, clrOldBk);

    /*
    ** now, position the wallpaper appropriately
    */
    if (flags & BP_NEWWALL)
    {
        g_bValidBitmap = TRUE;  // assume the new one is valid
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

        if (!*g_szCurWallpaper || !lstrcmpi(g_szCurWallpaper, g_szNone))
            goto DonePreview;

        g_hbmWall = LoadImage(NULL, g_szCurWallpaper, IMAGE_BITMAP, 0, 0,
            LR_LOADFROMFILE|LR_CREATEDIBSECTION);

        if (!g_hbmWall)
        {
            g_bValidBitmap = FALSE;  // until we know it's not
            goto DonePreview;
        }

        SelectObject(g_hdcWall, g_hbmWall); // bitmap stays in this DC
        GetObject(g_hbmWall, sizeof(bm), &bm);

        if (GetDeviceCaps(g_hdcMem, RASTERCAPS) & RC_PALETTE)
        {
            if (bm.bmBitsPixel * bm.bmPlanes > 8)
                g_hpalWall = CreateHalftonePalette(g_hdcMem);
            else if (bm.bmBitsPixel * bm.bmPlanes == 8)
                g_hpalWall = PaletteFromDS(g_hdcWall);
            else
                g_hpalWall = NULL;  //!!! assume 1 or 4bpp images dont have palettes
        }
    }

    if (g_hbmWall)
    {
        GetObject(g_hbmWall, sizeof(bm), &bm);

        dxWall = MulDiv(bm.bmWidth, MON_DX, GetDeviceCaps(g_hdcMem, HORZRES));
        dyWall = MulDiv(bm.bmHeight, MON_DY, GetDeviceCaps(g_hdcMem, VERTRES));

        if (dxWall < 1) dxWall = 1;
        if (dyWall < 1) dyWall = 1;

        if (g_hpalWall)
        {
            SelectPalette(g_hdcMem, g_hpalWall, TRUE);
            RealizePalette(g_hdcMem);
        }

        IntersectClipRect(g_hdcMem, MON_X, MON_Y, MON_X + MON_DX, MON_Y + MON_DY);
        SetStretchBltMode(g_hdcMem, COLORONCOLOR);

        if (flags & BP_TILE)
        {
            StretchBlt(g_hdcMem, MON_X, MON_Y, dxWall, dyWall,
                g_hdcWall, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

            for (i = MON_X+dxWall; i < (MON_X + MON_DX); i+= dxWall)
                BitBlt(g_hdcMem, i, MON_Y, dxWall, dyWall, g_hdcMem, MON_X, MON_Y, SRCCOPY);

            for (i = MON_Y; i < (MON_Y + MON_DY); i += dyWall)
                BitBlt(g_hdcMem, MON_X, i, MON_DX, dyWall, g_hdcMem, MON_X, MON_Y, SRCCOPY);
        }
        else
        {
            StretchBlt(g_hdcMem, MON_X + (MON_DX - dxWall)/2, MON_Y + (MON_DY - dyWall)/2,
                    dxWall, dyWall, g_hdcWall, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
        }

        // restore dc
        SelectPalette(g_hdcMem, GetStockObject(DEFAULT_PALETTE), TRUE);
        SelectClipRgn(g_hdcMem, NULL);
    }

DonePreview:
    SelectObject(g_hdcMem, hbmOld);

    if (hcurOld)
        SetCursor(hcurOld);
}


BOOL NEAR PASCAL BP_CreateGlobals(void)
{
    HDC hdc;

    hdc = GetDC(NULL);
    g_hdcWall = CreateCompatibleDC(hdc);
    ReleaseDC(NULL, hdc);
    g_hbmPreview = LoadMonitorBitmap( TRUE );

    if (!g_hdcWall || !g_hbmPreview)
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
        SelectPalette(g_hdcWall, GetStockObject(DEFAULT_PALETTE), TRUE);
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
}


LONG CALLBACK  BackPreviewWndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
PAINTSTRUCT     ps;
BITMAP          bm;
HBITMAP         hbmOld;
HPALETTE        hpalOld;
RECT            rc;

    switch(message)
    {
        case WM_CREATE:
            if (!BP_CreateGlobals())
                return -1L;
            break;

        case WM_DESTROY:
            BP_DestroyGlobals();
            break;

        case WM_SETBACKINFO:
            if (g_hbmPreview)
            {
                BuildPreviewBitmap(hWnd, wParam);
                g_bInfoSet = TRUE;

                // only invalidate the "screen" part of the monitor bitmap
                GetObject(g_hbmPreview, sizeof(bm), &bm);
                GetClientRect(hWnd, &rc);
                rc.left = ( rc.right - bm.bmWidth ) / 2 + MON_X;
                rc.top = ( rc.bottom - bm.bmHeight ) / 2 + MON_Y;
                rc.right = rc.left + MON_DX;
                rc.bottom = rc.top + MON_DY;
                InvalidateRect(hWnd, &rc, FALSE);
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
                hbmOld = SelectObject(g_hdcMem, g_hbmPreview);
                if (g_hpalWall)
                {
                    hpalOld = SelectPalette(ps.hdc, g_hpalWall, FALSE);
                    RealizePalette(ps.hdc);
                }

                GetObject(g_hbmPreview, sizeof(bm), &bm);
                GetClientRect(hWnd, &rc);
                rc.left = ( rc.right - bm.bmWidth ) / 2;
                rc.top = ( rc.bottom - bm.bmHeight ) / 2;
                BitBlt(ps.hdc, rc.left, rc.top, bm.bmWidth, bm.bmHeight, g_hdcMem,
                    0, 0, SRCCOPY);

                if (g_hpalWall)
                {
                    SelectPalette(ps.hdc, hpalOld, TRUE);
                    RealizePalette(ps.hdc);
                }
                SelectObject(g_hdcMem, hbmOld);
            }
            EndPaint(hWnd,&ps);
            return 0;
    }
    return DefWindowProc(hWnd,message,wParam,lParam);
}

BOOL FAR PASCAL RegisterBackPreviewClass(HINSTANCE hInst)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInst, BACKPREV_CLASS, &wc)) {
        wc.style = 0;
        wc.lpfnWndProc = BackPreviewWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInst;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = BACKPREV_CLASS;

        if (!RegisterClass(&wc))
            return FALSE;
    }

    return TRUE;
}

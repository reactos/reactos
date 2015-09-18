/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Logon User Interface Host
 * FILE:        base/system/logonui/NT5design.c
 * PROGRAMMERS: Ged Murphy (gedmurphy@reactos.org)
 */

#include "logonui.h"


/* GLOBALS ******************************************************************/

#define NT5_TOP_BORDER_HEIGHT       80
#define NT5_BOTTOM_BORDER_HEIGHT    96


/* FUNCTIONS ****************************************************************/

static VOID
NT5_DrawLogoffCaptionText(LPWSTR lpText,
                          HDC hdcMem)
{
    HFONT hFont;
    LOGFONTW LogFont;
    RECT TextRect;
    INT PrevBkMode;

    /* Setup the font we'll use */
    ZeroMemory(&LogFont, sizeof(LOGFONTW));
    LogFont.lfCharSet = DEFAULT_CHARSET;
    LogFont.lfHeight = 22;
    LogFont.lfWeight = 109; // From WinXP disassembly
    wcscpy_s(LogFont.lfFaceName, LF_FACESIZE, L"Arial");

    /* Create it */
    hFont = CreateFontIndirectW(&LogFont);
    if (hFont)
    {
        /* Set the font and font colour */
        SelectObject(hdcMem, hFont);
        SetTextColor(hdcMem, RGB(255, 255, 255));

        /* Create the text rect */
        TextRect.top = (g_pInfo->cy / 2) + 34;
        TextRect.bottom = (g_pInfo->cy / 2) + 34 + (GetDeviceCaps(hdcMem, LOGPIXELSY));
        TextRect.left = g_pInfo->cx / 3;
        TextRect.right = (g_pInfo->cx / 2) + 35 + 137;

        /* Set the background mode to transparent */
        PrevBkMode = SetBkMode(hdcMem, TRANSPARENT);

        /* Draw the text to the mem DC */
        DrawTextW(hdcMem,
                  lpText,
                  -1,
                  &TextRect,
                  DT_NOPREFIX | DT_WORDBREAK | DT_RIGHT); // WinXP disassembly uses 0x812

        /* Set the previous background mode */
        SetBkMode(hdcMem, PrevBkMode);

        /* Delete the font */
        DeleteObject(hFont);
    }
}

static VOID
NT5_DrawLogoffIcon(HDC hdcMem)
{
    HBITMAP hBitmap;
    BITMAP bitmap;
    HDC hTempDC;

    /* Load the XP logo */
    hBitmap = (HBITMAP)LoadImageW(g_pInfo->hInstance,
                                  MAKEINTRESOURCEW(IDB_MAIN_ROS_LOGO),
                                  IMAGE_BITMAP,
                                  0,
                                  0,
                                  LR_DEFAULTCOLOR);
    if (hBitmap)
    {
        /* Get the bitmap dimensions */
        GetObjectW(hBitmap, sizeof(BITMAP), &bitmap);

        /* Create a temp DC for the bitmap */
        hTempDC = CreateCompatibleDC(hdcMem);
        if (hTempDC)
        {
            /* Select the bitmap onto the temp DC */
            SelectObject(hTempDC, hBitmap);

            /* Paint it onto the centre block */
            BitBlt(hdcMem,
                   (g_pInfo->cx / 2) + 35,
                   (g_pInfo->cy / 2) - 72,
                   bitmap.bmWidth,
                   bitmap.bmHeight,
                   hTempDC,
                   0,
                   0,
                   SRCCOPY);

            /* Delete the DC */
            DeleteDC(hTempDC);
        }

        /* Delete the bitmap */
        DeleteObject(hBitmap);
    }
}

VOID
NT5_RefreshLogoffScreenText(LPWSTR lpText,
                            HDC hdcMem)
{
    /* FIXME: clear previous text */

    /* Draw the new text */
    NT5_DrawLogoffCaptionText(lpText, hdcMem);
}

VOID
NT5_CreateLogoffScreen(LPWSTR lpText,
                       HDC hdcMem)
{
    /* Draw the reactos logo */
    NT5_DrawLogoffIcon(hdcMem);

    /* Draw the first text string */
    NT5_DrawLogoffCaptionText(lpText, hdcMem);
}

HDC
NT5_DrawBaseBackground(HDC hdcDesktop)
{
    HBITMAP hBitmap = NULL;
    HDC hdcMem = NULL;
    BOOL bRet = FALSE;


    /* Create an an off screen DC to match the desktop DC */
    hdcMem = CreateCompatibleDC(hdcDesktop);
    if (hdcMem)
    {
        /* Create a bitmap to draw the logoff screen onto */
        hBitmap = CreateCompatibleBitmap(hdcDesktop, g_pInfo->cx, g_pInfo->cy);
        if (hBitmap)
        {
            /* Select it onto our off screen DC*/
            SelectObject(hdcMem, hBitmap);

            /* Draw the centre block */
            {
                HBITMAP hTempBitmap;
                HBRUSH hBrush;
                BITMAP bitmap;
                HDC hTempDC;

                /* Paint the blue centre block */
                hBrush = CreateSolidBrush(RGB(90, 126, 220));
                SelectObject(hdcMem, hBrush);
                PatBlt(hdcMem,
                        0,
                        NT5_TOP_BORDER_HEIGHT,
                        g_pInfo->cx,
                        g_pInfo->cy - NT5_TOP_BORDER_HEIGHT - NT5_BOTTOM_BORDER_HEIGHT,
                        PATCOPY);
                DeleteObject(hBrush);

                /* Load the shine effect */
                hTempBitmap = (HBITMAP)LoadImageW(g_pInfo->hInstance,
                                                    MAKEINTRESOURCEW(IDB_MAIN_PANEL_SHINE),
                                                    IMAGE_BITMAP,
                                                    0,
                                                    0,
                                                    LR_DEFAULTCOLOR);
                if (hTempBitmap)
                {
                    /* Get the bitmap dimensions */
                    GetObjectW(hTempBitmap, sizeof(BITMAP), &bitmap);

                    /* Create a temp DC for the bitmap */
                    hTempDC = CreateCompatibleDC(hdcDesktop);
                    if (hTempDC)
                    {
                        /* Select the bitmap onto the temp DC */
                        SelectObject(hTempDC, hTempBitmap);

                        /* Paint it onto the top left of the centre block */
                        BitBlt(hdcMem,
                                0,
                                NT5_TOP_BORDER_HEIGHT,
                                bitmap.bmWidth,
                                bitmap.bmHeight,
                                hTempDC,
                                0,
                                0,
                                SRCCOPY);

                        /* Delete the DC */
                        DeleteDC(hTempDC);
                    }

                    /* Delete the bitmap */
                    DeleteObject(hTempBitmap);
                }
            }

            /* Draw the top border */
            {
                HBITMAP hTempBitmap;
                HBRUSH hBrush;
                BITMAP bitmap;
                HDC hTempDC;

                /* Create the blue brush and paint the top bar */
                hBrush = CreateSolidBrush(RGB(0, 48, 156));
                SelectObject(hdcMem, hBrush);
                PatBlt(hdcMem, 0, 0, g_pInfo->cx, NT5_TOP_BORDER_HEIGHT, PATCOPY);
                DeleteObject(hBrush);

                /* Load the top divider strip */
                hTempBitmap = (HBITMAP)LoadImageW(g_pInfo->hInstance,
                                                    MAKEINTRESOURCEW(IDB_TOP_DIVIDER_STRIP),
                                                    IMAGE_BITMAP,
                                                    0,
                                                    0,
                                                    LR_DEFAULTCOLOR);
                if (hTempBitmap)
                {
                    /* Get the bitmap dimensions */
                    GetObjectW(hTempBitmap, sizeof(BITMAP), &bitmap);

                    /* Create a temp DC for the bitmap */
                    hTempDC = CreateCompatibleDC(hdcDesktop);
                    if (hTempDC)
                    {
                        /* Select the bitmap onto the temp DC */
                        SelectObject(hTempDC, hTempBitmap);

                        /* Paint the bitmap */
                        StretchBlt(hdcMem,
                                    0,
                                    NT5_TOP_BORDER_HEIGHT - bitmap.bmHeight,
                                    g_pInfo->cx,
                                    NT5_TOP_BORDER_HEIGHT,
                                    hTempDC,
                                    0,
                                    0,
                                    bitmap.bmWidth,
                                    NT5_TOP_BORDER_HEIGHT,
                                    SRCCOPY);

                        /* Delete the DC */
                        DeleteDC(hTempDC);
                    }

                    /* Delete the bitmap */
                    DeleteObject(hTempBitmap);
                }
            }

            /* Draw the bottom border */
            {
                HBITMAP hTempBitmap;
                TRIVERTEX vertex[2];
                GRADIENT_RECT gRect;
                BITMAP bitmap;
                HDC hTempDC;

                /*
                 * We paint the divider strip first as it's 3
                 * pixels high but MS only show 2 of them.
                 */

                /* Load the bottom divider strip */
                hTempBitmap = (HBITMAP)LoadImage(g_pInfo->hInstance,
                                                    MAKEINTRESOURCE(IDB_BOTTOM_DIVIDER_STRIP),
                                                    IMAGE_BITMAP,
                                                    0,
                                                    0,
                                                    LR_DEFAULTCOLOR);
                if (hTempBitmap)
                {
                    /* Get the bitmap dimensions */
                    GetObjectW(hTempBitmap, sizeof(BITMAP), &bitmap);

                    /* Create a temp DC for the bitmap */
                    hTempDC = CreateCompatibleDC(hdcDesktop);
                    if (hTempDC)
                    {
                        /* Select the bitmap onto the temp DC */
                        SelectObject(hTempDC, hTempBitmap);

                        /* Paint the bitmap */
                        StretchBlt(hdcMem,
                                    0,
                                    g_pInfo->cy - NT5_BOTTOM_BORDER_HEIGHT,
                                    g_pInfo->cx,
                                    g_pInfo->cy - NT5_BOTTOM_BORDER_HEIGHT + bitmap.bmHeight,
                                    hTempDC,
                                    0,
                                    0,
                                    bitmap.bmWidth,
                                    g_pInfo->cy - NT5_BOTTOM_BORDER_HEIGHT + bitmap.bmHeight,
                                    SRCCOPY);

                        /* Delete the DC */
                        DeleteDC(hTempDC);
                    }

                    /* Delete the bitmap */
                    DeleteObject(hTempBitmap);
                }

                /* Setup the left hand vertex */
                vertex[0].x     = 0;
                vertex[0].y     = g_pInfo->cy - NT5_BOTTOM_BORDER_HEIGHT + 2; // paint over 1 pixel of the bitmap
                vertex[0].Red   = 0x3900;
                vertex[0].Green = 0x3400;
                vertex[0].Blue  = 0xAE00;
                vertex[0].Alpha = 0x0000;

                /* Setup the right hand vertex */
                vertex[1].x     = g_pInfo->cx;
                vertex[1].y     = g_pInfo->cy;
                vertex[1].Red   = 0x0000;
                vertex[1].Green = 0x3000;
                vertex[1].Blue  = 0x9600;
                vertex[1].Alpha = 0x0000;

                /* Set the vertex structs */
                gRect.UpperLeft  = 0;
                gRect.LowerRight = 1;

                /* Paint the gradient across the bottom */
                GradientFill(hdcMem,
                                vertex,
                                2,
                                &gRect,
                                1,
                                GRADIENT_FILL_RECT_H);
            }

            /* Delete the bitmap */
            DeleteObject(hBitmap);
        }
    }

    return hdcMem;
}

/* EOF */

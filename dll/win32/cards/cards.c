/*
 * ReactOS Cards
 *
 * Copyright (C) 2003  Filip Navara <xnavara@volny.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdarg.h>
#include <windef.h>
#include <wingdi.h>
#include <winuser.h>
#include <winreg.h>
#include <winerror.h>
#include <cardgames.h>
#include <cardshape.h>

#include "cards.h"

HBITMAP g_CardBitmaps[MAX_CARD_BITMAPS];
HINSTANCE g_hModule = 0;
static INT g_CardWidth, g_CardHeight;
static UINT g_InitCount;

/*
 * Redundant function from 16-bit Windows time
 */
BOOL WINAPI WEP(DWORD Unknown)
{
    UNREFERENCED_PARAMETER(Unknown);
    return TRUE;
}

/*
 * Initialize card library and return cards width and height
 */
BOOL WINAPI cdtInit(INT *Width, INT *Height)
{
    DWORD dwIndex, style = 0, type, size = sizeof(style);
    UINT offset = 0;
    HKEY key;
    BITMAP bitmap;
    HDC dc;
    HGDIOBJ old;

    if (!Width || !Height)
        return FALSE;

    /* Keep the deck and its dimensions stable for existing callers. */
    if (!g_InitCount)
    {
        if (RegOpenKeyExW(HKEY_CURRENT_USER, CARDGAMES_REGKEY, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
        {
            if (RegQueryValueExW(key, CARDGAMES_STYLE_VALUE, NULL, &type,
                                (BYTE *)&style, &size) == ERROR_SUCCESS &&
                type == REG_DWORD && size == sizeof(style) && style == CARDGAMES_BAVARIAN)
                offset = BAVARIAN_RESOURCE_OFFSET;
            RegCloseKey(key);
        }

        for (dwIndex = 0; dwIndex < MAX_CARD_BITMAPS; ++dwIndex)
        {
            g_CardBitmaps[dwIndex] = LoadBitmapW(g_hModule, MAKEINTRESOURCEW(offset + dwIndex + 1));
            if (!g_CardBitmaps[dwIndex] ||
                !GetObjectW(g_CardBitmaps[dwIndex], sizeof(bitmap), &bitmap))
                goto failed;
            if (!dwIndex)
            {
                g_CardWidth = bitmap.bmWidth;
                g_CardHeight = bitmap.bmHeight;
            }
            else if (bitmap.bmWidth != g_CardWidth || bitmap.bmHeight != g_CardHeight)
                goto failed;
        }
    }

    if (!g_InitCount)
    {
        dc = CreateCompatibleDC(NULL);
        if (!dc)
            goto failed;
        for (dwIndex = 0; dwIndex < MAX_CARD_BITMAPS; ++dwIndex)
        {
            if (dwIndex == 52 || dwIndex >= 66)
                continue; /* Empty-slot and status symbols are not card faces. */
            old = SelectObject(dc, g_CardBitmaps[dwIndex]);
            FrameCardShape(dc, 0, 0, g_CardWidth, g_CardHeight,
                           CardCornerRadius(g_CardWidth, g_CardHeight), RGB(0, 0, 0));
            SelectObject(dc, old);
        }
        DeleteDC(dc);
    }

    ++g_InitCount;
    *Width = g_CardWidth;
    *Height = g_CardHeight;
    return TRUE;

failed:
    for (dwIndex = 0; dwIndex < MAX_CARD_BITMAPS; ++dwIndex)
    {
        DeleteObject(g_CardBitmaps[dwIndex]);
        g_CardBitmaps[dwIndex] = NULL;
    }
    return FALSE;
}

/* Terminate one client's use of the card library. */
VOID WINAPI cdtTerm(VOID)
{
    DWORD dwIndex;

    if (!g_InitCount || --g_InitCount)
        return;

    for (dwIndex = 0; dwIndex < MAX_CARD_BITMAPS; ++dwIndex)
    {
        DeleteObject(g_CardBitmaps[dwIndex]);
        g_CardBitmaps[dwIndex] = NULL;
    }
}

/*
 * Render card with no stretching
 */
BOOL WINAPI cdtDraw(HDC hdc, INT x, INT y, INT card, INT type, COLORREF color)
{
    return cdtDrawExt(hdc, x, y, g_CardWidth, g_CardHeight, card, type, color);
}

/*
 * internal
 */
static __inline VOID BltCard(HDC hdc, INT x, INT y, INT dx, INT dy, HDC hdcCard, DWORD dwRasterOp, BOOL bStretch)
{
    if (bStretch)
    {
        StretchBlt(hdc, x, y, dx, dy, hdcCard, 0, 0, g_CardWidth, g_CardHeight, dwRasterOp);
    }
    else
    {
        BitBlt(hdc, x, y, dx, dy, hdcCard, 0, 0, dwRasterOp);
/*
 * This is need when using Microsoft images, because they use two-color red/white images for
 * red cards and thus needs fix-up of the edge to black color.
 */
#if 0
        if (ISREDCARD(card))
        {
            PatBlt(hdc, x, y + 2, 1, dy - 4, BLACKNESS);
            PatBlt(hdc, x + dx - 1, y + 2, 1, dy - 4, BLACKNESS);
            PatBlt(hdc, x + 2, y, dx - 4, 1, BLACKNESS);
            PatBlt(hdc, x + 2, y + dy - 1, dx - 4, 1, BLACKNESS);
            SetPixel(hdc, x + 1, y + 1, 0);
            SetPixel(hdc, x + dx - 2, y + 1, 0);
            SetPixel(hdc, x + 1, y + dy - 2, 0);
            SetPixel(hdc, x + dx - 2, y + dy - 2, 0);
        }
#endif
    }
}

/*
 * Render card
 *
 * Parameters:
 *    hdc - Handle of destination device context
 *    x - Position left
 *    y - Position right
 *    dx - Destination width
 *    dy - Destination height
 *    card - Image id (meaning depend on type)
 *    type - One of edt* constants
 *    color - Background color (?)
 */
BOOL WINAPI cdtDrawExt(HDC hdc, INT x, INT y, INT dx, INT dy, INT card, INT type, COLORREF color)
{
    DWORD dwRasterOp = SRCCOPY;
    BOOL bSaveEdges = TRUE;
    BOOL bStretch = FALSE;
    INT saved = 0;

    if (!g_InitCount || dx <= 0 || dy <= 0)
        return FALSE;

    if (type & ectSAVEEDGESMASK)
    {
        type &= ~ectSAVEEDGESMASK;
        bSaveEdges = FALSE;
    }

    if (dx != g_CardWidth || dy != g_CardHeight)
    {
        bStretch = TRUE;
    }

    switch (type)
    {
        case ectINVERTED:
            dwRasterOp = NOTSRCCOPY;
        case ectFACES:
            if (card < 0 || card >= 52)
                return FALSE;
            card = (card % 4) * 13 + (card / 4);
            break;
        case ectBACKS:
            --card;
            break;
        case ectEMPTYNOBG:
            dwRasterOp = SRCAND;
        case ectEMPTY:
            card = 52;
            break;
        case ectERASE:
            break;
        case ectREDX:
            card = 66;
            break;
        case ectGREENO:
            card = 67;
            break;
        default:
            return FALSE;
    }

    if (type != ectERASE && (card < 0 || card >= MAX_CARD_BITMAPS))
        return FALSE;

    /* The high-bit flag requests a rectangular copy (e.g. a bitmap atlas).
     * Normal and stretched drawing both preserve the background at corners. */
    if (bSaveEdges && type != ectERASE)
    {
        saved = ClipCardShape(hdc, x, y, dx, dy, CardCornerRadius(dx, dy));
        if (!saved)
            return FALSE;
    }

    if (type == ectEMPTY || type == ectERASE)
    {
        POINT pPoint;
        HBRUSH hBrush, hOldBrush;

        hBrush = CreateSolidBrush(color);
        GetDCOrgEx(hdc, &pPoint);
        SetBrushOrgEx(hdc, pPoint.x, pPoint.y, 0);
        hOldBrush = SelectObject(hdc, hBrush);
        PatBlt(hdc, x, y, dx, dy, PATCOPY);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hBrush);
    }
    if (type != ectERASE)
    {
        HDC hdcCard;
        COLORREF OldBkColor;

        hdcCard = CreateCompatibleDC(hdc);
        SelectObject(hdcCard, g_CardBitmaps[card]);
        OldBkColor = SetBkColor(hdc, (type == ectFACES) ? 0xFFFFFF : color);
        BltCard(hdc, x, y, dx, dy, hdcCard, dwRasterOp, bStretch);
        SetBkColor(hdc, OldBkColor);
        DeleteDC(hdcCard);
    }

    if (saved)
        RestoreDC(hdc, saved);

    return TRUE;
}


/***********************************************************************
 *             cdtAnimate   (CARDS.@)
 *
 * Animate card background, we don't use it
 */
BOOL WINAPI cdtAnimate(HDC hdc, int cardback, int x, int y, int frame)
{
    UNREFERENCED_PARAMETER(frame);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(cardback);
    UNREFERENCED_PARAMETER(hdc);
    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    if (fdwReason == DLL_PROCESS_ATTACH)
        g_hModule = hinstDLL;

    return TRUE;
}

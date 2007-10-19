/*
 *  ReactOS Cards
 *
 *  Copyright (C) 2003  Filip Navara <xnavara@volny.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windows.h"
#include "cards.h"

HBITMAP g_CardBitmaps[MAX_CARD_BITMAPS];
HINSTANCE g_hModule = 0;

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
    DWORD dwIndex;

    /* Report card width and height to user */
	*Width = CARD_WIDTH;
	*Height = CARD_HEIGHT;

	/* Load images */
	for (dwIndex = 0; dwIndex < MAX_CARD_BITMAPS; ++dwIndex)
		g_CardBitmaps[dwIndex] =
			(HBITMAP)LoadBitmapA(g_hModule, MAKEINTRESOURCEA(dwIndex + 1));

	return TRUE;
}

/*
 * Terminate card library
 */
VOID WINAPI cdtTerm(VOID)
{
    DWORD dwIndex;

    /* Unload images */
	for (dwIndex = 0; dwIndex < MAX_CARD_BITMAPS; dwIndex++)
		DeleteObject(g_CardBitmaps[dwIndex]);
}

/*
 * Render card with no stretching
 */
BOOL WINAPI cdtDraw(HDC hdc, INT x, INT y, INT card, INT type, COLORREF color)
{
	return cdtDrawExt(hdc, x, y, CARD_WIDTH, CARD_HEIGHT, card, type, color);
}

/*
 * internal
 */
static __inline VOID BltCard(HDC hdc, INT x, INT y, INT dx, INT dy, HDC hdcCard, DWORD dwRasterOp, BOOL bStretch)
{
	if (bStretch)
	{
		StretchBlt(hdc, x, y, dx, dy, hdcCard, 0, 0, CARD_WIDTH, CARD_HEIGHT, dwRasterOp);
	} else
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
	HDC hdcCard;
	DWORD dwRasterOp = SRCCOPY, OldBkColor;
	BOOL bSaveEdges = TRUE;
	BOOL bStretch = FALSE;

	if (type & ectSAVEEDGESMASK)
	{
		type &= ~ectSAVEEDGESMASK;
		bSaveEdges = FALSE;
	}

	if (dx != CARD_WIDTH || dy != CARD_HEIGHT)
	{
		bStretch = TRUE;
		bSaveEdges = FALSE;
	}

	switch (type)
	{
		case ectINVERTED:
			dwRasterOp = NOTSRCCOPY;
		case ectFACES:
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

	if (type == ectEMPTY || type == ectERASE)
	{
	    POINT pPoint;
	    HBRUSH hBrush;

	    hBrush = CreateSolidBrush(color);
	   	GetDCOrgEx(hdc, &pPoint);
		SetBrushOrgEx(hdc, pPoint.x, pPoint.y, 0);
		SelectObject(hdc, hBrush);
		PatBlt(hdc, x, y, dx, dy, PATCOPY);
	}
	if (type != ectERASE)
	{
	   	hdcCard = CreateCompatibleDC(hdc);
	   	SelectObject(hdcCard, g_CardBitmaps[card]);
		OldBkColor = SetBkColor(hdc, (type == ectFACES) ? 0xFFFFFF : color);
		if (bSaveEdges)
		{
	   		COLORREF SavedPixels[12];
	   		SavedPixels[0] = GetPixel(hdc, x, y);
   			SavedPixels[1] = GetPixel(hdc, x + 1, y);
	   		SavedPixels[2] = GetPixel(hdc, x, y + 1);
	   		SavedPixels[3] = GetPixel(hdc, x + dx - 1, y);
   			SavedPixels[4] = GetPixel(hdc, x + dx - 2, y);
	   		SavedPixels[5] = GetPixel(hdc, x + dx - 1, y + 1);
	   		SavedPixels[6] = GetPixel(hdc, x, y + dy - 1);
   			SavedPixels[7] = GetPixel(hdc, x + 1, y + dy - 1);
	   		SavedPixels[8] = GetPixel(hdc, x, y + dy - 2);
	   		SavedPixels[9] = GetPixel(hdc, x + dx - 1, y + dy - 1);
   			SavedPixels[10] = GetPixel(hdc, x + dx - 2, y + dy - 1);
   			SavedPixels[11] = GetPixel(hdc, x + dx - 1, y + dy - 2);

   			BltCard(hdc, x, y, dx, dy, hdcCard, dwRasterOp, bStretch);

   			SetPixel(hdc, x, y, SavedPixels[0]);
   			SetPixel(hdc, x + 1, y, SavedPixels[1]);
   			SetPixel(hdc, x, y + 1, SavedPixels[2]);
	   		SetPixel(hdc, x + dx - 1, y, SavedPixels[3]);
   			SetPixel(hdc, x + dx - 2, y, SavedPixels[4]);
   			SetPixel(hdc, x + dx - 1, y + 1, SavedPixels[5]);
	   		SetPixel(hdc, x, y + dy - 1, SavedPixels[6]);
   			SetPixel(hdc, x + 1, y + dy - 1, SavedPixels[7]);
   			SetPixel(hdc, x, y + dy - 2, SavedPixels[8]);
	   		SetPixel(hdc, x + dx - 1, y + dy - 1, SavedPixels[9]);
   			SetPixel(hdc, x + dx - 2, y + dy - 1, SavedPixels[10]);
   			SetPixel(hdc, x + dx - 1, y + dy - 2, SavedPixels[11]);
   		}
   		else
   		{
   			BltCard(hdc, x, y, dx, dy, hdcCard, dwRasterOp, bStretch);
   		}
		SetBkColor(hdc, OldBkColor);
		DeleteDC(hdcCard);
	}

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

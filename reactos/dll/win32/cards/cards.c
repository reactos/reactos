/*
 * Cards dll implementation
 *
 * Copyright (C) 2004 Sami Nopanen
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"

#include "cards.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cards);


void WINAPI cdtTerm(void);


static HINSTANCE hInst;
static int cardWidth;
static int cardHeight;
static HBITMAP cardBitmaps[CARD_MAX + 1];


/***********************************************************************
 * Initializes the cards.dll library. Loads the card bitmaps from the
 * resources, and initializes the card size variables.
 */
BOOL WINAPI cdtInit(int *width, int *height)
{
	BITMAP bm;
	int i;

	TRACE("(%p, %p)\n", width, height);

	for(i = 0; i <= CARD_MAX; i++)
		cardBitmaps[i] = 0;

	for(i = 0; i <= CARD_MAX; i++)
	{
		cardBitmaps[i] = LoadBitmapA(hInst, MAKEINTRESOURCEA(i));
		if(cardBitmaps[i] == 0)
		{
			cdtTerm();
			return FALSE;
		}
	}

	GetObjectA(cardBitmaps[0], sizeof(BITMAP), &bm);
	*width = cardWidth = bm.bmWidth;
	*height = cardHeight = bm.bmHeight;
	return TRUE;
}

static DWORD do_blt(HDC hdc, int x, int y, int dx, int dy, HDC hMemoryDC, DWORD rasterOp )
{
	if((cardWidth == dx) && (cardHeight == dy))
		return BitBlt(hdc, x, y, cardWidth, cardHeight, hMemoryDC, 0, 0, rasterOp);
	return StretchBlt(hdc, x, y, dx, dy, hMemoryDC, 0, 0, cardWidth, cardHeight, rasterOp);
}

/***********************************************************************
 * Draw a card. Unlike cdtDrawCard, this version allows you to stretch
 * card bitmaps to the size you specify (dx, dy). See cdtDraw for info
 * on card, mode and color parameters.
 */
BOOL WINAPI cdtDrawExt(HDC hdc, int x, int y, int dx, int dy, int card, int mode, DWORD color)
{
	HDC hMemoryDC;
	HBITMAP hCardBitmap;
	HGDIOBJ result;
	DWORD rasterOp = SRCCOPY;
	BOOL roundCornersFlag;
	BOOL eraseFlag = FALSE;
	BOOL drawFlag = TRUE;

	TRACE("(%p, %d, %d, %d, %d, %d, %d, %ld)\n", hdc, x, y, dx, dy, card, mode, color);

	roundCornersFlag = !(mode & MODEFLAG_DONT_ROUND_CORNERS) &&
			   (dx == cardWidth) && (dy == cardHeight);
	mode &= ~MODEFLAG_DONT_ROUND_CORNERS;

	if((card < 0) || (card > CARD_MAX))
	{
		FIXME("Unexpected card: %d\n", card);
		return FALSE;
	}

	if((mode < MODE_FACEUP) || (mode > MODE_DECKO))
	{
		FIXME("Unexpected mode: %d\n", mode);
		return FALSE;
	}

	switch(mode)
	{
	case MODE_FACEUP:
		break;
	case MODE_FACEDOWN:
		break;
	case MODE_HILITE:
		rasterOp = NOTSRCCOPY;
		break;
	case MODE_GHOST:
		card = CARD_FREE_MASK;
		eraseFlag = TRUE;
		rasterOp = SRCAND;
		break;
	case MODE_REMOVE:
		eraseFlag = TRUE;
		drawFlag = FALSE;
		break;
	case MODE_INVISIBLEGHOST:
		card = CARD_FREE_MASK;
		rasterOp = SRCAND;
		break;
	case MODE_DECKX:
		card = CARD_BACK_THE_X;
		break;
	case MODE_DECKO:
		card = CARD_BACK_THE_O;
		break;
	}

	hMemoryDC = CreateCompatibleDC(hdc);
	if(hMemoryDC == 0)
		return FALSE;

	if(eraseFlag)
	{
		HBRUSH hBrush;
		RECT rect;
		hBrush = CreateSolidBrush(color);
		rect.left = x;
		rect.top = y;
		rect.right = x + cardWidth - 1;
		rect.bottom = y + cardHeight - 1;
		FillRect(hdc, &rect, hBrush);
	}

	if(drawFlag)
	{
		hCardBitmap = cardBitmaps[card];
		if(hCardBitmap == 0)
			return FALSE;

		result = SelectObject(hMemoryDC, hCardBitmap);
		if((result == 0) || (result == HGDI_ERROR))
		{
			DeleteDC(hMemoryDC);
			return FALSE;
		}

		SetBkColor(hdc, color);

		if(roundCornersFlag)
		{
			COLORREF savedPixels[12];

			savedPixels[0] = GetPixel(hdc, x, y);
			savedPixels[1] = GetPixel(hdc, x + 1, y);
			savedPixels[2] = GetPixel(hdc, x, y + 1);
			savedPixels[3] = GetPixel(hdc, x + dx - 1, y);
			savedPixels[4] = GetPixel(hdc, x + dx - 2, y);
			savedPixels[5] = GetPixel(hdc, x + dx - 1, y + 1);
			savedPixels[6] = GetPixel(hdc, x, y + dy - 1);
			savedPixels[7] = GetPixel(hdc, x + 1, y + dy - 1);
			savedPixels[8] = GetPixel(hdc, x, y + dy - 2);
			savedPixels[9] = GetPixel(hdc, x + dx - 1, y + dy - 1);
			savedPixels[10] = GetPixel(hdc, x + dx - 2, y + dy - 1);
			savedPixels[11] = GetPixel(hdc, x + dx - 1, y + dy - 2);

			do_blt(hdc, x, y, dx, dy, hMemoryDC, rasterOp);

			SetPixel(hdc, x, y, savedPixels[0]);
			SetPixel(hdc, x + 1, y, savedPixels[1]);
			SetPixel(hdc, x, y + 1, savedPixels[2]);
			SetPixel(hdc, x + dx - 1, y, savedPixels[3]);
			SetPixel(hdc, x + dx - 2, y, savedPixels[4]);
			SetPixel(hdc, x + dx - 1, y + 1, savedPixels[5]);
			SetPixel(hdc, x, y + dy - 1, savedPixels[6]);
			SetPixel(hdc, x + 1, y + dy - 1, savedPixels[7]);
			SetPixel(hdc, x, y + dy - 2, savedPixels[8]);
			SetPixel(hdc, x + dx - 1, y + dy - 1, savedPixels[9]);
			SetPixel(hdc, x + dx - 2, y + dy - 1, savedPixels[10]);
			SetPixel(hdc, x + dx - 1, y + dy - 2, savedPixels[11]);
		}
		else
			do_blt(hdc, x, y, dx, dy, hMemoryDC, rasterOp);
	}

	DeleteDC(hMemoryDC);

	return TRUE;
}


/***********************************************************************
 * Draws a card at position x, y in its default size (as returned by
 * cdtInit.
 *
 * Mode controls how the card gets drawn:
 *   MODE_FACEUP                ; draw card facing up
 *   MODE_FACEDOWN              ; draw card facing down
 *   MODE_HILITE                ; draw face up, with NOTSRCCOPY
 *   MODE_GHOST                 ; draw 'ghost' card
 *   MODE_REMOVE                ; draw with background color
 *   MODE_INVISIBLEGHOST        ; draw 'ghost' card, without clearing background
 *   MODE_DECKX                 ; draw X
 *   MODE_DECKO                 ; draw O
 *
 * The card parameter defines the card graphic to be drawn. If we are
 * drawing fronts of cards, card should have a value from 0 through 51
 * to represent the card face. If we are drawing card backs, 53 through
 * 68 represent different card backs.
 *
 * When drawing card faces, two lowest bits represent the card suit
 * (clubs, diamonds, hearts, spades), and the bits above that define the
 * card value (ace, 2, ..., king). That is,
 *   card = face * 4 + suit.
 *
 * Color parameter defines the background color, used when drawing some
 * card backs.
 */
BOOL WINAPI cdtDraw(HDC hdc, int x, int y, int card, int mode, DWORD color)
{
	TRACE("(%p, %d, %d, %d, %d, %ld)\n", hdc, x, y, card, mode, color);

	return cdtDrawExt(hdc, x, y, cardWidth, cardHeight, card, mode, color);
}


/***********************************************************************
 * Animates the card backs, e.g. blinking lights on the robot, the sun
 * donning sunglasses, bats flying across the caste, etc.. Works only
 * for cards of normal size (as drawn with cdtDraw). To draw frames of
 * the card back animation, start with frame = 0, and increment the
 * frame by one, until cdtAnimate returns FALSE (to indicate that we
 * have gone through all frames of animation).
 */
BOOL WINAPI cdtAnimate(HDC hdc, int cardback, int x, int y, int frame)
{
	TRACE("(%p, %d, %d, %d, %d)\n", hdc, cardback, x, y, frame);
	FIXME("Implement me.\n");

	return FALSE;
}


/***********************************************************************
 * Frees resources reserved by cdtInitialize.
 */
void WINAPI cdtTerm()
{
	int i;

	TRACE("()\n");

	for(i = 0; i <= CARD_MAX; i++)
	{
		if(cardBitmaps[i] != 0)
			DeleteObject(cardBitmaps[i]);
		cardBitmaps[i] = 0;
	}
}


/***********************************************************************
 * DllMain.
 */
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        hInst = inst;
        DisableThreadLibraryCalls( inst );
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

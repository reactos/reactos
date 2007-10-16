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

#ifndef _CARDS_H
#define _CARDS_H

/*
 * 52 card faces +
 * 12 card backs +
 * X Sign +
 * O Sign +
 * FreeCard +
 * Joker
 */
#define MAX_CARD_BITMAPS	68

#define ectFACES			0
#define ectBACKS			1
#define ectINVERTED			2
#define ectEMPTY			3
#define ectERASE			4
#define ectEMPTYNOBG		5
#define ectREDX				6
#define ectGREENO			7
#define ectSAVEEDGESMASK	0x80000000

#if defined(CARDSTYLE_DEFAULT)
#	define CARD_WIDTH		72	// The original Microsoft cards are 71px wide, but ours are taken from MacSolitaireX
#	define CARD_HEIGHT		96
#elif defined(CARDSTYLE_BAVARIAN)
#	define CARD_WIDTH		110
#	define CARD_HEIGHT		198
#else
#	error No or unsupported cardstyle defined
#endif

#define ISREDCARD(x)	(x >= 13 && x <= 39)

BOOL WINAPI cdtInit(int *width, int *height);
BOOL WINAPI cdtDraw(HDC hdc, int x, int y, int card, int type, DWORD color);
BOOL WINAPI cdtDrawExt(HDC hdc, int x, int y, int dx, int dy, int card, int suit, DWORD color);
BOOL WINAPI cdtAnimate(HDC hdc, int cardback, int x, int y, int frame);
void WINAPI cdtTerm(void);

#endif /* _CARDS_H */

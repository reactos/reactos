/*	File: D:\WACKER\tdll\termpnt.c (Created: 11-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:33p $
 */
// #define	DEBUGSTR	1

#include <windows.h>
#pragma hdrstop

#include "stdtyp.h"
#include "assert.h"
#include "timers.h"
#include "tchar.h"
#include <emu\emu.h>
#include "mc.h"
#include "term.hh"

static
void TextAttrOut(const HHTERM	  hhTerm,
				 const HDC		  hDC,
				 const int		  x,
				 const int		  y,
				 const ECHAR	  *lpachText,
				 const PSTATTR	  apstAttrs,
				 const int        fForceRight,
				 const int		  nCount,
				 const int		  iRow,
				 const int		  iCol);
static int MapCells(const ECHAR *each, const int nStrLen, const int nCellPos);

void termPaint(const HHTERM hhTerm, const HWND hwnd)
	{
	PAINTSTRUCT ps;
	RECT rc, rci;
	HBRUSH hBrush;
	HFONT hFont;
	int i, j, k, l, m, n;
	int iOffset;
	int iPaintBeg;
	int iPaintEnd;
	int xr, xl;
	ECHAR **fplpstrTxt;
	POINT ptTemp;
	const int iScrlInc = hhTerm->yChar;
	const HDC hdc = BeginPaint(hwnd, &ps);
#ifndef CHAR_NARROW
	int x;
	int fRight;
	ECHAR aechTmpBuf[MAX_EMUCOLS + 1];
#endif

	SetTextAlign(hdc, TA_BOTTOM);
	SelectObject(hdc, hhTerm->hbrushTerminal);
	hFont = (HFONT)SelectObject(hdc, hhTerm->hFont);

	iPaintBeg = max(hhTerm->iVScrlMin,
		hhTerm->iVScrlPos + (ps.rcPaint.top / hhTerm->yChar));

	iPaintEnd = min(hhTerm->iRows + 1, hhTerm->iVScrlPos +
			((min(hhTerm->iTermHite * hhTerm->yChar, ps.rcPaint.bottom) +
				hhTerm->yChar - 1) / hhTerm->yChar));

	rc = ps.rcPaint;

	/* -------------- xl is calculation of left edge. ------------- */

	if (hhTerm->iHScrlPos == 0)
		xl = 0;

	else
		xl = min(0, -(hhTerm->iHScrlPos * hhTerm->xChar)
				- hhTerm->xBezel + hhTerm->xChar);

	/* -------------- xr = right edge of text. ------------- */

	xr = xl + (hhTerm->iCols * hhTerm->xChar) + hhTerm->xIndent
		+ hhTerm->xBezel;

	if (ps.rcPaint.bottom > (i = ((hhTerm->iTermHite
			- (hhTerm->xBezel ? 1 : 0)) * hhTerm->yChar)))
		{
		// Only draw between bottom line and top of the bezel, since the
		// bezel gets drawn below anyway.  If no bezel, fill to the bottom
		// of the terminal window.
		//
		rc.top	  = max(rc.top, i);

		rc.bottom = min(rc.bottom,
					hhTerm->cy - ((hhTerm->iVScrlMax == hhTerm->iVScrlPos) ?
					hhTerm->xBezel : 0));

		rc.left  += (xl == 0 && iPaintEnd > 0 && rc.left == 0) ?
					hhTerm->xBezel : 0;

		rc.right  = min(rc.right, xr + hhTerm->xIndent);

		FillRect(hdc, &rc, (iPaintEnd < 0) ? hhTerm->hbrushBackScrl :
											 hhTerm->hbrushTerminal);
		}

	// Could be space beyond the emulator screen (ie. hi-res monitor)
	// that needs filling with the approproiate color.

	if (ps.rcPaint.right > xr)
		{
		rc.left = xr;

		ptTemp.x = 0;
		ptTemp.y = hhTerm->yBrushOrg;
		ClientToScreen(hwnd, &ptTemp);

		if (iPaintBeg <= 0)
			{
			rc.top = (-hhTerm->iVScrlPos + iPaintBeg) * hhTerm->yChar;

			rc.bottom = (iPaintEnd <= 0) ? ps.rcPaint.bottom :
				(-hhTerm->iVScrlPos + min(1, iPaintEnd)) * hhTerm->yChar;

			rc.right = rc.left + hhTerm->xIndent + hhTerm->xBezel;

			FillRect(hdc, &rc, hhTerm->hbrushBackScrl);
			rc.right = ps.rcPaint.right;

			if (i)
				{
				SetBkColor(hdc, hhTerm->crBackScrl);
				SetBrushOrgEx(hdc, ptTemp.x, ptTemp.y, NULL);
				rc.left += hhTerm->xIndent + hhTerm->xBezel;

				if (hhTerm->xBezel)
					FillRect(hdc, &rc, hhTerm->hbrushBackScrl);

				else
					FillRect(hdc, &rc, hhTerm->hbrushBackHatch);


				rc.left -= hhTerm->xIndent + hhTerm->xBezel;
				}
			}

		if (iPaintEnd >= 0)
			{
			rc.top = (-hhTerm->iVScrlPos + max(1, iPaintBeg)) * hhTerm->yChar
							- hhTerm->yChar/2;

			rc.bottom = ps.rcPaint.bottom;
			rc.right = rc.left + hhTerm->xIndent;

			FillRect(hdc, &rc, hhTerm->hbrushTerminal);
			rc.right = ps.rcPaint.right;

			if (i)
				{
				SetBkColor(hdc, hhTerm->crTerm);
				SetBrushOrgEx(hdc, ptTemp.x, ptTemp.y, 0);
				rc.left += hhTerm->xIndent + hhTerm->xBezel;

				if (hhTerm->xBezel)
					FillRect(hdc, &rc, hhTerm->hbrushBackScrl);

				else
					FillRect(hdc, &rc, hhTerm->hbrushTermHatch);
				}
			}
		}

	// Fill in the indent margin along the left side of the terminal.

	if (ps.rcPaint.left < (hhTerm->xIndent +
			(hhTerm->iHScrlPos ? 0 : hhTerm->xBezel)))
		{
		rc.left = ps.rcPaint.left;
		rc.right = hhTerm->xIndent + (hhTerm->iHScrlPos ? 0 : hhTerm->xBezel);

		// When scrolling down during marking, corners were left unpainted.
		// This guy corrects that. - mrw

		if (iPaintBeg < 0)
			{
			rc.top = (-hhTerm->iVScrlPos + iPaintBeg) * hhTerm->yChar;
			rc.bottom = (-hhTerm->iVScrlPos + min(0,iPaintEnd)) * hhTerm->yChar;
			FillRect(hdc, &rc, hhTerm->hbrushBackScrl);
			}

		if (iPaintEnd > 0)
			{
			// The top & bottom should only redraw from & to the bezel.
			// that is all that's needed...
			//
			rc.top	  = (-hhTerm->iVScrlPos + max(1, iPaintBeg)) *
							hhTerm->yChar;

			rc.bottom = (hhTerm->iVScrlPos == hhTerm->iVScrlMax) ?
							hhTerm->cy - hhTerm->xBezel : ps.rcPaint.bottom;

			rc.left   = max(ps.rcPaint.left,
							((hhTerm->iHScrlPos == 0) ? hhTerm->xBezel : 0));

			FillRect(hdc, &rc, hhTerm->hbrushTerminal);
			}

		// New outdented bezel style requires filling with gray between
		// bezel and left edge.

		if (hhTerm->iHScrlPos == 0 && ps.rcPaint.left <= OUTDENT &&
				iPaintEnd > 0)
			{
			rc.top   	= (-hhTerm->iVScrlPos + max(0, iPaintBeg)) * hhTerm->yChar;
			rc.bottom	= (-hhTerm->iVScrlPos + iPaintEnd + 2) * hhTerm->yChar;
			rc.right 	= OUTDENT;
			rc.left 	= ps.rcPaint.left;

			FillRect(hdc, &rc, hhTerm->hbrushBackScrl);
			}
		}

	/* -------------- Paint bezel here ------------- */

	if (hhTerm->xBezel)
		{
		/* -------------- Left edge ------------- */

		if (iPaintEnd >= 0)
			{
			// n is the width of the thick gray section of the bezel.
			// we surround the thick gray part with four lines of
			// of white and gray lines.  That's why we subtract 4.

			n = hhTerm->xBezel - OUTDENT - 4;

			/* -------------- Left edge ------------- */

			rc.left = xl + OUTDENT;
			rc.right = rc.left + hhTerm->xBezel;

			k = ((hhTerm->iVScrlMax - hhTerm->iVScrlPos + hhTerm->iTermHite)
					* hhTerm->yChar) + (hhTerm->cy % hhTerm->yChar);

			rc.top = (-hhTerm->iVScrlPos * hhTerm->yChar) + OUTDENT;
			rc.bottom = k - OUTDENT;

			if (IntersectRect(&rci, &rc, &ps.rcPaint))
				{
				/* --- Paint outer border --- */

				SelectObject(hdc, hhTerm->hWhitePen);
				MoveToEx(hdc, rc.left, rc.top++, NULL);
				LineTo(hdc, rc.left++, rc.bottom--);

				MoveToEx(hdc, rc.left, rc.top++, NULL);
				LineTo(hdc, rc.left++, rc.bottom--);

                /* --- Paint middle portion --- */

				rc.right = rc.left + n;
	            FillRect(hdc, &rc, hhTerm->hbrushBackScrl);
				rc.left += n;
				rc.top += n;
				rc.bottom -= n;

                /* --- Paint inner border --- */

				SelectObject(hdc, hhTerm->hDkGrayPen);
				MoveToEx(hdc, rc.left, rc.top++, NULL);
				LineTo(hdc, rc.left++, rc.bottom--);

				SelectObject(hdc, hhTerm->hBlackPen);
				MoveToEx(hdc, rc.left, rc.top, NULL);
				LineTo(hdc, rc.left, rc.bottom);
				}

			/* -------------- Bottom edge ------------- */

			rc.left = xl + OUTDENT;
			rc.right = xr + hhTerm->xIndent + hhTerm->xBezel - 1 - OUTDENT;
			l = rc.right; // save for right side.

			rc.bottom = k;
			rc.top = rc.bottom - hhTerm->xBezel;

			if (IntersectRect(&rci, &rc, &ps.rcPaint))
				{
				/* --- Paint from bezel to bottom of screen --- */

				m = rc.top;
				rc.top = rc.bottom - OUTDENT;
				FillRect(hdc, &rc, hhTerm->hbrushBackScrl);
				rc.top = m;
				rc.bottom -= OUTDENT + 1;

				/* --- Paint outer border --- */

				SelectObject(hdc, hhTerm->hBlackPen);
				MoveToEx(hdc, rc.left++, rc.bottom, NULL);
				LineTo(hdc, rc.right--, rc.bottom--);

				SelectObject(hdc, hhTerm->hDkGrayPen);
				MoveToEx(hdc, rc.left++, rc.bottom, NULL);
				LineTo(hdc, rc.right--, rc.bottom);

                /* --- Paint middle portion --- */

				rc.top = rc.bottom - n;
	            FillRect(hdc, &rc, hhTerm->hbrushBackScrl);
				rc.left += n;
				rc.right -= n-1;
				rc.bottom -= n;

                /* --- Paint inner border --- */

				SelectObject(hdc, hhTerm->hWhitePen);
				MoveToEx(hdc, rc.left++, rc.bottom, NULL);
				LineTo(hdc, rc.right--, rc.bottom--);

				MoveToEx(hdc, rc.left++, rc.bottom, NULL);
				LineTo(hdc, rc.right--, rc.bottom--);

				SelectObject(hdc, hhTerm->hDkGrayPen);
				MoveToEx(hdc, rc.left++, rc.bottom, NULL);
				LineTo(hdc, rc.right--, rc.bottom--);

				/* --- Fill in the bottom below bezel --- */

				rc.top = k;
				rc.bottom = rc.top + OUTDENT;
				rc.left = xl + OUTDENT;
				rc.right = l + 1;

				FillRect(hdc, &rc, hhTerm->hbrushBackScrl);
				}

			/* -------------- Right edge ------------- */

			rc.top = (-hhTerm->iVScrlPos * hhTerm->yChar) + OUTDENT;
			rc.bottom = k;
			rc.right = l + OUTDENT + 1;
			rc.left = rc.right - hhTerm->xBezel;

			if (IntersectRect(&rci, &rc, &ps.rcPaint))
				{
				/* --- Paint outdent region --- */

				rc.left = l;
				FillRect(hdc, &rc, hhTerm->hbrushBackScrl);
				rc.bottom -= OUTDENT;
				rc.right -= OUTDENT + 1;

				/* --- Paint outer border --- */

				SelectObject(hdc, hhTerm->hBlackPen);
				MoveToEx(hdc, rc.right, rc.top++, NULL);
				LineTo(hdc, rc.right--, rc.bottom--);

				SelectObject(hdc, hhTerm->hDkGrayPen);
				MoveToEx(hdc, rc.right, rc.top++, NULL);
				LineTo(hdc, rc.right, rc.bottom--);

                /* --- Paint middle portion --- */

				rc.left = rc.right - n;
	            FillRect(hdc, &rc, hhTerm->hbrushBackScrl);
				rc.top += n-1;
				rc.right -= n;
				rc.bottom -= n-1;

                /* --- Paint inner border --- */

				SelectObject(hdc, hhTerm->hWhitePen);
				MoveToEx(hdc, rc.right, rc.top++, NULL);
				LineTo(hdc, rc.right--, rc.bottom--);

				MoveToEx(hdc, rc.right, rc.top++, NULL);
				LineTo(hdc, rc.right--, rc.bottom--);

				SelectObject(hdc, hhTerm->hDkGrayPen);
				MoveToEx(hdc, rc.right, rc.top++, NULL);
				LineTo(hdc, rc.right--, rc.bottom--);

				/* --- Fill area to right of bezel --- */

				rc.left = l + 1;
				rc.right = rc.left + OUTDENT;
				rc.top = -hhTerm->iVScrlPos * hhTerm->yChar;
				rc.bottom = k + OUTDENT;

				FillRect(hdc, &rc, hhTerm->hbrushBackScrl);
				}
			}
		}

	// Paint backscroll buffer.  Stuff going into the backscroll
	// region is always on a line basis.   You'll notice that the
	// calculation for l in the terminal area portion is more
	// complicated since stuff comes in at times on a character
	// basis.  l represents the number of characters in the
	// character string to paint.

	i = iPaintBeg;

	if (i < iPaintEnd && i < 0)
		{
		j = (ps.rcPaint.left - hhTerm->xIndent
			- (hhTerm->iHScrlPos ? 0 : hhTerm->xBezel)) / hhTerm->xChar;

		j = max(j, 0);

		k = (j * hhTerm->xChar) + hhTerm->xIndent
				+ (hhTerm->iHScrlPos ? 0 : hhTerm->xBezel);

		j += hhTerm->iHScrlPos - (hhTerm->xBezel && hhTerm->iHScrlPos ? 1:0);

		l = min(hhTerm->iCols - j,
			(ps.rcPaint.right + hhTerm->xChar - 1 - k) / hhTerm->xChar);

		fplpstrTxt = hhTerm->fplpstrBkTxt;
		m = hhTerm->iPhysicalBkRows;

		if (hhTerm->fBackscrlLock == TRUE)
			iOffset = (abs(hhTerm->iVScrlPos - i) + hhTerm->iNextBkLn) % m;

		else
			iOffset = (m + i + hhTerm->iNextBkLn) % m;

		n = iScrlInc * (-hhTerm->iVScrlPos + i);

#ifdef CHAR_NARROW
		for ( ; i < iPaintEnd && i < 0 ; i+=1, n+=iScrlInc)
			{
			TextAttrOut(hhTerm, hdc, k, n, fplpstrTxt[iOffset]+j,
				(PSTATTR)0, FALSE, l, i, j);

			if (++iOffset >= m)
				iOffset = 0;
			}
#else
        // This little hack is here to display wide (Kanji) characters in
		// the backscroll, without all of the fuss of adding attributes.
		// It forces the backscroll to always paint the
        // enitre row, and strips out the repeated left/right pairs

		j = (OUTDENT - (hhTerm->iHScrlPos ? 0 : hhTerm->xBezel)) / hhTerm->xChar;
		j = max(j, 0);
			
		k = (j * hhTerm->xChar) + OUTDENT
				+ (hhTerm->iHScrlPos ? 0 : hhTerm->xBezel);
		j += hhTerm->iHScrlPos - (hhTerm->xBezel && hhTerm->iHScrlPos ? 1:0);
		l = hhTerm->iCols - j;
		
		for ( ; i < iPaintEnd && i < 0 ; i+=1, n+=iScrlInc)
			{
			// Hack for starting on the right half of a DB char
			fRight = FALSE;

			for (x = 0; x <= j; )
				{
				if (isDBCSChar(*(fplpstrTxt[iOffset]+x)))
					{
					fRight = (x == j) ? FALSE : TRUE;
					x += 2;
					}
				else
					{
					fRight = FALSE;
					x += 1;
					}
				}

			memset(aechTmpBuf, 0, sizeof(aechTmpBuf));
			StrCharStripDBCSString(aechTmpBuf, sizeof(aechTmpBuf),
				fplpstrTxt[iOffset]+j); //mpt:12-11-97 too many parameters? , l * sizeof(ECHAR));

			TextAttrOut(hhTerm, hdc, k, n, aechTmpBuf,
				(PSTATTR)0, fRight, l, i, j);

			if (++iOffset >= m)
				iOffset = 0;
			}
#endif
		}

	/* -------------- Paint divider bar if necessary ------------- */

	for ( ; i == 0 ; ++i)
		{
		if (hhTerm->xBezel)
			{
			// n is the width of the thick gray section of the bezel.
			// we surround the thick gray part with four lines of
			// of white and gray lines.  That's why we subtract 4.

			n = hhTerm->xBezel - OUTDENT - 4;

			rc.top = -hhTerm->iVScrlPos * hhTerm->yChar;
			rc.bottom = rc.top + OUTDENT;
            rc.left = ps.rcPaint.left;
            rc.right = ps.rcPaint.right;

			/* --- Paint gap above divider bar --- */

            FillRect(hdc, &rc, hhTerm->hbrushBackScrl);

			rc.left = xl + hhTerm->xBezel + hhTerm->xIndent;
			rc.right = xr;

            /* --- If hightlighting, paint gap above divider --- */

			if (min(hhTerm->ptBeg.y, hhTerm->ptEnd.y) < 0 &&
					max(hhTerm->ptBeg.y, hhTerm->ptEnd.y) > 0)
				{
				FillRect(hdc, &rc, hhTerm->hbrushHighlight);
				}

			rc.left = xl + OUTDENT;
			rc.right = xr + hhTerm->xIndent + hhTerm->xBezel - OUTDENT - 1;

			/* --- note: order is important, bottom, top --- */

			rc.bottom = rc.top + hhTerm->yChar;
			rc.top += OUTDENT;

            /* --- Paint top outside border --- */

			SelectObject(hdc, hhTerm->hWhitePen);
			MoveToEx(hdc, rc.left++, rc.top, NULL);
			LineTo(hdc, rc.right--, rc.top++);

			MoveToEx(hdc, rc.left++, rc.top, NULL);
			LineTo(hdc, rc.right--, rc.top++);

            /* --- Paint middle section --- */

            m = rc.bottom;
			rc.bottom = rc.top + n;
            FillRect(hdc, &rc, hhTerm->hbrushBackScrl);
            rc.bottom = m;
			rc.left += n;
			rc.top += n;
			rc.right -= n;

            /* --- Paint bottom border --- */

			SelectObject(hdc, hhTerm->hDkGrayPen);
			MoveToEx(hdc, rc.left++, rc.top, NULL);
			LineTo(hdc, rc.right--, rc.top++);

			SelectObject(hdc, hhTerm->hBlackPen);
			MoveToEx(hdc, rc.left++, rc.top, NULL);
			LineTo(hdc, rc.right, rc.top++);

			/* --- Fill in any space left below the bezel --- */

			FillRect(hdc, &rc, hhTerm->hbrushTerminal);

			if (min(hhTerm->ptBeg.y, hhTerm->ptEnd.y) < 0 &&
					max(hhTerm->ptBeg.y, hhTerm->ptEnd.y) > 0)
				{
				rc.left = hhTerm->xIndent;
				rc.left += (hhTerm->iHScrlPos) ? 0 : hhTerm->xBezel;
				rc.right -= hhTerm->xIndent;
				FillRect(hdc, &rc, hhTerm->hbrushHighlight);
				}
			}

		else
			{
			j = (hhTerm->yChar * -hhTerm->iVScrlPos);
			k = (hhTerm->yChar) / 3;

			// Create highlight brush if textmarking crosses divider

			if (min(hhTerm->ptBeg.y, hhTerm->ptEnd.y) < 0 &&
					max(hhTerm->ptBeg.y, hhTerm->ptEnd.y) > 0)
				{
				hBrush = hhTerm->hbrushHighlight;
				}

			else
				{
				hBrush = 0;
				}

			rc = ps.rcPaint;

			rc.top = j;
			rc.bottom = j + k + (hhTerm->yChar % 3);

			l = rc.right =
					((hhTerm->iCols - hhTerm->iHScrlPos) * hhTerm->xChar)
						+ hhTerm->xIndent + hhTerm->xIndent +
							hhTerm->xBezel + hhTerm->xBezel;

			FillRect(hdc, &rc, (hBrush != (HBRUSH)0) ?
												hBrush :
												hhTerm->hbrushBackScrl);

			rc.top = rc.bottom;
			rc.bottom = rc.top + k;
			rc.right = ps.rcPaint.right;
			FillRect(hdc, &rc, hhTerm->hbrushDivider);

			rc.top = rc.bottom;
			rc.bottom = j + hhTerm->yChar;
			rc.right = l;
			FillRect(hdc, &rc, (hBrush != (HBRUSH)0) ?
												hBrush :
												hhTerm->hbrushTerminal);
			}
		}

	/* -------------- Paint the active terminal portion. ------------- */

	xl = (hhTerm->iHScrlPos) ? 0 : hhTerm->xBezel;

	if (i < iPaintEnd)
		{
		j = (ps.rcPaint.left - hhTerm->xIndent - xl) / hhTerm->xChar;

		j = max(j, 0);

		k = (j * hhTerm->xChar) + hhTerm->xIndent + xl;

		j += hhTerm->iHScrlPos - (hhTerm->xBezel && hhTerm->iHScrlPos ? 1:0);

		l = min(hhTerm->iCols - j,
			(ps.rcPaint.right + hhTerm->xChar - 1 - k) / hhTerm->xChar);

		// Formulas to convert from terminal to buffer row and back.
		// b = (t - 1 + top) % rows
		// t = (b + rows - top) % rows
		// t is always between 1 and rows+1.

		iOffset = (i - 1 + hhTerm->iTopline) % MAX_EMUROWS;
		n = iScrlInc * (-hhTerm->iVScrlPos + i);

		for ( ; i < iPaintEnd ; i += 1, n += iScrlInc)
			{
			TextAttrOut(hhTerm, hdc, k, n, hhTerm->fplpstrTxt[iOffset]+j,
				hhTerm->fppstAttr[iOffset]+j, FALSE, l, i, j);

			if (++iOffset >= MAX_EMUROWS)
				iOffset = 0;
			}
		}

	/* --- Draw cursors --- */

	if (hhTerm->fHstCurOn)
		{
		hhTerm->fHstCurOn = FALSE;
		PaintHostCursor(hhTerm, TRUE, hdc);
		}

	if (hhTerm->fLclCurOn)
		{
		hhTerm->fLclCurOn = FALSE;
		PaintLocalCursor(hhTerm, TRUE, hdc);
		}

	/* --- cleanup time --- */

	SelectObject(hdc, GetStockObject(WHITE_BRUSH));
	SelectObject(hdc, GetStockObject(BLACK_PEN));
	SelectObject(hdc, hFont);

	EndPaint(hwnd, &ps);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TextAttrOut
 *
 * DESCRIPTION:
 *	Draws text and attributes on the terminal screen.  Optimizes call to
 *	TextOut based on attributes.  For example, If attributes are all the
 *	same for a given line, then only one call to TextOut is made.
 *
 * ARGUMENTS:
 *	HHTERM	hhTerm		- internal terminal handle
 *	HDC 	hdc 		- Device context from WM_PAINT
 *	int 	x			- x offset in pixels
 *	int 	y			- y offset in pixels
 *	ECHAR	*lpachText	- long pointer to begining of text to print
 *	PSTATTR apstAttrs	- Array of pointers to attribute structures
 *	int		fForceRight - used for DBCS in the backscroll
 *	int 	nCount		- number of characters to draw.
 *	int 	iRow		- logical row being displayed
 *	int 	iCol		- logical col being displayed
 *
 * RETURNS:
 *	void
 *
 */
static void TextAttrOut(const HHTERM	 hhTerm,
						const HDC		 hdc,
						const int		 x,
						const int		 y,
						const ECHAR	 	*lpachText,
						const PSTATTR	 apstAttrs,
						const int        fForceRight,
						const int		 nCount,
						const int		 iRow,
						const int		 iCol)
	{
	int 			 i = 0, 			// track of how many chars drawn.
					 j, 				// number of characters in a run.
					 k, 				// offset where run began.
					 nXPos;
	int 			 nXStart, nYStart;	// screen pos where chars are drawn.
#ifndef CHAR_NARROW
	int		nByteCount = 0;
#endif
	int				 nBegAdj = 0;
	int				 nEndAdj = 0;
	int				 nStrLen = 0;
	TCHAR            achBuf[MAX_EMUCOLS * 2];
	ECHAR			 aechTmp[MAX_EMUCOLS + 1];
	PSTATTR 		 pstAttr 		= 0;
	unsigned int	 uTxtclr, uBkclr;
	BOOL			 fUnderlnFont = FALSE;
	BOOL			 fSymbolFont = FALSE;
	BOOL			 fDblHiHi = FALSE;
	BOOL			 fDblHiLo = FALSE;
	BOOL			 fDblWiLf = FALSE;
	BOOL			 fDblWiRt = FALSE;
	BOOL			 fWiLf	  = FALSE;
	BOOL			 fWiRt 	  = FALSE;
	BOOL			 fFirstPass = TRUE;
	RECT			 rc;

	DbgOutStr("TAO %d\r\n", iRow, 0, 0, 0, 0);

	while (i < nCount)
		{
		k = i;	// save offset of where this run begins in k.
		if (iRow < 0)
			{
			int	  nCurPos;
			long  l,
				  lBeg,
				  lEnd;
			BOOL  fMarking = FALSE;

			// Store the positions of the 1st char to draw
			nCurPos = i;

			// This check is a speed tweak.  If the marking and backscrl
			// locks are off, we know that no text is can be marked.  This
			// would be the normal case when stuff is streaming into the
			// terminal so we save the work done of the else body by
			// by checking and thus speed up the display.

			// For Far East we have to force the backscroll to paint every
			// character individualy.  This is becuase MicroSquish is not
			// able to provide fixed pitch fonts that are fixed pitch for
			// DBCS characters.
            //
			if (hhTerm->fMarkingLock == FALSE && 
				hhTerm->fBackscrlLock == FALSE &&
				hhTerm->iEvenFont == TRUE)
				{
				SetBkColor(hdc, hhTerm->crBackScrl);
				SetTextColor(hdc, hhTerm->crBackScrlTxt);
				i = j = nCount;
				}

			else
				{
            #ifdef CHAR_NARROW
				lBeg = hhTerm->ptBeg.y * hhTerm->iCols + hhTerm->ptBeg.x;
				lEnd = hhTerm->ptEnd.y * hhTerm->iCols + hhTerm->ptEnd.x;

            #else
				// Hack city man :)
				// In the mouse handling routines, we modify the selection 
				// cursor position so that you can't put it into the middle
				// of a wide character.  The problem is that at this
				// point the repeated characters have been removed.
				// Therefore we need to modify the cursor position to take
				// into account the number of repeated characters that
				// were removed from the string as of our current drawing 
				// position into that string.
				// pant, pant, pant.
				lBeg = hhTerm->ptBeg.x;
				lEnd = hhTerm->ptEnd.x;

				if (hhTerm->ptBeg.y == iRow)
					{
					lBeg = MapCells(lpachText , hhTerm->iCols, hhTerm->ptBeg.x);
					}

				if (hhTerm->ptEnd.y == iRow)
					{
					lEnd = MapCells(lpachText , hhTerm->iCols, hhTerm->ptEnd.x);
					}

				if (hhTerm->iEvenFont)
					{
					nBegAdj = lBeg - hhTerm->ptBeg.x;
					nEndAdj = lEnd - hhTerm->ptEnd.x;
					}
				else
					{
					nBegAdj = 0;
					nEndAdj = 0;
					}

				lBeg = hhTerm->ptBeg.y * hhTerm->iCols + lBeg;
				lEnd = hhTerm->ptEnd.y * hhTerm->iCols + lEnd;
#endif

				if (lBeg > lEnd)
					{
					l = lEnd;
					lEnd = lBeg;
					lBeg = l;
					}

				// IN_RANGE macro is inclusive so subtract one from sEnd
				// which at this point is known to be larger than sBeg

				lEnd -= 1;

				l = iRow * hhTerm->iCols + i + iCol;
				fMarking = (BOOL)IN_RANGE(l, lBeg, lEnd);

				if (fMarking)
					{
					SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
					SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
					}
				else
					{
					SetBkColor(hdc, hhTerm->crBackScrl);
					SetTextColor(hdc, hhTerm->crBackScrlTxt);
					}

				// For Far East we have to force the backscroll to paint every
				// character individualy.  This is becuase MicroSquish is not
				// able to provide fixed pitch fonts that are fixed pitch for
				// DBCS characters.
				if (hhTerm->iEvenFont)
					{
					for (j = 0 ; i < nCount ; ++i, ++j, ++l)
						{
						if (fMarking != (BOOL)IN_RANGE(l, lBeg, lEnd))
							break;
						}
					}
				else
					{
					i++;
					j = 1;
					}
				}

			nXPos = k = i - j;

#ifndef CHAR_NARROW
			MemCopy(aechTmp, lpachText , (unsigned)nCurPos * sizeof(ECHAR));
			nXPos = CnvrtECHARtoMBCS(achBuf, sizeof(achBuf), aechTmp, (unsigned)nCurPos * sizeof(ECHAR));
			//nXPos = StrCharGetByteCount(achBuf); // mrw;5/17/95
			// 18-May-1995	DLW
			// Added for the case where a DBCS string is started with half of
			// the character, like when scrolling to the right
			if (fForceRight && fFirstPass)
				{
				fWiRt = TRUE;
				}
			else
				{
				if (fForceRight)
					nXPos = max(0, nXPos - 1);
				}
#endif
			}

		// At this point we know were not painting in the backscrl area.
		// Since the terminal area has attributes that backscrl does not
		// have (ie. color, underlining, etc.) we need to do more work.
		//
		else
			{
			pstAttr = apstAttrs+i;
			uTxtclr = pstAttr->txtclr;
			uBkclr	= pstAttr->bkclr;

			// Test for attributes

			if (pstAttr->revvid)
				{
				unsigned uTmp = uTxtclr;
				uTxtclr = uBkclr;
				uBkclr	= uTmp;
				}

			if (pstAttr->hilite)
				{
				// Colors are arranged so that high intensity colors are
				// in the lower half of the color array (currently 16
				// colors corresponding to the IBM PC colors).	This
				// guy sets the color to the opposite intensity.

				uTxtclr = (uTxtclr + (MAX_EMUCOLORS/2)) % MAX_EMUCOLORS;

				if (uTxtclr == uBkclr)
					uTxtclr = (uTxtclr + 1) % MAX_EMUCOLORS;
				}

			if (pstAttr->bklite)
				{
				uBkclr = (uBkclr + (MAX_EMUCOLORS/2)) % MAX_EMUCOLORS;

				if (uBkclr == uTxtclr)
					uBkclr = (uBkclr + 1) % MAX_EMUCOLORS;
				}

			if (pstAttr->blink)
				{
				int iBufRow;

				if (hhTerm->iBlink == 0)
					hhTerm->iBlink = 1;

				if (hhTerm->iBlink == -1)
					uTxtclr = uBkclr;

				iBufRow = (iRow - 1 + hhTerm->iTopline) % MAX_EMUROWS;
				hhTerm->abBlink[iBufRow] = (BYTE)TRUE;
				}

			if (pstAttr->undrln)
				fUnderlnFont = TRUE;

			if (pstAttr->blank)
				uTxtclr = uBkclr;

			// Only allow one double height attribute
			//
			if (pstAttr->dblhihi)
				fDblHiHi = TRUE;

			else if (pstAttr->dblhilo)
				fDblHiLo = TRUE;

			// Only allow one double wide attribute
			//
			if (pstAttr->dblwilf)
				fDblWiLf = TRUE;

			else if (pstAttr->dblwirt)
				fDblWiRt = TRUE;

			// Only allow one wide attribute
			//
			if (pstAttr->wilf)
				fWiLf = TRUE;

			else if (pstAttr->wirt)
				fWiRt = TRUE;

			// If the symbol attribute is on, use alternate font.
			//
			if (pstAttr->symbol)
				fSymbolFont = TRUE;

			// Find the longest run of characters with the same attributes.
			//
			for (j = 0 ; i < nCount ; ++i, ++j)
				{
				if (memcmp(pstAttr, apstAttrs+i, sizeof(STATTR)) != 0)
					break;
				}

			/* --- Set text and background colors --- */

			if (pstAttr->txtmrk)
				{
				SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
				SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
				}

			else
				{
				SetBkColor(hdc, hhTerm->pacrEmuColors[uBkclr]);
				SetTextColor(hdc, hhTerm->pacrEmuColors[uTxtclr]);
				}

			/* --- How hard can it be to pick a font? --- */

			if (fDblHiHi || fDblHiLo)
				{
				if (fDblWiLf || fDblWiRt)
					{
					if (fSymbolFont)
						{
						if (hhTerm->hSymDblHiWiFont == 0)
							hhTerm->hSymDblHiWiFont = termMakeFont(hhTerm, 0, 1, 1, 1);

						SelectObject(hdc, hhTerm->hSymDblHiWiFont);
						}

					else
						{
						if (hhTerm->hDblHiWiFont == 0)
							hhTerm->hDblHiWiFont = termMakeFont(hhTerm, 0, 1, 1, 0);

						SelectObject(hdc, hhTerm->hDblHiWiFont);
						}
					}

				else
					{
					if (fSymbolFont)
						{
						if (hhTerm->hSymDblHiFont == 0)
							hhTerm->hSymDblHiFont = termMakeFont(hhTerm, 0, 1, 0, 1);

						SelectObject(hdc, hhTerm->hSymDblHiFont);
						}

					else
						{
						if (hhTerm->hDblHiFont == 0)
							hhTerm->hDblHiFont = termMakeFont(hhTerm, 0, 1, 0, 0);

						SelectObject(hdc, hhTerm->hDblHiFont);
						}
					}
				}

			else if (fDblWiLf || fDblWiRt)
				{
				if (fSymbolFont)
					{
					if (hhTerm->hSymDblWiFont == 0)
						hhTerm->hSymDblWiFont = termMakeFont(hhTerm, 0, 0, 1, 1);

					SelectObject(hdc, hhTerm->hSymDblWiFont);
					}

				else
					{
					if (hhTerm->hDblWiFont == 0)
						hhTerm->hDblWiFont = termMakeFont(hhTerm, 0, 0, 1, 0);

					SelectObject(hdc, hhTerm->hDblWiFont);
					}
				}

			else if (fSymbolFont)
				{
				if (hhTerm->hSymFont == 0)
					hhTerm->hSymFont = termMakeFont(hhTerm, 0, 0, 0, 1);

				SelectObject(hdc, hhTerm->hSymFont);
				}

			nXPos = k = i - j;
			}

		// Ok, go ahead, paint that text, make my day...

		// Guess I better explain myself here.	When Windows synthesizes
		// a font to get something like bold or italic, TextOut has a nasty
		// habit of adding a pixel to the end of the string (be it a
		// character or a run of characters).  This is documented in the
		// TEXTMETRIC structure in the description of tmOverHang.  This has
		// some unfortunate side effects (Don't believe me?  Try defing the
		// old stuff back in, select a bold font and do some hightlighting
		// and scrolling).	The solution is to explicitly clip so that text
		// is drawn only where I say it can draw.  This amounts in extra
		// work but the end result seems no slower. - mrw
        //
		rc.left   = x + (nXPos * hhTerm->xChar);

#if defined(FAR_EAST)
		if (rc.left > (x + (hhTerm->iCols * hhTerm->xChar)))
			rc.left = x + (hhTerm->iCols * hhTerm->xChar);
#endif

		rc.right  = rc.left + ((j - nEndAdj) * hhTerm->xChar);

		if (rc.right > (x + (hhTerm->iCols * hhTerm->xChar)))
			rc.right = x + (hhTerm->iCols * hhTerm->xChar);

		rc.top	  = y;
		rc.bottom = y + hhTerm->yChar;

		// If we're painting the bottom half of a double high character,
		// we need to raise the origin of where the character is drawn
		// by an additional char height.  The clipping rectangle insures
		// that only the cell portion is actually drawn.

		nYStart = (fDblHiHi) ? rc.bottom + hhTerm->yChar : rc.bottom;

		// More painting tricks.  If we're painting double wide characters
		// we'll be painting them one at a time.  Why you ask?  Well, this
		// routine tries to paint the longest run of characters with the
		// same attributes.  Just makes good windows sense.  In the case
		// of double wide however, the attributes come as double-wide left,
		// double-wide right.  Since the "double-wide" character has a
		// different attribute in its left side and right side, this
		// routine will paint each half one at a time (seperate calls to
		// ExtTextOut()).  This is useful and wanted.  Because we clip
		// the drawing area to the cell or cells region, we can draw the
		// right half by offsetting the x origin by one character cell
		// to the left.  Windows clipping logic will only draw in the
		// actual cell region so we get the right half of the character.
		// Lot of explaination for one measely line of code.
        //
		nXStart = (fDblWiRt || fWiRt) ? rc.left - hhTerm->xChar : rc.left;

		/* --- Hero of the day, ExtTextOut! --- */

		MemCopy(aechTmp, lpachText + k, (unsigned int)j * sizeof(ECHAR));

		nStrLen = CnvrtECHARtoMBCS(achBuf, sizeof(achBuf), aechTmp,
			(unsigned)j * sizeof(ECHAR)); //mrw:5/17/95

		achBuf[nStrLen] = TEXT('\0');

        #if defined(FAR_EAST) // mrw:10/10/95 - ifdef'ed this code
		if (!hhTerm->iEvenFont && !(fForceRight && fFirstPass))
			{
			// If this is a DBCS character we are painting, then we want to paint 2 cells
			// instead of 1 (makes sense, DBCS characters are twice as wide as SBCS).
			if (IsDBCSLeadByte(*achBuf))
				{
				rc.right += hhTerm->xChar;
                rc.right = min(rc.right, x + (hhTerm->iCols * hhTerm->xChar));
				}
			}
        #endif

		ExtTextOut(hdc, nXStart, nYStart, ETO_CLIPPED | ETO_OPAQUE,
			&rc, achBuf, (unsigned)nStrLen, 0);

		// Experiment.	Most terminals underline their fonts by placing
		// underscores under the characters.  The Windows underline font
		// places a thin line through the baseline of the character.
		// Many hosts use underlining in conjunction with box draw chars
		// to envelop regions of the screen.  The underscores complete
		// envelope and don't give the apperance of part of the characters
		// bleeding through. - mrw
		//
		if (fUnderlnFont)
			{
			// The double height makes the underscores too thick yet we
			// want to maintain the double-wide or symbol font we may be
			// using so check and recast the font to the samething without
			// the height component. - mrw
			//
			if (fDblHiHi || fDblHiLo)
				{
				if (fDblWiLf || fDblWiRt)
					{
					if (fSymbolFont)
						{
						if (hhTerm->hSymDblWiFont == 0)
							hhTerm->hSymDblWiFont = termMakeFont(hhTerm, 0, 0, 1, 1);

						SelectObject(hdc, hhTerm->hSymDblWiFont);
						}

					else
						{
						if (hhTerm->hDblWiFont == 0)
							hhTerm->hDblWiFont = termMakeFont(hhTerm, 0, 0, 1, 0);

						SelectObject(hdc, hhTerm->hDblWiFont);
						}
					}

				else if (fSymbolFont)
					{
					if (hhTerm->hSymFont == 0)
						hhTerm->hSymFont = termMakeFont(hhTerm, 0, 0, 0, 1);

					SelectObject(hdc, hhTerm->hSymFont);
					}

				else
					{
					SelectObject(hdc, hhTerm->hFont);
					}
				}

			// Set background to transparent so only the underscore
			// portion overpaints the character cell.
			//
			SetBkMode(hdc, TRANSPARENT);

			ExtTextOut(hdc, nXStart, nYStart, ETO_CLIPPED,
				&rc, hhTerm->underscores, (unsigned)j, 0);

			SetBkMode(hdc, OPAQUE);
			}

		/* --- Reselect the regular font only if we had to switch --- */

		if (fUnderlnFont || fDblHiHi || fDblHiLo || fDblWiLf || fDblWiRt
				|| fSymbolFont)
			{
			SelectObject(hdc, hhTerm->hFont);
			fUnderlnFont = FALSE;
			fSymbolFont = FALSE;
			fDblHiHi = FALSE;
			fDblHiLo = FALSE;
			fDblWiLf = FALSE;
			fDblWiRt = FALSE;
			}

		fWiRt = FALSE;
		fWiLf = FALSE;
		fFirstPass = FALSE;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	MapCells
 *
 * DESCRIPTION:
 *  Maps screen cell position to internal screen image (echar image) position
 *
 * ARGUMENTS:
 *  each - source string (does it start from beg of line?)
 *  nStrLen - length in bytes
 *  nCellPos - screen position to 
 *
 * RETURNS:
 *	void
 *
 */
static int MapCells(const ECHAR *each, const int nStrLen, const int nCellPos)
	{
	int i, nCount;

	if (each == NULL)
		{
		assert(FALSE);
		return 0;
		}

	for (i = 0, nCount = 0; nCount < nCellPos && i < nStrLen; i++)
		{
		if (isDBCSChar(each[i]))
			{
			nCount += 2;
			}
		else
			{
			nCount += 1;
			}
		}
	return i;
	}

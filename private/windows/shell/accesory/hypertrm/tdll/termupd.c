/*	File: D:\WACKER\tdll\termupd.c (Created: 11-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:34p $
 */

#include <windows.h>
#pragma hdrstop

#include <stdlib.h>
#include <limits.h>

#include "stdtyp.h"
#include "assert.h"
#include "session.h"
#include <emu\emu.h>
#include "update.h"
#include "update.hh"
#include "backscrl.h"
#include "timers.h"
#include "tdll.h"
#include "tchar.h"
#include "term.h"
#include "term.hh"
#include "mc.h"

static void termUpdate(const HHTERM hhTerm);
static int termReallocBkBuf(const HHTERM hhTerm, const int iLines);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termGetUpdate
 *
 * DESCRIPTION:
 *	Queries the update records and emulator to update the terminal image.
 *
 * ARGUMENTS:
 *	hhTerm	- internal terminal handle.
 *
 * RETURNS:
 *	void
 *
 */
void termGetUpdate(const HHTERM hhTerm, const int fRedraw)
	{
	ECHAR		   **pachTxt,
					*pachTermTxt,
					*pachEmuTxt;
	PSTATTR 	   *pstAttr,
					pstTermAttr,
					pstEmuAttr;
	int 			i, j, k, m;
	BYTE		   *pabLines;

//	const iRows = hhTerm->iRows;
//	const iCols = hhTerm->iCols;
	const iRows = MAX_EMUROWS;
	const iCols = MAX_EMUCOLS;

	const HEMU hEmu = sessQueryEmuHdl(hhTerm->hSession);
	const HHUPDATE hUpd = (HHUPDATE)sessQueryUpdateHdl(hhTerm->hSession);

	/* --- Lock emulators so we have execlusive access --- */

	emuLock(hEmu);

	pachTxt = emuGetTxtBuf(hEmu);
	pstAttr = emuGetAttrBuf(hEmu);

	// Now check to see what needs updating...

	if (hUpd->bUpdateType == UPD_LINE)
		{
		struct stLineMode *pstLine = &hUpd->stLine;

		if (pstLine->iLine != -1)
			{
			// The emulators can place the cursor one past the number
			// of columns.	Why, I don't know, so we check and adjust
			// so we don't overwrite our client arrays.

			pstLine->xEnd = min(pstLine->xEnd, iCols);
			assert(pstLine->xBeg <= pstLine->xEnd);

			k = (pstLine->iLine + hUpd->iTopline) % MAX_EMUROWS;

			pachEmuTxt	= pachTxt[k] + pstLine->xBeg;
			pachTermTxt = hhTerm->fplpstrTxt[k] + pstLine->xBeg;

			pstEmuAttr	= pstAttr[k] + pstLine->xBeg;
			pstTermAttr = hhTerm->fppstAttr[k] + pstLine->xBeg;

			for (k = pstLine->xEnd - pstLine->xBeg ; k >= 0 ; --k)
				{
				// Televideo uses \xFF as a NULL character.

				if (*pachEmuTxt == ETEXT('\0') || *pachEmuTxt == ETEXT('\xFF'))
					*pachTermTxt = ETEXT(' ');

				else
					*pachTermTxt = *pachEmuTxt;

				*pstTermAttr = *pstEmuAttr;

				pachTermTxt += 1;
				pachEmuTxt	+= 1;

				pstTermAttr += 1;
				pstEmuAttr	+= 1;
				}
			}
		}

	else if (hUpd->bUpdateType == UPD_SCROLL)
		{
		struct stScrlMode *pstScrl = &hUpd->stScrl;

		hUpd->iLines = hhTerm->iBkLines =
			backscrlGetNumLines(sessQueryBackscrlHdl(hhTerm->hSession));

		/* -------------- Backscroll portion ------------- */

		if ((i = min(hhTerm->iBkLines, pstScrl->iBksScrlInc)) > 0)
			termGetBkLines(hhTerm, i, -i, BKPOS_ABSOLUTE);

		/* -------------- Terminal portion ------------- */

		if (pstScrl->iScrlInc != 0)
			{
			if (pstScrl->iScrlInc > 0)
				{
				i = max(0, pstScrl->yEnd - pstScrl->iScrlInc + 1);
				m = pstScrl->yEnd;
				}

			else
				{
				i = pstScrl->yBeg;
				m = min(iRows, pstScrl->yBeg - pstScrl->iScrlInc - 1);
				}


			k = ((i + hUpd->iTopline) % MAX_EMUROWS);

			for (; i <= m ; ++i)
				{
				// Server and Client have different size emulator images
				// for historical reasons.	Server side has 2 extra characters
				// per row.

				pachTermTxt = hhTerm->fplpstrTxt[k];
				pachEmuTxt = pachTxt[k];

				pstTermAttr = hhTerm->fppstAttr[k];
				pstEmuAttr = pstAttr[k];

				// Update the terminal buffer now.

				for (j = 0 ; j < iCols ; ++j, ++pachTermTxt, ++pachEmuTxt)
					{
					// Televideo uses \xFF as a NULL character.

					if (*pachEmuTxt == ETEXT('\0') ||
							*pachEmuTxt == ETEXT('\xFF'))
						{
						*pachTermTxt = ETEXT(' ');
						}

					else
						{
						*pachTermTxt = *pachEmuTxt;
						}
					}

				MemCopy(pstTermAttr, pstEmuAttr, iCols * sizeof(STATTR));

				if (++k >= MAX_EMUROWS)
					k = 0;
				}
			}

		// Check for lines now.

		k = hUpd->iTopline;
		pabLines = pstScrl->auchLines + pstScrl->iFirstLine;

		for (j = 0 ; j < iRows ; ++j, ++pabLines)
			{
			if (*pabLines != 0)
				{
				pachEmuTxt = pachTxt[k];
				pachTermTxt = hhTerm->fplpstrTxt[k];

				pstEmuAttr = pstAttr[k];
				pstTermAttr = hhTerm->fppstAttr[k];

				// Update the terminal buffer now.

				for (i = 0 ; i < iCols ; ++i, ++pachTermTxt, ++pachEmuTxt)
					{
					// Televideo uses \xFF as a NULL character.

					if (*pachEmuTxt == ETEXT('\0') ||
							*pachEmuTxt == ETEXT('\xFF'))
						{
						*pachTermTxt = ETEXT(' ');
						}

					else
						{
						*pachTermTxt = *pachEmuTxt;
						}
					}

				MemCopy(pstTermAttr, pstEmuAttr, iCols * sizeof(STATTR));
				}

			if (++k >= MAX_EMUROWS)
				k = 0;
			}

		// Another ugly situation.	Because we can mark stuff in the
		// backscroll buffer and still have updates coming in, we have
		// to bump the marking region by the scrollinc to keep everything
		// synchronized.  Note, we don't need to check for Marking locks
		// because we could not have been here had any been in place.

		if (hhTerm->ptBeg.y < 0 || hhTerm->ptEnd.y < 0)
			{
			hhTerm->ptBeg.y -= pstScrl->iScrlInc;
			hhTerm->ptEnd.y -= pstScrl->iScrlInc;
			}

		// Update emulator's topline field now.

		hhTerm->iTopline = hUpd->iTopline;
		}

	// Save a copy of the update handle in term.  This way we can
	// release our lock and paint without blocking the emulator.

	*(HHUPDATE)hhTerm->hUpdate = *hUpd;
	updateReset(hUpd);

	/* --- Important to remember to unlock emulator --- */

	emuUnlock(hEmu);

	/* --- Now let the terminal figure out how to paint itself. --- */

	if (fRedraw)
		termUpdate(hhTerm);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termUpdate
 *
 * DESCRIPTION:
 *	Invalidates the proper portions of the terminal buffers, updates
 *	scrollbars, and generally takes care of the busy work of keeping
 *	the terminal screen up to date.
 *
 * ARGUMENTS:
 *	hwnd	- terminal window handle
 *
 * RETURNS:
 *	void
 *
 */
static void termUpdate(const HHTERM hhTerm)
	{
	RECT  rc;
	int   i, j, l;
	BYTE *pauchLines;
	SCROLLINFO scrinf;
	const HHUPDATE hUpd = (HHUPDATE)hhTerm->hUpdate;

	GetClientRect(hhTerm->hwnd, &rc);

	// Adjust rectangle not to include indent/outdent areas.  This
	// will be ignored if we are scrolling the whole terminal.

	rc.left += hhTerm->xIndent + (hhTerm->iHScrlPos ? 0 : hhTerm->xBezel);
	rc.right = min(((hhTerm->iCols * hhTerm->xChar) + hhTerm->xIndent + hhTerm->xBezel),
					rc.right);

	if (hUpd->bUpdateType == UPD_LINE)
		{
		if (hUpd->stLine.iLine != -1)
			{
			rc.top	  = (hUpd->stLine.iLine -
							hhTerm->iVScrlPos + 1) * hhTerm->yChar;

			rc.bottom = rc.top + hhTerm->yChar;

			rc.left   = ((hUpd->stLine.xBeg -
							hhTerm->iHScrlPos) * hhTerm->xChar)
								+ hhTerm->xIndent + hhTerm->xBezel;

			rc.right  = ((hUpd->stLine.xEnd -
							hhTerm->iHScrlPos + 1) * hhTerm->xChar)
								+ hhTerm->xIndent + hhTerm->xBezel;

			// Invalidate entire line when doing italics
			//
			if (hhTerm->fItalic)
				{
				rc.left = 0;
				rc.right = hhTerm->cx;
				}

			InvalidateRect(hhTerm->hwnd, &rc, FALSE);
			}
		}

	else if (hUpd->bUpdateType == UPD_SCROLL)
		{
		// Scroll range will change because new text is
		// scrolling into the backscroll region.

		i = hhTerm->iVScrlMin;
		j = hhTerm->iVScrlMax;

		if (i == j)
			hhTerm->fBump = FALSE;

		l = 0;

		// If bezel is drawn, make sure we have room
		//
		if (hhTerm->xBezel)
			{
			if ((hhTerm->cy % hhTerm->yChar) < (hhTerm->xBezel + 1))
				l = 1;
			}

		hhTerm->iVScrlMin = min(-hUpd->iLines,
			hhTerm->iRows - hhTerm->iTermHite + 1 + l);

		// This forces the terminal paint correctly if the minimum
		// number of lines changes such that the current vertical
		// scrolling position is no longer valid. mrw:6/19/95
		//
		if (hhTerm->iVScrlPos < hhTerm->iVScrlMin)
			hhTerm->iVScrlPos = hhTerm->iVScrlMin;

		if (i != hhTerm->iVScrlMin)
			{
			scrinf.cbSize= sizeof(scrinf);
			scrinf.fMask = SIF_DISABLENOSCROLL | SIF_RANGE | SIF_PAGE;
			scrinf.nMin  = hhTerm->iVScrlMin;
			scrinf.nMax  = hhTerm->iVScrlMax + hhTerm->iTermHite - 1;
			scrinf.nPage = (unsigned int)hhTerm->iTermHite;
			SetScrollInfo(hhTerm->hwnd, SB_VERT, &scrinf, TRUE);
			}

		// This is subtle but necessary.  When the window is
		// large enough to show full terminal and backscroll
		// and the backscroll is empty, iVScrlPos is 0.  The
		// moment the backscroll buffer becomes larger than
		// the backscroll area displayed, the iVScrlPos becomes
		// iVScrlMax in the case where text is coming in at the
		// bottom of the terminal screen.  We could update the
		// scrollbar on every pass, but this causes an annoying
		// flicker in the scrollbar.  This next piece of code
		// catches the transition from backscroll smaller than
		// displayed area to backscroll greater than displayed
		// area and updates the scrollbar position.

		// DbgOutStr("bump=%d, Min=%d, Max=%d\r\n",
		//	  hhTerm->fBump, hhTerm->iVScrlMin, hhTerm->iVScrlMax, 0, 0);

		if (!hhTerm->fBump && hhTerm->iVScrlMin != hhTerm->iVScrlMax)
			{
			// DbgOutStr("Pos = %d\r\n", hhTerm->iVScrlPos, 0, 0, 0, 0);

			scrinf.cbSize= sizeof(scrinf);
			scrinf.fMask = SIF_DISABLENOSCROLL | SIF_POS;
			scrinf.nPos = hhTerm->iVScrlPos;
			SetScrollInfo(hhTerm->hwnd, SB_VERT, &scrinf, TRUE);
			hhTerm->fBump = TRUE;
			}

		// Scroll specified area.

		rc.top = max(0, hUpd->stScrl.yBeg -
			hhTerm->iVScrlPos + 1) * hhTerm->yChar;

		rc.bottom = min(hhTerm->iTermHite,
			(hUpd->stScrl.yEnd -
				 hhTerm->iVScrlPos + 2)) *	hhTerm->yChar;

		// use iOffset to check for cursor erase operation.

		if (!hhTerm->fBackscrlLock)
			{
			HideCursors(hhTerm);

			ScrollWindow(hhTerm->hwnd, 0, -hhTerm->yChar *
				hUpd->stScrl.iScrlInc, (LPRECT)0, &rc);

			DbgOutStr("scroll %d", -hhTerm->yChar * hUpd->stScrl.iScrlInc,
				0, 0, 0, 0);
			}

		// Examine the lines portion of update record.

		// Note, this is a negative area rectangle (ie. top is larger
		// than bottom).
		//
		rc.top = INT_MAX;
		rc.bottom = 0;

		pauchLines = hUpd->stScrl.auchLines +
						hUpd->stScrl.iFirstLine;

		for (j = 0 ; j < hhTerm->iRows ; ++j, ++pauchLines)
			{
			if (*pauchLines != (UCHAR)0)
				{
				//iPaintEnd = max(iPaintEnd, j+1);
				DbgOutStr("pauchLines->%d\r\n", j, 0, 0, 0, 0);

				// Map invalid line to terminal.

				l = (j - hhTerm->iVScrlPos + 1) * hhTerm->yChar;

				if (l >= 0)
					{
					rc.top = min(rc.top, l);
					rc.bottom = max(rc.bottom, l + hhTerm->yChar);
					}

				InvalidateRect(hhTerm->hwnd, &rc, FALSE);
				}
			}
		} /* else */

	// Update the host cursor position according to the update record.
	// mrw:6/19/95 - comparison was backwards.
	//
	if (hhTerm->ptHstCur.y != hUpd->iRow || hhTerm->ptHstCur.x != hUpd->iCol)
		{
		HideCursors(hhTerm);
		hhTerm->ptHstCur.y = hUpd->iRow;
		hhTerm->ptHstCur.x = hUpd->iCol;
		}

	// Important to paint now.	If we wait, and the
	// backscroll region is also invalid, Windows will take
	// the union of these two rectangles and paint a much
	// larger region of the screen than is needed or wanted.
	// Note: UpdateWindow does nothing if the update region
	// in empty.

	UpdateWindow(hhTerm->hwnd);

	// Now take care of the backscroll buffer.

	i = hUpd->stScrl.iBksScrlInc;

	if (i && hUpd->bUpdateType == UPD_SCROLL)
		{
		rc.top = 0;
		rc.bottom = min(hhTerm->iTermHite, -hhTerm->iVScrlPos) * hhTerm->yChar;

		if (rc.bottom > rc.top && !hhTerm->fBackscrlLock)
			{
			HideCursors(hhTerm);
			ScrollWindow(hhTerm->hwnd, 0, -hhTerm->yChar * i, (LPRECT)0, &rc);
			UpdateWindow(hhTerm->hwnd);
			}

		else if (hhTerm->fBackscrlLock)
			{
			hhTerm->iVScrlPos -= i;
			scrinf.cbSize= sizeof(scrinf);
			scrinf.fMask = SIF_DISABLENOSCROLL | SIF_POS;
			scrinf.nPos = hhTerm->iVScrlPos;
			SetScrollInfo(hhTerm->hwnd, SB_VERT, &scrinf, TRUE);
			}
		}

	// Important to do this again before we turn the host cursor
	// back on.
	// Note: UpdateWindow does nothing if the update region
	// is empty.

	UpdateWindow(hhTerm->hwnd);
	ShowCursors(hhTerm);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termGetBkLines
 *
 * DESCRIPTION:
 *	This function is uglier than ulgy.	This function updates the local
 *	backscroll buffer after it has been filled by backscrlCFillLocalBk().
 *	This function only graps the actual number of lines it needs to update
 *	the local backscroll page.	The reason it is complex is because the
 *	server also stores it's backscroll buffer in pages.  If this routine
 *	requires data from more than one page, it must repeatedly ask the
 *	server until the request is satisfied.	Some assumptions are made
 *	here (and in backscrlCFillLocalBk()).  First and foremost, requests
 *	are never made beyond the backscroll region.  The caller's of these
 *	routines does this right now.  Second, if x number of lines are asked
 *	for, this routine will continue to try until it meets the request.
 *	Again, the callers are smart enough not to exceed the backscroll ranges.
 *
 * ARGUMENTS:
 *	HHTERM		hhTerm			-- internal terminal handle
 *	int 		iScrlInc		-- the # of lines and the direction scrolled
 *	int 		yBeg			-- depends on sType
 *	int 		iType			-- if BKPOS_THUMBPOS, yBeg is the thumb pos.
 *								   if BKPOS_ABSOLUTE, yBeg is absolute pos.
 *
 * RETURNS:
 *	nothing
 *
 */
void termGetBkLines(const HHTERM hhTerm, const int iScrlInc, int yBeg, int sType)
	{
	int 			i, j, k, l;
	int 			iWant, iGot;
	int 			iOffset;
	ECHAR			*pachTxt,		// terminal buffer
					*pachBkTxt;		// engine buffer
	const HBACKSCRL hBackscrl = sessQueryBackscrlHdl(hhTerm->hSession);

	if (abs(iScrlInc) > hhTerm->iPhysicalBkRows)
		{
		termFillBk(hhTerm, yBeg);
		return;
		}

	// Get needed backscroll text from server

	if (iScrlInc < 0)
		{
		assert(sType != BKPOS_ABSOLUTE);
		i = iScrlInc;

		// l is a wrap counter and is calculated for speed.
		//
		l = hhTerm->iNextBkLn = (hhTerm->iNextBkLn +
			hhTerm->iPhysicalBkRows + iScrlInc) % hhTerm->iPhysicalBkRows;
		}

	else
		{
		if (sType == BKPOS_THUMBPOS)
			{
			yBeg += hhTerm->iTermHite - iScrlInc;
			assert(yBeg < 0);
			}

		i = 0;
		}

	// Since backscroll memory is pages we have to make multiple requests.
	//
	for (iWant=abs(iScrlInc), iGot=0 ; iWant > 0 ; iWant-=iGot, yBeg+=iGot)
		{
		if (backscrlGetBkLines(hBackscrl, yBeg, iWant, &iGot, &pachBkTxt,
				&iOffset) == FALSE)
			{
			return;
			}

		pachBkTxt += iOffset;

		// Apply text to backscroll buffer

		if (iScrlInc < 0)
			{
			for (k=0 ; (i < 0)	&&	(k < iGot) ; ++i, ++k)
				{
				pachTxt = hhTerm->fplpstrBkTxt[l];

				for (j = 0 ; j < MAX_EMUCOLS && *pachBkTxt != ETEXT('\n') ; ++j)
					{
					assert(*pachBkTxt);
					*pachTxt++ = *pachBkTxt++;
					}

				for ( ; j < MAX_EMUCOLS ; ++j)
					*pachTxt++ = ' ';

				pachBkTxt += 1;  // Blow past newline marker...

				if (++l >= hhTerm->iPhysicalBkRows)
					l = 0;
				}
			}

		else
			{
			for (k=0 ; (i < iScrlInc)  &&  (k < iGot) ; ++i, ++k)
				{
				pachTxt = hhTerm->fplpstrBkTxt[hhTerm->iNextBkLn];

				for (j = 0 ; j < MAX_EMUCOLS && *pachBkTxt != ETEXT('\n') ; ++j)
					{
					assert(*pachBkTxt != ETEXT('\0'));
					*pachTxt++ = *pachBkTxt++;
					}

				for ( ; j < MAX_EMUCOLS ; ++j)
					*pachTxt++ = ETEXT(' ');

				if (++hhTerm->iNextBkLn >= hhTerm->iPhysicalBkRows)
					hhTerm->iNextBkLn = 0;

				pachBkTxt += 1;  // Blow past newline marker...
				}
			}
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termFillBk
 *
 * DESCRIPTION:
 *	This routine fills an entire local (refering a view) backscroll buffer.
 *	It is called only three times.	Whenever the transistion is made from
 *	active backscrolling to static backscrolling.  When the Scroll
 *	increment during a static backscroll operation is greater than the size
 *	of the view (and therefore the number of lines in the backscroll buffer
 *	as denoted by hhTerm->iPhysicalBkRows).  And when the terminal window
 *	is resized.
 *
 * ARGUMENTS:
 *	HHTERM		hhTerm			-- internal terminal handle
 *	int 		iTermHite		-- # of rows that will fit on current terminal
 *	int 		iBkPos			-- where to start in backscroll
 *
 * RETURNS:
 *	void
 *
 */
void termFillBk(const HHTERM hhTerm, const int iBkPos)
	{
	int 			i, j;
	int 			iWant,
					iGot,
					yBeg;
	int 			iOffset;
	ECHAR		   **fplpstrBkTxt,
					*pachTxt,		// terminal buffer
					*pachBkTxt;		//
	const HBACKSCRL hBackscrl = sessQueryBackscrlHdl(hhTerm->hSession);

	if (hhTerm->iTermHite > hhTerm->iMaxPhysicalBkRows)
		{
		if (termReallocBkBuf(hhTerm, hhTerm->iTermHite) != 0)
			{
			assert(FALSE);
			return;
			}
		}

	/* --- Empty the rest of terminal's backscroll buffer --- */

	for (i = 0 ; i < hhTerm->iPhysicalBkRows ; ++i)
		ECHAR_Fill(hhTerm->fplpstrBkTxt[i], ETEXT(' '), MAX_EMUCOLS);

	/* --- Grab whatever we can from the engine's backscroll buffer --- */

	hhTerm->iPhysicalBkRows = hhTerm->iTermHite;
	hhTerm->iNextBkLn = 0;

    // mrw: 2/29/96 - moved the check for an empty buffer to past where 
    // the iPhyscialRows gets set.
    //
	if (hhTerm->iBkLines == 0)
		return;

	//*yBeg = iBkPos;
	yBeg = max(-hhTerm->iBkLines, iBkPos);
	iWant = min(hhTerm->iTermHite, abs(yBeg));
	fplpstrBkTxt = hhTerm->fplpstrBkTxt + (hhTerm->iTermHite - iWant);

	for (iGot=0 ; iWant > 0 ; iWant-=iGot, yBeg+=iGot)
		{
		if (backscrlGetBkLines(hBackscrl, yBeg, iWant, &iGot, &pachBkTxt,
				&iOffset) == FALSE)
			{
			return;
			}

		pachBkTxt += iOffset;

		for (i = 0 ; i < iGot ; ++i, ++fplpstrBkTxt)
			{
			pachTxt = *fplpstrBkTxt;

			for (j = 0 ; j < MAX_EMUCOLS && *pachBkTxt != ETEXT('\n') ; ++j)
				{
				assert(*pachBkTxt);
				*pachTxt++ = *pachBkTxt++;
				}

			for ( ; j < MAX_EMUCOLS ; ++j)
				*pachTxt++ = ETEXT(' ');

			pachBkTxt += 1;
			}
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termReallocBkBuf
 *
 * DESCRIPTION:
 *	This happens when the user changes to a smaller font which allows more
 *	rows on the screen.
 *
 * ARGUMENTS:
 *	hhTerm	- private terminal handle
 *	iLines	- number of lines to realloc
 *
 * RETURNS:
 *	0=OK, 1=error
 *
 */
static int termReallocBkBuf(const HHTERM hhTerm, const int iLines)
	{
	int i;

	if (iLines < hhTerm->iMaxPhysicalBkRows)
		return 0;

	if ((hhTerm->fplpstrBkTxt = realloc(hhTerm->fplpstrBkTxt,
			(unsigned int)iLines * sizeof(ECHAR *))) == 0)
		{
		return 1;
		}

	for (i = hhTerm->iMaxPhysicalBkRows ; i < iLines ; ++i)
		{
		if ((hhTerm->fplpstrBkTxt[i] = malloc(MAX_EMUCOLS * sizeof(ECHAR)))
				== 0)
			{
			assert(FALSE);
			return 1;
			}

		ECHAR_Fill(hhTerm->fplpstrBkTxt[i], ETEXT(' '), MAX_EMUCOLS);
		}

	hhTerm->iMaxPhysicalBkRows = iLines;
	return 0;
	}

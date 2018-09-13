/*	File: D:\WACKER\tdll\termcur.c (Created: 26-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */
#include <windows.h>
#pragma hdrstop

#include "stdtyp.h"
#include "session.h"
#include "assert.h"
#include "timers.h"
#include <emu\emu.h>
#include "term.h"
#include "term.hh"

static void CalcHstCursorRect(const HHTERM hhTerm, const PRECT prc);
static void CalcLclCursorRect(const HHTERM hhTerm, const PRECT prc);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CalHstCursorRect (static function seen only by TERMPROC.C)
 *
 * DESCRIPTION:
 *	Figures out the bounding rectangle of the host cursor.
 *
 * ARGUMENTS:
 *	HTERM	hTerm	  - pointer to the terminal instance data
 *	PRECT	prc 	  - pointer to rectangle to fill
 *
 * RETURNS:
 *	Nothing.
 *
 */
static void CalcHstCursorRect(const HHTERM hhTerm, const PRECT prc)
	{
	int iRow;
	
	prc->bottom = ((hhTerm->ptHstCur.y + 2 - hhTerm->iVScrlPos)) * hhTerm->yChar;
	prc->top	= prc->bottom - hhTerm->iHstCurSiz;

	prc->left	= (((hhTerm->ptHstCur.x - hhTerm->iHScrlPos) * hhTerm->xChar)) +
						hhTerm->xIndent + hhTerm->xBezel;

	prc->right	= prc->left + hhTerm->xChar;

	// Check for double wide left/right pair.  If so, make the
	// cursor wider.

	iRow = (hhTerm->ptHstCur.y + hhTerm->iTopline) % MAX_EMUROWS;

	if (hhTerm->ptHstCur.x < (MAX_EMUCOLS - 1) &&
			hhTerm->fppstAttr[iRow][hhTerm->ptHstCur.x].dblwilf &&
				hhTerm->fppstAttr[iRow][hhTerm->ptHstCur.x+1].dblwirt)
		{
		prc->right += hhTerm->xChar;
		}

	// This keeps the cursor from appearing in the indent margin.

	if (prc->left <= hhTerm->xIndent)
		prc->left = prc->right = 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CalLclCursorRect (static function seen only by TERMPROC.C)
 *
 * DESCRIPTION:
 *	Figures out the bounding rectangle of the selection cursor.
 *
 * ARGUMENTS:
 *	hhTerm	 hhTerm 	- pointer to the terminal instance data
 *	PRECT	prc 	  - pointer to rectangle to fill
 *
 * RETURNS:
 *	Nothing.
 *
 */
static void CalcLclCursorRect(const HHTERM hhTerm, const PRECT prc)
	{
	prc->left = ((hhTerm->ptLclCur.x - hhTerm->iHScrlPos) * hhTerm->xChar)
		+ hhTerm->xIndent + hhTerm->xBezel;

	prc->right = prc->left + 2;
	prc->top = (hhTerm->ptLclCur.y - hhTerm->iVScrlPos) * hhTerm->yChar;
	prc->bottom = prc->top + hhTerm->yChar;
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	PaintHostCursor
 *
 * DESCRIPTION:
 *	Paints the host cursor at the position and style in hLayout.  Also
 *	Hides and shows the caret or local cursor which was not part of the
 *	orginal design here but fit in nicely.
 *
 */
void PaintHostCursor(const HHTERM hhTerm, const BOOL fOn, const HDC hdc)
	{
	RECT	rc;

	if (hhTerm->fHstCurOn == fOn)
		return;

	hhTerm->fHstCurOn = fOn;
	CalcHstCursorRect(hhTerm, &rc);
	InvertRect(hdc, &rc);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	PaintLocalCursor
 *
 * DESCRIPTION:
 *	Local meaning the seletion cursor in this case.
 *
 */
void PaintLocalCursor(const HHTERM hhTerm, const BOOL fOn, const HDC hdc)
	{
	RECT	rc;

	if (hhTerm->fCursorsLinked && hhTerm->fLclCurOn == 0)
		return;

	if (hhTerm->fLclCurOn == fOn)
		return;

	hhTerm->fLclCurOn = fOn;
	CalcLclCursorRect(hhTerm, &rc);
	InvertRect(hdc, &rc);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ShowCursors
 *
 * DESCRIPTION:
 *	Use to just show the host cursor but also shows the caret or local
 *	cursor as well.
 *
 * ARGUMENTS:
 *	HHTERM	hhTerm	- internal terminal handle.
 *
 * RETURNS:
 *	VOID
 *
 */
void ShowCursors(const HHTERM hhTerm)
	{
	const HDC hdc = GetDC(hhTerm->hwnd);

	PaintHostCursor(hhTerm, TRUE, hdc);
	PaintLocalCursor(hhTerm, TRUE, hdc);

	ReleaseDC(hhTerm->hwnd, hdc);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	HideCursors
 *
 * DESCRIPTION:
 *	Use to just hide the host cursor but also hides the caret or local
 *	cursor as well.
 *
 * ARGUMENTS:
 *	HHTERM	hhTerm	- internal terminal handle.
 *
 * RETURNS:
 *	VOID
 *
 */
void HideCursors(const HHTERM hhTerm)
	{
	const HDC hdc = GetDC(hhTerm->hwnd);

	PaintHostCursor(hhTerm, FALSE, hdc);
	PaintLocalCursor(hhTerm, FALSE, hdc);

	ReleaseDC(hhTerm->hwnd, hdc);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SetLclCurPos
 *
 * DESCRIPTION:
 *	The local cursor is the marking cursor.  This routine just positions
 *	it according to the coordinates handling any painting requirements.
 *
 * ARGUMENTS:
 *	HHTERM	hhTerm	- internal terminal handle
 *	LPPOINT ptCur	- point structure where marking cursor goes
 *
 * RETURNS:
 *	VOID
 *
 */
void SetLclCurPos(const HHTERM hhTerm, const LPPOINT lpptCur)
	{
	const HDC hdc = GetDC(hhTerm->hwnd);

	PaintLocalCursor(hhTerm, FALSE, hdc);
	hhTerm->ptLclCur = *lpptCur;
	hhTerm->fCursorsLinked = FALSE;
	PaintLocalCursor(hhTerm, TRUE, hdc);

	ReleaseDC(hhTerm->hwnd, hdc);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	LinkCursors
 *
 * DESCRIPTION:
 *	Magic little function that causes the selection cursor to link-up with
 *	the host cursor.  This happens the view has to be shifted to present
 *	the host cursor or when characters are typed at the keyboard.
 *
 * ARGUMENTS:
 *	HHTERM	 hhTerm   - internal terminal handle.
 *
 * RETURNS:
 *	void
 *
 */
void LinkCursors(const HHTERM hhTerm)
	{
	if (!hhTerm->fCursorsLinked)
		{
		const HDC hdc = GetDC(hhTerm->hwnd);

		hhTerm->fCursorsLinked = TRUE;
		PaintLocalCursor(hhTerm, FALSE, hdc);
		ReleaseDC(hhTerm->hwnd, hdc);

		hhTerm->fExtSelect = FALSE;

		MarkText(hhTerm, &hhTerm->ptBeg, &hhTerm->ptEnd, FALSE, MARK_ABS);
		hhTerm->ptBeg = hhTerm->ptEnd = hhTerm->ptHstCur;
		TestForMarkingLock(hhTerm);
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	MoveSelectionCursor
 *
 * DESCRIPTION:
 *	Moves the selection cursor by the specified amount and updates the
 *	terminal window so that the cursor is in view.	Used by kbd routines.
 *
 * ARGUMENTS:
 *	HTERM	hTerm	- handle to terminal
 *	HWND	hwnd	- terminal window
 *	INT 	x		- amount to move left or right
 *	INT 	y		- amount to move up or down
 *	BOOL	fMarking- TRUE means we are marking text.
 *
 * RETURNS:
 *	void
 *
 */
void MoveSelectionCursor(const HHTERM hhTerm,
						 const HWND hwnd,
							   int	x,
							   int	y,
							   BOOL fMarking)
	{
	int yTemp;
	POINT ptTmp;

	// Extended selection is where the user does not have to hold down
	// shift keys to select text. - mrw

	fMarking |= hhTerm->fExtSelect;

	if (fMarking == FALSE)
		UnmarkText(hhTerm);

	ptTmp = hhTerm->ptEnd;

	// This tests to see if the selection cursor is on-screen.	If the
	// selection cursor is off-screen, place it at the top of the
	// screen and then perform the operation.  This is how MicroSoft
	// Word behaves.

	if (hhTerm->ptEnd.y < hhTerm->iVScrlPos ||
			(hhTerm->ptEnd.y - hhTerm->iTermHite + 1) > hhTerm->iVScrlPos)
		{
		hhTerm->ptEnd.y = hhTerm->iVScrlPos;
		}

	hhTerm->ptEnd.x += x;
	hhTerm->ptEnd.x = max(min(hhTerm->iCols, hhTerm->ptEnd.x), 0);

	yTemp = hhTerm->ptEnd.y += y;
	hhTerm->ptEnd.y = max(min(hhTerm->iRows, hhTerm->ptEnd.y), hhTerm->iVScrlMin);

 	//mpt:1-23-98 attempt to re-enable DBCS code
#ifndef CHAR_NARROW
	termValidatePosition(hhTerm,
						x >= 0 ? VP_ADJUST_RIGHT : VP_ADJUST_LEFT,
						&hhTerm->ptEnd);
#endif

	if (fMarking == FALSE)
		hhTerm->ptBeg = hhTerm->ptEnd;

	SetLclCurPos(hhTerm, &hhTerm->ptEnd);

	// Figure out much to scroll vertically.

	if (hhTerm->ptEnd.y < hhTerm->iVScrlPos)
		y = hhTerm->ptEnd.y;

	else if ((hhTerm->ptEnd.y - hhTerm->iTermHite) >= hhTerm->iVScrlPos)
		y = hhTerm->ptEnd.y - hhTerm->iTermHite + 1;

	else
		y = hhTerm->iVScrlPos;

	// This condition occurs when we are scrolling and the Bezel is
	// present.
	//
	if (yTemp > hhTerm->ptEnd.y && y < hhTerm->iVScrlMax)
		y = hhTerm->iVScrlMax;

	// Do the scroll
	//
	if (y != hhTerm->iVScrlPos)
		SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, y), 0);

	// Figure out much to scroll horizontally
	//
	if (hhTerm->ptEnd.x < hhTerm->iHScrlPos)
		x = hhTerm->ptEnd.x;

	else if (hhTerm->ptEnd.x >= (hhTerm->iHScrlPos + hhTerm->iCols - hhTerm->iHScrlMax))
		x = hhTerm->ptEnd.x - (hhTerm->iCols - hhTerm->iHScrlMax) + 1;

	else
		x = hhTerm->iHScrlPos;

	if (x != hhTerm->iHScrlPos)
		SendMessage(hwnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, x), 0);

	// Force update to keep things smooth looking.	Note, UpdateWindow()
	// does nothing if the update rectangle is NULL.

	if (fMarking)
		MarkText(hhTerm, &ptTmp, &hhTerm->ptEnd, TRUE, MARK_XOR);

	UpdateWindow(hwnd);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CursorTimerProc
 *
 * DESCRIPTION:
 *	Multiplex timer callback routine used for cursor blinking.	Also
 *	controls blinking text.
 *
 * ARGUMENTS:
 *	pvhwnd	- terminal window handle.
 *	ltime	- contains time elapsed.
 *
 * RETURNS:
 *	void
 *
 */
void CALLBACK CursorTimerProc(void *pvhwnd, long ltime)
	{
	const HWND hwnd = (HWND)pvhwnd;

	if (GetFocus() == hwnd)
		{
		const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

		if (hhTerm->fBlink)
			{
			const HDC hdc = GetDC(hhTerm->hwnd);
			PaintHostCursor(hhTerm, hhTerm->fHstCurOn ? FALSE : TRUE, hdc);
			ReleaseDC(hhTerm->hwnd, hdc);
			}

		BlinkText(hhTerm);
		}

	return;
	}

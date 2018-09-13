/*	File: D:\WACKER\tdll\termutil.c (Created: 23-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */
#include <windows.h>
#pragma hdrstop

//#define	DEBUGSTR	1	

#include <stdlib.h>
#include <limits.h>

#include "stdtyp.h"
#include "session.h"
#include "assert.h"
#include "timers.h"
#include "chars.h"
#include <emu\emu.h>
#include "term.h"
#include "term.hh"
#include "statusbr.h"
#include <term\res.h>

static int InMiddleofWideChar(ECHAR *pszRow, int iCol);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termQuerySnapRect
 *
 * DESCRIPTION:
 *	Returns the minimum rectangle that will encompass a full terminal.
 *
 * ARGUMENTS:
 *	hhTerm	- internal terminal handle.
 *	prc 	- pointer to rect
 *
 * RETURNS:
 *	void
 *
 */
void termQuerySnapRect(const HHTERM hhTerm, LPRECT prc)
	{
	prc->left = prc->top = 0;

	prc->right = (hhTerm->iCols * hhTerm->xChar) +
		(2 * (hhTerm->xIndent + hhTerm->xBezel)) +
			(2 * GetSystemMetrics(SM_CXEDGE)) +
				GetSystemMetrics(SM_CXVSCROLL);

	prc->bottom = ((hhTerm->iRows + 2) * hhTerm->yChar) +
		(2 * GetSystemMetrics(SM_CYEDGE));
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	MarkText
 *
 * DESCRIPTION:
 *	Work horse routine that marks and unmarks text on the terminal screen.
 *	It has two methods for marking, ABSOLUTE and XOR.  ABSOLUTE sets the
 *	range of cells given by ptBeg and ptEnd to the given setting (fMark)
 *	while XOR will perform an exclusive or on the cell range.  ABSOLUTE
 *	is used to unmark cells mostly.
 *
 * ARGUMENTS:
 *	HTERM	hTerm	- handle to a terminal
 *	LPPOINT ptBeg	- one end of the marking range
 *	LPPOINT ptEnd	- the other end of the marking range
 *	BOOL	fMark	- new marking state of cells
 *	SHORT	fMarkingMethod - MARK_ABS or MARK_XOR
 *
 *
 * RETURNS:
 *	VOID
 *
 */
void MarkText(const HHTERM	   hhTerm,
			  const LPPOINT    ptBeg,
			  const LPPOINT    ptEnd,
			  const BOOL	   fMark,
			  const int 	   sMarkingMethod)
	{
	int    iOffsetBeg,
		   iOffsetEnd,
		   sTermBeg,
		   sTermEnd,
		   i, j;
	RECT   rc;
	long   yBeg, yEnd;

	//const  int iMaxCells = hhTerm->iRows * TERM_COLS;
	//
	const  int iMaxCells = MAX_EMUROWS * MAX_EMUCOLS;

	//iOffsetBeg = ((ptBeg->y - 1) * TERM_COLS) + ptBeg->x;
	//iOffsetEnd = ((ptEnd->y - 1) * TERM_COLS) + ptEnd->x;
	//
	iOffsetBeg = ((ptBeg->y - 1) * MAX_EMUCOLS) + ptBeg->x;
	iOffsetEnd = ((ptEnd->y - 1) * MAX_EMUCOLS) + ptEnd->x;


	// Check if we moved enough to actually mark something.

	if (iOffsetBeg == iOffsetEnd)
		return;

	// Determine offsets for terminal area.

	sTermBeg = min(max(iOffsetBeg, 0), iMaxCells);
	sTermEnd = min(max(iOffsetEnd, 0), iMaxCells);

	// This routine use to reference the text and attribute buffers as
	// a continous buffer.	When switching over to pointer arrays, I
	// introduced the [i/sCols][i%sCols] notation to keep from having
	// to change the entire routine.

	if (sTermBeg != sTermEnd)
		{
		//i = (min(sTermBeg, sTermEnd)
		//    + (hhTerm->iTopline * TERM_COLS)) % iMaxCells;
		//
		i = (min(sTermBeg, sTermEnd)
		    + (hhTerm->iTopline * MAX_EMUCOLS)) % iMaxCells;


		j = abs(sTermEnd - sTermBeg);

		switch (sMarkingMethod)
			{
		case MARK_XOR:
			while (j-- > 0)
				{
				if (i >= iMaxCells)
					i = 0;

				//hhTerm->fppstAttr[i/TERM_COLS][i%TERM_COLS].txtmrk
				//    ^= (unsigned)fMark;
				//
				hhTerm->fppstAttr[i/MAX_EMUCOLS][i%MAX_EMUCOLS].txtmrk
				    ^= (unsigned)fMark;

				i += 1;
				}
			break;

		case MARK_ABS:
			while (j-- > 0)
				{
				if (i >= iMaxCells)
					i = 0;

				//hhTerm->fppstAttr[i/TERM_COLS][i%TERM_COLS].txtmrk
				//    = (unsigned)fMark;
				//
				hhTerm->fppstAttr[i/MAX_EMUCOLS][i%MAX_EMUCOLS].txtmrk
				    = (unsigned)fMark;


				i += 1;
				}
			break;

		default:
			assert(0);
			break;
			}
		}

	TestForMarkingLock(hhTerm);

	// Invalidate the rectangle covering the marked region

	yBeg = min(ptBeg->y, ptEnd->y);
	yEnd = max(ptBeg->y, ptEnd->y);

	rc.left = hhTerm->xIndent + (hhTerm->iHScrlPos ? 0 : hhTerm->xBezel);

	//rc.right = min((hhTerm->xChar * hhTerm->iCols) + hhTerm->xIndent +
	//			(hhTerm->iHScrlPos ? 0 : hhTerm->xBezel), hhTerm->cx);
	//
	rc.right = min((hhTerm->xChar * MAX_EMUCOLS) + hhTerm->xIndent +
				(hhTerm->iHScrlPos ? 0 : hhTerm->xBezel), hhTerm->cx);
	rc.top = (yBeg - hhTerm->iVScrlPos) * hhTerm->yChar;
	rc.bottom = (yEnd + 1 - hhTerm->iVScrlPos) * hhTerm->yChar;

	InvalidateRect(hhTerm->hwnd, &rc, FALSE);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	MarkTextAll
 *
 * DESCRIPTION:
 *	Marks all of the text on the terminal screen and backscroll buffer.
 *
 * ARGUMENTS:
 *	hhTerm	- private terminal handle
 *
 * RETURNS:
 *	void
 *
 */
void MarkTextAll(HHTERM hhTerm)
	{
	MarkText(hhTerm, &hhTerm->ptBeg, &hhTerm->ptEnd, FALSE, MARK_ABS);

	hhTerm->ptBeg.x = 0;
	hhTerm->ptBeg.y = hhTerm->iVScrlMin;

	//iEmuId = EmuQ(sessQueryEmuHdl(hhTerm->hSession)
	hhTerm->ptEnd.x = hhTerm->iCols;
	hhTerm->ptEnd.y = hhTerm->iRows;

	MarkText(hhTerm, &hhTerm->ptBeg, &hhTerm->ptEnd, TRUE, MARK_ABS);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	UnmarkText
 *
 * DESCRIPTION:
 *	Unmarks all text on the terminal screen
 *
 * ARGUMENTS:
 *	hhTerm	- private terminal handle
 *
 * RETURNS:
 *	void
 *
 */
void UnmarkText(const HHTERM hhTerm)
	{
	MarkText(hhTerm, &hhTerm->ptBeg, &hhTerm->ptEnd, FALSE, MARK_ABS);
	hhTerm->ptBeg = hhTerm->ptEnd;
	TestForMarkingLock(hhTerm);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TestForMarkingLock
 *
 * DESCRIPTION:
 *	Checks to seek if hTerm->fMarkingLock should be on or off.
 *
 * ARGUMENTS:
 *	HTERM	hTerm	- handle to a terminal
 *
 * RETURNS:
 *	VOID
 *
 */
void TestForMarkingLock(const HHTERM hhTerm)
	{
	hhTerm->fMarkingLock = (memcmp(&hhTerm->ptBeg, &hhTerm->ptEnd,
		sizeof(POINT)) == 0) ?	FALSE : TRUE;

	if (hhTerm->fMarkingLock)
		sessSetSuspend(hhTerm->hSession, SUSPEND_TERMINAL_MARKING);

	else
		sessClearSuspend(hhTerm->hSession, SUSPEND_TERMINAL_MARKING);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	PointInSelectionRange
 *
 * DESCRIPTION:
 *	Tests if given point is within the range of the given beginning and
 *	ending points.	Note, pptBeg does not have to be less the pptEnd.
 *
 * ARGUMENTS:
 *	const PPOINT	ppt 	- point to test.
 *	const PPOINT	pptBeg	- one end of the range.
 *	const PPOINT	pptEnd	- other end of the range.
 *	const int		iCols	- number of columns in current emulator.
 *
 * RETURNS:
 *	TRUE if in range, else FALSE
 *
 */
BOOL PointInSelectionRange(const PPOINT ppt,
						   const PPOINT pptBeg,
						   const PPOINT pptEnd,
						   const int	iCols)
	{
	long l, lBeg, lEnd;

	l = (ppt->y * iCols) + ppt->x;
	lBeg = (pptBeg->y * iCols) + pptBeg->x;
	lEnd = (pptEnd->y * iCols) + pptEnd->x;

	if (l >= min(lBeg, lEnd) && l < max(lBeg, lEnd))
		return TRUE;

	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termTranslateKey
 *
 * DESCRIPTION:
 *	Does the dirty work of translate accelator keys.
 *
 * ARGUMENTS:
 *	HTERM	hTerm	- terminal handle.
 *	HWND	hwnd	- terminal window handle.
 *	USHORT	usKey	- key code from utilGetCharacter().
 *
 * RETURNS:
 *	 TRUE if it processed char, else FALSE
 *
 */
BOOL termTranslateKey(const HHTERM hhTerm, const HWND hwnd, const KEY_T Key)
	{
	POINT		ptTmp;
	INT 		x = 0;
	STEMUSET	stEmuSet;
	BOOL		fShiftKey,
				fScrlLk;

	if (Key == 0)
		return TRUE;

	fScrlLk = GetKeyState(VK_SCROLL) & 1;
	fShiftKey = (Key & SHIFT_KEY) ? TRUE : FALSE;

	// Check to see if we use it.

	switch (Key)
		{
	/* -------------- VK_HOME ------------- */

	case VK_HOME | VIRTUAL_KEY:
	case VK_HOME | VIRTUAL_KEY | SHIFT_KEY:
	case VK_HOME | VIRTUAL_KEY | EXTENDED_KEY:
	case VK_HOME | VIRTUAL_KEY | EXTENDED_KEY | SHIFT_KEY:
		MoveSelectionCursor(hhTerm, hwnd, -hhTerm->iCols, 0, fShiftKey);
		break;

	case VK_HOME | VIRTUAL_KEY | CTRL_KEY:
	case VK_HOME | VIRTUAL_KEY | CTRL_KEY | SHIFT_KEY:
	case VK_HOME | VIRTUAL_KEY | CTRL_KEY | EXTENDED_KEY:
	case VK_HOME | VIRTUAL_KEY | CTRL_KEY | EXTENDED_KEY | SHIFT_KEY:
		MoveSelectionCursor(hhTerm, hwnd, -hhTerm->iCols,
			hhTerm->iVScrlMin - hhTerm->iRows, fShiftKey);
		break;

	case VK_HOME | VIRTUAL_KEY | ALT_KEY:
	case VK_HOME | VIRTUAL_KEY | ALT_KEY | SHIFT_KEY:
	case VK_HOME | VIRTUAL_KEY | ALT_KEY | SHIFT_KEY | CTRL_KEY:
	case VK_HOME | VIRTUAL_KEY | ALT_KEY | SHIFT_KEY | CTRL_KEY | EXTENDED_KEY:
		break;

	/* -------------- VK_END ------------- */

	case VK_END | VIRTUAL_KEY:
	case VK_END | VIRTUAL_KEY | SHIFT_KEY:
	case VK_END | VIRTUAL_KEY | EXTENDED_KEY:
	case VK_END | VIRTUAL_KEY | SHIFT_KEY | EXTENDED_KEY:
		MoveSelectionCursor(hhTerm, hwnd, hhTerm->iCols - hhTerm->ptEnd.x,
			0, fShiftKey);
		break;

	case VK_END | VIRTUAL_KEY | CTRL_KEY:
	case VK_END | VIRTUAL_KEY | CTRL_KEY | SHIFT_KEY:
	case VK_END | VIRTUAL_KEY | CTRL_KEY | EXTENDED_KEY:
	case VK_END | VIRTUAL_KEY | CTRL_KEY | SHIFT_KEY | EXTENDED_KEY:
		MoveSelectionCursor(hhTerm, hwnd, hhTerm->iCols, INT_MAX/2, fShiftKey);
		break;

	/* -------------- VK_PRIOR & VK_NEXT ------------- */

	case VK_PRIOR | VIRTUAL_KEY:
	case VK_NEXT  | VIRTUAL_KEY:
	case VK_PRIOR | VIRTUAL_KEY | SHIFT_KEY:
	case VK_NEXT  | VIRTUAL_KEY | SHIFT_KEY:
	case VK_PRIOR | VIRTUAL_KEY | EXTENDED_KEY:
	case VK_NEXT  | VIRTUAL_KEY | EXTENDED_KEY:
	case VK_PRIOR | VIRTUAL_KEY | EXTENDED_KEY| SHIFT_KEY:
	case VK_NEXT  | VIRTUAL_KEY | EXTENDED_KEY| SHIFT_KEY:
		ptTmp = hhTerm->ptEnd;

		if (fShiftKey == 0)
			UnmarkText(hhTerm);

		if (hhTerm->ptEnd.y < hhTerm->iVScrlPos ||
				(hhTerm->ptEnd.y - hhTerm->iTermHite + 1) > hhTerm->iVScrlPos
					|| !hhTerm->fLclCurOn)
			{
			x = 1; // means it is out of view.
			}

		SendMessage(hwnd, WM_VSCROLL, ((UCHAR)Key == VK_NEXT) ?
			SB_PAGEDOWN : SB_PAGEUP, 0);

		if (x)
			{
			hhTerm->ptEnd.y = hhTerm->iVScrlPos;

			if (hhTerm->fLclCurOn)
				SetLclCurPos(hhTerm, &hhTerm->ptEnd);
			}

		else
			{
			if ((UCHAR)Key == VK_NEXT)
				{
				hhTerm->ptEnd.y += hhTerm->iTermHite;
				hhTerm->ptEnd.y = min(hhTerm->iRows, hhTerm->ptEnd.y);
				}

			else
				{
				hhTerm->ptEnd.y -= hhTerm->iTermHite;
				hhTerm->ptEnd.y = max(hhTerm->iVScrlMin, hhTerm->ptEnd.y);
				}

			if (hhTerm->fLclCurOn)
				SetLclCurPos(hhTerm, &hhTerm->ptEnd);
			}

		if (!fShiftKey)
			hhTerm->ptBeg = hhTerm->ptEnd;

		if (fShiftKey)
			MarkText(hhTerm, &ptTmp, &hhTerm->ptEnd, TRUE, MARK_XOR);

		break;

	case VK_PRIOR | VIRTUAL_KEY | CTRL_KEY:
	case VK_NEXT  | VIRTUAL_KEY | CTRL_KEY:
	case VK_PRIOR | VIRTUAL_KEY | CTRL_KEY | EXTENDED_KEY:
	case VK_NEXT  | VIRTUAL_KEY | CTRL_KEY | EXTENDED_KEY:
		UnmarkText(hhTerm);

		SendMessage(hwnd, WM_HSCROLL, ((UCHAR)Key == VK_NEXT) ?
			SB_PAGEDOWN : SB_PAGEUP, 0);

		break;

	/* -------------- VK_UP ------------- */

	case VK_UP | VIRTUAL_KEY:
	case VK_UP | VIRTUAL_KEY | SHIFT_KEY:
	case VK_UP | VIRTUAL_KEY | EXTENDED_KEY:
	case VK_UP | VIRTUAL_KEY | EXTENDED_KEY | SHIFT_KEY:
		MoveSelectionCursor(hhTerm, hwnd, 0, -1, fShiftKey);
		break;

	/* -------------- VK_DOWN ------------- */

	case VK_DOWN | VIRTUAL_KEY:
	case VK_DOWN | VIRTUAL_KEY | SHIFT_KEY:
	case VK_DOWN | VIRTUAL_KEY | EXTENDED_KEY:
	case VK_DOWN | VIRTUAL_KEY | EXTENDED_KEY | SHIFT_KEY:
		MoveSelectionCursor(hhTerm, hwnd, 0, 1, fShiftKey);
		break;

	/* -------------- VK_LEFT ------------- */

	case VK_LEFT | VIRTUAL_KEY:
	case VK_LEFT | VIRTUAL_KEY | SHIFT_KEY:
	case VK_LEFT | VIRTUAL_KEY | EXTENDED_KEY:
	case VK_LEFT | VIRTUAL_KEY | EXTENDED_KEY | SHIFT_KEY:
		MoveSelectionCursor(hhTerm, hwnd, -1, 0, fShiftKey);
		break;

	/* -------------- VK_RIGHT ------------- */

	case VK_RIGHT | VIRTUAL_KEY:
	case VK_RIGHT | VIRTUAL_KEY | SHIFT_KEY:
	case VK_RIGHT | VIRTUAL_KEY | EXTENDED_KEY:
	case VK_RIGHT | VIRTUAL_KEY | EXTENDED_KEY | SHIFT_KEY:
		MoveSelectionCursor(hhTerm, hwnd, 1, 0, fShiftKey);
		break;

	/* -------------- VK_F4 ------------- */

	case VK_F4 | CTRL_KEY | VIRTUAL_KEY:
		PostMessage(sessQueryHwnd(hhTerm->hSession), WM_CLOSE, 0, 0L);
		break;

	/* -------------- VK_F8 ------------- */

	case VK_F8 | VIRTUAL_KEY:
		if (fScrlLk || hhTerm->fMarkingLock)
			{
			hhTerm->fExtSelect = !hhTerm->fExtSelect;
			break;
			}
		return FALSE;

	/* -------------- CTRL-C ------------- */

	case 0x03:
	case VK_INSERT | VIRTUAL_KEY | CTRL_KEY:
	case VK_INSERT | VIRTUAL_KEY | CTRL_KEY | EXTENDED_KEY:
		if (fScrlLk || hhTerm->fMarkingLock)
			{
			PostMessage(sessQueryHwnd(hhTerm->hSession), WM_COMMAND, IDM_COPY, 0);
			break;
			}

		return FALSE;

	/* -------------- CTRL-V ------------- */

	case 0x16:
	case VK_INSERT | VIRTUAL_KEY | SHIFT_KEY:
	case VK_INSERT | VIRTUAL_KEY | SHIFT_KEY | EXTENDED_KEY:
		if (emuQuerySettings(sessQueryEmuHdl(hhTerm->hSession),
				&stEmuSet) == 0 && stEmuSet.nTermKeys != EMU_KEYS_TERM)
			{
			PostMessage(sessQueryHwnd(hhTerm->hSession), WM_COMMAND,
				IDM_PASTE, 0);

			break;
			}

		 return FALSE;

	/* -------------- CTRL-X ------------- */
	/* -------------- CTRL-Z ------------- */

	case 0x18:
	case 0x1A:
		if (fScrlLk || hhTerm->fMarkingLock)
			return TRUE;

		return FALSE;

	/* -------------- Scroll Lock ------------- */

	case VIRTUAL_KEY | VK_SCROLL:
	case VIRTUAL_KEY | SHIFT_KEY | VK_SCROLL:
	case VIRTUAL_KEY | ALT_KEY | VK_SCROLL:
	case VIRTUAL_KEY | ALT_KEY | SHIFT_KEY | VK_SCROLL:
		if (fScrlLk)
			sessSetSuspend(hhTerm->hSession, SUSPEND_SCRLCK);
		else
			sessClearSuspend(hhTerm->hSession, SUSPEND_SCRLCK);

		PostMessage(sessQueryHwndStatusbar(hhTerm->hSession),
			SBR_NTFY_REFRESH, (WPARAM)SBR_SCRL_PART_NO, 0);

		return TRUE;

	/* -------------- Num Lock ------------- */

	case VIRTUAL_KEY | EXTENDED_KEY | VK_NUMLOCK:
	case VIRTUAL_KEY | EXTENDED_KEY | SHIFT_KEY | VK_NUMLOCK:
	case VIRTUAL_KEY | EXTENDED_KEY | ALT_KEY | VK_NUMLOCK:
	case VIRTUAL_KEY | EXTENDED_KEY | ALT_KEY | SHIFT_KEY | VK_NUMLOCK:
		PostMessage(sessQueryHwndStatusbar(hhTerm->hSession),
			SBR_NTFY_REFRESH, (WPARAM)SBR_NUML_PART_NO, 0);
		return TRUE;

	/* -------------- Caps Lock ------------- */

	case VIRTUAL_KEY | VK_CAPITAL:
	case VIRTUAL_KEY | SHIFT_KEY | VK_CAPITAL:
	case VIRTUAL_KEY | ALT_KEY | VK_CAPITAL:
	case VIRTUAL_KEY | ALT_KEY | SHIFT_KEY | VK_CAPITAL:
		PostMessage(sessQueryHwndStatusbar(hhTerm->hSession),
			SBR_NTFY_REFRESH, (WPARAM)SBR_CAPL_PART_NO, 0);
		return TRUE;

	/* -------------- Let it through based on scroll lock ------------- */

	default:
		return fScrlLk;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	MarkingTimerProc
 *
 * DESCRIPTION:
 *	Multiplex timer callback routine used for text marking
 *
 * ARGUMENTS:
 *	pvhWnd	- terminal window.
 *	lTime	- contains time elapsed.
 *
 * RETURNS:
 *	void
 *
 */
void CALLBACK MarkingTimerProc(void *pvhWnd, long lTime)
	{
	const HWND	 hwnd = (HWND)pvhWnd;
	const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	MSG 		 msg;
	POINT		 ptTemp;

	if (hhTerm->fCapture == FALSE)
		return;

	// This is TRUE if timer went off after we posted ourselves a message.
	//
	PeekMessage(&msg, hwnd, WM_TERM_SCRLMARK, WM_TERM_SCRLMARK, PM_REMOVE);

	// Because mouse messages go in the system queue and don't get put in
	// our application queue until its empty, we need to check.
	//
	if (PeekMessage(&msg, hwnd, WM_LBUTTONUP, WM_LBUTTONUP, PM_NOREMOVE) == TRUE)
		return;

	// The scrolling routines set this whenever they actually perform
	// a scroll.  So we set it FALSE here.	Then if any of the SendMessage()
	// calls below actually scroll, we know to post a message back to
	// ourselves to continue scrolling.

	hhTerm->fScrolled = FALSE;

	GetCursorPos(&ptTemp);
	MapWindowPoints(GetDesktopWindow(), hwnd, &ptTemp, 1);

	/* -------------- We control the horizontal ------------- */

	if (ptTemp.x > hhTerm->cx)
		SendMessage(hwnd, WM_HSCROLL, SB_LINEDOWN, 0);

	else if (ptTemp.x < 0)
		SendMessage(hwnd, WM_HSCROLL, SB_LINEUP, 0);

	/* -------------- We control the vertical ------------- */

	if (ptTemp.y > hhTerm->cy)
		SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);

	else if (ptTemp.y < 0)
		SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);

	// If we scrolled, post a message back to ourselves.  Do this because
	// the timer resolution is not short enough to produce a fast, smooth
	// scrolling effect.  Ideally, it would be better to drive this
	// entirely from timer intervals, but its too slow compared to other
	// apps.

	if (hhTerm->fScrolled)
		{
		SendMessage(hwnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(ptTemp.x, ptTemp.y));
		UpdateWindow(hwnd);
		//Sleep(10); // so we don't scroll too fast on fast machines.
		PostMessage(hwnd, WM_TERM_SCRLMARK, 0, 0);
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termSetClrAttr
 *
 * DESCRIPTION:
 *	Called as a result of the emulator notifying the terminal that
 *	it's clear attribute has changed.  This function calls the
 *	appropriate emulator call to get the attribute and then
 *	sets appropriate terminal variables.
 *
 * ARGUMENTS:
 *	hhTerm	- private terminal handle.
 *
 * RETURNS:
 *	void
 *
 */
void termSetClrAttr(const HHTERM hhTerm)
	{
	HBRUSH hBrush;
	STATTR stAttr;

	emuQueryClearAttr(sessQueryEmuHdl(hhTerm->hSession), &stAttr);

	hhTerm->crTerm = hhTerm->pacrEmuColors[(stAttr.revvid) ?
		stAttr.txtclr : stAttr.bkclr];

	if ((hBrush = CreateSolidBrush(hhTerm->crTerm)) == 0)
		return;

	if (hhTerm->hbrushTerminal)
		DeleteObject(hhTerm->hbrushTerminal);

	hhTerm->hbrushTerminal = hBrush;
	InvalidateRect(hhTerm->hwnd, 0, FALSE);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	BlinkText
 *
 * DESCRIPTION:
 *	Routine to toggle blinking-attribute cells on and off.
 *
 * ARGUMENTS:
 *	HWND	hwnd	- terminal window handle.
 *
 * RETURNS:
 *	VOID
 *
 */
void BlinkText(const HHTERM hhTerm)
	{
	int 		i, j, k;
	DWORD		dwTime;
	RECT		rc;
	BOOL		fUpdate,
				fBlinks = FALSE;
	const int m = hhTerm->iRows;		// for speed
	const int n = hhTerm->iCols;		// for speed


	// hhTerm->iBlink is a tristate variable.  If it is zero,
	// there are no blink attributes in the image and we can exit
	// immediately (this is an optimization).  Otherwise, we toggle
	// hhTerm->iBlink between -1 and 1, invalidate only those areas
	// that have blinks, and paint.

	if (hhTerm->iBlink == 0)
		return;

	hhTerm->iBlink = (hhTerm->iBlink == -1) ? 1 : -1;
	dwTime = GetTickCount();

	for (i = 0 ; i < m ; ++i)
		{
		const int r = (i + hhTerm->iTopline) % MAX_EMUROWS;

		if (hhTerm->abBlink[r] == 0)
			continue;

		// Don't let this routine gobble-up to much time.  If we can't
		// paint all the blinks, we can't paint all the blinks.

		if ((GetTickCount() - dwTime) >= (DWORD)(hhTerm->uBlinkRate/2))
			return;

		for (j = 0, fUpdate = FALSE ; j < n ; ++j)
			{
			if (hhTerm->fppstAttr[r][j].blink)
				{
				for (k = j, j += 1 ; j < n ; ++j)
					{
					if (hhTerm->fppstAttr[r][j].blink == 0)
						break;
					}

				rc.left = ((k - hhTerm->iHScrlPos) * hhTerm->xChar) + hhTerm->xIndent + hhTerm->xBezel;
				rc.right = rc.left + ((j - k) * hhTerm->xChar);

				rc.top = (i + 1 - hhTerm->iVScrlPos) * hhTerm->yChar;

				//rc.top = ((((i + m - hhTerm->iTopline) % m) + 1)
				//	  - hhTerm->iVScrlPos) * hhTerm->yChar;

				rc.bottom = rc.top + hhTerm->yChar;
				InvalidateRect(hhTerm->hwnd, &rc, FALSE);
				fBlinks = fUpdate = TRUE;
				}
			}

		// Should draw here.  Reason: if line 1 had a blinker and line 24
		// had a blinker, we would have to repaint the whole terminal window
		// since windows combines invalid regions.

		if (fUpdate)
			UpdateWindow(hhTerm->hwnd);

		hhTerm->abBlink[r] = (BYTE)fUpdate;
		}

	if (fBlinks == FALSE)
		hhTerm->iBlink = 0;

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	RefreshTernWindow
 *
 * DESCRIPTION:
 *	Calls the WM_SIZE code for terminal window
 *
 * ARGUMENTS:
 *	hwnd	- Terminal Window to refresh
 *
 * RETURNS:
 *	void
 *
 */
void RefreshTermWindow(const HWND hwndTerm)
	{
	RECT rc;
	const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwndTerm, GWLP_USERDATA);

	if (hhTerm) // need to check validatity of handle. mrw:3/1/95
		{
        TP_WM_SIZE(hhTerm->hwnd, 0, 0, 0); // mrw:11/3/95
		GetClientRect(hwndTerm, &rc);
		TP_WM_SIZE(hhTerm->hwnd, 0, rc.right, rc.bottom);
		InvalidateRect(hwndTerm, 0, FALSE);
		}
	return;
	}

//mpt:1-23-98 attempt to re-enable DBCS code
//#if 0
#ifndef CHAR_NARROW
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termValidatePosition
 *
 * DESCRIPTION:
 *	Checks the marking cursor position information in the HHTERM struct and
 *	determines if it is valid by checking the underlying characters to see if
 *	the cursor would split a double byte character.  It optionally adjusts
 *	the position to a valid position.
 *
 * ARGUMENTS:
 *	hhTerm				-- internal terminal data structure
 *	nAdjustmentMode		-- one of the following values:
 *								VP_NO_ADJUSTMENT
 *								VP_ADJUST_RIGHT
 *								VP_ADJUST_LEFT
 *
 * RETURNS:
 *	TRUE if the input values are OK, otherwise FALSE.  Note that the adjust
 *	parameter and any actions performed do not alter the return value.
 *
 */
BOOL termValidatePosition(const HHTERM	hhTerm,
						  const int		nAdjustmentMode,
						  		POINT	*pLocation)
	{
	BOOL bRet = TRUE;

	if (pLocation->y <= 0)
		return TRUE;
	
	if ( hhTerm->fppstAttr[pLocation->y - 1][pLocation->x].wirt == TRUE )
		{
		switch (nAdjustmentMode)
			{
			case VP_ADJUST_RIGHT:
				if ( pLocation->x < hhTerm->iCols )
					{
					DbgOutStr("incrementing %d\r\n", pLocation->x, 0,0,0,0);
					pLocation->x++;
					bRet = FALSE;
					}
				break;

			case VP_ADJUST_LEFT:
				if (pLocation->x > 0)
					{
					DbgOutStr("decrementing %d\r\n", pLocation->x, 0,0,0,0);
					pLocation->x--;
					bRet = FALSE;
					}
				break;

			default:
				break;
			}
		}
	
	return bRet;
	}
#endif // !CHAR_NARROW

#if 0 //DEADWOOD
	BOOL bRet = TRUE;
	int bLeadByteSeen;
	int nOffset;
	LONG lIndx;
	LPTSTR pszStr = (LPTSTR)NULL;

	/*
	 * This function just doesn't work correctly.  The reason for that is
	 * that I don't understand how the lines are organized and indexed in
	 * the terminal.  For now, this works OK if you just have a few lines
	 * on the terminal screen for testing.  Nothing in the backscroll.  I
	 * have no idea what other problems it has.
	 *
	 * DLW (17-Aug-1994)
	 *
	 * OK, I have made some attempt to get this stuff working correctly.
	 * At least it doesn't seg anymore and seems to work most of the time.
	 * I know that it still screws up when the scroll numbers are not just
	 * right, so it is pretty obvious that it still is not what it should
	 * be. So, it still needs to be checked by someone who knows what they
	 * are doing (which leaves me out).
	 *
	 * DLW (18-Aug-1994)
	 */

	nOffset = 0;
	if (pLocation->y <= 0)
		{
		nOffset  = hhTerm->iPhysicalBkRows + hhTerm->iNextBkLn + pLocation->y;
		nOffset %= hhTerm->iPhysicalBkRows;

		pszStr = (LPTSTR)hhTerm->fplpstrBkTxt[nOffset];
		}
	else if (pLocation->y > 0)
		{
		nOffset = pLocation->y - 1;

		pszStr = (LPTSTR)hhTerm->fplpstrTxt[nOffset];
		}

	assert(pszStr);

	if (pszStr == (LPTSTR)NULL)
		return FALSE;

	/*
	 * We need to loop thru the string and check if the last character before
	 * the specified character is a DBCS lead byte.  If it is, we return FALSE
	 * and perform any requested adjustment.
	 */

	bLeadByteSeen = FALSE;
	for (lIndx = 0; lIndx < pLocation->x; lIndx += 1)
		{
		if (bLeadByteSeen)
			{
			/* The rule is that a lead byte can NEVER follow a lead byte */
			bLeadByteSeen = FALSE;
			}
		else
			{
			bLeadByteSeen = IsDBCSLeadByte(pszStr[lIndx]);
			}
		}

	if (lIndx == pLocation->x)
		{
		bRet = !bLeadByteSeen;
		}

	if (!bRet)
		{
		switch (nAdjustmentMode)
			{
			case VP_ADJUST_RIGHT:
				DbgOutStr("incrementing %d\r\n", pLocation->x, 0,0,0,0);
				/* TODO: range check this first */
				pLocation->x += 1;
				break;

			case VP_ADJUST_LEFT:
				DbgOutStr("decrementing %d\r\n", pLocation->x, 0,0,0,0);
				/* TODO: range check this first */
				pLocation->x -= 1;
				break;

			default:
				break;
			}
		}
	return bRet;
	}
#endif


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termGetLogFont
 *
 * DESCRIPTION:
 *	This is here to handle a problem with calling send message in
 *	the minitel load routines.
 *
 * ARGUMENTS:
 *	hwndTerm	- Terminal Window
 *
 * RETURNS:
 *	0=OK
 *
 * AUTHOR: mrw,2/21/95
 *
 */
int termGetLogFont(const HWND hwndTerm, LPLOGFONT plf)
	{
	HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwndTerm, GWLP_USERDATA);
	assert(plf != 0);

	if (hhTerm == 0)
		return -1; // mrw:6/15/95

	*plf = hhTerm->lf;
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	termSetLogFont
 *
 * DESCRIPTION:
 *	This is here to handle a problem with calling send message in
 *	the minitel load routines.
 *
 * ARGUMENTS:
 *	hwndTerm	- Terminal Window
 *
 * RETURNS:
 *	0=OK
 *
 * AUTHOR: mrw,2/21/95
 *
 */
int termSetLogFont(const HWND hwndTerm, LPLOGFONT plf)
	{
	HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwndTerm, GWLP_USERDATA);
	assert(plf != 0);

	if (hhTerm == 0) // mrw,3/2/95
		return -1;

	hhTerm->lfHold = *plf;
	PostMessage(hwndTerm, WM_TERM_KLUDGE_FONT, 0, 0);

	return 0;
	}


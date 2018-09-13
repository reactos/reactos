/*	File: D:\WACKER\tdll\termmos.c (Created: 26-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */
//#define DEBUGSTR 1

#include <windows.h>
#pragma hdrstop

#include <stdlib.h>

#include "stdtyp.h"
#include "assert.h"
#include "session.h"
#include "timers.h"
#include "cloop.h"
#include "tchar.h"
#include <emu\emu.h>
#include "term.h"
#include "term.hh"

static int InMiddleofWideChar(ECHAR *pszRow, int iCol);


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TP_WM_LBTNDN
 *
 * DESCRIPTION:
 *	Message handler for left mouse button down.
 *
 * ARGUMENTS:
 *	hwnd	- terminal window handle
 *	iFlags	- mouse flags from message
 *	xPos	- x position from message
 *	yPos	- y position from message
 *
 * RETURNS:
 *	void
 *
 */
void TP_WM_LBTNDN(const HWND hwnd, const unsigned uFlags,
				  const int xPos, const int yPos)
	{
	MSG msg;
	POINT ptTemp;
	unsigned i, t;
#ifndef CHAR_NARROW
	int iRow;
#endif
	const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	// Need to wait here for the length of a double-click to see
	// if we are going to receive a double-click.

	i = GetDoubleClickTime();

	for (i += t = GetTickCount() ;	t < i ; t = GetTickCount())
		{
		// In a double-click sequence, we get a WM_LBUTTONUP message.
		// We need to avoid checking for WM_LBUTTONUP.	I tried to
		// do this by removing messages from the queue but things
		// got very sticky.

		if (PeekMessage(&msg, hwnd, WM_MOUSEFIRST, WM_MOUSELAST,
				PM_NOYIELD | PM_NOREMOVE))
			{
			if (msg.message == WM_LBUTTONDBLCLK)
				return;

			if (msg.message == WM_LBUTTONUP)
				continue;

			break;
			}
		}

	if (GetKeyState(VK_SHIFT) >= 0 || !hhTerm->fMarkingLock)
		{
		MarkText(hhTerm, &hhTerm->ptBeg, &hhTerm->ptEnd, FALSE, MARK_ABS);

		// Get new text caret position/marking region.

		hhTerm->ptBeg.x = hhTerm->ptEnd.x =
			min((xPos - hhTerm->xIndent - hhTerm->xBezel + (hhTerm->xChar/2))
					/ hhTerm->xChar + hhTerm->iHScrlPos,
				hhTerm->iHScrlPos + (hhTerm->iCols - hhTerm->iHScrlMax));

		hhTerm->ptBeg.y = hhTerm->ptEnd.y =
			min(yPos / hhTerm->yChar + hhTerm->iVScrlPos,
				hhTerm->iVScrlPos + hhTerm->iTermHite - 1);

	 	//mpt:1-23-98 attempt to re-enable DBCS code
#ifndef CHAR_NARROW
		termValidatePosition(hhTerm, VP_ADJUST_LEFT, &hhTerm->ptBeg);
		termValidatePosition(hhTerm, VP_ADJUST_LEFT, &hhTerm->ptEnd);
#endif
	}

	else
		{
		// Shift key in conjunction with LBUTTONDOWN allows user
		// to adjust selection area.

		ptTemp = hhTerm->ptEnd;

		hhTerm->ptEnd.y =
			min(yPos / hhTerm->yChar + hhTerm->iVScrlPos,
				hhTerm->iVScrlPos + hhTerm->iTermHite - 1);

		//(hhTerm, VP_ADJUST_LEFT, &hhTerm->ptEnd);

		MarkText(hhTerm, &ptTemp, &hhTerm->ptEnd, TRUE, MARK_XOR);
		}

	TestForMarkingLock(hhTerm);

	if (hhTerm->hMarkingTimer == 0)
		{
		TimerCreate(sessQueryTimerMux(hhTerm->hSession),
			&hhTerm->hMarkingTimer, 100, MarkingTimerProc, (void *)hwnd);
		}

#ifndef CHAR_NARROW
	if (hhTerm->ptBeg.y > 0)
		{

		iRow = ((hhTerm->ptBeg.y - 1) + hhTerm->iTopline) % MAX_EMUROWS;
		if (hhTerm->fppstAttr[iRow][hhTerm->ptBeg.x].wirt == TRUE)
			{
			hhTerm->ptBeg.x--;
			hhTerm->ptEnd.x--;
			}
		}
    else
        {
		iRow = yPos / hhTerm->yChar;

		// If the backscroll buffer is not filling the entire display,
		// then we need to adjust the offset by the amount not showing.

		if (abs(hhTerm->iVScrlPos) < hhTerm->iPhysicalBkRows)
			iRow += hhTerm->iPhysicalBkRows + hhTerm->iVScrlPos;

		// Calculate the offset into the local backscroll display.

		iRow = (hhTerm->iNextBkLn + hhTerm->iPhysicalBkRows + iRow) %
				hhTerm->iPhysicalBkRows;

        if (InMiddleofWideChar(hhTerm->fplpstrBkTxt[iRow], hhTerm->ptBeg.x))
			{
			hhTerm->ptBeg.x--;
			hhTerm->ptEnd.x--;
			}
        }
#endif

	SetLclCurPos(hhTerm, &hhTerm->ptBeg);
	SetCapture(hwnd);
	sessSetSuspend(hhTerm->hSession, SUSPEND_TERMINAL_LBTNDN);
	hhTerm->fCapture = TRUE;
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TP_WM_MOUSEMOVE
 *
 * DESCRIPTION:
 *	Message handler for mouse move
 *
 * ARGUMENTS:
 *	hwnd	- terminal window handle
 *	iFlags	- mouse flags from message
 *	xPos	- x position from message
 *	yPos	- y position from message
 *
 * RETURNS:
 *	void
 *
 */
void TP_WM_MOUSEMOVE(const HWND hwnd, const unsigned uFlags,
					 const int xPos, const int yPos)
	{
	int i, l; //iRow; mrw,3/1/95
#ifndef CHAR_NARROW
	int iRow;
#endif
	POINT ptTemp, ptBeg, ptEnd;
	ECHAR *pachTxt;
	long lBeg, lEnd, lOld;
	const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (hhTerm->fCapture == FALSE)
		return;

	ptTemp = hhTerm->ptEnd;

	// Rather a subtle move here.  If we are selecting by word, we
	// want to hightlight the word as soon as we touch the character.
	// In normal highlighting, we trigger at the half way point
	// on the character.  This removes a bug where you could double
	// click on the first char of a word and not select the word.

	i = (hhTerm->fSelectByWord) ? hhTerm->xChar-1 : hhTerm->xChar/2;

	ptEnd.x =
		max(min(((xPos - hhTerm->xIndent - hhTerm->xBezel + i) /
				hhTerm->xChar) + hhTerm->iHScrlPos,
			hhTerm->iHScrlPos + (hhTerm->iCols - hhTerm->iHScrlPos)),
				hhTerm->iHScrlPos);

	ptEnd.y = (yPos / hhTerm->yChar) + hhTerm->iVScrlPos;

	// Boundary conditions need special treatment.

	if (ptEnd.y > hhTerm->iRows) // bottom of terminal
		{
		ptEnd.y = hhTerm->iRows;
		ptEnd.x = hhTerm->iCols;
		}

	else if (ptEnd.y < hhTerm->iVScrlMin) // top of backscroll
		{
		ptEnd.y = hhTerm->iVScrlPos;
		ptEnd.x = 0;
		}

	#if 0
	{
	char ach[20];
	wsprintf(ach, "x=%d, y=%d, yPos=%d", ptEnd.x, ptEnd.y, yPos);
	SetWindowText(sessQueryHwndStatusbar(hhTerm->hSession), ach);
	}
	#endif

	// Selection by word requires more work.

	if (hhTerm->fSelectByWord)
		{
		if (ptEnd.y > 0)
			{
			i =  (ptEnd.y + hhTerm->iTopline - 1) % MAX_EMUROWS;
			pachTxt = hhTerm->fplpstrTxt[i];
			}

		else if (ptEnd.y < 0)
			{
			i = yPos / hhTerm->yChar;

			// If the backscroll buffer is not filling the entire display,
			// then we need to adjust the offset by the amount not showing.

			if (abs(hhTerm->iVScrlPos) < hhTerm->iPhysicalBkRows)
				i += hhTerm->iPhysicalBkRows + hhTerm->iVScrlPos;

			// Calculate the offset into the local backscroll display.

			i = (hhTerm->iNextBkLn + hhTerm->iPhysicalBkRows + i) %
					hhTerm->iPhysicalBkRows;

			pachTxt = hhTerm->fplpstrBkTxt[i];
			}

		else
			{
			return;
			}

		ptBeg = hhTerm->ptBeg;
		lBeg = (ptBeg.y * hhTerm->iCols) + ptBeg.x;
		lEnd = (ptEnd.y * hhTerm->iCols) + ptEnd.x;
		lOld = (ptTemp.y * hhTerm->iCols) + ptTemp.x;

		// When selecting by word, we want to keep the word we first
		// highlighted, highlighted if we change directions.  We do
		// this by swapping the beginning and end points.  Actually,
		// this can get fooled if the user moves across the the
		// transition point but not across the word but the extra
		// logic to get this case seemed more work than its worth.

		if (lEnd <= lBeg && lOld > lBeg)
			hhTerm->ptBeg = ptTemp;

		else if (lEnd >= lBeg && lOld < lBeg)
			hhTerm->ptBeg = ptTemp;

		// How we select things depends on the direction unfortunatly.

		if (lEnd > lBeg)
			{
			if ((l = strlentrunc(pachTxt, hhTerm->iCols)) < ptEnd.x)
				{
				i = l;
				}

			else if (xPos < 0)
				{
				// Special hack when cursor is off to the
				// left of the window.

				i = 0;
				}

			else if (i = max(ptEnd.x-1, 0), pachTxt[i] == ETEXT(' '))
				{
				for (i = ptEnd.x-1 ; i > -1 ; --i)
					{
					/* This may not work correctly for DBCS characters */
					if (i == -1 || pachTxt[i] != ETEXT(' '))
						{
						i += 1;
						break;
						}
					}
				}

			else
				{
				for (i = max(ptEnd.x-1, 0) ; i < l ; ++i)
					{
					/* This may not work correctly for DBCS characters */
					if (pachTxt[i] == ETEXT(' '))
						break;
					}
				}
			}

		else // Select backwords case
			{
			if ((l = strlentrunc(pachTxt, hhTerm->iCols)) < ptEnd.x)
				{
				i = hhTerm->iCols;
				}

			else if (i = max(ptEnd.x-1, 0), pachTxt[i] == ETEXT(' '))
				{
				for (i = max(ptEnd.x-1, 0) ; i < l ; ++i)
					{
					if (pachTxt[i] != ETEXT(' '))
						break;
					}
				}

			else
				{
				for (i = ptEnd.x-1 ; i > -1 ; --i)
					{
					/* This may not work correctly for DBCS characters */
					if (pachTxt[i] == ETEXT(' '))
						{
						i += 1;
						break;
						}
					}
				}
			}

		ptEnd.x = max(i, 0);
		}

	lBeg = (ptBeg.y * hhTerm->iCols) + ptBeg.x;
	lEnd = (ptEnd.y * hhTerm->iCols) + ptEnd.x;

#ifndef CHAR_NARROW
	// How we select things depends on the direction unfortunatly.
	if (ptEnd.y > 0)
		{
		iRow = ((ptEnd.y - 1) + hhTerm->iTopline) % MAX_EMUROWS;

		if (hhTerm->fppstAttr[iRow][ptEnd.x].wirt == TRUE)
			if (lEnd > lBeg)
				{
				ptEnd.x++;
				}
			else
				{
				ptEnd.x--;
				}
		}
    else
        {
		iRow = yPos / hhTerm->yChar;

		// If the backscroll buffer is not filling the entire display,
		// then we need to adjust the offset by the amount not showing.

		if (abs(hhTerm->iVScrlPos) < hhTerm->iPhysicalBkRows)
			iRow += hhTerm->iPhysicalBkRows + hhTerm->iVScrlPos;

		// Calculate the offset into the local backscroll display.

		iRow = (hhTerm->iNextBkLn + hhTerm->iPhysicalBkRows + iRow) %
				hhTerm->iPhysicalBkRows;
        if (InMiddleofWideChar(hhTerm->fplpstrBkTxt[iRow], ptEnd.x))
            {
			if (lEnd > lBeg)
				{
				ptEnd.x++;
				}
			else
				{
				ptEnd.x--;
				}
            }
		}
#endif

	if (ptTemp.x == ptEnd.x && ptTemp.y == ptEnd.y)
		return;

	//(hhTerm, VP_ADJUST_LEFT, &ptEnd);
	hhTerm->ptEnd = ptEnd;
	SetLclCurPos(hhTerm, &ptEnd);
	MarkText(hhTerm, &ptTemp, &ptEnd, TRUE, MARK_XOR);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TP_WM_LBTNUP
 *
 * DESCRIPTION:
 *	Message handler for left mouse button up.
 *
 * ARGUMENTS:
 *	hwnd	- terminal window handle
 *	iFlags	- mouse flags from message
 *	xPos	- x position from message
 *	yPos	- y position from message
 *
 * RETURNS:
 *	void
 *
 */
void TP_WM_LBTNUP(const HWND hwnd, const unsigned uFlags,
				  const int xPos, const int yPos)
	{
	const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (hhTerm->fCapture == FALSE)
		return;

	// Let mousemove logic set the final end point.

	SendMessage(hwnd, WM_MOUSEMOVE, uFlags, MAKELONG(xPos, yPos));

	// Kill the marking timer!

	if (hhTerm->hMarkingTimer != (HTIMER)0)
		TimerDestroy(&hhTerm->hMarkingTimer);

	// Turnoff flags associated with text-marking and give back the mouse.

	ReleaseCapture();
	hhTerm->fCapture = FALSE;
	hhTerm->fSelectByWord = FALSE;
	sessClearSuspend(hhTerm->hSession, SUSPEND_TERMINAL_LBTNDN);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TP_WM_LBTNDBLCLK
 *
 * DESCRIPTION:
 *	Message handler for left mouse button double click.
 *
 * ARGUMENTS:
 *	hwnd	- terminal window handle
 *	iFlags	- mouse flags from message
 *	xPos	- x position from message
 *	yPos	- y position from message
 *
 * RETURNS:
 *	void
 *
 */
void TP_WM_LBTNDBLCLK(const HWND hwnd, const unsigned uFlags,
					  const int xPos, const int yPos)
	{
	int i, j, k, l, m;
	ECHAR *pachTxt;
	POINT ptTemp, ptBeg, ptEnd;
	const HHTERM hhTerm = (HHTERM)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	i = (yPos / hhTerm->yChar) + hhTerm->iVScrlPos;
	j = ((xPos - hhTerm->xIndent - hhTerm->xBezel) / hhTerm->xChar) + hhTerm->iHScrlPos;

	ptTemp.x = j;
	ptTemp.y = i;

	ptBeg = hhTerm->ptBeg;
	ptEnd = hhTerm->ptEnd;

	// If we double-clicked over selected text and we have the correct
	// options set, then send it to the host.

	if (hhTerm->iBtnOne == B1_COPYWORD || hhTerm->iBtnOne == B1_COPYWORDENTER)
		{
		if (PointInSelectionRange(&ptTemp, &ptBeg, &ptEnd, hhTerm->iCols))
			{
			//*CopyTextToHost(hSession,
			//*    CopyTextFromTerminal(hSession, &ptBeg, &ptEnd, FALSE));

			if (hhTerm->iBtnOne == B1_COPYWORDENTER)
				CLoopSend(sessQueryCLoopHdl(hhTerm->hSession), TEXT("\r"), 1, 0);

			return;
			}
		}

	// Nope, just figure out what to highlight.

	if (i > 0)
		{
		m =  (i + hhTerm->iTopline - 1) % MAX_EMUROWS;
		pachTxt = hhTerm->fplpstrTxt[m];
		}

	else if (i < 0)
		{
		m = yPos / hhTerm->yChar;

		// If the backscroll buffer is not filling the entire display,
		// then we need to adjust the offset by the amount not showing.

		if (abs(hhTerm->iVScrlPos) < hhTerm->iPhysicalBkRows)
			m += hhTerm->iPhysicalBkRows + hhTerm->iVScrlPos;

		// Calculate the offset into the local backscroll display.

		m = (hhTerm->iNextBkLn + hhTerm->iPhysicalBkRows + m) %
				hhTerm->iPhysicalBkRows;

		pachTxt = hhTerm->fplpstrBkTxt[m];
		}

	else
		{
		return;
		}

	// Determine where the the word starts and ends.  If the user
	// clicks on white space, do the else case below.

	if ((m = strlentrunc(pachTxt, hhTerm->iCols)) > j)
		{
		if (pachTxt[j] != ETEXT(' '))
			{
			for (k = j ; k > 0 ; --k)
				{
				/* This may not work correctly for DBCS characters */
				if (pachTxt[k] == ETEXT(' '))
					{
					k += 1;
					break;
					}
				}

			for (l = j ; l < m ; ++l)
				{
				/* This may not work correctly for DBCS characters */
				if (pachTxt[l] == ETEXT(' '))
					break;
				}

			MarkText(hhTerm, &ptBeg, &ptEnd, FALSE, MARK_ABS);

			ptBeg.x = k;
			ptBeg.y = i;
			ptEnd.x = l;
			ptEnd.y = i;

			hhTerm->ptBeg = ptBeg;
			hhTerm->ptEnd = ptEnd;

			MarkText(hhTerm, &ptBeg, &ptEnd, TRUE, MARK_ABS);

			// If we have any kind of a sending operation selected,
			// then override marking

			if (hhTerm->iBtnOne == B1_COPYWORD || hhTerm->iBtnOne == B1_COPYWORDENTER)
				{
				if (PointInSelectionRange(&ptTemp, &ptBeg, &ptEnd, hhTerm->iCols))
					{
					//* CopyTextToHost(hSession,
					//* 	CopyTextFromTerminal(hSession, &ptBeg, &ptEnd,
					//* 		FALSE));

					if (hhTerm->iBtnOne == B1_COPYWORDENTER)
						CLoopSend(sessQueryCLoopHdl(hhTerm->hSession), TEXT("\r"), 1, 0);

					return;
					}
				}

			SetLclCurPos(hhTerm, &ptEnd);

			if (hhTerm->hMarkingTimer == 0)
				{
				TimerCreate(sessQueryTimerMux(hhTerm->hSession),
					&hhTerm->hMarkingTimer, 100, MarkingTimerProc,
						(void *)hwnd);
				}

			hhTerm->fSelectByWord = TRUE;
			SetCapture(hwnd);
			hhTerm->fCapture = TRUE;
			}
		}

	else /* --- white-space case. --- */
		{
		if (hhTerm->iBtnOne == B1_COPYWORDENTER)
			CLoopSend(sessQueryCLoopHdl(hhTerm->hSession), TEXT("\r"), 1, 0);
		}

	return;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	InMiddleofWideChar
 *
 * DESCRIPTION:
 *	Determines if we are in the middle of a wide character in the row.  We can do this because:
 *              1: The image is in ECHARS, which means that we can deal with individual col. positions.
 *              2: The characters are doubled in the image, being displayed as left/right pairs, we are
 *                      just trying to find out if we are in the middle of one of these pairs
 *
 * ARGUMENTS:
 *      ECHAR *pszString - emulator/backscroll row image
 *
 * RETURNS:
 *      TRUE: we are in the middle of a character
 *      FALSE: no we are not
 *
 * AUTHOR: JFH:1/28/94 - yes this was a saturday, oh well it's cold out right now anyways.
 */
static int InMiddleofWideChar(ECHAR *pszRow, int iCol)
    {
    ECHAR *pszChar;
    int nRet = FALSE;
    int nPos;
    BOOL fDBCSFlag = FALSE;

    if (pszRow == NULL)
		{
        assert(FALSE);
        return nRet;
        }

    // If we are in col. 0, we can not be in the middle of a character
    if (iCol == 0)
        {
        return FALSE;
        }

    pszChar = pszRow;

    for (nPos = 0; nPos <= iCol ; nPos++)
        {
        if (isDBCSChar(*pszChar))
	        {
            if (fDBCSFlag)
				{
				if ((nPos == iCol) && (*(pszChar - 1) == *pszChar))
					{
                    nRet = TRUE;
                    }
				fDBCSFlag = FALSE;
				}
				else
					{
					fDBCSFlag = TRUE;
					}
            }
        else
			{
			fDBCSFlag = FALSE;
    		}

		pszChar++;
		}

	return nRet;
    }

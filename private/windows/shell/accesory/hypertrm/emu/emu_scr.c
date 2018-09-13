/*	File: D:\WACKER\emu\emu_scr.c (Created: 08-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:28p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\tdll.h>
#include <tdll\cloop.h>
#include <tdll\assert.h>
#include <tdll\capture.h>
#include <tdll\session.h>
#include <tdll\backscrl.h>
#include <tdll\print.h>
#include <tdll\update.h>
#include <tdll\tchar.h>

#include "emu.h"
#include "emu.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * backspace
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void backspace(const HHEMU hhEmu)
	{
	INT bWide = 1;
	INT	iRow = row_index(hhEmu, hhEmu->emu_currow);
	INT iCol;

	// Move the cursor back.
	//
	if (hhEmu->emu_curcol > 0)
		{
		bWide = hhEmu->emu_apAttr[iRow][hhEmu->emu_curcol - 1].wirt ? 2 : 1;
		bWide = hhEmu->emu_apAttr[iRow][hhEmu->emu_curcol].wirt ? 0 : bWide;

		(*hhEmu->emu_setcurpos)(hhEmu,
								hhEmu->emu_currow,
								hhEmu->emu_curcol - bWide);

		}

	// Now see if we need to get rid of the character.
	//
	if ((hhEmu->stUserSettings.nEmuId == EMU_TTY &&
		hhEmu->stUserSettings.fDestructiveBk) || (bWide == 0))
		{
		if (bWide == 0)
			iCol = hhEmu->emu_curcol - 1;
		else
			iCol = hhEmu->emu_curcol;	// account for wide chars this way

		hhEmu->emu_code = ETEXT(' ');
		
		(*hhEmu->emu_graphic)(hhEmu);

		(*hhEmu->emu_setcurpos)(hhEmu,
								hhEmu->emu_currow,
								iCol); //MPT:12-8-97 hhEmu->emu_curcol - 1);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void carriagereturn(const HHEMU hhEmu)
	{
	(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow, 0);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *	iRow -- the row to clear
 *		Note: Since this function calls row_index() with iRow, DO NOT call this
 *		function with a row number that was returned by a call to row_index().
 *		You'll clear the wrong row.
 *
 * RETURNS:
 *
 */
void clear_imgrow(const HHEMU hhEmu, int iRow)
	{
	register int i;
	PSTATTR pstAttr;

	iRow = row_index(hhEmu, iRow);

	ECHAR_Fill(hhEmu->emu_apText[iRow], EMU_BLANK_CHAR, (size_t)MAX_EMUCOLS);

	for (i = 0, pstAttr = hhEmu->emu_apAttr[iRow] ; i <= MAX_EMUCOLS ; ++i)
		pstAttr[i] = hhEmu->emu_clearattr;

	hhEmu->emu_aiEnd[iRow] = EMU_BLANK_LINE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void emuLineFeed(const HHEMU hhEmu)
	{
	ECHAR aechBuf[10];
	int 	iRow = row_index(hhEmu, hhEmu->emu_currow);

	if (hhEmu->print_echo)
		{
		printEchoLine(hhEmu->hPrintEcho,
			hhEmu->emu_apText[row_index(hhEmu, hhEmu->emu_currow)],
			emuRowLen(hhEmu, iRow));

		if (hhEmu->emu_code == ETEXT('\f'))
			{
			CnvrtMBCStoECHAR(aechBuf, sizeof(aechBuf), "\f", StrCharGetByteCount("\f"));
			printEchoLine(hhEmu->hPrintEcho, aechBuf, sizeof(ECHAR));
			}
		}

	if (hhEmu->mode_LNM)
		ANSI_CNL(hhEmu, 1);
	else
		ANSI_IND(hhEmu);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * emu_print
 *
 * DESCRIPTION:
 *	Prints the specified bufr. Opens a print channel if one is not
 *	already open
 *
 * ARGUMENTS:
 *	bufr -- address of bufr to print
 *	length -- number of chars to print from bufr
 *
 * RETURNS:
 *	nothing
 */
void emuPrintChars(const HHEMU hhEmu, ECHAR *bufr, int nLen)
	{
	int nIndex;
	ECHAR *tChar;

	if (nLen == 0 || bufr == 0)
		return;

	for (nIndex = 0; nIndex < nLen; nIndex++)
		{
		tChar = bufr + nIndex;
		printEchoChar(hhEmu->hPrintHost, *tChar);
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void scrolldown(const HHEMU hhEmu, int nlines)
	{
	register int row, nrows;
	int toprow, botmrow;
	int nScrlInc;

	if (nlines <= 0)
		return;

	hhEmu->scr_scrollcnt -= nlines;
	nScrlInc = nlines;

	toprow = hhEmu->top_margin;
	botmrow = hhEmu->bottom_margin;

	if (hhEmu->top_margin == 0 && hhEmu->bottom_margin == hhEmu->emu_maxrow)
		{
		hhEmu->emu_imgtop = row_index(hhEmu, -nlines);
		}

	else if (nlines < hhEmu->bottom_margin - hhEmu->top_margin + 1)
		{
		nrows = hhEmu->bottom_margin - hhEmu->top_margin + 1 - nlines;

		for (row = hhEmu->bottom_margin; nrows > 0; --nrows, --row)
			{
			int c;
			PSTATTR pstAttr, pstAttr2;

			memmove(hhEmu->emu_apText[row_index(hhEmu, row)],
				 hhEmu->emu_apText[row_index(hhEmu, row - nlines)],
						(size_t)(hhEmu->emu_maxcol+2));

			hhEmu->emu_aiEnd[row_index(hhEmu, row - nlines)] =
				hhEmu->emu_aiEnd[row_index(hhEmu, row)];

			pstAttr  = hhEmu->emu_apAttr[row_index(hhEmu, row)];
			pstAttr2 = hhEmu->emu_apAttr[row_index(hhEmu, row - nlines)];

			for (c = 0 ; c <= hhEmu->emu_maxcol ; ++c)
				pstAttr[c] = pstAttr2[c];
			}
		}

	for (row = hhEmu->top_margin; nlines > 0; --nlines, ++row)
		clear_imgrow(hhEmu, row);

	hhEmu->emu_imgrow = row_index(hhEmu, hhEmu->emu_currow);

	updateScroll(sessQueryUpdateHdl(hhEmu->hSession),
					toprow, botmrow, -nScrlInc, hhEmu->emu_imgtop, TRUE);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void scrollup(const HHEMU hhEmu, int nlines)
	{
	register INT row;
	INT 	nrows, iLen, iThisRow;
	ECHAR *lp;			/* line pointer */
	ECHAR aechBuf[10];
	INT nScrlInc;		/* needed for call to Vid routine at bottom of func */

	HBACKSCRL hBackscrl = sessQueryBackscrlHdl(hhEmu->hSession);
	HCAPTUREFILE hCapture = sessQueryCaptureFileHdl(hhEmu->hSession);

	if (nlines <= 0)
		return;

	hhEmu->scr_scrollcnt += nlines;
	nScrlInc = nlines = min(nlines,
							hhEmu->bottom_margin - hhEmu->top_margin + 1);

	for (row = hhEmu->top_margin; row < (hhEmu->top_margin + nlines); ++row)
		{
		iThisRow = row_index(hhEmu, row);
		lp = hhEmu->emu_apText[iThisRow];
		iLen = emuRowLen(hhEmu, iThisRow);

		backscrlAdd(hBackscrl, lp, iLen);

		CaptureLine(hCapture, CF_CAP_SCREENS, lp, iLen);

		printEchoScreen(hhEmu->hPrintEcho, lp, iLen);
		CnvrtMBCStoECHAR(aechBuf, sizeof(aechBuf), "\r\n", StrCharGetByteCount("\r\n"));
		printEchoScreen(hhEmu->hPrintEcho, aechBuf, sizeof(ECHAR) * 2);

		clear_imgrow(hhEmu, row);
		}

	if (hhEmu->top_margin == 0 && hhEmu->bottom_margin == hhEmu->emu_maxrow)
		{
		hhEmu->emu_imgtop = row_index(hhEmu, nlines);
		}

	else if (nlines < (hhEmu->bottom_margin - hhEmu->top_margin + 1))
		{
		nrows = hhEmu->bottom_margin - hhEmu->top_margin + 1 - nlines;

		for (row = hhEmu->top_margin; nrows > 0; --nrows, ++row)
			{
			INT c;
			PSTATTR pstAttr, pstAttr2;

			memmove(hhEmu->emu_apText[row_index(hhEmu, row)],
				 hhEmu->emu_apText[row_index(hhEmu, row + nlines)],
				 (size_t)hhEmu->emu_maxcol + 2);

			hhEmu->emu_aiEnd[row_index(hhEmu, row + nlines)] =
				hhEmu->emu_aiEnd[row_index(hhEmu, row)];

			pstAttr  = hhEmu->emu_apAttr[row_index(hhEmu, row)];
			pstAttr2 = hhEmu->emu_apAttr[row_index(hhEmu, row + nlines)];

			for (c = 0 ; c <= hhEmu->emu_maxcol ; ++c)
				pstAttr[c] = pstAttr2[c];
			}

		for (row = hhEmu->bottom_margin; nlines > 0; --nlines, --row)
			clear_imgrow(hhEmu, row);
		}

	hhEmu->emu_imgrow = row_index(hhEmu, hhEmu->emu_currow);

	updateScroll(sessQueryUpdateHdl(hhEmu->hSession),
					hhEmu->top_margin,
					hhEmu->bottom_margin,
					nScrlInc,
					hhEmu->emu_imgtop,
					TRUE);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void tab(const HHEMU hhEmu)
	{
	int col;

	col = hhEmu->emu_curcol;
	while (col <= hhEmu->emu_maxcol)
		if (hhEmu->tab_stop[++col])
			break;

	(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow, col);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void backtab(const HHEMU hhEmu)
	{
	int col;

	col = hhEmu->emu_curcol;
	while (col >= 0)
		{
		if (hhEmu->tab_stop[-col])
			break;
		}

	(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow, col);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void tabn(const HHEMU hhEmu)
	{
	int iCol;
	int iTabSize;

	iCol = hhEmu->emu_curcol;

	iTabSize = CLoopGetTabSizeIn(sessQueryCLoopHdl(hhEmu->hSession));

	while (iCol <= hhEmu->emu_maxcol)
		{
		if (++iCol % iTabSize == 0)
			break;
		}

	(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow, iCol);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emu_bell
 *
 * DESCRIPTION:
 *	 Displays the bell code.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void emu_bell(const HHEMU hhEmu)
	{
	MessageBeep(0xFFFFFFFF); // Standard Beep
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emu_clearword
 *
 * DESCRIPTION:
 *	 Clears a part of current line.
 *
 * ARGUMENTS:
 *	 fromcol -- virtual image column of 1st character to be cleared
 *   tocol -- virtual image column of last character to be cleared
 *
 * RETURNS:
 *	 nothing
 */
void emu_clearword(const HHEMU hhEmu, int fromcol, int tocol)
	{
	int c;
	STATTR stAttr;
	int old_mode_IRM;

	(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow, fromcol);
	old_mode_IRM = hhEmu->mode_IRM;
	hhEmu->mode_IRM = RESET;		   /* overwrite, not insert */
	stAttr = hhEmu->attrState[CSCLEAR_STATE];

	for (c = fromcol; c <= tocol; ++c)
		{
		hhEmu->emu_code = ETEXT(' ');
		(*hhEmu->emu_graphic)(hhEmu);
		}

	hhEmu->attrState[CSCLEAR_STATE] = stAttr;
	hhEmu->mode_IRM = old_mode_IRM;
	(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow, fromcol);
	}

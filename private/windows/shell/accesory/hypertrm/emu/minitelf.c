/*	File: D:\WACKER\emu\minitelf.c (Created: 12-Apr-1994)
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
#include <tdll\session.h>
#include <tdll\print.h>
#include <tdll\capture.h>
#include <tdll\assert.h>
#include <tdll\mc.h>
#include <tdll\update.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"
#include "minitel.hh"

static void minitel_clear_imgrow(const HHEMU hhEmu, const int row);

#if defined(INCL_MINITEL)
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelLinefeed
 *
 * DESCRIPTION:
 *	Linefeeds work differently in minitel.	In page mode we wrap to line
 *	one (not zero) when at the bottom.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void minitelLinefeed(const HHEMU hhEmu)
	{
	const PSTMTPRIVATE pstPRI = hhEmu->pvPrivate;
	const ECHAR *tp = hhEmu->emu_apText[hhEmu->emu_imgrow];
	const PSTATTR ap = hhEmu->emu_apAttr[hhEmu->emu_imgrow];

	printEchoString(hhEmu->hPrintEcho, (ECHAR *)tp,
		emuRowLen(hhEmu, hhEmu->emu_currow)); // mrw,3/1/95

	// see page 97, bottom of page
	//
	if (hhEmu->emu_currow == 0)
		{
		hhEmu->emu_charattr = pstPRI->minitel_saved_attr;

		(*hhEmu->emu_setcurpos)(hhEmu, pstPRI->minitel_saved_row,
			pstPRI->minitel_saved_col);

		pstPRI->minitelG1Active = pstPRI->minitel_saved_minitelG1Active;
		pstPRI->stLatentAttr = pstPRI->saved_stLatentAttr;

		pstPRI->minitelUseSeparatedMosaics =
			pstPRI->saved_minitelUseSeparatedMosaics;
		}

	else if (hhEmu->emu_currow == hhEmu->bottom_margin)
		{
		if (pstPRI->fScrollMode)
			minitel_scrollup(hhEmu, 1);

		else
			(*hhEmu->emu_setcurpos)(hhEmu, 1, hhEmu->emu_curcol);
		}

	else
		{
		(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow + 1,
			hhEmu->emu_curcol);
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelBackspace
 *
 * DESCRIPTION:
 *	Backspaces are goofy.  They wrap to the previous line.	In scroll mode
 *	they cause scrolling if in line 1
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelBackspace(const HHEMU hhEmu)
	{
	const PSTMTPRIVATE pstPRI = hhEmu->pvPrivate;

	if (hhEmu->emu_curcol > 0)
		{
		(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow,
			hhEmu->emu_curcol-1);
		}

	else if (hhEmu->emu_currow == 1)
		{
		if (pstPRI->fScrollMode)
			{
			minitel_scrolldown(hhEmu, (hhEmu->emu_charattr.dblhilo) ? 2 : 1);
			}

		else
			{
			(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_maxrow,
				hhEmu->emu_maxcol);
			}
		}

	else
		{
		(*hhEmu->emu_setcurpos)(hhEmu,	hhEmu->emu_currow-1,
			hhEmu->emu_maxcol);
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelVerticalTab
 *
 * DESCRIPTION:
 *	Vertical tabs work differently.  They move the cursor up and wrap or
 *	scroll depending on the mode.
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelVerticalTab(const HHEMU hhEmu)
	{
	const PSTMTPRIVATE pstPRI = hhEmu->pvPrivate;

	// VT sequence not available in row 0
	//
	if (hhEmu->emu_currow == 0)
		return;

	if (hhEmu->emu_currow == 1)
		{
		if (pstPRI->fScrollMode)
			{
			minitel_scrolldown(hhEmu, (hhEmu->emu_charattr.dblhilo) ? 2 : 1);
			}

		else
			{
			(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_maxrow,
				hhEmu->emu_curcol);
			}
		}

	else
		{
		(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow-1,
			hhEmu->emu_curcol);
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelCursorUp
 *
 * DESCRIPTION:
 *	Moves cursor up n rows but not into row 00
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelCursorUp(const HHEMU hhEmu)
	{
	int nlines, row;

	// CSI sequences not available in row 0
	//
	if (hhEmu->emu_currow == 0)
		return;

	nlines = hhEmu->num_param[hhEmu->num_param_cnt];

	if (nlines < 1)
		nlines = 1;

	row = hhEmu->emu_currow;
	row -= nlines;

	if (row < 1)
		row = 1;

	(*hhEmu->emu_setcurpos)(hhEmu, row, hhEmu->emu_curcol);

	ANSI_Pn_Clr(hhEmu);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelCursorDirect
 *
 * DESCRIPTION:
 *	Moves cursor to specified coordinates but not row 00
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelCursorDirect(const HHEMU hhEmu)
	{
	int row, col;

	// CSI functions not available in row 0
	//
	if (hhEmu->emu_currow == 0)
		return;

	row = hhEmu->num_param[0];
	col = hhEmu->num_param_cnt > 0 ? hhEmu->num_param[1] : 0;

	if (row < 1)
		row = 1;

	if (col < 1)
		col = 1;

	if (row > hhEmu->emu_maxrow + 1)
		row = hhEmu->emu_maxrow + 1;

	if (col > hhEmu->emu_maxcol + 1)
		col = hhEmu->emu_maxcol + 1;

	// Again, can't go to row 00 with this call.

	(*hhEmu->emu_setcurpos)(hhEmu, row, col - 1);

	ANSI_Pn_Clr(hhEmu);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelFormFeed
 *
 * DESCRIPTION:
 *	Clears rows 1 thru 24 leaving row 00 alone.
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelFormFeed(const HHEMU hhEmu)
	{
	(*hhEmu->emu_setcurpos)(hhEmu, 1, 0);
	minitelClearScreen(hhEmu, 0);
	minitelReset(hhEmu);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelClearScreen
 *
 * DESCRIPTION:
 *	Works similar to the standard function but also has to clear the
 *	latent attribute and all serial attributes.
 *
 * ARGUMENTS:
 *	int iHow	- dirction to clear screen.
 *
 * RETURNS:
 *	void
 *
 */
void minitelClearScreen(const HHEMU hhEmu, const int iHow)
	{
	#define BLACK_MOSAIC ETEXT('\xff')
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	int  i;
	int  r;
	PSTMINITEL pstMT;
	STMINITEL stMT;
	ECHAR *pText;
	PSTATTR pstAttr;
	STATTR	stAttr;

	// CSI sequences not available in row 0
	//
	if (hhEmu->emu_currow == 0)
		return;

	memset(&stMT, 0, sizeof(stMT));
    stMT.ismosaic = 1;

	memset(&stAttr, 0, sizeof(stAttr));
	stAttr.txtclr = VC_BRT_WHITE;
	stAttr.bkclr = VC_BLACK;

	switch (iHow)
		{
	case 0: 	// cursor to end of screen inclusive
	default:
		pstMT = pstPRI->apstMT[hhEmu->emu_imgrow];
		pText = hhEmu->emu_apText[hhEmu->emu_imgrow];
		pstAttr = hhEmu->emu_apAttr[hhEmu->emu_imgrow];

		for (i = hhEmu->emu_curcol ; i < MAX_EMUCOLS ; ++i)
			{
			*pstMT++ = stMT;
			*pText++ = BLACK_MOSAIC;
			*pstAttr++ = stAttr;
			}

		for (r = hhEmu->emu_currow+1 ; r < MAX_EMUROWS ; ++r)
			{
			i = row_index(hhEmu, r);
			pstMT = pstPRI->apstMT[i];
			pText = hhEmu->emu_apText[i];
			pstAttr = hhEmu->emu_apAttr[i];

			for (i = 0 ; i < MAX_EMUCOLS ; ++i)
				{
				*pstMT++ = stMT;
				*pText++ = BLACK_MOSAIC;
				*pstAttr++ = stAttr;
				}
			}

		updateLine(sessQueryUpdateHdl(hhEmu->hSession), hhEmu->emu_currow,
										hhEmu->emu_maxrow);

		hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = hhEmu->emu_curcol;
		break;

	case 1: 	// beginning of screen to cursor inclusive
		for (r = 1 ; r < hhEmu->emu_currow ; ++r)
			{
			i = row_index(hhEmu, r);
			pstMT = pstPRI->apstMT[i];
			pText = hhEmu->emu_apText[i];
			pstAttr = hhEmu->emu_apAttr[i];

			for (i = 0 ; i < MAX_EMUCOLS ; ++i)
				{
				*pstMT++ = stMT;
				*pText++ = BLACK_MOSAIC;
				*pstAttr++ = stAttr;
				}
			}

		pstMT = pstPRI->apstMT[hhEmu->emu_imgrow];
		pText = hhEmu->emu_apText[hhEmu->emu_imgrow];
		pstAttr = hhEmu->emu_apAttr[hhEmu->emu_imgrow];

		for (i = 0 ; i <= hhEmu->emu_curcol ; ++i)
			{
			*pstMT++ = stMT;
			*pText++ = BLACK_MOSAIC;
			*pstAttr++ = stAttr;
			}

		updateLine(sessQueryUpdateHdl(hhEmu->hSession),
					0, hhEmu->emu_currow);

		hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = hhEmu->emu_curcol + 1;
		break;

	case 2: 	// entire screen (cursor position not changed)
		for (r = 1 ; r < MAX_EMUROWS ; ++r)
			{
			i = row_index(hhEmu, r);
			pstMT = pstPRI->apstMT[i];
			pText = hhEmu->emu_apText[i];
			pstAttr = hhEmu->emu_apAttr[i];

			hhEmu->emu_aiEnd[r] = EMU_BLANK_LINE;

			for (i = 0 ; i < MAX_EMUCOLS ; ++i)
				{
				*pstMT++ = stMT;
				*pText++ = BLACK_MOSAIC;
				*pstAttr++ = stAttr;
				}
			}

		updateLine(sessQueryUpdateHdl(hhEmu->hSession),
									0, hhEmu->emu_maxrow);
		break;
		}

	minitelRecordSeparator(hhEmu);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelClrScrn
 *
 * DESCRIPTION:
 *	Front end for minitelClearScreen() that reads the PSN argument,
 *	converts it, and passes it to minitelClearScreen.  Called from
 *	the state tables.
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelClrScrn(const HHEMU hhEmu)
	{
	minitelClearScreen(hhEmu, hhEmu->selector[0]);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelRecordSepartor
 *
 * DESCRIPTION:
 *	Record Separtor has special duties in Minitel.	In general it homes
 *	the cursor and returns the emulator to what's called an SI condition.
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelRecordSeparator(const HHEMU hhEmu)
	{
	(*hhEmu->emu_setcurpos)(hhEmu, 1, 0);
	minitelReset(hhEmu);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelClearLine
 *
 * DESCRIPTION:
 *	Handles the various clear line functions like cursor to end, beg to
 *	cursor, etc.
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelClearLine(const HHEMU hhEmu, const int iHow)
	{
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	int i;
	ECHAR *pText = hhEmu->emu_apText[hhEmu->emu_imgrow];
	PSTMINITEL pstMT = pstPRI->apstMT[hhEmu->emu_imgrow];
	PSTATTR pstAttr = hhEmu->emu_apAttr[hhEmu->emu_imgrow];
	const HUPDATE hUpdate= sessQueryUpdateHdl(hhEmu->hSession);
	STMINITEL stMT;
	STATTR	stAttr;

	// CSI sequences not available in row 0
	//
	if (hhEmu->emu_currow == 0)
		return;

	memset(&stMT, 0, sizeof(stMT));
    stMT.ismosaic = 1;

	memset(&stAttr, 0, sizeof(stAttr));
	stAttr.txtclr = VC_BRT_WHITE;
	stAttr.bkclr = VC_BLACK;

	switch (iHow)
		{
	case 0: 	// cursor to end of line inclusive
	default:
		for (i = hhEmu->emu_curcol ; i < MAX_EMUCOLS ; ++i)
			{
			*pText++ = BLACK_MOSAIC;
			*pstMT++ = stMT;
			*pstAttr++ = stAttr;
			}

		hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = hhEmu->emu_curcol - 1;
		updateChar(hUpdate, hhEmu->emu_currow,
					hhEmu->emu_curcol, MAX_EMUCOLS);
		break;

	case 1: 	// beginning of line to cursor inclusive
		for (i = 0 ; i <= hhEmu->emu_curcol ; ++i)
			{
			*pText++ = BLACK_MOSAIC;
			*pstMT++ = stMT;
			*pstAttr++ = stAttr;
			}

		hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = hhEmu->emu_curcol + 1;
		updateChar(hUpdate, hhEmu->emu_currow, 0, hhEmu->emu_curcol);
		break;

	case 2: 	// entire line
		for (i = 0 ; i < MAX_EMUCOLS ; ++i)
			{
			*pText++ = BLACK_MOSAIC;
			*pstMT++ = stMT;
			*pstAttr++ = stAttr;
			}

		hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = EMU_BLANK_LINE;
		updateLine(hUpdate, hhEmu->emu_currow, hhEmu->emu_currow);
		break;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelClrLn
 *
 * DESCRIPTION:
 *	Driver for minitelClearLine().
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelClrLn(const HHEMU hhEmu)
	{
	minitelClearLine(hhEmu, hhEmu->selector[0]);
	return;
	}

#if 0
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelDel
 *
 * DESCRIPTION:
 *	code 0x7F (Del) deletes the cursor location and moves the cursor
 *	one position right.
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelDel(const HHEMU hhEmu)
	{
	hhEmu->emu_apText[hhEmu->emu_imgrow][hhEmu->emu_curcol] = ETEXT('\x5F');
	hhEmu->emu_ap

	if (hhEmu->emu_aiEnd[hhEmu->emu_imgrow] == hhEmu->emu_curcol)
		hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = hhEmu->emu_curcol - 1;

	minitelHorzTab(hhEmu);
	return;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelHorzTab
 *
 * DESCRIPTION:
 *	minitel cursor has some special characteristics.  Minitel is always
 *	in wrap mode, so we wrap to begining of next row when beyond the
 *	last column.  Also, when at bottom, wrap to line 1.  Also, if in
 *	row 0, column 40, ignore.
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelHorzTab(const HHEMU hhEmu)
	{
	int row = hhEmu->emu_currow;
	int col = hhEmu->emu_curcol;

	if (col >= hhEmu->emu_maxcol)
		{
		if (hhEmu->emu_currow == 0)
			return;

		if (hhEmu->emu_currow >= hhEmu->emu_maxrow)
			row = 1;

		else
			row += 1;

		col = 0;
		}

	else
		{
		col += 1;
		}

	(*hhEmu->emu_setcurpos)(hhEmu, row, col);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelRepeat
 *
 * DESCRIPTION:
 *	Repeat code displays the last displayed character x number of
 *	times where x is the current emu_code coming in.
 *	I don't think wrapping is effective here.
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelRepeat(const HHEMU hhEmu)
	{
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	int x;

	// Already did range checking in state table to get here.
	// Repeat number is only the first six significant bits.

	x = max(0, hhEmu->emu_code-0x40);
	hhEmu->emu_code = pstPRI->minitel_last_char;

	while (x-- > 0)
		minitelGraphic(hhEmu);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelCancel
 *
 * DESCRIPTION:
 *	Fills current row from cursor position to end of row with spaces
 *	in the current character set current attributes.  Cursor doesn't move.
 *  Doco says this is not a delimiter.
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelCancel(const HHEMU hhEmu)
	{
	int i;
	int iMax;
	int fModified;
	const int row = hhEmu->emu_currow;
	const int col = hhEmu->emu_curcol;
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	hhEmu->emu_code = ETEXT('\x20');

	// Ah, the life of the undocumented.  The documentation says
	// that this guys does not validate, colors, act as a delimiter
	// and fills with spaces.  Wrong.  It does validate the color.
	// As such its a delimiter.  If the the current active char
	// set is G1, then it fills with mosaics, not spaces.
	//
	fModified = pstPRI->stLatentAttr.fModified;

	iMax = hhEmu->emu_maxcol;

	// minitelGraphic checks the InCancel flag and if TRUE suppresses
	// linewrap. mrw:5/3/95
	//
	pstPRI->fInCancel = TRUE;

	for (i = hhEmu->emu_curcol ; i <= iMax ; ++i)
		{
		minitelGraphic(hhEmu);
		}

	pstPRI->fInCancel = FALSE;

	// Ok, even though we validated the background color, we haven't
	// changed the state of the latent attribute (also undocumented).
	// So set it back to whatever is was before we entered this lovely
	// mess of a function - mrw
	//
	pstPRI->stLatentAttr.fModified = fModified;

	(*hhEmu->emu_setcurpos)(hhEmu, row, col);
	hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = hhEmu->emu_maxcol;
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelUSRow
 *
 * DESCRIPTION:
 *	Intermediate function that collects the row number
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelUSRow(const HHEMU hhEmu)
	{
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	pstPRI->us_row_code = hhEmu->emu_code;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelUSCol
 *
 * DESCRIPTION:
 *	Interestingly, columns are numbered from 1 to 40.  Unit seperators
 *  are ugly little beasts.  They indicate a row, col combo, but only
 *  if in a certain range.  Also, an obsolite sequence US,3/X,3/Y where
 *  0 < X < 3, 0 < Y < 9 and XY < 24 is not suppose to be used but 
 *  often is.
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelUSCol(const HHEMU hhEmu)
	{
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	int us_col_code = hhEmu->emu_code;
	int us_col = us_col_code - 0x41;
	int us_row = pstPRI->us_row_code - 0x40;

	if (us_row >= 0 && us_row <= hhEmu->emu_maxrow &&
		us_col >= 0 && us_col <= hhEmu->emu_maxcol)
		{
		if (us_row == 0)
			{
			// p.97, bottom of page
			//
			if (hhEmu->emu_currow != 0)
				{
				pstPRI->minitel_saved_attr = hhEmu->emu_charattr;
				pstPRI->minitel_saved_row = hhEmu->emu_currow;
				pstPRI->minitel_saved_col = hhEmu->emu_curcol;
				pstPRI->saved_stLatentAttr = pstPRI->stLatentAttr;

				pstPRI->minitel_saved_minitelG1Active =
					pstPRI->minitelG1Active;

				pstPRI->saved_minitelUseSeparatedMosaics =
					pstPRI->minitelUseSeparatedMosaics;
				}
			}

		(*hhEmu->emu_setcurpos)(hhEmu, us_row, us_col);
		minitelReset(hhEmu);
		}

	else if (pstPRI->us_row_code >= 0x30 &&
		pstPRI->us_row_code < 0x33 &&
		us_col_code >= 0x30 &&
		us_col_code <= 0x39)
		{
		us_row = ((pstPRI->us_row_code - 0x30) * 10) + (us_col_code - 0x30);

		if (us_row > 24)
			return;

		if (us_row == 0)
            {
			if (hhEmu->emu_currow != 0)
				{
				pstPRI->minitel_saved_attr = hhEmu->emu_charattr;
				pstPRI->minitel_saved_row = hhEmu->emu_currow;
				pstPRI->minitel_saved_col = hhEmu->emu_curcol;
				pstPRI->saved_stLatentAttr = pstPRI->stLatentAttr;

				pstPRI->minitel_saved_minitelG1Active =
					pstPRI->minitelG1Active;

				pstPRI->saved_minitelUseSeparatedMosaics =
					pstPRI->minitelUseSeparatedMosaics;
				}
            }

		(*hhEmu->emu_setcurpos)(hhEmu, us_row, 0);
		minitelReset(hhEmu);
        }

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelDelChars
 *
 * DESCRIPTION:
 *	Deletes n characters from cursor position inclusive.
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelDelChars(const HHEMU hhEmu)
	{
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;
	int i, n;
	ECHAR *tp = hhEmu->emu_apText[hhEmu->emu_imgrow]+hhEmu->emu_curcol;
	STATTR stAttr;
	PSTATTR ap = hhEmu->emu_apAttr[hhEmu->emu_imgrow]+hhEmu->emu_curcol;
	STMINITEL stMT;
	PSTMINITEL pstMT = pstPRI->apstMT[hhEmu->emu_imgrow];

	// CSI sequences not available in row 0
	//
	if (hhEmu->emu_currow == 0)
		return;

	n = min(hhEmu->emu_maxcol, hhEmu->num_param[0]);
	i = max(0, hhEmu->emu_maxcol - hhEmu->emu_curcol - n);

	/* --- Move characters down --- */

	memmove(tp, tp+n, (unsigned)i * sizeof(ECHAR));
	memmove(ap, ap+n, (unsigned)i * sizeof(STATTR));
	memmove(pstMT, pstMT+n, (unsigned)i * sizeof(STMINITEL));

	hhEmu->emu_aiEnd[hhEmu->emu_imgrow] =
		hhEmu->emu_aiEnd[hhEmu->emu_imgrow] - i;

	/* --- Fill remainder of line --- */

	tp += i;
	ap += i;

	memset(&stAttr, 0, sizeof(stAttr));
	stAttr.txtclr = VC_BRT_WHITE;
	stAttr.bkclr  = VC_BLACK;

	memset(&stMT, 0, sizeof(stMT));
	stMT.ismosaic = (unsigned)pstPRI->minitelG1Active;

	for (n = max(0, hhEmu->emu_maxcol - i) ; n > 0 ; --n)
		{
		*tp++ = EMU_BLANK_CHAR;
		*ap++ = stAttr;
		*pstMT++ = stMT;
		}

	updateChar(sessQueryUpdateHdl(hhEmu->hSession),
				hhEmu->emu_currow,
				hhEmu->emu_curcol,
				hhEmu->emu_maxcol);

	ANSI_Pn_Clr(hhEmu);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelInsChars
 *
 * DESCRIPTION:
 *	Inserts n characters from cursor position inclusive
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelInsChars(const HHEMU hhEmu)
	{
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;
	int i, n;
	ECHAR *tp = hhEmu->emu_apText[hhEmu->emu_imgrow]+hhEmu->emu_curcol;
	STATTR stAttr;
	PSTATTR ap = hhEmu->emu_apAttr[hhEmu->emu_imgrow]+hhEmu->emu_curcol;
	STMINITEL stMT;
	PSTMINITEL pstMT = pstPRI->apstMT[hhEmu->emu_imgrow];

	// CSI sequences not available in row 0
	//
	if (hhEmu->emu_currow == 0)
		return;

	n = min(hhEmu->emu_maxcol, hhEmu->num_param[0]);
	i = max(0, hhEmu->emu_maxcol - hhEmu->emu_curcol - n);

	/* --- Move stuff down --- */

	memmove(tp+n, tp, (unsigned)i * sizeof(ECHAR));
	memmove(ap+n, tp, (unsigned)i * sizeof(STATTR));
	memmove(pstMT+n, pstMT, (unsigned)i * sizeof(STMINITEL));

	hhEmu->emu_aiEnd[hhEmu->emu_imgrow] =
		hhEmu->emu_aiEnd[hhEmu->emu_imgrow] + i;

	/* --- Fill the gap --- */

	memset(&stAttr, 0, sizeof(stAttr));
	stAttr.txtclr = VC_BRT_WHITE;
	stAttr.bkclr  = VC_BLACK;

	memset(&stMT, 0, sizeof(stMT));
	stMT.ismosaic = (unsigned)pstPRI->minitelG1Active;

	while (--i >= 0)
		{
		*tp++ = EMU_BLANK_CHAR;
		*ap++ = stAttr;
		*pstMT++ = stMT;
		}

	updateChar(sessQueryUpdateHdl(hhEmu->hSession),
				hhEmu->emu_currow,
				hhEmu->emu_curcol,
				hhEmu->emu_maxcol);

	ANSI_Pn_Clr(hhEmu);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelDelRows
 *
 * DESCRIPTION:
 *	Deletes n rows from the current row.
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelDelRows(const HHEMU hhEmu)
	{
	int r, r1;
	int c, i, n;
	STATTR stAttr;
	STMINITEL stMT;
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	// CSI sequences not available in row 0
	//
	if (hhEmu->emu_currow == 0)
		return;

	n = min(hhEmu->emu_maxrow, hhEmu->num_param[0]);
	i = max(0, hhEmu->emu_maxrow - hhEmu->emu_currow - n);

	for (i = 0 ; i < n ; ++i)
		{
		if ((hhEmu->emu_currow+i+n) > hhEmu->emu_maxrow)
			break;

		r = row_index(hhEmu, hhEmu->emu_currow+i);
		r1 = row_index(hhEmu, hhEmu->emu_currow+i+n);

		MemCopy(hhEmu->emu_apText[r],
				hhEmu->emu_apText[r1],
				sizeof(ECHAR) * (unsigned)(hhEmu->emu_maxcol+1));

		MemCopy(hhEmu->emu_apAttr[r],
				hhEmu->emu_apAttr[r1],
				sizeof(STATTR) * (unsigned)(hhEmu->emu_maxcol+1));

		MemCopy(pstPRI->apstMT[r],
				pstPRI->apstMT[r1],
				sizeof(STMINITEL) * (unsigned)(hhEmu->emu_maxcol+1));

		hhEmu->emu_aiEnd[r] = hhEmu->emu_aiEnd[r1];
		}

	memset(&stAttr, 0, sizeof(stAttr));
	stAttr.txtclr = VC_BRT_WHITE;
	stAttr.bkclr  = VC_BLACK;

	memset(&stMT, 0, sizeof(stMT));
	stMT.ismosaic = (unsigned)pstPRI->minitelG1Active;

	for (n = max(0, hhEmu->emu_maxrow - i) ; n <= hhEmu->emu_maxrow ; ++n)
		{
		r = row_index(hhEmu, n);

		for (c = 0 ; c <= hhEmu->emu_maxcol ; ++c)
			{
			hhEmu->emu_apText[r][c] = EMU_BLANK_CHAR;
			hhEmu->emu_apAttr[r][c] = stAttr;
			pstPRI->apstMT[r][c] = stMT;
			hhEmu->emu_aiEnd[r] = EMU_BLANK_LINE;
			}
		}

	updateLine(sessQueryUpdateHdl(hhEmu->hSession),
				hhEmu->emu_currow,
				hhEmu->emu_maxrow);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelInsRows
 *
 * DESCRIPTION:
 *	Inserts n rows from current row inclusive
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void minitelInsRows(const HHEMU hhEmu)
	{
	int r, r1;
	int c, i, n;
	STATTR stAttr;
	STMINITEL stMT;
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	if (hhEmu->emu_currow == 0)
		return;

	n = min(hhEmu->emu_maxrow, hhEmu->num_param[0]);
	i = max(0, hhEmu->emu_maxrow - hhEmu->emu_currow - n);

	for (i = 0 ; i < n ; ++i)
		{
		if ((hhEmu->emu_currow+i+n) > hhEmu->emu_maxrow)
			break;

		r = row_index(hhEmu, hhEmu->emu_currow+i);
		r1 = row_index(hhEmu, hhEmu->emu_currow+i+n);

		MemCopy(hhEmu->emu_apText[r1],
				hhEmu->emu_apText[r],
				sizeof(ECHAR) * (unsigned)(hhEmu->emu_maxcol+1));

		MemCopy(hhEmu->emu_apAttr[r1],
				hhEmu->emu_apAttr[r],
				sizeof(STATTR) * (unsigned)(hhEmu->emu_maxcol+1));

		MemCopy(pstPRI->apstMT[r1],
				pstPRI->apstMT[r],
				sizeof(STMINITEL) * (unsigned)(hhEmu->emu_maxcol+1));

		hhEmu->emu_aiEnd[r1] = hhEmu->emu_aiEnd[r];
		}

	memset(&stAttr, 0, sizeof(stAttr));
	stAttr.txtclr = VC_BRT_WHITE;
	stAttr.bkclr  = VC_BLACK;

	memset(&stMT, 0, sizeof(stMT));
	stMT.ismosaic = (unsigned)pstPRI->minitelG1Active;

	for (n = hhEmu->emu_currow ; n < (hhEmu->emu_maxrow - i) ; ++n)
		{
		r = row_index(hhEmu, n);

		for (c = 0 ; c <= hhEmu->emu_maxcol ; ++c)
			{
			hhEmu->emu_apText[r][c] = EMU_BLANK_CHAR;
			hhEmu->emu_apAttr[r][c] = stAttr;
			pstPRI->apstMT[r][c] = stMT;
			hhEmu->emu_aiEnd[r] = EMU_BLANK_LINE;
			}
		}

	updateLine(sessQueryUpdateHdl(hhEmu->hSession),
				hhEmu->emu_currow,
				hhEmu->emu_maxrow);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelHomeHostCursor
 *
 * DESCRIPTION:
 *	Sets cursor to home position which is 1, 0 in this case.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle
 *
 * RETURNS:
 *	0=OK,else error.
 *
 */
int minitelHomeHostCursor(const HHEMU hhEmu)
	{
	if (hhEmu == 0)
		{
		assert(0);
		return -1;
		}

	(*hhEmu->emu_setcurpos)(hhEmu, 1, 0);
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitel_scrollup
 *
 * DESCRIPTION:
 *	Mintels of course scroll differently.  Actually, its the way they
 *	clear lines that keeps us from using the standard stuff.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *	nlines	- number of lines to scroll.
 *
 * RETURNS:
 *	void
 *
 */
void minitel_scrollup(const HHEMU hhEmu, int nlines)
	{
	register INT row;
	INT 	nrows, iLen, iThisRow;
	ECHAR *lp;			/* line pointer */
	INT nScrlInc;		/* needed for call to Vid routine at bottom of func */

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
		minitel_clear_imgrow(hhEmu, row);
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
			PSTMINITEL pstMT, pstMT2;

			memmove(hhEmu->emu_apText[row_index(hhEmu, row)],
				 hhEmu->emu_apText[row_index(hhEmu, row + nlines)],
				 (size_t)hhEmu->emu_maxcol + 2);

			hhEmu->emu_aiEnd[row_index(hhEmu, row + nlines)] =
				hhEmu->emu_aiEnd[row_index(hhEmu, row)];

			pstAttr  = hhEmu->emu_apAttr[row_index(hhEmu, row)];
			pstAttr2 = hhEmu->emu_apAttr[row_index(hhEmu, row + nlines)];

			for (c = 0 ; c <= hhEmu->emu_maxcol ; ++c)
				pstAttr[c] = pstAttr2[c];

			pstMT = ((PSTMTPRIVATE)hhEmu->pvPrivate)->apstMT[row_index(hhEmu, row)];
			pstMT2= ((PSTMTPRIVATE)hhEmu->pvPrivate)->apstMT[row_index(hhEmu, row + nlines)];

			for (c = 0 ; c <= hhEmu->emu_maxcol ; ++c)
				pstMT[c] = pstMT2[c];
			}

		for (row = hhEmu->bottom_margin; nlines > 0; --nlines, --row)
			minitel_clear_imgrow(hhEmu, row);
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
 *	minitel_scrolldown
 *
 * DESCRIPTION:
 *	Minitel of course works differently.  Mostly is has more work to
 *	clear a line.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle
 *	nlines	- number of lines to scroll
 *
 * RETURNS:
 *	void
 *
 */
void minitel_scrolldown(const HHEMU hhEmu, int nlines)
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
			PSTMINITEL pstMT, pstMT2;

			memmove(hhEmu->emu_apText[row_index(hhEmu, row)],
				 hhEmu->emu_apText[row_index(hhEmu, row - nlines)],
						(size_t)(hhEmu->emu_maxcol+2));

			hhEmu->emu_aiEnd[row_index(hhEmu, row - nlines)] =
				hhEmu->emu_aiEnd[row_index(hhEmu, row)];

			pstAttr  = hhEmu->emu_apAttr[row_index(hhEmu, row)];
			pstAttr2 = hhEmu->emu_apAttr[row_index(hhEmu, row - nlines)];

			for (c = 0 ; c <= hhEmu->emu_maxcol ; ++c)
				pstAttr[c] = pstAttr2[c];

			pstMT = ((PSTMTPRIVATE)hhEmu->pvPrivate)->apstMT[row_index(hhEmu, row)];
			pstMT2= ((PSTMTPRIVATE)hhEmu->pvPrivate)->apstMT[row_index(hhEmu, row + nlines)];

			for (c = 0 ; c <= hhEmu->emu_maxcol ; ++c)
				pstMT[c] = pstMT2[c];
			}
		}

	for (row = hhEmu->top_margin; nlines > 0; --nlines, ++row)
		minitel_clear_imgrow(hhEmu, row);

	hhEmu->emu_imgrow = row_index(hhEmu, hhEmu->emu_currow);

	updateScroll(sessQueryUpdateHdl(hhEmu->hSession),
					toprow, botmrow, -nScrlInc, hhEmu->emu_imgtop, TRUE);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitel_clear_imgrow
 *
 * DESCRIPTION:
 *	minitel's have to do more work to clear a line.
 *
 * ARGUMENTS:
 *	hhEmu	- private minitel handle.
 *	row 	- row to clear
 *
 * RETURNS:
 *	void
 *
 */
static void minitel_clear_imgrow(const HHEMU hhEmu, const int row)
	{
	const int save_row = hhEmu->emu_currow;
	const int save_imgrow = hhEmu->emu_imgrow;

	hhEmu->emu_currow = row;
	hhEmu->emu_imgrow = row_index(hhEmu, row);

	minitelClearLine(hhEmu, 2);

	hhEmu->emu_currow = save_row;
	hhEmu->emu_imgrow = save_imgrow;

	return;
	}
#endif	// INCL_MINITEL

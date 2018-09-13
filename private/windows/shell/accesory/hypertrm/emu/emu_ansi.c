/*	File: D:\WACKER\emu\emu_ansi.c (Created: 08-Dec-1993)
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
#include <tdll\cloop.h>
#include <tdll\update.h>
#include <tdll\tchar.h>

#include "emu.h"
#include "emu.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_CNL
 *
 * DESCRIPTION:
 *	 Moves the cursor to the start of the nth next line. The cursor can not
 *	 move past the end of the scrolling region.
 *
 * ARGUMENTS:
 *	 nline -- number of lines to move the cursor down
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_CNL(const HHEMU hhEmu, int nlines)
	{
	int row;

	if (nlines < 1)
		nlines = 1;

	row = hhEmu->emu_currow;
	row += nlines;

	if (row > hhEmu->bottom_margin)
		{
		(*hhEmu->emu_scroll)(hhEmu, row - hhEmu->bottom_margin, TRUE);
		row = hhEmu->bottom_margin;
		}

	(*hhEmu->emu_setcurpos)(hhEmu, row, 0);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_CUB
 *
 * DESCRIPTION:
 *	 Moves the cursor backwards (to the left) the specified number of
 *	 characters, but stops at the 1st character in the current line.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_CUB(const HHEMU hhEmu)
	{
	int nchars;
	nchars = hhEmu->num_param[hhEmu->num_param_cnt];
	if (nchars < 1)
		nchars = 1;

	(*hhEmu->emu_setcurpos)(hhEmu,
							hhEmu->emu_currow,
							hhEmu->emu_curcol - nchars);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_CUD
 *
 * DESCRIPTION: Moves the cursor down the specified number of lines, but stops
 *				at the bottom of the scrolling region. The column is constant.
 *				If below the scrolling region, it stops at the bottom of the
 *				screen.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_CUD(const HHEMU hhEmu)
	{
	int nlines, row;

	nlines = hhEmu->num_param[hhEmu->num_param_cnt];

	if (nlines < 1)
		nlines = 1;

	row = hhEmu->emu_currow;
	row += nlines;

	if (row > hhEmu->bottom_margin &&
				(hhEmu->emu_currow <= hhEmu->bottom_margin ||
				hhEmu->emu_currow > hhEmu->emu_maxrow))
		row = hhEmu->bottom_margin;

	(*hhEmu->emu_setcurpos)(hhEmu, row, hhEmu->emu_curcol);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_CUF
 *
 * DESCRIPTION:
 *	 Moves the cursor forward the specified number of characters, but stops
 *	 at the last character in the current line.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_CUF(const HHEMU hhEmu)
	{
	int nchars, col;

	nchars = hhEmu->num_param[hhEmu->num_param_cnt];

	if (nchars < 1)
		nchars = 1;

	col = hhEmu->emu_curcol;
	col += nchars;
	(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow, col);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_CUP
 *
 * DESCRIPTION:
 *	 Positions the cursor at the specified row and column. The row & column
 *	 numbering start at 1. If origin mode is on, the positioning is relative
 *	 to the home of the scrolling region.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_CUP(const HHEMU hhEmu)
	{
	int row, col;

	row = hhEmu->num_param[0];
	col = hhEmu->num_param_cnt > 0 ? hhEmu->num_param[1] : 0;

	if (row <= 1)
		row = 1;

	if (col <= 1)
		col = 1;

	if (hhEmu->mode_DECOM) /* VT100 Origin mode - position rel to margin */
		{
		row += hhEmu->top_margin;
		if (row > hhEmu->bottom_margin + 1)
			row = hhEmu->bottom_margin + 1;
		}
	else			/* Position is one-based from upper left */
		{
		if (row > hhEmu->emu_maxrow + 1)
			row = hhEmu->emu_maxrow + 1;
		}

	if (col > hhEmu->emu_maxcol + 1)
		col = hhEmu->emu_maxcol + 1;

	/* ANSI is one-based, HA zero-based */
	(*hhEmu->emu_setcurpos)(hhEmu, row - 1, col - 1);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_CUU
 *
 * DESCRIPTION: Moves the cursor up the specified number of lines, but stops
 *				at the top of the scrolling region. The column is constant.
 *				If above the scrolling region, it stops at the top of the
 *				screen.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_CUU(const HHEMU hhEmu)
	{
	int nlines, row;

	nlines = hhEmu->num_param[hhEmu->num_param_cnt];

	if (nlines < 1)
		nlines = 1;

	row = hhEmu->emu_currow;
	row -= nlines;

	if (row < hhEmu->top_margin &&
		(hhEmu->emu_currow >= hhEmu->top_margin || hhEmu->emu_currow < 0))
			row = hhEmu->top_margin;

	(*hhEmu->emu_setcurpos)(hhEmu, row, hhEmu->emu_curcol);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_DCH
 *
 * DESCRIPTION:
 *	Deletes the specified number of characters starting at the current
 *	cursor position and moving right. It stops at the end of the current
 *	line.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	nothing
 */
void ANSI_DCH(const HHEMU hhEmu)
	{
	int iChars, iR, i;
	ECHAR *lpStart;
	PSTATTR pstAttr;

	hhEmu->emu_imgrow = row_index(hhEmu, hhEmu->emu_currow);

	// Range check.  Have we been asked to delete more characters than
	// are displayed?  If so, change the number.
	//
	iChars = min(hhEmu->num_param[hhEmu->num_param_cnt],
					(hhEmu->emu_aiEnd[hhEmu->emu_imgrow] -
					hhEmu->emu_curcol) + 1);

	if (iChars < 1)
		iChars = 1;

	if (hhEmu->emu_aiEnd[hhEmu->emu_imgrow] == EMU_BLANK_LINE)
		return;

	// Do a special test for DEC emulation.
	//
	if ((hhEmu->emu_curcol == hhEmu->emu_maxcol) &&
		((hhEmu->stUserSettings.nEmuId == EMU_VT100) ||
		 (hhEmu->stUserSettings.nEmuId == EMU_VT100J)))
		{
		hhEmu->emu_curcol = hhEmu->emu_maxcol - 1;
		}
	else if (hhEmu->emu_curcol > hhEmu->emu_aiEnd[hhEmu->emu_imgrow])
		return;

	// Determine number of character that remain after the delete.
	//
	iR = hhEmu->emu_aiEnd[hhEmu->emu_imgrow] -
			hhEmu->emu_curcol - (iChars - 1);

	// Move the text image if there are remaining characters to
	// display.  Replace iChar characters at end of line with spaces.
	//
	#if 0
	if (iR)
		{
		memmove(hhEmu->emu_apText[hhEmu->emu_imgrow + hhEmu->emu_curcol],
				 &(hhEmu->emu_apText[hhEmu->emu_imgrow + hhEmu->emu_curcol][iChars]),
				 ((size_t)iR * sizeof(ECHAR)));   
		}
	#endif

	if (iR)
		{
		lpStart = hhEmu->emu_apText[hhEmu->emu_imgrow] + hhEmu->emu_curcol;

		memmove(lpStart, (lpStart + iChars), ((size_t)iR * sizeof(ECHAR)));
		}

    ECHAR_Fill(&hhEmu->emu_apText[hhEmu->emu_imgrow][hhEmu->emu_aiEnd[hhEmu->emu_imgrow] - iChars + 1],
				EMU_BLANK_CHAR, (unsigned int)iChars);

	// Move the attributes.  Clear iChar attributes at end of line.
	//
	if (iR)
		{
		pstAttr = hhEmu->emu_apAttr[hhEmu->emu_imgrow] + hhEmu->emu_curcol;
		memmove(pstAttr,
				(pstAttr + iChars),
				(size_t)(sizeof(STATTR)*(unsigned)iR));
		}

	pstAttr = hhEmu->emu_apAttr[hhEmu->emu_imgrow];
	for (i = (hhEmu->emu_aiEnd[hhEmu->emu_imgrow] - iChars) + 1;
			i <= hhEmu->emu_aiEnd[hhEmu->emu_imgrow]; i++)
				pstAttr[i] = hhEmu->emu_clearattr;

	// Note that emu_aiEnd[emu_imgrow] is used before we reset that
	// value.  Remember there may have been some characters and attributes
	// removed from the end of the line.  We need to tell the update stuff
	// to go that far over in the line.
	//
	updateChar(sessQueryUpdateHdl(hhEmu->hSession),
				hhEmu->emu_currow,
				hhEmu->emu_curcol,
				hhEmu->emu_aiEnd[hhEmu->emu_imgrow]);

	// Reset emu_aiEnd.  Note that it is expected and intended
	// that the result of the following calculation may
	// be (-1).
	//
	hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = hhEmu->emu_curcol + (iR - 1);

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_DL
 *
 * DESCRIPTION:
 *	 Deletes the specified number of lines starting at the current
 *	 cursor line and moving down. It stops at the bottom of the scrolling
 *	 region.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_DL(const HHEMU hhEmu)
	{
	int nlines;
	int save_top_margin;

	nlines = hhEmu->num_param[hhEmu->num_param_cnt];

	if (nlines < 1)
		nlines = 1;

	save_top_margin = hhEmu->top_margin;
	hhEmu->top_margin = hhEmu->emu_currow;

	if (hhEmu->top_margin <= hhEmu->bottom_margin)
		(*hhEmu->emu_scroll)(hhEmu, nlines, TRUE);

	hhEmu->top_margin = save_top_margin;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_ED
 *
 * DESCRIPTION:
 *	 Erases some or all of the virtual screen image and corresponding
 *	 real screen.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_ED(const HHEMU hhEmu)
	{
	int nClearSelect;

	nClearSelect = hhEmu->selector[0];
	switch (nClearSelect)
		{
	case 0: 	/* cursor to end of screen */
	case 0x0F:
	case 0xF0:
		(*hhEmu->emu_clearscreen)(hhEmu, 0);
		break;
	case 1: 	/* start of screen to cursor */
	case 0xF1:
		(*hhEmu->emu_clearscreen)(hhEmu, 1);
		break;
	case 2: 	/* Entire screen */
	case 0xF2:
		(*hhEmu->emu_clearscreen)(hhEmu, 2);

		// ANSI terminal homes after clearing.
		// DEC terminals do not
		//
		if ((hhEmu->stUserSettings.nEmuId == EMU_ANSI)  ||
			(hhEmu->stUserSettings.nEmuId == EMU_ANSIW) ||
			(hhEmu->stUserSettings.nEmuId == EMU_AUTO))
			{
			(*hhEmu->emu_setcurpos)(hhEmu, 0,0);
			}

		break;
	default:
		commanderror(hhEmu);
		break;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_EL
 *
 * DESCRIPTION:
 *	 Erases some or all of the current virtual screen line and corresponding
 *	 real screen line.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_EL(const HHEMU hhEmu)
	{
	int nClearSelect;

	nClearSelect = hhEmu->selector[0];
	switch (nClearSelect)
		{
	case 0: 	/* to end of line */
	case 0x0F:
	case 0xF0:
		(*hhEmu->emu_clearline)(hhEmu, 0);
		break;
	case 1: 	/* from start of line to cursor */
	case 0xF1:
		(*hhEmu->emu_clearline)(hhEmu, 1);
		break;
	case 2: 	/* Entire line */
	case 0xF2:
		(*hhEmu->emu_clearline)(hhEmu, 2);
		break;
	default:
		commanderror(hhEmu);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSIFormFeed
 *
 * DESCRIPTION: Scrolls the current screen until its all gone.
 *
 * ARGUMENTS:	none
 *	 none
 *
 * RETURNS: 	nothing
 */
void AnsiFormFeed(const HHEMU hhEmu)
	{
	std_clearscreen(hhEmu, 2);
	(*hhEmu->emu_setcurpos)(hhEmu, 0, 0);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_HTS
 *
 * DESCRIPTION:
 *	 Sets horizontal tab at current cursor position.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_HTS(const HHEMU hhEmu)
	{
	hhEmu->tab_stop[hhEmu->emu_curcol] = TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_ICH
 *
 * DESCRIPTION:
 *	 Inserts the specified number of spaces starting at the current
 *	 cursor position.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_ICH(const HHEMU hhEmu)
	{
	int nspaces, c, oldstate, oldrow, oldcol;
	int tmp_irm = hhEmu->mode_IRM;
	nspaces = hhEmu->num_param[hhEmu->num_param_cnt];

	if (nspaces <= 0)
		nspaces = 1;

	oldstate = hhEmu->iCurAttrState;
	hhEmu->iCurAttrState = CSCLEAR_STATE;
	oldrow = hhEmu->emu_currow;
	oldcol = hhEmu->emu_curcol;
	hhEmu->mode_IRM = SET;

	for (c = 0; c < nspaces; ++c)
		{
		hhEmu->emu_code = ETEXT(' ');
		(*hhEmu->emu_graphic)(hhEmu);
		}

	hhEmu->iCurAttrState = oldstate;
	(*hhEmu->emu_setcurpos)(hhEmu, oldrow, oldcol);

	if ((hhEmu->mode_IRM = tmp_irm) == 0)
		updateChar(sessQueryUpdateHdl(hhEmu->hSession),
					hhEmu->emu_currow,
					hhEmu->emu_curcol,
					hhEmu->emu_maxcol);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_IL
 *
 * DESCRIPTION:
 *	 Inserts the specified number of lines starting at the current
 *	 cursor row.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_IL(const HHEMU hhEmu)
	{
	int nlines;
	int save_top_margin;

	nlines = hhEmu->num_param[hhEmu->num_param_cnt];

	if (nlines < 1)
		nlines = 1;

	save_top_margin = hhEmu->top_margin;
	hhEmu->top_margin = hhEmu->emu_currow;

	if (hhEmu->top_margin < hhEmu->bottom_margin)
		(*hhEmu->emu_scroll)(hhEmu, nlines, FALSE);

	hhEmu->top_margin = save_top_margin;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_IND
 *
 * DESCRIPTION:
 *	 Moves cursor down 1 line and scrolls 1 line if necessary. IND stands
 *	 for index.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_IND(const HHEMU hhEmu)
	{
	if (hhEmu->emu_currow == hhEmu->bottom_margin)
		(*hhEmu->emu_scroll)(hhEmu, 1, TRUE);

	else
		(*hhEmu->emu_setcurpos)(hhEmu,
								hhEmu->emu_currow + 1,
								hhEmu->emu_curcol);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_NEL
 *
 * DESCRIPTION:
 *	 Inserts 1 new line on the line below current row.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_NEL(const HHEMU hhEmu)
	{
	ANSI_CNL(hhEmu, 1);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_Pn
 *
 * DESCRIPTION:
 *	 Saves ANSI style parameters and selectors. The selectors are saved as
 *	 hex and numeric parameters as decimals.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_Pn(const HHEMU hhEmu)
	{
	ECHAR ccode;

	ccode = hhEmu->emu_code;

	if (ccode == ETEXT('?') && hhEmu->selector_cnt == 0)
		hhEmu->DEC_private = TRUE;

	else if (hhEmu->DEC_private && hhEmu->selector[hhEmu->selector_cnt] == 0 && ccode != ETEXT('?'))
		hhEmu->selector[hhEmu->selector_cnt] = 0x0F;

	hhEmu->selector[hhEmu->selector_cnt] = (int)((unsigned)hhEmu->selector[hhEmu->selector_cnt] << 4) +
								ccode - ETEXT('0');
	hhEmu->num_param[hhEmu->num_param_cnt] = 10 * hhEmu->num_param[hhEmu->num_param_cnt] +
								(ccode - ETEXT('0'));
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_Pn_Clr
 *
 * DESCRIPTION:
 *	 Clears all ANSI style parameters and selectors.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_Pn_Clr(const HHEMU hhEmu)
	{
	hhEmu->num_param_cnt = hhEmu->selector_cnt = 0;
	hhEmu->num_param[0] = 0;
	hhEmu->selector[0] = 0;
	hhEmu->DEC_private = FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_Pn_End
 *
 * DESCRIPTION:
 *	 Sets next numeric parameter and selector to 0 to indicate end of
 *	 escape sequence.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_Pn_End(const HHEMU hhEmu)
	{
	hhEmu->num_param[++hhEmu->num_param_cnt] = 0;
	hhEmu->selector[++hhEmu->selector_cnt] = 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_RI
 *
 * DESCRIPTION:
 *	 Moves cursor up 1 line and scrolls 1 line if necessary. RI stands
 *	 for reverse index.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_RI(const HHEMU hhEmu)
	{
	/* If at the scrolling region's top margin, scroll down 1, but
	 * if above the top margin, and below to top edge of the screen,
	 * move the cursor up. If above the top margin and at the top edge of
	 * the screen, do nothing.
	 */
	if (hhEmu->emu_currow == hhEmu->top_margin)
		(*hhEmu->emu_scroll)(hhEmu, 1, FALSE);
	else if (hhEmu->emu_currow == 0)
		;
	else
		(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow - 1, hhEmu->emu_curcol);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_RIS
 *
 * DESCRIPTION:
 *	 Resets terminal emulator to initial state.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_RIS(const HHEMU hhEmu)
	{
	int col;
	int nTab;
	HCLOOP hCLoop;

	DEC_STBM(hhEmu, 0, 0);						/* Set scrolling region */

	(*hhEmu->emu_setcurpos)(hhEmu, 0, 0);		/* Home cursor */
	(*hhEmu->emu_clearscreen)(hhEmu, 0);		/* Clear screen */
	emu_cleartabs(hhEmu, 3);					/* Clear tabs */

	hCLoop = sessQueryCLoopHdl(hhEmu->hSession);

	nTab = CLoopGetTabSizeOut(hCLoop);

	if ( nTab <= 0)
		CLoopSetTabSizeOut(hCLoop, 8);

	for (col = 0; col <= MAX_EMUCOLS; col += nTab)
			hhEmu->tab_stop[col] = TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_SGR
 *
 * DESCRIPTION:
 *	 Sets character display attributes.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 *
 * NOTES:
 *	This function contains Auto Detect code.
 */
void ANSI_SGR(const HHEMU hhEmu)
	{
	STATTR stAttr;
	int fAnsi, sel, i;

	fAnsi = ((hhEmu->stUserSettings.nEmuId == EMU_ANSI)  ||
			 (hhEmu->stUserSettings.nEmuId == EMU_ANSIW) ||
			 (hhEmu->stUserSettings.nEmuId == EMU_VIEW)  ||
			 (hhEmu->stUserSettings.nEmuId == EMU_AUTO)) ? TRUE : FALSE;

	for (i = 0; i <= hhEmu->selector_cnt; i++)
		{
		sel = hhEmu->selector[i];
		stAttr = hhEmu->attrState[CS_STATE];

		switch(sel)
			{
		case 0: /* all attributes off */
			if (hhEmu->stUserSettings.nEmuId == EMU_AUTO)
				{
				// don't mess with color attributes in AutoDetect mode
				// for attribute reset case - mrw, 10/17/94
				//
				stAttr.undrln = 0;
				stAttr.hilite = 0;
				stAttr.bklite = 0;
				stAttr.blink = 0;
				stAttr.revvid = 0;
				stAttr.blank = 0;
				stAttr.dblwilf = 0;
				stAttr.dblwirt = 0;
				stAttr.dblhilo = 0;
				stAttr.dblhihi = 0;
				stAttr.symbol = 0;
				}

			else if (fAnsi)
				{
				memset(&stAttr, 0, sizeof(STATTR));

#if FALSE	// We've decided not to do this. rde 14 Jul 98
//#ifdef INCL_TERMINAL_SIZE_AND_COLORS
				// Reset to the user-defined colors. I'm afraid this may
				// re-introduce the intensity bug referred to below.
				// rde 9 Jun 98
                if (hhEmu->mode_DECSCNM == SET)
                    {
                    stAttr.txtclr = hhEmu->stUserSettings.nBackgroundColor;
                    stAttr.bkclr = hhEmu->stUserSettings.nTextColor;
                    }
                else
                    {
                    stAttr.txtclr = hhEmu->stUserSettings.nTextColor;
                    stAttr.bkclr = hhEmu->stUserSettings.nBackgroundColor;
                    }
#else
                // mrw:2/21/96 - Changed from 15 to 7 to fix bug with
                // intensity.
                //
				stAttr.txtclr = (hhEmu->mode_DECSCNM == SET) ? 0 : 7;
				stAttr.bkclr = (hhEmu->mode_DECSCNM == SET) ? 7 : 0;
#endif

				}
			else
				{
				if (hhEmu->mode_DECSCNM != SET)
					{
#if FALSE	// We've decided not to do this. rde 14 Jul 98
//#ifdef INCL_TERMINAL_SIZE_AND_COLORS
                    stAttr.txtclr = hhEmu->stUserSettings.nTextColor;
                    stAttr.bkclr = hhEmu->stUserSettings.nBackgroundColor;
#endif
					stAttr.undrln = 0;
					stAttr.hilite = 0;
					stAttr.bklite = 0;
					stAttr.blink = 0;
					stAttr.revvid = 0;
					stAttr.blank = 0;
					stAttr.dblwilf = 0;
					stAttr.dblwirt = 0;
					stAttr.dblhilo = 0;
					stAttr.dblhihi = 0;
					stAttr.symbol = 0;
					}
				}
			break;

		case 1: /* bold or increased intensity */
			stAttr.hilite = TRUE;
			break;

		case 2: /* faint */
		case 3:	/* italics */
			/* not supported */
			break;

		case 4: /* underscore */
			stAttr.undrln = TRUE;
			break;

		case 5: /* blink */
			stAttr.blink = TRUE;
			break;

		case 6:	/* rapid blink */
			/* not supported */
			break;

		case 7: /* reverse video */
            // Reverse video should be reverse video for a cell
            // reguardless of the current screen mode.  Changing
            // the following line removed a bug where blocks of
            // text would not get set to reverse video when the
            // screen was also in reverse mode. - rjk:02/04/97
            //
			//stAttr.revvid = (hhEmu->mode_DECSCNM == SET) ? FALSE : TRUE;
			stAttr.revvid = TRUE;
			break;

		case 8: /* invisible display */
			stAttr.blank = TRUE;
			break;

		case 9:	/* rapid blink */
			/* not supported */
			break;

		case 0x22:
			stAttr.hilite = FALSE;
			break;

		case 0x24:
			stAttr.undrln = FALSE;
			break;

		case 0x25:
			stAttr.blink = FALSE;
			break;

		case 0x27:
			stAttr.revvid = FALSE;
			break;

		case 0x30:
		case 0x32:
		case 0x35:
		case 0x37:
#ifndef INCL_VT100COLORS
#if !defined(FAR_EAST)
			emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
			emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
			if (fAnsi)
				{
				if (!stAttr.revvid)
					stAttr.txtclr = (unsigned)sel - 0x30;
				else
					stAttr.bkclr = (unsigned)sel - 0x30;
				}
#else
			stAttr.txtclr = (unsigned)sel - 0x30;
#endif
			break;

		case 0x31:
		case 0x33:
#ifndef INCL_VT100COLORS
#if !defined(FAR_EAST)
			emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
			emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
			if (fAnsi)
				{
				if (!stAttr.revvid)
					stAttr.txtclr = (unsigned)(sel - 0x30) + 3;
				else
					stAttr.bkclr = (unsigned)(sel - 0x30) + 3;
				}
#else
			stAttr.txtclr = (unsigned)(sel - 0x30) + 3;
#endif
			break;

		case 0x34:
		case 0x36:
#ifndef INCL_VT100COLORS
#if !defined(FAR_EAST)
			emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
			emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
			if (fAnsi)
				{
				if (!stAttr.revvid)
					stAttr.txtclr = (unsigned)(sel - 0x30) - 3;
				else
					stAttr.bkclr = (unsigned)(sel - 0x30) - 3;
				}
#else
			stAttr.txtclr = (unsigned)(sel - 0x30) - 3;
#endif
			break;

		case 0x40:
		case 0x42:
		case 0x45:
		case 0x47:
#ifndef INCL_VT100COLORS
#if !defined(FAR_EAST)
			emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
			emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
			if (fAnsi)
				{
				if (!stAttr.revvid)
					stAttr.bkclr = (unsigned)sel - 0x40;
				else
					stAttr.txtclr = (unsigned)sel - 0x40;
				}
#else
			stAttr.bkclr = (unsigned)sel - 0x40;
#endif
			break;

		case 0x41:
		case 0x43:
#ifndef INCL_VT100COLORS
#if !defined(FAR_EAST)
			emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
			emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
			if (fAnsi)
				{
				if (!stAttr.revvid)
					stAttr.bkclr = (unsigned)(sel - 0x40) + 3;
				else
					stAttr.txtclr = (unsigned)(sel - 0x40) + 3;
				}
#else
			stAttr.bkclr = (unsigned)(sel - 0x40) + 3;
#endif
			break;

		case 0x44:
		case 0x46:
#ifndef INCL_VT100COLORS
#if !defined(FAR_EAST)
			emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
			emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
			if (fAnsi)
				{
				if (!stAttr.revvid)
					stAttr.bkclr = (unsigned)(sel - 0x40) - 3;
				else
					stAttr.txtclr = (unsigned)(sel - 0x40) - 3;
				}
#else
			stAttr.bkclr = (unsigned)(sel - 0x40) - 3;
#endif
			break;

		default:
			/* keep processing--there may be valids codes remaining */
			/* commanderror();*/
			break;
			}

		/* --- commit changes --- */

		hhEmu->emu_charattr =
		hhEmu->attrState[CS_STATE] =
		hhEmu->attrState[CSCLEAR_STATE] = stAttr;

		hhEmu->attrState[CSCLEAR_STATE].revvid = 0;
		hhEmu->attrState[CSCLEAR_STATE].undrln = 0;

		if (fAnsi && hhEmu->attrState[CS_STATE].revvid)
			hhEmu->attrState[CSCLEAR_STATE].revvid =
				hhEmu->attrState[CS_STATE].revvid;

		hhEmu->emu_clearattr = hhEmu->attrState[CSCLEAR_STATE];
		}

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_TBC
 *
 * DESCRIPTION:
 *	 Clears one or all tab stops.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_TBC(const HHEMU hhEmu)
	{
	if (hhEmu->selector[0] == 0 || hhEmu->selector[0] == 3)
		emu_cleartabs(hhEmu, hhEmu->selector[0]);
	else
		commanderror(hhEmu);
	}

// End of ansi.c

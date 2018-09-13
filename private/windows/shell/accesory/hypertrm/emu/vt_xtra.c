/*	File: D:\WACKER\emu\vt100.c (Created: 09-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:27p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\tdll.h>
#include <tdll\session.h>
#include <tdll\assert.h>
#include <tdll\cloop.h>
#include <tdll\print.h>
#include <tdll\capture.h>
#include <tdll\update.h>
#include <tdll\tchar.h>

#include "emu.h"
#include "emu.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_DSR
 *
 * DESCRIPTION:
 *	 Reports the current cursor position to the host.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_DSR(const HHEMU hhEmu)
	{
	int sel, fOldValue;
	TCHAR achTemp[10];
	ECHAR s[10];
	ECHAR *sp;

	memset(s, 0, sizeof(s));

	sel = hhEmu->selector[0];
	if (sel == 5)
		CnvrtMBCStoECHAR(s, sizeof(s), "\033[0n", StrCharGetByteCount("\033[0n"));

	else if (sel == 6)
		{
		wsprintf(achTemp, "\033[%d;%dR", hhEmu->emu_currow + 1,
				 (hhEmu->emu_curcol == hhEmu->emu_maxcol + 1) ?
				 hhEmu->emu_curcol : hhEmu->emu_curcol + 1);
		CnvrtMBCStoECHAR(s, sizeof(s), achTemp, StrCharGetByteCount(achTemp));
		}

			/* 1st str is printer not ready */
	else if (sel == 0x0F25) /* are user-defined keys locked? */
		CnvrtMBCStoECHAR(s, sizeof(s), "\033[?20n", StrCharGetByteCount("\033[?20n"));

	else if (sel == 0x0F26) /* what is the keyboard language? */
		CnvrtMBCStoECHAR(s, sizeof(s), "\033[?27;1n", StrCharGetByteCount("\033[?27;1n"));

	else
		{
		commanderror(hhEmu);
		return;
		}
	sp = s;

	/* to not get recursive ANSI_DSR's if half duplex */

	fOldValue = CLoopGetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession));

	CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), FALSE);

	emuSendString(hhEmu, sp, StrCharGetEcharByteCount(sp));

	CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), fOldValue);

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_RM
 *
 * DESCRIPTION:
 *	 Sets character display attributes.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_RM(const HHEMU hhEmu)
	{
	int mode_id, i;
	HCLOOP	hCLoop = (HCLOOP)0;

	for (i = 0; i <= hhEmu->selector_cnt; i++)
		{
		mode_id = hhEmu->selector[i];
		switch (mode_id)
			{
		case 0x02:
			hhEmu->mode_KAM = RESET;
			hhEmu->emu_kbdin = vt100_kbdin;
			break;
		case 0x04:
			hhEmu->mode_IRM = RESET;
			break;
		case 0x07:
			hhEmu->mode_VEM = RESET;
			break;
		case 0x10:
			hhEmu->mode_HEM = RESET;
			break;
		case 0x12:
			hhEmu->mode_SRM = RESET;
			CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), TRUE);
			break;
		case 0x18:	/* actually ?18, but ? gets shifted out */
			hhEmu->mode_DECPFF = RESET;
			break;
		case 0x19:	/* acutally ?19, see above */
			hhEmu->mode_DECPEX = RESET;
			break;
		case 0x20:
			hhEmu->mode_LNM = RESET;
			/* also affects transmission of RET key */
            hCLoop = sessQueryCLoopHdl(hhEmu->hSession);
			CLoopSetAddLF(hCLoop, FALSE);
			CLoopSetSendCRLF(hCLoop, FALSE);
			break;
		case 0xF1:
			hhEmu->mode_DECCKM = RESET;
			break;
		case 0xF2:
			emuLoad((HEMU)hhEmu, EMU_VT52);
			break;
		case 0xF3:
			// Switch to 80 column mode.
			//
			emuSetDecColumns(hhEmu, VT_MAXCOL_80MODE, TRUE);
			break;
		case 0xF4:
			/* select jump scroll */
			/* we're always in jump scroll, just ignore */
			break;
		case 0xf5:
			if (hhEmu->mode_DECSCNM == SET)
				{
				emu_reverse_image(hhEmu);
				hhEmu->mode_DECSCNM = RESET;
				}
			break;
		case 0xf6:
			hhEmu->mode_DECOM = RESET;

			// This command homes the cursor. Added 16 Mar 98 rde
			ANSI_Pn_Clr(hhEmu);
			ANSI_CUP(hhEmu);
			
			break;
		case 0xd7:	/* for ANSI */
		case 0xf7:
			hhEmu->mode_AWM = RESET;
			break;
		case 0xF8:
			/* turn off auto repeat */
			break;
		case 0xF18:
			hhEmu->mode_DECPFF = RESET;
			break;
		case 0xF19:
			hhEmu->mode_DECPEX = RESET;
			break;
		case 0xF25:
			hhEmu->mode_DECTCEM = RESET;
			EmuStdSetCursorType(hhEmu, EMU_CURSOR_NONE);
			break;
		default:
			commanderror(hhEmu);
			break;
			}
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_SM
 *
 * DESCRIPTION:
 *	 Sets a mode for current terminal emulator.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_SM(const HHEMU hhEmu)
	{
	int mode_id, i;
	HCLOOP	hCLoop = (HCLOOP)0;

	for (i = 0; i <= hhEmu->selector_cnt; i++)
		{
		mode_id = hhEmu->selector[i];
		switch (mode_id)
			{
		case 0x02:
			hhEmu->mode_KAM = SET;
			hhEmu->emu_kbdin = emu_kbdlocked;
			break;
		case 0x04:
			hhEmu->mode_IRM = SET;
			break;
		case 0x07:
			hhEmu->mode_VEM = SET;
			break;
		case 0x10:
			hhEmu->mode_HEM = SET;
			break;
		case 0x12:
			hhEmu->mode_SRM = SET;
			CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), FALSE);
			break;
		case 0x18:	/* actually ?18, but ? gets shifted out */
			hhEmu->mode_DECPFF = SET;
			break;
		case 0x19:	/* acutally ?19, see above */
			hhEmu->mode_DECPEX = SET;
			break;
		case 0x20:
			hhEmu->mode_LNM = SET;
			/* also affects sending of RET key */
            hCLoop = sessQueryCLoopHdl(hhEmu->hSession);
			CLoopSetAddLF(hCLoop, TRUE);
			CLoopSetSendCRLF(hCLoop, TRUE);
			break;
		case 0xF1:
			hhEmu->mode_DECCKM = SET;
			break;
		case 0xF2:
			vt52_toANSI(hhEmu);
			break;
		case 0xF3:
			// Set 132 column mode.
			//
			emuSetDecColumns(hhEmu, VT_MAXCOL_132MODE, TRUE);
			break;
		case 0xF4:
			/* set smooth scrolling (not implemented) */
			break;
		case 0xF5:
			if (hhEmu->mode_DECSCNM == RESET)
				{
				emu_reverse_image(hhEmu);
				hhEmu->mode_DECSCNM = SET;
				}
			break;
		case 0xF6:
			hhEmu->mode_DECOM = SET;

			// This command homes the cursor. Added 16 Mar 98 rde
			ANSI_Pn_Clr(hhEmu);
			ANSI_CUP(hhEmu);
			
			break;
		case 0xD7:
		case 0xF7:
			hhEmu->mode_AWM = SET;
			break;
		case 0xF8:
			/* select auto repeat mode */
			break;
		case 0xF18:
			hhEmu->mode_DECPFF = SET;
			break;
		case 0xF19:
			hhEmu->mode_DECPEX = SET;
			break;
		case 0xF25:
			hhEmu->mode_DECTCEM = SET;
			EmuStdSetCursorType(hhEmu, hhEmu->stUserSettings.nCursorType);
			break;
		default:
			commanderror(hhEmu); break;
			}
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt_alt_kpmode
 *
 * DESCRIPTION:
 *	 Sets up emulator for alternate keypad mode.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt_alt_kpmode(const HHEMU hhEmu)
	{
	if (hhEmu->emu_code == ETEXT('='))
		hhEmu->mode_DECKPAM = SET;
	else if (hhEmu->emu_code == ETEXT('>'))
		hhEmu->mode_DECKPAM = RESET;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt_screen_adjust
 *
 * DESCRIPTION:
 *	 Fills the screen with E's.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt_screen_adjust(const HHEMU hhEmu)
	{
	register int i, row, cols;
	PSTATTR pstAttr;

	cols = hhEmu->mode_DECCOLM ? 132 : 80;

	for (row = 0; row < MAX_EMUROWS; ++row)
		{
		ECHAR_Fill(hhEmu->emu_apText[row], TEXT('E'), (unsigned)MAX_EMUCOLS);

		hhEmu->emu_aiEnd[row] = cols - 1;

		pstAttr = hhEmu->emu_apAttr[row];

		for (i = 0 ; i <= MAX_EMUCOLS; ++i)
			pstAttr[i] = hhEmu->emu_clearattr;

		updateChar(sessQueryUpdateHdl(hhEmu->hSession),
					row,
					0,
					hhEmu->emu_maxcol);
		}

	(*hhEmu->emu_setcurpos)(hhEmu, 0, 0);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt_scrollrgn
 *
 * DESCRIPTION:
 *	 Sets up a scrolling region
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt_scrollrgn(const HHEMU hhEmu)
	{
	int toprow, botmrow;

	toprow = hhEmu->num_param[0];
	botmrow = hhEmu->num_param_cnt > 0 ? hhEmu->num_param[1] : 0;
	DEC_STBM(hhEmu, toprow, botmrow);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * DEC_STBM
 *
 * DESCRIPTION:
 *	 Sets up a scrolling region
 *
 * ARGUMENTS:
 *	 top -- screen row to be top row of scrolling region
 *	 bottom -- screen row to be bottom row of scrolling region
 *				 NOTE - the top row of screen is row 1
 *
 * RETURNS:
 *	 nothing
 */
void DEC_STBM(const HHEMU hhEmu, int top, int bottom)
	{
	if (top < 1)
		top = 1;

	if (bottom < 1 || bottom > (hhEmu->emu_maxrow+1))
		bottom = (hhEmu->emu_maxrow+1);

	if (top >= bottom)
		{
		commanderror(hhEmu);
		return;
		}

	hhEmu->top_margin = top - 1;   /* convert one-based to zero-based */
	hhEmu->bottom_margin = bottom - 1;
	ANSI_Pn_Clr(hhEmu);
	ANSI_CUP(hhEmu);		 /* home cursor after setting */
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt52_to_ANSI
 *
 * DESCRIPTION:
 *	 Switches from VT52 mode to ANSI mode
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt52_toANSI(const HHEMU hhEmu)
	{
	#if FALSE	//DEADWOOD:rde 9 Mar 98
	emuLoad((HEMU)hhEmu, EMU_VT100);
	#endif

	if (!hhEmu)
		assert(FALSE);
	else
		{
		// Return to the original emu, not necessarily the VT100.
		if (hhEmu->mode_vt320)
			emuLoad((HEMU)hhEmu, EMU_VT320);
		else if (hhEmu->mode_vt220)
			emuLoad((HEMU)hhEmu, EMU_VT220);
		else
			emuLoad((HEMU)hhEmu, EMU_VT100);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt_DCH
 *
 * DESCRIPTION:
 *	 Deletes the specified number of characters starting at the current
 *	 cursor position and moving right. It stops at the end of the current line.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt_DCH(const HHEMU hhEmu)
	{
	int nchars;
	BOOL old_mode_protect;
	STATTR old_emu_charattr;

	nchars = hhEmu->num_param[hhEmu->num_param_cnt];
	if (nchars < 1)
		nchars = 1;
	if (hhEmu->emu_code == ETEXT('P'))	 /* delete & shift line left */
		{
		ANSI_DCH(hhEmu);
		}
	else					/* emu_code == 'X', just delete */
		{
		old_mode_protect = hhEmu->mode_protect;
		old_emu_charattr = hhEmu->emu_charattr;
		hhEmu->mode_protect = FALSE;
		hhEmu->emu_charattr.undrln = hhEmu->emu_charattr.blink = 0;
		emu_clearword(hhEmu, hhEmu->emu_curcol, hhEmu->emu_curcol + nchars - 1);
		hhEmu->mode_protect = old_mode_protect;
		hhEmu->emu_charattr = old_emu_charattr;
		}

	updateChar(sessQueryUpdateHdl(hhEmu->hSession),
					hhEmu->emu_currow,
					hhEmu->emu_curcol,
					hhEmu->emu_maxcol);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt_IL
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
void vt_IL(const HHEMU hhEmu)
	{
	int nlines;

	nlines = hhEmu->num_param[hhEmu->num_param_cnt];

	if (nlines < 1)
		nlines = 1;

	ANSI_IL(hhEmu);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt_DL
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
void vt_DL(const HHEMU hhEmu)
	{
	int nlines;

	nlines = hhEmu->num_param[hhEmu->num_param_cnt];

	if (nlines < 1)
		nlines = 1;

	ANSI_DL(hhEmu);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt_clearline
 *
 * DESCRIPTION:
 *	 Erases some or all of the current virtual screen line and corresponding
 *	 real screen line.
 *
 * ARGUMENTS:
 *	 select -- 0 to erase from cursor to end of line
 *			-- 1 to erase from start of line to cursor
 *			-- 2 to erase entire line
 *
 * RETURNS:
 *	 nothing
 */
void vt_clearline(const HHEMU hhEmu, const int nSelect)
	{
	std_clearline(hhEmu, nSelect);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt_clearscreen
 *
 * DESCRIPTION:
 *	 Erases some or all of the virtual screen image.
 *
 * ARGUMENTS:
 *	 select -- 0 to erase from cursor to end of screen
 *			-- 1 to erase from start of screen to cursor
 *			-- 2 to erase entire screen
 *
 * RETURNS:
 *	 nothing
 */
void vt_clearscreen(const HHEMU hhEmu, const int nSelect)
	{
	std_clearscreen(hhEmu, nSelect);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt_backspace
 *
 * DESCRIPTION:	Moves the cursor backwards (to the left) 1 column, but stops
 *				at the 1st character in the current	line. The vt emus need
 *				a special function to handle the virtual column beyond the
 *				edge of the screen.
 *
 * ARGUMENTS:	none
 *
 * RETURNS:		nothing
 */
void vt_backspace(const HHEMU hhEmu)
	{
	INT bWide = 1;
	INT	iRow = row_index(hhEmu, hhEmu->emu_currow);
	INT iCol;

	//if (hhEmu->emu_curcol > 0)
	//	{
	//	(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow,
	//		hhEmu->emu_curcol - 1);
	//	}
	if (hhEmu->emu_curcol > 0)
		{
		bWide = hhEmu->emu_apAttr[iRow][hhEmu->emu_curcol - 1].wirt ? 2 : 1;
		bWide = hhEmu->emu_apAttr[iRow][hhEmu->emu_curcol].wirt ? 0 : bWide;

		(*hhEmu->emu_setcurpos)(hhEmu,
								hhEmu->emu_currow,
								hhEmu->emu_curcol - bWide);
		}

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

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt_CUB
 *
 * DESCRIPTION:	Moves the cursor backwards (to the left) the specified number
 *				of characters, but stops at the 1st character in the current
 *				line. The vt emus need a special function to handle the
 *				virtual column beyond the edge of the screen.
 *
 * ARGUMENTS:	none
 *
 * RETURNS:		nothing
 */
void vt_CUB(const HHEMU hhEmu)
	{
	int nchars;

	nchars = hhEmu->num_param[hhEmu->num_param_cnt];
	if (nchars < 1)
		nchars = 1;

	(*hhEmu->emu_setcurpos)(hhEmu,
			hhEmu->emu_currow, (hhEmu->emu_curcol - nchars));
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuSetDecColumns
 *
 * DESCRIPTION:
 *	For DEC emulators only, this function sets either 80 or 132 columns.
 *
 * ARGUMENTS:
 *	hhEmu		-	The internal emualtor handle.
 *	nColumns	-	The number of columns to set.  This may be either...
 *					VT_MAXCOL_80MODE, or
 *					VT_MAXCOL_132MODE.
 *	fClear		-	clear screen if TRUE
 *
 * RETURNS:
 *	void
 *
 */
void emuSetDecColumns(const HHEMU hhEmu, const int nColumns, const int fClear)
	{
	if ((hhEmu->stUserSettings.nEmuId == EMU_VT100) ||
		(hhEmu->stUserSettings.nEmuId == EMU_VT220) ||
		(hhEmu->stUserSettings.nEmuId == EMU_VT320)||
	    (hhEmu->stUserSettings.nEmuId == EMU_VT100J))
		;
	else
		return;

	switch(nColumns)
		{
		case VT_MAXCOL_80MODE:
			hhEmu->mode_DECCOLM = RESET;
			hhEmu->emu_maxcol = VT_MAXCOL_80MODE;

			if (fClear)
				{
				DEC_STBM(hhEmu, 0, hhEmu->emu_maxrow + 1);
				(*hhEmu->emu_clearscreen)(hhEmu, 2);
				}

			NotifyClient(hhEmu->hSession, EVENT_EMU_SETTINGS, 0);
			break;

		case VT_MAXCOL_132MODE:
			hhEmu->mode_DECCOLM = SET;
			hhEmu->emu_maxcol = VT_MAXCOL_132MODE;

			if (fClear)
				{
				DEC_STBM(hhEmu, 0, hhEmu->emu_maxrow + 1);
				(*hhEmu->emu_clearscreen)(hhEmu, 2);
				}

			NotifyClient(hhEmu->hSession, EVENT_EMU_SETTINGS, 0);
			break;

		default:
			return;
		}
	}

/* end of vt_xtra.c */

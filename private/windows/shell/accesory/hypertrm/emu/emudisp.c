/*	File: D:\WACKER\emu\vid2.c (Created: 12-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:29p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\assert.h>
#include <tdll\session.h>
#include <tdll\update.h>

#include "emu.h"
#include "emu.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * emuDispSetCurPos
 *
 * DESCRIPTION:
 *	Sets the visible cursor on the display to row, col (zero based). If
 *	row and column values are out of range for the current display mode, they
 *	will be coerced into range.
 *
 * ARGUMENTS:
 *	row -- New row position for cursor.
 *	col -- New column poition for cursor.
 *
 * RETURNS:
 *	nothing
 */
void emuDispSetCurPos(int row, int col)
	{
	int nRow, nCol;

	if (row <= -1)
		row = emu_currow;

	if (col <= -1)
		col = emu_curcol;

	nRow = min(row, emu_maxrow);
	nCol = min(col, emu_maxcol);

	// Do range checking for DEC emulation.  This prevents the cursor
	// from being displayed in the 81st position, which is a valid
	// internal location, but is not a valid display column.
	//
	if (nCol == emu_maxcol && hhEmu->stUserSettings.nEmuId == EMU_VT100)
			nCol -= 1;

	updateCursorPos(sessQueryUpdateHdl(hhEmu->hSession), nRow, nCol);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * emuDispRgnScrollUp
 *
 * DESCRIPTION:
 *	Scrolls a rectangular area of the screen up 'nlines' lines. The color
 *	and attributes of the blank lines inserted at the bottom of the area as
 *	is scrolls up can be specified.
 *
 * ARGUMENTS:
 *	fromrow -- top row of area to be scrolled
 *	torow	-- bottom row of area to be scrolled
 *	nlines	-- number of lines to scroll
 *
 * RETURNS:
 *	nothing
 */
void emuDispRgnScrollUp(const int fromrow, const int torow, const int nlines)
	{
	updateScroll(sessQueryUpdateHdl(hhEmu->hSession),
					fromrow, torow, nlines, emu_imgtop, TRUE);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * emuDispRgnScrollDown
 *
 * DESCRIPTION:
 *	Scrolls a rectangular area of the screen down 'nlines' lines. The color
 *	and attributes of the blank lines inserted at the top of the area as
 *	is scrolls down can be specified.
 *
 * ARGUMENTS:
 *	fromrow -- top row of area to be scrolled
 *	torow	-- bottom row of area to be scrolled
 *	nlines	-- number of lines to scroll
 *
 * RETURNS:
 *	nothing
 */
void emuDispRgnScrollDown(const int fromrow,
							const int torow,
							const int nlines)
	{
	updateScroll(sessQueryUpdateHdl(hhEmu->hSession),
					fromrow, torow, -nlines, emu_imgtop, TRUE);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * emuDispRgnClear
 *
 * DESCRIPTION:
 *	Clears a rectangular area of the screen to blank spaces. The color and
 *	attributes of the spaces is determined by 'state'.
 *
 * ARGUMENTS:
 *	fromrow -- top row of area to be cleared
 *	torow	-- bottom row of area to be cleared
 *
 * RETURNS:
 *	nothing
 */
void emuDispRgnClear(const int fromrow, const int torow)
	{
	updateLine(sessQueryUpdateHdl(hhEmu->hSession), fromrow, torow);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * emuDispWrtQuickChr
 *
 * DESCRIPTION:
 *	Fast routine to write a character at current cursor position with
 *	current color and attributes. Handles all characters as displayable
 *	characters.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	nothing
 */
void emuDispWrtQuickChr(void)	/* assumes current cursor pos. and state */
	{
	if (++emu_curcol > emu_maxcol)
		emu_curcol = emu_maxcol;

	updateChar(sessQueryUpdateHdl(hhEmu->hSession),
					emu_currow, emu_curcol, emu_curcol);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * emuDispWrtTTYChr
 *
 * DESCRIPTION:
 *	Writes a single character to the screen at the current cursor postion
 *	using the current default colors and attributes. Unlike VidWrtQuickChr,
 *	this function will correctly interpret the TTY control codes such as
 *	'\r', '\n', '\b', and BELL
 *
 * ARGUMENTS:
 *	c -- Character to write
 *
 * RETURNS:
 *	nothing
 */
void emuDispWrtTTYChr(TCHAR c)
	{
	int beeplen = 50;

	if (c == TEXT('\007'))
		{
		if (beeplen)
			MessageBeep((UINT)-1);
		}

	else if (c == TEXT('\r'))
		{
		emu_curcol = 0;
		}

	else if (c == TEXT('\n'))
		{
		if (++emu_currow > emu_maxrow)
			--emu_currow;
		}

	else
		{
		emuDispWrtQuickChr();
		}

	//updateChar(sessQueryUpdateHdl(hhEmu->hSession),
	//				  emu_currow, emu_curcol, emu_curcol);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * emuDispWrtChrAttrStr
 *
 * DESCRIPTION:
 *	Updates the screen from memory buffers containing the text and attribute
 *	bytes of the display. This is used in cases where the contents of the
 *	screen have been stored with the low-level display attributes. The display
 *	begins at the current cursor postion.
 *
 * ARGUMENTS:
 *	nchars	-- Number of characters to be restored.
 *
 * RETURNS:
 *	none
 */
void emuDispWrtChrAttrStr(const int nChars)
	{
	updateChar(sessQueryUpdateHdl(hhEmu->hSession),
					emu_currow,
					emu_curcol,
					min( (emu_curcol + nChars), emu_maxcol));

	//if ((emu_curcol += nChars) > emu_maxcol)
	//	  emu_curcol = emu_maxcol;

	return;
	}

/* end of emudisp.c */

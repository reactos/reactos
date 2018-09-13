/*	File: D:\WACKER\emu\viewdata.c (Created: 31-Jan-1994)
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
#include <tdll\chars.h>
#include <tdll\tchar.h>
#include <tdll\print.h>
#include <tdll\update.h>
#include <tdll\assert.h>

#include "emu.h"
#include "emu.hh"
#include "viewdata.hh"

#if defined(INCL_VIEWDATA)

static void EmuViewdataDisplayLine(const HHEMU hhEmu, const int iRow, const int iStartCol);
static STATTR GetAttr(const HHEMU hhEmu, const int iRow, const int iCol);
static ECHAR MapMosaics(const HHEMU hhEmu, ECHAR ch);
static int RowHasDblHigh(const HHEMU hhEmu, const int iRow);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataAnswerback
 *
 * DESCRIPTION:	Sends the answerback message defined on the menus.
 *
 * ARGUMENTS:	none
 *
 * RETURNS:		nothing
 */
void EmuViewdataAnswerback(const HHEMU hhEmu)
	{
	TCHAR *sp;
	ECHAR *pech = NULL;

	sp = hhEmu->acAnswerback;

	// If there is nothing to send, there is nothing to send
	if (StrCharGetStrLength(sp) == 0)
		return;

	pech = malloc((unsigned int)StrCharGetByteCount(sp) + sizeof(TCHAR));

	if (pech == NULL)
		{
		assert(FALSE);
		return;
		}

	CnvrtMBCStoECHAR(pech, (unsigned long)StrCharGetByteCount(sp), sp, 
        (unsigned long)StrCharGetByteCount(sp) + sizeof(TCHAR));

	emuSendString(hhEmu, pech, (int)StrCharGetEcharByteCount(pech));
	free(pech);
	pech = NULL;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataCursorLeft
 *
 * DESCRIPTION:	Moves cursor left one column. If cursor starts at left edge,
 *				it moves cursor to the last column of the line above.
 *
 * ARGUMENTS:	none
 *
 * RETURNS:		nothing
 */
void EmuViewdataCursorLeft(const HHEMU hhEmu)
	{
	int iRow, iCol;

	iRow = hhEmu->emu_currow;
	iCol = hhEmu->emu_curcol;

	if (hhEmu->emu_curcol > 0)
		{
		iCol -= 1;
		}

	else if (hhEmu->emu_currow > 0)
		{
		iRow -= 1;
		iCol = hhEmu->emu_maxcol;
		}

	(*hhEmu->emu_setcurpos)(hhEmu, iRow, iCol);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataCursorRight
 *
 * DESCRIPTION:	Moves cursor right one column. If cursor starts at right edge,
 *				it moves cursor to the first column of the line below.
 *
 * ARGUMENTS:	none
 *
 * RETURNS:		nothing
 */
void EmuViewdataCursorRight(const HHEMU hhEmu)
	{
	int iRow, iCol;

	iRow = hhEmu->emu_currow;
	iCol = hhEmu->emu_curcol;

	if (hhEmu->emu_curcol < hhEmu->emu_maxcol)
		{
		iCol += 1;
		}

	else if (hhEmu->emu_currow < hhEmu->emu_maxrow)
		{
		iRow += 1;
		iCol = 0;
		}

	(*hhEmu->emu_setcurpos)(hhEmu, iRow, iCol);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataCursorDown
 *
 * DESCRIPTION:	Moves cursor down to the next line while maintaining the same
 *				column. If starting on the bottom line, the cursor moves to
 *				the top line.
 *
 * ARGUMENTS:	none
 *
 * RETURNS:		nothing
 */
void EmuViewdataCursorDown(const HHEMU hhEmu)
	{
	int iRow = hhEmu->emu_currow;

	(*hhEmu->emu_setcurpos)(hhEmu,
					(hhEmu->emu_currow < hhEmu->emu_maxrow ? ++iRow : 0),
					hhEmu->emu_curcol);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataCursorUp
 *
 * DESCRIPTION:	Moves cursor up to the next line while maintaining the same
 *				column. If starting on the top line, the cursor moves to
 *				the bottom line.
 *
 * ARGUMENTS:	none
 *
 * RETURNS:		nothing
 */
void EmuViewdataCursorUp(const HHEMU hhEmu)
	{
	int iRow = hhEmu->emu_currow;

	(*hhEmu->emu_setcurpos)(hhEmu,
					(hhEmu->emu_currow > 0 ? --iRow : hhEmu->emu_maxrow),
					hhEmu->emu_curcol);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataCursorHome
 *
 * DESCRIPTION:	Moves cursor to upper left corner of screen.
 *
 * ARGUMENTS:	none
 *
 * RETURNS:		nothing
 */
void EmuViewdataCursorHome(const HHEMU hhEmu)
	{
	(*hhEmu->emu_setcurpos)(hhEmu, 0,0);
    return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataCursorSet
 *
 * DESCRIPTION:	Turns the cursor on and off.
 *
 * ARGUMENTS:	none
 *
 * RETURNS:		nothing
 */
void EmuViewdataCursorSet(const HHEMU hhEmu)
	{
	switch(hhEmu->emu_code)
		{
	case ETEXT('\x11'):
		(*hhEmu->EmuSetCursorType)(hhEmu, EMU_CURSOR_BLOCK);
		break;

	case ETEXT('\x14'):
		(*hhEmu->EmuSetCursorType)(hhEmu, EMU_CURSOR_NONE);
		break;

	default:
		break;
		}

    return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataMosaicHold
 *
 * DESCRIPTION: Displays the last displayed mosaic TCHAR inn every attribute
 *				space that was defined during mosaic mode. If no mosaic has
 *				been displayed since the last change in alpha/mosaic setting
 *				or normal/double height setting or the last mosaic release,
 *				a space is displayed instead.
 *
 * ARGUMENTS:	none
 *
 * RETURNS:		nothing
 */
void EmuViewdataMosaicHold(const HHEMU hhEmu)
	{
	hhEmu->emu_code = ETEXT('\x20');
	EmuViewdataCharDisplay(hhEmu);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataMosaicRelease
 *
 * DESCRIPTION:	Displays a space.
 *
 * ARGUMENTS:	none
 *
 * RETURNS:		nothing
 */
void EmuViewdataMosaicRelease(const HHEMU hhEmu)
	{
	/* TODO write this function */
	hhEmu->emu_code = ETEXT('\x20');
	EmuViewdataCharDisplay(hhEmu);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataSetAttr
 *
 * DESCRIPTION:	Sets colors and alpha/mosaic modes.
 *
 * ARGUMENTS:	none
 *
 * RETURNS:		nothing
 */
void EmuViewdataSetAttr(const HHEMU hhEmu)
	{
	const ECHAR uch = hhEmu->emu_code;
	const PSTVIEWDATAPRIVATE pstPRI = (PSTVIEWDATAPRIVATE)hhEmu->pvPrivate;

	// For readability.
	//
	PSTVIEWDATA *stAttr = pstPRI->apstVD;
	int iRow = hhEmu->emu_imgrow;
	int iCol = hhEmu->emu_curcol;
	unsigned int *aiColors = pstPRI->aMapColors;

	STATTR charattr;

	hhEmu->emu_code = ETEXT('\x20');

	pstPRI->fSetAttr = TRUE;

	if (uch >= ETEXT('\x41') && uch <= ETEXT('\x47'))  // A thru G
		{
		stAttr[iRow][iCol].attr = ALPHA_ATTR;
		stAttr[iRow][iCol].clr	= aiColors[uch - ETEXT('\x41')];
		}

	else if (uch >= ETEXT('\x51') && uch <= ETEXT('\x57')) // Q thru W
		{
		charattr = GetAttr(hhEmu, hhEmu->emu_currow, hhEmu->emu_curcol);
		stAttr[iRow][iCol].attr = MOSAIC_ATTR;
		stAttr[iRow][iCol].clr	= aiColors[uch - ETEXT('\x51')];
		}

	else
		{
		switch(uch)
			{
		case ETEXT('\x48'): //'H':
			stAttr[iRow][iCol].attr = FLASH_ATTR;
			break;

		case ETEXT('\x49'): //'I':
			stAttr[iRow][iCol].attr = STEADY_ATTR;
			break;

		case ETEXT('\x4C'): //'L':
			stAttr[iRow][iCol].attr = NORMALSIZE_ATTR;
			break;

		case ETEXT('\x4D'): //'M':
			stAttr[iRow][iCol].attr = DOUBLESIZE_ATTR;
			break;

		case ETEXT('\x58'): //'X':
			stAttr[iRow][iCol].attr = CONCEAL_ATTR;
			break;

		case ETEXT('\x59'): //'Y':
			stAttr[iRow][iCol].attr = CONTIGUOUS_ATTR;
			break;

		case ETEXT('\x5A'): //'Z':
			stAttr[iRow][iCol].attr = SEPARATED_ATTR;
			break;

		case ETEXT('\x5C'): //'\\':
			stAttr[iRow][iCol].attr = NEW_BACKGROUND_ATTR;
			stAttr[iRow][iCol].clr	= 0;
			break;

		case ETEXT('\x5D'): //']':
			stAttr[iRow][iCol].attr = NEW_BACKGROUND_ATTR;
			charattr = GetAttr(hhEmu, hhEmu->emu_currow, hhEmu->emu_curcol);
			stAttr[iRow][iCol].clr = charattr.txtclr;
			break;

		default:
			return;
			}
		}

	EmuViewdataCharDisplay(hhEmu);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataCharDisplay
 *
 * DESCRIPTION:	Displays a single character
 *
 * ARGUMENTS:	none
 *
 * RETURNS:		nothing
 */
void EmuViewdataCharDisplay(const HHEMU hhEmu)
	{
	int  iRow = hhEmu->emu_currow;
	int  iCol = hhEmu->emu_curcol;
	ECHAR *tp = hhEmu->emu_apText[hhEmu->emu_imgrow];
	PSTATTR ap = hhEmu->emu_apAttr[hhEmu->emu_imgrow];
	const PSTVIEWDATAPRIVATE pstPRI = (PSTVIEWDATAPRIVATE)hhEmu->pvPrivate;

    // TODO: Temporary until we get Prestel font

	if (hhEmu->emu_code == ETEXT('\x7F'))
		hhEmu->emu_code = ETEXT('\x5B');

	if (RowHasDblHigh(hhEmu, iRow))
        goto SKIP;

	// Need to GetAtt() before calling MapMosaics() so vars are set right.
	//
	ap[iCol] = GetAttr(hhEmu, iRow, iCol);
	tp[iCol] = MapMosaics(hhEmu, hhEmu->emu_code);

	// Update the end of row index if necessary.
	//
	if (iCol > hhEmu->emu_aiEnd[hhEmu->emu_imgrow])
		hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = iCol;

	/* --- check to see if we are overwriting an attribute space --- */

	if (!pstPRI->fSetAttr)
		{
		pstPRI->fSetAttr =
			(BOOL)pstPRI->apstVD[hhEmu->emu_imgrow][hhEmu->emu_curcol].attr;

		pstPRI->apstVD[hhEmu->emu_imgrow][hhEmu->emu_curcol].attr = 0;
		}

    updateChar(sessQueryUpdateHdl(hhEmu->hSession), iRow, iCol, iCol);

    if (ap[iCol].dblhihi)
        {
		const PSTATTR apl =
			hhEmu->emu_apAttr[row_index(hhEmu, hhEmu->emu_currow+1)];

		hhEmu->emu_apText[row_index(hhEmu, hhEmu->emu_currow+1)][iCol] =
			tp[iCol];

        apl[iCol] = ap[iCol];
        apl[iCol].dblhihi = 0;
		apl[iCol].dblhilo = 1;

		pstPRI->fSetAttr = TRUE;	// need to redisplay line to get lower half to show.
        }

	if (pstPRI->fSetAttr)
        {
		EmuViewdataDisplayLine(hhEmu, iRow, iCol);
		pstPRI->fSetAttr = FALSE;
        }

    SKIP:
	if (++iCol > hhEmu->emu_maxcol)
		{
		if (hhEmu->print_echo)
			printEchoLine(hhEmu->hPrintEcho,
							tp,
							emuRowLen(hhEmu, hhEmu->emu_imgrow));

		if (++iRow > hhEmu->emu_maxrow)
			iRow = 0;

		iCol = 0;
		}

	(*hhEmu->emu_setcurpos)(hhEmu, iRow, iCol);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataDisplayLine
 *
 * DESCRIPTION:	Redisplays the specified row from the specified column
 *				using the current emu_charattr and character type up to
 *				the end of the line (the 1st nul) or the next attribute
 *				space (whichever comes first).
 *
 * ARGUMENTS:
 *				sRow -- screen row to redisplay
 *				sCol -- screen column at which to start displaying
 *
 * RETURNS:		nothing
 */
static void EmuViewdataDisplayLine(const HHEMU hhEmu,
									const int iRow,
									const int iStartCol)
	{
	int iCol;
    int fDblHi = FALSE;
	ECHAR *tp = hhEmu->emu_apText[row_index(hhEmu, iRow)];
	PSTATTR ap = hhEmu->emu_apAttr[row_index(hhEmu, iRow)];
	const PSTATTR apl = hhEmu->emu_apAttr[row_index(hhEmu, iRow+1)];

	for (iCol = iStartCol ; iCol <= hhEmu->emu_maxcol ; ++iCol)
		{
		ap[iCol] = GetAttr(hhEmu, iRow, iCol);
		tp[iCol] = MapMosaics(hhEmu, tp[iCol]);

		if (iRow < hhEmu->emu_maxrow && ap[iCol].dblhihi)
            fDblHi = TRUE;
		}

	updateChar(sessQueryUpdateHdl(hhEmu->hSession),
						iRow,
						iStartCol,
						hhEmu->emu_maxcol);

    if (fDblHi)
        {
		for (iCol = 0 ; iCol <= hhEmu->emu_maxcol ; ++iCol)
            {
            apl[iCol].bkclr = ap[iCol].bkclr;

            if (!apl[iCol].dblhilo)
                apl[iCol].blank = 1;
            }

        updateLine(sessQueryUpdateHdl(hhEmu->hSession), iRow+1, iRow+1);
        }

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	GetAttr
 *
 * DESCRIPTION:
 *	Walks the current row and builds a composite attribute based on
 *	the encountered attribute spaces.
 *
 * ARGUMENTS:
 *	iRow	- logical row
 *	iCol	- logical col
 *
 * RETURNS:
 *	composite attribute.
 *
 */
static STATTR GetAttr(const HHEMU hhEmu, const int iRow, const int iCol)
	{
	int i;
	STATTR stAttr;
	const PSTVIEWDATAPRIVATE pstPRI = (PSTVIEWDATAPRIVATE)hhEmu->pvPrivate;
	const PSTVIEWDATA pstVD = pstPRI->apstVD[row_index(hhEmu, iRow)];
	
	memset(&stAttr, 0, sizeof(STATTR));
	stAttr.txtclr = VC_BRT_WHITE;
	stAttr.bkclr  = VC_BLACK;

	pstPRI->fMosaicMode = FALSE;
	pstPRI->fSeperatedMosaic = FALSE;

	for (i = 0 ; i <= iCol ; ++i)
		{
		if (pstVD[i].attr)
			{
			switch (pstVD[i].attr)
				{
			case ALPHA_ATTR:
				pstPRI->fMosaicMode = FALSE;
				stAttr.txtclr = pstVD[i].clr;
				stAttr.symbol = FALSE;
				break;

			case MOSAIC_ATTR:
				pstPRI->fMosaicMode = TRUE;
				stAttr.txtclr = pstVD[i].clr;
				stAttr.symbol = TRUE;
				break;

			case CONTIGUOUS_ATTR:
				pstPRI->fMosaicMode = TRUE;
				pstPRI->fSeperatedMosaic = FALSE;
				stAttr.txtclr = pstVD[i].clr;
				stAttr.symbol = TRUE;
				break;

			case SEPARATED_ATTR:
				pstPRI->fMosaicMode = TRUE;
				pstPRI->fSeperatedMosaic = TRUE;
				stAttr.txtclr = pstVD[i].clr;
				stAttr.symbol = TRUE;
				break;

			case NORMALSIZE_ATTR:
				stAttr.dblhihi = 0;
				break;

			case FLASH_ATTR:
				stAttr.blink = 1;
				break;

			case STEADY_ATTR:
				stAttr.blink = 0;
				break;

			case NEW_BACKGROUND_ATTR:
				stAttr.bkclr = pstVD[i].clr;
				break;

			case DOUBLESIZE_ATTR:
				stAttr.dblhihi = 1;
				break;

			case CONCEAL_ATTR:
				stAttr.blank = 0; // ??
				break;

			default:
				break;
				}
			}
		}

	return stAttr;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	MapMosaics
 *
 * DESCRIPTION:
 *	Since attributes that come before characters affect the character
 *	display (alpha vs. mosaic) we need to map mosaic chars back to their
 *	alpha counterparts and vice-versa at anytime.  Prestel uses 7 bit
 *	ascii so we can map the mosaics to the upper 128 bytes.  This
 *	function just checks the current mode (mosaic/alpha) and if the
 *	character is in the proper range, its converted to its counterpart.
 *	Also converts NULL to a space.	View data doesn't support an end
 *	of line concept and instead always fills to the end of line.
 *
 *	Note:  This function assumes GetAttr() has been called since it
 *		   relies on fMosaic and fSeperatedMosaic to be set.
 *
 * ARGUMENTS:
 *	ch	- character to convert
 *
 * RETURNS:
 *	converted or original character.
 *
 */
static ECHAR MapMosaics(const HHEMU hhEmu, ECHAR ch)
	{
	const PSTVIEWDATAPRIVATE pstPRI = (PSTVIEWDATAPRIVATE)hhEmu->pvPrivate;

	if (pstPRI->fMosaicMode)
		{
		// This is temporary until the fonts get straightend out.
		//
		if (ch > ETEXT('\x21') && ch <= ETEXT('\x3F'))
			ch += ETEXT('\x1F');

		if (pstPRI->fSeperatedMosaic)
			ch += ETEXT('\x80');
		}

	else // convert to equivalent alpha
		{
		if (ch > ETEXT('\x80'))
			ch -= ETEXT('\x80');
		}


	return ch;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	EmuViewdataClearScreen
 *
 * DESCRIPTION:
 *	We use AnsiFormFeed() to do most of the work but we have to
 *	clear viewdata's  attribute buffer as well.
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	void
 *
 */
void EmuViewdataClearScreen(const HHEMU hhEmu)
	{
	const PSTVIEWDATAPRIVATE pstPRI = (PSTVIEWDATAPRIVATE)hhEmu->pvPrivate;
	register int i;

	AnsiFormFeed(hhEmu);

	for (i = 0 ; i < hhEmu->emu_maxrow ; ++i)
		memset(pstPRI->apstVD[i],
				0,
				sizeof(STVIEWDATA) * VIEWDATA_COLS_40MODE);

    return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataKbd
 *
 * DESCRIPTION:
 *	 Keyboard routine for processing local keys on Viewdata terminal
 *
 * ARGUMENTS:
 *	 kcode	-	Key
 *
 * RETURNS:
 *	 nothing
 */
int EmuViewdataKbd(const HHEMU hhEmu, int kcode, const BOOL fTest)
	{
	switch (kcode)
		{
	case VK_ESCAPE | VIRTUAL_KEY:
		kcode = ETEXT('[') | CTRL_KEY;
		if (fTest)
			return kcode;
		break;

	case VK_TAB | VIRTUAL_KEY:
		kcode = ETEXT('I') | CTRL_KEY;
		if (fTest)
			return kcode;
		break;

	case VK_RETURN | VIRTUAL_KEY | CTRL_KEY:
		kcode = ETEXT('J') | CTRL_KEY;
		if (fTest)
			return kcode;
		break;
	default:
		break;
		}

	return std_kbdin(hhEmu, kcode, fTest);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * RowHasDblHigh
 *
 * DESCRIPTION:
 *  Checks if we are in the second row of a double high sequence. 
 *
 * ARGUMENTS:
 *	 void
 *
 * RETURNS:
 *	 0=FALSE, 1=TRUE
 */
static int RowHasDblHigh(const HHEMU hhEmu, const int iRow)
    {
    int i;
	const int r = row_index(hhEmu, iRow);
	const PSTATTR ap = hhEmu->emu_apAttr[r];

	if (hhEmu->emu_currow != 0)
        {
		for (i = 0 ; i < hhEmu->emu_maxcol ; ++i)
            {
            if (ap[i].dblhilo)
                return 1;
            }
        }

    return 0;
    }

#endif // INCL_VIEWDATA
/************************** end of viewdata.c *****************************/

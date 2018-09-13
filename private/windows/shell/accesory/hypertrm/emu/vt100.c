/*	File: D:\WACKER\emu\vt100.c (Created: 08-Dec-1993)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 9/20/99 5:33p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\tdll.h>
#include <tdll\tchar.h>
#include <tdll\session.h>
#include <tdll\chars.h>
#include <tdll\com.h>
#include <tdll\cloop.h>
#include <tdll\assert.h>
#include <tdll\print.h>
#include <tdll\capture.h>
#include <tdll\update.h>
#include <tdll\backscrl.h>
#include <tdll\mc.h>

#include "emu.h"
#include "emu.hh"
#include "emudec.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuDecGraphic
 *
 * DESCRIPTION:
 *	This function is called to display the normal range of characters
 *	for the emulators.	It handles insertion modes, end of line wrapping,
 *	and cursor positioning.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void emuDecGraphic(const HHEMU hhEmu)
	{
	ECHAR ccode = hhEmu->emu_code;
	ECHAR aechBuf[10];
	int iCharsToMove;
	int fDecColHold = FALSE;

	int iRow = hhEmu->emu_currow;
	int iCol = hhEmu->emu_curcol;

	ECHAR	*tp = hhEmu->emu_apText[hhEmu->emu_imgrow];
	PSTATTR ap = hhEmu->emu_apAttr[hhEmu->emu_imgrow];

	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	// Initialize a test flag.	This is used for a special case when
	// emulating a VT100 (and maybe other DEC emulators) and the
	// current column position is the maximum.	When a character is placed
	// at emu_maxcol, instead of advancing the cursor to column 0 of the
	// next line, it is instead placed under that last character.  When the
	// next character arrives, it is placed at column 0 on the next line, and
	// then the cursor is advanced as expected.
	//

	// Special DEC test.
	//
	if (hhEmu->mode_AWM && iCol == hhEmu->emu_maxcol)
		{
		if (pstPRI->fDecColHold)
			{
			fDecColHold = FALSE;

			CaptureLine(sessQueryCaptureFileHdl(hhEmu->hSession),
							CF_CAP_LINES,
							tp,
							emuRowLen(hhEmu, hhEmu->emu_imgrow));

			printEchoString(hhEmu->hPrintEcho,
							tp,
							emuRowLen(hhEmu, hhEmu->emu_imgrow));


			CnvrtMBCStoECHAR(aechBuf, sizeof(aechBuf), "\r\n", (unsigned long)StrCharGetByteCount("\r\n"));
			printEchoString(hhEmu->hPrintEcho,
							aechBuf,
							sizeof(ECHAR) * 2);

			if (iRow == hhEmu->bottom_margin)
				(*hhEmu->emu_scroll)(hhEmu, 1, TRUE);
			else
				++iRow;

			iCol = 0;
			(*hhEmu->emu_setcurpos)(hhEmu, iRow, iCol);

			tp = hhEmu->emu_apText[hhEmu->emu_imgrow];
			ap = hhEmu->emu_apAttr[hhEmu->emu_imgrow];
			}

		else
			{
			iCol = hhEmu->emu_maxcol;
			fDecColHold = TRUE;
			}
		}

	// Is the emulator in insert mode?
	//
	if (hhEmu->mode_IRM == SET)
		{
		iCharsToMove = (hhEmu->emu_aiEnd[hhEmu->emu_imgrow] - iCol + 1);

		if (iCharsToMove + iCol >= hhEmu->emu_maxcol)
			iCharsToMove -= 1;

		if (iCharsToMove > 0)
			{
			memmove(&tp[iCol+1],
					&tp[iCol],
					(unsigned)iCharsToMove * sizeof(ECHAR));

			memmove(&ap[iCol+1],
					&ap[iCol],
					(unsigned)iCharsToMove * sizeof(ECHAR));

			hhEmu->emu_aiEnd[hhEmu->emu_imgrow] =
				min(hhEmu->emu_aiEnd[hhEmu->emu_imgrow] + 1,
					hhEmu->emu_maxcol - 1);
			}
		}

	// Our competetor's are eating the NULL's.	DOS ANSI doesn't.
	// For now we'll try it their way... - mrw
	//
	if (ccode == (ECHAR)0)
		return;

	// Place the character and the current attribute into the image.
	//
	if (((hhEmu->stUserSettings.nEmuId == EMU_VT100) ||
				(hhEmu->stUserSettings.nEmuId == EMU_VT220) ||
				(hhEmu->stUserSettings.nEmuId == EMU_VT320)) &&
			ccode < sizeof(hhEmu->dspchar))
		ccode = hhEmu->dspchar[ccode];

	tp[iCol] = ccode;
	ap[iCol] = hhEmu->emu_charattr;

	// Check for double high, double wide processing.
	//
	if (pstPRI->aiLineAttr[hhEmu->emu_imgrow] != NO_LINE_ATTR)
		{
		int iColPrev = iCol;

		ap[iCol].dblwilf = 1;
		ap[iCol].dblwirt = 0;
		ap[iCol].dblhihi = (pstPRI->aiLineAttr[hhEmu->emu_imgrow] == DBL_WIDE_HI) ? 1 : 0;
		ap[iCol].dblhilo = (pstPRI->aiLineAttr[hhEmu->emu_imgrow] == DBL_WIDE_LO) ? 1 : 0;

		iCol = min(iCol+1, hhEmu->emu_maxcol);

		tp[iCol] = ccode;
		ap[iCol] = ap[iColPrev];
		ap[iCol].dblwilf = 0;
		ap[iCol].dblwirt = 1;
		}

#ifndef CHAR_NARROW
	// Process Double Byte Characters
	//
	if (QueryCLoopMBCSState(sessQueryCLoopHdl(hhEmu->hSession)))
		{
		if (isDBCSChar(ccode))
			{
			int iColPrev = iCol;

			ap[iCol].wilf = 1;
			ap[iCol].wirt = 0;

			iCol = min(iCol+1, hhEmu->emu_maxcol);

			tp[iCol] = ccode;
			ap[iCol] = ap[iColPrev];
			ap[iCol].wilf = 0;
			ap[iCol].wirt = 1;
			}
#if 0
        //mpt:1-23-98 handles the case when an incoming character
        //            (single or double byte) overwrites the first half of
        //            a double byte character
	    if ( iCol < hhEmu->emu_maxcol )
		    {
		    //if we orphaned a right half of a dbcs char
		    if (hhEmu->emu_apAttr[iRow][iCol + 1].wirt == TRUE)
			    {
			    //slide characters and attribs to left
                iCharsToMove = hhEmu->emu_aiEnd[hhEmu->emu_imgrow] - iCol - 1;
			    if (iCol + 2 < hhEmu->emu_maxcol && iCharsToMove > 0)
				    {
				    memmove(&tp[iCol + 1],
                            &tp[iCol + 2],
                            (unsigned)iCharsToMove * sizeof(ECHAR));

                    memmove(&ap[iCol + 1],
                            &ap[iCol + 2],
                            (unsigned)iCharsToMove * sizeof(ECHAR));
				    }
			    
			    //blank out character at end of line
			    tp[hhEmu->emu_aiEnd[hhEmu->emu_imgrow]] = 32;
			    ap[hhEmu->emu_aiEnd[hhEmu->emu_imgrow]].wirt = 0;
			    
                //move end of row since we removed a character
                hhEmu->emu_aiEnd[hhEmu->emu_imgrow]--;

                //update the image
                updateChar(sessQueryUpdateHdl(hhEmu->hSession),
				    hhEmu->emu_imgrow,
				    hhEmu->emu_aiEnd[hhEmu->emu_imgrow],
				    hhEmu->mode_IRM ?
				    hhEmu->emu_maxcol :
				    hhEmu->emu_aiEnd[hhEmu->emu_imgrow]);
                }	
    		}
#endif
        }
#endif //CHAR_NARROW

	// Update the end of row index if necessary.
	//
	if (iCol > hhEmu->emu_aiEnd[hhEmu->emu_imgrow])
		hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = iCol;

	// Update the image.
	//
	updateChar(sessQueryUpdateHdl(hhEmu->hSession),
				iRow,
				hhEmu->emu_curcol,
				hhEmu->mode_IRM ? hhEmu->emu_maxcol : iCol);

	// Move the position of the cursor ahead of the last character
	// displayed, checking for end of line wrap.
	//
	iCol += 1;

	// Finally, set the cursor position.  This wil reset emu_currow
	// and emu_curcol.
	//
	(*hhEmu->emu_setcurpos)(hhEmu, iRow, iCol);

	// Whenever we call setcurpos, it resets pstPRI->fDecColHold so
	// don't set till after we postion cursor.
	//
	pstPRI->fDecColHold = fDecColHold;
	return;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ANSI_DA
 *
 * DESCRIPTION:
 *	 Sends the device attribute (DA) report to the host.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ANSI_DA(const HHEMU hhEmu)
	{
	int fOldValue;
	ECHAR ech[15];

	CnvrtMBCStoECHAR(ech, sizeof(ech), "\033[?1;2c", 
					(unsigned long)StrCharGetByteCount("\033[?1;2c"));

	fOldValue = CLoopGetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession));

	CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), FALSE);

	emuSendString(hhEmu, ech, (int)StrCharGetEcharByteCount(ech)); 

	CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), fOldValue);

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt100_savecursor
 *
 * DESCRIPTION:
 *	 Saves the current cursor postion; and, it also
 *	 saves display attributes, character set, wrap mode, and origin mode.
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
void vt100_savecursor(const HHEMU hhEmu)
	{
	ECHAR sel;
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	sel = hhEmu->emu_code;

	if (sel == ETEXT('7')) /* save cursor & attributes */
		{
		(*hhEmu->emu_getcurpos)(hhEmu, &pstPRI->sv_row, &pstPRI->sv_col);
		pstPRI->sv_state = hhEmu->iCurAttrState;
		pstPRI->sv_attr = hhEmu->attrState[hhEmu->iCurAttrState];
		vt_charset_save(hhEmu);
		pstPRI->sv_AWM = hhEmu->mode_AWM;
		pstPRI->sv_DECOM = hhEmu->mode_DECOM;
		pstPRI->sv_protectmode = hhEmu->mode_protect;
		pstPRI->fAttrsSaved = TRUE;
		}

	else if (sel == ETEXT('8'))	  /* restore cursor and attr. */
		{
		(*hhEmu->emu_setcurpos)(hhEmu, pstPRI->sv_row, pstPRI->sv_col);

		//if (pstPRI->sv_col == hhEmu->emu_maxcol)
		//	  hhEmu->emu_curcol = pstPRI->sv_col;		 /* in order to wrap on next char */

		if (pstPRI->fAttrsSaved)
			{
			hhEmu->iCurAttrState = pstPRI->sv_state;

			hhEmu->attrState[hhEmu->iCurAttrState] = pstPRI->sv_attr;

			hhEmu->emu_charattr = hhEmu->attrState[hhEmu->iCurAttrState];
			}

		vt_charset_restore(hhEmu);

		hhEmu->mode_AWM = pstPRI->sv_AWM;
		hhEmu->mode_DECOM = pstPRI->sv_DECOM;
		hhEmu->mode_protect = pstPRI->sv_protectmode;
		}

	else		/* clear saved conditions */
		{
		pstPRI->sv_row = pstPRI->sv_col = 0;
		pstPRI->sv_state = hhEmu->iCurAttrState;
		pstPRI->sv_AWM = pstPRI->sv_DECOM = RESET;
		pstPRI->sv_protectmode = FALSE;
		pstPRI->fAttrsSaved = FALSE;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt100_answerback
 *
 * DESCRIPTION:
 *	 Sends the answerback message defined on the menus.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt100_answerback(const HHEMU hhEmu)
	{
	int fOldValue;
	TCHAR *sp;
	ECHAR *pech = NULL;

	sp = hhEmu->acAnswerback;

	// If there is nothing to send, there is nothing to send
	if (StrCharGetStrLength(sp) == 0)
		return;

	pech = malloc((unsigned int)StrCharGetByteCount(sp));
	if (pech == NULL)
		{
		assert(FALSE);
		return;
		}

	CnvrtMBCStoECHAR(pech, (unsigned long)StrCharGetByteCount(sp), sp, (unsigned long)StrCharGetByteCount(sp));

	/* to not get recursive answerback's in half duplex */

	fOldValue = CLoopGetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession));

	CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), FALSE);

	emuSendString(hhEmu, pech, (int)StrCharGetEcharByteCount(pech));
	free(pech);
	pech = NULL;

	CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), fOldValue);

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt100_hostreset
 *
 * DESCRIPTION:
 *	 Calls vt100_reset() when told to reset by the host.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt100_hostreset(const HHEMU hhEmu)
	{
	vt100_reset(hhEmu, TRUE);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt100_reset
 *
 * DESCRIPTION:
 *	 Resets the emulator.
 *
 * ARGUMENTS:
 *	 host_request -- TRUE when told to reset by the host.
 *
 * RETURNS:
 *	 nothing
 */
int vt100_reset(const HHEMU hhEmu, const int host_request)
	{
	hhEmu->mode_KAM = hhEmu->mode_IRM = hhEmu->mode_VEM =
	hhEmu->mode_HEM = hhEmu->mode_LNM = hhEmu->mode_DECCKM =
	hhEmu->mode_DECOM = hhEmu->mode_DECCOLM = hhEmu->mode_DECPFF =
	hhEmu->mode_DECPEX = hhEmu->mode_DECSCNM = hhEmu->mode_25enab =
	hhEmu->mode_protect = hhEmu->mode_block =
	hhEmu->mode_local = RESET;

	hhEmu->mode_SRM = hhEmu->mode_DECTCEM = SET;

	hhEmu->mode_AWM = hhEmu->stUserSettings.fWrapLines;

	vt_charset_init(hhEmu);
	if (host_request)
		{
		ANSI_Pn_Clr(hhEmu);
		ANSI_SGR(hhEmu);
		ANSI_RIS(hhEmu);
		}
	hhEmu->emu_code = ETEXT('>');

	vt_alt_kpmode(hhEmu);

	if (hhEmu->stUserSettings.nEmuId == EMU_ANSI ||
		hhEmu->stUserSettings.nEmuId == EMU_AUTO)
		hhEmu->emu_kbdin = ansi_kbdin;
	else
		hhEmu->emu_kbdin = vt100_kbdin;

	hhEmu->mode_AWM = RESET;
	hhEmu->stUserSettings.fWrapLines = RESET;

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt100_report
 *
 * DESCRIPTION:
 *	 Sends the current terminal parameters specified by the DECREQTPARM.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt100_report(const HHEMU hhEmu)
	{
	int sol, i = 0, xspeed = 0;
	TCHAR str[20];
	TCHAR tempstr[4];
	ECHAR ech[20];
	int fOldValue;
	TCHAR *sp;
	long lBaud;
	int nDataBits, nParity;

	HCOM hCom;

	static int baudrates[] = {50, 75, 110, 135, 150, 200, 300, 600, 1200,
								 1800, 2000, 2400, 2600, 4800, 9600, 19200};

	sol = hhEmu->num_param[0];
	if (sol != 0 && sol != 1)
		return;

	wsprintf(str, TEXT("\x01B[%s;"),
				sol == 0 ? (LPTSTR)TEXT("2") : (LPTSTR)TEXT("3"));

	hCom = sessQueryComHdl(hhEmu->hSession);
	ComGetBaud(hCom, &lBaud);
	ComGetDataBits(hCom, &nDataBits);
	ComGetParity(hCom, &nParity);

	// Parity
	//
	if (nParity == 0)		// None
		StrCharCat(str, TEXT("1;"));
	else if (nParity == 1)	// Odd
		StrCharCat(str, TEXT("4;"));
	else 					// Even
		StrCharCat(str, TEXT("5;"));

	// Data bits
	//
	nDataBits == 8 ? StrCharCat(str, TEXT("1;")) : StrCharCat(str, TEXT("2;"));

	while (xspeed < 120)
		{
		if ((long)baudrates[i] >= lBaud)
			break;
		i++;
		xspeed += 8;
		}

	// Recieving speed
	//
	wsprintf(tempstr, "%d", xspeed);
	StrCharCat(str, tempstr);

	// Sending speed
	//
	StrCharCat(str, TEXT(";"));
	StrCharCat(str, tempstr);

	// Bit rate multiplier ; Flags
	//
	StrCharCat(str, TEXT(";1;0x"));

	sp = str;

	CnvrtMBCStoECHAR(ech, sizeof(ech), sp, (unsigned long)StrCharGetByteCount(sp));

	/* to not get recursive vt100_report's if half duplex */

	fOldValue = CLoopGetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession));

	CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), FALSE);

	emuSendString(hhEmu, ech, (int)StrCharGetEcharByteCount(ech));

	CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), fOldValue);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt100_kbdin
 *
 * DESCRIPTION:
 *	 Processes local keyboard keys for the VT100 emulator.
 *	 Note: mode_DECKPAM is TRUE or SET when in DEC KeyPad Aplication Mode.
 *	 Removed key processing 1/3/92.  Will replace with something resonable
 *	 later - mrw.
 *
 * ARGUMENTS:
 *	 key -- key to process
 *
 * RETURNS:
 *	 nothing
 */
int vt100_kbdin(const HHEMU hhEmu, int key, const int fTest)
	{
	int index;
	/* -------------- Check Backspace & Delete keys ------------- */

	if (hhEmu->stUserSettings.fReverseDelBk && ((key == VK_BACKSPACE) ||
			(key == DELETE_KEY) || (key == DELETE_KEY_EXT)))
		{
		key = (key == VK_BACKSPACE) ? DELETE_KEY : VK_BACKSPACE;
		}

	/* -------------- Mapped PF1-PF4 keys ------------- */

    #if 0 // mrw:11/3/95 - removed because we can't control num-lock
          // in Win95
	if (hhEmu->stUserSettings.fMapPFkeys &&
			(index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl4)) != -1)
		{
		if (!fTest)
			emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl4);
		}
    #endif

	/* -------------- Cursor Key Mode ------------- */

	else if (hhEmu->mode_DECCKM == SET &&
			(index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl3)) != -1)
		{
		if (!fTest)
			emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl3);
		}

	/* -------------- Keypad Application Mode ------------- */

	else if (hhEmu->mode_DECKPAM &&
			(index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl2)) != -1)
		{
		if (!fTest)
			emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl2);
		}

	/* -------------- Normal keys ------------- */

	else if ((index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl)) != -1)
		{
		if (!fTest)
			emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl);
		}

	else
		{
		index = std_kbdin(hhEmu, key, fTest);
		}

	return index;
	}

#if FALSE	// Never used in HyperTerminal.
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * fakevt_kbdin
 *
 * DESCRIPTION:
 *	 Processes local keyboard keys for the WANG, IBM3278 & RENX3278 emulators.
 *
 * ARGUMENTS:
 *	 key -- key to process
 *
 * RETURNS:
 *	 nothing
 */
int fakevt_kbdin(const HHEMU hhEmu, int key, const int fTest)
	{
	int index;

	/* -------------- Check Backspace & Delete keys ------------- */

	if (hhEmu->stUserSettings.fReverseDelBk && ((key == VK_BACKSPACE) ||
			(key == DELETE_KEY) || key == DELETE_KEY_EXT))
		{
		key = (key == VK_BACKSPACE) ? DELETE_KEY : VK_BACKSPACE;
		}

	if ((index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl)) != -1)
		{
		if (!fTest)
			emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl);
		}

	else
		{
		index = std_kbdin(hhEmu, key, fTest);
		}

	return index;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt100PrintCommands
 *
 * DESCRIPTION:
 *	 Processes VT100 printing commands.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt100PrintCommands(const HHEMU hhEmu)
	{
	int line;
	int from;
	int to;
	ECHAR sel;
	ECHAR aechBuf[10];

	sel = (ECHAR)hhEmu->selector[0];

	switch (sel)
		{
		// Auto print on.
		//
		case ETEXT(0xF5):
			hhEmu->print_echo = TRUE;
			printSetStatus(hhEmu->hPrintEcho, TRUE);
			break;

		// Auto print off.
		//
		case ETEXT(0xF4):
			hhEmu->print_echo = FALSE;
			printSetStatus(hhEmu->hPrintEcho, FALSE);
    		printEchoClose(hhEmu->hPrintEcho);

			break;

		// Print screen.
		//
		case ETEXT(0x00):
			if (hhEmu->mode_DECPEX == RESET)
				from = hhEmu->top_margin, to = hhEmu->bottom_margin;
			else
				from = 0, to = EMU_DEFAULT_MAXROW;

			for (line = from; line <= to; ++line)
				printEchoLine(hhEmu->hPrintHost,
								hhEmu->emu_apText[row_index(hhEmu, line)],
								emuRowLen(hhEmu, row_index(hhEmu, line)));

			if (hhEmu->mode_DECPFF == SET)	 /* print form feed */
				{
				CnvrtMBCStoECHAR(aechBuf, sizeof(aechBuf), "0x0C", (unsigned long)StrCharGetByteCount("0x0C"));
				printEchoLine(hhEmu->hPrintHost, aechBuf, sizeof(ECHAR));
				}

			break;

		// Print Cursor line.
		//
		case ETEXT(0xF1):
			printEchoLine(hhEmu->hPrintEcho,
							hhEmu->emu_apText[row_index(hhEmu,
							hhEmu->emu_currow)],
							emuRowLen(hhEmu,
										row_index(hhEmu, hhEmu->emu_currow)));
			break;

		// Enter printer controller mode.  State is hard coded for now...
		// Bad news.
		//
		case ETEXT(0x05):
			hhEmu->state = 6;
			printSetStatus(hhEmu->hPrintHost, TRUE);
			break;

		// Exit printer controller mode.  This is seen when not in
		// controller mode.
		//
		case ETEXT(0x04):
			break;

		default:
			break;
		}
	}

void vt100_prnc(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;
	ECHAR aechBuf[10];
	size_t size;

	*pstPRI->pntr++ = hhEmu->emu_code;
	*pstPRI->pntr = 0;
	++pstPRI->len_s;

	CnvrtMBCStoECHAR(aechBuf, sizeof(aechBuf), "\033[4i", (unsigned long)StrCharGetByteCount("\033[4i"));

	size = (size_t)StrCharGetByteCount("\033[4i");
	if ((pstPRI->len_s >= 4) && (memcmp(pstPRI->pntr - 4, aechBuf, size) == 0))
		{
		/* received termination string, wrap it up */
        printEchoRaw(hhEmu->hPrintHost, pstPRI->storage, pstPRI->len_s - ((*(pstPRI->pntr - 3) == (TCHAR)TEXT('\233')) ? 3 : 4));
//		emuPrintChars(hhEmu, pstPRI->storage, pstPRI->len_s - ((*(pstPRI->pntr - 3) == (TCHAR)TEXT('\233')) ? 3 : 4));
//		printEchoChar(hhEmu->hPrintHost, ETEXT('\n'));
		pstPRI->pntr = pstPRI->storage;
		pstPRI->len_s = 0;
		hhEmu->state = 0;  /* drop out of this routine */

		// Finish-up print job
		DbgOutStr("print-control off\r\n", 0, 0, 0, 0, 0);
		printEchoClose(hhEmu->hPrintHost);
		return;
		}

	/* haven't received termination sequence yet, is storage filled? */
	if (pstPRI->len_s >= (int)(sizeof(pstPRI->storage) - 1))
		{
		/* copy most of string to print buffer */
        printEchoRaw(hhEmu->hPrintHost, pstPRI->storage, pstPRI->len_s - 4);
//		emuPrintChars(hhEmu, pstPRI->storage, pstPRI->len_s - 4);

		/* move end of string to beginning of storage */
		memmove(pstPRI->storage, &pstPRI->storage[pstPRI->len_s - 4], 4 * sizeof(ECHAR));
		pstPRI->pntr = pstPRI->storage + 4;
		pstPRI->len_s = 4;
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
void emuSetDoubleAttr(const HHEMU hhEmu)
	{

	switch(hhEmu->emu_code)
		{
		// Double width, double height, top half.
		//
		case '3':
			emuSetDoubleAttrRow(hhEmu, DBL_WIDE_HI);
			break;

		// Double width double height, bottom half.
		//
		case '4':
			emuSetDoubleAttrRow(hhEmu, DBL_WIDE_LO);
			break;

		// Single width single height.
		//
		case '5':
			emuSetSingleAttrRow(hhEmu);
			break;

		// Double width, single height.
		//
		case '6':
			emuSetDoubleAttrRow(hhEmu, DBL_WIDE_SINGLE_HEIGHT);
			break;

		default:
			break;
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
void emuSetSingleAttrRow(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	int iOldRow,
		iOldCol,
		iImgRow;

	// Determine the image row.
	//
	iImgRow = row_index(hhEmu, hhEmu->emu_currow);

	// If the current line attribute is the same as the one we're
	// being asked to change to, get out-a-town.
	//
	if (pstPRI->aiLineAttr[iImgRow] == NO_LINE_ATTR)
		return;

	// Get the current cursor position.
	//
	std_getcurpos(hhEmu, &iOldRow, &iOldCol);

	// Convert the current row.
	//
	emuFromDblToSingle(hhEmu);

	// Update the line.
	//
	updateLine(sessQueryUpdateHdl(hhEmu->hSession),
				hhEmu->emu_currow,
				hhEmu->emu_currow);

	// Put the cursor back to where it was, or at the new rightmost
	// margin, whichever is less.
	//
	iOldCol = iOldCol / 2;
	iOldCol = min(iOldCol, hhEmu->emu_maxcol);
	std_setcurpos(hhEmu, iOldRow, iOldCol);

	// Finally, update this rows line attribute value.
	//
	pstPRI->aiLineAttr[iImgRow] = NO_LINE_ATTR;

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
void emuSetDoubleAttrRow(const HHEMU hhEmu, const int iLineAttr)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	int iChar,
		iImgRow,
		iOldRow,
		iOldCol,
		iUpperAttr,
		iLowerAttr;

	// Determine the image row.
	//
	iImgRow = row_index(hhEmu, hhEmu->emu_currow);

	// If the current line attribute is the same as the one we're
	// being asked to change to, get out-a-town.
	//
	if (pstPRI->aiLineAttr[iImgRow] == iLineAttr)
		return;

	// Get the current cursor position.
	//
	std_getcurpos(hhEmu, &iOldRow, &iOldCol);

	// If the current line attribute is anything but NO_LINE_ATTR, call
	// a routine that will first put the row back into that state.	That
	// is, this routine converts FROM a standard line INTO a double wide
	// line.
	//
	if (pstPRI->aiLineAttr[iImgRow] != NO_LINE_ATTR)
		{
		emuFromDblToSingle(hhEmu);
		iOldCol = iOldCol / 2;
		}

	// Start a shell game!
	//
	iChar = (hhEmu->emu_maxcol+1) / 2;

	// Remember that both of the following values will be zero in the
	// case of processing a DBL_WIDE_SINGLE_HEIGHT	request.
	//
	iUpperAttr = (iLineAttr == DBL_WIDE_HI) ? 1 : 0;
	iLowerAttr = (iLineAttr == DBL_WIDE_LO) ? 1 : 0;

	while (iChar >= 0)
		{
		hhEmu->emu_apText[iImgRow][(iChar * 2) + 1] = hhEmu->emu_apText[iImgRow][iChar];
		hhEmu->emu_apAttr[iImgRow][(iChar * 2) + 1].dblwirt = 1;
		hhEmu->emu_apAttr[iImgRow][(iChar * 2) + 1].dblwilf = 0;
		hhEmu->emu_apAttr[iImgRow][(iChar * 2) + 1].dblhihi = (unsigned)iUpperAttr;
		hhEmu->emu_apAttr[iImgRow][(iChar * 2) + 1].dblhilo = (unsigned)iLowerAttr;

		hhEmu->emu_apText[iImgRow][(iChar * 2)]  = hhEmu->emu_apText[iImgRow][iChar];
		hhEmu->emu_apAttr[iImgRow][(iChar * 2)].dblwirt = 0;
		hhEmu->emu_apAttr[iImgRow][(iChar * 2)].dblwilf = 1;
		hhEmu->emu_apAttr[iImgRow][(iChar * 2)].dblhihi = (unsigned)iUpperAttr;
		hhEmu->emu_apAttr[iImgRow][(iChar * 2)].dblhilo = (unsigned)iLowerAttr;

		iChar--;
		}

	// Null terminate the new text image.
	//
	hhEmu->emu_apText[iImgRow][hhEmu->emu_maxcol + 1] = ETEXT('\0');

	// Update the line.
	//
	updateLine(sessQueryUpdateHdl(hhEmu->hSession),
				hhEmu->emu_currow,
				hhEmu->emu_currow);

	// Put the cursor back to where it was, or at the new rightmost
	// margin, whichever is less.
	//
	iOldCol = iOldCol * 2;
	iOldCol = min(iOldCol, hhEmu->emu_maxcol);
	std_setcurpos(hhEmu, iOldRow, iOldCol);

	// Finally, update this rows line attribute value.
	//
	pstPRI->aiLineAttr[iImgRow] = iLineAttr;

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuFromDblToSingle
 *
 * DESCRIPTION:
 *	Note that this is a utility function and does not update the emulator
 *	image.	The calling function should do this.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void emuFromDblToSingle(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	register int i;

	int iSource,
		iDest,
		iLastCol,
		iImgRow;

	// Determine the image row.
	//
	iImgRow = row_index(hhEmu, hhEmu->emu_currow);

	// If the current line attribute is the same as the one we're
	// being asked to change to, get out-a-town.
	//
	if (pstPRI->aiLineAttr[iImgRow] == NO_LINE_ATTR)
		return;

	// A new shell game.
	//
	iSource = 0;
	iDest  = 0;

	iLastCol = (hhEmu->emu_maxcol+1) / 2;

	// TODO:  JCM - in two location below, the text buffer is set to
	// spaces, instead of nulls.  Figure out why this is the case.	We
	// should be able to set these to nulls.

	while (iSource <= hhEmu->emu_maxcol)
		{
		if (hhEmu->emu_apText[iImgRow][iSource] == ETEXT('\0'))  // TODO 1:
			hhEmu->emu_apText[iImgRow][iDest] = ETEXT(' ');
		else
			hhEmu->emu_apText[iImgRow][iDest] =
				hhEmu->emu_apText[iImgRow][iSource];

		hhEmu->emu_apAttr[iImgRow][iDest].dblwirt = 0;
		hhEmu->emu_apAttr[iImgRow][iDest].dblwilf = 0;
		hhEmu->emu_apAttr[iImgRow][iDest].dblhihi = 0;
		hhEmu->emu_apAttr[iImgRow][iDest].dblhilo = 0;

		iSource += 2;
		iDest += 1;
		}

	for (i = iLastCol; i < MAX_EMUCOLS ; ++i)
		{
		hhEmu->emu_apText[iImgRow][i] = ETEXT(' ');		  // TODO 2:
		hhEmu->emu_apAttr[iImgRow][i] = hhEmu->emu_clearattr;
		/*
		hhEmu->emu_apAttr[iImgRow][i].dblwirt = 0;
		hhEmu->emu_apAttr[iImgRow][i].dblwilf = 0;
		hhEmu->emu_apAttr[iImgRow][i].dblhihi = 0;
		hhEmu->emu_apAttr[iImgRow][i].dblhilo = 0;
		*/
		}

	pstPRI->aiLineAttr[iImgRow] = NO_LINE_ATTR;

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
void emuDecTab(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	int col;

	col = hhEmu->emu_curcol;

	while (col <= hhEmu->emu_maxcol)
		if (hhEmu->tab_stop[++col])
			break;

	if (pstPRI->aiLineAttr[hhEmu->emu_imgrow] != NO_LINE_ATTR)
		col = (col * 2) - 1;

	(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow, col);

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecCUF
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
void emuDecCUF(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	int nchars, col;

	nchars = hhEmu->num_param[hhEmu->num_param_cnt];

	if (nchars < 1)
		nchars = 1;

	if (pstPRI->aiLineAttr[hhEmu->emu_imgrow])
		nchars = (nchars * 2);

	col = hhEmu->emu_curcol;
	col += nchars;

	(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow, col);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecCUP
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
void emuDecCUP(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	int row, col;

	row = hhEmu->num_param[0];
	col = hhEmu->num_param_cnt > 0 ? hhEmu->num_param[1] : 0;

	if (pstPRI->aiLineAttr[row_index(hhEmu, row)] != NO_LINE_ATTR)
		col = (col * 2) - 1;

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
	else				/* Position is one-based from upper left */
		{
		if (row > hhEmu->emu_maxrow + 1)
			row = hhEmu->emu_maxrow + 1;
		}

	if (col > hhEmu->emu_maxcol + 1)
		col = hhEmu->emu_maxcol + 1;

	(*hhEmu->emu_setcurpos)(hhEmu, row - 1, col - 1); /* ANSI is one-based, HA zero-based */
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecCUB
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
void emuDecCUB(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	int nchars;

	nchars = hhEmu->num_param[hhEmu->num_param_cnt];

	if (nchars < 1)
		nchars = 1;

	if (pstPRI->aiLineAttr[hhEmu->emu_imgrow])
		nchars = (nchars * 2);

	(*hhEmu->emu_setcurpos)(hhEmu,
							hhEmu->emu_currow,
							hhEmu->emu_curcol - nchars);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecED
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
void emuDecED(const HHEMU hhEmu)
	{
	int selector = hhEmu->selector[0];

	switch (selector)
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
		break;
	default:
		commanderror(hhEmu);
		break;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecUnload
 *
 * DESCRIPTION:
 *	 Unloads current emulator by freeing used memory.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void emuVT100Unload(const HHEMU hhEmu)
	{
	PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	assert(hhEmu);

	if (pstPRI)
		{
		if (pstPRI->aiLineAttr)
			{
			free(pstPRI->aiLineAttr);
			pstPRI->aiLineAttr = 0;
			}

		free(hhEmu->pvPrivate);
		hhEmu->pvPrivate = 0;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecIND
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
void emuDecIND(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	int fSourceIsDbl, fDestIsDbl;
	int iCol;

	// If we're at the bottom line, scroll.
	//
	if (hhEmu->emu_currow == hhEmu->bottom_margin)
		{
		(*hhEmu->emu_scroll)(hhEmu, 1, TRUE);
		return;
		}

	fSourceIsDbl = (pstPRI->aiLineAttr[row_index(hhEmu, hhEmu->emu_currow)] == NO_LINE_ATTR)
					? FALSE : TRUE;
	fDestIsDbl = (pstPRI->aiLineAttr[row_index(hhEmu, hhEmu->emu_currow + 1)] == NO_LINE_ATTR)
					? FALSE : TRUE;

	iCol = hhEmu->emu_curcol;

	// If both source and dest are the same, regardless of size, go
	// ahead and make the move.  It only matters if they're different.
	//
	if (fSourceIsDbl == fDestIsDbl) 	// Both the same
		iCol = iCol;
	else if (fSourceIsDbl)				// Source is double, dest is single.
		iCol = iCol / 2;
	else								// Source is singel, dest is double.
		iCol = iCol * 2;

	(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow + 1, iCol);

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecRI
 *
 * DESCRIPTION:
 *	 Moves cursor up 1 line and scrolls 1 line if necessary.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void emuDecRI(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	int fSourceIsDbl, fDestIsDbl;
	int iCol;

	// If we're at the bottom line, scroll.
	//
	if (hhEmu->emu_currow == hhEmu->top_margin)
		{
		(*hhEmu->emu_scroll)(hhEmu, 1, FALSE);
		return;
		}

	fSourceIsDbl = (pstPRI->aiLineAttr[row_index(hhEmu, hhEmu->emu_currow)] == NO_LINE_ATTR)
					? FALSE : TRUE;
	fDestIsDbl = (pstPRI->aiLineAttr[row_index(hhEmu, hhEmu->emu_currow - 1)] == NO_LINE_ATTR)
					? FALSE : TRUE;

	iCol = hhEmu->emu_curcol;

	// If both source and dest are the same, regardless of size, go
	// ahead and make the move.  It only matters if they're different.
	//
	if (fSourceIsDbl == fDestIsDbl) 	// Both the same
		iCol = iCol;
	else if (fSourceIsDbl)				// Source is double, dest is single.
		iCol = iCol / 2;
	else								// Source is singel, dest is double.
		iCol = iCol * 2;

	(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow - 1, iCol);

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecCUU
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
void emuDecCUU(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	int nlines,
		row,
		fSourceIsDbl,
		fDestIsDbl,
		iCol;

	nlines = hhEmu->num_param[hhEmu->num_param_cnt];

	if (nlines < 1)
		nlines = 1;

	row = hhEmu->emu_currow;
	row -= nlines;

	fSourceIsDbl = (pstPRI->aiLineAttr[row_index(hhEmu, hhEmu->emu_currow)] == NO_LINE_ATTR)
					? FALSE : TRUE;
	fDestIsDbl = (pstPRI->aiLineAttr[row_index(hhEmu, hhEmu->emu_currow - nlines)] == NO_LINE_ATTR)
					? FALSE : TRUE;

	iCol = hhEmu->emu_curcol;

	// The following code adjusts the column value for double wide
	// characters.
	//
	if (fSourceIsDbl == fDestIsDbl) 	// Both the same
		iCol = iCol;
	else if (fSourceIsDbl)				// Source is double, dest is single.
		iCol = iCol / 2;
	else								// Source is singel, dest is double.
		iCol = iCol * 2;

	if (row < hhEmu->top_margin && (hhEmu->emu_currow >= hhEmu->top_margin || hhEmu->emu_currow < 0))
		row = hhEmu->top_margin;

	(*hhEmu->emu_setcurpos)(hhEmu, row, iCol);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecCUD
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
void emuDecCUD(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	int nlines,
		row,
		fSourceIsDbl,
		fDestIsDbl,
		iCol;

	nlines = hhEmu->num_param[hhEmu->num_param_cnt];

	if (nlines < 1)
		nlines = 1;

	row = hhEmu->emu_currow;
	row += nlines;

	fSourceIsDbl = (pstPRI->aiLineAttr[row_index(hhEmu, hhEmu->emu_currow)] == NO_LINE_ATTR)
					? FALSE : TRUE;
	fDestIsDbl = (pstPRI->aiLineAttr[row_index(hhEmu, hhEmu->emu_currow + nlines)] == NO_LINE_ATTR)
					? FALSE : TRUE;

	iCol = hhEmu->emu_curcol;

	// The following code adjusts the column value for double wide
	// characters.
	//
	if (fSourceIsDbl == fDestIsDbl) 	// Both the same
		iCol = iCol;
	else if (fSourceIsDbl)				// Source is double, dest is single.
		iCol = iCol / 2;
	else								// Source is singel, dest is double.
		iCol = iCol * 2;

	if (row > hhEmu->bottom_margin &&
				(hhEmu->emu_currow <= hhEmu->bottom_margin || hhEmu->emu_currow > hhEmu->emu_maxrow))
		row = hhEmu->bottom_margin;

	(*hhEmu->emu_setcurpos)(hhEmu, row, iCol);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecSetCurPos
 *
 * DESCRIPTION:
 *	 Moves the cursor to the specified position on the virtual screen.
 *	 If the cursor is beyond the end of existing text, the virtual screen
 *	 line is filled out with spaces. If the cursor is beyond the edges of
 *	 the video display, the video cursor is placed as close as possible
 *	 to the desired position as the cursor display is changed.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *	 nothing
 */
void emuDecSetCurPos(const HHEMU hhEmu, const int iRow, const int iCol)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	int iTempCol;
	int i;

	// If we  move the cursor, we need to clear the pstPRI->fDecHoldFlag;
	//
	pstPRI->fDecColHold = FALSE;

	hhEmu->emu_currow = max(min(iRow, hhEmu->emu_maxrow), 0);
	hhEmu->emu_curcol = max(min(iCol, hhEmu->emu_maxcol), 0);

	iTempCol = hhEmu->emu_curcol;

	// If the row is a double wide character row, don't ever let the
	// cursor land on an odd column number.  If it's there now, back
	// it up one.
	//
	i = row_index(hhEmu, hhEmu->emu_currow);

	if (pstPRI->aiLineAttr[i])
		{
		if (iTempCol  % 2 == 1)
			{
			iTempCol -= 1;
			}
		}

	updateCursorPos(sessQueryUpdateHdl(hhEmu->hSession), iRow, iTempCol);
	hhEmu->emu_imgrow = row_index(hhEmu, hhEmu->emu_currow);
	return;
#if 0
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	int iTempCol;

	hhEmu->emu_currow = max(min(iRow, hhEmu->emu_maxrow), 0);
	hhEmu->emu_curcol = max(min(iCol, hhEmu->emu_maxcol), 0);

	iTempCol = hhEmu->emu_curcol;

	// Do range checking for DEC emulation.  This prevents the cursor
	// from being displayed in the 81st position, which is a valid
	// internal location, but is not a valid display column.
	//
	if (hhEmu->emu_curcol == hhEmu->emu_maxcol &&
		    (hhEmu->stUserSettings.nEmuId == EMU_VT100 ||
		     hhEmu->stUserSettings.nEmuId == EMU_VT100J))
        {
		iTempCol -= 1;
		}

	// If the row is a double wide character row, don't ever let the
	// cursor land on an odd column number.  If it's there now, back
	// it up one.
	//
	if (pstPRI->aiLineAttr[row_index(hhEmu, hhEmu->emu_currow)])
		{
		if (iTempCol  % 2 == 1)
			{
			iTempCol -= 1;
			}
		}

	updateCursorPos(sessQueryUpdateHdl(hhEmu->hSession), iRow, iTempCol);

	hhEmu->emu_imgrow = row_index(hhEmu, hhEmu->emu_currow);
	return;
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecClearScreen
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
void emuDecClearScreen(const HHEMU hhEmu, const int nClearSelect)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;
	ECHAR aechBuf[10];
	register int r;
	int trow, tcol;
	PSTATTR pstAttr;
	BOOL fSave;

	trow = hhEmu->emu_currow;
	tcol = hhEmu->emu_curcol;

	switch (nClearSelect)
		{
	/* cursor to end of screen */
	case 0:
		fSave = (hhEmu->emu_currow == 0  &&
					hhEmu->emu_curcol == 0) ? TRUE : FALSE;

		for (r = hhEmu->emu_currow + (fSave ? 0 : 1) ; r < MAX_EMUROWS; ++r)
			{
			if (fSave)
				{
				backscrlAdd(sessQueryBackscrlHdl(hhEmu->hSession),
							hhEmu->emu_apText[row_index(hhEmu, r)],
							hhEmu->emu_maxcol+1);

				CaptureLine(sessQueryCaptureFileHdl(hhEmu->hSession),
									CF_CAP_SCREENS,
									hhEmu->emu_apText[row_index(hhEmu, r)],
									emuRowLen(hhEmu, row_index(hhEmu, r)));

				printEchoScreen(hhEmu->hPrintEcho,
									hhEmu->emu_apText[row_index(hhEmu, r)],
									emuRowLen(hhEmu, row_index(hhEmu, r)));

				CnvrtMBCStoECHAR(aechBuf, sizeof(aechBuf), "\r\n", 
								(unsigned long)StrCharGetByteCount("\r\n"));
				printEchoScreen(hhEmu->hPrintEcho,
									aechBuf,
									sizeof(ECHAR) * 2);
				}

			clear_imgrow(hhEmu, r);

			pstPRI->aiLineAttr[row_index(hhEmu, r)] = NO_LINE_ATTR;
			}

		// Clear the partial row now.
		//
		ECHAR_Fill(hhEmu->emu_apText[row_index(hhEmu, hhEmu->emu_currow)] +
						hhEmu->emu_curcol,
						EMU_BLANK_CHAR,
						(size_t)(MAX_EMUCOLS - hhEmu->emu_curcol + 1));

		if (hhEmu->emu_curcol <= hhEmu->emu_aiEnd[hhEmu->emu_imgrow])
			hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = hhEmu->emu_curcol - 1;

		pstAttr = hhEmu->emu_apAttr[row_index(hhEmu, hhEmu->emu_currow)];

		for (r = hhEmu->emu_curcol ; r < MAX_EMUCOLS ; ++r)
			pstAttr[r] = hhEmu->emu_clearattr;

		// Tell the video image what to do.  Use the emuDispRgnScrollUp() call
		// instead of RgnClear so edges of terminal get painted if
		// clear attribute changes.

		updateScroll(sessQueryUpdateHdl(hhEmu->hSession),
						0,
						hhEmu->emu_maxrow,
						hhEmu->emu_maxrow + 1,
						hhEmu->emu_imgtop,
						TRUE);

		(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow, hhEmu->emu_curcol);

		// Added a global to save the clear attribute at the time of
		// notification.  This is necessary since the message is posted
		// and a race condition can develop.

		hhEmu->emu_clearattr_sav = hhEmu->emu_clearattr;

		NotifyClient(hhEmu->hSession, EVENT_EMU_CLRATTR, 0);
		break;


	/* start of screen to cursor */

	case 1:
		for (r = 0; r < hhEmu->emu_currow; ++r)
			{
			clear_imgrow(hhEmu, r);

			pstPRI->aiLineAttr[row_index(hhEmu, r)] = NO_LINE_ATTR;
			}

		ECHAR_Fill(hhEmu->emu_apText[row_index(hhEmu, hhEmu->emu_currow)],
					EMU_BLANK_CHAR,
			  		(size_t)(hhEmu->emu_curcol + 1));

		if (hhEmu->emu_curcol >= hhEmu->emu_aiEnd[hhEmu->emu_imgrow])
			hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = EMU_BLANK_LINE;

		pstAttr = hhEmu->emu_apAttr[row_index(hhEmu, hhEmu->emu_currow)];

		for (r = 0 ; r <= hhEmu->emu_curcol ; ++r)
			pstAttr[r] = hhEmu->emu_clearattr;

		(*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow, hhEmu->emu_curcol);

		updateLine(sessQueryUpdateHdl(hhEmu->hSession), 0, hhEmu->emu_currow);
		break;

	/* Entire screen */
	case 2:
		for (r = 0; r < MAX_EMUROWS; ++r)
			{
			backscrlAdd(sessQueryBackscrlHdl(hhEmu->hSession),
							hhEmu->emu_apText[row_index(hhEmu, r)],
							hhEmu->emu_maxcol+1);

			CaptureLine(sessQueryCaptureFileHdl(hhEmu->hSession),
							CF_CAP_SCREENS,
							hhEmu->emu_apText[row_index(hhEmu, r)],
							emuRowLen(hhEmu, row_index(hhEmu, r)));

			printEchoScreen(hhEmu->hPrintEcho,
							hhEmu->emu_apText[row_index(hhEmu, r)],
							emuRowLen(hhEmu, row_index(hhEmu, r)));

			CnvrtMBCStoECHAR(aechBuf, sizeof(aechBuf), "\r\n", 
							(unsigned long)StrCharGetByteCount("\r\n"));
			printEchoScreen(hhEmu->hPrintEcho,
							aechBuf,
							sizeof(ECHAR) * 2);

			clear_imgrow(hhEmu, r);

			pstPRI->aiLineAttr[r] = NO_LINE_ATTR;
			}

		updateScroll(sessQueryUpdateHdl(hhEmu->hSession),
						0,
						hhEmu->emu_maxrow,
						hhEmu->emu_maxrow + 1,
						hhEmu->emu_imgtop,
						TRUE);


		// Save the clear attribute at the time of
		// notification.  This is necessary since the message is posted
		// and a race condition can develop.

		hhEmu->emu_clearattr_sav = hhEmu->emu_clearattr;

		NotifyClient(hhEmu->hSession, EVENT_EMU_CLRATTR, 0);
		break;

	default:
		commanderror(hhEmu);
		}

	(*hhEmu->emu_setcurpos)(hhEmu, trow, tcol);
	}


/* end of vt100.c */

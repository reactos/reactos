/*	File: D:\WACKER7\emu\emu.c (Created: 08-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:20p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\tdll.h>
#include <tdll\assert.h>
#include <tdll\mc.h>
#include <tdll\com.h>
#include <tdll\cloop.h>
#include <tdll\capture.h>
#include <tdll\session.h>
#include <tdll\load_res.h>
#include <tdll\globals.h>
#include <tdll\print.h>
#include <tdll\statusbr.h>
#include <tdll\tchar.h>
#include <search.h>
#include <tdll\update.h>
#include <term\res.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"

static int FFstrlen(const BYTE *);
int _cdecl KeyCmp(const void *, const void *);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	char_pn
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void char_pn(const HHEMU hhEmu) 	 /* interpret a character as a numeric param */
	{
	if (hhEmu->emu_code < ETEXT(' '))
		hhEmu->emu_code = ETEXT(' ');

	hhEmu->selector[hhEmu->selector_cnt] =
	hhEmu->num_param[hhEmu->num_param_cnt] = hhEmu->emu_code - ETEXT(' ') + 1;

	hhEmu->num_param[++hhEmu->num_param_cnt] = 0;

	hhEmu->selector[++hhEmu->selector_cnt] = 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	commanderror
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void commanderror(const HHEMU hhEmu)
	{
	hhEmu->state = 0;
	ANSI_Pn_Clr(hhEmu);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuAutoDetectLoad
 *
 * DESCRIPTION:
 *	if auto dection is on, loads the given emulator ID and sets auto
 *	detection off.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle
 *	EmuID	- id of emulator to load
 *
 * RETURNS:
 *	void
 *
 */
void emuAutoDetectLoad(const HHEMU hhEmu, const int nEmuID)
	{
	if (hhEmu->stUserSettings.nEmuId != EMU_AUTO)
		return;

	if (hhEmu->stUserSettings.nEmuId != nEmuID)
		{
		emuLock((HEMU)hhEmu);
		hhEmu->stUserSettings.nAutoAttempts = 0;
#ifdef INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
        // Load the default telnet terminal id for this emulator. - cab:11/18/96
        //
        emuQueryDefaultTelnetId(nEmuID, hhEmu->stUserSettings.acTelnetId,
            EMU_MAX_TELNETID);
#endif
		emuUnlock((HEMU)hhEmu);

		emuLoad((HEMU)hhEmu, nEmuID);
		}
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuStdGraphic
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
void emuStdGraphic(const HHEMU hhEmu)
	{
	ECHAR ccode;
	ECHAR echBuf[10];
	int iCharsToMove;

	int 	iRow = hhEmu->emu_currow;
	int 	iCol = hhEmu->emu_curcol;

	ECHAR	*tp = hhEmu->emu_apText[hhEmu->emu_imgrow];
	PSTATTR ap = hhEmu->emu_apAttr[hhEmu->emu_imgrow];

	ccode = hhEmu->emu_code;

	// Is the emulator in insert mode?
	//
	if (hhEmu->mode_IRM == SET)
		{
		iCharsToMove = hhEmu->emu_aiEnd[hhEmu->emu_imgrow] - iCol;

		if (iCharsToMove + iCol >= hhEmu->emu_maxcol)
			iCharsToMove -= 1;

		if (iCharsToMove > 0)
			{
			memmove(&tp[iCol+1], &tp[iCol], (unsigned)iCharsToMove * sizeof(ECHAR));
			memmove(&ap[iCol+1], &ap[iCol], (unsigned)iCharsToMove * sizeof(ECHAR));
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
	if ((hhEmu->stUserSettings.nEmuId == EMU_VT100) &&
			ccode < sizeof(hhEmu->dspchar))
			ccode = hhEmu->dspchar[ccode];

	tp[iCol] = ccode;
	ap[iCol] = hhEmu->emu_charattr;

#if !defined(CHAR_NARROW)
	
	if ((hhEmu->stUserSettings.nEmuId == EMU_AUTO) ||
		(hhEmu->stUserSettings.nEmuId == EMU_ANSIW))
		{
		// Process Double Byte Characters
		//		
		if (QueryCLoopMBCSState(sessQueryCLoopHdl(hhEmu->hSession)))
			{
			if (isDBCSChar(ccode))
				{
				int iColPrev = iCol;

				ap[iCol].wilf = 1;
				ap[iCol].wirt = 0;

				// Update the end of row index if necessary.
				//
				if (iCol > hhEmu->emu_aiEnd[hhEmu->emu_imgrow])
					hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = iCol;

				// Update the image.
				//
				updateChar(sessQueryUpdateHdl(hhEmu->hSession),
							iRow,
							iCol,
							hhEmu->mode_IRM ?
							hhEmu->emu_maxcol :
							hhEmu->emu_aiEnd[hhEmu->emu_imgrow]);

				iCol = min(iCol+1, hhEmu->emu_maxcol);

				tp[iCol] = ccode;
				ap[iCol] = ap[iColPrev];
				ap[iCol].wilf = 0;
				ap[iCol].wirt = 1;
				}
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
			    iCharsToMove = hhEmu->emu_aiEnd[hhEmu->emu_imgrow] - iCol + 1;
			    if (iCol + 2 < hhEmu->emu_maxcol && iCharsToMove > 0)
				    {
				    memmove(&tp[iCol + 1],
                            &tp[iCol + 2],
                            (unsigned)iCharsToMove * sizeof(ECHAR));

				    memmove(&ap[iCol + 1],
                            &ap[iCol + 2],
                            (unsigned)iCharsToMove * sizeof(ECHAR));
				    }
			
			
				//move end of row since we removed a character
                hhEmu->emu_aiEnd[hhEmu->emu_imgrow] -= 1;

                //update the image
                updateChar(sessQueryUpdateHdl(hhEmu->hSession),
				        hhEmu->emu_imgrow,
				        hhEmu->emu_aiEnd[hhEmu->emu_imgrow] + 1,
						hhEmu->mode_IRM ?
						hhEmu->emu_maxcol :
						hhEmu->emu_aiEnd[hhEmu->emu_imgrow] + 1);

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
				iCol,
				hhEmu->mode_IRM ?
				hhEmu->emu_maxcol :
				hhEmu->emu_aiEnd[hhEmu->emu_imgrow]);

	// Move the position of the cursor ahead of the last character
	// displayed, checking for end of line wrap.
	//
	iCol++;
	if (iCol > hhEmu->emu_maxcol)
		{
		if (hhEmu->mode_AWM)
			{
			// This code was added, but not enabled because we did not
			// want to introduce this without proper testing.  If line
			// wrap on capture to printer not working is reported as a
			// bug, enable this code.
			#if 0
			printEchoChar(hhEmu->hPrintEcho, ETEXT('\r'));
			printEchoChar(hhEmu->hPrintEcho, ETEXT('\n'));
			#endif
			printEchoString(hhEmu->hPrintEcho, tp, emuRowLen(hhEmu, iRow));
			CnvrtMBCStoECHAR(echBuf, sizeof(echBuf), "\r\n",
							  (unsigned long)StrCharGetByteCount("\r\n"));

			printEchoString(hhEmu->hPrintEcho,
								echBuf,
								sizeof(ECHAR) * 2);

			CaptureLine(sessQueryCaptureFileHdl(hhEmu->hSession),
								CF_CAP_LINES,
								tp,
								emuRowLen(hhEmu, iRow));

			if (iRow == hhEmu->bottom_margin)
				(*hhEmu->emu_scroll)(hhEmu, 1, TRUE);
			else
				iRow += 1;

			iCol = 0;
			}
		else
			{
			iCol = hhEmu->emu_maxcol;
			}
		}

	// Finally, set the cursor position.  This wil reset emu_currow
	// and emu_curcol.
	//
	(*hhEmu->emu_setcurpos)(hhEmu, iRow, iCol);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emu_cleartabs
 *
 * DESCRIPTION:
 *	 Clears one or all tab stops.
 *
 * ARGUMENTS:
 *	 selector -- 0 clears tab at current cursor position
 *			  -- 3 clears all tabs in current line
 *
 * RETURNS:
 *	 nothing
 */
void emu_cleartabs(const HHEMU hhEmu, int selecter)
	{
	int col;

	switch (selecter)
		{
	case 0:
		hhEmu->tab_stop[hhEmu->emu_curcol] = FALSE;
		break;

	case 3:
		for (col = 0; col <= hhEmu->emu_maxcol; ++col)
			hhEmu->tab_stop[col] = FALSE;
		break;

	default:
		commanderror(hhEmu);
		break;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuSendKeyString
 *
 * DESCRIPTION:
 *	 Sends the specified string.
 *
 * ARGUMENTS:
 *	hhEmu		- The internal emulator handle.
 *	nIndex		- Position of key in keytable array.
 *	pstKeyTbl	- Address of key strings table.

 *
 * RETURNS:
 *	 nothing
 */
void emuSendKeyString(const HHEMU hhEmu,
						const nIndex,
						const PSTKEYTABLE pstKeyTbl)
	{
	ECHAR  str[80];
	PSTKEY pstKey;
	TCHAR *pszTemp;

	memset(str, 0, sizeof(str));

	assert(nIndex >= 0 && nIndex < pstKeyTbl->iMaxKeys);

	pstKey = pstKeyTbl->pstKeys + nIndex;

	pszTemp = pstKey->fPointer ? pstKey->u.pachKeyStr : pstKey->u.achKeyStr;
	CnvrtMBCStoECHAR(str, sizeof(str), pszTemp,
					  (unsigned long)StrCharGetByteCount(pszTemp));

	emuSendString(hhEmu, str, pstKey->uLen);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuSendString
 *
 * DESCRIPTION:
 *	 Sends the specified string.
 *
 * ARGUMENTS:
 *	 str -- address of string
 *	 strlen -- length of string
 *
 * RETURNS:
 *	 nothing
 */
void emuSendString(const HHEMU hhEmu, ECHAR *str, int nLen)
	{
	TCHAR *pchMBCS = NULL;
	TCHAR *pchTemp = NULL;
	unsigned long ulSize = 0;
	unsigned int i = 0;

	// This probably allocates way to many bytes, but if the incomming
	// string is all MBC's we are safe.
	pchMBCS = malloc((unsigned long)nLen);
	if (pchMBCS == NULL)
		{
		assert(FALSE);
		return;
		}

	ulSize = (unsigned long)CnvrtECHARtoMBCS(pchMBCS, (unsigned long)nLen,
											  str, (unsigned long)nLen);
	pchTemp = pchMBCS;

#if 0	//DEADWOOD:jkh, 12/12/1996
	// Some systems mistake something like ESC 0 D  if the codes
	// are sent in separate packets. This now sends such sequences
	// in a single socket write which should usually put them in
	// the same packet (though it doesn't guarantee to do so.

	// Loop through the # of bytes in the string
	for (i = 0 ; i < ulSize ; ++i)
		CLoopCharOut(sessQueryCLoopHdl(hhEmu->hSession), *pchTemp++);
#endif

	CLoopBufrOut(sessQueryCLoopHdl(hhEmu->hSession), pchTemp, ulSize);

	free(pchMBCS);
	pchMBCS = NULL;
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emu_reverse_image
 *
 * DESCRIPTION:
 *	 Reverses the foreground and background colors for the entire virtual
 *	 image.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void emu_reverse_image(const HHEMU hhEmu)
	{
	int 	nRow, nCol;
	STATTR	stOldAttr;
	PSTATTR pstAttr;

	// Set reverse screen mode for both clear and character attributes.
	//
	hhEmu->attrState[CSCLEAR_STATE].revvid =
		!hhEmu->attrState[CSCLEAR_STATE].revvid;

	hhEmu->emu_clearattr_sav =
		hhEmu->emu_clearattr = hhEmu->attrState[CSCLEAR_STATE];

	hhEmu->attrState[CS_STATE].revvid =
		!hhEmu->attrState[CS_STATE].revvid;

	hhEmu->emu_charattr = hhEmu->attrState[CS_STATE];

	for (nRow = 0; nRow < (hhEmu->emu_maxrow+1); nRow++)
		{
		pstAttr = hhEmu->emu_apAttr[nRow];

		for (nCol = 0 ; nCol <= hhEmu->emu_maxcol ; ++nCol, ++pstAttr)
			{
			stOldAttr = *pstAttr;
			pstAttr->txtclr = stOldAttr.bkclr;
			pstAttr->bkclr	= stOldAttr.txtclr;
			}
		}

	updateLine(sessQueryUpdateHdl(hhEmu->hSession), 0, hhEmu->emu_maxrow);
	NotifyClient(hhEmu->hSession, EVENT_EMU_CLRATTR, 0);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emu_is25lines
 *
 * DESCRIPTION:
 *	 Tells the calling function if the emulator is using the 25th line.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 TRUE if the emulator is using the 25th line
 */
int emu_is25lines(const HHEMU hhEmu)
	{
	return (hhEmu->mode_25enab ? TRUE : FALSE);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emu_kbdlocked
 *
 * DESCRIPTION:
 *	 Replacement kbdin that ignores all keys passed to it.
 *
 * ARGUMENTS:
 *	 key -- key to process
 *
 * RETURNS:
 *	 nothing
 */
/* ARGSUSED */
int emu_kbdlocked(const HHEMU hhEmu, int key, const int fTest)
	{
	return -1;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	nothing
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
/* ARGSUSED */
void nothing(const HHEMU hhEmu)
	{
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuKbdKeyLookup
 *
 * DESCRIPTION:
 *	Main keyboard translation routine for all emulators.  Note, this
 *	routine will not lookup keys unless the iUseTermKeys flag is set.
 *
 * ARGUMENTS:
 *	UINT	key - lower byte is char or virtual key, upper byte has flags
 *
 * RETURNS:
 *	Index if translated, else minus one.
 *
 */
int emuKbdKeyLookup(const HHEMU hhEmu,
					const int uKey,
					const PSTKEYTABLE pstKeyTbl)
	{
	PSTKEY pstKey;

	if (hhEmu->stUserSettings.nTermKeys == EMU_KEYS_ACCEL)
		return -1;

	pstKey = bsearch(&uKey,
					pstKeyTbl->pstKeys,
					(unsigned)pstKeyTbl->iMaxKeys,
					sizeof(KEY), KeyCmp);

	if (pstKey)
		return (int)(pstKey - pstKeyTbl->pstKeys);

	return -1;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuKeyTableLoad
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
/* ARGSUSED */
int emuKeyTableLoad(const HHEMU hhEmu,
					const KEYTBLSTORAGE pstKeySource[],
					const int nNumKeys,
					PSTKEYTABLE const pstKeyTbl)
	{
	int 	l;
	int 	nLoop = 0;
	PSTKEY	pstKeys;

	if (nNumKeys == 0)
		return FALSE;

	emuKeyTableFree(pstKeyTbl); // free previous instance

	if ((pstKeyTbl->pstKeys = malloc((unsigned)(nNumKeys * (int)sizeof(KEY))))
			== 0)
		{
		assert(0);
		return FALSE;
		}

	memset(pstKeyTbl->pstKeys, 0, (unsigned)(nNumKeys * (int)sizeof(KEY)));

	if (pstKeyTbl->pstKeys)
		{
		for (pstKeys = pstKeyTbl->pstKeys; nLoop < nNumKeys ; pstKeys++, nLoop++)
			{
			pstKeys->key = pstKeySource[nLoop].KeyCode;

			l = FFstrlen(pstKeySource[nLoop].achKeyStr);

			if ( l	<= (int)sizeof(LPTSTR) )
				{
				pstKeys->fPointer = FALSE;

				// Because of the goofy resource compiler, it was
				// necessary to define a "\xff" in the resource data,
				// when what we really wanted was a "\x00\xff".  So,
				// now we determine when this case occurs, and load the
				// \x00 value manually.  Note that there is an additional
				// test for this below when determining the length of the
				// the data.
				//
				if (l != 0)
					{
					MemCopy(pstKeys->u.achKeyStr, pstKeySource[nLoop].achKeyStr, (unsigned)l);
					}

				else
					{
					pstKeys->u.achKeyStr[0] = '\x00';
					}
				}
			else
				{
				pstKeys->fPointer = TRUE;
				pstKeys->u.pachKeyStr = malloc((unsigned)(l+1));

				if (!pstKeys->u.pachKeyStr)
					{
					emuKeyTableFree(pstKeyTbl);
					break;
					}

				MemCopy(pstKeys->u.pachKeyStr, pstKeySource[nLoop].achKeyStr, (unsigned)l);
				}

			// Here's the special case test, again.
			//
			if (l !=0 )
				pstKeys->uLen = (int)l;
			else
				pstKeys->uLen = 1;

			pstKeyTbl->iMaxKeys += 1;
			}
		}

	if (pstKeyTbl->iMaxKeys)
		{
		qsort(pstKeyTbl->pstKeys,
					(unsigned)pstKeyTbl->iMaxKeys,
					sizeof(KEY),
					KeyCmp);
		}

	return (int)pstKeyTbl->iMaxKeys;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuKeyTableFree
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void emuKeyTableFree(PSTKEYTABLE const pstKeyTbl)
	{
	int i;

	for (i = 0 ; i < pstKeyTbl->iMaxKeys ; i++)
		{
		if (pstKeyTbl->pstKeys[i].fPointer)
			{
			free(pstKeyTbl->pstKeys[i].u.pachKeyStr);
			pstKeyTbl->pstKeys[i].u.pachKeyStr = NULL;
			}
		}

	pstKeyTbl->iMaxKeys = 0;

	if (pstKeyTbl->pstKeys)
		{
		free(pstKeyTbl->pstKeys);
		pstKeyTbl->pstKeys = (KEY *)0;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	FFstrlen
 *
 * DESCRIPTION:
 *	Local version of strlen that uses '\ff' as a string terminator
 *
 * ARGUMENTS:
 *	CHAR FAR *s - '\ff' terminated string.
 *
 * RETURNS:
 *	length
 *
 */
static int FFstrlen(const BYTE *s)
	{
	int i = 0;

	while (*s++ != 0xFF)
		i += 1;

	return i;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * KeyCmp
 *
 * DESCRIPTION: Compare function for qsort.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int _cdecl KeyCmp(PSTKEY pstKey1, PSTKEY pstKey2)
	{
	if (pstKey1->key > pstKey2->key)
		return 1;

	if (pstKey1->key < pstKey2->key)
		return -1;

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * emuInstallStateTable
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void emuInstallStateTable(const HHEMU hhEmu, struct trans_entry const *e, int size)
	{
	struct state_entry *state_pntr = 0;
	int nStateCnt = 0;
	int nTransCnt = 0;

	while (size--)
		{
		if (e->next_state == NEW_STATE) 	/* start a new state */
			{
			assert(nStateCnt < MAX_STATE);
			hhEmu->state_tbl[nStateCnt].first_trans = &hhEmu->trans_tbl[nTransCnt];
			state_pntr = &hhEmu->state_tbl[nStateCnt++];
			state_pntr->number_trans = 0;
			}
		else							/* add a transition */
			{
			assert(nTransCnt < MAX_TRANSITION);
			assert(state_pntr);
			++state_pntr->number_trans;
			hhEmu->trans_tbl[nTransCnt++] = *e;
			}
		++e;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuCreateTextAttrBufs
 *
 * DESCRIPTION:
 *	This one took a while to decipher but here is the bottom line.
 *	emu_maxrow and emu_maxcol refer to the last row and column from
 *	offset 0 (ZERO)!  The emulator image has 2 (two) more columns for the
 *	the stuff unknown to me at the present time.  This function wants
 *	the total number of rows and columns, so emu_maxrow = 23 means the
 *	the argument nRows is 24.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int emuCreateTextAttrBufs(const HEMU hEmu, const size_t nRows, size_t nCols)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;
	register size_t i, ndx;
	PSTATTR pstAttr;

	if (hhEmu->emu_apText && hhEmu->emu_apAttr && hhEmu->emu_aiEnd)
		return (TRUE);
	else
		emuDestroyTextAttrBufs(hEmu);

	nCols += 2; // Emulators need two extra columns.

	// Allocate the text buffer.
	//
	if ((hhEmu->emu_apText = (ECHAR **)calloc(nRows, sizeof(ECHAR *))) == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	for (i = 0 ; i < nRows ; ++i)
		{
		if ((hhEmu->emu_apText[i] = (ECHAR *)calloc(nCols, sizeof(ECHAR))) == 0)
			{
			assert(FALSE);
			emuDestroyTextAttrBufs(hEmu);
			return FALSE;
			}

		ECHAR_Fill(hhEmu->emu_apText[i], EMU_BLANK_CHAR, nCols);
		}

	// Allocate the array to hold the rightmost character column number
	// for each row.
	//
	if ((hhEmu->emu_aiEnd = (int *)calloc(nRows, sizeof(int))) == 0)
		{
		assert(FALSE);
		emuDestroyTextAttrBufs(hEmu);
		return FALSE;
		}

	for (ndx = 0;  ndx < nRows; ++ndx)
			hhEmu->emu_aiEnd[ndx] = EMU_BLANK_LINE;

	// Allocate the attribute buffer.
	//
	if ((hhEmu->emu_apAttr = (PSTATTR *)calloc(nRows, sizeof(LPTSTR))) == 0)
		{
		assert(FALSE);
		emuDestroyTextAttrBufs(hEmu);
		return FALSE;
		}

	for (i = 0 ; i < nRows ; ++i)
		{
		if ((hhEmu->emu_apAttr[i] = calloc(nCols, sizeof(STATTR))) == 0)
			{
			assert(FALSE);
			emuDestroyTextAttrBufs(hEmu);
			return FALSE;
			}

		for (ndx = 0, pstAttr = hhEmu->emu_apAttr[i] ; ndx < nCols ; ++ndx)
			pstAttr[ndx] = hhEmu->emu_clearattr;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuDestroyTextAttrBufs
 *
 * DESCRIPTION:
 *	Destroys any allocated buffers for text and attributes.
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	void
 *
 */
void emuDestroyTextAttrBufs(const HEMU hEmu)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;
	register int i;

	if (hhEmu->emu_apText)
		{
		for (i = 0 ; i <= hhEmu->emu_maxrow ; ++i)
			{
			if (hhEmu->emu_apText[i])
				{
				free(hhEmu->emu_apText[i]);
				hhEmu->emu_apText[i] = NULL;
				}
			}

		free(hhEmu->emu_apText);
		hhEmu->emu_apText = 0;
		}

	if (hhEmu->emu_aiEnd)
		{
		free(hhEmu->emu_aiEnd);
		hhEmu->emu_aiEnd = 0;
		}

	if (hhEmu->emu_apAttr)
		{
		for (i = 0 ; i <= hhEmu->emu_maxrow ; ++i)
			{
			if (hhEmu->emu_apAttr[i])
				{
				free(hhEmu->emu_apAttr[i]);
				hhEmu->emu_apAttr[i] = NULL;
				}
			}

		free(hhEmu->emu_apAttr);
		hhEmu->emu_apAttr = 0;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuCreateNameTable
 *
 * DESCRIPTION:
 *	Loads the Emulator Names into a table
 *
 * ARGUMENTS:
 *	HHEMU hhEmu	-	Emulator Handle
 *
 * RETURNS:
 *	Success/Failure
 *
 */
int emuCreateNameTable(const HHEMU hhEmu)
	{
	int 	iLen, idx, iRet;
	TCHAR	achText[256];

	iRet = TRUE;

	emuLock((HEMU)hhEmu);

	if (hhEmu->pstNameTable)
		{
		free(hhEmu->pstNameTable);
		hhEmu->pstNameTable = NULL;
		}

	if ((hhEmu->pstNameTable = malloc(sizeof(STEMUNAMETABLE) * NBR_EMULATORS)) == 0)
		{
		assert(FALSE);
		iRet = FALSE;
		goto LoadExit;
		}

	// Load the emulator name table.  It simply contains the name and id of
	// all of the supported emulators.
	//

	// EMU_AUTO
	//
	iLen = LoadString(glblQueryDllHinst(), IDS_EMUNAME_AUTO, achText, sizeof(achText) / sizeof(TCHAR));

	if (iLen >= EMU_MAX_NAMELEN)
		{
		assert(FALSE);
		iRet = FALSE;
		goto LoadExit;
		}

	idx = 0;
	StrCharCopy(hhEmu->pstNameTable[idx].acName, achText);
	hhEmu->pstNameTable[idx].nEmuId = EMU_AUTO;

	// EMU_ANSI
	//
	iLen = LoadString(glblQueryDllHinst(), IDS_EMUNAME_ANSI, achText, sizeof(achText) / sizeof(TCHAR));

	if (iLen >= EMU_MAX_NAMELEN)
		{
		assert(FALSE);
		iRet = FALSE;
		goto LoadExit;
		}

	idx++;
	StrCharCopy(hhEmu->pstNameTable[idx].acName, achText);
	hhEmu->pstNameTable[idx].nEmuId = EMU_ANSI;

	// EMU_ANSIW
	//
	iLen = LoadString(glblQueryDllHinst(), IDS_EMUNAME_ANSIW, achText, sizeof(achText) / sizeof(TCHAR));

	if (iLen >= EMU_MAX_NAMELEN)
		{
		assert(FALSE);
		iRet = FALSE;
		goto LoadExit;
		}

	idx++;
	StrCharCopy(hhEmu->pstNameTable[idx].acName, achText);
	hhEmu->pstNameTable[idx].nEmuId = EMU_ANSIW;

	// EMU_MIMI
	//
	iLen = LoadString(glblQueryDllHinst(), IDS_EMUNAME_MINI, achText, sizeof(achText) / sizeof(TCHAR));

	if (iLen >= EMU_MAX_NAMELEN)
		{
		assert(FALSE);
		iRet = FALSE;
		goto LoadExit;
		}

	idx++;
	StrCharCopy(hhEmu->pstNameTable[idx].acName, achText);
	hhEmu->pstNameTable[idx].nEmuId = EMU_MINI;

	// EMU_VIEW
	//
	iLen = LoadString(glblQueryDllHinst(), IDS_EMUNAME_VIEW, achText, sizeof(achText) / sizeof(TCHAR));

	if (iLen >= EMU_MAX_NAMELEN)
		{
		assert(FALSE);
		iRet = FALSE;
		goto LoadExit;
		}

	idx++;
	StrCharCopy(hhEmu->pstNameTable[idx].acName, achText);
	hhEmu->pstNameTable[idx].nEmuId = EMU_VIEW;


	// EMU_TTY
	//
	iLen = LoadString(glblQueryDllHinst(), IDS_EMUNAME_TTY, achText, sizeof(achText) / sizeof(TCHAR));

	if (iLen >= EMU_MAX_NAMELEN)
		{
		assert(FALSE);
		iRet = FALSE;
		goto LoadExit;
		}

	idx++;
	StrCharCopy(hhEmu->pstNameTable[idx].acName, achText);
	hhEmu->pstNameTable[idx].nEmuId = EMU_TTY;

	// EMU_VT100
	//
	iLen = LoadString(glblQueryDllHinst(), IDS_EMUNAME_VT100, achText, sizeof(achText) / sizeof(TCHAR));

	if (iLen >= EMU_MAX_NAMELEN)
		{
		assert(FALSE);
		iRet = FALSE;
		goto LoadExit;
		}

	idx++;
	StrCharCopy(hhEmu->pstNameTable[idx].acName, achText);
	hhEmu->pstNameTable[idx].nEmuId = EMU_VT100;

	// EMU_VT52
	//
	iLen = LoadString(glblQueryDllHinst(), IDS_EMUNAME_VT52, achText, sizeof(achText) / sizeof(TCHAR));

	if (iLen >= EMU_MAX_NAMELEN)
		{
		assert(FALSE);
		iRet = FALSE;
		goto LoadExit;
		}

	idx++;
	StrCharCopy(hhEmu->pstNameTable[idx].acName, achText);
	hhEmu->pstNameTable[idx].nEmuId = EMU_VT52;

	// EMU_VT100J
	//
	iLen = LoadString(glblQueryDllHinst(), IDS_EMUNAME_VT100J, achText, sizeof(achText) / sizeof(TCHAR));

	if (iLen >= EMU_MAX_NAMELEN)
		{
		assert(FALSE);
		iRet = FALSE;
		goto LoadExit;
		}

	idx++;
	StrCharCopy(hhEmu->pstNameTable[idx].acName, achText);
	hhEmu->pstNameTable[idx].nEmuId = EMU_VT100J;

#if defined(INCL_VT220)
	// EMU_VT220
	//
	iLen = LoadString(glblQueryDllHinst(), IDS_EMUNAME_VT220, achText, sizeof(achText) / sizeof(TCHAR));

	if (iLen >= EMU_MAX_NAMELEN)
		{
		assert(FALSE);
		iRet = FALSE;
		goto LoadExit;
		}

	idx++;
	StrCharCopy(hhEmu->pstNameTable[idx].acName, achText);
	hhEmu->pstNameTable[idx].nEmuId = EMU_VT220;
#endif

#if defined(INCL_VT320)
	// EMU_VT320
	//
	iLen = LoadString(glblQueryDllHinst(), IDS_EMUNAME_VT320, achText, sizeof(achText) / sizeof(TCHAR));

	if (iLen >= EMU_MAX_NAMELEN)
		{
		assert(FALSE);
		iRet = FALSE;
		goto LoadExit;
		}

	idx++;
	StrCharCopy(hhEmu->pstNameTable[idx].acName, achText);
	hhEmu->pstNameTable[idx].nEmuId = EMU_VT320;
#endif

	LoadExit:

	emuUnlock((HEMU)hhEmu);
	return(iRet);

	}


/* end of emu.c */

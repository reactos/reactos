/*	File: D:\WACKER\emu\vt52.c (Created: 28-Dec-1993)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:29p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\chars.h>
#include <tdll\print.h>
#include <tdll\mc.h>
#include <tdll\tchar.h>
#include <tdll\assert.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"
#include "emudec.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt52PrintCommands
 *
 * DESCRIPTION:
 *	 Processes vt52 printing commands.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt52PrintCommands(const HHEMU hhEmu)
	{
	int line;
	ECHAR code;
	ECHAR aechBuf[10];

	code = hhEmu->emu_code;
	switch (code)
		{
	case ETEXT('^'): 			  /* auto print on */
		hhEmu->print_echo = TRUE;
		break;

	case ETEXT('_'): 			  /* auto print off */
		hhEmu->print_echo = FALSE;
		break;

	case ETEXT(']'): 			  /* print screen */
		for (line = 0; line < 24; ++line)
			printEchoLine(hhEmu->hPrintHost,
							hhEmu->emu_apText[row_index(hhEmu, line)],
							emuRowLen(hhEmu, row_index(hhEmu, line)));

		CnvrtMBCStoECHAR(aechBuf, sizeof(aechBuf), "0x0C", (unsigned long)StrCharGetByteCount("0x0C"));
		printEchoLine(hhEmu->hPrintHost, aechBuf, sizeof(ECHAR));
		break;

	case ETEXT('V'): 			  /* print cursor line */
		printEchoLine(hhEmu->hPrintHost,
				hhEmu->emu_apText[row_index(hhEmu, hhEmu->emu_currow)],
				emuRowLen(hhEmu, row_index(hhEmu, hhEmu->emu_currow)));
		break;

	case ETEXT('W'): 			  /* enter printer controller mode */
		hhEmu->state = 4;		   /* start vt52_prnc() */
		break;

	case ETEXT('X'): 			  /* exit printer controller mode */
		break;
	default:
		break;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt52Print
 *
 * DESCRIPTION:
 *	 Stores character and prints saved string of characters when termination
 *	 character is received.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt52Print(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	ECHAR ccode;

	ccode = hhEmu->emu_code;
	*pstPRI->pntr++ = ccode;
	*pstPRI->pntr = 0;
	++pstPRI->len_s;

	if (pstPRI->len_s >= pstPRI->len_t &&
			memcmp(pstPRI->terminate,
					&pstPRI->storage[pstPRI->len_s - pstPRI->len_t], 
					sizeof(pstPRI->terminate)) == 0)
		{
		/* received termination string, wrap it up */
		emuPrintChars(hhEmu, pstPRI->storage, pstPRI->len_s - pstPRI->len_t);
		pstPRI->pntr = pstPRI->storage;
		pstPRI->len_s = 0;
		hhEmu->state = 0;  /* drop out of this routine */

		// Finish-up print job.
		//
		printEchoClose(hhEmu->hPrintHost);
		return;
		}

	/* haven't received termination sequence yet, is storage filled? */
	if ((unsigned)pstPRI->len_s >= (sizeof(pstPRI->storage) - 1))
		{
		/* copy most of string to print buffer */
		emuPrintChars(hhEmu, pstPRI->storage, pstPRI->len_s - pstPRI->len_t);

		/* move end of string to beginning of storage */
		memmove(pstPRI->storage,
					&pstPRI->storage[pstPRI->len_s - pstPRI->len_t],
					(unsigned)pstPRI->len_t);

		pstPRI->pntr = pstPRI->storage + pstPRI->len_t;
		pstPRI->len_s = pstPRI->len_t;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt52_id
 *
 * DESCRIPTION:
 *	 Transmits the VT52 id code--ESC/Z.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt52_id(const HHEMU hhEmu)
	{
	TCHAR *sp;
	ECHAR ech[10];

	sp = TEXT("\033/Z");
	CnvrtMBCStoECHAR(ech, sizeof(ech), "\033/Z", (unsigned long)StrCharGetByteCount("\033/Z"));
	emuSendString(hhEmu, ech, (int)StrCharGetStrLength(sp));
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt52_CUP
 *
 * DESCRIPTION:
 *	 Positions the cursor for the VT52 emulator.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void vt52_CUP(const HHEMU hhEmu)
	{
	char_pn(hhEmu);
	(*hhEmu->emu_setcurpos)(hhEmu,
							hhEmu->num_param[0] - 1,
							hhEmu->num_param[1] - 1);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt52_kbdin
 *
 * DESCRIPTION:
 *	 Processes local keyboard keys for the VT52 emulator.
 *
 * ARGUMENTS:
 *	 key -- key to process
 *	 fTest -- if TRUE, just checks if it is an emulator key.
 *
 * RETURNS:
 *	 nothing
 */
int vt52_kbdin(const HHEMU hhEmu, int key, const int fTest)
	{
	int index;
	/* -------------- Check Backspace & Delete keys ------------- */

	if (hhEmu->stUserSettings.fReverseDelBk && ((key == VK_BACKSPACE) ||
			(key == DELETE_KEY) || (key == DELETE_KEY_EXT)))
		{
		key = (key == VK_BACKSPACE) ? DELETE_KEY : VK_BACKSPACE;
		}

	/* -------------- Mapped PF1-PF4 keys ------------- */

	if (hhEmu->stUserSettings.fMapPFkeys &&
			(index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl3)) != -1)
		{
		if (!fTest)
			emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl3);
		}

	/* -------------- Keypad Application mode ------------- */

	else if (hhEmu->mode_DECKPAM &&
			(index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl2)) != -1)
		{
		if (!fTest)
			emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl2);
		}

	/* -------------- Normal mode keys ------------- */

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

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuVT52Unload
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
void emuVT52Unload(const HHEMU hhEmu)
	{
	assert(hhEmu);

	if (hhEmu->pvPrivate)
		{
		free(hhEmu->pvPrivate);
		hhEmu->pvPrivate = 0;
		}

	return;
	}


/* end of vt52.c */

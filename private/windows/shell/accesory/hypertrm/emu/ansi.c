/*	File: D:\WACKER\emu\ansi.c (Created: 08-Dec-1993)
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
#include <tdll\chars.h>
#include <tdll\session.h>
#include <tdll\mc.h>
#include <tdll\assert.h>

#include "emu.h"
#include "emu.hh"
#include "ansi.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ansi_setmode
 *
 * DESCRIPTION:
 *	 Sets a mode for ANSI emulator
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ansi_setmode(const HHEMU hhEmu)
	{
	int mode_id, i;

	for (i = 0; i <= hhEmu->selector_cnt; i++)
		{
		mode_id = hhEmu->selector[i];
		switch (mode_id)
			{
		case 0x02:
			break;
		case 0x04:
			break;
		case 0x07:
			break;
		case 0x10:
			break;
		case 0x12:
			break;
		case 0x18:
			break;
		case 0x19:
			break;
		case 0x20:
			break;
		case 0xF1:
			break;
		case 0xF2:
			break;
		case 0xF3:
			break;
		case 0xF4:
			break;
		case 0xF5:
			break;
		case 0xF6:
		case 0xD7:
		case 0xF7:
			hhEmu->mode_AWM = SET;
			break;
		case 0xF8:
			/* select auto repeat mode */
			break;
		case 0xF18:
			break;
		case 0xF19:
			break;
		default:
			commanderror(hhEmu);
			break;
			}
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ansi_resetmode
 *
 * DESCRIPTION:
 *	 Resets a mode for the ANSI emulator.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ansi_resetmode(const HHEMU hhEmu)
	{
	int mode_id, i;

	for (i = 0; i <= hhEmu->selector_cnt; i++)
		{
		mode_id = hhEmu->selector[i];
		switch (mode_id)
			{
		case 0x02:
			break;
		case 0x04:
			break;
		case 0x07:
			break;
		case 0x10:
			break;
		case 0x12:
			break;
		case 0x18:
			break;
		case 0x19:
			break;
		case 0x20:
			break;
		case 0xF1:
			break;
		case 0xF2:
			break;
		case 0xF3:
			break;
		case 0xF4:
			break;
		case 0xF5:
			break;
		case 0xF6:
		case 0xD7:
		case 0xF7:
			hhEmu->mode_AWM = RESET; break;
		case 0xF8:
			/* select auto repeat mode */
			break;
		case 0xF18:
			break;
		case 0xF19:
			break;
		default:
			commanderror(hhEmu); break;
			}
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ansi_savecursor
 *
 * DESCRIPTION:
 *	 Saves the current cursor postion
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void ansi_savecursor(const HHEMU hhEmu)
	{
	const PSTANSIPRIVATE pstPRI = (PSTANSIPRIVATE)hhEmu->pvPrivate;

	// Save or restor the cursor position.
	//
	if (hhEmu->emu_code == ETEXT('s'))
		(*hhEmu->emu_getcurpos)
			(hhEmu, &pstPRI->iSavedRow, &pstPRI->iSavedColumn);
	else		
		(*hhEmu->emu_setcurpos)
			(hhEmu, pstPRI->iSavedRow, pstPRI->iSavedColumn);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * ansi_kbdin
 *
 * DESCRIPTION:
 *	 Processes local keyboard keys for the ANSI emulator.
 *
 * ARGUMENTS:
 *	 key -- key to process
 *
 * RETURNS:
 *	 nothing
 */
int ansi_kbdin(const HHEMU hhEmu, int key, const int fTest)
	{
	int index;

	/* -------------- Check Backspace & Delete keys ------------- */

	if (hhEmu->stUserSettings.fReverseDelBk && ((key == VK_BACKSPACE) ||
			(key == DELETE_KEY) || (key == DELETE_KEY_EXT)))
		{
		key = (key == VK_BACKSPACE) ? DELETE_KEY : VK_BACKSPACE;
		}

	if (hhEmu->stUserSettings.nTermKeys == EMU_KEYS_SCAN)
		{
		if ((index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl2)) != -1)
			{
			if (!fTest)
				emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl2);
			}

		else
			{
			index = std_kbdin(hhEmu, key, fTest);
			}
		}

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

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DoorwayMode
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void DoorwayMode(const HHEMU hhEmu)
	{
	static INT iOldUseTermKeys;
	ECHAR ccode = hhEmu->emu_code;

	if (hhEmu->num_param[hhEmu->num_param_cnt] != 255)
		return;

	if (ccode == ETEXT('h'))
		{
		iOldUseTermKeys = hhEmu->stUserSettings.nTermKeys;
		hhEmu->stUserSettings.nTermKeys = EMU_KEYS_SCAN;
		}

	else if (ccode == ETEXT('l'))
		{
		hhEmu->stUserSettings.nTermKeys = iOldUseTermKeys;
		}

	NotifyClient(hhEmu->hSession, EVENT_EMU_SETTINGS, 0);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuAnsiUnload
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void emuAnsiUnload(const HHEMU hhEmu)
	{
	assert(hhEmu);

	if (hhEmu->pvPrivate)
		{
		free(hhEmu->pvPrivate);
		hhEmu->pvPrivate = 0;
		}

	return;
	}

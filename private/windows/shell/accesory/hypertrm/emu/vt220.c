/*  File: \wacker\emu\vt220.c (Created: 28-Jan-1998)
 *
 *  Copyright 1998 by Hilgraeve Inc. -- Monroe, MI
 *  All rights reserved
 *
 *  $Revision: 4 $
 *  $Date: 8/03/99 2:10p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\tdll.h>
#include <tdll\tchar.h>
#include <tdll\session.h>
#include <tdll\cloop.h>
#include <tdll\assert.h>
#include <tdll\print.h>
#include <tdll\update.h>
#include <tdll\capture.h>
#include <tdll\backscrl.h>
#include <tdll\chars.h>
#include <tdll\mc.h>

#include "emu.h"
#include "emu.hh"
#include "emudec.hh"

#if defined(INCL_VT220)

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt220_hostreset
 *
 * DESCRIPTION:
 *   Calls vt220_reset() when told by the host to reset.
 *
 * ARGUMENTS:
 *   none
 *
 * RETURNS:
 *   nothing
 */
void vt220_hostreset(const HHEMU hhEmu)
	{
	vt220_reset(hhEmu, TRUE);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt220_reset
 *
 * DESCRIPTION:
 *   Calls vt220_reset() when told by the host to reset.
 *
 * ARGUMENTS:
 *   host_request -- TRUE if told by host to reset
 *
 * RETURNS:
 *   nothing
 */
int vt220_reset(const HHEMU hhEmu, const int host_request)
	{
	hhEmu->mode_KAM = hhEmu->mode_IRM = hhEmu->mode_VEM =
		hhEmu->mode_HEM = hhEmu->mode_LNM = hhEmu->mode_DECCKM =
		hhEmu->mode_DECOM = hhEmu->mode_DECCOLM = hhEmu->mode_DECPFF =
//		hhEmu->mode_DECSCNM = hhEmu->mode_25enab = hhEmu->mode_blank =
		hhEmu->mode_DECSCNM = hhEmu->mode_25enab =
		hhEmu->mode_block = hhEmu->mode_local = RESET;

	hhEmu->mode_SRM = hhEmu->mode_DECPEX = hhEmu->mode_DECTCEM = SET;

	hhEmu->mode_AWM = TRUE;

//	hhEmu->mode_protect = hhEmu->vt220_protectmode = FALSE;
	hhEmu->mode_protect = FALSE;

	if (host_request)
		{
		ANSI_RIS(hhEmu);
		hhEmu->mode_AWM = RESET;
		}

	hhEmu->fUse8BitCodes = FALSE;
	hhEmu->mode_vt220 = TRUE;
	hhEmu->mode_vt320 = FALSE;

	if (hhEmu->nEmuLoaded == EMU_VT320)
		{
		hhEmu->mode_vt320 = TRUE;
		}

	vt_charset_init(hhEmu);

	hhEmu->emu_code = '>';

	vt_alt_kpmode(hhEmu);

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt220_softreset
 *
 * DESCRIPTION:
 *   Does a soft reset.
 *
 * ARGUMENTS:
 *   none
 *
 * RETURNS:
 *   nothing
 */
void vt220_softreset(const HHEMU hhEmu)
	{
	hhEmu->mode_KAM = hhEmu->mode_IRM = hhEmu->mode_DECCKM =
		hhEmu->mode_DECOM = hhEmu->mode_DECKPAM = RESET;

	hhEmu->mode_AWM = RESET;

	DEC_STBM(hhEmu, 0,0);

	ANSI_Pn_Clr(hhEmu);

	ANSI_SGR(hhEmu);

	hhEmu->emu_code = 0;

	vt100_savecursor(hhEmu);

	vt_charset_init(hhEmu);

	hhEmu->emu_code = '>';

	vt_alt_kpmode(hhEmu);

	hhEmu->mode_protect = FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt220mode_reset
 *
 * DESCRIPTION:
 *   Sets the VT220 emulator to the proper conditions when switching
 *  from vt100 mode.
 *
 * ARGUMENTS:
 *   none
 *
 * RETURNS:
 *   nothing
 */
void vt220mode_reset(const HHEMU hhEmu)
	{
	hhEmu->mode_KAM		= hhEmu->mode_IRM		= hhEmu->mode_VEM =
		hhEmu->mode_HEM		= hhEmu->mode_DECCKM	= hhEmu->mode_DECOM =
		hhEmu->mode_25enab  = hhEmu->mode_AWM		= RESET;

	hhEmu->mode_DECPEX = hhEmu-> mode_DECTCEM = SET;

	hhEmu->fUse8BitCodes = FALSE;

	hhEmu->mode_vt220 = TRUE;
	hhEmu->mode_vt320 = FALSE;

	vt_charset_init(hhEmu);

	hhEmu->emu_code = '>';

	vt_alt_kpmode(hhEmu);

	DEC_STBM(hhEmu, 0, hhEmu->emu_maxrow + 1);

	hhEmu->emu_code = 0;

	vt100_savecursor(hhEmu);

	hhEmu->mode_protect = FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt220_DA
 *
 * DESCRIPTION:
 *   Sends the primary device attribute (DA) information to the host.
 *
 * ARGUMENTS:
 *   none
 *
 * RETURNS:
 *   nothing
 */
void vt220_DA(const HHEMU hhEmu)
	{
	int		fOldValue;
	char	achStr[50];
	ECHAR	ech[50];

	//if (emuProjSuppressEmuReporting(hhEmu))
	//		return;

	// Build the 7-bit or 8-bit response.
	//
	if (hhEmu->fUse8BitCodes)
		{
		achStr[0] = '\x9B';
		achStr[1] = '\x00';
		}
	else
		{
		achStr[0] = '\x1B';
		achStr[1] = '[';
		achStr[2] = '\x00';
		}

	// Add the VT220 or VT320 part of the response.
	//
	if (hhEmu->mode_vt320)
		StrCharCat(achStr, "?63");
	else
		StrCharCat(achStr, "?62");

	// Add the rest of the respnse and send the result.
	//
	StrCharCat(achStr, ";1;2;6;8;9;14c");

	CnvrtMBCStoECHAR(ech, sizeof(ech), achStr, 
					(unsigned long)StrCharGetByteCount(achStr));

	fOldValue = CLoopGetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession));

	CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), FALSE);

	emuSendString(hhEmu, ech, (int)StrCharGetEcharByteCount(ech)); 

	CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), fOldValue);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt220_2ndDA
 *
 * DESCRIPTION:
 *   Sends the secondary device attribute (DA) information to the host.
 *
 * ARGUMENTS:
 *   none
 *
 * RETURNS:
 *   nothing
 */
void vt220_2ndDA(const HHEMU hhEmu)
	{
	int		fOldValue;
	char	achStr[50];
	ECHAR	ech[50];

	// Build the 7-bit or 8-bit response.
	//
	if (hhEmu->fUse8BitCodes)
		{
		achStr[0] = '\x9B';
		achStr[1] = '\x00';
		}
	else
		{
		achStr[0] = '\x1B';
		achStr[1] = '[';
		achStr[2] = '\x00';
		}

	// Add the VT220 or VT320 part of the response.
	//
	if (hhEmu->mode_vt320)
		StrCharCat(achStr, ">24;14;0c");
	else
		StrCharCat(achStr, ">1;23;0c");

	CnvrtMBCStoECHAR(ech, sizeof(ech), achStr, 
					(unsigned long)StrCharGetByteCount(achStr));

	fOldValue = CLoopGetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession));

	CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), FALSE);

	emuSendString(hhEmu, ech, (int)StrCharGetEcharByteCount(ech)); 

	CLoopSetLocalEcho(sessQueryCLoopHdl(hhEmu->hSession), fOldValue);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  emuDecClearUDK
 *
 * DESCRIPTION:
 *  This function clears (frees) all of the user defined key sequences
 *  that may have been previously stored.
 *
 * ARGUMENTS:
 *  HHEMU   hhEmu   -   The internal emulator handle.
 *
 * NOTES:
 *  This function is called in response to the following escape sequence.
 *  Esc Pc;Pl |
 *
 * RETURNS:
 *  void
 *
 * AUTHOR: John Masters, 05-Sep-1995
 */
void emuDecClearUDK(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;
	PSTEMUKEYDATA pstKey = pstPRI->pstUDK;
	int idx;

	// Set a flag that identifies the locked or unlocked status of the
	// UDK's that will be set after the definition of the keys has
	// completed.
	//
	if (hhEmu->selector[1] == 1)
		pstPRI->fUnlockedUDK = 1;   // Keys are unlocked after definition.
	else
		pstPRI->fUnlockedUDK = 0;   // Keys are locked after definition.

	// This function is called in response to an escape sequence that tells
	// the emulator to either clear each key sequence when a new one is
	// defined, or to clear all of the key sequences before any are defined.
	//
	// emuDecStoreUDK always clears the current sequence before assigning
	// a new one.  So, this function will clear all of the keys only
	// if that's what we were asked to do.  If the first selector is Zero,
	// then we will go ahead and clear all of the User Defined Keys.
	//
	if (hhEmu->selector[0] != 0)
		return;

	// Cycle through the user defined key table and free
	// any memory that may have been allocated for sequences.
	//
	if (pstKey)
		{
		for (idx = 0; idx < MAX_UDK_KEYS; idx++, pstKey++)
			{
			if (pstKey->iSequenceLen != 0)
				{
				free(pstKey->pSequence);
				pstKey->iSequenceLen = 0;
				}
			}
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecDefineUDK
 *
 * DESCRIPTION:
 *   Redefines the string output by a key.
 *
 * ARGUMENTS:
 *   none
 *
 * RETURNS:
 *   nothing
 */
void emuDecDefineUDK(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	TCHAR acAscii[16] = {   TEXT('0'), TEXT('1'), TEXT('2'), TEXT('3'),
							TEXT('4'), TEXT('5'), TEXT('6'), TEXT('7'),
							TEXT('8'), TEXT('9'), TEXT('A'), TEXT('B'),
							TEXT('C'), TEXT('D'), TEXT('E'), TEXT('F') };

	unsigned int aiHex[16] = {  0x00, 0x01, 0x02, 0x03,
								0x04, 0x05, 0x06, 0x07,
								0x08, 0x09, 0x0A, 0x0B,
								0x0C, 0x0D, 0x0E, 0x0F };

	int		idx;
	TCHAR   emuCode;

	emuCode = hhEmu->emu_code;

	switch(pstPRI->iUDKState)
		{
	case(KEY_NUMBER_NEXT):
		TCHAR_Fill(pstPRI->acUDKSequence,
			0, sizeof(pstPRI->acUDKSequence) / sizeof(TCHAR));

		pstPRI->iUDKSequenceLen = 0;
		pstPRI->chUDKAssignment = 0;
		pstPRI->iUDKState		= KEY_DIGIT2_NEXT;

		if (IN_RANGE(emuCode,TEXT('1'),TEXT('3')))
			{
			for (idx = 0; idx < 16; idx++)
				{
				if (emuCode == acAscii[idx])
					break;
				}

			pstPRI->chUDKAssignment = (TCHAR)(aiHex[idx] << 4);

			}
		else
			{
			goto UDKexit;
			}

		break;

	case(KEY_DIGIT2_NEXT):
		if (isdigit(emuCode))
			{
			for (idx = 0; idx < 16; idx++)
				{
				if (emuCode == acAscii[idx])
					break;
				}

			pstPRI->chUDKAssignment += (TCHAR)aiHex[idx];

			// The key to which the following sequence will be assigned
			// has been identified.  Lookup  that key in a table and
			// store an index that corresponds to the key table index.
			// See the initialization function for the emulator for
			// further clarification.
			//
			for (idx = 0; idx < MAX_UDK_KEYS; idx++)
				{
				if (pstPRI->chUDKAssignment == pstPRI->pacUDKSelectors[idx])
					break;
				}

			// Process a possible error.
			//
			if (idx >= MAX_UDK_KEYS)
				{
				goto UDKexit;
				}

			// When the sequence is saved in the key table,
			// this index will be used to identify which
			// key in that table will get the user defined sequence.
			//
			pstPRI->iUDKTableIndex  = idx;
			pstPRI->iUDKState		= SLASH_NEXT;
			}
		else
			{
			goto UDKexit;
			}

		break;

	case (SLASH_NEXT):
		if (emuCode == TEXT('/'))
			{
			pstPRI->iUDKState = CHAR_DIGIT1_NEXT;
			}
		else
			{
			goto UDKexit;
			}

		break;

	case (CHAR_DIGIT1_NEXT):
		switch(emuCode)
			{
		case(TEXT(';')):
		case 0x9C:
			if (emuDecStoreUDK(hhEmu) != 0)
				goto UDKexit;

			if (emuCode == TEXT('\x9C'))
				goto UDKexit;

			pstPRI->iUDKState = KEY_NUMBER_NEXT;
			break;

		case(TEXT('\x1B')):
			pstPRI->iUDKState = ESC_SEEN;
			break;

		default:
			if (!isxdigit(emuCode))
				{
				goto UDKexit;
				}

			// Collect the first half of the key comming in.
			//
			for (idx = 0; idx < 16; idx++)
				{
				if (emuCode == acAscii[idx])
					break;
				}

			pstPRI->chUDKAssignment = 0;
			pstPRI->chUDKAssignment = (TCHAR)(aiHex[idx] << 4);
			pstPRI->iUDKState		= CHAR_DIGIT2_NEXT;
			break;
			}

		break;

	case(CHAR_DIGIT2_NEXT):
		if (!isxdigit(emuCode))
			{
			goto UDKexit;
			}

		// This is the second half of the key comming in.
		//
		for (idx = 0; idx < 16; idx++)
			{
			if (emuCode == acAscii[idx])
				break;
			}

		pstPRI->chUDKAssignment += (TCHAR)aiHex[idx];

		if (pstPRI->chUDKAssignment >= 127)
			{
			goto UDKexit;
			}

		pstPRI->acUDKSequence[pstPRI->iUDKSequenceLen] =
			pstPRI->chUDKAssignment;

		pstPRI->iUDKSequenceLen += 1;
		pstPRI->iUDKState = CHAR_DIGIT1_NEXT;

		break;

	case(ESC_SEEN):
		if ((emuCode = TEXT('\\')) == 0)
			{
			goto UDKexit;
			}

		if (emuDecStoreUDK(hhEmu) != 0)
			{
			assert(FALSE);
			}

		// We have completed defining the user defined key sequences.
		// A flag set in emuDecClearUDK was set to identify the locked
		// or unlocked status of the sequences after their definition.
		// Promote that setting up to the variable used by the user
		// interface.
		//
		hhEmu->fAllowUserKeys = pstPRI->fUnlockedUDK;

		goto UDKexit;

	default:
		goto UDKexit;
		}

	// Returning from here allows the state table to pass control
	// back to this function, where the internal state (pstPRI->iUDKState)
	// will be used to control flow through the case statement above.
	//
	return;

UDKexit:
	// The sequence is complete or we're dropping out because of
	// an error.
	//

	// Initialize the UDK state and the emulators state.
	//
	pstPRI->iUDKState = KEY_NUMBER_NEXT;
	hhEmu->state = 0;

	return;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  emuDecStoreUDK
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 * Author:  John Masters
 *
 */
int emuDecStoreUDK(const HHEMU hhEmu)
	{
	const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	PSTEMUKEYDATA pstKey = pstPRI->pstUDK + pstPRI->iUDKTableIndex;

	// There may be a user settings that disables user defined keys.
	// If this feature in not enabled, get outta town.
	//
	if (!hhEmu->fAllowUserKeys)
		{
		return(0);
		}

	// First, free a previously allocated key for this entry, if
	// necessary.
	//
	if (pstKey->iSequenceLen != 0)
		{
		free(pstKey->pSequence);
		pstKey->iSequenceLen = 0;
		}

	// Now allocate the space for the key sequence.
	//
	pstKey->pSequence = malloc( sizeof(TCHAR) *
		(unsigned int)pstPRI->iUDKSequenceLen);

	if (pstKey->pSequence == 0)
		{
		assert(FALSE);
		return(-1);
		}

	// Now, copy the previously collected sequence into the key table
	// and initialize the length variable.
	//
	MemCopy(pstKey->pSequence,
		pstPRI->acUDKSequence,
		(unsigned int)pstPRI->iUDKSequenceLen);

	pstKey->iSequenceLen = (unsigned int)pstPRI->iUDKSequenceLen;

	return(0);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt220_level
 *
 * DESCRIPTION:
 *   Sets the compatibility level of the VT220.
 *
 * ARGUMENTS:
 *   none
 *
 * RETURNS:
 *   nothing
 */
void vt220_level(const HHEMU hhEmu)
	{
	int term, level;

	term = hhEmu->num_param[0];

	level = hhEmu->num_param_cnt > 0 ? hhEmu->num_param[1] : 0;

	if (level < 1)
		level = 0;

	if (term == 61)
		{
		if (hhEmu->mode_vt220)
			vt100_init(hhEmu);
		}

	else if (term == 62 || term == 63)
		{
		if (!hhEmu->mode_vt220)
			vt220_init(hhEmu);  /* sets mode_vt220 & mode_vt320 */

		if (level == 1)
			hhEmu->fUse8BitCodes = FALSE;

		if (level == 0 || level == 2)
			hhEmu->fUse8BitCodes = TRUE;

		if (term == 62 && hhEmu->mode_vt320)
			hhEmu->mode_vt320 = FALSE;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt220_protmode
 *
 * DESCRIPTION:
 *   Sets up and clears protect mode -- called selective erase on vt220.
 *
 * ARGUMENTS:
 *   none
 *
 * RETURNS:
 *   nothing
 */
void vt220_protmode(const HHEMU hhEmu)
	{
	hhEmu->mode_protect = (hhEmu->num_param[0] == 1);

	hhEmu->emu_charattr.protect = (unsigned int)hhEmu->mode_protect;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  emuDecKeyboardIn
 *
 * DESCRIPTION:
 *  This function processes the keyboard keys for all the DEC terminal
 *  emulators.
 *
 *  Please note that both emuSendKeyString, and emuDecSendKeyString are
 *  used in this function.  emuDecSendKeyString is a function that will
 *  convert the 8-bit sequence from the key table to a 7-bit value, if
 *  necessary.
 *
 *  Keys Used to Emulate a DEC Terminal's PF1-PF4 Keys
 *
 *  The keys normally used in HyperACCESS to emulate a DEC terminal's
 *  PF1-PF4 keys are F1-F4. Some people, however, prefer to use the
 *  keys at the top of the PC's numeric keypad (Num Lock, /, *, -),
 *  because these keys' physical location matches that of a DEC
 *  terminal's PF1-PF4 keys. If you prefer using these keys, select
 *  the "PF1-PF4 mapped to top row of keypad" checkbox in the
 *  terminal settings dialog for the DEC terminal you are using.
 *
 *  When "PF1-PF4 mapped to top row of keypad" is checked ...
 *
 *  The keys at the top of the keypad act as PF1-PF4, and F1-F4
 *  revert to performing functions defined by the operating system.
 *  For example, F1 displays help, and Num Lock sends the character
 *  sequence that the DEC terminal associates with PF1. The operating
 *  system will also sense that Num Lock has been pressed, and toggle
 *  the keyboard's Num Lock state. The Num Lock state, however, has
 *  no effect on the behavior of the DEC terminal emulator when
 *  PF1-PF4 are mapped to the top row of the keypad.
 *
 *  When "PF1-PF4 mapped to top row of keypad" is NOT checked...
 *
 *  F1-F4 act as PF1-PF4, and the keys at the top of the keypad (Num
 *  Lock, /, *, -) perform their normal functions. For example, F1
 *  sends the character sequence that the DEC terminal associates
 *  with PF1, and Num Lock toggles the keyboard's Num Lock state.
 *  When Num Lock is on, your PC's keypad (except the top row)
 *  emulates the numeric keypad of a DEC terminal. When Num Lock is
 *  off, your keypad's arrow keys emulate a DEC terminal's arrow
 *  keys. (If your keyboard has a separate set of arrow keys, that
 *  set will always emulate a DEC terminal's arrow keys, regardless
 *  Num Lock's state.)
 *
 * ARGUMENTS:
 *  HHEMU   hhEmu   -   The Internal emulator handle.
 *  int     Key     -   The key to process.
 *  int     fTest   -   Are we testing, or processing the key.
 *
 * RETURNS:
 *  This function returns the index of the table in which the key was
 *  found.
 *
 * AUTHOR: John Masters, 12-Sep-1995
 */
int emuDecKeyboardIn(const HHEMU hhEmu, int Key, const int fTest)
    {
    const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

    int index;
    int fNumlock;
    int fMovedPfKeys;
    int fSearchKeypad;
    int fCursorKeyMode;
    int fKeypadNumeric;
    int fKeypadApplication;
    int fSearchUDK;

    if (!fTest)
        {
#if 0
        DbgOutStr("VT100_KBDIN", 0, 0, 0, 0, 0);
        DbgOutStr("Key: Char     :0x%x", hKey.VKchar, 0, 0, 0, 0);
        DbgOutStr("     Virtual  :%d", hKey.Virtual, 0, 0, 0, 0);
        DbgOutStr("     Ctrl     :%d", hKey.Ctrl, 0, 0, 0, 0);
        DbgOutStr("     Alt      :%d", hKey.Alt, 0, 0, 0, 0);
        DbgOutStr("     Shift    :%d", hKey.Shift, 0, 0, 0, 0);
        DbgOutStr("     Extended :%d", hKey.Extended, 0, 0, 0, 0);
#endif
        }

    // Initialize some locals.  The keypad is either in Numeric Mode,
    // or Application Mode.  So, the first two locals below are mutually
    // exclusive variables.  They have been defined only to improve
    // readability in this code.
    //
    fKeypadApplication  = hhEmu->mode_DECKPAM;
    fKeypadNumeric      = !fKeypadApplication;
    //fMovedPfKeys        = hhEmu->stUserSettings.fMapPFkeys;
    fMovedPfKeys        = FALSE;
    //fNumlock            = emuProjQueryNumlockState(hhEmu);
	fNumlock			= FALSE;		// It doesn't matter.
    fSearchKeypad       = (fMovedPfKeys || fNumlock);
    fCursorKeyMode      = (hhEmu->mode_DECCKM == SET) &&
        (hhEmu->nEmuLoaded != EMU_VT52);
    fSearchUDK          = hhEmu->fAllowUserKeys &&
        ((hhEmu->nEmuLoaded == EMU_VT220) ||
        (hhEmu->nEmuLoaded == EMU_VT320));

    assert(fKeypadApplication != fKeypadNumeric);

	/* -------------- Check Backspace & Delete keys ------------- */

	if (hhEmu->stUserSettings.fReverseDelBk && ((Key == VK_BACKSPACE) ||
			(Key == DELETE_KEY) || (Key == DELETE_KEY_EXT)))
		{
		Key = (Key == VK_BACKSPACE) ? DELETE_KEY : VK_BACKSPACE;
		}

    // F1 thru F4 from either function keys on the top of the keyboard,
    // or from the function key pad on the left.  (They have not been
    // mapped to the top row of the numeric keypad).
    //
    if (!fMovedPfKeys && (index = emuDecKbdKeyLookup(hhEmu,
			Key, pstPRI->pstcEmuKeyTbl1, pstPRI->iKeyTable1Entries)) != -1)
        {
        if (!fTest)
            emuDecSendKeyString(hhEmu, index,
					pstPRI->pstcEmuKeyTbl1, pstPRI->iKeyTable1Entries);
        }

    // F1 thru F4, if they have been mapped to the top row of the
    // numeric keypad (Numlock, /, *, -).
    //
    else if (fMovedPfKeys && (index = emuDecKbdKeyLookup(hhEmu,
			Key, pstPRI->pstcEmuKeyTbl2, pstPRI->iKeyTable2Entries)) != -1)
        {
        if (!fTest)
            emuDecSendKeyString(hhEmu, index,
					pstPRI->pstcEmuKeyTbl2, pstPRI->iKeyTable2Entries);
        }

#if FALSE	// HT doesn't know the state of the numlock.
    // Keypad Numeric Mode.
    //
    else if (fSearchKeypad && fKeypadNumeric &&
			(index = emuDecKbdKeyLookup(hhEmu, Key, pstPRI->pstcEmuKeyTbl3,
					pstPRI->iKeyTable3Entries)) != -1)
        {
        if (!fTest)
            emuDecSendKeyString(hhEmu, index,
					pstPRI->pstcEmuKeyTbl3, pstPRI->iKeyTable3Entries);
        }
#endif

    // Keypad Application Mode.
    //
    else if (fSearchKeypad && fKeypadApplication &&
			(index = emuDecKbdKeyLookup(hhEmu, Key, pstPRI->pstcEmuKeyTbl4,
					pstPRI->iKeyTable4Entries)) != -1)
        {
        if (!fTest)
            emuDecSendKeyString(hhEmu, index,
					pstPRI->pstcEmuKeyTbl4, pstPRI->iKeyTable4Entries);
        }

    // Cursor Key Mode.
    //
    else if (fCursorKeyMode &&
			(index = emuDecKbdKeyLookup(hhEmu, Key, pstPRI->pstcEmuKeyTbl5,
					pstPRI->iKeyTable5Entries)) != -1)
        {
        if (!fTest)
            emuDecSendKeyString(hhEmu, index,
					pstPRI->pstcEmuKeyTbl5, pstPRI->iKeyTable5Entries);
        }

    // User defined keys.
    //
    else if (fSearchUDK &&
			(index = emuDecKbdKeyLookup(hhEmu, Key, pstPRI->pstUDK,
					pstPRI->iUDKTableEntries)) != -1)
        {
        if (!fTest)
            emuDecSendKeyString(hhEmu, index, pstPRI->pstUDK,
					pstPRI->iUDKTableEntries);
        }

    // Standard keys.
    //
    else if ((index = emuDecKbdKeyLookup(hhEmu, Key,
			pstPRI->pstcEmuKeyTbl6, pstPRI->iKeyTable6Entries)) != -1)
        {
        if (!fTest)
            {
            emuDecSendKeyString(hhEmu, index,
					pstPRI->pstcEmuKeyTbl6, pstPRI->iKeyTable6Entries);
            }
        }

    // Standard characters.
    //
    else
        {
        //DbgOutStr("VT100Kbdin calling std_kbdin", 0, 0, 0, 0, 0);

        index = std_kbdin(hhEmu, Key, fTest);
        }

    return index;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  emuDecKbdKeyLookup
 *
 * DESCRIPTION:
 *  Main keyboard translation routine for all emulators.  Note, this
 *  routine will not lookup keys unless the iUseTermKeys flag is set.
 *
 * ARGUMENTS:
 *  HHEMU hhEmu - Internal emulator handle
 *  UINT    key - lower byte is char or virtual key, upper byte has flags
 *
 * RETURNS:
 *  Index if translated, else minus one.
 *
 */
int emuDecKbdKeyLookup(const HHEMU hhEmu,
                    const KEYDEF Key,
                    PSTCEMUKEYDATA pstKeyTbl,
                    const int iMaxEntries)
    {
    PSTCEMUKEYDATA pstKey = pstKeyTbl;

    int idx,
        iRet;

    // There is no need to look for the key if the user has
    // the terminal set up for accelerator keys.
    //
    if (hhEmu->stUserSettings.nTermKeys == EMU_KEYS_ACCEL)
        {
        return -1;
        }

    // Do a linear search through the supplied table for the given
    // key.  Once it is found, return that index, or return (-1) if
    // the key is not located.
    //
    iRet = (-1);

    // The VT220 and VT320 key tables have user defined key tables
    // that have empty sequence entries in them, unless they have been
    // defined by the host. So, if they're empty, we're not going to
    // report them as keys that have been found.
    //
    for (idx = 0; idx < iMaxEntries; idx++, pstKey++)
        {
        if (pstKey->Key == Key && pstKey->iSequenceLen != 0)
            {
            iRet = idx;
            break;
            }
        }

    return iRet;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecSendKeyString
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *   nothing
 */
void emuDecSendKeyString(const HHEMU hhEmu, const int iIndex,
                        PSTCEMUKEYDATA pstcKeyTbl, const int iMaxEntries)
    {
    PSTCEMUKEYDATA pstKeyData   = pstcKeyTbl + iIndex;
    const PSTDECPRIVATE pstPRI  = (PSTDECPRIVATE)hhEmu->pvPrivate;
    const int fUDK              = (pstcKeyTbl == pstPRI->pstUDK) ? 1 : 0;
	int iLen;
    unsigned int iLeadByte;
    TCHAR   *pchLeadChar;
	TCHAR  str[80];

    if (pstcKeyTbl == 0)
        {
        assert(FALSE);
        return;
        }

 	memset(str, 0, sizeof(str));

    // If we are processing VT52 or VT100 keys, call the standard
    // send string function.  The VT100 doesn't switch between 7 and 8
    // bit controls.
    //
    if (hhEmu->nEmuLoaded == EMU_VT100 || hhEmu->nEmuLoaded == EMU_VT52)
        {
        emuVT220SendKeyString(hhEmu, iIndex, pstcKeyTbl, iMaxEntries);
        return;
        }

    // 7-bit controls don't apply to the user defined sequences.
    // Also, because empty key sequences are defined in the User Defined
    // Key Table, it is possible that the key sequence value is NULL. So,
    // we'll process the User Defined Key here, checking the sequence
    // before we try to operate on it.
    //
    if (fUDK)
        {
        if (pstKeyData->pSequence)
            {
            emuVT220SendKeyString(hhEmu, iIndex, pstcKeyTbl, iMaxEntries);
            }

        return;
        }

    // If we are are sending 8 bit codes, process the key directly
    // from the key table, as usual.
    //
    if (hhEmu->fUse8BitCodes)
        {
        emuVT220SendKeyString(hhEmu, iIndex, pstcKeyTbl, iMaxEntries);
        return;
        }

    // If we are processing 7 bit codes, the first character in the
    // sequence defined in the key table will be replaced with its 7-bit
    // value, sometimes.
    //
    iLeadByte = *(pstKeyData->pSequence);

    switch(iLeadByte)
        {
    case 0x84:
        // Send Esc - D
        //
        pchLeadChar = TEXT("\x1B\x44\x00");
        break;

    case 0x8F:
        // Send Esc - O
        //
        pchLeadChar = TEXT("\x1B\x4F\x00");
        break;

    case 0x9B:
        // Send Esc - [
        //
        pchLeadChar = TEXT("\x1B\x5B\x00");
        break;

    default:
        // Send sequence as defined in the key table.
        //
        pchLeadChar = TEXT("\x00");
        break;
        }

    // If we are sending a 7-bit version of the sequence, it gets sent out
    // in two pieces, otherwise send the sequence as it is defined
    // in the key table.
    //
    if (*pchLeadChar)
        {
		CnvrtMBCStoECHAR(str, sizeof(str), pchLeadChar,
				(unsigned long)StrCharGetByteCount(pchLeadChar));
		iLen = StrCharGetEcharByteCount(str);
		CnvrtMBCStoECHAR(&str[iLen], sizeof(str) - iLen, 
				pstKeyData->pSequence + 1, 
				(unsigned long)StrCharGetByteCount(pstKeyData->pSequence + 1));
		emuSendString(hhEmu, str, StrCharGetEcharByteCount(str));
        }
    else
        {
        emuVT220SendKeyString(hhEmu, iIndex, pstcKeyTbl, iMaxEntries);
        }

    return;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emuVT220SendKeyString
 *
 * DESCRIPTION:
 *  Sends the specified string.
 *
 *	This is a VT220 specific replacement for emuSendKeyString in emu.c. It's
 *  the emuSendKeyString from \shared\emulator\emu.c.
 *
 * ARGUMENTS:
 *  hhEmu       - The internal emulator handle.
 *  nIndex      - Position of key in keytable array.
 *  pstKeyTbl   - Address of key strings table.
 *
 * RETURNS:
 *  nothing
 *
 */
void emuVT220SendKeyString(const HHEMU hhEmu,
                        const int iIndex,
                        PSTCEMUKEYDATA pstcKeyTbl,
                        const int iMaxEntries)
    {
	ECHAR  str[80];
    PSTCEMUKEYDATA pstKeyData = pstcKeyTbl + iIndex;

 	memset(str, 0, sizeof(str));
	
	if (iIndex < 0 || iIndex >= iMaxEntries)
        {
        assert(FALSE);
        return;
        }

    pstKeyData = pstcKeyTbl + iIndex;

    #if defined(_DEBUG)

    DbgOutStr("%s", pstcKeyTbl[iIndex].pszKeyName, 0, 0, 0, 0);

    #endif

    if (pstKeyData->iSequenceLen > 0)
        {
		CnvrtMBCStoECHAR(str, sizeof(str), pstKeyData->pSequence, 
				(unsigned long)StrCharGetByteCount(pstKeyData->pSequence));
		emuSendString(hhEmu, str, StrCharGetEcharByteCount(str));
        }
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecEL
 *
 * DESCRIPTION:
 *  Erase In Line (EL).  This control function erases characters from
 *  part or all of the current line.  When you erase complete lines, they
 *  become single height and single width, with all character attributes
 *  cleared.
 *
 *  Selective Erase in Display (SED).  This control function lets you
 *  erase some or all of the erasable characters in the display.  DECSED
 *  cam only erase characters defined as erasable by the DECSCA control
 *  function.  A selective erase is controled by the DEC_private flag
 *  in the emulator handle.  This is dealt with in the lower level
 *  function that does the actual erasing.
 *
 * ARGUMENTS:
 *  hhEmu   -   The internal emulator handle.
 *
 * RETURNS:
 *   nothing
 */
void emuDecEL(const HHEMU hhEmu)
    {
    int iClearType;

    switch (hhEmu->selector[0])
        {
    case 0:
    case 0x0F:
    case 0xF0:
        if (hhEmu->emu_curcol == 0)
            iClearType = CLEAR_ENTIRE_LINE;
        else
            iClearType = CLEAR_CURSOR_TO_LINE_END;

        break;

    case 1:
    case 0xF1:
        iClearType = CLEAR_LINE_START_TO_CURSOR;
        break;

    case 2:
    case 0xF2:
        iClearType = CLEAR_ENTIRE_LINE;
        break;

    default:
        commanderror(hhEmu);
        return;
        }

    emuDecClearLine(hhEmu, iClearType);
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emuDecClearLine
 *
 * DESCRIPTION:
 *  Erases some or all of the virtual screen image.
 *
 * ARGUMENTS:
 *  HHEMU hhEmu         -   The internal emulator handle.
 *
 *  int iClearSelect    -   CLEAR_CURSOR_TO_LINE_END      0
 *                      -   CLEAR_LINE_START_TO_CURSOR    1
 *                      -   CLEAR_ENTIRE_LINE             2
 *
 *
 * RETURNS:
 *  nothing
 *
 */
void emuDecClearLine(const HHEMU hhEmu, const int iClearSelect)
    {
    register int	iCurrentImgRow, iCol;
	ECHAR	*		pechText = 0;
    PSTATTR			pstCell = 0;

    const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

    iCurrentImgRow = row_index(hhEmu, hhEmu->emu_currow);

    pstCell = hhEmu->emu_apAttr[iCurrentImgRow];
    pechText = hhEmu->emu_apText[iCurrentImgRow];

    switch (iClearSelect)
        {
    case CLEAR_CURSOR_TO_LINE_END:

        // Clear the current line from the current cursor position
        // through the end of the user's maximum screen width.
        //
        for(iCol = hhEmu->emu_curcol; iCol <= hhEmu->emu_maxcol; iCol++)
            {
            // If we're in DEC private mode and the character is
            // protected, don't do anything with it.
            //
            if (hhEmu->DEC_private && pstCell[iCol].protect)
                {
                continue;
                }

            // OK, clear the character and it's attribute at this column
            // location.
            //
			pechText[iCol] = (ECHAR)EMU_BLANK_CHAR;
            pstCell[iCol] = hhEmu->emu_clearattr;
            }

        // Adjust the array that contains the column number of the last
        // character in this row.
        //
        if (hhEmu->emu_curcol <= hhEmu->emu_aiEnd[iCurrentImgRow])
            {
            hhEmu->emu_aiEnd[iCurrentImgRow] = max(hhEmu->emu_curcol - 1, 0);
            }

        break;

    case CLEAR_LINE_START_TO_CURSOR:

        // Clear the range from the beginning of the line, through the
        // current cursor position.
        //
        for(iCol = 0; iCol <= hhEmu->emu_curcol; iCol++)
            {
            // If we're in DEC private mode and the character is
            // protected, don't do anything with it.
            //
            if (hhEmu->DEC_private && pstCell[iCol].protect)
                {
                continue;
                }

            // OK, clear the character and it's attribute at this column
            // location.
            //
			pechText[iCol] = (ECHAR)EMU_BLANK_CHAR;
            pstCell[iCol] = hhEmu->emu_clearattr;
            }

        // Adjust the array value that contains the column number
        // of the last character in this row.
        //
        if (hhEmu->emu_curcol >= hhEmu->emu_aiEnd[iCurrentImgRow])
            {
            hhEmu->emu_aiEnd[iCurrentImgRow] = EMU_BLANK_LINE;
            }

        break;

    case CLEAR_ENTIRE_LINE:

        // The entire line needs to be cleared, but we only want
        // to put the user defined size of the emulator into the
        // backscroll buffer.
        //
		backscrlAdd(sessQueryBackscrlHdl(hhEmu->hSession),
						pechText, hhEmu->emu_maxcol + 1);

        if (hhEmu->DEC_private)
            emuDecClearImageRowSelective(hhEmu, hhEmu->emu_currow);
        else
            clear_imgrow(hhEmu, hhEmu->emu_currow);

        pstPRI->aiLineAttr[iCurrentImgRow] = NO_LINE_ATTR;

        break;

    default:
        commanderror(hhEmu);
        }

    (*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow, hhEmu->emu_curcol);

    // Added a global to save the clear attribute at the time of
    // notification.  This is necessary since the message is posted
    // and a race condition can develop.
    //
    hhEmu->emu_clearattr_sav = hhEmu->emu_clearattr;

	updateLine(sessQueryUpdateHdl(hhEmu->hSession), 0, hhEmu->emu_currow);
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuVT220ED
 *
 * DESCRIPTION:
 *  Erase In Display (ED).  This control function erases characters from
 *  part or all of the display.  When you erase complete lines, they
 *  become single height and single width, with all character attributes
 *  cleared.
 *
 *  Selective Erase in Display (SED).  This control function lets you
 *  erase some or all of the erasable characters in the display.  DECSED
 *  cam only erase characters defined as erasable by the DECSCA control
 *  function.  A selective erase is controled by the DEC_private flag
 *  in the emulator handle.  This is dealt with in the lower level
 *  function that does the actual erasing.
 *
 * ARGUMENTS:
 *  hhEmu   -   The internal emulator handle.
 *
 * RETURNS:
 *   nothing
 */
void emuVT220ED(const HHEMU hhEmu)
    {
    int iClearType;

    switch (hhEmu->selector[0])
        {
    case 0:
    case 0x0F:
    case 0xF0:
        if (hhEmu->emu_currow == 0  && hhEmu->emu_curcol == 0)
            iClearType = CLEAR_ENTIRE_SCREEN;
        else
            iClearType = CLEAR_CURSOR_TO_SCREEN_END;

        break;

    case 1:
    case 0xF1:
        iClearType = CLEAR_SCREEN_START_TO_CURSOR;
        break;

    case 2:
    case 0xF2:
        iClearType = CLEAR_ENTIRE_SCREEN;
        break;

    default:
        commanderror(hhEmu);
        return;
        }

    emuDecEraseScreen(hhEmu, iClearType);
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  emuDecEraseScreen
 *
 * DESCRIPTION:
 *  Erases some or all of the virtual screen image.
 *
 * ARGUMENTS:
 *  HHEMU hhEmu         -   The internal emulator handle.
 *
 *  int iClearSelect    -   CLEAR_CURSOR_TO_SCREEN_END      0
 *                      -   CLEAR_SCREEN_START_TO_CURSOR    1
 *                      -   CLEAR_ENTIRE_SCREEN             2
 *
 *
 * RETURNS:
 *  nothing
 *
 */
void emuDecEraseScreen(const HHEMU hhEmu, const int iClearSelect)
    {
    register int	iRow;
    int				trow,
					iStartRow, iEndRow,
					tcol,
					iVirtualRow,
					iLineLen;
	ECHAR			aechBuf[10];

	ECHAR	*		pechText = 0;
    PSTATTR			pstCell = 0;


    const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

    trow = hhEmu->emu_currow;
    tcol = hhEmu->emu_curcol;

    switch (iClearSelect)
        {
    case CLEAR_CURSOR_TO_SCREEN_END:

        // Note that the calling function (emuDecED) changes iClearSelect
        // to CLEAR_ENTIRE_SCREEN if CLEAR_CURSOR_TO_SCREEN_END was
        // issued with a current cursor position at (0,0).  Clearing
        // the entire screen pushes the screen image into the backscroll
        // buffer, the capture file, and to the printer.  Anything less
        // than that simply gets cleared from the display.  Now...

        // Clear the range from one line below the current cursor position
        // through the end of the user's maximum screen size.
        //
        for (iRow = hhEmu->emu_currow + 1; iRow <= hhEmu->emu_maxrow; iRow++)
            {
            if (hhEmu->DEC_private)
                emuDecClearImageRowSelective(hhEmu, iRow);
            else
                clear_imgrow(hhEmu, iRow);
            }

        // Now clear the characters and attributes from the partial
        // row that the cursor was on.
        //
		emuDecClearLine(hhEmu, CLEAR_CURSOR_TO_LINE_END);

        // Identify the range of line attributes that need to be cleared,
        // and clear them.
        //
        if (hhEmu->emu_curcol == 0)
            iStartRow = hhEmu->emu_currow;
        else
            iStartRow = hhEmu->emu_currow + 1;

        iStartRow = min(iStartRow, hhEmu->emu_maxrow);

        iEndRow = hhEmu->emu_maxrow;

        if (iStartRow >= 0)
            {
            for(iRow = iStartRow; iRow <= iEndRow; iRow++)
                pstPRI->aiLineAttr[iRow] = NO_LINE_ATTR;
            }

        // Finally, update the image.
        //
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
        //
        hhEmu->emu_clearattr_sav = hhEmu->emu_clearattr;

		NotifyClient(hhEmu->hSession, EVENT_EMU_CLRATTR, 0);

        break;

    case CLEAR_SCREEN_START_TO_CURSOR:

        // Clear the range from the first row, to one row above the
        // current cursor position.
        //
        for (iRow = 0; iRow < hhEmu->emu_currow; iRow++)
            {
            if (hhEmu->DEC_private)
                emuDecClearImageRowSelective(hhEmu, iRow);
            else
                clear_imgrow(hhEmu, iRow);
            }

        // Now clear the partial (current) row.
        //
		emuDecClearLine(hhEmu, CLEAR_LINE_START_TO_CURSOR);

        // Identify the range of line attributes that need to be cleared,
        // and clear them.
        //
        iStartRow = 0;

        if (hhEmu->emu_curcol == hhEmu->emu_maxcol)
            iEndRow = hhEmu->emu_currow;
        else
            iEndRow = hhEmu->emu_currow - 1;

        iEndRow = max(iEndRow, 0);

        if (iStartRow >= 0)
            {
            for(iRow = iStartRow; iRow <= iEndRow; iRow++)
                pstPRI->aiLineAttr[iRow] = NO_LINE_ATTR;
            }

        (*hhEmu->emu_setcurpos)(hhEmu, hhEmu->emu_currow, hhEmu->emu_curcol);

		updateLine(sessQueryUpdateHdl(hhEmu->hSession), 0, hhEmu->emu_currow);

        break;

    case CLEAR_ENTIRE_SCREEN:

        // The entire buffer needs to be cleared, but we only want
        // to put the user defined size of the emualtor into the
        // backscroll buffer, capture file and print file.
        //
        for (iRow = 0; iRow < MAX_EMUROWS; iRow++)
            {
            if (iRow <= hhEmu->emu_maxrow)
                {
                iVirtualRow = row_index(hhEmu, iRow);
                iLineLen    = emuRowLen(hhEmu, iVirtualRow);
                pechText    = hhEmu->emu_apText[iVirtualRow];

				backscrlAdd(sessQueryBackscrlHdl(hhEmu->hSession),
						pechText, iLineLen);

				CaptureLine(sessQueryCaptureFileHdl(hhEmu->hSession),
						CF_CAP_SCREENS, pechText, iLineLen);

				printEchoScreen(hhEmu->hPrintEcho, pechText, iVirtualRow);

				CnvrtMBCStoECHAR(aechBuf, sizeof(aechBuf), "\r\n", 
						(unsigned long)StrCharGetByteCount("\r\n"));

				printEchoScreen(hhEmu->hPrintEcho, aechBuf, sizeof(ECHAR) * 2);

                if (hhEmu->DEC_private)
                    emuDecClearImageRowSelective(hhEmu, iRow);
                else
                    clear_imgrow(hhEmu, iRow);
                }
            else
                {
                clear_imgrow(hhEmu, iRow);
                }

            pstPRI->aiLineAttr[iRow] = NO_LINE_ATTR;
            }

        // Scroll the imgae.
        //
		updateScroll(sessQueryUpdateHdl(hhEmu->hSession),
						0,
						hhEmu->emu_maxrow,
						hhEmu->emu_maxrow + 1,
						hhEmu->emu_imgtop,
						TRUE);

        // Added a global to save the clear attribute at the time of
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

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  emuDecClearImageRowSelective
 *
 * DESCRIPTION:
 *  This function clears the characters and attributes from the supplied
 *  row, taking into account the protected bit in the character
 *  attributes.  It only clears those characters that are NOT protected.
 *
 * ARGUMENTS:
 *  HHEMU hhEmu         -   The internal emulator handle.
 *
 *  int iRow            -   The row to clear.
 *
 * RETURNS:
 *  nothing
 */
void emuDecClearImageRowSelective(const HHEMU hhEmu, const int iImageRow)
    {
    register int i, iRow;
	ECHAR	*pechText = 0;
    PSTATTR  pstCell = 0;

    iRow = row_index(hhEmu, iImageRow);

    pstCell = hhEmu->emu_apAttr[iRow];
    pechText = hhEmu->emu_apText[iRow];

    // Clear only the characters (and attributes) of the non-protected
    // characters.
    //
    for (i = 0; i < MAX_EMUCOLS; i++)
        {
        if ( pstCell[i].protect == 0 )
            {
            pstCell[i] = hhEmu->emu_clearattr;
			pechText[i] = EMU_BLANK_CHAR;
            }

        if (pechText[i] != EMU_BLANK_CHAR)
            hhEmu->emu_aiEnd[iRow] = i;
        }

    return;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDecUnload
 *
 * DESCRIPTION:
 *   Unloads current emulator by freeing used memory.
 *
 * ARGUMENTS:
 *   none
 *
 * RETURNS:
 *   nothing
 */
void emuDecUnload(const HHEMU hhEmu)
    {
    const PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

    PSTEMUKEYDATA pstKey = pstPRI->pstUDK;

    int idx;

    if (pstPRI)
        {
        // Clear the line attribute array.
        //
        if (pstPRI->aiLineAttr)
            {
            free(pstPRI->aiLineAttr);
            pstPRI->aiLineAttr = 0;
            }

        // Cycle through the user defined key table and free
        // any memory that may have been allocated for sequences.
        //
        if (pstKey)
            {
            for (idx = 0; idx < MAX_UDK_KEYS; idx++, pstKey++)
                {
                if (pstKey->iSequenceLen != 0)
                    {
                    free(pstKey->pSequence);
                    pstKey->iSequenceLen = 0;
                    }
                }
            }

        free(pstPRI);
        //pstPRI = NULL; //mpt:12-21-98 cannot modify a const object

        hhEmu->pvPrivate = NULL;
        }

    return;
    }
#endif // INCL_VT220

/* end of VT220.C */

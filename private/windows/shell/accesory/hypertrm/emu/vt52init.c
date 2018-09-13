/*	File: D:\WACKER\emu\vt52init.c (Created: 28-Dec-1993)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:28p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\assert.h>
#include <tdll\mc.h>
#include <tdll\backscrl.h>
#include <tdll\tchar.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"
#include "emudec.hh"
#include "keytbls.h"

static void vt52char_reset(const HHEMU hhEmu);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt52_init
 *
 * DESCRIPTION:
 *	 Loads and initializes the VT52 emulator.
 *
 * ARGUMENTS:
 *	 new_emu -- TRUE if emulating a power up on the real thing.
 *
 * RETURNS:
 *	 nothing
 */
void vt52_init(const HHEMU hhEmu)
	{
	PSTDECPRIVATE pstPRI;

	static struct trans_entry const vt52_tbl[] =
		{
		{NEW_STATE, 0, 0, 0}, // State 0
		{0, ETEXT('\x20'),	ETEXT('\x7E'),	emuStdGraphic}, 	// Space - ~
		{1, ETEXT('\x1B'),	ETEXT('\x1B'),	nothing},			// Esc
		{0, ETEXT('\x07'),	ETEXT('\x07'),	emu_bell},			// Ctrl-G
		{0, ETEXT('\x08'),	ETEXT('\x08'),	vt_backspace},		// Backspace
		{0, ETEXT('\x09'),	ETEXT('\x09'),	tabn},				// Tab
		{0, ETEXT('\x0A'),	ETEXT('\x0A'),	emuLineFeed},		// New Line
		{0, ETEXT('\x0D'),	ETEXT('\x0D'),	carriagereturn},	// CR
		{5, ETEXT('\x18'),	ETEXT('\x18'),	EmuStdChkZmdm}, 	// Ctrl-X

		{NEW_STATE, 0, 0, 0}, // State 1						// Esc
		{2, ETEXT('\x59'),	ETEXT('\x59'),	nothing},			// Y
		{0, ETEXT('\x3C'),	ETEXT('\x3C'),	vt52_toANSI},		// <
		{0, ETEXT('\x3D'),	ETEXT('\x3E'),	vt_alt_kpmode}, 	// = - >
		{0, ETEXT('\x41'),	ETEXT('\x41'),	ANSI_CUU},			// A
		{0, ETEXT('\x42'),	ETEXT('\x42'),	ANSI_CUD},			// B
		{0, ETEXT('\x43'),	ETEXT('\x43'),	ANSI_CUF},			// C
		{0, ETEXT('\x44'),	ETEXT('\x44'),	vt_CUB},			// D
		{0, ETEXT('\x46'),	ETEXT('\x47'),	vt_charshift},		// F - G
		{0, ETEXT('\x48'),	ETEXT('\x48'),	ANSI_CUP},			// H
		{0, ETEXT('\x49'),	ETEXT('\x49'),	ANSI_RI},			// I
		{0, ETEXT('\x4A'),	ETEXT('\x4A'),	ANSI_ED},			// J
		{0, ETEXT('\x4B'),	ETEXT('\x4B'),	ANSI_EL},			// K
		{0, ETEXT('\x56'),	ETEXT('\x56'),	vt52PrintCommands}, // V
		{4, ETEXT('\x57'),	ETEXT('\x57'),	nothing},			// W
		{0, ETEXT('\x58'),	ETEXT('\x58'),	nothing},			// X
		{0, ETEXT('\x5A'),	ETEXT('\x5A'),	vt52_id},			// Z
		{0, ETEXT('\x5D'),	ETEXT('\x5D'),	vt52PrintCommands}, // ]
		{0, ETEXT('\x5E'),	ETEXT('\x5E'),	vt52PrintCommands}, // ^
		{0, ETEXT('\x5F'),	ETEXT('\x5F'),	vt52PrintCommands}, // _

		{NEW_STATE, 0, 0, 0}, // State 2						// EscY
		// Accept all data--CUP will set the limits. Needed for more than 24 rows.
		{3, ETEXT('\x00'),	ETEXT('\xFF'),	char_pn},			// Space - 8
//		{3, ETEXT('\x20'),	ETEXT('\x38'),	char_pn},			// Space - 8

		{NEW_STATE, 0, 0, 0}, // State 3						// EscYn
		// Accept all data--CUP will set the limits. Needed for more than 80 columns.
		{0, ETEXT('\x00'),	ETEXT('\xFF'),	vt52_CUP},			// Space - o
//		{0, ETEXT('\x20'),	ETEXT('\x6F'),	vt52_CUP},			// Space - o

		{NEW_STATE, 0, 0, 0}, // State 4						// EscW
		{4, ETEXT('\x00'),	ETEXT('\xFF'),	vt52Print}, 		// All

		{NEW_STATE, 0, 0, 0}, // State 5						// Ctrl-X
		{5, ETEXT('\x00'),	ETEXT('\xFF'),	EmuStdChkZmdm}, 	// All

		};

	emuInstallStateTable(hhEmu, vt52_tbl, DIM(vt52_tbl));

	// Allocate space for and initialize data that is used only by the
	// VT52 emulator.
	//
	if (hhEmu->pvPrivate != 0)
		{
		free(hhEmu->pvPrivate);
		hhEmu->pvPrivate = 0;
		}

	hhEmu->pvPrivate = malloc(sizeof(DECPRIVATE));

	if (hhEmu->pvPrivate == 0)
		{
		assert(FALSE);
		return;
		}

	pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;
	CnvrtMBCStoECHAR(pstPRI->terminate, sizeof(pstPRI->terminate), "\033X", StrCharGetByteCount("\033X"));
	pstPRI->len_t = 2;
	pstPRI->pntr = pstPRI->storage;
	pstPRI->len_s = 0;

	// Initialize standard hhEmu values.
	//
	hhEmu->emu_kbdin 	= vt52_kbdin;
	hhEmu->emu_highchar = 0x7E;
	hhEmu->emu_deinstall = emuVT52Unload;	
	
	vt52char_reset(hhEmu);

//	emuKeyTableLoad(hhEmu, IDT_VT52_KEYS, &hhEmu->stEmuKeyTbl);
//	emuKeyTableLoad(hhEmu, IDT_VT52_KEYPAD_APP_MODE, &hhEmu->stEmuKeyTbl2);
//	emuKeyTableLoad(hhEmu, IDT_VT_MAP_PF_KEYS, &hhEmu->stEmuKeyTbl3);
	emuKeyTableLoad(hhEmu, VT52KeyTable, 
					 sizeof(VT52KeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl);
	emuKeyTableLoad(hhEmu, VT52_Keypad_KeyTable, 
					 sizeof(VT52_Keypad_KeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl2);
	emuKeyTableLoad(hhEmu, VT_PF_KeyTable, 
					 sizeof(VT_PF_KeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl3);

	backscrlSetShowFlag(sessQueryBackscrlHdl(hhEmu->hSession), TRUE);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt52char_reset
 *
 * DESCRIPTION:
 *	 Sets the VT52 emulator character set to its RESET conditions.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
static void vt52char_reset(const HHEMU hhEmu)
	{
	// Set up US ASCII character set as G0 and DEC graphics as G1
	//
	vt_charset_init(hhEmu);
	hhEmu->emu_code = ETEXT(')');
	vt_scs1(hhEmu);
	hhEmu->emu_code = (ECHAR)0;
	vt_scs2(hhEmu);
	}

/* end of vt52init.c */

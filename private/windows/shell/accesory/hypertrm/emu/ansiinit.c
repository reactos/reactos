/*	File: D:\WACKER\emu\ansiinit.c (Created: 08-Dec-1993)
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
#include <tdll\session.h>
#include <tdll\cloop.h>
#include <tdll\mc.h>
#include <tdll\assert.h>
#include <tdll\backscrl.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"
#include "ansi.hh"
#include "keytbls.h"


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuAnsiInit
 *
 * DESCRIPTION:
 *	 Sets up and installs the ANSI state table. Defines the ANSI
 *	 keyboard. Either resets the emulator completely or redefines emulator
 *	 conditions as they were when last saved.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *	 nothing
 */
void emuAnsiInit(const HHEMU hhEmu)
	{
	PSTANSIPRIVATE pstPRI;

	static struct trans_entry const ansi_tbl[] =
		{
		{NEW_STATE, 0, 0, 0}, // State 0
#if !defined(FAR_EAST)
		{0, ETEXT('\x20'),	ETEXT('\xFF'),	emuStdGraphic}, 	// Space - All
#else
		{0, ETEXT('\x20'),	0xFFFF,			emuStdGraphic}, 	// Space - All
#endif
		{1, ETEXT('\x1B'),	ETEXT('\x1B'),	nothing},			// Esc
		{0, ETEXT('\x05'),	ETEXT('\x05'),	vt100_answerback},	// Ctrl-E
		{0, ETEXT('\x07'),	ETEXT('\x07'),	emu_bell},			// Ctrl-G
		{0, ETEXT('\x08'),	ETEXT('\x08'),	backspace}, 		// Backspace
		{0, ETEXT('\x09'),	ETEXT('\x09'),	tabn},				// Tab
		{0, ETEXT('\x0A'),	ETEXT('\x0B'),	emuLineFeed},		// NL - VT
		{0, ETEXT('\x0C'),	ETEXT('\x0C'),	AnsiFormFeed},		// Form Feed
		{0, ETEXT('\x0D'),	ETEXT('\x0D'),	carriagereturn},	// CR
		{3, ETEXT('\x18'),	ETEXT('\x18'),	EmuStdChkZmdm}, 	// Ctrl-X
		{0, ETEXT('\x00'),	ETEXT('\x1F'),	emuStdGraphic}, 	// All Ctrl's

		{NEW_STATE, 0, 0, 0}, // State 1						// Esc
		{2, ETEXT('\x5B'),	ETEXT('\x5B'),	ANSI_Pn_Clr},		// [
		{0, ETEXT('\x44'),	ETEXT('\x44'),	ANSI_IND},			// D
		{0, ETEXT('\x45'),	ETEXT('\x45'),	ANSI_NEL},			// E
		{0, ETEXT('\x48'),	ETEXT('\x48'),	ANSI_HTS},			// H
		{0, ETEXT('\x4D'),	ETEXT('\x4D'),	ANSI_RI},			// M

		{NEW_STATE, 0, 0, 0}, // State 2						// Esc[
		{2, ETEXT('\x30'),	ETEXT('\x39'),	ANSI_Pn},			// 0 - 9
		{2, ETEXT('\x3B'),	ETEXT('\x3B'),	ANSI_Pn_End},		// ;
		{5, ETEXT('\x3D'),	ETEXT('\x3D'),	nothing},			// =
		{2, ETEXT('\x3A'),	ETEXT('\x3F'),	ANSI_Pn},			// : - ?
		{0, ETEXT('\x41'),	ETEXT('\x41'),	ANSI_CUU},			// A
		{0, ETEXT('\x42'),	ETEXT('\x42'),	ANSI_CUD},			// B
		{0, ETEXT('\x43'),	ETEXT('\x43'),	ANSI_CUF},			// C
		{0, ETEXT('\x44'),	ETEXT('\x44'),	ANSI_CUB},			// D
		{0, ETEXT('\x48'),	ETEXT('\x48'),	ANSI_CUP},			// H
		{0, ETEXT('\x4A'),	ETEXT('\x4A'),	ANSI_ED},			// J
		{0, ETEXT('\x4B'),	ETEXT('\x4B'),	ANSI_EL},			// K
		{0, ETEXT('\x4C'),	ETEXT('\x4C'),	ANSI_IL},			// L
		{0, ETEXT('\x4D'),	ETEXT('\x4D'),	ANSI_DL},			// M
		{0, ETEXT('\x50'),	ETEXT('\x50'),	ANSI_DCH},			// P
		{0, ETEXT('\x66'),	ETEXT('\x66'),	ANSI_CUP},			// f
		{0, ETEXT('\x67'),	ETEXT('\x67'),	ANSI_TBC},			// g
		{0, ETEXT('\x68'),	ETEXT('\x68'),	ansi_setmode},		// h
		{0, ETEXT('\x69'),	ETEXT('\x69'),	vt100PrintCommands},// i
		{0, ETEXT('\x6C'),	ETEXT('\x6C'),	ansi_resetmode},	// l
		{0, ETEXT('\x6D'),	ETEXT('\x6D'),	ANSI_SGR},			// m
		{0, ETEXT('\x6E'),	ETEXT('\x6E'),	ANSI_DSR},			// n
		{0, ETEXT('\x70'),	ETEXT('\x70'),	nothing},			// p
		{0, ETEXT('\x72'),	ETEXT('\x72'),	vt_scrollrgn},		// r
		{0, ETEXT('\x73'),	ETEXT('\x73'),	ansi_savecursor},	// s
		{0, ETEXT('\x75'),	ETEXT('\x75'),	ansi_savecursor},	// u

		{NEW_STATE, 0, 0, 0}, // State 3						// Ctrl-X
		{3, ETEXT('\x00'),	ETEXT('\xFF'),	EmuStdChkZmdm}, 	// all codes

		{NEW_STATE, 0, 0, 0}, // State 4						// Ctrl-A
		{4, ETEXT('\x00'),	ETEXT('\xFF'),	nothing},			// all codes

		{NEW_STATE, 0, 0, 0}, // State 5						// Esc[=
		{5, ETEXT('\x32'),	ETEXT('\x32'),	ANSI_Pn},			// 2
		{5, ETEXT('\x35'),	ETEXT('\x35'),	ANSI_Pn},			// 5
		{0, ETEXT('\x68'),	ETEXT('\x68'),	DoorwayMode},		// h
		{0, ETEXT('\x6C'),	ETEXT('\x6C'),	DoorwayMode},		// l
		};

	emuInstallStateTable(hhEmu, ansi_tbl, DIM(ansi_tbl));

	// Allocate space for and initialize data that is used only by the
	// ANSI emulator.
	//
	hhEmu->pvPrivate = malloc(sizeof(ANSIPRIVATE));

	if (hhEmu->pvPrivate == 0)
		{
		assert(FALSE);
		return;
		}

	pstPRI = (PSTANSIPRIVATE)hhEmu->pvPrivate;

	memset(pstPRI, 0, sizeof(ANSIPRIVATE));

	// Initialize standard handle items.
	//
	hhEmu->emuResetTerminal = emuAnsiReset;

	emuKeyTableLoad(hhEmu, AnsiKeyTable, 
					 sizeof(AnsiKeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl);
	emuKeyTableLoad(hhEmu, IBMPCKeyTable, 
					 sizeof(IBMPCKeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl2);

	hhEmu->emu_kbdin = ansi_kbdin;
	hhEmu->emu_deinstall = emuAnsiUnload;
	emuAnsiReset(hhEmu, FALSE);

#if !defined(FAR_EAST)
	hhEmu->emu_highchar = (TCHAR)0xFF;
#else
	hhEmu->emu_highchar = (TCHAR)0xFFFF;
#endif

	backscrlSetShowFlag(sessQueryBackscrlHdl(hhEmu->hSession), TRUE);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuAnsiReset
 *
 * DESCRIPTION:
 *	 Resets the ANSI emulator.
 *
 * ARGUMENTS:
 *	fHostRequest	-	TRUE if result of codes from host
 *
 * RETURNS:
 *	 nothing
 */
int emuAnsiReset(const HHEMU hhEmu, const int fHostRequest)
	{
	hhEmu->mode_KAM = hhEmu->mode_IRM = hhEmu->mode_VEM =
	hhEmu->mode_HEM = hhEmu->mode_LNM = hhEmu->mode_DECCKM =
	hhEmu->mode_DECOM  = hhEmu->mode_DECCOLM  = hhEmu->mode_DECPFF =
	hhEmu->mode_DECPEX = hhEmu->mode_DECSCNM =
	hhEmu->mode_25enab = hhEmu->mode_protect =
	hhEmu->mode_block = hhEmu->mode_local = RESET;

	hhEmu->mode_SRM = hhEmu->mode_DECTCEM = SET;

	hhEmu->mode_AWM = hhEmu->stUserSettings.fWrapLines;

	if (fHostRequest)
		{
		ANSI_Pn_Clr(hhEmu);
		ANSI_SGR(hhEmu);
		ANSI_RIS(hhEmu);
		}

	return 0;
	}

/* end of ansiinit.c */

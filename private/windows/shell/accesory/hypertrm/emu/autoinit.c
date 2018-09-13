/*	File: D:\WACKER\emu\autoinit.c (Created: 28-Feb-1994)
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
#include <tdll\assert.h>
#include <tdll\mc.h>
#include <tdll\backscrl.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"
#include "ansi.hh"
#include "viewdata.hh"
#include "minitel.hh"
#include "keytbls.h"


static void emuAutoNothingVT52(const HHEMU hhEmu);
static void emuAutoVT52toAnsi(const HHEMU hhEmu);
static void emuAutoAnsiEdVT52(const HHEMU hhEmu);
static void emuAutoAnsiElVT52(const HHEMU hhEmu);
static void emuAutoCharPnVT52(const HHEMU hhEmu);
static void emuAutoNothingVT100(const HHEMU hhEmu);
static void emuAutoScs1VT100(const HHEMU hhEmu);
static void emuAutoSaveCursorVT100(const HHEMU hhEmu);
static void emuAutoAnsiPnEndVT100(const HHEMU hhEmu);
static void emuAutoResetVT100(const HHEMU hhEmu);
static void emuAutoAnsiDaVT100(const HHEMU hhEmu);
static void emuAutoReportVT100(const HHEMU hhEmu);
static void emuAutoNothingViewdata(const HHEMU hhEmu);
static void emuAutoSetAttrViewdata(const HHEMU hhEmu);
static void emuAutoNothingAnsi(const HHEMU hhEmu);
static void emuAutoScrollAnsi(const HHEMU hhEmu);
static void emuAutoSaveCurAnsi(const HHEMU hhEmu);
static void emuAutoPnAnsi(const HHEMU hhEmu);
static void emuAutoDoorwayAnsi(const HHEMU hhEmu);
static void emuAutoNothingMinitel(const HHEMU hhEmu);
static void emuAutoMinitelCharAttr(const HHEMU hhEmu);
static void emuAutoMinitelFieldAttr(const HHEMU hhEmu);
static void emuAutoMinitelCursorReport(const HHEMU hhEmu);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuAutoInit
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *	 nothing
 */
void emuAutoInit(const HHEMU hhEmu)
	{
	PSTANSIPRIVATE pstPRI;

	static struct trans_entry const astfAutoAnsiTable[] =
		{
		// State 0
		//
		// Ansi emulation occupies all of state 0 codes.
		//
		{NEW_STATE, 0, 0, 0},
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
		//
		// State 1
		// At this point, an ESC has been seen.
		//
		{NEW_STATE, 0, 0, 0},										// Esc
		{2, ETEXT('\x5B'),	ETEXT('\x5B'),	ANSI_Pn_Clr},			// [
		{0, ETEXT('\x44'),	ETEXT('\x44'),	ANSI_IND},				// D
		{0, ETEXT('\x45'),	ETEXT('\x45'),	ANSI_NEL},				// E
		{0, ETEXT('\x48'),	ETEXT('\x48'),	ANSI_HTS},				// H
		{0, ETEXT('\x4D'),	ETEXT('\x4D'),	ANSI_RI},				// M
		//
		// Autodetect sequences for for VT52, State 1.
		//
		{2, ETEXT('\x59'),	ETEXT('\x59'),	emuAutoNothingVT52},	// Y
		{0, ETEXT('\x3C'),	ETEXT('\x3C'),	emuAutoVT52toAnsi}, 	// <
		{0, ETEXT('\x4A'),	ETEXT('\x4A'),	emuAutoAnsiEdVT52}, 	// J
		{0, ETEXT('\x4B'),	ETEXT('\x4B'),	emuAutoAnsiElVT52}, 	// K
		//
		// Autodetect sequences for for VT100, State 1.
		//
#if !defined(INCL_MINITEL)
		{3, ETEXT('\x23'),	ETEXT('\x23'),	emuAutoNothingVT100},	// #
#endif
		{4, ETEXT('\x28'),	ETEXT('\x29'),	emuAutoScs1VT100},		// ( - )
		{0, ETEXT('\x38'),	ETEXT('\x38'),	emuAutoSaveCursorVT100},// 8
#if !defined(INCL_MINITEL)
		{1, ETEXT('\x3B'),	ETEXT('\x3B'),	emuAutoAnsiPnEndVT100}, // ;
#endif
		{0, ETEXT('\x63'),	ETEXT('\x63'),	emuAutoResetVT100}, 	// c
		//
		// Autodetect sequences for for Viewdata, State 1.
		//
#if defined(INCL_VIEWDATA)
		{0, ETEXT('\x31'),	ETEXT('\x34'),	emuAutoNothingViewdata},// 1 - 4
#endif
		//
		// Autodetect sequences for for Minitel, State 1.
		//
#if defined(INCL_MINITEL)
		//{1, ETEXT('\x00'),  ETEXT('\x00'),  emuAutoNothingMinitel},
		//{14,ETEXT('\x25'),  ETEXT('\x25'),  emuAutoNothingMinitel},
		//{13,ETEXT('\x35'),  ETEXT('\x37'),  emuAutoNothingMinitel},	  // eat ESC,35-37,X sequences
		//{6, ETEXT('\x39'),  ETEXT('\x39'),  emuAutoNothingMinitel},	  // PROT1, p134
		{7, ETEXT('\x3A'),	ETEXT('\x3A'),	emuAutoNothingMinitel}, 	// PROT2, p134
		{0, ETEXT('\x40'),	ETEXT('\x43'),	emuAutoMinitelCharAttr},	// forground color, flashing
		//{0, ETEXT('\x4C'),  ETEXT('\x4C'),  emuAutoMinitelCharAttr},	  // char width & height
		//{0, ETEXT('\x4E'),  ETEXT('\x4E'),  emuAutoMinitelCharAttr},	  // char width & height
		//{0, ETEXT('\x4F'),  ETEXT('\x4F'),  emuAutoMinitelCharAttr},	  // char width & height
		//{0, ETEXT('\x50'),  ETEXT('\x59'),  emuAutoMinitelFieldAttr},   // background, underlining
		//{0, ETEXT('\x5F'),  ETEXT('\x5F'),  emuAutoMinitelFieldAttr},   // reveal display
		//{0, ETEXT('\x5C'),  ETEXT('\x5D'),  emuAutoMinitelCharAttr},	  // inverse
		//{0, ETEXT('\x61'),  ETEXT('\x61'),  emuAutoMinitelCursorReport},
#endif
		//
		// State 2
		//
		{NEW_STATE, 0, 0, 0},										// Esc[
		{2, ETEXT('\x30'),	ETEXT('\x39'),	ANSI_Pn},				// 0 - 9
		{2, ETEXT('\x3B'),	ETEXT('\x3B'),	ANSI_Pn_End},			// ;
		{5, ETEXT('\x3D'),	ETEXT('\x3D'),	nothing},				// =
		{2, ETEXT('\x3A'),	ETEXT('\x3F'),	ANSI_Pn},				// : - ?
		{0, ETEXT('\x41'),	ETEXT('\x41'),	ANSI_CUU},				// A
		{0, ETEXT('\x42'),	ETEXT('\x42'),	ANSI_CUD},				// B
		{0, ETEXT('\x43'),	ETEXT('\x43'),	ANSI_CUF},				// C
		{0, ETEXT('\x44'),	ETEXT('\x44'),	ANSI_CUB},				// D
		{0, ETEXT('\x48'),	ETEXT('\x48'),	ANSI_CUP},				// H
		{0, ETEXT('\x4A'),	ETEXT('\x4A'),	ANSI_ED},				// J
		{0, ETEXT('\x4B'),	ETEXT('\x4B'),	ANSI_EL},				// K
		{0, ETEXT('\x4C'),	ETEXT('\x4C'),	ANSI_IL},				// L
		{0, ETEXT('\x4D'),	ETEXT('\x4D'),	ANSI_DL},				// M
		{0, ETEXT('\x50'),	ETEXT('\x50'),	ANSI_DCH},				// P
		{0, ETEXT('\x66'),	ETEXT('\x66'),	ANSI_CUP},				// f
		{0, ETEXT('\x67'),	ETEXT('\x67'),	ANSI_TBC},				// g
		{0, ETEXT('\x68'),	ETEXT('\x68'),	ansi_setmode},			// h
		{0, ETEXT('\x69'),	ETEXT('\x69'),	vt100PrintCommands},	// i
		{0, ETEXT('\x6C'),	ETEXT('\x6C'),	ansi_resetmode},		// l
		{0, ETEXT('\x6D'),	ETEXT('\x6D'),	ANSI_SGR},				// m
		{0, ETEXT('\x6E'),	ETEXT('\x6E'),	ANSI_DSR},				// n
		{0, ETEXT('\x70'),	ETEXT('\x70'),	emuAutoNothingAnsi},	// p
		{0, ETEXT('\x72'),	ETEXT('\x72'),	emuAutoScrollAnsi}, 	// r
		{0, ETEXT('\x73'),	ETEXT('\x73'),	emuAutoSaveCurAnsi},	// s
		{0, ETEXT('\x75'),	ETEXT('\x75'),	ansi_savecursor},		// u
		//
		//Autodetect sequences for for VT52, State 2.
		//
		{3, ETEXT('\x20'),	ETEXT('\x20'),	emuAutoCharPnVT52}, 	// Space 
		{3, ETEXT('\x22'),	ETEXT('\x22'),	emuAutoCharPnVT52}, 	// "
		{3, ETEXT('\x24'),	ETEXT('\x2F'),	emuAutoCharPnVT52}, 	// $ - /
		//
		// Autodetect sequences for for VT100, State 2.
		//
		{0, ETEXT('\x63'),	ETEXT('\x63'),	emuAutoAnsiDaVT100},	// c
		{0, ETEXT('\x71'),	ETEXT('\x71'),	emuAutoNothingVT100},	// q
		{0, ETEXT('\x78'),	ETEXT('\x78'),	emuAutoReportVT100},	// x
		//
		// State 3
		//
		{NEW_STATE, 0, 0, 0},
		{3, ETEXT('\x00'),	ETEXT('\xFF'),	EmuStdChkZmdm}, 		// All
		//
		// State 4
		//
		{NEW_STATE, 0, 0, 0},
		{4, ETEXT('\x00'),	ETEXT('\xFF'),	nothing},				// All
		//
		// State 5
		//
		{NEW_STATE, 0, 0, 0},
		{5, ETEXT('\x32'),	ETEXT('\x32'),	emuAutoPnAnsi}, 		// 2
		{5, ETEXT('\x35'),	ETEXT('\x35'),	emuAutoPnAnsi}, 		// 5
		{0, ETEXT('\x68'),	ETEXT('\x68'),	emuAutoDoorwayAnsi},	// h
		{0, ETEXT('\x6C'),	ETEXT('\x6C'),	emuAutoDoorwayAnsi},	// l
		//
		// Autodetect sequences for for VT100, State 5.
		//
		{0, ETEXT('\x70'),	ETEXT('\x70'),	emuAutoNothingVT100},	// p
		//
		// Autodetect sequences for for VT52, State 5.
		//
		{0, ETEXT('\x00'),	ETEXT('\xFF'),	EmuStdChkZmdm}, 		// All
		//
		// State 6
		//
		{NEW_STATE, 0, 0, 0},
		//
		// Autodetect sequences for for VT52, VT100, State 6.
		//
		{6, ETEXT('\x00'),	ETEXT('\xFF'),	nothing},				// All
		//
		// State 7
		//
		// Autodetect sequences for for VT100, State 7.
		//
		{NEW_STATE, 0, 0, 0},
		{7, ETEXT('\x00'),	ETEXT('\xFF'),	EmuStdChkZmdm}, 		// All
		//
		// State 8
		//
		// Autodetect sequences for for VT100, State 8.
		//
		{NEW_STATE, 0, 0, 0},
		{8, ETEXT('\x00'),	ETEXT('\xFF'),	nothing},				// All
		};

	emuInstallStateTable(hhEmu, astfAutoAnsiTable, DIM(astfAutoAnsiTable));

	// Allocate space for and initialize data that is used only by the
	// Auto ANSI emulator.
	//
	hhEmu->pvPrivate = malloc(sizeof(ANSIPRIVATE));

	if (hhEmu->pvPrivate == 0)
		{
		assert(FALSE);
		return;
		}

	pstPRI = (PSTANSIPRIVATE)hhEmu->pvPrivate;

	memset(pstPRI, 0, sizeof(ANSIPRIVATE));

	hhEmu->emuResetTerminal = emuAnsiReset;

	// The Auto Detect emulator is ANSI based.	So, do the same
	// stuff we do when initializing the ANSI emulator.
	//
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
	hhEmu->emu_highchar = (ECHAR)0xFF;
#else
	hhEmu->emu_highchar = (ECHAR)0xFFFF;
#endif

	backscrlSetShowFlag(sessQueryBackscrlHdl(hhEmu->hSession), TRUE);
	return;
	}

// Ansi Auto Detect Functions.
//
void emuAutoNothingAnsi(const HHEMU hhEmu)
	{
#if !defined(FAR_EAST)
	emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
	emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
	nothing(hhEmu);
	return;
	}

void emuAutoScrollAnsi(const HHEMU hhEmu)
	{
#if !defined(FAR_EAST)
	emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
	emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
	vt_scrollrgn(hhEmu);
	return;
	}

void emuAutoSaveCurAnsi(const HHEMU hhEmu)
	{
#if !defined(FAR_EAST)
	emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
	emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
	ansi_savecursor(hhEmu);
	return;
	}

void emuAutoPnAnsi(const HHEMU hhEmu)
	{
#if !defined(FAR_EAST)
	emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
	emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
	ANSI_Pn(hhEmu);
	return;
	}

void emuAutoDoorwayAnsi(const HHEMU hhEmu)
	{
#if !defined(FAR_EAST)
	emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
	emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
	DoorwayMode(hhEmu);
	return;
	}

// VT52 Auto Detect Functions.
//
void emuAutoNothingVT52(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VT52);
	return;
	}

void emuAutoVT52toAnsi(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VT52);
	vt52_toANSI(hhEmu);
	return;
	}

void emuAutoAnsiEdVT52(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VT52);
	ANSI_ED(hhEmu);
	return;
	}

void emuAutoAnsiElVT52(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VT52);
	ANSI_EL(hhEmu);
	return;
	}

void emuAutoCharPnVT52(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VT52);
	char_pn(hhEmu);
	return;
	}

// VT100 Auto Detect Functions.
//
void emuAutoNothingVT100(const HHEMU hhEmu)
	{
#if !defined(FAR_EAST)
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	return;
	}

void emuAutoScs1VT100(const HHEMU hhEmu)
	{
#if !defined(FAR_EAST)
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	vt_scs1(hhEmu);
	return;
	}

void emuAutoSaveCursorVT100(const HHEMU hhEmu)
	{
#if !defined(FAR_EAST)
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	vt100_savecursor(hhEmu);
	return;
	}

void emuAutoAnsiPnEndVT100(const HHEMU hhEmu)
	{
#if !defined(FAR_EAST)
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	ANSI_Pn_End(hhEmu);
	return;
	}

void emuAutoResetVT100(const HHEMU hhEmu)
	{
#if !defined(FAR_EAST)
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	vt100_hostreset(hhEmu);
	return;
	}

void emuAutoAnsiDaVT100(const HHEMU hhEmu)
	{
#if !defined(FAR_EAST)
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	ANSI_DA(hhEmu);
	return;
	}

void emuAutoReportVT100(const HHEMU hhEmu)
	{
#if !defined(FAR_EAST)
	emuAutoDetectLoad(hhEmu, EMU_VT100);
#else
	emuAutoDetectLoad(hhEmu, EMU_VT100J);
#endif
	vt100_report(hhEmu);
	return;
	}

#if defined(INCL_VIEWDATA)
// Viewdata Auto Detect Functions.
//
void emuAutoNothingViewdata(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VIEW);
	return;
	}

void emuAutoSetAttrViewdata(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_VIEW);
	EmuViewdataSetAttr(hhEmu);
	return;
	}
#endif // INCL_VIEWDATA

#if defined(INCL_MINITEL)

void emuAutoNothingMinitel(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_MINI);
	return;
	}

void emuAutoMinitelCharAttr(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_MINI);
	emuMinitelCharAttr(hhEmu);
	return;
	}

void emuAutoMinitelFieldAttr(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_MINI);
	emuMinitelFieldAttr(hhEmu);
	return;
	}

void emuAutoMinitelCursorReport(const HHEMU hhEmu)
	{
	emuAutoDetectLoad(hhEmu, EMU_MINI);
	minitelCursorReport(hhEmu);
	return;
	}

#endif

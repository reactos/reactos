/*	File: D:\WACKER\emu\viewdini.c (Created: 31-Jan-1994)
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
#include <tdll\mc.h>
#include <tdll\assert.h>
#include <tdll\backscrl.h>
#include <tdll\tchar.h>
#include <tdll\term.h>
#include "emu.h"
#include "emu.hh"
#include "viewdata.hh"

#if defined(INCL_VIEWDATA)
#define MAX_ROWS 24

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataInit
 *
 * DESCRIPTION:	Performs the initialization of the Viewdata emulator that
 *              is common to DOS and OS2.
 *
 * ARGUMENTS:   ehdl -- handle the emulator session
 *
 * RETURNS:		nothing
 */
void EmuViewdataInit(const HHEMU hhEmu)
	{
	int i;
	LOGFONT lf;
	HWND hwndTerm;
	PSTVIEWDATAPRIVATE pstPRI;

	static struct trans_entry const astfViewdataTable[] =
	{
	{NEW_STATE, 0, 0, 0}, // State 0
	{0, ETEXT('\x20'),	ETEXT('\x7F'),	EmuViewdataCharDisplay},	// All
	{1, ETEXT('\x1B'),	ETEXT('\x1B'),	nothing},					// Esc
	{0, ETEXT('\x05'),	ETEXT('\x05'),	EmuViewdataAnswerback}, 	// Ctrl-E
	{0, ETEXT('\x08'),	ETEXT('\x08'),	EmuViewdataCursorLeft}, 	// Backspace
	{0, ETEXT('\x09'),	ETEXT('\x09'),	EmuViewdataCursorRight},	// Tab
	{0, ETEXT('\x0A'),	ETEXT('\x0A'),	EmuViewdataCursorDown}, 	// New Line
	{0, ETEXT('\x0B'),	ETEXT('\x0B'),	EmuViewdataCursorUp},		// VT
	{0, ETEXT('\x0C'),	ETEXT('\x0C'),	EmuViewdataClearScreen},	// Form Feed
	{0, ETEXT('\x0D'),	ETEXT('\x0D'),	carriagereturn},			// CR
	{0, ETEXT('\x11'),	ETEXT('\x11'),	EmuViewdataCursorSet},		// Ctrl-Q
	{0, ETEXT('\x14'),	ETEXT('\x14'),	EmuViewdataCursorSet},		// Ctrl-T
	{0, ETEXT('\x1E'),	ETEXT('\x1E'),	EmuViewdataCursorHome}, 	// Ctrl-^
	{0, ETEXT('\x80'),	ETEXT('\xFF'),	EmuChkChar},				// Upper Ascii

	{NEW_STATE, 0, 0, 0}, // State 1								// Esc
	{0, ETEXT('\x31'),	ETEXT('\x37'),	nothing},					// 1 - 7
	{0, ETEXT('\x41'),	ETEXT('\x49'),	EmuViewdataSetAttr},		// A - I
	{0, ETEXT('\x4C'),	ETEXT('\x4D'),	EmuViewdataSetAttr},		// L - M
	{0, ETEXT('\x51'),	ETEXT('\x5A'),	EmuViewdataSetAttr},		// Q - Z
	{0, ETEXT('\x5C'),	ETEXT('\x5D'),	EmuViewdataSetAttr},		// \ - ]
	{0, ETEXT('\x5E'),	ETEXT('\x5E'),	EmuViewdataMosaicHold}, 	// ^
	{0, ETEXT('\x5F'),	ETEXT('\x5F'),	EmuViewdataMosaicRelease},	// _
	};

	emuInstallStateTable(hhEmu, astfViewdataTable, DIM(astfViewdataTable));

	// Allocate and initialize private data for viewdata emulator.
	//
	if (hhEmu->pvPrivate != 0)
		{
		free(hhEmu->pvPrivate);
		hhEmu->pvPrivate = 0;
		}

	hhEmu->pvPrivate = malloc(sizeof(VIEWDATAPRIVATE));

	if (hhEmu->pvPrivate == 0)
		{
		assert(FALSE);
		return;
		}

	pstPRI = (PSTVIEWDATAPRIVATE)hhEmu->pvPrivate;
	pstPRI->aMapColors[0] = 4;
	pstPRI->aMapColors[1] = 2;
	pstPRI->aMapColors[2] = 6;
	pstPRI->aMapColors[3] = 1;
	pstPRI->aMapColors[4] = 5;
	pstPRI->aMapColors[5] = 3;
	pstPRI->aMapColors[6] = 15;

	/* --- Allocate attribute buffer for View Data junk --- */

	pstPRI->apstVD = malloc(MAX_EMUROWS * sizeof(PSTVIEWDATA));

	if (pstPRI->apstVD == 0)
		{
		assert(FALSE);
		return;
		}

	memset(pstPRI->apstVD, 0, MAX_EMUROWS * sizeof(PSTVIEWDATA));

	for (i = 0 ; i < MAX_EMUROWS ; ++i)
		{
		pstPRI->apstVD[i] = malloc(VIEWDATA_COLS_40MODE * sizeof(STVIEWDATA));

		if (pstPRI->apstVD[i] == 0)
			{
			assert(FALSE);
			return;
			}

		memset(pstPRI->apstVD[i], 0, sizeof(STVIEWDATA));
		}

	/* --- functions specific to prestel (viewdata) --- */

	hhEmu->emuResetTerminal = EmuViewdataReset;
	hhEmu->emu_deinstall = EmuViewdataDeinstall;

	hhEmu->emu_kbdin   = EmuViewdataKbd;
	hhEmu->emu_graphic = EmuViewdataCharDisplay;

	hhEmu->emu_highchar = ETEXT('\x7F');
	hhEmu->emu_maxcol = VIEWDATA_COLS_40MODE - 1;

	// Also, set font to Arial Alternative
	//
	memset(&lf, 0, sizeof(LOGFONT));
	hwndTerm = sessQueryHwndTerminal(hhEmu->hSession);
	termGetLogFont(hwndTerm, &lf);

	if (StrCharCmpi(lf.lfFaceName, "Arial Alternative") != 0)
		{
		StrCharCopy(lf.lfFaceName, "Arial Alternative");
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		termSetLogFont(hwndTerm, &lf);
		}

	EmuViewdataReset(hhEmu, FALSE);
	std_setcolors(hhEmu, VC_BRT_WHITE, VC_BLACK);

	// Turn backscroll off for Prestel
	//
	backscrlSetShowFlag(sessQueryBackscrlHdl(hhEmu->hSession), FALSE);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	EmuViewdataDeinstall
 *
 * DESCRIPTION:
 *	Frees up buffers allocated for view data junk.
 *
 * ARGUMENTS:
 *	fQuitting	- because other funcs have it.
 *
 * RETURNS:
 *	void
 *
 */
void EmuViewdataDeinstall(const HHEMU hhEmu)
	{
	int i;
	const PSTVIEWDATAPRIVATE pstPRI = (PSTVIEWDATAPRIVATE)hhEmu->pvPrivate;
	assert(hhEmu);

	if (pstPRI)
		{
		if (pstPRI->apstVD)
			{
			for (i = 0 ; i < 24 ; ++i)
				{
				if (pstPRI->apstVD[i])
					{
					free(pstPRI->apstVD[i]);
					pstPRI->apstVD[i] =NULL;
					}
				}

			free(pstPRI->apstVD);
			pstPRI->apstVD = NULL;
			}

		free(hhEmu->pvPrivate);
		hhEmu->pvPrivate = 0;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * EmuViewdataReset
 *
 * DESCRIPTION:	Sets the viewdata emulator to the proper conditions when
 *				starting up.
 *
 * ARGUMENTS:	ehdl -- emu handle
 *
 * RETURNS:		nothing
 */
/* ARGSUSED */
int EmuViewdataReset(const HHEMU hhEmu, int const fHost)
	{
	hhEmu->top_margin = 0;
	hhEmu->bottom_margin = MAX_ROWS-1;

	hhEmu->mode_KAM = hhEmu->mode_IRM = hhEmu->mode_VEM =
	hhEmu->mode_HEM = hhEmu->mode_DECCKM = hhEmu->mode_DECOM =
	hhEmu->mode_DECCOLM = hhEmu->mode_DECPFF = hhEmu->mode_DECPEX =
	hhEmu->mode_DECSCNM = hhEmu->mode_25enab =
	hhEmu->mode_protect = hhEmu->mode_block = hhEmu->mode_local = RESET;

	hhEmu->mode_SRM = hhEmu->mode_LNM = hhEmu->mode_DECTCEM = SET;

	hhEmu->mode_AWM = TRUE;

	emu_cleartabs(hhEmu, 3);
	return 0;
	}

#endif // INCL_VIEWDATA
/************************* end of viewdini.c **************************/

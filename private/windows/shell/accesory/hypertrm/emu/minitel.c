/*	File: D:\wacker\emu\minitel.c (Created: 05-Mar-1994)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:20p $
 */

#include <windows.h>
#pragma hdrstop
													
#include <time.h>

#include <tdll\stdtyp.h>
#include <tdll\tdll.h>
#include <tdll\session.h>
#include <tdll\cloop.h>
#include <tdll\print.h>
#include <tdll\capture.h>
#include <tdll\assert.h>
#include <tdll\mc.h>
#include <tdll\update.h>
#include <tdll\chars.h>
#include <tdll\cnct.h>
#include <tdll\term.h>
#include <tdll\backscrl.h>
#include <tdll\tchar.h>
#include <term\res.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"
#include "minitel.hh"
#include "keytbls.h"


#if defined(INCL_MINITEL)

static void emuMinitelRedisplayLine(const HHEMU hhEmu,
										const int row,
										const int col);

static int minitel_kbdin(const HHEMU hhEmu, int key, const int fTest);
static ECHAR minitelMapMosaics(const HHEMU hhEmu, ECHAR ch);
static void minitelFullScrnReveal(const HHEMU hhEmu);
static void minitelFullScrnConceal(const HHEMU hhEmu);
static void minitelSS2(const HHEMU hhEmu);
static void minitelSS2Part2(const HHEMU hhEmu);
static void minitelInsMode(const HHEMU hhEmu);
static void minitelPRO1(const HHEMU hhEmu);
static void minitelPRO2Part1(const HHEMU hhEmu);
static void minitelPRO2Part2(const HHEMU hhEmu);
static void minitelStatusReply(const HHEMU hhEmu);

/*
	Here begins the famed and fabled Minitel emulator.	Abandon all hope
	all ye who..., well you get the idea.  The Minitel emulator uses a
	combination of character attributes and field attributes.  See the
	book "minitel 1B" for description of field attributes.	Page numbers
	referenced in this code refer to the before mentioned book. - mrw
*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuMinitelInit
 *
 * DESCRIPTION:
 *	Startup routine for the minitel emulator.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void emuMinitelInit(const HHEMU hhEmu)
	{
	int i;
	LOGFONT lf;
	HWND hwndTerm;
	PSTMTPRIVATE pstPRI;

	static struct trans_entry const minitel_tbl[] =
		{
		{NEW_STATE, 0, 0, 0}, // 0
		{0, ETEXT('\x00'), ETEXT('\x01'), nothing},
		{0, ETEXT('\x20'), ETEXT('\x7F'), minitelGraphic},
		{1, ETEXT('\x1B'), ETEXT('\x1B'), nothing},
		{0, ETEXT('\x07'), ETEXT('\x07'), emu_bell},

		{0, ETEXT('\b'),   ETEXT('\b'), minitelBackspace},
		{0, ETEXT('\t'),   ETEXT('\t'),	minitelHorzTab},
		{0, ETEXT('\n'),   ETEXT('\n'),	minitelLinefeed},
		{0, ETEXT('\x0B'), ETEXT('\x0B'), minitelVerticalTab},
		{0, ETEXT('\x0C'), ETEXT('\x0C'), minitelFormFeed},
		{0, ETEXT('\r'),   ETEXT('\r'),	carriagereturn},
		{0, ETEXT('\x0E'), ETEXT('\x0F'), minitelCharSet},  // change char set
		{0, ETEXT('\x11'), ETEXT('\x11'), minitelCursorOn}, // cursor on
		{5, ETEXT('\x12'), ETEXT('\x12'), nothing},		  // repeat
		{12,ETEXT('\x13'), ETEXT('\x13'), nothing},		  // SEP
		{0, ETEXT('\x14'), ETEXT('\x14'), minitelCursorOff},// cursor off
		{20,ETEXT('\x16'), ETEXT('\x16'), nothing},		  // SS2 (undocumented)
		{0, ETEXT('\x18'), ETEXT('\x18'), minitelCancel},   // cancel
		{20,ETEXT('\x19'), ETEXT('\x19'), nothing}, 	  // SS2
		{0, ETEXT('\x1C'), ETEXT('\x1C'), nothing}, // really is nothing.
		{13,ETEXT('\x1D'), ETEXT('\x1D'), nothing}, // SS3,X ingnored, p99, 1.2.7
		{0, ETEXT('\x1E'), ETEXT('\x1E'), minitelRecordSeparator},
		{3, ETEXT('\x1F'), ETEXT('\x1F'), nothing}, // Unit Seperator
		//{0, ETEXT('\x7F'), ETEXT('\x7F'), minitelDel},

		{NEW_STATE, 0, 0, 0}, // 1 - seen ESC
		{1, ETEXT('\x00'), ETEXT('\x00'), nothing},
		{0, ETEXT('\x01'), ETEXT('\x1F'), minitelResync},
		{18,ETEXT('\x23'), ETEXT('\x23'), nothing},
		{14,ETEXT('\x25'), ETEXT('\x25'), nothing},
		{13,ETEXT('\x35'), ETEXT('\x37'), nothing},	// eat ESC,35-37,X sequences
		{6, ETEXT('\x39'), ETEXT('\x39'), nothing},	// PROT1, p134
		{7, ETEXT('\x3A'), ETEXT('\x3A'), nothing},	// PROT2, p134
		{8, ETEXT('\x3B'), ETEXT('\x3B'), nothing},	// PROT3, p134
		{2, ETEXT('\x5B'), ETEXT('\x5B'), ANSI_Pn_Clr},
		{0, ETEXT('\x40'), ETEXT('\x49'), emuMinitelCharAttr}, // forground color, flashing
		{0, ETEXT('\x4C'), ETEXT('\x4F'), emuMinitelCharAttr}, // char width & height
		{0, ETEXT('\x50'), ETEXT('\x5A'), emuMinitelFieldAttr},// background, underlining
		{0, ETEXT('\x5F'), ETEXT('\x5F'), emuMinitelFieldAttr},// reveal display
		{0, ETEXT('\x5C'), ETEXT('\x5D'), emuMinitelCharAttr}, // inverse
		{0, ETEXT('\x61'), ETEXT('\x61'), minitelCursorReport},
		{22,ETEXT('\x20'), ETEXT('\x2F'), nothing}, // p.99 ISO 2022

		{NEW_STATE, 0, 0, 0}, // 2 - seen ESC [
		{2, ETEXT('\x00'), ETEXT('\x00'), nothing},
		{2, ETEXT('\x30'), ETEXT('\x39'), ANSI_Pn},
		{2, ETEXT('\x3B'), ETEXT('\x3B'), ANSI_Pn_End},
		{2, ETEXT('\x3A'), ETEXT('\x3F'), ANSI_Pn},
		{0, ETEXT('\x40'), ETEXT('\x40'), minitelInsChars},
		{0, ETEXT('\x41'), ETEXT('\x41'), minitelCursorUp},
		{0, ETEXT('\x42'), ETEXT('\x42'), ANSI_CUD},
		{0, ETEXT('\x43'), ETEXT('\x43'), ANSI_CUF},
		{0, ETEXT('\x44'), ETEXT('\x44'), ANSI_CUB},
		{0, ETEXT('\x48'), ETEXT('\x48'), minitelCursorDirect},
		{0, ETEXT('\x4A'), ETEXT('\x4A'), minitelClrScrn},
		{0, ETEXT('\x4B'), ETEXT('\x4B'), minitelClrLn},
		{0, ETEXT('\x4C'), ETEXT('\x4C'), minitelInsRows},
		{0, ETEXT('\x4D'), ETEXT('\x4D'), minitelDelRows},
		{0, ETEXT('\x50'), ETEXT('\x50'), minitelDelChars},
		{0, ETEXT('\x68'), ETEXT('\x69'), minitelInsMode},
		{0, ETEXT('\x7A'), ETEXT('\x7B'), nothing}, //* p144 12.2
		{0, ETEXT('\x7D'), ETEXT('\x7D'), nothing}, //* p144 12.2
		{0, ETEXT('\x7F'), ETEXT('\x7F'), minitelResetTerminal}, //* p145, 13.2

		{NEW_STATE, 0, 0, 0}, // 3 - unit separtor character position
		{3, ETEXT('\x00'), ETEXT('\x00'), nothing},
		{4, ETEXT('\x01'), ETEXT('\xFF'), minitelUSRow},

		{NEW_STATE, 0, 0, 0}, // 4 - end of unit separtor character position
		{4, ETEXT('\x00'), ETEXT('\x00'), nothing},
		{0, ETEXT('\x01'), ETEXT('\xFF'), minitelUSCol},

		{NEW_STATE, 0, 0, 0}, // 5 - number of repeats
		{5, ETEXT('\x00'), ETEXT('\x00'), nothing},
		{0, ETEXT('\x40'), ETEXT('\x7F'), minitelRepeat},
		{0, ETEXT('\x00'), ETEXT('\xFF'), minitelResync},

		{NEW_STATE, 0, 0, 0}, // 6 - Protocol 1 sequence (PRO1,X)
		{6, ETEXT('\x00'), ETEXT('\x00'), nothing},
		{0, ETEXT('\x01'), ETEXT('\xFF'), minitelPRO1},

		{NEW_STATE, 0, 0, 0}, // 7 - Protocol 2 sequence (PRO2,X,Y)
		{7, ETEXT('\x00'), ETEXT('\x00'), nothing},
		{8, ETEXT('\x01'), ETEXT('\xFF'), minitelPRO2Part1},

		{NEW_STATE, 0, 0, 0}, // 8 - Protocol 2 sequence (PRO2,X,Y)
		{8, ETEXT('\x00'), ETEXT('\x00'), nothing},
		{0, ETEXT('\x01'), ETEXT('\xFF'), minitelPRO2Part2},

		{NEW_STATE, 0, 0, 0}, // 9 - Protocol 3 sequence (PRO3,X,Y,Z)
		{9, ETEXT('\x00'), ETEXT('\x00'), nothing},
		{10,ETEXT('\x01'), ETEXT('\xFF'), nothing},

		{NEW_STATE, 0, 0, 0}, // 10 - Protocol 3 sequence (PRO3,X,Y,Z)
		{10,ETEXT('\x00'), ETEXT('\x00'), nothing},
		{11,ETEXT('\x01'), ETEXT('\xFF'), nothing},

		{NEW_STATE, 0, 0, 0}, // 11 - Protocol 3 sequence (PRO3,X,Y,Z)
		{11,ETEXT('\x00'), ETEXT('\x00'), nothing},
		{0, ETEXT('\x01'), ETEXT('\xFF'), nothing},

		{NEW_STATE, 0, 0, 0}, // 12 - SEP
		{12,ETEXT('\x00'), ETEXT('\x00'), nothing},
		{0, ETEXT('\x01'), ETEXT('\xFF'), nothing},

		{NEW_STATE, 0, 0, 0}, // 13 - ESC,35-39,X sequences eaten, p99, 1.2.7
		{13,ETEXT('\x00'), ETEXT('\x00'), nothing},
		{0, ETEXT('\x01'), ETEXT('\xFF'), nothing},

		{NEW_STATE, 0, 0, 0}, // 14 - screen transparency mode
		{15,ETEXT('\x1B'), ETEXT('\x1B'), nothing},
		{14,ETEXT('\x00'), ETEXT('\xFF'), nothing},
		{23,ETEXT('\x20'), ETEXT('\x2F'), nothing}, // could be ISO 2022

		{NEW_STATE, 0, 0, 0}, // 15 - screen transparency mode continued, seen ESC
		{16,ETEXT('\x25'), ETEXT('\x25'), nothing},
		{17,ETEXT('\x2F'), ETEXT('\x2F'), nothing},
		{15,ETEXT('\x00'), ETEXT('\x00'), nothing},
		{14,ETEXT('\x00'), ETEXT('\xFF'), nothing},

		{NEW_STATE, 0, 0, 0}, // 16 - screen transparency mode continued, seen ESC \x25
		{0, ETEXT('\x40'), ETEXT('\x40'), nothing},
		{16,ETEXT('\x00'), ETEXT('\x00'), nothing},
		{14,ETEXT('\x00'), ETEXT('\xFF'), nothing},

		{NEW_STATE, 0, 0, 0}, // 17 - screen transparency mode continued, seen ESC \x2F
		{0, ETEXT('\x3F'), ETEXT('\x3F'), nothing},
		{17,ETEXT('\x00'), ETEXT('\x00'), nothing},
		{14,ETEXT('\x00'), ETEXT('\xFF'), nothing},

		{NEW_STATE, 0, 0, 0}, // 18 - Full screen reveal/hide, seen ESC \x23
		{18,ETEXT('\x00'), ETEXT('\x00'), nothing},
		{19,ETEXT('\x20'), ETEXT('\x20'), nothing},
		{23,ETEXT('\x20'), ETEXT('\x2F'), nothing}, // could be ISO 2022

		{NEW_STATE, 0, 0, 0}, // 19 - Full screen reveal/hide, seen ESC \x23 \x20
		{19,ETEXT('\x00'), ETEXT('\x00'), nothing},
		{0, ETEXT('\x58'), ETEXT('\x58'), minitelFullScrnConceal},
		{0, ETEXT('\x5F'), ETEXT('\x5F'), minitelFullScrnReveal},

		{NEW_STATE, 0, 0, 0}, // 20 - SS2
		{20,ETEXT('\x00'), ETEXT('\x00'), nothing},
		{0, ETEXT('\x00'), ETEXT('\x1F'), minitelResync},
		{21,ETEXT('\x20'), ETEXT('\x7F'), minitelSS2}, // valid SS2

		{NEW_STATE, 0, 0, 0}, // 21 - SS2 part 2
		{21,ETEXT('\x00'), ETEXT('\x00'), nothing},
		{0, ETEXT('\x20'), ETEXT('\x7F'), minitelSS2Part2}, // valid SS2

		{NEW_STATE, 0, 0, 0}, // 22 - p.99 ISO 2022
		{23,ETEXT('\x20'), ETEXT('\x2F'), nothing},
		{0, ETEXT('\x00'), ETEXT('\x1F'), minitelResync},

		{NEW_STATE, 0, 0, 0}, // 23 - p.99 ISO 2022
		{24,ETEXT('\x20'), ETEXT('\x2F'), nothing},
		{0, ETEXT('\x00'), ETEXT('\x1F'), minitelResync},

		{NEW_STATE, 0, 0, 0}, // 24 - p.99 ISO 2022
		{25, ETEXT('\x30'), ETEXT('\x3F'), nothing}, // page 107.
		{0,  ETEXT('\x30'), ETEXT('\x7E'), nothing}, // final character
		{0,  ETEXT('\x00'), ETEXT('\x1F'), minitelResync},

		{NEW_STATE, 0, 0, 0}, // 25 - p.99 ISO 2022
		{0,  ETEXT('\x0D'), ETEXT('\x0D'), nothing}, // page 107. eat CR
		{0,  ETEXT('\x00'), ETEXT('\x7F'), minitelResync},
		};

	if (hhEmu == 0)
		{
		assert(0);
		return;
		}

	emuInstallStateTable(hhEmu, minitel_tbl, DIM(minitel_tbl));

	// Allocate and initialize private data for Minitel emulator.
	//
	if (hhEmu->pvPrivate != 0)
		{
		free(hhEmu->pvPrivate);
		hhEmu->pvPrivate = 0;
		}

	hhEmu->pvPrivate = malloc(sizeof(MTPRIVATE));

	if (hhEmu->pvPrivate == 0)
		{
		assert(FALSE);
		return;
		}

	pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;
	memset(pstPRI, 0, sizeof(MTPRIVATE));

	pstPRI->minitel_last_char = ETEXT(' ');

	/* load key array */

	emuKeyTableLoad(hhEmu, Minitel_KeyTable,
					 sizeof(Minitel_KeyTable)/sizeof(KEYTBLSTORAGE),
					 &hhEmu->stEmuKeyTbl);

	/* --- Allocate attribute buffer for Minitel junk --- */

	pstPRI->apstMT = malloc(MAX_EMUROWS * sizeof(PSTMINITEL));

	if (pstPRI->apstMT == 0)
		{
		assert(FALSE);
		return;
		}

	memset(pstPRI->apstMT, 0, MAX_EMUROWS * sizeof(PSTMINITEL));

	for (i = 0 ; i < MAX_EMUROWS ; ++i)
		{
		pstPRI->apstMT[i] = malloc(MAX_EMUCOLS * sizeof(STMINITEL));

		if (pstPRI->apstMT[i] == 0)
			{
			assert(FALSE);
			return;
			}

		memset(pstPRI->apstMT[i], 0, MAX_EMUCOLS * sizeof(STMINITEL));
		}

	/* --- Setup defaults --- */

	hhEmu->emu_maxrow = 24; 		   // 25 line emulator
	hhEmu->emu_maxcol = 39; 		   // start in 40 column mode
	hhEmu->top_margin = 1;			   // access to row 0 is restricted.
	hhEmu->bottom_margin = hhEmu->emu_maxrow; // this has to equal emu_maxrow which changed

	hhEmu->emu_kbdin = minitel_kbdin;
	hhEmu->emu_graphic = minitelGraphic;
	hhEmu->emu_deinstall = emuMinitelDeinstall;
	hhEmu->emu_ntfy = minitelNtfy;
	hhEmu->emuHomeHostCursor = minitelHomeHostCursor;
#ifdef INCL_TERMINAL_SIZE_AND_COLORS
    hhEmu->emu_setscrsize	= emuMinitelSetScrSize;
#endif
	hhEmu->emu_highchar = (TCHAR)0xFF;

	if (hhEmu->emu_currow == 0)
		(*hhEmu->emu_setcurpos)(hhEmu, 1, hhEmu->emu_curcol - 1);

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

	// Backscrol not supported in minitel
	//
	backscrlSetUNumLines(sessQueryBackscrlHdl(hhEmu->hSession), 0);

	// Initialize colors for the Minitel.
	//
	std_setcolors(hhEmu, VC_BRT_WHITE, VC_BLACK);

	// Set terminal to power-up state
	//
	minitelResetTerminal(hhEmu);

	// Turn backscroll off for minitels
	//
	backscrlSetShowFlag(sessQueryBackscrlHdl(hhEmu->hSession), FALSE);

	// Enable Minitel toolbar buttons
	//
	PostMessage(sessQueryHwnd(hhEmu->hSession), WM_SESS_SHOW_SIDEBAR, 0, 0);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuMinitelDeinstall
 *
 * DESCRIPTION:
 *	Frees the extra attribute buffer needed to manage serial attributes.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void emuMinitelDeinstall(const HHEMU hhEmu)
	{
	int i;
	PSTMTPRIVATE pstPRI;

	assert(hhEmu);
	pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	if (pstPRI)
		{
		if (pstPRI->apstMT)
			{
			for (i = 0 ; i < MAX_EMUROWS ; ++i)
				{
				if (pstPRI->apstMT[i])
					{
					free(pstPRI->apstMT[i]);
					pstPRI->apstMT[i] = NULL;
					}
				}

			free(pstPRI->apstMT);
			pstPRI->apstMT = 0;
			}

		free(hhEmu->pvPrivate);
		hhEmu->pvPrivate = 0;
		}

	// Hide Minitel toolbar buttons
	//
	ShowWindow(sessQuerySidebarHwnd(hhEmu->hSession), SW_HIDE);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelReset
 *
 * DESCRIPTION:
 *	Sets emulator to an initial state.	Used for record and unit
 *	separators.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void minitelReset(const HHEMU hhEmu)
	{
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	pstPRI->minitelG1Active = FALSE;

	memset(&hhEmu->emu_charattr, 0, sizeof(hhEmu->emu_charattr));
	hhEmu->emu_charattr.txtclr = VC_BRT_WHITE;
	hhEmu->emu_charattr.bkclr = VC_BLACK;

	pstPRI->apstMT[hhEmu->emu_imgrow][hhEmu->emu_curcol].isattr = 0;

	memset(&pstPRI->stLatentAttr, 0, sizeof(pstPRI->stLatentAttr));
	pstPRI->stLatentAttr.fBkClr = TRUE;

	hhEmu->attrState[CS_STATE] =
		hhEmu->attrState[CSCLEAR_STATE] = hhEmu->emu_charattr;

	hhEmu->emu_clearattr = hhEmu->emu_charattr;
	minitelNtfy(hhEmu, 0);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelGraphic
 *
 * DESCRIPTION:
 *	Handles displayable characters.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void minitelGraphic(const HHEMU hhEmu)
	{
	ECHAR ccode;
	ECHAR aechBuf[10];
	int r;
	int row = hhEmu->emu_currow;
	int col = hhEmu->emu_curcol;
	BOOL fRedisplay = FALSE;
    BOOL fDblHi;
	STATTR stAttr;

	ECHAR *tp = hhEmu->emu_apText[hhEmu->emu_imgrow];
	const PSTATTR ap = hhEmu->emu_apAttr[hhEmu->emu_imgrow];
	const HUPDATE hUpdate = sessQueryUpdateHdl(hhEmu->hSession);
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	ccode = hhEmu->emu_code;

	if (ccode == 0)
		return;

	pstPRI->minitel_last_char = ccode;

	if (hhEmu->mode_IRM == SET)
		{
		if (col < hhEmu->emu_maxcol)
			{
			memmove(&tp[col+1], &tp[col],
						(unsigned)(hhEmu->emu_maxcol-col) * sizeof(ECHAR));

			memmove(&ap[col+1], &ap[col],
						(unsigned)(hhEmu->emu_maxcol-col) * sizeof(STATTR));

			memmove(&pstPRI->apstMT[hhEmu->emu_imgrow][col+1],
						&pstPRI->apstMT[hhEmu->emu_imgrow][col],
						(unsigned)(hhEmu->emu_maxcol-col) * sizeof(STMINITEL));
			}
		}

	/* --- check if we are overwriting an attribute space --- */

	if (pstPRI->apstMT[hhEmu->emu_imgrow][col].isattr)
		{
		pstPRI->apstMT[hhEmu->emu_imgrow][col].isattr = FALSE;
		fRedisplay = TRUE;
		}

	/* --- If we receive a space and have latent attributes, validate --- */

	if (ccode == ETEXT('\x20') && pstPRI->stLatentAttr.fModified
			&& pstPRI->minitelG1Active == FALSE)
		{
		r = hhEmu->emu_imgrow;

		// Color
		//
		pstPRI->apstMT[r][col].fbkclr = pstPRI->stLatentAttr.fBkClr;
		pstPRI->apstMT[r][col].bkclr = pstPRI->stLatentAttr.bkclr;

		// Conceal
		//
		pstPRI->apstMT[r][col].conceal = pstPRI->stLatentAttr.conceal;

		// Underline
		//
		pstPRI->apstMT[r][col].undrln = pstPRI->stLatentAttr.undrln;

		pstPRI->apstMT[hhEmu->emu_imgrow][col].isattr  = TRUE;

		// This is truely wierd.  We don't reset the fBkclr, fConceal, or
		// fUndrln fields, only the fModified flag.  Thus if any serial
		// attributes are set, all latent values get updated.  The only
		// guy who appears to be able to turn off an attribute is the
		// mosaic character which validates the background color and sets
		// the fBkClr flag to false.  I can't think of a reason why it
		// should work this way.  Pretty damn confusing if you ask me. - mrw
		//
		pstPRI->stLatentAttr.fModified = FALSE;
	    fRedisplay = TRUE;
		}

	/* --- If we switched to G1, map char to location in new font --- */

	if (pstPRI->minitelG1Active)
		ccode = minitelMapMosaics(hhEmu, ccode);

	pstPRI->apstMT[hhEmu->emu_imgrow][col].ismosaic =
		(unsigned)pstPRI->minitelG1Active;

	/* --- If we switched to semigraphic mode, validate the background --- */

	if (pstPRI->minitelG1Active)
		{
		// Guess what?	Latent background color is always adopted for mosaics.
		// This is a major undocumented find.  Basicly, mosaics
		// (semigraphics) always use the latent background color regardless
		// of the validation state.
		//
		ap[col].bkclr = pstPRI->stLatentAttr.bkclr;
		fRedisplay = TRUE;

		// Something tricky here.  Reception of a mosaic validates the
		// background color.  Validate means adopt the color.  It also
		// means that if we shift back to the alpha (G0 char set) and
		// recieve a space, we DON'T validate the background color a
		// second time.  So we keep a seperate flag for the background
		// color validation. - mrw
		//
		pstPRI->stLatentAttr.fBkClr = FALSE;
		}

	/* --- Normal character processing --- */

	tp[col] = ccode;

	// Update the end of row index if necessary.
	//
	if (col > hhEmu->emu_aiEnd[hhEmu->emu_imgrow])
		hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = col;

	/* --- Find out this characters current attributes and adjust --- */

	stAttr = GetAttr(hhEmu, row, col);
	ap[col] = hhEmu->emu_charattr;
	ap[col].bkclr = stAttr.bkclr;
	ap[col].blank = stAttr.blank;
	ap[col].undrln = stAttr.undrln;

	if (pstPRI->minitelG1Active)
		ap[col].symbol = 1;

	// Documented: 0x7F (mosaic or alpha) always maps to 0x5F in the G1
	// character set (solid block).
	//
	if (tp[col] == ETEXT('\x7F'))
		{
		tp[col] = ETEXT('\x5F');
		ap[col].symbol = 1;
		}

	/* --- Double high stuff --- */

	if (hhEmu->emu_charattr.dblhilo)
		{
		if (row >= 2)
			{
			r = row_index(hhEmu, row-1);

			hhEmu->emu_apText[r][col] = ccode;
			hhEmu->emu_apAttr[r][col] = ap[col];
			hhEmu->emu_apAttr[r][col].dblhilo = 0;
			hhEmu->emu_apAttr[r][col].dblhihi = 1;
			pstPRI->apstMT[r][col] = pstPRI->apstMT[hhEmu->emu_imgrow][col];
			updateChar(hUpdate, row-1, col, col);
			}
		}

	/* --- Double wide stuff --- */

	if (hhEmu->emu_charattr.dblwilf)
		{
		if (row > 0 && col < hhEmu->emu_maxcol)
			{
			tp[col+1] = ccode;
			ap[col+1] = ap[col];
			ap[col+1].dblwilf = 0;
			ap[col+1].dblwirt = 1;

			// Major league bug.
			//
			pstPRI->apstMT[hhEmu->emu_imgrow][col+1] =
				pstPRI->apstMT[hhEmu->emu_imgrow][col];
			}

		if (hhEmu->emu_charattr.dblhilo)
			{
			r = row_index(hhEmu, row-1);

			hhEmu->emu_apText[r][col+1] = ccode;
			hhEmu->emu_apAttr[r][col+1] = ap[col];
			hhEmu->emu_apAttr[r][col+1].dblwilf = 0;
			hhEmu->emu_apAttr[r][col+1].dblwirt = 1;
			hhEmu->emu_apAttr[r][col+1].dblhilo = 0;
			hhEmu->emu_apAttr[r][col+1].dblhihi = 1;
			pstPRI->apstMT[r][col+1] = pstPRI->apstMT[hhEmu->emu_imgrow][col];
			}
		}

	/* --- Need to use old row and column --- */

	if (fRedisplay)
		emuMinitelRedisplayLine(hhEmu, row, col);

	/* --- Need to bump the column guy an extra notch if double wide --- */

	if (hhEmu->emu_charattr.dblwilf)
		{
		col += 1;
		hhEmu->emu_aiEnd[hhEmu->emu_imgrow] = col;
		}

	updateChar(hUpdate, row, hhEmu->emu_curcol, col);

	/* --- bump column position, check for wrap, etc. --- */

	if (++col > hhEmu->emu_maxcol)
		{
		// Escape code 0x18 is referred to as cancel in the minitel doco.
		// It fills from the cursor pos to the end of the row with blanks.
		// Note: Also, it does not force wrap in anyway since
		// the cursor position is not updated.	That's why we have to
		// check here. - mrw:5/3/95
		//
		if (pstPRI->fInCancel || row == 0)
			{
			col = hhEmu->emu_maxcol;
			return;
			}

		printEchoString(hhEmu->hPrintEcho, tp,
			emuRowLen(hhEmu, hhEmu->emu_imgrow));

		CnvrtMBCStoECHAR(aechBuf, sizeof(aechBuf), TEXT("\r\n"),
			StrCharGetByteCount(TEXT("\r\n")));

		printEchoString(hhEmu->hPrintEcho, aechBuf, sizeof(ECHAR) * 2);

		CaptureLine(sessQueryCaptureFileHdl(hhEmu->hSession), CF_CAP_LINES,
			tp, emuRowLen(hhEmu, hhEmu->emu_imgrow));

        // Wrap around accounts for double high
		//
		fDblHi = (BOOL)hhEmu->emu_charattr.dblhilo;

		if (row == hhEmu->bottom_margin)
			{
			if (pstPRI->fScrollMode)
				minitel_scrollup(hhEmu, fDblHi ? 2 : 1);

			else
				row = fDblHi ? 2 : 1;
			}

		else if (row != 0)
			{
			row += fDblHi ? 2 : 1;

			if (row > hhEmu->emu_maxrow)
                row = 2;
			}

		col = 0;
		}

	// Finally, set the cursor position.  This will reset emu_currow
	// and emu_curcol.
	//
	(*hhEmu->emu_setcurpos)(hhEmu, row, col);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuMinitelRedisplayLine
 *
 * DESCRIPTION:
 *	The trick to field attributes is when you encounter one, you need to
 *	update the rest of the line that follows since changing or overwriting
 *	an attribute space affects stuff to the right.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *	int row - row to redisplay
 *	int col - start column
 *
 * RETURNS:
 *	void
 *
 */
static void emuMinitelRedisplayLine(const HHEMU hhEmu,
									const int row,
									const int col)
	{
	int i = row_index(hhEmu, row);
    int fDblHi = FALSE;
	const ECHAR *tp = hhEmu->emu_apText[i];
	const PSTATTR ap = hhEmu->emu_apAttr[i];
	const PSTATTR apl = hhEmu->emu_apAttr[row_index(hhEmu, row-1)];
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;
	const PSTMINITEL pstMT = pstPRI->apstMT[i];

	for (i = col ; i <= hhEmu->emu_maxcol ; ++i)
		{
		ap[i] = GetAttr(hhEmu, row, i);

		// Here's a wierd one.  Attribute spaces (as opposed to plain spaces)
		// validate but do not display the underline attribute.  I suspect
		// something similar with seperated mosaics. - mrw

		if (tp[i] == ETEXT('\x20') && pstMT[i].isattr)
			ap[i].undrln = 0;

        // If we're redisplaying a row that has a double hi character,
        // then we have to redisplay the upper-half as well.

        if (ap[i].dblhilo)
            {
            fDblHi = TRUE;
			apl[i] = GetAttr(hhEmu, row-1, i);
            }
		}

	if (fDblHi)
		{
		updateChar(sessQueryUpdateHdl(hhEmu->hSession),
						row-1, col, hhEmu->emu_maxcol);
		}

	updateChar(sessQueryUpdateHdl(hhEmu->hSession),
						row, col, hhEmu->emu_maxcol);

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
 *	hhEmu	- private emulator handle.
 *	iRow	- logical row
 *	iCol	- logical col
 *
 * RETURNS:
 *	composite attribute.
 *
 */
STATTR GetAttr(const HHEMU hhEmu, const int iRow, const int iCol)
	{
	int i;
	STATTR stAttr;
	const int r = row_index(hhEmu, iRow);
	const PSTATTR ap = hhEmu->emu_apAttr[r];
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;
	const PSTMINITEL pstMT = pstPRI->apstMT[r];

	stAttr = hhEmu->emu_apAttr[r][iCol];
	stAttr.bkclr = 0;
	stAttr.undrln = 0;
	stAttr.blank = 0;

	for (i = 0 ; i <= iCol ; ++i)
		{
        // Mosaics validate the background color.  Do it first however,
        // because an attribute space can change it to something else.

		if (pstMT[i].ismosaic)
			stAttr.bkclr = ap[i].bkclr;

		if (pstMT[i].isattr)
			{
			if (pstMT[i].fbkclr)
				stAttr.bkclr = pstMT[i].bkclr;

			stAttr.undrln = pstMT[i].undrln;
			stAttr.blank = pstMT[i].conceal;
			}

		// Mosaics always cancel underlining.
		//
		if (pstMT[i].ismosaic)
			stAttr.undrln = 0;
		}

	return stAttr;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuMinitelCharAttr
 *
 * DESCRIPTION:
 *	Modifies the current character attribute.  Does not affect field
 *	attributes.
 *
 *	mrw - 11/1/94: Went to high intensity colors to more closely match
 *	the minitel colors.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void emuMinitelCharAttr(const HHEMU hhEmu)
	{
	STATTR stAttr = hhEmu->attrState[CS_STATE];
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	switch (hhEmu->emu_code)
		{
	case ETEXT('\x40'):	 stAttr.txtclr = 0;  break;
	case ETEXT('\x41'):	 stAttr.txtclr = 12; /*4*/	break;
	case ETEXT('\x42'):	 stAttr.txtclr = 10; /*2*/	break;
	case ETEXT('\x43'):	 stAttr.txtclr = 14; /*6*/	break;
	case ETEXT('\x44'):	 stAttr.txtclr = 9;  /*1*/	break;
	case ETEXT('\x45'):	 stAttr.txtclr = 13; /*5*/	break;
	case ETEXT('\x46'):	 stAttr.txtclr = 11; /*3*/	break;
	case ETEXT('\x47'):	 stAttr.txtclr = 15; break;

	case ETEXT('\x48'):	 stAttr.blink  = 1;  break;
	case ETEXT('\x49'):	 stAttr.blink  = 0;  break;

	case ETEXT('\x4C'):			// normal size
		if (pstPRI->minitelG1Active)
			return;

		stAttr.dblhilo = 0;
		stAttr.dblwilf = 0;
		break;

	case ETEXT('\x4D'):			// double height
		if (pstPRI->minitelG1Active || hhEmu->emu_currow <= 1)
			return;

		stAttr.dblhilo= 1;
		stAttr.dblwilf = 0;
		break;

	case ETEXT('\x4E'):			// double width
		if (pstPRI->minitelG1Active || hhEmu->emu_currow < 1)
			return;

		stAttr.dblhilo = 0;
		stAttr.dblwilf	= 1;
		break;

	case ETEXT('\x4F'):			// double size
		if (pstPRI->minitelG1Active || hhEmu->emu_currow <= 1)
			return;

		stAttr.dblhilo = 1;
		stAttr.dblwilf	= 1;
		break;

	case ETEXT(ETEXT('\x5C')):    // normal polarity
		if (pstPRI->minitelG1Active)
			return;

		stAttr.revvid = 0;
		break;

	case ETEXT(ETEXT('\x5D')):    // reverse polarity
		if (pstPRI->minitelG1Active)
			return;

		stAttr.revvid = 1;
		break;

	default:
		break;
		}

	/* --- commit changes --- */

	hhEmu->emu_charattr =
	hhEmu->attrState[CS_STATE] =
	hhEmu->attrState[CSCLEAR_STATE] = stAttr;

	hhEmu->attrState[CSCLEAR_STATE].revvid = 0;
	hhEmu->attrState[CSCLEAR_STATE].undrln = 0;

	hhEmu->emu_clearattr = hhEmu->attrState[CSCLEAR_STATE];
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuMinitelFieldAttr
 *
 * DESCRIPTION:
 *	Dreaded field attributes.  Actually, this routine updates what is
 *	called a Latent Attribute.	The attributes only become effective
 *	when a space is recieved.  Who invents this stupid stuff anyways?
 *
 *	mrw - 11/1/94: Went to high intensity colors to more closely match
 *	the minitel colors.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void emuMinitelFieldAttr(const HHEMU hhEmu)
	{
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;
	LATENTATTR * const pstLA = &pstPRI->stLatentAttr;

	switch (hhEmu->emu_code)
		{
	case ETEXT('\x50'):	 pstLA->bkclr = 0;	 break;
	case ETEXT('\x51'):	 pstLA->bkclr = 12;  break;
	case ETEXT('\x52'):	 pstLA->bkclr = 10;  break;
	case ETEXT('\x53'):	 pstLA->bkclr = 14;  break;
	case ETEXT('\x54'):	 pstLA->bkclr = 9;	 break;
	case ETEXT('\x55'):	 pstLA->bkclr = 13;  break;
	case ETEXT('\x56'):	 pstLA->bkclr = 11;  break;
	case ETEXT('\x57'):	 pstLA->bkclr = 15;  break;

	case ETEXT('\x58'):	 pstLA->conceal=1;	 break;
	case ETEXT('\x5F'):	 pstLA->conceal=0;	 break;

	case ETEXT('\x59'):	// separated mosaics off
		if (pstPRI->minitelG1Active)
			{
			pstPRI->minitelUseSeparatedMosaics = 0;
			return;
			}

		else
			{
			pstPRI->stLatentAttr.undrln = 0;
			}
		break;

	case ETEXT('\x5A'):	// separated mosaics on
		if (pstPRI->minitelG1Active)
			{
			pstPRI->minitelUseSeparatedMosaics = 1;
			return;
			}

		else
			{
			pstPRI->stLatentAttr.undrln = 1;
			}
		break;

	default:	
	    return;
		}

	// Undocumented: setting any field attribute invalidates the
	// latent color context.
	//
	pstPRI->stLatentAttr.fBkClr = TRUE;

	pstPRI->stLatentAttr.fModified = TRUE;
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * minitel_kbdin
 *
 * DESCRIPTION:
 *	 Processes keys for the minitel emulator.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *	key 	- key to process
 *
 * RETURNS:
 *	 nothing
 */
static int minitel_kbdin(const HHEMU hhEmu, int key, const int fTest)
	{
	int index;
	TCHAR c;
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	if ((index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl)) != -1)
		{
		if (!fTest)
			{
			emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl);

			// This completes the code sent by emuSendKeyString(), page 124
			//
			if (key == (VK_RIGHT | VIRTUAL_KEY | SHIFT_KEY) ||
				key == (VK_RIGHT | VIRTUAL_KEY | SHIFT_KEY | EXTENDED_KEY))
				{
				c = (pstPRI->minitelSecondDep) ? TEXT('\x6c') : TEXT('\x68');

				CLoopCharOut(sessQueryCLoopHdl(hhEmu->hSession), c);
				}

			// Check for disconnect key.  If hit twice consecutively,
			// it posts a disconnect to the modem guy.
			//
			if (key == (VK_F9 | VIRTUAL_KEY))
				{
				pstPRI->F9 += 1;


				if (pstPRI->F9 == 2)
					{
					PostMessage(sessQueryHwnd(hhEmu->hSession),
						WM_DISCONNECT, 0, 0);
					}
				}
			}
		}

	else
		{
		// reset F9 counter, must have two consecutive to disconnect
		//
		pstPRI->F9 = 0;
		index = std_kbdin(hhEmu, key, fTest);
		}

	return index;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelCharSet
 *
 * DESCRIPTION:
 *	We don't really switch character sets here.  Since minitel uses
 *	7 bit ASCII, we can user the half of the ASCII table for the
 *	contigous and seperated mosaics that comprise the G1 char set.
 *	Other rule here is that switching the the G1 character set
 *	validates the background color on reception of the first mosaic.
 *	One thing I haven't figured out yet is when a field attribute is
 *	validated, does the latent attribute get reset or retain the last color?
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void minitelCharSet(const HHEMU hhEmu)
	{
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	pstPRI->stLatentAttr.undrln = 0;

	if (hhEmu->emu_code == ETEXT(0x0E)) // switch to G1
		{
		pstPRI->minitelG1Active = TRUE;
		pstPRI->minitelUseSeparatedMosaics = FALSE;
		}

	else
		{
		pstPRI->minitelG1Active = FALSE;
		}

	// underline, size and polarity attributes permanently canceled.
	//
	hhEmu->emu_charattr.undrln = 0;
	hhEmu->emu_charattr.dblhihi = 0;
	hhEmu->emu_charattr.dblhilo = 0;
	hhEmu->emu_charattr.dblwilf = 0;
	hhEmu->emu_charattr.dblwirt = 0;
	hhEmu->emu_charattr.revvid = 0;
	hhEmu->emu_charattr.symbol = 0;

	hhEmu->attrState[CS_STATE] =
		hhEmu->attrState[CSCLEAR_STATE] = hhEmu->emu_charattr;

	hhEmu->emu_clearattr = hhEmu->emu_charattr;

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelResetTerminal
 *
 * DESCRIPTION:
 *	Response to a 1B 39 7F sequence.  Puts terminal in power-up state.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void minitelResetTerminal(const HHEMU hhEmu)
	{
	minitelClearScreen(hhEmu, 0);  // clear screen
	(*hhEmu->emu_setcurpos)(hhEmu, 0, 0);

	minitelClearLine(hhEmu, 0);    // clear line 0
	(*hhEmu->emu_setcurpos)(hhEmu, 1, 0);

	minitelReset(hhEmu);		 // reset attributes

	((PSTMTPRIVATE)hhEmu->pvPrivate)->fScrollMode = 0;
	EmuStdSetCursorType(hhEmu, EMU_CURSOR_NONE);
	minitelNtfy(hhEmu, 0);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelCursorOn
 *
 * DESCRIPTION:
 *	Turns minitel cursor on.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void minitelCursorOn(const HHEMU hhEmu)
	{
	EmuStdSetCursorType(hhEmu, EMU_CURSOR_BLOCK);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelCursorOff
 *
 * DESCRIPTION:
 *	Turns minitel cursor off
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void minitelCursorOff(const HHEMU hhEmu)
	{
	#if defined(NDEBUG)
	EmuStdSetCursorType(hhEmu, EMU_CURSOR_NONE);
	#else
	EmuStdSetCursorType(hhEmu, EMU_CURSOR_BLOCK);
	#endif

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	mintelMapMosaics
 *
 * DESCRIPTION:
 *	Maps regular character to a mosaic char for our font only.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *	ch	- character to convert
 *
 * RETURNS:
 *	converted or original character.
 *
 */
static ECHAR minitelMapMosaics(const HHEMU hhEmu, ECHAR ch)
	{
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	if (ch >= ETEXT('\x21') && ch <= ETEXT('\x3F'))
		ch += ETEXT('\x1F');

	// Another weird undocumented affect.  Columns 4 and 5 (except 5F)
	// map to columns 6 and 7 (page 101)
    //
	else if (ch >= ETEXT('\x40') && ch <= ETEXT('\x5E'))
		ch += ETEXT('\x20');

	else if (ch >= ETEXT('\x60') && ch <= ETEXT('\x7E'))
		ch += 0;

	if (pstPRI->minitelUseSeparatedMosaics && ch >= ETEXT('\x21') &&
		ch <= ETEXT('\x7F'))
		{
		ch += ETEXT('\x40');
		}

	return ch;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelCursorReport
 *
 * DESCRIPTION:
 *	Return the current cursor location as US row col.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void minitelCursorReport(const HHEMU hhEmu)
	{
	TCHAR ach[40];
	ECHAR aech[40];

	wsprintf(ach, TEXT("US%c%c"), hhEmu->emu_currow, hhEmu->emu_curcol);

	CnvrtMBCStoECHAR(aech, sizeof(aech), ach,
        StrCharGetByteCount(ach) + sizeof(TCHAR));

	emuSendString(hhEmu, aech, StrCharGetEcharByteCount(aech));
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelFullScrnConceal
 *
 * DESCRIPTION:
 *	Just sets the blank bits on all the attributes.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
static void minitelFullScrnConceal(const HHEMU hhEmu)
	{
	int i, j;
	PSTATTR ap;

	for (i = 1 ; i <= hhEmu->emu_maxrow ; ++i)
		{
		ap = hhEmu->emu_apAttr[row_index(hhEmu, i)];

		for (j = 0 ; j <= hhEmu->emu_maxcol ; ++j, ++ap)
			ap->blank = 1;

		updateLine(sessQueryUpdateHdl(hhEmu->hSession),
										1, hhEmu->emu_maxrow);
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelFullScrnReveal
 *
 * DESCRIPTION:
 *	Just sets the blank bits on all the attributes.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
static void minitelFullScrnReveal(const HHEMU hhEmu)
	{
	int i, j;
	PSTATTR ap;

	for (i = 1 ; i <= hhEmu->emu_maxrow ; ++i)
		{
		ap = hhEmu->emu_apAttr[row_index(hhEmu, i)];

		for (j = 0 ; j <= hhEmu->emu_maxcol ; ++j, ++ap)
			ap->blank = 0;
		}

	updateLine(sessQueryUpdateHdl(hhEmu->hSession), 1, hhEmu->emu_maxrow);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelResync
 *
 * DESCRIPTION:
 *	Certain codes will cause the emulator to resynchronize, (goto state
 *	zero) and play the character thru.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void minitelResync(const HHEMU hhEmu)
	{
	hhEmu->state = 0;
#if defined(EXTENDED_FEATURES)
	(void)(*hhEmu->emu_datain)(hhEmu, hhEmu->emu_code);
#else
	(void)(*hhEmu->emu_datain)((HEMU)hhEmu, hhEmu->emu_code);
#endif
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelSS2
 *
 * DESCRIPTION:
 *	SS2 is an alternate character set.	It only has about 15 symbols so
 *	we just map them here to ones in our current minitel font.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
static void minitelSS2(const HHEMU hhEmu)
	{
	const int row = hhEmu->emu_currow;
	const int col = hhEmu->emu_curcol;
	BOOL  fNoAdvance = FALSE;
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	/* --- SS2 codes are ignored in semigraphic mode --- */

	if (pstPRI->minitelG1Active)
		{
		hhEmu->state = 0;
		return;
		}

	/* --- Map the character --- */

	switch (hhEmu->emu_code)
		{
	case ETEXT('\x23'):	hhEmu->emu_code = ETEXT('\xA3');  break; // british pound
	case ETEXT('\x24'):	hhEmu->emu_code = ETEXT('\x24');  break; // Dollar sign
	case ETEXT('\x26'):	hhEmu->emu_code = ETEXT('\x23');  break; // pound sign
	case ETEXT('\x27'):	hhEmu->emu_code = ETEXT('\xA7');  break; // integral
	case ETEXT('\x2C'):	hhEmu->emu_code = ETEXT('\xC3');  break; // left arrow
	case ETEXT('\x2D'):	hhEmu->emu_code = ETEXT('\xC0');  break; // up arrow
	case ETEXT('\x2E'):	hhEmu->emu_code = ETEXT('\xC4');  break; // right arrow
	case ETEXT('\x2F'):	hhEmu->emu_code = ETEXT('\xC5');  break; // down arrow
	case ETEXT('\x30'):	hhEmu->emu_code = ETEXT('\xB0');  break; // degree
	case ETEXT('\x31'):	hhEmu->emu_code = ETEXT('\xB1');  break; // plus-minus
	case ETEXT('\x38'):	hhEmu->emu_code = ETEXT('\xF7');  break; // divide
	case ETEXT('\x3C'):	hhEmu->emu_code = ETEXT('\xBC');  break; // 1/4
	case ETEXT('\x3D'):	hhEmu->emu_code = ETEXT('\xBD');  break; // 1/2
	case ETEXT('\x3E'):	hhEmu->emu_code = ETEXT('\xBE');  break; // 3/4
	case ETEXT('\x41'):	hhEmu->emu_code = ETEXT('\x60');  fNoAdvance = TRUE;  break;
	case ETEXT('\x42'):	hhEmu->emu_code = ETEXT('\xB4');  fNoAdvance = TRUE;  break;
	case ETEXT('\x43'):	hhEmu->emu_code = ETEXT('\x5E');  fNoAdvance = TRUE;  break;
	case ETEXT('\x48'):	hhEmu->emu_code = ETEXT('\xA8');  fNoAdvance = TRUE;  break;
	case ETEXT('\x4A'): hhEmu->emu_code = ETEXT('\xB8');  fNoAdvance = TRUE;  break;
	case ETEXT('\x4B'): hhEmu->emu_code = ETEXT('\xB8');  fNoAdvance = TRUE;  break;
	case ETEXT('\x6A'):	hhEmu->emu_code = ETEXT('\x8C');  break;
	case ETEXT('\x7A'):	hhEmu->emu_code = ETEXT('\x9C');  break;
	case ETEXT('\x7B'):	hhEmu->emu_code = ETEXT('\xDF');  break;
	default: hhEmu->emu_code = ETEXT('\x5F');			 break;
		}

	minitelGraphic(hhEmu);

	if (fNoAdvance)
		{
		(*hhEmu->emu_setcurpos)(hhEmu, row, col); // don't advance cursor
		}

	else
		{
		// If we don't advance the cursor, we're done with this SS2
		// sequence and reset the state to 0 - mrw, 2/3/95
		//
		hhEmu->state = 0;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelSS2Part2
 *
 * DESCRIPTION:
 *	The second half a an SS2 code is the vowel portion for the accents
 *	page 90.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
static void minitelSS2Part2(const HHEMU hhEmu)
	{
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	switch (hhEmu->emu_code)
		{
	case ETEXT('a'):
		switch (pstPRI->minitel_last_char)
			{
		case ETEXT('\x60'):	hhEmu->emu_code = ETEXT('\xE0');    break;
		case ETEXT('\x5E'):	hhEmu->emu_code = ETEXT('\xE2');    break;
		case ETEXT('\xA8'):	hhEmu->emu_code = ETEXT('\xE4');    break;
		default: break;
			}
		break;

	case ETEXT('e'):
		switch (pstPRI->minitel_last_char)
			{
		case ETEXT('\x60'):	hhEmu->emu_code = ETEXT('\xE8');    break;
		case ETEXT('\xB4'):	hhEmu->emu_code = ETEXT('\xE9');    break;
		case ETEXT('\x5E'):	hhEmu->emu_code = ETEXT('\xEA');    break;
		case ETEXT('\xA8'):	hhEmu->emu_code = ETEXT('\xEB');    break;
		default: break;
			}
		break;

	case ETEXT('i'):
		switch (pstPRI->minitel_last_char)
			{
		case ETEXT('\x5E'):	hhEmu->emu_code = ETEXT('\xEE');    break;
		case ETEXT('\xA8'):	hhEmu->emu_code = ETEXT('\xEF');    break;
		default: break;
			}
		break;

	case ETEXT('o'):
		switch (pstPRI->minitel_last_char)
			{
		case ETEXT('\x5E'):	hhEmu->emu_code = ETEXT('\xF4');    break;
		case ETEXT('\xA8'):	hhEmu->emu_code = ETEXT('\xF6');    break;
		default: break;
			}
		break;

	case ETEXT('u'):
		switch (pstPRI->minitel_last_char)
			{
		case ETEXT('\x60'):	hhEmu->emu_code = ETEXT('\xF9');    break;
		case ETEXT('\x5E'):	hhEmu->emu_code = ETEXT('\xFB');    break;
		case ETEXT('\xA8'):	hhEmu->emu_code = ETEXT('\xFC');    break;
		default: break;
			}
		break;

	case ETEXT('c'):
		switch (pstPRI->minitel_last_char)
			{
		case ETEXT('\xB8'): hhEmu->emu_code = ETEXT('\xE7');	break;
		default: break;
			}
		break;

	default:
		// Docs say if we're not one of the above chars, then overwrite
		// position with current char.
		break;
		}

	minitelGraphic(hhEmu);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelInsMode
 *
 * DESCRIPTION:
 *	Sets or Resets the insert mode depending on received code.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
static void minitelInsMode(const HHEMU hhEmu)
	{
	hhEmu->mode_IRM = (hhEmu->emu_code == ETEXT('\x68')) ? SET : RESET;
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelNtfy
 *
 * DESCRIPTION:
 *	Paints an inverted F or C when connection/disconnection notifications
 *	come in.
 *
 * ARGUMENTS:
 *	hhEmuPass	- change this to hhEmu when reentrancy done.
 *	nNtfyCode	- why it was called, (we don't use)
 *
 * RETURNS:
 *	void
 *
 */
void minitelNtfy(const HHEMU hhEmu, const int nNtfyCode)
	{
	const int r = row_index(hhEmu, 0);
	const int c = hhEmu->emu_maxcol - 1;
	ECHAR chr;
	BOOL  fFlash = FALSE;

	switch (cnctQueryStatus(sessQueryCnctHdl(hhEmu->hSession)))
		{
	case CNCT_STATUS_FALSE:
	default:
		chr = ETEXT('F');
		break;

	case CNCT_STATUS_TRUE:
		chr = ETEXT('C');
		break;

	case CNCT_STATUS_CONNECTING:
		chr = ETEXT('C');
		fFlash = TRUE;
		break;
		}

	hhEmu->emu_apText[r][c] = chr;
	hhEmu->emu_apAttr[r][c].revvid = 1;
	hhEmu->emu_apAttr[r][c].blink = (unsigned)fFlash;
	hhEmu->emu_apAttr[r][c].symbol = 0; // mrw-5/5/95

	hhEmu->emu_aiEnd[r] = c;

	updateChar(sessQueryUpdateHdl(hhEmu->hSession), 0, c, c);
	NotifyClient(hhEmu->hSession, EVENT_TERM_UPDATE, 0);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelPRO1
 *
 * DESCRIPTION:
 *	Handles PRO1 sequences (ESC,39,X).
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle
 *
 * RETURNS:
 *	void
 *
 */
static void minitelPRO1(const HHEMU hhEmu)
	{
	ECHAR aechBuf[35];
	static const TCHAR achID[] = "\x01\x43r0\x04";

	switch (hhEmu->emu_code)
		{
	case ETEXT('\x7B'): // ENQROM (page 139)
		// See pages 21 & 22.  Basicly we send back an indentification
		// sequence delimited by SOH and EOT
		//
		CnvrtMBCStoECHAR(aechBuf, sizeof(aechBuf), achID,
            StrCharGetByteCount(achID));

		CLoopSend(sessQueryCLoopHdl(hhEmu->hSession), aechBuf, 5, 0);
		break;

	case ETEXT('\x67'): // Disconnect (page 139)
		PostMessage(sessQueryHwnd(hhEmu->hSession), WM_DISCONNECT, 0, 0);
		break;

	case ETEXT('\x72'):
		minitelStatusReply(hhEmu);
		break;

	default:
		break;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelPRO2Part1
 *
 * DESCRIPTION:
 *	Handles first half of a PRO2 sequence.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
static void minitelPRO2Part1(const HHEMU hhEmu)
	{
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	pstPRI->minitel_PRO1 = hhEmu->emu_code;
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelPRO2Part2
 *
 * DESCRIPTION:
 *	Handles the second half of a PRO2 sequence.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle
 *
 * RETURNS:
 *	void
 *
 */
static void minitelPRO2Part2(const HHEMU hhEmu)
	{
	int fUpperCase;
	BYTE abKey[256];
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	switch (hhEmu->emu_code)
		{
	case ETEXT('\x43'):	  // scrolling
		if (pstPRI->minitel_PRO1 == ETEXT('\x69'))
			pstPRI->fScrollMode = TRUE;

		if (pstPRI->minitel_PRO1 == ETEXT('\x6A'))
			pstPRI->fScrollMode = FALSE;

		minitelStatusReply(hhEmu);
		break;

	case ETEXT('\x44'):	  // error correction procedure (not implemented)
		break;

	case ETEXT('\x45'):	  // keyboard upper/lower case
		if (pstPRI->minitel_PRO1 == ETEXT('\x69'))
			fUpperCase = FALSE;

		else if (pstPRI->minitel_PRO1 == ETEXT('\x6A'))
			fUpperCase = TRUE;

		else
			break;

		GetKeyboardState(abKey);

		if (fUpperCase)
			abKey[VK_CAPITAL] |= 0x01;

		else
			abKey[VK_CAPITAL] &= 0xfe;


		SetKeyboardState(abKey);

		minitelStatusReply(hhEmu);
		break;

	default:
		return;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	minitelStatusReply
 *
 * DESCRIPTION:
 *	Acknowledgement sequence for some PRO2 sequences and status requests.
 *
 * ARGUMENTS:
 *	hhEmu	- private emulator handle.
 *
 * RETURNS:
 *	void
 *
 * AUTHOR: Mike Ward, 08-May-1995
 */
static void minitelStatusReply(const HHEMU hhEmu)
	{
	ECHAR ach[10];
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	// The PRO2 sequences \x43 and \x45 all return and acknowledgement
	// of the form PRO2,\x73,status byte.  The format of the status byte
	// is defined in page 143, section 11.2.
	//
	// strcpy(ach, "\x1b\x3A\x73");
	CnvrtMBCStoECHAR(ach, sizeof(ach), "\x1b\x3A\x73",
					(unsigned long)StrCharGetByteCount("\x1b\x3A\x73"));

	ach[3] = ETEXT('\x40');  // bit 7 is always 1.
	ach[3] |= pstPRI->fScrollMode ? ETEXT('\x02') : ETEXT('\x00');
	ach[3] |=(GetKeyState(VK_CAPITAL) > 0) ? ETEXT('\x00') : ETEXT('\x08');
	ach[4] = ETEXT('\0');
	CLoopSend(sessQueryCLoopHdl(hhEmu->hSession), ach, 4, 0);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuMinitelSendKey
 *
 * DESCRIPTION:
 *	Used by the toolbar to emit the correct minitel sequence for the
 *	specified button.
 *
 * ARGUMENTS:
 *	hEmu	- public emulator handle.
 *	iCmd	- command string to send.
 *
 * RETURNS:
 *	void
 *
 * AUTHOR: Mike Ward, 10-Mar-1995
 */
void emuMinitelSendKey(const HEMU hEmu, const int iCmd)
	{
	TCHAR *pach;
	ECHAR aechBuf[20];
	const HHEMU hhEmu = (HHEMU)hEmu;
	const PSTMTPRIVATE pstPRI = (PSTMTPRIVATE)hhEmu->pvPrivate;

	switch (iCmd)
		{
	case IDM_MINITEL_INDEX: 	pach = TEXT("\x13") TEXT("F");	   break;
	case IDM_MINITEL_CANCEL:	pach = TEXT("\x13") TEXT("E");	   break;
	case IDM_MINITEL_PREVIOUS:	pach = TEXT("\x13") TEXT("B");	   break;
	case IDM_MINITEL_REPEAT:	pach = TEXT("\x13") TEXT("C");	   break;
	case IDM_MINITEL_GUIDE: 	pach = TEXT("\x13") TEXT("D");	   break;
	case IDM_MINITEL_CORRECT:	pach = TEXT("\x13") TEXT("G");	   break;
	case IDM_MINITEL_NEXT:		pach = TEXT("\x13") TEXT("H");	   break;
	case IDM_MINITEL_SEND:		pach = TEXT("\x13") TEXT("A");	   break;
	case IDM_MINITEL_CONFIN:
		pach = TEXT("\x13") TEXT("I");
		pstPRI->F9 += 1;

		if (pstPRI->F9 == 2)
			PostMessage(sessQueryHwnd(hhEmu->hSession), WM_DISCONNECT, 0, 0);

		break;

	default:
		assert(0);
		return;
		}

	CnvrtMBCStoECHAR(aechBuf, sizeof(aechBuf), pach,
		StrCharGetByteCount(pach) + sizeof(TCHAR));

	emuSendString((HHEMU)hEmu, aechBuf, StrCharGetEcharByteCount(aechBuf));
	return;
	}											

#ifdef INCL_TERMINAL_SIZE_AND_COLORS
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  emuMinitelSetScrSize
 *
 * DESCRIPTION:
 *  Replaces std_setscrsize which was added to allow user settable screen
 *	sizes. However, the Minitel doesn't allow this.
 *
 * ARGUMENTS:
 *  hhEmu - The internal emulator handle.
 *
 * RETURNS:
 *  void
 *
 * AUTHOR:	Bob Everett - 1 Sep 1998
 */
void emuMinitelSetScrSize(const HHEMU hhEmu)
    {
    }
#endif

#endif // INCL_MINITEL

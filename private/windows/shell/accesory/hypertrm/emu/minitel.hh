/*	File: D:\wacker\emu\minitel.hh (Created: 05-Mar-1994)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:28p $
 */

void emuMinitelInit(const HHEMU hhEmu);
void emuMinitelDeinstall(const HHEMU hhEmu);
void minitelGraphic(const HHEMU hhEmu);
void minitelLinefeed(const HHEMU hhEmu);
void minitelBackspace(const HHEMU hhEmu);
void minitelVerticalTab(const HHEMU hhEmu);
void minitelCursorUp(const HHEMU hhEmu);
void minitelCursorDirect(const HHEMU hhEmu);
void minitelFormFeed(const HHEMU hhEmu);
void minitelClearScreen(const HHEMU hhEmu, const int iHow);
void minitelClrScrn(const HHEMU hhEmu);
void minitelRecordSeparator(const HHEMU hhEmu);
void minitelClearLine(const HHEMU hhEmu, const int iHow);
void minitelClrLn(const HHEMU hhEmu);
void minitelUSCol(const HHEMU hhEmu);
void minitelUSRow(const HHEMU hhEmu);
void minitelReset(const HHEMU hhEmu);
void minitelHorzTab(const HHEMU hhEmu);
void minitelDel(const HHEMU hhEmu);
void minitelRepeat(const HHEMU hhEmu);
void minitelCharSet(const HHEMU hhEmu);
void minitelCharSize(const HHEMU hhEmu);
void minitelCancel(const HHEMU hhEmu);
void minitelResetTerminal(const HHEMU hhEmu);
void minitelCursorOn(const HHEMU hhEmu);
void minitelCursorOff(const HHEMU hhEmu);
void minitelResync(const HHEMU hhEmu);
void minitelDelChars(const HHEMU hhEmu);
void minitelInsChars(const HHEMU hhEmu);
void minitelDelRows(const HHEMU hhEmu);
void minitelInsRows(const HHEMU hhEmu);
void minitelNtfy(const HHEMU hhEmu, const int nNtfyCode);
void minitelCursorReport(const HHEMU hhEmu);
void emuMinitelCharAttr(const HHEMU hhEmu);
void emuMinitelFieldAttr(const HHEMU hhEmu);
int  minitelHomeHostCursor(const HHEMU hhEmu);
void minitel_scrollup(const HHEMU hhEmu, int nlines);
void minitel_scrolldown(const HHEMU hhEmu, int nlines);
void emuMinitelSetScrSize(const HHEMU hhEmu);


STATTR GetAttr(const HHEMU hhEmu, const int iRow, const int iCol);

/* --- minitel latent attribute structure --- */

typedef struct _minitel
	{
	unsigned int bkclr	 : 4;  // background color
	unsigned int conceal : 1;  // conceal
	unsigned int undrln  : 1;  // underline
	unsigned int isattr  : 1;  // true if this is an attribute space
	unsigned int ismosaic: 1;  // TRUE if char is mosaic
	unsigned int fbkclr  : 1;  // This attribute spaces validates color
	} STMINITEL;

// The latent attribute

typedef struct _latent
	{
	unsigned bkclr; 	    // background color
	unsigned conceal;	    // conceal chars
	unsigned undrln;	    // underline
	unsigned fModified; 	// latent attribute changed.
	unsigned fBkClr;		// TRUE if modified and not validated.
	} LATENTATTR;

typedef STMINITEL *PSTMINITEL;

// Private emulator data for Minitel.
//
typedef struct stPrivateMinitel
	{
	// Latent attribute and attribute array used for serial attributes.
	//
	LATENTATTR	stLatentAttr;
	LATENTATTR	saved_stLatentAttr;
	PSTMINITEL	*apstMT;
	ECHAR		minitel_last_char;
	int 		minitelG1Active;
	int 		minitelUseSeparatedMosaics;
	int 		saved_minitelUseSeparatedMosaics;
	int 		minitel_saved_row;
	int 		minitel_saved_col;
	STATTR		minitel_saved_attr;
	int 		minitel_saved_minitelG1Active;
	ECHAR		minitel_PRO1; // first part of a PRO sequence
	int 		us_row_code;  // used for US codes
	int 		minitelSecondDep;
	int 		fScrollMode;
	int 		F9; 		  // two consecutive F9's disconnect.
	int 		fInCancel;	  // used in minitelCancel and minitelGraphic
	} MTPRIVATE;

typedef MTPRIVATE *PSTMTPRIVATE;

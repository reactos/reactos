/*	File: D:\WACKER\tdll\term.h (Created: 29-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:35p $
 */

#if !defined(INCL_EXT_TERM)
#define INCL_EXT_TERM

/* --- Class name needed in ProcessMessage() --- */

#define TERM_CLASS		"Term Class"

// Some conventions:
//	Q = Query
//	S = Set

#define WM_TERM_GETUPDATE		WM_USER+0x100	// read update records
												// wPar=0, lPar=0
#define WM_TERM_BEZEL			WM_USER+0x101	// toggle bezel on/off
												// wPar=0, lPar=0
#define WM_TERM_Q_BEZEL 		WM_USER+0x102	// query bezel status (on/off)
												// wPar=0, lPar=0
#define WM_TERM_Q_SNAP			WM_USER+0x103	// calculate snapped size
												// wPar=0, lPar=LPRECT
#define WM_TERM_KEY 			WM_USER+0x104	// terminal key pressed
												// wPar=key, lPar=0
#define WM_TERM_CLRATTR 		WM_USER+0x105	// Emulator's clear attr changed
												// wPar=0, lPar=0
#define WM_TERM_GETLOGFONT		WM_USER+0x106	// Query terminal's logfont
												// wPar=0, lPar=&lf
#define WM_TERM_SETLOGFONT		WM_USER+0x107	// Set terminal's logfont
												// wPar=0, lPar=&lf
#define WM_TERM_Q_MARKED		WM_USER+0x108	// Is text marked
												// wPar=0, lPar=0
#define WM_TERM_UNMARK			WM_USER+0x109	// Unmarks any text
												// wPar=0, lPar=0
#define WM_TERM_TRACK			WM_USER+0x10A	// shift terminal to show cursor
												// wPar=0, lPar=0
#define WM_TERM_EMU_SETTINGS	WM_USER+0x10B	// emulator settings have changed
												// wPar=0, lPar=0
#define WM_TERM_Q_MARKED_RANGE 	WM_USER+0x10C	// query marked text range
												// wPar=PPOINT, lPar=PPOINT
#define WM_TERM_LOAD_SETTINGS 	WM_USER+0x10D	// Read terminal settings
												// wPar=0, lPar=0
#define WM_TERM_SAVE_SETTINGS	WM_USER+0x10E	// Save terminal settings
                                                // wPar=0, lPar=0
#define WM_TERM_MARK_ALL	    WM_USER+0x10F	// Mark all terminal text
												// wPar=0, lPar=0
#define WM_TERM_FORCE_WMSIZE	WM_USER+0x110	// Calls wm_size code
												// wPar=0, lPar=0
#define WM_TERM_CLEAR_BACKSCROLL WM_USER+0x111  // Clear backscroll area

#define WM_TERM_CLEAR_SCREEN    WM_USER+0x112   // Clear terminal screen

// Button 1 Double-click settings.

#define B1_SELECTWORD		0	// Selects a word
#define B1_COPYWORD 		1	// Copies word or selected text to host
#define B1_COPYWORDENTER	2	// Same but adds <ENTER> at end

// Button 2 click settings.

#define B2_CONTEXTMENU		0	// Popup context menu
#define B2_HOSTCURSOR		1	// Positions host cursor
#define B2_SINGLELETTER 	2	// Copies single letter to host
#define B2_DONOTHING		3	// disables button 2 clicks.

/* --- Color table lives in terminal files --- */

int GetNearestColorIndex(COLORREF cr);
void RefreshTermWindow(const HWND hwndTerm);

int termSetLogFont(const HWND hwndTerm, LPLOGFONT plf);
int termGetLogFont(const HWND hwndTerm, LPLOGFONT plf);

#endif

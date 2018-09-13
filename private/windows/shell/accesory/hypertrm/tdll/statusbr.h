/*	File: D:\WACKER\tdll\statusbr.h (Created: 08-Feb-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:41p $
 */

//
// Defines...
//
#define EXTRASPACE	(UINT)8

#define SBR_NTFY_REFRESH	(WM_USER+0x400)	// Update the statusbar display
#define SBR_NTFY_TIMER		(WM_USER+0x401) // Update time connected display
#define SBR_NTFY_NOPARTS	(WM_USER+0x402)	// Set status bar to show no parts
#define SBR_NTFY_INITIALIZE (WM_USER+0x403) // Calculate part sizes... once
#define SBR_NTFY_DRAWITEM	(WM_USER+0x404)	// DrawItem

#define SBR_ATOM_NAME		"PROP_SBRDATA"

#define YEAR(t)		(t << 9)
#define MONTH(t)	(t << 5)
#define HOUR(t)		(t << 11)
#define MINUTE(t)	(t << 5)
#define SECOND(t)	(t >> 1)

#define SBR_MAX_PARTS		8				// Maximum number of parts
#define SBR_CNCT_PART_NO	0				// connection status
#define SBR_EMU_PART_NO 	1				// emulator
#define SBR_COM_PART_NO 	2				// Com part
#define SBR_SCRL_PART_NO	3				// scroll lock
#define SBR_CAPL_PART_NO	4				// caps lock
#define SBR_NUML_PART_NO	5				// num lock
#define SBR_CAPT_PART_NO	6				// capture
#define SBR_PRNE_PART_NO	7				// print echo
#define SBR_ALL_PARTS		98
#define SBR_KEY_PARTS		99				// All key parts, SCRL, NUML, CAPL

typedef struct SBR
	{
	WNDPROC 	wpOrigStatusbarWndProc;		// Original Statusbar window proc
	HWND		hwnd;						// Statusbar window

	HSESSION	hSession;                   // Session handle
	HTIMER		hTimer;						// Timer used to update clocks
	int			iLastCnctStatus;			// Last connection status
	int			aWidths[SBR_MAX_PARTS];		// Statusbar part widths

	// Since we look at all string ahead of time to determine their length
	// and adjust the statusbar parts accordingly, we migth as well remember
	// the strings we read from the resource file. This saves us alot of
	// LoadString() calls.
	//
	LPTSTR		pachCNCT;					// Connected + time...
	LPTSTR		pachCAPL;					// CAP Lock label
	LPTSTR		pachNUML;					// NUM Lock label
	LPTSTR		pachSCRL;					// SCR Lock label
	LPTSTR		pachPECHO;					// Print Echo label
	LPTSTR		pachCAPT;					// Capture label
	LPTSTR		pachCOM;					// Com label

	} SBR, *pSBR;


//
// Function prototypes...
//
HWND sbrCreateSessionStatusbar(HSESSION hSession);

LRESULT APIENTRY sbrWndProc(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar);
void CALLBACK sbrTimerProc(void *pvData, long uTime);
void sbr_WM_DRAWITEM(HWND hwnd, LPDRAWITEMSTRUCT lpdis);

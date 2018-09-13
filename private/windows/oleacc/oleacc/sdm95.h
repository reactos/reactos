// Copyright (c) 1996-1999 Microsoft Corporation

//----------------------------------------------------------------------//
// WARNING! DANGER! WARNING! DANGER! WARNING! DANGER! WARNING!  DANGER! //
// DANGER! WARNING! DANGER! WARNING! DANGER! WARNING!  DANGER! WARNING! //
// WARNING! DANGER! WARNING! DANGER! WARNING! DANGER! WARNING!  DANGER! //
//----------------------------------------------------------------------//
//                                                                      //
// WARNING!       This file is part of the SDM project.         DANGER! //
// WARNING!    Do not modify it!  If you change this file       DANGER! //
// WARNING!   you will break the build when SDM is updated!     DANGER! //
// WARNING!   If you need to make changes, make them in the     DANGER! //
// WARNING!   SDM project.  If you don't know where that is     DANGER! //
// WARNING!    contact NeilH.  In fact, contact NeilH           DANGER! //
// WARNING!   anyway, to make sure your change doesn't break    DANGER! //
// WARNING!    a version that you're not using.  Thank you.     DANGER! //
//                                                                      //
//----------------------------------------------------------------------//
// WARNING! DANGER! WARNING! DANGER! WARNING! DANGER! WARNING!  DANGER! //
// DANGER! WARNING! DANGER! WARNING! DANGER! WARNING!  DANGER! WARNING! //
// WARNING! DANGER! WARNING! DANGER! WARNING! DANGER! WARNING!  DANGER! //
//----------------------------------------------------------------------//

//------------------------------------------------------------------------
// SDMTOWCT.H - include file containing the interface to be used for
// communication from outside applications with SDM dialogs.
//------------------------------------------------------------------------

#ifndef SDM_WCT_DEFINED
#define SDM_WCT_DEFINED

//------------------------------------------------------------------------
// WCT/SDM Values - Definitions and descriptions
//------------------------------------------------------------------------
#define wVerWord		2		// WinWord format


// A pointer to an array of WCTL structures is passed as the lParam
//  in a WM_GETCONTROLS message when wParam is wVerWord.
//

// 32-bit structure
typedef struct _wctl32
{
	WORD wtp;				// Item type
	WORD wId;				// Unique identifier within this dialog (TMC)
	WORD wState;			// Current value if fHasState
	WORD cchText;			// Size of text value, if fHasText
	WORD cchTitle;			// Size of title, if fHasTitle
    WORD wPad1;             // First padding word for WIN32
#ifdef MAC
	Rect rect;				// Rectangle in dialog window
#else
	RECT rect;				// Rectangle in dialog window
#endif
	LONG fHasState:1;		// Can this type of item have a numeric state?
	LONG fHasText:1;		// Can this type of item have a text value?
	LONG fHasTitle:1;		// Does the item have a title?
	LONG fEnabled:1;		// Is the item currently enabled?
	LONG fVisible:1;		// Is the item visible?
	LONG fCombo:1;			// Is the item a combo edit or listbox?
	LONG fSpin:1;			// Is the item a spin edit?
	LONG fOwnerDraw:1;		// Is the item owner-draw (or extended listbox)?
	LONG fCanFocus:1;		// Can the item receive focus?
	LONG fHasFocus:1;		// Does the item have focus?
	LONG fList:1;			// Supports wtxi.wIndex, WM_GETLISTCOUNT
	LONG lReserved:21;		// A bunch o' bits
	WORD wParam1;			// for tmtStaticText, tmtFormattedText
	WORD wParam2;			// as above
	WORD wParam3;			// yet another spare value for drawing routines
    WORD wPad2;             // Second padding word for WIN32
} WCTL32, *PWCTL32, FAR *LPWCTL32;


#pragma pack(1)
// 16-bit structure
typedef struct _wctl16
{
	WORD wtp;				// Item type
	WORD wId;				// Unique identifier within this dialog (TMC)
	WORD wState;			// Current value if fHasState
	WORD cchText;			// Size of text value, if fHasText
	WORD cchTitle;			// Size of title, if fHasTitle
#ifdef MAC
	Rect rect;				// Rectangle in dialog window
#else
    short   left;
    short   top;
    short   right;
    short   bottom;
#endif
	LONG fHasState:1;		// Can this type of item have a numeric state?
	LONG fHasText:1;		// Can this type of item have a text value?
	LONG fHasTitle:1;		// Does the item have a title?
	LONG fEnabled:1;		// Is the item currently enabled?
	LONG fVisible:1;		// Is the item visible?
	LONG fCombo:1;			// Is the item a combo edit or listbox?
	LONG fSpin:1;			// Is the item a spin edit?
	LONG fOwnerDraw:1;		// Is the item owner-draw (or extended listbox)?
	LONG fCanFocus:1;		// Can the item receive focus?
	LONG fHasFocus:1;		// Does the item have focus?
	LONG fList:1;			// Supports wtxi.wIndex, WM_GETLISTCOUNT
	LONG lReserved:21;		// A bunch o' bits
} WCTL16, *PWCTL16, FAR *LPWCTL16;
#pragma pack()


/* Possible values for wctl.wtp */
#define wtpMin				1
#define wtpStaticText		1
#define wtpPushButton		2
#define wtpCheckBox			3
#define wtpRadioButton		4
#define wtpGroupBox			5
#define wtpEdit				6
#define wtpFormattedText	7
#define wtpListBox			8
#define	wtpDropList			9
#define wtpBitmap			10
#define wtpGeneralPicture	11
#define wtpScroll			12
#define wtpMax				13

// A pointer to a WTXI structure is passed as the lParam of
//  a WM_GETCTLTEXT or WM_GETCTLTITLE message.
//

// Win32 structure
typedef struct _wtxi32		// WinWord text info
{
#ifdef MAC
	char*	lpszBuffer;		// Buffer to receive string
#else
	LPSTR	lpszBuffer;		// Buffer to receive string
#endif
	WORD	cch;			// Size of buffer to receive string, in chars
	WORD	wId;			// Item identifier (TMC) (as in wctl.wId)
#ifdef MAC
	Rect	rect;			// Only used for WM_GETCTLTITLE
#else
	RECT	rect;			// Only used for WM_GETCTLTITLE
#endif
	WORD	wIndex;			// Only used for WM_GETCTLTEXT on ListBoxes
    WORD    wPad1;          // Padding for Win32
} WTXI32, *PWTXI32, FAR *LPWTXI32;


// Win16 structure
#pragma pack(1)
typedef struct _wtxi16
{
#ifdef MAC
	char*	lpszBuffer;		// Buffer to receive string
#else
	LPSTR	lpszBuffer;		// Buffer to receive string
#endif
	WORD	cch;			// Size of buffer to receive string, in chars
	WORD	wId;			// Item identifier (TMC) (as in wctl.wId)
#ifdef MAC
	Rect	rect;			// Only used for WM_GETCTLTITLE
#else
    short   left;
    short   top;
    short   right;
    short   bottom;
#endif
	WORD	wIndex;			// Only used for WM_GETCTLTEXT on ListBoxes
} WTXI16, *PWTXI16, FAR *LPWTXI16;
#pragma pack()


//------------------------------------------------------------------------
// WCT/SDM MESSAGES - Definitions and descriptions
//------------------------------------------------------------------------

#define WM_GETCOUNT		0x7FFE
	// Returns the number of bytes needed to store control info.
	//	wParam	- the version id
	//		Must be wVerWord
	//	lParam	- Unused
	//		Must be 0

#define WM_GETCONTROLSSHAREDMEM 0x7FF6
#define WM_GETCONTROLS	0x7FF7
	// Retrieves control information for the dialog.
	//	wParam	- the version id
	//		Must be wVerWord
	//	lParam	- LPWCTL
	//		Must be at least the size returned by WM_GETCOUNT
	// Return value is the number of WCTL structures filled.

#define WM_GETCTLTEXT	0x7FFD
	// Retrieves the text value for the specified control
	//	wParam	- the version id
	//		Must be wVerWord
	//	lParam	- LPWTXI
	//		(*lParam)->wId is the wctl.wId retrieved by WM_GETCONTROLS.
	//		For a listbox (wtpListBox or wtpDropList) (*lParam)->wIndex
	//		  must be the index of the listbox entry to be retrieved.

#define WM_GETCTLTITLE	0x7FFC
	// Retrieves the title of the specified control
	//	wParam	- the version id
	//		Must be wVerWord
	//	lParam	- LPWTXI
	//		(*lParam)->wId is the wctl.wId retrieved by WM_GETCONTROLS

#define WM_GETCTLFOCUS	0x7FFB
	// Returns the wId (TMC) (as in wctl.wId) of the control with focus.
	//	wParam	- the version id
	//		Must be wVerWord
	//	lParam	- Unused
	//		Must be 0

#define WM_SETCTLFOCUS	0x7FFA
	// Sets focus to the specified control
	//	wParam	- the version id
	//		Must be wVerWord
	//	lParam	- a wId value as retrieved by WM_GETCONTROLS

#define WM_GETLISTCOUNT 0x7FF9
	// Returns the number of entries in a listbox
	// wParam	- the version id
	//		Must be wVerWord
	//	lParam	- a wId value as retrieved by WM_GETCONTROLS
	//		Must be a listbox (wtpListBox or wtpDropList)

#define WM_GETHELPID	0x7FF8
	// Returns the dialog's Help ID
	// wParam	- the version id
	//		Must be wVerWord
	//	lParam	- Unused
	//		Must be 0

#define WM_GETCONTROLSMOUSEDRV 0x7FFF
	// special "light" version of GETCONTROLS
	// used by the mouse 9.01 driver

#define WM_GETDROPDOWNID 0x7FF5
	//Returns the item identifier (TMC) of the control that currently owns the dropdown list window
	//This message should be sent directly to the dropdown list window.
	// wParam	- the version id
	//		Must be wVerAnsi or wVerUnicode
	//	lParam	- Unused
	//		Must be 0

#endif //SDM_WCT_DEFINED


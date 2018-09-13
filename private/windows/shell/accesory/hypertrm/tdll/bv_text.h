/*	File: C:|WACKER\TDLL\BV_TEXT.H (Created: 11-JAN-1994)
 *	Created from:
 *	File: C:\HA5G\ha5g\s_text.h (Created: 27-SEP-1991)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:35p $
 */

#define WM_STXT_SET_BK			(WM_USER+0x380)
#define WM_STXT_SET_TXT 		(WM_USER+0x381)
#define WM_STXT_SET_UE			(WM_USER+0x382)
#define WM_STXT_SET_LE			(WM_USER+0x383)
#define WM_STXT_SET_DEPTH		(WM_USER+0x384)
#define WM_STXT_OWNERDRAW		(WM_USER+0x385)

#define STXT_DEF_DEPTH	   2

extern BOOL RegisterBeveledTextClass(HANDLE hInstance);

extern LONG CALLBACK BeveledTextWndProc(HWND hWnd,
									  UINT wMsg,
									  WPARAM wPar,
									  LPARAM lPar);


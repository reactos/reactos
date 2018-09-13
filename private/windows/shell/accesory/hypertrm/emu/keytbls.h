/*	File: D:\LELAND\emu\keytbls.h (Created: 27-Dec-1994)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:27p $
 */
// ------------  OS/2 Include Files ------------
#include <windows.h>
#pragma hdrstop
#include <tdll\chars.h>

#define MAX_ANSI_KEYS 16
extern const KEYTBLSTORAGE AnsiKeyTable[MAX_ANSI_KEYS];

#define MAX_IBMPC_KEYS 185
extern const KEYTBLSTORAGE IBMPCKeyTable[MAX_IBMPC_KEYS];

#define MAX_VT52_KEYS 19
extern const KEYTBLSTORAGE VT52KeyTable[MAX_VT52_KEYS];

#define MAX_VT52_KEYPAD_KEYS 14
extern const KEYTBLSTORAGE VT52_Keypad_KeyTable[MAX_VT52_KEYPAD_KEYS];

#define MAX_VT_PF_KEYS 4
extern const KEYTBLSTORAGE VT_PF_KeyTable[MAX_VT_PF_KEYS];

#define MAX_VT100_KEYS 24
extern const KEYTBLSTORAGE VT100KeyTable[MAX_VT100_KEYS];

#define MAX_VT100_CURSOR_KEYS 8
extern const KEYTBLSTORAGE VT100_Cursor_KeyTable[MAX_VT100_CURSOR_KEYS];

#define MAX_VT100_KEYPAD_KEYS 14
extern const KEYTBLSTORAGE VT100_Keypad_KeyTable[MAX_VT100_KEYPAD_KEYS];

#define MAX_MINITEL_KEYS 82
extern const KEYTBLSTORAGE Minitel_KeyTable[MAX_MINITEL_KEYS];

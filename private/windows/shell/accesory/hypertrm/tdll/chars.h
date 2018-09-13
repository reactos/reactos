/*	File: D:\WACKER\tdll\chars.h (Created: 30-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:40p $
 */

#if !defined(INCL_CHARS)
#define INCL_CHARS

/*
 * Keystrokes are passed in the following manner:
 *
 *	If the VIRTUAL_KEY flag is clear, then the lower byte of the value is the
 *		displayable (usually ASCII) code for the character
 *
 *	If the VIRTUAL_KEY flag is set, then the lower byte ov the value is the
 *		WINDOWs VK_* code for the key that was pressed.  In addition, the flags
 *		for ALT_KEY, CTRL_KEY, and SHIFT_KEY are set to the correct values.
 */

#define VIRTUAL_KEY 		0x00800000

#define ALT_KEY 			0x00010000
#define CTRL_KEY			0x00020000
#define SHIFT_KEY			0x00040000
#define EXTENDED_KEY		0x00080000

KEY_T TranslateToKey(const LPMSG pmsg);

#endif

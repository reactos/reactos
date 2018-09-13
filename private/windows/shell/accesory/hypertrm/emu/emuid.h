/*	File: D:\WACKER\emu\emuid.h (Created: 08-Dec-1993)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:27p $
 */

#define IDT_BASE 0xD00 // = 3328 in decimal

// Table identifiers should be prefixed with "IDT" meaning ID Table.
// Interestingly, the RC compiler won't do integer math on RCDATA
// indentifiers so I have to hard code the numbers.  Actually, its
// worse than that, it can't even read 0x000 format numbers!  They
// have to be decimal.	Stupid!!!

//#define IDT_EMU_NAMES 			  3328

#define IDT_EMU_VT100_CHAR_SETS 	3383
#define IDT_EMU_NAT_CHAR_SETS		3384

#define IDT_ANSI_KEYS				3385

#define IDT_VT_MAP_PF_KEYS			3386

#define IDT_VT52_KEYS				3387
#define IDT_VT52_KEYPAD_APP_MODE	3388

#define IDT_VT100_KEYS				3389
#define IDT_VT100_CURSOR_KEY_MODE	3390
#define IDT_VT100_KEYPAD_APP_MODE	3391

#define IDT_VT220_KEYS				3392
#define IDT_VT220_CURSOR_KEY_MODE	3393
#define IDT_VT220_KEYPAD_APP_MODE	3394
#define IDT_VT220_MAP_PF_KEYS_MODE	3395

#define IDT_TV950_KEYS				3396
#define IDT_TV950_FKEYS 			3397
#define IDT_WANG_KEYS				3398
#define IDT_IBM3278_KEYS			3399
#define IDT_RENX3278_KEYS			3400
#define IDT_IBM3101_KEYS			3401
#define IDT_IBMPC_KEYS				3402

#define IDT_MINITEL_KEYS			3403

#define IDT_EMU_VT220_CHAR_SETS 	3404

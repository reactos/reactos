/*	File: D:\WACKER\emu\emuhdl.c (Created: 10-Dec-1993)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 4/22/99 12:30p $
 */

#include <windows.h>
#pragma hdrstop
// #define DEBUGSTR

#include <time.h>

#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\assert.h>
#include <tdll\sf.h>
#include <tdll\sess_ids.h>
#include <tdll\session.h>
#include <tdll\capture.h>
#include <tdll\cloop.h>
#include <tdll\term.h>
#include <tdll\print.h>
#include <tdll\update.h>
#include <tdll\load_res.h>
#include <tdll\globals.h>
#include <tdll\statusbr.h>
#include <tdll\cnct.h>
#include <tdll\tchar.h>
#include <term\res.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"
#include "viewdata.hh"
#include "minitel.hh"
#include "emudec.hh"
#include "keytbls.h"

// Function prototypes...
//
STATIC_FUNC int emuLoadSettings(const HHEMU hhEmu, 
								const unsigned int uiId,
								unsigned long ulDataSize,
								void *pvData);
STATIC_FUNC void emuCheckSettings(struct stEmuBaseSFSettings *pstBaseSettings);

// Keytable declarations
//
const KEYTBLSTORAGE AnsiKeyTable[MAX_ANSI_KEYS] =
    {
	{VK_UP		| VIRTUAL_KEY,	{"\x1B[A\xff"}},
	{VK_DOWN 	| VIRTUAL_KEY,	{"\x1B[B\xff"}},
	{VK_RIGHT	| VIRTUAL_KEY,	{"\x1B[C\xff"}},
	{VK_LEFT 	| VIRTUAL_KEY,	{"\x1B[D\xff"}},

	// Added 27 Aug 98 rde
    {VK_HOME    | VIRTUAL_KEY,	{"\x1B[H\xff"}},
    {VK_END     | VIRTUAL_KEY,	{"\x1B[K\xff"}},

	//VK_UP 	  | VIRTUAL_KEY | CTRL_KEY,  "\x1BOx\xff",
	//VK_DOWN	  | VIRTUAL_KEY | CTRL_KEY,  "\x1BOr\xff",
	//VK_RIGHT	  | VIRTUAL_KEY | CTRL_KEY,  "\x1BOv\xff",
	//VK_LEFT	  | VIRTUAL_KEY | CTRL_KEY,  "\x1BOt\xff",

	//VK_HOME	  | VIRTUAL_KEY,  "\x1B[H\xff",
	//VK_END	  | VIRTUAL_KEY,  "\x1B[K\xff",

	//VK_HOME	  | VIRTUAL_KEY | CTRL_KEY,  "\x1BOw\xff",
	//VK_END	  | VIRTUAL_KEY | CTRL_KEY,  "\x1BOq\xff",

	//VK_PRIOR	  | VIRTUAL_KEY,  "\x1B[M\xff",
	//VK_NEXT	  | VIRTUAL_KEY,  "\x1B[H\x1B[2J\xff",

	//VK_PRIOR	  | VIRTUAL_KEY | CTRL_KEY,  "\x1BOy\xff",
	//VK_NEXT	  | VIRTUAL_KEY | CTRL_KEY,  "\x1BOs\xff",

	//VK_RETURN   | VIRTUAL_KEY,  "\x0D\xff",
	//VK_RETURN   | VIRTUAL_KEY | CTRL_KEY,  "\x1BOM\xff",

	//VK_INSERT   | VIRTUAL_KEY | CTRL_KEY,  "\x1BOp\xff",

	/* -------------- Function keys ------------- */

	{VK_F1		| VIRTUAL_KEY,	{"\x1BOP\xff"}},
	{VK_F2		| VIRTUAL_KEY,	{"\x1BOQ\xff"}},
	{VK_F3		| VIRTUAL_KEY,	{"\x1BOR\xff"}},
	{VK_F4		| VIRTUAL_KEY,	{"\x1BOS\xff"}},

	/* -------------- Gray keys, (extended edit pad) ------------- */

	{VK_UP		| VIRTUAL_KEY | EXTENDED_KEY,  {"\x1B[A\xff"}},
	{VK_DOWN 	| VIRTUAL_KEY | EXTENDED_KEY,  {"\x1B[B\xff"}},
	{VK_RIGHT	| VIRTUAL_KEY | EXTENDED_KEY,  {"\x1B[C\xff"}},
	{VK_LEFT 	| VIRTUAL_KEY | EXTENDED_KEY,  {"\x1B[D\xff"}},

	// Added 27 Aug 98 rde
	{VK_HOME 	| VIRTUAL_KEY | EXTENDED_KEY,  {"\x1B[H\xff"}},
	{VK_END 	| VIRTUAL_KEY | EXTENDED_KEY,  {"\x1B[K\xff"}},

	//VK_HOME	  | VIRTUAL_KEY | EXTENDED_KEY,  "\x1B[H\xff",
	//VK_END	  | VIRTUAL_KEY | EXTENDED_KEY,  "\x1B[K\xff",

	//VK_PRIOR	  | VIRTUAL_KEY | EXTENDED_KEY,  "\x1B[M\xff",
	//VK_NEXT	  | VIRTUAL_KEY | EXTENDED_KEY,  "\x1B[H\x1B[2J\xff",
	};

/* Also for ANSI emulator */
const KEYTBLSTORAGE IBMPCKeyTable[MAX_IBMPC_KEYS] =
	{
	{VK_BACK,				   {"\x08\xff"}},		/* KN_BS */
	{VK_DELETE	| VIRTUAL_KEY, {"\x00\x53\xff"}},	/* KN_DEL */
	{VK_DOWN 	| VIRTUAL_KEY, {"\x00\x50\xff"}},	/* KN_DOWN */
	{VK_END		| VIRTUAL_KEY, {"\x00\x4F\xff"}},	/* KN_END */
	{VK_RETURN,				   {"\x0D\xff"}},		/* KN_ENTER */
	{VK_ESCAPE,				   {"\x1B\xff"}},		/* KN_ESC */
	{VK_F1		| VIRTUAL_KEY, {"\x00\x3B\xff"}},	/* KN_F1 */
	{VK_F2		| VIRTUAL_KEY, {"\x00\x3C\xff"}},	/* KN_F2 */
	{VK_F3		| VIRTUAL_KEY, {"\x00\x3D\xff"}},	/* KN_F3 */
	{VK_F4		| VIRTUAL_KEY, {"\x00\x3E\xff"}},	/* KN_F4 */
	{VK_F5		| VIRTUAL_KEY, {"\x00\x3F\xff"}},	/* KN_F5 */
	{VK_F6		| VIRTUAL_KEY, {"\x00\x40\xff"}},	/* KN_F6 */
	{VK_F7		| VIRTUAL_KEY, {"\x00\x41\xff"}},	/* KN_F7 */
	{VK_F8		| VIRTUAL_KEY, {"\x00\x42\xff"}},	/* KN_F8 */
	{VK_F9		| VIRTUAL_KEY, {"\x00\x43\xff"}},	/* KN_F9 */
	{VK_F10		| VIRTUAL_KEY, {"\x00\x44\xff"}},	/* KN_F10 */
	{VK_F11		| VIRTUAL_KEY, {"\x00\x85\xff"}},	/* KN_F11 */
	{VK_F11		| VIRTUAL_KEY, {"\x00\x86\xff"}},	/* KN_F12 */
	{VK_HOME 	| VIRTUAL_KEY, {"\x00\x47\xff"}},	/* KN_HOME */
	{VK_INSERT	| VIRTUAL_KEY, {"\x00\x52\xff"}},	/* KN_INS */
	{VK_LEFT 	| VIRTUAL_KEY, {"\x00\x4B\xff"}},	/* KN_LEFT */
	{VK_NEXT 	| VIRTUAL_KEY, {"\x00\x51\xff"}},	/* KN_PGDN */
	{VK_PRIOR	| VIRTUAL_KEY, {"\x00\x49\xff"}},	/* KN_PGUP */
	{VK_RIGHT	| VIRTUAL_KEY, {"\x00\x4D\xff"}},	/* KN_RIGHT */
	{VK_TAB, 				   {"\x09\xff"}},		/* KN_TAB */
	{VK_UP		| VIRTUAL_KEY, {"\x00\x48\xff"}},	/* KN_UP */

	{VK_BACK 	| VIRTUAL_KEY | CTRL_KEY, {"\x7F\xff"}},	   /* KT_CTRL + KN_BS */
	{VK_DELETE	| VIRTUAL_KEY | CTRL_KEY, {"\x00\x93\xff"}},  /* KT_CTRL + KN_DEL */
	{VK_DOWN 	| VIRTUAL_KEY | CTRL_KEY, {"\x00\x91\xff"}},  /* KT_CTRL + KN_DOWN */
	{VK_END		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x75\xff"}},  /* KT_CTRL + KN_END */
	{VK_RETURN	| VIRTUAL_KEY | CTRL_KEY, {"\x0A\xff"}},	   /* KT_CTRL + KN_ENTER */

	{VK_F1		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x5E\xff"}},  /* KT_CTRL + KN_F1 */
	{VK_F10		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x67\xff"}},  /* KT_CTRL + KN_F10 */
	{VK_F11		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x89\xff"}},  /* KT_CTRL + KN_F11 */
	{VK_F12		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x8A\xff"}},  /* KT_CTRL + KN_F12 */
	{VK_F2		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x5F\xff"}},  /* KT_CTRL + KN_F2 */
	{VK_F3		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x60\xff"}},  /* KT_CTRL + KN_F3 */
	{VK_F4		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x61\xff"}},  /* KT_CTRL + KN_F4 */
	{VK_F5		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x62\xff"}},  /* KT_CTRL + KN_F5 */
	{VK_F6		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x63\xff"}},  /* KT_CTRL + KN_F6 */
	{VK_F7		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x64\xff"}},  /* KT_CTRL + KN_F7 */
	{VK_F8		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x65\xff"}},  /* KT_CTRL + KN_F8 */
	{VK_F9		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x66\xff"}},  /* KT_CTRL + KN_F9 */
	{VK_HOME 	| VIRTUAL_KEY | CTRL_KEY, {"\x00\x77\xff"}},  /* KT_CTRL + KN_HOME */
	{VK_INSERT	| VIRTUAL_KEY | CTRL_KEY, {"\x00\x92\xff"}},  /* KT_CTRL + KN_INS */
	{VK_LEFT 	| VIRTUAL_KEY | CTRL_KEY, {"\x00\x73\xff"}},  /* KT_CTRL + KN_LEFT */
	{VK_F1		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x8F\xff"}},  /* KT_CTRL + KN_MID */
	{VK_PRIOR	| VIRTUAL_KEY | CTRL_KEY, {"\x00\x76\xff"}},  /* KT_CTRL + KN_PGDN */
	{VK_NEXT 	| VIRTUAL_KEY | CTRL_KEY, {"\x00\x84\xff"}},  /* KT_CTRL + KN_PGUP */
	{VK_PRINT	| VIRTUAL_KEY | CTRL_KEY, {"\x00\x72\xff"}},  /* KT_CTRL + KN_PRTSC */
	{VK_RIGHT	| VIRTUAL_KEY | CTRL_KEY, {"\x00\x74\xff"}},  /* KT_CTRL + KN_RIGHT */
	{VK_TAB		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x94\xff"}},  /* KT_CTRL + KN_TAB */
	{VK_UP		| VIRTUAL_KEY | CTRL_KEY, {"\x00\x8D\xff"}},  /* KT_CTRL + KN_UP */
														 
	{VK_F1		| VIRTUAL_KEY | SHIFT_KEY,{"\x00\x54\xff"}},  /* KT_SHIFT + KN_F1 */
	{VK_F10		| VIRTUAL_KEY | SHIFT_KEY,{"\x00\x5D\xff"}},  /* KT_SHIFT + KN_F10 */
	{VK_F11		| VIRTUAL_KEY | SHIFT_KEY,{"\x00\x87\xff"}},  /* KT_SHIFT + KN_F11 */
	{VK_F12		| VIRTUAL_KEY | SHIFT_KEY,{"\x00\x88\xff"}},  /* KT_SHIFT + KN_F12 */
	{VK_F2		| VIRTUAL_KEY | SHIFT_KEY,{"\x00\x55\xff"}},  /* KT_SHIFT + KN_F2 */
	{VK_F3		| VIRTUAL_KEY | SHIFT_KEY,{"\x00\x56\xff"}},  /* KT_SHIFT + KN_F3 */
	{VK_F4		| VIRTUAL_KEY | SHIFT_KEY,{"\x00\x57\xff"}},  /* KT_SHIFT + KN_F4 */
	{VK_F5		| VIRTUAL_KEY | SHIFT_KEY,{"\x00\x58\xff"}},  /* KT_SHIFT + KN_F5 */
	{VK_F6		| VIRTUAL_KEY | SHIFT_KEY,{"\x00\x59\xff"}},  /* KT_SHIFT + KN_F6 */
	{VK_F7		| VIRTUAL_KEY | SHIFT_KEY,{"\x00\x5A\xff"}},  /* KT_SHIFT + KN_F7 */
	{VK_F8		| VIRTUAL_KEY | SHIFT_KEY,{"\x00\x5B\xff"}},  /* KT_SHIFT + KN_F8 */
	{VK_F9		| VIRTUAL_KEY | SHIFT_KEY,{"\x00\x5C\xff"}},  /* KT_SHIFT + KN_F9 */
	{VK_TAB		| VIRTUAL_KEY | SHIFT_KEY,{"\x00\x0F\xff"}},  /* KT_SHIFT + KN_TAB */

	{0x5C | ALT_KEY | VIRTUAL_KEY, {"\x00\x28\xff"}},  /* KT_ALT + '\'' */
	{0x2C | ALT_KEY | VIRTUAL_KEY, {"\x00\x33\xff"}},  /* KT_ALT + ',' */
	{0x2D | ALT_KEY | VIRTUAL_KEY, {"\x00\x82\xff"}},  /* KT_ALT + '-' */
	{0x2E | ALT_KEY | VIRTUAL_KEY, {"\x00\x34\xff"}},  /* KT_ALT + '.' */
	{0x2F | ALT_KEY | VIRTUAL_KEY, {"\x00\x35\xff"}},  /* KT_ALT + '/' */
	{0x30 | ALT_KEY | VIRTUAL_KEY, {"\x00\x81\xff"}},  /* KT_ALT + '0' */
	{0x31 | ALT_KEY | VIRTUAL_KEY, {"\x00\x78\xff"}},  /* KT_ALT + '1' */
	{0x32 | ALT_KEY | VIRTUAL_KEY, {"\x00\x79\xff"}},  /* KT_ALT + '2' */
	{0x33 | ALT_KEY | VIRTUAL_KEY, {"\x00\x7A\xff"}},  /* KT_ALT + '3' */
	{0x34 | ALT_KEY | VIRTUAL_KEY, {"\x00\x7B\xff"}},  /* KT_ALT + '4' */
	{0x35 | ALT_KEY | VIRTUAL_KEY, {"\x00\x7C\xff"}},  /* KT_ALT + '5' */
	{0x36 | ALT_KEY | VIRTUAL_KEY, {"\x00\x7D\xff"}},  /* KT_ALT + '6' */
	{0x37 | ALT_KEY | VIRTUAL_KEY, {"\x00\x7E\xff"}},  /* KT_ALT + '7' */
	{0x38 | ALT_KEY | VIRTUAL_KEY, {"\x00\x7F\xff"}},  /* KT_ALT + '8' */
	{0x39 | ALT_KEY | VIRTUAL_KEY, {"\x00\x80\xff"}},  /* KT_ALT + '9' */
	{0x3B | ALT_KEY | VIRTUAL_KEY, {"\x00\x27\xff"}},  /* KT_ALT + ';' */
	{0x3D | ALT_KEY | VIRTUAL_KEY, {"\x00\x83\xff"}},  /* KT_ALT + '=' */
	{0x41 | ALT_KEY | VIRTUAL_KEY, {"\x00\x1E\xff"}},  /* KT_ALT + 'A' */
	{0x42 | ALT_KEY | VIRTUAL_KEY, {"\x00\x30\xff"}},  /* KT_ALT + 'B' */
	{0x43 | ALT_KEY | VIRTUAL_KEY, {"\x00\x2E\xff"}},  /* KT_ALT + 'C' */
	{0x44 | ALT_KEY | VIRTUAL_KEY, {"\x00\x20\xff"}},  /* KT_ALT + 'D' */
	{0x45 | ALT_KEY | VIRTUAL_KEY, {"\x00\x12\xff"}},  /* KT_ALT + 'E' */
	{0x46 | ALT_KEY | VIRTUAL_KEY, {"\x00\x21\xff"}},  /* KT_ALT + 'F' */
	{0x47 | ALT_KEY | VIRTUAL_KEY, {"\x00\x22\xff"}},  /* KT_ALT + 'G' */
	{0x48 | ALT_KEY | VIRTUAL_KEY, {"\x00\x23\xff"}},  /* KT_ALT + 'H' */
	{0x49 | ALT_KEY | VIRTUAL_KEY, {"\x00\x17\xff"}},  /* KT_ALT + 'I' */
	{0x4A | ALT_KEY | VIRTUAL_KEY, {"\x00\x24\xff"}},  /* KT_ALT + 'J' */
	{0x4B | ALT_KEY | VIRTUAL_KEY, {"\x00\x25\xff"}},  /* KT_ALT + 'K' */
	{0x4C | ALT_KEY | VIRTUAL_KEY, {"\x00\x26\xff"}},  /* KT_ALT + 'L' */
	{0x4D | ALT_KEY | VIRTUAL_KEY, {"\x00\x32\xff"}},  /* KT_ALT + 'M' */
	{0x4E | ALT_KEY | VIRTUAL_KEY, {"\x00\x31\xff"}},  /* KT_ALT + 'N' */
	{0x4F | ALT_KEY | VIRTUAL_KEY, {"\x00\x18\xff"}},  /* KT_ALT + 'O' */
	{0x50 | ALT_KEY | VIRTUAL_KEY, {"\x00\x19\xff"}},  /* KT_ALT + 'P' */
	{0x51 | ALT_KEY | VIRTUAL_KEY, {"\x00\x10\xff"}},  /* KT_ALT + 'Q' */
	{0x52 | ALT_KEY | VIRTUAL_KEY, {"\x00\x13\xff"}},  /* KT_ALT + 'R' */
	{0x53 | ALT_KEY | VIRTUAL_KEY, {"\x00\x1F\xff"}},  /* KT_ALT + 'S' */
	{0x54 | ALT_KEY | VIRTUAL_KEY, {"\x00\x14\xff"}},  /* KT_ALT + 'T' */
	{0x55 | ALT_KEY | VIRTUAL_KEY, {"\x00\x16\xff"}},  /* KT_ALT + 'U' */
	{0x56 | ALT_KEY | VIRTUAL_KEY, {"\x00\x2F\xff"}},  /* KT_ALT + 'V' */
	{0x57 | ALT_KEY | VIRTUAL_KEY, {"\x00\x11\xff"}},  /* KT_ALT + 'W' */
	{0x58 | ALT_KEY | VIRTUAL_KEY, {"\x00\x2D\xff"}},  /* KT_ALT + 'X' */
	{0x59 | ALT_KEY | VIRTUAL_KEY, {"\x00\x15\xff"}},  /* KT_ALT + 'Y' */
	{0x5A | ALT_KEY | VIRTUAL_KEY, {"\x00\x2C\xff"}},  /* KT_ALT + 'Z' */
	{0x5B | ALT_KEY | VIRTUAL_KEY, {"\x00\x1A\xff"}},  /* KT_ALT + '[' */
	{0x5D | ALT_KEY | VIRTUAL_KEY, {"\x00\x1B\xff"}},  /* KT_ALT + ']' */
	{0x60 | ALT_KEY | VIRTUAL_KEY, {"\x00\x29\xff"}},  /* KT_ALT + '`' */
								   				  
	{VK_BACK 	| VIRTUAL_KEY | ALT_KEY, {"\x00\x0E\xff"}},  /* KT_ALT + KN_BS */
	{VK_RETURN	| VIRTUAL_KEY | ALT_KEY, {"\x00\x1C\xff"}},  /* KT_ALT + KN_ENTER */
	{VK_ESCAPE	| VIRTUAL_KEY | ALT_KEY, {"\x00\x01\xff"}},  /* KT_ALT + KN_ESC */
	{VK_F1		| VIRTUAL_KEY | ALT_KEY, {"\x00\x68\xff"}},  /* KT_ALT + KN_F1 */
	{VK_F10		| VIRTUAL_KEY | ALT_KEY, {"\x00\x71\xff"}},  /* KT_ALT + KN_F10 */
	{VK_F11		| VIRTUAL_KEY | ALT_KEY, {"\x00\x8B\xff"}},  /* KT_ALT + KN_F11 */
	{VK_F12		| VIRTUAL_KEY | ALT_KEY, {"\x00\x8C\xff"}},  /* KT_ALT + KN_F12 */
	{VK_F2		| VIRTUAL_KEY | ALT_KEY, {"\x00\x69\xff"}},  /* KT_ALT + KN_F2 */
	{VK_F3		| VIRTUAL_KEY | ALT_KEY, {"\x00\x6A\xff"}},  /* KT_ALT + KN_F3 */
	{VK_F4		| VIRTUAL_KEY | ALT_KEY, {"\x00\x6B\xff"}},  /* KT_ALT + KN_F4 */
	{VK_F5		| VIRTUAL_KEY | ALT_KEY, {"\x00\x6C\xff"}},  /* KT_ALT + KN_F5 */
	{VK_F6		| VIRTUAL_KEY | ALT_KEY, {"\x00\x6D\xff"}},  /* KT_ALT + KN_F6 */
	{VK_F7		| VIRTUAL_KEY | ALT_KEY, {"\x00\x6E\xff"}},  /* KT_ALT + KN_F7 */
	{VK_F8		| VIRTUAL_KEY | ALT_KEY, {"\x00\x6F\xff"}},  /* KT_ALT + KN_F8 */
	{VK_F9		| VIRTUAL_KEY | ALT_KEY, {"\x00\x70\xff"}},  /* KT_ALT + KN_F9 */
	{VK_TAB		| VIRTUAL_KEY | ALT_KEY, {"\x00\xA5\xff"}},  /* KT_ALT + KN_TAB */
										 
	{VK_RETURN | EXTENDED_KEY,	{"\x00\xE0\x0D\xff"}},  /* KT_KP + KN_ENTER */

	{0x2F | EXTENDED_KEY,   {"\x00\xE0\x2F\xff"}},	/* KT_KP + '/' */

	{VK_MULTIPLY | VIRTUAL_KEY,	  {"\x2A\xff"}},	   /* KT_KP + '*' */
	{VK_ADD		| VIRTUAL_KEY,	  {"\x2B\xff"}},	   /* KT_KP + '+' */
	{VK_SUBTRACT | VIRTUAL_KEY,	  {"\x2D\xff"}},	   /* KT_KP + '-' */
	{VK_DECIMAL | VIRTUAL_KEY,	  {"\x2E\xff"}},	   /* KT_KP + '.' */
	{VK_NUMPAD0 | VIRTUAL_KEY,	  {"\x30\xff"}},	   /* KT_KP + '0' */
	{VK_NUMPAD1 | VIRTUAL_KEY,	  {"\x31\xff"}},	   /* KT_KP + '1' */
	{VK_NUMPAD2 | VIRTUAL_KEY,	  {"\x32\xff"}},	   /* KT_KP + '2' */
	{VK_NUMPAD3 | VIRTUAL_KEY,	  {"\x33\xff"}},	   /* KT_KP + '3' */
	{VK_NUMPAD4 | VIRTUAL_KEY,	  {"\x34\xff"}},	   /* KT_KP + '4' */
	{VK_NUMPAD5 | VIRTUAL_KEY,	  {"\x35\xff"}},	   /* KT_KP + '5' */
	{VK_NUMPAD6 | VIRTUAL_KEY,	  {"\x36\xff"}},	   /* KT_KP + '6' */
	{VK_NUMPAD7 | VIRTUAL_KEY,	  {"\x37\xff"}},	   /* KT_KP + '7' */
	{VK_NUMPAD8 | VIRTUAL_KEY,	  {"\x38\xff"}},	   /* KT_KP + '8' */
	{VK_NUMPAD9 | VIRTUAL_KEY,	  {"\x39\xff"}},	   /* KT_KP + '9' */
								  
	{VK_MULTIPLY | VIRTUAL_KEY | CTRL_KEY, {"\x00\x96\xff"}},  /* KT_CTRL + KT_KP + '*' */
	{VK_ADD		| VIRTUAL_KEY | CTRL_KEY,  {"\x00\x90\xff"}},  /* KT_CTRL + KT_KP + '+' */
	{VK_SUBTRACT | VIRTUAL_KEY | CTRL_KEY, {"\x00\x8E\xff"}},  /* KT_CTRL + KT_KP + '-' */
	{VK_DIVIDE	| VIRTUAL_KEY | EXTENDED_KEY | CTRL_KEY, {"\x00\x95\xff"}},  /* KT_CTRL + KT_KP + '/' */
	{0x0a | EXTENDED_KEY | CTRL_KEY, {"\x0A\xff"}}, 	 /* KT_CTRL+KT_KP+KN_ENTER */

	{VK_MULTIPLY | VIRTUAL_KEY | ALT_KEY,  {"\x00\x37\xff"}},  /* KT_ALT + KT_KP + '*' */
	{VK_ADD		| VIRTUAL_KEY | ALT_KEY,   {"\x00\x4E\xff"}},  /* KT_ALT + KT_KP + '+' */
	{VK_SUBTRACT | VIRTUAL_KEY | ALT_KEY,  {"\x00\x4A\xff"}},  /* KT_ALT + KT_KP + '-' */
	{VK_DIVIDE	| VIRTUAL_KEY | ALT_KEY,   {"\x00\xA4\xff"}},  /* KT_ALT + KT_KP + '/' */
														  
	{0x0d | VIRTUAL_KEY | EXTENDED_KEY | ALT_KEY,  {"\x00\xA6\xff"}},  /* KT_ALT + KT_KP +KN_ENTER */

	{VK_DELETE | VIRTUAL_KEY | EXTENDED_KEY, {"\x00\xE0\x53\xff"}},  /* KT_EP +KN_DEL */
	{VK_DOWN   | VIRTUAL_KEY | EXTENDED_KEY, {"\x00\xE0\x50\xff"}},  /* KT_EP +KN_DOWN */
	{VK_END	  | VIRTUAL_KEY | EXTENDED_KEY,  {"\x00\xE0\x4F\xff"}},  /* KT_EP +KN_END */
	{VK_HOME   | VIRTUAL_KEY | EXTENDED_KEY, {"\x00\xE0\x47\xff"}},  /* KT_EP +KN_HOME */
	{VK_INSERT | VIRTUAL_KEY | EXTENDED_KEY, {"\x00\xE0\x52\xff"}},  /* KT_EP +KN_INS */
	{VK_LEFT   | VIRTUAL_KEY | EXTENDED_KEY, {"\x00\xE0\x4B\xff"}},  /* KT_EP +KN_LEFT */
	{VK_NEXT   | VIRTUAL_KEY | EXTENDED_KEY, {"\x00\xE0\x51\xff"}},  /* KT_EP +KN_PGDN */
	{VK_PRIOR  | VIRTUAL_KEY | EXTENDED_KEY, {"\x00\xE0\x49\xff"}},  /* KT_EP +KN_PGUP */
	{VK_RIGHT  | VIRTUAL_KEY | EXTENDED_KEY, {"\x00\xE0\x4D\xff"}},  /* KT_EP +KN_RIGHT */
	{VK_UP	  | VIRTUAL_KEY | EXTENDED_KEY,  {"\x00\xE0\x48\xff"}},  /* KT_EP +KN_UP */

	{VK_DELETE | VIRTUAL_KEY | EXTENDED_KEY | CTRL_KEY, {"\x00\xE0\x93\xff"}},	/* KT_CTRL + KT_EP +KN_DEL */
	{VK_DOWN   | VIRTUAL_KEY | EXTENDED_KEY | CTRL_KEY, {"\x00\xE0\x91\xff"}},	/* KT_CTRL + KT_EP +KN_DOWN */
	{VK_END	  | VIRTUAL_KEY | EXTENDED_KEY | CTRL_KEY,  {"\x00\xE0\x75\xff"}},	/* KT_CTRL + KT_EP +KN_END */
	{VK_HOME   | VIRTUAL_KEY | EXTENDED_KEY | CTRL_KEY, {"\x00\xE0\x77\xff"}},	/* KT_CTRL + KT_EP +KN_HOME */
	{VK_INSERT | VIRTUAL_KEY | EXTENDED_KEY | CTRL_KEY, {"\x00\xE0\x92\xff"}},	/* KT_CTRL + KT_EP +KN_INS */
	{VK_LEFT   | VIRTUAL_KEY | EXTENDED_KEY | CTRL_KEY, {"\x00\xE0\x73\xff"}},	/* KT_CTRL + KT_EP +KN_LEFT */
	{VK_NEXT   | VIRTUAL_KEY | EXTENDED_KEY | CTRL_KEY, {"\x00\xE0\x76\xff"}},	/* KT_CTRL + KT_EP +KN_PGDN */
	{VK_PRIOR  | VIRTUAL_KEY | EXTENDED_KEY | CTRL_KEY, {"\x00\xE0\x84\xff"}},	/* KT_CTRL + KT_EP +KN_PGUP */
	{VK_RIGHT  | VIRTUAL_KEY | EXTENDED_KEY | CTRL_KEY, {"\x00\xE0\x74\xff"}},	/* KT_CTRL + KT_EP +KN_RIGHT */
	{VK_UP	  | VIRTUAL_KEY | EXTENDED_KEY | CTRL_KEY,  {"\x00\xE0\x8D\xff"}},	/* KT_CTRL + KT_EP +KN_UP */

	{VK_DELETE | VIRTUAL_KEY | EXTENDED_KEY | ALT_KEY, {"\x00\xA3\xff"}},  /* KT_ALT + KT_EP +KN_DEL */
	{VK_DOWN   | VIRTUAL_KEY | EXTENDED_KEY | ALT_KEY, {"\x00\xA0\xff"}},  /* KT_ALT + KT_EP +KN_DOWN */
	{VK_END	  | VIRTUAL_KEY | EXTENDED_KEY | ALT_KEY,  {"\x00\x9F\xff"}},  /* KT_ALT + KT_EP +KN_END */
	{VK_HOME   | VIRTUAL_KEY | EXTENDED_KEY | ALT_KEY, {"\x00\x97\xff"}},  /* KT_ALT + KT_EP +KN_HOME */
	{VK_INSERT | VIRTUAL_KEY | EXTENDED_KEY | ALT_KEY, {"\x00\xA2\xff"}},  /* KT_ALT + KT_EP +KN_INS */
	{VK_LEFT   | VIRTUAL_KEY | EXTENDED_KEY | ALT_KEY, {"\x00\x9B\xff"}},  /* KT_ALT + KT_EP +KN_LEFT */
	{VK_NEXT   | VIRTUAL_KEY | EXTENDED_KEY | ALT_KEY, {"\x00\xA1\xff"}},  /* KT_ALT + KT_EP +KN_PGDN */
	{VK_PRIOR  | VIRTUAL_KEY | EXTENDED_KEY | ALT_KEY, {"\x00\x99\xff"}},  /* KT_ALT + KT_EP +KN_PGUP */
	{VK_RIGHT  | VIRTUAL_KEY | EXTENDED_KEY | ALT_KEY, {"\x00\x9D\xff"}},  /* KT_ALT + KT_EP +KN_RIGHT */
	{VK_UP	  | VIRTUAL_KEY | EXTENDED_KEY | ALT_KEY,  {"\x00\x98\xff"}},  /* KT_ALT + KT_EP +KN_UP */

	{0x32 | VIRTUAL_KEY | CTRL_KEY, {"\x00\x03\xff"}},	/* KT_CTRL + '@' (NUL) */
	};


/* for VT52 emulator */
const KEYTBLSTORAGE VT52KeyTable[MAX_VT52_KEYS] =
    {
	{VK_UP	  | EXTENDED_KEY | VIRTUAL_KEY,{"\x1B\x41\xff"}},  /* KN_UP */
	{VK_DOWN | EXTENDED_KEY | VIRTUAL_KEY,{"\x1B\x42\xff"}},  /* KN_DOWN */
	{VK_RIGHT| EXTENDED_KEY | VIRTUAL_KEY,{"\x1B\x43\xff"}},  /* KN_RIGHT */
	{VK_LEFT | EXTENDED_KEY | VIRTUAL_KEY,{"\x1B\x44\xff"}},  /* KN_LEFT */
	   
	{VK_UP	 | VIRTUAL_KEY,					{"\x1B\x41\xff"}},  /* KN_UP */
	{VK_DOWN | VIRTUAL_KEY,				{"\x1B\x42\xff"}},  /* KN_DOWN */
	{VK_RIGHT| VIRTUAL_KEY,				{"\x1B\x43\xff"}},  /* KN_RIGHT */
	{VK_LEFT | VIRTUAL_KEY,				{"\x1B\x44\xff"}},  /* KN_LEFT */

	{VK_F1	| VIRTUAL_KEY,					{"\x1BP\xff"}},  /* KN_F1 */
	{VK_F2	| VIRTUAL_KEY,					{"\x1BQ\xff"}},  /* KN_F2 */
	{VK_F3	| VIRTUAL_KEY,					{"\x1BR\xff"}},  /* KN_F3 */
	{VK_F4	| VIRTUAL_KEY,					{"\x1BS\xff"}},  /* KN_F4 */

	{VK_F1	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1BP\xff"}},  /* KN_F1 */
	{VK_F2	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1BQ\xff"}},  /* KN_F2 */
	{VK_F3	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1BR\xff"}},  /* KN_F3 */
	{VK_F4	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1BS\xff"}},  /* KN_F4 */

	{VK_DELETE	| VIRTUAL_KEY,				{"\x7F\xff"}}, 	/* KN_DEL */
	{VK_DELETE	| VIRTUAL_KEY | EXTENDED_KEY,{"\x7F\xff"}}, 	/* KN_DEL */

	{VK_ADD	| VIRTUAL_KEY,					{",\xff"}},
	};

const KEYTBLSTORAGE VT52_Keypad_KeyTable[MAX_VT52_KEYPAD_KEYS] =
    {
	{VK_NUMPAD0 | VIRTUAL_KEY,	 {"\x1B?p\xff"}}, /* KT_KP + '0' (alternate mode) */
	{VK_NUMPAD1 | VIRTUAL_KEY,	 {"\x1B?q\xff"}}, /* KT_KP + '1' (alternate mode) */
	{VK_NUMPAD2 | VIRTUAL_KEY,	 {"\x1B?r\xff"}}, /* KT_KP + '2' (alternate mode) */
	{VK_NUMPAD3 | VIRTUAL_KEY,	 {"\x1B?s\xff"}}, /* KT_KP + '3' (alternate mode) */
	{VK_NUMPAD4 | VIRTUAL_KEY,	 {"\x1B?t\xff"}}, /* KT_KP + '4' (alternate mode) */
	{VK_NUMPAD5 | VIRTUAL_KEY,	 {"\x1B?u\xff"}}, /* KT_KP + '5' (alternate mode) */
	{VK_NUMPAD6 | VIRTUAL_KEY,	 {"\x1B?v\xff"}}, /* KT_KP + '6' (alternate mode) */
	{VK_NUMPAD7 | VIRTUAL_KEY,	 {"\x1B?w\xff"}}, /* KT_KP + '7' (alternate mode) */
	{VK_NUMPAD8 | VIRTUAL_KEY,	 {"\x1B?x\xff"}}, /* KT_KP + '8' (alternate mode) */
	{VK_NUMPAD9 | VIRTUAL_KEY,	 {"\x1B?y\xff"}}, /* KT_KP + '9' (alternate mode) */
	{VK_DECIMAL | VIRTUAL_KEY,	 {"\x1B?n\xff"}}, /* KT_KP + '.' (alternate mode) */

	{VK_ADD		| VIRTUAL_KEY,	 {"\x1B?l\xff"}},	/* KT_KP + '+' (alternate mode) */
	{VK_RETURN	| EXTENDED_KEY,  {"\x1B?M\xff"}},	/* KT_KP + enter  (alternate mode) */
	{VK_SUBTRACT | VIRTUAL_KEY,	 {"\x1B?m\xff"}},	/* KT_KP + '-' (alternate mode) */
	};						  

const KEYTBLSTORAGE VT_PF_KeyTable[MAX_VT_PF_KEYS] =
    {
	{VK_NUMLOCK	| VIRTUAL_KEY | EXTENDED_KEY, {"\x1BP\xff"}},	/* KT_KP + NUMLOCK */
	{0x2F		| EXTENDED_KEY, 			  {"\x1BQ\xff"}},	/* KT_KP + '/' */
	{VK_MULTIPLY | VIRTUAL_KEY,				  {"\x1BR\xff"}},	/* KT_KP + '*' */
	{VK_SUBTRACT | VIRTUAL_KEY,				  {"\x1BS\xff"}},	/* KT_KP + '-' */
	};

/* for VT100 emulator */

const KEYTBLSTORAGE VT100KeyTable[MAX_VT100_KEYS] =
    {
	{VK_UP	 | VIRTUAL_KEY,				{"\x1B[A\xff"}},  /* KN_UP */
	{VK_DOWN | VIRTUAL_KEY,				{"\x1B[B\xff"}},  /* KN_DOWN */
	{VK_RIGHT| VIRTUAL_KEY,				{"\x1B[C\xff"}},  /* KN_RIGHT */
	{VK_LEFT | VIRTUAL_KEY,				{"\x1B[D\xff"}},  /* KN_LEFT */

	{VK_UP	 | EXTENDED_KEY | VIRTUAL_KEY,	{"\x1B[A\xff"}},  /* KN_UP */
	{VK_DOWN | EXTENDED_KEY | VIRTUAL_KEY,{"\x1B[B\xff"}},  /* KN_DOWN */
	{VK_RIGHT| EXTENDED_KEY | VIRTUAL_KEY,{"\x1B[C\xff"}},  /* KN_RIGHT */
	{VK_LEFT | EXTENDED_KEY | VIRTUAL_KEY,{"\x1B[D\xff"}},  /* KN_LEFT */

	{VK_F1	| VIRTUAL_KEY,					{"\x1BOP\xff"}},  /* KN_F1 */
	{VK_F2	| VIRTUAL_KEY,					{"\x1BOQ\xff"}},  /* KN_F2 */
	{VK_F3	| VIRTUAL_KEY,					{"\x1BOR\xff"}},  /* KN_F3 */
	{VK_F4	| VIRTUAL_KEY,					{"\x1BOS\xff"}},  /* KN_F4 */

	{VK_F1	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1BOP\xff"}},  /* KN_F1 */
	{VK_F2	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1BOQ\xff"}},  /* KN_F2 */
	{VK_F3	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1BOR\xff"}},  /* KN_F3 */
	{VK_F4	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1BOS\xff"}},  /* KN_F4 */

	{VK_DELETE | VIRTUAL_KEY,				{"\x7F\xff"}},	/* KN_DEL */
	{VK_DELETE | VIRTUAL_KEY | EXTENDED_KEY,{"\x7F\xff"}},	/* KN_DEL */

	{VK_ADD	| VIRTUAL_KEY,							{",\xff"}},

	{VK_SPACE | VIRTUAL_KEY | CTRL_KEY,				{"\x00\xff"}}, 	  /* CTRL + SPACE */
	{0x32   | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x00\xff"}}, 	/* CTRL + @ */
	{0x32	| VIRTUAL_KEY | CTRL_KEY,				{"\x00\xff"}}, 	/* CTRL + 2 */
	{0x36	| VIRTUAL_KEY | CTRL_KEY,				{"\x1e\xff"}},	/* CTRL + 6 */
	{0xbd	| VIRTUAL_KEY | CTRL_KEY,				{"\x1f\xff"}},	/* CTRL + - */
	};

const KEYTBLSTORAGE VT100_Cursor_KeyTable[MAX_VT100_CURSOR_KEYS] =
    {
	{VK_UP	 | EXTENDED_KEY | VIRTUAL_KEY,	{"\x1BOA\xff"}},  /* KN_UP */
	{VK_DOWN | EXTENDED_KEY | VIRTUAL_KEY,{"\x1BOB\xff"}},  /* KN_DOWN */
	{VK_RIGHT| EXTENDED_KEY | VIRTUAL_KEY,{"\x1BOC\xff"}},  /* KN_RIGHT */
	{VK_LEFT | EXTENDED_KEY | VIRTUAL_KEY,{"\x1BOD\xff"}},  /* KN_LEFT */

	{VK_UP	 | VIRTUAL_KEY,				{"\x1BOA\xff"}},  /* KN_UP */
	{VK_DOWN | VIRTUAL_KEY,				{"\x1BOB\xff"}},  /* KN_DOWN */
	{VK_RIGHT| VIRTUAL_KEY,				{"\x1BOC\xff"}},  /* KN_RIGHT */
	{VK_LEFT | VIRTUAL_KEY,				{"\x1BOD\xff"}},  /* KN_LEFT */
	};

const KEYTBLSTORAGE VT100_Keypad_KeyTable[MAX_VT100_KEYPAD_KEYS] =
    {
	{VK_NUMPAD0 | VIRTUAL_KEY,	 {"\x1BOp\xff"}}, /* KT_KP + '0' (alternate mode) */
	{VK_NUMPAD1 | VIRTUAL_KEY,	 {"\x1BOq\xff"}}, /* KT_KP + '1' (alternate mode) */
	{VK_NUMPAD2 | VIRTUAL_KEY,	 {"\x1BOr\xff"}}, /* KT_KP + '2' (alternate mode) */
	{VK_NUMPAD3 | VIRTUAL_KEY,	 {"\x1BOs\xff"}}, /* KT_KP + '3' (alternate mode) */
	{VK_NUMPAD4 | VIRTUAL_KEY,	 {"\x1BOt\xff"}}, /* KT_KP + '4' (alternate mode) */
	{VK_NUMPAD5 | VIRTUAL_KEY,	 {"\x1BOu\xff"}}, /* KT_KP + '5' (alternate mode) */
	{VK_NUMPAD6 | VIRTUAL_KEY,	 {"\x1BOv\xff"}}, /* KT_KP + '6' (alternate mode) */
	{VK_NUMPAD7 | VIRTUAL_KEY,	 {"\x1BOw\xff"}}, /* KT_KP + '7' (alternate mode) */
	{VK_NUMPAD8 | VIRTUAL_KEY,	 {"\x1BOx\xff"}}, /* KT_KP + '8' (alternate mode) */
	{VK_NUMPAD9 | VIRTUAL_KEY,	 {"\x1BOy\xff"}}, /* KT_KP + '9' (alternate mode) */
	{VK_DECIMAL | VIRTUAL_KEY,	 {"\x1BOn\xff"}}, /* KT_KP + '.' (alternate mode) */

	{VK_ADD		| VIRTUAL_KEY,	 {"\x1BOl\xff"}}, /* KT_KP + '*' (alternate mode) */
	{VK_RETURN	| EXTENDED_KEY,  {"\x1BOM\xff"}}, /* KT_KP + '+' (alternate mode) */
	{VK_SUBTRACT | VIRTUAL_KEY,	 {"\x1BOm\xff"}}, /* KT_KP + '-' (alternate mode) */
	};

#if defined(INCL_MINITEL)
const KEYTBLSTORAGE Minitel_KeyTable[MAX_MINITEL_KEYS] =
	{
	{0x4D		| VIRTUAL_KEY	| CTRL_KEY, 	{"\x0D\xff"}},  /* ctrl-m */

	{VK_RETURN  | VIRTUAL_KEY,					{"\x13\x41\xff"}}, /* Envoi CNTRL-MA*/
	{VK_RETURN	| EXTENDED_KEY,					{"\x13\x41\xff"}}, /* Send CNTRL-MA*/
	{VK_TAB		| VIRTUAL_KEY,					{"\x13\x41\xff"}}, /* Send CNTRL-MA*/
	{VK_F8		| VIRTUAL_KEY,					{"\x13\x41\xff"}}, /* Send CNTRL-MA*/

	{VK_HOME 	| VIRTUAL_KEY,					{"\x13\x46"}}, /* Sommaire CNTRL-MF*/
	{VK_HOME 	| VIRTUAL_KEY	| EXTENDED_KEY, {"\x13\x46\xff"}}, /* Index CNTRL-MF*/
	{VK_F1		| VIRTUAL_KEY,					{"\x13\x46\xff"}}, /* Index CNTRL-MF*/

	{VK_DELETE	| VIRTUAL_KEY,					{"\x13\x45\xff"}}, /* Annulation CNTRL-ME*/
	{VK_DELETE	| VIRTUAL_KEY	| EXTENDED_KEY, {"\x13\x45\xff"}}, /* Cancel CNTRL-ME*/
	{VK_F2		| VIRTUAL_KEY,					{"\x13\x45\xff"}}, /* Cancel CNTRL-ME*/

	{VK_PRIOR	| VIRTUAL_KEY,					{"\x13\x42\xff"}}, /* Retour CNTRL-MB*/
	{VK_PRIOR	| VIRTUAL_KEY	| EXTENDED_KEY, {"\x13\x42\xff"}}, /* Previous CNTRL-MB*/
	{VK_F3		| VIRTUAL_KEY,					{"\x13\x42\xff"}}, /* Previous CNTRL-MB*/

	{VK_INSERT	| VIRTUAL_KEY,					{"\x13\x43\xff"}}, /* Repeat CNTRL-MC*/
	{VK_INSERT	| VIRTUAL_KEY	| EXTENDED_KEY, {"\x13\x43\xff"}}, /* Repeat CNTRL-MC*/
	{VK_F4		| VIRTUAL_KEY,					{"\x13\x43\xff"}}, /* Repeat CNTRL-MC*/
															
	{VK_BACK 	| VIRTUAL_KEY,					{"\x13\x47\xff"}}, /* Correct CNTRL-MG*/
	{VK_F6		| VIRTUAL_KEY,					{"\x13\x47\xff"}}, /* Correct CNTRL-MG*/

	{VK_INSERT	| VIRTUAL_KEY,					{"\x13\x44\xff"}}, /* Guide CNTRL-MD*/
	{VK_INSERT	| VIRTUAL_KEY	| EXTENDED_KEY, {"\x13\x44\xff"}}, /* Guide CNTRL-MD*/
	{VK_F5		| VIRTUAL_KEY,					{"\x13\x44\xff"}}, /* Guide CNTRL-MD*/

	{VK_NEXT 	| VIRTUAL_KEY,					{"\x13\x48\xff"}}, /* Suite CNTRL-MH*/
	{VK_NEXT 	| VIRTUAL_KEY	| EXTENDED_KEY, {"\x13\x48\xff"}}, /* Next CNTRL-MH*/
	{VK_F7		| VIRTUAL_KEY,					{"\x13\x48\xff"}}, /* Next CNTRL-MH*/

	{VK_F9		| VIRTUAL_KEY,					{"\x13\x49\xff"}}, /* Connect (page 123) */

	// Page 124

	{VK_UP		| VIRTUAL_KEY,								   {"\x1B[A\xff"}},
	{VK_UP		| VIRTUAL_KEY | EXTENDED_KEY,				   {"\x1B[A\xff"}},
	
	{VK_UP		| VIRTUAL_KEY | SHIFT_KEY,					   {"\x1B[M\xff"}},
	{VK_UP		| VIRTUAL_KEY | EXTENDED_KEY | SHIFT_KEY,	   {"\x1B[M\xff"}},
																			
	{VK_DOWN 	| VIRTUAL_KEY,								   {"\x1B[B\xff"}},
	{VK_DOWN 	| VIRTUAL_KEY | EXTENDED_KEY,				   {"\x1B[B\xff"}},

	{VK_DOWN 	| VIRTUAL_KEY | SHIFT_KEY,					   {"\x1B[L\xff"}},
	{VK_DOWN 	| VIRTUAL_KEY | SHIFT_KEY | EXTENDED_KEY,	   {"\x1B[L\xff"}},

	{VK_RIGHT	| VIRTUAL_KEY,								   {"\x1B[C\xff"}},
	{VK_RIGHT	| VIRTUAL_KEY | EXTENDED_KEY,				   {"\x1B[C\xff"}},

	/* See minitel kbdin routine on this one - mrw */
	{VK_RIGHT	| VIRTUAL_KEY | SHIFT_KEY,					   {"\x1B[4\xff"}},
	{VK_RIGHT	| VIRTUAL_KEY | SHIFT_KEY | EXTENDED_KEY,	   {"\x1B[4\xff"}},

	{VK_LEFT 	| VIRTUAL_KEY,								   {"\x1B[D\xff"}},
	{VK_LEFT 	| VIRTUAL_KEY | EXTENDED_KEY,				   {"\x1B[D\xff"}},

	{VK_LEFT 	| VIRTUAL_KEY | SHIFT_KEY,					   {"\x1B[P\xff"}},
	{VK_LEFT 	| VIRTUAL_KEY | SHIFT_KEY | EXTENDED_KEY,	   {"\x1B[P\xff"}},
	{VK_LEFT 	| VIRTUAL_KEY | CTRL_KEY,					   {"\x7F\xff"}},	
	{VK_LEFT 	| VIRTUAL_KEY | CTRL_KEY | EXTENDED_KEY,	   {"\x7F\xff"}},	

	/* numpad enter */
	{VK_RETURN	| VIRTUAL_KEY | EXTENDED_KEY,				   {"\x13\x41\xff"}},
	{VK_RETURN	| VIRTUAL_KEY | EXTENDED_KEY | SHIFT_KEY,	   {"\x1B[H\xff"}},
	{VK_RETURN	| VIRTUAL_KEY | EXTENDED_KEY | CTRL_KEY,	   {"\x1B[2J\xff"}},

	// page 118

	{0xA3,	{"\x19\x23\xff"}},		// British pound symbol.
	{0xA7,	{"\x19\x27\xff"}},		// Paragraph symbol.
	{0xA8,	{"\x19\x48\xff"}},		// umluot

	{0xB0,	{"\x19\x30\xff"}},		// degree symbol
	{0xB1,	{"\x19\x31\xff"}},		// plus over minus symbol
	{0xB4,	{"\x19\x42\xff"}},		// accute accent
	{0xBC,	{"\x19\x3C\xff"}},		// 1/4
	{0xBD,	{"\x19\x3D\xff"}},		// 1/2
	{0xBE,	{"\x19\x3E\xff"}},		// 3/4
	{0xB8,	{"\x19\x4B\xff"}},		// beard

	{0xC0,	{"\x19\x2D\xff"}},		// up arrow symbol
	{0xC3,	{"\x19\x2C\xff"}},		// left arrow symbol
	{0xC4,	{"\x19\x2E\xff"}},		// right arrow symbol
	{0xC5,	{"\x19\x2F\xff"}},		// down arrow symbol

	{0xDF,	{"\x19\x7B\xff"}},		// Beta

	{0xE0,	{"\x19\x41\x61\xff"}},	// a grave accent
	{0xE2,	{"\x19\x43\x61\xff"}},	// a circumflex
	{0xE4,	{"\x19\x48\x61\xff"}},	// a umluot
	{0xE7,	{"\x19\x4B\x63\xff"}},	// c with a beard
	{0xE8,	{"\x19\x41\x65\xff"}},	// e grave accent
	{0xE9,	{"\x19\x42\x65\xff"}},	// e accute accent
	{0xEA,	{"\x19\x43\x65\xff"}},	// e circumflex
	{0xEB,	{"\x19\x48\x65\xff"}},	// e umluot
	{0xEE,	{"\x19\x43\x69\xff"}},	// i circumflex
	{0xEF,	{"\x19\x48\x69\xff"}},	// i umluot

	{0xF4,	{"\x19\x43\x6f\xff"}},	// o circumflex
	{0xF5,	{"\x19\x48\x6f\xff"}},	// o umluot
	{0xF7,	{"\x19\x38\xff"}},		// divide-by symbol
	{0xF9,	{"\x19\x41\x75\xff"}},	// u grave accent
	{0xFB,	{"\x19\x43\x75\xff"}},	// u circumflex
	{0xFC,	{"\x19\x48\x75\xff"}},	// u circumflex

	{0x5E,	{"\x19\x43\xff"}},		// circumflex accent (^)
	{0x60,	{"\x19\x41\xff"}},		// grave accent(`)
	{0x8C,	{"\x19\x6A\xff"}},		// big OE
	{0x9C,	{"\x19\x7A\xff"}},		// little oe
	};		
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuCreateHdl
 *
 * DESCRIPTION:
 *	Creates an emulator handle.  The creation of an Emulator handle
 *	includes setting default user settings and loading the ANSI
 *	emulator.
 *
 * ARGUMENTS:
 *	hSession - session handle.
 *
 * RETURNS:
 *	HEMU or zero on error.
 *
 */
HEMU emuCreateHdl(const HSESSION hSession)
	{
	HHEMU hhEmu;

	hhEmu = malloc(sizeof(*hhEmu));

	if (hhEmu == 0)
		{
		assert(FALSE);
		return 0;
		}

	memset(hhEmu, 0, sizeof(*hhEmu));

	InitializeCriticalSection(&hhEmu->csEmu);

	hhEmu->hSession = hSession;

	// Create and load the Emulator Name & Id table.
	//
	if (!emuCreateNameTable(hhEmu))
		{
		assert(FALSE);
		emuDestroyHdl((HEMU)hhEmu);
		hhEmu = NULL;
		return 0;
		}

	// Create the text and attribute buffers.
	//
	if (!emuCreateTextAttrBufs((HEMU)hhEmu, MAX_EMUROWS, MAX_EMUCOLS))
		{
		assert(FALSE);
		emuDestroyHdl((HEMU)hhEmu);
		hhEmu = NULL;
		return 0;
		}

	// Create the Print handle used for Printer Echo.
	//
	hhEmu->hPrintEcho = printCreateHdl(hSession);
	if(hhEmu->hPrintEcho == 0)
		{
		assert(FALSE);
		emuDestroyHdl((HEMU)hhEmu);
		hhEmu = NULL;
		return 0;
		}

	// Create the Print handle used for Host directed printing.
	//
	hhEmu->hPrintHost = printCreateHdl(hSession);
	if (hhEmu->hPrintHost == 0)
		{
		assert(FALSE);
		emuDestroyHdl((HEMU)hhEmu);
		hhEmu = NULL;
		return 0;
		}

	// Initialize the user settings for the emulation handle.
	//
	if (emuInitializeHdl((HEMU)hhEmu) != 0)
		{
		assert(FALSE);
		emuDestroyHdl((HEMU)hhEmu);
		hhEmu = NULL;
		return 0;
		}

	return (HEMU)hhEmu;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuDestroyHdl
 *
 * DESCRIPTION:
 *	Death and destruction of the once noble emulator handle.
 *
 * ARGUMENTS:
 *	hEmu	- external emulator handle.
 *
 * RETURNS:
 *	0=OK, else error
 *
 */
int emuDestroyHdl(const HEMU hEmu)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hhEmu == 0)
		{
		assert(0);
		return -1;
		}

	if (hhEmu->pstNameTable)
		{
		free(hhEmu->pstNameTable);
		hhEmu->pstNameTable = NULL;
		}

	printDestroyHdl(hhEmu->hPrintEcho);
	hhEmu->hPrintEcho = NULL;
	printDestroyHdl(hhEmu->hPrintHost);
	hhEmu->hPrintHost = NULL;
	emuDestroyTextAttrBufs(hEmu);
	DeleteCriticalSection(&hhEmu->csEmu);

	if(hhEmu)
		free(hhEmu);

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuInitializeHdl
 *
 * DESCRIPTION:
 *	This function initializes the stUserSettings structure of the internal
 *	emulator handle with the values stored in the session file.
 *
 * ARGUMENTS:
 *	HEMU	-	The External Emulator Handle.
 *
 * RETURNS:
 *	0 if successful, otherwise -1
 *
 */
int emuInitializeHdl(const HEMU hEmu)
	{
	int nRet, nReturn;
	struct stEmuBaseSFSettings stBaseSFSettings;

	const HHEMU hhEmu = (HHEMU)hEmu;

	nReturn = -1;

	if (hhEmu == 0)
		{
		assert(FALSE);
		return nReturn;
		}

	emuLock(hEmu);

	hhEmu->emu_maxcol		= EMU_DEFAULT_MAXCOL;
	hhEmu->emu_maxrow		= EMU_DEFAULT_MAXROW;
	hhEmu->bottom_margin	= EMU_DEFAULT_MAXROW;

	hhEmu->mode_vt220 = FALSE;
	hhEmu->mode_vt320 = FALSE;

	// Added for the VT220/320 rde 16 Feb 1998
	hhEmu->fUse8BitCodes = FALSE;
	hhEmu->fAllowUserKeys = FALSE;

	hhEmu->attrState[0].txtclr =
	hhEmu->attrState[1].txtclr = VC_WHITE;

	hhEmu->attrState[0].bkclr =
	hhEmu->attrState[1].bkclr = VC_BLACK;

	hhEmu->iCurAttrState = CS_STATE;

	std_setcolors(hhEmu, GetNearestColorIndex(GetSysColor(COLOR_WINDOWTEXT)),
							GetNearestColorIndex(GetSysColor(COLOR_WINDOW)));

	// Initialize the user setting default values.
	//
	memset(&stBaseSFSettings, 0, sizeof(stBaseSFSettings));

	stBaseSFSettings.nTermKeys 			= EMU_KEYS_TERM;
	stBaseSFSettings.nCursorType		= EMU_CURSOR_LINE;
	stBaseSFSettings.fCursorBlink		= TRUE;
	stBaseSFSettings.nCharacterSet 		= EMU_CHARSET_ASCII;
	stBaseSFSettings.fMapPFkeys			= FALSE;
	stBaseSFSettings.fAltKeypadMode		= FALSE;
	stBaseSFSettings.fKeypadAppMode		= FALSE;
	stBaseSFSettings.fCursorKeypadMode	= FALSE;
	stBaseSFSettings.fReverseDelBk 		= FALSE;
	stBaseSFSettings.f132Columns		= FALSE;
	stBaseSFSettings.fWrapLines			= TRUE;
 	stBaseSFSettings.fDestructiveBk		= TRUE;
	stBaseSFSettings.fLbSymbolOnEnter	= FALSE;
    stBaseSFSettings.fUse8BitCodes		= FALSE; 
    stBaseSFSettings.fAllowUserKeys		= FALSE;  
	stBaseSFSettings.nEmuId				= EMU_AUTO;
#if defined(INCL_ULTC_VERSION)
	stBaseSFSettings.nEmuId				= EMU_VT220;
	stBaseSFSettings.nCharacterSet 		= EMU_CHARSET_MULTINATIONAL;
#endif
	stBaseSFSettings.nAutoAttempts		= 0;
#ifdef INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
    // Initialize new user settings. - cab:11/14/96
    //
    stBaseSFSettings.nBackspaceKeys		= EMU_BKSPKEYS_CTRLH;

    // Zero the telnet id string. I am doing this because if there
    // isn't one in the session file, I want to load the default.
    //
    stBaseSFSettings.acTelnetId[0]		= TEXT('\0');

    emuQueryDefaultTelnetId(stBaseSFSettings.nEmuId,
			stBaseSFSettings.acTelnetId, EMU_MAX_TELNETID);
#endif

	// Load the settings if they exist.
	if ((nRet = emuLoadSettings(hhEmu, 
								SFID_EMU_SETTINGS,
								sizeof(stBaseSFSettings),
								&stBaseSFSettings)) == -1)
		{
		nReturn = -1;
		goto InitExit;
		}

	if (nRet > 0)
		{
		// The data was present.
        // Check to see if there is a telnet id. If not, use the default.
        //
		emuCheckSettings(&stBaseSFSettings); 

#ifdef INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
        if ( stBaseSFSettings.acTelnetId[0] == TEXT('\0') )
            {
            emuQueryDefaultTelnetId(stBaseSFSettings.nEmuId,
					stBaseSFSettings.acTelnetId, EMU_MAX_TELNETID);
            }
#endif
		}

	// Initialize the emu's user settings structure from the data stored
	// in the session file.
	hhEmu->stUserSettings.nEmuId = stBaseSFSettings.nEmuId;
	hhEmu->stUserSettings.nTermKeys = stBaseSFSettings.nTermKeys;	
	hhEmu->stUserSettings.nCursorType = stBaseSFSettings.nCursorType;
	hhEmu->stUserSettings.nCharacterSet = stBaseSFSettings.nCharacterSet;
	hhEmu->stUserSettings.nAutoAttempts = stBaseSFSettings.nAutoAttempts;
	hhEmu->stUserSettings.fCursorBlink = stBaseSFSettings.fCursorBlink;
	hhEmu->stUserSettings.fMapPFkeys = stBaseSFSettings.fMapPFkeys;
	hhEmu->stUserSettings.fAltKeypadMode = stBaseSFSettings.fAltKeypadMode;
	hhEmu->stUserSettings.fKeypadAppMode = stBaseSFSettings.fKeypadAppMode;
	hhEmu->stUserSettings.fCursorKeypadMode = stBaseSFSettings.fCursorKeypadMode;
	hhEmu->stUserSettings.fReverseDelBk = stBaseSFSettings.fReverseDelBk;
	hhEmu->stUserSettings.f132Columns = stBaseSFSettings.f132Columns;
	hhEmu->stUserSettings.fDestructiveBk = stBaseSFSettings.fDestructiveBk;
	hhEmu->stUserSettings.fWrapLines = stBaseSFSettings.fWrapLines;
	hhEmu->stUserSettings.fLbSymbolOnEnter = stBaseSFSettings.fLbSymbolOnEnter;
    hhEmu->stUserSettings.fUse8BitCodes = stBaseSFSettings.fUse8BitCodes;	
    hhEmu->stUserSettings.fAllowUserKeys = stBaseSFSettings.fAllowUserKeys;
    hhEmu->stUserSettings.nBackspaceKeys = stBaseSFSettings.nBackspaceKeys;

    StrCharCopy(hhEmu->stUserSettings.acTelnetId, stBaseSFSettings.acTelnetId);

#ifdef INCL_TERMINAL_SIZE_AND_COLORS
	// Set up the default colors in case the user settings don't exist.
	#if TRUE  
	hhEmu->stUserSettings.nTextColor = 
			GetNearestColorIndex(GetSysColor(COLOR_WINDOWTEXT));
	hhEmu->stUserSettings.nBackgroundColor =
			GetNearestColorIndex(GetSysColor(COLOR_WINDOW));
	#else
	// We've decided not to do this. rde 14 Jul 98
	// Use white (non-bold) on black. rde 8 Jul 98
	hhEmu->stUserSettings.nBackgroundColor = VC_BLACK;
	hhEmu->stUserSettings.nTextColor = VC_WHITE;
	#endif

	// Load the settings if they exist.
	if ((nRet = emuLoadSettings(hhEmu, 
						SFID_EMU_TEXTCOLOR_SETTING,
						sizeof(hhEmu->stUserSettings.nTextColor),
						&hhEmu->stUserSettings.nTextColor)) == -1)
		{
		nReturn = -1;
		goto InitExit;
		}

	if (nRet > 0)
		{
		// The data was present, so use it.
		std_setcolors(hhEmu, 
				hhEmu->stUserSettings.nTextColor,
				hhEmu->stUserSettings.nBackgroundColor);
		}

	if ((nRet = emuLoadSettings(hhEmu, 
						SFID_EMU_BKGRNDCOLOR_SETTING,
						sizeof(hhEmu->stUserSettings.nBackgroundColor),
						&hhEmu->stUserSettings.nBackgroundColor)) == -1)
		{
		nReturn = -1;
		goto InitExit;
		}

	if (nRet > 0)
		{
		// The data was present, so use it.
		std_setcolors(hhEmu, 
				hhEmu->stUserSettings.nTextColor,
				hhEmu->stUserSettings.nBackgroundColor);
		}

	// Set up the default screen size in case the user settings don't exist.
	hhEmu->stUserSettings.nUserDefRows = 24;
	hhEmu->stUserSettings.nUserDefCols = 
			hhEmu->stUserSettings.f132Columns ? 132 : 80;

	// Load the settings if they exist.
	if ((nRet = emuLoadSettings(hhEmu, 
						SFID_EMU_SCRNROWS_SETTING,
						sizeof(hhEmu->stUserSettings.nUserDefRows),
						&hhEmu->stUserSettings.nUserDefRows)) == -1)
		{
		nReturn = -1;
		goto InitExit;
		}

	if ((nRet = emuLoadSettings(hhEmu, 
						SFID_EMU_SCRNCOLS_SETTING,
						sizeof(hhEmu->stUserSettings.nUserDefCols),
						&hhEmu->stUserSettings.nUserDefCols)) == -1)
		{
		nReturn = -1;
		goto InitExit;
		}

	if (nRet > 0)
		{
		// The data was present, so use it.
		hhEmu->stUserSettings.f132Columns = 
				(hhEmu->stUserSettings.nUserDefCols == 132) ? TRUE : FALSE;

#if FALSE	// TODO:rde Is this needed?
		hhEmu->emu_setscrsize = std_setscrsize;
#endif
		}
#endif

	// Load the emulator.
	//
	nReturn = emuLoad((HEMU)hhEmu, hhEmu->stUserSettings.nEmuId);

    //JMH 01-09-97 emuSetSettings() was being called before emuLoad(), but
    // emuLoad() initialized some values that got set by emuSetSettings().
    // (e.g. 132-column mode).
    //
	// Tell the emulator about the user's settings.
	//
	emuSetSettings((HEMU)hhEmu, &hhEmu->stUserSettings);

	// Clear the emulator image.
	//
	//*for (nRow = 0 ; nRow < MAX_EMUROWS ; ++nRow)
	//*    clear_imgrow(hhEmu, nRow);

	//* This breaks the MINITEL bad	- mrw
	//*(hhEmu->emu_setcurpos)(hhEmu, 0, 0);

	InitExit:

	emuUnlock(hEmu);

	NotifyClient(hhEmu->hSession, EVENT_EMU_SETTINGS, 0);
	NotifyClient(hhEmu->hSession, EVENT_TERM_UPDATE, 0);

	if (nReturn < 0)
		nRet = 0;

	return nReturn;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuSaveHdl
 *
 * DESCRIPTION:
 *	This function stores the emulator user settings in the session file.
 *
 *
 * ARGUMENTS:
 *	HEMU	-	The External Emulator Handle.
 *
 * RETURNS:
 *	0=OK, else error
 *
 */
int emuSaveHdl(const HEMU hEmu)
	{
	struct stEmuBaseSFSettings stBaseSFSettings;
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hhEmu == 0)
		{
		assert(FALSE);
		return -1;
		}

	// Initialize the base structure that gets saved in the session file.
	stBaseSFSettings.nEmuId = hhEmu->stUserSettings.nEmuId;
	stBaseSFSettings.nTermKeys = hhEmu->stUserSettings.nTermKeys;		
	stBaseSFSettings.nCursorType = hhEmu->stUserSettings.nCursorType;
	stBaseSFSettings.nCharacterSet = hhEmu->stUserSettings.nCharacterSet;
	stBaseSFSettings.nAutoAttempts = hhEmu->stUserSettings.nAutoAttempts;
	stBaseSFSettings.fCursorBlink = hhEmu->stUserSettings.fCursorBlink;
	stBaseSFSettings.fMapPFkeys = hhEmu->stUserSettings.fMapPFkeys;
	stBaseSFSettings.fAltKeypadMode = hhEmu->stUserSettings.fAltKeypadMode;
	stBaseSFSettings.fKeypadAppMode = hhEmu->stUserSettings.fKeypadAppMode;
	stBaseSFSettings.fCursorKeypadMode = hhEmu->stUserSettings.fCursorKeypadMode;
	stBaseSFSettings.fReverseDelBk = hhEmu->stUserSettings.fReverseDelBk;
	stBaseSFSettings.f132Columns = hhEmu->stUserSettings.f132Columns;
	stBaseSFSettings.fDestructiveBk = hhEmu->stUserSettings.fDestructiveBk;
	stBaseSFSettings.fWrapLines = hhEmu->stUserSettings.fWrapLines;
	stBaseSFSettings.fLbSymbolOnEnter = hhEmu->stUserSettings.fLbSymbolOnEnter;
    stBaseSFSettings.fUse8BitCodes = hhEmu->stUserSettings.fUse8BitCodes;
    stBaseSFSettings.fAllowUserKeys = hhEmu->stUserSettings.fAllowUserKeys;
    stBaseSFSettings.nBackspaceKeys = hhEmu->stUserSettings.nBackspaceKeys;

    StrCharCopy(stBaseSFSettings.acTelnetId, hhEmu->stUserSettings.acTelnetId);

	emuLock(hEmu);

	sfPutSessionItem(sessQuerySysFileHdl(hhEmu->hSession),
						SFID_EMU_SETTINGS,
						sizeof(stBaseSFSettings),
						&stBaseSFSettings);

#ifdef INCL_TERMINAL_SIZE_AND_COLORS
	sfPutSessionItem(sessQuerySysFileHdl(hhEmu->hSession),
						SFID_EMU_TEXTCOLOR_SETTING,
						sizeof(hhEmu->stUserSettings.nTextColor),
						&hhEmu->stUserSettings.nTextColor);

	sfPutSessionItem(sessQuerySysFileHdl(hhEmu->hSession),
						SFID_EMU_BKGRNDCOLOR_SETTING,
						sizeof(hhEmu->stUserSettings.nBackgroundColor),
						&hhEmu->stUserSettings.nBackgroundColor);

	sfPutSessionItem(sessQuerySysFileHdl(hhEmu->hSession),
						SFID_EMU_SCRNROWS_SETTING,
						sizeof(hhEmu->stUserSettings.nUserDefRows),
						&hhEmu->stUserSettings.nUserDefRows);

	sfPutSessionItem(sessQuerySysFileHdl(hhEmu->hSession),
						SFID_EMU_SCRNCOLS_SETTING,
						sizeof(hhEmu->stUserSettings.nUserDefCols),
						&hhEmu->stUserSettings.nUserDefCols);
#endif

	emuUnlock(hEmu);

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuLock
 *
 * DESCRIPTION:
 *	Grabs the emulator's critical section semaphore.  Access functions
 *	to the emulator should also call this so we can call emulator
 *	functions from anywhere.
 *
 * ARGUMENTS:
 *	hEmu	- external emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void emuLock(const HEMU hEmu)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;
	EnterCriticalSection(&hhEmu->csEmu);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuUnlock
 *
 * DESCRIPTION:
 *	Releases the emulator's critical section semaphore.
 *
 * ARGUMENTS:
 *	hEmu	- external emulator handle.
 *
 * RETURNS:
 *	void
 *
 */
void emuUnlock(const HEMU hEmu)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;
	LeaveCriticalSection(&hhEmu->csEmu);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuGetTxtBuf
 *
 * DESCRIPTION:
 *	Obviously we don't need the handle here but when we do go reentrant,
 *	we only change the guts of this function.  Also, I don't call
 *	emuLock(), emuUnlock() since only termGetUpdate() calls these functions
 *	and at that point the emulator is already locked down.
 *
 * ARGUMENTS:
 *	hEmu	- external emulator handle
 *
 * RETURNS:
 *	pointer to text buf array
 *
 */
ECHAR **emuGetTxtBuf(const HEMU hEmu)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hEmu == 0)
		{
		assert(0);
		return 0;
		}

	return hhEmu->emu_apText;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuGetAttrBuf
 *
 * DESCRIPTION:
 *	Obviously we don't need the handle here but when we do go reentrant,
 *	we only change the guts of this function.  Also, I don't call
 *	emuLock(), emuUnlock() since only termGetUpdate() calls these functions
 *	and at that point the emulator is already locked down.
 *
 * ARGUMENTS:
 *	hEmu	- external emulator handle
 *
 * RETURNS:
 *	pointer to attribute buf array
 *
 */
PSTATTR *emuGetAttrBuf(const HEMU hEmu)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hEmu == 0)
		{
		assert(0);
		return 0;
		}

	return hhEmu->emu_apAttr;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuKbdIn
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *	0=termkey, -1=not termkey, -2=error
 *
 */
int emuKbdIn(const HEMU hEmu, KEY_T key, const int fTest)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;
	int iRet;

	if (hEmu == 0)
		{
		assert(0);
		return -2;
		}

	emuLock(hEmu);
	iRet = (*hhEmu->emu_kbdin)(hhEmu, (int)key, fTest);
	emuUnlock(hEmu);
	return iRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuIsEmuKey
 *
 * DESCRIPTION:
 *	Checks to see if the the given key maps to any defined terminal keys.
 *
 * ARGUMENTS:
 *	HEMU hEmu	- External emulator handle.
 *	int key 	- Key to test.
 *
 * RETURNS:
 *	TRUE=termkey, FALSE=not termkey
 *
 */
int emuIsEmuKey(const HEMU hEmu, KEY_T key)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;
	int iRet;

	if (hEmu == 0)
		{
		assert(0);
		return FALSE;
		}

	if (cnctQueryStatus(sessQueryCnctHdl(hhEmu->hSession)) !=
			CNCT_STATUS_TRUE || IsSessionSuspended(hhEmu->hSession) ||
				hhEmu->stUserSettings.nTermKeys == EMU_KEYS_ACCEL)
		{
		return FALSE;
		}

	emuLock(hEmu);
	iRet = (*hhEmu->emu_kbdin)(hhEmu, (int)key, TRUE);
	emuUnlock(hEmu);
	return (iRet == -1) ? FALSE : TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * emuComDone
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *	0=OK, else error
 *
 */
int emuComDone(const HEMU hEmu)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hEmu == 0)
		{
		assert(0);
		return -1;
		}

	NotifyClient(hhEmu->hSession, EVENT_TERM_UPDATE, 0L);
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuQueryCurPos
 *
 * DESCRIPTION: Returns the row and column position of the current
 *				cursor position.
 *
 * ARGUMENTS:	hEmu	- The External emulator handle.
 *				*row	- A pointer to an integer.
 *				*col	- A pointer to an integer.
 *
 * RETURNS:
 *	0=OK, else error
 *
 */
int emuQueryCurPos(const HEMU hEmu, int *row, int *col)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hEmu == 0)
		{
		assert(0);
		return -1;
		}

	emuLock(hEmu);
	*row = hhEmu->emu_currow;
	*col = hhEmu->emu_curcol;
	emuUnlock(hEmu);

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuQueryCursorType
 *
 * DESCRIPTION:
 *	Returns the current cursor type.
 *
 * ARGUMENTS:
 *	hEmu	- public emulator handle.
 *
 * RETURNS:
 *	The cursor type.
 *
 */
int emuQueryCursorType(const HEMU hEmu)
	{
	int iCurType;
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hEmu == 0)
		{
		assert(0);
		return -1;
		}

	emuLock(hEmu);
	iCurType = hhEmu->iCurType;
	emuUnlock(hEmu);

	return iCurType;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuTrackingNotify
 *
 * DESCRIPTION:
 *	Cloop calls this function when it detects a pause in the data flow.
 *	This allows the client side to track to the cursor position if that
 *	option is enabled.
 *
 * ARGUMENTS:
 *	HEMU hEmu	- External emulator handle.
 *
 * RETURNS:
 *	0
 *
 */
int emuTrackingNotify(const HEMU hEmu)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hhEmu == 0)
		{
		assert(FALSE);
		return 0;
		}

	NotifyClient(hhEmu->hSession, EVENT_TERM_TRACK, 0);

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuQueryClearAttr
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *	0=OK, else error
 *
 */
int emuQueryClearAttr(const HEMU hEmu, PSTATTR pstClearAttr)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hEmu == 0)
		{
		assert(0);
		return -1;
		}

	emuLock(hEmu);
	*pstClearAttr = hhEmu->emu_clearattr_sav;
	emuUnlock(hEmu);

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuQueryRowsCols
 *
 * DESCRIPTION:
 *	Returns the current number of rows and columns set in the emulator.
 *
 * ARGUMENTS:
 *	hEmu	- public emulator handle
 *	piRows	- pointer to row variable
 *	piCols	- pointer to col variable
 *
 * RETURNS:
 *	0=OK, else error
 *
 */
int emuQueryRowsCols(const HEMU hEmu, int *piRows, int *piCols)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hEmu == 0)
		{
		assert(0);
		return -1;
		}

	emuLock(hEmu);
	*piRows = hhEmu->emu_maxrow + 1;
	*piCols = hhEmu->emu_maxcol + 1;

	emuUnlock(hEmu);
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuQueryPrintEchoHdl
 *
 * DESCRIPTION: This routine returns the Print handle used for
 *				Printer Echo for the given emulator handle.
 *
 * ARGUMENTS:	hEmu	- The external emulator handle.
 *
 * RETURNS: 	HPRINT	- The External print handle for Printer Echo.
 *
 */
HPRINT emuQueryPrintEchoHdl(const HEMU hEmu)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;
	HPRINT hPrint;

	if (hEmu == 0)
		{
		assert(FALSE);
		return 0;
		}

	emuLock(hEmu);
	hPrint = hhEmu->hPrintEcho;
	emuUnlock(hEmu);

	return hPrint;
	}

#if 0
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuQueryEmuName
 *
 * DESCRIPTION:
 *	Returns the name of the emulator.
 *
 * ARGUMENTS:
 *	hEmu		- The External Emulator Handle.
 *	*acBuffer	- The address of a buffer to receive the information.
 *	nSize		- The size of the buffer.
 *
 * RETURNS:
 *	0=OK else error
 *
 */
int emuQueryName(const HEMU hEmu, TCHAR *achBuffer, int nSize)
	{
	const	HHEMU hhEmu = (HHEMU)hEmu;
	BYTE	*pv;
	BYTE	*temp;
	int 	nLen,
			indx,
			nEmuCount,
			nEmuId;

	if (hhEmu == 0)
		{
		*achBuffer = 0;
		assert(FALSE);
		return -1 ;
		}

	emuLock(hEmu);
	nEmuId = hhEmu->stUserSettings.nEmuId;
	emuUnlock(hEmu);

	if (resLoadDataBlock(glblQueryDllHinst(),
							IDT_EMU_NAMES,
							(LPVOID *)&pv, &nLen))
		{
		assert(FALSE);
		return -2;
		}

	nEmuCount = *(RCDATA_TYPE *)pv;
	pv += sizeof(RCDATA_TYPE);

	for (indx = 0 ; indx < nEmuCount ; indx++)
		{
		nLen = StrCharGetByteCount((LPTSTR)pv) + (int)sizeof(BYTE);

		if (nLen == 0)
			{
			assert(FALSE);
			return -3;
			}

		temp = pv + nLen;

		if (*(RCDATA_TYPE *)temp == nEmuId)
			{
			if (StrCharGetByteCount(pv) < nSize)
				{
				StrCharCopy(achBuffer, pv);
				break;
				}
			else
				{
				*achBuffer = 0;
				return -4;
				}
			}

		pv += (nLen + (int)sizeof(RCDATA_TYPE));
		}

	return 0;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuQueryEmuName
 *
 * DESCRIPTION:
 *	Returns the name of the emulator.
 *
 * ARGUMENTS:
 *	hEmu		- The External Emulator Handle.
 *	*acBuffer	- The address of a buffer to receive the information.
 *	nSize		- The size of the buffer.
 *
 * RETURNS:
 *	0=OK else error
 *
 */
int emuQueryName(const HEMU hEmu, TCHAR *achBuffer, int nSize)
	{
	const	HHEMU hhEmu = (HHEMU)hEmu;
	TCHAR	achText[256];
	int 	nEmuId,
			nResourceId,
			nLen;

	emuLock(hEmu);
	nEmuId = hhEmu->stUserSettings.nEmuId;
	emuUnlock(hEmu);

	switch(nEmuId)
		{
		case EMU_AUTO:
			nResourceId = IDS_EMUNAME_AUTO;
			break;

		case EMU_ANSI:
			nResourceId = IDS_EMUNAME_ANSI;
			break;

		case EMU_ANSIW:
			nResourceId = IDS_EMUNAME_ANSIW;
			break;

		case EMU_MINI:
			nResourceId = IDS_EMUNAME_MINI;
			break;

		case EMU_VIEW:
			nResourceId = IDS_EMUNAME_VIEW;
			break;

		case EMU_TTY:
			nResourceId = IDS_EMUNAME_TTY;
			break;

		case EMU_VT100:
			nResourceId = IDS_EMUNAME_VT100;
			break;

		case EMU_VT52:
			nResourceId = IDS_EMUNAME_VT52;
			break;

		case EMU_VT100J:
			nResourceId = IDS_EMUNAME_VT100J;
			break;

#if defined(INCL_VT220)
		case EMU_VT220:
			nResourceId = IDS_EMUNAME_VT220;
			break;
#endif

#if defined(INCL_VT320)
		case EMU_VT320:
			nResourceId = IDS_EMUNAME_VT320;
			break;
#endif

		default:
			assert(FALSE);
			return(-1);
		}

	nLen = LoadString(glblQueryDllHinst(), (unsigned)nResourceId, achText,
		sizeof(achText) / sizeof(TCHAR));

	// Has the caller supplied a large enough buffer.
	//
	if (nSize <= nLen)
		return(-1);

	StrCharCopy(achBuffer, achText);

	return(0);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuGetIdFromName
 *
 * DESCRIPTION:
 *	Returns the Id for the supplied emulator name.
 *
 * ARGUMENTS:
 *	hEmu		- The External Emulator Handle.
 *	*acBuffer	- The address of a buffer to receive the information.
 *	nSize		- The size of the buffer.
 *
 * RETURNS:
 *	An emulator ID otherwise (-1) if the function fails.
 *
 */
int emuGetIdFromName(const HEMU hEmu, TCHAR *achEmuName)
	{
	const	HHEMU hhEmu = (HHEMU)hEmu;
	int 	iRet, idx;

	for(idx = 0; idx < NBR_EMULATORS; idx++)
		{
		iRet = StrCharCmp(achEmuName, hhEmu->pstNameTable[idx].acName);

		if (iRet == 0)
			{
			return(hhEmu->pstNameTable[idx].nEmuId);
			}

		}

	return(-1);

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuQueryEmulatorId
 *
 * DESCRIPTION:
 *	Returns the oridinal value for the current emulator.  This can be
 *	used through-out the program to write conditional code for emulators.
 *
 * ARGUMENTS:
 *	HEMU	hEmulator - external emulator handle.
 *
 * RETURNS:
 *	ordinal value (>0).
 *
 */
int emuQueryEmulatorId(const HEMU hEmulator)
	{
	const HHEMU hhEmu = (HHEMU)hEmulator;
	int			nEmuId;

	if (hhEmu == 0)
		{
		assert(0);
		return -1;
		}

	emuLock(hEmulator);
	nEmuId = hhEmu->stUserSettings.nEmuId;
	emuUnlock(hEmulator);

	return (nEmuId);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuLoad
 *
 * DESCRIPTION:
 *	 Loads all pertinent tables into memory for the specified emulator.
 *
 * ARGUMENTS:	hEmu	External Emulator Handle.
 *				nEmuId	ID that identifies a specific emulator.
 *				fForceLoad -
 *
 *
 * RETURNS: 	0 if successful.  -1 if nEmuId is invalid.
 *
 */
int emuLoad(const HEMU hEmu, const int nEmuId)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;
	int col;
	void (*emuInitFunction)(const HHEMU hhEmu);

	if (hEmu == 0)
		{
		assert(0);
		return -2;
		}

	emuLock(hEmu);

	// If the requested emulator is already loaded, return.
	//
	if (hhEmu->nEmuLoaded == nEmuId)
		{
		emuUnlock(hEmu);
		return 0;
		}

	// Validate nEmuId and set set initialization function.

	switch(nEmuId)
		{
		#if defined(INCL_MINITEL)
		case EMU_MINI:
			emuInitFunction = emuMinitelInit;
			break;
		#endif

		case EMU_AUTO:
			emuInitFunction = emuAutoInit;
			break;

		case EMU_ANSIW:
		case EMU_ANSI:
			emuInitFunction = emuAnsiInit;
			break;

		case EMU_TTY:
			emuInitFunction = emuAnsiInit;
			break;

		case EMU_VT100J:
		case EMU_VT100:
			emuInitFunction = vt100_init;
			break;

		case EMU_VT52:
			emuInitFunction = vt52_init;
			break;

		#if defined(INCL_VT220)
		case EMU_VT220:
			emuInitFunction = vt220_init;
			break;
		#endif

		#if defined(INCL_VT320)
		case EMU_VT320:
			emuInitFunction = vt220_init;	// The vt320 is the same as the vt220.
			break;
		#endif

		#if defined(INCL_VIEWDATA)
		case EMU_VIEW:
			emuInitFunction = EmuViewdataInit;
			break;
		#endif

		default:
			emuUnlock(hEmu);
			return(-1);
		}

	// Remove the current emulator, if one is loaded.
	//
	if (hhEmu->emu_deinstall)
		(*hhEmu->emu_deinstall)(hhEmu);

	// Save the new emulator Id.
	//
	hhEmu->stUserSettings.nEmuId = nEmuId;
	hhEmu->nEmuLoaded = nEmuId;

	// setup function pointers to standard routines.

	hhEmu->EmuSetCursorType = EmuStdSetCursorType;
	hhEmu->emuResetTerminal = stdResetTerminal;
	hhEmu->emu_graphic 		= emuStdGraphic;
#if defined(EXTENDED_FEATURES)	
	hhEmu->emu_datain 		= emuStdDataIn;
#else
	hhEmu->emu_datain 		= emuDataIn;
#endif
	hhEmu->emu_kbdin 		= std_kbdin;
	hhEmu->emu_getscrsize 	= std_getscrsize;
#ifdef INCL_TERMINAL_SIZE_AND_COLORS
    hhEmu->emu_setscrsize	= std_setscrsize;
#endif
	hhEmu->emu_getscrollcnt = std_getscrollcnt;
	hhEmu->emu_getcurpos 	= std_getcurpos;
	hhEmu->emu_setcurpos 	= std_setcurpos;
	hhEmu->emu_getattr 		= std_getattr;
	hhEmu->emu_setattr 		= std_setattr;
	hhEmu->emu_setcolors 	= std_setcolors;
	hhEmu->emu_getcolors 	= std_getcolors;
	hhEmu->emu_initcolors 	= std_initcolors;
	hhEmu->emu_clearscreen 	= std_clearscreen;
	hhEmu->emu_clearline 	= std_clearline;
	hhEmu->emu_clearrgn 	= std_clearrgn;
	hhEmu->emu_scroll 		= std_scroll;
	hhEmu->emu_deinstall 	= std_deinstall;
	hhEmu->emu_ntfy 		= std_emu_ntfy;
	hhEmu->emuHomeHostCursor= std_HomeHostCursor;

	hhEmu->emu_maxrow 		= EMU_DEFAULT_MAXROW;
	hhEmu->emu_maxcol 		= EMU_DEFAULT_MAXCOL;
    hhEmu->bottom_margin    = hhEmu->emu_maxrow;    // mrw:2/21/96
    hhEmu->top_margin       = 0;                    // mrw:2/21/96

	hhEmu->emu_charattr 	= hhEmu->attrState[CS_STATE];

	hhEmu->emu_clearattr =
	hhEmu->emu_clearattr_sav = hhEmu->attrState[CSCLEAR_STATE];

	// Initialize mode variables.
	//
	hhEmu->mode_KAM = RESET;	   /* Enable Keyboard */
	hhEmu->mode_IRM = RESET;	   /* Replace chars rather than insert */
	hhEmu->mode_VEM = RESET;	   /* Inserting lines scrolls down, not up */
	hhEmu->mode_HEM = RESET;	   /* Inserting chars scrolls right, not left */
	hhEmu->mode_SRM = SET;		   /* Send-Receive. No local character echo */
	hhEmu->mode_LNM = RESET;	   /* LF moves vertically only */
	hhEmu->mode_DECOM = RESET;	   /* Absolute cursor positioning */
	hhEmu->mode_DECPFF = RESET;    /* No form feed after screen prINT */
	hhEmu->mode_DECPEX = RESET;    /* PrINT only scroll rgn. on screen prINT */
	hhEmu->mode_DECSCNM = RESET;   /* Screen mode. RESET=normal vid, SET=reverse vid */
	hhEmu->mode_DECTCEM = SET;	   /* Cursor enable. RESET=hidden, SET=visible */
	hhEmu->mode_25enab = RESET;    /* When true (SET), emulator can use 25th line */
	hhEmu->mode_protect = RESET;   /* When true (SET), protected mode is on */
	hhEmu->mode_block = RESET;	   /* When true (SET), block mode is on */
	hhEmu->mode_local = RESET;	   /* When true (SET), block mode is on */
	hhEmu->print_echo = FALSE;

	// Initialize state table.
	//
	hhEmu->emu_highchar = 0x7F;

	// Set default tab stops to 8.
	//
	//for (col = 0; col <= EMU_DEFAULT_MAXCOL; ++col)
	// Need to define tab stops for the whole screen so received tabs
	// with cursor at column 72 or more will go to the next tab rather
	// than the right edge of the screen. 22 Apr 99 rde
	for (col = 0; col < MAX_EMUCOLS; ++col)
		{
		if (!(col % 8))
			hhEmu->tab_stop[col] = TRUE;
		}

	// Call initialization function for emulator.
	//
	(*emuInitFunction)(hhEmu);

	// Paints every cell with the new attributes...
	//
	std_initcolors(hhEmu);

#ifdef INCL_TERMINAL_SIZE_AND_COLORS
    // Update the screen size.
    //
    hhEmu->emu_setscrsize(hhEmu);
#endif

	// We want the terminal to read the emulator image after loading
	// but we don't want a scroll operation so update each line
	// individually - mrw

	updateLine(sessQueryUpdateHdl(hhEmu->hSession), 0, hhEmu->emu_maxrow);

	NotifyClient(hhEmu->hSession, EVENT_EMU_CLRATTR, 0);
	NotifyClient(hhEmu->hSession, EVENT_EMU_SETTINGS, 0);
	NotifyClient(hhEmu->hSession, EVENT_TERM_UPDATE, 0);

	emuUnlock(hEmu);

	PostMessage(sessQueryHwndStatusbar(hhEmu->hSession),
		SBR_NTFY_REFRESH, (WPARAM)SBR_EMU_PART_NO, 0);

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuQuerySettings
 *
 * DESCRIPTION:
 *	This function returns a copy of the of the User Settings structure
 *	found in the internal emulator handle.
 *
 * ARGUMENTS:
 *	HEMU		-	The External emulator handle.
 *	PSTEMUSET	-	A pointer to a structure of type STEMUSET
 *
 * RETURNS:
 *	0=OK, else error
 *
 */
int emuQuerySettings(const HEMU hEmu, PSTEMUSET pstSettings)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hEmu == 0)
		{
		assert(0);
		return -1;
		}

	emuLock(hEmu);
	MemCopy(pstSettings, &hhEmu->stUserSettings, sizeof(STEMUSET));
	emuUnlock(hEmu);

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuSetSettings
 *
 * DESCRIPTION:
 *	This function stores the Emulator Settings in the internal Emualtor
 *	handle.  This function DOES NOT save the information in the session
 *	file.  The values to be set are validated.  If any values passed
 *	into this routine are invalid, default values will be set.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *	0 if the values passed in are validated.  A number less than 0 will
 *	be returned if any one of the values are invalid.  Note that in the 
 *	case of several invalid settings, the return value will point only
 *	to the last one that was invalid.
 *
 */
int emuSetSettings(const HEMU hEmu, const PSTEMUSET pstSettings)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;
	int iReturn;

	if (hEmu == 0)
		{
		assert(0);
		return -1;
		}

	iReturn = 0;

	switch(pstSettings->nEmuId)
		{
		case EMU_AUTO:
		case EMU_ANSI:
		case EMU_ANSIW:
		case EMU_MINI:
		case EMU_VIEW:
		case EMU_TTY:
		case EMU_VT100:
		case EMU_VT100J:
		case EMU_VT52:
		case EMU_VT220:
		case EMU_VT320:
			break;
		default:
			pstSettings->nEmuId = EMU_AUTO;
			iReturn = -1;
			assert(FALSE);
			break;
		}

	switch(pstSettings->nTermKeys)
		{
		case EMU_KEYS_ACCEL:
		case EMU_KEYS_TERM:
		case EMU_KEYS_SCAN:
			break;
		default:
			pstSettings->nTermKeys = EMU_KEYS_TERM;
			iReturn = -2;
			assert(FALSE);
			break;
		}

	switch(pstSettings->nCursorType)
		{
		case EMU_CURSOR_BLOCK:
		case EMU_CURSOR_LINE:
		case EMU_CURSOR_NONE:
			break;
		default:
			pstSettings->nCursorType = EMU_CURSOR_LINE;
			iReturn = -3;
			assert(FALSE);
			break;
		}

	switch(pstSettings->nCharacterSet)
		{
		case EMU_CHARSET_ASCII:
		case EMU_CHARSET_UK:
		case EMU_CHARSET_SPECIAL:
#if defined(INCL_VT220)
		case EMU_CHARSET_MULTINATIONAL:	
		case EMU_CHARSET_FRENCH:			
		case EMU_CHARSET_FRENCHCANADIAN:	
		case EMU_CHARSET_GERMAN:			
#endif
			break;
		default:
			pstSettings->nCharacterSet = EMU_CHARSET_ASCII;
			iReturn = -4;
			assert(FALSE);
			break;
		}

	// The values to be set have been validated.  Set the emulator handle
	// values, and the internal emualtor variables that correspond.
	//
	emuLock(hEmu);

#ifdef INCL_TERMINAL_SIZE_AND_COLORS
	if (pstSettings->nTextColor != hhEmu->stUserSettings.nTextColor ||
			pstSettings->nBackgroundColor != hhEmu->stUserSettings.nBackgroundColor)
		{
		// The user defined colors have changed--implement them.
		std_setcolors(hhEmu, 
				pstSettings->nTextColor,
				pstSettings->nBackgroundColor);
		}
#endif

	// When called from emuInitializeHdl, the source and dest pointers
	// for the following call are the same, so there's no need to
	// to do the copy.
	//
	if(&hhEmu->stUserSettings != pstSettings)
		MemCopy(&hhEmu->stUserSettings, pstSettings, sizeof(STEMUSET));

	hhEmu->iCurType = hhEmu->stUserSettings.nCursorType;
	hhEmu->mode_AWM = hhEmu->stUserSettings.fWrapLines;
	hhEmu->mode_DECKPAM = hhEmu->stUserSettings.fKeypadAppMode;
	hhEmu->mode_DECCKM = hhEmu->stUserSettings.fCursorKeypadMode;

	hhEmu->fUse8BitCodes = hhEmu->stUserSettings.fUse8BitCodes;
	hhEmu->fAllowUserKeys = hhEmu->stUserSettings.fAllowUserKeys;

	// Call emuSetDecColumns only if the emulator is a VT100, and
	// there has been change in the user setting of 132 column mode.
	//
	emuSetDecColumns(hhEmu,
						hhEmu->stUserSettings.f132Columns ?
						VT_MAXCOL_132MODE :
						VT_MAXCOL_80MODE,
						FALSE);

#ifdef INCL_TERMINAL_SIZE_AND_COLORS
    // Set the possible new row and column values if
    // the emulator is not about to change.
    //
    hhEmu->emu_setscrsize(hhEmu);
#endif

	emuUnlock(hEmu);

	NotifyClient(hhEmu->hSession, EVENT_EMU_SETTINGS, 0);
    //JMH 01-09-97 These two events also need to be sent, says JCM.
    //
    NotifyClient(hhEmu->hSession, EVENT_TERM_UPDATE, 0);
    NotifyClient(hhEmu->hSession, EVENT_EMU_CLRATTR, 0);

	return(iReturn);
	}

#if defined(EXTENDED_FEATURES)	//TODO:rde 23 mar 98
	// When we're comfortable enough with changing to using a function
	// pointer here, do it for all builds.
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  emuDataIn
 *
 * DESCRIPTION:
 *  External entry point for incoming data.
 *
 * ARGUMENTS:
 *  hEmu    - public emulator handle
 *  ccode   - character code
 *
 * RETURNS:
 *  TRUE if displayable
 *
 */
int emuDataIn(const HEMU hEmu, const ECHAR ccode)
    {
	int iRetVal = TRUE;
    const HHEMU hhEmu = (HHEMU)hEmu;

	if (hhEmu == 0)
		{
		assert(FALSE);
		}
	else
		{
    	emuLock(hEmu);

	    CaptureChar(sessQueryCaptureFileHdl(hhEmu->hSession),
					    CPF_MODE_RAW,
					    ccode); //moved from emuStdDataIn 05AUG98 mpt
        emuUnlock(hEmu);

        iRetVal = (*hhEmu->emu_datain)(hhEmu, ccode);
		}

	return iRetVal;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuStdDataIn
 *
 * DESCRIPTION:
 *	 Processes passed in codes through the emulation state tables.
 *
 * ARGUMENTS:
 *	hhEmu	- The internal emulator handle.
 *	ccode	- The character to process.
 *
 * RETURNS:
 *	 TRUE if displayable character
 */
int emuStdDataIn(const HHEMU hhEmu, const ECHAR ccode)
	{
	//ECHAR echCode;
	int ntrans;
	struct trans_entry *tptr;

	emuLock((HEMU)hhEmu);

	hhEmu->emu_code = ETEXT(ccode);

	// Capture raw data.
	//
#if 0 //moved to emuDataIn to prevent characters being captures
      //multiple times due to the re-entrant nature of this function. - mpt 5aug98
    CaptureChar(sessQueryCaptureFileHdl(hhEmu->hSession),
					CPF_MODE_RAW,
					ccode);
#endif
	// Seek next state by finding character range.
	//
	tptr = hhEmu->state_tbl[hhEmu->state].first_trans;
	ntrans = hhEmu->state_tbl[hhEmu->state].number_trans;

	for (; ntrans > 0; ntrans--, ++tptr)
		if (ccode >= tptr->lochar && ccode <= tptr->hichar)
			break;

	if (ntrans <= 0)
		{
		// Added to handle the case of 2 successive ESC chars--just
		// the 1st ESC should be thrown away

		hhEmu->state = 0;
		tptr = hhEmu->state_tbl[hhEmu->state].first_trans;
		ntrans = hhEmu->state_tbl[hhEmu->state].number_trans;

		for (; ntrans > 0; ntrans--, ++tptr)
			{
			if (ccode >= tptr->lochar && ccode <= tptr->hichar)
				break;
			}

		// 6-14-83
		// second condition (below) added to allow emulator to toss invalid
		// escpae sequences (i.e.ESC [ 0v).

		if (ntrans <= 0 || tptr->next_state == 0)
			{
			commanderror(hhEmu);
			goto DataInExit;
			}

		else
			{
			CaptureChar(sessQueryCaptureFileHdl(hhEmu->hSession),
						CF_CAP_CHARS, ccode);
			printEchoChar(hhEmu->hPrintEcho, ccode);
			}
		}

	hhEmu->state = tptr->next_state;
	(*tptr->funct_ptr)(hhEmu);

	// The code has been process through the emulator.	Check for
	// capturing and printing, and reset other emulator values.
	//
	if (hhEmu->state == 0)
		{
		if (IN_RANGE(ccode, ETEXT(' '), hhEmu->emu_highchar) ||
						ccode == ETEXT('\r') ||
						ccode == ETEXT('\n'))
			{
			CaptureChar(sessQueryCaptureFileHdl(hhEmu->hSession),
						CF_CAP_CHARS, ccode);
			printEchoChar(hhEmu->hPrintEcho, ccode);
			}
		hhEmu->num_param_cnt = hhEmu->selector_cnt =
		hhEmu->selector[0] = hhEmu->num_param[0] = 0;

		hhEmu->DEC_private = FALSE;
		}

	DataInExit:

	emuUnlock((HEMU)hhEmu);

	return(TRUE);
	}
//#if FALSE
#else
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuDataIn
 *
 * DESCRIPTION:
 *	 Processes passed in codes through the emulation state tables.
 *
 * ARGUMENTS:
 *	hhEmu	- The internal emulator handle.
 *	ccode	- The character to process.
 *
 * RETURNS:
 *	 TRUE if displayable character
 */
int emuDataIn(const HEMU hEmu, const ECHAR ccode)
	{
	//ECHAR echCode;
	int ntrans;
	struct trans_entry *tptr;
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hhEmu == 0)
		{
		assert(FALSE);
		goto DataInExit;
		}

	emuLock(hEmu);

	hhEmu->emu_code = ETEXT(ccode);

	// Capture raw data.
	//
	CaptureChar(sessQueryCaptureFileHdl(hhEmu->hSession),
					CPF_MODE_RAW,
					ccode);

	// Seek next state by finding character range.
	//
	tptr = hhEmu->state_tbl[hhEmu->state].first_trans;
	ntrans = hhEmu->state_tbl[hhEmu->state].number_trans;

	for (; ntrans > 0; ntrans--, ++tptr)
		if (ccode >= tptr->lochar && ccode <= tptr->hichar)
			break;

	if (ntrans <= 0)
		{
		// Added to handle the case of 2 successive ESC chars--just
		// the 1st ESC should be thrown away

		hhEmu->state = 0;
		tptr = hhEmu->state_tbl[hhEmu->state].first_trans;
		ntrans = hhEmu->state_tbl[hhEmu->state].number_trans;

		for (; ntrans > 0; ntrans--, ++tptr)
			{
			if (ccode >= tptr->lochar && ccode <= tptr->hichar)
				break;
			}

		// 6-14-83
		// second condition (below) added to allow emulator to toss invalid
		// escpae sequences (i.e.ESC [ 0v).

		if (ntrans <= 0 || tptr->next_state == 0)
			{
			commanderror(hhEmu);
			goto DataInExit;
			}

		else
			{
			CaptureChar(sessQueryCaptureFileHdl(hhEmu->hSession),
						CF_CAP_CHARS, ccode);
			printEchoChar(hhEmu->hPrintEcho, ccode);
			}
		}

	hhEmu->state = tptr->next_state;
	(*tptr->funct_ptr)(hhEmu);

	// The code has been process through the emulator.	Check for
	// capturing and printing, and reset other emulator values.
	//
	if (hhEmu->state == 0)
		{
		if (IN_RANGE(ccode, ETEXT(' '), hhEmu->emu_highchar) ||
						ccode == ETEXT('\r') ||
						ccode == ETEXT('\n'))
			{
			CaptureChar(sessQueryCaptureFileHdl(hhEmu->hSession),
						CF_CAP_CHARS, ccode);
			printEchoChar(hhEmu->hPrintEcho, ccode);
			}
		hhEmu->num_param_cnt = hhEmu->selector_cnt =
		hhEmu->selector[0] = hhEmu->num_param[0] = 0;

		hhEmu->DEC_private = FALSE;
		}

	DataInExit:

	emuUnlock(hEmu);

	return(TRUE);
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuNotify
 *
 * DESCRIPTION:
 *	A function that can be called to notify the emulators of an event.
 *	As events are identified, they can be added to this function.
 *
 * ARGUMENTS:
 *	hEmu	-	The external emulator handle.
 *	nEvent	-	The event ID.
 *
 * RETURNS:
 *	0=OK, else error
 *
 */
int emuNotify(const HEMU hEmu, const int nEvent)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hhEmu == 0)
		{
		assert(FALSE);
		return -1;
		}

	switch(nEvent)
		{
		case EMU_EVENT_CONNECTED:
			emuLock(hEmu);
			hhEmu->fWasConnected = TRUE;
			emuUnlock(hEmu);
			break;

		case EMU_EVENT_DISCONNECTED:
			emuLock(hEmu);

			if (hhEmu->stUserSettings.nEmuId == EMU_AUTO)
				{
				if (hhEmu->fWasConnected)
					hhEmu->stUserSettings.nAutoAttempts++;

				if (hhEmu->stUserSettings.nAutoAttempts ==
						EMU_MAX_AUTODETECT_ATTEMPTS)
#if !defined(FAR_EAST)
					emuAutoDetectLoad(hhEmu, EMU_ANSI);
#else
					emuAutoDetectLoad(hhEmu, EMU_ANSIW);
#endif
				}

			hhEmu->fWasConnected = FALSE;
			emuUnlock(hEmu);
			break;

		default:
			break;
		}

	emuLock(hEmu);
	(*hhEmu->emu_ntfy)(hhEmu, nEvent);
	emuUnlock(hEmu);

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuHomeHostCursor
 *
 * DESCRIPTION:
 *	Homes the cursor.  Needed when we first load a session.  Incidently,
 *	homing the cursor for a minitel places the cursor at 1,0, not 0,0
 *	which is why this function exists.
 *
 * ARGUMENTS:
 *	hEmu	- public emulator handle.
 *
 * RETURNS:
 *	0=OK,else error
 *
 */
int emuHomeHostCursor(const HEMU hEmu)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;

	if (hhEmu == 0)
		{
		assert(0);
		return -1;
		}

	return (*hhEmu->emuHomeHostCursor)(hhEmu);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuEraseTerminalScreen
 *
 * DESCRIPTION:
 *	Erases emulator image, doesn't put stuff in the backscroll.
 *
 * ARGUMENTS:
 *	hEmu	- public emulator handle.
 *
 * RETURNS:
 *	0=OK,else error
 *
 */
int emuEraseTerminalScreen(const HEMU hEmu)
	{
	const HHEMU hhEmu = (HHEMU)hEmu;
	int i;

	if (hhEmu == 0)
		{
		assert(0);
		return -1;
		}

	for (i = 0 ; i <= hhEmu->emu_maxrow ; ++i)
		clear_imgrow(hhEmu, i);

	updateLine(sessQueryUpdateHdl(hhEmu->hSession), 0, hhEmu->emu_maxrow);

	// The notify function is used to in minitel to set the state
	// of the screen based on connection status (F or C in upper corner).
	//
	hhEmu->emu_ntfy(hhEmu, 0);
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuQueryDefaultTelnetId
 *
 * DESCRIPTION:
 *	Gets the default telnet terminal id string for the given emulator.
 *
 * ARGUMENTS:
 *	nEmuId	    - id of the emulator (e.g. EMU_ANSI).
 *  achTelnetId - pointer to the buffer for the string.
 *  nSize       - size of the buffer (in characters).
 *
 * RETURNS:
 *	0=OK, else error
 *
 * AUTHOR:  C. Baumgartner, 11/15/96
 */
int emuQueryDefaultTelnetId(const int nEmuId, TCHAR *achTelnetId, int nSize)
    {
    int     iRet = 0;
    TCHAR * pszDefaultId;

    // Note : Since these strings are a part of the telnet
    // protocol, it is okay if they are hard-coded.
    //
    switch(nEmuId)
        {
    case EMU_AUTO:
    case EMU_ANSI:
        pszDefaultId = TEXT("ANSI");
        break;

    case EMU_TTY:
        pszDefaultId = TEXT("TELETYPE-33");
        break;

    case EMU_VT52:
        pszDefaultId = TEXT("VT52");
        break;

#if defined(INCL_VT220)
    case EMU_VT220:
        pszDefaultId = TEXT("VT220");
        break;
#endif

#if defined(INCL_VT320)
    case EMU_VT320:
        pszDefaultId = TEXT("VT320");
        break;
#endif

    case EMU_VT100:
    default:
        pszDefaultId = TEXT("VT100");
        break;
        }

    if ( StrCharGetStrLength(pszDefaultId) < nSize )
        {
        StrCharCopy(achTelnetId, pszDefaultId);
        iRet = 0;
        }
    else
        {
        iRet = -1;
        }

    return iRet;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuLoadDefaultTelnetId
 *
 * DESCRIPTION:
 *	Loads the given emulator handle with the default telnet terminal id
 *  string for the current emulator type.
 *
 * ARGUMENTS:
 *	hEmu - A public emulator handle
 *
 * RETURNS:
 *	0=OK, else error
 *
 * AUTHOR:  C. Baumgartner, 11/18/96
 */
int emuLoadDefaultTelnetId(const HEMU hEmu)
    {
    HHEMU hhEmu = (HHEMU)hEmu;

    return emuQueryDefaultTelnetId(hhEmu->stUserSettings.nEmuId,
        hhEmu->stUserSettings.acTelnetId, EMU_MAX_TELNETID);
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuLoadSettings
 *
 * DESCRIPTION:
 *	Loads the given emulator user settings.
 *
 * ARGUMENTS:
 *	hhEmu - an internal emulator handle
 *	uiId - the SFID of the data to load
 *	ulDataSize - the size of the data to load (in bytes)
 *	pvData - address at which to put the data
 *
 * RETURNS:
 *	The size (in bytes) of the data loaded, -1 if an error occured
 *
 * AUTHOR:	Bob Everett - 6 Jun 1998 
 */
STATIC_FUNC int emuLoadSettings(const HHEMU hhEmu, 
								const unsigned int uiId,
								unsigned long ulDataSize,
								void *pvData)
    {
	int iResult = 0;
	int iRetVal = 0;
	unsigned long ulSize = 0;

	// Do the screen color settings exist?
	sfGetSessionItem(sessQuerySysFileHdl(hhEmu->hSession), uiId, &ulSize, 0);

	if (ulSize > 0)
		{
		// The settings exist. Go get them.
		iResult = sfGetSessionItem(sessQuerySysFileHdl(hhEmu->hSession),
				uiId, &ulDataSize, pvData);

		if (iResult != 0)
			iRetVal = -1;
		else
			iRetVal = (int)ulSize;
		}

	return iRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	emuCheckSettings
 *
 * DESCRIPTION:
 *	This ugly piece of code checks for a mismatch in settings between versions.
 *	The settings affected are fUse8BitCodes, fAllowUserKeys, nBackspaceKeys,
 *	and acTelnetId. The mismatch occurs when moving an HTPE3 session file to
 *	any newer version. This includes HT98 and HTPE4. 
 *
 * ARGUMENTS:
 *	hhEmu - an internal emulator handle
 *
 * RETURNS:
 *	Nada, zip, not a thing.
 *
 * AUTHOR:	Bob Everett - 8 Jun 1998 
 */
STATIC_FUNC void emuCheckSettings(struct stEmuBaseSFSettings *pstBaseSFSettings) 
    {
	TCHAR *psz = 0;
	TCHAR sz[EMU_MAX_TELNETID];

	if ((pstBaseSFSettings->nBackspaceKeys != EMU_BKSPKEYS_CTRLH) &&
			(pstBaseSFSettings->nBackspaceKeys != EMU_BKSPKEYS_DEL) &&
			(pstBaseSFSettings->nBackspaceKeys != EMU_BKSPKEYS_CTRLHSPACE))
		{
		// This is the best test for this scenario. Move the data.
		psz = (TCHAR *)&pstBaseSFSettings->fAllowUserKeys;
		if (StrCharGetStrLength(psz) < EMU_MAX_TELNETID)
			{
			StrCharCopy(sz, psz);
			StrCharCopy(pstBaseSFSettings->acTelnetId, sz);
			}

		pstBaseSFSettings->nBackspaceKeys = 
				pstBaseSFSettings->fUse8BitCodes;

		// Use the defaults.
		pstBaseSFSettings->fUse8BitCodes = FALSE;
		pstBaseSFSettings->fAllowUserKeys = FALSE;
		}
		
	// Check the data.
	if ((pstBaseSFSettings->fUse8BitCodes != TRUE) &&
			(pstBaseSFSettings->fUse8BitCodes != FALSE))
		pstBaseSFSettings->fUse8BitCodes = FALSE;

	if ((pstBaseSFSettings->fAllowUserKeys != TRUE) &&
			(pstBaseSFSettings->fAllowUserKeys != FALSE))
		pstBaseSFSettings->fAllowUserKeys = FALSE;

	if ((pstBaseSFSettings->nBackspaceKeys != EMU_BKSPKEYS_CTRLH) &&
			(pstBaseSFSettings->nBackspaceKeys != EMU_BKSPKEYS_DEL) &&
			(pstBaseSFSettings->nBackspaceKeys != EMU_BKSPKEYS_CTRLHSPACE))
		{
		pstBaseSFSettings->nBackspaceKeys = EMU_BKSPKEYS_CTRLH;

		// Assume that if nBackspaceKeys is messed up, so is acTelnetId.
	    pstBaseSFSettings->acTelnetId[0] = TEXT('\0');
		}
	}
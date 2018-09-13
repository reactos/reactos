/*	File: \wacker\emu\vt220ini.c (Created: 24-Jan-1998)
 *
 *	Copyright 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:28p $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\assert.h>
#include <tdll\mc.h>
#include <tdll\backscrl.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"
#include "emudec.hh"
#include "keytbls.h"

#if defined(INCL_VT220)

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vt220_init
 *
 * DESCRIPTION:
 *	 Initializes the VT220 emulator.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *	 nothing
 */
void vt220_init(const HHEMU hhEmu)
	{
	PSTDECPRIVATE pstPRI;
	int iRow;

	static struct trans_entry const vt220_tbl[] =
		{
		{NEW_STATE, 0, 0, 0}, // State 0
#if !defined(FAR_EAST)	// Left in from the VT100.
		{0, ETEXT('\x20'),	ETEXT('\x7E'),	emuDecGraphic}, 	// Space - ~
		{0, ETEXT('\xA0'),	ETEXT('\xFF'),	emuDecGraphic}, 	// 
#else
		{0, ETEXT('\x20'),	ETEXT('\x7E'),	emuDecGraphic}, 	// Space - ~
		{0, ETEXT('\xA0'),	0xFFFF,			emuDecGraphic}, 	// 
#endif

		{1, ETEXT('\x1B'),	ETEXT('\x1B'),	nothing},			// Esc
		{2, ETEXT('\x9B'),	ETEXT('\x9B'),	nothing},			// CSI

		// 7 bit control codes
//		{13,TEXT('\x01'),   ETEXT('\x01'),   nothing},			// Ctrl-A
		{0, ETEXT('\x05'),	ETEXT('\x05'),	vt100_answerback},	// Ctrl-E
		{0, ETEXT('\x07'),	ETEXT('\x07'),	emu_bell},			// Ctrl-G
		{0, ETEXT('\x08'),	ETEXT('\x08'),	vt_backspace},		// BackSpace
		{0, ETEXT('\x09'),	ETEXT('\x09'),	emuDecTab}, 		// Tab
		{0, ETEXT('\x0A'),	ETEXT('\x0C'),	emuLineFeed},		// NL - FF
		{0, ETEXT('\x0D'),	ETEXT('\x0D'),	carriagereturn},	// CR
		{0, ETEXT('\x0E'),	ETEXT('\x0F'),	vt_charshift},		// Ctrl-N, Ctrl-O
		{12,ETEXT('\x18'),	ETEXT('\x18'),	EmuStdChkZmdm}, 	// Ctrl-X

		// 8 bit control codes
		{0, ETEXT('\x84'),	ETEXT('\x84'),	emuDecIND}, 		// Index cursor
		{0, ETEXT('\x85'),	ETEXT('\x85'),	ANSI_NEL}, 			// Next line
		{0, ETEXT('\x88'),	ETEXT('\x88'),	ANSI_HTS}, 			// Set Horizontal Tab
		{0, ETEXT('\x8D'),	ETEXT('\x8D'),	emuDecRI}, 			// Reverse index
		{0, ETEXT('\x8E'),	ETEXT('\x8F'),	vt_charshift}, 		// SingleShift G2,G3
		{5, ETEXT('\x90'),	ETEXT('\x90'),	nothing}, 			// Device Control String (DCS)

		// Ignore these codes. They just show what functionality is still missing.
		{0, ETEXT('\x00'),	ETEXT('\x00'),	nothing},			// ignore nuls
		{0, ETEXT('\x1A'),	ETEXT('\x1A'),	nothing},			// ignore Substitute
		{0, ETEXT('\x7F'),	ETEXT('\x7F'),	nothing},			// ignore Delete
		{0, ETEXT('\x9C'),	ETEXT('\x9C'),	nothing},			// ignore String Terminator


		{NEW_STATE, 0, 0, 0},   // State 1						// Esc
		{2, ETEXT('\x5B'),  ETEXT('\x5B'),  ANSI_Pn_Clr},		// '['
		{7, ETEXT('\x20'),  ETEXT('\x20'),  nothing},			// Space
		{3, ETEXT('\x23'),  ETEXT('\x23'),  nothing},			// #
		{4, ETEXT('\x28'),  ETEXT('\x2B'),  vt_scs1},			// ( - +
		{0, ETEXT('\x37'),  ETEXT('\x38'),  vt100_savecursor},  // 8
		{1, ETEXT('\x3B'),  ETEXT('\x3B'),  ANSI_Pn_End},		// ;
		{0, ETEXT('\x3D'),  ETEXT('\x3E'),  vt_alt_kpmode},		// = - >
		{0, ETEXT('\x44'),  ETEXT('\x44'),  emuDecIND},			// D
		{0, ETEXT('\x45'),  ETEXT('\x45'),  ANSI_NEL},			// E
		{0, ETEXT('\x48'),  ETEXT('\x48'),  ANSI_HTS},			// H
		{0, ETEXT('\x4D'),  ETEXT('\x4D'),  emuDecRI},			// M
		{0, ETEXT('\x4E'),  ETEXT('\x4F'),  vt_charshift},		// N - O
		{5, ETEXT('\x50'),  ETEXT('\x50'),  nothing},			// P
		{0, ETEXT('\x5A'),  ETEXT('\x5A'),  vt220_DA},			// Z
		{0, ETEXT('\\'),	ETEXT('\\'),	nothing},			// Backslash
		{0, ETEXT('\x63'),  ETEXT('\x63'),  vt220_hostreset},   // c
		{0, ETEXT('\x6E'),  ETEXT('\x6F'),  vt_charshift},		// n - o
		{0, ETEXT('\x7D'),  ETEXT('\x7E'),  vt_charshift},		// } - ~

		{NEW_STATE, 0, 0, 0},   // State 2						// ESC [
		{8, ETEXT('\x21'),  ETEXT('\x21'),  nothing},			// !
		{2, ETEXT('\x3B'),  ETEXT('\x3B'),  ANSI_Pn_End},		// ;
		{9, ETEXT('\x3E'),  ETEXT('\x3E'),  nothing},			// >
		{2, ETEXT('\x30'),  ETEXT('\x3F'),  ANSI_Pn},			// 0 - ?
		{11,ETEXT('\x22'),  ETEXT('\x22'),  nothing},			// "
//		{16,ETEXT('\x24'),  ETEXT('\x24'),  nothing},			// $
		{2, ETEXT('\x27'),  ETEXT('\x27'),  nothing},			// Eat Esc [ m ; m ; ' z
		{0, ETEXT('\x40'),  ETEXT('\x40'),  ANSI_ICH},			// @
		{0, ETEXT('\x41'),  ETEXT('\x41'),  emuDecCUU},			// A
		{0, ETEXT('\x42'),  ETEXT('\x42'),  emuDecCUD},			// B
		{0, ETEXT('\x43'),  ETEXT('\x43'),  emuDecCUF},			// C
		{0, ETEXT('\x44'),  ETEXT('\x44'),  emuDecCUB},			// D
		{0, ETEXT('\x48'),  ETEXT('\x48'),  emuDecCUP},			// H
		{0, ETEXT('\x4A'),  ETEXT('\x4A'),  emuVT220ED},		// J
		{0, ETEXT('\x4B'),  ETEXT('\x4B'),  emuDecEL},			// K
		{0, ETEXT('\x4C'),  ETEXT('\x4C'),  vt_IL},				// L
		{0, ETEXT('\x4D'),  ETEXT('\x4D'),  vt_DL},				// M
		{0, ETEXT('\x50'),  ETEXT('\x50'),  vt_DCH},			// P
		{0, ETEXT('\x58'),  ETEXT('\x58'),  vt_DCH},			// X
		{0, ETEXT('\x63'),  ETEXT('\x63'),  vt220_DA},			// c
		{0, ETEXT('\x66'),  ETEXT('\x66'),  emuDecCUP},			// f
		{0, ETEXT('\x67'),  ETEXT('\x67'),  ANSI_TBC},			// g
		{0, ETEXT('\x68'),  ETEXT('\x68'),  ANSI_SM},			// h
		{0, ETEXT('\x69'),  ETEXT('\x69'),  vt100PrintCommands},// i
		{0, ETEXT('\x6C'),  ETEXT('\x6C'),  ANSI_RM},			// l
		{0, ETEXT('\x6D'),  ETEXT('\x6D'),  ANSI_SGR},			// m
		{0, ETEXT('\x6E'),  ETEXT('\x6E'),  ANSI_DSR},			// n
		{0, ETEXT('\x71'),  ETEXT('\x71'),  nothing},			// q
		{0, ETEXT('\x72'),  ETEXT('\x72'),  vt_scrollrgn},		// r
		{0, ETEXT('\x75'),  ETEXT('\x75'),  nothing},			// u
		{0, ETEXT('\x79'),  ETEXT('\x79'),  nothing},			// y
		{0, ETEXT('\x7A'),  ETEXT('\x7A'),  nothing},			// z

		{NEW_STATE, 0, 0, 0},   // State 3						// Esc #
		{0, ETEXT('\x33'),  ETEXT('\x36'),  emuSetDoubleAttr},  // 3 - 6
		{0, ETEXT('\x38'),  ETEXT('\x38'),  vt_screen_adjust},  // 8

		{NEW_STATE, 0, 0, 0},   // State 4						// Esc ( - +
		{0, ETEXT('\x01'),  ETEXT('\xFF'),  vt_scs2},			// All

		{NEW_STATE, 0, 0, 0},   // State 5						// Esc P
		{5, ETEXT('\x3B'),  ETEXT('\x3B'),  ANSI_Pn_End},		// ;
		{5, ETEXT('\x30'),  ETEXT('\x3F'),  ANSI_Pn},			// 0 - ?
		{10,ETEXT('\x7C'),  ETEXT('\x7C'),  emuDecClearUDK},	// |

		{NEW_STATE, 0, 0, 0},   // State 6
		{6, ETEXT('\x00'),  ETEXT('\xFF'),  vt100_prnc},		// All

		{NEW_STATE, 0, 0, 0},   // State 7						// Esc Sapce
		{0, ETEXT('\x46'),  ETEXT('\x47'),  nothing},			// F - G

		{NEW_STATE, 0, 0, 0},   // State 8						// Esc [ !
		{0, ETEXT('\x70'),  ETEXT('\x70'),  vt220_softreset},   // p

		{NEW_STATE, 0, 0, 0},   // State 9						// Esc [ >
		{0, ETEXT('\x63'),  ETEXT('\x63'),  vt220_2ndDA},		// c

		{NEW_STATE, 0, 0, 0},   // State 10						// Esc P n;n |
		{10,ETEXT('\x00'),  ETEXT('\xFF'),  emuDecDefineUDK},   // All

		{NEW_STATE, 0, 0, 0},   // State 11						// Esc [ "
		{0, ETEXT('\x70'),  ETEXT('\x70'),  vt220_level},		// p
		{0, ETEXT('\x71'),  ETEXT('\x71'),  vt220_protmode},	// q

		{NEW_STATE, 0, 0, 0},   // State 12						// Ctrl-X
		{12,ETEXT('\x00'),  ETEXT('\xFF'),  EmuStdChkZmdm},		// All

		// States 13-17 are not used in HT but included for reference. 
//		{NEW_STATE, 0, 0, 0},   // State 13						// Ctrl-A
//		{14,ETEXT('\x08'),  ETEXT('\x08'),  emuSerialNbr},		// Backspace
//		{15,ETEXT('\x48'),  ETEXT('\x48'),  EmuStdChkHprP},		// H

//		{NEW_STATE, 0, 0, 0},   // State 14						// Ctrl-A bs
//		{14,ETEXT('\x00'),  ETEXT('\xFF'),  emuSerialNbr},		// All

//		{NEW_STATE, 0, 0, 0},   // State 15						// Ctrl-A H
//		{15,ETEXT('\x00'),  ETEXT('\xFF'),  EmuStdChkHprP},		// All

		// A real VT220/320 does not support the status line sequences.
//		{NEW_STATE, 0, 0, 0},   // State 16								// Esc [ n $
//		{16,ETEXT('\x7E'),  ETEXT('\x7E'),  emuDecSelectStatusLine},	// ~
//		{17,ETEXT('\x7D'),  ETEXT('\x7D'),  emuDecSelectActiveDisplay}, // }

//		{NEW_STATE, 0, 0, 0},   // State 17
//		{17,ETEXT('\x00'),  ETEXT('\xFF'),  emuDecStatusLineToss},  // All

		};

	// The following key tables were copied from \shared\emulator\vt220ini.c  
	// because they support user-defined keys. The tables have been modified 
	// so keydef.h is not needed and to match HT's use of keys. rde 2 Feb 98

	// The following key tables are defined in the order that they
	// are searched.
	//

	// These are the (standard) F1 thru F4 keys on the top and left of the
	// keyboard.  Note that these keys may be mapped to the top row of the
	// numeric keypad.  In that case, these keys (at the standard locations),
	// are not mapped to emulator keys. NOTE: HTPE does not use this mapping.
	//
	// Please note that the sequences defined in this table are the
	// 8-bit versions of the responses.  The function emuDecSendKeyString
	// will convert this sequence to the 7-bit equivalent if necessary.
	//
	static  STEMUKEYDATA const VT220StdPfKeyTable[] =
		{
		EMUKEY(VK_F1,		1, 0, 0, 0, 0,  "\x8F\x50",			2), // P
		EMUKEY(VK_F2,		1, 0, 0, 0, 0,  "\x8F\x51",			2), // Q
		EMUKEY(VK_F3,		1, 0, 0, 0, 0,  "\x8F\x52",			2), // R
		EMUKEY(VK_F4,		1, 0, 0, 0, 0,  "\x8F\x53",			2), // S

		EMUKEY(VK_F1,		1, 0, 0, 0, 1,  "\x8F\x50",			2), // P
		EMUKEY(VK_F2,		1, 0, 0, 0, 1,  "\x8F\x51",			2), // Q
		EMUKEY(VK_F3,		1, 0, 0, 0, 1,  "\x8F\x52",			2), // R
		EMUKEY(VK_F4,		1, 0, 0, 0, 1,  "\x8F\x53",			2), // S
		};

	// When the user has selected the option to map the top 4 keys of the
	// numeric keypad to be the same as F1 thru F4, these key sequences are
	// used. NOTE: This is the mapping HTPE uses.
	//
	// Please note that the sequences defined in this table are the
	// 8-bit versions of the responses.  The function emuDecSendKeyString
	// will convert this sequence to the 7-bit equivalent if necessary.
	//
	static  STEMUKEYDATA const VT220MovedPfKeyTable[] =
		{
		EMUKEY(VK_NUMLOCK,	1, 0, 0, 0, 1,  "\x8F\x50",			2), // P
		EMUKEY(VK_DIVIDE,	1, 0, 0, 0, 1,  "\x8F\x51",			2), // Q
		EMUKEY(VK_MULTIPLY,	1, 0, 0, 0, 1,  "\x8F\x52",			2), // R
		EMUKEY(VK_SUBTRACT,	1, 0, 0, 0, 1,  "\x8F\x53",			2), // S
		};

	// VT220 Keypad Numeric Mode.
	//
	static STEMUKEYDATA const VT220KeypadNumericMode[] =
		{
		// Keypad keys with Numlock off.
		//
		EMUKEY(VK_INSERT,	1, 0, 0, 0, 0,  "\x30",			1), // 0
		EMUKEY(VK_END,		1, 0, 0, 0, 0,  "\x31",			1), // 1
		EMUKEY(VK_DOWN,		1, 0, 0, 0, 0,  "\x32",			1), // 2
		EMUKEY(VK_NEXT,		1, 0, 0, 0, 0,  "\x33",			1), // 3
		EMUKEY(VK_LEFT,		1, 0, 0, 0, 0,  "\x34",			1), // 4
		EMUKEY(VK_NUMPAD5,	1, 0, 0, 0, 0,  "\x35",			1), // 5
		EMUKEY(VK_RIGHT,	1, 0, 0, 0, 0,  "\x36",			1), // 6
		EMUKEY(VK_HOME,		1, 0, 0, 0, 0,  "\x37",			1), // 7
		EMUKEY(VK_UP,		1, 0, 0, 0, 0,  "\x38",			1), // 8
		EMUKEY(VK_PRIOR,	1, 0, 0, 0, 0,  "\x39",			1), // 9
		EMUKEY(VK_DELETE,	1, 0, 0, 0, 0,  "\x2E",			1), // .

		// Keypad keys with Numlock on.
		//
		EMUKEY(VK_NUMPAD0,		1, 0, 0, 0, 0,  "\x30",			1), // 0
		EMUKEY(VK_NUMPAD1,		1, 0, 0, 0, 0,  "\x31",			1), // 1
		EMUKEY(VK_NUMPAD2,		1, 0, 0, 0, 0,  "\x32",			1), // 2
		EMUKEY(VK_NUMPAD3,		1, 0, 0, 0, 0,  "\x33",			1), // 3
		EMUKEY(VK_NUMPAD4,		1, 0, 0, 0, 0,  "\x34",			1), // 4
		EMUKEY(VK_NUMPAD5,		1, 0, 0, 0, 0,  "\x35",			1), // 5
		EMUKEY(VK_NUMPAD6,		1, 0, 0, 0, 0,  "\x36",			1), // 6
		EMUKEY(VK_NUMPAD7,		1, 0, 0, 0, 0,  "\x37",			1), // 7
		EMUKEY(VK_NUMPAD8,		1, 0, 0, 0, 0,  "\x38",			1), // 8
		EMUKEY(VK_NUMPAD9,		1, 0, 0, 0, 0,  "\x39",			1), // 9
		EMUKEY(VK_DECIMAL,		1, 0, 0, 0, 0,  "\x2E",			1), // .

		// Other keypad keys (minus, plus, Enter).
		//
		EMUKEY(VK_SUBTRACT,		1, 0, 0, 0, 0,  "\x2D",			1), // -
		EMUKEY(VK_ADD,			1, 0, 0, 0, 0,  "\x2C",			1), // ,
		EMUKEY(VK_RETURN,		1, 0, 0, 0, 1,  "\x0D",			1), // CR
		};

	// VT220 Keypad Application Mode.
	//
	// Please note that the sequences defined in this table are the
	// 8-bit versions of the responses.  The function emuDecSendKeyString
	// will convert this sequence to the 7-bit equivalent if necessary.
	//
	static STEMUKEYDATA const VT220KeypadApplicationMode[] =
		{
		// Keypad keys with Numlock off.
		//
		EMUKEY(VK_NUMPAD0,		1, 0, 0, 0, 0,  "\x8F\x70",		2), // p
		EMUKEY(VK_NUMPAD1,		1, 0, 0, 0, 0,  "\x8F\x71",		2), // q
		EMUKEY(VK_NUMPAD2,		1, 0, 0, 0, 0,  "\x8F\x72",		2), // r
		EMUKEY(VK_NUMPAD3,		1, 0, 0, 0, 0,  "\x8F\x73",		2), // s
		EMUKEY(VK_NUMPAD4,		1, 0, 0, 0, 0,  "\x8F\x74",		2), // t
		EMUKEY(VK_NUMPAD5,		1, 0, 0, 0, 0,  "\x8F\x75",		2), // u
		EMUKEY(VK_NUMPAD6,		1, 0, 0, 0, 0,  "\x8F\x76",		2), // v
		EMUKEY(VK_NUMPAD7,		1, 0, 0, 0, 0,  "\x8F\x77",		2), // w
		EMUKEY(VK_NUMPAD8,		1, 0, 0, 0, 0,  "\x8F\x78",		2), // x
		EMUKEY(VK_NUMPAD9,		1, 0, 0, 0, 0,  "\x8F\x79",		2), // y
		EMUKEY(VK_DECIMAL,		1, 0, 0, 0, 0,  "\x8F\x6E",		2), // n

		// Keypad keys with Numlock on.
		//
		EMUKEY(VK_NUMPAD0,		1, 0, 0, 0, 0,  "\x8F\x70",		2), // p
		EMUKEY(VK_NUMPAD1,		1, 0, 0, 0, 0,  "\x8F\x71",		2), // q
		EMUKEY(VK_NUMPAD2,		1, 0, 0, 0, 0,  "\x8F\x72",		2), // r
		EMUKEY(VK_NUMPAD3,		1, 0, 0, 0, 0,  "\x8F\x73",		2), // s
		EMUKEY(VK_NUMPAD4,		1, 0, 0, 0, 0,  "\x8F\x74",		2), // t
		EMUKEY(VK_NUMPAD5,		1, 0, 0, 0, 0,  "\x8F\x75",		2), // u
		EMUKEY(VK_NUMPAD6,		1, 0, 0, 0, 0,  "\x8F\x76",		2), // v
		EMUKEY(VK_NUMPAD7,		1, 0, 0, 0, 0,  "\x8F\x77",		2), // w
		EMUKEY(VK_NUMPAD8,		1, 0, 0, 0, 0,  "\x8F\x78",		2), // x
		EMUKEY(VK_NUMPAD9,		1, 0, 0, 0, 0,  "\x8F\x79",		2), // y
		EMUKEY(VK_DECIMAL,		1, 0, 0, 0, 0,  "\x8F\x6E",		2), // n

		// Other keypad keys (minus, plus, Enter).
		//
		EMUKEY(VK_SUBTRACT,		1, 0, 0, 0, 0,  "\x8F\x6D",		2), // m
		EMUKEY(VK_ADD,			1, 0, 0, 0, 0,  "\x8F\x6C",		2), // l
		EMUKEY(VK_RETURN,		1, 0, 0, 0, 1,  "\x8F\x4D",		2), // M
		};

	// VT220 Cursor Key Mode.
	//
	// Please note that the sequences defined in this table are the
	// 8-bit versions of the responses.  The function emuDecSendKeyString
	// will convert this sequence to the 7-bit equivalent if necessary.
	//
	static STEMUKEYDATA const VT220CursorKeyMode[] =
		{
		// Arrow keys on the numeric keypad.  These sequences are used
		// when the emulator is using Cursor Key Mode (Application Keys).
		//
		EMUKEY(VK_UP,		1, 0, 0, 0, 0,  "\x8F\x41",			2), // A
		EMUKEY(VK_DOWN,		1, 0, 0, 0, 0,  "\x8F\x42",			2), // B
		EMUKEY(VK_RIGHT,	1, 0, 0, 0, 0,  "\x8F\x43",			2), // C
		EMUKEY(VK_LEFT,		1, 0, 0, 0, 0,  "\x8F\x44",			2), // D

		// Arrow keys on the edit pad.  These sequences are used
		// when the emulator is using Cursor Key Mode (Application Keys).
		//
		EMUKEY(VK_UP,		1, 0, 0, 0, 1,  "\x8F\x41",			2), // A
		EMUKEY(VK_DOWN,		1, 0, 0, 0, 1,  "\x8F\x42",			2), // B
		EMUKEY(VK_RIGHT,	1, 0, 0, 0, 1,  "\x8F\x43",			2), // C
		EMUKEY(VK_LEFT,		1, 0, 0, 0, 1,  "\x8F\x44",			2), // D
		};

	// VT220 Standard Key Table.
	//
	static STEMUKEYDATA const VT220StandardKeys[] =
		{
		// Some keys on the numeric keypad will respond in the same
		// way the corresponding keys on the edit pad respond.
		//
		EMUKEY(VK_HOME,		1, 0, 0, 0, 0,  "\x9B\x31\x7E",		3), // 1 ~
		EMUKEY(VK_INSERT,	1, 0, 0, 0, 0,  "\x9B\x32\x7E",		3), // 2 ~
		EMUKEY(VK_DELETE,	1, 0, 0, 0, 0,  "\x9B\x33\x7E",		3), // 3 ~
		EMUKEY(VK_END,		1, 0, 0, 0, 0,  "\x9B\x34\x7E",		3), // 4 ~
		EMUKEY(VK_PRIOR,	1, 0, 0, 0, 0,  "\x9B\x35\x7E",		3), // 5 ~
		EMUKEY(VK_NEXT,		1, 0, 0, 0, 0,  "\x9B\x36\x7E",		3), // 6 ~

		// These are the keys on the edit pad.
		//
		EMUKEY(VK_HOME,		1, 0, 0, 0, 1,  "\x9B\x31\x7E",		3), // 1 ~
		EMUKEY(VK_INSERT,	1, 0, 0, 0, 1,  "\x9B\x32\x7E",		3), // 2 ~
		EMUKEY(VK_DELETE,	1, 0, 0, 0, 1,  "\x9B\x33\x7E",		3), // 3 ~
		EMUKEY(VK_END,		1, 0, 0, 0, 1,  "\x9B\x34\x7E",		3), // 4 ~
		EMUKEY(VK_PRIOR,	1, 0, 0, 0, 1,  "\x9B\x35\x7E",		3), // 5 ~
		EMUKEY(VK_NEXT,		1, 0, 0, 0, 1,  "\x9B\x36\x7E",		3), // 6 ~

		// Arrow keys on the numeric keypad.
		//
		EMUKEY(VK_UP,		1, 0, 0, 0, 0,  "\x9B\x41",			2), // A
		EMUKEY(VK_DOWN,		1, 0, 0, 0, 0,  "\x9B\x42",			2), // B
		EMUKEY(VK_RIGHT,	1, 0, 0, 0, 0,  "\x9B\x43",			2), // C
		EMUKEY(VK_LEFT,		1, 0, 0, 0, 0,  "\x9B\x44",			2), // D

		// Arrow keys on the edit pad.
		//
		EMUKEY(VK_UP,		1, 0, 0, 0, 1,  "\x9B\x41",			2), // A
		EMUKEY(VK_DOWN,		1, 0, 0, 0, 1,  "\x9B\x42",			2), // B
		EMUKEY(VK_RIGHT,	1, 0, 0, 0, 1,  "\x9B\x43",			2), // C
		EMUKEY(VK_LEFT,		1, 0, 0, 0, 1,  "\x9B\x44",			2), // D

		// Function keys (F5)F6 thru F10.
		//
#if defined(INCL_ULTC_VERSION)
		EMUKEY(VK_F5,		1, 0, 0, 0, 0,  "\x9B\x31\x36\x7E", 4), // 1 6 ~
#endif
		EMUKEY(VK_F6,		1, 0, 0, 0, 0,  "\x9B\x31\x37\x7E", 4), // 1 7 ~
		EMUKEY(VK_F7,		1, 0, 0, 0, 0,  "\x9B\x31\x38\x7E", 4), // 1 8 ~
		EMUKEY(VK_F8,		1, 0, 0, 0, 0,  "\x9B\x31\x39\x7E", 4), // 1 9 ~
		EMUKEY(VK_F9,		1, 0, 0, 0, 0,  "\x9B\x32\x30\x7E", 4), // 2 0 ~
		EMUKEY(VK_F10,		1, 0, 0, 0, 0,  "\x9B\x32\x31\x7E", 4), // 2 1 ~

#if defined(INCL_ULTC_VERSION)
		EMUKEY(VK_F5,		1, 0, 0, 0, 1,  "\x9B\x31\x36\x7E", 4), // 1 6 ~
#endif
		EMUKEY(VK_F6,		1, 0, 0, 0, 1,  "\x9B\x31\x37\x7E", 4), // 1 7 ~
		EMUKEY(VK_F7,		1, 0, 0, 0, 1,  "\x9B\x31\x38\x7E", 4), // 1 8 ~
		EMUKEY(VK_F8,		1, 0, 0, 0, 1,  "\x9B\x31\x39\x7E", 4), // 1 9 ~
		EMUKEY(VK_F9,		1, 0, 0, 0, 1,  "\x9B\x32\x30\x7E", 4), // 2 0 ~
		EMUKEY(VK_F10,		1, 0, 0, 0, 1,  "\x9B\x32\x31\x7E", 4), // 2 1 ~

		// Function keys F11 thru F20 are invoked by the user pressing
		// Ctrl-F1 thru Ctrl-F10.
		//
		// Function keys Ctrl-F1 thru Ctrl-F10 (Top row).
		//
		EMUKEY(VK_F1,		1, 1, 0, 0, 0,  "\x9B\x32\x33\x7E", 4), // 2 3 ~
		EMUKEY(VK_F2,		1, 1, 0, 0, 0,  "\x9B\x32\x34\x7E", 4), // 2 4 ~
		EMUKEY(VK_F3,		1, 1, 0, 0, 0,  "\x9B\x32\x35\x7E", 4), // 2 5 ~
		EMUKEY(VK_F4,		1, 1, 0, 0, 0,  "\x9B\x32\x36\x7E", 4), // 2 6 ~
		EMUKEY(VK_F5,		1, 1, 0, 0, 0,  "\x9B\x32\x38\x7E", 4), // 2 8 ~
		EMUKEY(VK_F6,		1, 1, 0, 0, 0,  "\x9B\x32\x39\x7E", 4), // 2 9 ~
		EMUKEY(VK_F7,		1, 1, 0, 0, 0,  "\x9B\x33\x31\x7E", 4), // 3 1 ~
		EMUKEY(VK_F8,		1, 1, 0, 0, 0,  "\x9B\x33\x32\x7E", 4), // 3 2 ~
		EMUKEY(VK_F9,		1, 1, 0, 0, 0,  "\x9B\x33\x33\x7E", 4), // 3 3 ~
		EMUKEY(VK_F10,		1, 1, 0, 0, 0,  "\x9B\x33\x34\x7E", 4), // 3 4 ~

		EMUKEY(VK_F1,		1, 1, 0, 0, 1,  "\x9B\x32\x33\x7E", 4), // 2 3 ~
		EMUKEY(VK_F2,		1, 1, 0, 0, 1,  "\x9B\x32\x34\x7E", 4), // 2 4 ~
		EMUKEY(VK_F3,		1, 1, 0, 0, 1,  "\x9B\x32\x35\x7E", 4), // 2 5 ~
		EMUKEY(VK_F4,		1, 1, 0, 0, 1,  "\x9B\x32\x36\x7E", 4), // 2 6 ~
		EMUKEY(VK_F5,		1, 1, 0, 0, 1,  "\x9B\x32\x38\x7E", 4), // 2 8 ~
		EMUKEY(VK_F6,		1, 1, 0, 0, 1,  "\x9B\x32\x39\x7E", 4), // 2 9 ~
		EMUKEY(VK_F7,		1, 1, 0, 0, 1,  "\x9B\x33\x31\x7E", 4), // 3 1 ~
		EMUKEY(VK_F8,		1, 1, 0, 0, 1,  "\x9B\x33\x32\x7E", 4), // 3 2 ~
		EMUKEY(VK_F9,		1, 1, 0, 0, 1,  "\x9B\x33\x33\x7E", 4), // 3 3 ~
		EMUKEY(VK_F10,		1, 1, 0, 0, 1,  "\x9B\x33\x34\x7E", 4), // 3 4 ~

		EMUKEY(VK_F1,		1, 0, 0, 0, 0,	"\x8FP",			2),
		EMUKEY(VK_F2,		1, 0, 0, 0, 0,	"\x8FQ",			2),
		EMUKEY(VK_F3,		1, 0, 0, 0, 0,	"\x8FR",			2),
		EMUKEY(VK_F4,		1, 0, 0, 0, 0,	"\x8FS",			2),

		EMUKEY(VK_F1,		1, 0, 0, 0, 1,	"\x8FP",			2),
		EMUKEY(VK_F2,		1, 0, 0, 0, 1,	"\x8FQ",			2),
		EMUKEY(VK_F3,		1, 0, 0, 0, 1,	"\x8FR",			2),
		EMUKEY(VK_F4,		1, 0, 0, 0, 1,	"\x8FS",			2),

		EMUKEY(VK_DELETE,	1, 0, 0, 0, 0,	"\x7F",				1),	// KN_DEL
		EMUKEY(VK_DELETE,	1, 0, 0, 0, 1,	"\x7F",				1),	// KN_DEL

		EMUKEY(VK_ADD,		1, 0, 0, 0, 0,	",",				1),

		// Ctrl-2.
		// Ctrl-@.
		//
		EMUKEY(0x32,		1, 1, 0, 0, 0,  "\x00",				1),
		EMUKEY(0x32,		1, 1, 0, 1, 0,  "\x00",				1),

		// Ctrl-6.
		// Ctrl-^.
		EMUKEY(0x36,		1, 1, 0, 0, 0,  "\x1E",				1),
		EMUKEY(0x36,		1, 1, 0, 1, 0,  "\x1E",				1),

		// Ctrl-Space
		//
		EMUKEY(VK_SPACE,	1, 1, 0, 0, 0,  "\x00",				1),

		// Ctrl-- key.
		//
		EMUKEY(VK_SUBTRACT,	1, 1, 0, 0, 1,  "\x1F",				1),
		};

	// VT220 User Defined keys.
	static STEMUKEYDATA VT220UserDefinedKeys[MAX_UDK_KEYS] =
		{
		// NOTE: Do not change the order of these user defined entries.
		// emuDecDefineUDK assumes a 1:1 correspondance with this
		// table and the UDKSelector table defined below.
		//
		// Initialize Virtual and Shift flags.
		//
		EMUKEY(VK_F6,		1, 0, 0, 1, 0,  0,					0),
		EMUKEY(VK_F7,		1, 0, 0, 1, 0,  0,					0),
		EMUKEY(VK_F8,		1, 0, 0, 1, 0,  0,					0),
		EMUKEY(VK_F9,		1, 0, 0, 1, 0,  0,					0),
		EMUKEY(VK_F10,		1, 0, 0, 1, 0,  0,					0),

		// Initialize Virtual and Alt flags.
		//
		EMUKEY(VK_F1,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F2,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F3,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F4,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F5,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F6,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F7,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F8,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F9,		1, 0, 1, 0, 0,  0,					0),
		EMUKEY(VK_F10,		1, 0, 1, 0, 0,  0,					0),
		};

	// NOTE: Do not change the order of these entries.
	// There is a 1:1 correspondance between this table and the
	// user defined key table defined above.
	//
	static TCHAR const acUDKSelectors[MAX_UDK_KEYS] =
		{
		TEXT('\x17'), TEXT('\x18'), TEXT('\x19'), TEXT('\x20'), // F6 -  F9
		TEXT('\x21'), TEXT('\x23'), TEXT('\x24'), TEXT('\x25'), // F10 - F13
		TEXT('\x26'), TEXT('\x28'), TEXT('\x29'), TEXT('\x31'), // F14 - F17
		TEXT('\x32'), TEXT('\x33'), TEXT('\x34'),				// F18 - F20
		};

	emuInstallStateTable(hhEmu, vt220_tbl, DIM(vt220_tbl));

	// Allocate space for and initialize data that is used only by the
	// VT220 emulator.
	//
	hhEmu->pvPrivate = malloc(sizeof(DECPRIVATE));

	if (hhEmu->pvPrivate == 0)
		{
		assert(FALSE);
		return;
		}

	pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	memset(pstPRI, 0, sizeof(DECPRIVATE));

	// NOTE:	The order of these definitions directly correspond to the
	//			search order used by the emuDecKeyboardIn function.
	//			Don't change these.
	//
	// In shared code, these are all part of hhEmu.
	pstPRI->pstcEmuKeyTbl1 = VT220StdPfKeyTable;
	pstPRI->pstcEmuKeyTbl2 = VT220MovedPfKeyTable;
	pstPRI->pstcEmuKeyTbl3 = VT220KeypadNumericMode;
	pstPRI->pstcEmuKeyTbl4 = VT220KeypadApplicationMode;
	pstPRI->pstcEmuKeyTbl5 = VT220CursorKeyMode;
	pstPRI->pstcEmuKeyTbl6 = VT220StandardKeys;

	pstPRI->iKeyTable1Entries = DIM(VT220StdPfKeyTable);
	pstPRI->iKeyTable2Entries = DIM(VT220MovedPfKeyTable);
	pstPRI->iKeyTable3Entries = DIM(VT220KeypadNumericMode);
	pstPRI->iKeyTable4Entries = DIM(VT220KeypadApplicationMode);
	pstPRI->iKeyTable5Entries = DIM(VT220CursorKeyMode);
	pstPRI->iKeyTable6Entries = DIM(VT220StandardKeys);

	// Allocate an array to hold line attribute values.
	//
	pstPRI->aiLineAttr = malloc(MAX_EMUROWS * sizeof(int) );

	if (pstPRI->aiLineAttr == 0)
		{
		assert(FALSE);
		return;
		}

	for (iRow = 0; iRow < MAX_EMUROWS; iRow++)
		pstPRI->aiLineAttr[iRow] = NO_LINE_ATTR;

	pstPRI->sv_row			= 0;
	pstPRI->sv_col			= 0;
	pstPRI->gn				= 0;
	pstPRI->sv_AWM			= RESET;
	pstPRI->sv_DECOM		= RESET;
	pstPRI->sv_protectmode	= FALSE;
	pstPRI->fAttrsSaved 	= FALSE;
	pstPRI->pntr			= pstPRI->storage;

	// Initialize hhEmu values for VT220.
	//
	hhEmu->emu_setcurpos	= emuDecSetCurPos;
	hhEmu->emu_deinstall	= emuDecUnload;
	hhEmu->emu_clearline	= emuDecClearLine;
	hhEmu->emu_clearscreen  = emuDecClearScreen;
	hhEmu->emu_kbdin		= emuDecKeyboardIn;
	hhEmu->emuResetTerminal = vt220_reset;
	hhEmu->emu_graphic		= emuDecGraphic;
//	hhEmu->emu_scroll		= emuDecScroll;

#if !defined(FAR_EAST)
	hhEmu->emu_highchar 	= 0x7E;
#else
	hhEmu->emu_highchar 	= 0xFFFF;
#endif

	hhEmu->emu_maxcol		= VT_MAXCOL_80MODE;
	hhEmu->fUse8BitCodes		= FALSE;
	hhEmu->mode_vt220			= FALSE;
	hhEmu->mode_vt320			= FALSE;
	//hhEmu->vt220_protectmode	= FALSE;
	hhEmu->mode_protect			= FALSE;

	if (hhEmu->nEmuLoaded == EMU_VT220)
		hhEmu->mode_vt220 = TRUE;

	else if (hhEmu->nEmuLoaded == EMU_VT320)
		hhEmu->mode_vt320 = TRUE;

	else
		assert(FALSE);

// UNDO:rde
//	pstPRI->vt220_protimg = 0;

	pstPRI->pstUDK			= VT220UserDefinedKeys;
	pstPRI->iUDKTableEntries= DIM(VT220UserDefinedKeys);

	pstPRI->pacUDKSelectors = acUDKSelectors;
	pstPRI->iUDKState		= KEY_NUMBER_NEXT;

	std_dsptbl(hhEmu, TRUE);
	vt_charset_init(hhEmu);

	switch(hhEmu->stUserSettings.nEmuId)
		{
	case EMU_VT220:
		hhEmu->mode_vt220		= TRUE;
		hhEmu->mode_vt320		= FALSE;
		vt220_reset(hhEmu, FALSE);
		break;

	case EMU_VT320:
		hhEmu->mode_vt220		= FALSE;
		hhEmu->mode_vt320		= TRUE;
		break;

	default:
		assert(FALSE);
		break;
		}

	backscrlSetShowFlag(sessQueryBackscrlHdl(hhEmu->hSession), TRUE);
	return;
	}
#endif // INCL_VT220

/* end of vt220ini.c */

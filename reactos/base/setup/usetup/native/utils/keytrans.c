/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/native/utils/keytrans.c
 * PURPOSE:         Console support functions: keyboard translation
 * PROGRAMMER:      Tinus
 *
 * NB: Hardcoded to US keyboard
 */
#include <usetup.h>
#include "keytrans.h"

#define NDEBUG
#include <debug.h>

static WORD KeyTable[] = {
/* 0x00 */
	0x00,		VK_ESCAPE,	0x31,		0x32,
	0x33,		0x34,		0x35,		0x36,
	0x37,		0x38,		0x39,		0x30,
	VK_OEM_MINUS,	VK_OEM_PLUS,	VK_BACK,	VK_TAB,
/* 0x10 */
	0x51,		0x57,		0x45,		0x52,
	0x54,		0x59,		0x55,		0x49,
	0x4f,		0x50,		VK_OEM_4,	VK_OEM_6,
	VK_RETURN,	VK_CONTROL,	0x41,		0x53,
/* 0x20 */
	0x44,		0x46,		0x47,		0x48,
	0x4a,		0x4b,		0x4c,		VK_OEM_1,
	VK_OEM_7,	0xc0,		VK_LSHIFT,	VK_OEM_5,
	0x5a,		0x58,		0x43,		0x56,
/* 0x30 */
	0x42,		0x4e,		0x4d,		VK_OEM_COMMA,
	VK_OEM_PERIOD,	VK_OEM_2,	VK_RSHIFT,	VK_MULTIPLY,
	VK_LMENU,	VK_SPACE,	VK_CAPITAL,	VK_F1,
	VK_F2,		VK_F3,		VK_F4,		VK_F5,
/* 0x40 */
	VK_F6,		VK_F7,		VK_F8,		VK_F9,
	VK_F10,		VK_NUMLOCK,	VK_SCROLL,	VK_HOME,
	VK_UP,		VK_PRIOR,	VK_SUBTRACT,	VK_LEFT,
	0,		VK_RIGHT,	VK_ADD,		VK_END,
/* 0x50 */
	VK_DOWN,	VK_NEXT,	VK_INSERT,	VK_DELETE,
	0,		0,		0,		VK_F11,
	VK_F12,		0,		0,		0,
	0,		0,		0,		0,
/* 0x60 */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
/* 0x70 */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0
};

static WORD KeyTableEnhanced[] = {
/* 0x00 */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
/* 0x10 */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	VK_RETURN,	VK_RCONTROL,	0,		0,
/* 0x20 */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
/* 0x30 */
	0,		0,		0,		0,
	0,		VK_DIVIDE,	0,		VK_SNAPSHOT,
	VK_RMENU,	0,		0,		0,
	0,		0,		0,		0,
/* 0x40 */
	0,		0,		0,		0,
	0,		0,		0,		VK_HOME,
	VK_UP,		VK_PRIOR,	0,		VK_LEFT,
	0,		VK_RIGHT,	0,		VK_END,
/* 0x50 */
	VK_DOWN,	VK_NEXT,	VK_INSERT,	VK_DELETE,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
/* 0x60 */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
/* 0x70 */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0
};

/*
 * Note how the keyboard lights are not handled, so while NUMLOCK_ON can
 * be on, the light will never be. If this starts to be a problem it can be
 * fixed, but it's too much work for too little gain to do now.
 * Look in win32k/ntuser/input.c for an example.
 */

static WORD KeyTableNumlock[] = {
/* 0x00 */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
/* 0x10 */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
/* 0x20 */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
/* 0x30 */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
/* 0x40 */
	0,		0,		0,		0,
	0,		0,		0,		VK_NUMPAD7,
	VK_NUMPAD8,	VK_NUMPAD9,	0,		VK_NUMPAD4,
	VK_NUMPAD5,	VK_NUMPAD6,	0,		VK_NUMPAD1,
/* 0x50 */
	VK_NUMPAD2,	VK_NUMPAD3,	VK_NUMPAD0,	0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
/* 0x60 */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
/* 0x70 */
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0
};

typedef struct _SCANTOASCII {
	USHORT ScanCode;
	USHORT Enhanced;
	UCHAR Normal;
	UCHAR Shift;
	UCHAR NumLock;
} SCANTOASCII, *PSCANTOASCII;

SCANTOASCII ScanToAscii[] = {
{	0x1e,	0,	'a',	'A',	0	},
{	0x30,	0,	'b',	'B',	0	},
{	0x2e,	0,	'c',	'C',	0	},
{	0x20,	0,	'd',	'D',	0	},
{	0x12,	0,	'e',	'E',	0	},
{	0x21,	0,	'f',	'F',	0	},
{	0x22,	0,	'g',	'G',	0	},
{	0x23,	0,	'h',	'H',	0	},
{	0x17,	0,	'i',	'I',	0	},
{	0x24,	0,	'j',	'J',	0	},
{	0x25,	0,	'k',	'K',	0	},
{	0x26,	0,	'l',	'L',	0	},
{	0x32,	0,	'm',	'M',	0	},
{	0x31,	0,	'n',	'N',	0	},
{	0x18,	0,	'o',	'O',	0	},
{	0x19,	0,	'p',	'P',	0	},
{	0x10,	0,	'q',	'Q',	0	},
{	0x13,	0,	'r',	'R',	0	},
{	0x1f,	0,	's',	'S',	0	},
{	0x14,	0,	't',	'T',	0	},
{	0x16,	0,	'u',	'U',	0	},
{	0x2f,	0,	'v',	'V',	0	},
{	0x11,	0,	'w',	'W',	0	},
{	0x2d,	0,	'x',	'X',	0	},
{	0x15,	0,	'y',	'Y',	0	},
{	0x2c,	0,	'z',	'Z',	0	},

{	0x02,	0,	'1',	'!',	0	},
{	0x03,	0,	'2',	'@',	0	},
{	0x04,	0,	'3',	'#',	0	},
{	0x05,	0,	'4',	'$',	0	},
{	0x06,	0,	'5',	'%',	0	},
{	0x07,	0,	'6',	'^',	0	},
{	0x08,	0,	'7',	'&',	0	},
{	0x09,	0,	'8',	'*',	0	},
{	0x0a,	0,	'9',	'(',	0	},
{	0x0b,	0,	'0',	')',	0	},

{	0x29,	0,	'\'',	'~',	0	},
{	0x0c,	0,	'-',	'_',	0	},
{	0x0d,	0,	'=',	'+',	0	},
{	0x1a,	0,	'[',	'{',	0	},
{	0x1b,	0,	']',	'}',	0	},
{	0x2b,	0,	'\\',	'|',	0	},
{	0x27,	0,	';',	':',	0	},
{	0x28,	0,	'\'',	'"',	0	},
{	0x33,	0,	',',	'<',	0	},
{	0x34,	0,	'.',	'>',	0	},
{	0x35,	0,	'/',	'?',	0	},

{	0x4f,	0,	0,	0,	'1'	},
{	0x50,	0,	0,	0,	'2'	},
{	0x51,	0,	0,	0,	'3'	},
{	0x4b,	0,	0,	0,	'4'	},
{	0x4c,	0,	0,	0,	'5'	},
{	0x4d,	0,	0,	0,	'6'	},
{	0x47,	0,	0,	0,	'7'	},
{	0x48,	0,	0,	0,	'8'	},
{	0x49,	0,	0,	0,	'9'	},
{	0x52,	0,	0,	0,	'0'	},

{	0x4a,	0,	'-',	'-',	0	},
{	0x4e,	0,	'+',	'+',	0	},
{	0x37,	0,	'*',	'*',	0	},
{	0x35,	1,	'/',	'/',	0	},
{	0x53,	0,	0,	0,	'.'	},

{	0x39,	0,	' ',	' ',	0	},

{	0x1c,	0,	'\r',	'\r',	0	},
{	0x1c,	1,	'\r',	'\r',	0	},
{	0x0e,	0,	0x08,	0x08,	0	}, /* backspace */

{	0,	0,	0,	0,	0	}
};


static void
IntUpdateControlKeyState(LPDWORD State, PKEYBOARD_INPUT_DATA InputData)
{
	DWORD Value = 0;

	if (InputData->Flags & KEY_E1) /* Only the pause key has E1 */
		return;

	if (!(InputData->Flags & KEY_E0)) {
		switch (InputData->MakeCode) {
			case 0x2a:
			case 0x36:
				Value = SHIFT_PRESSED;
				break;

			case 0x1d:
				Value = LEFT_CTRL_PRESSED;
				break;

			case 0x38:
				Value = LEFT_ALT_PRESSED;
				break;

			case 0x45:
				Value = NUMLOCK_ON;
				if (!(InputData->Flags & KEY_BREAK))
					*State ^= Value;
				return;
			default:
				return;
		}
	} else {
		switch (InputData->MakeCode) {
			case 0x1d:
				Value = RIGHT_CTRL_PRESSED;
				break;

			case 0x38:
				Value = RIGHT_ALT_PRESSED;
				break;

			default:
				return;
		}
	}

	if (InputData->Flags & KEY_BREAK)
		*State &= ~Value;
	else
		*State |= Value;
}

static DWORD
IntVKFromKbdInput(PKEYBOARD_INPUT_DATA InputData, DWORD KeyState)
{
	if (!(KeyState & ENHANCED_KEY)) {
		if ((KeyState & NUMLOCK_ON) &&
		    KeyTableNumlock[InputData->MakeCode & 0x7f]) {
			DPRINT("Numlock, using %x\n",
			       InputData->MakeCode & 0x7f);
			return KeyTableNumlock[InputData->MakeCode & 0x7f];
		}
		DPRINT("Not enhanced, using %x\n", InputData->MakeCode & 0x7f);
		return KeyTable[InputData->MakeCode & 0x7f];
	}

	DPRINT("Enhanced, using %x\n", InputData->MakeCode & 0x7f);
	return KeyTableEnhanced[InputData->MakeCode & 0x7f];
}

static UCHAR
IntAsciiFromInput(PKEYBOARD_INPUT_DATA InputData, DWORD KeyState)
{
	UINT Counter = 0;
	USHORT Enhanced = 0;

	if (KeyState & ENHANCED_KEY) Enhanced = 1;

	while (ScanToAscii[Counter].ScanCode != 0) {
		if ((ScanToAscii[Counter].ScanCode == InputData->MakeCode)  &&
		    (ScanToAscii[Counter].Enhanced == Enhanced)) {
			if (ScanToAscii[Counter].NumLock) {
				if ((KeyState & NUMLOCK_ON) &&
				    !(KeyState & SHIFT_PRESSED)) {
					return ScanToAscii[Counter].NumLock;
				} else {
					return ScanToAscii[Counter].Normal;
				}
			}
			if (KeyState & SHIFT_PRESSED)
				return ScanToAscii[Counter].Shift;

			return ScanToAscii[Counter].Normal;
		}
		Counter++;
	}

	return 0;
}

/* This is going to be quick and messy. The usetup app runs in native mode
 * so it cannot use the translation routines in win32k which means it'll have
 * to be done here too.
 *
 * Only the bKeyDown, AsciiChar and wVirtualKeyCode members are used
 * in the app so I'll just fill the others with somewhat sane values
 */
NTSTATUS
IntTranslateKey(PKEYBOARD_INPUT_DATA InputData, KEY_EVENT_RECORD *Event)
{
	static DWORD dwControlKeyState;

	RtlZeroMemory(Event, sizeof(KEY_EVENT_RECORD));

	if (!(InputData->Flags & KEY_BREAK))
		Event->bKeyDown = TRUE;
	else
		Event->bKeyDown = FALSE;

	Event->wRepeatCount = 1;
	Event->wVirtualScanCode = InputData->MakeCode;

	DPRINT("Translating: %x\n", InputData->MakeCode);

	IntUpdateControlKeyState(&dwControlKeyState, InputData);
	Event->dwControlKeyState = dwControlKeyState;

	if (InputData->Flags & KEY_E0)
		Event->dwControlKeyState |= ENHANCED_KEY;

	Event->wVirtualKeyCode = IntVKFromKbdInput(InputData,
	                                           Event->dwControlKeyState);

	DPRINT("Result: %x\n", Event->wVirtualKeyCode);

	if (Event->bKeyDown) {
		Event->uChar.AsciiChar =
		                   IntAsciiFromInput(InputData,
		                                     Event->dwControlKeyState);
		DPRINT("Char: %x\n", Event->uChar.AsciiChar);
	} else {
		Event->uChar.AsciiChar = 0;
	}

	return STATUS_SUCCESS;
}

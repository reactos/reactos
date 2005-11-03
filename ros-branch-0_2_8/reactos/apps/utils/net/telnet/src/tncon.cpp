///////////////////////////////////////////////////////////////////////////////
//Telnet Win32 : an ANSI telnet client.
//Copyright (C) 1998-2000 Paul Brannan
//Copyright (C) 1998 I.Ioannou
//Copyright (C) 1997 Brad Johnson
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//I.Ioannou
//roryt@hol.gr
//
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// Module:		tncon.cpp
//
// Contents:	telnet console processing
//
// Product:		telnet
//
// Revisions: August 30, 1998 Paul Brannan <pbranna@clemson.edu>
//            July 29, 1998 Paul Brannan
//            June 15, 1998 Paul Brannan
//            May 16, 1998 Paul Brannan
//            5.April.1997 jbj@nounname.com
//            9.Dec.1996 jbj@nounname.com
//            Version 2.0
//
//            02.Apr.1995	igor.milavec@uni-lj.si
//					  Original code
//
///////////////////////////////////////////////////////////////////////////////
#include "tncon.h"
#include "keytrans.h"
#include "ttelhndl.h"
#include "tconsole.h"

#define KEYEVENT InputRecord[i].Event.KeyEvent

// Paul Brannan 6/25/98
// #ifdef __MINGW32__
// #define KEYEVENT_CHAR KEYEVENT.AsciiChar
// #else
#define KEYEVENT_CHAR KEYEVENT.uChar.AsciiChar
// #endif

#define KEYEVENT_PCHAR &KEYEVENT_CHAR

// This is for local echo (Paul Brannan 5/16/98)
inline void DoEcho(const char *p, int l, TConsole &Console,
				   TNetwork &Network, NetParams *pParams) {
	// Pause the console (Paul Brannan 8/24/98)
	if(Network.get_local_echo()) {
		ResetEvent(pParams->hUnPause);
		SetEvent(pParams->hPause);
		while (!*pParams->bNetPaused); // Pause

		Console.WriteCtrlString(p, l);

		SetEvent(pParams->hUnPause); // Unpause
	}
}

// This is for line mode (Paul Brannan 12/31/98)
static char buffer[1024];
static unsigned int bufptr = 0;

// Line mode -- currently uses sga/echo to determine when to enter line mode
// (as in RFC 858), but correct behaviour is as described in RFC 1184.
// (Paul Brannan 12/31/98)
// FIX ME!!  What to do with unflushed data when we change from line mode
// to character mode?
inline bool DoLineModeSpecial(char keychar, TConsole &Console, TNetwork &Network,
					   NetParams *pParams) {
	if(keychar == VK_BACK) {
		if(bufptr) bufptr--;
		DoEcho("\b \b", 3, Console, Network, pParams);
		return true;
	} else if(keychar == VK_RETURN) {
		Network.WriteString(buffer, bufptr);
		Network.WriteString("\012", 1);
		DoEcho("\r\n", 2, Console, Network, pParams);
		bufptr = 0;
		return true;
	}
	return false;
}

inline void DoLineMode(const char *p, int p_len, TConsole &Console,
					   TNetwork &Network) {
	if(Network.get_line_mode()) {
		if(bufptr < sizeof(buffer) + p_len - 1) {
			memcpy(buffer + bufptr, p, p_len);
			bufptr += p_len;
		} else {
			Console.Beep();
		}
	} else {
		Network.WriteString(p, p_len);
	}
}

// Paul Brannan 5/27/98
// Fixed this code for use with appliation cursor keys
// This should probably be optimized; it's pretty ugly as it is
// Rewrite #1: now uses ClosestStateKey (Paul Brannan 12/9/98)
const char *ClosestStateKey(WORD keyCode, DWORD keyState,
							KeyTranslator &KeyTrans) {
	char const *p;

	if((p = KeyTrans.TranslateKey(keyCode, keyState))) return p;

	// Check numlock and scroll lock (Paul Brannan 9/23/98)
	if((p = KeyTrans.TranslateKey(keyCode, keyState & ~NUMLOCK_ON))) return p;
	if((p = KeyTrans.TranslateKey(keyCode, keyState & ~ENHANCED_KEY
		& ~NUMLOCK_ON))) return p;
	if((p = KeyTrans.TranslateKey(keyCode, keyState & ~SCROLLLOCK_ON))) return p;
	if((p = KeyTrans.TranslateKey(keyCode, keyState & ~ENHANCED_KEY
		& ~SCROLLLOCK_ON))) return p;

	//	 John Ioannou (roryt@hol.gr)
	//   Athens 31/03/97 00:25am GMT+2
	//   fix for win95 CAPSLOCK bug
	//   first check if the user has keys with capslock and then we filter it
	if((p = KeyTrans.TranslateKey(keyCode, keyState & ~ENHANCED_KEY))) return p;
	if((p = KeyTrans.TranslateKey(keyCode, keyState & ~CAPSLOCK_ON))) return p;
	if((p = KeyTrans.TranslateKey(keyCode, keyState & ~ENHANCED_KEY
		& ~CAPSLOCK_ON))) return p;

	return 0; // we couldn't find a suitable key translation
}

const char *FindClosestKey(WORD keyCode, DWORD keyState,
						   KeyTranslator &KeyTrans) {
	char const *p;
	
	// Paul Brannan 7/20/98
	if(ini.get_alt_erase()) {
		if(keyCode == VK_BACK) {
			keyCode = VK_DELETE;
			keyState |= ENHANCED_KEY;
		} else if(keyCode == VK_DELETE && (keyState & ENHANCED_KEY)) {
			keyCode = VK_BACK;
			keyState &= ~ENHANCED_KEY;
		}
	}

	DWORD ext_mode = KeyTrans.get_ext_mode();
	if(ext_mode) {
		// Not as fast as an unrolled loop, but certainly more
		// compact (Paul Brannan 12/9/98)
		for(DWORD j = ext_mode; j >= APP_KEY; j -= APP_KEY) {
			if((j | ext_mode) == ext_mode) {
				if((p = ClosestStateKey(keyCode, keyState | j,
					KeyTrans))) return p;
			}
		}
	}
	return ClosestStateKey(keyCode, keyState, KeyTrans);
}

// Paul Brannan Feb. 22, 1999
int do_op(tn_ops op, TNetwork &Network, Tnclip &Clipboard) {
	switch(op) {
	case TN_ESCAPE:
		return TNPROMPT;
	case TN_SCROLLBACK:
		return TNSCROLLBACK;
	case TN_DIAL:
		return TNSPAWN;
	case TN_PASTE:
		if(ini.get_keyboard_paste()) Clipboard.Paste();
		else return 0;
		break;
	case TN_NULL:
		Network.WriteString("", 1);
		return 0;
	case TN_CR:
		Network.WriteString("\r", 2); // CR must be followed by NUL
		return 0;
	case TN_CRLF:
		Network.WriteString("\r\n", 2);
		return 0;
	}
	return 0;
}

int telProcessConsole(NetParams *pParams, KeyTranslator &KeyTrans,
					  TConsole &Console, TNetwork &Network, TMouse &Mouse,
					  Tnclip &Clipboard, HANDLE hThread)
{
	KeyDefType_const keydef;
	const char *p;
	int p_len;
	unsigned int i;
	int opval;
	HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleMode(hConsole, ini.get_enable_mouse() ? ENABLE_MOUSE_INPUT : 0);
	
	const DWORD nHandle = 2;
	HANDLE hHandle[nHandle] = {hConsole, pParams->hExit};
	
	for (;;) {
		DWORD dwInput;
		switch (WaitForMultipleObjects(nHandle, hHandle, FALSE, INFINITE)) {
        case WAIT_OBJECT_0: {

			// Paul Brannan 7/29/98
			if(ini.get_input_redir()) {
				char InputBuffer[10];

				// Correction from Joe Manns <joe.manns@ardenenginneers.com>
				// to fix race conditions (4/13/99)
				int bResult;
				bResult = ReadFile(hConsole, InputBuffer, 10, &dwInput, 0);
				if(bResult && dwInput == 0) return TNNOCON;

				// no key translation for redirected input
				Network.WriteString(InputBuffer, dwInput);
				break;
			}

			INPUT_RECORD InputRecord[11];
			if (!ReadConsoleInput(hConsole, &InputRecord[0], 10, &dwInput))
				return TNPROMPT;
			
			for (i = 0; (unsigned)i < dwInput; i++){
				switch (InputRecord[i].EventType) {
				case KEY_EVENT:{
					if (KEYEVENT.bKeyDown) {

						WORD  keyCode  = KEYEVENT.wVirtualKeyCode;
						DWORD keyState = KEYEVENT.dwControlKeyState;
						
						// Paul Brannan 5/27/98
						// Moved the code that was here to FindClosestKey()
						keydef.szKeyDef = FindClosestKey(keyCode,
							keyState, KeyTrans);

						if(keydef.szKeyDef) {
							if(!keydef.op->sendstr)
								if((opval = do_op(keydef.op->the_op, Network,
									Clipboard)) != 0)
									return opval;
						}

						if(Network.get_line_mode()) {
							if(DoLineModeSpecial(KEYEVENT_CHAR, Console, Network, pParams))
								continue;
						}

						p = keydef.szKeyDef;
						if (p == NULL) { // if we don't have a translator
							if(!KEYEVENT_CHAR) continue;
							p_len = 1;
							p = KEYEVENT_PCHAR;
						} else {
							p_len = strlen(p);
						}

						// Local echo (Paul Brannan 5/16/98)
						DoEcho(p, p_len, Console, Network, pParams);
						// Line mode (Paul Brannan 12/31/98)
						DoLineMode(p, p_len, Console, Network);
					}
							   }
					break;

				case MOUSE_EVENT:
					if(!InputRecord[i].Event.MouseEvent.dwEventFlags) {
						ResetEvent(pParams->hUnPause);
						SetEvent(pParams->hPause);
						while (!*pParams->bNetPaused);	// thread paused
						// SuspendThread(hThread);

						// Put the mouse's X and Y coords back into the
						// input buffer
						DWORD Result;
						WriteConsoleInput(hConsole, &InputRecord[i], 1,
							&Result);

						Mouse.doMouse();

						SetEvent(pParams->hUnPause);
						// ResumeThread(hThread);
					}
					break;

				case FOCUS_EVENT:
					break;
				case WINDOW_BUFFER_SIZE_EVENT:
					// FIX ME!!  This should take care of the window re-sizing bug
					// Unfortunately, it doesn't.
					Console.sync();
					Network.do_naws(Console.GetWidth(), Console.GetHeight());
					break;
				}
			
			} // keep going until no more input
			break;
							}
        default:
			return TNNOCON;
		}
	}
}

WORD scrollkeys() {
	HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
	INPUT_RECORD InputRecord;
	BOOL done = FALSE;
	
	while (!done) {
		DWORD dwInput;
		WaitForSingleObject( hConsole, INFINITE );
		if (!ReadConsoleInput(hConsole, &InputRecord, 1, &dwInput)){
			done = TRUE;
			continue;
		}
		if (InputRecord.EventType == KEY_EVENT &&
			InputRecord.Event.KeyEvent.bKeyDown ) {
			// Why not just return the key code?  (Paul Brannan 12/5/98)
			return InputRecord.Event.KeyEvent.wVirtualKeyCode;
		} else if(InputRecord.EventType == MOUSE_EVENT) {
			if(!InputRecord.Event.MouseEvent.dwEventFlags) {
				// Put the mouse's X and Y coords back into the input buffer
				WriteConsoleInput(hConsole, &InputRecord, 1, &dwInput);
				return SC_MOUSE;
			}
		}
	}
	return SC_ESC;
}

// FIX ME!!  This is more evidence that tncon.cpp ought to have class structure
// (Paul Brannan 12/10/98)

// Bryan Montgomery 10/14/98
static TNetwork net;
void setTNetwork(TNetwork tnet) {
	net = tnet;
}

// Thomas Briggs 8/17/98
BOOL WINAPI ControlEventHandler(DWORD event) {
	switch(event) {
	case CTRL_BREAK_EVENT:
		// Bryan Montgomery 10/14/98
		if(ini.get_control_break_as_c()) net.WriteString("\x3",1);
		return TRUE;
	default:
		return FALSE;
	}
}

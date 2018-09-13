/*	File: D:\WACKER\tdll\getchar.c (Created: 30-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:20p $
 */

#include <windows.h>
#pragma hdrstop

//#define DEBUGSTR
#include <time.h>
#include <stdio.h>
#include "stdtyp.h"
#include "globals.h"
#include "session.h"
#include "term.h"
#include "chars.h"
#include "assert.h"
#include "statusbr.h"
#include "cloop.h"
#include "cnct.h"
#include <tdll\tchar.h>
#include <tdll\keyutil.h>
#include <emu\emu.h>
#include <term\res.h>

static BOOL WackerKeys(const KEY_T Key);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TranslateToKey
 *
 * DESCRIPTION:
 *	Translates a key into our internal format.	Waits for the WM_CHAR if
 *	windows is going to translate the key by checking the message queue.
 *
 * ARGUMENTS:
 *	hSession	- external session handle
 *	pmsg		- pointer to message
 *
 * RETURNS:
 *	Internal key value if translated, otherwise 0.
 *
 */
KEY_T TranslateToKey(const LPMSG pmsg)
	{
	KEY_T	 Key = 0;

	switch (pmsg->message)
		{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		switch (pmsg->wParam)
			{
			case VK_SHIFT:
			case VK_CONTROL:
				return 0;

			case VK_MENU:
				return (KEY_T)-1;

			default:
				Key = (KEY_T)(VIRTUAL_KEY | pmsg->wParam);

				if (GetKeyState(VK_MENU) < 0)
					Key |= ALT_KEY;

				if (GetKeyState(VK_CONTROL) < 0)
					Key |= CTRL_KEY;

				if (GetKeyState(VK_SHIFT) < 0)
					Key |= SHIFT_KEY;

				if (pmsg->lParam & 0x01000000)	/* Extended, bit 24 */
					Key |= EXTENDED_KEY;

				break;
			}
		break;

	case WM_CHAR:
	case WM_SYSCHAR:
		Key = (KEY_T)pmsg->wParam;

		if (pmsg->lParam & 0x01000000)	/* Extended, bit 24 */
			Key |= EXTENDED_KEY;

		if (pmsg->lParam & 0x20000000)	/* Context, bit 29 */
			Key |= ALT_KEY;

		if (pmsg->wParam == VK_TAB)
			{
			if (GetKeyState(VK_SHIFT) < 0)
				{
				Key |= SHIFT_KEY;
				Key |= VIRTUAL_KEY;
				}
			}

		// Believe it or not CTRL+SHIFT+@ gets translated to a
		// char of 0 (zero).  So virtualize the key if it matches
		// the criteria. - mrw
		//
		if (pmsg->wParam == 0)
			{
			if (GetKeyState(VK_SHIFT) < 0 && GetKeyState(VK_CONTROL) < 0)
				Key |= VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY;
			}

		break;

	default:
		break;
		}

	DbgOutStr("%x %x\r\n", Key, pmsg->message, 0, 0, 0);
	return Key;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	WackerKeys
 *
 * DESCRIPTION:
 *	Handles our special global keys.
 *
 * ARGUMENTS:
 *	Key - key from TranslateToKey()
 *
 * RETURNS:
 *	TRUE if this routine acts on it, FALSE if not.
 *
 */
static BOOL WackerKeys(const KEY_T Key)
	{
	BYTE pbKeyState[256];

	switch (Key)
		{
	case VIRTUAL_KEY | VK_SCROLL:
	case VIRTUAL_KEY | SHIFT_KEY | VK_SCROLL:
	case VIRTUAL_KEY | ALT_KEY | VK_SCROLL:
	case VIRTUAL_KEY | ALT_KEY | SHIFT_KEY | VK_SCROLL:
		// In the case of scroll lock, we want to toggle to the
		// previous state.	Only when it is destined for the terminal
		// window is it processed meaning it doesn't get here.

		GetKeyboardState(pbKeyState);

		if (GetKeyState(VK_SCROLL) & 1)
			pbKeyState[VK_SCROLL] &= ~0x01;

		else
			pbKeyState[VK_SCROLL] |= 0x01;

		SetKeyboardState(pbKeyState);
		break;

	default:
		return FALSE;
		}

	return TRUE;
	}

//******************************************************************************
// Method:
//    IsSessionMacroKey
//
// Description:
//    Determines if the specified key is a user defined macro key
//
// Arguments:
//    hSess - Session handle
//    Key   - The key to be tested
//
// Returns:
//    TRUE if the key is defined as a macro FALSE otherwise
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 06/10/1998
//
//

static BOOL IsSessionMacroKey(const HSESSION hSess, const KEY_T Key)
	{
#if defined INCL_KEY_MACROS
    keyMacro lKeyMacro;
    lKeyMacro.keyName = Key;

    return keysFindMacro( &lKeyMacro ) == -1 ? FALSE : TRUE;
#else
    return FALSE;
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ProcessMessage
 *
 * DESCRIPTION:
 *	Despite the apparent simplicity of this function it is any but simple.
 *	The entire functionality of the keyboard interface rests upon this
 *	function.  Handle with care!
 *
 * ARGUMENTS:
 *	pmsg	- pointer to message struct returned from GetMessage()
 *
 * RETURNS:
 *	void
 *
 */
void ProcessMessage(MSG *pmsg)
	{
	#if defined(FAR_EAST)
	static KEY_T keyLeadByte = 0;
	#endif
	KEY_T Key;

	HSESSION hSession;
	HCNCT hCnct;
	TCHAR achClassName[20];

	switch (pmsg->message)
		{
	case WM_CHAR:
		#if defined(FAR_EAST)
		hSession = (HSESSION)GetWindowLong(pmsg->hwnd, 0);
		if ((IsDBCSLeadByte( (BYTE) pmsg->wParam)) && (keyLeadByte == 0))
			{
			keyLeadByte = (KEY_T)pmsg->wParam;
			return ;
			}
		else
			{
			if (keyLeadByte != 0)
				{
				Key = (KEY_T)pmsg->wParam;
				CLoopSend(sessQueryCLoopHdl(hSession), &keyLeadByte, 1, CLOOP_KEYS);
				CLoopSend(sessQueryCLoopHdl(hSession), &Key, 1, CLOOP_KEYS);
				keyLeadByte = 0;
				return ;
				}

			keyLeadByte = 0;
			}
		#endif
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_SYSCHAR:
		/* --- Translate the key to our format --- */

		Key = TranslateToKey(pmsg);

		// We need to decide if the window this message is going to is
		// a terminal window since that's the only place we do our
		// translations.  We check if the window class matches our
		// terminal class.	If so, the session handle is stored in the
		// 0 offset of the window's extra data.  This way multiple sessions
		// can be serviced from this one routine.

		if (GetClassName(pmsg->hwnd, achClassName, sizeof(achClassName)) == 0)
			break;

		if (StrCharCmp(achClassName, TERM_CLASS))
			break;

		hSession = (HSESSION)GetWindowLong(pmsg->hwnd, 0);

		if (hSession == 0)
			{
			// There are certain keys we want to handle regardless of
			// their destination.

			if (WackerKeys(Key))
				return;

			break;
			}

		// We need to prevent an F1 key event from initiating
		// a connection when "emu keys" is turned on.
		//
		if (Key == (VIRTUAL_KEY | VK_F1))
			{
			hCnct = sessQueryCnctHdl(hSession);
			assert(hCnct);

			if((cnctQueryStatus(hCnct) == CNCT_STATUS_TRUE) &&
			    emuIsEmuKey(sessQueryEmuHdl(hSession), Key))
				{
				// do nothing - fall through
				}
			else
				{
				// eat it

				// We handle the F1 key event in the terminal proc
				//    (to bring up help), so we don't need to here.
				return;
				}
			}

		// The order of evaluation is important here.  Both IsMacroKey()
		// and emuIsEmuKey() know if that the message is bound for the
		// terminal.  Also emuIsEmuKey() checks if terminal keys are
		// enabled.  Evaluating messages in this order keeps us from
		// having to "disable" accelerators when the user defines macro
		// or terminal keys that conflict with accelerator keys.

		if ( IsSessionMacroKey(hSession, Key) ||
				emuIsEmuKey(sessQueryEmuHdl(hSession), Key))
			{
			// We need to modify this message to an internal message
			// for two reasons.
			//
			// 1. Menu accelerators get translated in DispatchMessage().
			//	  This seems a little strange but hey, that's windows
			//	  for ya.
			//
			// 2. It's important to know if the key was an emulator key.
			//	  The emulator's keys take precedence over window's
			//	  accelerators yet the emulator is the last guy to see
			//	  the key.	For instance, PageUp can pageup through the
			//	  backscroll execept when it is mapped to an emulator
			//	  key.	Unfortuanately, the PageUp key is interperted
			//	  by the client-terminal before being passed to the
			//	  emulator.  If we sent this as a WM_KEYDOWN or WM_CHAR
			//	  message, we would have to play it through the emulator
			//	  again to find out if it is an emulator key.

			pmsg->message = WM_TERM_KEY;
			pmsg->wParam = (WPARAM)Key;

			DbgOutStr("Session or Macro key\r\n", 0, 0, 0, 0, 0);
			DispatchMessage(pmsg);

			if (Key == (VK_NUMLOCK | VIRTUAL_KEY | EXTENDED_KEY))
				{
				static BYTE abKeyState[256];

				GetKeyboardState(abKeyState);

				if ((GetKeyState(VK_NUMLOCK) & 1))
					abKeyState[VK_NUMLOCK] &= 0xfe;

				else
					abKeyState[VK_NUMLOCK] |= 0x01;

				SetKeyboardState(abKeyState);
				}

			return;
			}

		else
			{
			// Win32 got this one right.  TranslateMesssage returns TRUE
			// only if it translates (ie. produces a WM_CHAR).	Win31
			// didn't do this.  If a WM_CHAR is generated, then we want
			// to eat the WM_KEYDOWN and wait for the WM_CHAR event.

			// Bug in TranslateMessage().  It returns TRUE on all
			// WM_KEYDOWN messages regardless of translation.  Reported
			// bug 1/5/93

			if (!TranslateAccelerator(glblQueryHwndFrame(),
					glblQueryAccelHdl(), pmsg))
				{
				MSG msg;

				TranslateMessage(pmsg);

				if (PeekMessage(&msg, pmsg->hwnd, WM_CHAR, WM_CHAR,
						PM_NOREMOVE) == FALSE)
					{
					DispatchMessage(pmsg);
					}
				}

			return;
			}

		default:
			break;
		}

	// Not for the terminal window? Do the normal thing...

	if (!TranslateAccelerator(glblQueryHwndFrame(), glblQueryAccelHdl(), pmsg))
		{
		TranslateMessage(pmsg);
		DispatchMessage(pmsg);
		}

	return;
	}

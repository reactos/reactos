//////////////////////////////////////////////////////////////////////////////
//
//  KBDDLL.C -
//
//      Windows Keyboard Select Dynalink Source File
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "kbddll.h"


#pragma data_seg(".MYSEC")
HWND ghwndMsg = NULL;   // the handle back to the keyboard executable
WORD fHotKeys = 0;
BYTE retStatus = 0;
#pragma data_seg()


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: LibMain(HANDLE, DWORD, LPVOID)
//
//  PURPOSE:  Initialize the DLL
//
//////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY LibMain( HANDLE hDll, DWORD dwReason, LPVOID lpReserved )
{
    return (TRUE);
}


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: InitKbdHook(HWND, WORD)
//
//  PURPOSE:  Initialize keyboard hook
//
//////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY InitKbdHook( HWND hwnd, WORD keyCombo )
{
    ghwndMsg = hwnd;
    fHotKeys = keyCombo;

    return (TRUE);
}


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: KbdGetMsgProc(INT, WPARAM, LPARAM)
//
//  PURPOSE:  The Get Message hook function
//
//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK KbdGetMsgProc( INT hc, WPARAM wParam, LPARAM lParam )
{
    DWORD pid;

    if (hc >= HC_ACTION) {
        //
        // key going up?
        //
        if ((lParam & 0xc0000000) == 0xc0000000) {
	    //OutputDebugStringA("Keyup\n");
            //
            // is it a menu, control or shift key depressed?
            //
            if (retStatus && (wParam == VK_MENU || wParam == VK_CONTROL ||
                              wParam == VK_SHIFT)) {
		//OutputDebugStringA("MENU | CTRL | SHIFT\n");
                //
                // is Tab or Escape key pressed?
                //
                if (!(GetKeyState (VK_TAB) & 0x8000) &&
                    !(GetKeyState (VK_ESCAPE) & 0x8000)) {
		    //OutputDebugStringA("Neither TAB | ESCAPE are down\n");
                    if (fHotKeys == ALT_SHIFT_COMBO &&
                        !(GetKeyState (VK_CONTROL) & 0x8000)) {
			//OutputDebugStringA("CONTROL & ALT_SHIFT_COMBO\n");
                        PostMessage (ghwndMsg, WM_USER, retStatus, 0);
                    }
                    else if (fHotKeys == CTRL_SHIFT_COMBO &&
                             !(GetKeyState (VK_MENU) & 0x8000)) {
			//OutputDebugStringA("MENU & CTRL_SHIFT_COMBO\n");
                        PostMessage (ghwndMsg, WM_USER, retStatus, 0);
                    }
                }
            }
            retStatus = 0;
            goto CallNext;
        }

        //
        // ignore other keys going up
        //
        if ((lParam & 0xc0000000) == 0xc0000000)
            goto CallNext;

        //
        // is it a menu key?
        //
	//OutputDebugStringA("Keydown\n");
        if ((fHotKeys == ALT_SHIFT_COMBO && wParam == VK_MENU) ||
            (fHotKeys == CTRL_SHIFT_COMBO && wParam == VK_CONTROL)) {
	    //OutputDebugStringA("(ALT_SHIFT & MENU) | (CTRL_SHIFT & CONTROL)\n");
SetState:
            if (GetKeyState (VK_LSHIFT) & 0x8000) {
		//OutputDebugStringA("Left Shift\n");
                retStatus = KEY_PRIMARY_KL;
	    }
            else if (GetKeyState (VK_RSHIFT) & 0x8000) {
		//OutputDebugStringA("Right Shift\n");
                retStatus = KEY_ALTERNATE_KL;
	    }
            //else if (GetKeyState (VK_SHIFT) & 0x8000)
		//OutputDebugStringA("SHIFT\n");
	    //else
		//OutputDebugStringA("Neither Left || Right SHIFT is down\n");
        }
        //
        // is it a shift key?
        //
        else if (wParam == VK_SHIFT) {
	    //OutputDebugStringA("SHIFT\n");
            if ((fHotKeys == ALT_SHIFT_COMBO && GetKeyState (VK_MENU) & 0x8000) ||
                (fHotKeys == CTRL_SHIFT_COMBO && GetKeyState (VK_CONTROL) & 0x8000)) {
		//OutputDebugStringA("(ALT_SHIFT & MENU) | (CTRL_SHIFT & CONTROL)\n");
                goto SetState;
	    }
	    //OutputDebugStringA("Neither MENU || CONTROL is down\n");
        }
        //
        // all other keys - reset retStatus
        //
        else {
	    //OutputDebugStringA("Not MENU || CONTROL || SHIFT\n");
            retStatus = 0;
	}
    }

    //
    // Note that CallNextHookEx ignores the first parameter (hhook) so
    // it is acceptable (barely) to pass in a NULL.
    //
CallNext:
    return CallNextHookEx (NULL, hc, wParam, lParam);
}

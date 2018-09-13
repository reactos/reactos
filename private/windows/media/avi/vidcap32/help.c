/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
/****************************************************************************
 *
 *   help.c: Help system interface
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/

/*
 * supports F1 key help in app and in dialog by installing a hook,
 *
 * Keep track of which dialog is currently displayed in a global:
 * dialog ids are also topic ids in the help file.
 */


#include <windows.h>
#include <windowsx.h>
#include <string.h>

#include "help.h"

int CurrentDialogID = 0;


// app info passed to helpinit
HINSTANCE hInstance;
TCHAR HelpFile[MAX_PATH];
HWND hwndApp;

//hook proc and old msg filter
#ifdef _WIN32
HHOOK hOurHook;
#else
FARPROC fnOldMsgFilter = NULL;
FARPROC fnMsgHook = NULL;
#endif


// call DialogBoxParam, but ensuring correct help processing:
// assumes that each Dialog Box ID is a context number in the help file.
// calls MakeProcInstance as necessary. Uses instance data passed to
// HelpInit().
INT_PTR
DoDialog(
   HWND hwndParent,     // parent window
   int DialogID,        // dialog resource id
   DLGPROC fnDialog,    // dialog proc
   LPARAM lParam          // passed as lparam in WM_INITDIALOG
)
{
    int olddialog;
    DLGPROC fn;
    INT_PTR result;

    // remember current id (for nested dialogs)
    olddialog = CurrentDialogID;

    // save the current id so the hook proc knows what help to display
    CurrentDialogID = DialogID;

    fn = (DLGPROC) MakeProcInstance(fnDialog, hInstance);
    result = DialogBoxParam(
                hInstance,
                MAKEINTRESOURCE(CurrentDialogID),
                hwndParent,
                fn,
                lParam);
    FreeProcInstance(fn);
    CurrentDialogID = olddialog;

    return result;
}


// set the help context id for a dialog displayed other than by DoDialog
// (eg by GetOpenFileName). Returns the old help context that you must
// restore by a further call to this function
int
SetCurrentHelpContext(int DialogID)
{
    int oldid = CurrentDialogID;
    CurrentDialogID = DialogID;
    return(oldid);
}



// return TRUE if lpMsg is a non-repeat F1 key message
BOOL
IsHelpKey(LPMSG lpMsg)
{
    return lpMsg->message == WM_KEYDOWN &&
               lpMsg->wParam == VK_F1 &&
               !(HIWORD(lpMsg->lParam) & KF_REPEAT) &&
               GetKeyState(VK_SHIFT) >= 0 &&
               GetKeyState(VK_CONTROL) >= 0 &&
               GetKeyState(VK_MENU) >= 0;
}



LRESULT CALLBACK
HelpMsgHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0) {
        if (IsHelpKey((LPMSG)lParam)) {
            if (CurrentDialogID != 0) {
                WinHelp(hwndApp, HelpFile, HELP_CONTEXT, CurrentDialogID);
            } else {
                WinHelp(hwndApp, HelpFile, HELP_CONTENTS, 0);
            }
        }
    }
#ifdef _WIN32
    return CallNextHookEx(hOurHook, nCode, wParam, lParam);
#else
    return DefHookProc(nCode, wParam, lParam, fnOldMsgFilter);
#endif

}





// help init - initialise the support for the F1 key help
BOOL
HelpInit(HINSTANCE hinstance, LPSTR helpfilepath, HWND hwnd)
{
    LPSTR pch;

    // save app details
    hwndApp = hwnd;
    hInstance = hinstance;

    // assume that the help file is in the same directory as the executable-
    // get the executable path, and replace the filename with the help
    // file name.
    GetModuleFileName(hinstance, HelpFile, sizeof(HelpFile));

    // find the final backslash, and append the help file name there
    pch = _fstrrchr(HelpFile, '\\');
    pch++;
    lstrcpy(pch, helpfilepath);

    // install a hook for msgs and save old one
#ifdef _WIN32
    hOurHook = SetWindowsHookEx(
                        WH_MSGFILTER,
                        (HOOKPROC) HelpMsgHook,
                        NULL, GetCurrentThreadId());
#else
    fnMsgHook = (FARPROC) MakeProcInstance(HelpMsgHook, hInstance);
    fnOldMsgFilter = SetWindowsHook(WH_MSGFILTER, (HOOKPROC) fnMsgHook);
#endif

    return(TRUE);
}



// shutdown the help system
void
HelpShutdown(void)
{
#ifdef _WIN32
    UnhookWindowsHookEx(hOurHook);
#else
    if (fnOldMsgFilter) {
        UnhookWindowsHook(WH_MSGFILTER, fnMsgHook);
        FreeProcInstance(fnMsgHook);
    }
#endif

    WinHelp(hwndApp, HelpFile, HELP_QUIT, 0);
}


// start help at the contents page
void
HelpContents(void)
{
    WinHelp(hwndApp, HelpFile, HELP_CONTENTS, 0);
}




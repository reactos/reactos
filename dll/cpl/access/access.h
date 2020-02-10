#ifndef _ACCESS_H
#define _ACCESS_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <commctrl.h>
#include <tchar.h>
#include <cpl.h>

#include "resource.h"

typedef struct _APPLET
{
    INT idIcon;
    INT idName;
    INT idDescription;
    APPLET_PROC AppletProc;
} APPLET, *PAPPLET;


typedef struct _GLOBAL_DATA
{
    /* Keyboard page */
    STICKYKEYS stickyKeys;
    STICKYKEYS oldStickyKeys;
    FILTERKEYS filterKeys;
    FILTERKEYS oldFilterKeys;
    TOGGLEKEYS toggleKeys;
    TOGGLEKEYS oldToggleKeys;
    BOOL bKeyboardPref;

    /* Sound page */
    SOUNDSENTRY ssSoundSentry;
    BOOL bShowSounds;

    /* Display page */
    HIGHCONTRAST highContrast;
    UINT uCaretBlinkTime;
    UINT uCaretWidth;
    BOOL fShowCaret;
    RECT rcCaret;
    RECT rcOldCaret;

    /* Mouse page */
    MOUSEKEYS mouseKeys;

    /* General page */
    ACCESSTIMEOUT accessTimeout;
    SERIALKEYS serialKeys;
    TCHAR szActivePort[MAX_PATH];
    TCHAR szPort[MAX_PATH];
    BOOL bWarningSounds;
    BOOL bSoundOnActivation;

} GLOBAL_DATA, *PGLOBAL_DATA;


extern HINSTANCE hApplet;

void ShowLastWin32Error(HWND hWndOwner);

INT_PTR CALLBACK DisplayPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK KeyboardPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MousePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SoundPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif /* _ACCESS_H */

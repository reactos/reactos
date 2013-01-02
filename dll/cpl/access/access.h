#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <prsht.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "resource.h"

typedef LONG (CALLBACK *APPLET_INITPROC)(VOID);

typedef struct _APPLET
{
    INT idIcon;
    INT idName;
    INT idDescription;
    APPLET_INITPROC AppletProc;
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

/* EOF */

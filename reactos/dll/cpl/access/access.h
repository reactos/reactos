#ifndef __CPL_SYSDM_H
#define __CPL_SYSDM_H

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
    /* keyboard page */
    STICKYKEYS stickyKeys;
    STICKYKEYS oldStickyKeys;
    FILTERKEYS filterKeys;
    FILTERKEYS oldFilterKeys;
    TOGGLEKEYS toggleKeys;
    TOGGLEKEYS oldToggleKeys;
    BOOL bKeyboardPref;

    /* sound page */
    SOUNDSENTRY ssSoundSentry;
    BOOL bShowSounds;

    /* display page */
    HIGHCONTRAST highContrast;
    UINT uCaretBlinkTime;
    UINT uCaretWidth;
    BOOL fShowCaret;
    RECT rcCaret;
    RECT rcOldCaret;

    /* mouse page */
    MOUSEKEYS mouseKeys;

    /* general page */
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

#endif /* __CPL_SYSDM_H */

/* EOF */

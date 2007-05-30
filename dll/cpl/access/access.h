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

extern HINSTANCE hApplet;

void ShowLastWin32Error(HWND hWndOwner);

INT_PTR CALLBACK DisplayPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK KeyboardPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MousePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SoundPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif /* __CPL_SYSDM_H */

/* EOF */

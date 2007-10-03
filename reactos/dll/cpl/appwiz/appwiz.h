#ifndef __CPL_APPWIZ_H
#define __CPL_APPWIZ_H

typedef LONG (CALLBACK *CPLAPPLET_PROC)(VOID);

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  CPLAPPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

/* remove.c */
INT_PTR CALLBACK
RemovePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* add.c */
INT_PTR CALLBACK
AddPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* rossetup.c */
INT_PTR CALLBACK
RosPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* createlink.c */
INT_PTR CALLBACK
WelcomeDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK
FinishDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

LONG CALLBACK
NewLinkHere(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2);

void ShowLastWin32Error(HWND hWndOwner);

#endif /* __CPL_APPWIZ_H */

/* EOF */

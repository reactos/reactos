#ifndef __CPL_SYSDM_H
#define __CPL_SYSDM_H

typedef LONG (CALLBACK *APPLET_INITPROC)(VOID);

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  APPLET_INITPROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

void ShowLastWin32Error(HWND hWndOwner);

BOOL CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ComputerPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK HardwarePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AdvancedPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK UserProfilePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


//BOOL CALLBACK EnvironmentDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


#endif /* __CPL_SYSDM_H */

/* EOF */

#ifndef __CPL_INPUT_H
#define __CPL_INPUT_H

typedef LONG (CALLBACK *CPLAPPLET_PROC)(VOID);

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  CPLAPPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

#define BEGIN_LAYOUT 5000
#define END_LAYOUT   5133

/* input.c */
VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc);

/* settings.c */
INT_PTR CALLBACK
SettingPageProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

/* advanced.c */
INT_PTR CALLBACK
AdvancedPageProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

/* langbar.c */
INT_PTR CALLBACK
LangBarDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

/* keysettings.c */
INT_PTR CALLBACK
KeySettingsDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

/* add.c */
INT_PTR CALLBACK
AddDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

/* changekeyseq.c */
INT_PTR CALLBACK
ChangeKeySeqDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

/* inputlangprop.c */
INT_PTR CALLBACK
InputLangPropDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

/* misc.c */
VOID
CreateKeyboardLayoutList(HWND hWnd);

void ShowLastWin32Error(HWND hWndOwner);

#endif /* __CPL_INPUT_H */

/* EOF */

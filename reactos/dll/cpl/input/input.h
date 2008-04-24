#ifndef __CPL_INPUT_H
#define __CPL_INPUT_H

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <process.h>

typedef LONG (CALLBACK *CPLAPPLET_PROC)(VOID);

typedef struct
{
    int idIcon;
    int idName;
    int idDescription;
    CPLAPPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;
extern HANDLE hProcessHeap;

// Character Count of a layout ID like "00000409"
#define CCH_LAYOUT_ID    8

// Maximum Character Count of a ULONG in decimal
#define CCH_ULONG_DEC    10

/* input.c */
VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc);

/* settings.c */
INT_PTR CALLBACK
SettingPageProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL
GetLayoutName(LPCTSTR lcid, LPTSTR name);
VOID
UpdateLayoutsList(VOID);

/* keysettings.c */
INT_PTR CALLBACK
KeySettingsDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

/* add.c */
INT_PTR CALLBACK
AddDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
VOID
CreateKeyboardLayoutList(VOID);

/* changekeyseq.c */
INT_PTR CALLBACK
ChangeKeySeqDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

/* inputlangprop.c */
INT_PTR CALLBACK
InputLangPropDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

void ShowLastWin32Error(HWND hWndOwner);

#endif /* __CPL_INPUT_H */

/* EOF */

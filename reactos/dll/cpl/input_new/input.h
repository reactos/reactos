#ifndef _INPUT_H
#define _INPUT_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <winuser.h>
#include <commctrl.h>
#include <tchar.h>

#include "resource.h"

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
SettingsPageProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL
GetLayoutName(LPCTSTR szLCID, LPTSTR szName);
VOID
UpdateLayoutsList(VOID);
BOOL
IsLayoutExists(LPTSTR szLayoutID, LPTSTR szLangID);

/* advsettings.c */
INT_PTR CALLBACK
AdvancedSettingsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* keysettings.c */
INT_PTR CALLBACK
KeySettingsDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
VOID
UpdateKeySettingsList(VOID);

/* add.c */
INT_PTR CALLBACK
AddDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
VOID
CreateKeyboardLayoutList(HWND hItemsList);
INT
GetLayoutCount(LPTSTR szLang);

/* changekeyseq.c */
INT_PTR CALLBACK
ChangeKeySeqDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
BOOL
GetHotkeys(LPTSTR szHotkey, LPTSTR szLangHotkey, LPTSTR szLayoutHotkey);

void ShowLastWin32Error(HWND hWndOwner);

#endif /* _INPUT_H */

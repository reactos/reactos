#ifndef __CPL_APPWIZ_H
#define __CPL_APPWIZ_H

#include <windows.h>
#include <windowsx.h> /* GET_X/Y_LPARAM */
#include <commctrl.h>
#include <cpl.h>
#include <prsht.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <process.h>
#include <prsht.h>
#include <shlobj.h>
#include <objbase.h>
#include <shobjidl.h>
#include <shlguid.h>

#include "resource.h"

typedef LONG (CALLBACK *CPLAPPLET_PROC)(VOID);

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  CPLAPPLET_PROC AppletProc;
} APPLET, *PAPPLET;

typedef struct
{
   WCHAR szTarget[MAX_PATH];
   WCHAR szWorkingDirectory[MAX_PATH];
   WCHAR szDescription[MAX_PATH];
   WCHAR szLinkName[MAX_PATH];
}CREATE_LINK_CONTEXT, *PCREATE_LINK_CONTEXT;


extern HINSTANCE hApplet;

/* remove.c */
INT_PTR CALLBACK
RemovePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK
InfoPropDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

VOID
AddListColumn(HWND hList, LPTSTR Caption);

VOID
FillSoftwareList(HWND hwndDlg, BOOL bShowUpdates);

VOID
AddItemsToViewControl(HWND hwndDlg);

VOID
FindItems(HWND hwndDlg);

VOID
CallUninstall(HWND hwndDlg, UINT Control, BOOL isUpdate);

VOID
GetCurrentView(HWND hwndDlg);

VOID
CallInformation(HWND hwndDlg);

VOID
ShowPopupMenu(HWND hwndDlg, UINT ResMenu, INT xPos, INT yPos);

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

/* updates.c */
INT_PTR CALLBACK
UpdatesPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void ShowLastWin32Error(HWND hWndOwner);

#endif /* __CPL_APPWIZ_H */

/* EOF */

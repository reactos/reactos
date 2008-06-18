#ifndef __WINETESTGUI_PRECOMP_H
#define __WINETESTGUI_PRECOMP_H

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>
#include "resource.h"

extern HINSTANCE hInstance;

typedef struct _MAIN_WND_INFO
{
    HWND  hMainWnd;
    HWND  hBrowseDlg;
    HWND  hStatus;
    int   nCmdShow;

    HICON hSmIcon;
    HICON hBgIcon;

    INT SelectedItem;/* selection number in the list view */
    BOOL bDlgOpen;
    BOOL bInMenuLoop;
    BOOL bIsUserAnAdmin;

    LPWSTR lpDllList;

} MAIN_WND_INFO, *PMAIN_WND_INFO;


/* browsewnd.c */
BOOL CALLBACK BrowseDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam);

/* misc.c */
VOID DisplayString(LPWSTR lpMsg);
VOID DisplayError(INT err);

#endif /* __WINETESTGUI_PRECOMP_H */

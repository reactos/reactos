#ifndef __WINETESTGUI_PRECOMP_H
#define __WINETESTGUI_PRECOMP_H

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>
#include "resource.h"

extern HINSTANCE hInstance;

#define MAX_RUN_CMD 256

typedef struct _TEST_ITEM
{
    WCHAR szSelectedExe[MAX_PATH];
    WCHAR szRunString[MAX_RUN_CMD];

} TEST_ITEM, *PTEST_ITEM;

typedef struct _MAIN_WND_INFO
{
    HWND hMainWnd;
    HWND hBrowseDlg;
    HWND hBrowseTV;
    HWND hStatus;
    HANDLE hPipe;
    int  nCmdShow;

    HICON hSmIcon;
    HICON hBgIcon;

    LPWSTR lpExeList;
    INT numExes;

    TEST_ITEM SelectedTest;

    BOOL bRunOnStart;
    BOOL bHideConsole;

} MAIN_WND_INFO, *PMAIN_WND_INFO;

/* dll exports */
wchar_t *GetTestName();
int GetModulesInTest(char **modules);
int RunTest(const char *lpTest);


/* mainwnd.c */
BOOL SendCommandToClient(PMAIN_WND_INFO pInfo, LPWSTR lpCommand);

/* browsewnd.c */
BOOL CALLBACK BrowseDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam);

/* optionswnd.c */
BOOL CALLBACK OptionsDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam);

/* misc.c */
HIMAGELIST InitImageList(UINT StartResource, UINT EndResource, UINT Width, UINT Height);
VOID DisplayMessage(LPWSTR lpMsg);
VOID DisplayError(INT err);
DWORD AnsiToUnicode(LPCSTR lpSrcStr, LPWSTR *lpDstStr);
DWORD UnicodeToAnsi(LPCWSTR lpSrcStr, LPSTR *lpDstStr);

#endif /* __WINETESTGUI_PRECOMP_H */

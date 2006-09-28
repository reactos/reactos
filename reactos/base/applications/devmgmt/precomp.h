#ifndef __DEVMGMT_PRECOMP_H
#define __DEVMGMT_PRECOMP_H

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h> /* GET_X/Y_LPARAM */
#include <stdio.h>
#include <tchar.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <commctrl.h>
#include "resource.h"

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif

#ifndef SB_SIMPLEID
#define SB_SIMPLEID 0xFF
#endif

#define NO_ITEM_SELECTED -1
#define MAX_KEY_LENGTH 256
#define MAX_DEV_LEN 1000


typedef struct _MAIN_WND_INFO
{
    HWND  hMainWnd;
    HWND  hTreeView;
    HWND  hStatus;
    HWND  hTool;
    HWND  hProgDlg;
    HMENU hShortcutMenu;
    int   nCmdShow;

    /* status flags */
    BOOL InMenuLoop : 1;

} MAIN_WND_INFO, *PMAIN_WND_INFO;


BOOL CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


/* servman.c */
extern HINSTANCE hInstance;
extern HANDLE ProcessHeap;

/* mainwnd.c */
typedef struct _MENU_HINT
{
    WORD CmdId;
    UINT HintId;
} MENU_HINT, *PMENU_HINT;

BOOL InitMainWindowImpl(VOID);
VOID UninitMainWindowImpl(VOID);
HWND CreateMainWindow(LPCTSTR lpCaption, int nCmdShow);


/* enumdevices.c */
HTREEITEM InitTreeView(PMAIN_WND_INFO Info);
VOID ListDevicesByType(PMAIN_WND_INFO Info, HTREEITEM hRoot);


/* misc.c */
INT AllocAndLoadString(OUT LPTSTR *lpTarget,
                       IN HINSTANCE hInst,
                       IN UINT uID);

DWORD LoadAndFormatString(IN HINSTANCE hInstance,
                          IN UINT uID,
                          OUT LPTSTR *lpTarget,
                          ...);

BOOL StatusBarLoadAndFormatString(IN HWND hStatusBar,
                                  IN INT PartId,
                                  IN HINSTANCE hInstance,
                                  IN UINT uID,
                                  ...);

BOOL StatusBarLoadString(IN HWND hStatusBar,
                         IN INT PartId,
                         IN HINSTANCE hInstance,
                         IN UINT uID);

INT GetTextFromEdit(OUT LPTSTR lpString,
                    IN HWND hDlg,
                    IN UINT Res);

VOID GetError(VOID);

VOID DisplayString(PTCHAR);

HIMAGELIST InitImageList(UINT NumButtons,
                         UINT StartResource,
                         UINT Width,
                         UINT Height);


#endif /* __DEVMGMT_PRECOMP_H */

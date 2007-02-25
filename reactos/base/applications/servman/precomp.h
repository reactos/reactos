#ifndef __SERVMAN_PRECOMP_H
#define __SERVMAN_PRECOMP_H

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h> /* GET_X/Y_LPARAM */
#include <stdio.h>
#include <tchar.h>
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
#define NUM_BUTTONS 14
#define PROGRESSRANGE 8


typedef struct _PROP_DLG_INFO
{
    HWND   hwndGenDlg;
    HWND   hwndDepDlg;
    LPTSTR lpServiceName;
    LPTSTR lpDisplayName;
    LPTSTR lpDescription;
    LPTSTR lpPathToExe;
    TCHAR  szStartupType;
    TCHAR  szServiceStatus[25];
    LPTSTR lpStartParams;

} PROP_DLG_INFO, *PPROP_DLG_INFO;

typedef struct _MAIN_WND_INFO
{
    HWND  hMainWnd;
    HWND  hListView;
    HWND  hStatus;
    HWND  hTool;
    HWND  hProgDlg;
    HMENU hShortcutMenu;
    int   nCmdShow;

    /* Stores the complete services array */
    ENUM_SERVICE_STATUS_PROCESS *pServiceStatus;

    /* Stores the current selected service */
    ENUM_SERVICE_STATUS_PROCESS *CurrentService;

    /* selection number in the list view */
    INT SelectedItem;

    struct _PROP_DLG_INFO *PropSheet;

    /* status flags */
    BOOL InMenuLoop : 1;

} MAIN_WND_INFO, *PMAIN_WND_INFO;


BOOL CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK CreateDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DeleteDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ProgressDialogProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam);


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

/* start */
BOOL DoStart(PMAIN_WND_INFO Info);

/* stop */
BOOL DoStop(PMAIN_WND_INFO Info);

/* control */
BOOL Control(PMAIN_WND_INFO Info, DWORD Control);

/* query.c */
ENUM_SERVICE_STATUS_PROCESS* GetSelectedService(PMAIN_WND_INFO Info);
BOOL SetDescription(LPTSTR, LPTSTR);
BOOL GetDescription(LPTSTR, LPTSTR *);
BOOL GetExecutablePath(PMAIN_WND_INFO Info, LPTSTR *);
BOOL RefreshServiceList(PMAIN_WND_INFO Info);
DWORD GetServiceList(PMAIN_WND_INFO Info);

/* propsheet.c */
LONG APIENTRY OpenPropSheet(PMAIN_WND_INFO Info);

/* export.c */
VOID ExportFile(PMAIN_WND_INFO Info);

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


#endif /* __SERVMAN_PRECOMP_H */

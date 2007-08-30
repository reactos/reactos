#ifndef __SERVMAN_PRECOMP_H
#define __SERVMAN_PRECOMP_H

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h> /* GET_X/Y_LPARAM */
#include <stdio.h>
#include <tchar.h>
#include <commctrl.h>
#include <shlobj.h>
#include "resource.h"

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif

#define NO_ITEM_SELECTED -1
#define MAX_KEY_LENGTH 256

#define LVNAME 0
#define LVDESC 1
#define LVSTATUS 2
#define LVSTARTUP 3
#define LVLOGONAS 4

typedef struct _MAIN_WND_INFO
{
    HWND  hMainWnd;
    HWND  hListView;
    HWND  hStatus;
    HWND  hTool;
    HMENU hShortcutMenu;
    int   nCmdShow;

    ENUM_SERVICE_STATUS_PROCESS *pAllServices;
    ENUM_SERVICE_STATUS_PROCESS *pCurrentService;

    INT SelectedItem;/* selection number in the list view */
    BOOL bDlgOpen;
    BOOL bInMenuLoop;
    BOOL bIsUserAnAdmin;

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

VOID UpdateServiceCount(PMAIN_WND_INFO Info);
VOID ChangeListViewText(PMAIN_WND_INFO Info, ENUM_SERVICE_STATUS_PROCESS* pService, UINT Column);
BOOL InitMainWindowImpl(VOID);
VOID UninitMainWindowImpl(VOID);
HWND CreateMainWindow(LPCTSTR lpCaption, int nCmdShow);

/* listview.c */
VOID SetListViewStyle(HWND hListView, DWORD View);
VOID ListViewSelectionChanged(PMAIN_WND_INFO Info, LPNMLISTVIEW pnmv);
BOOL CreateListView(PMAIN_WND_INFO Info);

/* start */
BOOL DoStart(PMAIN_WND_INFO Info);

/* control */
BOOL DoStop(PMAIN_WND_INFO Info);
BOOL DoPause(PMAIN_WND_INFO Info);
BOOL DoResume(PMAIN_WND_INFO Info);

/* progress.c */
HWND CreateProgressDialog(HWND hParent, LPTSTR lpServiceName, UINT Event);
VOID IncrementProgressBar(HWND hProgDlg);
VOID CompleteProgressBar(HWND hProgDlg);

/* query.c */
ENUM_SERVICE_STATUS_PROCESS* GetSelectedService(PMAIN_WND_INFO Info);
LPQUERY_SERVICE_CONFIG GetServiceConfig(LPTSTR lpServiceName);
BOOL SetServiceConfig(LPQUERY_SERVICE_CONFIG pServiceConfig, LPTSTR lpServiceName, LPTSTR lpPassword);
LPTSTR GetServiceDescription(LPTSTR lpServiceName);
BOOL SetServiceDescription(LPTSTR lpServiceName, LPTSTR lpDescription);
LPTSTR GetExecutablePath(LPTSTR lpServiceName);
BOOL RefreshServiceList(PMAIN_WND_INFO Info);
BOOL UpdateServiceStatus(ENUM_SERVICE_STATUS_PROCESS* pService);
BOOL GetServiceList(PMAIN_WND_INFO Info, DWORD *NumServices);

/* reg */
BOOL SetDescription(LPTSTR, LPTSTR);

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

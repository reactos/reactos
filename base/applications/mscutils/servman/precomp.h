#ifndef __SERVMAN_PRECOMP_H
#define __SERVMAN_PRECOMP_H

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <wingdi.h>
#include <winsvc.h>
#include <wincon.h>

#include <shlobj.h>
#include <commdlg.h>
#include <strsafe.h>
#include <process.h>

#include "resource.h"

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif

#define NO_ITEM_SELECTED -1
#define MAX_KEY_LENGTH  256

#define LVNAME          0
#define LVDESC          1
#define LVSTATUS        2
#define LVSTARTUP       3
#define LVLOGONAS       4
#define LVMAX           5

#define IMAGE_UNKNOWN   0
#define IMAGE_SERVICE   1
#define IMAGE_DRIVER    2

#define ACTION_START    1
#define ACTION_STOP     2
#define ACTION_PAUSE    3
#define ACTION_RESUME   4
#define ACTION_RESTART  5

#define ORD_ASCENDING   1
#define ORD_DESCENDING  -1

typedef struct _MAIN_WND_INFO
{
    HWND  hMainWnd;
    HWND  hListView;
    HWND  hStatus;
    HWND  hTool;
    HWND  hHeader;
    HMENU hShortcutMenu;
    int   nCmdShow;

    ENUM_SERVICE_STATUS_PROCESS *pAllServices;
    ENUM_SERVICE_STATUS_PROCESS *pCurrentService;
    DWORD NumServices;

    INT SelectedItem;/* selection number in the list view */
    INT SortSelection;
    INT SortDirection;

    BOOL bDlgOpen;
    BOOL bInMenuLoop;
    BOOL bIsUserAnAdmin;

    PVOID pTag;

} MAIN_WND_INFO, *PMAIN_WND_INFO;


INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CreateDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DeleteDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProgressDialogProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam);


/* servman.c */
extern HINSTANCE hInstance;
extern HANDLE ProcessHeap;

/* mainwnd.c */
typedef struct _MENU_HINT
{
    WORD CmdId;
    UINT HintId;
} MENU_HINT, *PMENU_HINT;

VOID SetMenuAndButtonStates(PMAIN_WND_INFO Info);
VOID UpdateServiceCount(PMAIN_WND_INFO Info);
VOID ChangeListViewText(PMAIN_WND_INFO Info, ENUM_SERVICE_STATUS_PROCESS* pService, UINT Column);
BOOL InitMainWindowImpl(VOID);
VOID UninitMainWindowImpl(VOID);
HWND CreateMainWindow(LPCTSTR lpCaption, int nCmdShow);

/* listview.c */
VOID SetListViewStyle(HWND hListView, DWORD View);
VOID ListViewSelectionChanged(PMAIN_WND_INFO Info, LPNMLISTVIEW pnmv);
BOOL CreateListView(PMAIN_WND_INFO Info);

/* start / stop / control */
DWORD DoStartService(LPWSTR ServiceName, HANDLE hProgress, LPWSTR lpStartParams);
DWORD DoStopService(LPWSTR ServiceName, HANDLE hProgress);
DWORD DoControlService(LPWSTR ServiceName, HWND hProgress, DWORD Control);

/* progress.c */
#define DEFAULT_STEP 0
BOOL RunActionWithProgress(HWND hParent, LPWSTR ServiceName, LPWSTR DisplayName, UINT Action, PVOID Param);
VOID IncrementProgressBar(HANDLE hProgress, UINT NewPos);
VOID CompleteProgressBar(HANDLE hProgress);

/* query.c */
ENUM_SERVICE_STATUS_PROCESS* GetSelectedService(PMAIN_WND_INFO Info);
LPQUERY_SERVICE_CONFIG GetServiceConfig(LPWSTR lpServiceName);
BOOL SetServiceConfig(LPQUERY_SERVICE_CONFIG pServiceConfig, LPWSTR lpServiceName, LPWSTR lpPassword);
LPWSTR GetServiceDescription(LPWSTR lpServiceName);
BOOL SetServiceDescription(LPWSTR lpServiceName, LPWSTR lpDescription);
LPWSTR GetExecutablePath(LPWSTR lpServiceName);
VOID FreeServiceList(PMAIN_WND_INFO Info);
BOOL RefreshServiceList(PMAIN_WND_INFO Info);
BOOL UpdateServiceStatus(ENUM_SERVICE_STATUS_PROCESS* pService);
BOOL GetServiceList(PMAIN_WND_INFO Info);

/* propsheet.c */
typedef struct _SERVICEPROPSHEET
{
    PMAIN_WND_INFO Info;
    ENUM_SERVICE_STATUS_PROCESS *pService;

} SERVICEPROPSHEET, *PSERVICEPROPSHEET;

typedef struct _DEPENDDATA
{
    PSERVICEPROPSHEET pDlgInfo;
    HIMAGELIST hDependsImageList;
    HWND hDependsWnd;
    HWND hDependsTreeView1;
    HWND hDependsTreeView2;

} DEPENDDATA, *PDEPENDDATA;


HTREEITEM AddItemToTreeView(HWND hTreeView, HTREEITEM hRoot, LPWSTR lpDisplayName, LPWSTR lpServiceName, ULONG serviceType, BOOL bHasChildren);

/* stop_dependencies */
INT_PTR CALLBACK StopDependsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LPWSTR GetListOfServicesToStop(LPWSTR lpServiceName);
BOOL
CreateStopDependsDialog(HWND hParent,
LPWSTR ServiceName,
LPWSTR DisplayName,
LPWSTR ServiceList);

/* tv1_dependencies */
BOOL TV1_Initialize(PDEPENDDATA pDependData, LPWSTR lpServiceName);
VOID TV1_AddDependantsToTree(PDEPENDDATA pDependData, HTREEITEM hParent, LPWSTR lpServiceName);

/* tv2_dependencies */
BOOL TV2_Initialize(PDEPENDDATA pDependData, LPWSTR lpServiceName);
VOID TV2_AddDependantsToTree(PDEPENDDATA pDependData, HTREEITEM hParent, LPWSTR lpServiceName);
BOOL TV2_HasDependantServices(LPWSTR lpServiceName);
LPENUM_SERVICE_STATUS TV2_GetDependants(LPWSTR lpServiceName, LPDWORD lpdwCount);

VOID OpenPropSheet(PMAIN_WND_INFO Info);

/* propsheet window procs */
INT_PTR CALLBACK DependenciesPageProc(HWND hwndDlg,
                                      UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam);
INT_PTR CALLBACK GeneralPageProc(HWND hwndDlg,
                                 UINT uMsg,
                                 WPARAM wParam,
                                 LPARAM lParam);
INT_PTR CALLBACK LogonPageProc(HWND hwndDlg,
                               UINT uMsg,
                               WPARAM wParam,
                               LPARAM lParam);
INT_PTR CALLBACK RecoveryPageProc(HWND hwndDlg,
                                  UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam);

/* export.c */
VOID ExportFile(PMAIN_WND_INFO Info);

/* misc.c */
INT AllocAndLoadString(OUT LPWSTR *lpTarget,
                       IN HINSTANCE hInst,
                       IN UINT uID);
DWORD LoadAndFormatString(IN HINSTANCE hInstance,
                          IN UINT uID,
                          OUT LPWSTR *lpTarget,
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
INT GetTextFromEdit(OUT LPWSTR lpString,
                    IN HWND hDlg,
                    IN UINT Res);
VOID GetError(VOID);
VOID DisplayString(PWCHAR);
HIMAGELIST InitImageList(UINT StartResource,
                         UINT EndResource,
                         UINT Width,
                         UINT Height,
                         ULONG type);
VOID
ResourceMessageBox(
    HINSTANCE hInstance,
    HWND hwnd,
    UINT uType,
    UINT uCaptionId,
    UINT uMessageId);

#endif /* __SERVMAN_PRECOMP_H */

#ifndef __DEVMGMT_PRECOMP_H
#define __DEVMGMT_PRECOMP_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <wingdi.h>
#include <windowsx.h>
#include <tchar.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <dll/devmgr/devmgr.h>
#include <initguid.h>
#include <devguid.h>

#include "resource.h"

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif

#define MAX_DEV_LEN 256

typedef enum
{
    DevicesByType,
    DevicesByConnection,
    RessourcesByType,
    RessourcesByConnection
} DISPLAY_TYPE;

typedef struct _MAIN_WND_INFO
{
    HWND  hMainWnd;
    HWND  hTreeView;
    HWND  hStatus;
    HWND  hTool;
    HWND  hProgDlg;
    HMENU hShortcutMenu;
    int   nCmdShow;

    DISPLAY_TYPE Display;
    BOOL bShowHidden;

    /* status flags */
    UINT InMenuLoop : 1;

} MAIN_WND_INFO, *PMAIN_WND_INFO;


INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


/* devmgmt.c */
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

VOID FreeDeviceStrings(HWND hTreeView);
VOID OpenPropSheet(HWND hTreeView, HTREEITEM hItem);
HTREEITEM InitTreeView(HWND hTreeView);
VOID ListDevicesByType(HWND hTreeView, HTREEITEM hRoot, BOOL bShowHidden);
VOID ListDevicesByConnection(HWND hTreeView, HTREEITEM hRoot, BOOL bShowHidden);


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

HIMAGELIST InitImageList(UINT NumButtons,
                         UINT StartResource,
                         UINT Width,
                         UINT Height);

VOID GetError(VOID);
VOID DisplayString(LPTSTR);

#endif /* __DEVMGMT_PRECOMP_H */

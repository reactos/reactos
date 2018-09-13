// Need this so progman.c can include it in
// a meaningful way.

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <winuserp.h>

// Taskman prototyes

LONG APIENTRY TaskmanDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
WORD APIENTRY TMExecProgram(LPTSTR lpszPath, LPTSTR lpDir, LPTSTR lpTitle);

VOID GetPathInfo(PTSTR szPath,PTSTR *pszFileName,PTSTR *pszExt,WORD *pich,BOOL *pfUnc);
VOID GetFilenameFromPath(PTSTR szPath, PTSTR szFilename);
VOID GetDirectoryFromPath(PTSTR szFilePath, PTSTR szDir);
BOOL TestTokenForAdmin(HANDLE Token);

WINUSERAPI VOID SwitchToThisWindow(HWND, BOOL);
INT TMMessageBox(HWND hWnd,WORD idTitle,WORD idMessage,PTSTR psz,WORD wStyle);

// Taskman global variables

HWND ghwndTMDialog;
BOOL fTMActive;
INT dxTaskman;
INT dyTaskman;
INT dxScreen;
INT dyScreen;

#define MAXTASKNAMELEN      512
#define MAXMSGBOXLEN        513

#define PWRTASKMANDLG       10
#define WMPTASKMANDLG       11

#define IDD_TMTEXT          499
#define IDD_TASKLISTBOX     500
#define IDD_TERMINATE       501
#define IDD_CASCADE         502
#define IDD_TILE            503
#define IDD_ARRANGEICONS    504
#define IDD_RUN             505
#define IDD_TMPATH          506
#define IDD_CLTEXT          507
#define IDD_SWITCH          508

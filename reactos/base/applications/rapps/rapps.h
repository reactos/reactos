#ifndef _RAPPS_H
#define _RAPPS_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winnls.h>
#include <winuser.h>
#include <wincon.h>
#include <richedit.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include <strsafe.h>
#include <ndk/rtlfuncs.h>

#include <rappsmsg.h>

#include "resource.h"

#ifdef USE_CERT_PINNING
  #define CERT_ISSUER_INFO "BE\r\nGlobalSign nv-sa\r\nGlobalSign Domain Validation CA - SHA256 - G2"
  #define CERT_SUBJECT_INFO "Domain Control Validated\r\n*.reactos.org"
#endif

#define APPLICATION_DATABASE_URL L"https://svn.reactos.org/packages/rappmgr.cab"

#define SPLIT_WIDTH 4
#define MAX_STR_LEN 256

#define LISTVIEW_ICON_SIZE 24
#define TREEVIEW_ICON_SIZE 24

/* EnumType flags for EnumInstalledApplications */
#define ENUM_ALL_COMPONENTS    30
#define ENUM_APPLICATIONS      31
#define ENUM_UPDATES           32
/* EnumType flags for EnumAvailableApplications */
#define ENUM_ALL_AVAILABLE     0
#define ENUM_CAT_AUDIO         1
#define ENUM_CAT_VIDEO         2
#define ENUM_CAT_GRAPHICS      3
#define ENUM_CAT_GAMES         4
#define ENUM_CAT_INTERNET      5
#define ENUM_CAT_OFFICE        6
#define ENUM_CAT_DEVEL         7
#define ENUM_CAT_EDU           8
#define ENUM_CAT_ENGINEER      9
#define ENUM_CAT_FINANCE       10
#define ENUM_CAT_SCIENCE       11
#define ENUM_CAT_TOOLS         12
#define ENUM_CAT_DRIVERS       13
#define ENUM_CAT_LIBS          14
#define ENUM_CAT_OTHER         15

#define ENUM_INSTALLED_MIN ENUM_ALL_COMPONENTS
#define ENUM_INSTALLED_MAX ENUM_UPDATES
#define ENUM_AVAILABLE_MIN ENUM_ALL_AVAILABLE
#define ENUM_AVAILABLE_MAX ENUM_CAT_OTHER

#define IS_INSTALLED_ENUM(a) (a >= ENUM_INSTALLED_MIN && a <= ENUM_INSTALLED_MAX)
#define IS_AVAILABLE_ENUM(a) (a >= ENUM_AVAILABLE_MIN && a <= ENUM_AVAILABLE_MAX)

/* aboutdlg.c */
VOID ShowAboutDialog(VOID);

/* available.c */
typedef struct
{
    INT Category;
    WCHAR szName[MAX_PATH];
    WCHAR szRegName[MAX_PATH];
    WCHAR szVersion[MAX_PATH];
    WCHAR szLicense[MAX_PATH];
    WCHAR szDesc[MAX_PATH];
    WCHAR szSize[MAX_PATH];
    WCHAR szUrlSite[MAX_PATH];
    WCHAR szUrlDownload[MAX_PATH];
    WCHAR szCDPath[MAX_PATH];

    /* caching mechanism related entries */
    WCHAR cFileName[MAX_PATH];
    FILETIME ftCacheStamp;
    LIST_ENTRY List;

    /* optional integrity checks */
    BYTE MD5Checksum[16];

} APPLICATION_INFO, *PAPPLICATION_INFO;

typedef struct
{
    HKEY hRootKey;
    HKEY hSubKey;
    WCHAR szKeyName[MAX_PATH];

} INSTALLED_INFO, *PINSTALLED_INFO;

typedef struct
{
    BOOL bSaveWndPos;
    BOOL bUpdateAtStart;
    BOOL bLogEnabled;
    WCHAR szDownloadDir[MAX_PATH];
    BOOL bDelInstaller;
    /* Window Pos */
    BOOL Maximized;
    INT Left;
    INT Top;
    INT Width;
    INT Height;
    /* Proxy settings */
    INT Proxy;
    WCHAR szProxyServer[MAX_PATH];
    WCHAR szNoProxyFor[MAX_PATH];

} SETTINGS_INFO, *PSETTINGS_INFO;

/* available.c */
typedef BOOL (CALLBACK *AVAILENUMPROC)(PAPPLICATION_INFO Info);
BOOL EnumAvailableApplications(INT EnumType, AVAILENUMPROC lpEnumProc);
BOOL ShowAvailableAppInfo(INT Index);
BOOL UpdateAppsDB(VOID);
VOID FreeCachedAvailableEntries(VOID);

/* installdlg.c */
BOOL InstallApplication(INT Index);

/* installed.c */
typedef BOOL (CALLBACK *APPENUMPROC)(INT ItemIndex, LPWSTR lpName, PINSTALLED_INFO Info);
BOOL EnumInstalledApplications(INT EnumType, BOOL IsUserKey, APPENUMPROC lpEnumProc);
BOOL GetApplicationString(HKEY hKey, LPWSTR lpKeyName, LPWSTR lpString);
BOOL ShowInstalledAppInfo(INT Index);
BOOL UninstallApplication(INT Index, BOOL bModify);
BOOL IsInstalledApplication(LPWSTR lpRegName, BOOL IsUserKey);
VOID RemoveAppFromRegistry(INT Index);

/* winmain.c */
extern HWND hMainWnd;
extern HINSTANCE hInst;
extern INT SelectedEnumType;
extern SETTINGS_INFO SettingsInfo;
VOID SaveSettings(HWND hwnd);
VOID FillDefaultSettings(PSETTINGS_INFO pSettingsInfo);

/* listview.c */
extern HWND hListView;
extern BOOL bAscending;
BOOL CreateListView(HWND hwnd);
BOOL ListViewAddColumn(INT Index, LPWSTR lpText, INT Width, INT Format);
INT ListViewAddItem(INT ItemIndex, INT IconIndex, LPWSTR lpText, LPARAM lParam);
INT CALLBACK ListViewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
PVOID ListViewGetlParam(INT Index);

/* loaddlg.c */
BOOL DownloadApplication(INT Index);
VOID DownloadApplicationsDB(LPWSTR lpUrl);

/* misc.c */
INT GetSystemColorDepth(VOID);
int GetWindowWidth(HWND hwnd);
int GetWindowHeight(HWND hwnd);
int GetClientWindowWidth(HWND hwnd);
int GetClientWindowHeight(HWND hwnd);
VOID CopyTextToClipboard(LPCWSTR lpszText);
VOID SetWelcomeText(VOID);
VOID ShowPopupMenu(HWND hwnd, UINT MenuID, UINT DefaultItem);
BOOL StartProcess(LPWSTR lpPath, BOOL Wait);
BOOL GetStorageDirectory(PWCHAR lpDirectory, DWORD cch);
BOOL ExtractFilesFromCab(LPWSTR lpCabName, LPWSTR lpOutputPath);
VOID InitLogs(VOID);
VOID FreeLogs(VOID);
BOOL WriteLogMessage(WORD wType, DWORD dwEventID, LPWSTR lpMsg);

UINT ParserGetString(LPCWSTR lpKeyName, LPWSTR lpReturnedString, UINT nSize, LPCWSTR lpFileName);
UINT ParserGetInt(LPCWSTR lpKeyName, LPCWSTR lpFileName);

/* richedit.c */
extern HWND hRichEdit;
extern PWSTR pLink;
BOOL CreateRichEdit(HWND hwnd);
VOID RichEditOnLink(HWND hwnd, ENLINK *Link);
VOID InsertRichEditText(LPCWSTR lpszText, DWORD dwEffects);
VOID NewRichEditText(LPCWSTR lpszText, DWORD dwEffects);

/* settingsdlg.c */
VOID CreateSettingsDlg(HWND hwnd);

/* splitter.c */
extern HWND hVSplitter;
extern HWND hHSplitter;
BOOL CreateVSplitBar(HWND hwnd);
BOOL CreateHSplitBar(HWND hwnd);
int GetHSplitterPos(VOID);
VOID SetHSplitterPos(int Pos);

/* statusbar.c */
extern HWND hStatusBar;
BOOL CreateStatusBar(HWND hwnd);
VOID SetStatusBarText(LPCWSTR lpszText);

/* toolbar.c */
extern HWND hToolBar;
extern HWND hSearchBar;
BOOL CreateToolBar(HWND hwnd);
VOID ToolBarOnGetDispInfo(LPTOOLTIPTEXT lpttt);

/* treeview.c */
extern HWND hTreeView;
BOOL CreateTreeView(HWND hwnd);
HTREEITEM TreeViewAddItem(HTREEITEM hParent, LPWSTR lpText, INT Image, INT SelectedImage, LPARAM lParam);

#endif /* _RAPPS_H */

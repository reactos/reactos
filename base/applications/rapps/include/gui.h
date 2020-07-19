#pragma once

#include "rapps.h"
#include "rosui.h"
#include "crichedit.h"
#include "asyncinet.h"
#include "misc.h"
#include "appview.h"
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atltypes.h>
#include <atlwin.h>
#include <wininet.h>
#include <shellutils.h>
#include <rosctrls.h>
#include <gdiplus.h>
#include <math.h>

#define SEARCH_TIMER_ID 'SR'
#define TREEVIEW_ICON_SIZE 24


class CMainToolbar :
    public CUiWindow< CToolbar<> >
{
    const INT m_iToolbarHeight;
    DWORD m_dButtonsWidthMax;

    WCHAR szInstallBtn[MAX_STR_LEN];
    WCHAR szUninstallBtn[MAX_STR_LEN];
    WCHAR szModifyBtn[MAX_STR_LEN];
    WCHAR szSelectAll[MAX_STR_LEN];

    VOID AddImageToImageList(HIMAGELIST hImageList, UINT ImageIndex);

    HIMAGELIST InitImageList();

public:

    CMainToolbar();

    VOID OnGetDispInfo(LPTOOLTIPTEXT lpttt);

    HWND Create(HWND hwndParent);

    VOID HideButtonCaption();

    VOID ShowButtonCaption();

    DWORD GetMaxButtonsWidth() const;
};

class CSideTreeView :
    public CUiWindow<CTreeView>
{
    HIMAGELIST hImageTreeView;

public:
    CSideTreeView();

    HTREEITEM AddItem(HTREEITEM hParent, ATL::CStringW &Text, INT Image, INT SelectedImage, LPARAM lParam);

    HTREEITEM AddCategory(HTREEITEM hRootItem, UINT TextIndex, UINT IconIndex);

    HIMAGELIST SetImageList();

    VOID DestroyImageList();

    ~CSideTreeView();
};

class CSearchBar :
    public CWindow
{
public:
    const INT m_Width;
    const INT m_Height;

    CSearchBar();

    VOID SetText(LPCWSTR lpszText);

    HWND Create(HWND hwndParent);

};

class CMainWindow :
    public CWindowImpl<CMainWindow, CWindow, CFrameWinTraits>
{
    CUiPanel *m_ClientPanel = NULL;
    CUiSplitPanel *m_VSplitter = NULL;

    CMainToolbar *m_Toolbar = NULL;

    CSideTreeView *m_TreeView = NULL;
    CUiWindow<CStatusBar> *m_StatusBar = NULL;

    CApplicationView *m_ApplicationView = NULL;

    CUiWindow<CSearchBar> *m_SearchBar = NULL;
    CAvailableApps m_AvailableApps;
    CInstalledApps m_InstalledApps;

    BOOL bSearchEnabled;
    BOOL bUpdating = FALSE;

    ATL::CStringW szSearchPattern;
    INT SelectedEnumType;

public:
    CMainWindow();

    ~CMainWindow();
private:

    VOID InitCategoriesList();

    BOOL CreateStatusBar();

    BOOL CreateToolbar();

    BOOL CreateTreeView();

    BOOL CreateApplicationView();

    BOOL CreateVSplitter();

    BOOL CreateSearchBar();

    BOOL CreateLayout();

    VOID LayoutCleanup();

    BOOL InitControls();

    VOID OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam);

    BOOL RemoveSelectedAppFromRegistry();

    BOOL UninstallSelectedApp(BOOL bModify);

    BOOL ProcessWindowMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT &theResult, DWORD dwMapId);

    BOOL IsSelectedNodeInstalled();

    VOID ShowAboutDlg();

    VOID OnCommand(WPARAM wParam, LPARAM lParam);

    static BOOL SearchPatternMatch(LPCWSTR szHaystack, LPCWSTR szNeedle);

    BOOL CALLBACK EnumInstalledAppProc(CInstalledApplicationInfo *Info);

    BOOL CALLBACK EnumAvailableAppProc(CAvailableApplicationInfo *Info, BOOL bInitialCheckState);

    static BOOL CALLBACK s_EnumInstalledAppProc(CInstalledApplicationInfo *Info, PVOID param);

    static BOOL CALLBACK s_EnumAvailableAppProc(CAvailableApplicationInfo *Info, BOOL bInitialCheckState, PVOID param);

    static BOOL CALLBACK s_EnumSelectedAppForDownloadProc(CAvailableApplicationInfo *Info, BOOL bInitialCheckState, PVOID param);

    VOID UpdateStatusBarText();

    VOID UpdateApplicationsList(INT EnumType);

public:
    static ATL::CWndClassInfo &GetWndClassInfo();

    HWND Create();

    // this function is called when a item of application-view is checked/unchecked
    // CallbackParam is the param passed to application-view when adding the item (the one getting focus now).
    BOOL ItemCheckStateChanged(BOOL bChecked, LPVOID CallbackParam);

    void HandleTabOrder(int direction);
};


VOID ShowMainWindow(INT nShowCmd);

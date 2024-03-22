#pragma once

#include "rapps.h"
#include "rosui.h"
#include "crichedit.h"
#include "asyncinet.h"
#include "appview.h"
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atltypes.h>
#include <atlwin.h>
#include <wininet.h>
#include <shellutils.h>
#include <ui/rosctrls.h>
#include <gdiplus.h>
#include <math.h>

#define SEARCH_TIMER_ID 'SR'
#define TREEVIEW_ICON_SIZE 24

class CSideTreeView : public CUiWindow<CTreeView>
{
    HIMAGELIST hImageTreeView;

  public:
    CSideTreeView();

    HTREEITEM
    AddItem(HTREEITEM hParent, CStringW &Text, INT Image, INT SelectedImage, LPARAM lParam);

    HTREEITEM
    AddCategory(HTREEITEM hRootItem, UINT TextIndex, UINT IconIndex);

    HIMAGELIST
    SetImageList();

    VOID
    DestroyImageList();

    ~CSideTreeView();
};

class CMainWindow : public CWindowImpl<CMainWindow, CWindow, CFrameWinTraits>
{
    CUiPanel *m_ClientPanel = NULL;
    CUiSplitPanel *m_VSplitter = NULL;

    CSideTreeView *m_TreeView = NULL;
    CUiWindow<CStatusBar> *m_StatusBar = NULL;

    CApplicationView *m_ApplicationView = NULL;
    friend class CApplicationView;

    CAppDB *m_Db;
    CAtlList<CAppInfo *> m_Selected;

    BOOL bUpdating = FALSE;
    BOOL m_bAppwizMode;
    HTREEITEM hRootItemInstalled;

    CStringW szSearchPattern;
    AppsCategories SelectedEnumType;

  public:
    explicit CMainWindow(CAppDB *db, BOOL bAppwiz = FALSE);

    ~CMainWindow();

  private:
    VOID
    InitCategoriesList();

    BOOL
    CreateStatusBar();
    BOOL
    CreateTreeView();
    BOOL
    CreateApplicationView();
    BOOL
    CreateVSplitter();
    BOOL
    CreateLayout();
    VOID
    LayoutCleanup();
    BOOL
    InitControls();

    VOID
    OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam);

    VOID
    CheckAvailable();

    BOOL
    RemoveSelectedAppFromRegistry();
    BOOL
    UninstallSelectedApp(BOOL bModify);

    BOOL
    ProcessWindowMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT &theResult, DWORD dwMapId);
    VOID
    ShowAboutDlg();
    VOID
    OnCommand(WPARAM wParam, LPARAM lParam);
    VOID
    UpdateStatusBarText();

    VOID
    UpdateApplicationsList(AppsCategories EnumType, BOOL bReload = FALSE, BOOL bCheckAvailable = FALSE);
    VOID
    AddApplicationsToView(CAtlList<CAppInfo *> &List);

  public:
    static ATL::CWndClassInfo &
    GetWndClassInfo();

    HWND
    Create();

    // this function is called when a item of application-view is checked/unchecked
    // CallbackParam is the param passed to application-view when adding the item (the one getting focus now).
    VOID
    ItemCheckStateChanged(BOOL bChecked, LPVOID CallbackParam);

    // this function is called when application-view is asked to install an application
    // if Info is not zero, this app should be installed. otherwise those checked apps should be installed
    BOOL
    InstallApplication(CAppInfo *Info);

    // this function is called when search text is changed
    BOOL
    SearchTextChanged(CStringW &SearchText);

    void
    HandleTabOrder(int direction);
};

// Main window
VOID
MainWindowLoop(CMainWindow *wnd, INT nShowCmd);

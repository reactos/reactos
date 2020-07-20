/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:        base/applications/rapps/gui.cpp
 * PURPOSE:     GUI classes for RAPPS
 * COPYRIGHT:   Copyright 2015 David Quintana           (gigaherz@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov   (sanchaez@reactos.org)
 */

#include "rapps.h"
#include "rosui.h"
#include "crichedit.h"
#include "appview.h"
#include "asyncinet.h"
#include "misc.h"
#include "gui.h"
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


// **** CMainToolbar ****

VOID CMainToolbar::AddImageToImageList(HIMAGELIST hImageList, UINT ImageIndex)
{
    HICON hImage;

    if (!(hImage = (HICON)LoadImageW(hInst,
        MAKEINTRESOURCE(ImageIndex),
        IMAGE_ICON,
        m_iToolbarHeight,
        m_iToolbarHeight,
        0)))
    {
        /* TODO: Error message */
    }

    ImageList_AddIcon(hImageList, hImage);
    DeleteObject(hImage);
}

HIMAGELIST CMainToolbar::InitImageList()
{
    HIMAGELIST hImageList;

    /* Create the toolbar icon image list */
    hImageList = ImageList_Create(m_iToolbarHeight,//GetSystemMetrics(SM_CXSMICON),
        m_iToolbarHeight,//GetSystemMetrics(SM_CYSMICON),
        ILC_MASK | GetSystemColorDepth(),
        1, 1);
    if (!hImageList)
    {
        /* TODO: Error message */
        return NULL;
    }

    AddImageToImageList(hImageList, IDI_INSTALL);
    AddImageToImageList(hImageList, IDI_UNINSTALL);
    AddImageToImageList(hImageList, IDI_MODIFY);
    AddImageToImageList(hImageList, IDI_CHECK_ALL);
    AddImageToImageList(hImageList, IDI_REFRESH);
    AddImageToImageList(hImageList, IDI_UPDATE_DB);
    AddImageToImageList(hImageList, IDI_SETTINGS);
    AddImageToImageList(hImageList, IDI_EXIT);

    return hImageList;
}

CMainToolbar::CMainToolbar() : m_iToolbarHeight(24)
{
}

VOID CMainToolbar::OnGetDispInfo(LPTOOLTIPTEXT lpttt)
{
    UINT idButton = (UINT)lpttt->hdr.idFrom;

    switch (idButton)
    {
    case ID_EXIT:
        lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_EXIT);
        break;

    case ID_INSTALL:
        lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_INSTALL);
        break;

    case ID_UNINSTALL:
        lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_UNINSTALL);
        break;

    case ID_MODIFY:
        lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_MODIFY);
        break;

    case ID_SETTINGS:
        lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_SETTINGS);
        break;

    case ID_REFRESH:
        lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_REFRESH);
        break;

    case ID_RESETDB:
        lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_UPDATE_DB);
        break;
    }
}

HWND CMainToolbar::Create(HWND hwndParent)
{
    /* Create buttons */
    TBBUTTON Buttons[] =
    {   /* iBitmap, idCommand, fsState, fsStyle, bReserved[2], dwData, iString */
        {  0, ID_INSTALL,   TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)szInstallBtn      },
        {  1, ID_UNINSTALL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)szUninstallBtn    },
        {  2, ID_MODIFY,    TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)szModifyBtn       },
        {  3, ID_CHECK_ALL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)szSelectAll       },
        { -1, 0,            TBSTATE_ENABLED, BTNS_SEP,                    { 0 }, 0, 0                           },
        {  4, ID_REFRESH,   TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0                           },
        {  5, ID_RESETDB,   TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0                           },
        { -1, 0,            TBSTATE_ENABLED, BTNS_SEP,                    { 0 }, 0, 0                           },
        {  6, ID_SETTINGS,  TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0                           },
        {  7, ID_EXIT,      TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0                           },
    };

    LoadStringW(hInst, IDS_INSTALL, szInstallBtn, _countof(szInstallBtn));
    LoadStringW(hInst, IDS_UNINSTALL, szUninstallBtn, _countof(szUninstallBtn));
    LoadStringW(hInst, IDS_MODIFY, szModifyBtn, _countof(szModifyBtn));
    LoadStringW(hInst, IDS_SELECT_ALL, szSelectAll, _countof(szSelectAll));

    m_hWnd = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST,
        0, 0, 0, 0,
        hwndParent,
        0, hInst, NULL);

    if (!m_hWnd)
    {
        /* TODO: Show error message */
        return FALSE;
    }

    SendMessageW(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_HIDECLIPPEDBUTTONS);
    SetButtonStructSize();

    /* Set image list */
    HIMAGELIST hImageList = InitImageList();

    if (!hImageList)
    {
        /* TODO: Show error message */
        return FALSE;
    }

    ImageList_Destroy(SetImageList(hImageList));

    AddButtons(_countof(Buttons), Buttons);

    /* Remember ideal width to use as a max width of buttons */
    SIZE size;
    GetIdealSize(FALSE, &size);
    m_dButtonsWidthMax = size.cx;

    return m_hWnd;
}

VOID CMainToolbar::HideButtonCaption()
{
    DWORD dCurrentExStyle = (DWORD)SendMessageW(TB_GETEXTENDEDSTYLE, 0, 0);
    SendMessageW(TB_SETEXTENDEDSTYLE, 0, dCurrentExStyle | TBSTYLE_EX_MIXEDBUTTONS);
}

VOID CMainToolbar::ShowButtonCaption()
{
    DWORD dCurrentExStyle = (DWORD)SendMessageW(TB_GETEXTENDEDSTYLE, 0, 0);
    SendMessageW(TB_SETEXTENDEDSTYLE, 0, dCurrentExStyle & ~TBSTYLE_EX_MIXEDBUTTONS);
}

DWORD CMainToolbar::GetMaxButtonsWidth() const
{
    return m_dButtonsWidthMax;
}
// **** CMainToolbar ****


// **** CSideTreeView ****

CSideTreeView::CSideTreeView() :
    CUiWindow(),
    hImageTreeView(ImageList_Create(TREEVIEW_ICON_SIZE, TREEVIEW_ICON_SIZE,
        GetSystemColorDepth() | ILC_MASK,
        0, 1))
{
}

HTREEITEM CSideTreeView::AddItem(HTREEITEM hParent, ATL::CStringW &Text, INT Image, INT SelectedImage, LPARAM lParam)
{
    return CUiWindow<CTreeView>::AddItem(hParent, const_cast<LPWSTR>(Text.GetString()), Image, SelectedImage, lParam);
}

HTREEITEM CSideTreeView::AddCategory(HTREEITEM hRootItem, UINT TextIndex, UINT IconIndex)
{
    ATL::CStringW szText;
    INT Index;
    HICON hIcon;

    hIcon = (HICON)LoadImageW(hInst,
        MAKEINTRESOURCE(IconIndex),
        IMAGE_ICON,
        TREEVIEW_ICON_SIZE,
        TREEVIEW_ICON_SIZE,
        LR_CREATEDIBSECTION);
    if (hIcon)
    {
        Index = ImageList_AddIcon(hImageTreeView, hIcon);
        DestroyIcon(hIcon);
    }

    szText.LoadStringW(TextIndex);
    return AddItem(hRootItem, szText, Index, Index, TextIndex);
}

HIMAGELIST CSideTreeView::SetImageList()
{
    return CUiWindow<CTreeView>::SetImageList(hImageTreeView, TVSIL_NORMAL);
}

VOID CSideTreeView::DestroyImageList()
{
    if (hImageTreeView)
        ImageList_Destroy(hImageTreeView);
}

CSideTreeView::~CSideTreeView()
{
    DestroyImageList();
}
// **** CSideTreeView ****


// **** CSearchBar ****

CSearchBar::CSearchBar() : m_Width(200), m_Height(22)
{
}

VOID CSearchBar::SetText(LPCWSTR lpszText)
{
    SendMessageW(SB_SETTEXT, SBT_NOBORDERS, (LPARAM)lpszText);
}

HWND CSearchBar::Create(HWND hwndParent)
{
    ATL::CStringW szBuf;
    m_hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", NULL,
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
        0, 0, m_Width, m_Height,
        hwndParent, (HMENU)NULL,
        hInst, 0);

    SendMessageW(WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
    szBuf.LoadStringW(IDS_SEARCH_TEXT);
    SetWindowTextW(szBuf);
    return m_hWnd;
}
// **** CSearchBar ****


// **** CMainWindow ****

CMainWindow::CMainWindow() :
    m_ClientPanel(NULL),
    bSearchEnabled(FALSE),
    SelectedEnumType(ENUM_ALL_INSTALLED)
{
}

CMainWindow::~CMainWindow()
{
    LayoutCleanup();
}

VOID CMainWindow::InitCategoriesList()
{
    HTREEITEM hRootItemInstalled, hRootItemAvailable;

    hRootItemInstalled = m_TreeView->AddCategory(TVI_ROOT, IDS_INSTALLED, IDI_CATEGORY);
    m_TreeView->AddCategory(hRootItemInstalled, IDS_APPLICATIONS, IDI_APPS);
    m_TreeView->AddCategory(hRootItemInstalled, IDS_UPDATES, IDI_APPUPD);

    m_TreeView->AddCategory(TVI_ROOT, IDS_SELECTEDFORINST, IDI_SELECTEDFORINST);

    hRootItemAvailable = m_TreeView->AddCategory(TVI_ROOT, IDS_AVAILABLEFORINST, IDI_CATEGORY);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_AUDIO, IDI_CAT_AUDIO);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_VIDEO, IDI_CAT_VIDEO);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_GRAPHICS, IDI_CAT_GRAPHICS);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_GAMES, IDI_CAT_GAMES);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_INTERNET, IDI_CAT_INTERNET);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_OFFICE, IDI_CAT_OFFICE);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_DEVEL, IDI_CAT_DEVEL);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_EDU, IDI_CAT_EDU);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_ENGINEER, IDI_CAT_ENGINEER);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_FINANCE, IDI_CAT_FINANCE);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_SCIENCE, IDI_CAT_SCIENCE);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_TOOLS, IDI_CAT_TOOLS);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_DRIVERS, IDI_CAT_DRIVERS);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_LIBS, IDI_CAT_LIBS);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_THEMES, IDI_CAT_THEMES);
    m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_OTHER, IDI_CAT_OTHER);

    m_TreeView->SetImageList();
    m_TreeView->Expand(hRootItemInstalled, TVE_EXPAND);
    m_TreeView->Expand(hRootItemAvailable, TVE_EXPAND);
    m_TreeView->SelectItem(hRootItemAvailable);
}

BOOL CMainWindow::CreateStatusBar()
{
    m_StatusBar = new CUiWindow<CStatusBar>();
    m_StatusBar->m_VerticalAlignment = UiAlign_RightBtm;
    m_StatusBar->m_HorizontalAlignment = UiAlign_Stretch;
    m_ClientPanel->Children().Append(m_StatusBar);

    return m_StatusBar->Create(m_hWnd, (HMENU)IDC_STATUSBAR) != NULL;
}

BOOL CMainWindow::CreateToolbar()
{
    m_Toolbar = new CMainToolbar();
    m_Toolbar->m_VerticalAlignment = UiAlign_LeftTop;
    m_Toolbar->m_HorizontalAlignment = UiAlign_Stretch;
    m_ClientPanel->Children().Append(m_Toolbar);

    return m_Toolbar->Create(m_hWnd) != NULL;
}

BOOL CMainWindow::CreateTreeView()
{
    m_TreeView = new CSideTreeView();
    m_TreeView->m_VerticalAlignment = UiAlign_Stretch;
    m_TreeView->m_HorizontalAlignment = UiAlign_Stretch;
    m_VSplitter->First().Append(m_TreeView);

    return m_TreeView->Create(m_hWnd) != NULL;
}

BOOL CMainWindow::CreateApplicationView()
{
    m_ApplicationView = new CApplicationView(this); // pass this to ApplicationView for callback purpose
    m_ApplicationView->m_VerticalAlignment = UiAlign_Stretch;
    m_ApplicationView->m_HorizontalAlignment = UiAlign_Stretch;
    m_VSplitter->Second().Append(m_ApplicationView);

    return m_ApplicationView->Create(m_hWnd) != NULL;
}

BOOL CMainWindow::CreateVSplitter()
{
    m_VSplitter = new CUiSplitPanel();
    m_VSplitter->m_VerticalAlignment = UiAlign_Stretch;
    m_VSplitter->m_HorizontalAlignment = UiAlign_Stretch;
    m_VSplitter->m_DynamicFirst = FALSE;
    m_VSplitter->m_Horizontal = FALSE;
    m_VSplitter->m_MinFirst = 0;
    m_VSplitter->m_MinSecond = 320;
    m_VSplitter->m_Pos = 240;
    m_ClientPanel->Children().Append(m_VSplitter);

    return m_VSplitter->Create(m_hWnd) != NULL;
}

BOOL CMainWindow::CreateSearchBar()
{
    m_SearchBar = new CUiWindow<CSearchBar>();
    m_SearchBar->m_VerticalAlignment = UiAlign_LeftTop;
    m_SearchBar->m_HorizontalAlignment = UiAlign_RightBtm;
    m_SearchBar->m_Margin.top = 4;
    m_SearchBar->m_Margin.right = 6;

    return m_SearchBar->Create(m_Toolbar->m_hWnd) != NULL;
}

BOOL CMainWindow::CreateLayout()
{
    BOOL b = TRUE;
    bUpdating = TRUE;

    m_ClientPanel = new CUiPanel();
    m_ClientPanel->m_VerticalAlignment = UiAlign_Stretch;
    m_ClientPanel->m_HorizontalAlignment = UiAlign_Stretch;

    // Top level
    b = b && CreateStatusBar();
    b = b && CreateToolbar();
    b = b && CreateSearchBar();
    b = b && CreateVSplitter();

    // Inside V Splitter
    b = b && CreateTreeView();
    b = b && CreateApplicationView();

    if (b)
    {
        RECT rTop;
        RECT rBottom;

        /* Size status bar */
        m_StatusBar->SendMessageW(WM_SIZE, 0, 0);

        /* Size tool bar */
        m_Toolbar->AutoSize();

        ::GetWindowRect(m_Toolbar->m_hWnd, &rTop);
        ::GetWindowRect(m_StatusBar->m_hWnd, &rBottom);

        m_VSplitter->m_Margin.top = rTop.bottom - rTop.top;
        m_VSplitter->m_Margin.bottom = rBottom.bottom - rBottom.top;
    }

    bUpdating = FALSE;
    return b;
}

VOID CMainWindow::LayoutCleanup()
{
    delete m_TreeView;
    delete m_ApplicationView;
    delete m_VSplitter;
    delete m_SearchBar;
    delete m_Toolbar;
    delete m_StatusBar;
    return;
}

BOOL CMainWindow::InitControls()
{
    if (CreateLayout())
    {
        InitCategoriesList();

        UpdateStatusBarText();

        return TRUE;
    }

    return FALSE;
}

VOID CMainWindow::OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (wParam == SIZE_MINIMIZED)
        return;

    /* Size status bar */
    m_StatusBar->SendMessage(WM_SIZE, 0, 0);

    /* Size tool bar */
    m_Toolbar->AutoSize();

    /* Automatically hide captions */
    DWORD dToolbarTreshold = m_Toolbar->GetMaxButtonsWidth();
    DWORD dSearchbarMargin = (LOWORD(lParam) - m_SearchBar->m_Width);

    if (dSearchbarMargin > dToolbarTreshold)
    {
        m_Toolbar->ShowButtonCaption();
    }
    else if (dSearchbarMargin < dToolbarTreshold)
    {
        m_Toolbar->HideButtonCaption();
    }

    RECT r = { 0, 0, LOWORD(lParam), HIWORD(lParam) };
    HDWP hdwp = NULL;
    INT count = m_ClientPanel->CountSizableChildren();

    hdwp = BeginDeferWindowPos(count);
    if (hdwp)
    {
        hdwp = m_ClientPanel->OnParentSize(r, hdwp);
        if (hdwp)
        {
            EndDeferWindowPos(hdwp);
        }
    }

    // TODO: Sub-layouts for children of children
    count = m_SearchBar->CountSizableChildren();
    hdwp = BeginDeferWindowPos(count);
    if (hdwp)
    {
        hdwp = m_SearchBar->OnParentSize(r, hdwp);
        if (hdwp)
        {
            EndDeferWindowPos(hdwp);
        }
    }
}

BOOL CMainWindow::RemoveSelectedAppFromRegistry()
{
    if (!IsInstalledEnum(SelectedEnumType))
        return FALSE;

    ATL::CStringW szMsgText, szMsgTitle;

    if (!szMsgText.LoadStringW(IDS_APP_REG_REMOVE) ||
        !szMsgTitle.LoadStringW(IDS_INFORMATION))
        return FALSE;

    if (MessageBoxW(szMsgText, szMsgTitle, MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        CInstalledApplicationInfo *InstalledApp = (CInstalledApplicationInfo *)m_ApplicationView->GetFocusedItemData();
        LSTATUS Result = InstalledApp->RemoveFromRegistry();
        if (Result != ERROR_SUCCESS)
        {
            // TODO: popup a messagebox telling user it fails somehow
            return FALSE;
        }

        // as it's already removed form registry, this will also remove it from the list
        UpdateApplicationsList(-1);
        return TRUE;
    }

    return FALSE;
}

BOOL CMainWindow::UninstallSelectedApp(BOOL bModify)
{
    if (!IsInstalledEnum(SelectedEnumType))
        return FALSE;

    CInstalledApplicationInfo *InstalledApp = (CInstalledApplicationInfo *)m_ApplicationView->GetFocusedItemData();

    return InstalledApp->UninstallApplication(bModify);
}

BOOL CMainWindow::ProcessWindowMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT &theResult, DWORD dwMapId)
{
    theResult = 0;
    switch (Msg)
    {
    case WM_CREATE:
        if (!InitControls())
            ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
        break;

    case WM_DESTROY:
    {
        ShowWindow(SW_HIDE);
        SaveSettings(hwnd);

        FreeLogs();
        m_AvailableApps.FreeCachedEntries();
        m_InstalledApps.FreeCachedEntries();

        delete m_ClientPanel;

        PostQuitMessage(0);
        return 0;
    }

    case WM_COMMAND:
        OnCommand(wParam, lParam);
        break;

    case WM_NOTIFY:
    {
        LPNMHDR data = (LPNMHDR)lParam;

        switch (data->code)
        {
        case TVN_SELCHANGED:
        {
            if (data->hwndFrom == m_TreeView->m_hWnd)
            {
                switch (((LPNMTREEVIEW)lParam)->itemNew.lParam)
                {
                case IDS_INSTALLED:
                    UpdateApplicationsList(ENUM_ALL_INSTALLED);
                    break;

                case IDS_APPLICATIONS:
                    UpdateApplicationsList(ENUM_INSTALLED_APPLICATIONS);
                    break;

                case IDS_UPDATES:
                    UpdateApplicationsList(ENUM_UPDATES);
                    break;

                case IDS_AVAILABLEFORINST:
                    UpdateApplicationsList(ENUM_ALL_AVAILABLE);
                    break;

                case IDS_CAT_AUDIO:
                    UpdateApplicationsList(ENUM_CAT_AUDIO);
                    break;

                case IDS_CAT_DEVEL:
                    UpdateApplicationsList(ENUM_CAT_DEVEL);
                    break;

                case IDS_CAT_DRIVERS:
                    UpdateApplicationsList(ENUM_CAT_DRIVERS);
                    break;

                case IDS_CAT_EDU:
                    UpdateApplicationsList(ENUM_CAT_EDU);
                    break;

                case IDS_CAT_ENGINEER:
                    UpdateApplicationsList(ENUM_CAT_ENGINEER);
                    break;

                case IDS_CAT_FINANCE:
                    UpdateApplicationsList(ENUM_CAT_FINANCE);
                    break;

                case IDS_CAT_GAMES:
                    UpdateApplicationsList(ENUM_CAT_GAMES);
                    break;

                case IDS_CAT_GRAPHICS:
                    UpdateApplicationsList(ENUM_CAT_GRAPHICS);
                    break;

                case IDS_CAT_INTERNET:
                    UpdateApplicationsList(ENUM_CAT_INTERNET);
                    break;

                case IDS_CAT_LIBS:
                    UpdateApplicationsList(ENUM_CAT_LIBS);
                    break;

                case IDS_CAT_OFFICE:
                    UpdateApplicationsList(ENUM_CAT_OFFICE);
                    break;

                case IDS_CAT_OTHER:
                    UpdateApplicationsList(ENUM_CAT_OTHER);
                    break;

                case IDS_CAT_SCIENCE:
                    UpdateApplicationsList(ENUM_CAT_SCIENCE);
                    break;

                case IDS_CAT_TOOLS:
                    UpdateApplicationsList(ENUM_CAT_TOOLS);
                    break;

                case IDS_CAT_VIDEO:
                    UpdateApplicationsList(ENUM_CAT_VIDEO);
                    break;

                case IDS_CAT_THEMES:
                    UpdateApplicationsList(ENUM_CAT_THEMES);
                    break;

                case IDS_SELECTEDFORINST:
                    UpdateApplicationsList(ENUM_CAT_SELECTED);
                    break;
                }
            }

            HMENU mainMenu = ::GetMenu(hwnd);

            /* Disable/enable items based on treeview selection */
            if (IsSelectedNodeInstalled())
            {
                EnableMenuItem(mainMenu, ID_REGREMOVE, MF_ENABLED);
                EnableMenuItem(mainMenu, ID_INSTALL, MF_GRAYED);
                EnableMenuItem(mainMenu, ID_UNINSTALL, MF_ENABLED);
                EnableMenuItem(mainMenu, ID_MODIFY, MF_ENABLED);

                m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_REGREMOVE, TRUE);
                m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_INSTALL, FALSE);
                m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_UNINSTALL, TRUE);
                m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_MODIFY, TRUE);
            }
            else
            {
                EnableMenuItem(mainMenu, ID_REGREMOVE, MF_GRAYED);
                EnableMenuItem(mainMenu, ID_INSTALL, MF_ENABLED);
                EnableMenuItem(mainMenu, ID_UNINSTALL, MF_GRAYED);
                EnableMenuItem(mainMenu, ID_MODIFY, MF_GRAYED);

                m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_REGREMOVE, FALSE);
                m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_INSTALL, TRUE);
                m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_UNINSTALL, FALSE);
                m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_MODIFY, FALSE);
            }
        }
        break;

        case TTN_GETDISPINFO:
            m_Toolbar->OnGetDispInfo((LPTOOLTIPTEXT)lParam);
            break;
        }
    }
    break;

    case WM_SIZE:
        OnSize(hwnd, wParam, lParam);
        break;

    case WM_SIZING:
    {
        LPRECT pRect = (LPRECT)lParam;

        if (pRect->right - pRect->left < 565)
            pRect->right = pRect->left + 565;

        if (pRect->bottom - pRect->top < 300)
            pRect->bottom = pRect->top + 300;

        return TRUE;
    }

    case WM_SYSCOLORCHANGE:
    {
        /* Forward WM_SYSCOLORCHANGE to common controls */
        m_ApplicationView->SendMessageW(WM_SYSCOLORCHANGE, wParam, lParam);
        m_TreeView->SendMessageW(WM_SYSCOLORCHANGE, wParam, lParam);
        m_Toolbar->SendMessageW(WM_SYSCOLORCHANGE, 0, 0);
    }
    break;

    case WM_TIMER:
        if (wParam == SEARCH_TIMER_ID)
        {
            ::KillTimer(hwnd, SEARCH_TIMER_ID);
            if (bSearchEnabled)
                UpdateApplicationsList(-1);
        }
        break;
    }

    return FALSE;
}

BOOL CMainWindow::IsSelectedNodeInstalled()
{
    HTREEITEM hSelectedItem = m_TreeView->GetSelection();
    TV_ITEM tItem;

    tItem.mask = TVIF_PARAM | TVIF_HANDLE;
    tItem.hItem = hSelectedItem;
    m_TreeView->GetItem(&tItem);
    switch (tItem.lParam)
    {
    case IDS_INSTALLED:
    case IDS_APPLICATIONS:
    case IDS_UPDATES:
        return TRUE;
    default:
        return FALSE;
    }
}

VOID CMainWindow::ShowAboutDlg()
{
    ATL::CStringW szApp;
    ATL::CStringW szAuthors;
    HICON hIcon;

    szApp.LoadStringW(IDS_APPTITLE);
    szAuthors.LoadStringW(IDS_APP_AUTHORS);
    hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN));
    ShellAboutW(m_hWnd, szApp, szAuthors, hIcon);
    DestroyIcon(hIcon);
}

VOID CMainWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
    WORD wCommand = LOWORD(wParam);

    if (lParam == (LPARAM)m_SearchBar->m_hWnd)
    {
        ATL::CStringW szBuf;

        switch (HIWORD(wParam))
        {
        case EN_SETFOCUS:
        {
            ATL::CStringW szWndText;

            szBuf.LoadStringW(IDS_SEARCH_TEXT);
            m_SearchBar->GetWindowTextW(szWndText);
            if (szBuf == szWndText)
            {
                bSearchEnabled = FALSE;
                m_SearchBar->SetWindowTextW(L"");
            }
        }
        break;

        case EN_KILLFOCUS:
        {
            m_SearchBar->GetWindowTextW(szBuf);
            if (szBuf.IsEmpty())
            {
                szBuf.LoadStringW(IDS_SEARCH_TEXT);
                bSearchEnabled = FALSE;
                m_SearchBar->SetWindowTextW(szBuf.GetString());
            }
        }
        break;

        case EN_CHANGE:
        {
            ATL::CStringW szWndText;

            if (!bSearchEnabled)
            {
                bSearchEnabled = TRUE;
                break;
            }

            szBuf.LoadStringW(IDS_SEARCH_TEXT);
            m_SearchBar->GetWindowTextW(szWndText);
            if (szBuf == szWndText)
            {
                szSearchPattern.Empty();
            }
            else
            {
                szSearchPattern = szWndText;
            }

            DWORD dwDelay;
            SystemParametersInfoW(SPI_GETMENUSHOWDELAY, 0, &dwDelay, 0);
            SetTimer(SEARCH_TIMER_ID, dwDelay);
        }
        break;
        }

        return;
    }

    switch (wCommand)
    {
    case ID_SETTINGS:
        CreateSettingsDlg(m_hWnd);
        break;

    case ID_EXIT:
        PostMessageW(WM_CLOSE, 0, 0);
        break;

    case ID_SEARCH:
        m_SearchBar->SetFocus();
        break;

    case ID_INSTALL:
        if (IsAvailableEnum(SelectedEnumType))
        {
            ATL::CSimpleArray<CAvailableApplicationInfo> AppsList;

            // enum all selected apps
            m_AvailableApps.Enum(ENUM_CAT_SELECTED, s_EnumSelectedAppForDownloadProc, (PVOID)&AppsList);

            if (AppsList.GetSize())
            {
                if (DownloadListOfApplications(AppsList, FALSE))
                {
                    m_AvailableApps.RemoveAllSelected();
                    UpdateApplicationsList(-1);
                }
            }
            else
            {
                // use the currently focused item in application-view
                CAvailableApplicationInfo *FocusedApps = (CAvailableApplicationInfo *)m_ApplicationView->GetFocusedItemData();
                if (FocusedApps)
                {
                    if (DownloadApplication(FocusedApps, FALSE))
                    {
                        UpdateApplicationsList(-1);
                    }
                }
                else
                {
                    // TODO: in this case, Install button in toolbar (and all other places) should be disabled
                    // or at least popup a messagebox telling user to select/check some app first
                }
            }
        }
        break;

    case ID_UNINSTALL:
        if (UninstallSelectedApp(FALSE))
            UpdateApplicationsList(-1);
        break;

    case ID_MODIFY:
        if (UninstallSelectedApp(TRUE))
            UpdateApplicationsList(-1);
        break;

    case ID_REGREMOVE:
        RemoveSelectedAppFromRegistry();
        break;

    case ID_REFRESH:
        UpdateApplicationsList(-1);
        break;

    case ID_RESETDB:
        CAvailableApps::ForceUpdateAppsDB();
        UpdateApplicationsList(-1);
        break;

    case ID_HELP:
        MessageBoxW(L"Help not implemented yet", NULL, MB_OK);
        break;

    case ID_ABOUT:
        ShowAboutDlg();
        break;

    case ID_CHECK_ALL:
        m_ApplicationView->CheckAll();
        break;
    }
}

BOOL CMainWindow::SearchPatternMatch(LPCWSTR szHaystack, LPCWSTR szNeedle)
{
    if (!*szNeedle)
        return TRUE;
    /* TODO: Improve pattern search beyond a simple case-insensitive substring search. */
    return StrStrIW(szHaystack, szNeedle) != NULL;
}

BOOL CALLBACK CMainWindow::EnumInstalledAppProc(CInstalledApplicationInfo *Info)
{
    if (!SearchPatternMatch(Info->szDisplayName.GetString(), szSearchPattern))
    {
        return TRUE;
    }
    return m_ApplicationView->AddInstalledApplication(Info, Info); // currently, the callback param is Info itself
}

BOOL CALLBACK CMainWindow::EnumAvailableAppProc(CAvailableApplicationInfo *Info, BOOL bInitialCheckState)
{
    if (!SearchPatternMatch(Info->m_szName.GetString(), szSearchPattern) &&
        !SearchPatternMatch(Info->m_szDesc.GetString(), szSearchPattern))
    {
        return TRUE;
    }
    return m_ApplicationView->AddAvailableApplication(Info, bInitialCheckState, Info); // currently, the callback param is Info itself
}

BOOL CALLBACK CMainWindow::s_EnumInstalledAppProc(CInstalledApplicationInfo *Info, PVOID param)
{
    CMainWindow *pThis = (CMainWindow *)param;
    return pThis->EnumInstalledAppProc(Info);
}

BOOL CALLBACK CMainWindow::s_EnumAvailableAppProc(CAvailableApplicationInfo *Info, BOOL bInitialCheckState, PVOID param)
{
    CMainWindow *pThis = (CMainWindow *)param;
    return pThis->EnumAvailableAppProc(Info, bInitialCheckState);
}

BOOL CALLBACK CMainWindow::s_EnumSelectedAppForDownloadProc(CAvailableApplicationInfo *Info, BOOL bInitialCheckState, PVOID param)
{
    ATL::CSimpleArray<CAvailableApplicationInfo> *pAppList = (ATL::CSimpleArray<CAvailableApplicationInfo> *)param;
    pAppList->Add(*Info);
    return TRUE;
}

VOID CMainWindow::UpdateStatusBarText()
{
    if (m_StatusBar)
    {
        ATL::CStringW szBuffer;

        szBuffer.Format(IDS_APPS_COUNT, m_ApplicationView->GetItemCount(), m_AvailableApps.GetSelectedCount());
        m_StatusBar->SetText(szBuffer);
    }
}

VOID CMainWindow::UpdateApplicationsList(INT EnumType)
{
    bUpdating = TRUE;

    if (EnumType == -1)
    {
        // keep the old enum type
        EnumType = SelectedEnumType;
    }
    else
    {
        SelectedEnumType = EnumType;
    }

    m_ApplicationView->SetRedraw(FALSE);
    if (IsInstalledEnum(EnumType))
    {
        // set the display mode of application-view. this will remove all the item in application-view too.
        m_ApplicationView->SetDisplayMode(ApplicationViewInstalledApps);

        // enum installed softwares 
        m_InstalledApps.Enum(EnumType, s_EnumInstalledAppProc, this);
    }
    else if (IsAvailableEnum(EnumType))
    {
        // set the display mode of application-view. this will remove all the item in application-view too.
        m_ApplicationView->SetDisplayMode(ApplicationViewAvailableApps);

        // enum available softwares 
        m_AvailableApps.Enum(EnumType, s_EnumAvailableAppProc, this);
    }
    m_ApplicationView->SetRedraw(TRUE);
    m_ApplicationView->RedrawWindow(0, 0, RDW_INVALIDATE | RDW_ALLCHILDREN); // force the child window to repaint
    UpdateStatusBarText();
    bUpdating = FALSE;
}

ATL::CWndClassInfo &CMainWindow::GetWndClassInfo()
{
    DWORD csStyle = CS_VREDRAW | CS_HREDRAW;
    static ATL::CWndClassInfo wc =
    {
        {
            sizeof(WNDCLASSEX),
            csStyle,
            StartWindowProc,
            0,
            0,
            NULL,
            LoadIconW(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCEW(IDI_MAIN)),
            LoadCursorW(NULL, IDC_ARROW),
            (HBRUSH)(COLOR_BTNFACE + 1),
            MAKEINTRESOURCEW(IDR_MAINMENU),
            L"RAppsWnd",
            NULL
        },
        NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
    };
    return wc;
}

HWND CMainWindow::Create()
{
    ATL::CStringW szWindowName;
    szWindowName.LoadStringW(IDS_APPTITLE);

    RECT r = {
        (SettingsInfo.bSaveWndPos ? SettingsInfo.Left : CW_USEDEFAULT),
        (SettingsInfo.bSaveWndPos ? SettingsInfo.Top : CW_USEDEFAULT),
        (SettingsInfo.bSaveWndPos ? SettingsInfo.Width : 680),
        (SettingsInfo.bSaveWndPos ? SettingsInfo.Height : 450)
    };
    r.right += r.left;
    r.bottom += r.top;

    return CWindowImpl::Create(NULL, r, szWindowName.GetString(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
}

// this function is called when a item of application-view is checked/unchecked
// CallbackParam is the param passed to application-view when adding the item (the one getting focus now).
BOOL CMainWindow::ItemCheckStateChanged(BOOL bChecked, LPVOID CallbackParam)
{
    if (!bUpdating)
    {
        if (bChecked)
        {
            if (!m_AvailableApps.AddSelected((CAvailableApplicationInfo *)CallbackParam))
            {
                return FALSE;
            }
        }
        else
        {
            if (!m_AvailableApps.RemoveSelected((CAvailableApplicationInfo *)CallbackParam))
            {
                return FALSE;
            }
        }

        UpdateStatusBarText();
        return TRUE;
    }
    else
    {
        return TRUE;
    }
}

void CMainWindow::HandleTabOrder(int direction)
{
    HWND Controls[] = { m_Toolbar->m_hWnd, m_SearchBar->m_hWnd, m_TreeView->m_hWnd, m_ApplicationView->m_hWnd };
    // When there is no control found, go to the first or last (depending on tab vs shift-tab)
    int current = direction > 0 ? 0 : (_countof(Controls) - 1);
    HWND hActive = ::GetFocus();
    for (size_t n = 0; n < _countof(Controls); ++n)
    {
        if (hActive == Controls[n])
        {
            current = n + direction;
            break;
        }
    }

    if (current < 0)
        current = (_countof(Controls) - 1);
    else if ((UINT)current >= _countof(Controls))
        current = 0;

    ::SetFocus(Controls[current]);
}
// **** CMainWindow ****



VOID ShowMainWindow(INT nShowCmd)
{
    HACCEL KeyBrd;
    MSG Msg;

    CMainWindow* wnd = new CMainWindow();
    if (!wnd)
        return;

    hMainWnd = wnd->Create();
    if (!hMainWnd)
        return;

    /* Maximize it if we must */
    wnd->ShowWindow((SettingsInfo.bSaveWndPos && SettingsInfo.Maximized) ? SW_MAXIMIZE : nShowCmd);
    wnd->UpdateWindow();

    /* Load the menu hotkeys */
    KeyBrd = LoadAcceleratorsW(NULL, MAKEINTRESOURCEW(HOTKEYS));

    /* Message Loop */
    while (GetMessageW(&Msg, NULL, 0, 0))
    {
        if (!TranslateAcceleratorW(hMainWnd, KeyBrd, &Msg))
        {
            if (Msg.message == WM_CHAR &&
                Msg.wParam == VK_TAB)
            {
                // Move backwards if shift is held down
                int direction = (GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1;

                wnd->HandleTabOrder(direction);
                continue;
            }

            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
        }
    }

    delete wnd;
}

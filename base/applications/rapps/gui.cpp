/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     GUI classes for RAPPS
 * COPYRIGHT:   Copyright 2015 David Quintana (gigaherz@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov (sanchaez@reactos.org)
 *              Copyright 2020 He Yang (1160386205@qq.com)
 */

#include "rapps.h"
#include "rosui.h"
#include "crichedit.h"
#include "asyncinet.h"
#include "gui.h"
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

// **** CSideTreeView ****

CSideTreeView::CSideTreeView()
    : CUiWindow(),
      hImageTreeView(ImageList_Create(TREEVIEW_ICON_SIZE, TREEVIEW_ICON_SIZE, GetSystemColorDepth() | ILC_MASK, 0, 1))
{
}

HTREEITEM
CSideTreeView::AddItem(HTREEITEM hParent, CStringW &Text, INT Image, INT SelectedImage, LPARAM lParam)
{
    return CUiWindow<CTreeView>::AddItem(hParent, const_cast<LPWSTR>(Text.GetString()), Image, SelectedImage, lParam);
}

HTREEITEM
CSideTreeView::AddCategory(HTREEITEM hRootItem, UINT TextIndex, UINT IconIndex)
{
    CStringW szText;
    INT Index = 0;
    HICON hIcon;

    hIcon = (HICON)LoadImageW(
        hInst, MAKEINTRESOURCE(IconIndex), IMAGE_ICON, TREEVIEW_ICON_SIZE, TREEVIEW_ICON_SIZE, LR_CREATEDIBSECTION);
    if (hIcon)
    {
        Index = ImageList_AddIcon(hImageTreeView, hIcon);
        DestroyIcon(hIcon);
    }

    szText.LoadStringW(TextIndex);
    return AddItem(hRootItem, szText, Index, Index, TextIndex);
}

HIMAGELIST
CSideTreeView::SetImageList()
{
    return CUiWindow<CTreeView>::SetImageList(hImageTreeView, TVSIL_NORMAL);
}

VOID
CSideTreeView::DestroyImageList()
{
    if (hImageTreeView)
        ImageList_Destroy(hImageTreeView);
}

CSideTreeView::~CSideTreeView()
{
    DestroyImageList();
}
// **** CSideTreeView ****

// **** CMainWindow ****

CMainWindow::CMainWindow(CAppDB *db, BOOL bAppwiz) : m_ClientPanel(NULL), m_Db(db), m_bAppwizMode(bAppwiz), SelectedEnumType(ENUM_ALL_INSTALLED)
{
}

CMainWindow::~CMainWindow()
{
    LayoutCleanup();
}

VOID
CMainWindow::InitCategoriesList()
{
    HTREEITEM hRootItemAvailable;

    hRootItemInstalled = m_TreeView->AddCategory(TVI_ROOT, IDS_INSTALLED, IDI_CATEGORY);
    m_TreeView->AddCategory(hRootItemInstalled, IDS_APPLICATIONS, IDI_APPS);
    m_TreeView->AddCategory(hRootItemInstalled, IDS_UPDATES, IDI_APPUPD);

    // Do not show any other categories in APPWIZ-mode.
    if (m_bAppwizMode)
        goto Finish;

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

Finish:
    m_TreeView->SetImageList();
    m_TreeView->Expand(hRootItemInstalled, TVE_EXPAND);
    if (!m_bAppwizMode)
        m_TreeView->Expand(hRootItemAvailable, TVE_EXPAND);
    m_TreeView->SelectItem(m_bAppwizMode ? hRootItemInstalled : hRootItemAvailable);
}

BOOL
CMainWindow::CreateStatusBar()
{
    m_StatusBar = new CUiWindow<CStatusBar>();
    m_StatusBar->m_VerticalAlignment = UiAlign_RightBtm;
    m_StatusBar->m_HorizontalAlignment = UiAlign_Stretch;
    m_ClientPanel->Children().Append(m_StatusBar);

    return m_StatusBar->Create(m_hWnd, (HMENU)IDC_STATUSBAR) != NULL;
}

BOOL
CMainWindow::CreateTreeView()
{
    m_TreeView = new CSideTreeView();
    m_TreeView->m_VerticalAlignment = UiAlign_Stretch;
    m_TreeView->m_HorizontalAlignment = UiAlign_Stretch;
    m_VSplitter->First().Append(m_TreeView);

    return m_TreeView->Create(m_hWnd) != NULL;
}

BOOL
CMainWindow::CreateApplicationView()
{
    m_ApplicationView = new CApplicationView(this); // pass this to ApplicationView for callback purpose
    m_ApplicationView->m_VerticalAlignment = UiAlign_Stretch;
    m_ApplicationView->m_HorizontalAlignment = UiAlign_Stretch;
    m_VSplitter->Second().Append(m_ApplicationView);

    return m_ApplicationView->Create(m_hWnd) != NULL;
}

BOOL
CMainWindow::CreateVSplitter()
{
    m_VSplitter = new CUiSplitPanel();
    m_VSplitter->m_VerticalAlignment = UiAlign_Stretch;
    m_VSplitter->m_HorizontalAlignment = UiAlign_Stretch;
    m_VSplitter->m_DynamicFirst = FALSE;
    m_VSplitter->m_Horizontal = FALSE;
    m_VSplitter->m_MinFirst = 0;

    // TODO: m_MinSecond should be calculate dynamically instead of hard-coded
    m_VSplitter->m_MinSecond = 480;
    m_VSplitter->m_Pos = 240;
    m_ClientPanel->Children().Append(m_VSplitter);

    return m_VSplitter->Create(m_hWnd) != NULL;
}

BOOL
CMainWindow::CreateLayout()
{
    BOOL b = TRUE;
    bUpdating = TRUE;

    m_ClientPanel = new CUiPanel();
    m_ClientPanel->m_VerticalAlignment = UiAlign_Stretch;
    m_ClientPanel->m_HorizontalAlignment = UiAlign_Stretch;

    // Top level
    b = b && CreateStatusBar();
    b = b && CreateVSplitter();

    // Inside V Splitter
    b = b && CreateTreeView();
    b = b && CreateApplicationView();

    if (b)
    {
        RECT rBottom;

        /* Size status bar */
        m_StatusBar->SendMessageW(WM_SIZE, 0, 0);

        ::GetWindowRect(m_StatusBar->m_hWnd, &rBottom);

        m_VSplitter->m_Margin.bottom = rBottom.bottom - rBottom.top;
    }

    bUpdating = FALSE;
    return b;
}

VOID
CMainWindow::LayoutCleanup()
{
    delete m_TreeView;
    delete m_ApplicationView;
    delete m_VSplitter;
    delete m_StatusBar;
    return;
}

BOOL
CMainWindow::InitControls()
{
    if (CreateLayout())
    {
        InitCategoriesList();
        UpdateStatusBarText();

        return TRUE;
    }

    return FALSE;
}

VOID
CMainWindow::OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (wParam == SIZE_MINIMIZED)
        return;

    /* Size status bar */
    m_StatusBar->SendMessage(WM_SIZE, 0, 0);

    RECT r = {0, 0, LOWORD(lParam), HIWORD(lParam)};
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
}

BOOL
CMainWindow::RemoveSelectedAppFromRegistry()
{
    if (!IsInstalledEnum(SelectedEnumType))
        return FALSE;

    CStringW szMsgText, szMsgTitle;

    if (!szMsgText.LoadStringW(IDS_APP_REG_REMOVE) || !szMsgTitle.LoadStringW(IDS_INFORMATION))
        return FALSE;

    CAppInfo *InstalledApp = (CAppInfo *)m_ApplicationView->GetFocusedItemData();
    if (!InstalledApp)
        return FALSE;

    if (MessageBoxW(szMsgText, szMsgTitle, MB_YESNO | MB_ICONQUESTION) == IDYES)
        return m_Db->RemoveInstalledAppFromRegistry(InstalledApp) == ERROR_SUCCESS;

    return FALSE;
}

BOOL
CMainWindow::UninstallSelectedApp(BOOL bModify)
{
    if (!IsInstalledEnum(SelectedEnumType))
        return FALSE;

    CAppInfo *InstalledApp = (CAppInfo *)m_ApplicationView->GetFocusedItemData();
    if (!InstalledApp)
        return FALSE;

    return InstalledApp->UninstallApplication(bModify ? UCF_MODIFY : UCF_NONE);
}

VOID
CMainWindow::CheckAvailable()
{
    if (m_Db->GetAvailableCount() == 0)
    {
        CUpdateDatabaseMutex lock;
        m_Db->RemoveCached();
        m_Db->UpdateAvailable();
    }
}

BOOL
CMainWindow::ProcessWindowMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT &theResult, DWORD dwMapId)
{
    theResult = 0;
    switch (Msg)
    {
        case WM_CREATE:
            if (!InitControls())
                ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
            ::PostMessageW(hwnd, DM_REPOSITION, 0, 0);
            break;

        case WM_DESTROY:
        {
            hMainWnd = NULL;
            SaveSettings(hwnd, &SettingsInfo);
            FreeLogs();

            delete m_ClientPanel;

            PostQuitMessage(0);
            return 0;
        }

        case WM_CLOSE:
            ShowWindow(SW_HIDE);
            return g_Busy;

        case WM_NOTIFY_OPERATIONCOMPLETED:
            if (!g_Busy && !IsWindowVisible())
                SendMessage(WM_CLOSE, 0, 0);
            break;

        case DM_REPOSITION:
            EmulateDialogReposition(hwnd); // We are not a real dialog, we need help from a real one
            break;

        case WM_COMMAND:
            OnCommand(wParam, lParam);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR)lParam;

            switch (data->code)
            {
                case TVN_ITEMEXPANDING:
                {
                    if (data->hwndFrom == m_TreeView->m_hWnd)
                    {
                        // APPWIZ-mode: forbid item collapse.
                        // FIXME: Prevent collapse (COMCTL32 is buggy)
                        // https://bugs.winehq.org/show_bug.cgi?id=53727
                        if (m_bAppwizMode && (((LPNMTREEVIEW)lParam)->action & TVE_TOGGLE) == TVE_COLLAPSE)
                        {
                            theResult = TRUE;
                            return TRUE; // Handled
                        }
                    }
                    break;
                }

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
                                UpdateApplicationsList(ENUM_ALL_AVAILABLE, FALSE, TRUE);
                                break;

                            case IDS_CAT_AUDIO:
                                UpdateApplicationsList(ENUM_CAT_AUDIO, FALSE, TRUE);
                                break;

                            case IDS_CAT_DEVEL:
                                UpdateApplicationsList(ENUM_CAT_DEVEL, FALSE, TRUE);
                                break;

                            case IDS_CAT_DRIVERS:
                                UpdateApplicationsList(ENUM_CAT_DRIVERS, FALSE, TRUE);
                                break;

                            case IDS_CAT_EDU:
                                UpdateApplicationsList(ENUM_CAT_EDU, FALSE, TRUE);
                                break;

                            case IDS_CAT_ENGINEER:
                                UpdateApplicationsList(ENUM_CAT_ENGINEER, FALSE, TRUE);
                                break;

                            case IDS_CAT_FINANCE:
                                UpdateApplicationsList(ENUM_CAT_FINANCE, FALSE, TRUE);
                                break;

                            case IDS_CAT_GAMES:
                                UpdateApplicationsList(ENUM_CAT_GAMES, FALSE, TRUE);
                                break;

                            case IDS_CAT_GRAPHICS:
                                UpdateApplicationsList(ENUM_CAT_GRAPHICS, FALSE, TRUE);
                                break;

                            case IDS_CAT_INTERNET:
                                UpdateApplicationsList(ENUM_CAT_INTERNET, FALSE, TRUE);
                                break;

                            case IDS_CAT_LIBS:
                                UpdateApplicationsList(ENUM_CAT_LIBS, FALSE, TRUE);
                                break;

                            case IDS_CAT_OFFICE:
                                UpdateApplicationsList(ENUM_CAT_OFFICE, FALSE, TRUE);
                                break;

                            case IDS_CAT_OTHER:
                                UpdateApplicationsList(ENUM_CAT_OTHER, FALSE, TRUE);
                                break;

                            case IDS_CAT_SCIENCE:
                                UpdateApplicationsList(ENUM_CAT_SCIENCE, FALSE, TRUE);
                                break;

                            case IDS_CAT_TOOLS:
                                UpdateApplicationsList(ENUM_CAT_TOOLS, FALSE, TRUE);
                                break;

                            case IDS_CAT_VIDEO:
                                UpdateApplicationsList(ENUM_CAT_VIDEO, FALSE, TRUE);
                                break;

                            case IDS_CAT_THEMES:
                                UpdateApplicationsList(ENUM_CAT_THEMES, FALSE, TRUE);
                                break;

                            case IDS_SELECTEDFORINST:
                                UpdateApplicationsList(ENUM_CAT_SELECTED);
                                break;
                        }
                    }
                }
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
        }
        break;

        case WM_SETTINGCHANGE:
            if (wParam == SPI_SETNONCLIENTMETRICS || wParam == SPI_SETICONMETRICS)
            {
                DestroyIcon(g_hDefaultPackageIcon);
                g_hDefaultPackageIcon = NULL; // Trigger imagelist recreation on next load
            }
            break;

        case WM_TIMER:
            if (wParam == SEARCH_TIMER_ID)
            {
                ::KillTimer(hwnd, SEARCH_TIMER_ID);

                UpdateApplicationsList(SelectedEnumType);
            }
            break;
    }

    return FALSE;
}

VOID
CMainWindow::ShowAboutDlg()
{
    CStringW szApp;
    CStringW szAuthors;

    szApp.LoadStringW(IDS_APPTITLE);
    szAuthors.LoadStringW(IDS_APP_AUTHORS);
    ShellAboutW(m_hWnd, szApp, szAuthors,
                LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN)));
}

VOID
CMainWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
    const BOOL bReload = TRUE;
    WORD wCommand = LOWORD(wParam);

    if (!lParam)
    {
        switch (wCommand)
        {
            case ID_SETTINGS:
                CreateSettingsDlg(m_hWnd);
                break;

            case ID_EXIT:
                PostMessageW(WM_CLOSE, 0, 0);
                break;

            case ID_SEARCH:
                m_ApplicationView->SetFocusOnSearchBar();
                break;

            case ID_INSTALL:
                if (IsAvailableEnum(SelectedEnumType))
                {
                    if (!m_Selected.IsEmpty())
                    {
                        if (DownloadListOfApplications(m_Selected, FALSE))
                        {
                            m_Selected.RemoveAll();
                            UpdateApplicationsList(SelectedEnumType);
                        }
                    }
                    else
                    {
                        CAppInfo *App = (CAppInfo *)m_ApplicationView->GetFocusedItemData();
                        if (App)
                        {
                            InstallApplication(App);
                        }
                    }
                }
                break;

            case ID_UNINSTALL:
                if (UninstallSelectedApp(FALSE))
                    UpdateApplicationsList(SelectedEnumType, bReload);
                break;

            case ID_MODIFY:
                if (UninstallSelectedApp(TRUE))
                    UpdateApplicationsList(SelectedEnumType, bReload);
                break;

            case ID_REGREMOVE:
                if (RemoveSelectedAppFromRegistry())
                    UpdateApplicationsList(SelectedEnumType, bReload);
                break;

            case ID_REFRESH:
                UpdateApplicationsList(SelectedEnumType, bReload);
                break;

            case ID_RESETDB:
            {
                CUpdateDatabaseMutex lock;
                m_Db->RemoveCached();
                UpdateApplicationsList(SelectedEnumType, bReload);
                break;
            }

            case ID_HELP:
                MessageBoxW(L"Help not implemented yet", NULL, MB_OK);
                break;

            case ID_ABOUT:
                ShowAboutDlg();
                break;

            case ID_CHECK_ALL:
                m_ApplicationView->CheckAll();
                break;

            case ID_ACTIVATE_APPWIZ:
                if (hRootItemInstalled)
                    m_TreeView->SelectItem(hRootItemInstalled);
                break;
        }
    }
}

VOID
CMainWindow::UpdateStatusBarText()
{
    if (m_StatusBar)
    {
        CStringW szBuffer;
        szBuffer.Format(IDS_APPS_COUNT, m_ApplicationView->GetItemCount());

        // Append the number of selected apps if not in APPWIZ-mode.
        if (!m_bAppwizMode)
        {
            CStringW szBuffer2;
            szBuffer2.Format(IDS_APPS_SELECT_COUNT, m_Selected.GetCount());
            szBuffer += szBuffer2;
        }

        m_StatusBar->SetText(szBuffer);
    }
}

VOID
CMainWindow::AddApplicationsToView(CAtlList<CAppInfo *> &List)
{
    POSITION CurrentListPosition = List.GetHeadPosition();
    while (CurrentListPosition)
    {
        CAppInfo *Info = List.GetNext(CurrentListPosition);
        if (szSearchPattern.IsEmpty() || SearchPatternMatch(Info->szDisplayName, szSearchPattern) ||
            SearchPatternMatch(Info->szComments, szSearchPattern))
        {
            BOOL bSelected = m_Selected.Find(Info) != NULL;
            m_ApplicationView->AddApplication(Info, bSelected);
        }
    }
    m_ApplicationView->AddApplication(NULL, FALSE); // Tell the list we are done
}

VOID
CMainWindow::UpdateApplicationsList(AppsCategories EnumType, BOOL bReload, BOOL bCheckAvailable)
{
    // Only installed applications should be enumerated in APPWIZ-mode.
    if (m_bAppwizMode && !IsInstalledEnum(EnumType))
    {
        ATLASSERT(FALSE && "Should not be called in APPWIZ-mode");
        return;
    }

    bUpdating = TRUE;

    if (HCURSOR hCursor = LoadCursor(NULL, IDC_APPSTARTING))
    {
        // The database (.ini files) is parsed on the UI thread, let the user know we are busy
        SetCursor(hCursor);
        PostMessage(WM_SETCURSOR, (WPARAM)m_hWnd, MAKELONG(HTCLIENT, WM_MOUSEMOVE));
    }

    if (bCheckAvailable)
        CheckAvailable();

    BOOL TryRestoreSelection = SelectedEnumType == EnumType;
    if (SelectedEnumType != EnumType)
        SelectedEnumType = EnumType;

    CApplicationView::RESTORELISTSELECTION RestoreSelection;
    if (TryRestoreSelection)
        m_ApplicationView->GetRestoreListSelectionData(RestoreSelection);

    if (bReload)
        m_Selected.RemoveAll();

    m_ApplicationView->SetRedraw(FALSE);
    if (IsInstalledEnum(EnumType))
    {
        if (bReload)
            m_Db->UpdateInstalled();

        // set the display type of application-view. this will remove all the item in application-view too.
        m_ApplicationView->SetDisplayAppType(AppViewTypeInstalledApps);

        CAtlList<CAppInfo *> List;
        m_Db->GetApps(List, EnumType);
        AddApplicationsToView(List);
    }
    else if (IsAvailableEnum(EnumType))
    {
        // We shouldn't get there in APPWIZ-mode.
        ATLASSERT(!m_bAppwizMode);

        if (bReload)
            m_Db->UpdateAvailable();

        // set the display type of application-view. this will remove all the item in application-view too.
        m_ApplicationView->SetDisplayAppType(AppViewTypeAvailableApps);

        // enum available softwares
        if (EnumType == ENUM_CAT_SELECTED)
        {
            AddApplicationsToView(m_Selected);
        }
        else
        {
            CAtlList<CAppInfo *> List;
            m_Db->GetApps(List, EnumType);
            AddApplicationsToView(List);
        }
    }
    else
    {
        ATLASSERT(0 && "This should be unreachable!");
    }

    if (TryRestoreSelection)
        m_ApplicationView->RestoreListSelection(RestoreSelection);
    m_ApplicationView->SetRedraw(TRUE);
    m_ApplicationView->RedrawWindow(0, 0, RDW_INVALIDATE | RDW_ALLCHILDREN); // force the child window to repaint
    UpdateStatusBarText();

    CStringW text;
    if (m_ApplicationView->GetItemCount() == 0 && !szSearchPattern.IsEmpty())
    {
        text.LoadString(IDS_NO_SEARCH_RESULTS);
    }
    m_ApplicationView->SetWatermark(text);

    bUpdating = FALSE;
}

ATL::CWndClassInfo &
CMainWindow::GetWndClassInfo()
{
    DWORD csStyle = CS_VREDRAW | CS_HREDRAW;
    static ATL::CWndClassInfo wc = {
        {sizeof(WNDCLASSEX), csStyle, StartWindowProc, 0, 0, NULL,
         LoadIconW(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCEW(IDI_MAIN)), LoadCursorW(NULL, IDC_ARROW),
         (HBRUSH)(COLOR_BTNFACE + 1), MAKEINTRESOURCEW(IDR_MAINMENU), szWindowClass, NULL},
        NULL,
        NULL,
        IDC_ARROW,
        TRUE,
        0,
        _T("")};
    return wc;
}

HWND
CMainWindow::Create()
{
    const CStringW szWindowName(MAKEINTRESOURCEW(m_bAppwizMode ? IDS_APPWIZ_TITLE : IDS_APPTITLE));

    RECT r = {
        (SettingsInfo.bSaveWndPos ? SettingsInfo.Left : CW_USEDEFAULT),
        (SettingsInfo.bSaveWndPos ? SettingsInfo.Top : CW_USEDEFAULT),
        (SettingsInfo.bSaveWndPos ? SettingsInfo.Width : 680), (SettingsInfo.bSaveWndPos ? SettingsInfo.Height : 450)};
    r.right += r.left;
    r.bottom += r.top;

    return CWindowImpl::Create(
        NULL, r, szWindowName.GetString(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
}

// this function is called when a item of application-view is checked/unchecked
// CallbackParam is the param passed to application-view when adding the item (the one getting focus now).
VOID
CMainWindow::ItemCheckStateChanged(BOOL bChecked, LPVOID CallbackParam)
{
    if (bUpdating)
        return;

    CAppInfo *Info = (CAppInfo *)CallbackParam;

    if (bChecked)
    {
        m_Selected.AddTail(Info);
    }
    else
    {
        POSITION Pos = m_Selected.Find(Info);
        ATLASSERT(Pos != NULL);

        if (Pos != NULL)
        {
            m_Selected.RemoveAt(Pos);
        }
    }

    UpdateStatusBarText();
}

// this function is called when one or more application(s) should be installed
// if Info is not zero, this app should be installed. otherwise those checked apps should be installed
BOOL
CMainWindow::InstallApplication(CAppInfo *Info)
{
    if (Info)
    {
        if (DownloadApplication(Info))
        {
            //FIXME: Delay UpdateApplicationsList(SelectedEnumType); until install completes
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
CMainWindow::SearchTextChanged(CStringW &SearchText)
{
    if (szSearchPattern == SearchText)
    {
        return FALSE;
    }

    szSearchPattern = SearchText;

    DWORD dwDelay = 0;
    SystemParametersInfoW(SPI_GETMENUSHOWDELAY, 0, &dwDelay, 0);
    SetTimer(SEARCH_TIMER_ID, dwDelay);

    return TRUE;
}

void
CMainWindow::HandleTabOrder(int direction)
{
    ATL::CSimpleArray<HWND> TabOrderHwndList;

    m_TreeView->AppendTabOrderWindow(direction, TabOrderHwndList);
    m_ApplicationView->AppendTabOrderWindow(direction, TabOrderHwndList);

    if (TabOrderHwndList.GetSize() == 0)
    {
        // in case the list is empty
        return;
    }

    int FocusIndex;

    if ((FocusIndex = TabOrderHwndList.Find(GetFocus())) == -1)
    {
        FocusIndex = 0; // focus the first window in the list
    }
    else
    {
        FocusIndex += direction;
        FocusIndex +=
            TabOrderHwndList.GetSize(); // FocusIndex might be negative. we don't want to mod a negative number
        FocusIndex %= TabOrderHwndList.GetSize();
    }

    ::SetFocus(TabOrderHwndList[FocusIndex]);
    return;
}
// **** CMainWindow ****

VOID
MainWindowLoop(CMainWindow *wnd, INT nShowCmd)
{
    HACCEL KeyBrd;
    MSG Msg;

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
            if (Msg.message == WM_CHAR && Msg.wParam == VK_TAB)
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
}

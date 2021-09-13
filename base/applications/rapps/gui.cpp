/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     GUI classes for RAPPS
 * COPYRIGHT:   Copyright 2015 David Quintana           (gigaherz@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov   (sanchaez@reactos.org)
 *              Copyright 2020 He Yang                  (1160386205@qq.com)
 */

#include "rapps.h"
#include "rosui.h"
#include "crichedit.h"
#include "appview.h"
#include "asyncinet.h"
#include "misc.h"
#include "gui.h"
#include "appview.h"
#include "winmain.h"
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
    INT Index = 0;
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



// **** CMainWindow ****

CMainWindow::CMainWindow() :
    m_ClientPanel(NULL),
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

    // TODO: m_MinSecond should be calculate dynamically instead of hard-coded
    m_VSplitter->m_MinSecond = 480;
    m_VSplitter->m_Pos = 240;
    m_ClientPanel->Children().Append(m_VSplitter);

    return m_VSplitter->Create(m_hWnd) != NULL;
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

VOID CMainWindow::LayoutCleanup()
{
    delete m_TreeView;
    delete m_ApplicationView;
    delete m_VSplitter;
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
        if (!InstalledApp)
            return FALSE;

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
    if (!InstalledApp)
        return FALSE;

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
        SaveSettings(hwnd, &SettingsInfo);

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
            }
            else
            {
                EnableMenuItem(mainMenu, ID_REGREMOVE, MF_GRAYED);
                EnableMenuItem(mainMenu, ID_INSTALL, MF_ENABLED);
                EnableMenuItem(mainMenu, ID_UNINSTALL, MF_GRAYED);
                EnableMenuItem(mainMenu, ID_MODIFY, MF_GRAYED);
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

    case WM_TIMER:
        if (wParam == SEARCH_TIMER_ID)
        {
            ::KillTimer(hwnd, SEARCH_TIMER_ID);

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
        // set the display type of application-view. this will remove all the item in application-view too.
        m_ApplicationView->SetDisplayAppType(AppViewTypeInstalledApps);

        // enum installed softwares
        m_InstalledApps.Enum(EnumType, s_EnumInstalledAppProc, this);
    }
    else if (IsAvailableEnum(EnumType))
    {
        // set the display type of application-view. this will remove all the item in application-view too.
        m_ApplicationView->SetDisplayAppType(AppViewTypeAvailableApps);

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
            szWindowClass,
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

// this function is called when one or more application(s) should be installed install
// if Info is not zero, this app should be installed. otherwise those checked apps should be installed
BOOL CMainWindow::InstallApplication(CAvailableApplicationInfo *Info)
{
    if (Info)
    {
        if (DownloadApplication(Info, FALSE))
        {
            UpdateApplicationsList(-1);
            return TRUE;
        }
    }
    else
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
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL CMainWindow::SearchTextChanged(ATL::CStringW &SearchText)
{
    if (szSearchPattern == SearchText)
    {
        return FALSE;
    }

    szSearchPattern = SearchText;

    DWORD dwDelay;
    SystemParametersInfoW(SPI_GETMENUSHOWDELAY, 0, &dwDelay, 0);
    SetTimer(SEARCH_TIMER_ID, dwDelay);

    return TRUE;
}

void CMainWindow::HandleTabOrder(int direction)
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
        FocusIndex += TabOrderHwndList.GetSize(); // FocusIndex might be negative. we don't want to mod a negative number
        FocusIndex %= TabOrderHwndList.GetSize();
    }

    ::SetFocus(TabOrderHwndList[FocusIndex]);
    return;
}
// **** CMainWindow ****



VOID MainWindowLoop(INT nShowCmd)
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

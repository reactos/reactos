/* PROJECT:     ReactOS CE Applications Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * AUTHORS:     David Quintana <gigaherz@gmail.com>
 */

#include "rapps.h"

#include <shlobj_undoc.h>
#include <shlguid_undoc.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <wininet.h>
#include <shellutils.h>

#include <rosctrls.h>

#include "rosui.h"
#include "crichedit.h"

#define SEARCH_TIMER_ID 'SR'

HWND hListView = NULL;

class CMainToolbar :
    public CUiWindow< CToolbar<> >
{
#define TOOLBAR_HEIGHT 24

    WCHAR szInstallBtn[MAX_STR_LEN];
    WCHAR szUninstallBtn[MAX_STR_LEN];
    WCHAR szModifyBtn[MAX_STR_LEN];

    VOID AddImageToImageList(HIMAGELIST hImageList, UINT ImageIndex)
    {
        HICON hImage;

        if (!(hImage = (HICON) LoadImage(hInst,
            MAKEINTRESOURCE(ImageIndex),
            IMAGE_ICON,
            TOOLBAR_HEIGHT,
            TOOLBAR_HEIGHT,
            0)))
        {
            /* TODO: Error message */
        }

        ImageList_AddIcon(hImageList, hImage);
        DeleteObject(hImage);
    }

    HIMAGELIST InitImageList(VOID)
    {
        HIMAGELIST hImageList;

        /* Create the toolbar icon image list */
        hImageList = ImageList_Create(TOOLBAR_HEIGHT,//GetSystemMetrics(SM_CXSMICON),
            TOOLBAR_HEIGHT,//GetSystemMetrics(SM_CYSMICON),
            ILC_MASK | GetSystemColorDepth(),
            1,
            1);
        if (!hImageList)
        {
            /* TODO: Error message */
            return NULL;
        }

        AddImageToImageList(hImageList, IDI_INSTALL);
        AddImageToImageList(hImageList, IDI_UNINSTALL);
        AddImageToImageList(hImageList, IDI_MODIFY);
        AddImageToImageList(hImageList, IDI_REFRESH);
        AddImageToImageList(hImageList, IDI_SETTINGS);
        AddImageToImageList(hImageList, IDI_EXIT);

        return hImageList;
    }

public:
    VOID OnGetDispInfo(LPTOOLTIPTEXT lpttt)
    {
        UINT idButton = (UINT) lpttt->hdr.idFrom;

        switch (idButton)
        {
        case ID_EXIT:
            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_EXIT);
            break;

        case ID_INSTALL:
            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_INSTALL);
            break;

        case ID_UNINSTALL:
            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_UNINSTALL);
            break;

        case ID_MODIFY:
            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_MODIFY);
            break;

        case ID_SETTINGS:
            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_SETTINGS);
            break;

        case ID_REFRESH:
            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_REFRESH);
            break;
        }
    }

    HWND Create(HWND hwndParent)
    {
        static TBBUTTON Buttons [] =
        {   /* iBitmap, idCommand, fsState, fsStyle, bReserved[2], dwData, iString */
            { 0, ID_INSTALL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR) szInstallBtn },
            { 1, ID_UNINSTALL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR) szUninstallBtn },
            { 2, ID_MODIFY, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR) szModifyBtn },
            { 5, 0, TBSTATE_ENABLED, BTNS_SEP, { 0 }, 0, 0 },
            { 3, ID_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0 },
            { 5, 0, TBSTATE_ENABLED, BTNS_SEP, { 0 }, 0, 0 },
            { 4, ID_SETTINGS, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0 },
            { 5, ID_EXIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0 }
        };

        INT NumButtons = sizeof(Buttons) / sizeof(Buttons[0]);
        HIMAGELIST hImageList;

        LoadStringW(hInst, IDS_INSTALL, szInstallBtn, sizeof(szInstallBtn) / sizeof(WCHAR));
        LoadStringW(hInst, IDS_UNINSTALL, szUninstallBtn, sizeof(szUninstallBtn) / sizeof(WCHAR));
        LoadStringW(hInst, IDS_MODIFY, szModifyBtn, sizeof(szModifyBtn) / sizeof(WCHAR));

        m_hWnd = CreateWindowExW(0,
            TOOLBARCLASSNAMEW,
            NULL,
            WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST,
            0, 0, 0, 0,
            hwndParent,
            0,
            hInst,
            NULL);

        if (!m_hWnd)
        {
            /* TODO: Show error message */
            return FALSE;
        }

        SendMessageW(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_HIDECLIPPEDBUTTONS);
        SetButtonStructSize();

        hImageList = InitImageList();

        if (!hImageList)
        {
            /* TODO: Show error message */
            return FALSE;
        }

        ImageList_Destroy((HIMAGELIST) SetImageList(hImageList));

        AddButtons(NumButtons, Buttons);

        return m_hWnd;
    }
};

class CAppsListView :
    public CUiWindow<CListView>
{
    struct SortContext
    {
        CAppsListView * lvw;
        int iSubItem;
    };

public:
    BOOL bAscending;

    CAppsListView()
    {
        bAscending = TRUE;
    }

    VOID ColumnClick(LPNMLISTVIEW pnmv)
    {
        SortContext ctx = { this, pnmv->iSubItem };

        SortItems(s_CompareFunc, &ctx);

        bAscending = !bAscending;
    }

    PVOID GetLParam(INT Index)
    {
        INT ItemIndex;
        LVITEM Item;

        if (Index == -1)
        {
            ItemIndex = (INT) SendMessage(LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
            if (ItemIndex == -1)
                return NULL;
        }
        else
        {
            ItemIndex = Index;
        }

        ZeroMemory(&Item, sizeof(LVITEM));

        Item.mask = LVIF_PARAM;
        Item.iItem = ItemIndex;
        if (!GetItem(&Item))
            return NULL;

        return (PVOID) Item.lParam;
    }

    BOOL AddColumn(INT Index, LPWSTR lpText, INT Width, INT Format)
    {
        LV_COLUMN Column;

        ZeroMemory(&Column, sizeof(LV_COLUMN));

        Column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        Column.iSubItem = Index;
        Column.pszText = (LPTSTR) lpText;
        Column.cx = Width;
        Column.fmt = Format;

        return (InsertColumn(Index, &Column) == -1) ? FALSE : TRUE;
    }

    INT AddItem(INT ItemIndex, INT IconIndex, LPWSTR lpText, LPARAM lParam)
    {
        LV_ITEMW Item;

        ZeroMemory(&Item, sizeof(LV_ITEM));

        Item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
        Item.pszText = lpText;
        Item.lParam = lParam;
        Item.iItem = ItemIndex;
        Item.iImage = IconIndex;

        return InsertItem(&Item);
    }

    static INT CALLBACK s_CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
    {
        SortContext * ctx = ((SortContext*) lParamSort);
        return ctx->lvw->CompareFunc(lParam1, lParam2, ctx->iSubItem);
    }
    
    INT CompareFunc(LPARAM lParam1, LPARAM lParam2, INT iSubItem)
    {
        WCHAR Item1[MAX_STR_LEN], Item2[MAX_STR_LEN];
        LVFINDINFO IndexInfo;
        INT Index;

        IndexInfo.flags = LVFI_PARAM;

        IndexInfo.lParam = lParam1;
        Index = FindItem(-1, &IndexInfo);
        GetItemText(Index, iSubItem, Item1, sizeof(Item1) / sizeof(WCHAR));

        IndexInfo.lParam = lParam2;
        Index = FindItem(-1, &IndexInfo);
        GetItemText(Index, iSubItem, Item2, sizeof(Item2) / sizeof(WCHAR));

        if (bAscending)
            return wcscmp(Item2, Item1);
        else
            return wcscmp(Item1, Item2);

        return 0;
    }

    HWND Create(HWND hwndParent)
    {
        RECT r = { 205, 28, 465, 250 };
        DWORD style = WS_CHILD | WS_VISIBLE | LVS_SORTASCENDING | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS;
        HMENU menu = GetSubMenu(LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_APPLICATIONMENU)), 0);
        
        HWND hwnd = CListView::Create(hwndParent, r, NULL, style, WS_EX_CLIENTEDGE, menu);

        if (hwnd)
            SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);

        return hwnd;
    }

};

class CMainWindow :
    public CWindowImpl<CMainWindow, CWindow, CFrameWinTraits>
{
    CUiPanel * m_ClientPanel;
    CUiSplitPanel * m_VSplitter;
    CUiSplitPanel * m_HSplitter;

    CMainToolbar * m_Toolbar;
    CAppsListView * m_ListView;

    CUiWindow<CTreeView> * m_TreeView;
    CUiWindow<CStatusBar> * m_StatusBar;
    CUiWindow<CRichEdit> * m_RichEdit;

    CUiWindow<> * m_SearchBar;

    HIMAGELIST hImageTreeView;

    PWSTR pLink;

    BOOL SearchEnabled;

public:
    CMainWindow() :
        m_ClientPanel(NULL),
        hImageTreeView(NULL),
        pLink(NULL),
        SearchEnabled(TRUE)
    {
    }

private:

    VOID InitApplicationsList(VOID)
    {
        WCHAR szText[MAX_STR_LEN];

        /* Add columns to ListView */
        LoadStringW(hInst, IDS_APP_NAME, szText, _countof(szText));
        m_ListView->AddColumn(0, szText, 200, LVCFMT_LEFT);

        LoadStringW(hInst, IDS_APP_INST_VERSION, szText, _countof(szText));
        m_ListView->AddColumn(1, szText, 90, LVCFMT_RIGHT);

        LoadStringW(hInst, IDS_APP_DESCRIPTION, szText, _countof(szText));
        m_ListView->AddColumn(3, szText, 250, LVCFMT_LEFT);

        UpdateApplicationsList(ENUM_ALL_COMPONENTS);
    }

    HTREEITEM AddCategory(HTREEITEM hRootItem, UINT TextIndex, UINT IconIndex)
    {
        WCHAR szText[MAX_STR_LEN];
        INT Index;
        HICON hIcon;

        hIcon = (HICON) LoadImage(hInst,
            MAKEINTRESOURCE(IconIndex),
            IMAGE_ICON,
            TREEVIEW_ICON_SIZE,
            TREEVIEW_ICON_SIZE,
            LR_CREATEDIBSECTION);

        Index = ImageList_AddIcon(hImageTreeView, hIcon);
        DestroyIcon(hIcon);

        LoadStringW(hInst, TextIndex, szText, _countof(szText));

        return m_TreeView->AddItem(hRootItem, szText, Index, Index, TextIndex);
    }

    VOID InitCategoriesList(VOID)
    {
        HTREEITEM hRootItem1, hRootItem2;

        hRootItem1 = AddCategory(TVI_ROOT, IDS_INSTALLED, IDI_CATEGORY);
        AddCategory(hRootItem1, IDS_APPLICATIONS, IDI_APPS);
        AddCategory(hRootItem1, IDS_UPDATES, IDI_APPUPD);

        hRootItem2 = AddCategory(TVI_ROOT, IDS_AVAILABLEFORINST, IDI_CATEGORY);
        AddCategory(hRootItem2, IDS_CAT_AUDIO, IDI_CAT_AUDIO);
        AddCategory(hRootItem2, IDS_CAT_VIDEO, IDI_CAT_VIDEO);
        AddCategory(hRootItem2, IDS_CAT_GRAPHICS, IDI_CAT_GRAPHICS);
        AddCategory(hRootItem2, IDS_CAT_GAMES, IDI_CAT_GAMES);
        AddCategory(hRootItem2, IDS_CAT_INTERNET, IDI_CAT_INTERNET);
        AddCategory(hRootItem2, IDS_CAT_OFFICE, IDI_CAT_OFFICE);
        AddCategory(hRootItem2, IDS_CAT_DEVEL, IDI_CAT_DEVEL);
        AddCategory(hRootItem2, IDS_CAT_EDU, IDI_CAT_EDU);
        AddCategory(hRootItem2, IDS_CAT_ENGINEER, IDI_CAT_ENGINEER);
        AddCategory(hRootItem2, IDS_CAT_FINANCE, IDI_CAT_FINANCE);
        AddCategory(hRootItem2, IDS_CAT_SCIENCE, IDI_CAT_SCIENCE);
        AddCategory(hRootItem2, IDS_CAT_TOOLS, IDI_CAT_TOOLS);
        AddCategory(hRootItem2, IDS_CAT_DRIVERS, IDI_CAT_DRIVERS);
        AddCategory(hRootItem2, IDS_CAT_LIBS, IDI_CAT_LIBS);
        AddCategory(hRootItem2, IDS_CAT_OTHER, IDI_CAT_OTHER);

        m_TreeView->SetImageList(hImageTreeView, TVSIL_NORMAL);
        m_TreeView->Expand(hRootItem2, TVE_EXPAND);
        m_TreeView->Expand(hRootItem1, TVE_EXPAND);
        m_TreeView->SelectItem(hRootItem1);
    }

    BOOL CreateStatusBar()
    {
        m_StatusBar = new CUiWindow<CStatusBar>();
        m_StatusBar->m_VerticalAlignment = UiAlign_RightBtm;
        m_StatusBar->m_HorizontalAlignment = UiAlign_Stretch;
        m_ClientPanel->Children().Append(m_StatusBar);

        return m_StatusBar->Create(m_hWnd, (HMENU)IDC_STATUSBAR) != NULL;
    }

    BOOL CreateToolbar()
    {
        m_Toolbar = new CMainToolbar();
        m_Toolbar->m_VerticalAlignment = UiAlign_LeftTop;
        m_Toolbar->m_HorizontalAlignment = UiAlign_Stretch;
        m_ClientPanel->Children().Append(m_Toolbar);
        
        return m_Toolbar->Create(m_hWnd) != NULL;
    }

    BOOL CreateTreeView()
    {
        m_TreeView = new CUiWindow<CTreeView>();
        m_TreeView->m_VerticalAlignment = UiAlign_Stretch;
        m_TreeView->m_HorizontalAlignment = UiAlign_Stretch;
        m_VSplitter->First().Append(m_TreeView);
        
        return m_TreeView->Create(m_hWnd) != NULL;
    }

    BOOL CreateListView()
    {
        m_ListView = new CAppsListView();
        m_ListView->m_VerticalAlignment = UiAlign_Stretch;
        m_ListView->m_HorizontalAlignment = UiAlign_Stretch;
        m_HSplitter->First().Append(m_ListView);

        hListView = m_ListView->Create(m_hWnd);
        return hListView != NULL;
    }

    BOOL CreateRichEdit()
    {
        m_RichEdit = new CUiWindow<CRichEdit>();
        m_RichEdit->m_VerticalAlignment = UiAlign_Stretch;
        m_RichEdit->m_HorizontalAlignment = UiAlign_Stretch;
        m_HSplitter->Second().Append(m_RichEdit);

        return m_RichEdit->Create(m_hWnd) != NULL;
    }

    BOOL CreateVSplitter()
    {
        m_VSplitter = new CUiSplitPanel();
        m_VSplitter->m_VerticalAlignment = UiAlign_Stretch;
        m_VSplitter->m_HorizontalAlignment = UiAlign_Stretch;
        m_VSplitter->m_DynamicFirst = FALSE;
        m_VSplitter->m_Horizontal = FALSE;
        m_VSplitter->m_MinFirst = 240;
        m_VSplitter->m_MinSecond = 300;
        m_ClientPanel->Children().Append(m_VSplitter);

        return m_VSplitter->Create(m_hWnd) != NULL;
    }

    BOOL CreateHSplitter()
    {
        m_HSplitter = new CUiSplitPanel();
        m_HSplitter->m_VerticalAlignment = UiAlign_Stretch;
        m_HSplitter->m_HorizontalAlignment = UiAlign_Stretch;
        m_HSplitter->m_DynamicFirst = TRUE;
        m_HSplitter->m_Horizontal = TRUE;
        m_HSplitter->m_Pos = 32768;
        m_HSplitter->m_MinFirst = 300;
        m_HSplitter->m_MinSecond = 80;
        m_VSplitter->Second().Append(m_HSplitter);

        return m_HSplitter->Create(m_hWnd) != NULL;
    }

    BOOL CreateSearchBar(VOID)
    {
        WCHAR szBuf[MAX_STR_LEN];

        // TODO: WRAPPER
        m_SearchBar = new CUiWindow<>();
        m_SearchBar->m_VerticalAlignment = UiAlign_LeftTop;
        m_SearchBar->m_HorizontalAlignment = UiAlign_RightBtm;
        m_SearchBar->m_Margin.top = 6;
        m_SearchBar->m_Margin.right = 6;
        //m_ClientPanel->Children().Append(m_SearchBar);

        HWND hwnd = CreateWindowExW(WS_EX_CLIENTEDGE,
            L"Edit",
            NULL,
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
            0,
            0,
            200,
            22,
            m_Toolbar->m_hWnd,
            (HMENU) 0,
            hInst,
            0);

        m_SearchBar->m_hWnd = hwnd;

        m_SearchBar->SendMessageW(WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

        LoadStringW(hInst, IDS_SEARCH_TEXT, szBuf, sizeof(szBuf) / sizeof(WCHAR));
        m_SearchBar->SetWindowTextW(szBuf);

        return hwnd != NULL;
    }

    BOOL CreateLayout()
    {
        bool b = TRUE;

        m_ClientPanel = new CUiPanel();
        m_ClientPanel->m_VerticalAlignment = UiAlign_Stretch;
        m_ClientPanel->m_HorizontalAlignment = UiAlign_Stretch;

        // Top level
        b = b && CreateStatusBar();
        b = b && CreateToolbar();
        b = b && CreateSearchBar();
        b = b && CreateVSplitter();

        // Inside V Splitter
        b = b && CreateHSplitter();
        b = b && CreateTreeView();

        // Inside H Splitter
        b = b && CreateListView();
        b = b && CreateRichEdit();

        if (b)
        {
            RECT rTop;
            RECT rBottom;

            /* Size status bar */
            m_StatusBar->SendMessage(WM_SIZE, 0, 0);

            /* Size tool bar */
            m_Toolbar->AutoSize();

            ::GetWindowRect(m_Toolbar->m_hWnd, &rTop);
            ::GetWindowRect(m_StatusBar->m_hWnd, &rBottom);

            m_VSplitter->m_Margin.top = rTop.bottom - rTop.top;
            m_VSplitter->m_Margin.bottom = rBottom.bottom-rBottom.top;
        }

        return b;
    }

    BOOL InitControls()
    {
        /* Create image list */
        hImageTreeView = ImageList_Create(TREEVIEW_ICON_SIZE, TREEVIEW_ICON_SIZE,
            GetSystemColorDepth() | ILC_MASK,
            0, 1);

        if (CreateLayout())
        {
            WCHAR szBuffer1[MAX_STR_LEN], szBuffer2[MAX_STR_LEN];

            InitApplicationsList();

            InitCategoriesList();

            LoadStringW(hInst, IDS_APPS_COUNT, szBuffer2, _countof(szBuffer2));
            StringCbPrintfW(szBuffer1, sizeof(szBuffer1),
                szBuffer2,
                m_ListView->GetItemCount());
            m_StatusBar->SetText(szBuffer1);
            return TRUE;
        }

        return FALSE;
    }

    VOID OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        /* Size status bar */
        m_StatusBar->SendMessage(WM_SIZE, 0, 0);

        /* Size tool bar */
        m_Toolbar->AutoSize();


        RECT r = { 0, 0, LOWORD(lParam), HIWORD(lParam) };

        HDWP hdwp = NULL;

        int count = m_ClientPanel->CountSizableChildren();
        hdwp = BeginDeferWindowPos(count);
        hdwp = m_ClientPanel->OnParentSize(r, hdwp);
        EndDeferWindowPos(hdwp);

        // TODO: Sub-layouts for children of children
        count = m_SearchBar->CountSizableChildren();
        hdwp = BeginDeferWindowPos(count);
        hdwp = m_SearchBar->OnParentSize(r, hdwp);
        EndDeferWindowPos(hdwp);
    }

    BOOL ProcessWindowMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT& theResult, DWORD dwMapId)
    {
        theResult = 0;
        switch (Msg)
        {
        case WM_CREATE:
            if (!InitControls())
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        case WM_DESTROY:
        {
            ShowWindow(SW_HIDE);
            SaveSettings(hwnd);

            FreeLogs();

            FreeCachedAvailableEntries();

            if (IS_INSTALLED_ENUM(SelectedEnumType))
                FreeInstalledAppList();

            if (hImageTreeView)
                ImageList_Destroy(hImageTreeView);

            delete m_ClientPanel;

            PostQuitMessage(0);
            return 0;
        }

        case WM_COMMAND:
            OnCommand(wParam, lParam);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR) lParam;

            switch (data->code)
            {
            case TVN_SELCHANGED:
            {
                if (data->hwndFrom == m_TreeView->m_hWnd)
                {
                    switch (((LPNMTREEVIEW) lParam)->itemNew.lParam)
                    {
                    case IDS_INSTALLED:
                        UpdateApplicationsList(ENUM_ALL_COMPONENTS);
                        break;

                    case IDS_APPLICATIONS:
                        UpdateApplicationsList(ENUM_APPLICATIONS);
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
                    }
                }

                HMENU mainMenu = GetMenu(hwnd);
                HMENU lvwMenu = GetMenu(m_ListView->m_hWnd);

                /* Disable/enable items based on treeview selection */
                if (IsSelectedNodeInstalled())
                {
                    EnableMenuItem(mainMenu, ID_REGREMOVE, MF_ENABLED);
                    EnableMenuItem(mainMenu, ID_INSTALL, MF_GRAYED);
                    EnableMenuItem(mainMenu, ID_UNINSTALL, MF_ENABLED);
                    EnableMenuItem(mainMenu, ID_MODIFY, MF_ENABLED);

                    EnableMenuItem(lvwMenu, ID_REGREMOVE, MF_ENABLED);
                    EnableMenuItem(lvwMenu, ID_INSTALL, MF_GRAYED);
                    EnableMenuItem(lvwMenu, ID_UNINSTALL, MF_ENABLED);
                    EnableMenuItem(lvwMenu, ID_MODIFY, MF_ENABLED);

                    m_Toolbar->SendMessage(TB_ENABLEBUTTON, ID_REGREMOVE, TRUE);
                    m_Toolbar->SendMessage(TB_ENABLEBUTTON, ID_INSTALL, FALSE);
                    m_Toolbar->SendMessage(TB_ENABLEBUTTON, ID_UNINSTALL, TRUE);
                    m_Toolbar->SendMessage(TB_ENABLEBUTTON, ID_MODIFY, TRUE);
                }
                else
                {
                    EnableMenuItem(mainMenu, ID_REGREMOVE, MF_GRAYED);
                    EnableMenuItem(mainMenu, ID_INSTALL, MF_ENABLED);
                    EnableMenuItem(mainMenu, ID_UNINSTALL, MF_GRAYED);
                    EnableMenuItem(mainMenu, ID_MODIFY, MF_GRAYED);

                    EnableMenuItem(lvwMenu, ID_REGREMOVE, MF_GRAYED);
                    EnableMenuItem(lvwMenu, ID_INSTALL, MF_ENABLED);
                    EnableMenuItem(lvwMenu, ID_UNINSTALL, MF_GRAYED);
                    EnableMenuItem(lvwMenu, ID_MODIFY, MF_GRAYED);

                    m_Toolbar->SendMessage(TB_ENABLEBUTTON, ID_REGREMOVE, FALSE);
                    m_Toolbar->SendMessage(TB_ENABLEBUTTON, ID_INSTALL, TRUE);
                    m_Toolbar->SendMessage(TB_ENABLEBUTTON, ID_UNINSTALL, FALSE);
                    m_Toolbar->SendMessage(TB_ENABLEBUTTON, ID_MODIFY, FALSE);
                }
            }
            break;

            case LVN_ITEMCHANGED:
            {
                LPNMLISTVIEW pnic = (LPNMLISTVIEW) lParam;

                if (pnic->hdr.hwndFrom == m_ListView->m_hWnd)
                {
                    /* Check if this is a valid item
                    * (technically, it can be also an unselect) */
                    INT ItemIndex = pnic->iItem;
                    if (ItemIndex == -1 ||
                        ItemIndex >= ListView_GetItemCount(pnic->hdr.hwndFrom))
                    {
                        break;
                    }

                    /* Check if the focus has been moved to another item */
                    if ((pnic->uChanged & LVIF_STATE) &&
                        (pnic->uNewState & LVIS_FOCUSED) &&
                        !(pnic->uOldState & LVIS_FOCUSED))
                    {
                        if (IS_INSTALLED_ENUM(SelectedEnumType))
                            ShowInstalledAppInfo(ItemIndex);
                        if (IS_AVAILABLE_ENUM(SelectedEnumType))
                            ShowAvailableAppInfo(ItemIndex);
                    }
                }
            }
            break;

            case LVN_COLUMNCLICK:
            {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

                m_ListView->ColumnClick(pnmv);
            }
            break;

            case NM_CLICK:
            {
                if (data->hwndFrom == m_ListView->m_hWnd && ((LPNMLISTVIEW) lParam)->iItem != -1)
                {
                    if (IS_INSTALLED_ENUM(SelectedEnumType))
                        ShowInstalledAppInfo(-1);
                    if (IS_AVAILABLE_ENUM(SelectedEnumType))
                        ShowAvailableAppInfo(-1);
                }
            }
            break;

            case NM_DBLCLK:
            {
                if (data->hwndFrom == m_ListView->m_hWnd && ((LPNMLISTVIEW) lParam)->iItem != -1)
                {
                    /* this won't do anything if the program is already installed */
                    SendMessage(hwnd, WM_COMMAND, ID_INSTALL, 0);
                }
            }
            break;

            case NM_RCLICK:
            {
                if (data->hwndFrom == m_ListView->m_hWnd && ((LPNMLISTVIEW) lParam)->iItem != -1)
                {
                    ShowPopupMenu(m_ListView->m_hWnd, 0, ID_INSTALL);
                }
            }
            break;

            case EN_LINK:
                OnLink((ENLINK*) lParam);
                break;

            case TTN_GETDISPINFO:
                m_Toolbar->OnGetDispInfo((LPTOOLTIPTEXT) lParam);
                break;
            }
        }
        break;

        case WM_SIZE:
            OnSize(hwnd, wParam, lParam);
            break;

        case WM_SIZING:
        {
            LPRECT pRect = (LPRECT) lParam;

            if (pRect->right - pRect->left < 565)
                pRect->right = pRect->left + 565;

            if (pRect->bottom - pRect->top < 300)
                pRect->bottom = pRect->top + 300;

            return TRUE;
        }

        case WM_SYSCOLORCHANGE:
        {
            /* Forward WM_SYSCOLORCHANGE to common controls */
            m_ListView->SendMessage(WM_SYSCOLORCHANGE, 0, 0);
            m_TreeView->SendMessage(WM_SYSCOLORCHANGE, 0, 0);
            m_Toolbar->SendMessage(WM_SYSCOLORCHANGE, 0, 0);
            m_ListView->SendMessage(EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
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

    virtual VOID OnLink(ENLINK *Link)
    {
        switch (Link->msg)
        {
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        {
            if (pLink) HeapFree(GetProcessHeap(), 0, pLink);

            pLink = (PWSTR) HeapAlloc(GetProcessHeap(), 0,
                (max(Link->chrg.cpMin, Link->chrg.cpMax) -
                min(Link->chrg.cpMin, Link->chrg.cpMax) + 1) * sizeof(WCHAR));
            if (!pLink)
            {
                /* TODO: Error message */
                return;
            }

            m_RichEdit->SendMessageW(EM_SETSEL, Link->chrg.cpMin, Link->chrg.cpMax);
            m_RichEdit->SendMessageW(EM_GETSELTEXT, 0, (LPARAM) pLink);

            ShowPopupMenu(m_RichEdit->m_hWnd, IDR_LINKMENU, -1);
        }
        break;
        }
    }

    BOOL IsSelectedNodeInstalled(void)
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

    VOID OnCommand(WPARAM wParam, LPARAM lParam)
    {
        WORD wCommand = LOWORD(wParam);

        if (lParam == (LPARAM) m_SearchBar->m_hWnd)
        {
            WCHAR szBuf[MAX_STR_LEN];

            switch (HIWORD(wParam))
            {
            case EN_SETFOCUS:
            {
                WCHAR szWndText[MAX_STR_LEN];

                LoadStringW(hInst, IDS_SEARCH_TEXT, szBuf, _countof(szBuf));
                GetWindowTextW(m_SearchBar->m_hWnd, szWndText, MAX_STR_LEN);
                if (wcscmp(szBuf, szWndText) == 0)
                {
                    SearchEnabled = FALSE;
                    m_SearchBar->SetWindowTextW(L"");
                }
            }
            break;

            case EN_KILLFOCUS:
            {
                GetWindowTextW(m_SearchBar->m_hWnd, szBuf, MAX_STR_LEN);
                if (wcslen(szBuf) < 1)
                {
                    LoadStringW(hInst, IDS_SEARCH_TEXT, szBuf, _countof(szBuf));
                    SearchEnabled = FALSE;
                    m_SearchBar->SetWindowTextW(szBuf);
                }
            }
            break;

            case EN_CHANGE:
            {
                WCHAR szWndText[MAX_STR_LEN];

                if (!SearchEnabled)
                {
                    SearchEnabled = TRUE;
                    break;
                }

                LoadStringW(hInst, IDS_SEARCH_TEXT, szBuf, _countof(szBuf));
                GetWindowTextW(m_SearchBar->m_hWnd, szWndText, MAX_STR_LEN);
                if (wcscmp(szBuf, szWndText) != 0)
                {
                    StringCbCopy(szSearchPattern, sizeof(szSearchPattern),
                        szWndText);
                }
                else
                {
                    szSearchPattern[0] = UNICODE_NULL;
                }

                DWORD dwDelay;
                SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &dwDelay, 0);
                SetTimer(SEARCH_TIMER_ID, dwDelay);
            }
            break;
            }

            return;
        }

        switch (wCommand)
        {
        case ID_OPEN_LINK:
            ShellExecuteW(m_hWnd, L"open", pLink, NULL, NULL, SW_SHOWNOACTIVATE);
            HeapFree(GetProcessHeap(), 0, pLink);
            break;

        case ID_COPY_LINK:
            CopyTextToClipboard(pLink);
            HeapFree(GetProcessHeap(), 0, pLink);
            break;

        case ID_SETTINGS:
            CreateSettingsDlg(m_hWnd);
            break;

        case ID_EXIT:
            PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
            break;

        case ID_INSTALL:
            if (DownloadApplication(-1))
                /* TODO: Implement install dialog
                *   if (InstallApplication(-1))
                */
                UpdateApplicationsList(-1);
            break;

        case ID_UNINSTALL:
            if (UninstallApplication(-1, FALSE))
                UpdateApplicationsList(-1);
            break;

        case ID_MODIFY:
            if (UninstallApplication(-1, TRUE))
                UpdateApplicationsList(-1);
            break;

        case ID_REGREMOVE:
            RemoveAppFromRegistry(-1);
            break;

        case ID_REFRESH:
            UpdateApplicationsList(-1);
            break;

        case ID_RESETDB:
            UpdateAppsDB();
            UpdateApplicationsList(-1);
            break;

        case ID_HELP:
            MessageBoxW(m_hWnd, L"Help not implemented yet", NULL, MB_OK);
            break;

        case ID_ABOUT:
            ShowAboutDialog();
            break;
        }
    }

    VOID FreeInstalledAppList(VOID)
    {
        INT Count = ListView_GetItemCount(hListView) - 1;
        PINSTALLED_INFO Info;

        while (Count >= 0)
        {
            Info = (PINSTALLED_INFO) ListViewGetlParam(Count);
            if (Info)
            {
                RegCloseKey(Info->hSubKey);
                HeapFree(GetProcessHeap(), 0, Info);
            }
            Count--;
        }
    }

    static BOOL SearchPatternMatch(PCWSTR szHaystack, PCWSTR szNeedle)
    {
        if (!*szNeedle)
            return TRUE;
        /* TODO: Improve pattern search beyond a simple case-insensitive substring search. */
        return StrStrIW(szHaystack, szNeedle) != NULL;
    }

    static BOOL CALLBACK s_EnumInstalledAppProc(INT ItemIndex, LPWSTR lpName, PINSTALLED_INFO Info)
    {
        PINSTALLED_INFO ItemInfo;
        WCHAR szText[MAX_PATH];
        INT Index;

        if (!SearchPatternMatch(lpName, szSearchPattern))
        {
            RegCloseKey(Info->hSubKey);
            return TRUE;
        }

        ItemInfo = (PINSTALLED_INFO) HeapAlloc(GetProcessHeap(), 0, sizeof(INSTALLED_INFO));
        if (!ItemInfo)
        {
            RegCloseKey(Info->hSubKey);
            return FALSE;
        }

        RtlCopyMemory(ItemInfo, Info, sizeof(INSTALLED_INFO));

        Index = ListViewAddItem(ItemIndex, 0, lpName, (LPARAM) ItemInfo);

        /* Get version info */
        GetApplicationString(ItemInfo->hSubKey, L"DisplayVersion", szText);
        ListView_SetItemText(hListView, Index, 1, szText);

        /* Get comments */
        GetApplicationString(ItemInfo->hSubKey, L"Comments", szText);
        ListView_SetItemText(hListView, Index, 2, szText);

        return TRUE;
    }

    static BOOL CALLBACK s_EnumAvailableAppProc(PAPPLICATION_INFO Info)
    {
        INT Index;

        if (!SearchPatternMatch(Info->szName, szSearchPattern) &&
            !SearchPatternMatch(Info->szDesc, szSearchPattern))
        {
            return TRUE;
        }

        /* Only add a ListView entry if...
        - no RegName was supplied (so we cannot determine whether the application is installed or not) or
        -  a RegName was supplied and the application is not installed
        */
        if (!*Info->szRegName || (!IsInstalledApplication(Info->szRegName, FALSE) && !IsInstalledApplication(Info->szRegName, TRUE)))
        {
            Index = ListViewAddItem(Info->Category, 0, Info->szName, (LPARAM) Info);

            ListView_SetItemText(hListView, Index, 1, Info->szVersion);
            ListView_SetItemText(hListView, Index, 2, Info->szDesc);
        }

        return TRUE;
    }

    VOID UpdateApplicationsList(INT EnumType)
    {
        WCHAR szBuffer1[MAX_STR_LEN], szBuffer2[MAX_STR_LEN];
        HICON hIcon;
        HIMAGELIST hImageListView;

        m_ListView->SendMessage(WM_SETREDRAW, FALSE, 0);

        if (EnumType == -1) EnumType = SelectedEnumType;

        if (IS_INSTALLED_ENUM(SelectedEnumType))
            FreeInstalledAppList();

        (VOID) ListView_DeleteAllItems(hListView);

        /* Create image list */
        hImageListView = ImageList_Create(LISTVIEW_ICON_SIZE,
            LISTVIEW_ICON_SIZE,
            GetSystemColorDepth() | ILC_MASK,
            0, 1);

        hIcon = (HICON) LoadImage(hInst,
            MAKEINTRESOURCE(IDI_MAIN),
            IMAGE_ICON,
            LISTVIEW_ICON_SIZE,
            LISTVIEW_ICON_SIZE,
            LR_CREATEDIBSECTION);

        ImageList_AddIcon(hImageListView, hIcon);
        DestroyIcon(hIcon);

        if (IS_INSTALLED_ENUM(EnumType))
        {
            /* Enum installed applications and updates */
            EnumInstalledApplications(EnumType, TRUE, s_EnumInstalledAppProc);
            EnumInstalledApplications(EnumType, FALSE, s_EnumInstalledAppProc);
        }
        else if (IS_AVAILABLE_ENUM(EnumType))
        {
            /* Enum availabled applications */
            EnumAvailableApplications(EnumType, s_EnumAvailableAppProc);
        }

        /* Set image list for ListView */
        hImageListView = ListView_SetImageList(hListView, hImageListView, LVSIL_SMALL);

        /* Destroy old image list */
        if (hImageListView)
            ImageList_Destroy(hImageListView);

        SelectedEnumType = EnumType;

        LoadStringW(hInst, IDS_APPS_COUNT, szBuffer2, _countof(szBuffer2));
        StringCbPrintfW(szBuffer1, sizeof(szBuffer1),
            szBuffer2,
            ListView_GetItemCount(hListView));
        SetStatusBarText(szBuffer1);

        SetWelcomeText();

        /* set automatic column width for program names if the list is not empty */
        if (ListView_GetItemCount(hListView) > 0)
            ListView_SetColumnWidth(hListView, 0, LVSCW_AUTOSIZE);

        SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
    }

public:
    static ATL::CWndClassInfo& GetWndClassInfo()
    {
        DWORD csStyle = CS_VREDRAW |CS_HREDRAW;
        static ATL::CWndClassInfo wc =
        {
            { sizeof(WNDCLASSEX), csStyle, StartWindowProc,
            0, 0, NULL, 
            LoadIcon(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDI_MAIN)),
            LoadCursor(NULL, IDC_ARROW),
            (HBRUSH) (COLOR_BTNFACE + 1), NULL,
            L"RAppsWnd", NULL },
            NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
        };
        return wc;
    }

    HWND Create()
    {
        WCHAR szWindowName[MAX_STR_LEN];

        LoadStringW(hInst, IDS_APPTITLE, szWindowName, _countof(szWindowName));

        RECT r = {
            (SettingsInfo.bSaveWndPos ? SettingsInfo.Left : CW_USEDEFAULT),
            (SettingsInfo.bSaveWndPos ? SettingsInfo.Top : CW_USEDEFAULT),
            (SettingsInfo.bSaveWndPos ? SettingsInfo.Width : 680),
            (SettingsInfo.bSaveWndPos ? SettingsInfo.Height : 450)
        };
        r.right += r.left;
        r.bottom += r.top;

        return CWindowImpl::Create(NULL, r, szWindowName, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
    }

    CStatusBar * GetStatusBar()
    {
        return m_StatusBar;
    }

    CAppsListView * GetListView()
    {
        return m_ListView;
    }

    CRichEdit * GetRichEdit()
    {
        return m_RichEdit;
    }
};

CMainWindow * g_MainWindow;

HWND CreateMainWindow()
{
    g_MainWindow = new CMainWindow();
    return g_MainWindow->Create();
}

DWORD_PTR ListViewGetlParam(INT item)
{
    if (item < 0)
    {
        item = g_MainWindow->GetListView()->GetSelectionMark();
    }
    return g_MainWindow->GetListView()->GetItemData(item);
}

VOID SetStatusBarText(PCWSTR szText)
{
    g_MainWindow->GetStatusBar()->SetText(szText);
}

INT ListViewAddItem(INT ItemIndex, INT IconIndex, PWSTR lpName, LPARAM lParam)
{
    return g_MainWindow->GetListView()->AddItem(ItemIndex, IconIndex, lpName, lParam);
}

VOID NewRichEditText(PCWSTR szText, DWORD flags)
{
    g_MainWindow->GetRichEdit()->SetText(szText, flags);
}

VOID InsertRichEditText(PCWSTR szText, DWORD flags)
{
    g_MainWindow->GetRichEdit()->InsertText(szText, flags);
}
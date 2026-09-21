/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main window class
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller
 *              Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

static TBBUTTON TbButtons[] =
{
    { BTN_UNDO, IDM_TB_UNDO, 0, BTNS_BUTTON, {0}, 0, 0 },
    { BTN_REDO, IDM_TB_REDO, 0, BTNS_BUTTON, {0}, 0, 0 },
    { 4, IDC_STATIC, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { BTN_UP, IDM_TB_UP, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
    { BTN_SCOPE_PANE, IDM_TB_SCOPE_PANE, TBSTATE_ENABLED | TBSTATE_CHECKED, BTNS_CHECK, {0}, 0, 0 },
    { 4, IDC_STATIC, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
//    { BTN_EXPORT_LIST, IDM_TB_EXPORT_LIST, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
    { 4, IDC_STATIC, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
//    { BTN_HELP, IDM_TB_HELP, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
    { BTN_ACTIONS_PANE, IDM_TB_ACTIONS_PANE, TBSTATE_ENABLED, BTNS_CHECK, {0}, 0, 0 }
};

CMainWnd::CMainWnd()
    : m_ConsoleNumber(0)
    , m_DocumentMode(DocumentMode_Author)
    , m_LogicalReadOnly(FALSE)
    , m_PreventViewCustomization(FALSE)
    , m_bToolBarVisible(true)
    , m_NextViewId(1)
    , m_RootNode(NULL)
{
    m_FrameThunk.Init(XDefFrameProc, this);
    m_pfnSuperWindowProc = m_FrameThunk.GetWNDPROC();
    ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    m_hMenuConsoleSmall = LoadMenu(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDM_CONSOLE_SMALL));
    m_hMenuConsoleLarge = LoadMenu(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDM_CONSOLE_LARGE));
    m_hSnapinImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                          GetSystemMetrics(SM_CYSMICON),
                                          ILC_MASK | ILC_COLOR32,
                                          0,
                                          4);

    m_hToolBarImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                           GetSystemMetrics(SM_CYSMICON),
                                           ILC_MASK | ILC_COLOR32,
                                           0,
                                           4);

    HBITMAP hToolBarBitmap = LoadBitmapW(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDB_TOOLBAR));
    ImageList_AddMasked(m_hToolBarImageList,
                        hToolBarBitmap,
                        RGB(0,0,0));
    DeleteObject(hToolBarBitmap);

    LoadSnapinCache();
    LoadRecentFiles();
}

CMainWnd::~CMainWnd()
{
    DestroyMenu(m_hMenuConsoleSmall);
    DestroyMenu(m_hMenuConsoleLarge);
    ImageList_Destroy(m_hSnapinImageList);
    ImageList_Destroy(m_hToolBarImageList);
    ::CoUninitialize();
}

LRESULT
CMainWnd::OnCreate(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CLIENTCREATESTRUCT ccs;
    LPCTSTR lpFileName = (LPCTSTR)(((LPCREATESTRUCT)lParam)->lpCreateParams);
    RECT rect;

    m_bStandardMenusVisible = TRUE;

    /* Create and initialize the Toolbar */
    m_bToolBarVisible = TRUE;
    m_ToolBar.Create(m_hWnd,
                     WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | TBSTYLE_FLAT,
                     0);

    m_ToolBar.SendMessageW(TB_SETBITMAPSIZE, 0, MAKELONG(16, 16));
    m_ToolBar.SendMessageW(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    m_ToolBar.SetImageList(m_hToolBarImageList);
    m_ToolBar.SendMessageW(TB_ADDBUTTONSW, _countof(TbButtons), (LPARAM)TbButtons);

    m_ToolBar.GetClientRect(&rect);
    m_iToolBarHeight = rect.bottom - rect.top;

    GetClientRect(&rect);
    rect.top += m_iToolBarHeight;

    /* Create the MDI client window */
    ccs.hWindowMenu = GetSubMenu(m_hMenuConsoleLarge, 4);
    ccs.idFirstChild = IDM_MDI_FIRSTCHILD;

    m_MDIClient.Create(L"MDICLIENT", m_hWnd, rect, (LPCTSTR)NULL, WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL, WS_EX_CLIENTEDGE, 0U, &ccs);
    m_MDIClient.ShowWindow(SW_SHOW);
    m_MDIClient.UpdateWindow();

    if (lpFileName == NULL)
    {
        PostMessage(WM_COMMAND, IDM_FILE_NEW, NULL);
    }
    return 0;
}

LRESULT
CMainWnd::OnSize(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UpdateLayout();
    return 0;
}

LRESULT
CMainWnd::OnCloseChild(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UpdateTitle();
    UpdateMenu();
    return 0;
}

LRESULT
CMainWnd::OnDestroy(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PostQuitMessage(0);
    SetMenu(NULL);
    return 0;
}

LRESULT
CMainWnd::OnClose(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DestroyWindow();
    return 0;
}

LRESULT
CMainWnd::OnFileNew(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    MDICREATESTRUCT mcs;
    HWND hChild;
    CAtlString rootName(MAKEINTRESOURCE(IDS_CONSOLEROOT));
//    CAtlString nodeGuid(L"{C96401CC-0E17-11D3-885B-00C04F72C717}");

    /* Close all views */
    POSITION pos = m_ViewList.GetHeadPosition();
    while (pos)
    {
        CConsoleWnd *console = (CConsoleWnd*)m_ViewList.GetNext(pos);
        console->SendMessage(WM_CLOSE, 0, 0);
    }

    /* Delete the snapin tree */
    delete m_RootNode;
    m_RootNode = NULL;

    /* Create a new snapin root node */
    CSnapinCacheEntry *CacheEntry = GetSnapinCacheEntryByGuid((PWSTR)L"{C96401CC-0E17-11D3-885B-00C04F72C717}");
    if (!CacheEntry)
    {
        DPRINT1("No folder cache entry!\n");
        return 0;
    }

    m_RootNode = new CSnapin(CacheEntry, rootName.GetString());
    if (!CacheEntry)
    {
        DPRINT1("No root folder!\n");
        return 0;
    }

    /* Create a new view */
    mcs.szTitle = rootName.GetString();
    mcs.szClass = CConsoleWnd::GetWndClassName();
    mcs.hOwner = _AtlBaseModule.GetModuleInstance();
    mcs.x = mcs.cx = CW_USEDEFAULT;
    mcs.y = mcs.cy = CW_USEDEFAULT;
    mcs.style = MDIS_ALLCHILDSTYLES;
    BOOL bMaximized = FALSE;
    HWND hWndOld = (HWND)m_MDIClient.SendMessage(WM_MDIGETACTIVE, 0, (LPARAM)&bMaximized);
    mcs.lParam = bMaximized || !hWndOld;
    /* This object registers itself in the _AtlWinModule to be assigned to the next window created */
    CConsoleWnd* child = new CConsoleWnd(this, m_RootNode);
    /* Ask for a new MDI Child window */
    hChild = (HWND)m_MDIClient.SendMessage(WM_MDICREATE, 0, (LONG_PTR)&mcs);
    if (hChild)
    {
        m_ConsoleNumber++;
    }
    else
    {
        delete child;
        delete m_RootNode;
        m_RootNode = NULL;
    }

    UpdateTitle();
    UpdateMenu();

    return 1;
}

LRESULT
CMainWnd::OnFileOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    OPENFILENAME openas;
    WCHAR szPath[MAX_PATH];

    CConsoleWnd* child = GetActiveChildInfo();
    if (child == NULL)
        return 0;

    ZeroMemory(&openas, sizeof(openas));
    wcscpy(szPath, L"");

    openas.lStructSize = sizeof(OPENFILENAME);
    openas.hwndOwner = m_hWnd;
    openas.hInstance = _AtlBaseModule.GetModuleInstance();
    openas.lpstrFilter = L"MSC Files (*.msc)\0*.msc\0All Files (*.*)\0*.*\0";
    openas.lpstrFile = szPath;
    openas.nMaxFile = MAX_PATH;
    openas.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    openas.lpstrDefExt = L"msc";

    if (GetOpenFileNameW(&openas))
    {
        m_Filename = szPath;
        LRESULT ret = LoadMscFile(m_Filename);
        if (ret == ERROR_SUCCESS)
            AddToRecentFiles(m_Filename);
    }

    return 0;
}

LRESULT
CMainWnd::OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CConsoleWnd* child = GetActiveChildInfo();
    if (child == NULL)
        return 0;

    if (m_Filename.IsEmpty())
        return OnFileSaveAs(wNotifyCode, wID, hWndCtl, bHandled);

    return SaveMscFile(m_Filename);
}

LRESULT
CMainWnd::OnFileSaveAs(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    OPENFILENAME saveas;
    WCHAR szPath[MAX_PATH];

    CConsoleWnd* child = GetActiveChildInfo();
    if (child == NULL)
        return 0;

    ZeroMemory(&saveas, sizeof(saveas));
    if (!m_Filename.IsEmpty())
    {
        wcscpy(szPath, m_Filename);
    }
    else
    {
        child->GetWindowText(szPath, MAX_PATH);
        CreateNewFilename(szPath, MAX_PATH, (DWORD)m_ConsoleNumber);
        wcscat(szPath, L".msc");
    }

    saveas.lStructSize = sizeof(OPENFILENAME);
    saveas.hwndOwner = m_hWnd;
    saveas.hInstance = _AtlBaseModule.GetModuleInstance();
    saveas.lpstrFilter = L"MSC Files (*.msc)\0*.msc\0All Files (*.*)\0*.*\0";
    saveas.lpstrFile = szPath;
    saveas.nMaxFile = MAX_PATH;
    saveas.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    saveas.lpstrDefExt = L"msc";

    if (GetSaveFileNameW(&saveas))
    {
        m_Filename = szPath;
        LRESULT ret = SaveMscFile(m_Filename);
        if (ret == ERROR_SUCCESS)
            AddToRecentFiles(m_Filename);
        return ret;
    }

    return 0;
}

LRESULT
CMainWnd::OnFileAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CConsoleWnd* child = GetActiveChildInfo();
    if (child == NULL)
        return 0;
    CAddDialog dlg(this, child);
    dlg.DoModal(m_hWnd, (LPARAM)child);
    return 0;
}

LRESULT
CMainWnd::OnFileOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CConsoleWnd* child = GetActiveChildInfo();
    if (child == NULL)
        return 0;
    COptionsDialog dlg(this, child);
    dlg.DoModal(m_hWnd, (LPARAM)child);
    return 0;
}

LRESULT
CMainWnd::OnFileExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    PostMessage(WM_CLOSE, 0, 0);
    return 0;
}

LRESULT
CMainWnd::OnViewCustomize(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CConsoleWnd* child = GetActiveChildInfo();
    if (child == NULL)
        return 0;

    CCustomizeDialog dlg2(this, child);
    dlg2.DoModal(m_hWnd, (LPARAM)child);
    return 0;
}

LRESULT
CMainWnd::OnWindowsNew(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    MDICREATESTRUCT mcs;
    HWND hChild;

    mcs.szTitle = m_RootNode->DisplayName().GetString();
    mcs.szClass = CConsoleWnd::GetWndClassName();
    mcs.hOwner = _AtlBaseModule.GetModuleInstance();
    mcs.x = mcs.cx = CW_USEDEFAULT;
    mcs.y = mcs.cy = CW_USEDEFAULT;
    mcs.style = MDIS_ALLCHILDSTYLES;
    BOOL bMaximized = FALSE;
    HWND hWndOld = (HWND)m_MDIClient.SendMessage(WM_MDIGETACTIVE, 0, (LPARAM)&bMaximized);
    mcs.lParam = bMaximized || !hWndOld;
    /* This object registers itself in the _AtlWinModule to be assigned to the next window created */
    CConsoleWnd* child = new CConsoleWnd(this, m_RootNode);
    /* Ask for a new MDI Child window */
    hChild = (HWND)m_MDIClient.SendMessage(WM_MDICREATE, 0, (LONG_PTR)&mcs);
    if (!hChild)
    {
        delete child;
    }

    UpdateMenu();
    return 0;
}

LRESULT
CMainWnd::OnWindowsCascade(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_MDIClient.SendMessage(WM_MDICASCADE, 0, 0);
    return 0;
}

LRESULT
CMainWnd::OnWindowsTile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_MDIClient.SendMessage(WM_MDITILE, MDITILE_HORIZONTAL, 0);
    return 0;
}

LRESULT
CMainWnd::OnWindowsArrange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_MDIClient.SendMessage(WM_MDIICONARRANGE, 0, 0);
    return 0;
}

LRESULT
CMainWnd::OnHelpAboutMMC(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HICON hIcon = LoadIcon(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDI_MAINAPP));
    CAtlString AboutTitle(MAKEINTRESOURCE(IDS_ABOUTTITLE));
    CAtlString AppTitle(MAKEINTRESOURCE(IDS_APPTITLE));
    CAtlString TitleString;

    TitleString = AboutTitle;
    TitleString += L"#";
    TitleString += AppTitle;

    ::ShellAboutW(this->m_hWnd, (LPWSTR)TitleString.GetString(), NULL, hIcon);

    return 0;
}

LRESULT
CMainWnd::OnToolbarScopePane(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CConsoleWnd* child = GetActiveChildInfo();
    if (child == NULL)
        return 0;

    child->SetTreeViewVisible(!child->IsTreeViewVisible());
    return 0;
}

LRESULT
CMainWnd::OnToolbarActionsPane(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CConsoleWnd* child = GetActiveChildInfo();
    if (child == NULL)
        return 0;

    child->SetActionsPaneVisible(!child->IsActionsPaneVisible());
    return 0;
}

LRESULT
CMainWnd::OnMDIForward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HWND hChild = (HWND)m_MDIClient.SendMessage(WM_MDIGETACTIVE, 0, 0);
    if (hChild)
    {
        WPARAM wParam = MAKEWPARAM(wNotifyCode, wID);
        LPARAM lParam = (LPARAM)hWndCtl;
        ::SendMessage(hChild, WM_COMMAND, wParam, lParam);
    }
    return 0;
}

CAtlString *CMainWnd::GetConsoleTitle()
{
    return &m_ConsoleTitle;
}

void CMainWnd::SetConsoleTitle(CAtlString consoleTitle)
{
    m_ConsoleTitle = consoleTitle;
    SetWindowTextW(consoleTitle.GetString());
}

BOOL
CMainWnd::IsToolBarVisible()
{
    return m_bToolBarVisible;
}

VOID
CMainWnd::SetToolBarVisible(BOOL bVisible)
{
    m_bToolBarVisible = bVisible;
    UpdateLayout();
}

BOOL
CMainWnd::AreStandardMenusVisible()
{
    return m_bStandardMenusVisible;
}

VOID
CMainWnd::SetStandardMenusVisible(BOOL bVisible)
{
    m_bStandardMenusVisible = bVisible;
//        m_MainWnd->UpdateMenu(m_bStandardMenusVisible);
//    UpdateLayout();
}

LRESULT
CMainWnd::LoadSnapinCache()
{
    CRegKey SnapinsKey;
    if (ERROR_SUCCESS == SnapinsKey.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\MMC\\SnapIns", KEY_READ))
    {
        WCHAR namebuf[MAX_PATH];
        DWORD index = 0, namelen = _countof(namebuf);
        while (ERROR_SUCCESS == SnapinsKey.EnumKey(index++, namebuf, &namelen))
        {
            DPRINT1("%S\n", namebuf);
            CSnapinCacheEntry* snapin = CSnapinCacheEntry::Create(SnapinsKey, namebuf, m_hSnapinImageList);
            DPRINT1("CacheEntry %p\n", snapin);
            if (snapin)
            {
                m_SnapinCache.Add(snapin);
            }
            namelen = _countof(namebuf);
        }
        SnapinsKey.Close();
    }
    return S_OK;
}

int
CMainWnd::GetSnapinCacheCount()
{
    return m_SnapinCache.GetSize();
}

CSnapinCacheEntry *
CMainWnd::GetSnapinCacheEntry(int nIndex)
{
    return m_SnapinCache[nIndex];
}

CSnapinCacheEntry *
CMainWnd::GetSnapinCacheEntryByGuid(PWSTR pszGuid)
{
    for (int i = 0; i < m_SnapinCache.GetSize(); i++)
    {
        CSnapinCacheEntry *entry = m_SnapinCache[i];
        if (entry->GuidString().CompareNoCase(pszGuid) == 0)
            return entry;
    }
    return NULL;
}

HIMAGELIST
CMainWnd::SnapinImageList()
{
    return m_hSnapinImageList;
}

int
CMainWnd::RegisterView(CConsoleWnd *pView)
{
    m_ViewList.AddTail(pView);
    return m_NextViewId++;
}

void
CMainWnd::UnregisterView(CConsoleWnd *pView)
{
    POSITION pos = m_ViewList.Find(pView);
    if (pos)
        m_ViewList.RemoveAt(pos);
}

void
CMainWnd::UpdateViews()
{
    CConsoleWnd *console;
    POSITION pos;

    pos = m_ViewList.GetHeadPosition();
    while (pos)
    {
        console = (CConsoleWnd*)m_ViewList.GetNext(pos);
        if (console)
            console->UpdateView();

    }
}

void
CMainWnd::UpdateLayout()
{
    RECT rcClient, rcToolBar = {0, 0, 0, 0};
    GetClientRect(&rcClient);

    int nToolBarFlags = SWP_NOZORDER;
    if (m_bToolBarVisible)
    {
        rcClient.top += m_iToolBarHeight;
        rcClient.bottom -= m_iToolBarHeight;

        rcToolBar.right = rcClient.right;
        rcToolBar.bottom = m_iToolBarHeight;
        nToolBarFlags |= SWP_SHOWWINDOW;
    }
    else
    {
        nToolBarFlags |= SWP_HIDEWINDOW;
    }

    m_ToolBar.SetWindowPos(NULL, rcToolBar.left, rcToolBar.top, rcToolBar.right, rcToolBar.bottom, nToolBarFlags);

    m_MDIClient.SetWindowPos(NULL, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, SWP_NOZORDER);
}

DOCUMENT_MODE
CMainWnd::GetDocumentMode()
{
    return m_DocumentMode;
}

void
CMainWnd::SetDocumentMode(DOCUMENT_MODE DocumentMode)
{
    m_DocumentMode = DocumentMode;
}

BOOL
CMainWnd::GetLogicalReadOnly()
{
    return m_LogicalReadOnly;
}

void
CMainWnd::SetLogicalReadOnly(BOOL LogicalReadOnly)
{
    m_LogicalReadOnly = LogicalReadOnly;
}

BOOL
CMainWnd::GetPreventViewCustomization()
{
    return m_PreventViewCustomization;
}

void
CMainWnd::SetPreventViewCustomization(BOOL PreventCustomization)
{
    m_PreventViewCustomization = PreventCustomization;
}

VOID
CMainWnd::UpdateRecentFilesMenu()
{
    MENUITEMINFOW mi;
    HMENU hMenu = GetMenu();

    mi.cbSize = sizeof(MENUITEMINFOW);
    mi.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
    mi.fType = MFT_STRING;
    mi.wID = IDM_FILE_RECENT1;

    RemoveMenu(hMenu, IDM_FILE_RECENT_SEP, MF_BYCOMMAND);

    INT i = 0;
    CAtlString ValueName;
    POSITION pos = m_RecentFilesList.GetHeadPosition();
    while (pos != NULL)
    {
        RemoveMenu(hMenu, IDM_FILE_RECENT1 + i, MF_BYCOMMAND);

        CAtlString ValueData = m_RecentFilesList.GetNext(pos);

        mi.dwTypeData = ValueData.GetString();

        InsertMenuItemW(hMenu, IDM_FILE_EXIT, FALSE, &mi);
        mi.wID++;

        i++;
        if (i >= 4)
            break;
    }

    mi.fType = MFT_SEPARATOR;
    mi.fMask = MIIM_FTYPE | MIIM_ID;
    InsertMenuItemW(hMenu, IDM_FILE_EXIT, FALSE, &mi);
}

VOID
CMainWnd::LoadRecentFiles()
{
    CRegKey RecentFilesKey;
    if (ERROR_SUCCESS == RecentFilesKey.Open(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\MMC_NEW\\Recent Files List", KEY_READ))
    {
        WCHAR pathBuf[MAX_PATH];
        ULONG pathLength, i;
        DWORD err;

        CAtlString valueName;
        CAtlString fileName;

        for (i = 0; i < 4; i++)
        {
            valueName.Format(L"File%u", i + 1);
            pathLength = _countof(pathBuf);
            err = RecentFilesKey.QueryStringValue(valueName.GetString(),
                                                  pathBuf,
                                                  &pathLength);
            if (err == ERROR_SUCCESS)
            {
                CAtlString fileName(pathBuf);
                m_RecentFilesList.AddTail(fileName);
            }
        }

        RecentFilesKey.Close();
    }

    UpdateRecentFilesMenu();
}

VOID
CMainWnd::AddToRecentFiles(CAtlString &FileName)
{
    POSITION pos = m_RecentFilesList.Find(FileName);
    if (pos == NULL)
    {
        /* Insert at top */
        m_RecentFilesList.AddHead(FileName);
        if (m_RecentFilesList.GetCount() > 4)
            m_RecentFilesList.RemoveTail();
    }
    else
    {
        /* Move to top */
        /* m_RecentFilesList.MoveToHead(pos); */
        CAtlString str = m_RecentFilesList.GetAt(pos);
        m_RecentFilesList.RemoveAt(pos);
        m_RecentFilesList.AddHead(str);
    }

    /* Update the registry key */
    CRegKey RecentFilesKey;
    if (ERROR_SUCCESS == RecentFilesKey.Create(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\MMC_NEW\\Recent Files List"))
    {
        INT count = 1;
        CAtlString ValueName;
        pos = m_RecentFilesList.GetHeadPosition();
        while (pos != NULL)
        {
            CAtlString ValueData = m_RecentFilesList.GetNext(pos);
            ValueName.Format(L"File%u", count);
            RecentFilesKey.SetStringValue(ValueName, ValueData);
            count++;
            if (count >= 5)
                break;
        }

        RecentFilesKey.Close();
    }

    UpdateRecentFilesMenu();
}

DWORD
CMainWnd::CreateNewFilename(PWSTR pBuffer, DWORD dwSize, DWORD Number)
{
    DWORD_PTR args[1] = { (DWORD_PTR)Number };
    CAtlString str;

    str.LoadString(IDS_CONSOLETITLE);

    return FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            str.GetString(), 0, 0, pBuffer, dwSize, (va_list*)args);
}

LPWSTR
CMainWnd::ProgramModeToString()
{
    switch (m_DocumentMode)
    {
        case DocumentMode_Author:
            return (LPWSTR)L"Author";

        case DocumentMode_User:
            return (LPWSTR)L"User";

        case DocumentMode_UserMDI:
            return (LPWSTR)L"UserMDI";

        case DocumentMode_UserSDI:
            return (LPWSTR)L"UserSDI";

        default:
            return (LPWSTR)L"";
    }
}

LRESULT
CMainWnd::SaveMscFile(CAtlString &FileName)
{
    IXMLDOMElement *pRootNode = NULL;
    IXMLDOMElement *pNode = NULL;
    IXMLDOMElement *pFrameStateNode = NULL;
    IXMLDOMElement *pViewsNode = NULL;
    IXMLDOMElement *pScopeTreeNode = NULL;
    IXMLDOMElement *pSnapinCacheNode = NULL;
    IXMLDOMElement *pSnapinNode = NULL;
    IXMLDOMElement *pNodesNode = NULL;
    POSITION pos;
    CConsoleWnd* console;
    HRESULT hr = S_OK;

    MscFile *mscFile = new MscFile(FileName.GetString());

    CHK_HR(mscFile->CreateAndInitDOM());

    /* <?xml version="1.0"?> */
    CHK_HR(mscFile->CreateAndAddPINode(L"xml", L"version='1.0'"));

    /* <MMC_ConsoleFile ConsoleVersion="2.0" ProgramMode="Author"> */
    CHK_HR(mscFile->CreateElement(L"MMC_ConsoleFile", &pRootNode));
    CHK_HR(mscFile->CreateAndAddAttributeNode(L"ConsoleVersion", L"2.0", pRootNode));
    CHK_HR(mscFile->CreateAndAddAttributeNode(L"ProgramMode", ProgramModeToString(), pRootNode));

    /* <ConsoleFileID> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"ConsoleFileID", pRootNode, &pNode));
    CHK_HR(mscFile->CreateAndAddTextNode(L"{Random GUID}", pNode));
    SAFE_RELEASE(pNode); /* </ConsoleFileID> */

    /* <FrameState> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"FrameState", pRootNode, &pFrameStateNode));
    CHK_HR(mscFile->CreateAndAddAttributeNode(L"ShowStatusBar", L"true", pFrameStateNode));
    if ((m_DocumentMode != DocumentMode_Author) && (m_LogicalReadOnly))
        CHK_HR(mscFile->CreateAndAddAttributeNode(L"LogicalReadOnly", L"true", pFrameStateNode));
    if ((m_DocumentMode != DocumentMode_Author) && (m_PreventViewCustomization))
        CHK_HR(mscFile->CreateAndAddAttributeNode(L"PreventViewCustomization", L"true", pFrameStateNode));

    mscFile->SaveWindowPlacement(this, pFrameStateNode);

    SAFE_RELEASE(pFrameStateNode); /* </FrameState> */

    /* <Views> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"Views", pRootNode, &pViewsNode));
    pos = m_ViewList.GetHeadPosition();
    do
    {
        console = (CConsoleWnd*)m_ViewList.GetNext(pos);
        if (console)
            console->SaveView(mscFile, pViewsNode);

    } while (pos != NULL);
    SAFE_RELEASE(pViewsNode); /* </Views> */

    /* <ScopeTree> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"ScopeTree", pRootNode, &pScopeTreeNode));

    /* <SnapinCache> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"SnapinCache", pScopeTreeNode, &pSnapinCacheNode));

    for (int i = 0; i < GetSnapinCacheCount(); i++)
    {
        CSnapinCacheEntry *entry = GetSnapinCacheEntry(i);
        if (entry)
        {
            /* <Snapin> */
            CHK_HR(mscFile->CreateAndAddElementNode(L"Snapin", pSnapinCacheNode, &pSnapinNode));
            CHK_HR(mscFile->CreateAndAddAttributeNode(L"CLSID", entry->GuidString().GetString(), pSnapinNode));
            CHK_HR(mscFile->CreateAndAddAttributeNode(L"AllExtensionsEnabled", L"true", pSnapinNode)); /* FIXME */
            CHK_HR(mscFile->CreateAndAddAttributeNode(L"Name", entry->Name().GetString(), pSnapinNode));
            CHK_HR(mscFile->CreateAndAddAttributeNode(L"Provider", entry->Provider().GetString(), pSnapinNode));
            SAFE_RELEASE(pSnapinNode); /* </Snapin> */
        }
    }
    SAFE_RELEASE(pSnapinCacheNode); /* </SnapinCache> */

    /* <Nodes> */
    CHK_HR(mscFile->CreateAndAddElementNode(L"Nodes", pScopeTreeNode, &pNodesNode));
    if (m_RootNode)
        m_RootNode->SaveNode(mscFile, pNodesNode);
    SAFE_RELEASE(pNodesNode); /* </Nodes> */

    SAFE_RELEASE(pScopeTreeNode); /* </ScopeTree> */

    CHK_HR(mscFile->AppendChildToParent(pRootNode));

    CHK_HR(mscFile->SaveDOM());

CleanUp:
    SAFE_RELEASE(pRootNode); /* </MMC_ConsoleFile> */

    delete mscFile;

    return 0;
}

LRESULT
CMainWnd::LoadMscFile(CAtlString &FileName)
{
    HRESULT hr = S_OK;

    MscFile *mscFile = new MscFile(FileName.GetString());

    CHK_HR(mscFile->CreateAndInitDOM());

    CHK_HR(mscFile->LoadDOM());

    /* FIXME: Parse the dom and set up the app, the views and the snapin tree */

CleanUp:

    delete mscFile;

    return 0;
}

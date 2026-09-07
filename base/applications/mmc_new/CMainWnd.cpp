/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main window class
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller
 *              Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

CMainWnd::CMainWnd()
    : m_NewConsoleCount(0)
    , m_nConsoleCount(0)
    , m_AppAuthorMode(false)
    , m_hSnapinImageList(NULL)
    , m_NextViewId(1)
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
    LoadSnapinCache();
}

CMainWnd::~CMainWnd()
{
    DestroyMenu(m_hMenuConsoleSmall);
    DestroyMenu(m_hMenuConsoleLarge);
    ImageList_Destroy(m_hSnapinImageList);
    ::CoUninitialize();
}

LRESULT
CMainWnd::OnCreate(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CLIENTCREATESTRUCT ccs;
    LPCTSTR lpFileName = (LPCTSTR)(((LPCREATESTRUCT)lParam)->lpCreateParams);
    m_AppAuthorMode = TRUE;
    UpdateMenu();
    SetWindowText(TEXT("ReactOS Management Console"));
    ccs.hWindowMenu = GetSubMenu(m_hMenuConsoleLarge, 1);
    ccs.idFirstChild = IDM_MDI_FIRSTCHILD;
    RECT rect;
    GetClientRect(&rect);
    /* Create the MDI client window */
    m_MDIClient.Create(L"MDICLIENT", m_hWnd, rect, (LPCTSTR)NULL, WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL, WS_EX_CLIENTEDGE, 0U, &ccs);
    //hwndMDIClient = CreateWindowEx(WS_EX_CLIENTEDGE, L"MDICLIENT", (LPCTSTR)NULL,
    //    ,
    //    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
    //    m_hWnd, NULL, _AtlBaseModule.GetModuleInstance(), (LPVOID)&ccs);
    m_MDIClient.ShowWindow(SW_SHOW);
    m_MDIClient.UpdateWindow();
    if (lpFileName == NULL)
    {
        PostMessage(WM_COMMAND, IDM_FILE_NEW, NULL);
    }
    return 0;
}

LRESULT
CMainWnd::OnNewMDIChild(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    MDICREATESTRUCT mcs;
    HWND hChild;
    CAtlString title;
    CreateNewConsoleTitle(title);
    mcs.szTitle = title.GetString();
    mcs.szClass = CConsoleWnd::GetWndClassName();
    mcs.hOwner = _AtlBaseModule.GetModuleInstance();
    mcs.x = mcs.cx = CW_USEDEFAULT;
    mcs.y = mcs.cy = CW_USEDEFAULT;
    mcs.style = MDIS_ALLCHILDSTYLES;
    BOOL bMaximized = FALSE;
    HWND hWndOld = (HWND)m_MDIClient.SendMessage(WM_MDIGETACTIVE, 0, (LPARAM)&bMaximized);
    mcs.lParam = bMaximized || !hWndOld;
    /* This object registers itself in the _AtlWinModule to be assigned to the next window created */
    CConsoleWnd* child = new CConsoleWnd(this);
    /* Ask for a new MDI Child window */
    hChild = (HWND)m_MDIClient.SendMessage(WM_MDICREATE, 0, (LONG_PTR)&mcs);
    if (hChild)
    {
        m_nConsoleCount++;
    }
    else
    {
        delete child;
    }
    UpdateMenu();
    return 1;
}

LRESULT
CMainWnd::OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CConsoleWnd* child = GetActiveChildInfo();
    if (child == NULL)
        return 0;
    if (child->m_Filename.IsEmpty())
        return OnFileSaveAs(wNotifyCode, wID, hWndCtl, bHandled);
    // save, and if fails clear pFileName
    return 0;
}

LRESULT
CMainWnd::OnFileSaveAs(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    OPENFILENAME saveas;
    TCHAR szPath[MAX_PATH];
    CConsoleWnd* child = GetActiveChildInfo();
    if (child == NULL)
        return 0;
    ZeroMemory(&saveas, sizeof(saveas));
    if (!child->m_Filename.IsEmpty())
    {
        _tcscpy(szPath, child->m_Filename);
    }
    else
    {
        child->GetWindowText(szPath, MAX_PATH);
        _tcscat(szPath, TEXT(".msc"));
    }
    saveas.lStructSize = sizeof(OPENFILENAME);
    saveas.hwndOwner = m_hWnd;
    saveas.hInstance = _AtlBaseModule.GetModuleInstance();
    saveas.lpstrFilter = L"MSC Files\0*.msc\0";
    saveas.lpstrFile = szPath;
    saveas.nMaxFile = MAX_PATH;
    saveas.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    saveas.lpstrDefExt = L"msc";
    if (GetSaveFileName(&saveas))
    {
        child->m_Filename = szPath;
        return OnFileSave(wNotifyCode, wID, hWndCtl, bHandled);
    }
    else
    {
        return 0;
    }
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
CMainWnd::OnFileExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    PostMessage(WM_CLOSE, 0, 0);
    return 0;
}

LRESULT
CMainWnd::OnHelpAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
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

LRESULT
CMainWnd::OnSize(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rcClient;
    GetClientRect(&rcClient);
    m_MDIClient.SetWindowPos(NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);
    return 0;
}

LRESULT
CMainWnd::OnCloseChild(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_nConsoleCount--;
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
    do
    {
        console = (CConsoleWnd*)m_ViewList.GetNext(pos);
        if (console)
            console->UpdateView();

    } while (pos != NULL);   
}

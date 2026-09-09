/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main window class
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller
 *              Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#pragma once

class CMainWnd :
    public CWindowImpl<CMainWnd>
{
private:
    CWndProcThunk m_FrameThunk;

    int m_NewConsoleCount;
    int m_nConsoleCount;
    HMENU m_hMenuConsoleSmall;
    HMENU m_hMenuConsoleLarge;
    bool m_AppAuthorMode;

    CToolbar<DWORD_PTR> m_ToolBar;
    int m_iToolBarHeight;
    BOOL m_ToolBarVisible;
    HIMAGELIST m_hToolBarImageList;

    CSimpleArray<CSnapinCacheEntry*> m_SnapinCache;
    HIMAGELIST m_hSnapinImageList;

    CAtlList<CConsoleWnd*> m_ViewList;
    int m_NextViewId;

public:
    CWindow m_MDIClient;

public:

    BEGIN_MSG_MAP(CMainWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_USER_CLOSE_CHILD, OnCloseChild)

        COMMAND_ID_HANDLER(IDM_FILE_NEW, OnNewMDIChild)
        COMMAND_ID_HANDLER(IDM_FILE_SAVE, OnFileSave)
        COMMAND_ID_HANDLER(IDM_FILE_SAVEAS, OnFileSaveAs)
        COMMAND_ID_HANDLER(IDM_FILE_ADD, OnFileAdd)
        COMMAND_ID_HANDLER(IDM_FILE_EXIT, OnFileExit)
        COMMAND_ID_HANDLER(IDM_WINDOWS_CASCADE, OnWindowsCascade)
        COMMAND_ID_HANDLER(IDM_WINDOWS_TILE, OnWindowsTile)
        COMMAND_ID_HANDLER(IDM_WINDOWS_ARRANGE, OnWindowsArrange)
        COMMAND_ID_HANDLER(IDM_HELP_ABOUT, OnHelpAbout)
        COMMAND_RANGE_HANDLER(0, IDM_MDI_FIRSTCHILD - 1, OnMDIForward)
    END_MSG_MAP()

    static CWndClassInfo& GetWndClassInfo()
    {
        static CWndClassInfo wc =
        {
            {
                /* cbSize= */sizeof(WNDCLASSEX),
                /* style= */0,
                /* lpfnWndProc= */StartWindowProc,
                /* cbClsExtra= */0,
                /* cbWndExtra= */0,
                /* hInstance= */NULL,
                /* hIcon= */LoadIcon(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDI_MAINAPP)),
                /* hCursor= */NULL,
                /* hbrBackground= */(HBRUSH)(COLOR_BTNFACE + 1),
                /* lpszMenuName= */NULL,
                /* lpszClassName= */TEXT("MMCMainFrame"),
                /* hIconSm= */LoadIcon(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDI_MAINAPP))
            },
            NULL, NULL, IDC_ARROW, TRUE, 0, L""
        };
        return wc;
    }

    static LPCTSTR GetWndClassName()
    {
        return GetWndClassInfo().m_wc.lpszClassName;
    }

    static LRESULT CALLBACK XDefFrameProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CMainWnd* pThis = reinterpret_cast<CMainWnd*>(hWnd);
        return ::DefFrameProc(pThis->m_hWnd, pThis->m_MDIClient.m_hWnd, uMsg, wParam, lParam);
    }

private:

    void UpdateMenu()
    {
        if (m_nConsoleCount == 0)
            SetMenu(m_hMenuConsoleSmall);
        else
            SetMenu(m_hMenuConsoleLarge);
    }

    CConsoleWnd* GetActiveChildInfo()
    {
        HWND hWndMDIChild;

        hWndMDIChild = (HWND)m_MDIClient.SendMessage(WM_MDIGETACTIVE, 0, 0);
        if (hWndMDIChild == NULL)
            return NULL;

        return (CConsoleWnd*)::GetWindowLongPtr(hWndMDIChild, 0);
    }

    void CreateNewConsoleTitle(CAtlString& str)
    {
        DWORD_PTR args[1] = { (DWORD_PTR)(++m_NewConsoleCount) };
        str.LoadString(IDS_CONSOLETITLE);

        LPTSTR lpTarget = NULL;
        DWORD Ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
            str.GetString(), 0, 0, (LPTSTR)&lpTarget, 0, (va_list*)args);

        if (Ret)
        {
            str = lpTarget;
            LocalFree(lpTarget);
        }
    }


public:
    CMainWnd();
    ~CMainWnd();

    LRESULT OnCreate(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCloseChild(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnNewMDIChild(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFileSaveAs(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFileAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFileExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnWindowsCascade(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnWindowsTile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnWindowsArrange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnHelpAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnMDIForward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
    LRESULT LoadSnapinCache();
    void UpdateLayout();
    void UpdateViews();

public:
    int GetSnapinCacheCount();
    CSnapinCacheEntry *GetSnapinCacheEntry(int nIndex);
    CSnapinCacheEntry *GetSnapinCacheEntryByGuid(PWSTR pszGuid);
    HIMAGELIST SnapinImageList();
    int RegisterView(CConsoleWnd *pView);
    void UnregisterView(CConsoleWnd *pView);
};


/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main window class
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller
 *              Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#pragma once

/* Toolbar Button Indices */
#define BTN_UNDO            0
#define BTN_REDO            1
#define BTN_UP              2
#define BTN_SCOPE_PANE      3
#define BTN_EXPORT_LIST     4
#define BTN_HELP            5
#define BTN_ACTIONS_PANE    6

#define MAX_RECENT_FILES    4

class CMainWnd :
    public CWindowImpl<CMainWnd>
{
private:
    CWndProcThunk m_FrameThunk;

    int m_ConsoleNumber;
    CAtlString m_ConsoleTitle;
    HMENU m_hMenuConsoleSmall;
    HMENU m_hMenuConsoleLarge;
    DOCUMENT_MODE m_DocumentMode;
    BOOL m_LogicalReadOnly;
    BOOL m_PreventViewCustomization;

    CToolbar<DWORD_PTR> m_ToolBar;
    int m_iToolBarHeight;
    BOOL m_bToolBarVisible;
    HIMAGELIST m_hToolBarImageList;
    BOOL m_bStandardMenusVisible;

    CSimpleArray<CSnapinCacheEntry*> m_SnapinCache;
    HIMAGELIST m_hSnapinImageList;

    CAtlList<CConsoleWnd*> m_ViewList;
    int m_NextViewId;

    CSnapin *m_RootNode;

    CAtlList<CRecentFileEntry *> m_RecentFilesList;

public:
    CWindow m_MDIClient;
    CAtlString m_Filename;

public:

    BEGIN_MSG_MAP(CMainWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_USER_CLOSE_CHILD, OnCloseChild)

        COMMAND_ID_HANDLER(IDM_FILE_NEW, OnFileNew)
        COMMAND_ID_HANDLER(IDM_FILE_OPEN, OnFileOpen)
        COMMAND_ID_HANDLER(IDM_FILE_SAVE, OnFileSave)
        COMMAND_ID_HANDLER(IDM_FILE_SAVEAS, OnFileSaveAs)
        COMMAND_ID_HANDLER(IDM_FILE_ADD, OnFileAdd)
        COMMAND_ID_HANDLER(IDM_FILE_OPTIONS, OnFileOptions)
        COMMAND_ID_HANDLER(IDM_FILE_RECENT1, OnFileRecent)
        COMMAND_ID_HANDLER(IDM_FILE_RECENT2, OnFileRecent)
        COMMAND_ID_HANDLER(IDM_FILE_RECENT3, OnFileRecent)
        COMMAND_ID_HANDLER(IDM_FILE_RECENT4, OnFileRecent)
        COMMAND_ID_HANDLER(IDM_FILE_EXIT, OnFileExit)

        COMMAND_ID_HANDLER(IDM_VIEW_CUSTOMIZE, OnViewCustomize)

        COMMAND_ID_HANDLER(IDM_WINDOWS_NEW, OnWindowsNew)
        COMMAND_ID_HANDLER(IDM_WINDOWS_CASCADE, OnWindowsCascade)
        COMMAND_ID_HANDLER(IDM_WINDOWS_TILE, OnWindowsTile)
        COMMAND_ID_HANDLER(IDM_WINDOWS_ARRANGE, OnWindowsArrange)

        COMMAND_ID_HANDLER(IDM_HELP_ABOUT_MMC, OnHelpAboutMMC)

        COMMAND_ID_HANDLER(IDM_TB_SCOPE_PANE, OnToolbarScopePane)
        COMMAND_ID_HANDLER(IDM_TB_ACTIONS_PANE, OnToolbarActionsPane)

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
        if (m_ViewList.IsEmpty())
            SetMenu(m_hMenuConsoleSmall);
        else
            SetMenu(m_hMenuConsoleLarge);
        UpdateRecentFilesMenu();
    }

    void UpdateTitle()
    {
        if (m_ViewList.IsEmpty())
            m_ConsoleTitle.LoadString(IDS_APPTITLE);
        else
            CreateNewConsoleTitle(m_ConsoleTitle);
        SetWindowTextW(m_ConsoleTitle.GetString());
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
        DWORD_PTR args[1] = { (DWORD_PTR)(m_ConsoleNumber) };
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

    LRESULT OnFileNew(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFileOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFileSaveAs(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFileAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFileOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFileRecent(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFileExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnViewCustomize(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnWindowsNew(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnWindowsCascade(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnWindowsTile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnWindowsArrange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnHelpAboutMMC(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnToolbarScopePane(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnToolbarActionsPane(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnMDIForward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
    LRESULT LoadSnapinCache();
    void UpdateLayout();
    void UpdateViews();

    VOID UpdateRecentFilesMenu();
    VOID LoadRecentFiles();
    VOID AddToRecentFiles(const CAtlString &FileName);
    DWORD CreateNewFilename(PWSTR pBuffer, DWORD dwSize, DWORD Number);
    LPWSTR ProgramModeToString();
    LRESULT SaveMscFile(const CAtlString &FileName);
    LRESULT LoadMscFile(const CAtlString &FileName);

public:
    CAtlString *GetConsoleTitle();
    void SetConsoleTitle(CAtlString consoleTitle);
    BOOL IsToolBarVisible();
    VOID SetToolBarVisible(BOOL bVisible);
    BOOL AreStandardMenusVisible();
    VOID SetStandardMenusVisible(BOOL bVisible);

    int GetSnapinCacheCount();
    CSnapinCacheEntry *GetSnapinCacheEntry(int nIndex);
    CSnapinCacheEntry *GetSnapinCacheEntryByGuid(PWSTR pszGuid);
    HIMAGELIST SnapinImageList();
    int RegisterView(CConsoleWnd *pView);
    void UnregisterView(CConsoleWnd *pView);
    DOCUMENT_MODE GetDocumentMode();
    void SetDocumentMode(DOCUMENT_MODE DocumentMode);
    BOOL GetLogicalReadOnly();
    void SetLogicalReadOnly(BOOL LogicalReadOnly);
    BOOL GetPreventViewCustomization();
    void SetPreventViewCustomization(BOOL PreventCustomization);
};


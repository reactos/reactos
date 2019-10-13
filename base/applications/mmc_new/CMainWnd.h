/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main window class
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller
 *              Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
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
            NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
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
    CMainWnd()
        : m_NewConsoleCount(0)
        , m_nConsoleCount(0)
        , m_AppAuthorMode(false)
    {
        m_FrameThunk.Init(XDefFrameProc, this);
        m_pfnSuperWindowProc = m_FrameThunk.GetWNDPROC();

        m_hMenuConsoleSmall = LoadMenu(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDM_CONSOLE_SMALL));
        m_hMenuConsoleLarge = LoadMenu(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDM_CONSOLE_LARGE));
    }

    ~CMainWnd()
    {
        DestroyMenu(m_hMenuConsoleSmall);
        DestroyMenu(m_hMenuConsoleLarge);
    }


    LRESULT OnCreate(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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

    LRESULT OnNewMDIChild(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
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

    LRESULT OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        CConsoleWnd* child = GetActiveChildInfo();
        if (child == NULL)
            return 0;

        if (child->m_Filename.IsEmpty())
            return OnFileSaveAs(wNotifyCode, wID, hWndCtl, bHandled);

        // save, and if fails clear pFileName

        return 0;
    }


    LRESULT OnFileSaveAs(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
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

    LRESULT OnFileAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        CConsoleWnd* child = GetActiveChildInfo();
        if (child == NULL)
            return 0;

        CAddDialog dlg(child);
        dlg.DoModal(m_hWnd, (LPARAM)child);
        return 0;
    }

    LRESULT OnFileExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        PostMessage(WM_CLOSE, 0, 0);
        return 0;
    }

    LRESULT OnMDIForward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
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

    LRESULT OnSize(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        RECT rcClient;
        GetClientRect(&rcClient);
        m_MDIClient.SetWindowPos(NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);
        return 0;
    }

    LRESULT OnCloseChild(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        m_nConsoleCount--;
        UpdateMenu();

        return 0;
    }

    LRESULT OnDestroy(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        PostQuitMessage(0);
        SetMenu(NULL);

        return 0;
    }

    LRESULT OnClose(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        DestroyWindow();
        return 0;
    }
};


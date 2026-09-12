/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Single 'console' window
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller
 *              Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once

class CMainWnd;

class CConsoleWnd :
    public CWindowImpl<CConsoleWnd>,
    public IConsole2,
    public IConsoleNameSpace2
{
private:
    int m_ViewId;
    CMainWnd* m_MainWnd;
    CTreeView m_TreeView;
    CListView m_ListView;
    CStatusBar m_StatusBar;
    CWindow m_DescriptionBar;
    int m_iTreeViewWidth;
    int m_iActionsPaneWidth;
    int m_iSplitterWidth;
    int m_iSplitOffset;
    int m_iSplitSide;
    int m_iStatusBarHeight;
    BOOL m_bTreeViewVisible;
    BOOL m_bActionsPaneVisible;
    BOOL m_bStatusBarVisible;
    BOOL m_bDescriptionBarVisible;

public:
    CAtlString m_Filename;

public:

    BEGIN_MSG_MAP(CConsoleWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    END_MSG_MAP()


    static CWndClassInfo& GetWndClassInfo()
    {
        static CWndClassInfo wc =
        {
            {
                /* cbSize= */sizeof(WNDCLASSEX),
                /* style= */CS_HREDRAW | CS_VREDRAW,
                /* lpfnWndProc= */StartWindowProc,
                /* cbClsExtra= */0,
                /* cbWndExtra= */sizeof(CConsoleWnd*),
                /* hInstance= */NULL,
                /* hIcon= */LoadIcon(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDI_MAINAPP)),
                /* hCursor= */NULL,
                /* hbrBackground= */(HBRUSH)(COLOR_3DFACE + 1),
                /* lpszMenuName= */NULL,
                /* lpszClassName= */L"MMCChildFrm",
                /* hIconSm= */LoadIcon(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDI_MAINAPP))
            },
            NULL, NULL, IDC_SIZEWE, TRUE, 0, L""
        };
        return wc;
    }

    static LPCWSTR GetWndClassName()
    {
        return GetWndClassInfo().m_wc.lpszClassName;
    }

private:
    VOID UpdateLayout();

public:
    CConsoleWnd(CMainWnd *MainWnd);
    ~CConsoleWnd();

    LRESULT OnCreate(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDrawItem(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BOOL IsTreeViewVisible();
    VOID SetTreeViewVisible(BOOL bVisible);
    BOOL IsStatusBarVisible();
    VOID SetStatusBarVisible(BOOL bVisible);
    BOOL IsDescriptionBarVisible();
    VOID SetDescriptionBarVisible(BOOL bVisible);
    BOOL IsActionsPaneVisible();
    VOID SetActionsPaneVisible(BOOL bVisible);

    VOID UpdateView();

    // +IConsole
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP SetHeader(LPHEADERCTRL pHeader);
    STDMETHODIMP SetToolbar(LPTOOLBAR pToolbar);
    STDMETHODIMP QueryResultView(LPUNKNOWN *pUnknown);
    STDMETHODIMP QueryScopeImageList(LPIMAGELIST *ppImageList);
    STDMETHODIMP QueryResultImageList(LPIMAGELIST *ppImageList);
    STDMETHODIMP UpdateAllViews(LPDATAOBJECT lpDataObject, LPARAM data, LONG_PTR hint);
    STDMETHODIMP MessageBox(LPCWSTR lpszText, LPCWSTR lpszTitle, UINT fuStyle, int *piRetval);
    STDMETHODIMP QueryConsoleVerb(LPCONSOLEVERB *ppConsoleVerb);
    STDMETHODIMP SelectScopeItem(HSCOPEITEM hScopeItem);
    STDMETHODIMP GetMainWindow(HWND *phwnd);
    STDMETHODIMP NewWindow(HSCOPEITEM hScopeItem, ULONG lOptions);
    // -IConsole

    // +IConsole2
    STDMETHODIMP Expand(HSCOPEITEM hItem, BOOL bExpand);
    STDMETHODIMP IsTaskpadViewPreferred();
    STDMETHODIMP SetStatusText(LPOLESTR pszStatusText);
    // -IConsole2

    // +IConsoleNameSpace
    STDMETHODIMP InsertItem(LPSCOPEDATAITEM item);
    STDMETHODIMP DeleteItem(HSCOPEITEM hItem, LONG fDeleteThis);
    STDMETHODIMP SetItem(LPSCOPEDATAITEM item);
    STDMETHODIMP GetItem(LPSCOPEDATAITEM item);
    STDMETHODIMP GetChildItem(HSCOPEITEM item, HSCOPEITEM *pItemChild, LONG *plCookie);
    STDMETHODIMP GetNextItem(HSCOPEITEM item, HSCOPEITEM *pItemNext, LONG *plCookie);
    STDMETHODIMP GetParentItem(HSCOPEITEM item, HSCOPEITEM *pItemParent, LONG *plCookie);
    // -IConsoleNameSpace

    // +IConsoleNameSpace2
    STDMETHODIMP Expand(HSCOPEITEM hItem);
    STDMETHODIMP AddExtension(HSCOPEITEM hItem, LPCLSID lpClsid);
    // -IConsoleNameSpace2
};


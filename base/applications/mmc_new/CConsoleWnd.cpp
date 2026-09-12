/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Single 'console' window
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller
 *              Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

    CConsoleWnd::CConsoleWnd(CMainWnd *MainWnd)
    {
        m_MainWnd = MainWnd;
        m_pfnSuperWindowProc = DefMDIChildProc;

        if (!m_thunk.Init(NULL, NULL))
            return;
        _AtlWinModule.AddCreateWndData(&m_thunk.cd, this);
    }

    CConsoleWnd::~CConsoleWnd()
    {
    }

    LRESULT CConsoleWnd::OnCreate(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SetWindowLongPtr(0, (LONG_PTR)this);

        m_bStatusBarVisible = TRUE;
        m_bDescriptionBarVisible = FALSE;
        m_bTreeViewVisible = TRUE;
        m_bActionsPaneVisible = FALSE;

        m_iSplitterWidth = 4;
        m_iSplitSide = 0;

        RECT Rect;
        GetClientRect(&Rect);
        m_iTreeViewWidth = (Rect.right - Rect.left) / 4;
        m_iActionsPaneWidth = (Rect.right - Rect.left) / 4;

        m_StatusBar.Create(this->m_hWnd, NULL);
        m_StatusBar.GetClientRect(&Rect);
        m_iStatusBarHeight = Rect.bottom - Rect.top;

        Rect.right = 0;
        Rect.left = 0;
        Rect.top = 0;
        Rect.bottom = 0;
        m_DescriptionBar.Create(L"Static", this->m_hWnd, &Rect, L"Description",
                                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_OWNERDRAW,
                                WS_EX_STATICEDGE);
        m_DescriptionBar.SetWindowText(L"Test");

        m_TreeView.Create(this->m_hWnd,
                          WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_EDITLABELS);

        m_TreeView.SetImageList(m_MainWnd->SnapinImageList(), TVSIL_NORMAL);

        m_ListView.Create(this->m_hWnd, &Rect, NULL, WS_CHILD | WS_VISIBLE, WS_EX_CLIENTEDGE);

        m_ViewId = m_MainWnd->RegisterView(this);

        LPMDICREATESTRUCT mdicreate = reinterpret_cast<LPMDICREATESTRUCT>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
        if (mdicreate->lParam)
            PostMessage(WM_SYSCOMMAND, SC_MAXIMIZE, 0);

        return 0;
    }

    LRESULT CConsoleWnd::OnDestroy(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        m_MainWnd->UnregisterView(this);
        m_MainWnd->PostMessage(WM_USER_CLOSE_CHILD, 0, 0);
        return 0;
    }

    LRESULT CConsoleWnd::OnSize(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        UpdateLayout();
        return DefMDIChildProc(this->m_hWnd, WM_SIZE, wParam, lParam);
    }

    LRESULT CConsoleWnd::OnDrawItem(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
        WCHAR WindowText[256];

        HBRUSH hBrush = GetSysColorBrush(COLOR_3DDKSHADOW);
        FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, hBrush);

        ::SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_3DHILIGHT));

        ::GetWindowTextW(lpDrawItem->hwndItem, WindowText, ARRAYSIZE(WindowText));
        ::SetBkMode(lpDrawItem->hDC, TRANSPARENT);

        lpDrawItem->rcItem.left += 4;
        DrawTextW(lpDrawItem->hDC, (LPCWSTR)WindowText, -1, &lpDrawItem->rcItem,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_MODIFYSTRING | DT_END_ELLIPSIS | DT_NOPREFIX);

        return TRUE;
    }

    BOOL CConsoleWnd::IsTreeViewVisible()
    {
        return m_bTreeViewVisible;
    }

    VOID CConsoleWnd::SetTreeViewVisible(BOOL bVisible)
    {
        m_bTreeViewVisible = bVisible;
        UpdateLayout();
    }

    BOOL CConsoleWnd::IsStatusBarVisible()
    {
        return m_bStatusBarVisible;
    }

    VOID CConsoleWnd::SetStatusBarVisible(BOOL bVisible)
    {
        m_bStatusBarVisible = bVisible;
        UpdateLayout();
    }

    BOOL CConsoleWnd::IsDescriptionBarVisible()
    {
        return m_bDescriptionBarVisible;
    }

    VOID CConsoleWnd::SetDescriptionBarVisible(BOOL bVisible)
    {
        m_bDescriptionBarVisible = bVisible;
        UpdateLayout();
    }

    BOOL CConsoleWnd::IsActionsPaneVisible()
    {
        return m_bActionsPaneVisible;
    }

    VOID CConsoleWnd::SetActionsPaneVisible(BOOL bVisible)
    {
        m_bActionsPaneVisible = bVisible;
        UpdateLayout();
    }

    VOID CConsoleWnd::UpdateLayout()
    {
        RECT Rect;
        int iTreeViewWidth, iTreeViewHeight;
        int iListViewPosX, iListViewPosY = 0;
        int iListViewWidth, iListViewHeight;

        GetClientRect(&Rect);
        int iClientHeight = Rect.bottom - Rect.top;
        int iClientWidth = Rect.right - Rect.left;

        int iStatusHeight = 0;

        int iDescriptionBarHeight = 0;

        if (m_bTreeViewVisible)
        {
            iTreeViewWidth = m_iTreeViewWidth;
            iListViewPosX = m_iTreeViewWidth + m_iSplitterWidth;
            iListViewWidth = iClientWidth - m_iTreeViewWidth - m_iSplitterWidth;
        }
        else
        {
            iTreeViewWidth = 0;
            iListViewPosX = 0;
            iListViewWidth = iClientWidth;
        }

        iTreeViewHeight = iClientHeight;
        iListViewHeight = iClientHeight;

        if (m_bStatusBarVisible)
        {
            iStatusHeight = m_iStatusBarHeight;
            iTreeViewHeight -= iStatusHeight;
            iListViewHeight -= iStatusHeight;
        }

        if (m_bDescriptionBarVisible)
        {
            iDescriptionBarHeight = 24;
            iListViewPosY += iDescriptionBarHeight;
            iListViewHeight -= iDescriptionBarHeight;
        }

        if (m_bActionsPaneVisible)
        {
            iListViewWidth -= (m_iActionsPaneWidth + m_iSplitterWidth);
        }

        /* Move the status bar */
        m_StatusBar.MoveWindow(0, iClientHeight - iStatusHeight, iClientWidth, iStatusHeight);

        /* Move the tree view */
        m_TreeView.MoveWindow(0, 0, iTreeViewWidth, iTreeViewHeight);

        /* Move the list view */
        m_ListView.MoveWindow(iListViewPosX, iListViewPosY, iListViewWidth, iListViewHeight);

        m_DescriptionBar.MoveWindow(iListViewPosX, 0, iListViewWidth, iDescriptionBarHeight);
    }

    VOID CConsoleWnd::UpdateView()
    {
    }

    // +IConsole
    STDMETHODIMP CConsoleWnd::QueryInterface(REFIID riid, void **ppvObject)
    {
        if (riid == IID_IUnknown || riid == IID_IConsole)
        {
            *ppvObject = static_cast<IConsole*>(this);
            AddRef();
            return S_OK;
        }
        else if (riid == IID_IConsole2)
        {
            *ppvObject = static_cast<IConsole2*>(this);
            AddRef();
            return S_OK;
        }
        else if (riid == IID_IConsoleNameSpace)
        {
            *ppvObject = static_cast<IConsoleNameSpace*>(this);
            AddRef();
            return S_OK;
        }
        else if (riid == IID_IConsoleNameSpace2)
        {
            *ppvObject = static_cast<IConsoleNameSpace2*>(this);
            AddRef();
            return S_OK;
        }
        else
        {
            CComHeapPtr<OLECHAR> guidstr;
            HRESULT hr = StringFromCLSID(riid, &guidstr);
            if (SUCCEEDED(hr))
                DPRINT1("Unhandled riid: %S\n", (PCWSTR)guidstr);
            else
                DPRINT1("Unhandled riid\n");
        }


        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) CConsoleWnd::AddRef()
    {
        DPRINT("%s()\n", __FUNCTION__);
        return 2;
    }

    STDMETHODIMP_(ULONG) CConsoleWnd::Release()
    {
        DPRINT("%s()\n", __FUNCTION__);
        return 1;
    }


    STDMETHODIMP CConsoleWnd::SetHeader(LPHEADERCTRL pHeader)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::SetToolbar(LPTOOLBAR pToolbar)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::QueryResultView(LPUNKNOWN *pUnknown)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::QueryScopeImageList(LPIMAGELIST *ppImageList)
    {
        DPRINT("%s(%p)\n", __FUNCTION__);
        if (!ppImageList)
            return E_INVALIDARG;

        __debugbreak();
        //*ppImageList = m_ScopeImageList;
        return S_OK;
    }

    STDMETHODIMP CConsoleWnd::QueryResultImageList(LPIMAGELIST *ppImageList)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::UpdateAllViews(LPDATAOBJECT lpDataObject, LPARAM data, LONG_PTR hint)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::MessageBox(LPCWSTR lpszText, LPCWSTR lpszTitle, UINT fuStyle, int *piRetval)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::QueryConsoleVerb(LPCONSOLEVERB *ppConsoleVerb)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::SelectScopeItem(HSCOPEITEM hScopeItem)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::GetMainWindow(HWND *phwnd)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::NewWindow(HSCOPEITEM hScopeItem, ULONG lOptions)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }
    // -IConsole

    // +IConsole2
    STDMETHODIMP CConsoleWnd::Expand(HSCOPEITEM hItem, BOOL bExpand)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::IsTaskpadViewPreferred()
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }


    STDMETHODIMP CConsoleWnd::SetStatusText(LPOLESTR pszStatusText)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }
    // -IConsole2

    // +IConsoleNameSpace
    STDMETHODIMP CConsoleWnd::InsertItem(LPSCOPEDATAITEM item)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::DeleteItem(HSCOPEITEM hItem, LONG fDeleteThis)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::SetItem(LPSCOPEDATAITEM item)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::GetItem(LPSCOPEDATAITEM item)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::GetChildItem(HSCOPEITEM item, HSCOPEITEM *pItemChild, LONG *plCookie)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::GetNextItem(HSCOPEITEM item, HSCOPEITEM *pItemNext, LONG *plCookie)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::GetParentItem(HSCOPEITEM item, HSCOPEITEM *pItemParent, LONG *plCookie)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }
    // -IConsoleNameSpace



    // +IConsoleNameSpace2
    STDMETHODIMP CConsoleWnd::Expand(HSCOPEITEM hItem)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP CConsoleWnd::AddExtension(HSCOPEITEM hItem, LPCLSID lpClsid)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }
    // -IConsoleNameSpace2

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

        LPMDICREATESTRUCT mdicreate = reinterpret_cast<LPMDICREATESTRUCT>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
        if (mdicreate->lParam)
            PostMessage(WM_SYSCOMMAND, SC_MAXIMIZE, 0);

        m_ViewId = m_MainWnd->RegisterView(this);

        return 0;
    }

    LRESULT CConsoleWnd::OnDestroy(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        m_MainWnd->UnregisterView(this);
        m_MainWnd->PostMessage(WM_USER_CLOSE_CHILD, 0, 0);
        return 0;
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

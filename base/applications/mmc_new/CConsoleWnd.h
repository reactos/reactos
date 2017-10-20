/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Single 'console' window
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller
 *              Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once

class CConsoleWnd :
    public CWindowImpl<CConsoleWnd>,
    public IConsole2,
    public IConsoleNameSpace2
{
private:
    CWindow* m_MainWnd;

public:
    CAtlString m_Filename;

    //CComPtr<CImageList> m_ScopeImageList;

public:

    BEGIN_MSG_MAP(CConsoleWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()


    static CWndClassInfo& GetWndClassInfo()
    {
        static CWndClassInfo wc =
        {
            {
                sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, StartWindowProc,
                0, sizeof(CConsoleWnd*), NULL, NULL, NULL, (HBRUSH)(COLOR_3DFACE + 1), NULL, L"MMCChildFrm", NULL
            },
            NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
        };
        return wc;
    }


    static LPCTSTR GetWndClassName()
    {
        return GetWndClassInfo().m_wc.lpszClassName;
    }

public:
    CConsoleWnd(CWindow* parent)
    {
        m_MainWnd = parent;
        m_pfnSuperWindowProc = DefMDIChildProc;

        //m_ScopeImageList = new CComObject<CImageList>();

        if (!m_thunk.Init(NULL, NULL))
            return;
        _AtlWinModule.AddCreateWndData(&m_thunk.cd, this);
    }

    LRESULT OnCreate(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SetWindowLongPtr(0, (LONG_PTR)this);

        LPMDICREATESTRUCT mdicreate = reinterpret_cast<LPMDICREATESTRUCT>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
        if (mdicreate->lParam)
            PostMessage(WM_SYSCOMMAND, SC_MAXIMIZE, 0);

        return 0;
    }

    LRESULT OnDestroy(UINT nMessage, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        m_MainWnd->PostMessage(WM_USER_CLOSE_CHILD, 0, 0);
        return 0;
    }


    // +IConsole
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject)
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

    STDMETHODIMP_(ULONG) AddRef()
    {
        DPRINT("%s()\n", __FUNCTION__);
        return 2;
    }

    STDMETHODIMP_(ULONG) Release()
    {
        DPRINT("%s()\n", __FUNCTION__);
        return 1;
    }


    STDMETHODIMP SetHeader(LPHEADERCTRL pHeader)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP SetToolbar(LPTOOLBAR pToolbar)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP QueryResultView(LPUNKNOWN *pUnknown)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP QueryScopeImageList(LPIMAGELIST *ppImageList)
    {
        DPRINT("%s(%p)\n", __FUNCTION__);
        if (!ppImageList)
            return E_INVALIDARG;

        __debugbreak();
        //*ppImageList = m_ScopeImageList;
        return S_OK;
    }

    STDMETHODIMP QueryResultImageList(LPIMAGELIST *ppImageList)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP UpdateAllViews(LPDATAOBJECT lpDataObject, LPARAM data, LONG_PTR hint)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP MessageBox(LPCWSTR lpszText, LPCWSTR lpszTitle, UINT fuStyle, int *piRetval)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP QueryConsoleVerb(LPCONSOLEVERB *ppConsoleVerb)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP SelectScopeItem(HSCOPEITEM hScopeItem)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP GetMainWindow(HWND *phwnd)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP NewWindow(HSCOPEITEM hScopeItem, ULONG lOptions)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }
    // -IConsole

    // +IConsole2
    STDMETHODIMP Expand(HSCOPEITEM hItem, BOOL bExpand)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP IsTaskpadViewPreferred()
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }


    STDMETHODIMP SetStatusText(LPOLESTR pszStatusText)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }
    // -IConsole2

    // +IConsoleNameSpace
    STDMETHODIMP InsertItem(LPSCOPEDATAITEM item)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP DeleteItem(HSCOPEITEM hItem, LONG fDeleteThis)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP SetItem(LPSCOPEDATAITEM item)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP GetItem(LPSCOPEDATAITEM item)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP GetChildItem(HSCOPEITEM item, HSCOPEITEM *pItemChild, LONG *plCookie)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP GetNextItem(HSCOPEITEM item, HSCOPEITEM *pItemNext, LONG *plCookie)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP GetParentItem(HSCOPEITEM item, HSCOPEITEM *pItemParent, LONG *plCookie)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }
    // -IConsoleNameSpace



    // +IConsoleNameSpace2
    STDMETHODIMP Expand(HSCOPEITEM hItem)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }

    STDMETHODIMP AddExtension(HSCOPEITEM hItem, LPCLSID lpClsid)
    {
        DPRINT("%s()\n", __FUNCTION__);
        __debugbreak();
        return E_NOTIMPL;
    }
    // -IConsoleNameSpace2


};


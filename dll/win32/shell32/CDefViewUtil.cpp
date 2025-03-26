/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     DefView utility functions
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

class CObjectWithSiteBase :
    public IObjectWithSite
{
public:
    IUnknown* m_pUnkSite;

    CObjectWithSiteBase() : m_pUnkSite(NULL) {}
    virtual ~CObjectWithSiteBase() { SetSite(NULL); }

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *pUnkSite) override
    {
        IUnknown_Set(&m_pUnkSite, pUnkSite);
        return S_OK;
    }
    STDMETHODIMP GetSite(REFIID riid, void **ppvSite) override
    {
        *ppvSite = NULL;
        return m_pUnkSite ? m_pUnkSite->QueryInterface(riid, ppvSite) : E_FAIL;
    }
};

// This class adapts the legacy function callback to work as an IShellFolderViewCB
class CShellFolderViewCBWrapper :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolderViewCB,
    public CObjectWithSiteBase
{
protected:
    HWND                    m_hWndMain;
    PIDLIST_ABSOLUTE        m_Pidl;
    CComPtr<IShellFolder>   m_psf;
    CComPtr<IShellView>     m_psvOuter;
    LPFNVIEWCALLBACK        m_Callback;
    FOLDERVIEWMODE          m_FVM;
    LONG                    m_Events;

public:
    CShellFolderViewCBWrapper() : m_hWndMain(NULL), m_Pidl(NULL) {}

    virtual ~CShellFolderViewCBWrapper()
    {
        ILFree(m_Pidl);
    }

    HRESULT WINAPI Initialize(LPCSFV psvcbi)
    {
        m_psf = psvcbi->pshf;
        m_psvOuter = psvcbi->psvOuter;
        m_Pidl = psvcbi->pidl ? ILClone(psvcbi->pidl) : NULL;
        m_Callback = psvcbi->pfnCallback;
        m_FVM = psvcbi->fvm;
        m_Events = psvcbi->lEvents;
        return S_OK;
    }

    // IShellFolderViewCB
    STDMETHODIMP MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        switch (uMsg)
        {
            case SFVM_HWNDMAIN:
                m_hWndMain = (HWND)lParam;
                break;

            case SFVM_DEFVIEWMODE:
                if (m_FVM)
                    *(FOLDERVIEWMODE*)lParam = m_FVM;
                break;
        }

        HRESULT hr = m_Callback(m_psvOuter, m_psf, m_hWndMain, uMsg, wParam, lParam);
        if (SUCCEEDED(hr))
            return hr;

        switch (uMsg)
        {
            case SFVM_GETNOTIFY:
                *(LPITEMIDLIST*)wParam = m_Pidl;
                *(LONG*)lParam = m_Events;
                return S_OK;
        }
        return hr;
    }

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *pUnkSite) override
    {
        // learn.microsoft.com/en-us/windows/win32/shell/sfvm-setisfv
        HRESULT hr = CObjectWithSiteBase::SetSite(pUnkSite);
        MessageSFVCB(SFVM_SETISFV, 0, (LPARAM)pUnkSite);
        return hr;
    }

    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(CShellFolderViewCBWrapper)
    BEGIN_COM_MAP(CShellFolderViewCBWrapper)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolderViewCB, IShellFolderViewCB)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
    END_COM_MAP()
};

/*************************************************************************
 * SHCreateShellFolderViewEx [SHELL32.174] (Win95+)
 */
EXTERN_C HRESULT WINAPI
SHCreateShellFolderViewEx(_In_ LPCSFV pcsfv, _Out_ IShellView **ppsv)
{
    if (!ppsv)
        return E_INVALIDARG;
    *ppsv = NULL;

    TRACE("sf=%p pidl=%p cb=%p mode=0x%08x outer=%p\n",
          pcsfv->pshf, pcsfv->pidl, pcsfv->pfnCallback,
          pcsfv->fvm, pcsfv->psvOuter);

    CComPtr<IShellFolderViewCB> psfvcb;
    SFV_CREATE create = { sizeof(create), pcsfv->pshf, pcsfv->psvOuter };

    if (pcsfv->pfnCallback)
    {
        HRESULT hr = ShellObjectCreatorInit<CShellFolderViewCBWrapper>(pcsfv,
                        IID_PPV_ARG(IShellFolderViewCB, &psfvcb));
        if (FAILED(hr))
            return hr;
        create.psfvcb = psfvcb;
    }
    return SHCreateShellFolderView(&create, ppsv);
}

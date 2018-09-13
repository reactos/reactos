#include "stdafx.h"
#pragma hdrstop

#include "stdenum.h"

#define DM_SDFOLDER 0

// in DVOC.CPP
extern DWORD GetViewOptionsForDispatch(void);

//=====================================================================
// ShellFolderView Class Definition...
//=====================================================================
class CShellFolderView :  public IShellFolderViewDual,
                            public IShellService,
                            public CObjectSafety,
                            public CObjectWithSite, 
                            protected CImpIConnectionPointContainer,
                            protected CImpIExpDispSupport,
                            protected CImpIDispatch
{
public:
    CShellFolderView(void);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDispatch
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);

    // ShellWindow
    STDMETHODIMP get_Application(IDispatch **ppid);
    STDMETHODIMP get_Parent(IDispatch **ppid);

    STDMETHODIMP get_Folder(Folder **ppid);
    STDMETHODIMP SelectedItems(FolderItems **ppid);
    STDMETHODIMP get_FocusedItem(FolderItem **ppid);
    STDMETHODIMP SelectItem(VARIANT *pvfi, int dwFlags);
    STDMETHODIMP PopupItemMenu(FolderItem * pfi, VARIANT vx, VARIANT vy, BSTR * pbs);
    STDMETHODIMP get_Script(IDispatch **ppid);
    STDMETHODIMP get_ViewOptions(long *plSetting);

    // IShellService methods.
    STDMETHODIMP SetOwner(IUnknown* punkOwner);

    // CImpIConnectionPoint
    STDMETHODIMP EnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum);

    // CObjectWithSite overriding
    virtual STDMETHODIMP SetSite(IUnknown *punkSite);

private:
    ~CShellFolderView(void);
    HRESULT _GetFolder();
    // CImpIExpDispSupport
    virtual CConnectionPoint* _FindCConnectionPointNoRef(BOOL fdisp, REFIID iid);

    LONG m_cRef;
    CFolder *m_psdf;        // The shell folder we talk to ...
    IUnknown *m_punkOwner;   // The owner object of us...
    IShellFolderView *m_psfvOwner;  // The owners Shell folder view...
    HWND m_hwnd;         // the hwnd for the window...

    // Embed our Connection Point object - implmentation in cnctnpt.cpp
    CConnectionPoint m_cpEvents;
};

STDAPI CShellFolderView_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CShellFolderView* psfv = new CShellFolderView();
    if (psfv) 
    {
        hr = psfv->QueryInterface(riid, ppvOut);
        psfv->Release();
    }
    return hr;
}

CShellFolderView::CShellFolderView(void) :
        CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_IShellFolderViewDual), m_cRef(1), m_psdf(NULL)
{
    DllAddRef();
    m_cpEvents.SetOwner((IUnknown*)SAFECAST(this, IShellFolderViewDual*), &DIID_DShellFolderViewEvents);
}

CShellFolderView::~CShellFolderView(void)
{
    TraceMsg(DM_SDFOLDER, "CShellFolderView::~CShellFolderView called");

    // if we ever grabbed a shell folder for this window release it also
    if (m_psdf)
    {
        m_psdf->SetSite(NULL);
        m_psdf->Release();
    }

    ASSERT(m_punkOwner == NULL);
    ASSERT(m_psfvOwner == NULL);

    DllRelease();
}

STDMETHODIMP CShellFolderView::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CShellFolderView, IShellFolderViewDual),
        QITABENTMULTI(CShellFolderView, IDispatch, IShellFolderViewDual),
        QITABENT(CShellFolderView, IShellService),
        QITABENT(CShellFolderView, IConnectionPointContainer),
        QITABENT(CShellFolderView, IExpDispSupport),
        QITABENT(CShellFolderView, IObjectSafety),
        QITABENT(CShellFolderView, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CShellFolderView::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellFolderView::Release(void)
{
    //CCosmoClient deletes this object during shutdown
    if (0 != --m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}

//The ShellWindow implementation
// let folder we have handle this.  Probably won't work for webviews as this object
// is not secure...

STDMETHODIMP CShellFolderView::get_Application(IDispatch **ppid)
{
    HRESULT hres = _GetFolder();
    if (SUCCEEDED(hres))
        hres = m_psdf->get_Application(ppid);
    return hres;
}

STDMETHODIMP CShellFolderView::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_FAIL;
}

HRESULT CShellFolderView::_GetFolder()
{
    if (m_psdf)
        return NOERROR;

    HRESULT hres;

    LPITEMIDLIST pidl = NULL;
    IShellFolder *psf = NULL;

    if (m_psfvOwner)
    {
        IDefViewFrame *pdvf;
        if (SUCCEEDED(m_psfvOwner->QueryInterface(IID_IDefViewFrame, (void**)&pdvf)))
        {
            if (SUCCEEDED(pdvf->GetShellFolder(&psf)))
            {
                if (SHGetIDListFromUnk(psf, &pidl) != S_OK)
                {
                    psf->Release();
                    psf = NULL;
                }
            }
            pdvf->Release();
        }

        if (!pidl)
        {
            LPCITEMIDLIST pidlT;
            hres = GetObjectSafely(m_psfvOwner, &pidlT, (UINT)-42);
            if (SUCCEEDED(hres))
            {
                pidl = ILClone(pidlT);
            }
        }
    }

    if (pidl)
    {
        hres = CFolder_Create2(m_hwnd, pidl, psf, &m_psdf);

        if (_dwSafetyOptions && _punkSite && SUCCEEDED(hres))
        {
            m_psdf->SetSite(_punkSite);
            hres = MakeSafeForScripting((IUnknown**)&m_psdf);
        }

        if (psf)
            psf->Release();
        ILFree(pidl);
    }
    else
        hres = E_FAIL;

    return hres;
}

STDMETHODIMP CShellFolderView::SetSite(IUnknown *punkSite)
{
    if (m_psdf)
        m_psdf->SetSite(punkSite);
    return CObjectWithSite::SetSite(punkSite);
}

STDMETHODIMP CShellFolderView::get_Folder(Folder **ppid)
{
    *ppid = NULL;

    HRESULT hres = _GetFolder();
    if (SUCCEEDED(hres))
        hres = m_psdf->QueryInterface(IID_Folder, (void **)ppid);
    return hres;
}


STDMETHODIMP CShellFolderView::SelectedItems(FolderItems **ppid)
{
    // We need to talk to the actual window under us
    HRESULT hres = _GetFolder();
    if (FAILED(hres))
        return hres;

    hres = CFolderItems_Create(m_psdf, TRUE, ppid);

    if (_dwSafetyOptions && SUCCEEDED(hres))
        hres = MakeSafeForScripting((IUnknown**)ppid);

    return hres;
}

// NOTE: this returns an alias pointer, it is not allocated

HRESULT GetObjectSafely(IShellFolderView *psfv, LPCITEMIDLIST *ppidl, UINT iType)
{
	// cast needed because GetObject() is declared wrong, it returns a const ptr
    HRESULT hr = psfv->GetObject((LPITEMIDLIST *)ppidl, iType);
    if (SUCCEEDED(hr))
    {
        // On the off chance this is coppied across process boundries...
        __try
        {
            // force a full deref this PIDL to generate a fault if cross process
            if (ILGetSize(*ppidl) > 0)
                hr = S_OK;
            // Don't free it as it was not cloned...
        }
        __except(SetErrorMode(SEM_NOGPFAULTERRORBOX), UnhandledExceptionFilter(GetExceptionInformation()))
        {
            *ppidl = NULL;
            hr = E_FAIL;
        }
    }
    return hr;
}


STDMETHODIMP CShellFolderView::get_FocusedItem(FolderItem **ppid)
{
    // We need to talk to the actual window under us
    HRESULT hr = _GetFolder();
    if (FAILED(hr))
        return hr;

    *ppid = NULL;
    hr = S_FALSE;

    if (m_psfvOwner)
    {
        // Warning:
        //   It is common for the following function to fail (which means no item has the focus).
        // So, do not save the return code from GetObjectSafely() into "hr" that will ruin the
        // S_FALSE value already stored there and result in script errors. (Bug #301306)
        //
        LPCITEMIDLIST pidl;
        if (SUCCEEDED(GetObjectSafely(m_psfvOwner, &pidl, (UINT)-2)))
        {
            hr = CFolderItem_Create(m_psdf, pidl, ppid);

            if (_dwSafetyOptions && SUCCEEDED(hr))
                hr = MakeSafeForScripting((IUnknown**)ppid);
        }
    }
    else
        hr = E_FAIL;
    return hr;
}

// pvfi should be a "FolderItem" IDispatch

STDMETHODIMP CShellFolderView::SelectItem(VARIANT *pvfi, int dwFlags)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidl = VariantToIDList(pvfi);
    if (pidl)
    {
        IShellView *psv;    // use this to select the item...
        if (m_punkOwner && SUCCEEDED(m_punkOwner->QueryInterface(IID_IShellView, (void **)&psv)))
        {
            hr = psv->SelectItem(ILFindLastID(pidl), dwFlags);
            psv->Release();
        }
        ILFree(pidl);
    }
    return hr;
}

STDMETHODIMP CShellFolderView::PopupItemMenu(FolderItem *pfi, VARIANT vx, VARIANT vy, BSTR * pbs)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellFolderView::get_Script(IDispatch **ppid)
{
    // Say that we got nothing...
    *ppid = NULL;

    // Assume we don't have one...
    if (!m_punkOwner)
        return S_FALSE;

    IShellView *psv;
    HRESULT hres = m_punkOwner->QueryInterface(IID_IShellView, (void **)&psv);
    if (SUCCEEDED(hres))
    {
        // lets see if there is a IHTMLDocument that is below us now...
        IHTMLDocument *phtmld;
        hres = psv->GetItemObject(SVGIO_BACKGROUND, IID_IHTMLDocument, (void **)&phtmld);
        if (SUCCEEDED(hres))
        {
            if (_dwSafetyOptions)
                hres = MakeSafeForScripting((IUnknown **)&phtmld);

            if (SUCCEEDED(hres))
            {
                hres = phtmld->get_Script(ppid);
                phtmld->Release();
            }
        }
        psv->Release();
    }

    return hres;
}

STDMETHODIMP CShellFolderView::get_ViewOptions(long *plSetting)
{
    *plSetting = (LONG)GetViewOptionsForDispatch();
    return S_OK;
}

STDMETHODIMP CShellFolderView::SetOwner(IUnknown* punkOwner)
{
    // Release any previous owners. (ATOMICRELEASE takes care of the null check)
    ATOMICRELEASE(m_psfvOwner);
    ATOMICRELEASE(m_punkOwner);

    // Set the new owner if any set then increment reference count...
    m_punkOwner = punkOwner;
    if (m_punkOwner)
    {
        m_punkOwner->AddRef();
        m_punkOwner->QueryInterface(IID_IShellFolderView, (void **)&m_psfvOwner);

        if (!m_hwnd)
        {
            IShellView *psv;

            // this is gross, until we can merge the two models, create one of our
            // Window objects.
            if (SUCCEEDED(m_punkOwner->QueryInterface(IID_IShellView, (void **)&psv)))
            {
                HWND hwndFldr;
                psv->GetWindow(&hwndFldr);

                // Really gross, but assume parent HWND is the HWND we are after...
                m_hwnd = GetParent(hwndFldr);
                psv->Release();
            }
        }
    }

    return NOERROR;
}

STDMETHODIMP CShellFolderView::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
{
    return CreateInstance_IEnumConnectionPoints(ppEnum, 1, m_cpEvents.CastToIConnectionPoint());
}

CConnectionPoint* CShellFolderView::_FindCConnectionPointNoRef(BOOL fdisp, REFIID iid)
{
    if (IsEqualIID(iid, DIID_DShellFolderViewEvents) || (fdisp && IsEqualIID(iid, IID_IDispatch)))
        return &m_cpEvents;

    return NULL;
}

HRESULT CShellFolderView::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    HRESULT hr;

    if (dispidMember == DISPID_READYSTATE)
        return DISP_E_MEMBERNOTFOUND;   // perf: what typeinfo would return.

    if (dispidMember == DISPID_WINDOWOBJECT) 
    {
        IDispatch *pdisp;
        if (SUCCEEDED(get_Script(&pdisp))) 
        {
            hr = pdisp->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
            pdisp->Release();
        } 
        else 
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
    }
    else
        hr = CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
    return hr;
}

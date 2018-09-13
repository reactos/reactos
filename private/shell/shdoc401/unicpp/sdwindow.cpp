#include "stdafx.h"
#pragma hdrstop

#ifdef POSTSPLIT

//#include "cnctnpt.h"
//#include "stdenum.h"

//#include "sdspatch.h"
//#include <mshtml.h>
//#include <mshtmdid.h>
//#include "expdsprt.h"
//#include "dutil.h"
//#include "clsobj.h"

#define DM_SDFOLDER 0

// in DVOC.CPP
extern DWORD GetViewOptionsForDispatch(void);

//=====================================================================
// ShellFolderView Class Definition...
//=====================================================================
class CSDShellFolderView :  public IShellFolderViewDual,
                            public IShellService,
                            protected CImpIConnectionPointContainer,
                            protected CImpIExpDispSupport,
                            protected CImpIDispatch
{
protected:
    ULONG           m_cRef;
    CSDFolder       *m_psdf;        // The shell folder we talk to ...
    IUnknown        *m_punkOwner;   // The owner object of us...
    IShellFolderView *m_psfvOwner;  // The owners Shell folder view...
    HWND            m_hwnd;         // the hwnd for the window...

    // Embed our Connection Point object - implmentation in cnctnpt.cpp
    CConnectionPoint m_cpEvents;

    CSDShellFolderView(void);
    ~CSDShellFolderView(void);

    BOOL Init(void);
    LPITEMIDLIST _GetFolderPidl();
    HRESULT      _GetFolder();

    // CImpIExpDispSupport
    virtual CConnectionPoint* _FindCConnectionPointNoRef(BOOL fdisp, REFIID iid);

    friend STDMETHODIMP CShellFolderView_CreateInstance(IUnknown* punkOuter, REFIID riid, LPVOID * ppvOut);

public:
    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IDispatch members
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);

    //ShellWindow methods
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
};

STDAPI CShellFolderView_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hres = E_OUTOFMEMORY;
    *ppvOut = NULL;

    CSDShellFolderView* psfv = new CSDShellFolderView();
    if (psfv) 
    {
        if (psfv->Init())
        {
            hres = psfv->QueryInterface(riid, ppvOut);
        }
        psfv->Release();
    }

    return hres;
}

CSDShellFolderView::CSDShellFolderView(void) :
        CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_IShellFolderViewDual), m_cRef(1), m_psdf(NULL)
{
    DllAddRef();
    m_cpEvents.SetOwner((IUnknown*)SAFECAST(this, IShellFolderViewDual*), &DIID_DShellFolderViewEvents);
}

CSDShellFolderView::~CSDShellFolderView(void)
{
    TraceMsg(DM_SDFOLDER, "CSDShellFolderView::~CSDShellFolderView called");

    // if we ever grabbed a shell folder for this window release it also
    if (m_psdf)
        m_psdf->Release();

    ASSERT(m_punkOwner == NULL);
    ASSERT(m_psfvOwner == NULL);

    DllRelease();
}

BOOL CSDShellFolderView::Init(void)
{
    return TRUE;
}

STDMETHODIMP CSDShellFolderView::QueryInterface(REFIID riid, void **ppv)
{
    TraceMsg(DM_SDFOLDER, "TraceMsgCSDShellFolderView::QueryInterface called");
    static const QITAB qit[] = {
        QITABENT(CSDShellFolderView, IShellFolderViewDual),
        QITABENTMULTI(CSDShellFolderView, IDispatch, IShellFolderViewDual),
        QITABENT(CSDShellFolderView, IShellService),
        QITABENT(CSDShellFolderView, IConnectionPointContainer),
        QITABENT(CSDShellFolderView, IExpDispSupport),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSDShellFolderView::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CSDShellFolderView::Release(void)
{
    //CCosmoClient deletes this object during shutdown
    if (0 != --m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}

//The ShellWindow implementation

STDMETHODIMP CSDShellFolderView::get_Application(IDispatch **ppid)
{
    // let folder we have handle this.  Probably won't work for webviews as this object
    // is not secure...
    HRESULT hres = _GetFolder();
    if (FAILED(hres))
        return hres;

    return m_psdf->get_Application(ppid);
}

STDMETHODIMP CSDShellFolderView::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_FAIL;
}

LPITEMIDLIST CSDShellFolderView::_GetFolderPidl()
{
    LPITEMIDLIST pidl = NULL;

    if (m_psfvOwner)
    {
        LPITEMIDLIST pidlT;
        if (SUCCEEDED(m_psfvOwner->GetObject(&pidlT, (UINT)-42)))
        {
            // On the off chance this is copied across process boundries...
            __try
            {
                pidl = ILClone(pidlT);
            }
            __except(SetErrorMode(SEM_NOGPFAULTERRORBOX),UnhandledExceptionFilter(GetExceptionInformation()))
            {
                pidl = NULL;
            }
        }
    }

    return pidl;
}

HRESULT CSDShellFolderView::_GetFolder()
{
    if (m_psdf)
        return NOERROR;

    HRESULT hres;
    LPITEMIDLIST pidl = _GetFolderPidl();

    if (pidl)
    {
        hres = CSDFolder_Create(m_hwnd, pidl, NULL, &m_psdf);
        ILFree(pidl);
    }
    else
        hres = E_FAIL;

    return hres;
}

STDMETHODIMP CSDShellFolderView::get_Folder(Folder **ppid)
{
    *ppid = NULL;

    HRESULT hres = _GetFolder();

    if (FAILED(hres))
        return hres;

    return m_psdf->QueryInterface(IID_Folder, (void **)ppid);
}


STDMETHODIMP CSDShellFolderView::SelectedItems(FolderItems **ppid)
{
    // We need to talk to the actual window under us
    HRESULT hres = _GetFolder();
    if (FAILED(hres))
        return hres;

    return CSDFldrItems_Create(m_psdf, TRUE, ppid);
}


STDMETHODIMP CSDShellFolderView::get_FocusedItem(FolderItem **ppid)
{
    // We need to talk to the actual window under us
    HRESULT hres = _GetFolder();
    if (FAILED(hres))
        return hres;

    LPITEMIDLIST pidl;

    *ppid = NULL;
    hres = S_FALSE;

    if (m_psfvOwner)
    {
        if (SUCCEEDED(m_psfvOwner->GetObject(&pidl, (UINT)-2)))
        {
            // On the off chance this is coppied across process boundries...
            __try
            {
                hres = CSDFldrItem_Create(m_psdf, pidl, ppid);
                // Don't free it as it was not cloned...
            }
            __except(SetErrorMode(SEM_NOGPFAULTERRORBOX),UnhandledExceptionFilter(GetExceptionInformation()))
            {
                ;
            }
        }
    }
    else
        hres = E_FAIL;
    return hres;
}

STDMETHODIMP CSDShellFolderView::SelectItem(VARIANT *pvfi, int dwFlags)
{
    HRESULT hres = E_FAIL;
    LPCITEMIDLIST pidl = VariantToConstIDList(pvfi);
    if (pidl)
    {
        IShellView *psv;    // use this to select the item...

        if (m_punkOwner && SUCCEEDED(hres = m_punkOwner->QueryInterface(IID_IShellView, (void **)&psv)))
        {
            hres = psv->SelectItem(pidl, dwFlags);
            psv->Release();
        }
    }
    return hres;
}

STDMETHODIMP CSDShellFolderView::PopupItemMenu(FolderItem *pfi, VARIANT vx, VARIANT vy, BSTR * pbs)
{
    return E_NOTIMPL;
}

STDMETHODIMP CSDShellFolderView::get_Script(IDispatch **ppid)
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
            hres = phtmld->get_Script(ppid);
            phtmld->Release();
        }
        psv->Release();
    }

    return hres;
}

STDMETHODIMP CSDShellFolderView::get_ViewOptions(long *plSetting)
{
    *plSetting = (LONG)GetViewOptionsForDispatch();
    return S_OK;
}

STDMETHODIMP CSDShellFolderView::SetOwner(IUnknown* punkOwner)
{
    // Release any previous owners.
    if (m_psfvOwner)
    {
        m_psfvOwner->Release();
        m_psfvOwner = NULL;
    }
    if (m_punkOwner)
    {
        IUnknown *punk = m_punkOwner;
        m_punkOwner = NULL;
        punk->Release();
    }

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


STDMETHODIMP CSDShellFolderView::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
{
    return CreateInstance_IEnumConnectionPoints(ppEnum, 1, m_cpEvents.CastToIConnectionPoint());
}

CConnectionPoint* CSDShellFolderView::_FindCConnectionPointNoRef(BOOL fdisp, REFIID iid)
{
    if (IsEqualIID(iid, DIID_DShellFolderViewEvents) || (fdisp && IsEqualIID(iid, IID_IDispatch)))
        return &m_cpEvents;

    return NULL;
}

HRESULT CSDShellFolderView::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    HRESULT        hr;
    IDispatch      *pdisp;

    if (dispidMember == DISPID_READYSTATE)
        return DISP_E_MEMBERNOTFOUND;   // This is what typeinfo would return.

    if (dispidMember == DISPID_WINDOWOBJECT) {
        if (SUCCEEDED(get_Script(&pdisp))) {
            hr = pdisp->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
            pdisp->Release();
            return hr;
        } else {
            return DISP_E_MEMBERNOTFOUND;
        }
    }

    return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}


#endif // POSTSPLIT

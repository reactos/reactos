//--------------------------------------------------------------------------
// Manage the windows list, such that we can get the IDispatch for each of
// the shell windows to be marshalled to different processes
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Includes...
#include "priv.h"
#include "sccls.h"
#include <shdguid.h>
#include "winlist.h"
#include "iedde.h"

#define DM_WINLIST  0

void IEInitializeClassFactoryObject(IUnknown* punkAuto);
void IERevokeClassFactoryObject(void);

class CShellWindowListCF : public IClassFactory
{
    // IUnKnown
    public:
    virtual STDMETHODIMP QueryInterface(REFIID,void **);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IClassFactory
    virtual STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
    virtual STDMETHODIMP LockServer(BOOL fLock);

    // constructor
    CShellWindowListCF();
    BOOL Init(void);

protected:
    ~CShellWindowListCF();

    // locals

    LONG            _cRef;
    IShellWindows    *_pswWinList;
};

DWORD g_dwWinListCFRegister = 0;
DWORD g_fWinListRegistered = FALSE;     // Only used in browser only mode...
IShellWindows *g_pswWinList = NULL;

// Function to get called by the tray to create the global window list and register
// it with the system

//=================================== Class Factory implemention ========================
CShellWindowListCF::CShellWindowListCF()
{
    _cRef = 1;
    DllAddRef();
}

BOOL CShellWindowListCF::Init()
{
    HRESULT hres = CSDWindows_CreateInstance(&_pswWinList);
    g_pswWinList = _pswWinList;

    // First see if there already is one defined...

    if (FAILED(hres))
    {
        TraceMsg(DM_WINLIST, "WinList_Init CoCreateInstance Failed: %x", hres);
        return FALSE;
    }

    // And register our class factory with the system...
    hres = CoRegisterClassObject(CLSID_ShellWindows, this,
                                 CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
                                 REGCLS_MULTIPLEUSE, &g_dwWinListCFRegister);

    ASSERT(SUCCEEDED(hres));

    //  this call governs when we will call CoRevoke on the CF
    if (SUCCEEDED(hres) && g_pswWinList)
        g_pswWinList->ProcessAttachDetach(TRUE);

    // Create an instance of the underlying window list class...
    TraceMsg(DM_WINLIST, "WinList_Init CoRegisterClass: %x", hres);

    return SUCCEEDED(hres);
}

CShellWindowListCF::~CShellWindowListCF()
{
    if (_pswWinList)
    {
        g_pswWinList = NULL;
        _pswWinList->Release();
    }
    DllRelease();
}

STDMETHODIMP CShellWindowListCF::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = { 
        QITABENT(CShellWindowListCF, IClassFactory), // IID_IClassFactory
        { 0 }, 
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CShellWindowListCF::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShellWindowListCF::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CShellWindowListCF::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    // aggregation checking is done in class factory
    // For now simply use our QueryService to get the dispatch.
    // this will do all of the things to create it and the like.
    if (!_pswWinList) 
    {
        ASSERT(0);
        return E_FAIL;
    }
    return _pswWinList->QueryInterface(riid, ppvObj);
}

STDMETHODIMP CShellWindowListCF::LockServer(BOOL fLock)
{
    return S_OK;    // we don't do anything with this...
}

// As this is marshalled over to the main shell process hopefully this will take care of
// most of the serialization problems.  Probably still need a way to handle the case better
// where a window is coming up at the same time the last one is going down...
STDAPI CWinListShellProc_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;
    if (g_dwWinListCFRegister)
        return CO_E_OBJISREG;

    CShellWindowListCF *pswWinList = new CShellWindowListCF;
    if (pswWinList)
    {
        pswWinList->Init(); // tell it to initialize
        *ppunk = SAFECAST(pswWinList, IUnknown *);
        return NOERROR;
    }
    return E_OUTOFMEMORY;
}


BOOL WinList_Init(void)
{
    // Create our clas factory to register out there...
    TraceMsg(DM_WINLIST, "WinList_Init called");

    //
    //  If this is not a browser-only install. Register the class factory
    // object now with no instance. Otherwise, do it when the first instance
    // is created (see shbrowse.cpp).
    //
    if (!g_fBrowserOnlyProcess)
    {
        //
        //  First, register the class factory object for CLSID_InternetExplorer.
        // Note that we pass NULL indicating that subsequent CreateInstance
        // should simply create a new instance.
        //
        IEInitializeClassFactoryObject(NULL);

        CShellWindowListCF *pswWinList = new CShellWindowListCF;
        if (pswWinList)
        {
            BOOL fRetVal = pswWinList->Init(); // tell it to initialize
            pswWinList->Release(); // Release our handle hopefully init registered

            //
            // Initialize IEDDE.
            //
            if (!IsBrowseNewProcessAndExplorer())
                IEDDE_Initialize();

            return fRetVal;
        }
    }
    else
    {
        //
        // Initialize IEDDE. - Done before cocreate below for timing issues
        //
        IEDDE_Initialize();

        // All of the main processing moved to first call to WinList_GetShellWindows
        // as the creating of OLE object across processes screwed up DDE timing.

        return TRUE;
    }

    return FALSE;
}

#ifdef UNIX
HRESULT CoCreateShellWindows(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext,
                 REFIID riid,  void **ppv)
{
    HRESULT hres;

    if (!g_pswWinList)
    {
        hres = CSDWindows_CreateInstance(&g_pswWinList);

        if (FAILED(hres))
        {
            return E_FAIL;
        }
    }

    hres = g_pswWinList->QueryInterface(riid, ppv);
    if (SUCCEEDED(hres))
    {
    g_dwWinListCFRegister = 1;
    }

    return hres;
}
#endif

// Helper function to get the ShellWindows Object

IShellWindows* WinList_GetShellWindows(BOOL fForceMarshalled)
{
    IShellWindows *psw;

    if (fForceMarshalled)
        psw = NULL;
    else
        psw = g_pswWinList;

    if (psw) 
    {
        // Optimize the inter-thread case by using the global WinList,
        // this makes opening folders much faster.
        psw->AddRef();
    } 
    else 
    {
        SHCheckRegistry();

#ifndef NO_RPCSS_ON_UNIX
        HRESULT hres = CoCreateInstance(CLSID_ShellWindows, NULL,
                         CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
                         IID_IShellWindows,  (void **)&psw);
#else
        HRESULT hres = CoCreateShellWindows(CLSID_ShellWindows, NULL,
                         CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
                         IID_IShellWindows,  (void **)&psw);
#endif

        if ( (g_fBrowserOnlyProcess || !IsInternetExplorerApp()) && !g_fWinListRegistered)
        {
            // If it failed and we are not funning in integrated mode, and this is the
            // first time for this process, we should then register the Window List with
            // the shell process.  We moved that from WinList_Init as that caused us to
            // do interprocess send/post messages to early which caused DDE to break...
            g_fWinListRegistered = TRUE;    // only call once
            if (FAILED(hres))
            {
                SHLoadInProc(CLSID_WinListShellProc);

                hres = CoCreateInstance(CLSID_ShellWindows, NULL,
                                 CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
                                 IID_IShellWindows,  (void **)&psw);
            }

            if (psw)
            {
                psw->ProcessAttachDetach(TRUE);
            }
        }

        // hr == REGDB_E_CLASSNOTREG when the shell process isn't running.
        // hr == RPC_E_CANTCALLOUT_ININPUTSYNCCALL happens durring DDE launch of IE.
        // should investigate, but removing assert for IE5 ship.
        AssertMsg(SUCCEEDED(hres) || hres == REGDB_E_CLASSNOTREG || hres == RPC_E_CANTCALLOUT_ININPUTSYNCCALL,
                  TEXT("WinList_GetShellWindows CoCreateInst(CLSID_ShellWindows) failed %x"), hres);
    }

    return psw;
}


// Function to terminate our use of the window list.
void WinList_Terminate(void)
{
    // Lets release everything in a thread safe way...
    TraceMsg(DM_WINLIST, "WinList_Terminate called");

    IEDDE_Uninitialize();

    // Release our usage of the object to allow the system to clean it up
    if (!g_fBrowserOnlyProcess)
    {
        //  this is the explorer process, and we control the vertical

        if (g_dwWinListCFRegister) {
            IShellWindows* psw = WinList_GetShellWindows(FALSE);
            ASSERT(psw);
#ifdef DEBUG
            long cwindow = -1;
            psw->get_Count(&cwindow);
            //ASSERT(cwindow==0);
            if (cwindow != 0)
                TraceMsg(DM_ERROR, "wl_t: cwindow=%d (!=0)", cwindow);
#endif
            psw->ProcessAttachDetach(FALSE);
            psw->Release();

            //  the processattachdetach() should kill the CF in our process
            if (g_dwWinListCFRegister != 0)
                TraceMsg(DM_ERROR, "wl_t: g_dwWinListCFRegister=%d (!=0)", g_dwWinListCFRegister);

        }

        IERevokeClassFactoryObject();
        CUrlHistory_CleanUp();
    }
    else
    {
        if (g_fWinListRegistered)
        {
            // only do this if we actually registered...
            IShellWindows* psw = WinList_GetShellWindows(TRUE);
            if (psw)
            {
                psw->ProcessAttachDetach(FALSE);    // Tell it we are going away...
                psw->Release();
            }
        }
    }

#ifdef UNIX
    if (g_pswWinList)
    {
        g_pswWinList->Release();
    }
#endif
}

//  BUGBUG chrisfra 10/17/96 - mike schmidt needs to look at why delayed
//  register causes death in OleUnitialize accessing freed vtable
//  in winlist, under ifdef, i've also made WinList_GetShellWindows ignore
//  BOOLEAN parameter

HRESULT WinList_Revoke(long dwRegister)
{
#ifndef DELAY_REGISTER
    IShellWindows* psw = WinList_GetShellWindows(TRUE);
#else
    IShellWindows* psw = WinList_GetShellWindows(FALSE);
#endif
    HRESULT hres = E_FAIL;
    TraceMsg(DM_WINLIST, "WinList_Reevoke called on %x", dwRegister);

    ASSERT(psw);
    if (psw)
    {
        hres = psw->Revoke((long)dwRegister);
        ASSERT(SUCCEEDED(hres));
        psw->Release();
    }

    return hres;
}

STDAPI NavigateToPIDL(IWebBrowser2* pwb, LPCITEMIDLIST pidl)
{
    HRESULT hr;

    VARIANT varThePidl;

    if (InitVariantFromIDList(&varThePidl, pidl))
    {
        hr = pwb->Navigate2(&varThePidl, PVAREMPTY, PVAREMPTY, PVAREMPTY, PVAREMPTY);
        VariantClear(&varThePidl);       // Needed to free the copy of the PIDL in varThePidl.
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

BOOL _InitVariantFromIDListLazy(IShellWindows* psw, VARIANT* pvar, LPCITEMIDLIST pidl)
{
    memset(pvar, 0, sizeof(*pvar));
    ASSERT(pvar->vt == VT_EMPTY);

    if (!pidl)
        return TRUE;    // Leave as empty!

    if (psw == g_pswWinList)
    {
        InitVariantFromIDListInProc(pvar, pidl);  // non-marshalled call
        return TRUE;
    }

    return InitVariantFromIDList(pvar, pidl);
}


HRESULT WinList_NotifyNewLocation(IShellWindows* psw, long dwRegister, LPCITEMIDLIST pidl)
{
    HRESULT hres = E_UNEXPECTED;
    if (pidl) {
        VARIANT var;
        if (_InitVariantFromIDListLazy(psw, &var, pidl)) {
            hres = psw->OnNavigate(dwRegister, &var);
            VariantClearLazy(&var);
        } else {
            hres = E_OUTOFMEMORY;
        }
    }
    return hres;
}

// Winlist_RegisterPending
STDAPI WinList_RegisterPending(DWORD dwThread, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlRoot, long *pdwRegister)
{
    // Register with the window list that we have a pidl that we are starting up.
    HRESULT hres = E_UNEXPECTED;
    ASSERT(!pidlRoot);
    if (pidl)
    {
        IShellWindows* psw = WinList_GetShellWindows(FALSE);
        if (psw)
        {
            VARIANT var;
            if (_InitVariantFromIDListLazy(psw, &var, pidl))
            {
                hres = psw->RegisterPending(dwThread, &var, PVAREMPTY, SWC_BROWSER, pdwRegister);
            }
            else
            {
                hres = E_OUTOFMEMORY;
            }
            VariantClearLazy(&var);
        }
    }
    return hres;
}

/*
 * WinList_FindFolderWindow
 *
 * PERFORMANCE note - getting back the automation object (ppauto) is really
 * expensive due to the marshalling overhead.  Don't query for it unless you
 * absolutely need it!
 */
HRESULT WinList_FindFolderWindow(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlRoot, HWND *phwnd, IWebBrowserApp **ppauto)
{
    HRESULT hres = E_UNEXPECTED;
    ASSERT(!pidlRoot);
    if (ppauto)
        *ppauto = NULL;
    if (phwnd)
        *phwnd = NULL;

    if (pidl) 
    {
        // Try a cached psw if we don't need ppauto
        IShellWindows* psw = WinList_GetShellWindows(ppauto != NULL);
        if (psw)
        {
            VARIANT var;
            if (_InitVariantFromIDListLazy(psw, &var, pidl)) 
            {
                IDispatch* pdisp = NULL;
                hres = psw->FindWindow(&var, PVAREMPTY, SWC_BROWSER, (long *)phwnd,
                        ppauto ? (SWFO_NEEDDISPATCH | SWFO_INCLUDEPENDING) : SWFO_INCLUDEPENDING,
                        &pdisp);
                if (pdisp) 
                {
                    // if this fails it's because we are inside SendMessage loop and ole doesn't like it
                    if (ppauto)
                        hres = pdisp->QueryInterface(IID_IWebBrowserApp, (void **)ppauto);
                    pdisp->Release();
                }
            }
            VariantClearLazy(&var);
            psw->Release();
        }
    }
    return hres;
}

#if 0
HRESULT WinList_OnActivate(IShellWindows* psw, long dwRegister, BOOL fActivate, LPCITEMIDLIST pidl)
{
    HRESULT hres;
    hres = psw->OnActivated(dwRegister, fActivate);
    if (!fActivate) {
        hres = WinList_NotifyNewLocation(psw, dwRegister, pidl);
    }
    return hres;
}
#endif

//===============================================================================
// Support for Being able to open a folder and get it's idispatch...
//
class CGIDFWait
{
public:
    ULONG AddRef(void) ;
    ULONG Release(void);

    BOOL Init(IShellWindows *psw, LPCITEMIDLIST pidl, DWORD dwPending);
    void CleanUp(void);
    HRESULT WaitForWindowToOpen(DWORD dwTimeout);

    CGIDFWait(void);

    protected:

    ~CGIDFWait(void);
        // internal class to watch for events...
        class CGIDFEvents : public DShellWindowsEvents
        {
            public:

            // IUnknown methods
            //
            virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);
            virtual STDMETHODIMP_(ULONG) AddRef(void) ;
            virtual STDMETHODIMP_(ULONG) Release(void);

            // IDispatch methods
            //
            STDMETHOD(GetTypeInfoCount)(THIS_ UINT * pctinfo);
            STDMETHOD(GetTypeInfo)(THIS_ UINT itinfo, LCID lcid, ITypeInfo * * pptinfo);
            STDMETHOD(GetIDsOfNames)(THIS_ REFIID riid, OLECHAR * * rgszNames,
                UINT cNames, LCID lcid, DISPID * rgdispid);
            STDMETHOD(Invoke)(THIS_ DISPID dispidMember, REFIID riid,
                LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult,
                EXCEPINFO * pexcepinfo, UINT * puArgErr);

        } m_EventHandler;

        friend class CGIDFEvents;

        DWORD m_dwCookie;
        IShellWindows *m_psw;
        IConnectionPoint *m_picp;
        DWORD m_dwPending;
        LPITEMIDLIST m_pidl;
        HANDLE m_hevent;
        BOOL  m_fAdvised;
        int m_cRef;
};


ULONG CGIDFWait::AddRef(void)
{
    m_cRef++;
    return m_cRef;
}

ULONG CGIDFWait::Release(void)
{
    m_cRef--;
    if (m_cRef != 0)
        return m_cRef;
    delete this;
    return 0;
}

CGIDFWait::CGIDFWait(void)
{
    ASSERT(m_psw == NULL);
    ASSERT(m_picp == NULL);
    ASSERT(m_hevent == NULL);
    ASSERT(m_dwCookie == 0);
    ASSERT(m_fAdvised == FALSE);
    m_cRef = 1;
    return;
}

CGIDFWait::~CGIDFWait(void)
{
    ATOMICRELEASE(m_psw);


    CleanUp();

    if (m_hevent)
        CloseHandle(m_hevent);

    if (m_pidl)
        ILFree(m_pidl);
}

BOOL CGIDFWait::Init(IShellWindows *psw, LPCITEMIDLIST pidl, DWORD dwPending)
{
    // First try to create an event object
    m_hevent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!m_hevent)
        return FALSE;

    // We do not have a window or it is pending...
    // first lets setup that we want to be notified of new windows.
    if (FAILED(ConnectToConnectionPoint(SAFECAST(&m_EventHandler, IDispatch*), DIID_DShellWindowsEvents, TRUE, psw, &m_dwCookie, &m_picp)))
        return FALSE;

    // Save away passed in stuff that we care about.
    m_psw = psw;
    psw->AddRef();
    m_pidl = ILClone(pidl);
    m_dwPending = dwPending;

    return TRUE;
}

void CGIDFWait::CleanUp(void)
{
    // Don't need to listen anmore.
    if (m_dwCookie)
    {
        m_picp->Unadvise(m_dwCookie);
        m_dwCookie = 0;
    }

    ATOMICRELEASE(m_picp);
}

HRESULT CGIDFWait::WaitForWindowToOpen(DWORD dwTimeOut)
{
    if (!m_hevent || !m_dwCookie)
        return E_FAIL;

    ENTERCRITICAL;

    if (!m_fAdvised)
        ResetEvent(m_hevent);

    LEAVECRITICAL;

    DWORD dwStart = GetTickCount();
    DWORD dwWait = dwTimeOut;
    DWORD dwWaitResult;

    do
    {
        dwWaitResult= MsgWaitForMultipleObjects(1, &m_hevent,
                                                FALSE, // fWaitAll, wait for any one
                                                dwWait, QS_ALLINPUT);

        // Check if we are signaled for a send message.
        if (dwWaitResult != WAIT_OBJECT_0 + 1)
        {
            // No. Break out of the loop.
            break;
        }

        // We may need to dispatch stuff here.
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // than MSEC_MAXWAIT if we wait more than that.
        //
        dwWait = dwStart+dwTimeOut - GetTickCount();


    } while (dwWait <= dwTimeOut);


    BOOL fAdvised;
    
    {
    ENTERCRITICAL;
    
    fAdvised = m_fAdvised;
    m_fAdvised = FALSE;
    
    LEAVECRITICAL;
    }

    return fAdvised ? NOERROR : E_FAIL;
}

STDMETHODIMP CGIDFWait::CGIDFEvents::QueryInterface(REFIID riid, void **ppvOut)
{
    if (IsEqualIID(riid, IID_IDispatch) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppvOut = SAFECAST(this, IDispatch *);
    }
    else if (IsEqualIID(riid, DIID_DShellWindowsEvents))
    {
        *ppvOut = SAFECAST(this, DShellWindowsEvents *);
    }
    else
    {
        *ppvOut = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG CGIDFWait::CGIDFEvents::AddRef(void)
{
    CGIDFWait* pdfwait = IToClass(CGIDFWait, m_EventHandler, this);
    return pdfwait->AddRef();
}

ULONG CGIDFWait::CGIDFEvents::Release(void)
{
    CGIDFWait* pdfwait = IToClass(CGIDFWait, m_EventHandler, this);
    return pdfwait->Release();
}

HRESULT CGIDFWait::CGIDFEvents::GetTypeInfoCount(UINT *pctinfo)
{
    return E_NOTIMPL;
}

HRESULT CGIDFWait::CGIDFEvents::GetTypeInfo(UINT itinfo, LCID lcid,
                                             ITypeInfo **pptinfo)
{
    return E_NOTIMPL;
}

HRESULT CGIDFWait::CGIDFEvents::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames,
                                              UINT cNames, LCID lcid, DISPID *rgdispid)
{
    return E_NOTIMPL;
}


HRESULT CGIDFWait::CGIDFEvents::Invoke(DISPID dispid, REFIID riid,
    LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult,
    EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    CGIDFWait* pdfwait = IToClass(CGIDFWait, m_EventHandler, this);

    if (dispid == DISPID_WINDOWREGISTERED)
    {
        ENTERCRITICAL;
        
        // Signal the event
        pdfwait->m_fAdvised = TRUE;
        ::SetEvent(pdfwait->m_hevent);
        
        LEAVECRITICAL;
    }

    return S_OK;
}


// BugBug:: this assumes not rooted
STDAPI SHGetIDispatchForFolder(LPCITEMIDLIST pidl, IWebBrowserApp **ppauto)
{
    HRESULT hres = E_UNEXPECTED;

    if (ppauto)
        *ppauto = NULL;

    if (!pidl)
        return E_POINTER;

    // Try a cached psw if we don't need ppauto
    IShellWindows* psw = WinList_GetShellWindows(ppauto != NULL);
    if (psw)
    {
        VARIANT var;
        LONG lhwnd;
        if (InitVariantFromIDList(&var, pidl)) 
        {
            IDispatch* pdisp;
            hres = psw->FindWindow(&var, PVAREMPTY, SWC_BROWSER, &lhwnd,
                    ppauto ? (SWFO_NEEDDISPATCH | SWFO_INCLUDEPENDING) : SWFO_INCLUDEPENDING,
                    &pdisp);
            if ((hres == E_PENDING) || (hres == S_FALSE))
            {
                HRESULT hresOld = hres;
                hres = E_FAIL;
                CGIDFWait *pdfwait = new CGIDFWait();   // Setup a wait object...
                if (pdfwait)
                {
                    if (pdfwait->Init(psw, pidl, 0))
                    {
                        if (hresOld == S_FALSE)
                        {
                            // Startup opening a new window
                            SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};

                            sei.lpIDList = (void *)pidl;

                            //
                            //  WARNING - old versions of ShellExec() didnt pay attention - ZekeL - 30-DEC-98
                            //  to whether the hwnd is in the same process or not, 
                            //  and so could fault in TryDDEShortcut().
                            //  only pass the hwnd if the shell window shares 
                            //  the same process.
                            //
                            sei.hwnd = GetShellWindow();
                            DWORD idProcess;
                            GetWindowThreadProcessId(sei.hwnd, &idProcess);
                            if (idProcess != GetCurrentProcessId())
                                sei.hwnd = NULL;

                            // Everything should have been initialize to NULL(0)
                            sei.fMask = SEE_MASK_IDLIST | SEE_MASK_FLAG_DDEWAIT;
                            sei.nShow = SW_SHOWNORMAL;

                            hres = ShellExecuteEx(&sei) ? NOERROR : S_FALSE;
                        }

                        while ((hres = psw->FindWindow(&var, PVAREMPTY, SWC_BROWSER, &lhwnd,
                                ppauto ? (SWFO_NEEDDISPATCH | SWFO_INCLUDEPENDING) : SWFO_INCLUDEPENDING,
                                &pdisp)) != S_OK)
                        {
                            if (FAILED(pdfwait->WaitForWindowToOpen(20 * 1000)))
                            {
                                hres = E_ABORT;
                                break;
                            }
                        }
                    }
                    pdfwait->CleanUp();   // No need to watch things any more...
                    pdfwait->Release(); // release our use of this object...
                }
            }
            if (hres == S_OK && ppauto) 
            {
                // if this fails this is because we are inside SendMessage loop
                hres = pdisp->QueryInterface(IID_IWebBrowserApp, (void **)ppauto);
            }
            if (pdisp)
                pdisp->Release();
            VariantClear(&var);
        }
        psw->Release();
    }
    return hres;
}

#ifdef POSTPOSTSPLIT

// at this point, this code should be loaded from browseui.dll

#else

#undef VariantCopy

WINOLEAUTAPI VariantCopyLazy(VARIANTARG * pvargDest, VARIANTARG * pvargSrc)
{
    VariantClearLazy(pvargDest);

    switch(pvargSrc->vt) {
    case VT_I4:
    case VT_UI4:
    case VT_BOOL:
        // we can add more
        *pvargDest = *pvargSrc;
        return S_OK;

    case VT_UNKNOWN:
        if (pvargDest) {
            *pvargDest = *pvargSrc;
            pvargDest->punkVal->AddRef();
            return S_OK;
        }
        ASSERT(0);
        return E_INVALIDARG;
    }

    return VariantCopy(pvargDest, pvargSrc);
}

//
// WARNING: This function must be placed at the end because we #undef
// VariantClear
//
#undef VariantClear

HRESULT VariantClearLazy(VARIANTARG *pvarg)
{
    switch(pvarg->vt) 
    {
        case VT_I4:
        case VT_UI4:
        case VT_EMPTY:
        case VT_BOOL:
            // No operation
            break;

        case VT_UNKNOWN:
            if(V_UNKNOWN(pvarg) != NULL)
              V_UNKNOWN(pvarg)->Release();
            break;

        case VT_DISPATCH:
            if(V_DISPATCH(pvarg) != NULL)
              V_DISPATCH(pvarg)->Release();
            break;

        case VT_SAFEARRAY:
            THR(SafeArrayDestroy(V_ARRAY(pvarg)));
            break;

        default:
            return VariantClear(pvarg);
    }

    V_VT(pvarg) = VT_EMPTY;
    return S_OK;
}


#endif

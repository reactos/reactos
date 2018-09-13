#include "stdafx.h"
#pragma hdrstop

#ifdef POSTSPLIT

#include "cmnquery.h"
#include "dsquery.h"
#include "startids.h"
#include "dsgetdc.h"
#include "lm.h"
#include "wab.h"
#include "winldap.h"
#include "activeds.h"


// This is the implementation for the Shell Application level IDispatch
// Currently we will try to maintain only one object per process.
// BUGBUG:: The following defines must be equal to the stuff in cabinet.h...

#define IDM_SYSBUTTON   300
#define IDM_FINDBUTTON  301
#define IDM_HELPBUTTON  302
#define IDM_FILERUN                 401
#define IDM_CASCADE                 403
#define IDM_HORIZTILE               404
#define IDM_VERTTILE                405
#define IDM_DESKTOPARRANGEGRID      406
#define IDM_ARRANGEMINIMIZEDWINDOWS 407
#define IDM_SETTIME                 408
#define IDM_SUSPEND                 409
#define IDM_EJECTPC                 410
#define IDM_TASKLIST                412
#define IDM_TRAYPROPERTIES          413
#define IDM_EDITSTARTMENU           414
#define IDM_MINIMIZEALL             415
#define IDM_UNDO                    416
#define IDM_RETURN                  417
#define IDM_PRINTNOTIFY_FOLDER      418
#define IDM_MINIMIZEALLHOTKEY       419
#define IDM_SHOWTASKMAN             420
#define IDM_RECENT              501
#define IDM_FIND                502
#define IDM_PROGRAMS            504
#define IDM_CONTROLS            505
#define IDM_EXITWIN             506
// #define IDM_FONTS            509
#define IDM_PRINTERS            510
#define IDM_STARTMENU           511
#define IDM_MYCOMPUTER          512
#define IDM_PROGRAMSINIT        513
#define IDM_RECENTINIT          514
#define IDM_MENU_FIND           520
#define TRAY_IDM_FINDFIRST      521  // this range
#define TRAY_IDM_FINDLAST       550  // is reserved for find command
#define IDM_RECENTLIST          650
#define IDM_QUICKTIPS   800
#define IDM_HELPCONT    801
#define IDM_WIZARDS     802
#define IDM_USEHELP     803             // REVIEW: probably won't be used
#define IDM_TUTORIAL    804
#define IDM_ABOUT       805
#define IDM_LAST_MENU_ITEM   IDM_ABOUT
#define FCIDM_FIRST             FCIDM_GLOBALFIRST
#define FCIDM_LAST              FCIDM_BROWSERLAST
//#define FCIDM_FINDFILES         (FCIDM_BROWSER_TOOLS+0x0005)
#define FCIDM_FINDCOMPUTER      (FCIDM_BROWSER_TOOLS+0x0006)

//============================================================================

class CShellDispatch : public IShellDispatch2, 
                       public CObjectSafety,
                       protected CImpIDispatch,
                       public CObjectWithSite
{
   friend class CAdviseRouter;
   friend HRESULT GetApplicationObject(DWORD dwSafetyOptions, IUnknown *punkSite, IDispatch **ppid);

    public:

        //Non-delegating object IUnknown
        STDMETHODIMP         QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IDispatch members
        virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
            { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
        virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
            { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
        virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
            { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
        virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
            { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

        //IShellDispatch functions
        STDMETHODIMP get_Application(IDispatch **ppid);
        STDMETHODIMP get_Parent (IDispatch **ppid);
        STDMETHOD(Open)(THIS_ VARIANT vDir);
        STDMETHOD(Explore)(THIS_ VARIANT vDir);
        STDMETHOD(NameSpace)(THIS_ VARIANT vDir, Folder **ppsdf);
        STDMETHODIMP BrowseForFolder(long Hwnd, BSTR Title, long Options, VARIANT RootFolder, Folder **ppsdf);
        STDMETHODIMP ControlPanelItem(BSTR szDir);
        STDMETHODIMP MinimizeAll(void);
        STDMETHODIMP UndoMinimizeALL(void);
        STDMETHODIMP FileRun(void);
        STDMETHODIMP CascadeWindows(void);
        STDMETHODIMP TileVertically(void);
        STDMETHODIMP TileHorizontally(void);
        STDMETHODIMP ShutdownWindows(void);
        STDMETHODIMP Suspend(void);
        STDMETHODIMP EjectPC(void);
        STDMETHODIMP SetTime(void);
        STDMETHODIMP TrayProperties(void);
        STDMETHODIMP Help(void);
        STDMETHODIMP FindFiles(void);
        STDMETHODIMP FindComputer(void);
        STDMETHODIMP RefreshMenu(void);
        STDMETHODIMP Windows(IDispatch **ppid);
        STDMETHODIMP get_ObjectCount(int *pcObjs);
        STDMETHODIMP IsRestricted(BSTR Group, BSTR Restriction, long * lpValue);
        STDMETHODIMP ShellExecute(BSTR File, VARIANT vArgs, VARIANT vDir, VARIANT vOperation, VARIANT vShow);
        STDMETHODIMP FindPrinter(BSTR name, BSTR location, BSTR model);
        STDMETHODIMP GetSystemInformation(BSTR name, VARIANT * pvOut);
        STDMETHODIMP ServiceStart(BSTR ServiceName, VARIANT Persistent, VARIANT *pSuccess);
        STDMETHODIMP ServiceStop(BSTR ServiceName, VARIANT Persistent, VARIANT *pSuccess);
        STDMETHODIMP IsServiceRunning(BSTR ServiceName, VARIANT *pRunning);
        STDMETHODIMP CanStartStopService(BSTR ServiceName, VARIANT *pCanStartStop);
        STDMETHODIMP ShowBrowserBar( BSTR bstrClsid, VARIANT bShow, VARIANT *pSuccess );
                
        // Constructor and the like.. 
        CShellDispatch(void);
    protected:
        ULONG           m_cRef;             //Object reference count
    
        ~CShellDispatch(void);
        BOOL            FAllowUserToDoAnything(void);   // Check if we are in paranoid mode...
        HRESULT         _TrayCommand(UINT idCmd);
        HRESULT         ExecuteFolder(VARIANT vDir, LPCTSTR pszVerb);
        VARIANT_BOOL    _ServiceStartStop(BSTR ServiceName, BOOL fStart, BOOL fPersist);

        HMODULE         m_hmodWAB;          // module handle for WAB32 
        LPWABOPEN       _GetWABOpen();
        HRESULT         _GetNC(BSTR *pbstrResult, LPCTSTR pszDnsForestName);

};


STDAPI CShellDispatch_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppvOut = NULL;
    TraceMsg(DM_TRACE, "CSD_CreateInstance called");

    // aggregation checking is handled in class factory
    CShellDispatch * pshd = new CShellDispatch();
    if (pshd)
    {
        hr = pshd->QueryInterface(riid, ppvOut);
        pshd->Release();   // g_pCShellDispatch doesn't want a ref.
    }

    return hr;
}

HRESULT GetApplicationObject(DWORD dwSafetyOptions, IUnknown *punkSite, IDispatch **ppid)
{
    *ppid = NULL;   // Handle failure cases.

    if (dwSafetyOptions && (!punkSite || IsSafePage(punkSite) != S_OK))
        return E_ACCESSDENIED;

    HRESULT hres = CShellDispatch_CreateInstance(NULL, IID_IDispatch, (void **) ppid);
    if (SUCCEEDED(hres))
    {
        if (punkSite)
            IUnknown_SetSite(*ppid, punkSite);

        if (dwSafetyOptions && SUCCEEDED(hres))
            hres = MakeSafeForScripting((IUnknown**)ppid);
    }

    return hres;
}

CShellDispatch::CShellDispatch(void) :
        m_cRef(1), 
        m_hmodWAB(NULL),
        CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_IShellDispatch2)
{
    DllAddRef();
    TraceMsg(DM_TRACE, "CShellDispatch::CShellDispatch called");
}


CShellDispatch::~CShellDispatch(void)
{
    TraceMsg(DM_TRACE, "CShellDispatch::~CShellDispatch called");
    
    if ( m_hmodWAB )
        FreeLibrary(m_hmodWAB);

    DllRelease();
}

BOOL  CShellDispatch::FAllowUserToDoAnything(void)
{
    if (!_dwSafetyOptions)
        return TRUE;   // We are not running in safe mode so say everything is OK...   

    if (!_punkSite)
        return FALSE;   // We are in safe but don't have anyway to know where we are...
    return (IsSafePage(_punkSite) == S_OK);

}

STDMETHODIMP CShellDispatch::QueryInterface(REFIID riid, void ** ppv)
{
    TraceMsg(DM_TRACE, "CShellDispatch::QueryInterface called");
    static const QITAB qit[] = {
        QITABENT(CShellDispatch, IShellDispatch2),
        QITABENTMULTI(CShellDispatch, IShellDispatch, IShellDispatch2),
        QITABENTMULTI(CShellDispatch, IDispatch, IShellDispatch2),
        QITABENT(CShellDispatch, IObjectSafety),
        QITABENT(CShellDispatch, IObjectWithSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}


STDMETHODIMP_(ULONG) CShellDispatch::AddRef(void)
{
    TraceMsg(DM_TRACE, "CShellDispatch::AddRef called");
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CShellDispatch::Release(void)
{
    TraceMsg(DM_TRACE, "CShellDispatch::Release called");
    if (0L!=--m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}

// Helper function to process commands to the tray.
HRESULT CShellDispatch::_TrayCommand(UINT idCmd)
{
    if (!FAllowUserToDoAnything())
        return E_ACCESSDENIED;

    HWND hwndTray = FindWindowA(WNDCLASS_TRAYNOTIFY, NULL);
    if (hwndTray)
        PostMessage(hwndTray, WM_COMMAND, idCmd, 0);
    return NOERROR;
}

STDMETHODIMP CShellDispatch::get_Application(IDispatch **ppid)
{
    // Simply return ourself
    return QueryInterface(IID_IDispatch, (PVOID*)ppid);
}

STDMETHODIMP CShellDispatch::get_Parent(IDispatch **ppid)
{
    return QueryInterface(IID_IDispatch, (PVOID*)ppid);
}

HRESULT CShellDispatch::ExecuteFolder(VARIANT vDir, LPCTSTR pszVerb)
{
    SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};  

    // Check to see if we allow the user to do this...
    if (!FAllowUserToDoAnything())
        return E_ACCESSDENIED;

    sei.lpIDList = (void *)VariantToIDList(&vDir);
    if (sei.lpIDList)
    {
        // Everything should have been initialize to 0
        // BUGBUG:: Should be invoke idlist but that is failing when
        // explore
        sei.fMask = SEE_MASK_IDLIST;
        sei.nShow = SW_SHOWNORMAL;
        sei.lpVerb = pszVerb;

        //TraceMsg(DM_TRACE, "CShellDispatch::Open(%s) called", szParam);

        HRESULT hres = ShellExecuteEx(&sei) ? NOERROR : S_FALSE;

        ILFree((LPITEMIDLIST)sei.lpIDList);

        return hres;
    }
    return S_FALSE; // bad dir
}


STDMETHODIMP CShellDispatch::Open(VARIANT vDir)
{
    return ExecuteFolder(vDir, NULL);
}

STDMETHODIMP CShellDispatch::Explore(VARIANT vDir)
{
    return ExecuteFolder(vDir, TEXT("explore"));
}

STDMETHODIMP CShellDispatch::NameSpace(VARIANT vDir, Folder **ppsdf)
{
    *ppsdf = NULL;

    if (!FAllowUserToDoAnything())
        return E_ACCESSDENIED;

    HRESULT hres;
    LPITEMIDLIST pidl = VariantToIDList(&vDir);
    if (pidl)
    {
        hres = CFolder_Create(NULL, pidl, NULL, IID_Folder, (void **)ppsdf);
        ILFree(pidl);
    }
    else
        hres = S_FALSE; // bad dir
    return hres;
}

STDMETHODIMP CShellDispatch::IsRestricted(BSTR Group, BSTR Restriction, long * lpValue)
{
    if (lpValue) 
        *lpValue = SHGetRestriction(NULL, Group, Restriction);
    return S_OK;
}

STDMETHODIMP CShellDispatch::ShellExecute(BSTR File, VARIANT vArgs, VARIANT vDir, VARIANT vOperation, VARIANT vShow)
{
    SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};  
    TCHAR szFile[MAX_PATH];
    TCHAR szDir[MAX_PATH];
    TCHAR szOper[128];  // don't think any verb longer than this...

    // Check to see if we allow the user to do this...
    if (!FAllowUserToDoAnything())
        return E_ACCESSDENIED;

    // Initialize the shellexecute structure...

    sei.nShow = SW_SHOWNORMAL;

    // Ok setup the FileName.
    SHUnicodeToTCharCP(CP_ACP, File, szFile, ARRAYSIZE(szFile));
    sei.lpFile = szFile;

    // Now the Args
    sei.lpParameters = VariantToStr(&vArgs, NULL, 0);
    sei.lpDirectory = VariantToStr(&vDir, szDir, ARRAYSIZE(szDir));
    sei.lpVerb = VariantToStr(&vOperation, szOper, ARRAYSIZE(szOper));

    // Finally the show -- Could use convert, but that takes 3 calls...
    if (vShow.vt == (VT_BYREF|VT_VARIANT) && vShow.pvarVal)
        vShow = *vShow.pvarVal;
    switch (vShow.vt)
    {
    case VT_I2:
        sei.nShow = (int)vShow.iVal;
        break;

    case VT_I4:
        sei.nShow = (int)vShow.lVal;
    }

    HRESULT hres = ShellExecuteEx(&sei) ? NOERROR : S_FALSE;

    // Cleanup anything we allocated
    if (sei.lpParameters)
        LocalFree((HLOCAL)sei.lpParameters);

    return hres;

}
//
// These next few methods deal with NT services in general, and the
// Content Indexing Service in particular, so they're stubbed out
// to return E_NOTIMPL on Win9x.
//

//
// Helper function for ServiceStart and ServiceStop
//
VARIANT_BOOL CShellDispatch::_ServiceStartStop(BSTR ServiceName, BOOL fStart, BOOL fPersistent)
{
#ifdef WINNT
    SC_HANDLE hSvc = 0;
    SC_HANDLE hSc = 0;
    BOOL fResult = FALSE;
    VARIANT_BOOL fRetVal = VARIANT_FALSE;
    SERVICE_STATUS ServiceStatus;

    hSc = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSc) {
        goto exit_gracefully;
    } // if

    hSvc = OpenServiceW(
        hSc,
        ServiceName,
        (fStart ? SERVICE_START : SERVICE_STOP) 
            | (fPersistent ? SERVICE_CHANGE_CONFIG : 0)
    );
    if (!hSvc) {
        goto exit_gracefully;
    } // if
        
    if (fPersistent) {

        fResult = ChangeServiceConfig(
            hSvc,
            SERVICE_NO_CHANGE,
            (fStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START),
            SERVICE_NO_CHANGE,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
        );
        
        //
        // Consider failure here a soft error.
        //

    } // if (fPersistent)

    if (fStart) {
        fResult = StartService(hSvc, 0, NULL);
    }
    else {
        fResult = ControlService(hSvc, SERVICE_CONTROL_STOP, &ServiceStatus);
    }
    if (!fResult) {
        goto exit_gracefully;
    } // if

    fRetVal = VARIANT_TRUE;

exit_gracefully:
    if (hSvc) {
        CloseServiceHandle(hSvc);
        hSvc = 0;
    } // if
    if (hSc) {
        CloseServiceHandle(hSc);
        hSc = 0;
    } // if

    return(fRetVal);
#else // WINNT
    return(FALSE);
#endif // WINNT
}

STDMETHODIMP CShellDispatch::ServiceStart(BSTR ServiceName, VARIANT Persistent, VARIANT *pSuccess)
{
#ifdef WINNT
    // Check to see if we allow the user to do this...
    if (!FAllowUserToDoAnything())
        return E_ACCESSDENIED;

    if (VT_BOOL != Persistent.vt) {
        return(E_INVALIDARG);
    } // if

    VariantClear(pSuccess);
    pSuccess->vt = VT_BOOL;
    pSuccess->boolVal = _ServiceStartStop(ServiceName, TRUE, Persistent.boolVal);

    return(S_OK);
#else // WINNT
    return(E_NOTIMPL);
#endif // WINNT
}

STDMETHODIMP CShellDispatch::ServiceStop(BSTR ServiceName, VARIANT Persistent, VARIANT *pSuccess)
{
#ifdef WINNT
    // Check to see if we allow the user to do this...
    if (!FAllowUserToDoAnything())
        return E_ACCESSDENIED;

    if (VT_BOOL != Persistent.vt) {
        return(E_INVALIDARG);
    } // if

    VariantClear(pSuccess);
    pSuccess->vt = VT_BOOL;
    pSuccess->boolVal = _ServiceStartStop(ServiceName, FALSE, Persistent.boolVal);

    return(S_OK);
#else // WINNT
    return(E_NOTIMPL);
#endif // WINNT
}

STDMETHODIMP CShellDispatch::IsServiceRunning(BSTR ServiceName, VARIANT *pIsRunning)
{
#ifdef WINNT
    SC_HANDLE hSvc = 0;
    SC_HANDLE hSc = 0;
    SERVICE_STATUS ServiceStatus;
    BOOL fResult = FALSE;

    VariantClear(pIsRunning);
    pIsRunning->vt = VT_BOOL;
    pIsRunning->boolVal = VARIANT_FALSE;

    // Check to see if we allow the user to do this...
    if (!FAllowUserToDoAnything())
        return E_ACCESSDENIED;

    hSc = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSc) {
        goto exit_gracefully;
    } // if

    hSvc = OpenService(
        hSc,
        ServiceName,
        SERVICE_QUERY_STATUS
    );
    if (!hSvc) {
        goto exit_gracefully;
    } // if
        
    fResult = QueryServiceStatus(hSvc, &ServiceStatus);
    if (!fResult) {
        goto exit_gracefully;
    } // if

    switch (ServiceStatus.dwCurrentState) {
        case SERVICE_START_PENDING:
        case SERVICE_RUNNING:
        case SERVICE_CONTINUE_PENDING:
            pIsRunning->boolVal = VARIANT_TRUE;
            break;

        default:
            break;

    } // switch

exit_gracefully:

    if (hSc) {
        CloseServiceHandle(hSc);
        hSc = 0;
    } // if
    if (hSvc) {
        CloseServiceHandle(hSvc);
        hSvc = 0;
    } // if

    return(S_OK);
#else // WINNT
    return(E_NOTIMPL);
#endif

}

STDMETHODIMP CShellDispatch::CanStartStopService(BSTR ServiceName, VARIANT *pCanStartStop)
{
#ifdef WINNT
    SC_HANDLE hSvc = 0;
    SC_HANDLE hSc = 0;
    BOOL fResult = FALSE;

    VariantClear(pCanStartStop);
    pCanStartStop->vt = VT_BOOL;
    pCanStartStop->boolVal = VARIANT_FALSE;

    // Check to see if we allow the user to do this...
    if (!FAllowUserToDoAnything())
        return E_ACCESSDENIED;

    hSc = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSc) {
        goto exit_gracefully;
    } // if

    hSvc = OpenService(
        hSc,
        ServiceName,
        SERVICE_START | SERVICE_STOP | SERVICE_CHANGE_CONFIG
    );
    if (!hSvc) {
        DWORD dwErr = GetLastError() ;
        goto exit_gracefully;
    } // if
        
    pCanStartStop->boolVal = VARIANT_TRUE;

exit_gracefully:

    if (hSc) {
        CloseServiceHandle(hSc);
        hSc = 0;
    } // if
    if (hSvc) {
        CloseServiceHandle(hSvc);
        hSvc = 0;
    } // if

    return(S_OK);
#else // WINNT
    return(E_NOTIMPL);
#endif // WINNT

}

STDMETHODIMP CShellDispatch::ShowBrowserBar(BSTR bstrClsid, VARIANT varShow, VARIANT *pSuccess)
{
    if( !(bstrClsid && *bstrClsid && pSuccess) )
        return E_INVALIDARG ;

    HRESULT hr        = E_FAIL ;
    pSuccess->vt      = VT_BOOL ;
    pSuccess->boolVal = VARIANT_FALSE ;

    if( NULL == _punkSite )
        return E_FAIL ;

    IShellBrowser* psb ;
    hr = IUnknown_QueryService( _punkSite, SID_STopLevelBrowser, IID_IShellBrowser, (void **)&psb ) ;

    if( SUCCEEDED( hr ) ) 
    {
        IWebBrowser2 *pb2;
        hr = IUnknown_QueryService( psb, SID_SWebBrowserApp, IID_IWebBrowser2, 
                                   (void **)&pb2);
        if (SUCCEEDED(hr))
        {
            VARIANT varGuid, varNil ;
        
            varGuid.vt      = VT_BSTR ;
            varGuid.bstrVal = bstrClsid ;
            VariantInit( &varNil ) ;

            hr = pb2->ShowBrowserBar( &varGuid, &varShow, &varNil ) ;
            
            if( SUCCEEDED( hr ) )
                pSuccess->boolVal = VARIANT_TRUE ;
        
            pb2->Release() ;
        }
        psb->Release() ;
    }

    return hr ;
}

STDMETHODIMP CShellDispatch::BrowseForFolder(long Hwnd, BSTR Title, long Options, 
        VARIANT RootFolder, Folder **ppsdf)
{
    // BUGBUG:: Not all of the arguments are being processed yet...
    TCHAR szTitle[MAX_PATH];      // hopefully long enough...
    BROWSEINFO browse;
    LPITEMIDLIST pidl;
    HRESULT hres;

    if (!FAllowUserToDoAnything())
        return E_ACCESSDENIED;

    SHUnicodeToTCharCP(CP_ACP, Title, szTitle, ARRAYSIZE(szTitle));
    browse.lpszTitle = szTitle;

    browse.pszDisplayName = NULL;
    browse.hwndOwner = (HWND)LongToHandle( Hwnd );
    browse.ulFlags = (ULONG)Options;
    browse.lpfn = NULL;
    browse.lParam = 0;
    browse.iImage = 0;
    browse.pidlRoot = VariantToIDList(&RootFolder);

    pidl = SHBrowseForFolder(&browse);

    if (browse.pidlRoot)
        ILFree((LPITEMIDLIST)browse.pidlRoot);

    *ppsdf = NULL;
    if (pidl)
    {
        hres = CFolder_Create(NULL, pidl, NULL, IID_Folder, (void **)ppsdf);
        ILFree(pidl);
    }
    else
        hres = S_FALSE;     // Not a strong error...
    return hres;
}

STDMETHODIMP CShellDispatch::ControlPanelItem(BSTR bszDir)
{
    if (!FAllowUserToDoAnything())
        return E_ACCESSDENIED;
#ifdef UNICODE
    SHRunControlPanel(bszDir, NULL);

#else // UNICODE
    TCHAR szParam[MAX_PATH];

    SHUnicodeToAnsi(bszDir, szParam, ARRAYSIZE(szParam));
    SHRunControlPanel(szParam, NULL);
    TraceMsg(DM_TRACE, "CShellDispatch::ControlPanelItem(%s) called", szParam);
#endif // UNICODE

    return NOERROR;
}

STDMETHODIMP CShellDispatch::MinimizeAll(void)
{
    return _TrayCommand(IDM_MINIMIZEALL);
}

STDMETHODIMP CShellDispatch::UndoMinimizeALL(void)
{
    return _TrayCommand(IDM_UNDO);
}

STDMETHODIMP CShellDispatch::FileRun(void)
{
    return _TrayCommand(IDM_FILERUN);
}

STDMETHODIMP CShellDispatch::CascadeWindows(void)
{
    return _TrayCommand(IDM_CASCADE);
}

STDMETHODIMP CShellDispatch::TileVertically(void)
{
    return _TrayCommand(IDM_VERTTILE);
}

STDMETHODIMP CShellDispatch::TileHorizontally(void)
{
    return _TrayCommand(IDM_HORIZTILE);
}

STDMETHODIMP CShellDispatch::ShutdownWindows(void)
{
    return _TrayCommand(IDM_EXITWIN);
}

STDMETHODIMP CShellDispatch::Suspend(void)
{
    return _TrayCommand(IDM_SUSPEND);
}

STDMETHODIMP CShellDispatch::EjectPC(void)
{
    return _TrayCommand(IDM_EJECTPC);
}


STDMETHODIMP CShellDispatch::SetTime(void)
{
    return _TrayCommand(IDM_SETTIME);
}

STDMETHODIMP CShellDispatch::TrayProperties(void)
{
    return _TrayCommand(IDM_TRAYPROPERTIES);
}

STDMETHODIMP CShellDispatch::Help(void)
{
    return _TrayCommand(IDM_HELPSEARCH);
}

STDMETHODIMP CShellDispatch::FindFiles(void)
{
    return _TrayCommand(FCIDM_FINDFILES);
}

STDMETHODIMP CShellDispatch::FindComputer(void)
{
    return _TrayCommand(FCIDM_FINDCOMPUTER);
}

STDMETHODIMP CShellDispatch::RefreshMenu(void)
{
    return _TrayCommand(FCIDM_REFRESH);
}

STDMETHODIMP CShellDispatch::Windows(IDispatch **ppid)
{
    if (!FAllowUserToDoAnything())
        return E_ACCESSDENIED;
    return CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_LOCAL_SERVER, IID_IDispatch, (void **)ppid);
}


//
// the "FindPrinter" method on the application object invokes the DS query to find a printer given
// the name, location and model.  Because the query UI is a blocking API we spin this onto a seperate
// thread before calling "OpenQueryWindow".
//

typedef struct 
{
    LPWSTR pszName;
    LPWSTR pszLocation;
    LPWSTR pszModel;
} FINDPRINTERINFO;

void _FreeFindPrinterInfo(FINDPRINTERINFO *pfpi)
{
    if ( pfpi )
    {
        Str_SetPtrW(&pfpi->pszName, NULL);
        Str_SetPtrW(&pfpi->pszLocation, NULL);
        Str_SetPtrW(&pfpi->pszModel, NULL);
        LocalFree(pfpi);               // free the parameters we were given
    }
}

HRESULT _SetStrToPropertyBag(IPropertyBag *ppb, LPCWSTR pszProperty, LPCWSTR pszValue)
{
    VARIANT variant = { 0 };

    V_VT(&variant) = VT_BSTR;
    V_BSTR(&variant) = SysAllocString(pszValue);

    if ( !V_BSTR(&variant) )
        return E_OUTOFMEMORY;

    HRESULT hres = ppb->Write(pszProperty, &variant);
    SysFreeString(V_BSTR(&variant));    

    return hres;
}

HRESULT _GetPrintPropertyBag(FINDPRINTERINFO *pfpi, IPropertyBag **pppb)
{
    HRESULT hres = S_OK;
    IPropertyBag *ppb = NULL;

    // if we have properties that need to be passed then lets package them up
    // into a property bag.

    if ( pfpi->pszName || pfpi->pszLocation || pfpi->pszModel )
    {
        hres = SHCreatePropertyBag(IID_IPropertyBag, (void **)&ppb);
        if ( SUCCEEDED(hres) )
        {
            if ( pfpi->pszName )
                hres = _SetStrToPropertyBag(ppb, L"printName", pfpi->pszName);

            if ( pfpi->pszLocation && SUCCEEDED(hres) )
                hres = _SetStrToPropertyBag(ppb, L"printLocation", pfpi->pszLocation);

            if ( pfpi->pszModel && SUCCEEDED(hres) )
                hres = _SetStrToPropertyBag(ppb, L"printModel", pfpi->pszModel);
        }
    }

    if ( FAILED(hres) && ppb )
        ppb->Release();
    else
        *pppb = ppb;

    return hres;
}

DWORD WINAPI _FindPrinterThreadProc(void *ptp)
{
    FINDPRINTERINFO *pfpi = (FINDPRINTERINFO*)ptp;

    ICommonQuery *pcq;
    if (SUCCEEDED(CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (void **)&pcq)))
    {
        OPENQUERYWINDOW oqw = { 0 };

        oqw.cbStruct = SIZEOF(oqw);
        oqw.dwFlags = OQWF_DEFAULTFORM | OQWF_REMOVEFORMS | OQWF_PARAMISPROPERTYBAG;
        oqw.clsidHandler = CLSID_DsQuery;
        oqw.clsidDefaultForm = CLSID_DsFindPrinter;
    
        if ( SUCCEEDED(_GetPrintPropertyBag(pfpi, &oqw.ppbFormParameters)) )
            pcq->OpenQueryWindow(NULL, &oqw, NULL);

        if ( oqw.pFormParameters )
            oqw.ppbFormParameters->Release();

        pcq->Release();
    }

    _FreeFindPrinterInfo(pfpi);

    return 0;
}

STDMETHODIMP CShellDispatch::FindPrinter(BSTR name, BSTR location, BSTR model)
{
    if (!FAllowUserToDoAnything())
        return E_ACCESSDENIED;

    // bundle the parameters to pass over to the bg thread which will issue the query

    FINDPRINTERINFO *pfpi = (FINDPRINTERINFO*)LocalAlloc(LPTR, SIZEOF(FINDPRINTERINFO));
    if ( !pfpi )
        return E_OUTOFMEMORY;

    if ( Str_SetPtrW(&pfpi->pszName, name) && 
         Str_SetPtrW(&pfpi->pszLocation, location) && 
         Str_SetPtrW(&pfpi->pszModel, model) )
    {
        if (SHCreateThread(_FindPrinterThreadProc, pfpi, CTF_PROCESS_REF | CTF_COINIT, NULL))
        {
            pfpi = NULL;            // thread owns
        }
    }

    // either close the thread handle, or release the parameter block.   we assume
    // that if the thread was created it will handle discarding the block.

    if (pfpi)
        _FreeFindPrinterInfo(pfpi);

    return S_OK;
}


#ifdef WINNT

// NT4 returns a good processor level in wProcessorLevel
#define _GetProcessorLevelFromSystemInfo(pinfo) (pinfo)->wProcessorLevel

#else

// Win95 doesn't support SYSTEM_INFO.wProcessorLevel so we have
// to do it the hard way.  Win98 does support it, but that's scant
// consolation.

UINT _GetProcessorLevelFromSystemInfo(SYSTEM_INFO *pinfo)
{
    UINT uiLevel = 0;

    switch (pinfo->dwProcessorType)
    {
    case PROCESSOR_INTEL_386:
        uiLevel = 3;
        break;

    case PROCESSOR_INTEL_486:
        uiLevel = 4;

    // We'll assume that everything Pentium or better supports CPUID
    // But just in case, we'll wrap this inside a try/except.
    default:
        __try {

            // The CPUID instruction trashes EBX but the compiler doesn't know
            // that so we have to trash it explicitly
            _asm {
                xor eax, eax
                inc eax
                _emit 0x0F          // CPUID
                _emit 0xA2
                xor ebx, ebx        // so compiler doesn't assume ebx is preserved
                and ah, 15          // Processor level returned in low nibble of ah
                mov byte ptr uiLevel, ah
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) { }
        break;
    }
    return uiLevel;
}
#endif

STDMETHODIMP CShellDispatch::GetSystemInformation(BSTR bstrName, VARIANT * pvOut)
{
    if (!FAllowUserToDoAnything())
        return E_ACCESSDENIED;
    
    TCHAR szName[MAX_PATH];
    SHUnicodeToTCharCP(CP_ACP, bstrName, szName, ARRAYSIZE(szName));

    if (!lstrcmpi(szName, TEXT("DirectoryServiceAvailable")))
    {
        pvOut->vt = VT_BOOL;
        V_BOOL(pvOut) = GetEnvironmentVariable(TEXT("USERDNSDOMAIN"), NULL, 0) > 0;
        return S_OK;
    }
    else if (!lstrcmpi(szName, TEXT("DoubleClickTime")))
    {
        pvOut->vt = VT_UI4;
        V_UI4(pvOut) = GetDoubleClickTime();
        return S_OK;
    }
    else if (!lstrcmpi(szName, TEXT("ProcessorLevel")))
    {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        pvOut->vt = VT_I4;
        V_UI4(pvOut) = _GetProcessorLevelFromSystemInfo(&info);
        return S_OK;
    }
    else if (!lstrcmpi(szName, TEXT("ProcessorSpeed")))
    {
        HKEY hkey;
        if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                          TEXT("Hardware\\Description\\System\\CentralProcessor\\0"), 
                                          0, KEY_READ, &hkey))
            return E_FAIL;

        DWORD dwValue = 0;
        DWORD cb = sizeof( dwValue );

        if (ERROR_SUCCESS != SHQueryValueEx(hkey,
                    TEXT("~Mhz"),
                    NULL,
                    NULL,
                    (LPBYTE) &dwValue,
                    &cb) == ERROR_SUCCESS) 
        {       
            RegCloseKey(hkey);
            return E_FAIL;
        }
        RegCloseKey(hkey);
        pvOut->vt = VT_I4;
        V_UI4(pvOut) = dwValue;
        return S_OK;
    }
    else if (!lstrcmpi(szName, TEXT("ProcessorArchitecture")))
    {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        pvOut->vt = VT_I4;
        V_UI4(pvOut) = info.wProcessorArchitecture;
        return S_OK;
    }
    else if (!lstrcmpi(szName, TEXT("PhysicalMemoryInstalled")))
    {
        MEMORYSTATUSEX MemoryStatus;
        MemoryStatus.dwLength = SIZEOF(MEMORYSTATUSEX);
        NT5_GlobalMemoryStatusEx(&MemoryStatus);
        pvOut->vt = VT_R8;
        V_R8(pvOut) = (double)(signed __int64) MemoryStatus.ullTotalPhys;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}



#endif // POSTSPLIT 

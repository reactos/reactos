#include "stdafx.h"
#pragma hdrstop

#ifdef POSTSPLIT

//#include "sdspatch.h"
//#include "..\ids.h"
//#include "dutil.h"
//#include <shsemip.h>

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

class CShellDispatch : public IShellDispatch, 
                       public CObjectSafety,
                       protected CImpIDispatch,
                       public CObjectWithSite
{
   friend class CAdviseRouter;
   friend HRESULT GetApplicationObject(DWORD dwSafetyOptions, IUnknown *punkSite, IDispatch **ppid);

    public:

        //Non-delegating object IUnknown
        STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
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

        // Constructor and the like.. 
        CShellDispatch(void);
    protected:
            ULONG           m_cRef;             //Object reference count
    
        ~CShellDispatch(void);
        BOOL            FAllowUserToDoAnything(void);   // Check if we are in paranoid mode...
        HRESULT         _TrayCommand(UINT idCmd);
        HRESULT         ExecuteFolder(VARIANT vDir, LPCTSTR pszVerb);
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

    if (dwSafetyOptions && (!punkSite || LocalZoneCheck(punkSite) != S_OK))
        return E_ACCESSDENIED;

    HRESULT hres = CShellDispatch_CreateInstance(NULL, IID_IDispatch, (LPVOID *) ppid);
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
        CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_IShellDispatch)
{
    DllAddRef();
    TraceMsg(DM_TRACE, "CShellDispatch::CShellDispatch called");
}


CShellDispatch::~CShellDispatch(void)
{
    TraceMsg(DM_TRACE, "CShellDispatch::~CShellDispatch called");
    DllRelease();
}

BOOL  CShellDispatch::FAllowUserToDoAnything(void)
{
    
    if (!_dwSafetyOptions)
        return TRUE;   // We are not running in safe mode so say everything is OK...   

    if (!_punkSite)
        return FALSE;   // We are in safe but don't have anyway to know where we are...
    return (LocalZoneCheck(_punkSite) == S_OK);

}

STDMETHODIMP CShellDispatch::QueryInterface(REFIID riid, LPVOID* ppv)
{
    TraceMsg(DM_TRACE, "CShellDispatch::QueryInterface called");
    static const QITAB qit[] = {
        QITABENT(CShellDispatch, IShellDispatch),
        QITABENTMULTI(CShellDispatch, IDispatch, IShellDispatch),
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

    sei.lpIDList = (LPVOID)VariantToIDList(&vDir);
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

    LPITEMIDLIST pidl = VariantToIDList(&vDir);
    if (pidl)
    {
        CSDFolder *psdf;
        HRESULT hres = CSDFolder_Create(NULL, pidl, NULL, &psdf);
        if (SUCCEEDED(hres))
        {
            hres = psdf->QueryInterface(IID_Folder, (LPVOID *)ppsdf);
            psdf->Release();
        }
        ILFree(pidl);
        return hres;
    }
    return S_FALSE; // bad dir
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
    browse.hwndOwner = (HWND)Hwnd;
    browse.ulFlags = (ULONG)Options;
    browse.lpfn = NULL;
    browse.lParam = 0;
    browse.iImage = 0;
    browse.pidlRoot = VariantToIDList(&RootFolder);

    pidl = SHBrowseForFolder(&browse);

    if (browse.pidlRoot)
        ILFree((LPITEMIDLIST)browse.pidlRoot);

    *ppsdf = NULL;
    if (!pidl)
        return S_FALSE;     // Not a strong error...

    CSDFolder *psdf;
    hres = CSDFolder_Create(NULL, pidl, NULL, &psdf);
    if (SUCCEEDED (hres))
    {
        hres = psdf->QueryInterface(IID_Folder, (LPVOID *)ppsdf);
        psdf->Release();
    }
    ILFree(pidl);

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

    SHUnicodeToAnsiCP(CP_ACP, bszDir, szParam, ARRAYSIZE(szParam));
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
    return _TrayCommand(DVIDM_HELPSEARCH);
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
    return CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_LOCAL_SERVER, IID_IDispatch, (LPVOID*)ppid);
}

#endif // POSTSPLIT 

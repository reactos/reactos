#include "private.h"
#include "ssseprox.h"
#include <shlguid.h>

#define TF_THISMODULE TF_WEBCHECKCORE

DWORD   g_idSchedThread = 0;

// global containing pointer to instance of CWebcheck.  Needed to control
// externals loading on demand.
CWebCheck *g_pwc = NULL;

//////////////////////////////////////////////////////////////////////////
//
// CWebCheck implementation
//
//////////////////////////////////////////////////////////////////////////

CWebCheck::CWebCheck()
{
    // Maintain global object count
    DllAddRef();

    // Initialize object
    m_cRef = 1;

    // save our instance
    g_pwc = this;
}

CWebCheck::~CWebCheck()
{
    // Maintain global object count
    DllRelease();

    // no longer available
    g_pwc = NULL;
}

//
// IUnknown members
//

STDMETHODIMP_(ULONG) CWebCheck::AddRef(void)
{
//  TraceMsg(TF_THISMODULE, "CWebCheck::AddRef m_cRef=%d", m_cRef+1);

    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CWebCheck::Release(void)
{
//  TraceMsg(TF_THISMODULE, "CWebCheck::Release m_cRef=%d", m_cRef-1);

    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CWebCheck::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;

    // Validate requested interface
    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = (IUnknown *)this;
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *ppv = (IOleCommandTarget *)this;
    else
        return E_NOINTERFACE;

    // Addref through the interface
    ((LPUNKNOWN)*ppv)->AddRef();
    return S_OK;
}

//
// IOleCommandTarget members
// The shell will send notifications to us through this interface.
//

STDMETHODIMP CWebCheck::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
                                    OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    if (IsEqualGUID(*pguidCmdGroup, CGID_ShellServiceObject))
    {
        // We like Shell Service Object notifications...
        return S_OK;
    }

    return(OLECMDERR_E_UNKNOWNGROUP);
}

STDMETHODIMP CWebCheck::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
                             DWORD nCmdexecopt, VARIANTARG *pvaIn,
                             VARIANTARG *pvaOut)
{
    if (pguidCmdGroup && IsEqualGUID(*pguidCmdGroup, CGID_ShellServiceObject))
    {
        // Handle Shell Service Object notifications here.
        switch (nCmdID)
        {
            case SSOCMDID_OPEN:
                StartService(FALSE);
                break;

            case SSOCMDID_CLOSE:
                StopService();
                break;
        }
        return S_OK;
    }
    
    return(E_NOTIMPL);
}


//
// IWebCheck members
//

// Starts the webcheck service in a process
STDMETHODIMP CWebCheck::StartService(BOOL fForceExternals)
{
    DBG("CWebCheck::StartService entered");

    // reset offline mode for all platforms except NT5
    if(FALSE == g_fIsWinNT5)
    {
        HMODULE hWininet = GetModuleHandle(TEXT("WININET.DLL"));
        if(hWininet)
        {
            // wininet is loaded - tell it to go online
            INTERNET_CONNECTED_INFO ci;
            memset(&ci, 0, sizeof(ci));
            ci.dwConnectedState = INTERNET_STATE_CONNECTED;
            InternetSetOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));
        } 
        else 
        {
            // wininet not loaded - blow away offline reg key so we'll
            // be online when it does load
            DWORD dwOffline = 0;        // FALSE => not offline
            WriteRegValue(HKEY_CURRENT_USER, c_szRegPathInternetSettings,
                TEXT("GlobalUserOffline"), &dwOffline, sizeof(DWORD), REG_DWORD);
        }
    }

    // create dialmon window
    DialmonInit();

    // Fire up LCE and sens if necessary
    if(fForceExternals || ShouldLoadExternals())
        LoadExternals();

    //
    // Process the Infodelivery Admin Policies on user login.  (User login coincides
    // with webcheck's StartService() call.)
    //
    ProcessInfodeliveryPolicies();

    // Initialize the Channel Screen Saver hlink proxy.
    if (g_fIsWinNT)
        InitSEProx();
    
    DBG("CWebCheck::StartService exiting");
    return S_OK;
}


// Stops Webcheck if running.
STDMETHODIMP CWebCheck::StopService(void)
{
    DBG("CWebCheck::StopService entered");

    // kill dialmon window
    DialmonShutdown();

    // shut down the external bits
    if(FALSE == g_fIsWinNT)
        UnloadExternals();

    // Release resources for the Channel Screen Saver hlink proxy.
    if (g_fIsWinNT)
        FreeSEProx();

    DBG("CWebCheck::StopService exiting");
    return S_OK;
}

//
// load behavior: (win9x)
//
// "auto"   Load if on a laptop
// "yes"    Load always
// "no"     Load never
//
static const WCHAR s_szAuto[] = TEXT("auto");
static const WCHAR s_szYes[] = TEXT("yes");
static const WCHAR s_szNo[] = TEXT("no");

BOOL CWebCheck::ShouldLoadExternals(void)
{
    WCHAR   szSens[16], szLce[16];
    DWORD   cbData;

    //
    // don't load on NT
    //
    if(g_fIsWinNT)
    {
        DBG("CWebCheck::ShouldLoadExternals -> NO (NT)");
        return FALSE;
    }

    //
    // read sens/lce user settings - no setting means auto
    //
    cbData = sizeof(szLce);
    if(ERROR_SUCCESS != SHGetValueW(HKEY_LOCAL_MACHINE, c_szRegKey, L"LoadLCE", NULL, szLce, &cbData))
    {
        StrCpyW(szLce, s_szAuto);
    }

    cbData = sizeof(szSens);
    if(ERROR_SUCCESS != SHGetValueW(HKEY_LOCAL_MACHINE, c_szRegKey, L"LoadSens", NULL, szSens, &cbData))
    {
        StrCpyW(szSens, s_szAuto);
    }

    //
    // if either is yes, load
    //
    if(0 == StrCmpIW(szLce, s_szYes) || 0 == StrCmpIW(szSens, s_szYes))
    {
        DBG("CWebCheck::ShouldLoadExternals -> YES (reg = yes)");
        return TRUE;
    }

    //
    // if either is auto, check for laptop
    //
    if(0 == StrCmpIW(szLce, s_szAuto) || 0 == StrCmpIW(szSens, s_szAuto))
    {
        if(SHGetMachineInfo(GMI_LAPTOP))
        {
            // Is a laptop - load
            DBG("CWebCheck::ShouldLoadExternals -> YES (reg = auto, laptop)");
            return TRUE;
        }
    }

    // don't load
    DBG("CWebCheck::ShouldLoadExternals -> NO");
    return FALSE;
}

BOOL CWebCheck::AreExternalsLoaded(void)
{
    return (_hThread != NULL);
}

void CWebCheck::LoadExternals(void)
{
    DWORD dwThreadId;

    DBG("CWebCheck::LoadExternals");

    if(_hThread)
    {
        DBG("CWebCheck::LoadExternals - already loaded");
        return;
    }

    // create initializion and termination events
    _hInitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    _hTerminateEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(NULL == _hInitEvent || NULL == _hTerminateEvent) {
        DBG("LoadExternals failed to create termination event");
        return;
    }

    // fire up a thread to do the work
    _hThread = CreateThread(NULL, 4096, ExternalsThread, this, 0, &dwThreadId);
    if(NULL == _hThread) {
        DBG("LoadExternals failed to create externals thread!");
        return;
    }

    // wait for initialization event to trigger then free it
    WaitForSingleObject(_hInitEvent, INFINITE);
    CloseHandle(_hInitEvent);
    _hInitEvent = NULL;

    DBG("CWebCheck::LoadExternals exiting");
    return;
}

void CWebCheck::UnloadExternals(void)
{
    if(NULL == _hThread)
    {
        DBG("CWebCheck::UnloadExternals - nothing to unload");
        return;
    }

    // tell externals thread to go away by setting termination event
    SetEvent(_hTerminateEvent);

    // Give thread a 10 second grace period to shut down
    // don't really care if it goes away or not... our process is going away!
    WaitForSingleObject(_hThread, 10000);

    // clean up
    CloseHandle(_hThread);
    CloseHandle(_hTerminateEvent);
    _hThread = NULL;
    _hTerminateEvent = NULL;

    return;
}

DWORD WINAPI ExternalsThread(LPVOID lpData)
{
    CWebCheck * pWebCheck = (CWebCheck *)lpData;
    HINSTANCE hLCE, hSENS = NULL;
    BOOL fLCEStarted = FALSE, fSENSStarted = FALSE;
    DWORD dwRet;
    MSG msg;

    // fire up com
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if(FAILED(hr)) {
        DBG("LoadExternals: Failed to initialize COM");
        SetEvent(pWebCheck->_hInitEvent);
        return 0;
    }

    // load and start LCE
    hLCE = LoadLibrary(TEXT("estier2.dll"));
    DBGASSERT(hLCE, "LoadExternals: Failed to load estier2.dll");
    if(hLCE) {
        LCESTART startfunc;
        startfunc = (LCESTART)GetProcAddress(hLCE, "LCEStartServer");
        DBGASSERT(startfunc, "LoadExternals: Failed to find LCEStartServer");
        if(startfunc) {
            hr = startfunc();
            if(SUCCEEDED(hr))
                fLCEStarted = TRUE;
            DBGASSERT(fLCEStarted, "LoadExternals: Failed to start LCE");
        }
    }

    // if LCE started sucessfully, load and start SENS
    if(fLCEStarted) {
        hSENS = LoadLibrary(TEXT("sens.dll"));
        DBGASSERT(hSENS, "LoadExternals: Failed to load sens.dll");
        if(hSENS) {
            SENSSTART startfunc;
            startfunc = (SENSSTART)GetProcAddress(hSENS, "SensInitialize");
            DBGASSERT(startfunc, "LoadExternals: Failed to find SensInitialize");
            if(startfunc) {
                if(startfunc())
                    fSENSStarted = TRUE;
                DBGASSERT(fSENSStarted, "LoadExternals: Failed to start SENS");
            }
        }
    }

    // trigger initialization event to tell caller we're up and running
    SetEvent(pWebCheck->_hInitEvent);

    // Wait for our shutdown event but pump messages in the mean time
    do {
        dwRet = MsgWaitForMultipleObjects(1, &(pWebCheck->_hTerminateEvent),
                    FALSE, INFINITE, QS_ALLINPUT);
        if(WAIT_OBJECT_0 == dwRet) {
            // got our event, drop out of do loop
            break;
        }

        // empty the message queue...
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    } while(TRUE);

    // shut down SENS
    if(fSENSStarted) {
        ASSERT(hSENS);
        SENSSTOP stopfunc;
        stopfunc = (SENSSTOP)GetProcAddress(hSENS, "SensUninitialize");
        if(stopfunc) {
            stopfunc();
        }
    }

    //
    // [darrenmi] beta-1 hack: Sens may have a thread sitting in its code
    // at this point so it's not safe to unload sens.  Since we're in the
    // process of shutting down anyway, just leave it alone and let the 
    // system unload it.
    //
    //if(hSENS) {
    //    FreeLibrary(hSENS);
    //}

    // shut down LCE
    if(fLCEStarted) {
        ASSERT(hLCE)
        LCESTOP stopfunc;
        stopfunc = (LCESTOP)GetProcAddress(hLCE, "LCEStopServer");
        if(stopfunc) {
            stopfunc();
        }
    }

    if(hLCE) {
        FreeLibrary(hLCE);
    }

    // clean up com goo
    CoUninitialize();

    return 0;
}

#if 0
//  TODO: need similar functionality in the new world
void SetNotificationMgrRestrictions(INotificationProcessMgr0 *pNotfMgrProcess)
{
    HRESULT hr;
    INotificationProcessMgr0 *pNotProcess = pNotfMgrProcess;

    // get NotificationMgr if it wasn't passed in
    if (!pNotfMgrProcess)
    {
        hr = CoCreateInstance(CLSID_StdNotificationMgr,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_INotificationProcessMgr0,
                          (void**)&pNotProcess);
        DBGASSERT(SUCCEEDED(hr), "SetNotificationMgrRestrictions - failed to create notification mgr");
    }
    // set the restrictions
    if (pNotProcess)
    {
        const TCHAR c_szNoScheduledUpdates[] = TEXT("NoScheduledUpdates");
        THROTTLEITEM ti = {0};
        ti.NotificationType = NOTIFICATIONTYPE_AGENT_START;
        ti.nParallel = 3;
        
        // Has the user has disabled scheduled subscription updates?
        DWORD dwData;
        DWORD cbData = sizeof(dwData);
        if ((ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, c_szRegKey, c_szNoScheduledUpdates, NULL, &dwData, &cbData))
            && dwData)
        {
            ti.dwFlags |= TF_DONT_DELIVER_SCHEDULED_ITEMS;
        }
        
        // Has the administrator has disabled scheduled subscription updates?
        if (SHRestricted2W(REST_NoScheduledUpdates, NULL, 0))
        {
            ti.dwFlags |= TF_DONT_DELIVER_SCHEDULED_ITEMS;
        }
        
        // Has the administrator has excluded scheduled subscription updates
        // from this time range?
        DWORD dwBegin = SHRestricted2W(REST_UpdateExcludeBegin, NULL, 0);
        DWORD dwEnd = SHRestricted2W(REST_UpdateExcludeEnd, NULL, 0);
        if (dwBegin && dwEnd)
        {
            ti.dwFlags |= TF_APPLY_EXCLUDE_RANGE;
            ti.stBegin.wHour = (WORD)(dwBegin / 60);
            ti.stBegin.wMinute = (WORD)(dwBegin % 60);
            ti.stEnd.wHour = (WORD)(dwEnd / 60);
            ti.stEnd.wMinute = (WORD)(dwEnd %60);
        }

        // Has the admin set a minimum interval for scheduled subscription updates?
        dwData = SHRestricted2W(REST_MinUpdateInterval, NULL, 0);
        if (dwData)
        {
            ti.dwFlags |= TF_APPLY_UPDATEINTERVAL;
            ti.dwMinItemUpdateInterval = dwData;
        }
        
        hr = pNotProcess->RegisterThrottleNotificationType(1, &ti, 0, NULL, 0, 0);
        DBGASSERT(SUCCEEDED(hr), "SetNotificationMgrRestrictions - failed to register throttle type & restrictions");
    }
    // release NotificationMgr if it wasn't passed in
    if (!pNotfMgrProcess)
    {
        SAFERELEASE(pNotProcess);
    }
}

#endif


//
// OLE bypass code
//
// Expose a couple of APIs to call start and stop service so loadwc doesn't
// need to load up OLE at start time.
//

HRESULT
ExtStartService(
    BOOL    fForceExternals
    )
{
    HRESULT hr = E_FAIL;

    // make a webcheck object
    ASSERT(NULL == g_pwc);
    if(NULL == g_pwc)
    {
        g_pwc = new CWebCheck;
        if(g_pwc)
        {
            hr = g_pwc->StartService(fForceExternals);
        }
    }

    return hr;
}

HRESULT
ExtStopService(
    void
    )
{
    HRESULT hr = E_FAIL;

    if(g_pwc)
    {
        hr = g_pwc->StopService();
        SAFERELEASE(g_pwc);
    }

    return hr;
}

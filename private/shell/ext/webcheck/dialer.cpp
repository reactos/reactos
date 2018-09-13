#include "private.h"
#include "offline.h"

#undef TF_THISMODULE
#define TF_THISMODULE TF_DIALMON


// prototypes
DIALPROPDATA * InitDialData(void);

//
// uuid for our command target
//
const UUID CGID_ConnCmdGrp = { 0x1dc1fd0, 0xdc49, 0x11d0, {0xaf, 0x95, 0x00, 0xc0, 0x4f, 0xd9, 0x40, 0xbe} };

//
// Registry keys we use to get autodial information
//
                                                                     
// Key name
const TCHAR c_szAutodial[]  = TEXT("EnableAutodial");
const TCHAR c_szProxy[]     = TEXT("ProxyServer");

const TCHAR c_szUnknown[]   = TEXT("<unknown>");

// from schedule.cpp
extern TCHAR szInternetSettings[];
extern TCHAR szProxyEnable[];

// length of general text strings
#define TEXT_LENGTH     200

// our connection agent instance
CConnectionAgent *pAgent = NULL;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
//                 Connection Agent class factory helper
//
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

HRESULT CConnectionAgent_CreateInstance(LPUNKNOWN pUnkOuter, IUnknown **ppunk)
{
    IUnknown            *punk, *punkClient = NULL;
    HRESULT             hr;
    CConnClient         *pClient;

    *ppunk = NULL;

    // create new con client instance
    pClient = new CConnClient;

    if(NULL == pClient)
        return E_OUTOFMEMORY;

    // Look for CConnectionAgent in ROT
    hr = GetActiveObject(CLSID_ConnectionAgent, NULL, &punk);
    if (NULL == punk) {
        // Not there - create one
        ASSERT(NULL == pAgent);
        pAgent = new CConnectionAgent;

        if(NULL == pAgent) {
            pClient->Release();
            return E_OUTOFMEMORY;
        }

        // Get an IUnknown on new object
        hr = pAgent->QueryInterface(IID_IUnknown, (LPVOID *)&punk);
        if (FAILED(hr) || NULL == punk) {
            SAFERELEASE(pAgent);
            pClient->Release();
            return E_FAIL;
        }

        // Register new connection agent
        hr = RegisterActiveObject(punk, CLSID_ConnectionAgent,
                    ACTIVEOBJECT_STRONG, &pAgent->m_dwRegisterHandle);
        SAFERELEASE(pAgent);
        if (FAILED(hr))
        {
            DBG_WARN("CConnectionAgentClassFactory RegisterActiveObject failed.");
            pClient->Release();
            punk->Release();
            return hr;
        }
        DBG("New connection agent object created.");
    }
#ifdef DEBUG
    else
    {
        DBG("Using existing connection agent object.");
    }
#endif

    // Now we have pClient and punk.  Tell client who the boss is
    hr = pClient->SetConnAgent(punk);
    punk->Release();
    if(FAILED(hr)) {
        pClient->Release();
        return E_FAIL;
    }

    *ppunk = (IOleCommandTarget *)pClient;

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
//                          Helper functions
//
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
//                             Ping Class
//
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// helper stuff
//
#define IS_DIGIT(ch)    InRange(ch, TEXT('0'), TEXT('9'))

void UnloadICMP(void);

long        g_lPingWndReg = 0;
const TCHAR c_szPingWndClass[] = TEXT("PingClass");

#define WM_NAME     (WM_USER)
#define WM_STOP     (WM_USER+1)

class CPing                  
{
protected:
    HANDLE  _hPing;
    HANDLE  _hAsync;
    HWND    _hwnd;
    UINT    _uTimerID;
    BOOL    _fResult;
    UINT    _uTimeoutSec;

    BOOL    EnableAutodial(BOOL fEnable);

public:
    CPing();
    ~CPing();

    BOOL    Init(UINT uTimeoutSec);
    BOOL    PingSite(LPTSTR pszSite);

static LRESULT CPing::WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

};

CPing::CPing()
{
    _hPing = INVALID_HANDLE_VALUE;
    _hwnd = NULL;
    _hAsync = NULL;
    _uTimeoutSec = 10;
}

CPing::~CPing()
{
    if(_hwnd)
        DestroyWindow(_hwnd);

    if(_hPing)
        IcmpCloseHandle(_hPing);

    UnloadICMP();
}

BOOL
CPing::Init(UINT uTimeoutSec)
{
    // save timeout
    _uTimeoutSec = uTimeoutSec;

    // load ICMP.DLL and get a ping handle
    _hPing = IcmpCreateFile();
    if(INVALID_HANDLE_VALUE == _hPing)
        return FALSE;

    // register window class if necessary
    if(!g_lPingWndReg) {
        g_lPingWndReg++;
        WNDCLASS wc;

        wc.style = 0;
        wc.lpfnWndProc = CPing::WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = g_hInst;
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = (HBRUSH)NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = c_szPingWndClass;

        RegisterClass(&wc);
    }

    if(NULL == _hwnd)
        _hwnd = CreateWindow(c_szPingWndClass, NULL, WS_OVERLAPPED,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                NULL, NULL, g_hInst, (LPVOID)this);

    if(NULL == _hwnd)
        return FALSE;

    return TRUE;
}

LRESULT
CPing::WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    CPing *pping = (CPing *)GetWindowLong(hWnd, GWL_USERDATA);
    LPCREATESTRUCT pcs = NULL;

    switch(Msg) {
    case WM_CREATE:
        pcs = (LPCREATESTRUCT)lParam;
        SetWindowLong(hWnd, GWL_USERDATA, ((LONG) (pcs->lpCreateParams)));
        break;
    case WM_NAME:
        // gethostbyname completed
        if(0 == WSAGETASYNCERROR(lParam))
            pping->_fResult = TRUE;

        // fall through to WM_TIMER

    case WM_TIMER:
        // we ran out of time
        PostMessage(hWnd, WM_STOP, 0, 0);
        break;
    default:
        return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    return 0;
}

BOOL
CPing::EnableAutodial(BOOL fEnable)
{
    DWORD   dwNewState = 0, dwOldState = 0, dwSize = sizeof(DWORD);
    BOOL    fOldEnable = FALSE;

    if(g_fIsWinNT) {

        //
        // In NT land, 1 means disabled, 0 means enabled
        //

        // Get WinNT autodial state
        _RasGetAutodialParam(RASADP_LoginSessionDisable, &dwOldState, &dwSize);
        if(0 == dwOldState) fOldEnable = TRUE;

        // set new state
        if(FALSE == fEnable) dwNewState = 1;
        _RasSetAutodialParam(RASADP_LoginSessionDisable, &dwNewState, sizeof(DWORD));

    } else {

        //
        // In Win95 land, 1 means enabled, 0 means disabled
        //

        // Get Win95 autodial state
        if(!ReadRegValue(HKEY_CURRENT_USER, szInternetSettings, c_szAutodial,
                    &dwOldState, sizeof(DWORD)))
            dwOldState = 0;
        if(dwOldState) fOldEnable = TRUE;

        // set new state
        if(fEnable) dwNewState = 1;
        WriteRegValue(HKEY_CURRENT_USER, szInternetSettings, c_szAutodial,
            &dwNewState, sizeof(DWORD), REG_BINARY);
    }

    return fOldEnable;
}

BOOL
CPing::PingSite(LPTSTR pszHost)
{
    IPAddr          ipAddress = INADDR_NONE;
    DWORD           dwPingSend = 0xdeadbeef, dwCount;
    WSADATA         wsaData;
    BOOL            fOldState;
    TCHAR           pszWork[1024], *pEnd;
    BYTE            pGetHostBuff[MAXGETHOSTSTRUCT];
    int             iErr;

    // assume failure
    _fResult = FALSE;

    if(INVALID_HANDLE_VALUE == _hPing) {
        DBG("CPing::PingSite no ICMP handle");
        return FALSE;
    }

    // fire up winsock
    if(iErr = WSAStartup(0x0101, &wsaData)) {
        TraceMsg(TF_THISMODULE,"CPing::PingSite WSAStartup failed, iErr=%d", iErr);
        return FALSE;
    }
 
    if(IS_DIGIT(*pszHost)) {
        // try to convert ip address
        ipAddress = inet_addr(pszHost);
    }

    // turn off autodial
    fOldState = EnableAutodial(FALSE);

    if(INADDR_NONE == ipAddress) {

        // strip port (if any) from host name
        lstrcpyn(pszWork, pszHost, ARRAYSIZE(pszWork));//BUGBUG-FIXED-OVERFLOW
        pEnd = StrChr(pszWork, TEXT(':'));
        if(pEnd)
            *pEnd = 0;

        // start async gethostbyname
        _uTimerID = SetTimer(_hwnd, 1, _uTimeoutSec * 1000, NULL);
        _hAsync = WSAAsyncGetHostByName(_hwnd, WM_NAME, pszWork,
                    (char *)pGetHostBuff, MAXGETHOSTSTRUCT);

        if(_hAsync) {
            // operation started... wait for completion or time out
            MSG msg;
            while(1) {
                GetMessage(&msg, _hwnd, 0, 0);
                if(msg.message == WM_STOP)
                    break;
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } /* while */

            if(_fResult) {
                // it worked, snarf address
                struct hostent *phe;
                phe = (struct hostent *)pGetHostBuff;
                memcpy(&ipAddress, phe->h_addr, sizeof(IPAddr));
            } else {
                // If we timed out, clean up pending request
                WSACancelAsyncRequest(_hAsync);
            }

#ifdef DEBUG
        } else {
            // operation failed to start
            iErr = WSAGetLastError();
            TraceMsg(TF_THISMODULE, "CPing::PingSite WSAAsyncGetHostByName failed, error=%d", iErr);
#endif
        }

        // kill the timer
        if(_uTimerID) {
            KillTimer(_hwnd, _uTimerID);
            _uTimerID = 0;
        }
    }

    // assume the ping will fail
    _fResult = FALSE;

    if(INADDR_NONE != ipAddress) {
        // try to ping that address
        dwCount = IcmpSendEcho(
            _hPing,
            ipAddress,
            &dwPingSend,           
            sizeof(DWORD),
            NULL,
            pszWork,
            sizeof(pszWork),
            _uTimeoutSec * 1000);

        if(dwCount) {
            // ping succeeded!!
            _fResult = TRUE;
#ifdef DEBUG
        } else {
            // didn't work - spew
            iErr = GetLastError();
            TraceMsg(TF_THISMODULE, "CPing::PingSite IcmpSendEcho failed, error=%x", iErr);
#endif
        }
    }

    // restore autodial
    EnableAutodial(fOldState);

#ifdef DEBUG
    if(_fResult)
        TraceMsg(TF_THISMODULE, "CPing::PingSite ping <%s> success", pszHost);
    else
        TraceMsg(TF_THISMODULE, "CPing::PingSite ping <%s> FAILURE", pszHost);
#endif

    WSACleanup();
    return _fResult;
}

//
// PingProxy - exported function to decide if the proxy is available or not
//
BOOL PingProxy(UINT uTimeoutSec)
{
    BOOL    fRet = FALSE;
    TCHAR   pszProxy[TEXT_LENGTH], *pszHttp, *pszSemi;
    DWORD   dwValue;

    // check for proxy enabled
    if(ReadRegValue(HKEY_CURRENT_USER, szInternetSettings, szProxyEnable,
            &dwValue, sizeof(DWORD))) {
        if(0 == dwValue)
            return FALSE;
    }

    // proxy is enabled in registry.  Ping it to see if it's around.
    if(ReadRegValue(HKEY_CURRENT_USER, szInternetSettings, c_szProxy,
            pszProxy, TEXT_LENGTH)) {

        // if there's an '=' in the proxy string, we'll look for the http= proxy
        if(NULL != StrChr(pszProxy, '=')) {
            pszHttp = StrStrI(pszProxy, TEXT("http="));
            if(NULL == pszHttp)
                // don't understand proxy string
                return FALSE;
            pszHttp += 5;       // 5 chars in "http="

            // remove following entries - they're separated by ;
            pszSemi = StrChr(pszHttp, ';');
            if(pszSemi)
                *pszSemi = 0;
        } else {
            pszHttp = pszProxy;
        }

        // got a proxy, crack the host name out of it
        TCHAR *pszPingSite;
        URL_COMPONENTS comp;
        ZeroMemory(&comp, sizeof(comp));
        comp.dwStructSize = sizeof(comp);
        comp.dwHostNameLength = 1;

        if(InternetCrackUrlA(pszHttp, 0, 0, &comp) && (comp.nScheme != INTERNET_SCHEME_UNKNOWN)) {
            pszPingSite = comp.lpszHostName;
            pszPingSite[comp.dwHostNameLength] = 0;
        } else {
            pszPingSite = pszHttp;
        }

        // ping it
        CPing ping;
        if(ping.Init(uTimeoutSec))
            fRet = ping.PingSite(pszPingSite);
    }

    return fRet;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
//                      Connection Client object
//
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// Constructor / Destructor
//
CConnClient::CConnClient()
{
    m_cRef = 1;
    m_poctAgent = NULL;
    m_pReport = NULL;
    m_State = CLIENT_NEW;
    m_bstrURL = NULL;
}

//////////////////////////////////////////////////////////////////////////

//
// IUnknown members
//
STDMETHODIMP CConnClient::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;

    // Validate requested interface
    if ((IID_IUnknown == riid) ||
        (IID_INotificationSink == riid))
    {
        *ppv=(INotificationSink*)this;
    } else if(IID_IOleCommandTarget == riid) {
        *ppv=(IOleCommandTarget*)this;
    } else {
        return E_NOINTERFACE;
    }

    ((LPUNKNOWN)*ppv)->AddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CConnClient::AddRef(void)
{
    TraceMsg(TF_THISMODULE, "CConnClient::Addref (%08x) m_cRef=%d", this, m_cRef+1);
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CConnClient::Release(void)
{
    TraceMsg(TF_THISMODULE, "CConnClient::Release (%08x) m_cRef=%d", this, m_cRef-1);
    if( 0L != --m_cRef )
        return m_cRef;

    DBG("CConClient::Release Bye Bye");

    // Make sure we're disconnected
    Disconnect();

    m_poctAgent->Release();

    if(m_bstrURL)
        SysFreeString(m_bstrURL);

    delete this;
    return 0L;
}

//////////////////////////////////////////////////////////////////////////

//
// CConClient helper functions
//
HRESULT CConnClient::SetConnAgent(IUnknown *punk)
{
    return punk->QueryInterface(IID_IOleCommandTarget, (void **)&m_poctAgent);
}

HRESULT CConnClient::Connect()
{
    HRESULT     hr;
    VARIANTARG  vin, vout;

    m_State = CLIENT_CONNECTING;

    // tell agent we want to connect
    vin.vt = VT_UNKNOWN;
    vin.punkVal = (IOleCommandTarget *)this;
    VariantInit(&vout);
    hr = m_poctAgent->Exec(&CGID_ConnCmdGrp, AGENT_CONNECT, 0, &vin, &vout);
    if(SUCCEEDED(hr)) {
        ASSERT(vout.vt == VT_I4);
        m_iCookie = vout.lVal;
    }

    return hr;
}

HRESULT CConnClient::Disconnect()
{
    HRESULT     hr;
    VARIANTARG  vin;

    if(CLIENT_DISCONNECTED == m_State)
        return S_OK;

    m_State = CLIENT_DISCONNECTED;

    // tell agent we want to disconnect
    vin.vt = VT_I4;
    vin.ulVal = m_iCookie;
    hr = m_poctAgent->Exec(&CGID_ConnCmdGrp, AGENT_DISCONNECT, 0, &vin, NULL);

    // done with report pointer
    SAFERELEASE(m_pReport);

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// INotificationSink members
//
STDMETHODIMP CConnClient::OnNotification(
        LPNOTIFICATION         pNotification,
        LPNOTIFICATIONREPORT   pNotificationReport,
        DWORD                  dwReserved
        )
{
    NOTIFICATIONTYPE    nt;
    HRESULT             hr = S_OK;

    hr = pNotification->GetNotificationInfo(&nt, NULL,NULL,NULL,0);

    if(FAILED(hr)) {
        DBG_WARN("CConnClient::OnNotification failed to get not type!");
        return E_INVALIDARG;
    }

    if(IsEqualGUID(nt, NOTIFICATIONTYPE_AGENT_START)) {

        DBG("CConnClient::OnNotification AGENT_START");

        if(CLIENT_NEW == m_State) {

            // Must have a report pointer!
            if(NULL == pNotificationReport) {
                DBG("CConnClient::OnNotification no report on START!");
                return E_UNEXPECTED;
            }

            // save report pointer
            TraceMsg(TF_THISMODULE, "CConClient::OnNotification (%08x) addreffing report pointer", this);
            m_pReport = pNotificationReport;
            m_pReport->AddRef();

            // get the URL
            hr = ReadBSTR(pNotification, NULL, c_szPropURL, &m_bstrURL);

            // convert to ansi and log url connection request
            TCHAR pszURL[INTERNET_MAX_URL_LENGTH];

            MyOleStrToStrN(pszURL, INTERNET_MAX_URL_LENGTH, m_bstrURL);
            LogEvent("Connecting for <%s>", pszURL);

            // Tell agent to connect
            Connect();

        } else {
            DBG("CConnClient::OnNotification unexpected connect");
            return E_UNEXPECTED;
        }

    } else if(IsEqualGUID(nt, NOTIFICATIONTYPE_TASKS_COMPLETED)) {
        DBG("CConnClient::OnNotification TASKS_COMPLETED");

        // convert url to ansi
        TCHAR pszURL[INTERNET_MAX_URL_LENGTH];
        MyOleStrToStrN(pszURL, INTERNET_MAX_URL_LENGTH, m_bstrURL);

        switch(m_State) {
        case CLIENT_CONNECTING:
            m_State = CLIENT_ABORT;

            // log connection abort
            LogEvent("Aborting connection for <%s>", pszURL);

            break;
        case CLIENT_CONNECTED:
            // log disconnect
            LogEvent("Disconnecting for <%s>", pszURL);

            Disconnect();
            break;
        default:
            DBG("CConnClient::OnNotification unexpected disconnect");
            return E_UNEXPECTED;
        }
    } else {
        DBG("CConnClient::OnNotification unknown type");
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// IOleCommandTarget members
//
STDMETHODIMP CConnClient::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
                                    OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    if (IsEqualGUID(*pguidCmdGroup, CGID_ConnCmdGrp)) {
        return S_OK;
    }

    return OLECMDERR_E_UNKNOWNGROUP;
}

STDMETHODIMP CConnClient::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
                             DWORD nCmdexecopt, VARIANTARG *pvaIn,
                             VARIANTARG *pvaOut)
{
    HRESULT hr = E_NOTIMPL;

    if (pguidCmdGroup &&  IsEqualGUID(*pguidCmdGroup, CGID_ConnCmdGrp))
    {
        switch(nCmdID) {
        case AGENT_NOTIFY:
            if(VT_ERROR == pvaIn->vt) {
                hr = DeliverProgressReport(pvaIn->scode, NULL);
            } else {
                hr = E_INVALIDARG;
            }
            break;
        }
    }
    
    return hr;
}

//////////////////////////////////////////////////////////////////////////

//
// Other methods
//
HRESULT CConnClient::DeliverProgressReport(SCODE scode, BSTR bstrErrorText)
{
    HRESULT             hr = S_OK;
    INotificationMgr    *pMgr;
    INotification       *pStatus;

    switch(m_State) {
    case CLIENT_CONNECTING:
        // Get Notification manager
        hr = CoCreateInstance(CLSID_StdNotificationMgr, NULL,
                CLSCTX_INPROC_SERVER, IID_INotificationMgr, (void**)&pMgr);
        if(FAILED(hr))
            return hr;

        // create notification to deliver
        hr = pMgr->CreateNotification(NOTIFICATIONTYPE_PROGRESS_REPORT,
            (NOTIFICATIONFLAGS)0, NULL, &pStatus, 0);
        pMgr->Release();
        if(FAILED(hr))
            return hr;

        // stick result and string in progress report
//      WriteOLESTR(pStatus, NULL, c_szPropStatusString, bstrErrorText);
        WriteSCODE(pStatus, NULL, c_szPropStatusCode, scode);

        // deliver notification
        hr = m_pReport->DeliverUpdate(pStatus, 0, 0);
        pStatus->Release();

        if(SUCCEEDED(scode)) {
            m_State = CLIENT_CONNECTED;
        } else {
            Disconnect();
        }
        break;
    case CLIENT_ABORT:
        Disconnect();
        break;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
//                          Connection agent
//
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// Constructor and destructor
//
CConnectionAgent::CConnectionAgent()
{
    m_pData = NULL;
    m_cRef = 1;
    m_dwRegisterHandle = 0;

    m_lConnectionCount = 0;
    m_dwFlags = 0;
    m_hdpaClient = NULL;

    // Get the notification manager
    m_pMgr = NULL;
    CoCreateInstance(CLSID_StdNotificationMgr, NULL,
                CLSCTX_INPROC_SERVER, IID_INotificationMgr, (void**)&m_pMgr);
}

CConnectionAgent::~CConnectionAgent()
{
    SAFERELEASE(m_pMgr);
    SAFELOCALFREE(m_pData);

    if(IsFlagSet(m_dwFlags, CA_LOADED_RAS))
        UnloadRasDLL();
}

//
// Clean - clean up strong lock and ref count
//
void
CConnectionAgent::Clean(void)
{
    DWORD   dwHandle = m_dwRegisterHandle;
    int     iCurReport, i;
    CLIENTINFO *pClient;

    // don't do anything if there are outstanding connections
    if(m_lConnectionCount)
        return;

    // clean up client dpa
    if(m_hdpaClient) {
        iCurReport = DPA_GetPtrCount(m_hdpaClient);
        for(i=0; i<iCurReport; i++) {
            pClient = (CLIENTINFO *)(DPA_GetPtr(m_hdpaClient, i));
            if(pClient)
                delete pClient;
        }
        DPA_Destroy(m_hdpaClient);
        m_hdpaClient = NULL;
    }

    // release our strong registration
    DBG("CConnectionAgent::Clean revoking connection agent object");
    m_dwRegisterHandle = 0;
    RevokeActiveObject(dwHandle, NULL);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// Connect - entry point to connect
//
void
CConnectionAgent::Connect(void)
{
    BOOL            fDone = FALSE;
    TCHAR           pszText[TEXT_LENGTH];

    //
    // increment connection count
    //
    m_lConnectionCount ++;
    TraceMsg(TF_THISMODULE, "CConnectionAgent::Connect ref count now %d", m_lConnectionCount);

    //
    // Store offline state if we haven't already and go online
    //
    if(FALSE == IsFlagSet(m_dwFlags, CA_OFFLINE_STATE_READ)) {

        if(IsGlobalOffline())
            SetFlag(m_dwFlags, CA_OFFLINE);

        // make sure we're online
        SetGlobalOffline(FALSE);

        SetFlag(m_dwFlags, CA_OFFLINE_STATE_READ);
    }

    //
    // check for pending dialup connection
    //
    if(IsFlagSet(m_dwFlags, CA_CONNECTING_NOW)) {
        // already working on a connection
        DBG("CConnectionAgent::Connect already trying to connect");
        return;
    }

    //
    // check to see if we can dial to get a connection
    //
    if(FALSE == IsDialPossible()) {

        //
        // can't dial - better have a direct connection
        //
        DBG("CConnectionAgent::Connect guessing connected");
        if(!MLLoadString(IDS_DIAL_DIRECT, pszText, TEXT_LENGTH))
            lstrcpy(pszText, "direct");
        Notify(S_OK, pszText);
        return;
    }

    //
    // check for an existing dialup connection
    //

    if(FALSE == IsFlagSet(m_dwFlags, CA_LOADED_RAS)) {
        SetFlag(m_dwFlags, CA_LOADED_RAS);
        LoadRasDLL();
    }

    if(IsDialExisting()) {
        DBG("CConnectionAgent::Connect already connected");
        if(!MLLoadString(IDS_DIAL_ALREADY_CONNECTED, pszText, TEXT_LENGTH))
            lstrcpy(pszText, "success");
        Notify(S_OK, pszText);
        return;
    }

#ifdef COOL_MODEM_ACTION
    // since we assume connected for direct connect, no proxy check is
    // necessary.  If this check is put in, cool behavior results: It uses
    // the proxy server if it can find it, otherwise it autodials.

    // Autodial needs to be made consistant with this before it gets turned
    // on

    //
    // check for a proxy connection
    //
    if(PingProxy(5)) {
        // Dial is possible but proxy is available! Turn off autodial and
        // flag to turn it back on when we're done
        CPing::EnableAutodial(FALSE);
        SetFlag(m_dwFlags, CA_AUTODIAL_OFF);

        // Using proxy
        DBG("CConnectionAgent::Connect using proxy");
        if(!MLLoadString(IDS_DIAL_PROXY, pszText, TEXT_LENGTH))
            lstrcpy(pszText, "proxy");
        Notify(S_OK, pszText);
        return;
    }
#endif

    /////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////

    //
    // Ensure we can dial without user intervention
    //
    BOOL fContinue = TRUE;

    if(SHRestricted2W(REST_NoUnattendedDialing, NULL, 0))
        // dialing restricted
        fContinue = FALSE;

    //
    // No existing connection but we can dial.  do it now.
    //
    SetFlag(m_dwFlags, CA_CONNECTING_NOW);

    
    DWORD dwRetCode;
    if(fContinue)
    {
        dwRetCode = ERROR_SUCCESS;
        if(!InternetAutodial((INTERNET_AUTODIAL_FORCE_ONLINE | 
                                        INTERNET_AUTODIAL_FORCE_UNATTENDED | 
                                        INTERNET_AUTODIAL_FAILIFSECURITYCHECK), 0))
        {
            dwRetCode = GetLastError();
        }
                
    }
                                        
    if (fContinue && (ERROR_SUCCESS == dwRetCode)) {
        // successful connection made
        SetFlag(m_dwFlags, CA_DIALED);
        if(!MLLoadString(IDS_DIAL_SUCCESS, pszText, TEXT_LENGTH))
            lstrcpy(pszText, "success");
        Notify(S_OK, pszText);
    } else {
        //UINT uID;
        HRESULT hrDialResult;
        // unable to dial
        if(ERROR_INTERNET_FAILED_DUETOSECURITYCHECK == dwRetCode)
        {
            hrDialResult = E_ABORT;
            // uID = IDS_STRING_E_SECURITYCHECK; not needed - since MLLoadString below is commented out
        }
        else
        {
            hrDialResult = E_INVALIDARG;
            // uID = IDS_STRING_E_CONFIG;not needed - since MLLoadString below is commented out
        }
        
     // if(!MLLoadString(uID, pszText, TEXT_LENGTH)) // Don't bother : This is ignored anyway
        lstrcpy(pszText, "Connection Not Made");
        Notify(hrDialResult, pszText);
    }

    ClearFlag(m_dwFlags, CA_CONNECTING_NOW);
}

//
// Disconnect - entry point to disconnect
//
void
CConnectionAgent::Disconnect(void)
{
    // If we're the last connection, hang up
    m_lConnectionCount --;
    TraceMsg(TF_THISMODULE, "CConnectionAgent::Disconnect ref count now %d", m_lConnectionCount);

    if(0 == m_lConnectionCount) {

        // If we dialed this connection, hang it up
        if(IsFlagSet(m_dwFlags, CA_DIALED)) {
            LogEvent(TEXT("EVT: Hanging up"));
            InternetAutodialHangup(0);
        }

#ifdef COOL_MODEM_ACTION
        if(IsFlagSet(m_dwFlags, CA_AUTODIAL_OFF)) {
            // we turned autodial off - turn it back on
            CPing::EnableAutodial(TRUE);

            // tell wininet we've changed it
            InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
        }
#endif

        // restore original offline state
        SetGlobalOffline(IsFlagSet(m_dwFlags, CA_OFFLINE));

        // revoke our object
        Clean();
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// IsDialPossible - search around and make sure all necessary info
// is available to connect.  If fQuiet is false, may bring up dialogs
// to get necessary info
//
BOOL
CConnectionAgent::IsDialPossible()
{
    BOOL    fPossible = TRUE;

    // Refresh data in case properties has been up and changed it
    if(m_pData) 
        MemFree(m_pData);
    m_pData = InitDialData();

    if(NULL == m_pData)
        return FALSE;

    if(FALSE == m_pData->fEnabled)
        // not enabled
        fPossible = FALSE;
    if(!m_pData->pszConnection[0])
        // no connection
        fPossible = FALSE;

    return fPossible;
}


//
// IsDialExisting - check to see if there's an existing dialup connection
//                  that we didn't do
//

#define MAX_CONNECTION 8

BOOL
CConnectionAgent::IsDialExisting(void)
{
    TCHAR   pszConn[RAS_MaxEntryName+1];
    RASCONN pRasCon[MAX_CONNECTION]; 
    DWORD   dwSize = MAX_CONNECTION * sizeof(RASCONN), dwConn, dwCur;
    HKEY    hkeyRoot = HKEY_CURRENT_USER;

    // read internet connectoid from registry
    if(!ReadRegValue(hkeyRoot, c_szRASKey, c_szProfile, pszConn,
            RAS_MaxEntryName+1)) {
        DBG("CConnectionAgent::IsDialExisting unable to read internet connectoid");
        return FALSE;
    }

    // have Ras enumerate existing connections 
    pRasCon[0].dwSize = sizeof(RASCONN);
    if(_RasEnumConnections(pRasCon, &dwSize, &dwConn)) {
        DBG("CConnectionAgent::IsDialExisting RasEnumConnections failed");
        return FALSE;
    }

    // do any of them match our internet connectoid?
    for(dwCur=0; dwCur<dwConn; dwCur++) {
        if(0 == lstrcmp(pszConn, pRasCon[dwCur].szEntryName))
            return TRUE;
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// IUnknown members
//
STDMETHODIMP CConnectionAgent::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;

    // Validate requested interface
    if ((IID_IUnknown == riid) ||
        (IID_IOleCommandTarget == riid))
    {
        *ppv=(IOleCommandTarget*)this;
    } else {
        return E_NOINTERFACE;
    }

    ((LPUNKNOWN)*ppv)->AddRef();
    return NOERROR;
}


STDMETHODIMP_(ULONG) CConnectionAgent::AddRef(void)
{
    TraceMsg(TF_THISMODULE, "CConnectionAgent::Addref m_cRef=%d", m_cRef+1);
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CConnectionAgent::Release(void)
{
    TraceMsg(TF_THISMODULE, "CConnectionAgent::Release m_cRef=%d", m_cRef-1);
    if( 0L != --m_cRef )
        return m_cRef;

    DBG("CConnectionAgent::Release Bye Bye");

    delete this;
    return 0L;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// IOleCommandTarget members
//
STDMETHODIMP CConnectionAgent::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
                                    OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    if (IsEqualGUID(*pguidCmdGroup, CGID_ConnCmdGrp))
    {
        // We like connection agent commands
        return S_OK;
    }

    return OLECMDERR_E_UNKNOWNGROUP;
}

STDMETHODIMP CConnectionAgent::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
                             DWORD nCmdexecopt, VARIANTARG *pvaIn,
                             VARIANTARG *pvaOut)
{
    HRESULT     hr;
    CLIENTINFO  *pInfo;
    int         iIndex;

    if (pguidCmdGroup && IsEqualGUID(*pguidCmdGroup, CGID_ConnCmdGrp))
    {
        switch(nCmdID) {
        case AGENT_CONNECT:
            // validate input arguments
            if(VT_UNKNOWN != pvaIn->vt || NULL == pvaOut)
                return E_INVALIDARG;

            // create dpa if necessary
            if(NULL == m_hdpaClient)
                m_hdpaClient = DPA_Create(0);
            if(NULL == m_hdpaClient)
                return E_OUTOFMEMORY;

            // create and initialize new clientinfo struct
            pInfo = new CLIENTINFO;
            if(NULL == pInfo)
                return E_OUTOFMEMORY;
            pInfo->dwFlags = 0;
            hr = pvaIn->punkVal->QueryInterface(IID_IOleCommandTarget, (void **)&pInfo->poctClient);
            if(FAILED(hr))
                return hr;

            // insert struct into dpa and return index
            iIndex = DPA_InsertPtr(m_hdpaClient, DPA_APPEND, pInfo);
            if(iIndex < 0) {
                delete pInfo;
                return E_OUTOFMEMORY;
            } else {
                pvaOut->vt = VT_I4;
                pvaOut->ulVal = iIndex;
            }

            // connect
            Connect();
            return S_OK;

        case AGENT_DISCONNECT:
            // validate input parameters
            if(VT_I4 != pvaIn->vt)
                return E_INVALIDARG;

            // mark client record as disconnected
            pInfo = (CLIENTINFO *)DPA_GetPtr(m_hdpaClient, pvaIn->lVal);
            if(pInfo) {
                pInfo->dwFlags |= CLIENT_DISCONNECT;
                SAFERELEASE(pInfo->poctClient);
            }

            // disconnect
            Disconnect();
            return S_OK;
        }
    }
    
    return E_NOTIMPL;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// 
// Notify all waiting agents of success or failure of dial attempt
//
void CConnectionAgent::Notify(HRESULT hrDialResult, TCHAR *pszErrorText)
{
    CLIENTINFO          *pClient;
    int                 i, iCurReport;
    //WCHAR               pwszStatus[TEXT_LENGTH];
    VARIANTARG          vin;

    // We're done connecting
    ClearFlag(m_dwFlags, CA_CONNECTING_NOW);

    // Create the notifications to send
    if(S_OK == hrDialResult) {
        DBG("CConnectionAgent::Notify sending ONLINE");
        LogEvent(TEXT("EVT: Successful connection"));
    } else {
        DBG("CConnectionAgent::Notify sending OFFLINE");
        LogEvent(TEXT("EVT: Unsuccessful connection - hr=%08x"), hrDialResult);
    }

    // convert string to bstr
    // MyStrToOleStrN(pwszStatus, TEXT_LENGTH, pszErrorText);

    // build exec paramaters
    vin.vt = VT_ERROR;
    vin.scode = hrDialResult;

    // Send it to all the clients
    iCurReport = DPA_GetPtrCount(m_hdpaClient);
    for(i=0; i<iCurReport; i++) {
        pClient = (CLIENTINFO *)(DPA_GetPtr(m_hdpaClient, i));
        if(pClient && 0 == pClient->dwFlags) {
            pClient->poctClient->Exec(&CGID_ConnCmdGrp, AGENT_NOTIFY, 0, 
                &vin, NULL);
            //  This can get blown away out from under us.
            if (m_hdpaClient)
            {
                pClient->dwFlags |= CLIENT_NOTIFIED;
                SAFERELEASE(pClient->poctClient);
            }
        }
    }

    // if we're disconnected, clean ourselves up
    Clean();
}

BOOL
GetLogonInfo(DIALPROPDATA *pData)
{
    RASDIALPARAMS   dp;
    DWORD           dwRes;
    BOOL            fPassword = FALSE;

    // initially set name/password/domain to null
    pData->pszUsername[0] = 0;
    pData->pszPassword[0] = 0;
    pData->pszDomain[0] = 0;

    // if there's no connection, we're done
    if(0 == pData->pszConnection[0])
        return FALSE;

    // Try and get name/password/domain from Ras
    memset(&dp, 0, sizeof(RASDIALPARAMS));
    dp.dwSize = sizeof(RASDIALPARAMS);
    lstrcpyn(dp.szEntryName, pData->pszConnection, ARRAYSIZE(dp.szEntryName));//BUGBUG-FIXED-OVERFLOW
    dwRes = _RasGetEntryDialParams(NULL, &dp, &fPassword);
    if(fPassword && 0 == dwRes) {
        // Copy ras information to pData.
        lstrcpyn(pData->pszUsername, dp.szUserName, ARRAYSIZE(pData->pszUsername));//BUGBUG-FIXED-OVERFLOW
        lstrcpyn(pData->pszPassword, dp.szPassword, ARRAYSIZE(pData->pszPassword));//BUGBUG-FIXED-OVERFLOW
        lstrcpyn(pData->pszDomain, dp.szDomain, ARRAYSIZE(pData->pszDomain));//BUGBUG-FIXED-OVERFLOW
    }

    return fPassword;
}

DIALPROPDATA * InitDialData(void)
{
    DIALPROPDATA *  pData = (DIALPROPDATA *)MemAlloc(LPTR, sizeof(DIALPROPDATA));
    HKEY            hkeyRoot = HKEY_CURRENT_USER;
    BOOL            fGotInfo = FALSE;
    DWORD           dwValue;

    if(NULL == pData)
        return NULL;

    // Fix fEnabled from registry   HKCU\...\Internet Settings\EnableAutodial
    ReadRegValue(hkeyRoot, szInternetSettings, c_szAutodial, &dwValue, sizeof(DWORD));
    if(dwValue == 1) {
        pData->fEnabled = TRUE;
    }

    // Fix fUnattended from registry   HKCU\...\Internet Settings\EnableUnattended
    ReadRegValue(hkeyRoot, szInternetSettings, c_szEnable, &dwValue, sizeof(DWORD));
    if(dwValue == 1) {
        pData->fUnattended = TRUE;
    }

    // Try to find a connection     HKCU\Remote Access\Internet Profile
    if(ReadRegValue(hkeyRoot, c_szRASKey, c_szProfile, pData->pszConnection,
        RAS_MaxEntryName+1)) {
        GetLogonInfo(pData);
    }
            
    return pData;
}

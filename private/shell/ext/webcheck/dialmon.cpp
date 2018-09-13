//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1995                    **
//*********************************************************************

//
//      DIALMON.C - Window proc for dial monitor app
//

//      HISTORY:
//      
//      4/18/95         jeremys         Created.
//

#include "private.h"

#include <mluisupp.h>

#define TF_THISMODULE TF_DIALMON

//
// Registry keys we use to get autodial information
//
                                                                     
// Internet connection goes in remote access key
const TCHAR c_szRASKey[]    = TEXT("RemoteAccess");

// Key name
const TCHAR c_szProfile[]   = TEXT("InternetProfile");
const TCHAR c_szEnable[]    = TEXT("EnableUnattended");

// registry keys of interest
const TCHAR c_szRegPathInternetSettings[] =         REGSTR_PATH_INTERNET_SETTINGS;
static const TCHAR szRegValEnableAutoDisconnect[] = REGSTR_VAL_ENABLEAUTODISCONNECT; 
static const TCHAR szRegValDisconnectIdleTime[] =   REGSTR_VAL_DISCONNECTIDLETIME; 
static const TCHAR szRegValExitDisconnect[] =       REGSTR_VAL_ENABLEEXITDISCONNECT;
static const TCHAR szEllipsis[] =                   TEXT("...");
static const CHAR szDashes[] =                      "----";
static const TCHAR szAutodialMonitorClass[] =       REGSTR_VAL_AUTODIAL_MONITORCLASSNAME;
static const TCHAR c_szDialmonClass[] =             TEXT("MS_WebcheckMonitor");

// Dialmon globals
UINT_PTR    g_uDialmonSecTimerID = 0;  

CDialMon *  g_pDialMon = NULL;

// Function prototypes for dialog handling functions
INT_PTR CALLBACK DisconnectPromptDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam);
BOOL DisconnectDlgInit(HWND hDlg,DISCONNECTDLGINFO * pDisconnectDlgInfo);
VOID DisconnectDlgCancel(HWND hDlg);
VOID DisconnectDlgTimerProc(HWND hDlg);
VOID DisconnectDlgDisableAutodisconnect(HWND hDlg);
VOID DisconnectDlgShowCountdown(HWND hDlg,DWORD dwSecsRemaining);
VOID EnableDisconnectDlgCtrls(HWND hDlg,BOOL fEnable);
BOOL CenterWindow (HWND hwndChild, HWND hwndParent);

////////////////////////////////////////////////////////////////////////
// RAS delay load helpers
//

typedef DWORD (WINAPI* _RASSETAUTODIALPARAM) (
    DWORD, LPVOID, DWORD
);

typedef DWORD (WINAPI* _RASENUMCONNECTIONSA) (
    LPRASCONNA, LPDWORD, LPDWORD
);

typedef DWORD (WINAPI* _RASENUMCONNECTIONSW) (
    LPRASCONNW, LPDWORD, LPDWORD
);

typedef DWORD (WINAPI* _RASHANGUP) (
    HRASCONN
);

typedef struct _tagAPIMAPENTRY {
    FARPROC* pfn;
    LPSTR pszProc;
} APIMAPENTRY;

static _RASSETAUTODIALPARAM     pfnRasSetAutodialParam = NULL;
static _RASENUMCONNECTIONSA     pfnRasEnumConnectionsA = NULL;
static _RASENUMCONNECTIONSW     pfnRasEnumConnectionsW = NULL;
static _RASHANGUP               pfnRasHangUp = NULL;

static HINSTANCE    g_hRasLib = NULL;
static long         g_lRasRefCnt = 0;

APIMAPENTRY rgRasApiMap[] = {
    { (FARPROC*) &pfnRasSetAutodialParam,       "RasSetAutodialParamA" },
    { (FARPROC*) &pfnRasEnumConnectionsA,       "RasEnumConnectionsA" },
    { (FARPROC*) &pfnRasEnumConnectionsW,       "RasEnumConnectionsW" },
    { (FARPROC*) &pfnRasHangUp,                 "RasHangUpA" },
    { NULL, NULL },
};

/////////////////////////////////////////////////////////////////////////////
//
// RasEnumHelp
//
// Abstract grusome details of getting a correct enumeration of connections 
// from RAS.  Works on all 9x and NT platforms correctly, maintaining unicode
// whenever possible.
//
/////////////////////////////////////////////////////////////////////////////

class RasEnumHelp
{
private:

    //
    // Possible ways we got info from RAS
    //
    typedef enum {
        ENUM_MULTIBYTE,             // Win9x
        ENUM_UNICODE,               // NT
    } ENUM_TYPE;

    //
    // How we got the info
    //
    ENUM_TYPE       _EnumType;     

    //
    // Any error we got during enumeration
    //
    DWORD           _dwLastError;

    //
    // Number of entries we got
    //
    DWORD           _dwEntries;

    //
    // Pointer to info retrieved from RAS
    //
    RASCONNW *      _rcList;

    //
    // Last entry returned as multibyte or unicode when conversion required
    //
    RASCONNW        _rcCurrentEntryW;


public:
    RasEnumHelp();
    ~RasEnumHelp();

    DWORD       GetError();
    DWORD       GetEntryCount();
    LPRASCONNW  GetEntryW(DWORD dwEntry);
};

RasEnumHelp::RasEnumHelp()
{
    DWORD           dwBufSize, dwStructSize;

    // init
    _dwEntries = 0;
    _dwLastError = 0;

    // figure out which kind of enumeration we're doing - start with multibyte
    _EnumType = ENUM_MULTIBYTE;
    dwStructSize = sizeof(RASCONNA);

    if (g_fIsWinNT)
    {
        _EnumType = ENUM_UNICODE;
        dwStructSize = sizeof(RASCONNW);
    }

    // allocate space for 16 entries
    dwBufSize = 16 * dwStructSize;
    _rcList = (LPRASCONNW)LocalAlloc(LMEM_FIXED, dwBufSize);
    if(_rcList)
    {
        do
        {
            // set up list
            _rcList[0].dwSize = dwStructSize;

            // call ras to enumerate
            _dwLastError = ERROR_UNKNOWN;
            if(ENUM_MULTIBYTE == _EnumType)
            {
                if(pfnRasEnumConnectionsA)
                {
                    _dwLastError = pfnRasEnumConnectionsA(
                                    (LPRASCONNA)_rcList,
                                    &dwBufSize,
                                    &_dwEntries
                                    );
                }
            }
            else
            {
                if(pfnRasEnumConnectionsW)
                {
                    _dwLastError = pfnRasEnumConnectionsW(
                                    _rcList,
                                    &dwBufSize,
                                    &_dwEntries
                                    );
                }
            }
       
            // reallocate buffer if necessary
            if(ERROR_BUFFER_TOO_SMALL == _dwLastError)
            {
                LocalFree(_rcList);
                _rcList = (LPRASCONNW)LocalAlloc(LMEM_FIXED, dwBufSize);
                if(NULL == _rcList)
                {
                    _dwLastError = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }
            else
            {
                break;
            }

        } while(TRUE);
    }
    else
    {
        _dwLastError = ERROR_NOT_ENOUGH_MEMORY;
    }

    if(_rcList && (ERROR_SUCCESS != _dwLastError))
    {
        LocalFree(_rcList);
        _rcList = NULL;
        _dwEntries = 0;
    }

    return;
}

RasEnumHelp::~RasEnumHelp()
{
    if(_rcList)
    {
        LocalFree(_rcList);
    }
}

DWORD
RasEnumHelp::GetError()
{
    return _dwLastError;
}

DWORD
RasEnumHelp::GetEntryCount()
{
    return _dwEntries;
}

LPRASCONNW
RasEnumHelp::GetEntryW(DWORD dwEntryNum)
{
    LPRASCONNW  prc = NULL;

    if(dwEntryNum < _dwEntries)
    {
        _rcCurrentEntryW.hrasconn = _rcList[dwEntryNum].hrasconn;

        switch(_EnumType)
        {
        case ENUM_MULTIBYTE:
            {
                MultiByteToWideChar(CP_ACP, 0,
                                    ((LPRASCONNA)_rcList)[dwEntryNum].szEntryName,
                                    -1, _rcCurrentEntryW.szEntryName,
                                    ARRAYSIZE(_rcCurrentEntryW.szEntryName));
            }
            break;

        case ENUM_UNICODE:
            {
                StrCpyNW(_rcCurrentEntryW.szEntryName,
                         _rcList[dwEntryNum].szEntryName,
                         ARRAYSIZE(_rcCurrentEntryW.szEntryName));
            }   
            break;
        }

        prc = &_rcCurrentEntryW;
    }

    return prc;
}


//
// Functions we can call once ras is loaded
//

DWORD _RasSetAutodialParam(DWORD dwKey, LPVOID lpvValue, DWORD dwcbValue)
{
    if (pfnRasSetAutodialParam == NULL)
        return ERROR_UNKNOWN;

    return (*pfnRasSetAutodialParam)(dwKey, lpvValue, dwcbValue);
}

DWORD _RasEnumConnections(LPRASCONNW lpRasConn, LPDWORD lpdwSize, LPDWORD lpdwConn)
{
    RasEnumHelp     reh;
    DWORD           dwRet = reh.GetError();

    if (ERROR_SUCCESS == dwRet)
    {
        DWORD   cItems = reh.GetEntryCount();
        DWORD   cbNeeded = cItems * sizeof(RASCONNW);

        *lpdwConn = 0;

        if (*lpdwSize >= cbNeeded)
        {

            *lpdwConn = cItems;

            DWORD   dw;

            for (dw = 0; dw < cItems; dw++)
            {
                LPRASCONNW  prc = reh.GetEntryW(dw);

                ASSERT(prc != NULL);

                lpRasConn[dw].hrasconn = prc->hrasconn;
                StrCpyNW(lpRasConn[dw].szEntryName,
                         prc->szEntryName,
                         ARRAYSIZE(lpRasConn[dw].szEntryName));
            }
        }
        else
        {
            dwRet = ERROR_BUFFER_TOO_SMALL;
        }

        *lpdwSize = cbNeeded;
    }

    return dwRet;
}

DWORD _RasHangUp(HRASCONN hRasConn)
{
    if (pfnRasHangUp == NULL)
        return ERROR_UNKNOWN;

    return (*pfnRasHangUp)(hRasConn);
}

BOOL
LoadRasDll(void)
{
    if(NULL == g_hRasLib) {
        g_hRasLib = LoadLibrary(TEXT("RASAPI32.DLL"));

        if(NULL == g_hRasLib)
            return FALSE;

        int nIndex = 0;
        while (rgRasApiMap[nIndex].pszProc != NULL) {
            *rgRasApiMap[nIndex].pfn =
                    GetProcAddress(g_hRasLib, rgRasApiMap[nIndex].pszProc);
            // GetProcAddress will fail on Win95 for a couple of NT only apis.
            // ASSERT(*rgRasApiMap[nIndex].pfn != NULL);

            nIndex++;
        }
    }

    if(g_hRasLib) {
        return TRUE;
    }

    return FALSE;
}

void
UnloadRasDll(void)
{
    if(g_hRasLib) {
        FreeLibrary(g_hRasLib);
        g_hRasLib = NULL;
        int nIndex = 0;
        while (rgRasApiMap[nIndex].pszProc != NULL) {
            *rgRasApiMap[nIndex].pfn = NULL;
            nIndex++;
        }
    }
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
// Helper to tell dialmon something is going on
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void IndicateDialmonActivity(void)
{
    static HWND hwndDialmon = NULL;
    HWND hwndMonitor;

    // this one is dynamic - have to find window every time
    hwndMonitor = FindWindow(szAutodialMonitorClass, NULL);
    if(hwndMonitor)
        PostMessage(hwndMonitor, WM_WINSOCK_ACTIVITY, 0, 0);

    // dialmon lives forever - find it once and we're set
    if(NULL == hwndDialmon)
        hwndDialmon = FindWindow(c_szDialmonClass, NULL);
    if(hwndDialmon)
        PostMessage(hwndDialmon, WM_WINSOCK_ACTIVITY, 0, 0);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
// Dialmon startup and shutdown
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
BOOL DialmonInit(void)
{
    g_pDialMon = new CDialMon;

    if(g_pDialMon)
        return TRUE;

    return FALSE;
}


void DialmonShutdown(void)
{
    SAFEDELETE(g_pDialMon);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
// Dialmon window functions
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

// this cwebcheck instance is in iwebck.cpp
extern CWebCheck *g_pwc;

LRESULT CALLBACK Dialmon_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDialMon *pDialMon = (CDialMon*) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_CREATE:
        {
            // snag our class pointer and save in window data
            CREATESTRUCT *pcs;
            pcs = (CREATESTRUCT *)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pcs->lpCreateParams);
            break;
        }

        // DialMon messages (starting at WM_USER+100)
        case WM_SET_CONNECTOID_NAME:
            if(pDialMon)
                pDialMon->OnSetConnectoid(wParam!=0);
            break;
        case WM_WINSOCK_ACTIVITY:
            if(pDialMon)
                pDialMon->OnActivity();
            break;
        case WM_IEXPLORER_EXITING:
            if(pDialMon)
                pDialMon->OnExplorerExit();
            break;
        case WM_TIMER:
            if(pDialMon)
                pDialMon->OnTimer(wParam);
            break;
        case WM_LOAD_SENSLCE:
            DBG("Dialmon_WndProc - got WM_LOAD_SENSLCE");
            if(g_pwc)
            {
                g_pwc->LoadExternals();
            }
            break;
        case WM_IS_SENSLCE_LOADED:
            if(g_pwc)
            {
                return g_pwc->AreExternalsLoaded();
            }
            else
            {
                return FALSE;
            }
            break;
        case WM_WININICHANGE:
            if (lParam && !StrCmpI((LPCTSTR)lParam, TEXT("policy")))
            {
                ProcessInfodeliveryPolicies();
            }
            // BUGBUG: This should be done on Policy and another filter, not for
            // all changes.  (The other filter hasn't been defined yet.)

            //  TODO: handle this in the new architecture!
            //SetNotificationMgrRestrictions(NULL);
            break;

    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                   CDialMon class implementation
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

//
// Constructor / Destructor
//
CDialMon::CDialMon()
{
    WNDCLASS wc;

    // register dialmon window class
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = Dialmon_WndProc;
    wc.hInstance = g_hInst;
    wc.lpszClassName = c_szDialmonClass;
    RegisterClass(&wc);

    // create dialmon window
    _hwndDialmon = CreateWindow(c_szDialmonClass,
                c_szDialmonClass,
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                NULL,
                NULL,
                g_hInst,
                (LPVOID)this);
}

CDialMon::~CDialMon()
{
    if(_hwndDialmon)
        DestroyWindow(_hwndDialmon);

    // unload ras if it's still around
    UnloadRasDll();
}


///////////////////////////////////////////////////////////////////////////
//
// Start/StopMonitoring
//
///////////////////////////////////////////////////////////////////////////

BOOL CDialMon::StartMonitoring(void)
{
    DBG("CDialMon::StartMonitoring");

    // read timeout settings from registry
    RefreshTimeoutSettings();

    // set a one-minute timer
    StopIdleTimer();
    if(!StartIdleTimer())
        return FALSE;

    _dwElapsedTicks = 0;
    
    return TRUE;
}

void CDialMon::StopMonitoring(void)
{
    DBG("CDialMon::StopMonitoring");

    // don't ever hang up now but keep an eye on ras connection
    _dwTimeoutMins = 0;
    _fDisconnectOnExit = FALSE;
}

///////////////////////////////////////////////////////////////////////////
//
// Start/StopIdleTimer, OnTimer
//
///////////////////////////////////////////////////////////////////////////

INT_PTR CDialMon::StartIdleTimer(void)
{
    if(0 == _uIdleTimerID)
        _uIdleTimerID = SetTimer(_hwndDialmon, TIMER_ID_DIALMON_IDLE, 30000, NULL);

    ASSERT(_uIdleTimerID);

    return _uIdleTimerID;
}

void CDialMon::StopIdleTimer(void)
{
    if(_uIdleTimerID) {
        KillTimer(_hwndDialmon, _uIdleTimerID);
        _uIdleTimerID = 0;
    }
}

void CDialMon::OnTimer(UINT_PTR uTimerID)
{
    DBG("CDialMon::OnMonitorTimer");

    // if it's not our timer, ignore it
    if(uTimerID != _uIdleTimerID)
        return;

    // prevent re-entrancy of timer proc (we can stay in here indefinitely
    // since we may bring up a dialog box)
    if (_fInDisconnectFunction) {
        // disconnect dialog already launched, ignore timer ticks while
        // it's present
        return;
    }

    _fInDisconnectFunction = TRUE;
    CheckForDisconnect(TRUE);
    _fInDisconnectFunction = FALSE;

    if(FALSE == _fConnected) {
        StopIdleTimer();
    }
}

///////////////////////////////////////////////////////////////////////////
//
// OnSetConnectoid/OnActivity/OnExplorerExit
//
///////////////////////////////////////////////////////////////////////////

void CDialMon::OnSetConnectoid(BOOL fNoTimeout)
{
    RASCONN RasCon[MAX_CONNECTION];
    DWORD   dwBytes, dwRes, dwConnections;

    // save no timeout setting
    _fNoTimeout = fNoTimeout;

    // Ask ras which connectoid is connected and watch that one
    LoadRasDll();
    RasCon[0].dwSize = sizeof(RasCon[0]);
    dwBytes = MAX_CONNECTION * sizeof(RasCon[0]);
    dwRes = _RasEnumConnections(RasCon, &dwBytes, &dwConnections);
    
    // No connections? bail.
    if(0 == dwConnections) {
        *_pszConnectoidName = TEXT('\0');
        _fConnected = FALSE;
        return;
    }

    // Monitor first connectoid
    StrCpyN(_pszConnectoidName, RasCon[0].szEntryName, ARRAYSIZE(_pszConnectoidName));

    // send ras connect notification if we weren't previously connected
    if(FALSE == _fConnected) {
        _fConnected = TRUE;
    }

    // start watching it
    StartMonitoring();
}

void CDialMon::OnActivity(void)
{
    DBG("CDialMon::OnActivity");

    // reset idle tick count
    _dwElapsedTicks = 0;

    // if the disconnect dialog is present and winsock activity
    // resumes, then dismiss the dialog
    if(_hDisconnectDlg) {
        SendMessage(_hDisconnectDlg, WM_QUIT_DISCONNECT_DLG, 0, 0);
        _hDisconnectDlg = NULL;
    }
}

void CDialMon::OnExplorerExit()
{
    DBG("CDialMon::OnIExplorerExit");

    if(FALSE == _fDisconnectOnExit && FALSE == _fNoTimeout) {
        // no exit disconnection so bail
        DBG("CDialMon::OnIExplorerExit - exit hangup not enabled");
        return;
    }

    // prevent re-entrancy of this function (we can stay in here indefinitely
    // since we may bring up a dialog box)
    if (_fInDisconnectFunction) {
        // some UI already launched
        return;
    }

    _fInDisconnectFunction = TRUE;
    CheckForDisconnect(FALSE);
    _fInDisconnectFunction = FALSE;

    if(FALSE == _fConnected) {
        StopIdleTimer();
    }
}


///////////////////////////////////////////////////////////////////////////
//
// RefreshTimeoutSettings
//
///////////////////////////////////////////////////////////////////////////

BOOL CDialMon::RefreshTimeoutSettings(void)
{
    HKEY    hKey;
    BOOL    fSuccess = FALSE;
    TCHAR   szKey[MAX_PATH];
    DWORD   dwRes, dwData, dwSize, dwDisp;

    // assume disconnect monitoring is off
    _dwTimeoutMins = 0;
    _fDisconnectOnExit = FALSE;

    // figure out appropriate key
    wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("%s\\Profile\\%s"),
            REGSTR_PATH_REMOTEACCESS, _pszConnectoidName);

    // open a regstry key to the internet settings section
    dwRes = RegCreateKeyEx(HKEY_CURRENT_USER, szKey, 0, TEXT(""), 0,
            KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hKey, &dwDisp);
    
    if(ERROR_SUCCESS == dwRes)
    {
        //
        // is autodisconnect enabled?
        //
        dwSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey,szRegValEnableAutoDisconnect,NULL,NULL,
                (LPBYTE) &dwData,&dwSize) == ERROR_SUCCESS)
        {
            if(dwData)
            {
                // what's the timeout?
                dwSize = sizeof(DWORD);
                if (RegQueryValueEx(hKey,szRegValDisconnectIdleTime,NULL,NULL,
                        (LPBYTE) &dwData,&dwSize) == ERROR_SUCCESS && dwData)
                {
                    _dwTimeoutMins = dwData;
                    fSuccess = TRUE;
                }
            }

            // is disconnect on exit enabled?
            dwSize = sizeof(DWORD);
            if (RegQueryValueEx(hKey,szRegValExitDisconnect,NULL,NULL,
                    (LPBYTE) &dwData,&dwSize) == ERROR_SUCCESS && dwData)
            {
                _fDisconnectOnExit = TRUE;
                fSuccess = TRUE;
            }
        }
        else
        {
            //
            // couldn't find enable autodisconnect key.  Set all disconnect
            // settings to their defaults
            //

            // set class members to default values
            _dwTimeoutMins = 20;
            _fDisconnectOnExit = TRUE;
            fSuccess = TRUE;

            // enable idle disconnect and exit disconnect
            dwData = 1;
            RegSetValueEx(hKey, szRegValEnableAutoDisconnect, 0, REG_DWORD,
                    (LPBYTE)&dwData, sizeof(DWORD));
            RegSetValueEx(hKey, szRegValExitDisconnect, 0, REG_DWORD,
                    (LPBYTE)&dwData, sizeof(DWORD));

            // Save idle minutes
            RegSetValueEx(hKey, szRegValDisconnectIdleTime, 0, REG_DWORD,
                    (LPBYTE)&_dwTimeoutMins, sizeof(DWORD));
        }

        RegCloseKey(hKey);
    }

    return fSuccess;
}

///////////////////////////////////////////////////////////////////////////
//
// Disconnection handling
//
///////////////////////////////////////////////////////////////////////////

void CDialMon::CheckForDisconnect(BOOL fTimer)
{
    BOOL    fPromptForDisconnect = TRUE;       // assume we should prompt for disconnect
    BOOL    fDisconnectDisabled = FALSE;
    BOOL    fConnectoidAlive = FALSE;
    RASCONN RasCon[MAX_CONNECTION];
    DWORD   dwBytes, dwRes, dwConnections = 0, i;
    HRASCONN hConnection = NULL;
        
    // Verify we still have a connection
    RasCon[0].dwSize = sizeof(RasCon[0]);
    dwBytes = MAX_CONNECTION * sizeof(RasCon[0]);
    dwRes = _RasEnumConnections(RasCon, &dwBytes, &dwConnections);
    
    // If ras is connected at all, stay alive to monitor it
    if(0 == dwConnections)
        _fConnected = FALSE;

    // Find connectoid we're supposed to watch
    if(TEXT('\0') == *_pszConnectoidName) {
        DBG_WARN("DisconnectHandler: No designated connection to monitor");
        return;
    }
        
    for(i=0; i<dwConnections; i++) {
        if(!StrCmp(RasCon[i].szEntryName, _pszConnectoidName)) {
            fConnectoidAlive = TRUE;
            hConnection = RasCon[i].hrasconn;
        }
    }

    // if we're not connected to out monitor connectoid, ditch our hangup
    // dialog if we have one and bail out
    if(FALSE == fConnectoidAlive) {
        if(_hDisconnectDlg) {
            SendMessage(_hDisconnectDlg, WM_QUIT_DISCONNECT_DLG, 0, 0);
            _hDisconnectDlg = NULL;
        }
        return;
    }

    // Check timeout if we got a timer tick
    if(fTimer) {
        // increment tick count
        _dwElapsedTicks ++;

        // Haven't exceeded idle threshold or not watching for idle
        if (0 == _dwTimeoutMins || _dwElapsedTicks < _dwTimeoutMins * 2)
            fPromptForDisconnect = FALSE;
    }

    if(FALSE == fPromptForDisconnect) {
        return;
    }

    // prompt user to see if they want to hang up
    if(PromptForDisconnect(fTimer, &fDisconnectDisabled)) {
        // hang it up
        ASSERT(hConnection);
        if(hConnection)
            _RasHangUp(hConnection);
        _fConnected = FALSE;
    }

    if (fDisconnectDisabled) {
        StopMonitoring();
    }

    _dwElapsedTicks = 0;
}

BOOL CDialMon::PromptForDisconnect(BOOL fTimer, BOOL *pfDisconnectDisabled)
{
    ASSERT(_pszConnectoidName);
    ASSERT(pfDisconnectDisabled);

    // fill out struct to pass to dialog
    DISCONNECTDLGINFO DisconnectDlgInfo;
    memset(&DisconnectDlgInfo,0,sizeof(DisconnectDlgInfo));
    DisconnectDlgInfo.pszConnectoidName = _pszConnectoidName;
    DisconnectDlgInfo.fTimer = fTimer;
    DisconnectDlgInfo.dwTimeout = _dwTimeoutMins;
    DisconnectDlgInfo.pDialMon = this;

    // choose the appropriate dialog depending on if this a "timeout" dialog
    // or "app exiting" dialog
    UINT uDlgTemplateID = fTimer ? IDD_DISCONNECT_PROMPT:IDD_APP_EXIT_PROMPT;

    // run the dialog
    BOOL fRet = (BOOL)DialogBoxParam(MLGetHinst(),MAKEINTRESOURCE(uDlgTemplateID),
            NULL, DisconnectPromptDlgProc,(LPARAM) &DisconnectDlgInfo);

    // dialog box stores its window handle in our class so we can send
    // messages to it, clear the global handle now that it's dismissed
    _hDisconnectDlg = NULL;

    *pfDisconnectDisabled = FALSE;
    if (!fRet && DisconnectDlgInfo.fDisconnectDisabled) {
        *pfDisconnectDisabled=TRUE;

        // turn off reg keys for this connection
        TCHAR   szKey[128];
        DWORD   dwRes, dwValue = 0;
        HKEY    hKey;

        wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("%s\\Profile\\%s"),
                REGSTR_PATH_REMOTEACCESS, _pszConnectoidName);
        dwRes = RegOpenKey(HKEY_CURRENT_USER, szKey, &hKey);
        if(ERROR_SUCCESS == dwRes) {

            // Turn off idle disconnect
            RegSetValueEx(hKey, szRegValEnableAutoDisconnect, 0, REG_DWORD,
                    (LPBYTE)&dwValue, sizeof(DWORD));

            // Turn off exit disconnect
            RegSetValueEx(hKey, szRegValExitDisconnect, 0, REG_DWORD,
                    (LPBYTE)&dwValue, sizeof(DWORD));

            RegCloseKey(hKey);
        }
    }   

    return fRet;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                  Disconnect dialog implementation
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK DisconnectPromptDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        // lParam points to data struct, store a pointer to it in window data
        SetWindowLongPtr(hDlg, DWLP_USER,lParam);
        return DisconnectDlgInit(hDlg,(DISCONNECTDLGINFO *) lParam);
        break;
    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            EndDialog(hDlg,TRUE);
            break;
        case IDCANCEL:
            DisconnectDlgCancel(hDlg);
            EndDialog(hDlg,FALSE);
            break;
        case IDC_DISABLE_AUTODISCONNECT:
            DisconnectDlgDisableAutodisconnect(hDlg);
            break;
        }
        break;
    case WM_QUIT_DISCONNECT_DLG:
        // parent window wants to terminate us
        EndDialog(hDlg,FALSE);
        break;
    case WM_TIMER:
        DisconnectDlgTimerProc(hDlg);
        break;
    }

    return FALSE;
}

BOOL __cdecl _FormatMessage(LPCWSTR szTemplate, LPWSTR szBuf, UINT cchBuf, ...)
{
    BOOL fRet;
    va_list ArgList;
    va_start(ArgList, cchBuf);

    fRet = FormatMessageWrapW(FORMAT_MESSAGE_FROM_STRING, szTemplate, 0, 0, szBuf, cchBuf, &ArgList);

    va_end(ArgList);
    return fRet;
}

BOOL DisconnectDlgInit(HWND hDlg,DISCONNECTDLGINFO * pDisconnectDlgInfo)
{
    ASSERT(pDisconnectDlgInfo);
    if (!pDisconnectDlgInfo)
        return FALSE;

    // allocate buffers to build text for dialog
    BUFFER BufText(MAX_RES_LEN + MAX_CONNECTOID_DISPLAY_LEN + 1);
    BUFFER BufFmt(MAX_RES_LEN),BufConnectoidName(MAX_CONNECTOID_DISPLAY_LEN+4);
    ASSERT(BufText && BufFmt && BufConnectoidName);
    if (!BufText || !BufFmt || !BufConnectoidName)
        return FALSE;

    UINT uStringID;
    // choose the appropriate text string for dialog
    if (pDisconnectDlgInfo->fTimer) {
        uStringID = IDS_DISCONNECT_DLG_TEXT;
    } else {
        uStringID = IDS_APP_EXIT_TEXT;
    }

    // load the format string from resource
    MLLoadString(uStringID,BufFmt.QueryPtr(),BufFmt.QuerySize());

    // copy the connectoid name into buffer, and truncate it if it's really
    // long
    StrCpyN(BufConnectoidName.QueryPtr(),pDisconnectDlgInfo->pszConnectoidName,
              BufConnectoidName.QuerySize());
    if (lstrlen(pDisconnectDlgInfo->pszConnectoidName) > MAX_CONNECTOID_DISPLAY_LEN) {
        StrCpyN(((TCHAR *) BufConnectoidName.QueryPtr()) + MAX_CONNECTOID_DISPLAY_LEN,
                 szEllipsis, BufConnectoidName.QuerySize());
    }

    if (pDisconnectDlgInfo->fTimer)
    {
        _FormatMessage(BufFmt.QueryPtr(),
                       BufText.QueryPtr(),
                       BufText.QuerySize(),
                       BufConnectoidName.QueryPtr(),
                       pDisconnectDlgInfo->dwTimeout);
    }
    else
    {
        _FormatMessage(BufFmt.QueryPtr(),
                       BufText.QueryPtr(),
                       BufText.QuerySize(),
                       BufConnectoidName.QueryPtr());
    }

    // set text in dialog
    SetDlgItemText(hDlg,IDC_TX1,BufText.QueryPtr());

    // if this timeout dialog (which counts down), initialize countdown timer
    if (pDisconnectDlgInfo->fTimer) {
        pDisconnectDlgInfo->dwCountdownVal = DISCONNECT_DLG_COUNTDOWN;

        DisconnectDlgShowCountdown(hDlg,pDisconnectDlgInfo->dwCountdownVal);

        // set a one-second timer
        g_uDialmonSecTimerID = SetTimer(hDlg,TIMER_ID_DIALMON_SEC,1000,NULL);
        ASSERT(g_uDialmonSecTimerID);
        if (!g_uDialmonSecTimerID) {
            // it's very unlikely that setting the timer will fail... but if it
            // does, then we'll act just like a normal dialog and won't have
            // a countdown.  Hide the countdown-related windows...
            ShowWindow(GetDlgItem(hDlg,IDC_TX2),SW_HIDE);
            ShowWindow(GetDlgItem(hDlg,IDC_GRP),SW_HIDE);
            ShowWindow(GetDlgItem(hDlg,IDC_TIME_REMAINING),SW_HIDE);
            ShowWindow(GetDlgItem(hDlg,IDC_TX3),SW_HIDE);
        }

        // beep to alert user
        MessageBeep(MB_ICONEXCLAMATION);
    }

    // center this dialog on the screen
    CenterWindow(hDlg,GetDesktopWindow());

    // default: assume user does not disable auto disconnect, change
    // this later if they do (this field is output to dialog invoker)
    pDisconnectDlgInfo->fDisconnectDisabled = FALSE;

    // Save dialog handle so we can get quit messages
    pDisconnectDlgInfo->pDialMon->_hDisconnectDlg = hDlg;
 
    return TRUE;
}

VOID DisconnectDlgCancel(HWND hDlg)
{
    // get pointer to data struct out of window data
    DISCONNECTDLGINFO * pDisconnectDlgInfo = (DISCONNECTDLGINFO *)
                                             GetWindowLongPtr(hDlg, DWLP_USER);
    ASSERT(pDisconnectDlgInfo);

    // check to see if user checked 'disable autodisconnect' checkbox
    if(IsDlgButtonChecked(hDlg,IDC_DISABLE_AUTODISCONNECT))
    {
        // set the output field to indicate that user wanted to disable
        // auto disconnect
        pDisconnectDlgInfo->fDisconnectDisabled = TRUE;
    }       
}

VOID DisconnectDlgTimerProc(HWND hDlg)
{
    // ignore timer ticks (e.g. hold countdown) if "disable autodisconnect"
    // checkbox is checked
    if (IsDlgButtonChecked(hDlg,IDC_DISABLE_AUTODISCONNECT))
        return;

    // get pointer to data struct out of window data
    DISCONNECTDLGINFO * pDisconnectDlgInfo =
                (DISCONNECTDLGINFO *) GetWindowLongPtr(hDlg, DWLP_USER);
    ASSERT(pDisconnectDlgInfo);
    if (!pDisconnectDlgInfo)
        return;

    if (pDisconnectDlgInfo->dwCountdownVal) {
        // decrement countdown value
        pDisconnectDlgInfo->dwCountdownVal --;

        // update the dialog with the new value
        if (pDisconnectDlgInfo->dwCountdownVal) {
            DisconnectDlgShowCountdown(hDlg,pDisconnectDlgInfo->dwCountdownVal);
            return;
        }
    }

    // countdown has run out!

    // kill the timer
    KillTimer(hDlg,g_uDialmonSecTimerID);
    g_uDialmonSecTimerID = 0;

    // send a 'OK' message to the dialog to dismiss it
    SendMessage(hDlg,WM_COMMAND,IDOK,0);
}

VOID DisconnectDlgShowCountdown(HWND hDlg,DWORD dwSecsRemaining)
{
    // build a string showing the number of seconds left
    CHAR szSecs[10];
    if (dwSecsRemaining == (DWORD) -1) {
        lstrcpyA(szSecs, szDashes);
    } else {
        wnsprintfA(szSecs, ARRAYSIZE(szSecs), "%lu", dwSecsRemaining);
    }

    // set string in text control
    SetDlgItemTextA(hDlg, IDC_TIME_REMAINING, szSecs);
}

VOID DisconnectDlgDisableAutodisconnect(HWND hDlg)
{
    // get pointer to data struct out of window data
    DISCONNECTDLGINFO * pDisconnectDlgInfo = (DISCONNECTDLGINFO *)
            GetWindowLongPtr(hDlg, DWLP_USER);
    ASSERT(pDisconnectDlgInfo);

    // find out if disable autodisconnect checkbox is checked
    BOOL fDisabled = IsDlgButtonChecked(hDlg,IDC_DISABLE_AUTODISCONNECT);

    // enable or disable controls appropriately
    EnableDisconnectDlgCtrls(hDlg,!fDisabled);

    if (!fDisabled) {
        // reset timer if we're re-enabling autodisconnect
        pDisconnectDlgInfo->dwCountdownVal = DISCONNECT_DLG_COUNTDOWN;
        // show timer value
        DisconnectDlgShowCountdown(hDlg,pDisconnectDlgInfo->dwCountdownVal);
    } else {
        // show "--" in countdown value
        DisconnectDlgShowCountdown(hDlg,(DWORD) -1);
    }
}

VOID EnableDisconnectDlgCtrls(HWND hDlg,BOOL fEnable)
{
    EnableWindow(GetDlgItem(hDlg,IDC_TX1),fEnable);
    EnableWindow(GetDlgItem(hDlg,IDC_TX2),fEnable);
    EnableWindow(GetDlgItem(hDlg,IDC_TX3),fEnable);
    EnableWindow(GetDlgItem(hDlg,IDC_TIME_REMAINING),fEnable);
    EnableWindow(GetDlgItem(hDlg,IDOK),fEnable);
}

BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
    RECT    rChild, rParent;
    int     wChild, hChild, wParent, hParent;
    int     wScreen, hScreen, xNew, yNew;
    HDC     hdc;

    // Get the Height and Width of the child window
    GetWindowRect (hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    // Get the Height and Width of the parent window
    GetWindowRect (hwndParent, &rParent);
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    // Get the display limits
    hdc = GetDC (hwndChild);
    wScreen = GetDeviceCaps (hdc, HORZRES);
    hScreen = GetDeviceCaps (hdc, VERTRES);
    ReleaseDC (hwndChild, hdc);

    // Calculate new X position, then adjust for screen
    xNew = rParent.left + ((wParent - wChild) /2);
    if (xNew < 0) {
        xNew = 0;
    } else if ((xNew+wChild) > wScreen) {
        xNew = wScreen - wChild;
    }

    // Calculate new Y position, then adjust for screen
    yNew = rParent.top  + ((hParent - hChild) /2);
    if (yNew < 0) {
        yNew = 0;
    } else if ((yNew+hChild) > hScreen) {
        yNew = hScreen - hChild;
    }

    // Set it, and return
    return SetWindowPos (hwndChild, NULL, xNew, yNew, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                      BUFFER class implementation
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

BOOL BUFFER::Alloc( UINT cchBuffer )
{
    _lpBuffer = (LPTSTR)::MemAlloc(LPTR,cchBuffer*sizeof(TCHAR));
    if (_lpBuffer != NULL) {
        _cch = cchBuffer;
        return TRUE;
    }
    return FALSE;
}

BOOL BUFFER::Realloc( UINT cchNew )
{
    LPVOID lpNew = ::MemReAlloc((HLOCAL)_lpBuffer, cchNew*sizeof(TCHAR),
            LMEM_MOVEABLE | LMEM_ZEROINIT);
    if (lpNew == NULL)
        return FALSE;

    _lpBuffer = (LPTSTR)lpNew;
    _cch = cchNew;
    return TRUE;
}

BUFFER::BUFFER( UINT cchInitial /* =0 */ )
  : BUFFER_BASE(),
        _lpBuffer( NULL )
{
    if (cchInitial)
        Alloc( cchInitial );
}

BUFFER::~BUFFER()
{
    if (_lpBuffer != NULL) {
        MemFree((HLOCAL) _lpBuffer);
        _lpBuffer = NULL;
    }
}

BOOL BUFFER::Resize( UINT cchNew )
{
    BOOL fSuccess;

    if (QuerySize() == 0)
        fSuccess = Alloc( cchNew*sizeof(TCHAR) );
    else {
        fSuccess = Realloc( cchNew*sizeof(TCHAR) );
    }
    if (fSuccess)
        _cch = cchNew;
    return fSuccess;
}

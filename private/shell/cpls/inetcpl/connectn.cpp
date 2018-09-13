///////////////////////////////////////////////////////////////////////
//                   Microsoft Windows                               //
//             Copyright(c) Microsoft Corp., 1995                    //
///////////////////////////////////////////////////////////////////////
//
// CONNECTN.C - "Connection" Property Sheet
//
// HISTORY:
//
// 6/22/96  t-gpease    moved to this file
//

#include "inetcplp.h"
#include <inetcpl.h>
#include <rasdlg.h>

#include <mluisupp.h>

HINSTANCE   hInstRNADll = NULL;
DWORD       dwRNARefCount = 0;
BOOL        g_fWin95 = TRUE;
BOOL        g_fWin2K = FALSE;

static const TCHAR g_szSensPath[]            = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Webcheck");

// clsids used to jit in features
static const CLSID clsidFeatureICW = {      // {5A8D6EE0-3E18-11D0-821E-444553540000}
    0x5A8D6EE0, 0x3E18, 0x11D0, {0x82, 0x1E, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

static const CLSID clsidFeatureMobile = {   // {3af36230-a269-11d1-b5bf-0000f8051515}
    0x3af36230, 0xa269, 0x11d1, {0xb5, 0xbf, 0x00, 0x00, 0xf8, 0x05, 0x15, 0x15}};

// RNA api function names
static const CHAR szRasEditPhonebookEntryA[]   = "RasEditPhonebookEntryA";
static const CHAR szRasEditPhonebookEntryW[]   = "RasEditPhonebookEntryW";
static const CHAR szRasEnumEntriesA[]          = "RasEnumEntriesA";
static const CHAR szRasEnumEntriesW[]          = "RasEnumEntriesW";
static const CHAR szRasDeleteEntryA[]          = "RasDeleteEntryA";
static const CHAR szRasDeleteEntryW[]          = "RasDeleteEntryW";
static const CHAR szRasGetEntryDialParamsA[]   = "RasGetEntryDialParamsA";
static const CHAR szRasGetEntryDialParamsW[]   = "RasGetEntryDialParamsW";
static const CHAR szRasSetEntryDialParamsA[]   = "RasSetEntryDialParamsA";
static const CHAR szRasSetEntryDialParamsW[]   = "RasSetEntryDialParamsW";
static const CHAR szRasCreatePhonebookEntryA[] = "RasCreatePhonebookEntryA";
static const CHAR szRasGetEntryPropertiesW[]   = "RasGetEntryPropertiesW";
static const CHAR szRnaActivateEngine[]        = "RnaActivateEngine";
static const CHAR szRnaDeactivateEngine[]      = "RnaDeactivateEngine";
static const CHAR szRnaDeleteEntry[]           = "RnaDeleteConnEntry";

RASEDITPHONEBOOKENTRYA   lpRasEditPhonebookEntryA   = NULL;
RASEDITPHONEBOOKENTRYW   lpRasEditPhonebookEntryW   = NULL;
RASENUMENTRIESA          lpRasEnumEntriesA          = NULL;
RASENUMENTRIESW          lpRasEnumEntriesW          = NULL;
RASDELETEENTRYA          lpRasDeleteEntryA          = NULL;
RASDELETEENTRYW          lpRasDeleteEntryW          = NULL;
RASGETENTRYDIALPARAMSA   lpRasGetEntryDialParamsA   = NULL;
RASGETENTRYDIALPARAMSW   lpRasGetEntryDialParamsW   = NULL;
RASSETENTRYDIALPARAMSA   lpRasSetEntryDialParamsA   = NULL;
RASSETENTRYDIALPARAMSW   lpRasSetEntryDialParamsW   = NULL;
RASCREATEPHONEBOOKENTRYA lpRasCreatePhonebookEntryA = NULL;
RASGETENTRYPROPERTIESW   lpRasGetEntryPropertiesW   = NULL;
RNAACTIVATEENGINE        lpRnaActivateEngine        = NULL;
RNADEACTIVATEENGINE      lpRnaDeactivateEngine      = NULL;
RNADELETEENTRY           lpRnaDeleteEntry           = NULL;

#define NUM_RNAAPI_PROCS        15
APIFCN RasApiList[NUM_RNAAPI_PROCS] = {
    { (PVOID *) &lpRasEditPhonebookEntryA,   szRasEditPhonebookEntryA},
    { (PVOID *) &lpRasEditPhonebookEntryW,   szRasEditPhonebookEntryW},
    { (PVOID *) &lpRasEnumEntriesA,          szRasEnumEntriesA},
    { (PVOID *) &lpRasEnumEntriesW,          szRasEnumEntriesW},
    { (PVOID *) &lpRasGetEntryDialParamsA,   szRasGetEntryDialParamsA},
    { (PVOID *) &lpRasGetEntryDialParamsW,   szRasGetEntryDialParamsW},
    { (PVOID *) &lpRasSetEntryDialParamsA,   szRasSetEntryDialParamsA},
    { (PVOID *) &lpRasSetEntryDialParamsW,   szRasSetEntryDialParamsW},
    { (PVOID *) &lpRasDeleteEntryA,          szRasDeleteEntryA},
    { (PVOID *) &lpRasDeleteEntryW,          szRasDeleteEntryW},
    { (PVOID *) &lpRasCreatePhonebookEntryA, szRasCreatePhonebookEntryA},
    { (PVOID *) &lpRasGetEntryPropertiesW,   szRasGetEntryPropertiesW},
    { (PVOID *) &lpRnaActivateEngine,        szRnaActivateEngine},
    { (PVOID *) &lpRnaDeactivateEngine,      szRnaDeactivateEngine},
    { (PVOID *) &lpRnaDeleteEntry,           szRnaDeleteEntry}
};


//
// Connection dialog needs info
//
typedef struct _conninfo {

    HTREEITEM   hDefault;
    TCHAR       szEntryName[RAS_MaxEntryName+1];

} CONNINFO, *PCONNINFO;

//
// dial dialog needs some info asssociated with its window
//
typedef struct _dialinfo {

    PROXYINFO   proxy;              // manual proxy info
    BOOL        fClickedAutodetect; // did the user actually click autodetect?
    LPTSTR      pszConnectoid;
#ifdef UNIX
    TCHAR       szEntryName[RAS_MaxEntryName+1];
#endif

} DIALINFO, *PDIALINFO;

//
// Private Functions
//

BOOL ConnectionDlgInit(HWND hDlg, PCONNINFO pConn);
BOOL ConnectionDlgOK(HWND hDlg, PCONNINFO pConn);
VOID EnableConnectionControls(HWND hDlg, PCONNINFO pConn, BOOL fSetText);
BOOL LoadRNADll(VOID);
VOID UnloadRNADll(VOID);
DWORD PopulateRasEntries(HWND hDlg, PCONNINFO pConn);
BOOL MakeNewConnectoid(HWND hDlg, PCONNINFO pConn);
BOOL EditConnectoid(HWND hDlg);

INT_PTR CALLBACK DialupDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK AdvDialupDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK AdvAutocnfgDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

// Handy stuff for looking at proxy exceptions (from proxysup.cpp)
BOOL RemoveLocalFromExceptionList(IN LPTSTR lpszExceptionList);

extern const TCHAR cszLocalString[];

// defines for tree view image list
#define BITMAP_WIDTH    16
#define BITMAP_HEIGHT   16
#define CONN_BITMAPS    2
#define IMAGE_LAN       0
#define IMAGE_MODEM     1

void GetConnKey(LPTSTR pszConn, LPTSTR pszBuffer, int iBuffLen)
{
    if(NULL == pszConn || 0 == *pszConn) {
        // use lan reg location
        StrCpyN(pszBuffer, REGSTR_PATH_INTERNET_LAN_SETTINGS, iBuffLen);
    } else {
        // use connectoid reg location
        wnsprintf(pszBuffer, iBuffLen, TEXT("%s\\Profile\\%s"), REGSTR_PATH_REMOTEACCESS, pszConn);
    }
}

DWORD GetCoverExclude(LPTSTR pszConn)
{
    TCHAR   szKey[64];

    GetConnKey(pszConn, szKey, 64);
    RegEntry re(szKey, HKEY_CURRENT_USER);
    return re.GetNumber(REGSTR_VAL_COVEREXCLUDE, 0);
}

void SetCoverExclude(LPTSTR pszConn, DWORD dwCoverExclude)
{
    TCHAR   szKey[64];

    GetConnKey(pszConn, szKey, 64);
    RegEntry re(szKey, HKEY_CURRENT_USER);
    re.SetValue(REGSTR_VAL_COVEREXCLUDE, dwCoverExclude);
}

/////////////////////////////////////////////////////////////////////////////
//
// JitFeature - decide if a feature is present, not present but
//              jitable, or not present and not jitable.  Actually JIT it
//              in if requested
//
/////////////////////////////////////////////////////////////////////////////
#define JIT_PRESENT         0           // Installed
#define JIT_AVAILABLE       1           // Can be JIT'ed
#define JIT_NOT_AVAILABLE   2           // You're in trouble - can't be JIT'ed

DWORD JitFeature(HWND hwnd, REFCLSID clsidFeature, BOOL fCheckOnly)
{
    HRESULT     hr  = REGDB_E_CLASSNOTREG;
    uCLSSPEC    classpec;
    DWORD       dwFlags = 0;

    // figure out struct and flags
    classpec.tyspec = TYSPEC_CLSID;
    classpec.tagged_union.clsid = clsidFeature;

    if(fCheckOnly)
        dwFlags = FIEF_FLAG_PEEK;

    //
    // since we only come to install of JIT features
    // only via a UI code path in inetcpl, we want to
    // simply ignore any previous UI action
    //
    dwFlags |= FIEF_FLAG_FORCE_JITUI;
    // call jit code
    hr = FaultInIEFeature(hwnd, &classpec, NULL, dwFlags);

    if(S_OK == hr) {
        // feature present
        return JIT_PRESENT;
    }

    if(S_FALSE == hr || E_ACCESSDENIED == hr) {
        // jit doesn't know about this feature.  Assume it's present.
        return JIT_PRESENT;
    }

    if(HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr) {
        // user didn't want it - may try again sometime, however
        return JIT_AVAILABLE;
    }

    if(fCheckOnly) {
        if(HRESULT_FROM_WIN32(ERROR_PRODUCT_UNINSTALLED) == hr) {
            // not present but can get it
            return JIT_AVAILABLE;
        }
    }

    //
    // Actually tried to get it but didn't - return not available
    //
    return JIT_NOT_AVAILABLE;
}


/////////////////////////////////////////////////////////////////////////////
//
// RasEnumHelp
//
// Abstract grusome details of getting a correct enumeration of entries 
// from RAS.  Works on all 9x and NT platforms correctly, maintaining unicode
// whenever possible.
//
/////////////////////////////////////////////////////////////////////////////

class RasEnumHelp
{
private:
    
    //
    // Win2k version of RASENTRYNAMEW struct
    //
    #define W2KRASENTRYNAMEW struct tagW2KRASENTRYNAMEW
    W2KRASENTRYNAMEW
    {
        DWORD dwSize;
        WCHAR szEntryName[ RAS_MaxEntryName + 1 ];
        DWORD dwFlags;
        WCHAR szPhonebookPath[MAX_PATH + 1];
    };
    #define LPW2KRASENTRYNAMEW W2KRASENTRYNAMEW*

    //
    // Possible ways we got info from RAS
    //
    typedef enum {
        ENUM_MULTIBYTE,             // Win9x
        ENUM_UNICODE,               // NT4
        ENUM_WIN2K                  // Win2K
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
    RASENTRYNAMEA * _preList;

    //
    // Last entry returned as multibyte or unicode when conversion required
    //
    WCHAR           _szCurrentEntryW[RAS_MaxEntryName + 1];


public:
    RasEnumHelp();
    ~RasEnumHelp();

    DWORD   GetError();
    DWORD   GetEntryCount();
    LPWSTR  GetEntryW(DWORD dwEntry);
};



RasEnumHelp::RasEnumHelp()
{
    DWORD           dwBufSize, dwStructSize;
    OSVERSIONINFO   ver;

    // init
    _dwEntries = 0;
    _dwLastError = 0;

    // figure out which kind of enumeration we're doing - start with multibyte
    _EnumType = ENUM_MULTIBYTE;
    dwStructSize = sizeof(RASENTRYNAMEA);

    ver.dwOSVersionInfoSize = sizeof(ver);
    if(GetVersionEx(&ver))
    {
        if(VER_PLATFORM_WIN32_NT == ver.dwPlatformId)
        {
            _EnumType = ENUM_UNICODE;
            dwStructSize = sizeof(RASENTRYNAMEW);

            if(ver.dwMajorVersion >= 5)
            {
                _EnumType = ENUM_WIN2K;
                dwStructSize = sizeof(W2KRASENTRYNAMEW);
            }
        }
    }

    // allocate space for 16 entries
    dwBufSize = 16 * dwStructSize;
    _preList = (LPRASENTRYNAMEA)GlobalAlloc(LMEM_FIXED, dwBufSize);
    if(_preList)
    {
        do
        {
            // set up list
            _preList[0].dwSize = dwStructSize;

            // call ras to enumerate
            _dwLastError = ERROR_UNKNOWN;
            if(ENUM_MULTIBYTE == _EnumType)
            {
                if(lpRasEnumEntriesA)
                {
                    _dwLastError = lpRasEnumEntriesA(
                                    NULL,
                                    NULL,
                                    (LPRASENTRYNAMEA)_preList,
                                    &dwBufSize,
                                    &_dwEntries
                                    );
                }
            }
            else
            {
                if(lpRasEnumEntriesW)
                {
                    _dwLastError = lpRasEnumEntriesW(
                                    NULL,
                                    NULL,
                                    (LPRASENTRYNAMEW)_preList,
                                    &dwBufSize,
                                    &_dwEntries
                                    );
                }
            }
       
            // reallocate buffer if necessary
            if(ERROR_BUFFER_TOO_SMALL == _dwLastError)
            {
                GlobalFree(_preList);
                _preList = (LPRASENTRYNAMEA)GlobalAlloc(LMEM_FIXED, dwBufSize);
                if(NULL == _preList)
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

    if(_preList && (ERROR_SUCCESS != _dwLastError))
    {
        GlobalFree(_preList);
        _preList = NULL;
        _dwEntries = 0;
    }

    return;
}

RasEnumHelp::~RasEnumHelp()
{
    if(_preList)
    {
        GlobalFree(_preList);
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

LPWSTR
RasEnumHelp::GetEntryW(
    DWORD dwEntryNum
    )
{
    LPWSTR  pwszName = NULL;

    if(dwEntryNum >= _dwEntries)
    {
        return NULL;
    }

    switch(_EnumType)
    {
    case ENUM_MULTIBYTE:
        MultiByteToWideChar(CP_ACP, 0, _preList[dwEntryNum].szEntryName,
            -1, _szCurrentEntryW, RAS_MaxEntryName + 1);
        pwszName = _szCurrentEntryW;
        break;
    case ENUM_UNICODE:
        {
        LPRASENTRYNAMEW lpTemp = (LPRASENTRYNAMEW)_preList;
        pwszName = lpTemp[dwEntryNum].szEntryName;
        break;
        }
    case ENUM_WIN2K:
        {
        LPW2KRASENTRYNAMEW lpTemp = (LPW2KRASENTRYNAMEW)_preList;
        pwszName = lpTemp[dwEntryNum].szEntryName;
        break;
        }
    }

    return pwszName;
}

/////////////////////////////////////////////////////////////////////////////
//
//        NAME:           MakeNewConnectoid
//
//        SYNOPSIS:       Launches RNA new connectoid wizard; selects newly
//                                created connectoid (if any) in combo box
//
/////////////////////////////////////////////////////////////////////////////

typedef BOOL (*PFRED)(LPTSTR, LPTSTR, LPRASENTRYDLG);

BOOL MakeNewConnectoid(HWND hDlg, PCONNINFO pConn)
{
    BOOL fRet = FALSE, fDone = FALSE;
    DWORD dwRes = 0;

    ASSERT(lpRasCreatePhonebookEntryA);

    if(FALSE == g_fWin95) {
        // on NT, use RasEntryDlg so we know who we created and can edit
        // proxy info for that connectoid
        HMODULE hRasDlg = LoadLibrary(TEXT("rasdlg.dll"));
        if(hRasDlg) {
#ifdef UNICODE
            PFRED pfred = (PFRED)GetProcAddress(hRasDlg, "RasEntryDlgW");
#else
            PFRED pfred = (PFRED)GetProcAddress(hRasDlg, "RasEntryDlgA");
#endif
            if(pfred) {
                RASENTRYDLG info;

                memset(&info, 0, sizeof(RASENTRYDLG));
                info.dwSize = sizeof(RASENTRYDLG);
                info.hwndOwner = hDlg;
                info.dwFlags = RASEDFLAG_NewEntry;

                dwRes = (pfred)(NULL, NULL, &info);
                if(dwRes) {
                    DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_DIALUP), hDlg,
                        DialupDlgProc, (LPARAM)info.szEntry);
                    dwRes = ERROR_SUCCESS;

                    // save name as default
                    lstrcpyn(pConn->szEntryName, info.szEntry, RAS_MaxEntryName);
                } else {
                    dwRes = info.dwError;
                }
                fDone = TRUE;
            }

            FreeLibrary(hRasDlg);
        }
    }

    if(FALSE == fDone) {
        // on win95, show the ui to make new entry
        if(lpRasCreatePhonebookEntryA)
        {
            dwRes = (lpRasCreatePhonebookEntryA)(hDlg,NULL);
        }
    }

    if(ERROR_SUCCESS == dwRes) {
        // make sure dial default is turned on.  If this is NT, default entry
        // is set above to new entry.
        if(IsDlgButtonChecked(hDlg, IDC_DIALUP_NEVER))
        {
            CheckRadioButton(hDlg, IDC_DIALUP_NEVER, IDC_DIALUP, IDC_DIALUP);
        }
        PopulateRasEntries(hDlg, pConn);
        EnableConnectionControls(hDlg, pConn, FALSE);
        fRet = TRUE;
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////
//
//        NAME:           PopulateRasEntries
//
//        ENTRY:          hwndDlg - dlg box window handle
//
//        SYNOPSIS:       Fills specified combo box with list of existing RNA
//                                connectoids
//
///////////////////////////////////////////////////////////////////////////

#define DEF_ENTRY_BUF_SIZE      8192

DWORD PopulateRasEntries(HWND hDlg, PCONNINFO pConn)
{
    HWND hwndTree = GetDlgItem(hDlg, IDC_CONN_LIST);
    DWORD i;
    DWORD dwBufSize = 16 * sizeof(RASENTRYNAMEA);
    DWORD dwEntries = 0;
    TVITEM tvi;
    TVINSERTSTRUCT tvins;
    HTREEITEM hFirst = NULL;

    ASSERT(hwndTree);

    // init tvi and tvins
    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    tvi.lParam = 0;
    tvins.hInsertAfter = (HTREEITEM)TVI_SORT;
    tvins.hParent = TVI_ROOT;

    // clear list
    TreeView_DeleteAllItems(hwndTree);

    // any old htree is now bogus - we'll get a new one
    pConn->hDefault = NULL;

    // enumerate
    RasEnumHelp reh;

    if(ERROR_SUCCESS == reh.GetError())
    {
        TCHAR szTemp[RAS_MaxEntryName + 64];
        BOOL fDefault, fFoundDefault = FALSE;
        LPTSTR pszEntryName;

        // insert connectoid names from buffer into combo box
        for(i=0; i<reh.GetEntryCount(); i++)
        {
            pszEntryName = reh.GetEntryW(i);
            fDefault = FALSE;

            // if there's only one entry, force it to be the default
            if(1 == dwEntries)
            {
                StrCpyN(pConn->szEntryName, pszEntryName, RAS_MaxEntryName);
            }

            if(*pConn->szEntryName && 0 == StrCmp(pszEntryName, pConn->szEntryName)) {
                // this is the default entry - stick it in the default
                // text control and append (Default) to it
                SetWindowText(GetDlgItem(hDlg, IDC_DIAL_DEF_ISP), pConn->szEntryName);
                StrCpyN(szTemp, pszEntryName, RAS_MaxEntryName);
                MLLoadString(IDS_DEFAULT_TEXT, szTemp + lstrlen(szTemp), 64);
                tvi.pszText = szTemp;
                fDefault = TRUE;
                fFoundDefault = TRUE;
            } else {
                tvi.pszText = pszEntryName;
            }
            tvi.iImage = IMAGE_MODEM;
            tvi.iSelectedImage = IMAGE_MODEM;
            tvi.lParam = i;
            tvins.item = tvi;
            HTREEITEM hItem = TreeView_InsertItem(hwndTree, &tvins);
            if(NULL == hFirst)
                hFirst = hItem;
            if(fDefault)
                pConn->hDefault = hItem;
        }

        // if we didn't match our default with a connectoid, kill it
        if(FALSE == fFoundDefault)
        {
            *pConn->szEntryName = 0;
            MLLoadString(IDS_NONE, szTemp, 64);
            SetWindowText(GetDlgItem(hDlg, IDC_DIAL_DEF_ISP), szTemp);
        }
    }

    // select default or first entry if there is one
    if(pConn->hDefault)
    {
        TreeView_Select(hwndTree, pConn->hDefault, TVGN_CARET);
    }
    else if(hFirst)
    {
        TreeView_Select(hwndTree, hFirst, TVGN_CARET);
    }

    return reh.GetEntryCount();
}

void PopulateProxyControls(HWND hDlg, LPPROXYINFO pInfo, BOOL fSetText)
{
    BOOL fManual = FALSE, fScript = FALSE, fDisable, fTemp;

    // decide if everything is disabled
    fDisable = IsDlgButtonChecked(hDlg, IDC_DONT_USE_CONNECTION);

    //
    // disable proxy enable check box if proxy restricted
    //
    fTemp = fDisable || g_restrict.fProxy;
    EnableDlgItem(hDlg, IDC_MANUAL, !fTemp);
    if(FALSE == g_restrict.fProxy)
    {
        fManual = !fDisable && pInfo->fEnable;
    }

    //
    // Disable autoconfig if restricted
    //
    fScript = !fDisable && IsDlgButtonChecked(hDlg, IDC_CONFIGSCRIPT);

    fTemp = fDisable || g_restrict.fAutoConfig;
    EnableDlgItem(hDlg, IDC_CONFIGSCRIPT, !fTemp);
    EnableDlgItem(hDlg, IDC_AUTODISCOVER, !fTemp);
    if(fTemp)
    {
        fScript = FALSE;
    }

    // enable config script controls
    EnableDlgItem(hDlg, IDC_CONFIG_ADDR, fScript);
    EnableDlgItem(hDlg, IDC_CONFIGADDR_TX, fScript);
    EnableDlgItem(hDlg, IDC_AUTOCNFG_ADVANCED, fScript);

    // Button is always on and omit local addresses is available if proxy is checked
    EnableDlgItem(hDlg, IDC_PROXY_ADVANCED, fManual);
    EnableDlgItem(hDlg, IDC_PROXY_OMIT_LOCAL_ADDRESSES, fManual);

    // Enable dial controls as necessary
    EnableDlgItem(hDlg, IDC_USER, !fDisable && !pInfo->fCustomHandler);
    EnableDlgItem(hDlg, IDC_PASSWORD, !fDisable && !pInfo->fCustomHandler);
    EnableDlgItem(hDlg, IDC_DOMAIN, !fDisable && !pInfo->fCustomHandler);
    EnableDlgItem(hDlg, IDC_TX_USER, !fDisable && !pInfo->fCustomHandler);
    EnableDlgItem(hDlg, IDC_TX_PASSWORD, !fDisable && !pInfo->fCustomHandler);
    EnableDlgItem(hDlg, IDC_TX_DOMAIN, !fDisable && !pInfo->fCustomHandler);
    EnableDlgItem(hDlg, IDC_RAS_SETTINGS, !fDisable);
    EnableDlgItem(hDlg, IDC_DIAL_ADVANCED, !fDisable && !pInfo->fCustomHandler);

    // settings changed in here are enabled/disabled based on the actual proxy settings
    if(StrChr(pInfo->szProxy, TEXT('=')))
    {
        // different servers for each - disable fields on this dialog
        fManual = FALSE;
        if (fSetText)
        {
            SetWindowText(GetDlgItem(hDlg, IDC_PROXY_ADDR), TEXT(""));
            SetWindowText(GetDlgItem(hDlg, IDC_PROXY_PORT), TEXT(""));
        }
    }
    else if (fSetText)
    {
        TCHAR       *pszColon, *pszColon2;
        //Is there a : in the proxy string ?
        pszColon = StrChr(pInfo->szProxy, TEXT(':'));
        if(pszColon)
        {
            //Yes, Find if we have another ':'
            pszColon2 = StrChr(pszColon + 1, TEXT(':'));
            if(pszColon2)
            {
                //Yes, so we have strig like http://itgproxy:80
                pszColon = pszColon2;
                SetWindowText(GetDlgItem(hDlg, IDC_PROXY_PORT), pszColon + 1);
                *pszColon = 0;
            }
            else
            {
                //No, We dont have a second ':'

                int ilength =  (int) (pszColon - pInfo->szProxy);
                //Are there atleast two characters  left beyond the first ':'
                if (lstrlen(pInfo->szProxy) - ilength >= 2 )
                {
                    //Yes, Are Those characters equal //
                    if((pInfo->szProxy[++ilength] == TEXT('/')) &&
                        (pInfo->szProxy[++ilength] == TEXT('/')))
                    {
                        //Yes then we have string like http://itgproxy
                        //make the whole thing as the server and make port fiel empty
                       SetWindowText(GetDlgItem(hDlg, IDC_PROXY_PORT), TEXT(""));
                    }
                    else
                    {
                        //No, so we have string like itgproxy:80.
                        SetWindowText(GetDlgItem(hDlg, IDC_PROXY_PORT), pszColon + 1);
                        *pszColon = 0;
                    }
                }
                else
                {
                  //No We dont have atleast two character so lets parse this as server and port
                  // Assuming this strign to be something like itgproxy:8
                  SetWindowText(GetDlgItem(hDlg, IDC_PROXY_PORT), pszColon + 1);
                  *pszColon = 0;
                }

            }
        }
        else
        {
            //No we dont have a : so treat the string as just the proxy server.
            //Case itgproxy
            SetWindowText(GetDlgItem(hDlg, IDC_PROXY_PORT), TEXT(""));
        }
        SetWindowText(GetDlgItem(hDlg, IDC_PROXY_ADDR), pInfo->szProxy);
    }


    EnableDlgItem(hDlg, IDC_ADDRESS_TEXT,   fManual);
    EnableDlgItem(hDlg, IDC_PORT_TEXT,      fManual);
    EnableDlgItem(hDlg, IDC_PROXY_ADDR,     fManual);
    EnableDlgItem(hDlg, IDC_PROXY_PORT,     fManual);
}

void GetProxyInfo(HWND hDlg, PDIALINFO pDI)
{
    pDI->proxy.fEnable = IsDlgButtonChecked(hDlg, IDC_MANUAL);

    if(NULL == StrChr(pDI->proxy.szProxy, TEXT('=')))
    {
        //
        // not per-protocol, so read edit boxes
        //
        TCHAR szProxy[MAX_URL_STRING];
        TCHAR szPort[INTERNET_MAX_PORT_NUMBER_LENGTH + 1];

        GetWindowText(GetDlgItem(hDlg, IDC_PROXY_ADDR), szProxy, ARRAYSIZE(szProxy) );
        GetWindowText(GetDlgItem(hDlg, IDC_PROXY_PORT), szPort, ARRAYSIZE(szPort) );

        // if we got a proxy and a port, combine in to one string
        if(*szProxy && *szPort)
            wnsprintf(pDI->proxy.szProxy, ARRAYSIZE(pDI->proxy.szProxy), TEXT("%s:%s"), szProxy, szPort);
        else
            StrCpyN(pDI->proxy.szProxy, szProxy, ARRAYSIZE(pDI->proxy.szProxy));
    }

    //
    // fix manual settings override
    //
    pDI->proxy.fOverrideLocal = IsDlgButtonChecked(hDlg, IDC_PROXY_OMIT_LOCAL_ADDRESSES);

    if(pDI->proxy.fOverrideLocal) {
        RemoveLocalFromExceptionList(pDI->proxy.szOverride);
        if(*pDI->proxy.szOverride)
            wnsprintf(pDI->proxy.szOverride, ARRAYSIZE(pDI->proxy.szOverride), TEXT("%s;%s"), pDI->proxy.szOverride, cszLocalString);
        else
            StrCpyN(pDI->proxy.szOverride, cszLocalString, ARRAYSIZE(pDI->proxy.szOverride));
    }
}

//////////////////////////////////////////////////////////////////////
//
//        NAME:       DeleteRasEntry
//
//        SYNOPSIS:   Delete a connectoid
//
//////////////////////////////////////////////////////////////////////
void DeleteRasEntry(LPTSTR pszEntry)
{
    // Use RasDeleteEntryW if possible
    if(lpRasDeleteEntryW)
    {
        (lpRasDeleteEntryW)(NULL, pszEntry);
    }
    else
    {
        CHAR szEntryA[MAX_PATH];
        SHUnicodeToAnsi(pszEntry, szEntryA, ARRAYSIZE(szEntryA));

        // Use RasDeleteEntryA if possible
        if(lpRasDeleteEntryA)
        {
            (lpRasDeleteEntryA)(NULL, szEntryA);
        }
        else
        {
            // no RasDeleteEntry - must by Win95 gold machine.  Use RNA. Ick.
            if( lpRnaActivateEngine &&
                lpRnaDeleteEntry &&
                lpRnaDeactivateEngine &&
                ERROR_SUCCESS == (lpRnaActivateEngine)())
            {
                (lpRnaDeleteEntry)(szEntryA);
                (lpRnaDeactivateEngine)();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
//
//        NAME:       ChangeDefault
//
//        SYNOPSIS:   Change default connectoid to currently selected one
//
//////////////////////////////////////////////////////////////////////
void ChangeDefault(HWND hDlg, PCONNINFO pConn)
{
    TVITEM tvi;
    HWND hwndTree = GetDlgItem(hDlg, IDC_CONN_LIST);
    HTREEITEM hCur;

    memset(&tvi, 0, sizeof(TVITEM));

    // find current selection - if there isn't one, bail
    hCur = TreeView_GetSelection(hwndTree);
    if(NULL == hCur)
        return;

    // remove (default) from current default
    if(pConn->hDefault) {
        tvi.mask = TVIF_HANDLE | TVIF_TEXT;
        tvi.hItem = pConn->hDefault;
        tvi.pszText = pConn->szEntryName;
        tvi.cchTextMax = RAS_MaxEntryName;
        TreeView_SetItem(hwndTree, &tvi);
    }

    // get text for current item
    tvi.mask = TVIF_HANDLE | TVIF_TEXT;
    tvi.hItem = hCur;
    tvi.pszText = pConn->szEntryName;
    tvi.cchTextMax = RAS_MaxEntryName;
    TreeView_GetItem(hwndTree, &tvi);

    // fill in default text field
    SetWindowText(GetDlgItem(hDlg, IDC_DIAL_DEF_ISP), pConn->szEntryName);

    // make sure cover exclude is 0 for this connectoid
    SetCoverExclude(pConn->szEntryName, 0);

    // add (default) to current selection
    TCHAR szTemp[RAS_MaxEntryName + 64];

    StrCpyN(szTemp, pConn->szEntryName, RAS_MaxEntryName);
    MLLoadString(IDS_DEFAULT_TEXT, szTemp + lstrlen(szTemp), 64);

    // stick it back in the tree
    tvi.mask = TVIF_HANDLE | TVIF_TEXT;
    tvi.hItem = hCur;
    tvi.pszText = szTemp;
    tvi.cchTextMax = RAS_MaxEntryName;
    TreeView_SetItem(hwndTree, &tvi);

    // save htree
    pConn->hDefault = hCur;
}

//////////////////////////////////////////////////////////////////////
//
//        NAME:       ShowConnProps
//
//        SYNOPSIS:   Show properties for selected connection
//
//////////////////////////////////////////////////////////////////////

HTREEITEM GetCurSel(PCONNINFO pConn, HWND hDlg, LPTSTR pszBuffer, int iLen, BOOL *pfChecked)
{
    HWND    hwndTree = GetDlgItem(hDlg, IDC_CONN_LIST);
    TVITEM  tvi;

    tvi.hItem = TreeView_GetSelection(hwndTree);

    if(tvi.hItem) {
        tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE;
        tvi.stateMask = TVIS_STATEIMAGEMASK;

        // get test if needed
        if(pszBuffer) {
            tvi.mask |= TVIF_TEXT;
            tvi.pszText = pszBuffer;
            tvi.cchTextMax = iLen;
        }
        TreeView_GetItem(hwndTree, &tvi);

        if(pfChecked)
            *pfChecked = (BOOL)(tvi.state >> 12) - 1;
    }

    // if this is the default connectiod, return name without (default) part
    if(pszBuffer && tvi.hItem == pConn->hDefault) {
        StrCpyN(pszBuffer, pConn->szEntryName, iLen);
    }

    return tvi.hItem;
}

void ShowConnProps(HWND hDlg, PCONNINFO pConn, BOOL fLan)
{
    HTREEITEM   hItem = NULL;
    TCHAR       szEntryName[RAS_MaxEntryName+1];
    BOOL        fChecked = FALSE;

    // default to lan
    *szEntryName = 0;

    // find item of interest
    if(FALSE == fLan)
        hItem = GetCurSel(pConn, hDlg, szEntryName, RAS_MaxEntryName, &fChecked);

    if(hItem || fLan) {
        // show settings
        DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_DIALUP), hDlg,
            DialupDlgProc, (LPARAM)szEntryName);
    }
}

BOOL GetConnSharingDll(LPTSTR pszPath)
{
    DWORD cb = SIZEOF(TCHAR) * MAX_PATH;
    return SHGetValue(HKEY_LOCAL_MACHINE, 
                TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
                TEXT("SharingDLL"), NULL, pszPath, &cb) == ERROR_SUCCESS;
}

BOOL IsConnSharingAvail()
{
    TCHAR szPath[MAX_PATH];
    return GetConnSharingDll(szPath);
}

typedef HRESULT (WINAPI *PFNCONNECTIONSHARING)(HWND hwnd, DWORD dwFlags);

void ShowConnSharing(HWND hDlg)
{
    TCHAR szPath[MAX_PATH];
    if (GetConnSharingDll(szPath))
    {
        HMODULE hmod = LoadLibrary(szPath);
        if (hmod)
        {
            PFNCONNECTIONSHARING pfn = (PFNCONNECTIONSHARING)GetProcAddress(hmod, "InternetConnectionSharing");
            if (pfn)
                pfn(hDlg, 0);
            FreeLibrary(hmod);
        }
    }
}


//////////////////////////////////////////////////////////////////////
//
//        NAME:       ConnectionDlgProc
//
//        SYNOPSIS:   Connection property sheet dialog proc.
//
//////////////////////////////////////////////////////////////////////


INT_PTR CALLBACK ConnectionDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                LPARAM lParam)
{
    PCONNINFO pConn = (PCONNINFO)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    if (NULL == pConn && uMsg != WM_INITDIALOG)
    {
        return FALSE;
    }

    switch (uMsg)
    {
        case WM_INITDIALOG:
            // build and save conninfo struct
            pConn = new CONNINFO;
            if(NULL == pConn)
                return FALSE;
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pConn);
            memset(pConn, 0, sizeof(CONNINFO));

            return ConnectionDlgInit(hDlg, pConn);

        case WM_DESTROY:
        {
            UnloadRNADll();

            // Free the image list used by the connection list
            HWND hwndConnList = GetDlgItem(hDlg, IDC_CONN_LIST);
            HIMAGELIST himl = TreeView_SetImageList(hwndConnList, NULL, TVSIL_NORMAL);
            if (himl)
            {
                ImageList_Destroy(himl);
            }

            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)NULL);
            delete pConn;
            return TRUE;
        }
        case WM_NOTIFY:
        {
            NMHDR * lpnm = (NMHDR *) lParam;
            switch (lpnm->code)
            {
                case TVN_KEYDOWN:
                {
                    TV_KEYDOWN *pkey = (TV_KEYDOWN*)lpnm;
                    if(pkey->wVKey == VK_SPACE)
                    {
                        ENABLEAPPLY(hDlg);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE); // eat the key
                        return TRUE;
                    }
                    break;
                }

                case NM_CLICK:
                case NM_DBLCLK:
                {   // is this click in our tree?
                    HWND hwndTree = GetDlgItem(hDlg, IDC_CONN_LIST);
                    if(lpnm->idFrom == IDC_CONN_LIST)
                    {   // yes...
                        TV_HITTESTINFO  ht;
                        HTREEITEM       hItem;

                        GetCursorPos(&ht.pt);
                        ScreenToClient(hwndTree, &ht.pt);
                        hItem = TreeView_HitTest(hwndTree, &ht);
                        TreeView_SelectItem(hwndTree, hItem);

                        if(ht.flags & TVHT_ONITEMSTATEICON) {
                            // clicked on state - if turned off default
                            // connectoid, uncheck dial
                            if(hItem == pConn->hDefault) {
                                BOOL fChecked;
                                GetCurSel(pConn, hDlg, NULL, 0, &fChecked);
                                if(fChecked) {
                                    // is about to be unchecked...
                                    CheckRadioButton(hDlg, IDC_DIALUP_NEVER, IDC_DIALUP, IDC_DIALUP_NEVER);
                                }
                            }

                            ENABLEAPPLY(hDlg);
                        }

                        // if it's a double click, change default
                        if(NM_DBLCLK == lpnm->code)
                            ShowConnProps(hDlg, pConn, FALSE);
                    }
                    EnableConnectionControls(hDlg, pConn, FALSE);
                    break;
                }

                case TVN_SELCHANGED:
                    EnableConnectionControls(hDlg, pConn, FALSE);
                    break;

                case PSN_QUERYCANCEL:
                case PSN_KILLACTIVE:
                case PSN_RESET:
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    return TRUE;

                case PSN_APPLY:
                {
                    BOOL fRet = ConnectionDlgOK(hDlg, pConn);
                    SetPropSheetResult(hDlg,!fRet);
                    return !fRet;
                    break;
                }
            }
            break;
        }

        case WM_COMMAND:
            switch  (LOWORD(wParam))
            {
                case IDC_LAN_SETTINGS:
                    ShowConnProps(hDlg, pConn, TRUE);
                    break;

                case IDC_CON_SHARING:
                    ShowConnSharing(hDlg);
                    break;

                case IDC_DIALUP_ADD:
                    MakeNewConnectoid(hDlg, pConn);
                    break;

                case IDC_DIALUP_REMOVE:
                {
                    TCHAR   szEntryName[RAS_MaxEntryName+1];

                    if (GetCurSel(pConn, hDlg, szEntryName, RAS_MaxEntryName, NULL) &&
                        *szEntryName) {
                        if(IDOK == MsgBox(hDlg, IDS_DELETECONNECTOID, MB_ICONWARNING, MB_OKCANCEL)) {
                            DeleteRasEntry(szEntryName);
                            PopulateRasEntries(hDlg, pConn);

                            // fix controls
                            EnableConnectionControls(hDlg, pConn, FALSE);
                        }
                    }
                    break;
                }


                case IDC_DIALUP:
                case IDC_DIALUP_ON_NONET:
                    // make sure default connectoid is active
                    SetCoverExclude(pConn->szEntryName, 0);

                    // Fall through...

                case IDC_DIALUP_NEVER:

                    // fix radio buttons
                    CheckRadioButton(hDlg, IDC_DIALUP_NEVER, IDC_DIALUP, LOWORD(wParam));

                    // enable/disable other controls appropriately
                    EnableConnectionControls(hDlg, pConn, FALSE);
                    ENABLEAPPLY(hDlg);
                    break;

                case IDC_ENABLE_SECURITY:
                    ENABLEAPPLY(hDlg);
                    break;

                case IDC_SET_DEFAULT:
                    ChangeDefault(hDlg, pConn);
                    EnableConnectionControls(hDlg, pConn, FALSE);
                    ENABLEAPPLY(hDlg);
                    break;

                case IDC_MODEM_SETTINGS:
                    ShowConnProps(hDlg, pConn, FALSE);
                    break;

                case IDC_CONNECTION_WIZARD:
                {
                    TCHAR       szICWReg[MAX_PATH];
                    TCHAR       szICWPath[MAX_PATH + 1];
                    DWORD       cbSize = MAX_PATH, dwType;

                    // Try and get ICW from IOD.  If it fails, try to run
                    // ICW anyway.  We may luck out and get an old one.
                    DWORD dwRes = JitFeature(hDlg, clsidFeatureICW, FALSE);

                    // find path of ICW
                    MLLoadString(IDS_ICW_NAME, szICWPath, MAX_PATH);
                    wnsprintf(szICWReg, ARRAYSIZE(szICWReg), TEXT("%s\\%s"), REGSTR_PATH_APPPATHS, szICWPath);

                    // read app paths key
                    if(ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, szICWReg, NULL, &dwType, szICWPath, &cbSize))
                        break;

                    // run connection wizard
                    STARTUPINFO si;
                    PROCESS_INFORMATION pi;
                    memset(&si, 0, sizeof(si));
                    si.cb = sizeof(si);

                    if(CreateProcess(NULL, szICWPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                    {
                        // successfully ran ICW - get rid of this dialog
                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);
                        PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
                    }
                    break;
                }
            }
            break;

        case WM_HELP:      // F1
            ResWinHelp((HWND) ((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                       HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            ResWinHelp((HWND)wParam, IDS_HELPFILE,
                       HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

/*******************************************************************

NAME:           ConnectionDlgOK

SYNOPSIS:       OK button handler for connection prop page

********************************************************************/

// prototype for IsNetworkAlive()
typedef BOOL (WINAPI *ISNETWORKALIVE)(LPDWORD);

BOOL ConnectionDlgOK(HWND hDlg, PCONNINFO pConn)
{
    DWORD   dwValue, dwNoNetValue;

    RegEntry re(REGSTR_PATH_INTERNETSETTINGS,HKEY_CURRENT_USER);
    RegEntry reCC(REGSTR_PATH_INTERNETSETTINGS,HKEY_CURRENT_CONFIG);
    if(ERROR_SUCCESS == re.GetError()) {

        // autodial
        dwValue = 0;
        dwNoNetValue = 0;
        if(IsDlgButtonChecked(hDlg, IDC_DIALUP))
        {
            dwValue = 1;
        }
        else if(IsDlgButtonChecked(hDlg, IDC_DIALUP_ON_NONET))
        {
            dwValue = 1;
            dwNoNetValue = 1;

            DWORD dwRes = JitFeature(hDlg, clsidFeatureMobile, FALSE);
            if(JIT_PRESENT != dwRes) {
                // user doesn't want MOP, change to dial always.
                dwNoNetValue = 0;
            }
            else
            {
                // Call IsNetworkAlive.  This will start sens service
                // and next instance of wininet will use it.
                HINSTANCE hSens;
                ISNETWORKALIVE pfnIsNetworkAlive;
                DWORD dwFlags;

                hSens = LoadLibrary(TEXT("sensapi.dll"));
                if(hSens)
                {
                    pfnIsNetworkAlive = (ISNETWORKALIVE)GetProcAddress(hSens, "IsNetworkAlive");
                    if(pfnIsNetworkAlive)
                    {
                        // Call it.  Don't really care about the result.
                        pfnIsNetworkAlive(&dwFlags);
                    }
                    FreeLibrary(hSens);
                }
            }
        }
        re.SetValue(REGSTR_VAL_ENABLEAUTODIAL, dwValue);
        re.SetValue(REGSTR_VAL_NONETAUTODIAL, dwNoNetValue);

        // in order to support Docking/Undocking.
        reCC.SetValue(REGSTR_VAL_ENABLEAUTODIAL, dwValue);
        reCC.SetValue(REGSTR_VAL_NONETAUTODIAL, dwNoNetValue);

        // save default connectoid
        if(*pConn->szEntryName) {
            RegEntry reTmp(REGSTR_PATH_REMOTEACESS,HKEY_CURRENT_USER);
            if (reTmp.GetError() == ERROR_SUCCESS) {
                reTmp.SetValue(REGSTR_VAL_INTERNETENTRY,pConn->szEntryName);
            }
        }
    }

    // save security check state on win95
    if(g_fWin95)
    {
        dwValue = 0;
        if(IsDlgButtonChecked(hDlg, IDC_ENABLE_SECURITY))
        {
            dwValue = 1;
        }
        re.SetValue(REGSTR_VAL_ENABLESECURITYCHECK, dwValue);
    }

    //
    // Have wininet refresh it's connection settings
    //
    InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);

    UpdateAllWindows();

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//        NAME:           EnableConnectionControls
//
//        SYNOPSIS:       Enables controls appropriately depending on what
//                                checkboxes are checked.
//
/////////////////////////////////////////////////////////////////////////////

#define REGSTR_CCS_CONTROL_WINDOWS  REGSTR_PATH_CURRENT_CONTROL_SET TEXT("\\WINDOWS")
#define CSDVERSION      TEXT("CSDVersion")
#define NTSP3_VERSION   0x0300

BOOL IsNT4SP3(void)
{
    HKEY    hKey;
    DWORD   dwCSDVersion;
    DWORD   dwSize;
    BOOL    bNTSP3 = FALSE;
    OSVERSIONINFO VerInfo;

    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&VerInfo);

    // make sure we're on NT4
    if (VER_PLATFORM_WIN32_NT != VerInfo.dwPlatformId || VerInfo.dwMajorVersion > 4)
        return FALSE;

    // check for installed SP3
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_CCS_CONTROL_WINDOWS, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwCSDVersion);
        if (RegQueryValueEx(hKey, CSDVERSION, NULL, NULL, (unsigned char*)&dwCSDVersion, &dwSize) == ERROR_SUCCESS)
        {
            bNTSP3 = (LOWORD(dwCSDVersion) == NTSP3_VERSION);
        }
        RegCloseKey(hKey);
    }
    return bNTSP3;
}

VOID EnableConnectionControls(HWND hDlg, PCONNINFO pConn, BOOL fSetText)
{
    TCHAR   szEntryName[RAS_MaxEntryName + 1];
    BOOL    fList = FALSE, fDial = FALSE, fAutodial = FALSE;
    BOOL    fAdd = FALSE, fSettings = FALSE, fLan = TRUE, fSetDefault = TRUE;
    BOOL    fDialDefault = FALSE, fNT4SP3;
    HTREEITEM   hItem;
    int     iCount;

    fNT4SP3 = IsNT4SP3();

    if(fNT4SP3)
    {
        // no sens stuff on NT4SP3, so make sure on no net isn't picked
        if(IsDlgButtonChecked(hDlg, IDC_DIALUP_ON_NONET))
        {
            CheckRadioButton(hDlg, IDC_DIALUP_NEVER, IDC_DIALUP, IDC_DIALUP);
        }
    }

    //
    // Check out how much stuff is in the tree view and what's selected
    //
    iCount = TreeView_GetCount(GetDlgItem(hDlg, IDC_CONN_LIST));
    hItem = GetCurSel(pConn, hDlg, szEntryName, RAS_MaxEntryName, NULL);

    if(dwRNARefCount) {
        // Ras is loaded so enable list control
        fList = TRUE;

        // if anything is selected, turn on settings button
        if(hItem)
        {
            fSettings = TRUE;
            if(hItem == pConn->hDefault)
            {
                fSetDefault = FALSE;
            }
        }

        // Ensure ras is loaded
        if(iCount > 0)
            fDial = TRUE;
    }

    // check to see if dial default is checked
    if(fDial)
        fDialDefault = !IsDlgButtonChecked(hDlg, IDC_DIALUP_NEVER);

    if(fList && lpRasCreatePhonebookEntryA)
        fAdd = TRUE;

    // if dialing restriction is present, make sure user can't do nothing.
    if(g_restrict.fDialing)
        fAdd = fList = fDial = fDialDefault = fAutodial = fSettings = fLan = fSetDefault = FALSE;

    // enable list controls
    EnableDlgItem(hDlg, IDC_CONN_LIST,       fList);
    EnableDlgItem(hDlg, IDC_DIALUP_ADD,      fAdd);
    EnableDlgItem(hDlg, IDC_DIALUP_REMOVE,   fSettings);
    EnableDlgItem(hDlg, IDC_MODEM_SETTINGS,  fSettings);

    // enable lan controls
    EnableDlgItem(hDlg, IDC_LAN_SETTINGS,    fLan);

    // enable default controls
    EnableDlgItem(hDlg, IDC_DIALUP_NEVER,    fDial);
    EnableDlgItem(hDlg, IDC_DIALUP_ON_NONET, fDial && !fNT4SP3);
    EnableDlgItem(hDlg, IDC_DIALUP,          fDial);
    EnableDlgItem(hDlg, IDC_DIAL_DEF_TXT,    fDialDefault);
    EnableDlgItem(hDlg, IDC_DIAL_DEF_ISP,    fDialDefault);
    EnableDlgItem(hDlg, IDC_ENABLE_SECURITY, fDialDefault);
    EnableDlgItem(hDlg, IDC_SET_DEFAULT,     fDialDefault && fSetDefault);

    // if autodialing is disabled (no connectoids) make sure it's not checked
    if(FALSE == fDial)
        CheckRadioButton(hDlg, IDC_DIALUP_NEVER, IDC_DIALUP, IDC_DIALUP_NEVER);

    //
    // Fix connection wizard
    //
    if (g_restrict.fConnectionWizard)
    {
        EnableDlgItem(hDlg, IDC_CONNECTION_WIZARD, FALSE);
    }
}

BOOL ConnectionDlgInit(HWND hDlg, PCONNINFO pConn)
{
    BOOL        fProxy = FALSE;
    BOOL        fDial = FALSE;
    BOOL        fSuccessfulRegRead;
    HKEY        hKey;
    HIMAGELIST  himl;
    HICON       hIcon;

    // Get platform - we need this as there's no security check on NT.
    OSVERSIONINFOA osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionExA(&osvi);

    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) 
    {
        g_fWin95 = FALSE;

        if(osvi.dwMajorVersion > 4)
        {
            g_fWin2K = TRUE;
        }
    }

    // load ras (success checked later - see dwRNARefCount in EnableConnectionControls
    LoadRNADll();

    // create image list for tree view
    himl = ImageList_Create(BITMAP_WIDTH, BITMAP_HEIGHT, ILC_COLOR | ILC_MASK, CONN_BITMAPS, 4 );
    hIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_LAN));
    ImageList_AddIcon(himl, hIcon);
    hIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_PHONE));
    ImageList_AddIcon(himl, hIcon);

    TreeView_SetImageList(GetDlgItem(hDlg, IDC_CONN_LIST), himl, TVSIL_NORMAL);

    // Open the Reg Key.
    //
    RegEntry re(REGSTR_PATH_INTERNETSETTINGS,HKEY_CURRENT_USER);
    if (re.GetError() == ERROR_SUCCESS)
    {
        fSuccessfulRegRead = TRUE;
    }

    // Find default connectoid
    if(RegOpenKeyEx(HKEY_CURRENT_USER,REGSTR_PATH_REMOTEACESS, 0, KEY_READ|KEY_WRITE, &hKey)
            == ERROR_SUCCESS)
    {
        DWORD dwSize = RAS_MaxEntryName + 1;

        if (RegQueryValueEx(hKey,REGSTR_VAL_INTERNETENTRY,NULL,NULL,
                    (LPBYTE) pConn->szEntryName,&dwSize) != ERROR_SUCCESS)
        {
            // No default connectoid - paranoia
            *pConn->szEntryName = 0;
        }
        RegCloseKey(hKey);

    }

    // populate connectoids
    PopulateRasEntries(hDlg, pConn);

    // fix autodial radio buttons
    int iSel = 0;
    DWORD dwAutodial = 0;

    dwAutodial = re.GetNumber(REGSTR_VAL_ENABLEAUTODIAL, 0);
    iSel = IDC_DIALUP_NEVER;
    if(dwAutodial)
    {
        dwAutodial = re.GetNumber(REGSTR_VAL_NONETAUTODIAL, 0);
        if(dwAutodial)
        {
            iSel = IDC_DIALUP_ON_NONET;
        }
        else
        {
            iSel = IDC_DIALUP;
        }
    }
    CheckRadioButton(hDlg, IDC_DIALUP_NEVER, IDC_DIALUP, iSel);

    // enable appropriate controls
    EnableConnectionControls(hDlg, pConn, TRUE);

    // fix security check
    if(g_fWin95)
    {
        if(re.GetNumber(REGSTR_VAL_ENABLESECURITYCHECK,0))
        {
            CheckDlgButton(hDlg, IDC_ENABLE_SECURITY, TRUE);
        }
    }
    else
    {
        // no security check on NT so hide check box
        ShowWindow(GetDlgItem(hDlg, IDC_ENABLE_SECURITY), SW_HIDE);
    }

    if (!IsConnSharingAvail())
        ShowWindow(GetDlgItem(hDlg, IDC_CON_SHARING), SW_HIDE);


    // disable wizard button if for some reason ICW cannot be JITed in
    DWORD dwRes = JitFeature(hDlg, clsidFeatureICW, TRUE);
    if(JIT_NOT_AVAILABLE == dwRes) {
        // can never get ICW so grey button
        EnableWindow(GetDlgItem(hDlg, IDC_CONNECTION_WIZARD), FALSE);
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////
//
//        NAME:           LoadRNADll
//
//        SYNOPSIS:       Loads RNA dll if not already loaded and obtains pointers
//                        for function addresses.
//
//        NOTES:          Maintains a reference count so we know when to unload
//
///////////////////////////////////////////////////////////////////////////

BOOL LoadRNADll(VOID)
{
    // increase reference count
    dwRNARefCount++;

    if (hInstRNADll)
    {
        // already loaded, nothing to do
        return TRUE;
    }

    // Ask wininet if Ras is installed.  Always make this call even if ras
    // dll doesn't load since it also forces wininet to migrate proxy
    // settings if necessary.
    DWORD dwFlags;
    InternetGetConnectedStateExA(&dwFlags, NULL, 0, 0);
    if(0 == (dwFlags & INTERNET_RAS_INSTALLED)) {
        // not installed - none of the functions will work so bail
        dwRNARefCount--;
        return FALSE;
    }

    // get the file name from resource
    TCHAR szDllFilename[SMALL_BUF_LEN+1];
    if (!MLLoadString(IDS_RNADLL_FILENAME,szDllFilename,ARRAYSIZE(szDllFilename))) {
        dwRNARefCount--;
        return FALSE;
    }

    // load the DLL
    hInstRNADll = LoadLibrary(szDllFilename);
    if (!hInstRNADll) {
        dwRNARefCount--;
        return FALSE;
    }

    // cycle through the API table and get proc addresses for all the APIs we
    // need
    UINT nIndex;
    for (nIndex = 0;nIndex < NUM_RNAAPI_PROCS;nIndex++)
    {
        if (!(*RasApiList[nIndex].ppFcnPtr = (PVOID) GetProcAddress(hInstRNADll,
            RasApiList[nIndex].pszName)))
        {
            // no longer fatal - no RasDeleteEntry on Win95 gold.
            TraceMsg(TF_GENERAL, "Unable to get address of function %s", RasApiList[nIndex].pszName);

//          UnloadRNADll();
//          return FALSE;
        }
    }

    if(g_fWin95)
    {
        // make sure we don't use any W versions that may be around on Win9x.
        // They'll almost certainly be stubs.
        lpRasEditPhonebookEntryW   = NULL;
        lpRasEnumEntriesW          = NULL;
        lpRasDeleteEntryW          = NULL;
        lpRasGetEntryDialParamsW   = NULL;
        lpRasSetEntryDialParamsW   = NULL;
        lpRasGetEntryPropertiesW   = NULL;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//        NAME:           UnloadRNADll
//
//        SYNOPSIS:       Decrements RNA dll reference count and unloads it if
//                        zero
//
/////////////////////////////////////////////////////////////////////////////

VOID UnloadRNADll(VOID)
{
    // decrease reference count
    if (dwRNARefCount)
        dwRNARefCount --;

    // unload DLL if reference count hits zero
    if (!dwRNARefCount && hInstRNADll)
    {

        // set function pointers to NULL
        UINT nIndex;
        for (nIndex = 0;nIndex < NUM_RNAAPI_PROCS;nIndex++)
            *RasApiList[nIndex].ppFcnPtr = NULL;

        // free the library
        FreeLibrary(hInstRNADll);
        hInstRNADll = NULL;
    }
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//                           Dialup Dialog ie modem settings
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define W2KRASENTRYW struct tagW2KRASENTRYW
W2KRASENTRYW
{
    DWORD       dwSize;
    DWORD       dwfOptions;
    DWORD       dwCountryID;
    DWORD       dwCountryCode;
    WCHAR       szAreaCode[ RAS_MaxAreaCode + 1 ];
    WCHAR       szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
    DWORD       dwAlternateOffset;
    RASIPADDR   ipaddr;
    RASIPADDR   ipaddrDns;
    RASIPADDR   ipaddrDnsAlt;
    RASIPADDR   ipaddrWins;
    RASIPADDR   ipaddrWinsAlt;
    DWORD       dwFrameSize;
    DWORD       dwfNetProtocols;
    DWORD       dwFramingProtocol;
    WCHAR       szScript[ MAX_PATH ];
    WCHAR       szAutodialDll[ MAX_PATH ];
    WCHAR       szAutodialFunc[ MAX_PATH ];
    WCHAR       szDeviceType[ RAS_MaxDeviceType + 1 ];
    WCHAR       szDeviceName[ RAS_MaxDeviceName + 1 ];
    WCHAR       szX25PadType[ RAS_MaxPadType + 1 ];
    WCHAR       szX25Address[ RAS_MaxX25Address + 1 ];
    WCHAR       szX25Facilities[ RAS_MaxFacilities + 1 ];
    WCHAR       szX25UserData[ RAS_MaxUserData + 1 ];
    DWORD       dwChannels;
    DWORD       dwReserved1;
    DWORD       dwReserved2;
    DWORD       dwSubEntries;
    DWORD       dwDialMode;
    DWORD       dwDialExtraPercent;
    DWORD       dwDialExtraSampleSeconds;
    DWORD       dwHangUpExtraPercent;
    DWORD       dwHangUpExtraSampleSeconds;
    DWORD       dwIdleDisconnectSeconds;
    DWORD       dwType;
    DWORD       dwEncryptionType;
    DWORD       dwCustomAuthKey;
    GUID        guidId;
    WCHAR       szCustomDialDll[MAX_PATH];
    DWORD       dwVpnStrategy;
};


BOOL GetConnectoidInfo(HWND hDlg, LPTSTR pszEntryName)
{
    BOOL    fPassword = FALSE;

    if(g_fWin2K && lpRasGetEntryPropertiesW)
    {
        W2KRASENTRYW    re[2];
        DWORD           dwSize;

        // get props for this connectoid and see if it has a custom dial dll
        re[0].dwSize = sizeof(W2KRASENTRYW);
        dwSize = sizeof(re);
        if(ERROR_SUCCESS == (lpRasGetEntryPropertiesW)(NULL, pszEntryName,
                    (LPRASENTRYW)re, &dwSize, NULL, NULL))
        {
            if(0 != re[0].szCustomDialDll[0])
            {
                // Win2K handler exists - flag that we need to grey out
                // credential fields
                return TRUE;
            }
        }
    }
    else
    {
        // on down level platforms, check registry for cdh
        TCHAR   szTemp[MAX_PATH];

        GetConnKey(pszEntryName, szTemp, 64);
        RegEntry re(szTemp, HKEY_CURRENT_USER);
        if(ERROR_SUCCESS == re.GetError())
        {
            if(re.GetString(REGSTR_VAL_AUTODIALDLLNAME, szTemp, MAX_PATH) && *szTemp)
            {
                // CDH exists - flag that we need to grey credentials
                return TRUE;
            }
        }
    }

    if(lpRasGetEntryDialParamsW)
    {
        RASDIALPARAMSW params;
        WCHAR  *pszUser = L"", *pszPassword = L"", *pszDomain = L"";

        memset(&params, 0, sizeof(params));
        params.dwSize = sizeof(params);

        StrCpyN(params.szEntryName, pszEntryName, RAS_MaxEntryName);
        if(ERROR_SUCCESS == (lpRasGetEntryDialParamsW)(NULL, (LPRASDIALPARAMSW)&params, &fPassword))
        {
            pszUser = params.szUserName;
            if(' ' != params.szDomain[0])
                pszDomain = params.szDomain;
            if(fPassword)
                pszPassword = params.szPassword;
        }

        SetWindowText(GetDlgItem(hDlg, IDC_USER), pszUser);
        SetWindowText(GetDlgItem(hDlg, IDC_DOMAIN), pszDomain);
        SetWindowText(GetDlgItem(hDlg, IDC_PASSWORD), pszPassword);
    }
    else if(lpRasGetEntryDialParamsA)
    {
        RASDIALPARAMSA  params;
        CHAR            *pszUser = "", *pszPassword = "", *pszDomain = "";

        memset(&params, 0, sizeof(params));
        params.dwSize = sizeof(params);
        SHUnicodeToAnsi(pszEntryName, params.szEntryName, ARRAYSIZE(params.szEntryName));

        if(ERROR_SUCCESS == (lpRasGetEntryDialParamsA)(NULL, &params, &fPassword))
        {
            pszUser = params.szUserName;
            if(' ' != params.szDomain[0])
                pszDomain = params.szDomain;
            if(fPassword)
                pszPassword = params.szPassword;
        }

        SetWindowTextA(GetDlgItem(hDlg, IDC_USER), pszUser);
        SetWindowTextA(GetDlgItem(hDlg, IDC_DOMAIN), pszDomain);
        SetWindowTextA(GetDlgItem(hDlg, IDC_PASSWORD), pszPassword);
    }

    return FALSE;
}

#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL                 0x00400000L // Right to left mirroring
#else
#error "WS_EX_LAYOUTRTL is already defined in winuser.h"
#endif // WS_EX_LAYOUTRTL

///////////////////////////////////////////////////////////////////
//
// NAME:       FixDialogForLan
//
// SYNOPSIS:   Remove dialing section of dialog box
//
///////////////////////////////////////////////////////////////////
void FixDialogForLan(HWND hDlg)
{
    RECT rectParent, rectDial, rectNet, rectCur;
    POINT pt;
    int i;

    static int iHideIDs[] = {
        IDC_GRP_DIAL, IDC_RAS_SETTINGS, IDC_TX_USER, IDC_USER, IDC_TX_PASSWORD,
        IDC_PASSWORD, IDC_TX_DOMAIN, IDC_DOMAIN, IDC_DIAL_ADVANCED,
        IDC_DONT_USE_CONNECTION
      };
#define NUM_HIDE (sizeof(iHideIDs) / sizeof(int))

    static int iMoveIDs[] = {
        IDCANCEL, IDOK
      };
#define NUM_MOVE (sizeof(iMoveIDs) / sizeof(int))

    // hide relevant windows
    for(i=0; i<NUM_HIDE; i++) {
        ShowWindow(GetDlgItem(hDlg, iHideIDs[i]), SW_HIDE);
    }

    // move relevant windows (yuck)
    GetWindowRect(hDlg, &rectParent);
    GetWindowRect(GetDlgItem(hDlg, IDC_GRP_DIAL), &rectDial);
    GetWindowRect(GetDlgItem(hDlg, IDC_GRP_PROXY), &rectNet);

    for(i=0; i<NUM_MOVE; i++) {
        GetWindowRect(GetDlgItem(hDlg, iMoveIDs[i]), &rectCur);
        pt.x = (GetWindowLong(hDlg, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) ? rectCur.right : rectCur.left;
        pt.y = rectCur.top;
        ScreenToClient(hDlg, &pt);
        MoveWindow(GetDlgItem(hDlg, iMoveIDs[i]), pt.x,
            pt.y - (rectDial.bottom - rectNet.bottom),
            rectCur.right - rectCur.left,
            rectCur.bottom - rectCur.top,
            TRUE);
    }

    // adjust dialog box size
    MoveWindow(hDlg, rectParent.left, rectParent.top,
        rectParent.right - rectParent.left,
        rectParent.bottom - rectParent.top - (rectDial.bottom - rectNet.bottom),
        TRUE);
}

///////////////////////////////////////////////////////////////////
//
// NAME:       DialupDlgInit
//
// SYNOPSIS:   Does initalization for dialup dialog
//
////////////////////////////////////////////////////////////////////

BOOL DialupDlgInit(HWND hDlg, LPTSTR pszConnectoid)
{
    PDIALINFO pDI;
    TCHAR   szTemp[MAX_PATH], szSettings[64];
    DWORD   dwIEAK = 0, cb;

    // set up dailinfo struct
    pDI = new DIALINFO;
    if(NULL == pDI)
        return FALSE;
    memset(pDI, 0, sizeof(DIALINFO));  // bugbug new already zero init?
#ifndef UNIX
    pDI->pszConnectoid = pszConnectoid;
#else
    // Can't pass lparam from PSheet because we put dialup dialog directly
    // on the tab.
    pszConnectoid = TEXT("");
    pDI->pszConnectoid = pDI->szEntryName;

#endif
    SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pDI);

    // Fix window title
    if(0 == *(pDI->pszConnectoid)) {
        MLLoadString(IDS_LAN_SETTINGS, szTemp, MAX_PATH);
    } else {
        MLLoadString(IDS_SETTINGS, szSettings, 64);
        wnsprintf(szTemp, ARRAYSIZE(szTemp), TEXT("%s %s"), pDI->pszConnectoid, szSettings);
    }
    SetWindowText(hDlg, szTemp);

    // figure out our registry key
    GetConnKey(pDI->pszConnectoid, szTemp, MAX_PATH);

#ifndef UNIX
    // Different stuff if we're editing a connectoid vs. lan settings
    if(NULL == pszConnectoid || 0 == *pszConnectoid) {
        // remove dialing goo from dialog
        FixDialogForLan(hDlg);
    } else {
        // fill in username/password/domain
        pDI->proxy.fCustomHandler = GetConnectoidInfo(hDlg, pszConnectoid);
    }

    // populate use this connection check box
    if(pDI->pszConnectoid && *pDI->pszConnectoid &&
        0 != GetCoverExclude(pDI->pszConnectoid))
    {
        // coverage excluded, check 'don't use connection'
        CheckDlgButton(hDlg, IDC_DONT_USE_CONNECTION, TRUE);
    }
#endif

    // hide advanced button for autoconfig info if IEAK restriction is not set

    cb = sizeof(dwIEAK);
    if ((SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_INETCPL_RESTRICTIONS, REGSTR_VAL_INETCPL_IEAK,
        NULL, (LPVOID)&dwIEAK, &cb) != ERROR_SUCCESS) || !dwIEAK)
        ShowWindow(GetDlgItem(hDlg, IDC_AUTOCNFG_ADVANCED), SW_HIDE);

    // open connectoid or lan settings
    RegEntry reCon(szTemp, HKEY_CURRENT_USER);
    if(ERROR_SUCCESS != reCon.GetError())
        return FALSE;

    //
    // Read proxy and autoconfig settings for this connection
    //
    INTERNET_PER_CONN_OPTION_LIST list;
    DWORD dwBufSize = sizeof(list);

    list.pszConnection = (pszConnectoid && *pszConnectoid) ? pszConnectoid : NULL;
    list.dwSize = sizeof(list);
    list.dwOptionCount = 4;
    list.pOptions = new INTERNET_PER_CONN_OPTION[4];
    if(NULL == list.pOptions)
    {
        return FALSE;
    }

    list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
    list.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    list.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
    list.pOptions[3].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;

    if(FALSE == InternetQueryOption(NULL,
            INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwBufSize))
    {
        delete [] list.pOptions;
        return FALSE;
    }

    //
    // move options to pDI struct
    //
    pDI->proxy.fEnable = (list.pOptions[0].Value.dwValue & PROXY_TYPE_PROXY);
    if(list.pOptions[1].Value.pszValue)
    {
        StrCpyN(pDI->proxy.szProxy, list.pOptions[1].Value.pszValue, MAX_URL_STRING);
        GlobalFree(list.pOptions[1].Value.pszValue);
        list.pOptions[1].Value.pszValue = NULL;
    }
    if(list.pOptions[2].Value.pszValue)
    {
        StrCpyN(pDI->proxy.szOverride, list.pOptions[2].Value.pszValue, MAX_URL_STRING);
        GlobalFree(list.pOptions[2].Value.pszValue);
        list.pOptions[2].Value.pszValue = NULL;
    }

    //
    // fill in dialog fields
    //

    // proxy enable
    if(pDI->proxy.fEnable)
    {
        CheckDlgButton(hDlg, IDC_MANUAL, TRUE);
    }

    // autoconfig enable and url
    if(list.pOptions[0].Value.dwValue & PROXY_TYPE_AUTO_PROXY_URL)
    {
        CheckDlgButton(hDlg, IDC_CONFIGSCRIPT, TRUE);
    }
    if(list.pOptions[3].Value.pszValue)
    {
        SetWindowText(GetDlgItem(hDlg, IDC_CONFIG_ADDR), list.pOptions[3].Value.pszValue);
        GlobalFree(list.pOptions[3].Value.pszValue);
        list.pOptions[3].Value.pszValue = NULL;
    }

    // autodiscovery enable
    if(list.pOptions[0].Value.dwValue & PROXY_TYPE_AUTO_DETECT)
    {
        CheckDlgButton(hDlg, IDC_AUTODISCOVER, TRUE);
    }

    // all done with options list
    delete [] list.pOptions;

    // check enable and override and parse out server and port
    pDI->proxy.fOverrideLocal = RemoveLocalFromExceptionList(pDI->proxy.szOverride);
    CheckDlgButton(hDlg, IDC_PROXY_ENABLE, pDI->proxy.fEnable);
    CheckDlgButton(hDlg, IDC_PROXY_OMIT_LOCAL_ADDRESSES, pDI->proxy.fOverrideLocal);
    PopulateProxyControls(hDlg, &pDI->proxy, TRUE);

    return TRUE;
}

///////////////////////////////////////////////////////////////////
//
// NAME:       DialupDlgOk
//
// SYNOPSIS:   Apply settings for dial up dialog box
//
////////////////////////////////////////////////////////////////////

BOOL DialupDlgOk(HWND hDlg, PDIALINFO pDI)
{
    DWORD   dwValue = 0;
    TCHAR   szTemp[MAX_URL_STRING];

    GetConnKey(pDI->pszConnectoid, szTemp, MAX_PATH);

    // open connectoid or lan settings
    RegEntry reCon(szTemp, HKEY_CURRENT_USER);
    if(ERROR_SUCCESS != reCon.GetError())
        return FALSE;

    RegEntry re(REGSTR_PATH_INTERNETSETTINGS,HKEY_CURRENT_USER);

#ifndef UNIX
    //
    // Save 'don't use this connection'
    //
    if(IsDlgButtonChecked(hDlg, IDC_DONT_USE_CONNECTION))
    {
        // don't use => exclude coverage
        dwValue = (CO_INTERNET | CO_INTRANET);
    }
    else
    {
        // don't use not checked => don't exclude any coverage
        dwValue = 0;
    }
    SetCoverExclude(pDI->pszConnectoid, dwValue);
#endif

    //
    // Save proxy settings
    //
    INTERNET_PER_CONN_OPTION_LIST list;
    DWORD   dwBufSize = sizeof(list);
    DWORD   dwOptions = 2;              // always save FLAGS & DISCOVERY_FLAGS
    TCHAR   szAutoConfig[MAX_URL_STRING];
    
    list.pszConnection = (pDI->pszConnectoid && *pDI->pszConnectoid) ? pDI->pszConnectoid : NULL;
    list.dwSize = sizeof(list);
    list.dwOptionCount = 1;
    list.pOptions = new INTERNET_PER_CONN_OPTION[5];
    if(NULL == list.pOptions)
    {
        return FALSE;
    }

    list.pOptions[0].dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;

    //
    // Query autodiscover flags - we just need to set one bit in there
    //
    if(FALSE == InternetQueryOption(NULL,
            INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwBufSize))
    {
        delete [] list.pOptions;
        return FALSE;
    }

    //
    // save off all other options
    //
    list.pOptions[1].dwOption = INTERNET_PER_CONN_FLAGS;
    list.pOptions[1].Value.dwValue = PROXY_TYPE_DIRECT;

    //
    // save proxy settings
    //
    GetProxyInfo(hDlg, pDI);

    if(pDI->proxy.fEnable)
    {
        list.pOptions[1].Value.dwValue |= PROXY_TYPE_PROXY;
    }

    list.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    list.pOptions[2].Value.pszValue = pDI->proxy.szProxy;
    list.pOptions[3].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
    list.pOptions[3].Value.pszValue = pDI->proxy.szOverride;

    dwOptions += 2;

    //
    // save autodetect
    //
    if(IsDlgButtonChecked(hDlg, IDC_AUTODISCOVER))
    {
        list.pOptions[1].Value.dwValue |= PROXY_TYPE_AUTO_DETECT;
        if(pDI->fClickedAutodetect)
        {
            list.pOptions[0].Value.dwValue |= AUTO_PROXY_FLAG_USER_SET;
        }
    }
    else
    {
        list.pOptions[0].Value.dwValue &= ~AUTO_PROXY_FLAG_DETECTION_RUN;
    }

    //
    // save autoconfig
    //
    if(IsDlgButtonChecked(hDlg, IDC_CONFIGSCRIPT) &&
       GetWindowText(GetDlgItem(hDlg, IDC_CONFIG_ADDR), szAutoConfig, MAX_URL_STRING))
    {
        list.pOptions[1].Value.dwValue |= PROXY_TYPE_AUTO_PROXY_URL;
        list.pOptions[dwOptions].Value.pszValue = szAutoConfig;
        list.pOptions[dwOptions].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
        dwOptions++;
    }

    // update wininet
    list.dwOptionCount = dwOptions;
    InternetSetOption(NULL,
            INTERNET_OPTION_PER_CONNECTION_OPTION, &list, dwBufSize);

    // tell wininet that the proxy info has changed
    InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);

    // all done with options list
    delete [] list.pOptions;

    // all done if we're editing lan settings
    if(NULL == pDI->pszConnectoid || 0 == *pDI->pszConnectoid)
        return TRUE;

    // no credentials to store if we have a w2k custom handler
    if(pDI->proxy.fCustomHandler)
    {
        return TRUE;
    }

    //
    // save connectoid information - use wide version if possible
    //
    BOOL            fDeletePassword = FALSE;

    if(lpRasSetEntryDialParamsW)
    {
        RASDIALPARAMSW  params;

        memset(&params, 0, sizeof(RASDIALPARAMSW));
        params.dwSize = sizeof(RASDIALPARAMSW);
        StrCpyN(params.szEntryName, pDI->pszConnectoid, RAS_MaxEntryName+1);
        GetWindowText(GetDlgItem(hDlg, IDC_USER), params.szUserName, UNLEN);
        GetWindowText(GetDlgItem(hDlg, IDC_PASSWORD), params.szPassword, PWLEN);
        if(0 == params.szPassword[0])
            fDeletePassword = TRUE;
        GetWindowText(GetDlgItem(hDlg, IDC_DOMAIN), params.szDomain, DNLEN);
        if(0 == params.szDomain[0]) {
            // user wants empty domain
            params.szDomain[0] = TEXT(' ');
        }
        (lpRasSetEntryDialParamsW)(NULL, &params, fDeletePassword);

    }
    else if(lpRasSetEntryDialParamsA)
    {
        RASDIALPARAMSA  params;

        memset(&params, 0, sizeof(RASDIALPARAMSA));
        params.dwSize = sizeof(RASDIALPARAMSA);
        SHUnicodeToAnsi(pDI->pszConnectoid, params.szEntryName, ARRAYSIZE(params.szEntryName));
        GetWindowTextA(GetDlgItem(hDlg, IDC_USER), params.szUserName, UNLEN);
        GetWindowTextA(GetDlgItem(hDlg, IDC_PASSWORD), params.szPassword, PWLEN);
        if(0 == params.szPassword[0])
            fDeletePassword = TRUE;
        GetWindowTextA(GetDlgItem(hDlg, IDC_DOMAIN), params.szDomain, DNLEN);
        if(0 == params.szDomain[0]) {
            // user wants empty domain
            params.szDomain[0] = TEXT(' ');
        }
        (lpRasSetEntryDialParamsA)(NULL, &params, fDeletePassword);
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////
//
// NAME:       DialupDlgProc
//
// SYNOPSIS:   Dialog proc for dial up dialog
//
////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK DialupDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    PDIALINFO pDI = (PDIALINFO)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (uMsg) {

        case WM_INITDIALOG:
            ASSERT(lParam);
            DialupDlgInit(hDlg, (LPTSTR)lParam);
            return FALSE;

        case WM_DESTROY:
            delete pDI;
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
            case IDC_AUTODISCOVER:
                pDI->fClickedAutodetect = TRUE;
                break;
            case IDC_AUTOCNFG_ADVANCED:
                if(GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
                    break;
                // show advanced dialog box
                DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_AUTOCNFG_SETTINGS), hDlg,
                    AdvAutocnfgDlgProc, (LPARAM) pDI->pszConnectoid);
                break;

            case IDC_PROXY_ADVANCED:
                if(GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
                    break;

                GetProxyInfo(hDlg, pDI);
                pDI->proxy.fReadProxyFromRegistry = FALSE;

                // remove local so it doesn't show up in advanced dialog
                RemoveLocalFromExceptionList(pDI->proxy.szOverride);

                // show advanced dialog box
                if (DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_PROXY_SETTINGS), hDlg,
                                ProxyDlgProc, (LPARAM) &pDI->proxy) == IDOK)
                {
                    if(FALSE == pDI->proxy.fEnable)
                    {
                        // user disabled proxy in advanced dialog
                        CheckDlgButton(hDlg, IDC_MANUAL, FALSE);
                    }
                    PopulateProxyControls(hDlg, &pDI->proxy, TRUE);
                }
                break;

            case IDC_RAS_SETTINGS:
                if(lpRasEditPhonebookEntryW)
                {
                    (lpRasEditPhonebookEntryW)(hDlg, NULL, pDI->pszConnectoid);
                }
                else if(lpRasEditPhonebookEntryA)
                {
                    CHAR  szConnectoid[MAX_PATH];
                    SHUnicodeToAnsi(pDI->pszConnectoid, szConnectoid, ARRAYSIZE(szConnectoid));
                    (lpRasEditPhonebookEntryA)(hDlg, NULL, szConnectoid);
                }
                break;

            case IDC_DIAL_ADVANCED:
                DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_DIALUP_ADVANCED), hDlg,
                    AdvDialupDlgProc, (LPARAM) pDI->pszConnectoid);
                break;

            case IDC_MANUAL:
                if(IsDlgButtonChecked(hDlg, IDC_MANUAL))
                {
                    pDI->proxy.fEnable = TRUE;
                    SetFocus(GetDlgItem(hDlg, IDC_PROXY_ADDR));
                    SendMessage(GetDlgItem(hDlg, IDC_PROXY_ADDR), EM_SETSEL, 0, -1);
                }
                else
                {
                    pDI->proxy.fEnable = FALSE;
                }
                PopulateProxyControls(hDlg, &pDI->proxy, FALSE);
#ifdef UNIX
                ENABLEAPPLY(hDlg);
#endif
                break;

            case IDC_CONFIGSCRIPT:
                if(IsDlgButtonChecked(hDlg, IDC_CONFIGSCRIPT))
                {
                    // set focus to config script url
                    SetFocus(GetDlgItem(hDlg, IDC_CONFIG_ADDR));
                    SendMessage(GetDlgItem(hDlg, IDC_CONFIG_ADDR), EM_SETSEL, 0, -1);
                }
                PopulateProxyControls(hDlg, &pDI->proxy, FALSE);
#ifdef UNIX
                ENABLEAPPLY(hDlg);
#endif
                break;


#ifdef UNIX
            case IDC_AUTODISCOVER:
            case IDC_PROXY_OMIT_LOCAL_ADDRESSES:
                ENABLEAPPLY(hDlg);
                break;
            case IDC_PROXY_PORT:
            case IDC_PROXY_ADDR:
            case IDC_CONFIG_ADDR:
                switch (HIWORD(wParam))
                {
                    case EN_CHANGE:
                        ENABLEAPPLY(hDlg);
                        break;
                }
                break;
#endif
            case IDC_DONT_USE_CONNECTION:
                PopulateProxyControls(hDlg, &pDI->proxy, FALSE);
                break;

            case IDOK:
                if(FALSE == DialupDlgOk(hDlg, pDI))
                    // something is wrong... don't exit yet
                    break;

                // fall through
            case IDCANCEL:
                return EndDialog(hDlg, 0);
            }
            break;

        case WM_HELP:      // F1
            ResWinHelp((HWND) ((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                       HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            ResWinHelp((HWND)wParam, IDS_HELPFILE,
                       HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;
#ifdef UNIX
        case WM_NOTIFY:
        {
            NMHDR * lpnm = (NMHDR *) lParam;
            switch (lpnm->code)
            {
                case PSN_APPLY:
                {
            if(FALSE == DialupDlgOk(hDlg, pDI))
                // something is wrong... don't exit yet
                break;
                // fall through
                }
        }
    }
#endif
    }

    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//                  Advanced dial-up settings dialog
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
// NAME:       AdvDialupDlgProc
//
// SYNOPSIS:   Dialog proc for dial up dialog
//
////////////////////////////////////////////////////////////////////

void EnableAdvDialControls(HWND hDlg)
{
    BOOL fIdle      = IsDlgButtonChecked(hDlg, IDC_ENABLE_AUTODISCONNECT);

    // on if we have idle disconnect...
    EnableDlgItem(hDlg, IDC_IDLE_TIMEOUT, fIdle);
    EnableDlgItem(hDlg, IDC_IDLE_SPIN, fIdle);
}

BOOL AdvDialupDlgInit(HWND hDlg, LPTSTR pszConnectoid)
{
    TCHAR szTemp[MAX_PATH];
    DWORD dwRedialAttempts, dwRedialInterval, dwAutodisconnectTime;
    BOOL fExit, fDisconnect;

    // save connectoid name
    SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pszConnectoid);

    // figure out our registry key
    GetConnKey(pszConnectoid, szTemp, MAX_PATH);

    // open connectoid or lan settings
    RegEntry reCon(szTemp, HKEY_CURRENT_USER);
    if(ERROR_SUCCESS != reCon.GetError())
        return FALSE;

    //
    // Read autodial / redial stuff
    //
    // We get this stuff from the connectoid if possible.  We assume it's all
    // saved to the connectoid together so if EnableAutodial is present there,
    // read everything from there else read everything from the IE4 settings.
    //
    dwRedialInterval = reCon.GetNumber(REGSTR_VAL_REDIALINTERVAL, DEF_REDIAL_WAIT);
    dwRedialAttempts = reCon.GetNumber(REGSTR_VAL_REDIALATTEMPTS, DEF_REDIAL_TRIES);

    // autodisconnect
    fDisconnect = (BOOL)reCon.GetNumber(REGSTR_VAL_ENABLEAUTODIALDISCONNECT, 0);
    dwAutodisconnectTime = reCon.GetNumber(REGSTR_VAL_DISCONNECTIDLETIME, DEF_AUTODISCONNECT_TIME);
    fExit = (BOOL)reCon.GetNumber(REGSTR_VAL_ENABLEEXITDISCONNECT, 0);

    //
    // Check if the mobile pack is installed and if it is not - then do not have it checked
    //
    DWORD dwRes = JitFeature(hDlg, clsidFeatureMobile, TRUE);
    if(JIT_PRESENT != dwRes)
    {
        fDisconnect = FALSE;
        fExit = FALSE;
        // check to see if offline pack is installed and disable the disconnect
        // options if user has bailed on it
        if(JIT_NOT_AVAILABLE == dwRes) {
            // can't get offline pack so disable options
            CheckDlgButton(hDlg, IDC_ENABLE_AUTODISCONNECT, FALSE);
            CheckDlgButton(hDlg, IDC_EXIT_DISCONNECT, FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_ENABLE_AUTODISCONNECT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_TX_AUTODISCONNECT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EXIT_DISCONNECT), FALSE);
        }
    }

    //
    // Populate controls
    //
    CheckDlgButton(hDlg, IDC_ENABLE_AUTODISCONNECT, fDisconnect);
    CheckDlgButton(hDlg, IDC_EXIT_DISCONNECT, fExit);

    SendDlgItemMessage(hDlg, IDC_IDLE_SPIN, UDM_SETPOS, 0, dwAutodisconnectTime);
    SendDlgItemMessage(hDlg, IDC_CONNECT_SPIN,UDM_SETPOS, 0, dwRedialAttempts);
    SendDlgItemMessage(hDlg, IDC_INTERVAL_SPIN,UDM_SETPOS, 0, dwRedialInterval);

    //
    // Set control limits
    //
    Edit_LimitText(GetDlgItem(hDlg,IDC_IDLE_TIMEOUT), 2);    // limit edit ctrl to 2 chars
    Edit_LimitText(GetDlgItem(hDlg,IDC_IDLE_SPIN), 2);
    Edit_LimitText(GetDlgItem(hDlg,IDC_CONNECT_SPIN), 2);

    // set spin control min/max
    SendDlgItemMessage(hDlg,IDC_IDLE_SPIN,UDM_SETRANGE,0,
                    MAKELPARAM(MAX_AUTODISCONNECT_TIME,MIN_AUTODISCONNECT_TIME));
    SendDlgItemMessage(hDlg,IDC_CONNECT_SPIN,UDM_SETRANGE,0,
                    MAKELPARAM(MAX_REDIAL_TRIES,MIN_REDIAL_TRIES));
    SendDlgItemMessage(hDlg,IDC_INTERVAL_SPIN,UDM_SETRANGE,0,
                    MAKELPARAM(MAX_REDIAL_WAIT,MIN_REDIAL_WAIT));

    // enable controls
    EnableAdvDialControls(hDlg);

    return TRUE;
}

BOOL AdvDialupDlgOk(HWND hDlg, LPTSTR pszConnectoid)
{
    TCHAR szTemp[MAX_PATH];

    GetConnKey(pszConnectoid, szTemp, MAX_PATH);

    // open connectoid or lan settings
    RegEntry reCon(szTemp, HKEY_CURRENT_USER);
    if(ERROR_SUCCESS != reCon.GetError())
        return FALSE;

    // Save autodisconnect values
    BOOL fExit = IsDlgButtonChecked(hDlg,IDC_EXIT_DISCONNECT);
    BOOL fDisconnect = IsDlgButtonChecked(hDlg,IDC_ENABLE_AUTODISCONNECT);

    if(fExit || fDisconnect) {
        // make sure offline pack is installed or this feature won't work.
        DWORD dwRes = JitFeature(hDlg, clsidFeatureMobile, FALSE);
        if(JIT_PRESENT != dwRes) {
            // user doesn't want to download it so turn off autodisconnect
            fExit = FALSE;
            fDisconnect = FALSE;
        }
    }

    reCon.SetValue(REGSTR_VAL_ENABLEAUTODIALDISCONNECT, (DWORD)fDisconnect);
    reCon.SetValue(REGSTR_VAL_ENABLEEXITDISCONNECT, (DWORD)fExit);

    if(fDisconnect)
    {
        // get autodisconnect time from edit control
        // Sundown: coercion to 32b since values are range checked
        DWORD dwAutoDisconnectTime = (DWORD) SendDlgItemMessage(hDlg, IDC_IDLE_SPIN,
                                                                UDM_GETPOS,0,0);

        if(HIWORD(dwAutoDisconnectTime)) {
            MsgBox(hDlg, IDS_INVALID_AUTODISCONNECT_TIME, 0, MB_OK);

            // decide if it's too big or too small and fix it appropriately
            if(GetDlgItemInt(hDlg, IDC_IDLE_TIMEOUT, NULL, FALSE) < MIN_AUTODISCONNECT_TIME)
                dwAutoDisconnectTime = MIN_AUTODISCONNECT_TIME;
            else
                dwAutoDisconnectTime = MAX_AUTODISCONNECT_TIME;
            SendDlgItemMessage(hDlg, IDC_IDLE_SPIN, UDM_SETPOS, 0, dwAutoDisconnectTime);
            SetFocus(GetDlgItem(hDlg, IDC_IDLE_TIMEOUT));
            return FALSE;
        }

        // save it to registry
        reCon.SetValue(REGSTR_VAL_DISCONNECTIDLETIME, dwAutoDisconnectTime);

        // also save this value to MSN autodisconnect value, to
        // avoid confusion.  At some point in the future we'll
        // combine our UI...
        RegEntry reMSN(REGSTR_PATH_MOSDISCONNECT,HKEY_CURRENT_USER);
        if (reMSN.GetError() == ERROR_SUCCESS)
        {
            reMSN.SetValue(REGSTR_VAL_MOSDISCONNECT,dwAutoDisconnectTime);
        }
    }

    // save redial info
    DWORD_PTR dwRedialTry = SendDlgItemMessage(hDlg, IDC_CONNECT_SPIN, UDM_GETPOS, 0, 0);
    DWORD_PTR dwRedialWait = SendDlgItemMessage(hDlg, IDC_INTERVAL_SPIN, UDM_GETPOS, 0, 0);

    if(HIWORD(dwRedialTry)) {
        MsgBox(hDlg, IDS_INVALID_REDIAL_ATTEMPTS, 0, MB_OK);
        if(GetDlgItemInt(hDlg, IDC_CONNECT, NULL, FALSE) < MIN_REDIAL_TRIES)
            dwRedialTry = MIN_REDIAL_TRIES;
        else
            dwRedialTry = MAX_REDIAL_TRIES;
        SendDlgItemMessage(hDlg, IDC_CONNECT_SPIN, UDM_SETPOS, 0, dwRedialTry);
        SetFocus(GetDlgItem(hDlg, IDC_CONNECT));
        return FALSE;
    }

    if(HIWORD(dwRedialWait)) {
        MsgBox(hDlg, IDS_INVALID_REDIAL_WAIT, 0, MB_OK);
        if(GetDlgItemInt(hDlg, IDC_INTERVAL, NULL, FALSE) < MIN_REDIAL_WAIT)
            dwRedialWait = MIN_REDIAL_WAIT;
        else
            dwRedialWait = MAX_REDIAL_WAIT;
        SendDlgItemMessage(hDlg, IDC_INTERVAL_SPIN, UDM_SETPOS, 0, dwRedialWait);
        SetFocus(GetDlgItem(hDlg, IDC_INTERVAL));
        return FALSE;
    }

    reCon.SetValue(REGSTR_VAL_REDIALATTEMPTS, (DWORD) dwRedialTry);
    reCon.SetValue(REGSTR_VAL_REDIALINTERVAL, (DWORD) dwRedialWait);

    return TRUE;
}

INT_PTR CALLBACK AdvDialupDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    LPTSTR pszConn = (LPTSTR) GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (uMsg) {

        case WM_INITDIALOG:
            ASSERT(lParam);
            AdvDialupDlgInit(hDlg, (LPTSTR)lParam);
            return FALSE;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
            case IDC_ENABLE_AUTODISCONNECT:
                EnableAdvDialControls(hDlg);
                break;

            case IDOK:
                if(FALSE == AdvDialupDlgOk(hDlg, pszConn))
                    // something is wrong... don't exit yet
                    break;

                // fall through
            case IDCANCEL:
                return EndDialog(hDlg, 0);
            }
            break;

        case WM_HELP:      // F1
            ResWinHelp((HWND) ((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                       HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            ResWinHelp((HWND)wParam, IDS_HELPFILE,
                       HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;
    }

    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//                  Advanced autoconfig settings dialog (only used by IEAK)
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL AdvAutocnfgDlgInit(HWND hDlg, LPTSTR pszConnectoid)
{
    INTERNET_PER_CONN_OPTION_LIST list;
    DWORD dwBufSize = sizeof(list);

    // save connectoid name
    SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pszConnectoid);

    list.pszConnection = (pszConnectoid && *pszConnectoid) ? pszConnectoid : NULL;
    list.dwSize = sizeof(list);
    list.dwOptionCount = 3;
    list.pOptions = new INTERNET_PER_CONN_OPTION[3];
    if(NULL == list.pOptions)
    {
        return FALSE;
    }

    list.pOptions[0].dwOption = INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL;
    list.pOptions[1].dwOption = INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS;
    list.pOptions[2].dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;

    if(FALSE == InternetQueryOption(NULL,
            INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwBufSize))
    {
        delete [] list.pOptions;
        return FALSE;
    }

    // autoproxy url (js)
    if(list.pOptions[0].Value.pszValue)
    {
        SetWindowText(GetDlgItem(hDlg, IDC_CONFIGJS_ADDR), list.pOptions[0].Value.pszValue);
    }

    
    // autoconfig timer interval
    if(list.pOptions[1].Value.dwValue)
    {
        TCHAR szTimerInterval[16];

        wsprintf(szTimerInterval, TEXT("%d"), list.pOptions[1].Value.dwValue);
        SetWindowText(GetDlgItem(hDlg, IDC_CONFIGTIMER), szTimerInterval);
    }

    // autoconfig optimization
    CheckDlgButton(hDlg, IDC_CONFIGOPTIMIZE, 
        (list.pOptions[2].Value.dwValue & AUTO_PROXY_FLAG_CACHE_INIT_RUN ) ? BST_CHECKED : BST_UNCHECKED);

    // all done with options list
    if (list.pOptions[0].Value.pszValue)
    {
        GlobalFree(list.pOptions[0].Value.pszValue);
    }
    delete [] list.pOptions;

    return TRUE;
}

BOOL AdvAutocnfgDlgOk(HWND hDlg, LPTSTR pszConnectoid)
{
    INTERNET_PER_CONN_OPTION_LIST list;
    DWORD   dwBufSize = sizeof(list);
    TCHAR    szAutoconfig[MAX_URL_STRING];
    TCHAR   szTimerInterval[16];

    list.pszConnection = (pszConnectoid && *pszConnectoid) ? pszConnectoid : NULL;
    list.dwSize = sizeof(list);
    list.dwOptionCount = 1;
    list.pOptions = new INTERNET_PER_CONN_OPTION[3];
    if(NULL == list.pOptions)
    {
        return FALSE;
    }

    list.pOptions[0].dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;
    //
    // Query autodiscover flags - we just need to set one bit in there
    //
    if(FALSE == InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwBufSize))
    {
        delete [] list.pOptions;
        return FALSE;
    }

    // save autoconfiguration optimization field

    if (IsDlgButtonChecked(hDlg, IDC_CONFIGOPTIMIZE) == BST_CHECKED)
        list.pOptions[0].Value.dwValue |= AUTO_PROXY_FLAG_CACHE_INIT_RUN ;
    else
        list.pOptions[0].Value.dwValue &= ~AUTO_PROXY_FLAG_CACHE_INIT_RUN ;

    //
    // save autoproxy url
    //
    list.pOptions[1].dwOption = INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL;
    GetWindowText(GetDlgItem(hDlg, IDC_CONFIGJS_ADDR), szAutoconfig, sizeof(szAutoconfig));
    list.pOptions[1].Value.pszValue = szAutoconfig;

    //
    // save autoconfig timer
    //
    list.pOptions[2].dwOption = INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS;
    list.pOptions[2].Value.dwValue = 0;
        
    if(GetWindowText(GetDlgItem(hDlg, IDC_CONFIGTIMER), szTimerInterval, sizeof(szTimerInterval)))
        list.pOptions[2].Value.dwValue = StrToInt(szTimerInterval);
    
    // update wininet
    list.dwOptionCount = 3;
    InternetSetOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, dwBufSize);

    // tell wininet that the proxy info has changed
    InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);

    delete [] list.pOptions;

    return TRUE;
}

///////////////////////////////////////////////////////////////////
//
// NAME:       AdvAutocnfgProc
//
// SYNOPSIS:   Dialog proc for autoconfig dialog
//
////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK AdvAutocnfgDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    LPTSTR pszConn = (LPTSTR) GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (uMsg) {

        case WM_INITDIALOG:
            AdvAutocnfgDlgInit(hDlg, (LPTSTR)lParam);
            return FALSE;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
            case IDOK:
                if(FALSE == AdvAutocnfgDlgOk(hDlg, pszConn))
                    // something is wrong... don't exit yet
                    break;

                // fall through
            case IDCANCEL:
                return EndDialog(hDlg, 0);
            }
            break;

        case WM_HELP:      // F1
            ResWinHelp((HWND) ((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                       HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            ResWinHelp((HWND)wParam, IDS_HELPFILE,
                       HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;
    }

    return FALSE;
}

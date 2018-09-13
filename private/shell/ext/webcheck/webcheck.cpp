#include "private.h"
#include "subsmgrp.h"
#include "offsync.h"
#include "offl_cpp.h"
#include "factory.h"
#include "notfcvt.h"
#define TF_THISMODULE TF_WEBCHECKCORE

#include "resource.h"

#define INITGUIDS
#include <shlguid.h>

#define MLUI_INIT
#include <mluisupp.h>

// Free us from the tyranny of CRT by defining our own new and delete
#define CPP_FUNCTIONS
// We cannot define DEFINE_FLOAT_STUFF because there is floating-point
// arithmetic performed in this component.
//#include <crtfree.h>

//  We're going to use our own new and delete so that we can 
//  use shdocvw's leak detection code
//

extern "C" int _fltused;    // define this so that floats and doubles don't bring in the CRT

//
// Subscription property names
//
// Agent Start
extern const WCHAR  c_szPropURL[] = L"URL";
extern const WCHAR  c_szPropName[] = L"Name";
extern const WCHAR  c_szPropAgentFlags[] = L"AgentFlags";
extern const WCHAR  c_szPropCrawlLevels[] = L"RecurseLevels";
extern const WCHAR  c_szPropCrawlFlags[] = L"RecurseFlags";
extern const WCHAR  c_szPropCrawlMaxSize[] = L"MaxSizeKB";
extern const WCHAR  c_szPropCrawlChangesOnly[] = L"CheckChangesOnly";
extern const WCHAR  c_szPropChangeCode[] = L"ChangeCode";
extern const WCHAR  c_szPropCrawlUsername[] = L"Username";
extern const WCHAR  c_szPropEmailNotf[] = L"EmailNotification";
extern const WCHAR  c_szPropCrawlLocalDest[] = L"LocalDest";
extern const WCHAR  c_szPropCrawlGroupID[] = L"GroupID";
extern const WCHAR  c_szPropCrawlNewGroupID[] = L"NewGroupID";
extern const WCHAR  c_szPropActualProgressMax[] = L"ActualProgressMax";
extern const WCHAR  c_szPropCrawlActualSize[] = L"ActualSizeKB";
extern const WCHAR  c_szPropEnableShortcutGleam[] = L"EnableShortcutGleam";
extern const WCHAR  c_szPropChannelFlags[] = L"ChannelFlags";
extern const WCHAR  c_szPropChannel[] = L"Channel";
extern const WCHAR  c_szPropDesktopComponent[] = L"DesktopComponent";
extern const WCHAR  c_szPropStatusCode[] = L"StatusCode";
extern const WCHAR  c_szPropStatusString[] = L"StatusString";
extern const WCHAR  c_szPropCompletionTime[] = L"CompletionTime";
extern const WCHAR  c_szPropPassword[] = L"Password";
// End Report
extern const WCHAR  c_szPropEmailURL[] = L"EmailURL";
extern const WCHAR  c_szPropEmailFlags[] = L"EmailFlags";
extern const WCHAR  c_szPropEmailTitle[] = L"EmailTitle";
extern const WCHAR  c_szPropEmailAbstract[] = L"EmailAbstract";
extern const WCHAR  c_szPropCharSet[] = L"CharSet";

// Tray Agent Properties
extern const WCHAR  c_szPropGuidsArr[] = L"Guids Array";

// Tracking Properties
extern const WCHAR  c_szTrackingCookie[] = L"LogGroupID";
extern const WCHAR  c_szTrackingPostURL[] = L"PostURL";
extern const WCHAR  c_szPostingRetry[] = L"PostFailureRetry";
extern const WCHAR  c_szPostHeader[] = L"PostHeader";
extern const WCHAR  c_szPostPurgeTime[] = L"PostPurgeTime";

// Delivery Agent Properties
extern const WCHAR  c_szStartCookie[] = L"StartCookie";

// Initial cookie in AGENT_INIT
extern const WCHAR  c_szInitCookie[] = L"InitCookie";

STDAPI OfflineFolderRegisterServer();
STDAPI OfflineFolderUnregisterServer();

// Count number of objects and number of locks.
ULONG       g_cObj=0;
ULONG       g_cLock=0;

// DLL Instance handle
HINSTANCE   g_hInst=0;

// other globals
BOOL        g_fIsWinNT;    // Are we on WinNT? Always initialized.
BOOL        g_fIsWinNT5;   // Is it NT5?

// logging globals
BOOL        g_fCheckedForLog = FALSE;       // have we checked registry?
TCHAR *     g_pszLoggingFile = NULL;        // file to write log to

TCHAR szInternetSettings[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
TCHAR szProxyEnable[] = TEXT("ProxyEnable");
const TCHAR c_szRegKey[] = WEBCHECK_REGKEY;
const TCHAR c_szRegKeyUsernames[] = WEBCHECK_REGKEY TEXT("\\UserFormFieldNames");
const TCHAR c_szRegKeyPasswords[] = WEBCHECK_REGKEY TEXT("\\PasswordFormFieldNames");
const TCHAR c_szRegKeyStore[] = WEBCHECK_REGKEY_STORE;

// Pstore related variables.
static PST_PROVIDERID s_provID = GUID_NULL;

// {14D96C20-255B-11d1-898F-00C04FB6BFC4}
static const GUID GUID_PStoreType = { 0x14d96c20, 0x255b, 0x11d1, { 0x89, 0x8f, 0x0, 0xc0, 0x4f, 0xb6, 0xbf, 0xc4 } };

static PST_KEY s_Key = PST_KEY_CURRENT_USER;
static WCHAR c_szInfoDel[] = L"InfoDelivery";
static WCHAR c_szSubscriptions[] = L"Subscriptions";

// This is needed so we can link to libcmt.dll, because floating-point
// initialization code is required.
void __cdecl main()
{
}

//////////////////////////////////////////////////////////////////////////
//
// DLL entry point
//
//////////////////////////////////////////////////////////////////////////
EXTERN_C BOOL WINAPI LibMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{

    switch (ulReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            OSVERSIONINFOA vi;

            _fltused = 0;

            DisableThreadLibraryCalls(hInstance);
            g_hInst = hInstance;

            MLLoadResources(g_hInst, TEXT("webchklc.dll"));

            vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
            GetVersionExA(&vi);
            if(vi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
                g_fIsWinNT = TRUE;
                if(vi.dwMajorVersion > 4)
                    g_fIsWinNT5 = TRUE;
                else
                    g_fIsWinNT5 = FALSE;
            } else {
                g_fIsWinNT = FALSE;
                g_fIsWinNT5 = FALSE;
            }

#ifdef DEBUG
            // g_dwTraceFlags = TF_ALWAYS;    // Default if not overridden from INI
            g_dwTraceFlags = TF_NEVER;    // Default if not overridden from INI
            CcshellGetDebugFlags();
#endif
        }
        break;

        case DLL_PROCESS_DETACH:
        {
            MLFreeResources(g_hInst);
        }
        break;
    }


    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
// Standard OLE entry points
//
//////////////////////////////////////////////////////////////////////////

//  Class factory -
//  For classes with no special needs these macros should take care of it.
//  If your class needs some special stuff just to get the ball rolling,
//  implement your own CreateInstance method.  (ala, CConnectionAgent)

#define DEFINE_CREATEINSTANCE(cls, iface) \
HRESULT cls##_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk) \
{ \
    ASSERT(NULL == punkOuter); \
    ASSERT(NULL != ppunk); \
    *ppunk = (iface *)new cls; \
    return (NULL != *ppunk) ? S_OK : E_OUTOFMEMORY; \
}

#define DEFINE_AGGREGATED_CREATEINSTANCE(cls, iface) \
HRESULT cls##_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk) \
{ \
    ASSERT(NULL != punkOuter); \
    ASSERT(NULL != ppunk); \
    *ppunk = (iface *)new cls(punkOuter); \
    return (NULL != *ppunk) ? S_OK : E_OUTOFMEMORY; \
}

DEFINE_CREATEINSTANCE(CWebCheck, IOleCommandTarget)
DEFINE_CREATEINSTANCE(CWebCrawler, ISubscriptionAgentControl)
DEFINE_CREATEINSTANCE(CChannelAgent, ISubscriptionAgentControl)
DEFINE_CREATEINSTANCE(COfflineFolder, IShellFolder)
// extern HRESULT CConnectionAgent_CreateInstance(LPUNKNOWN pUnkOuter, IUnknown **ppunk);
DEFINE_CREATEINSTANCE(CSubscriptionMgr, ISubscriptionMgr2);
DEFINE_CREATEINSTANCE(CWCPostAgent, ISubscriptionAgentControl)
DEFINE_CREATEINSTANCE(CCDLAgent, ISubscriptionAgentControl)
DEFINE_CREATEINSTANCE(COfflineSync, ISyncMgrSynchronize)

const CFactoryData g_FactoryData[] = 
{
 {   &CLSID_WebCheck,             CWebCheck_CreateInstance,           0 }
,{   &CLSID_WebCrawlerAgent,      CWebCrawler_CreateInstance,         0 }
,{   &CLSID_ChannelAgent,         CChannelAgent_CreateInstance,       0 }
,{   &CLSID_OfflineFolder,        COfflineFolder_CreateInstance,      0 }
// ,{   &CLSID_ConnectionAgent,      CConnectionAgent_CreateInstance,    0 }
,{   &CLSID_SubscriptionMgr,      CSubscriptionMgr_CreateInstance,    0 }
,{   &CLSID_PostAgent,            CWCPostAgent_CreateInstance,        0 }
,{   &CLSID_CDLAgent,             CCDLAgent_CreateInstance,           0 }
,{   &CLSID_WebCheckOfflineSync,  COfflineSync_CreateInstance,        0 }
};

HRESULT APIENTRY DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;
    IUnknown *punk = NULL;

    *ppv = NULL;
    
    // Validate request
    for (int i = 0; i < ARRAYSIZE(g_FactoryData); i++)
    {
        if (rclsid == *g_FactoryData[i].m_pClsid)
        {
            punk = new CClassFactory(&g_FactoryData[i]);
            break;
        }
    }

    if (ARRAYSIZE(g_FactoryData) <= i)
    {
        ASSERT(NULL == punk);
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }
    else if (NULL == punk)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = punk->QueryInterface(riid, ppv);
        punk->Release();
    } 

    ASSERT((SUCCEEDED(hr) && (NULL != *ppv)) ||
           (FAILED(hr) && (NULL == *ppv)));

    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    // check objects and locks
    return (0L == DllGetRef() && 0L == DllGetLock()) ? S_OK : S_FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
// helper functions
//
//////////////////////////////////////////////////////////////////////////
int MyOleStrToStrN(LPTSTR psz, int cchMultiByte, LPCOLESTR pwsz)
{
    int i;
#ifdef UNICODE
    StrCpyN(psz, pwsz, cchMultiByte);
    i = cchMultiByte;
#else
    i=WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz,
                    cchMultiByte, NULL, NULL);

    if (!i)
    {
        DBG_WARN("MyOleStrToStrN string too long; truncated");
        psz[cchMultiByte - 1]=0;
    }
#ifdef DEBUG
    else
        ZeroMemory(psz+i, sizeof(TCHAR)*(cchMultiByte-i));
#endif
#endif

    return i;
}

int MyStrToOleStrN(LPOLESTR pwsz, int cchWideChar, LPCTSTR psz)
{
    int i;

#ifdef UNICODE
    StrCpyN(pwsz, psz, cchWideChar);
    i = cchWideChar;
#else
    i=MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, cchWideChar);
    if (!i)
    {
        DBG_WARN("MyStrToOleStrN string too long; truncated");
        pwsz[cchWideChar-1]=0;
    }
#ifdef DEBUG
    else
        ZeroMemory(pwsz+i, sizeof(OLECHAR)*(cchWideChar-i));
#endif
#endif
    return i;
}

// Convert upper to lower for ASCII wide characters
inline WCHAR MyToLower(WCHAR wch)
{
    return (wch >= 'A' && wch <= 'Z') ? (wch - 'A'+ 'a') : wch;
}

// Optimized for the knowledge that urls are 7-bit characters.
int MyAsciiCmpNIW(LPCWSTR pwsz1, LPCWSTR pwsz2, int iLen)
{
    while (iLen-- && *pwsz1 && *pwsz2)
    {
        ASSERT(*pwsz1 || *pwsz2);

        if (MyToLower(*pwsz1++) != MyToLower(*pwsz2++))
            return 1;
    }

    return 0;
}

int MyAsciiCmpW(LPCWSTR pwsz1, LPCWSTR pwsz2)
{
    while (*pwsz1)
    {
        if (*pwsz1++ != *pwsz2++)
        {
            return 1;
        }
    }

    if (*pwsz2)
        return 1;

    return 0;
}


#ifdef DEBUG
void DumpIID(LPCSTR psz, REFIID riid)
{
    // Convert the GUID to an ANSI string
    TCHAR pszGUID[GUIDSTR_MAX];
    WCHAR pwszGUID[GUIDSTR_MAX];
    int len = StringFromGUID2(riid, pwszGUID, ARRAYSIZE(pwszGUID));
    ASSERT(GUIDSTR_MAX == len);
    ASSERT(0 == pwszGUID[GUIDSTR_MAX - 1]);
    len = MyOleStrToStrN(pszGUID, GUIDSTR_MAX, pwszGUID);
    ASSERT(GUIDSTR_MAX == len);
    ASSERT(0 == pszGUID[GUIDSTR_MAX - 1]);

    // See if the IID has a string in the registry
    TCHAR pszKey[MAX_PATH];
    TCHAR pszIIDName[MAX_PATH];
    wnsprintf(pszKey, ARRAYSIZE(pszKey), TEXT("Interface\\%s"), pszGUID);
    BOOL fRet;
    fRet = ReadRegValue(HKEY_CLASSES_ROOT, pszKey, NULL, pszIIDName, sizeof(pszIIDName));

    // Print all the strings
    if (fRet)
        TraceMsg(TF_THISMODULE, "%s - %s %s", psz, pszIIDName, pszGUID);
    else
        TraceMsg(TF_THISMODULE, "%s - %s", psz, pszGUID);
}
#endif // DEBUG

//////////////////////////////////////////////////////////////////////////
//
// Autoregistration entry points
//
//////////////////////////////////////////////////////////////////////////

HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, achREGINSTALL);

        if (pfnri)
        {
            hr = pfnri(g_hInst, szSection, NULL);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}

STDAPI DllRegisterServer(void)
{
    // Delete any old registration entries, then add the new ones.
    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
//  CallRegInstall("UnReg");
    CallRegInstall("Reg");
    if (hinstAdvPack)
    {
        FreeLibrary(hinstAdvPack);
    }

    // OfflineFolder registers.
    OfflineFolderRegisterServer();

    // do external setup stuff on non-NT5 platforms
    if(FALSE == g_fIsWinNT5)
    {
        // register LCE
        HINSTANCE hLCE = LoadLibrary(TEXT("estier2.dll"));
        if (hLCE)
        {
            LCEREGISTER regfunc;
            regfunc = (LCEREGISTER)GetProcAddress(hLCE, "LCERegisterServer");
            if (regfunc)
                if (FAILED(regfunc(NULL)))
                    DBG_WARN("LCE register server failed!");

            FreeLibrary(hLCE);
        }

        // create reg key that SENS needs
        DWORD dwValue = 0;
        WriteRegValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Mobile\\Sens"),
                      TEXT("Configured"), &dwValue, sizeof(DWORD), REG_DWORD);

        // if we're on NT4, call SENS configuration api
        if (g_fIsWinNT)
        {
            HINSTANCE hSENS = LoadLibrary(TEXT("senscfg.dll"));

            if(hSENS)
            {
                SENSREGISTER regfunc;
                regfunc = (SENSREGISTER)GetProcAddress(hSENS, "SensRegister");
                if(regfunc)
                    if (FAILED(regfunc()))
                        DBG_WARN("SENS register server failed!");

                FreeLibrary(hSENS);
            }
        }
    }

    return NOERROR;
}

STDAPI
DllUnregisterServer(void)
{
    HRESULT hr;

    hr = OfflineFolderUnregisterServer();
    hr = CallRegInstall("UnReg");

    // do external unregister stuff on non-NT5 platforms
    if(FALSE == g_fIsWinNT5) {

        // unregister SENS on NT4
        if(g_fIsWinNT){
            HINSTANCE hSENS = LoadLibrary(TEXT("senscfg.dll"));
            if(hSENS) {
                SENSREGISTER regfunc;
                regfunc = (SENSREGISTER)GetProcAddress(hSENS, "SensUnregister");
                if(regfunc)
                    regfunc();
                FreeLibrary(hSENS);
            }
        }

        // unregister LCE
        HINSTANCE hLCE = LoadLibrary(TEXT("estier2.dll"));
        if(hLCE) {
            LCEUNREGISTER unregfunc;
            unregfunc = (LCEUNREGISTER)GetProcAddress(hLCE, "LCEUnregisterServer");
            if(unregfunc)
                unregfunc(NULL);
            FreeLibrary(hLCE);
        }

        // Remove Sens key
        SHDeleteKey( HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Mobile\\Sens") );
    }

    return hr;
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    HRESULT hr = S_OK;
    typedef enum { InstallNone, InstallPolicies, InstallPerUser } InstallType;
    
    InstallType installType = InstallNone;
    
    ASSERT(IS_VALID_STRING_PTRW(pszCmdLine, -1));

    //
    // Setup will call DllInstall by running 'regsvr32 /n /i:Policy webcheck.dll'.
    // This tells webcheck to process the Infodelivery Admin Policies.
    //
    if (0 == StrCmpIW(pszCmdLine, TEXTW("policy")))
    {
        installType = InstallPolicies;
    }
    else if (0 == StrCmpIW(pszCmdLine, TEXTW("U")))
    {
        installType = InstallPerUser;
    }

    if (bInstall && (installType != InstallNone))
    {
        hr = CoInitialize(NULL);

        if (SUCCEEDED(hr))
        {
            switch (installType)
            {
                case InstallPolicies:
                    hr = ProcessInfodeliveryPolicies();
                    break;

                case InstallPerUser:
                    hr = ConvertIE4Subscriptions();
                    DBGASSERT(SUCCEEDED(hr), "webcheck DllInstall - Failed to convert notification manager subscriptions");
                    break;
            }
        }

        CoUninitialize();
    }

    return SUCCEEDED(hr) ? S_OK : hr;    
}    


//////////////////////////////////////////////////////////////////////////
//
// Helper functions for Subscription Store
//
//////////////////////////////////////////////////////////////////////////
// Can return S_OK with NULL bstr
HRESULT ReadBSTR(ISubscriptionItem *pItem, LPCWSTR szName, BSTR *bstrRet)
{
    ASSERT(pItem && bstrRet);

    VARIANT     Val;
    
    Val.vt = VT_EMPTY;

    if (SUCCEEDED(pItem->ReadProperties(1, &szName, &Val)) &&
            (Val.vt==VT_BSTR))
    {
        *bstrRet = Val.bstrVal;
        return S_OK;
    }
    else
    {
        VariantClear(&Val); // free any return value of wrong type
        *bstrRet = NULL;
        return E_INVALIDARG;
    }
}

// Cannot return S_OK with emptry string
HRESULT ReadOLESTR(ISubscriptionItem *pItem, LPCWSTR szName, LPWSTR *ppszRet)
{
    HRESULT hr;
    BSTR bstrRet = NULL;
    *ppszRet = NULL;
    hr = ReadBSTR(pItem, szName, &bstrRet);
    if (SUCCEEDED(hr) && bstrRet && bstrRet[0])
    {
        int len = (lstrlenW(bstrRet) + 1) * sizeof(WCHAR);
        *ppszRet = (LPWSTR) CoTaskMemAlloc(len);
        if (*ppszRet)
        {
            CopyMemory(*ppszRet, bstrRet, len);
        }
    }
    
    SAFEFREEBSTR(bstrRet);
    if (*ppszRet)
        return S_OK;
    else
        return E_FAIL;
}

HRESULT ReadAnsiSTR(ISubscriptionItem *pItem, LPCWSTR szName, LPSTR *ppszRet)
{
    HRESULT hr;
    BSTR bstrRet = NULL;
    *ppszRet = NULL;
    hr = ReadBSTR(pItem, szName, &bstrRet);
    if (SUCCEEDED(hr) && bstrRet && bstrRet[0])
    {
        // Don't forget to allocate a long string for DBCS.
        int len = (lstrlenW(bstrRet) + 1) * sizeof(CHAR) * 2;
        *ppszRet = (LPSTR) MemAlloc(LMEM_FIXED, len);
        if (*ppszRet)
        {
            SHUnicodeToAnsi(bstrRet, *ppszRet, len);
        }
    }
    
    SAFEFREEBSTR(bstrRet);
    if (*ppszRet)
        return S_OK;
    else
        return E_FAIL;
}

HRESULT ReadBool(ISubscriptionItem *pItem, LPCWSTR szName, VARIANT_BOOL *pBoolRet)
{
    ASSERT(pItem && pBoolRet);

    VARIANT     Val;
    
    Val.vt = VT_EMPTY;

    // accept VT_I4 or VT_BOOL
    if (SUCCEEDED(pItem->ReadProperties(1, &szName, &Val)) &&
            (Val.vt==VT_BOOL || Val.vt==VT_I4))
    {
        if (Val.vt==VT_I4)
        {
            if (Val.lVal)
                *pBoolRet = VARIANT_TRUE;
            else
                *pBoolRet = VARIANT_FALSE;
        }
        else
            *pBoolRet = Val.boolVal;
        return S_OK;
    }
    else
    {
        VariantClear(&Val); // free any return value of wrong type
        return E_INVALIDARG;
    }
}

HRESULT ReadSCODE(ISubscriptionItem *pItem, LPCWSTR szName, SCODE *pscRet)
{
    ASSERT(pItem && pscRet);

    VARIANT Val;

    Val.vt = VT_EMPTY;

    if (SUCCEEDED(pItem->ReadProperties(1, &szName, &Val)) && Val.vt == VT_ERROR)
    {
        *pscRet = Val.scode;
        return S_OK;
    }
    else
    {
        VariantClear(&Val);
        return E_INVALIDARG;
    }
}

HRESULT WriteEMPTY(ISubscriptionItem *pItem, LPCWSTR szName)
{
    ASSERT(pItem);

    VARIANT Val;

    Val.vt = VT_EMPTY;
    return pItem->WriteProperties(1, &szName, &Val);
}

HRESULT WriteSCODE(ISubscriptionItem *pItem, LPCWSTR szName, SCODE scVal)
{
    ASSERT(pItem);

    VARIANT Val;

    Val.vt = VT_ERROR;
    Val.scode = scVal;

    return pItem->WriteProperties(1, &szName, &Val);
}
    
HRESULT ReadDWORD(ISubscriptionItem *pItem, LPCWSTR szName, DWORD *pdwRet)
{
    ASSERT(pItem && pdwRet);

    VARIANT     Val;
    
    Val.vt = VT_EMPTY;

    if (SUCCEEDED(pItem->ReadProperties(1, &szName, &Val)) &&
            (Val.vt==VT_I4 || Val.vt==VT_I2))
    {
        if (Val.vt==VT_I4)
            *pdwRet = (DWORD) Val.lVal;
        else
            *pdwRet = (DWORD) Val.iVal;

        return S_OK;
    }
    else
    {
        VariantClear(&Val); // free any return value of wrong type
        return E_INVALIDARG;
    }
}

HRESULT ReadLONGLONG(ISubscriptionItem *pItem, LPCWSTR szName, LONGLONG *pllRet)
{
    ASSERT(pItem && pllRet);

    VARIANT     Val;
    
    Val.vt = VT_EMPTY;

    if (SUCCEEDED(pItem->ReadProperties(1, &szName, &Val)) &&
            (Val.vt==VT_CY))
    {
        *pllRet = *((LONGLONG *) &(Val.cyVal));

        return S_OK;
    }
    else
    {
        VariantClear(&Val); // free any return value of wrong type
        return E_INVALIDARG;
    }
}
    
HRESULT ReadGUID(ISubscriptionItem *pItem, LPCWSTR szName, GUID *pGuid)
{
    ASSERT(pItem && pGuid);

    BSTR    bstrGUID = NULL;
    HRESULT hr = E_INVALIDARG;
    
    if (SUCCEEDED(ReadBSTR(pItem, szName, &bstrGUID)) &&
        SUCCEEDED(CLSIDFromString(bstrGUID, pGuid)))
    {
        hr = NOERROR;
    }
    SAFEFREEBSTR(bstrGUID);

    return hr;
}

HRESULT WriteGUID(ISubscriptionItem *pItem, LPCWSTR szName, GUID *pGuid)
{
    ASSERT(pItem && pGuid);
    
    WCHAR   wszCookie[GUIDSTR_MAX];

#ifdef DEBUG
    int len = 
#endif
    StringFromGUID2(*pGuid, wszCookie, ARRAYSIZE(wszCookie));
    ASSERT(GUIDSTR_MAX == len);
    return WriteOLESTR(pItem, szName, wszCookie);
}

HRESULT WriteLONGLONG(ISubscriptionItem *pItem, LPCWSTR szName, LONGLONG llVal)
{
    VARIANT Val;

    Val.vt = VT_CY;
    Val.cyVal = *((CY *) &llVal);

    return pItem->WriteProperties(1, &szName, &Val);
}

HRESULT WriteDWORD(ISubscriptionItem *pItem, LPCWSTR szName, DWORD dwVal)
{
    VARIANT Val;

    Val.vt = VT_I4;
    Val.lVal = dwVal;

    return pItem->WriteProperties(1, &szName, &Val);
}

HRESULT ReadDATE(ISubscriptionItem *pItem, LPCWSTR szName, DATE *dtVal)
{
    ASSERT(pItem && dtVal);

    VARIANT     Val;
    
    Val.vt = VT_EMPTY;

    if (SUCCEEDED(pItem->ReadProperties(1, &szName, &Val)) && (Val.vt==VT_DATE))
    {
        *dtVal = Val.date;
        return S_OK;
    }
    else
    {
        VariantClear(&Val); // free any return value of wrong type
        return E_INVALIDARG;
    }
}

HRESULT WriteDATE(ISubscriptionItem *pItem, LPCWSTR szName, DATE *dtVal)
{
    VARIANT Val;

    Val.vt = VT_DATE;
    Val.date= *dtVal;

    return pItem->WriteProperties(1, &szName, &Val);
}

HRESULT ReadVariant     (ISubscriptionItem *pItem, LPCWSTR szName, VARIANT *pvarRet)
{
    ASSERT(pvarRet->vt == VT_EMPTY);
    return pItem->ReadProperties(1, &szName, pvarRet);
}

HRESULT WriteVariant    (ISubscriptionItem *pItem, LPCWSTR szName, VARIANT *pvarVal)
{
    return pItem->WriteProperties(1, &szName, pvarVal);
}

HRESULT WriteOLESTR(ISubscriptionItem *pItem, LPCWSTR szName, LPCWSTR szVal)
{
    VARIANT Val;

    Val.vt = VT_BSTR;
    Val.bstrVal = SysAllocString(szVal);

    HRESULT hr = pItem->WriteProperties(1, &szName, &Val);

    SysFreeString(Val.bstrVal);

    return hr;
}

HRESULT WriteAnsiSTR(ISubscriptionItem *pItem, LPCWSTR szName, LPCSTR szVal)
{
    VARIANT Val;
    BSTR    bstrVal;
    HRESULT hr;

    bstrVal = SysAllocStringByteLen(szVal, lstrlenA(szVal));
    if (bstrVal)
    {
        Val.vt = VT_BSTR;
        Val.bstrVal = bstrVal;

        hr = pItem->WriteProperties(1, &szName, &Val);

        SysFreeString(bstrVal);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT WriteResSTR(ISubscriptionItem *pItem, LPCWSTR szName, UINT uID)
{
    TCHAR szString[MAX_RES_STRING_LEN];

    if (MLLoadString(uID, szString, ARRAYSIZE(szString)))
    {
        return WriteTSTR(pItem, szName, szString);
    }

    return E_INVALIDARG;
}


DWORD LogEvent(LPTSTR pszFormat, ...)
{

    // check registry if necessary
    if(FALSE == g_fCheckedForLog) {

        TCHAR   pszFilePath[MAX_PATH];

        if(ReadRegValue(HKEY_CURRENT_USER, c_szRegKey, TEXT("LoggingFile"),
                pszFilePath, sizeof(pszFilePath))) {

            g_pszLoggingFile = new TCHAR[lstrlen(pszFilePath) + 1];
            if(g_pszLoggingFile) {
                StrCpy(g_pszLoggingFile, pszFilePath);
            }
        }

        g_fCheckedForLog = TRUE;
    }

    if(g_pszLoggingFile) {

        TCHAR       pszString[MAX_PATH+INTERNET_MAX_URL_LENGTH];
        SYSTEMTIME  st;
        HANDLE      hLog;
        DWORD       dwWritten;
        va_list     va;

        hLog = CreateFile(g_pszLoggingFile, GENERIC_WRITE, 0, NULL,
                OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if(INVALID_HANDLE_VALUE == hLog)
            return GetLastError();

        // seek to end of file
        SetFilePointer(hLog, 0, 0, FILE_END);

        // dump time
        GetLocalTime(&st);
        wnsprintf(pszString, ARRAYSIZE(pszString), TEXT("\r\n%02d:%02d:%02d - "), st.wHour, st.wMinute, st.wSecond);
        WriteFile(hLog, pszString, lstrlen(pszString), &dwWritten, NULL);

        // dump passed in string
        va_start(va, pszFormat);
        wvnsprintf(pszString, ARRAYSIZE(pszString), pszFormat, va);
        va_end(va);
        WriteFile(hLog, pszString, lstrlen(pszString), &dwWritten, NULL);

        // clean up
        CloseHandle(hLog);
    }

    return 0;
}

// Functions related to saving and restoring user passwords from the pstore.


// We have wrappers around Create and Release to allow for future caching of the pstore
// instance within webcheck. 

STDAPI CreatePStore(IPStore **ppIPStore)
{
    HRESULT hr;

    hr = PStoreCreateInstance ( ppIPStore,
                                IsEqualGUID(s_provID, GUID_NULL) ? NULL : &s_provID,
                                NULL,
                                0);
    return hr;
}


STDAPI ReleasePStore(IPStore *pIPStore)
{
    HRESULT hr;

    if (pIPStore)
    {
        pIPStore->Release();
        hr = S_OK;
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

// Given a field name this figures out the type and sub-type in the pstore
// that should be queried. Currently these are hard-coded.
STDAPI GetPStoreTypes(LPCWSTR /* wszField */, GUID * pguidType, GUID * pguidSubType)
{
    *pguidType = GUID_PStoreType;
    *pguidSubType = GUID_NULL;

    return S_OK;
}


STDAPI  ReadNotificationPassword(LPCWSTR wszUrl, BSTR *pbstrPassword)
{
    GUID             itemType = GUID_NULL;
    GUID             itemSubtype = GUID_NULL;
    PST_PROMPTINFO   promptInfo = {0};
    IPStore*         pStore = NULL;
    HRESULT          hr ;
     
    if (wszUrl == NULL || pbstrPassword == NULL)
        return E_POINTER;

    // Will return NULL if there is no password entry or we 
    // fail for some reason. 
    *pbstrPassword = NULL;

    promptInfo.cbSize = sizeof(promptInfo);
    promptInfo.szPrompt = NULL;
    promptInfo.dwPromptFlags = 0;
    promptInfo.hwndApp = NULL;
    
    hr = CreatePStore(&pStore);    

    if (SUCCEEDED(hr))
    {
        ASSERT(pStore != NULL);
        hr = GetPStoreTypes(wszUrl, &itemType, &itemSubtype);

        if (SUCCEEDED(hr))
        {
            DWORD   cbData;
            BYTE *  pbData = NULL;

            hr = pStore->ReadItem(
                            s_Key,
                            &itemType,
                            &itemSubtype,
                            wszUrl,
                            &cbData,
                            &pbData,
                            &promptInfo,
                            0);

            if (SUCCEEDED(hr))
            {
                *pbstrPassword = SysAllocString((OLECHAR *)pbData);
                CoTaskMemFree(pbData);
                hr = S_OK;
            }
        }

        ReleasePStore(pStore);
    }

    return hr;
}

STDAPI WriteNotificationPassword(LPCWSTR wszUrl, BSTR bstrPassword)
{
    HRESULT         hr;
    PST_TYPEINFO    typeInfo;
    PST_PROMPTINFO  promptInfo;
    IPStore *       pStore;

    if (wszUrl == NULL)
        return E_POINTER;

    typeInfo.cbSize = sizeof(typeInfo);


    typeInfo.szDisplayName = c_szInfoDel;

    promptInfo.cbSize = sizeof(promptInfo);
    promptInfo.dwPromptFlags = 0;
    promptInfo.hwndApp = NULL;
    promptInfo.szPrompt = NULL;

    hr = CreatePStore(&pStore);

    if (SUCCEEDED(hr))
    {
        GUID itemType = GUID_NULL;
        GUID itemSubtype = GUID_NULL;

        ASSERT(pStore != NULL);

        hr = GetPStoreTypes(wszUrl, &itemType, &itemSubtype);
                
        if (SUCCEEDED(hr))
        {
            hr = pStore->CreateType(s_Key, &itemType, &typeInfo, 0);

            // PST_E_TYPE_EXISTS implies type already exists which is just fine
            // by us.
            if (SUCCEEDED(hr) || hr == PST_E_TYPE_EXISTS)
            {
                typeInfo.szDisplayName = c_szSubscriptions;

                hr = pStore->CreateSubtype(
                                        s_Key,
                                        &itemType,
                                        &itemSubtype,
                                        &typeInfo,
                                        NULL,
                                        0);

                if (SUCCEEDED(hr) || hr == PST_E_TYPE_EXISTS)
                {
                    if (bstrPassword != NULL)
                    {
                        hr = pStore->WriteItem(
                                            s_Key,
                                            &itemType,
                                            &itemSubtype,
                                            wszUrl,
                                            ((lstrlenW(bstrPassword)+1) * sizeof(WCHAR)),
                                            (BYTE *)bstrPassword,
                                            &promptInfo,
                                            PST_CF_NONE,
                                            0);
                    }
                    else
                    {
                        hr = pStore->DeleteItem(
                                            s_Key,
                                            &itemType,
                                            &itemSubtype,
                                            wszUrl,
                                            &promptInfo,
                                            0);
                    }
                }
            }
        }
        
        ReleasePStore(pStore);
    }
    
    return hr;
}                                                                       


HRESULT WritePassword(ISubscriptionItem *pItem, BSTR bstrPassword)
{
    BSTR    bstrURL = NULL;
    HRESULT hr = E_FAIL;

    hr = ReadBSTR(pItem, c_szPropURL, &bstrURL);
    RETURN_ON_FAILURE(hr);

    hr = WriteNotificationPassword(bstrURL, bstrPassword);
    SAFEFREEBSTR(bstrURL);
    return hr;
}

HRESULT ReadPassword(ISubscriptionItem *pItem, BSTR * pBstrPassword)
{
    BSTR    bstrURL = NULL;
    HRESULT hr = E_FAIL;

    hr = ReadBSTR(pItem, c_szPropURL, &bstrURL);
    RETURN_ON_FAILURE(hr);

    ASSERT(pBstrPassword);
    hr = ReadNotificationPassword(bstrURL, pBstrPassword);
    SAFEFREEBSTR(bstrURL);
    return hr;
}


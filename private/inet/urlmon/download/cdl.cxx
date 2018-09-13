// ===========================================================================
// File: CDL.CXX
//    The main code downloader file.
//

#include <cdlpch.h>
#include <stdio.h>
#include <shlwapi.h>
#include <regstr.h>
#include <initguid.h>
#include <pkgguid.h>
#include <winineti.h>
#include <shlwapip.h>
#include "advpub.h"
#include "advpkp.h"
#include "shlobj.h"
#include "helpers.hxx"

typedef HRESULT (WINAPI *REMOVECONTROLBYNAME)(
             LPCTSTR lpszFile,
             LPCTSTR lpszCLSID,
             LPCTSTR lpszTypeLibID,
             BOOL bForceRemove = FALSE,
             DWORD dwIsDistUnit = FALSE
             );

typedef BOOL (*pfnSfcIsFileProtected)(HANDLE RpcHandle,LPCWSTR ProtFileName);


extern LCID g_lcidBrowser;     // default to english

extern char g_szOCXCacheDir[];
extern char g_szPlatform[]; // platform specific string for location of file
extern char g_szOMNIPROX[]; // name of the omniprox dll
extern HINSTANCE g_hInst;

const static char *sz_USE_CODE_URL = "CODEBASE";
const static char szCLSID[] = "CLSID";
const static char szVersion[] = "Version";

const static char *szTHISCAB = "thiscab";
const static char *szIGNORE = "ignore";

extern LPCSTR szWinNT;
extern LPCSTR szWin95;

extern LPCSTR szPlatform;

extern char *g_szProcessorTypes[];

extern CRunSetupHook g_RunSetupHook;

extern int g_CPUType;
extern BOOL g_bRunOnWin95;

#define RANDNUM_MAX 0x7fff

LONG InitializeRandomSeed()
{
    SYSTEMTIME  st;
    GetSystemTime(&st);
    return (LONG)st.wMilliseconds;
}

//+-------------------------------------------------------------------------
//
//  Function:   randnum
//
//  Synopsys:   Generate random number based on seed. (copied from crt)
//
//+-------------------------------------------------------------------------
int randnum (void)
{
    static long holdrand = InitializeRandomSeed();
    return(holdrand = ((holdrand * 214013L + 2531011L) >> 16) & RANDNUM_MAX);
}



// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::CCodeDownload
//  CCodeDownload (main class tracking as a whole)
//  It has the client's BSC and creates a CClBinding for client.
// ---------------------------------------------------------------------------
CCodeDownload::CCodeDownload(
    LPCWSTR szDistUnit,
    LPCWSTR szURL,
    LPCWSTR szType,
    LPCWSTR szExt,
    DWORD dwFileVersionMS,
    DWORD dwFileVersionLS,
    HRESULT *phr)
{
    DllAddRef();
    m_szLastMod[0] = '\0';
    m_plci = NULL;

    m_cRef = 1;

    m_hr = S_OK;    // assume success

    m_url = 0;

    m_szDistUnit = NULL;

    m_pmkContext = NULL;

    m_dwFileVersionMS = dwFileVersionMS;
    m_dwFileVersionLS = dwFileVersionLS;

    m_lcid = GetThreadLocale();

    m_flags = CD_FLAGS_INIT;

    m_szInf = NULL;
    m_szOSD = NULL;
    m_szDisplayName = NULL;

    m_szCacheDir = NULL; // set to default of g_szOCXCacheDir by DoSetup
                         // the non-zeroness of this is also used by DoSetup
                         // to find it it's state machine has been init'ed

    m_szWaitForEXE = NULL;

    m_state = CDL_NoOperation;

    m_hKeySearchPath = NULL;
    m_pSearchPath = NULL;
    m_pSearchPathNextComp = NULL;

    m_pDownloads.RemoveAll(); // init to NULL

    m_pClientbinding.RemoveAll(); // init to NULL

    m_ModuleUsage.RemoveAll(); // init to NULL

    m_pDependencies.RemoveAll(); // init to NULL

    m_dwSystemComponent = FALSE;

    m_pCurCode = m_pAddCodeSection = NULL;

    if (szURL) {

        DWORD len = lstrlenW(szURL) +1;

        if (len <= INTERNET_MAX_URL_LENGTH) {

            m_url = new WCHAR [len]; // make private copy

            if (m_url)
                StrCpyW(m_url, szURL);
            else
                *phr = E_OUTOFMEMORY;
        } else {
            // we make assumptions all over that URL size is less than
            // INTERNET_MAX_URL_LENGTH
            *phr = E_INVALIDARG;
        }
    }

    if (szDistUnit) {

        DWORD len = lstrlenW(szDistUnit) +1;

        m_szDistUnit = new WCHAR [len]; // make private copy

        if (m_szDistUnit)
            StrCpyW(m_szDistUnit, szDistUnit);
        else
            *phr = E_OUTOFMEMORY;
    }

    m_pi.hProcess = INVALID_HANDLE_VALUE;
    m_pi.hThread = INVALID_HANDLE_VALUE;

    if (szExt) {

        DWORD len = lstrlenW(szExt) +1;

        m_szExt = new WCHAR [len]; // make private copy

        if (m_szExt)
            StrCpyW(m_szExt, szExt);
        else
            *phr = E_OUTOFMEMORY;
    }

    if (szType) {

        DWORD len = lstrlenW(szType) +1;

        m_szType = new WCHAR [len]; // make private copy

        if (m_szType)
            StrCpyW(m_szType, szType);
        else
            *phr = E_OUTOFMEMORY;
    }

    m_szVersionInManifest = NULL;
    m_szCatalogFile = NULL;

    m_dwExpire = 0xFFFFFFFF;

    m_pbEtag = NULL;
    m_pbJavaTrust = NULL;
    m_debuglog = CDLDebugLog::MakeDebugLog();
    if(! m_debuglog)
        *phr = E_OUTOFMEMORY;
    else
        m_debuglog->Init(this);

    m_bIsZeroImpact = FALSE;
    m_wszZIDll = NULL;

    m_bUninstallOld = FALSE;
    m_bExactVersion = FALSE;
    m_hModSFC = 0;
}  // CCodeDownload

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::~CCodeDownload
// ---------------------------------------------------------------------------
CCodeDownload::~CCodeDownload()
{
    Assert(m_cRef == 0);
    if (RelContextMk())
        SAFERELEASE(m_pmkContext);


    LISTPOSITION pos = m_pClientbinding.GetHeadPosition();
    int iNum = m_pClientbinding.GetCount();

    for (int i=0; i < iNum; i++) {
        CClBinding *pbinding = m_pClientbinding.GetNext(pos); // pass ref!
        pbinding->ReleaseClient();
        pbinding->Release();
    }
    m_pClientbinding.RemoveAll();

    pos = m_ModuleUsage.GetHeadPosition();
    iNum = m_ModuleUsage.GetCount();

    for (i=0; i < iNum; i++) {
        CModuleUsage *pModuleUsage = m_ModuleUsage.GetNext(pos); // pass ref!
        delete pModuleUsage;
    }
    m_ModuleUsage.RemoveAll(); // init to NULL

    m_pDownloads.RemoveAll(); // init to NULL

    pos = m_pDependencies.GetHeadPosition();
    iNum = m_pDependencies.GetCount();

    for (i=0; i < iNum; i++) {
        LPWSTR szDistUnit = m_pDependencies.GetNext(pos);
        delete szDistUnit;
    }

    if (m_szCacheDir != g_szOCXCacheDir)
        SAFEDELETE(m_szCacheDir);

    if (m_hKeySearchPath)
        ::RegCloseKey(m_hKeySearchPath);

    SAFEDELETE(m_szDistUnit);

    SAFEDELETE(m_url);
    SAFEDELETE(m_szType);
    SAFEDELETE(m_szExt);

    SAFEDELETE(m_szVersionInManifest);
    SAFEDELETE(m_szWaitForEXE);

    SAFEDELETE(m_pSearchPath);
    SAFEDELETE(m_szOSD);
    SAFEDELETE(m_szInf);
    SAFEDELETE(m_szDisplayName);
    SAFEDELETE(m_pAddCodeSection);
    SAFEDELETE(m_plci);
    SAFEDELETE(m_pbEtag);

    SAFERELEASE(m_pPackageManager);
    SAFEDELETE(m_szCatalogFile);
    DllRelease();

    if (m_pbJavaTrust) {
        if (m_pbJavaTrust->pwszZone) {
            delete (LPWSTR)m_pbJavaTrust->pwszZone;
        }
        SAFEDELETE(m_pbJavaTrust->pbSigner);
        SAFEDELETE(m_pbJavaTrust->pbJavaPermissions);
        delete m_pbJavaTrust;
    }

    if(m_debuglog)
    {
        m_debuglog->Release();
        m_debuglog = NULL;
    }

    if (m_hModSFC) {
        FreeLibrary(m_hModSFC);
    }

    if(m_wszZIDll)
        delete [] m_wszZIDll;
}  // ~CCodeDownload

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::SetDebugLog()
// Remove the old log and set a new one
// If debuglog is NULL, starts a new log
// ---------------------------------------------------------------------------

void CCodeDownload::SetDebugLog(CDLDebugLog * debuglog)
{
    CDLDebugLog * pdlNew = NULL;

    if(debuglog)
        pdlNew = debuglog;
    else
    {
        pdlNew = CDLDebugLog::MakeDebugLog();
        if(!pdlNew)
            // Out of Memory
            return;
        pdlNew->Init(this);
    }

    if(pdlNew)
    {
        m_debuglog->Release();
        pdlNew->AddRef();
        m_debuglog = pdlNew;
    }

}


HRESULT
CCodeDownload::CreateClientBinding(
    CClBinding **ppClientBinding,
    IBindCtx* pClientBC,
    IBindStatusCallback* pClientbsc,
    REFCLSID rclsid,
    DWORD dwClsContext,
    LPVOID pvReserved,
    REFIID riid,
    BOOL fAddHead,
    IInternetHostSecurityManager *pHostSecurityManager)
{
    HRESULT hr = S_OK;

    Assert(ppClientBinding);
    *ppClientBinding = NULL;

    // make an IBinding for the client
    // this gets passed on the OnstartBinding of first download
    // as parameter for clientbsc::OnstartBinding

    CClBinding *pClientbinding= new
        CClBinding(this, pClientbsc, pClientBC,
            rclsid, dwClsContext, pvReserved, riid, pHostSecurityManager);

    if (pClientbinding) {

        if (fAddHead) {
            m_pClientbinding.AddHead(pClientbinding);
        } else {
            m_pClientbinding.AddTail(pClientbinding);
        }

    } else {
        hr =  E_OUTOFMEMORY;
    }

    *ppClientBinding = pClientbinding;

    return hr;
}

HRESULT
CCodeDownload::AbortBinding( CClBinding *pbinding)
{
    IBindStatusCallback* pbsc;
    int iNumBindings = m_pClientbinding.GetCount();
    ICodeInstall *pCodeInstall;
    HRESULT hr = S_OK;
    LISTPOSITION curpos;

    Assert(iNumBindings > 1);

    if (GetState() == CDL_Completed) {
        goto Exit;
    }

    curpos = m_pClientbinding.Find(pbinding);

    Assert(pbinding == m_pClientbinding.GetAt(curpos));

    if (pbinding != m_pClientbinding.GetAt(curpos)) {
        hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION);
        goto Exit;
    }

    // now that we know the position of the binding,
    // pull out the binding and its related BSC from the list

    m_pClientbinding.RemoveAt(curpos);

    pbsc = pbinding->GetAssBSC();

    // report completion for this binding
    // note: if we are called to abort on a thread other than the one that
    // initiated the code download, then we report this onstopbinding on the
    // aborting thread (this one).
    pbsc->OnStopBinding(HRESULT_FROM_WIN32(ERROR_CANCELLED), NULL);

    // since we removed this binding from the list
    // we have to release this now. This will release the client BSC, BC
    pbinding->ReleaseClient();
    pbinding->Release();


Exit:
    return hr;
}


// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::PiggybackDupRequest
// piggy backs DUP request on to this exitsing CCodeDownload
// with matching szURL or rclsid
//  Returns:
//          S_OK: piggyback successful
//          Any other error: fatal error: fail
HRESULT
CCodeDownload::PiggybackDupRequest(
    IBindStatusCallback *pDupClientBSC,
    IBindCtx *pbc,
    REFCLSID rclsid,
    DWORD dwClsContext,
    LPVOID pvReserved,
    REFIID riid)
{
    HRESULT hr = S_OK;
    CClBinding *pClientBinding = NULL;

    Assert(m_pClientbinding.GetCount() > 0);
    if (m_pClientbinding.GetCount() <= 0) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    hr = CreateClientBinding( &pClientBinding, pbc, pDupClientBSC,
        rclsid, dwClsContext, pvReserved, riid,
        FALSE /* fAddHead */, NULL);

    if (SUCCEEDED(hr)) {

        Assert(pClientBinding);

        pClientBinding->SetState(CDL_Downloading);
        pDupClientBSC->OnStartBinding(0, pClientBinding);
    }

Exit:

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::AnyCodeDownloadsInThread
// checks if any code downloads are in progress in this thread
//  Returns:
//          S_OK: yes, downloads in progress
//          S_FALSE: none in progress
//          Any other error: fatal error: fail
HRESULT
CCodeDownload::AnyCodeDownloadsInThread()
{
    HRESULT hr = S_OK;

    CUrlMkTls tls(hr); // hr passed by reference!
    if (FAILED(hr))
        return hr;

    int iNumCDL = tls->pCodeDownloadList->GetCount();

    if (!iNumCDL)
        hr = S_FALSE;

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::IsDuplicateJavaSetup
//  Returns:
//          S_OK: Yes its a DUP
HRESULT
CCodeDownload::IsDuplicateJavaSetup(
    LPCWSTR szPackage)
{
    HRESULT hr = S_FALSE;       // assume not found
    CDownload *pdlCur = NULL;

    LISTPOSITION curpos = m_pDownloads.GetHeadPosition();
    for (int i=0; i < m_pDownloads.GetCount(); i++) {

        pdlCur = m_pDownloads.GetNext(curpos);

        if (pdlCur->FindJavaSetup(szPackage) != NULL) {
            hr = S_OK;
            break;
        }

    }

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::IsDuplicateHook
//  Returns:
//          S_OK: Yes its a DUP
HRESULT
CCodeDownload::IsDuplicateHook(
    LPCSTR szHook)
{
    HRESULT hr = S_FALSE;       // assume not found
    CDownload *pdlCur = NULL;

    LISTPOSITION curpos = m_pDownloads.GetHeadPosition();
    for (int i=0; i < m_pDownloads.GetCount(); i++) {

        pdlCur = m_pDownloads.GetNext(curpos);

        if (pdlCur->FindHook(szHook) != NULL) {
            hr = S_OK;
            break;
        }

    }

    return hr;
}

HRESULT
SetComponentDeclined(
    LPCWSTR pwszDistUnit,
    LPSTR pszSecId)
{
    HRESULT hr = S_FALSE;   // assume need to fault in
    LPSTR pszDistUnit = NULL;
    LONG    lResult = ERROR_SUCCESS;
    HKEY    hkeyDec    = NULL;
    DWORD   dwSize;
    DWORD   dwValue;
    LPSTR szNull = "";
    char szKey[MAX_PATH*2];

    if (FAILED((hr=::Unicode2Ansi(pwszDistUnit, &pszDistUnit))))
    {
        goto Exit;
    }

    lstrcpyn(szKey, REGKEY_DECLINED_COMPONENTS, MAX_PATH*2);

#ifndef ENABLE_PERSIST_DECLINED_COMPONNETS
    if (RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_WRITE, &hkeyDec) != ERROR_SUCCESS) {
        hr = S_OK;
        goto Exit;
    } else {
        if (hkeyDec) {
            RegCloseKey(hkeyDec);
            hkeyDec = 0;
        }
    }
#endif

    StrCatBuff(szKey, "\\", MAX_PATH*2);
    StrCatBuff(szKey, pszDistUnit, MAX_PATH*2);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_WRITE, &hkeyDec) != ERROR_SUCCESS)
    {
        if ((lResult = RegCreateKey( HKEY_CURRENT_USER,
                   szKey, &hkeyDec)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }
    }

    if (((lResult = RegSetValueEx (hkeyDec, pszSecId, 0, REG_SZ,
            (unsigned char *)szNull, 1))) != ERROR_SUCCESS) {

        hr = HRESULT_FROM_WIN32(lResult);
    }


Exit:

    if (hkeyDec)
        RegCloseKey(hkeyDec);

    SAFEDELETE(pszDistUnit);
    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::SetUserDeclined
HRESULT
CCodeDownload::SetUserDeclined()
{
    HRESULT hr = S_OK;
    LISTPOSITION pos = m_pClientbinding.GetHeadPosition();
    int iNum = m_pClientbinding.GetCount();
    int i;
    BYTE pbSecIdDocBase[INTERNET_MAX_URL_LENGTH];
    DWORD dwSecIdDocBase = INTERNET_MAX_URL_LENGTH;
    IInternetHostSecurityManager *pHostSecurityManager = GetClientBinding()->GetHostSecurityManager();


    if (!pHostSecurityManager) {
        // called by a host without sec mgr, don't record that you have
        // declined this component
        goto Exit;
    }

    hr = pHostSecurityManager->GetSecurityId(pbSecIdDocBase, &dwSecIdDocBase, 0);

    Assert(hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    if (FAILED(hr))
        goto Exit;

    // Hack!
    // Assumes internal knowledge of security id
    // the security has the zone id as the last dword, the rest of the stuff is
    // just the protocol followed by the site
    if (dwSecIdDocBase - sizeof(DWORD)) {
        pbSecIdDocBase[dwSecIdDocBase - sizeof(DWORD)] = '\0';
    }

    if (GetMainDistUnit()) {
        hr = SetComponentDeclined(GetMainDistUnit(), (char *)pbSecIdDocBase);

        if (FAILED(hr))
            goto Exit;
    }

    if (GetMainType()) {
        hr = SetComponentDeclined(GetMainType(), (char *)pbSecIdDocBase);

        if (FAILED(hr))
            goto Exit;
    }

    if (GetMainExt()) {
        hr = SetComponentDeclined(GetMainExt(), (char *)pbSecIdDocBase);

        if (FAILED(hr))
            goto Exit;
    }

    for (i=0; i < iNum; i++) {

        CClBinding *pbinding = m_pClientbinding.GetNext(pos); // pass ref!
        LPOLESTR pwszClsid;

        pwszClsid = NULL;

        if (!IsEqualGUID(pbinding->GetClsid() , CLSID_NULL)) {

            hr=StringFromCLSID(pbinding->GetClsid(), &pwszClsid);

            if (FAILED(hr))
                goto Exit;

            hr = SetComponentDeclined(pwszClsid, (char *)pbSecIdDocBase);
            SAFEDELETE(pwszClsid);

            if (FAILED(hr))
                goto Exit;
        }
    }

Exit:
    return hr;
}

BOOL IsDeclined(
    LPCWSTR pwszDistUnit,
    IInternetHostSecurityManager *pHostSecurityManager)
{
    BOOL bDeclined = FALSE;
    LPSTR pszDistUnit = NULL;
    BYTE pbSecIdDocBase[INTERNET_MAX_URL_LENGTH];
    DWORD dwSecIdDocBase = INTERNET_MAX_URL_LENGTH;
    HRESULT hr = S_OK;
    char szKey[MAX_PATH*2];


    Assert(pHostSecurityManager);

    hr = pHostSecurityManager->GetSecurityId(pbSecIdDocBase, &dwSecIdDocBase, 0);

    Assert(hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    if (FAILED(hr))
        goto Exit;

    // Hack!
    // Assumes internal knowledge of security id
    // the security has the zone id as the last dword, the rest of the stuff is
    // just the protocol followed by the site
    if (dwSecIdDocBase - sizeof(DWORD)) {
        pbSecIdDocBase[dwSecIdDocBase - sizeof(DWORD)] = '\0';
    }

    lstrcpyn(szKey, REGKEY_DECLINED_COMPONENTS, MAX_PATH*2);

    if (SUCCEEDED(::Unicode2Ansi(pwszDistUnit, &pszDistUnit)))
    {
        StrCatBuff(szKey, "\\", MAX_PATH*2);
        StrCatBuff(szKey, pszDistUnit, MAX_PATH*2);

        SAFEDELETE(pszDistUnit);

        if (SHRegGetUSValue( szKey, (char *)pbSecIdDocBase, NULL, NULL, NULL, 0,NULL,0) == ERROR_SUCCESS)
        {
            bDeclined = TRUE;
        }
    }

Exit:

    return bDeclined;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::HasUserDeclined
HRESULT
CCodeDownload::HasUserDeclined(
    LPCWSTR szDistUnit,
    LPCWSTR szType,
    LPCWSTR szExt,
    IBindStatusCallback *pClientBSC,
    IInternetHostSecurityManager *pHostSecurityManager)
{

    HRESULT hr = S_OK;
    DWORD grfBINDF = 0;
    BINDINFO bindInfo;
    memset(&bindInfo, 0, sizeof(BINDINFO));
    bindInfo.cbSize = sizeof(BINDINFO);

    if (pHostSecurityManager) {

        pClientBSC->GetBindInfo(&grfBINDF, &bindInfo);

        ReleaseBindInfo(&bindInfo);

        if (!(grfBINDF & BINDF_RESYNCHRONIZE)) {    // User has hit refresh

            if ((szDistUnit && IsDeclined(szDistUnit,pHostSecurityManager)) ||
                (szType && IsDeclined(szType,pHostSecurityManager)) ||
                (szExt && IsDeclined(szExt,pHostSecurityManager))) {

                hr = INET_E_CODE_DOWNLOAD_DECLINED;

            }
        }
    }

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::HandleDuplicateCodeDownloads
// handles duplicates by piggy backing them on to exitsing CCodeDownloads
// with matching szURL or rclsid
//  Returns:
//          S_OK: no dups found, do separate code download
//          MK_S_ASYNCHRONOUS: dup piggybacked
//          Any other error: fatal error: fail
HRESULT
CCodeDownload::HandleDuplicateCodeDownloads(
    LPCWSTR szURL,
    LPCWSTR szType,
    LPCWSTR szExt,
    REFCLSID rclsid,
    LPCWSTR szDistUnit,
    DWORD dwClsContext,
    LPVOID pvReserved,
    REFIID riid,
    IBindCtx* pbc,
    IBindStatusCallback *pDupClientBSC,
    DWORD dwFlags,
    IInternetHostSecurityManager *pHostSecurityManager)
{
    HRESULT hr = S_OK;
    LISTPOSITION curpos;
    CCodeDownload *pcdl;
    int iNumCDL;
    int i;

    CUrlMkTls tls(hr); // hr passed by reference!

    if (FAILED(hr))
        goto Exit;

    // first check to make sure that
    // this object has not been cancelled before by user
    // we will skip this check only when the user hits refresh on a page

    if (!(dwFlags & CD_FLAGS_SKIP_DECLINED_LIST_CHECK)) {
        hr = HasUserDeclined(szDistUnit, szType, szExt,pDupClientBSC,pHostSecurityManager);
        if (FAILED(hr))
            goto Exit;
    }

    iNumCDL = tls->pCodeDownloadList->GetCount();
    curpos = tls->pCodeDownloadList->GetHeadPosition();

    // walk thru all the code downloads in the thread and check for DUPs
    for (i=0; i < iNumCDL; i++) {

        pcdl = tls->pCodeDownloadList->GetNext(curpos);

        BOOL bNullClsid = IsEqualGUID(rclsid , CLSID_NULL);

        if (bNullClsid) {

            // handle dups based on TYPE and Ext
            if (! ( ( szDistUnit && pcdl->GetMainDistUnit() &&
                    (StrCmpIW(szDistUnit, pcdl->GetMainDistUnit()) == 0)) ||
                    ( szType && pcdl->GetMainType() &&
                    (StrCmpIW(szType, pcdl->GetMainType()) == 0)) ||
                    ( szExt && pcdl->GetMainExt() &&
                    (StrCmpIW(szExt, pcdl->GetMainExt()) == 0))
                   ) ) {

                // no match by type or ext

                continue;
            }

            // found match, fall thru to piggyback

        } else if (IsEqualGUID(rclsid , pcdl->GetClsid())) {

            if (szURL) {

                if(StrCmpIW(szURL, pcdl->GetMainURL()) != 0) {
                    pcdl->m_debuglog->DebugOut(DEB_CODEDL, FALSE, ID_CDLDBG_OBJ_TAG_MIXED_USAGE,
                                  (pcdl->GetClsid()).Data1,szURL, pcdl->GetMainURL());

                }

            } else {

                if(pcdl->GetMainURL() != NULL) {

                    pcdl->m_debuglog->DebugOut(DEB_CODEDL, FALSE, ID_CDLDBG_OBJ_TAG_MIXED_USAGE,
                    (pcdl->GetClsid()).Data1, pcdl->GetMainURL(), NULL);

                }
            }

            // found matching GUID, fall thru to piggyback

        } else if (szURL && (pcdl->GetMainURL() != NULL)) {

                if (StrCmpIW(szURL, pcdl->GetMainURL()) != 0) {
                    continue;
                }

                // found matching CODEBASE, fall thru to piggyback

        } else {
            continue;
        }

        // found DUP!
        if (pcdl->GetState() != CDL_Completed) {
            hr = pcdl->PiggybackDupRequest(pDupClientBSC, pbc,
                rclsid, dwClsContext, pvReserved, riid);

            if (hr == S_OK) {
                // piggy back was successful
                hr = MK_S_ASYNCHRONOUS;
            }
        }


        break;

    } /* for */

Exit:

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::SetWaitingForEXE
//  set that we are waiting for an EXE
//  either a self-registering localserver32 or a setup program
HRESULT
CCodeDownload::SetWaitingForEXE(
    LPCSTR szEXE,
    BOOL bDeleteEXEWhenDone)
{
    m_flags |= CD_FLAGS_WAITING_FOR_EXE;

    SAFEDELETE(m_szWaitForEXE);

    int len = 0;

    if (szEXE)
        len = lstrlen(szEXE);

    if (len) {
        m_szWaitForEXE = new CHAR [len + 1];
    } else {
        return E_INVALIDARG;
    }

    if (!m_szWaitForEXE) {
        return  E_OUTOFMEMORY;
    }

    lstrcpy(m_szWaitForEXE, szEXE);

    if (bDeleteEXEWhenDone)
        SetDeleteEXEWhenDone();

    return S_OK;
}

typedef HRESULT
(*POMNIPROX_REGISTER)(
    LPCWSTR szFileName
    );

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::RegisterOmniDll
//              Self-register's the PE OCX.
HRESULT
CCodeDownload::RegisterOmniDll(
    LPCSTR lpSrcFileName)
{
    HMODULE hMod;
    POMNIPROX_REGISTER lpfnDllRegisterServer;
    HRESULT hr = S_OK;
    WCHAR szBuf[MAX_PATH];

    if (!MultiByteToWideChar(CP_ACP, 0, lpSrcFileName, -1, szBuf, MAX_PATH)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if ((hMod = LoadLibrary(g_szOMNIPROX)) == NULL) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if ( (lpfnDllRegisterServer = (POMNIPROX_REGISTER)GetProcAddress( hMod,
                        "DllRegisterOmniServer")) == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (lpfnDllRegisterServer)
        hr = (*lpfnDllRegisterServer)(szBuf);

    FreeLibrary(hMod);

Exit:

    return hr;
}

typedef HRESULT (STDAPICALLTYPE *LPFNREGSVR)();

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::RegisterPEDll
//              Self-register's the PE OCX.
HRESULT
CCodeDownload::RegisterPEDll(
    LPCSTR lpSrcFileName)
{
    HMODULE hMod;
    LPFNREGSVR lpRegSvrFn;
    HRESULT hr = S_OK;
    IActiveXSafetyProvider *pProvider;

    if ((hr = IsRegisterableDLL(lpSrcFileName)) != S_OK) {

        // no DllRegisterServer entry point, don't LoadLibarary it.
        m_debuglog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_MISSING_DLLREGISTERSERVER, lpSrcFileName);
        goto Exit;
    }

    m_debuglog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_DLL_REGISTERED, lpSrcFileName);

    // assuming oleinitialze
    hr = GetActiveXSafetyProvider(&pProvider);
    if (hr != S_OK) {
        goto Exit;
    }

    if (pProvider) {

        hr = pProvider->SafeDllRegisterServerA(lpSrcFileName);
        pProvider->Release();

    } else {
        if ((hMod = LoadLibraryEx(lpSrcFileName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)) == NULL) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        lpRegSvrFn = (LPFNREGSVR) GetProcAddress(hMod, "DllRegisterServer");
        if (lpRegSvrFn == NULL) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        if (lpRegSvrFn)
            hr = (*lpRegSvrFn)();

        FreeLibrary(hMod);
    }

Exit:

    return hr;
}


#ifdef WX86
// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::RegisterWx86Dll
//              Self-register's the PE OCX.
HRESULT
CCodeDownload::RegisterWx86Dll(
    LPCSTR lpSrcFileName)
{
    HMODULE hModWx86;
    HMODULE hMod;
    FARPROC lpfnDllRegisterServerX86;
    FARPROC lpfnDllRegisterServer;
    HRESULT hr = S_OK;
    LPWSTR  lpwSrcFileName;

    typedef HMODULE (*pfnLoadFn)(LPCWSTR name, DWORD dwFlags);
    typedef PVOID (*pfnThunkFn)(PVOID pvAddress, PVOID pvCbDispatch, BOOLEAN fNativeToX86);
    typedef BOOL (*pfnUnloadFn)(HMODULE hMod);
    pfnLoadFn pfnLoad;
    pfnThunkFn pfnThunk;
    pfnUnloadFn pfnUnload;

    if ((hr = IsRegisterableDLL(lpSrcFileName)) != S_OK) {

        // no DllRegisterServer entry point, don't LoadLibarary it.
        CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_MISSING_DLLREGISTERSERVER, lpSrcFileName);
        goto Exit;
    }

    // Load Wx86 and get pointers to some useful exports

    hModWx86 = LoadLibrary("wx86.dll");
    if (!hModWx86) {
        hr = HRESULT_FROM_WIN32(ERROR_EXE_MACHINE_TYPE_MISMATCH);
        CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_INCOMPATIBLE_BINARY, lpSrcFileName);
        goto Exit;
    }
    pfnLoad = (pfnLoadFn)GetProcAddress(hModWx86, "Wx86LoadX86Dll");
    pfnThunk = (pfnThunkFn)GetProcAddress(hModWx86, "Wx86ThunkProc");
    pfnUnload = (pfnUnloadFn)GetProcAddress(hModWx86, "Wx86FreeX86Dll");
    if (!pfnLoad || !pfnThunk || !pfnUnload) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit1;
    }

    CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_DLL_REGISTERED, lpSrcFileName);

    // assuming oleinitialze

    if (FAILED((hr=::Ansi2Unicode(lpSrcFileName, &lpwSrcFileName)))) {
        goto Exit1;
    }

    if ((hMod = (*pfnLoad)(lpwSrcFileName, LOAD_WITH_ALTERED_SEARCH_PATH)) == NULL) {


        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit1;
    }

    delete lpwSrcFileName;

    if ( (lpfnDllRegisterServerX86 = GetProcAddress( hMod,
                        "DllRegisterServer")) == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (lpfnDllRegisterServerX86) {
        //
        // lpfnDllRegisterServerX86 is a pointer to an x86 function which
        // takes no parameters.  Create a native-to-x86 thunk for it.
        //
        lpfnDllRegisterServer = (FARPROC)(*pfnThunk)(lpfnDllRegisterServerX86,
                                                     (PVOID)0,
                                                     TRUE
                                                    );
        if (lpfnDllRegisterServer == (FARPROC)-1) {
            //
            // Something went wrong.  Possibly out-of-memory.
            //
            hr = E_UNEXPECTED;
            goto Exit1;
        }

        hr = (*lpfnDllRegisterServer)();
    }

    (*pfnUnload)(hMod);

Exit1:
    FreeLibrary(hModWx86);

Exit:

    return hr;
}
#endif  //WX86


// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::DelayRegisterOCX
//              Self-register's the OCX.
HRESULT
CCodeDownload::DelayRegisterOCX(
    LPCSTR pszSrcFileName,
    FILEXTN extn)
{
    HRESULT hr = S_OK;
    HKEY hKeyRunOnce = NULL;
    int line = 0;
    char szPath[MAX_PATH];
    char lpszCmdLine[2*MAX_PATH];
    char lpSrcFileName[MAX_PATH];
    char szTgtFileName[MAX_PATH];
    DWORD   dwTmp;
    const char *szICDRUNONCE = "ICDRegOCX%d";
    const char *szICDRUNDLL="rundll32.exe advpack.dll,RegisterOCX %s";



    // See comment in UpdateFileList to see why this is necessary
    // The reason we do this here is the same, except we need to use
    // the ANSI code page for regsvr32 this time.

    if (g_bRunOnWin95) {
        OemToChar(pszSrcFileName, lpSrcFileName);
        lstrcpy(szTgtFileName, lpSrcFileName);
    }
    else {
        lstrcpy(szTgtFileName, pszSrcFileName);
    }

    if ( RegCreateKeyEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, (ULONG)0, NULL,
           REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyRunOnce, &dwTmp ) != ERROR_SUCCESS ) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Check if key already exists -- if so, go with next number.
    //
    for (;;)
    {
        wsprintf( szPath, szICDRUNONCE, line++ );
        if ( RegQueryValueEx( hKeyRunOnce, szPath, 0, NULL, NULL, &dwTmp )
             != ERROR_SUCCESS )
        {
            break;
        }
    }

#ifdef WX86
    if (GetMultiArch()->GetRequiredArch() == PROCESSOR_ARCHITECTURE_INTEL) {
        char szSysDirX86[MAX_PATH];

        // An x86 control is downloading.  Tell GetSystemDirectory
        // to return the sys32x86 dir instead of system32
        NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = TRUE;
        GetSystemDirectory(szSysDirX86, MAX_PATH);

        // Run the RISC rundll32.exe but specify the fully-qualified name of
        // the x86 advpack.dll installed in %windir%\sys32x86.  RISC rundll32
        // is Wx86-aware and will load and run the x86 DLL.
        wsprintf( lpszCmdLine, "rundll32.exe %s\advpack.dll,RegisterOCX %s", szSysDirX86, szTgtFileName );

    } else {
        wsprintf( lpszCmdLine, szICDRUNDLL, szTgtFileName );
    }
#else
    wsprintf( lpszCmdLine, szICDRUNDLL, szTgtFileName );
#endif

    if ( RegSetValueEx( hKeyRunOnce, szPath, 0, REG_SZ, (CONST UCHAR *) lpszCmdLine, lstrlen(lpszCmdLine)+1 )
         != ERROR_SUCCESS ) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:

    if ( hKeyRunOnce != NULL ) {
        RegCloseKey( hKeyRunOnce );
    }

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::InstallOCX
//              Self-register's the OCX.
HRESULT
CCodeDownload::InstallOCX(
    LPCSTR lpSrcFileName,
    FILEXTN extn,
    BOOL bLocalServer)
{
    HMODULE hMod;
    FARPROC lpfnDllRegisterServer;
    HRESULT hr = S_OK;
    const static char *szREGSVR = " /RegServer";
    char szCmdLine[MAX_PATH+sizeof(szREGSVR)];
    STARTUPINFO si;
    DWORD dwResult;
    DWORD dwMachineType = 0;

    switch (extn) {

    case FILEXTN_CAB:
    case FILEXTN_INF:

        // can't install cab or INF
        hr = E_INVALIDARG;
        break;

    case FILEXTN_EXE:

        lstrcpy(szCmdLine, lpSrcFileName);
        if (bLocalServer)
            lstrcat(szCmdLine, szREGSVR);

        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);

        if (!CreateProcess(NULL,
                szCmdLine,
                0,                  // security
                0,                  // security
                FALSE,              // Don't inherit my handles!
                0,                  // Creation flags
                NULL,               // env = inherit
                NULL,               // cur dir. = inherit
                &si,                // no startup info
                &m_pi))
            {

            hr = HRESULT_FROM_WIN32(GetLastError());

        } else {

            hr = SetWaitingForEXE(lpSrcFileName, !bLocalServer);
        }

        goto Exit;


    case FILEXTN_OCX:
    case FILEXTN_DLL:
    case FILEXTN_NONE:
    case FILEXTN_UNKNOWN:


        // sniff machine type of PE

        hr = IsCompatibleFile(lpSrcFileName, &dwMachineType);

        if (hr == HRESULT_FROM_WIN32(ERROR_EXE_MACHINE_TYPE_MISMATCH)) {

            // if its of worng CPU flavor fail and clean up the OCX
            m_debuglog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_INCOMPATIBLE_BINARY, lpSrcFileName);
            break;
        }

        if (hr == S_FALSE) {
            // not a PE file, no need to call LoadLibrary
            // just copy and install the file

            break;
        }

        if (dwMachineType == IMAGE_FILE_MACHINE_OMNI)
            hr = RegisterOmniDll(lpSrcFileName);
#ifdef WX86
        else if (g_fWx86Present && dwMachineType == IMAGE_FILE_MACHINE_I386)
            hr = RegisterWx86Dll(lpSrcFileName);
#endif
        else
            hr = RegisterPEDll(lpSrcFileName);

    }

Exit:
    return hr;
}


// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::UpdateZeroImpactCache
//             Set up the ZI specific registry keys
//             Assumes UpdateDistUnit has already been called
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::UpdateZeroImpactCache()
{
    TCHAR szKeyName[MAX_PATH];
    TCHAR szDll[MAX_PATH];
    WCHAR wszDll[MAX_PATH];
    CHAR szVersion[40];
    HKEY hkey = NULL;
    HKEY hkeyClasses = NULL;
    HRESULT hr = S_OK;
    LONG lRes = 0;
    int iRes = 0;
    BOOL bHaveszDll = FALSE;
    const TCHAR * szDirectoryValueName = TEXT("Directory");
    const TCHAR * szContainsClassesKeyName = TEXT("Contains\\Classes");
    const TCHAR * szZeroImpactValueName = TEXT("ZeroImpact");
    TCHAR szClassString[MAX_PATH];
    DWORD dwDummy = 0;

    Assert(GetClientBinding());
    LPWSTR pwzClassString = GetClientBinding()->GetClassString();

    // ***Begin writing zero impact values to the registry***

    if(! m_szDistUnit || ! m_szDistUnit[0] ||
       ! m_szCacheDir || ! m_szCacheDir[0])
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // buffer overflow protection
    if(MAX_VERSIONLENGTH + lstrlenW(m_szDistUnit) + lstrlen(REGKEY_ZEROIMPACT_DIRS) + 3 > MAX_PATH)
        return E_UNEXPECTED;

    if(! GetStringFromVersion(szVersion, m_dwFileVersionMS, m_dwFileVersionLS, '_'))
    {
        // no good version string, there's nothing we can do
        hr = E_UNEXPECTED;
        goto Exit;
    }

    // szKeyName = "ROOT_ZERO_IMPACT_KEY\DistUnit!Version
    wsprintf(szKeyName, TEXT("%s\\%ls!%s"), REGKEY_ZEROIMPACT_DIRS, m_szDistUnit, szVersion);

    // If the key does not exist (should have been created in update dist unit) create it
    lRes = RegOpenKey(HKEY_LOCAL_MACHINE, szKeyName, &hkey);
    if(lRes != ERROR_SUCCESS)
    {
        lRes = RegCreateKey(HKEY_LOCAL_MACHINE, szKeyName, &hkey);
        if(lRes != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRes);
            goto Exit;
        }
    }

    // set its Directory value to the cache dir
    lRes = RegSetValueEx(hkey, szDirectoryValueName, 0, REG_SZ, (BYTE *) m_szCacheDir, (lstrlen(m_szCacheDir) + 1)*sizeof(CHAR));
    if(lRes != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRes);
        goto Exit;
    }

    // mark it as zeroimpact
    dwDummy = 0;
    lRes = RegSetValueEx(hkey, szZeroImpactValueName, 0, REG_DWORD, (BYTE *) &dwDummy, sizeof(DWORD));
    if(lRes != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRes);
        goto Exit;
    }

    // now set the dll name, if it exists
    bHaveszDll = FALSE;
    if (!GetClientBinding()->GetZIDll())
    {
        // Not existant?  Try to recover it from the file in the cache directory
        HRESULT hr1 = S_OK;
        int iBufLen = MAX_PATH;
        hr1 = ZIGetDllName(m_szCacheDir, m_szDistUnit,
                           pwzClassString, m_dwFileVersionMS,
                           m_dwFileVersionLS, szDll, &iBufLen);
        if ((hr1 == S_OK) && (MultiByteToWideChar(CP_ACP, 0, szDll, -1, wszDll, MAX_PATH) != 0))
        {
            GetClientBinding()->SetZIDll(wszDll);
            bHaveszDll = TRUE;
        }
    }
    else
    {
        // It exists, try to convert it to a TCHAR to write to the registry
        iRes = WideCharToMultiByte(CP_ACP, 0, GetClientBinding()->GetZIDll(),
                                   -1, szDll, MAX_PATH, 0,0);
        if(iRes == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit; // bad translation
        }
        bHaveszDll = TRUE;
    }

    if (pwzClassString)
    {
        iRes = WideCharToMultiByte(CP_ACP, 0, pwzClassString, -1, szClassString, MAX_PATH, 0,0);
    }
    if (pwzClassString && iRes != 0)
    {
        // If we have a dll string to write to the registry...
        if(bHaveszDll)
        {
            lRes = RegCreateKey(hkey, szContainsClassesKeyName, &hkeyClasses);
            if(lRes != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(lRes);
                goto Exit;
            }

            lRes = RegSetValueEx(hkeyClasses, szClassString, 0, REG_SZ, (BYTE *)szDll, (lstrlen(szDll) + 1)*sizeof(TCHAR));
            if(FAILED(HRESULT_FROM_WIN32(lRes)))
            {
                hr = HRESULT_FROM_WIN32(lRes);
                goto Exit;
            }
        }
        else
        {
            // make sure no values from a previous install exist
            lRes = RegOpenKey(HKEY_LOCAL_MACHINE, szContainsClassesKeyName, &hkeyClasses);
            if(lRes != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(lRes);
                goto Exit;
            }
            RegDeleteValue(hkey, szClassString);
        }
    }

    //BUGBUG: enumerate through files to add them to contains\files for removal by ocxcache?


Exit:
    if(hkeyClasses)
        RegCloseKey(hkeyClasses);
    if(hkey)
        RegCloseKey(hkey);
    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::HandleUnSafeAbort
HRESULT
CCodeDownload::HandleUnSafeAbort()
{
    HRESULT hr = S_FALSE;
    ICodeInstall* pCodeInstall = GetICodeInstall();

    if(WaitingForEXE()) { // Did the setup start a self-registering EXE?

        // we are waiting for an EXE to complete self-registering
        // notify client of condition and maybe it wants to
        // ask the user if (s)he wants to wait for the EXE or abort
        // download
        // we never kill the EXE though we just ignore it

        if (pCodeInstall) {
            WCHAR szBuf[MAX_PATH];

            szBuf[0] = '\0';

            if (m_szWaitForEXE) {
                MultiByteToWideChar(CP_ACP, 0, m_szWaitForEXE, -1, szBuf, MAX_PATH);
            }
            hr = pCodeInstall->OnCodeInstallProblem(
                                CIP_EXE_SELF_REGISTERATION_TIMEOUT,
                                NULL, szBuf, 0);
        } else {
            hr = S_FALSE; // assume skip EXE and proceed
        }

        if (hr == S_OK)         // continue to wait?
            return S_FALSE;     // yes

        // if hr == S_FALSE/E_ABORT ignore this EXE and try to proceed with
        // rest of installation

        if (m_pi.hProcess != INVALID_HANDLE_VALUE) {
            CloseHandle(m_pi.hProcess);
            m_pi.hProcess = INVALID_HANDLE_VALUE;
        }
        if (m_pi.hThread != INVALID_HANDLE_VALUE) {
            CloseHandle(m_pi.hThread);
            m_pi.hThread = INVALID_HANDLE_VALUE;
        }
        SetNotWaitingForEXE();

        m_hr = E_ABORT;
        CCDLPacket *pPkt= new CCDLPacket(CODE_DOWNLOAD_SETUP, this, 0);

        if (pPkt) {
            hr = pPkt->Post();
        } else {
            hr = E_OUTOFMEMORY;
        }

        return hr;
    }

    if (pCodeInstall) {
        hr = pCodeInstall->OnCodeInstallProblem(CIP_UNSAFE_TO_ABORT,
                                NULL, NULL, 0);
    } else {
        hr = E_ABORT;
    }

    if (hr == S_OK) {
        hr = S_FALSE;
    } else {
        SetUserCancelled();
        hr = E_ABORT;
    }

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::SelfRegEXETimeout
HRESULT
CCodeDownload::SelfRegEXETimeout()
{
    HRESULT hr = S_OK;

    if (!WaitingForEXE())
        return S_FALSE;

    Assert(m_pi.hProcess != INVALID_HANDLE_VALUE);
    DWORD dwResult = WaitForSingleObject(m_pi.hProcess, 0);

    if (dwResult != WAIT_OBJECT_0) {

        // the EXE has not yet completed.
        // just wait for it till we get it or client calls
        // IClientBinding::Abort()

        CCDLPacket *pPkt= new CCDLPacket(CODE_DOWNLOAD_WAIT_FOR_EXE,
            this,0);

        if (pPkt) {
            hr = pPkt->Post();
        } else {
            hr = E_OUTOFMEMORY;
        }

        return hr;
    }

    if (!GetExitCodeProcess(m_pi.hProcess, &dwResult))
        dwResult = GetLastError();

    if (m_pi.hProcess != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pi.hProcess);
        m_pi.hProcess = INVALID_HANDLE_VALUE;
    }
    if (m_pi.hThread != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pi.hThread);
        m_pi.hThread = INVALID_HANDLE_VALUE;
    }

    SetNotWaitingForEXE();

    if (DeleteEXEWhenDone()) {
        DeleteFile(m_szWaitForEXE);
        ResetDeleteEXEWhenDone();
    }

    hr = HRESULT_FROM_WIN32(dwResult);

    if (hr == ERROR_SUCCESS_REBOOT_REQUIRED) {
        SetRebootRequired();
        hr = S_OK;
    }

    m_hr = hr;
    CCDLPacket *pPkt= new CCDLPacket(CODE_DOWNLOAD_SETUP, this, 0);

    if (pPkt) {
        hr = pPkt->Post();
    } else {
        hr = E_OUTOFMEMORY;
    }

    Assert(SUCCEEDED(hr));

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::SetManifest()
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::SetManifest(FILEXTN extn, LPCSTR szManifest)
{
    HRESULT hr = S_OK;

    LPSTR szFile = new char [lstrlen(szManifest)+1];

    if (!szFile) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    lstrcpy(szFile, szManifest);

    if (extn == FILEXTN_INF) {
        SAFEDELETE(m_szInf);
        m_szInf = szFile;
    } else {
        SAFEDELETE(m_szOSD);
        m_szOSD = szFile;
    }

Exit:

    return hr;

}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::VersionFromManifest
// ---------------------------------------------------------------------------
BOOL
CCodeDownload::VersionFromManifest(LPSTR szVersionInManifest)
{

    if (m_szVersionInManifest) {

        lstrcpy(szVersionInManifest, m_szVersionInManifest);
        return TRUE;
    }

    return FALSE;
}


// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::ProcessJavaManifest
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::ProcessJavaManifest(IXMLElement *pJava, const char *szOSD, char *szOSDBaseName, CDownload *pdl)
{
    HRESULT hr = S_OK;
    IXMLElement *pPackage = NULL, *pElemTmp = NULL, *pConfig = NULL;
    CDownload *pdlCur = NULL;
    LPWSTR szPackageName = NULL;
    char szPackageVersion[MAX_PATH];
    DWORD dwVersionMS = 0, dwVersionLS = 0, dwJavaFlags = 0;
    int nLastPackage, nLastConfig;
    CCodeBaseHold *pcbh = NULL;
    char szPackageURLA[INTERNET_MAX_URL_LENGTH];
    char *pBaseFileName = NULL;
    LPWSTR pszNameSpace = NULL;
    CList<CCodeBaseHold *, CCodeBaseHold *>    *pcbhList = NULL;
    BOOL bDestroyPCBHList = FALSE;
    int iCount = 0;

    if (!pdl->HasJavaPermissions()) {

        if (IsSilentMode())
        {
            SetBitsInCache();
        } else {

            hr = TRUST_E_FAIL;
            goto Exit;
        }
    }

    hr = GetTextContent(pJava, DU_TAG_NAMESPACE, &pszNameSpace);

    if (FAILED(hr))
        goto Exit;

    // while more packages

    nLastPackage = -1;
    while( (GetNextChildTag(pJava, DU_TAG_PACKAGE, &pPackage, nLastPackage)) == S_OK) {

        SAFEDELETE(szPackageName);

        // process package
        hr = DupAttribute(pPackage, DU_ATTRIB_NAME, &szPackageName);
        CHECK_ERROR_EXIT(SUCCEEDED(hr), ID_CDLDBG_JAVAPACKAGE_SYNTAX);

        if (GetAttributeA(pPackage, DU_ATTRIB_VERSION,
            szPackageVersion, MAX_PATH) == S_OK) {

            hr = GetVersionFromString(szPackageVersion, &dwVersionMS, &dwVersionLS);
            CHECK_ERROR_EXIT(SUCCEEDED(hr), ID_CDLDBG_JAVAPACKAGE_SYNTAX);
        } else {
            hr = E_INVALIDARG;
            CHECK_ERROR_EXIT(SUCCEEDED(hr), ID_CDLDBG_JAVAPACKAGE_SYNTAX);
        }

        if (GetFirstChildTag(pPackage, DU_TAG_SYSTEM, &pElemTmp) == S_OK) {
            m_dwSystemComponent = TRUE;
            SAFERELEASE(pElemTmp);
        }

        // check if package of right version is already locally installed
        // if so go to next package

        hr = IsPackageLocallyInstalled(szPackageName, pszNameSpace, dwVersionMS, dwVersionLS);

        if (FAILED(hr))
            goto Exit;

        if (hr == S_OK) {

            // OK, so this package that is reqd by this dist unit
            // is already present on machine
            // we still need to create a NOSETUP JavaSetup obj just so
            // it gets marked as belonging to/used by this dist unit.

            // for a NOSETUP CJavaSetup it doesn't matter which pdl it gets
            // added on to.
            hr = pdl->AddJavaSetup(szPackageName, pszNameSpace, pPackage, dwVersionMS, dwVersionLS, CJS_FLAG_NOSETUP);

            if (FAILED(hr))
                goto Exit;

            goto nextPackage;
        }

        hr = S_OK;  // reset

        // else, make a CJavaSetup for each package that needs to be installed
        // also make sure that the CABs in those packages are downloaded

        nLastConfig = -1;

        // OR all NEEDSTRUSTEDSOURCE & SYSTEM flags from all CONFIG's since there may be
        // multiple CODEBASE's.
        dwJavaFlags = CJS_FLAG_INIT;

        if (m_dwSystemComponent) {
            dwJavaFlags |= CJS_FLAG_SYSTEM;
        }

        if (GetFirstChildTag(pPackage, DU_TAG_NEEDSTRUSTEDSOURCE, &pElemTmp) == S_OK) {
            dwJavaFlags |= CJS_FLAG_NEEDSTRUSTEDSOURCE;
            SAFERELEASE(pElemTmp);
        }

        // If no CODEBASE specified in CONFIG, add Setup of Java package to this download.
        pdlCur = pdl;

        while (GetNextChildTag(pPackage, DU_TAG_CONFIG, &pConfig, nLastConfig) == S_OK) {

            // This is destroyed by destructor of DoDownload called

            if (bDestroyPCBHList) {
                DestroyPCBHList(pcbhList);
                SAFEDELETE(pcbhList);
            }
            pcbhList = new CList<CCodeBaseHold *, CCodeBaseHold *>;
            if (pcbhList == NULL) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            bDestroyPCBHList = TRUE;
            pcbhList->RemoveAll();

            if (ProcessImplementation(pConfig, pcbhList, m_lcid
#ifdef WX86
                                      , GetMultiArch()
#endif
                                      ) == S_OK) {

                iCount = pcbhList->GetCount();
                if (iCount) {
                    pcbh = pcbhList->GetHead();
                    pcbh->dwFlags |= CBH_FLAGS_DOWNLOADED;
                }
                else {
                    pcbh = NULL;
                }

                if (pcbh && pcbh->wszCodeBase) {

                    WideCharToMultiByte(CP_ACP, 0, pcbh->wszCodeBase, -1,
                          szPackageURLA, sizeof(szPackageURLA),NULL, NULL);

                    FILEXTN extn = ::GetExtnAndBaseFileName(szPackageURLA, &pBaseFileName);

                    if (extn != FILEXTN_CAB) {
                        hr = E_INVALIDARG;
                        goto Exit;
                    }

                    if (pcbh->bHREF) {
                        // CODEBASE HREF="..." download CAB with java package

                        hr = FindCABInDownloadList(pcbh->wszCodeBase, pdl, &pdlCur);

                        if (FAILED(hr))
                            goto Exit;

                        if (!pdlCur) {

                            // did not find CAB
                            // fresh CAB needs to get pulled down.

                            pdlCur = new CDownload(pcbh->wszCodeBase, extn, &hr);
                            if (!pdlCur) {
                                hr = E_OUTOFMEMORY;
                            }

                            if (FAILED(hr)) {
                                SAFEDELETE(pdlCur);
                                goto Exit;
                            }

                            AddDownloadToList(pdlCur);

                            {
                            BOOL bSetOnStack = SetOnStack();
                            bDestroyPCBHList = FALSE;
                            hr = pdlCur->DoDownload(&m_pmkContext,
                                                     (BINDF_ASYNCHRONOUS|
                                                     BINDF_ASYNCSTORAGE),
                                                     pcbhList);
                            if (bSetOnStack)
                                ResetOnStack();
                            }

                        }

                    }

                    if (FAILED(hr)) {
                        goto Exit;
                    }


                } else {
                    // found a valid config but no CODEBASE
                    // assume that the pkg is in 'thiscab'
                    // goto add package
                }

                goto addPackage;

            } // Got CONFIG tag successfully

            SAFERELEASE(pConfig);

        } // <CONFIG> tag loop

        if (SUCCEEDED(hr)) {
            hr = HRESULT_FROM_WIN32(ERROR_APP_WRONG_OS);
        }

        goto nextPackage;

addPackage:
        hr = pdlCur->AddJavaSetup(szPackageName, pszNameSpace, pPackage, dwVersionMS, dwVersionLS, dwJavaFlags);

nextPackage:
        SAFERELEASE(pPackage);
        SAFERELEASE(pConfig);

        if (FAILED(hr))
            break;

        if (hr == S_FALSE)
            hr = S_OK;  // reset

    } // <PACKAGE> tag loop

Exit:
    SAFERELEASE(pConfig);
    SAFERELEASE(pPackage);

    SAFEDELETE(szPackageName);
    SAFEDELETE(pszNameSpace);

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::ProcessDependency
//    Processes <dependency> tag and spins off any dependency code downloads
//    as appropriate.
// ---------------------------------------------------------------------------
HRESULT CCodeDownload::ProcessDependency(CDownload *pdl, IXMLElement *pDepend)
{
    HRESULT              hr = S_OK;
    int                  nLast2, nLast3;
    BOOL                 fAssert = FALSE, fInstall = FALSE;
    IXMLElement         *pSoftDist2 = NULL, *pLang = NULL, *pConfig = NULL;
    LPWSTR               szDistUnit = NULL;
    LPWSTR               pwszURL = NULL;
    LPSTR                szLanguages = NULL;
    LPSTR                pBaseFileName = NULL;
    WCHAR                szCDLURL[2*INTERNET_MAX_URL_LENGTH];
    WCHAR                wszURLBuf[2*INTERNET_MAX_URL_LENGTH];
    WCHAR                szResult[INTERNET_MAX_URL_LENGTH];
    DWORD                dwSize = 0;
    DWORD                dwVersionMS = 0, dwVersionLS = 0, dwStyle;
    CDownload           *pdlCur = NULL;
    CLSID                inclsid = CLSID_NULL;
    CCodeBaseHold       *pcbh = NULL;
    CLocalComponentInfo  lci;
    int                  i, iCount = 0, iLen = 0;
    LISTPOSITION         lpos = 0;
    CCodeBaseHold       *pcbhCur = NULL;
    LPWSTR               pwszStr = NULL;
    CList<CCodeBaseHold *, CCodeBaseHold *>    *pcbhList = NULL;
    BOOL bDestroyPCBHList = FALSE;
    LPWSTR pwszVersion = NULL;

    union {
        char szAction[MAX_PATH];
        char szVersion[MAX_PATH];
        char szStyle[MAX_PATH];
        char szPackageURLA[INTERNET_MAX_URL_LENGTH];
    };

    if (SUCCEEDED(GetAttributeA(pDepend, DU_ATTRIB_ACTION, szAction, MAX_PATH))) {

        if (lstrcmpi(szAction, "ASSERT") == 0)
            fAssert = TRUE;
        else if (lstrcmpi(szAction, "INSTALL") == 0)
            fInstall = TRUE;
        else
            goto Exit;
    } else
        fAssert = TRUE;

    nLast2 = -1;
    while (GetNextChildTag(pDepend, DU_TAG_SOFTDIST, &pSoftDist2, nLast2) == S_OK) {

        if (FAILED(hr))
            break;

        // get NAME attribute
        hr = DupAttribute(pSoftDist2, DU_ATTRIB_NAME, &szDistUnit);
        CHECK_ERROR_EXIT(SUCCEEDED(hr), ID_CDLDBG_DEPENDENCY_SYNTAX);

        // get VERSION attribute
        hr = GetAttributeA(pSoftDist2, DU_ATTRIB_VERSION, szVersion, MAX_PATH);
        CHECK_ERROR_EXIT(SUCCEEDED(hr), ID_CDLDBG_DEPENDENCY_SYNTAX);

        // convert VERSION string
        hr = GetVersionFromString(szVersion, &dwVersionMS, &dwVersionLS);
        CHECK_ERROR_EXIT(SUCCEEDED(hr), ID_CDLDBG_DEPENDENCY_SYNTAX);

        // remember the version string in uni
        hr = Ansi2Unicode(szVersion, &pwszVersion);
        if (FAILED(hr))
            goto Exit;

        // get STYLE attribute
        if (SUCCEEDED(GetAttributeA(pSoftDist2, DU_ATTRIB_STYLE, szStyle, MAX_PATH))) {
            (void) GetStyleFromString(szStyle, &dwStyle);
        } else
            dwStyle = STYLE_MSICD;

        // Check if distribution unit is currently installed
        // NOTE: This assumes MSICD

        inclsid = CLSID_NULL;
        CLSIDFromString((LPOLESTR)szDistUnit, &inclsid);

        if ((hr = IsControlLocallyInstalled(NULL,
                (LPCLSID)&inclsid, szDistUnit,
                dwVersionMS, dwVersionLS, &lci, NULL)) != S_FALSE) {

            // add distribution unit as a dependency
            AddDistUnitList(szDistUnit);

            goto nextDepend;
        }

        // if Action=ASSERT and we don't have distribution unit, then skip this <softdist>.
        if (fAssert) {
            hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
            goto Exit;
        }

        // minimal check for circular dependency
        if (StrCmpIW(szDistUnit, m_szDistUnit)==0) {
            hr = HRESULT_FROM_WIN32(ERROR_CIRCULAR_DEPENDENCY);
            goto Exit;
        }

        // process CONFIG tags
        nLast3 = -1;
        pcbh = NULL;
        while (GetNextChildTag(pSoftDist2, DU_TAG_CONFIG, &pConfig, nLast3) == S_OK) {
            
            if (bDestroyPCBHList) {
                DestroyPCBHList(pcbhList);
                SAFEDELETE(pcbhList);
            }
            pcbhList = new CList<CCodeBaseHold *, CCodeBaseHold *>;
            if (pcbhList == NULL) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            bDestroyPCBHList = TRUE;
            pcbhList->RemoveAll();

            pcbh = NULL;
            hr = ProcessImplementation(pConfig, pcbhList, m_lcid
#ifdef WX86
                                       , GetMultiArch()
#endif
                                      );

            if (SUCCEEDED(hr)) {
                iCount = pcbhList->GetCount();
                if (iCount) {
                    pcbh = pcbhList->GetHead();
                    pcbh->dwFlags |= CBH_FLAGS_DOWNLOADED;
                }
                else {
                    pcbh = NULL;
                }
            }
            SAFERELEASE(pConfig);

            if (hr == S_OK) {

                szPackageURLA[0] = '\0';

                //BUGBUG: If no CODEBASE how do we know extension?  Assuming it is CAB
                FILEXTN extn = FILEXTN_CAB;

                if (pcbh && pcbh->wszCodeBase && pcbh->bHREF) {

                    WideCharToMultiByte(CP_ACP, 0, pcbh->wszCodeBase, -1, szPackageURLA,
                        INTERNET_MAX_URL_LENGTH,NULL, NULL);

                    extn = ::GetExtnAndBaseFileName(szPackageURLA, &pBaseFileName);

                }

                // "cdl:[clsid=xxx|codebase=xxx|mimetype=xxx|extension=xxx];"
                // we use: "cdl:distunit=xxxx[|codebase=xxxx]"

                // BUGBUG: We could mess up CDLProtocol if any of these embedded fields are
                //         illformatted (contains '=' or '\\' or '//').

                // cdl: protocol treats clsid as DistUnit name if not a valid CLSID.
                StrCpyW(szCDLURL, L"cdl:distunit=");
                StrCatW(szCDLURL, szDistUnit);
                StrCatW(szCDLURL, L";version=");
                StrCatW(szCDLURL, pwszVersion);

                if (szPackageURLA[0]) {
                    StrCatW(szCDLURL, L";codebase=");
                    if (SUCCEEDED(GetContextMoniker()->GetDisplayName(NULL, NULL, &pwszURL))) {
                        dwSize = INTERNET_MAX_URL_LENGTH;
                        if(FAILED(UrlCombineW(pwszURL, pcbh->wszCodeBase, szResult, &dwSize, 0))) {
                            hr = E_UNEXPECTED;
                            goto Exit;
                        }
                        StrCatW(szCDLURL, szResult);
                        SAFEDELETE(pwszURL);
                    }
                    else {
                        // A context moniker should always exist if we
                        // are looking at a dependency.
                        hr = E_UNEXPECTED;
                        goto Exit;
                    }
                }

                // Iterate over all codebases in the list, and covert them
                // to CDL: protocols instead of HTTP:.

                lpos = pcbhList->GetHeadPosition();
                while (lpos) {
                    pcbhCur = pcbhList->GetNext(lpos);
                    if (pcbhCur != NULL) {
                        StrCpyW(wszURLBuf, L"cdl:distunit=");
                        StrCatW(wszURLBuf, szDistUnit);
                        StrCatW(wszURLBuf, L";version=");
                        StrCatW(wszURLBuf, pwszVersion);
                        StrCatW(wszURLBuf, L";codebase=");

                        // Combine the context moniker's URL with the
                        // codebase supplied to handle relative dependency
                        // URLs. If the dependency URL is absolute,
                        // UrlCombineW will just return the absolute
                        // dependency URL.

                        if (SUCCEEDED(GetContextMoniker()->GetDisplayName(NULL, NULL, &pwszURL))) {
                            dwSize = INTERNET_MAX_URL_LENGTH;
                            if (FAILED(UrlCombineW(pwszURL, pcbhCur->wszCodeBase, szResult, &dwSize, 0))) {
                                hr = E_UNEXPECTED;
                                goto Exit;
                            }
                            iLen = lstrlenW(szResult) + lstrlenW(wszURLBuf) + 1;
                            pwszStr = new WCHAR[iLen];
                            if (pwszStr == NULL) {
                                SAFEDELETE(pwszURL);
                                hr = E_OUTOFMEMORY;
                                goto Exit;
                            }
                            StrCpyW(pwszStr, wszURLBuf);
                            StrCatW(pwszStr, szResult);
                            SAFEDELETE(pcbhCur->wszCodeBase);
                            pcbhCur->wszCodeBase = pwszStr;
                            SAFEDELETE(pwszURL);
                        }
                        else {
                            hr = E_UNEXPECTED;
                            goto Exit;
                        }
                    }
                }

                // Because of way this is processed it should create URLMoniker, which in
                // turn creates CCodeDownload and properly installs before we do our
                // setup here.  Thus we don't need to do anything else explicit here.

                pdlCur = new CDownload(szCDLURL, extn, &hr);
                if (!pdlCur) {
                    hr = E_OUTOFMEMORY;
                }

                if (FAILED(hr)) {
                    SAFEDELETE(pdlCur);
                    goto Exit;
                }

                AddDownloadToList(pdlCur);

                hr = pdlCur->SetUsingCdlProtocol(szDistUnit);

                if (FAILED(hr))
                    goto Exit;


                {
                BOOL bSetOnStack = SetOnStack();
                bDestroyPCBHList = FALSE;
                hr = pdlCur->DoDownload(&m_pmkContext, (BINDF_ASYNCHRONOUS|
                                        BINDF_ASYNCSTORAGE), pcbhList);
                if (bSetOnStack)
                    ResetOnStack();
                }

                // this is an indication "cdl://" is not installed.
                CHECK_ERROR_EXIT((hr != E_NOINTERFACE),ID_CDLDBG_CDL_HANDLER_MISSING);

                if (FAILED(hr))
                    goto Exit;

                goto nextDepend;

            }

            SAFEDELETE(pcbh);
        }

        if (SUCCEEDED(hr))
            hr = HRESULT_FROM_WIN32(ERROR_APP_WRONG_OS);

nextDepend:
        SAFERELEASE(pLang);
        SAFERELEASE(pSoftDist2);
        SAFEDELETE(szDistUnit);
        SAFEDELETE(szLanguages);

        if (FAILED(hr))
            break;
    }

Exit:
    SAFERELEASE(pLang);
    SAFERELEASE(pSoftDist2);
    SAFERELEASE(pConfig);
    SAFEDELETE(szDistUnit);
    SAFEDELETE(szLanguages);
    SAFEDELETE(pwszVersion);

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::ExtractInnerCAB
//    We have a nested CAB, extract its contents into temporary directory (do not
//    process any OSD, INF files for this).  If duplicate files exist we ignore
//    since this is a design error.
// ---------------------------------------------------------------------------
HRESULT CCodeDownload::ExtractInnerCAB(CDownload *pdl, LPSTR szCABFile)
{
    HRESULT hr = S_OK;
    SESSION *psess;
    CHAR szTempCABFile[MAX_PATH];

    psess = new SESSION;
    if (!psess) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    psess->pFileList        = NULL;
    psess->cFiles           = 0;
    psess->cbCabSize        = 0;
    psess->flags = SESSION_FLAG_ENUMERATE | SESSION_FLAG_EXTRACT_ALL;
    lstrcpy(psess->achLocation,pdl->GetSession()->achLocation);
    psess->pFilesToExtract = NULL;

    if (!catDirAndFile(szTempCABFile, MAX_PATH, psess->achLocation, szCABFile)) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    hr = ::Extract(psess, szTempCABFile);

    if (psess->pFileList && SUCCEEDED(hr)) {

        // add extracted files to download list for cleanup purposes
        PFNAME pfl = psess->pFileList;
        SESSION *psessdl = pdl->GetSession();
        while (pfl->pNextName) {
            pfl=pfl->pNextName;
        }
        pfl->pNextName = psessdl->pFileList;
        psessdl->pFileList = psess->pFileList;

    }

Exit:
    SAFEDELETE(psess);

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::ProcessNativeCode
//    Processes <nativecode> tag and spins off any dependency code downloads
//    as appropriate.
// ---------------------------------------------------------------------------
HRESULT CCodeDownload::ProcessNativeCode(CDownload *pdl, IXMLElement *pNativeCode)
{
    HRESULT     hr = S_OK;
    int         iCount;


    LPWSTR szName = NULL;
    union {
        char szCLSID[MAX_PATH];
        char szVersion[MAX_PATH];
    };
    char szTempFile[INTERNET_MAX_URL_LENGTH];
    LPSTR szCodeBase = NULL, szNativeName = NULL, pBaseFileName = NULL, szTempDir = NULL;
    CCodeBaseHold *pcbh = NULL;
    int nLast2, nLast3;
    DWORD dwVersionMS = 0, dwVersionLS = 0;
    CLSID clsid = CLSID_NULL;
    IXMLElement *pCode = NULL, *pElemTmp = NULL, *pConfig = NULL;
    BOOL fSetupInf = FALSE;
    CLocalComponentInfo lci;
    CSetup *pSetup = NULL;
    ICodeInstall* pCodeInstall = GetICodeInstall();
    BOOL bSystem = FALSE;
    CList<CCodeBaseHold *, CCodeBaseHold *>     *pcbhList = NULL;
    BOOL bDestroyPCBHList = FALSE;

    if (!pdl->HasAllActiveXPermissions()) {

        if (IsSilentMode())
        {
            SetBitsInCache();
        } else {

            hr = TRUST_E_FAIL;
            goto Exit;
        }
    }

    szTempDir = pdl->GetSession()->achLocation;

    nLast2 = -1;
    while (GetNextChildTag(pNativeCode, DU_TAG_CODE, &pCode, nLast2) == S_OK) {

        SAFEDELETE(szName);
        SAFEDELETE(szNativeName);

        if (FAILED(hr)) break;

        // get CLSID attribute
        hr = GetAttributeA(pCode, DU_ATTRIB_CLSID, szCLSID, MAX_PATH);
        if (SUCCEEDED(hr))
        {
            // convert CLSID attribute
            hr = ConvertFriendlyANSItoCLSID(szCLSID, &clsid);
            CHECK_ERROR_EXIT(SUCCEEDED(hr),ID_CDLDBG_NATIVECODE_SYNTAX);
        }
        else
        {
            clsid = CLSID_NULL;
            szCLSID[0] = '\0';
        }

        // get NAME attribute
        hr = DupAttribute(pCode, DU_ATTRIB_NAME, &szName);
        CHECK_ERROR_EXIT(SUCCEEDED(hr),ID_CDLDBG_NATIVECODE_SYNTAX);

        // use "NAME" attribute as file name to OCX/INF/DLL
        if (FAILED(hr = Unicode2Ansi(szName, &szNativeName)))
             break;

        // get VERSION attribute
        if (SUCCEEDED(GetAttributeA(pCode, DU_ATTRIB_VERSION, szVersion, MAX_PATH))) {

            // convert VERSION string
            hr = GetVersionFromString(szVersion, &dwVersionMS, &dwVersionLS);
            CHECK_ERROR_EXIT(SUCCEEDED(hr),ID_CDLDBG_NATIVECODE_SYNTAX);

        } else {

            dwVersionMS = 0;
            dwVersionLS = 0;

        }

        if (GetFirstChildTag(pCode, DU_TAG_SYSTEM, &pElemTmp) == S_OK) {
            bSystem = TRUE;
            SAFERELEASE(pElemTmp);
        } else {
            bSystem = FALSE;
        }

        // Check if object CLSID unit is currently installed
        // NOTE: This assumes MSICD

        // Never skip installing an object if this download is zero impact.  ALL files should be copied
        // to the zero impact directory.
        if(! IsZeroImpact())
        {
            HRESULT    hrExact;
            HRESULT    hrAny;

            hrAny = IsControlLocallyInstalled(szNativeName,
                                              (LPCLSID)&clsid, NULL,
                                              dwVersionMS, dwVersionLS,
                                              &lci, GetDestDirHint(),
                                              FALSE);

            if (m_bExactVersion) {
                hrExact = IsControlLocallyInstalled(szNativeName,
                                                    (LPCLSID)&clsid, NULL,
                                                    dwVersionMS, dwVersionLS,
                                                    &lci, GetDestDirHint(),
                                                    TRUE);
            }

            if (m_bExactVersion && hrExact == S_FALSE && hrAny == S_OK) {

                // Newer version exists on the machine.
                // Check if we are going to install outside of DPF
                // and disallow if we are going to downgrade.

                BOOL bIsDPFComponent = FALSE;
                CHAR szOCXCacheDirSFN[MAX_PATH];
                CHAR szFNameSFN[MAX_PATH];

                if (lci.szExistingFileName[0]) {

                    GetShortPathName(lci.szExistingFileName, szFNameSFN, MAX_PATH);
                    GetShortPathName(g_szOCXCacheDir, szOCXCacheDirSFN, MAX_PATH);

                    if (StrStrI(szFNameSFN, szOCXCacheDirSFN)) {
                        bIsDPFComponent = TRUE;
                    }
                }

                if (!bIsDPFComponent) {
                    // Trying to downgrade a system component. Just pretend
                    // system component is OK.
                    if (!IsEqualGUID(clsid, GetClsid())) {
                        if (lci.szExistingFileName[0]) {
                            hr = QueueModuleUsage(lci.szExistingFileName, MU_CLIENT);
                        }

                    }
                    goto nextNativeCode;
                }


            }
            // Else, we are a legacy case (non-sxs) or
            // hrExact == S_OK (therefore, hrAny == S_OK) or
            // hrAny == hrExact == S_FALSE (and we fall through).
            else {
                if (hrAny != S_FALSE) {
                    if (!IsEqualGUID(clsid, GetClsid())) {

                        if (lci.szExistingFileName[0]) {
                            hr = QueueModuleUsage(lci.szExistingFileName, MU_CLIENT);
                        }

                    }
                    goto nextNativeCode;
                }

            }

            // Disallow replacement of SFC files for Win2K

            if (g_bRunOnNT5) {
                LPWSTR                     wzFileName = NULL;
                BOOL                       bIsProtectedFile = FALSE;
                pfnSfcIsFileProtected      pfn = NULL;

                if (SUCCEEDED(::Ansi2Unicode(lci.szExistingFileName, &wzFileName))) {

                    if (!m_hModSFC) {
                        m_hModSFC = LoadLibrary("SFC.DLL");
                    }

                    if (m_hModSFC) {
                        pfn = (pfnSfcIsFileProtected)GetProcAddress(m_hModSFC, "SfcIsFileProtected");
                        if (pfn) {
                            bIsProtectedFile = (*pfn)(NULL,wzFileName);
                        }
                    }

                    SAFEDELETE(wzFileName);
                }

                if (bIsProtectedFile) {
                    // Fail out. Can't replace a system file.
                    hr = INET_E_CANNOT_REPLACE_SFP_FILE;
                    goto Exit;
                }
            }
        }

        // process CONFIG tag.
        nLast3 = -1;
        while (GetNextChildTag(pCode, DU_TAG_CONFIG, &pConfig, nLast3) == S_OK) {
            if (bDestroyPCBHList) {
                DestroyPCBHList(pcbhList);
                SAFEDELETE(pcbhList);
            }
            pcbhList = new CList<CCodeBaseHold *, CCodeBaseHold *>;
            if (pcbhList == NULL) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            bDestroyPCBHList = TRUE;
            pcbhList->RemoveAll();

            hr = ProcessImplementation(pConfig, pcbhList, m_lcid
#ifdef WX86
                                       , GetMultiArch()
#endif
                                      );

            SAFERELEASE(pConfig);

            if (FAILED(hr))
                break;

            if (hr == S_OK) {

                pBaseFileName = NULL;

                iCount = pcbhList->GetCount();
                if (iCount) {
                    pcbh = pcbhList->GetHead();
                    pcbh->dwFlags |= CBH_FLAGS_DOWNLOADED;
                }
                else {
                    pcbh = NULL;
                }

                if (pcbh) {

                    if (FAILED(hr = Unicode2Ansi(pcbh->wszCodeBase, &szCodeBase)))
                            break;

                    if (!pcbh->bHREF) {

                        // CODEBASE FILENAME= has precedence over NAME="" for file name.
                        // If FILENAME is CAB, then extract contents
                        //      with szNativeName=NAME, szCodeBase=thiscab
                        // otherwise
                        //      szNativeName=FILENAME, szCodeBase=thiscab, ignore NAME
                        FILEXTN extn = ::GetExtnAndBaseFileName(szCodeBase, &pBaseFileName);
                        if (extn == FILEXTN_CAB) {

                            ExtractInnerCAB(pdl, szCodeBase);

                        } else {

                            SAFEDELETE(szNativeName);
                            if (FAILED(hr = Unicode2Ansi(pcbh->wszCodeBase, &szNativeName)))
                                break;

                        }
                        SAFEDELETE(szCodeBase);

                        szCodeBase = new char[lstrlenA(szTHISCAB)+1];
                        if (!szCodeBase) {
                             hr = E_OUTOFMEMORY;
                             break;
                        }
                        lstrcpyA(szCodeBase, szTHISCAB);
                    }

                } else {

                    // No FILENAME field, szNativeName=NAME & szCodeBase=thiscab
                    szCodeBase = new char[lstrlenA(szTHISCAB)+1];
                    if (!szCodeBase) {
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    lstrcpyA(szCodeBase,szTHISCAB);

                }

                FILEXTN extn = ::GetExtnAndBaseFileName(szNativeName, &pBaseFileName);

                //BUGBUG: Should we limit ourselves to at most one INF file per OSD?
                if ((!pcbh || !pcbh->bHREF) && extn == FILEXTN_INF) {

                    // File is in temporary directory somewhere,  We extract Temp
                    if (!catDirAndFile(szTempFile, MAX_PATH, (char *)szTempDir, szNativeName)) {
                        hr = E_FAIL;
                        goto Exit;
                    }

                    hr = SetupInf(szTempFile, pBaseFileName, pdl);

                    if (SUCCEEDED(hr)) {
                        fSetupInf = TRUE;
                    }

                } else {

                    if (lci.IsPresent() && pCodeInstall) {

                        // a prev version exists. get permission to overwrite
                        // if ICodeInstall available
                        WCHAR szBuf[MAX_PATH];

                        MultiByteToWideChar(CP_ACP, 0,
                            (lci.szExistingFileName[0])?lci.szExistingFileName:szNativeName, -1, szBuf, MAX_PATH);

                        hr = pCodeInstall->OnCodeInstallProblem( CIP_OLDER_VERSION_EXISTS,
                            NULL, szBuf, 0);

                        if (FAILED(hr)) {

                            if (hr == E_ABORT)
                                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);

                            break;
                        }
                    }

                    //BUGBUG: Need a way to do this stuff in OSD
                    DESTINATION_DIR dest = LDID_OCXCACHE;

                    //DWORD dwRegisterServer = CST_FLAG_REGISTERSERVER;
                    // we can't force a register server here as this will
                    // mean as if we have an override in the INF/OSD
                    // whereas there is no support for this in OSD.
                    // turning this off here means:
                    // for EXE we will run if pointed to in the OSD or
                    // directly by codebase, but we will run with /regsvr
                    // and leave installed only if marked oleself register

                    // for an OCX unless overrideen we will alwys register
                    // if the DLL is registerable (has dllregisterserver entrypt

                    DWORD dwRegisterServer = 0;
                    DWORD dwCopyFlags = 0;

                    if (m_dwSystemComponent || bSystem) {
                        m_dwSystemComponent = TRUE;
                        dest = LDID_SYS;
                    }

                    hr = StartDownload(szNativeName, pdl, szCodeBase,
                                dest, lci.lpDestDir, dwRegisterServer, dwCopyFlags,
                                pcbhList);
                    bDestroyPCBHList = FALSE;
                }

                SAFEDELETE(szCodeBase);

                goto nextNativeCode;
            }

            if (FAILED(hr))
                break;

        }


        // here if anything in above loop failed or we never found an
        // implmentation matching our config

        if (SUCCEEDED(hr))
            hr = HRESULT_FROM_WIN32(ERROR_APP_WRONG_OS);

nextNativeCode:
        SAFERELEASE(pCode);
        SAFERELEASE(pConfig);

    }

Exit:
    SAFERELEASE(pCode);
    SAFERELEASE(pConfig);
    SAFEDELETE(szName);
    SAFEDELETE(szNativeName);
    SAFEDELETE(szCodeBase);

    if (SUCCEEDED(hr) && fSetupInf)
        hr = S_FALSE;

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::ParseOSD
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::ParseOSD(const char *szOSD, char *szOSDBaseName, CDownload *pdl)
{

    HRESULT hr = S_OK;
    IXMLElement *pSoftDist = NULL, *pDepend = NULL, *pJava = NULL,
                *pNativeCode = NULL, *pTitle = NULL, *pExpire = NULL,
                *pZeroImpact = NULL, *pSystemTag = NULL, *pSXS = NULL;
    LPSTR pBaseFileName = NULL, lpTmpDir = NULL;
    DWORD len = 0;
    int nLast, nLast2, nLast3;
    BOOL bSetupInf = FALSE;

    // create a CSetup OBJ and add it to the CDownload obj
    CSetup *pSetup = new CSetup(szOSD, szOSDBaseName, FILEXTN_OSD, NULL, &hr);
    if(!pSetup) {
        hr = E_OUTOFMEMORY;
    }
    if (FAILED(hr))
        goto Exit;

    pdl->AddSetupToList(pSetup);

    hr = SetManifest(FILEXTN_OSD, szOSD);
    if (FAILED(hr))
        goto Exit;

    hr = GetSoftDistFromOSD(szOSD, &pSoftDist);
    CHECK_ERROR_EXIT(SUCCEEDED(hr), ID_CDLDBG_FAILED_OSD_OM);

    hr = DupAttribute(pSoftDist, DU_ATTRIB_NAME, &m_szDistUnit);
    CHECK_ERROR_EXIT(SUCCEEDED(hr), ID_CDLDBG_DU_REQUIRED_ATTRIB_MISSING);

    hr = DupAttributeA(pSoftDist, DU_ATTRIB_VERSION, &m_szVersionInManifest);
    CHECK_ERROR_EXIT(SUCCEEDED(hr), ID_CDLDBG_DU_REQUIRED_ATTRIB_MISSING);


    // process TITLE display name
    if (GetFirstChildTag(pSoftDist, DU_TAG_TITLE, &pTitle) == S_OK) {

        BSTR bstrTitle = NULL;
        hr = pTitle->get_text(&bstrTitle);
        if (FAILED(hr)) {
            goto Exit;
        }

        if (FAILED(Unicode2Ansi(bstrTitle, &m_szDisplayName))) {
            hr = E_OUTOFMEMORY;
            SAFESYSFREESTRING(bstrTitle);
            goto Exit;
        }
        SAFESYSFREESTRING(bstrTitle);
    }

    // See if there is a SYSTEM tag

    if (GetFirstChildTag(pSoftDist, DU_TAG_SYSTEM, &pSystemTag) == S_OK) {
        SAFERELEASE(pSystemTag);
        m_dwSystemComponent = TRUE;
    }

    // process expire date
    if (GetFirstChildTag(pSoftDist, DU_TAG_EXPIRE, &pExpire) == S_OK) {

        BSTR bstrExpire = NULL;
        hr = pExpire->get_text(&bstrExpire);
        if (SUCCEEDED(hr)) {
            OLECHAR *pch = bstrExpire;

            m_dwExpire = 0;

            for ( ; *pch && m_dwExpire <= MAX_EXPIRE_DAYS; pch++ ) {
                if ( (*pch >= TEXT('0') && *pch <= TEXT('9')) )
                    m_dwExpire = m_dwExpire * 10 + *pch - TEXT('0');
                else
                    break;
            }

            if (m_dwExpire > MAX_EXPIRE_DAYS)
                m_dwExpire = MAX_EXPIRE_DAYS;
        }
        // else treat failure with a NOP

        SAFESYSFREESTRING(bstrExpire);
    }

    // Is this a zero impact install?
    // Only one osd is allowed per code download, so setting the ZeroImpactness
    // of this code download based on this one osd file is OK
    // This check should be made before any native code checks (code path change if zero impact)
    if(GetFirstChildTag(pSoftDist, DU_TAG_ZEROIMPACT, &pZeroImpact) == S_OK)
    {
        this->SetZeroImpact(TRUE);

        // If zero impact, the requested version is not needed, write over it with the
        // version in the manifest
        GetVersionFromString(m_szVersionInManifest, &m_dwFileVersionMS, &m_dwFileVersionLS);

        // BUGBUG: what if the page requests a version, but the manifest is different?
        // We should probably fail: (replace line above with)
        /* DWORD dwVersionMS, dwVersionLS;
        GetVersionFromString(m_szVersionInManifest, &dwVersionMS, &dwVersionLS);
        if(m_dwFileVersionMS != dwVersionMS || m_dwFileVersionLS != dwVersionLS)
        {
            hr = ??????;
            goto Exit;
        }
        */
    }

    if (!m_bExactVersion) {
        // Exact Version necessarily means uninstall old. Don't bother
        // looking it up.
        if (GetFirstChildTag(pSoftDist, DU_TAG_UNINSTALL_OLD, &pSXS) == S_OK) {
            if (!m_bIsZeroImpact) {
                // If both ZI/SxS defined, use ZI (less intrusive of the two).
                m_bUninstallOld = TRUE;
            }
        }
    }

    //REVIEW: optionally look for ABSTRACT

    //REVIEW: CONFIG tags at highest level are ignored.

    // process all DEPENDENCY tags (installing Distribution Units)
    nLast = -1;
    while (GetNextChildTag(pSoftDist, DU_TAG_DEPENDENCY, &pDepend, nLast) == S_OK) {

        hr = ProcessDependency(pdl, pDepend);
        SAFERELEASE(pDepend);
        if (FAILED(hr))
            goto Exit;

    }

    // process only one NATIVECODE tags (Installing ActiveX/CLSID specified controls)
    nLast = -1;
    if (GetNextChildTag(pSoftDist, DU_TAG_NATIVECODE, &pNativeCode, nLast) == S_OK) {

        hr = ProcessNativeCode(pdl, pNativeCode);
        SAFERELEASE(pNativeCode);
        if (FAILED(hr))
            goto Exit;

        if (hr == S_FALSE)
            bSetupInf = TRUE;
    }

    // process JAVA tags (Installing Java packages)
    nLast = -1;
    while (GetNextChildTag(pSoftDist, DU_TAG_JAVA, &pJava, nLast) == S_OK) {

        //BUGBUG: Parameters szOSD, szOSDBaseName are currently unused.
        hr = ProcessJavaManifest(pJava, szOSD, szOSDBaseName, pdl);
        SAFERELEASE(pJava);
        if (FAILED(hr))
            goto Exit;
    }


Exit:

    if (!bSetupInf) {

        if (SUCCEEDED(hr)) {

            pdl->SetDLState(DLSTATE_READY_TO_SETUP);

        } else {

            // we encountered an error, go to done state.

            pdl->SetDLState(DLSTATE_DONE);
        }
    }

    SAFERELEASE(pJava);
    SAFERELEASE(pTitle);
    SAFERELEASE(pExpire);
    SAFERELEASE(pNativeCode);
    SAFERELEASE(pDepend);
    SAFERELEASE(pSoftDist);
    SAFERELEASE(pZeroImpact);
    SAFERELEASE(pSXS);

    return hr;
}


// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::AddDistUnitList
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::AddDistUnitList(LPWSTR szDistUnit)
{
    HRESULT hr = E_FAIL;
    LPWSTR wszDistUnit = 0;

    hr = CDLDupWStr(&wszDistUnit, szDistUnit);
    if (SUCCEEDED(hr) && wszDistUnit) {

        m_pDependencies.AddHead(wszDistUnit);

    }

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::SetupInf
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::SetupInf(const char *szInf, char *szInfBaseName, CDownload *pdl)
{
    HRESULT hr = S_OK;
    CSetup* pSetup = NULL;
    int nBuffSize = MAX_INF_SECTIONS_SIZE;
    char lpSections[MAX_INF_SECTIONS_SIZE];
    const static char *szAddCodeSection = "Add.Code";
    const static char *szHooksSection = "Setup Hooks";
    const static char *szUninstallOld = "UninstallOld";
    static char *szDefault = "";
    DWORD len;

    SetHaveInf();

    if (!pdl->HasAllActiveXPermissions()) {

        if (IsSilentMode())
        {
            SetBitsInCache();
        } else {

            hr = TRUST_E_FAIL;
            goto Exit;
        }
    }

    pdl->SetDLState(DLSTATE_INF_PROCESSING);

    Assert(m_szInf == NULL);

    m_szInf = new char [lstrlen(szInf)+1];

    if (!m_szInf) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    lstrcpy(m_szInf, szInf);

    // Add a setup obj for this INF file
    // We keep the INF file in the ocxcache dir
    // to be able to nuke the OCX

    // create a CSetup OBJ and add it to the CDownload obj
    pSetup = new CSetup(szInf, szInfBaseName, FILEXTN_INF, NULL, &hr);
    if(!pSetup) {
        hr = E_OUTOFMEMORY;
    }
    if (FAILED(hr))
        goto Exit;

    pdl->AddSetupToList(pSetup);

    len = GetPrivateProfileString(szAddCodeSection, NULL, szDefault,
                                                lpSections, nBuffSize, m_szInf);

    if (!len) {

        // no Internet Code Downloader known sections in INF may be a
        // regular Win32 INF file format, make a hook if the
        // INF came in a CAB, which will be to extract all files in the
        // current CAB and then RunSetupCommand

        // there's no [add.code]
        // look to see if there's a [setup hooks]
        // if not we then create a hook to process the default install section
        // if there's a [setup hooks] we won't make a default hokk for you
        // as you can make a hook yourself to process default install
        // the idea is you either don't know about us (we need to help you)
        // or you are code downloader aware (help yourself with our capabilty)

        // this allows the user to have an INF with any or all of the following
        // 1) [add.code]
        // 2) [Setup hooks]
        // 3) win32 inf : defaultinstall

        len = GetPrivateProfileString(szHooksSection, NULL, szDefault,
                                                lpSections, nBuffSize, m_szInf);
        if (!len) {

            // make a new hook and add it to this CAB
            // post a message to trigger setup phase as nothing else is needed

            hr = pdl->AddHook(NULL, szInfBaseName, NULL/* szInfSection */, RSC_FLAG_INF);

            goto Exit;
        }


    } else {

        m_pCurCode = m_pAddCodeSection = new char [len + 1];

        memcpy(m_pAddCodeSection, lpSections, len);
        m_pAddCodeSection[len] = '\0';
    }

    if (!m_bExactVersion) {
        m_bUninstallOld=GetPrivateProfileInt(szAddCodeSection, szUninstallOld, 0, m_szInf);
    }

Exit:

    if (SUCCEEDED(hr)) {

        CCDLPacket *pPkt= new CCDLPacket(CODE_DOWNLOAD_PROCESS_INF, this, (DWORD_PTR)pdl);

        if (pPkt) {
            hr = pPkt->Post();
        } else {
            hr = E_OUTOFMEMORY;
        }

    }

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::IsSectionInINF
// Checks if a section is in the INF
// returns:
//      S_OK: lpCurCode has the satellite binary name
//      S_FALSE: ignore this code and use default resources in main dll
//      E_XXX: any other error
BOOL
CCodeDownload::IsSectionInINF(
    LPCSTR lpCurCode)
{
    const char *szDefault = "";
    DWORD len;
#define FAKE_BUF_SIZE   3
    char szBuf[FAKE_BUF_SIZE];

    len = GetPrivateProfileString(lpCurCode, NULL, szDefault,
                                                szBuf, FAKE_BUF_SIZE, m_szInf);

    if (len == (FAKE_BUF_SIZE - 2)) {   // returns Out Of Buffer Space?
        // yes, section found
        return TRUE;
    } else {
        return FALSE;
    }
}



void CCodeDownload::CodeDownloadDebugOut(int iOption, BOOL fOperationFailed,
                                         UINT iResId, ...)
{

    static char             szDebugString[INTERNET_MAX_URL_LENGTH*5];
    static char             szFormatString[MAX_DEBUG_FORMAT_STRING_LENGTH];
    va_list                 args;

    LoadString(g_hInst, iResId, szFormatString, MAX_DEBUG_FORMAT_STRING_LENGTH);
    va_start(args, iResId);
    vsprintf(szDebugString, szFormatString, args);
    va_end(args);

    m_debuglog->DebugOutPreFormatted(iOption, fOperationFailed, szDebugString);

}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::GetSatelliteName
// gets the lang specific satellite DLL name
// in the INF
// returns:
//      S_OK: lpCurCode has the satellite binary name
//      S_FALSE: ignore this code and use default resources in main dll
//      E_XXX: any other error
HRESULT
CCodeDownload::GetSatelliteName(
    LPSTR lpCurCode)
{
    HRESULT hr = S_OK;
    const char *szDefault = "";
    DWORD len;
#define FAKE_BUF_SIZE   3
    char szBuf[FAKE_BUF_SIZE];
    char szExtension[5];
    int iReturn = 0;

    Assert(lpCurCode);

    szExtension[0] = *lpCurCode = '\0';



    // get a quick out for code that does not have any vars in them
    if ((StrChr(m_pCurCode, '%') == NULL) &&
        IsSectionInINF(m_pCurCode)) {

        // not a satellite
        lstrcpy(lpCurCode, m_pCurCode);
        m_debuglog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_ITEM_PROCESSED, lpCurCode);
        return hr;
    }

    // allow IE3 workarounds for %LANG%
    // by looking  for sections that have a %LANG% literally in them
    // after looking for sections with the variable expanded

    // BEGIN NOTE: add vars and values in matching order
    // add a var by adding a new define VAR_NEW_VAR = NUM_VARS++
    const char *szVars[] = {

#define VAR_LANG        0       // 3 letter lang extension
        "%LANG%",


#define NUM_VARS            1

        ""
    };

    const char *szValues[NUM_VARS + 1];
    szValues[VAR_LANG] = szExtension;
    szValues[NUM_VARS] = NULL;
    // END NOTE: add vars and values in matching order


    UINT uLocaleTest=0;
    uLocaleTest = (LOWORD(m_lcid) & (~(~0 << 4) << 0)) >> 0;

    // obtain the 3 character Lang abbreviation for the
    // LCID we're running on.
    // if it doesn't exist we'll get just the 2 charact Lang abbreviation
    // and try again, failing that we default to English

    iReturn = m_langinfo.GetLocaleStrings(m_lcid, szExtension);

    if (!iReturn) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        m_debuglog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_ERR_PRIMARY_LANGUAGE, hr, lpCurCode);
        goto Exit;
    }

    // expand the variables names if any
    hr = CSetupHook::ExpandCommandLine(m_pCurCode, lpCurCode, MAX_PATH, szVars, szValues);

    if (FAILED(hr)) { // failed
        m_debuglog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_ERR_NO_SECTION, m_pCurCode, szExtension);
        goto Exit;
    }

    // vars are expanded correctly (S_OK) or
    // no vars got expanded.(S_FALSE) maybe we could try the section as is
    if ( IsSectionInINF(lpCurCode)) {
        // satellite found!
        m_debuglog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_SATELLITE_FOUND, lpCurCode);
        hr = S_OK;
        goto Exit;
    }


    // we couldn't find it with the entire LCID, try it with just the primary
    // langid

    LCID lcid;
    lcid = MAKELCID(MAKELANGID(PRIMARYLANGID(LANGIDFROMLCID(m_lcid)), SUBLANG_DEFAULT), SORT_DEFAULT);

    iReturn = m_langinfo.GetLocaleStrings(lcid, szExtension);

    if (!iReturn) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        m_debuglog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_PROCESSINF_FAILED, hr, lpCurCode);
        goto Exit;
    }

    // expand the variables names with new value
    hr = CSetupHook::ExpandCommandLine(m_pCurCode, lpCurCode, MAX_PATH, szVars, szValues);

    if (FAILED(hr) || (hr == S_FALSE))  { // failed or no vars
        m_debuglog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_ERR_NO_SECTION, m_pCurCode, szExtension);
        if (hr == S_FALSE)
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Exit;
    }

    // try the INF section again
    if ( !IsSectionInINF(lpCurCode)) {
        m_debuglog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_ERR_NO_SECTION, m_pCurCode, szExtension);

        // no section for this language. This is OK skip the file
        // browser will end up using default lang/resources
        hr = S_FALSE;

    } else {

        // satellite found!
        m_debuglog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_SATELLITE_FOUND, lpCurCode);

    }

Exit:

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::GetInfCodeLocation
// gets the platform specific or independent location URL of the code specified
// in the INF
// returns:
//      S_OK: szURL has the location
//      S_FALSE: ignore this code for the current platform
//      E_XXX: any other error
HRESULT
CCodeDownload::GetInfCodeLocation(
    LPCSTR lpCurCode,
    LPSTR szURL)
{
    const static char *szLoc = "File";
    static char *szDefault = "";
    HRESULT hr = S_OK;

    Assert(m_szInf);

    szURL[0] = '\0';            // init to empty string

    // look for platform specific URL first
    // this is needed to skip some files for some
    // platforms
#ifdef WX86
    char *szPreferredArch;
    char *szAlternateArch;
    HRESULT hrArch;

    GetMultiArch()->SelectArchitecturePreferences(
                g_szPlatform,
                "file-win32-x86",
                &szPreferredArch,
                &szAlternateArch);

    GetPrivateProfileString(lpCurCode, szPreferredArch, szDefault, szURL,
        INTERNET_MAX_URL_LENGTH, m_szInf);
    if (szURL[0] != '\0' && lstrcmpi(szURL, szIGNORE) != 0) {
        // There was a URL and it was not 'ignore' to indicate it is not
        // applicable to this platform.
        CodeDownloadDebugOut(DEB_CODEDL, FALSE, ID_CDLDBG_WX86_REQUIRE_PRIMARY_ARCH, szURL);
        hrArch = GetMultiArch()->RequirePrimaryArch();
        Assert(SUCCEEDED(hrArch));
    } else if (szAlternateArch) {
        GetPrivateProfileString(lpCurCode, szAlternateArch, szDefault, szURL,
            INTERNET_MAX_URL_LENGTH, m_szInf);
        if (szURL[0]) {
            if (lstrcmpi(szURL, szIGNORE) != 0) {
                // The alternate architecture matched and the URL was not
                // 'ignore' to indicate it is not applicable to this platform.
                CodeDownloadDebugOut(DEB_CODEDL, FALSE, ID_CDLDBG_WX86_REQUIRE_ALTERNATE_ARCH, szURL);
                hrArch = GetMultiArch()->RequireAlternateArch();
                Assert(SUCCEEDED(hrArch));
            }
        }
    }
#else
    GetPrivateProfileString(lpCurCode, g_szPlatform, szDefault, szURL,
        INTERNET_MAX_URL_LENGTH, m_szInf);
#endif

    if (szURL[0] == '\0') {
        GetPrivateProfileString(lpCurCode, szLoc, szDefault,
                        szURL, INTERNET_MAX_URL_LENGTH, m_szInf);
    } else {
        // got a platform specific URL
        // look for 'ignore' keyword to mean that this is
        // not applicable for this platform

        if (lstrcmpi(szURL, szIGNORE) == 0) {
            hr = S_FALSE;
        }
    }

    return hr;

}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::GetInfSectionInfo
HRESULT
CCodeDownload::GetInfSectionInfo(
    LPSTR lpCurCode,
    LPSTR szURL,
    LPCLSID *plpClsid,
    LPDWORD pdwFileVersionMS,
    LPDWORD pdwFileVersionLS,
    DESTINATION_DIR *pdest,
    LPDWORD pdwRegisterServer,
    LPDWORD pdwCopyFlags,
    BOOL *pbDestDir
)
{
    const static char *szFileVersion = "FileVersion";
    const static char *szDest = "DestDir";
    const static char *szRegisterServerOverride = "RegisterServer";
    const static char *szCopyFlags = "CopyFlags";
    const static char *szForceDestDir = "ForceDestDir";
    static char *szDefault = "";
    DWORD len;
    HRESULT hr = S_OK;
    char szBuf[MAX_PATH];

    hr = GetSatelliteName(lpCurCode);

    if (hr != S_OK)
        goto Exit;

    hr =  GetInfCodeLocation( lpCurCode, szURL);

    if (hr != S_OK)
        goto Exit;

    // get RegisterServerOverride if any
    if (GetPrivateProfileString(lpCurCode, szRegisterServerOverride, szDefault,
                                    szBuf, MAX_PATH, m_szInf)) {

        *pdwRegisterServer = CST_FLAG_REGISTERSERVER_OVERRIDE;

        if ((szBuf[0] == 'y') || (szBuf[0] == 'Y') ||
             (szBuf[0] == '1') || (lstrcmpi(szBuf, "true") == 0)) {
            *pdwRegisterServer |= CST_FLAG_REGISTERSERVER;
        }
    }

    // get CopyFlags if any
    *pdwCopyFlags=GetPrivateProfileInt(lpCurCode, szCopyFlags, 0, m_szInf);

    // get version string
    if (!(len =GetPrivateProfileString(lpCurCode, szFileVersion, szDefault,
                                                szBuf, MAX_PATH, m_szInf))) {
        // if no version specified, local copy is always OK!
        szBuf[0] = '\0';
    }


    if ( FAILED(GetVersionFromString(szBuf, pdwFileVersionMS,
                                                        pdwFileVersionLS))){
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // get Destination dir if suggested
    *pdest=(DESTINATION_DIR)GetPrivateProfileInt(lpCurCode, szDest, 0, m_szInf);

    // get ForceDestDir flag
    *pbDestDir=GetPrivateProfileInt(lpCurCode, szForceDestDir, 0, m_szInf);

    // get clsid string
    if (!(len = GetPrivateProfileString(lpCurCode, szCLSID, szDefault,
                                                szBuf, MAX_PATH, m_szInf))){
        // if no clsid specified, not a control, just a plain dll?
        *plpClsid = NULL;
        goto Exit;
    }

    // Get CLSID from string
    hr = ConvertANSItoCLSID(szBuf, *plpClsid);

Exit:

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::StartDownload
HRESULT
CCodeDownload::StartDownload(
    LPSTR szCurCode,
    CDownload *pdl,
    LPSTR szURL,
    DESTINATION_DIR dest,
    LPSTR szDestDir,
    DWORD dwRegisterServer,
    DWORD dwCopyFlags,
    CList<CCodeBaseHold *, CCodeBaseHold *> *pcbhList)
{
    FILEXTN extn;
    char *pBaseFileName;
    HRESULT hr = NO_ERROR;
    WCHAR szBuf[INTERNET_MAX_URL_LENGTH];
    CDownload *pdlCur = NULL;
    CSetup* pSetup = NULL;
    BOOL bDestroyList = TRUE;


    extn = ::GetExtnAndBaseFileName(szURL, &pBaseFileName);

    // if this INF came in a CAB then anything pointing to
    // file=thiscab, means this szCurCode can be found in the CAB
    // that this INF came in. This makes authoring the INFs easy
    // because you don't have to know the name of the site that's going
    // to distribute the OCX.
    // Also, allows for web publisher to change the name of the
    // CAB
    if ((pdl->GetExtn() == FILEXTN_CAB) &&
        (lstrcmpi(szTHISCAB, szURL) == 0)) {
        pdl->AddSetupToExistingCAB(szCurCode, szDestDir, dest,
            dwRegisterServer, dwCopyFlags);
        goto Exit;
    }


    switch (extn) {

    case FILEXTN_INF:
    case FILEXTN_OSD:
        hr = E_INVALIDARG; // don't supp multiple INFs (recursive downloads)
        goto Exit;

    case FILEXTN_CAB:

        // check if URL is a cab that the inf came with (pdl->psess)
        // else check if CAB has been submitted for download in some other
        // CDownload that we just started when processing lines in INF
        // above this one
        // either case if you find a CAB then piggy back this code setup to
        // that CDownload of the same CAB file

        MultiByteToWideChar(CP_ACP, 0, szURL, -1, szBuf,
            INTERNET_MAX_URL_LENGTH);

        hr = FindCABInDownloadList(szBuf, pdl, &pdlCur);

        if (FAILED(hr))
            goto Exit;

        if (pdlCur) {

            // matching CAB found that we can pile on this setup
            pdlCur->AddSetupToExistingCAB(szCurCode, szDestDir, dest, dwRegisterServer, dwCopyFlags);
            goto Exit;
        }

        // fresh CAB needs to get pulled down.
        // download the CODE=URL (ie. CAB or INF file first)
        pdlCur = new CDownload(szBuf, extn, &hr);
        if (!pdlCur) {
            hr = E_OUTOFMEMORY;
        }

        if (FAILED(hr)) {
            SAFEDELETE(pdlCur);
            goto Exit;
        }

        AddDownloadToList(pdlCur);

        {
        BOOL bSetOnStack = SetOnStack();
        hr = (pcbhList == NULL) ? (pdlCur->DoDownload(&m_pmkContext,
                                                     (BINDF_ASYNCHRONOUS|
                                                     BINDF_ASYNCSTORAGE)))
                                : (pdlCur->DoDownload(&m_pmkContext,
                                                     (BINDF_ASYNCHRONOUS|
                                                     BINDF_ASYNCSTORAGE),
                                                     pcbhList));
        bDestroyList = FALSE;

        if (bSetOnStack)
            ResetOnStack();
        }

        if (FAILED(hr)) {
            goto Exit;
        }

        pdlCur->AddSetupToExistingCAB(szCurCode, szDestDir, dest, dwRegisterServer, dwCopyFlags);

        break;


    case FILEXTN_EXE:
    case FILEXTN_OCX:
    case FILEXTN_DLL:
    case FILEXTN_NONE:
    case FILEXTN_UNKNOWN:

        MultiByteToWideChar(CP_ACP, 0, szURL, -1, szBuf,
            INTERNET_MAX_URL_LENGTH);

        // download the CODE=URL (ie. CAB or INF file first)
        pdlCur = new CDownload(szBuf, extn, &hr);

        if (!pdlCur){
            hr = E_OUTOFMEMORY;
        }

        if (FAILED(hr))
            goto Exit;

        AddDownloadToList(pdlCur);

        // create a CSetup OBJ and add it to the CDownload obj
        pSetup = new CSetup(NULL, szCurCode, extn, szDestDir, &hr,dest);


        if(!pSetup) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        } else if (FAILED(hr)) {
            delete pSetup;
            goto Exit;
        }

        pSetup->SetCopyFlags (dwCopyFlags);

        if (dwRegisterServer) {
            pSetup->SetUserOverrideRegisterServer
                (dwRegisterServer&CST_FLAG_REGISTERSERVER);
        }

        pdlCur->AddSetupToList(pSetup);

        {
        BOOL bSetOnStack = SetOnStack();
        hr = (pcbhList == NULL) ? (pdlCur->DoDownload(&m_pmkContext,
                                                     (BINDF_ASYNCHRONOUS|
                                                     BINDF_ASYNCSTORAGE)))
                                : (pdlCur->DoDownload(&m_pmkContext,
                                                     (BINDF_ASYNCHRONOUS|
                                                     BINDF_ASYNCSTORAGE),
                                                     pcbhList));
        bDestroyList = FALSE;


        if (bSetOnStack)
            ResetOnStack();
        }

        if (FAILED(hr)) {
            goto Exit;
        }

    }

Exit:
    if (bDestroyList && pcbhList) {
        DestroyPCBHList(pcbhList);
        SAFEDELETE(pcbhList);
    }

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::ProcessInf

/*
 *sample INF file:
;This is the INF file for CIRC3.OCX
[Add.Code]
circ3.ocx=circ3.ocx
random.dll=random.dll
mfc40.dll=mfc40.dll
foo.ocx=foo.ocx

[circ3.ocx]
file=http:\\ohserv\users\vatsanp\circ3.cab
clsid={9DBAFCCF-592F-101B-85CE-00608CEC297B}
FileVersion=1,0,0,143

[random.dll]
file=http://ohserv/users/vatsanp/random.dll
FileVersion=
;DestDir = 10 or 11 ( LDID_WIN or LDID_SYS by INF convention)
; if none specified installed in ocxcache directory, which is the typical case.
DestDir=10

[mfc40.dll]
; way of saying I need mfc40 (version 4,0,0,5) but, I can't provide it
; if absent on client fail load!
file=
FileVersion=4,0,0,5

[foo.ocx]
; way of saying I need foo (clsid, version 4,0,0,5) but, I can't provide it
; if absent on client fail load!
file=
clsid={DEADBEEF-592F-101B-85CE-00608CEC297B}
FileVersion=1,0,0,143

*/

//
//    We walk thru all the INF sections of code that needs to get installed.
//    For each we get the CLSID, FileVersion (both optional) and URL to get from
//  Depending on the extension of the URL we:
//
//    CAB:
//        if CAB is the one the INF came with
//            extract file; create CSetup to install it (piggy back to pdl)
//        else if some other CAB that has been set for download
//                attach file to be extracted to pFilesToExtract
//                attach a CSetup for this file
//        else
//            make a CDownload for this new CAB
//            attach file to be extracted to pFilesToExtract
//            attach a CSetup for this file
//            start off download
//    INF:
//        Fail: don't support multiple INFs
//
//    Anything else:
//        Make a new CDownload for this
//        start off download
//        make CSetup
//
// ---------------------------------------------------------------------------
VOID
CCodeDownload::ProcessInf(CDownload *pdl)
{
    char szURL[INTERNET_MAX_URL_LENGTH];
    static char *szDefault = "";
    const static char *szHOOK = "Hook";
    char szCurCode[MAX_PATH];

    DWORD len = 0;
    FILEXTN extn;
    DESTINATION_DIR dest;

    DWORD dwFileVersionMS = 0;
    DWORD dwFileVersionLS = 0;
    CLSID clsid;
    LPCLSID lpclsid = &clsid;

    HRESULT hr = NO_ERROR;
    char * pFileName = NULL;

    DWORD dwRegisterServer = 0;
    DWORD dwCopyFlags = 0;
    BOOL  bForceDestDir = FALSE;

    CLocalComponentInfo lci;
    ICodeInstall* pCodeInstall = GetICodeInstall();


    if ( pdl->GetDLState() == DLSTATE_ABORT) {
        hr = E_ABORT;
        goto PI_Exit;           // all done
    }

    if (!m_pCurCode || !(*m_pCurCode)) {
        goto PI_Exit;           // all done
    }

    hr = GetInfSectionInfo( szCurCode, szURL, &lpclsid,
        &dwFileVersionMS, &dwFileVersionLS, &dest, &dwRegisterServer, &dwCopyFlags,
        &bForceDestDir
        );

    if (hr != S_OK)
        goto PI_Exit;

    HRESULT    hrExact;
    HRESULT    hrAny;

    if (m_bExactVersion) {
        hrExact = IsControlLocallyInstalled(szCurCode,
                                            lpclsid, NULL,
                                            dwFileVersionMS, dwFileVersionLS,
                                            &lci, GetDestDirHint(),
                                            TRUE);
    }

    hrAny = IsControlLocallyInstalled(szCurCode,
                                      lpclsid, NULL,
                                      dwFileVersionMS, dwFileVersionLS,
                                      &lci, GetDestDirHint(),
                                      FALSE);

    if (m_bExactVersion && hrExact == S_FALSE && hrAny == S_OK) {

        // Newer version exists on the machine.
        // Check if we are going to install outside of DPF
        // and disallow if we are going to downgrade.

        BOOL bIsDPFComponent = FALSE;
        CHAR szOCXCacheDirSFN[MAX_PATH];
        CHAR szFNameSFN[MAX_PATH];

        GetShortPathName(lci.szExistingFileName, szFNameSFN, MAX_PATH);
        GetShortPathName(g_szOCXCacheDir, szOCXCacheDirSFN, MAX_PATH);

        if (StrStrI(szFNameSFN, szOCXCacheDirSFN)) {
            bIsDPFComponent = TRUE;
        }

        if (!bIsDPFComponent) {
            // Trying to downgrade a system component. Just pretend
            // system component is OK.
            if (lpclsid && IsEqualGUID(clsid, GetClsid())) {
                goto PI_Exit;
            }

            if (lci.szExistingFileName[0])
                hr= QueueModuleUsage(lci.szExistingFileName, MU_CLIENT);

            goto PI_Exit;
        }


    }
    // Else, we are a legacy case (non-sxs) or
    // hrExact == S_OK (therefore, hrAny == S_OK) or
    // hrAny == hrExact == S_FALSE (and we fall through).
    else {
        if (hrAny == S_OK) {

            // make sure we have a ref count for the code downloader in
            // shareddlls as well as mark us as a client in the usage section
            // we need to do this only for a dependency, not for the main
            // ocx. We can always get the main OCX back with CODEBASE. Its
            // only if the dependency gets removed are we somewhat busted.
            // Keep the registry small and simple.

            if (lpclsid && IsEqualGUID(clsid, GetClsid())) {
                goto PI_Exit;
            }

            if (lci.szExistingFileName[0])
                hr= QueueModuleUsage(lci.szExistingFileName, MU_CLIENT);

            goto PI_Exit;
        }

    }

    if (g_bRunOnNT5 && !(bForceDestDir && dest == LDID_OCXCACHE)) {
        LPWSTR                     wzFileName = NULL;
        BOOL                       bIsProtectedFile = FALSE;
        pfnSfcIsFileProtected      pfn = NULL;

        if (SUCCEEDED(::Ansi2Unicode(lci.szExistingFileName, &wzFileName))) {

            if (!m_hModSFC) {
                m_hModSFC = LoadLibrary("SFC.DLL");
            }

            if (m_hModSFC) {
                pfn = (pfnSfcIsFileProtected)GetProcAddress(m_hModSFC, "SfcIsFileProtected");
                if (pfn) {
                    bIsProtectedFile = (*pfn)(NULL,wzFileName);
                }
            }

            SAFEDELETE(wzFileName);
        }

        if (bIsProtectedFile) {
            // Fail out. Can't replace a system file.
            hr = INET_E_CANNOT_REPLACE_SFP_FILE;
            goto PI_Exit;
        }
    }

    if (szURL[0] == '\0') {

        // if not file/location is available then look to see if a
        // hook is available to download/install this component.

        if (GetPrivateProfileString(szCurCode, szHOOK, szDefault,
                                        szURL, MAX_PATH, m_szInf)) {

            hr = ProcessHookSection(szURL /* hook section */, pdl);
            goto PI_Exit;

        }

        // this is a way someone can say I need this file (clsid, version)
        // to run, if absent just fail the load!
        hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
        goto PI_Exit;
    }

    if (lci.IsPresent() && pCodeInstall) {

        // a prev ver exists. get permission to overwrite
        // if ICodeInstall available
        WCHAR szBuf[MAX_PATH];

        MultiByteToWideChar(CP_ACP, 0,
            (lci.szExistingFileName[0])?lci.szExistingFileName:szCurCode, -1, szBuf, MAX_PATH);

        hr = pCodeInstall->OnCodeInstallProblem( CIP_OLDER_VERSION_EXISTS,
                    NULL, szBuf, 0);

        // hr == E_ABORT: abort whole download
        if (FAILED(hr)) {

            if (hr == E_ABORT)
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);

            // else preserve error code of OnCodeInstallProblem

            goto PI_Exit;
        }

    }

    hr = StartDownload( szCurCode, pdl, szURL,
        dest, ((bForceDestDir) ? (NULL) : (lci.lpDestDir)), dwRegisterServer, dwCopyFlags);


PI_Exit:

    if (SUCCEEDED(hr)) {

        if (m_pCurCode)
            len = lstrlen(m_pCurCode);
        else
            len = 0;


        if (len) {

            m_pCurCode += (len+1); // next

            // skip side by side
            while (!StrCmpI(m_pCurCode, INF_TAG_UNINSTALL_OLD)) {
                len = lstrlen(m_pCurCode);
                m_pCurCode += (len+1);
            }

            if (*m_pCurCode) {

                CCDLPacket *pPkt= new CCDLPacket(CODE_DOWNLOAD_PROCESS_INF,
                    this, (DWORD_PTR)pdl);

                if (pPkt) {
                    hr = pPkt->Post();
                } else {
                    hr = E_OUTOFMEMORY;
                }

                if (SUCCEEDED(hr))
                    goto Exit;
            }

        }

        hr = ProcessHooks(pdl);

    }

    if (FAILED(hr)) {
        m_debuglog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_PROCESSINF_FAILED,
                             hr, (m_pCurCode && *m_pCurCode)?m_pCurCode:"Setup Hooks");

        // done with this CDownload. Mark it ready for setup
        pdl->SetDLState(DLSTATE_DONE);

    } else {

        // done with this CDownload. Mark it ready for setup
        pdl->SetDLState(DLSTATE_READY_TO_SETUP);
    }


    pdl->CompleteSignal(hr, S_OK, S_OK, NULL);

Exit:

    return;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::QueueModuleUsage
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::QueueModuleUsage(
    LPCSTR szFileName,
    LONG muFlags)
{
    HRESULT hr = S_OK;

    CModuleUsage *pModuleUsage = new CModuleUsage(szFileName, muFlags, &hr);

    if (!pModuleUsage) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    } else if (FAILED(hr)) {
        delete pModuleUsage;
        goto Exit;
    }

    m_ModuleUsage.AddTail(pModuleUsage);

Exit:

    return hr;
}


// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::UpdateModuleUsage
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::UpdateModuleUsage()
{
    HRESULT hr = S_OK;
    char *lpClientName = NULL;
    LPOLESTR pwcsClsid = (LPOLESTR)GetMainDistUnit();
    LISTPOSITION curpos;
    int i, iNumClients;
    CLSID myclsid;

    Assert(pwcsClsid);

    if (FAILED((hr=::Unicode2Ansi(pwcsClsid, &lpClientName))))
    {
        goto Exit;
    }

    curpos = m_ModuleUsage.GetHeadPosition();
    iNumClients = m_ModuleUsage.GetCount();
    for (i=0; i < iNumClients; i++) {

        (m_ModuleUsage.GetNext(curpos))->Update(lpClientName);
    }


Exit:

    if (pwcsClsid && (pwcsClsid != GetMainDistUnit()) )
        delete pwcsClsid;

    if (lpClientName)
        delete lpClientName;

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::ProcessHookSection
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::ProcessHookSection(LPCSTR lpCurHook, CDownload *pdl)
{
    HRESULT hr = S_OK;
    char szURL[INTERNET_MAX_URL_LENGTH];
    WCHAR szBuf[INTERNET_MAX_URL_LENGTH];
    char szCmdLine[1024];
    char szInfSection[MAX_PATH];
    const static char *szINFNAME = "InfFile";
    const static char *szINFSECTION = "InfSection";
    const static char *szCMDLINE = "Run";
    static char *szDefault = "";
    DWORD flags = 0;
    CDownload *pdlCur = pdl;
    char *pBaseFileName = NULL;

    // Get cmd line for hook if any
    szCmdLine[0] = '\0';
    GetPrivateProfileString(lpCurHook, szCMDLINE, szDefault,
                                    szCmdLine, MAX_PATH, m_szInf);

    if (!szCmdLine[0]) {
        flags |= RSC_FLAG_INF;

        // Get Inf filename if any
        GetPrivateProfileString(lpCurHook, szINFNAME, szDefault,
                                    szCmdLine, MAX_PATH, m_szInf);

        // Get Inf section name if any
        szInfSection[0] = '\0';
        GetPrivateProfileString(lpCurHook, szINFSECTION, szDefault,
                                        szInfSection, MAX_PATH, m_szInf);
    }

    hr = GetInfCodeLocation(lpCurHook, szURL);

    if (hr != S_OK)
        goto Exit;

    if (szURL[0]) {

        MultiByteToWideChar(CP_ACP, 0, szURL, -1, szBuf,
            INTERNET_MAX_URL_LENGTH);

        pdlCur = NULL;
        hr = FindCABInDownloadList(szBuf, pdl, &pdlCur);

        if (FAILED(hr))
            goto Exit;

        if (!pdlCur) {

            // did not find CAB
            // fresh CAB needs to get pulled down.

            FILEXTN extn = ::GetExtnAndBaseFileName(szURL, &pBaseFileName);

            if (extn != FILEXTN_CAB) {
                hr = E_INVALIDARG;
                goto Exit;
            }

            pdlCur = new CDownload(szBuf, extn, &hr);
            if (!pdlCur) {
                hr = E_OUTOFMEMORY;
            }

            if (FAILED(hr))
                goto Exit;


            AddDownloadToList(pdlCur);

            {
            BOOL bSetOnStack = SetOnStack();
            hr = pdlCur->DoDownload(&m_pmkContext,
                        (BINDF_ASYNCHRONOUS| BINDF_ASYNCSTORAGE));

            if (bSetOnStack)
                ResetOnStack();
            }

            if (FAILED(hr)) {
                goto Exit;
            }

        }
    }

    if ( !szCmdLine[0] )
        ::GetExtnAndBaseFileName(m_szInf, &pBaseFileName);

    Assert(pdlCur);

    hr = pdlCur->AddHook(lpCurHook, (szCmdLine[0])?szCmdLine:pBaseFileName,
        (szInfSection[0])?szInfSection:NULL,
        flags);

Exit:

    return hr;

}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::ProcessHooks
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::ProcessHooks(CDownload *pdl)
{
    HRESULT hr = S_OK;
    int nBuffSize = MAX_INF_SECTIONS_SIZE;
    char lpSections[MAX_INF_SECTIONS_SIZE];
    const static char *szHooksSection = "Setup Hooks";
    static char *szDefault = "";
    char *lpCurHook = NULL;
    DWORD len;

    len = GetPrivateProfileString(szHooksSection, NULL, szDefault,
                                                lpSections, nBuffSize, m_szInf);

    if (len) {

        for (lpCurHook =lpSections;*lpCurHook;
            lpCurHook+= (lstrlen(lpCurHook)+1)) {

            hr = ProcessHookSection(lpCurHook, pdl);

            if (FAILED(hr))
                break;
        }

    } else {
        hr = S_FALSE; // no hooks!
    }

    return hr;

}


// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::Complete
// CCodeDownload::Complete is called whenever a CDownload obj completes
// its download and initiates further downloads if necessary (eg. ProcessInf)
// It does nothing until all pending downloads are complete. Until then it
// just returns and we unwind back to BSC::OnStopBinding
//
// When all downloads completed, we then start processingall the Csetups
// We do this code download in two stages to
// keep capability to back out of entire code download for as late as we can
// until the setup stage calling CClBinding::Abort with IBinding returned by
// code downloader in client's BSC::OnStartBinding will cleanly abort and
// restore initial state.
// We don't honor Abort once in setup stage.
//
// To keep this stage as clean and failsafe as we can we check for
// disk space in the OCX cache as well as check for IN USE OCXes that we
// plan on updating. We abort on either of these two conditions.
//
// CCodeDownload::Complete than proceeds to walk thru all its download objs
// calling DoSetup which in turn causes CSetup::DoSetup() to get invoked
// for every CSetup.
//
// ---------------------------------------------------------------------------
VOID
CCodeDownload::CompleteOne(CDownload *pdl, HRESULT hrOSB, HRESULT hrStatus, HRESULT hrResponseHdr, LPCWSTR szError)
{

    CDownload *pdlCur = NULL;
    HRESULT hr = S_OK;
    HGLOBAL hPostData = NULL;
    WCHAR szURL[INTERNET_MAX_URL_LENGTH];
    FILEXTN extn = FILEXTN_UNKNOWN;
    LPWSTR lpDownloadURL;
    CDownload *pdlNew;
    DWORD cbPostData = 0;
    BOOL fWaitForAbortCompletion = FALSE;
    LISTPOSITION curpos;
    int i = 0;

    m_debuglog->DebugOut(DEB_CODEDL, FALSE, ID_CDLDBG_COMPLETEONE_IN,
                         hrStatus, hrOSB, hrResponseHdr, pdl->GetURL());

    CUrlMkTls tls(hr); // hr passed by reference!
    Assert(SUCCEEDED(hr));

    if (pdl->GetDLState() != DLSTATE_READY_TO_SETUP) {

        pdl->SetDLState(DLSTATE_DONE);
    }

    if (FAILED(hr)) {
        goto Complete_failed;
    }

    Assert(tls->pCDLPacketMgr);

    // called each time a download object is done downloading
    // and installing itself
    // this is the master state analyser that will determine if the total
    // code download is complete and clean up if reqd

    Assert(m_pDownloads.GetCount());    // atleast one (this one)

    // the three HRESULTS passed in are as follows

    // hrOSB = hr of OnStopBinding, ie URLMON came back with good HR
    // but we had some 'processing error with the data we got back
    // in such cases just assume a bad install and fail the operation. ie.
    // don't go into next component of CodeSearchPath to retify such errors

    if (FAILED(hrOSB)) {
        hr = hrOSB;
        goto Complete_failed;
    }

    // hrStatus = hr that URLMON came back with for the binding
    // right now URLMON does a terrible job with errors. sometimes we get back
    // an HTML response with a displayable error and URLMON say things
    // succeeded, which is why we have our own hrResponseHdr which is the
    // status as we fill in OnResponse.

    // there are some URLMON errors that make sense to allow further search
    // on CodeSearchPath and some others like E_ABORT, ie the
    // client did an IBinding::Abort().

    if (SUCCEEDED(hrResponseHdr) && SUCCEEDED(hrStatus)) {
            // here if the current download was completely successful
            // if all downloads are done then call DoSetup()

            if (WeAreReadyToSetup()) {              // more processing left?
                                                    // no, enter setup phase

                CCDLPacket *pPkt= new CCDLPacket(CODE_DOWNLOAD_SETUP, this, 0);

                if (pPkt) {
                    hr = pPkt->Post();
                } else {
                    hr = E_OUTOFMEMORY;
                }

                if (FAILED(hr)) {
                    goto Complete_failed;
                }


            }

            goto Exit;

    }

    if (hrStatus == E_ABORT) {
        // USER cancelled, abort entire code download
        hr = hrStatus;
        goto Complete_failed;
    }

    // here if the response header indicated a failure
    // errors we know about now are if the response URL is absent and no
    // suitable MIME tyoe was found or
    // the resource we queried for is absent on this server
    // try the next comp. on CodeSearchPath

    Assert(m_pmkContext);

    // if a top level req. failed wtih a HTTP error
    // we need to go next on searchpath.
    // we detect top level, either by the fact that it involed a POST
    // or by the fact that the context moniker is the same
    // moniker as the current download. By same moniker, we mean
    // the same pointer (not pmk->IsEqual(), this will match for
    // same URLs which is not necessarily top level)

    // This check makes an assumption that we will not change the
    // context moniker, excpet when redirecting a POST. if we do this is a
    // BUGBUG: !!!
    if (!(pdl->DoPost()) && (m_pmkContext != pdl->GetMoniker())) {
        if (FAILED(hrStatus))
            hr = hrStatus;
        else
            hr = hrResponseHdr;

        goto Complete_failed;
    }

    // reset the context to zero, so we will set a fresh one to the next
    // element on code searchpath
    if (RelContextMk()) {
        SAFERELEASE(m_pmkContext);
    } else {
        m_pmkContext = NULL;
    }

    // if the HTTP_ERROR was Not Modified, then this at the top level
    // is not an error: ie use current version. But, we any way go past
    // CODEBASE (that's the only one that can come back with Not Modified
    // everything else on the searchpath is a POST) to check all
    // servers on searchpath before we decide to use the current local version

    hr = GetNextOnInternetSearchPath(GetClsid(), &hPostData, &cbPostData,
                szURL, INTERNET_MAX_URL_LENGTH, &lpDownloadURL, &extn);

    if (FAILED(hr)) {

        // OK all tries failed at the top level
        // were we monkeying around for the very LATEST version
        // when in fact there was a local version already?
        if ( NeedLatestVersion() && m_plci->IsPresent()) {

                Assert(WeAreReadyToSetup());

                hr = S_OK;                        // no, fake a success
                SetFakeSuccess();
                CompleteAll(hr, NULL);            // and instantiate the object

                goto Exit;

        } else {
            goto Complete_failed;
        }
    }

    // download the CODE=URL (ie. CAB or INF file first)
    pdlNew = new CDownload(lpDownloadURL, extn, &hr);

    if (!pdlNew) {
        hr = E_OUTOFMEMORY;
        goto Complete_failed;
    } else if (FAILED(hr)) {
                delete pdlNew;
                goto Complete_failed;
            }

    AddDownloadToList(pdlNew);

    if (hPostData) {

        pdlNew->SetPostData(hPostData, cbPostData);
        hPostData = NULL; // mark as delegated, destructor for pdl will free
    }

    {
    BOOL bSetOnStack = SetOnStack();
    hr = pdlNew->DoDownload(&m_pmkContext,
                        (BINDF_ASYNCHRONOUS| BINDF_ASYNCSTORAGE));
    if (bSetOnStack)
        ResetOnStack();
    }

    if (SUCCEEDED(hr)) {
        // initiated a new download, return and wait for that to complete
        goto Exit;
    }

    // error initiating new download, abort


Complete_failed:

    Assert(FAILED(hr));

    if (SUCCEEDED(m_hr)) {

        // first failure in a multi-piece download
        // save away the real reason we failed. We pass this to
        // CompleteAll

        m_hr = hr;
    }

    // problem with download
    // abort all other downloads and then CompleteAll/cleanup

    // to mark that atleast one real URLMON bindign was aborted
    // in this case URLMON will post an OnStopBinding for that
    // and we will end up aborting all other bindings and the whole
    // code download. However if that's not the case then we were probably
    // in some post binding processing such as verifytrust cab extraction etc
    // and so we need to post a DoSetup() packet with UserCancelled flag set.

    fWaitForAbortCompletion = FALSE;


    curpos = m_pDownloads.GetHeadPosition();
    for (i=0; !IsOnStack() && ( i < m_pDownloads.GetCount()); i++) {

        pdlCur = m_pDownloads.GetNext(curpos);

        if (!pdlCur->IsSignalled(this)) {

            // packet processing pending for this state. we will check for
            // DLSTATE_ABORT in each packet processing state and if true
            // it will call CompleteOne(us), which marks each piece DLSTATE_DONE

            BOOL bSetOnStack = SetOnStack();
            pdlCur->Abort(this);
            if (bSetOnStack)
                ResetOnStack();

            if (!pdlCur->IsSignalled(this)) {
                fWaitForAbortCompletion = TRUE;
            }

        }


    }

    if (FAILED(m_hr)) {
        // fail with first real failure of a multipart code download
        hr = m_hr;
    }

    if (!fWaitForAbortCompletion && !IsOnStack())       // more processing left?
        CompleteAll(hr, szError);       // no, call complete all to cleanup

Exit:

    return;

}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::WeAreReadyToSetup()
// ---------------------------------------------------------------------------
BOOL
CCodeDownload::WeAreReadyToSetup()
{
    BOOL fReady = TRUE;
    CDownload *pdlCur = NULL;

    LISTPOSITION curpos = m_pDownloads.GetHeadPosition();
    for (int i=0; i < m_pDownloads.GetCount(); i++) {

        pdlCur = m_pDownloads.GetNext(curpos);

        if (! (( pdlCur->GetDLState() == DLSTATE_READY_TO_SETUP) ||
               ( pdlCur->GetDLState() == DLSTATE_DONE)) ) {

            fReady = FALSE;
            break;
        }
    }

    return fReady;

}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::ResolveCacheDirNameConflicts()
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::ResolveCacheDirNameConflicts()
{
    HRESULT hr = S_OK;
    char szDir[MAX_PATH];
    static char *szCONFLICT = "CONFLICT";
    CDownload *pdlCur = NULL;
    int n = 1;

    if (m_szCacheDir)       // the non-zeroness of this is also used by DoSetup
        goto Exit;          // to find it it's state machine has been init'ed

    // ease the update of in-memory OCXes that have been released
    // but still in memory as an optiization.
    CoFreeUnusedLibraries();

    // get a cache dir that has no name collisions for any of the
    // Csetup objs for this CodeDownload

    m_szCacheDir = g_szOCXCacheDir;

    do {

        LISTPOSITION curpos = m_pDownloads.GetHeadPosition();
        for (int i=0; i < m_pDownloads.GetCount(); i++) {

            pdlCur = m_pDownloads.GetNext(curpos);


            if ( (hr = pdlCur->CheckForNameCollision(m_szCacheDir)) != S_OK)
                break;
        }

        if (hr == S_OK) {

            if (m_szCacheDir == g_szOCXCacheDir)
                goto Exit;
            else
                goto Alloc_new;
        }

        // current m_szCacheDir did not work, try next conflict.<n> dir
        wsprintf(szDir,"%s\\%s.%d", g_szOCXCacheDir, szCONFLICT, n++);

        m_szCacheDir = szDir;


    } while (GetFileAttributes(szDir) != -1); // while conflict dirs exist

    // none of our existing conflict dirs solved the problem
    // create a new conflict dir named conflict.<n>

    if (!CreateDirectory(szDir, NULL)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Alloc_new:

    m_szCacheDir = new char [lstrlen(szDir)+1];

    if (m_szCacheDir) {
        lstrcpy(m_szCacheDir, szDir);
    } else {
        hr = E_OUTOFMEMORY;
    }

Exit:

    return hr;
}


// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::SetupZeroImpactDir()
//
// Makes a new cache dir according to the current private dist unit and version numbers
// Stores the zero-impact directory in m_szCacheDir and makes sure the dir exists
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::SetupZeroImpactDir()
{
    TCHAR szVersion[MAX_VERSIONLENGTH];
    TCHAR szZIDirectory[MAX_PATH];
    HRESULT hr = S_OK;

    if(! m_szDistUnit || ! m_szDistUnit[0])
        return E_INVALIDARG;
    // Will enter this function many times over the course of a code download setup
    // cache the cachedir
    if(m_szCacheDir && m_szCacheDir[0])
        return S_OK;

    // buffer overflow protection
    if(lstrlenW(m_szDistUnit) + lstrlen(GetZeroImpactRootDir()) + MAX_VERSIONLENGTH +3 > MAX_PATH)
        return E_UNEXPECTED;


    // Get the DistUnit!Version directory
    GetStringFromVersion(szVersion, m_dwFileVersionMS, m_dwFileVersionLS, '_');

    wsprintf(szZIDirectory, TEXT("%s\\%ls!%s"), GetZeroImpactRootDir(), m_szDistUnit, szVersion);

    // Create the directory GetZeroImpactRootDir()\DistUnit!Version
    // If the directory already exists, well and good
    if(GetFileAttributes(szZIDirectory) == -1)
    {
        if(! CreateDirectory(szZIDirectory, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            return hr;
        }
    }


    // Copy the destdir to the cache dir
    if(m_szCacheDir)
    {
        delete [] m_szCacheDir;
        m_szCacheDir = NULL;
    }
    m_szCacheDir = new TCHAR[lstrlen(szZIDirectory) + 1];
    if(! m_szCacheDir)
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }
    if(! lstrcpy(m_szCacheDir, szZIDirectory))
    {
        hr = GetLastError();
        return hr;
    }

    hr = S_OK;

    return hr;
}


// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::DoSetup()
//        Setup Phase:
// ---------------------------------------------------------------------------
VOID
CCodeDownload::DoSetup()
{
    HRESULT hr = S_OK;
    CDownload *pdlCur = NULL;
    int nSetupsPerCall = 0;
    HRESULT hr1 = S_OK;
    CUrlMkTls tls(hr1); // hr1 passed by reference!
    Assert(SUCCEEDED(hr1));
    Assert(tls->pCDLPacketMgr);
    int i;
    LISTPOSITION curpos;

    if (FAILED(m_hr)) {
        // the self-registering EXE failed or user cancelled waiting
        // for self-registering EXE
        hr = m_hr;
        goto ErrorExit;
    }

    if (UserCancelled()) {
        // user cancelled and CodeInstallProblem asked to abort
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        goto ErrorExit;
    }

    if (IsSilentMode()) {

        SetBitsInCache();   // flag that we have a new available version

        hr = ERROR_IO_INCOMPLETE;
        goto ErrorExit;
    }

    // ease the update of in-memory OCXes that have been released
    // but still in memory as an optiization.
    CoFreeUnusedLibraries();

    if (m_bUninstallOld) {
        LPSTR pPluginFileName = NULL;
        CLSID myclsid = GetClsid();
        CLocalComponentInfo lci;

        if ((SUCCEEDED(GetClsidFromExtOrMime( GetClsid(), myclsid,
            GetMainExt(), GetMainType(), &pPluginFileName)))) {

            if (IsControlLocallyInstalled(pPluginFileName,
                                          (pPluginFileName)?(LPCLSID)&GetClsid():&myclsid,
                                          GetMainDistUnit(), 0, 0, &lci, NULL) == S_OK) {
                HMODULE                   hMod;
                CHAR                     *szDU = NULL;
                REMOVECONTROLBYNAME pfn =  NULL;

                hMod = LoadLibrary("OCCACHE.DLL");
                if (hMod) {
                    pfn = (REMOVECONTROLBYNAME)GetProcAddress(hMod, "RemoveControlByName");
                    if (pfn) {
                        if (SUCCEEDED(Unicode2Ansi(GetMainDistUnit(), &szDU))) {
                            (*pfn)(lci.szExistingFileName, szDU, NULL, FALSE, TRUE);
                            SAFEDELETE(szDU);
                        }
                    }
                    FreeLibrary(hMod);
                }
            }
        }
    }

    if(this->IsZeroImpact())
        hr = SetupZeroImpactDir();
    else
        hr = ResolveCacheDirNameConflicts();

    if (FAILED(hr)) {
        goto ErrorExit;
    }


    // -------- UNSAFE TO ABORT BEGIN --------------
    SetUnsafeToAbort();

    //  we can start processing CSetup
    curpos = m_pDownloads.GetHeadPosition();
    for (i=0; i < m_pDownloads.GetCount(); i++) {

        pdlCur = m_pDownloads.GetNext(curpos);


        if ( (pdlCur->GetDLState() == DLSTATE_READY_TO_SETUP) ||
            (pdlCur->GetDLState() == DLSTATE_SETUP)) {


            // serialize all setups in this thread
            hr = AcquireSetupCookie();
            if (FAILED(hr)) {
                goto ErrorExit;
            } else if (hr == S_FALSE) {

                goto Exit;     // some other Code download on same thread
                            // is already in Setup phase. We will get a
                            // msg when its our turn
            }

            // acquired the setup cookie!


            if (nSetupsPerCall++) {
                // here if we have already done 1 setup in one
                // CDownload

                // post a message to ourselves and we can do the next
                // setup in that. This will give a chance for our client
                // to process messages.

                CCDLPacket *pPkt= new CCDLPacket(CODE_DOWNLOAD_SETUP,this,S_OK);

                if (pPkt) {
                    hr = pPkt->Post();
                } else {
                    hr = E_OUTOFMEMORY;
                }

                if (FAILED(hr))
                    break;

                goto Exit;
            }

            if(this->IsZeroImpact())
            {
                // make all the downloads of a ZeroImpact CodeDownload be zeroimpact before they setup
                pdlCur->SetZeroImpact(TRUE);
            }
            else if (m_bExactVersion)
            {
                pdlCur->SetExactVersion(TRUE);
            }

            hr = pdlCur->DoSetup();

            if (FAILED(hr))
                break;



            if(WaitingForEXE()) { // Did the setup start a self-registering EXE?

                // if we are waiting for an EXE to complete self registeration,
                // we can't proceed unless it completes. So kick off a
                // packet for waiting for the EXE to complete.

                CCDLPacket *pPkt= new CCDLPacket(CODE_DOWNLOAD_WAIT_FOR_EXE,
                    this,0);

                if (pPkt) {
                    hr = pPkt->Post();
                } else {
                    hr = E_OUTOFMEMORY;
                }

                if (FAILED(hr))
                    break;

                goto Exit;
            }


            if ( (pdlCur->GetDLState() == DLSTATE_READY_TO_SETUP) ||
                (pdlCur->GetDLState() == DLSTATE_SETUP)) {

                // more setup work left in pdlCur
                // wait to get to this and other pieces in next msg
                CCDLPacket *pPkt= new CCDLPacket(CODE_DOWNLOAD_SETUP,this,S_OK);

                if (pPkt) {
                    hr = pPkt->Post();
                } else {
                    hr = E_OUTOFMEMORY;
                }

                if (FAILED(hr))
                    break;

                goto Exit;

            }

        } /* if pdlCur needs setup */

    } /* for each pdlCur */


ErrorExit:


    hr1 = tls->pCDLPacketMgr->AbortPackets(GetDownloadHead());//aborts pdlCur, pdlCur->pcdl

    Assert(SUCCEEDED(hr1));
    if (FAILED(hr1)) {
        hr = hr1;
    }

    // here when completed the setup phase
    // give up the cookie and let someone else thru.
    RelinquishSetupCookie();

    CompleteAll(hr, NULL);

Exit:

    return;
    // -------- NO ABORT TILL SETUP COMPLETES in COmpleteAll --------------

}

HRESULT CCodeDownload::UpdateJavaList(HKEY hkeyContains)
{
    HRESULT hr = S_OK;
    HKEY hkeyJava = 0;
    LPSTR lpVersion = "";
    CDownload *pdlCur = NULL;
    int iNumJava = 0;
    int i;
    const static char *szJAVA = "Java";
    LONG lResult = ERROR_SUCCESS;


    //  count total number of Java setups if any
    LISTPOSITION curpos = m_pDownloads.GetHeadPosition();
    for (i=0; i < m_pDownloads.GetCount(); i++) {

        pdlCur = m_pDownloads.GetNext(curpos);

        iNumJava += (pdlCur->GetJavaSetupList())->GetCount();
    }

    if (!iNumJava)
        goto Exit;


    // open/create the Contains\Java key for this dist unit.
    if (RegOpenKeyEx( hkeyContains, szJAVA,
            0, KEY_ALL_ACCESS, &hkeyJava) != ERROR_SUCCESS) {
        if ((lResult = RegCreateKey( hkeyContains,
                   szJAVA, &hkeyJava)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
            }
    }

    curpos = m_pDownloads.GetHeadPosition();
    for (i=0; i < m_pDownloads.GetCount(); i++) {

        pdlCur = m_pDownloads.GetNext(curpos);

        LISTPOSITION curJavapos = (pdlCur->GetJavaSetupList())->GetHeadPosition();
        iNumJava = (pdlCur->GetJavaSetupList())->GetCount();


        for (int j=0; j < iNumJava; j++) {

            CJavaSetup *pJavaSetup = (pdlCur->GetJavaSetupList())->GetNext(curJavapos);
            LPCWSTR szPkg = pJavaSetup->GetPackageName();
            LPCWSTR szNameSpace = pJavaSetup->GetNameSpace();
            char szPkgA[MAX_PATH];
            if (szPkg)
                WideCharToMultiByte(CP_ACP, 0, szPkg, -1, szPkgA,
                                MAX_PATH, NULL, NULL);
            char szNameSpaceA[MAX_PATH];
            if (szNameSpace)
                WideCharToMultiByte(CP_ACP, 0, szNameSpace, -1, szNameSpaceA,
                                MAX_PATH, NULL, NULL);

            if (szNameSpace == NULL) { // global namespace if not specified
                if ( (lResult = ::RegSetValueEx(hkeyJava, szPkgA, NULL, REG_SZ,
                        (unsigned char *)lpVersion, 1)) != ERROR_SUCCESS) {

                    hr = HRESULT_FROM_WIN32(lResult);
                    goto Exit;
                }
            } else {

                // specific namespace provided. create a key under java
                // for that namespace

                HKEY hkeyNameSpace = 0;
                // open/create the Contains\Java\<namespace> key
                if (RegOpenKeyEx( hkeyJava, szNameSpaceA,
                        0, KEY_ALL_ACCESS, &hkeyNameSpace) != ERROR_SUCCESS) {
                    if ((lResult = RegCreateKey( hkeyJava,
                               szNameSpaceA, &hkeyNameSpace)) != ERROR_SUCCESS){
                        hr = HRESULT_FROM_WIN32(lResult);
                        goto Exit;
                    }
                }
                if ((lResult=RegSetValueEx(hkeyNameSpace, szPkgA, NULL, REG_SZ,
                        (unsigned char *)lpVersion, 1)) != ERROR_SUCCESS) {

                    hr = HRESULT_FROM_WIN32(lResult);
                    goto Exit;
                }

                if (hkeyNameSpace)
                    RegCloseKey(hkeyNameSpace);

            }

        }

    }

Exit:

    SAFEREGCLOSEKEY(hkeyJava);

    return hr;

}

HRESULT CCodeDownload::UpdateFileList(HKEY hkeyContains)
{
    HRESULT hr = S_OK;
    HKEY hkeyFiles = 0;
    LPSTR lpVersion = "";
    LONG lResult = ERROR_SUCCESS;
    const static char * szFILES = "Files";
    int iNumFiles = m_ModuleUsage.GetCount();
    int i;
    LISTPOSITION curpos = m_ModuleUsage.GetHeadPosition();
    char szAnsiFileName[MAX_PATH];

    // open/create the Contains\Files key for this dist unit.
    if (RegOpenKeyEx( hkeyContains, szFILES,
            0, KEY_ALL_ACCESS, &hkeyFiles) != ERROR_SUCCESS) {
        if (iNumFiles && (lResult = RegCreateKey( hkeyContains,
                   szFILES, &hkeyFiles)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
            }
    }

    if ( hkeyFiles) {

        int iValue = 0;
        DWORD dwType = REG_SZ;
        DWORD dwValueSize = MAX_PATH;
        char szFileName[MAX_PATH];

        while (RegEnumValue(hkeyFiles, iValue++,
            szFileName, &dwValueSize, 0, &dwType, NULL, NULL) == ERROR_SUCCESS) {

            dwValueSize = MAX_PATH; // reset

            if (GetFileAttributes(szFileName) == -1) {

                // if file is not physically present then clear out our
                // database. This is typically so, when you update
                // on older version with a newer version, but deleted the
                // old copy before installing the new one + changed the file
                // names or location.


                iValue = 0;
                RegDeleteValue(hkeyFiles, szFileName);
            }
        }
    }

    for (i=0; i < iNumFiles; i++) {

        LPCSTR szFileName = (m_ModuleUsage.GetNext(curpos))->GetFileName();

        char szShortFileName[MAX_PATH];
#ifdef SHORTEN
        if (!GetShortPathName(szFileName, szShortFileName, MAX_PATH)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
#else
        lstrcpy(szShortFileName, szFileName);
#endif

        // Under Win95 (and ONLY Win95), Setup API will convert characters
        // from the OEM code page to the ANSI code page. The codebase we have
        // is in the OEM codepage. After the Setup API installed the file,
        // the installed file name is in ANSI. Therefore, in the enumeration,
        // we need to look for the ANSI file name. Under other platforms,
        // this just works, and converting to the ANSI code page should not
        // be done. See IE5 RAID #34606 for more details.

        if (g_bRunOnWin95) {
            OemToChar(szShortFileName, szAnsiFileName);
            lstrcpy(szShortFileName, szAnsiFileName);
        }

        if ( (lResult = ::RegSetValueEx(hkeyFiles, szShortFileName, NULL, REG_SZ,
                    (unsigned char *)lpVersion, 1)) != ERROR_SUCCESS) {

            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }

    }

Exit:
    SAFEREGCLOSEKEY(hkeyFiles);

    return hr;

}

HRESULT CCodeDownload::UpdateDependencyList(HKEY hkeyContains)
{
    HRESULT hr = S_OK;
    HKEY hkeyDU = 0;
    LPSTR lpVersion = "";
    LONG lResult = ERROR_SUCCESS;
    const static char * szDU = "Distribution Units";
    int iNumFiles;
    int i;
    LISTPOSITION curpos;
    BOOL fFirstDependency = TRUE;
    LPWSTR wszDistUnit = NULL;
    LPSTR szDistUnit = NULL;

    iNumFiles = m_pDownloads.GetCount();
    curpos = m_pDownloads.GetHeadPosition();

    RegDeleteKey(hkeyContains, szDU);   // delete old version dependencies

    for (i=0; i < iNumFiles; i++) {

        CDownload *pdl = m_pDownloads.GetNext(curpos);

        if (pdl->UsingCdlProtocol() && pdl->GetDLState() == DLSTATE_DONE) {

            AddDistUnitList(pdl->GetDistUnitName());

        }

    }

    iNumFiles = m_pDependencies.GetCount();
    curpos = m_pDependencies.GetHeadPosition();

    if (!iNumFiles)
        goto Exit;

    for (i=0; i < iNumFiles; i++) {

        wszDistUnit = m_pDependencies.GetNext(curpos);

        if (wszDistUnit) {

            SAFEDELETE(szDistUnit);

            if (FAILED(Unicode2Ansi(wszDistUnit, &szDistUnit))) {
                hr = S_OK;
                goto Exit;
            }

            if (fFirstDependency) {

                if ((lResult = RegCreateKey( hkeyContains,
                       szDU, &hkeyDU)) != ERROR_SUCCESS) {
                    hr = HRESULT_FROM_WIN32(lResult);
                    goto Exit;
                }
                fFirstDependency = FALSE;
            }

            if ( (lResult = ::RegSetValueEx(hkeyDU, szDistUnit, NULL, REG_SZ,
                    (unsigned char *)lpVersion, 1)) != ERROR_SUCCESS) {
                hr = HRESULT_FROM_WIN32(lResult);
                goto Exit;
            }

        }

    }


Exit:
    SAFEREGCLOSEKEY(hkeyDU);
    SAFEDELETE(szDistUnit);

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::UpdateLanguageCheck()
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::UpdateLanguageCheck(CLocalComponentInfo *plci)
{
    HRESULT hr = S_OK;
    BOOL bNullClsid = IsEqualGUID(GetClsid() , CLSID_NULL);
    HKEY hkeyCheckPeriod = 0;
    HKEY hkeyClsid = 0;
    HKEY hkeyEmbedding = 0;
    LPOLESTR pwcsClsid = NULL;
    DWORD dwType;
    LONG lResult = ERROR_SUCCESS;
    LPSTR pszClsid = NULL;
    FILETIME ftnow;
    SYSTEMTIME st;
    const char *szCHECKPERIOD = "LanguageCheckPeriod";
    const char *szLASTCHECKEDHI = "LastCheckedHi";

    if (bNullClsid)
        goto Exit;

    // return if we can't get a valid string representation of the CLSID
    if (FAILED((hr=StringFromCLSID(GetClsid(), &pwcsClsid))))
        goto Exit;

    Assert(pwcsClsid != NULL);

    // Open root HKEY_CLASSES_ROOT\CLSID key
    lResult = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, "CLSID", 0, KEY_READ, &hkeyClsid);


    if (lResult == ERROR_SUCCESS)
    {
        if (FAILED((hr=::Unicode2Ansi(pwcsClsid, &pszClsid))))
        {
            goto Exit;
        }

        // Open the key for this embedding:
        lResult = ::RegOpenKeyEx(hkeyClsid, pszClsid, 0, KEY_ALL_ACCESS,
                        &hkeyEmbedding);

        if (lResult == ERROR_SUCCESS) {

            if ((lResult = RegOpenKeyEx( hkeyEmbedding, szCHECKPERIOD,
                    0, KEY_ALL_ACCESS, &hkeyCheckPeriod)) != ERROR_SUCCESS) {

                if ((lResult = RegCreateKey( hkeyEmbedding,
                           szCHECKPERIOD, &hkeyCheckPeriod)) != ERROR_SUCCESS) {
                    hr = HRESULT_FROM_WIN32(lResult);
                    goto Exit;
                }

            }

            GetSystemTime(&st);
            SystemTimeToFileTime(&st, &ftnow);

            RegSetValueEx(hkeyCheckPeriod, szLASTCHECKEDHI, NULL, REG_DWORD,
               (unsigned char *)&ftnow.dwHighDateTime, sizeof(DWORD));
        }
    }

Exit:
    SAFEDELETE(pwcsClsid);
    SAFEDELETE (pszClsid);

    SAFEREGCLOSEKEY (hkeyClsid);
    SAFEREGCLOSEKEY (hkeyEmbedding);
    SAFEREGCLOSEKEY (hkeyCheckPeriod);

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::UpdateDistUnit()
//        Add proper entries to the registry and register control to
//        WebCheck so that control gets updated periodically.
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::UpdateDistUnit(CLocalComponentInfo *plci)
{
    HRESULT hr = S_OK;
    LONG lResult = ERROR_SUCCESS;
    HKEY hkeyDist =0;
    HKEY hkeyThisDist = 0;
    HKEY hkeyDownloadInfo = 0;
    HKEY hkeyContains = 0;
    HKEY hkeyVersion = 0;
    const static char * szInstalledVersion = "InstalledVersion";
    const static char * szAvailableVersion = "AvailableVersion";
    const static char * szDownloadInfo = "DownloadInformation";
    const static char * szCODEBASE = "CODEBASE";
    const static char * szContains = "Contains";
    const static char * szLOCALINF = "INF";
    const static char * szLOCALOSD = "OSD";
    const static char * szLASTMODIFIED = "LastModified";
    const static char * szETAG = "Etag";
    const static char * szINSTALLER = "Installer";
    const static char * szExpire = "Expire";
    const static char * szMSICD = "MSICD";
    const static char * szPrecache = "Precache";
    const static char * szSYSTEM = "SystemComponent";
    LPSTR pszDist = NULL;
    LPSTR pszURL = NULL;
    LPWSTR pwszURL = NULL;
    char szVersionBuf[MAX_PATH];
    DWORD dwExpire;
    char szDistDotVersion[MAX_PATH];

    if ((lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_DIST_UNITS,
                        0, KEY_ALL_ACCESS, &hkeyDist)) != ERROR_SUCCESS) {
        if ((lResult = RegCreateKey( HKEY_LOCAL_MACHINE,
                   REGSTR_PATH_DIST_UNITS, &hkeyDist)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }
    }

    if (FAILED((hr=::Unicode2Ansi(m_szDistUnit, &pszDist))))
    {
        goto Exit;
    }


    if(GetContextMoniker()) {

        if (SUCCEEDED(hr = GetContextMoniker()->GetDisplayName(NULL, NULL, &pwszURL))) {

            hr = Unicode2Ansi( pwszURL, &pszURL);
        }

        if (FAILED(hr)) {
            goto Exit;
        }
    }

    // open/create the dist unit key for this dist unit.
    // If this is zero impact, the key needs to be distunit!version
    if(IsZeroImpact())
    {
        // BUGBUG: we do this twice (version is recalculated below: extra work
        // Did it this way to minimize zero-impact impact (heh, heh)
        GetStringFromVersion(szVersionBuf, m_dwFileVersionMS, m_dwFileVersionLS, '_');
        wsprintf(szDistDotVersion, "%s!%s", pszDist, szVersionBuf);

        if (RegOpenKeyEx( hkeyDist, szDistDotVersion,
                0, KEY_ALL_ACCESS, &hkeyThisDist) != ERROR_SUCCESS) {
            if ((lResult = RegCreateKey( hkeyDist,
                       szDistDotVersion, &hkeyThisDist)) != ERROR_SUCCESS) {
                hr = HRESULT_FROM_WIN32(lResult);
                goto Exit;
                }
        }
    }
    else
    {
        if (RegOpenKeyEx( hkeyDist, pszDist,
                0, KEY_ALL_ACCESS, &hkeyThisDist) != ERROR_SUCCESS) {
            if ((lResult = RegCreateKey( hkeyDist,
                       pszDist, &hkeyThisDist)) != ERROR_SUCCESS) {
                hr = HRESULT_FROM_WIN32(lResult);
                goto Exit;
                }
        }
    }

    if (m_szDisplayName &&
        ((lResult = ::RegSetValueEx(hkeyThisDist, NULL, NULL, REG_SZ,
                (unsigned char *)m_szDisplayName,
                lstrlen(m_szDisplayName)+1)) != ERROR_SUCCESS)){

        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    lResult = ::RegSetValueEx(hkeyThisDist, szSYSTEM, NULL, REG_DWORD,
                              (unsigned char *)&m_dwSystemComponent,
                              sizeof(DWORD));
    if (lResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    lResult = ::RegSetValueEx(hkeyThisDist, szINSTALLER, NULL, REG_SZ,
                        (unsigned char *)szMSICD, sizeof(szMSICD)+1);

    if (lResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    // if the OSD told us an expire interval, or the PE specifies it.
    if ( m_dwExpire != 0xFFFFFFFF ||
         (plci->szExistingFileName[0] &&
          WantsAutoExpire( plci->szExistingFileName, &m_dwExpire )) ) {

        lResult = ::RegSetValueEx(hkeyThisDist, szExpire, NULL, REG_DWORD,
                                  (unsigned char *)&m_dwExpire, sizeof(DWORD));

        if (lResult != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }
    }



    // open/create the download info key for this dist unit.
    if (RegOpenKeyEx( hkeyThisDist, szDownloadInfo,
            0, KEY_ALL_ACCESS, &hkeyDownloadInfo) != ERROR_SUCCESS) {
        if ((lResult = RegCreateKey( hkeyThisDist,
                   szDownloadInfo, &hkeyDownloadInfo)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
            }
    }

    // set download info params

    if (pszURL && (lResult = ::RegSetValueEx(hkeyDownloadInfo, szCODEBASE,
        NULL, REG_SZ, (unsigned char *)pszURL, lstrlen(pszURL)+1)) != ERROR_SUCCESS) {

        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    if (!BitsInCache()) {

        char szOldManifest[MAX_PATH];
        DWORD Size = MAX_PATH;
        DWORD dwType;
        DWORD lResult = ::RegQueryValueEx(hkeyDownloadInfo, szLOCALINF, NULL, &dwType,
                            (unsigned char *)szOldManifest, &Size);

        if (lResult == ERROR_SUCCESS) {

            if (!(GetMainInf() && (lstrcmpi(GetMainInf(), szOldManifest) == 0)) ) {

                // there is an old entry, clean up the entry and also the file
                // before upgrading to newer version

                DeleteFile(szOldManifest);
                if (!GetMainInf())
                    RegDeleteValue(hkeyDownloadInfo, szLOCALINF);
            }

        }

        Size = MAX_PATH;
        lResult = ::RegQueryValueEx(hkeyDownloadInfo, szLOCALOSD, NULL, &dwType,
                            (unsigned char *)szOldManifest, &Size);

        if (lResult == ERROR_SUCCESS) {

            if (!(GetOSD() && (lstrcmpi(GetOSD(), szOldManifest) == 0)) ) {

                // there is an old entry, clean up the entry and also the file
                // before upgrading to newer version

                DeleteFile(szOldManifest);
                if (!GetOSD())
                    RegDeleteValue(hkeyDownloadInfo, szLOCALOSD);
            }

        }



        if (GetOSD() &&  (lResult = ::RegSetValueEx(hkeyDownloadInfo,
            szLOCALOSD, NULL, REG_SZ, (unsigned char *)GetOSD(), lstrlen(GetOSD())+1)) != ERROR_SUCCESS) {

            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }

        if (GetMainInf() && (lResult = ::RegSetValueEx(hkeyDownloadInfo,
            szLOCALINF, NULL, REG_SZ, (unsigned char *)GetMainInf(), lstrlen(GetMainInf())+1)) != ERROR_SUCCESS) {

            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }

    }

    // end set download info params

    if (!VersionFromManifest(szVersionBuf)) {

        if (!BitsInCache()) {
            if (!plci->IsPresent()) {
                // This used to be an E_UNEXPECTED case. Still unexpected,
                // but we'll trace it and cope with it.
                m_debuglog->DebugOut(DEB_CODEDL, FALSE,
                                     ID_CDLDBG_DL_UPDATE_DU_NO_VERS,
                                     plci->szExistingFileName);
                plci->dwLocFVLS = 1;
            }

            wsprintf(szVersionBuf, "%d,%d,%d,%d",
                    (plci->dwLocFVMS & 0xffff0000)>>16,
                    (plci->dwLocFVMS & 0xffff),
                    (plci->dwLocFVLS & 0xffff0000)>>16,
                    (plci->dwLocFVLS & 0xffff));
        } else {
            // use the version number in the HTML or
            // the one called by the code download delivery agent
            // if present

            if (m_dwFileVersionMS | m_dwFileVersionLS) {

                wsprintf(szVersionBuf, "%d,%d,%d,%d",
                    (m_dwFileVersionMS & 0xffff0000)>>16,
                    (m_dwFileVersionMS & 0xffff),
                    (m_dwFileVersionLS & 0xffff0000)>>16,
                    (m_dwFileVersionLS & 0xffff));

            } else {
                lstrcpy(szVersionBuf, "-1,-1,-1,-1");
            }
        }
    }

    if (BitsInCache()) {

        if (RegOpenKeyEx( hkeyThisDist, szAvailableVersion,
                0, KEY_ALL_ACCESS, &hkeyVersion) != ERROR_SUCCESS) {
            if ((lResult = RegCreateKey( hkeyThisDist,
                       szAvailableVersion, &hkeyVersion)) != ERROR_SUCCESS) {
                hr = HRESULT_FROM_WIN32(lResult);
                goto Exit;
                }
        }

        // record result of caching bits.

        HRESULT hrRecord = m_hr;

        if (m_hr == TRUST_E_FAIL ||
            m_hr == TRUST_E_SUBJECT_NOT_TRUSTED)
        {
            hrRecord = ERROR_IO_INCOMPLETE;
        }

        lResult = ::RegSetValueEx(hkeyVersion, szPrecache, NULL, REG_DWORD,
                            (unsigned char *)&hrRecord, sizeof(DWORD));
        if (lResult != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }

    } else {

        if (RegOpenKeyEx( hkeyThisDist, szInstalledVersion,
                0, KEY_ALL_ACCESS, &hkeyVersion) != ERROR_SUCCESS) {
            if ((lResult = RegCreateKey( hkeyThisDist,
                       szInstalledVersion, &hkeyVersion)) != ERROR_SUCCESS) {
                hr = HRESULT_FROM_WIN32(lResult);
                goto Exit;
                }
        }

        // when we install a real version take out the
        // AvailableVersion key if one exists

        RegDeleteKey(hkeyThisDist, szAvailableVersion);
    }

    lResult = ::RegSetValueEx(hkeyVersion, NULL, NULL, REG_SZ,
                        (unsigned char *)szVersionBuf, lstrlen(szVersionBuf)+1);

    if (lResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    if (BitsInCache()) {   // in silent mode the control is not
                            // installed and so the below params to
                            // the dist unit are not relevant.
        goto Exit;
    }

    if (GetLastMod() && (lResult = ::RegSetValueEx(hkeyVersion,
        szLASTMODIFIED, NULL, REG_SZ, (unsigned char *)GetLastMod(), lstrlen(GetLastMod())+1)) != ERROR_SUCCESS) {

        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    if (GetEtag() && (lResult = ::RegSetValueEx(hkeyVersion,
        szETAG, NULL, REG_SZ, (unsigned char *)GetEtag(), lstrlen(GetEtag())+1)) != ERROR_SUCCESS) {

        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    // store dist unit dependencies
    // store the dist unit files, pkgs installed
    // save away the manifest location/name


    // open/create the Contains key for this dist unit.
    if (RegOpenKeyEx( hkeyThisDist, szContains,
            0, KEY_ALL_ACCESS, &hkeyContains) != ERROR_SUCCESS) {
        if ((lResult = RegCreateKey( hkeyThisDist,
                   szContains, &hkeyContains)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
            }
    }

    hr = UpdateFileList(hkeyContains);

    if (SUCCEEDED(hr))
        hr = UpdateDependencyList(hkeyContains);

    if (SUCCEEDED(hr))
        hr = UpdateJavaList(hkeyContains);


Exit:
    SAFEDELETE(pszDist);
    SAFEDELETE(pszURL);
    SAFEDELETE(pwszURL);

    SAFEREGCLOSEKEY(hkeyContains);
    SAFEREGCLOSEKEY(hkeyDownloadInfo);
    SAFEREGCLOSEKEY(hkeyVersion);
    SAFEREGCLOSEKEY(hkeyDist);
    SAFEREGCLOSEKEY(hkeyThisDist);

    return hr;
}

typedef BOOL (WINAPI *SHRESTARTDIALOG)( HWND, LPTSTR, DWORD );

HRESULT DoReboot(HWND hWndParent)
{
    HRESULT hr = S_OK;
    HINSTANCE    hShell32Lib;

#define SHRESTARTDIALOG_ORDINAL    59       // restart only exported by ordinal

    SHRESTARTDIALOG          pfSHRESTARTDIALOG = NULL;

    if ( ( hShell32Lib = LoadLibrary( "shell32.dll" ) ) != NULL )  {

        if ( !( pfSHRESTARTDIALOG = (SHRESTARTDIALOG)
                      GetProcAddress( hShell32Lib, MAKEINTRESOURCE(SHRESTARTDIALOG_ORDINAL)) ) ) {

            hr = HRESULT_FROM_WIN32(GetLastError());
        } else {
            pfSHRESTARTDIALOG(hWndParent, NULL, EWX_REBOOT);
        }

    } else  {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (hShell32Lib)
        FreeLibrary( hShell32Lib );

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::CompleteAll(HRESULT hr, LPCWSTR szError)
//        All code is installed. If code install was succesful instantiate
//        object if reqd, and report ClientBSC::OnStopBinding.
// ---------------------------------------------------------------------------
VOID
CCodeDownload::CompleteAll(HRESULT hr, LPCWSTR szError)
{
    char szCacheFileName[MAX_PATH];
    HRESULT hrTls = S_OK;
    LISTPOSITION curpos;
    int iNumClients;
    int i;
//  TCHAR szBuffer[MAX_FORMAT_MESSAGE_BUFFER_LEN];
    LPTSTR szBuffer = NULL;
    DWORD dwFMResult = 0;
    BOOL bForceWriteLog = FALSE;
    WCHAR wszDll[MAX_PATH];
    TCHAR szDll[MAX_PATH];
    BOOL            bLogGenOk = FALSE;
    OSVERSIONINFO   osvi;
    WCHAR          *pwszOSBErrMsg = NULL;
    char           *pszExactErrMsg = NULL;
    char            szDPFPath[MAX_PATH];


    Assert(GetState() != CDL_Completed);
    SetState(CDL_Completed);

    CUrlMkTls tls(hrTls); // hr passed by reference!
    if (FAILED(hrTls)) {
        hr = hrTls;
    }

    // get the installed version one more time
    // to store in the dist unit db
    CLocalComponentInfo lci;

    LPSTR pPluginFileName = NULL;
    CLSID myclsid = GetClsid();

    if ((SUCCEEDED(GetClsidFromExtOrMime( GetClsid(), myclsid,
        GetMainExt(), GetMainType(), &pPluginFileName)))) {

        // get current version, pass 0, 1 to goose IsControl
        // into filling in the version data, otherwise UpdateDistUnit
        // will put a funny version in the registry ( which is better
        // than bug 12081

        //BUGBUG: make sure this call does the right things with zero impact

        IsControlLocallyInstalled(pPluginFileName,
                (pPluginFileName)?(LPCLSID)&GetClsid():&myclsid, GetMainDistUnit(),
                0, 1, &lci, NULL);


    }

    if ( m_plci->bForceLangGetLatest ||
         (lci.bForceLangGetLatest && SUCCEEDED(hr)) ) {
        hr = UpdateLanguageCheck(&lci);
    }

    if (!IsZeroImpact()) {
        if (SUCCEEDED(hr) && hr != ERROR_IO_INCOMPLETE) {

            // update all the queued up ModuleUsage records
            // we need to also remap to get the main clsid
            // incase we didn't have one to begin with

            UpdateModuleUsage();

        }
    }

    if ( !FakeSuccess() && (SUCCEEDED(hr) || BitsInCache())) {
        UpdateDistUnit(&lci);
    }

    if (IsZeroImpact()) {    // Register this download as zero impact, and remember the dll name for bubbling upwards
        if(SUCCEEDED(hr)) {  // should be done after UpdateDistUnit
            UpdateZeroImpactCache();

            if (GetClientBinding()->GetZIDll())
            {
                wsprintf(szDll, TEXT("%s\\%ls"), m_szCacheDir, GetClientBinding()->GetZIDll());
                if (MultiByteToWideChar(CP_ACP, 0, szDll, -1, wszDll, MAX_PATH) == 0) {
                    wszDll[0] = L'\0';
                }
            }
            else {
                wszDll[0] = L'\0';
            }
        }
    }

    if (NeedToReboot()) {
        HWND hWnd = GetClientBinding()->GetHWND();

        // pass a notification to reboot
        if (hWnd != INVALID_HANDLE_VALUE) {
            // g_RunSetupHook.DoReboot(hWnd, TRUE);
            DoReboot(hWnd);

        } else {

            ICodeInstall* pCodeInstall = GetICodeInstall();
            if (pCodeInstall)
                pCodeInstall->OnCodeInstallProblem( CIP_NEED_REBOOT, NULL, NULL, 0);
       }
    }

    iNumClients = m_pClientbinding.GetCount();

    // if called from CoGetClassFromURL we need to report
    // ClientBSC::OnObjectAvailable with requested obj
    if (SUCCEEDED(hr) && NeedObject() && hr != ERROR_IO_INCOMPLETE) {

        curpos = m_pClientbinding.GetHeadPosition();
        for (i=0; i < iNumClients; i++) {

            hr = (m_pClientbinding.GetNext(curpos))->InstantiateObjectAndReport(this);

            if(FAILED(hr)) {
                bLogGenOk = GenerateErrStrings(hr, &pszExactErrMsg, &pwszOSBErrMsg);
                if (!bLogGenOk) {
                    pwszOSBErrMsg = NULL;
                    pszExactErrMsg = NULL;
                }
            }
        }

    } else {
        // call OnStopBinding for all BSCs (since we either do not need
        // an instantiated object or we don't have one to give)

        if(FAILED(hr)) {
            bLogGenOk = GenerateErrStrings(hr, &pszExactErrMsg, &pwszOSBErrMsg);
            if (!bLogGenOk) {
                pwszOSBErrMsg = NULL;
                pszExactErrMsg = NULL;
            }
        }

        // call client's onstopbinding
        curpos = m_pClientbinding.GetHeadPosition();
        for (i=0; i < iNumClients; i++) {
            ((m_pClientbinding.GetNext(curpos))->GetAssBSC())->
                OnStopBinding(hr, pwszOSBErrMsg);
        }
        SAFEDELETE(pwszOSBErrMsg);

        m_debuglog->DebugOut(DEB_CODEDL, hr != S_OK, ID_CDLDBG_ONSTOPBINDING_CALLED,
                             hr, (hr == S_OK)?TEXT(" (SUCCESS)"):TEXT(" (FAILED)"),
                             (GetClsid()).Data1, GetMainURL(), GetMainType(),
                             GetMainExt());
    }

    if (m_hKeySearchPath) {
        if (RegQueryValueEx(m_hKeySearchPath,"ForceCodeDownloadLog", NULL, NULL,
            NULL, NULL) == ERROR_SUCCESS)
             bForceWriteLog = TRUE;
    }

    if (bForceWriteLog || (hr != S_OK && hr != ERROR_IO_INCOMPLETE)) {
        // BUGBUG: move these into .rc
        if (!bLogGenOk && (HRESULT_FACILITY(hr) == FACILITY_CERT)) {
            DumpDebugLog(szCacheFileName, "Trust verification failed!!", hr);
        } else if (!bLogGenOk &&
                    (hr==HRESULT_FROM_WIN32(ERROR_EXE_MACHINE_TYPE_MISMATCH))) {
            DumpDebugLog(szCacheFileName,
                "Incompatible Binary for your platform", hr);
        } else if (bLogGenOk) {
            DumpDebugLog(szCacheFileName, pszExactErrMsg, hr);
        } else {
            DumpDebugLog(szCacheFileName, "Unknown Error!!", hr);
        }

    }

    // Refresh OCCACHE

    // Only do this for NT for now. For some reason under Win95, sending
    // this message here will cause a crash in SHELL32.DLL.

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && m_szCacheDir) {
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, m_szCacheDir, 0);
    }

    // free all memory and clean up temp files

    SAFEDELETE(pszExactErrMsg);

    Release();

    return;
    // -------- END OF UNSAFE TO ABORT --------------
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::GenerateErrStrings()
//    Parameters:
//        ppszErrMsg: Saved error message or result of FormatMessage
//        pwszError: Error message to pass back as szError in OSB
//        hr: HRESULT of binding operation
//    Returns:
//        TRUE if successful
// ---------------------------------------------------------------------------

BOOL CCodeDownload::GenerateErrStrings(HRESULT hr, char **ppszErrMsg,
                                       WCHAR **ppwszError)
{
    DWORD                  dwFMResult;
    LPCTSTR                pszSavedErrMsg = NULL;
    TCHAR                 *szBuf = NULL;
    char                  *szURL = NULL;
    char                  *pszErrorMessage = NULL;
    char                   szErrString[MAX_DEBUG_STRING_LENGTH];
    char                   szDetails[MAX_DEBUG_STRING_LENGTH];
    int                    iSize = 0;

    if (!ppszErrMsg || !ppwszError) {
        dwFMResult = FALSE;
        goto Exit;
    }

    // Get a saved error message if available

    dwFMResult = FALSE;
    pszSavedErrMsg = CDLDebugLog::GetSavedMessage();

    if (pszSavedErrMsg[0] != '\0') {

        *ppszErrMsg = new char[lstrlen(pszSavedErrMsg) + 1];
        if (!*ppszErrMsg) {
            dwFMResult = FALSE;
            goto Exit;
        }

        lstrcpy(*ppszErrMsg, pszSavedErrMsg);
        dwFMResult = TRUE;
    }

    if (!dwFMResult) {

        // We don't have a saved message we can use. Try calling
        // FormatMessage().

        if (!dwFMResult) {
            dwFMResult = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                       FORMAT_MESSAGE_FROM_SYSTEM, 0, hr, 0,
                                       (LPTSTR)&szBuf,
                                       0, NULL);
        }

        if (!dwFMResult) {
            dwFMResult = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                       FORMAT_MESSAGE_FROM_HMODULE, g_hInst,
                                       hr, 0, (LPTSTR)&szBuf, 0, NULL);
        }

        if (dwFMResult) {

            ASSERT(szBuf);
            ASSERT(lstrlen(szBuf));

            *ppszErrMsg = new char[lstrlen(szBuf) + 1];
            if (!*ppszErrMsg) {
                dwFMResult = FALSE;
                goto Exit;
            }

            lstrcpy(*ppszErrMsg, szBuf);
            LocalFree(szBuf);
        }
        else {
            char szUnknown[MAX_DEBUG_FORMAT_STRING_LENGTH];

            if (!dwFMResult && HRESULT_FACILITY(hr) == FACILITY_SETUPAPI) {
                LoadString(g_hInst, ID_CDLDBG_UNKNOWN_SETUP_ERROR, szUnknown,
                           MAX_DEBUG_FORMAT_STRING_LENGTH);
            }
            else {
                LoadString(g_hInst, ID_CDLDBG_UNKNOWN_ERROR, szUnknown,
                           MAX_DEBUG_FORMAT_STRING_LENGTH);
            }

            *ppszErrMsg = new char[lstrlen(szUnknown) + 1];
            if (!*ppszErrMsg) {
                dwFMResult = FALSE;
                goto Exit;
            }

            lstrcpy(*ppszErrMsg, szUnknown);
        }
    }

    // ppszErrMsg now holds saved error message or whatever came back from
    // FormatMessage(). Construct a complete error message.

    m_debuglog->MakeFile();

    LoadString(g_hInst, ID_CDLDBG_ERROR_STRING, szErrString, MAX_DEBUG_STRING_LENGTH);
    LoadString(g_hInst, ID_CDLDBG_DETAILS_STRING, szDetails, MAX_DEBUG_STRING_LENGTH);

    szURL = (char *)m_debuglog->GetUrlName();
    ASSERT(szURL[0] != '\0');

    iSize = lstrlen(*ppszErrMsg) + lstrlen(szErrString) + lstrlen(szDetails)
            + lstrlen(szURL) + 1;

    pszErrorMessage = new char[iSize];
    if (!pszErrorMessage) {
        dwFMResult = FALSE;
        goto Exit;
    }

    wnsprintfA(pszErrorMessage, ID_CDLDBG_UNKNOWN_ERROR, "%s%s%s%s"
                              , szErrString, *ppszErrMsg, szDetails, szURL);

    if (FAILED(Ansi2Unicode(pszErrorMessage, ppwszError))) {
        dwFMResult = FALSE;
        goto Exit;
    }

Exit:

    SAFEDELETE(pszErrorMessage);

    return (dwFMResult != 0);
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::DumpDebugLog()
//   Output the debug error log. This log is written as a cache entry.
// ---------------------------------------------------------------------------

void CCodeDownload::DumpDebugLog(char *szCacheFileName, LPTSTR szErrorMsg,
                                 HRESULT hrError)
{
    m_debuglog->DumpDebugLog(szCacheFileName, MAX_PATH,
                          szErrorMsg, hrError);
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CCodeDownload::AddRef()
{
    return m_cRef++;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::Release
//        Clean up all temp files and free all memory
//        Caller of this function cannot rely on accessing any data
//        other than locals on their stack.
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CCodeDownload::Release()
{
    CDownload *pdlCur;
    HRESULT hr = S_OK;

    Assert(m_cRef > 0);

    if (--m_cRef != 0) {
        return m_cRef;
    }

    // release all CDownload objs

    LISTPOSITION curpos = m_pDownloads.GetHeadPosition();
    for (int i=0; i < m_pDownloads.GetCount(); i++) {

        pdlCur = m_pDownloads.GetNext(curpos);

        pdlCur->ReleaseParent(this);
    }

    CUrlMkTls tls(hr); // hr passed by reference!

    Assert(SUCCEEDED(hr));

    // remove this CCodeDownload from the per-thread list
    if (m_ListCookie)
        tls->pCodeDownloadList->RemoveAt(m_ListCookie);

    delete this;

    return 0;
}


// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::AcquireSetupCookie()
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::AcquireSetupCookie()
{
    HRESULT hr = S_OK;
    CUrlMkTls tls(hr); // hr passed by reference!

    if (FAILED(hr))     // if tls ctor failed above
        goto Exit;

    Assert(tls->pSetupCookie);

    // need to serialize all Setup on this thread
    // grab the Setup cookie

    hr = tls->pSetupCookie->Acquire(this);
    if (hr != S_OK) {

        Assert(!tls->pSetupCookie->IsFree());
        Assert(!tls->pSetupCookie->IsOwner(this));

        // wait till we get posted a message when the current owner
        // relinquishes the cookie

        goto Exit;
    }

    // have the cookie
    Assert(tls->pSetupCookie->IsOwner(this));

    SetState(CDL_Setup);

Exit:

    return hr;

}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::RelinquishSetupCookie()
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::RelinquishSetupCookie()
{
    HRESULT hr = S_OK;
    CUrlMkTls tls(hr); // hr passed by reference!

    if (FAILED(hr))     // if tls ctor failed above
        goto Exit;

    if (tls->pSetupCookie->IsOwner(this)) {

        tls->pSetupCookie->Relinquish(this);

        Assert(!tls->pSetupCookie->IsOwner(this));

    }

Exit:

    return hr;

}


// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::AddDownloadToList
// ---------------------------------------------------------------------------
#ifndef unix
inline VOID
#else
VOID
#endif /* !unix */
CCodeDownload::AddDownloadToList(CDownload *pdl)
{
    pdl->AddParent(this);

    m_pDownloads.AddHead(pdl);

}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload:: FindCABInDownloadList(szURL, pdlHint)
//    Find a download (typically a CAB) in download list
//    pdlHint is the first thing we look at (for perf.) as most usually
//    is a case of primary OCX in a CAB that the INF came in
//  Returns:
//      hr = ERROR (some error occurred, ignore *pdlMatch
//      hr = S_OK
//          if (*pdlMatch) match found, match is *pdlMatch
//          else
//              no match, or dups in other code downloads, download your own
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::FindCABInDownloadList(LPCWSTR szURL, CDownload* pdlHint, CDownload **ppdlMatch)
{
    CDownload *pdlCur = NULL;
    HRESULT hr = S_OK;
    IMoniker* pmk = NULL;
    int i;
    LISTPOSITION curpos;

    *ppdlMatch = pdlCur;

    // create a moniker for the URL passed in and then we can pmk->IsEqual
    // with every other CDownload's moniker.

    IBindHost *pBH = GetClientBinding()->GetIBindHost();
    if (pBH) {
        hr = pBH->CreateMoniker((LPWSTR)szURL, pdlHint->GetBindCtx(), &pmk, 0);
    } else {
        hr =  CreateURLMoniker(m_pmkContext, szURL, &pmk);
    }


    if (FAILED(hr))
        goto Exit;

    pdlCur = pdlHint; // assume hit

    hr = pmk->IsEqual(pdlHint->GetMoniker());

    if (hr != S_FALSE)
        goto Exit;

    if (pdlHint->DoPost()) {

        hr = pmk->IsEqual(m_pmkContext);

        if (hr != S_FALSE)
            goto Exit;
    }

    // hint failed, try the whole list
    curpos = m_pDownloads.GetHeadPosition();
    for (i=0; i < m_pDownloads.GetCount(); i++) {

        pdlCur = m_pDownloads.GetNext(curpos);


        if (pdlCur == pdlHint) // already tried the pdlHint, don't retry
            continue;

        hr = pmk->IsEqual(pdlCur->GetMoniker());

        if (hr != S_FALSE)
            goto Exit;

    }

    pdlCur = NULL;

    // now look across downloads

    hr = FindDupCABInThread(pmk, &pdlCur);

Exit:

    if (pmk)
        pmk->Release();

    *ppdlMatch = pdlCur;

    return hr;

}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::FindDupCABInThread(IMoniker *pmk, CDownload **ppdlMatch)
//    Find a download (typically a CAB) across all code downloads in thread
//  Returns:
//      hr = ERROR
//      hr = S_OK pdlCur?(match found):(no match found, do your own download)
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::FindDupCABInThread(IMoniker *pmk, CDownload **ppdlMatch)
{
    CDownload *pdlCur = NULL;
    HRESULT hr = S_OK;
    LISTPOSITION curposCDL, curposDL;
    CCodeDownload *pcdl;
    int iNumCDL;
    int i,j;

    CUrlMkTls tls(hr); // hr passed by reference!

    if (FAILED(hr))
        goto Exit;

    iNumCDL = tls->pCodeDownloadList->GetCount();
    curposCDL = tls->pCodeDownloadList->GetHeadPosition();

    // walk thru all the code downloads in the thread and check for DUPs
    for (i=0; i < iNumCDL; i++) {

        pcdl = tls->pCodeDownloadList->GetNext(curposCDL);

        if (pcdl == this)
            continue;

        // look into this CCodeDownload tree for dup

        curposDL = pcdl->m_pDownloads.GetHeadPosition();
        for (j=0; j < pcdl->m_pDownloads.GetCount(); j++) {

            pdlCur = pcdl->m_pDownloads.GetNext(curposDL);

            hr = pmk->IsEqual(pdlCur->GetMoniker());

            if (hr != S_FALSE)
                goto Exit;

        }
        pdlCur = NULL;
    }

    hr = S_OK;


Exit:

    if (pdlCur) {

        // found a match in another Code Download
        // add this pdl to our download list as well.


        if (pdlCur->GetDLState() > DLSTATE_SETUP) {
            pdlCur = NULL;          // too late to piggy back
        } else {

            m_debuglog->DebugOut(DEB_CODEDL, FALSE, ID_CDLDBG_FOUND_DUP,
                                 pdlCur->GetURL(), GetClsid().Data1,
                                 GetMainURL(), m_dwFileVersionMS,
                                 m_dwFileVersionLS);

            AddDownloadToList(pdlCur);
        }

    }

    *ppdlMatch = pdlCur;

    return hr;

}


// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::InitLastModifiedFromDistUnit
// ---------------------------------------------------------------------------
VOID
CCodeDownload::InitLastModifiedFromDistUnit()
{

    WIN32_FIND_DATA fd;
    HANDLE hf;

    IsDistUnitLocallyInstalled( m_szDistUnit, 0, 0, m_plci, NULL, NULL, 0);

    if (!m_plci->GetLastModifiedTime() ) {

        if ((hf = FindFirstFile(m_plci->szExistingFileName, &fd)) != INVALID_HANDLE_VALUE) {
            memcpy(&(m_plci->ftLastModified), &(fd.ftLastWriteTime), sizeof(FILETIME));
            FindClose(hf);
        }
    }

}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::DoCodeDownload
//    Main action entry point for CCodeDownload
//
// This triggers creation of the first CDownload object for the CODE url
// if a local check for CLSID,FileVersion returns update_needed.
// (note : it is interesting to note here that if a control needs to just
// update a dependent DLL file it still needs to update the FileVersion
// of the primary control file (with CLSID implementation) for triggering
// any download at all!
//
// Once DoCodeDownload determines that an update is in order it creates
// a CClBinding for its client to call client BSC::OnstartBinding with.
//
// It then adds this CDownload obj to its list of downloads.
//
// If the m_url is a CAB or INF we need to download it before we know
// what we need to do next. Otherwise we create a CSetup obj for the
// download and add it to CDownload's list of pending Setup processing for
// stage 2 (setup and registeration). CSetup details later.
//
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::DoCodeDownload(
    CLocalComponentInfo *plci,
    DWORD flags)
{
    CDownload *pdl = NULL;
    HRESULT hr = NOERROR;
    FILEXTN extn = FILEXTN_UNKNOWN;
    WCHAR szURL[INTERNET_MAX_URL_LENGTH];
    LPWSTR lpDownloadURL;
    HGLOBAL hPostData = NULL;
    DWORD cbPostData = 0;
    ICodeInstall* pCodeInstall = GetICodeInstall();


    // set if we need to instantiate the object or just download/install it
    SetNeedObject(flags);

    // set if we should ignore the internet search path
    SetUseCodebaseOnly(flags);

    m_plci = plci;
    Assert(plci);

    // get lcid from the bind context
    BIND_OPTS2 bopts;
    bopts.cbStruct = sizeof(BIND_OPTS2);

    if (SUCCEEDED(GetClientBC()->GetBindOptions(&bopts)) &&
        (bopts.cbStruct == sizeof(BIND_OPTS2)) ) {

        m_lcid = bopts.locale;  // else user default lcid is already in
    }

    if (m_plci->IsPresent() && pCodeInstall) {

        // a prev ver exists. get permission to overwrite
        // if ICodeInstall available

        WCHAR szBuf[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, m_plci->szExistingFileName, -1, szBuf, MAX_PATH);
        hr = pCodeInstall->OnCodeInstallProblem( CIP_OLDER_VERSION_EXISTS,
                    NULL, szBuf, 0);

        // hr == E_ABORT: abort whole download
        if (FAILED(hr)) {

            if (hr == E_ABORT)
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);

            // else preserve error code of OnCodeInstallProblem

            goto CD_Exit;
        }

    }



    // we need local version modified time only when doing GetLatest and
    // only for the top most file
    if (NeedLatestVersion() && m_plci->szExistingFileName[0] &&
            !m_plci->GetLastModifiedTime() ) {

        InitLastModifiedFromDistUnit();

    }

    if ((!m_url) || !(*m_url))  // if no CODE= <url>, mark that the option is
        SetUsedCodeURL();       // already exhausted

    // get the first site to try downloading from
    hr = GetNextOnInternetSearchPath(GetClsid(), &hPostData, &cbPostData, szURL,
            INTERNET_MAX_URL_LENGTH, &lpDownloadURL, &extn);

    if ( FAILED(hr))
        goto CD_Exit;

    // download the CODE=URL (ie. CAB or INF file first)
    pdl = new CDownload(lpDownloadURL, extn, &hr);

    if (!pdl) {
        hr = E_OUTOFMEMORY;
        goto CD_Exit;
    } else if (FAILED(hr)) {
        delete pdl;
        goto CD_Exit;
    }

    AddDownloadToList(pdl);

    if (hPostData) {

        pdl->SetPostData(hPostData, cbPostData);
        hPostData = NULL; // mark as delegated, destructor for pdl will free
    }

    // don't need to set on stack for this case as we have addref'ed the
    // pcdl. The reason we don't use the same addref technique on other
    // situations is because while addref controls the life of the object
    // the onstack is a dictate to not issue the OnStopBinding
    // setting onstack from here will prevent the client from getting
    // the OnStopBinding a must if an OnStartBinding has been issued.

    hr = pdl->DoDownload(&m_pmkContext,
                        (BINDF_ASYNCHRONOUS| BINDF_ASYNCSTORAGE));

    if (hr == MK_S_ASYNCHRONOUS) {
        SetState(CDL_Downloading);
    }


CD_Exit:

    if (FAILED(hr)) {

        if (hPostData)
            GlobalFree(hPostData);

    }

    return hr;
}


// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::SetupCODEUrl
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::SetupCODEUrl(LPWSTR *ppDownloadURL, FILEXTN *pextn)
{
    char *pBaseFileName = NULL;
    char szBuf[INTERNET_MAX_URL_LENGTH];

    WideCharToMultiByte(CP_ACP, 0, m_url, -1, szBuf,
        INTERNET_MAX_URL_LENGTH, 0,0);
    *pextn = GetExtnAndBaseFileName( szBuf, &pBaseFileName);

    *ppDownloadURL = m_url;
    SetUsedCodeURL();

    return S_OK;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::GetNextComponent
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::GetNextComponent(
    LPSTR szURL,
    LPSTR *ppCur
    )
{
    HRESULT hr = S_OK;
    LPSTR pch = *ppCur;
    LPSTR pchOut = szURL;
    int cbBuffer = 0;

#define BEGIN_ANGLE_BRKT    '<'
#define END_ANGLE_BRKT  '>'
#define COMP_DELIMITER  ';'

    if (!pch || *pch == '\0') {
        hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
        goto Exit;
    }

    if (*pch == BEGIN_ANGLE_BRKT) {

        pch++; // skip BEGIN_ANGLE_BRKT
        for (; *pch && (*pch != END_ANGLE_BRKT);) {
            *pchOut++ = *pch++;
            if (cbBuffer++ >= INTERNET_MAX_URL_LENGTH) {
                hr = E_INVALIDARG;
                goto Exit;
            }
        }

        if (*pch)
            pch++;  // skip the END_ANGLE_BRKT

    } else {

        // assume its CODEBASE, just copy the string till we reach the
        // next COMP_DELIMITER
        for (; *pch && (*pch != COMP_DELIMITER);) {
            *pchOut++ = *pch++;
            if (cbBuffer++ >= INTERNET_MAX_URL_LENGTH) {
                hr = E_INVALIDARG;
                goto Exit;
            }
        }
    }

    *pchOut = '\0';

    if (*pch)
        *ppCur = pch+1; // skip the COMP_DELIMITER
    else
        *ppCur = pch;

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::GetNextOnInternetSearchPath
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::GetNextOnInternetSearchPath(
    REFCLSID rclsid,
    HGLOBAL *phPostData,
    DWORD *pcbPostData,
    LPWSTR szURL,
    DWORD dwSize,
    LPWSTR *ppDownloadURL,
    FILEXTN *pextn
    )
{

    LONG lResult;
    HRESULT hr = S_OK;
    DWORD Size;
    DWORD dwType;
    char szBuf[INTERNET_MAX_URL_LENGTH];
    DWORD cb;
    static char *szISP = "CodeBaseSearchPath";

    char szClsid[MAX_PATH];
    char szID[MAX_PATH];
    char szHackMimeType[MAX_PATH];
    HGLOBAL hPostData = NULL;
    char szNeedVersion[100]; // enough to hold four shorts as a,b,c,d + nulterm
    BOOL bMimeType = FALSE;

    // Ignore Internet Search path if set.
    if (UseCodebaseOnly())
    {
        SetupCODEUrl(ppDownloadURL,pextn);
        goto Exit;
    }

    if (!m_hKeySearchPath) {

        lResult = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_IE_SETTINGS, 0,
                        KEY_READ, &m_hKeySearchPath);

        if (lResult == ERROR_SUCCESS) {

            // get size reqd to store away entire searchpath
            lResult = ::SHQueryValueEx(m_hKeySearchPath, szISP, NULL, &dwType,
                                /* get size */ NULL, &Size);

            if ( lResult == ERROR_SUCCESS) {

                if (Size == 0) {
                    // we don't check the CODE url in the case where there is a
                    // searchpath specified in the registry, but UseCodeURL is
                    // not one of the elements in the searchpath.
                    // This gives the client a choice to completely ignore the
                    // CODE URL if needed, but without specifying any other
                    // HTTP-POST url either
                    hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
                    goto Exit;
                }

                // alloc memory
                Size++;
                m_pSearchPath = new char [Size];
                if (m_pSearchPath) {
                    lResult = ::SHQueryValueEx(m_hKeySearchPath, szISP, NULL,
                                    &dwType, (unsigned char *)m_pSearchPath,
                                    &Size);
                    Assert(lResult == ERROR_SUCCESS);
                    m_pSearchPathNextComp = m_pSearchPath;
                } else {
                    lResult = E_OUTOFMEMORY;
                }
            }
        }

        if (lResult != ERROR_SUCCESS) {

            if (UsedCodeURL()) { // no searchpath, already used CODE url?
                hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
                goto Exit;
            }

            // no searchpath, use CODE=<url> in OBJECT tag
            SetupCODEUrl(ppDownloadURL,pextn);
            goto Exit;
        }
    }

    do {

        hr = GetNextComponent(szBuf, &m_pSearchPathNextComp);
        if (FAILED(hr))
            goto Exit;

        if (lstrcmpi(szBuf, sz_USE_CODE_URL) == 0) {

            if (UsedCodeURL()) { // already used CODE url?
                continue;
            }

            // use code=<url> in OBJECT tag
            SetupCODEUrl(ppDownloadURL,pextn);
            goto Exit;

        } else {
            break;
        }

    } while (TRUE);

    // here if HTTP-POST url
    MultiByteToWideChar(CP_ACP, 0, szBuf, -1, szURL, INTERNET_MAX_URL_LENGTH);

    *ppDownloadURL = szURL;

    // do POST: form the post data


    if (GetMainDistUnit()) {

        WideCharToMultiByte(CP_ACP, 0, GetMainDistUnit(), -1, szClsid, MAX_PATH, 0,0);
        wsprintf(szID, "CLSID=%s", szClsid);

    } else {

        // no clsid, dispatch the mime type or ext

        if (GetMainType()) {

            // type available
            WideCharToMultiByte(CP_ACP, 0, GetMainType(), -1, szClsid, MAX_PATH, 0,0);
            wsprintf(szID, "MIMETYPE=%s", szClsid);
            bMimeType = TRUE;

        } else {

            // ext
            Assert(GetMainExt());

            WideCharToMultiByte(CP_ACP, 0, GetMainExt(), -1, szClsid, MAX_PATH, 0,0);
            wsprintf(szID, "EXTENSION=%s", szClsid);
        }

    }

    cb = lstrlen(szID);

    // compute increased size if Version is specified.
    if (m_dwFileVersionMS || m_dwFileVersionLS) {

        wsprintf(szNeedVersion, "&%s=%d,%d,%d,%d",szVersion,
                    (m_dwFileVersionMS & 0xffff0000)>>16,
                    (m_dwFileVersionMS & 0xffff),
                    (m_dwFileVersionLS & 0xffff0000)>>16,
                    (m_dwFileVersionLS & 0xffff));

        cb += lstrlen(szNeedVersion);
    }

    if (bMimeType) {

        // hack the OBJECT index
        // it doesn't support query by mime type
        // so send out post data with the mime type in the CLSID=
        // we also need to escape the '/' if any in the mime type
        ComposeHackClsidFromMime(szHackMimeType, szClsid);
        cb += lstrlen(szHackMimeType);
    }

    hPostData = GlobalAlloc(GPTR, cb+1 ); // + 1 for null term

    if (!hPostData) {
        hr = HRESULT_FROM_WIN32(GetLastError()); // typically, E_OUTOFMEMORY
        goto ReleaseAndExit;
    }

    lstrcpy((char *)hPostData, szID);

    if (m_dwFileVersionMS || m_dwFileVersionLS)
        lstrcat( (char *)hPostData, szNeedVersion);

    if (bMimeType)
        lstrcat( (char *)hPostData, szHackMimeType);

    Assert(cb == (DWORD)lstrlen((char *)hPostData));

    *pcbPostData = cb;

ReleaseAndExit:


Exit:

    *phPostData = hPostData;

    return hr;

}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::IsPackageLocallyInstalled
// ---------------------------------------------------------------------------
HRESULT
CCodeDownload::IsPackageLocallyInstalled(LPCWSTR szPackageName, LPCWSTR szNameSpace, DWORD dwVersionMS, DWORD dwVersionLS)
{
    return ::IsPackageLocallyInstalled(&m_pPackageManager, szPackageName, szNameSpace, dwVersionMS, dwVersionLS);
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::DestroyPCBHList
// ---------------------------------------------------------------------------
void CCodeDownload::DestroyPCBHList(CList<CCodeBaseHold *, CCodeBaseHold *> *pcbhList)
{
    LISTPOSITION               lpos = 0;
    CCodeBaseHold             *pcbh = NULL;

    if (pcbhList) {
        lpos = pcbhList->GetHeadPosition();
        while (lpos) {
            pcbh = pcbhList->GetNext(lpos);
            delete pcbh;
        }
        pcbhList->RemoveAll();
    }
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::SetCatalogFile
// ---------------------------------------------------------------------------
HRESULT CCodeDownload::SetCatalogFile(LPSTR szCatalogFile)
{
    HRESULT                  hr = S_OK;

    SAFEDELETE(m_szCatalogFile);
    m_szCatalogFile = new char[lstrlen(szCatalogFile) + 1];
    if (m_szCatalogFile == NULL) {
        hr = E_OUTOFMEMORY;
    }
    else {
        lstrcpy(m_szCatalogFile, szCatalogFile);
    }

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::GetCatalogFile
// ---------------------------------------------------------------------------
LPSTR CCodeDownload::GetCatalogFile()
{
    HRESULT                  hr = S_OK;
    LPSTR                    szCatalogFile = NULL;
    CClBinding              *pBinding = NULL;
    IBindStatusCallback     *pBSC = NULL;
    IServiceProvider        *pServProv = NULL;
    LPCATALOGFILEINFO        pcfi = NULL;

    if (m_szCatalogFile) {
        szCatalogFile = m_szCatalogFile;
    }
    else {
        pBinding = m_pClientbinding.GetHead();
        if (pBinding) {
            pBSC = pBinding->GetAssBSC();
            if (pBSC) {
                hr = pBSC->QueryInterface(IID_ICatalogFileInfo, (void **)&pcfi);
                if (SUCCEEDED(hr)) {
                    pcfi->GetCatalogFile(&szCatalogFile);
                    m_szCatalogFile = szCatalogFile;
                    SAFERELEASE(pcfi);
                }
                else {
                    hr = pBSC->QueryInterface(IID_IServiceProvider, (void **)&pServProv);
                    if (SUCCEEDED(hr)) {
                        hr = pServProv->QueryService(IID_ICatalogFileInfo, IID_ICatalogFileInfo, (void **)&pcfi);
                        if (SUCCEEDED(hr)) {
                            pcfi->GetCatalogFile(&szCatalogFile);
                            m_szCatalogFile = szCatalogFile;
                        }
                        SAFERELEASE(pServProv);
                        SAFERELEASE(pcfi);
                    }
                }

            }
        }
    }

    return szCatalogFile;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::SetMainCABJavaTrustPermissions
// ---------------------------------------------------------------------------
HRESULT CCodeDownload::SetMainCABJavaTrustPermissions(PJAVA_TRUST pbJavaTrust)
{
    DWORD                            dwLen = 0;
    HRESULT                          hr = S_OK;

    if (pbJavaTrust && m_pbJavaTrust == NULL) { // only do this once

        // Clone the JAVA_TRUST object

        if (pbJavaTrust->cbSize) {
            m_pbJavaTrust = (PJAVA_TRUST)new BYTE[pbJavaTrust->cbSize];
            if (m_pbJavaTrust == NULL) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
        } else {
            m_pbJavaTrust = NULL;
            goto Exit;
        }

        memset(m_pbJavaTrust, 0, sizeof(JAVA_TRUST));

        m_pbJavaTrust->cbJavaPermissions = pbJavaTrust->cbJavaPermissions;
        if (pbJavaTrust->cbJavaPermissions) {
            m_pbJavaTrust->pbJavaPermissions = new BYTE[m_pbJavaTrust->cbJavaPermissions];
            if (m_pbJavaTrust->pbJavaPermissions == NULL) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            memcpy(m_pbJavaTrust->pbJavaPermissions, pbJavaTrust->pbJavaPermissions,
                   m_pbJavaTrust->cbJavaPermissions);
        }
        else {
            m_pbJavaTrust->pbJavaPermissions = NULL;
        }

        m_pbJavaTrust->cbSigner = pbJavaTrust->cbSigner;
        if (pbJavaTrust->cbSigner) {
            m_pbJavaTrust->pbSigner = new BYTE[m_pbJavaTrust->cbSigner];
            if (m_pbJavaTrust->pbSigner == NULL) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            memcpy(m_pbJavaTrust->pbSigner, pbJavaTrust->pbSigner,
                   m_pbJavaTrust->cbSigner);
        }
        else {
            m_pbJavaTrust->pbSigner = NULL;
        }


        // pbJavaTrust in IE4 had a bug where this zone URL is not NULL
        // terminated. Besides, we don't really require cloning the zone as we
        // don't use it to install. So, we are not cloning the zone url
        m_pbJavaTrust->pwszZone = NULL;


        m_pbJavaTrust->cbSize = pbJavaTrust->cbSize;
        m_pbJavaTrust->flag = pbJavaTrust->flag;
        m_pbJavaTrust->fAllActiveXPermissions = pbJavaTrust->fAllActiveXPermissions;
        m_pbJavaTrust->fAllPermissions = pbJavaTrust->fAllPermissions;
        m_pbJavaTrust->dwEncodingType = pbJavaTrust->dwEncodingType;
        m_pbJavaTrust->guidZone = pbJavaTrust->guidZone;
        m_pbJavaTrust->hVerify = pbJavaTrust->hVerify;
    }

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CCodeDownload::SetMainCABJavaTrustPermissions
// ---------------------------------------------------------------------------
PJAVA_TRUST CCodeDownload::GetJavaTrust()
{
    return m_pbJavaTrust;
}






HRESULT ProcessImplementation(IXMLElement *pConfig,
                              CList<CCodeBaseHold *, CCodeBaseHold *> *pcbhList,
                              LCID lcidOverride,
#ifdef WX86
                              CMultiArch *MultiArch,
#endif
                              LPWSTR szBaseURL)
{
    int nLastChildTag = -1;
    int nLastCodeBase = -1;
    int nLastOS = -1;
    int nLastProc = -1;
    OSVERSIONINFO osvi;
    BOOL fFoundAnyConfig = FALSE;
    BOOL fFoundAnyOS = FALSE, fFoundMatchingOS = FALSE;
    BOOL fFoundAnyProc = FALSE, fFoundMatchingProc = FALSE;
    IXMLElement *pCodeBase = NULL, *pLang = NULL, *pOS = NULL;
    IXMLElement *pOSVersion = NULL, *pProcessor = NULL;
    HRESULT hr = S_FALSE;               // default: failed configuration match
    BOOL bSetMainCodeBase = FALSE;
#ifdef WX86
    char *szPreferredArch;
    char *szAlternateArch;
    HRESULT hrArch;
#endif

    union {
        char szLang[MAX_PATH];
        char szOS[MAX_PATH];
        char szOSVersion[MAX_PATH];
        char szProcessor[MAX_PATH];
    };


    if (pcbhList == NULL)
        return E_INVALIDARG;

    pcbhList->RemoveAll();

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    // process LANGUAGES tag.
    if (GetFirstChildTag(pConfig, DU_TAG_LANG, &pLang) == S_OK) {

        if (SUCCEEDED(GetAttributeA(pLang, DU_ATTRIB_VALUE, szLang, MAX_PATH))) {

            if (FAILED(CheckLanguage(lcidOverride, szLang))) {

                if ((lcidOverride == g_lcidBrowser) ||
                    (FAILED(CheckLanguage(g_lcidBrowser, szLang)))) {
                        hr = S_FALSE;
                        goto Exit;

                }
            }

        } else {                // improperly formatted, skip it.
            hr = S_FALSE;
            goto Exit;
        }

    } // languages

    // process OS tag
    nLastOS = -1;
    fFoundAnyOS = FALSE;
    fFoundMatchingOS = FALSE;
    while (GetNextChildTag(pConfig, DU_TAG_OS, &pOS, nLastOS) == S_OK) {
        fFoundAnyOS = TRUE;

        if (SUCCEEDED(GetAttributeA(pOS, DU_ATTRIB_VALUE, szOS, MAX_PATH))) {

            if (lstrcmpi(szOS, (const char *) (g_fRunningOnNT) ? szWinNT : szWin95) == 0) {

                if (GetFirstChildTag(pOS, DU_TAG_OSVERSION, &pOSVersion) == S_OK) {

                    if (SUCCEEDED(GetAttributeA(pOSVersion, DU_ATTRIB_VALUE, szOSVersion, MAX_PATH))) {

                        DWORD dwVersionMS = 0, dwVersionLS = 0;

                        if (SUCCEEDED(GetVersionFromString(szOSVersion, &dwVersionMS, &dwVersionLS))) {
                            if (!((osvi.dwMajorVersion < (dwVersionMS>>16)) || (osvi.dwMajorVersion == (dwVersionMS>>16) &&
                                 (osvi.dwMinorVersion < (dwVersionMS & 0xFFFF))) )) {
                                fFoundMatchingOS = TRUE;
                                break;
                            }

                        } else {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                            goto Exit;
                        }

                    }
                } else {
                    // OS with no version
                    fFoundMatchingOS = TRUE;
                    break;
                }
            }
        }

        SAFERELEASE(pOS);
        SAFERELEASE(pOSVersion);
    }

    if (fFoundAnyOS && !fFoundMatchingOS) {
        hr = S_FALSE;
        goto Exit;
    }

    // check PROCESSOR tag
    nLastProc = -1;
    fFoundAnyProc = FALSE;
    fFoundMatchingProc = FALSE;
#ifdef WX86
    MultiArch->SelectArchitecturePreferences(
                g_szProcessorTypes[g_CPUType],
                g_szProcessorTypes[PROCESSOR_ARCHITECTURE_INTEL],
                &szPreferredArch,
                &szAlternateArch);
#endif

    while (GetNextChildTag(pConfig, DU_TAG_PROCESSOR, &pProcessor, nLastProc) == S_OK) {

        fFoundAnyProc = TRUE;
        if (SUCCEEDED(GetAttributeA(pProcessor, DU_ATTRIB_VALUE, szProcessor, MAX_PATH))) {

#ifdef WX86
            if (lstrcmpi(szPreferredArch, szProcessor) == 0) {
                hrArch = MultiArch->RequirePrimaryArch();
                Assert(SUCCEEDED(hrArch));
                fFoundMatchingProc = TRUE;
                break;
            } else if (szAlternateArch) {
                if (lstrcmpi(szAlternateArch, szProcessor) == 0) {
                    hrArch = MultiArch->RequireAlternateArch();
                    Assert(SUCCEEDED(hrArch));
                    fFoundMatchingProc = TRUE;
                    break;
                }
            }
#else
            if (lstrcmpi(g_szProcessorTypes[g_CPUType],szProcessor) == 0) {
                fFoundMatchingProc = TRUE;
                break;
            }
#endif
        }

        SAFERELEASE(pProcessor);
    }

    if (fFoundAnyProc && !fFoundMatchingProc) {
        hr = S_FALSE;
        goto Exit;
    }

    // process CODEBASE tag.
    nLastCodeBase = -1;
    bSetMainCodeBase = FALSE;
    while (GetNextChildTag(pConfig, DU_TAG_CODEBASE, &pCodeBase, nLastCodeBase) == S_OK) {

        hr = ProcessCodeBaseList(pCodeBase, pcbhList, szBaseURL);
        if (!bSetMainCodeBase) {
            CCodeBaseHold           *pcbhMain;

            pcbhMain = pcbhList->GetHead();
            if (pcbhMain) {
                pcbhMain->dwFlags |= CBH_FLAGS_MAIN_CODEBASE;
                bSetMainCodeBase = TRUE;
            }
        }

        SAFERELEASE(pCodeBase);

    }

    //REVIEW: We could also extract ABSTRACT, TITLE here

    // NEEDSTRUSTEDSOURCE & SYSTEM only apply to Java applets so they can be checked in
    // addition to this by ProcessJavaManifest.

    // we passed all configuration filter criteria
    hr = S_OK;

Exit:
    SAFERELEASE(pCodeBase);
    SAFERELEASE(pLang);
    SAFERELEASE(pOS);
    SAFERELEASE(pOSVersion);
    SAFERELEASE(pProcessor);

    return hr;
}

HRESULT ProcessCodeBaseList(IXMLElement *pCodeBase,
                            CList<CCodeBaseHold *, CCodeBaseHold *> *pcbhList,
                            LPWSTR szBaseURL)
{
    HRESULT                           hr = S_OK;
    DWORD                             dwSize = 0;
    CCodeBaseHold                    *pcbh = NULL;
    CCodeBaseHold                    *pcbhCur = NULL;
    LISTPOSITION                      lpos = 0;
    LPINTERNET_CACHE_ENTRY_INFO       lpCacheEntryInfo = NULL;
    LPSTR                             szCodeBase = NULL;
    BOOL                              bRandom = FALSE;
    int                               iIndex = 0;
    int                               iCount = 0;
    int                               i;
    int                               iLastIndexInCache;
    char                              achBuffer[MAX_CACHE_ENTRY_INFO_SIZE];
    char                              szRandom[MAX_PATH];
    WCHAR                             szResult[INTERNET_MAX_URL_LENGTH];

    union {
        char szSize[MAX_PATH];
        char szStyle[MAX_PATH];
    };

    if (!pcbhList) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    pcbh = new CCodeBaseHold();

    if (!pcbh) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (SUCCEEDED(hr = DupAttribute(pCodeBase, DU_ATTRIB_HREF, &pcbh->wszCodeBase))) {
        pcbh->bHREF = TRUE;
        if (szBaseURL) {
            dwSize = INTERNET_MAX_PATH_LENGTH;
            UrlCombineW(szBaseURL, pcbh->wszCodeBase, szResult, &dwSize, 0);
            delete pcbh->wszCodeBase;
            pcbh->wszCodeBase = new WCHAR[dwSize + 1];
            if (pcbh->wszCodeBase == NULL) {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            StrCpyW(pcbh->wszCodeBase, szResult);
        }

    } else if (SUCCEEDED(hr = DupAttribute(pCodeBase, DU_ATTRIB_FILENAME, &pcbh->wszCodeBase))) {
        pcbh->bHREF = FALSE;
    } else {
        SAFEDELETE(pcbh);
        goto Exit;
    }

    bRandom = FALSE;
    if (SUCCEEDED(GetAttributeA(pCodeBase, DU_ATTRIB_RANDOM, szRandom, MAX_PATH))) {
        if (szRandom[0] == 'y' || szRandom[0] == 'Y') {
            bRandom = TRUE;
        }
    }

    pcbh->wszDLGroup = NULL;
    pcbh->dwFlags &= ~CBH_FLAGS_DOWNLOADED;
    DupAttribute(pCodeBase, DU_ATTRIB_DL_GROUP, &pcbh->wszDLGroup);

    if (SUCCEEDED(GetAttributeA(pCodeBase, DU_ATTRIB_SIZE, szSize, MAX_PATH))) {
        pcbh->dwSize = StrToIntA(szSize);
    } else {
        pcbh->dwSize = -1;
    }

    pcbh->dwFlags &= ~CBH_FLAGS_MAIN_CODEBASE;

    // If the cache entry for this URL exists, put this at the head of the
    // list.

    lpCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)achBuffer;
    dwSize = MAX_CACHE_ENTRY_INFO_SIZE;
    if (SUCCEEDED(hr = Unicode2Ansi(pcbh->wszCodeBase, &szCodeBase))) {
        if (GetUrlCacheEntryInfo(szCodeBase, lpCacheEntryInfo,
                                 &dwSize)) {
            pcbhList->AddHead(pcbh);
            goto Exit;
        }
        SAFEDELETE(szCodeBase);
    } else {
        goto Exit;
    }

    if (bRandom) {

        // Set iLastIndexInCache to the last index in the linked list that
        // contains a cache entry. The goal is to ensure all cache entries
        // appear FIRST in the linked list.

        iLastIndexInCache = -1;
        lpos = pcbhList->GetHeadPosition();
        i = 0;
        while (lpos) {
            pcbhCur = pcbhList->GetNext(lpos);
            lpCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)achBuffer;
            dwSize = MAX_CACHE_ENTRY_INFO_SIZE;

            if (pcbhCur != NULL) {
                if (SUCCEEDED(Unicode2Ansi(pcbhCur->wszCodeBase,
                              &szCodeBase))) {
                    if (GetUrlCacheEntryInfo(szCodeBase, lpCacheEntryInfo,
                                             &dwSize)) {
                        iLastIndexInCache = i;
                    }
                    SAFEDELETE(szCodeBase);
                }
            }
            i++;
        }

        // Place codebase in list in a random order so that redundant codebases
        // can traverse list in order yet still achieve randomness

        iCount = pcbhList->GetCount();
        if (iCount) {
            // Generate random insertion index, x, in the range:
            // (iLastIndexInCache + 1) <= x < iCount

            if (iCount - iLastIndexInCache == 1) {
                // must add at tail, since last list entry == last cache entry
                pcbhList->AddTail(pcbh);
            } else {
                iIndex = (iLastIndexInCache + 1) + (randnum() % (iCount - iLastIndexInCache));
                if (iIndex == iCount) {
                    pcbhList->AddTail(pcbh);
                }
                else {
                    lpos = pcbhList->FindIndex(iIndex);
                    pcbhList->InsertBefore(lpos, pcbh);
                }
            }
        }
        else {
            pcbhList->AddTail(pcbh);
        }
    }
    else {
        // Not random, just ad as tail
        pcbhList->AddTail(pcbh);
    }

Exit:
    SAFEDELETE(szCodeBase);

    return hr;
}

#ifdef WX86
// ---------------------------------------------------------------------------
// %%Function: CMultiArch::RequirePrimaryArch
// ---------------------------------------------------------------------------
HRESULT CMultiArch::RequirePrimaryArch()
{
    if (m_RequiredArch != PROCESSOR_ARCHITECTURE_UNKNOWN &&
        m_RequiredArch != (DWORD)g_CPUType) {
        //
        // The required arch has already been set for this download.
        // The download to change the required arch in the middle or
        // else a control and its support pieces may end up getting
        // different architectures.
        //
        return E_FAIL;
    }
    m_RequiredArch = (DWORD)g_CPUType;
    return S_OK;
}

// ---------------------------------------------------------------------------
// %%Function: CMultiArch::RequireAlternateArch
// ---------------------------------------------------------------------------
HRESULT CMultiArch::RequireAlternateArch()
{
    if (m_RequiredArch != PROCESSOR_ARCHITECTURE_UNKNOWN &&
        m_RequiredArch != PROCESSOR_ARCHITECTURE_INTEL) {
        //
        // The required arch has already been set for this download.
        // The download to change the required arch in the middle or
        // else a control and its support pieces may end up getting
        // different architectures.
        //
        return E_FAIL;
    }
    m_RequiredArch = PROCESSOR_ARCHITECTURE_INTEL;
    return S_OK;
}

// ---------------------------------------------------------------------------
// %%Function: CMultiArch::SelectArchitecturePreferences
// ---------------------------------------------------------------------------
VOID
CMultiArch::SelectArchitecturePreferences(
    char *szNativeArch,
    char *szIntelArch,
    char **pszPreferredArch,
    char **pszAlternateArch
    )
{
    if (g_fWx86Present) {
        switch (m_RequiredArch) {
        case PROCESSOR_ARCHITECTURE_INTEL:
            // An i386 binary has already been downloaded.  Only download
            // i386 binaries now.
            *pszPreferredArch = szIntelArch;
            *pszAlternateArch = NULL;
            break;

        case PROCESSOR_ARCHITECTURE_UNKNOWN:
            // No binaries downloaded so far.  Prefer native and fallback
            // to i386
            *pszPreferredArch = szNativeArch;
            *pszAlternateArch = szIntelArch;
            break;

        default:
            // A native binary has already been downloaded.  Only download
            // native binaries now.
            *pszPreferredArch = szNativeArch;
            *pszAlternateArch = NULL;
        }
    } else {
        // No Wx86
        *pszPreferredArch = szNativeArch;
        *pszAlternateArch = NULL;
    }
}
#endif

#include <cdlpch.h>
#include <softpub.h>

#define WINTRUST TEXT("wintrust.dll")

#ifdef DELAY_LOAD_WVT

#ifndef _WVTP_NOCODE_
Cwvt::Cwvt()
{
    m_fInited = FALSE;
    m_bHaveWTData = FALSE;
}

Cwvt::~Cwvt()
{
    if (m_fInited) {
        FreeLibrary(m_hMod);
    }
}

HRESULT 
Cwvt::Init(void)
{
    GUID PublishedSoftware = WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
    GUID *ActionID = &PublishedSoftware;


    if (m_fInited) {
        return S_OK;
    }

    m_hMod = LoadLibrary( WINTRUST );

    if (NULL == m_hMod) {
        return (HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND));
    }


#define CHECKAPI(_fn) \
    *(FARPROC*)&(_pfn##_fn) = GetProcAddress(m_hMod, #_fn); \
    if (!(_pfn##_fn)) { \
        FreeLibrary(m_hMod); \
        return (HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)); \
    }

    CHECKAPI(WinVerifyTrust);

    m_fInited = TRUE;
    return S_OK;
}

BOOL
IsUIRestricted()
{

    HKEY hkeyRest = 0;
    BOOL bUIRest = FALSE;
    DWORD dwValue = 0;
    DWORD dwLen = sizeof(DWORD);

    // per-machine UI off policy
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_INFODEL_REST, 0, KEY_READ, &hkeyRest) == ERROR_SUCCESS) {

        if (RegQueryValueEx( hkeyRest, REGVAL_UI_REST, NULL, NULL,
                      (LPBYTE)&dwValue, &dwLen) == ERROR_SUCCESS && dwValue)
            bUIRest = TRUE;

        RegCloseKey(hkeyRest);
    }

    return bUIRest;
}

#endif // _WVTP_NOCODE_
#endif // DELAY_LOAD_WVT

// {D41E4F1D-A407-11d1-8BC9-00C04FA30A41}
#define COR_POLICY_PROVIDER_DOWNLOAD \
{ 0xd41e4f1d, 0xa407, 0x11d1, {0x8b, 0xc9, 0x0, 0xc0, 0x4f, 0xa3, 0xa, 0x41 } }

#define ZEROSTRUCT(arg)  memset( &arg, 0, sizeof(arg))

HRESULT Cwvt::VerifyTrust(HANDLE hFile, HWND hWnd, PJAVA_TRUST *ppJavaTrust,
                          LPCWSTR szStatusText,
                          IInternetHostSecurityManager *pHostSecurityManager,
                          LPSTR szFilePath, LPSTR szCatalogFile,
                          CDownload *pdl) 
{
    LPWSTR                   wzFileName = NULL;
    LPWSTR                   wzFilePath = NULL;
    LPWSTR                   wzCatalogFile = NULL;
    GUID                     guidJava = JAVA_POLICY_PROVIDER_DOWNLOAD;
    GUID                     guidCor = COR_POLICY_PROVIDER_DOWNLOAD;
    GUID                     guidAuthenticode = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    GUID                    *pguidActionIDJava = &guidJava;
    GUID                    *pguidActionIDCor = &guidCor;
    WINTRUST_DATA            wintrustData;
    WINTRUST_DATA            wtdAuthenticode;
    WINTRUST_FILE_INFO       fileData;
    JAVA_POLICY_PROVIDER     javaPolicyData;
    WCHAR                    wpath [MAX_PATH];
    PJAVA_TRUST              pbJavaTrust = NULL;
    IServiceProvider        *pServProv = NULL;
    LPCATALOGFILEINFO        pcfi = NULL;
    HRESULT                  hr = S_OK;

    ZEROSTRUCT(wintrustData);
    ZEROSTRUCT(fileData);
    ZEROSTRUCT(javaPolicyData);

    javaPolicyData.cbSize = sizeof(JAVA_POLICY_PROVIDER);
    javaPolicyData.VMBased = FALSE;
    javaPolicyData.fNoBadUI = FALSE;

    javaPolicyData.pwszZone = szStatusText;
    javaPolicyData.pZoneManager = (LPVOID)pHostSecurityManager;


    fileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileData.pcwszFilePath = szStatusText;
    fileData.hFile = hFile;

    wintrustData.cbStruct = sizeof(WINTRUST_DATA);
    wintrustData.pPolicyCallbackData = &javaPolicyData;

    if ( (hWnd == INVALID_HANDLE_VALUE) || IsUIRestricted())
        wintrustData.dwUIChoice = WTD_UI_NONE;
    else
        wintrustData.dwUIChoice = WTD_UI_ALL;

    wintrustData.dwUnionChoice = WTD_CHOICE_FILE;
    wintrustData.pFile = &fileData;

    if (szCatalogFile) {
        ::Ansi2Unicode(szCatalogFile, &wzCatalogFile);
        ::Ansi2Unicode(szFilePath, &wzFilePath);
        wzFileName = PathFindFileNameW(szStatusText);

        if (!m_bHaveWTData) {
            memset(&m_wtCatalogInfo, 0x0, sizeof(m_wtCatalogInfo));
            m_wtCatalogInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
            m_bHaveWTData = TRUE;
        }
        m_wtCatalogInfo.pcwszCatalogFilePath = wzCatalogFile;
        m_wtCatalogInfo.pcwszMemberTag = wzFileName;
        m_wtCatalogInfo.pcwszMemberFilePath = wzFilePath;

        wtdAuthenticode = wintrustData;
        wtdAuthenticode.pCatalog = &m_wtCatalogInfo;
        wtdAuthenticode.dwUnionChoice = WTD_CHOICE_CATALOG;
        wtdAuthenticode.dwStateAction = WTD_STATEACTION_VERIFY;
        wtdAuthenticode.dwUIChoice = WTD_UI_NONE;

        hr = WinVerifyTrust(hWnd, &guidAuthenticode, &wtdAuthenticode);
        if (FAILED(hr)) {
            hr = WinVerifyTrust(hWnd, pguidActionIDCor, &wintrustData);

            if (hr == TRUST_E_PROVIDER_UNKNOWN)
                hr = WinVerifyTrust(hWnd, pguidActionIDJava, &wintrustData);
        }
        else {
            // Clone Java permissions
            pbJavaTrust = pdl->GetCodeDownload()->GetJavaTrust();
            if (!pbJavaTrust) {
                hr = pdl->GetBSC()->QueryInterface(IID_IServiceProvider, (void **)&pServProv);
                if (SUCCEEDED(hr)) {
                    hr = pServProv->QueryService(IID_ICatalogFileInfo, IID_ICatalogFileInfo, (void **)&pcfi);
                    if (SUCCEEDED(hr)) {
                        pcfi->GetJavaTrust((void **)&pbJavaTrust);
                    }
                }
                SAFERELEASE(pServProv);
                SAFERELEASE(pcfi);
                pdl->SetMainCABJavaTrustPermissions(pbJavaTrust);
            }
        }
            
    }
    else {
        hr =  WinVerifyTrust(hWnd, pguidActionIDCor, &wintrustData);
        if (hr == TRUST_E_PROVIDER_UNKNOWN)
            hr = WinVerifyTrust(hWnd, pguidActionIDJava, &wintrustData);

        if (SUCCEEDED(hr)) {
            pdl->SetMainCABJavaTrustPermissions(javaPolicyData.pbJavaTrust);
        }
    }

    SAFEDELETE(wzCatalogFile);
    SAFEDELETE(wzFilePath);

    // BUGBUG: this works around a wvt bug that returns 0x57 (success) when
    // you hit No to an usigned control
    if (SUCCEEDED(hr) && hr != S_OK) {
        hr = TRUST_E_FAIL;
    }

    if (FAILED(hr)) {
        // display original hr intact to help debugging
//        CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_VERIFYTRUST_FAILED, hr);
    } else {
        *ppJavaTrust = javaPolicyData.pbJavaTrust;
    }
    if (hr == TRUST_E_SUBJECT_NOT_TRUSTED && wintrustData.dwUIChoice == WTD_UI_NONE) {
        // if we didn't ask for the UI to be out up there has been no UI
        // work around WVT bvug that it returns us this special error code
        // without putting up UI.

        hr = TRUST_E_FAIL; // this will put up mshtml ui after the fact
                           // that security settings prevented us
    }

    if (FAILED(hr) && (hr != TRUST_E_SUBJECT_NOT_TRUSTED)) {

        // trust system has failed without UI

        // map error to this generic error that will falg our client to put
        // up additional info that this is a trust system error if reqd.
        hr = TRUST_E_FAIL;
    }

    if (hr == TRUST_E_SUBJECT_NOT_TRUSTED) {

        pdl->GetCodeDownload()->SetUserDeclined();
    }


    return hr;
}

HRESULT GetActivePolicy(IInternetHostSecurityManager* pZoneManager, 
                        LPCWSTR pwszZone,
                        DWORD  dwUrlAction,
                        DWORD& dwPolicy)
{
    HRESULT hr = TRUST_E_FAIL;
    HRESULT hr2 = TRUST_E_FAIL;
    DWORD cbPolicy = sizeof(DWORD);

    // Policy are ordered such that high numbers are the most conservative 
    // and the lower numbers are less conservative

    DWORD dwDocumentPolicy = URLPOLICY_ALLOW;
    DWORD dwUrlPolicy = URLPOLICY_ALLOW;
    
    // We are going for the most conservative so lets set the 
    // value to the least conservative
    dwPolicy = URLPOLICY_ALLOW;

    IInternetSecurityManager* iSM = NULL;

    // Ask the document base for its policy
    if(pZoneManager) { //  Given a IInternetHostSecurityManager
        hr = pZoneManager->ProcessUrlAction(dwUrlAction,
                                            (PBYTE) &dwDocumentPolicy,
                                            cbPolicy,
                                            NULL,
                                            0,
                                            PUAF_NOUI,
                                            0);
    }
    
    // Get the policy for the URL 
    if(pwszZone) { // Create an IInternetSecurityManager
        hr2 = CoInternetCreateSecurityManager(NULL,
                                              &iSM,
                                              0);
        if(hr2 == S_OK) { // We got the manager so get the policy info
            hr2 = iSM->ProcessUrlAction(pwszZone,
                                        dwUrlAction,
                                        (PBYTE) &dwUrlPolicy,
                                        cbPolicy,
                                        NULL,
                                        0,
                                        PUAF_NOUI,
                                        0);
            iSM->Release();
        }
        else 
            iSM = NULL;
    }

    // if they both failed and we have zones then set it to deny and return an error
    if(FAILED(hr) && FAILED(hr2)) {
        // If we failed because there are on zones then lets QUERY
        // BUGBUG: we should actually try to get the IE30 security policy here.
        if(iSM == NULL && pZoneManager == NULL) {
            dwPolicy = URLPOLICY_QUERY;
            hr = S_OK;
        }
        else {
            dwPolicy = URLPOLICY_DISALLOW;
            hr = TRUST_E_FAIL;
        }
    }
    else {
        if(SUCCEEDED(hr))
            dwPolicy = dwDocumentPolicy;
        if(SUCCEEDED(hr2))
            dwPolicy = dwPolicy > dwUrlPolicy ? dwPolicy : dwUrlPolicy;

        if (dwPolicy == URLPOLICY_DISALLOW)
            hr = TRUST_E_FAIL;
        else
            hr = S_OK;
    }
    return hr;
}

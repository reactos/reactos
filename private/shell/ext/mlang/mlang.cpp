#include "private.h"
#include "mlmain.h"
#include "cpdetect.h"
#include "codepage.h"

STDAPI CMultiLanguage::GetNumberOfCodePageInfo(UINT *pcCodePage)
{
    if (NULL != m_pMimeDatabase)
        return m_pMimeDatabase->GetNumberOfCodePageInfo(pcCodePage);
    else
        return E_FAIL;
}

STDAPI CMultiLanguage::GetCodePageInfo(UINT uiCodePage, PMIMECPINFO pcpInfo)
{

    if (NULL != m_pMimeDatabase)
        return m_pMimeDatabase->GetCodePageInfo(uiCodePage, GetSystemDefaultLangID(), pcpInfo);
    else
        return E_FAIL;
}

STDAPI CMultiLanguage::GetFamilyCodePage(UINT uiCodePage, UINT *puiFamilyCodePage)
{
        HRESULT hr = S_OK;
        int idx = 0;

        DebugMsg(DM_TRACE, TEXT("CMultiLanguage::GetFamilyCodePage called."));

        while(MimeCodePage[idx].uiCodePage)
        {
            if ((uiCodePage == MimeCodePage[idx].uiCodePage) &&
                (MimeCodePage[idx].dwFlags & dwMimeSource))
                break;
            idx++;
        }

        if (MimeCodePage[idx].uiCodePage)
        {
            if (MimeCodePage[idx].uiFamilyCodePage)
                *puiFamilyCodePage = MimeCodePage[idx].uiFamilyCodePage;
            else
                *puiFamilyCodePage = uiCodePage;
        }
        else
        {
            hr = E_FAIL;
            *puiFamilyCodePage = 0;
        }
        return hr;
}

STDAPI CMultiLanguage::EnumCodePages(DWORD grfFlags, IEnumCodePage **ppEnumCodePage)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::EnumCodePages called."));
    *ppEnumCodePage = NULL;

    // Return IE4 MIME DB data in IMultiLanguage    
    CEnumCodePage *pCEnumCodePage = new CEnumCodePage(grfFlags, GetSystemDefaultLangID(), MIMECONTF_MIME_IE4);

    if (NULL != pCEnumCodePage)
    {
        HRESULT hr = pCEnumCodePage->QueryInterface(IID_IEnumCodePage, (void**)ppEnumCodePage);
        pCEnumCodePage->Release();
        return hr;
    }
    return E_OUTOFMEMORY;
}

STDAPI CMultiLanguage2::EnumCodePages(DWORD grfFlags, LANGID LangId, IEnumCodePage **ppEnumCodePage)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::EnumCodePages called."));
    *ppEnumCodePage = NULL;

    CEnumCodePage *pCEnumCodePage = new CEnumCodePage(grfFlags, LangId, dwMimeSource);
    if (NULL != pCEnumCodePage)
    {
        HRESULT hr = pCEnumCodePage->QueryInterface(IID_IEnumCodePage, (void**)ppEnumCodePage);
        pCEnumCodePage->Release();
        return hr;
    }
    return E_OUTOFMEMORY;
}

STDAPI CMultiLanguage2::EnumScripts(DWORD dwFlags, LANGID LangId, IEnumScript **ppEnumScript)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage2::EnumScripts called."));
    *ppEnumScript = NULL;

    CEnumScript *pCEnumScript = new CEnumScript(dwFlags, LangId, dwMimeSource);
    if (NULL != pCEnumScript)
    {
        HRESULT hr = pCEnumScript->QueryInterface(IID_IEnumScript, (void**)ppEnumScript);
        pCEnumScript->Release();
        return hr;
    }
    return E_OUTOFMEMORY;
}

STDAPI CMultiLanguage::GetCharsetInfo(BSTR Charset, PMIMECSETINFO pcsetInfo)
{
    if (NULL != m_pMimeDatabase)
        return m_pMimeDatabase->GetCharsetInfo(Charset, pcsetInfo);
    else
        return E_FAIL;
}

STDAPI CMultiLanguage::IsConvertible(DWORD dwSrcEncoding, DWORD dwDstEncoding)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::IsConvertINetStringAvailable called."));
    return IsConvertINetStringAvailable(dwSrcEncoding, dwDstEncoding);
}

STDAPI CMultiLanguage::ConvertString(LPDWORD lpdwMode, DWORD dwSrcEncoding, DWORD dwDstEncoding, BYTE *pSrcStr, UINT *pcSrcSize, BYTE *pDstStr, UINT *pcDstSize)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::ConvertStringEx called."));
    return ConvertINetString(lpdwMode, dwSrcEncoding, dwDstEncoding, (LPCSTR)pSrcStr, (LPINT)pcSrcSize, (LPSTR)pDstStr, (LPINT)pcDstSize);
}

STDAPI CMultiLanguage::ConvertStringToUnicode(LPDWORD lpdwMode, DWORD dwEncoding, CHAR *pSrcStr, UINT *pcSrcSize, WCHAR *pDstStr, UINT *pcDstSize)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::ConvertStringToUnicode called."));
    return ConvertINetMultiByteToUnicode(lpdwMode, dwEncoding, (LPCSTR)pSrcStr, (LPINT)pcSrcSize, (LPWSTR)pDstStr, (LPINT)pcDstSize);
}

STDAPI CMultiLanguage::ConvertStringFromUnicode(LPDWORD lpdwMode, DWORD dwEncoding, WCHAR *pSrcStr, UINT *pcSrcSize, CHAR *pDstStr, UINT *pcDstSize)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::ConvertStringFromUnicode called."));
    return ConvertINetUnicodeToMultiByte(lpdwMode, dwEncoding, (LPCWSTR)pSrcStr, (LPINT)pcSrcSize, (LPSTR)pDstStr, (LPINT)pcDstSize);
}

STDAPI CMultiLanguage::ConvertStringReset(void)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::Reset called."));
    return ConvertINetReset();
}

STDAPI CMultiLanguage::GetRfc1766FromLcid(LCID Locale, BSTR *pbstrRfc1766)
{
    HRESULT hr = E_INVALIDARG;

    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::GetRfc1766FromLcid called."));

    if (NULL != pbstrRfc1766)
    {
        WCHAR wsz[MAX_RFC1766_NAME];

        hr = LcidToRfc1766W(Locale, wsz, ARRAYSIZE(wsz));
        if (SUCCEEDED(hr))
            *pbstrRfc1766 = SysAllocString(wsz);
        else
            *pbstrRfc1766 = NULL;
    }
    return hr;
}

STDAPI CMultiLanguage::GetLcidFromRfc1766(PLCID pLocale, BSTR bstrRfc1766)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::GetLcidFromRfc1766 called."));
    return Rfc1766ToLcidW(pLocale, bstrRfc1766);   
}

STDAPI CMultiLanguage::EnumRfc1766(IEnumRfc1766 **ppEnumRfc1766)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::EnumRfc1766 called."));
    *ppEnumRfc1766 = NULL;

    CEnumRfc1766 *pCEnumRfc1766 = new CEnumRfc1766(dwMimeSource,GetSystemDefaultLangID());
    if (NULL != pCEnumRfc1766)
    {
        HRESULT hr = pCEnumRfc1766->QueryInterface(IID_IEnumRfc1766, (void**)ppEnumRfc1766);
        pCEnumRfc1766->Release();
        return hr;
    }
    return E_OUTOFMEMORY;
}

STDAPI CMultiLanguage2::EnumRfc1766(LANGID LangId, IEnumRfc1766 **ppEnumRfc1766)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::EnumRfc1766 called."));
    *ppEnumRfc1766 = NULL;

    CEnumRfc1766 *pCEnumRfc1766 = new CEnumRfc1766(dwMimeSource, LangId);
    if (NULL != pCEnumRfc1766)
    {
        HRESULT hr = pCEnumRfc1766->QueryInterface(IID_IEnumRfc1766, (void**)ppEnumRfc1766);
        pCEnumRfc1766->Release();
        return hr;
    }
    return E_OUTOFMEMORY;
}

STDAPI CMultiLanguage::GetRfc1766Info(LCID Locale, PRFC1766INFO pRfc1766Info)
{
    UINT i;
    HRESULT hr = E_INVALIDARG;

    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::GetRfc1766Info called."));


    if (NULL != pRfc1766Info)
    {
        for (i = 0; i < g_cRfc1766; i++)
        {
            if (MimeRfc1766[i].LcId == Locale)
                break;
        }
        if (i < g_cRfc1766)
        {
            pRfc1766Info->lcid = MimeRfc1766[i].LcId;
            MLStrCpyNW(pRfc1766Info->wszRfc1766, MimeRfc1766[i].szRfc1766, MAX_RFC1766_NAME);
            _LoadStringExW(g_hInst, MimeRfc1766[i].uidLCID, pRfc1766Info->wszLocaleName, 
                 MAX_LOCALE_NAME, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
            hr = S_OK;
        }
        else
            hr = E_FAIL;
    }
    return hr;
}

STDAPI CMultiLanguage2::GetRfc1766Info(LCID Locale, LANGID LangId, PRFC1766INFO pRfc1766Info)
{
    UINT i;
    HRESULT hr = E_INVALIDARG;

    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::GetRfc1766Info called."));


    if (NULL != pRfc1766Info)
    {
        for (i = 0; i < g_cRfc1766; i++)
        {
            if (MimeRfc1766[i].LcId == Locale)
                break;
        }
        if (i < g_cRfc1766)
        {
            if (!LangId)
                LangId = GetSystemDefaultLangID();

            pRfc1766Info->lcid = MimeRfc1766[i].LcId;
            MLStrCpyNW(pRfc1766Info->wszRfc1766, MimeRfc1766[i].szRfc1766, MAX_RFC1766_NAME);

            if (!_LoadStringExW(g_hInst, MimeRfc1766[i].uidLCID, pRfc1766Info->wszLocaleName, 
                 MAX_LOCALE_NAME, LangId))
            {
                    _LoadStringExW(g_hInst, MimeRfc1766[i].uidLCID, pRfc1766Info->wszLocaleName, 
                         MAX_LOCALE_NAME, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
            }
            hr = S_OK;
        }
        else
            hr = E_FAIL;
    }
    return hr;
}

STDAPI CMultiLanguage::CreateConvertCharset(UINT uiSrcCodePage, UINT uiDstCodePage, DWORD dwProperty, IMLangConvertCharset **ppMLangConvertCharset)
{
    HRESULT hr;
    IClassFactory* pClassObj;

    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::CreateCharsetConvert called."));

    if (SUCCEEDED(hr = _Module.GetClassObject(CLSID_CMLangConvertCharset, IID_IClassFactory, (void**)&pClassObj)))
    {
        hr = pClassObj->CreateInstance(NULL, IID_IMLangConvertCharset, (void**)ppMLangConvertCharset);
        pClassObj->Release();
    }

    if (ppMLangConvertCharset && FAILED(hr))
        *ppMLangConvertCharset = NULL;

    if (NULL != *ppMLangConvertCharset)
        hr = (*ppMLangConvertCharset)->Initialize(uiSrcCodePage, uiDstCodePage, dwProperty);

    return hr;
}

STDAPI CMultiLanguage2::ConvertStringInIStream(LPDWORD lpdwMode, DWORD dwFlag, WCHAR *lpFallBack, DWORD dwSrcEncoding, DWORD dwDstEncoding, IStream *pstmIn, IStream *pstmOut)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage2::ConvertStringInIStream called."));
    return ConvertINetStringInIStream(lpdwMode,dwSrcEncoding,dwDstEncoding,pstmIn,pstmOut,dwFlag,lpFallBack);
}

STDAPI CMultiLanguage2::ConvertStringToUnicodeEx(LPDWORD lpdwMode, DWORD dwEncoding, CHAR *pSrcStr, UINT *pcSrcSize, WCHAR *pDstStr, UINT *pcDstSize, DWORD dwFlag, WCHAR *lpFallBack)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage2::ConvertBufferStringToUnicodeEx called."));
    return ConvertINetMultiByteToUnicodeEx(lpdwMode, dwEncoding, (LPCSTR)pSrcStr, (LPINT)pcSrcSize, (LPWSTR)pDstStr, (LPINT)pcDstSize, dwFlag, lpFallBack);
}

STDAPI CMultiLanguage2::ConvertStringFromUnicodeEx(LPDWORD lpdwMode, DWORD dwEncoding, WCHAR *pSrcStr, UINT *pcSrcSize, CHAR *pDstStr, UINT *pcDstSize, DWORD dwFlag, WCHAR *lpFallBack)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage2::ConvertBufferStringFromUnicodeEx called."));
    return ConvertINetUnicodeToMultiByteEx(lpdwMode, dwEncoding, (LPCWSTR)pSrcStr, (LPINT)pcSrcSize, (LPSTR)pDstStr, (LPINT)pcDstSize, dwFlag, lpFallBack);
}

STDAPI CMultiLanguage2::DetectCodepageInIStream(DWORD dwFlag, DWORD uiPrefWinCodepage, IStream *pstmIn, DetectEncodingInfo *lpEncoding, INT *pnScores)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage2::DetectCodepageInIStream called. "));
    return _DetectCodepageInIStream(dwFlag, uiPrefWinCodepage, pstmIn, lpEncoding, pnScores);
}

STDAPI CMultiLanguage2::DetectInputCodepage(DWORD dwFlag, DWORD uiPrefWinCodepage, CHAR *pSrcStr, INT *pcSrcSize, DetectEncodingInfo *lpEncoding, INT *pnScores)
{
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage2::DetectInputCodepage called. "));
    return _DetectInputCodepage(dwFlag, uiPrefWinCodepage, pSrcStr, pcSrcSize, lpEncoding, pnScores);
}

STDAPI CMultiLanguage2::ValidateCodePage(UINT uiCodePage, HWND hwnd)
{
    return ValidateCodePageEx(uiCodePage, hwnd, 0);
}
// this is private function to serve both for IML2 and IML3
STDAPI CMultiLanguage2::ValidateCodePageEx(UINT uiCodePage, HWND hwnd, DWORD dwfIODControl)
{
    MIMECPINFO cpInfo;
    CLSID      clsid;
    UINT       uiFamCp;
    HRESULT    hr;
    
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage2::ValidateCodePage called. "));
    
    if (NULL != g_pMimeDatabase)
        hr = g_pMimeDatabase->GetCodePageInfo(uiCodePage, 0x409, &cpInfo);
    else
        hr = E_OUTOFMEMORY;

    if (FAILED(hr))
        return E_INVALIDARG;

    EnterCriticalSection(&g_cs);
    if (NULL == g_pCpMRU)
        if (g_pCpMRU = new CCpMRU)
            g_pCpMRU->Init();
    LeaveCriticalSection(&g_cs);

    if (g_pCpMRU && g_pCpMRU->dwCpMRUEnable)
        g_pCpMRU->UpdateCPMRU(uiCodePage);

    if (cpInfo.dwFlags & MIMECONTF_VALID)
        return S_OK;

    // always handle family codepage because a caller
    // of this function is not generally aware if
    // the codepage is primary one. i.e., they can
    // call with cp=20268 to validate the entire 1251
    // family.
    //
    uiFamCp = cpInfo.uiFamilyCodePage;

    
    // Ignore IOD check on NT5
    if (g_bIsNT5)
    {
        // Currently, NT5 doesn't install 20127 and 28605 NLS files.
        // We should prevent langpack installation loop and let clients resolve 
        // them with CP_ACP in case of 20127 and 28605 validation.
        // This hack can be removed once NT5 bundles these NLS files by default.
        if ((uiCodePage == CP_20127 || uiCodePage == CP_ISO_8859_15) && IsValidCodePage(uiFamCp))
            return E_FAIL;
        hr = IsNTLangpackAvailable(uiFamCp);
        if (hr != S_OK)
            return hr;
    }
    else
    {
        // check if JIT langpack is enabled.
        //
        hr = EnsureIEStatus();
        if (hr == S_OK) 
        {
            if (!m_pIEStat || m_pIEStat->IsJITEnabled() != TRUE)
            {
                // the codepage is neither valid or installable
                return S_FALSE;
            }
        }
    }    

    if (hwnd == NULL)
    {
        hwnd = GetForegroundWindow();   
    }

    // Special handling for NT.
    if (g_bIsNT5)
    {
        DWORD   dwInstallLpk = 1;
        HKEY    hkey;
        DWORD   dwAction = 0;

        // HKCR\\Software\\Microsoft\internet explorer\\international
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, 
                         REGSTR_PATH_INTERNATIONAL,
                         NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dwAction)) 
        {
            DWORD dwType = REG_DWORD;
            DWORD dwSize = sizeof(DWORD);

            if (ERROR_SUCCESS != RegQueryValueEx(hkey, REG_KEY_NT5LPK, 0, &dwType, (LPBYTE)&dwInstallLpk, &dwSize))
            {
                dwInstallLpk = 1;
                RegSetValueEx(hkey, REG_KEY_NT5LPK, 0, REG_DWORD, (LPBYTE)&dwInstallLpk, sizeof(dwInstallLpk));
            }
            RegCloseKey(hkey);
        }

        hr = S_FALSE;

        // Pops up NT5 langpack dialog box if langpack is enabled or user selects it from encoding menu
        if (dwInstallLpk || (dwfIODControl & CPIOD_FORCE_PROMPT))
        {
            LPCDLGTEMPLATE pTemplate;
            HRSRC   hrsrc;
            int     iRet;
            LANGID  LangId = GetNT5UILanguage();

            dwInstallLpk |= uiFamCp << 16;

            // Load correct resource to match NT5 UI language
            hrsrc = FindResourceExW(g_hInst, (LPCWSTR) RT_DIALOG, (LPCWSTR) MAKEINTRESOURCE(IDD_DIALOG_LPK), LangId);

            // Pack LPARAM, code page value in HIWORD, installation flag in LOWORD
            if (hrsrc &&
                (pTemplate = (LPCDLGTEMPLATE)LoadResource(g_hInst, hrsrc)))
            {
                iRet = (int) DialogBoxIndirectParamW(g_hInst, pTemplate,
                   hwnd, LangpackDlgProc, (LPARAM) dwInstallLpk);
            }
            else 
                iRet = (int) DialogBoxParamW(g_hInst, (LPCWSTR) MAKEINTRESOURCE(IDD_DIALOG_LPK), hwnd, LangpackDlgProc, (LPARAM) dwInstallLpk); 
            
            if (iRet)
            {
                hr = _InstallNT5Langpack(hwnd, uiFamCp);
                if (S_OK == hr)
                {                
                    WCHAR wszLangInstall[MAX_PATH];
                    WCHAR wszNT5LangPack[1024];
                    

                    // Fall back to English (US) if we don't have a specific language resource
                    if (!_LoadStringExW(g_hInst, IDS_LANGPACK_INSTALL, wszLangInstall, ARRAYSIZE(wszLangInstall), LangId) ||
                        !_LoadStringExW(g_hInst, IDS_NT5_LANGPACK, wszNT5LangPack, ARRAYSIZE(wszNT5LangPack), LangId))
                    {
                        LangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
                        _LoadStringExW(g_hInst, IDS_LANGPACK_INSTALL, wszLangInstall, ARRAYSIZE(wszLangInstall), LangId);
                        _LoadStringExW(g_hInst, IDS_NT5_LANGPACK, wszNT5LangPack, ARRAYSIZE(wszNT5LangPack), LangId);
                    }
                
                    MessageBoxW(hwnd, wszNT5LangPack, wszLangInstall, MB_OK);
                }
            }
        }


        goto SKIP_IELANGPACK;
    }

    // Initiate JIT using CLSID give to the langpack
    hr = _GetJITClsIDForCodePage(uiFamCp, &clsid);
    if (SUCCEEDED(hr))
    {
        hr = InstallIEFeature(hwnd, &clsid, dwfIODControl);
    }

SKIP_IELANGPACK:  
    // if JIT returns S_OK, we now have everything installed
    // then we'll validate the codepage and add font
    // NOTE: there can be more than codepage to validate here,
    //      for example, PE langpack contains more than one 
    //      NLS file to get greek, cyrillic and Turkish at the
    //      same time.
    if (hr == S_OK)
    {
        hr = _ValidateCPInfo(uiFamCp);
        if (SUCCEEDED(hr))
        {
            _AddFontForCP(uiFamCp);
        }
    }
    
    return hr;
}


// IMultiLanguage2::GetCodePageDescription
//
// Provide native code page description in UNICODE.
// If not resource is vailable for the specified LCID, 
// we'll try the primary language first, then English.
// In this case, we'll return S_FALSE to caller.
STDAPI CMultiLanguage2::GetCodePageDescription(
    UINT uiCodePage,        // Specifies the required code page for description.
    LCID lcid,              // Specifies locale ID for prefered language.
    LPWSTR lpWideCharStr,   // Points to a buffer that receives the code page description.
    int cchWideChar)        // Specifies the size, in wide characters, of the buffer 
                            // pointed by lpWideCharStr.
{
    HRESULT hr = E_FAIL;
    UINT    CountCPId;
    UINT    i = 0, j = 0;

    g_pMimeDatabase->GetNumberOfCodePageInfo(&CountCPId);
    
    if (cchWideChar == 0)
    {
        return E_INVALIDARG;
    }        

    while (i < CountCPId)
    {
        if (MimeCodePage[j].dwFlags & dwMimeSource)
        {
            if ((MimeCodePage[j].uiCodePage == uiCodePage))
            {
                if (_LoadStringExW(g_hInst, MimeCodePage[j].uidDescription, lpWideCharStr,
                            cchWideChar, LANGIDFROMLCID(lcid))) 
                {
                    hr = S_OK;
                }
                else // Resource not find in the specificed language
                {
                        if (_LoadStringExW(g_hInst, MimeCodePage[j].uidDescription, lpWideCharStr,
                            cchWideChar, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)))
                        {
                            hr = S_FALSE;                
                        }
                }
                break;
            }  
            i++;
        }
        j++;
    }
    
    if (i >= CountCPId) // Code page description is not available in MLANG
    {
        hr = E_INVALIDARG;
    }

    return (hr);
}

STDAPI CMultiLanguage2::IsCodePageInstallable(UINT uiCodePage)
{
    MIMECPINFO cpInfo;
    UINT       uiFamCp;
    HRESULT    hr;
    
    DebugMsg(DM_TRACE, TEXT("CMultiLanguage::IsCPInstallable called. "));

    if (NULL != g_pMimeDatabase)
        hr = g_pMimeDatabase->GetCodePageInfo(uiCodePage, 0x409, &cpInfo);
    else
        hr = E_OUTOFMEMORY;

    if (FAILED(hr))
        return E_INVALIDARG;

    // if it's already valid, no need to check if it's installable
    if (cpInfo.dwFlags & MIMECONTF_VALID)
    {
        hr = S_OK;
    }
    else
    {        
        uiFamCp = cpInfo.uiFamilyCodePage;

        // it is currently not valid, if NT5, ignore IOD check
        if (g_bIsNT5)
        {
            hr = IsNTLangpackAvailable(uiFamCp);
        }
        else
        {
            // now check to see if the cp can be IOD
            hr = EnsureIEStatus();
        
            // we'll return FALSE if we couldn't get IOD status
            if (hr == S_OK)
            {
                if (!m_pIEStat || !m_pIEStat->IsJITEnabled())
                    hr = S_FALSE;
            }

            // then see if we have langpack for
            // the family codepage
            if (hr == S_OK)
            {
                CLSID      clsid;
                // clsid is just used for place holder
                hr = _GetJITClsIDForCodePage(uiFamCp, &clsid);
            }
        }
    }
    return hr;
}

STDAPI CMultiLanguage2::SetMimeDBSource(MIMECONTF dwSource)
{        
        if ((dwSource != MIMECONTF_MIME_IE4) &&
            (dwSource != MIMECONTF_MIME_LATEST) &&
            (dwSource != MIMECONTF_MIME_REGISTRY))
        {
            return E_INVALIDARG;
        }

        if (dwSource & MIMECONTF_MIME_REGISTRY)
        {
            EnterCriticalSection(&g_cs);
            if (!g_pMimeDatabaseReg)
            {
                g_pMimeDatabaseReg = new CMimeDatabaseReg;
            }
            LeaveCriticalSection(&g_cs);
        }

        dwMimeSource = dwSource;
        if (NULL != m_pMimeDatabase)
            m_pMimeDatabase->SetMimeDBSource(dwSource);
        return S_OK;
}

CMultiLanguage2::CMultiLanguage2(void)
{
        CComCreator< CComPolyObject< CMultiLanguage > >::CreateInstance( NULL, IID_IMultiLanguage, (void **)&m_pIML );

        m_pMimeDatabase = new CMimeDatabase;
        dwMimeSource = MIMECONTF_MIME_LATEST;
        if (m_pMimeDatabase)
            m_pMimeDatabase->SetMimeDBSource(MIMECONTF_MIME_LATEST);

        m_pIEStat = NULL;
}

CMultiLanguage2::~CMultiLanguage2(void)
{
        if (m_pIML)
        {
            m_pIML->Release();
            m_pIML = NULL;
        }

        if (m_pMimeDatabase)
        {
            delete m_pMimeDatabase;
        }

        if (m_pIEStat)
        {
            delete m_pIEStat;
        }
}

STDAPI CMultiLanguage2::GetNumberOfCodePageInfo(UINT *pcCodePage)
{
        if (dwMimeSource &  MIMECONTF_MIME_REGISTRY)         
        {
            if (NULL != g_pMimeDatabaseReg)
                return g_pMimeDatabaseReg->GetNumberOfCodePageInfo(pcCodePage);
            else
                return E_FAIL;
        }    
        else
        {    
            if (NULL != m_pMimeDatabase)
                return m_pMimeDatabase->GetNumberOfCodePageInfo(pcCodePage);
            else
                return E_FAIL;
        }
}

STDAPI CMultiLanguage2::GetNumberOfScripts(UINT *pnScripts)
{
    if (pnScripts)
        *pnScripts = g_cScript;

    return NOERROR;
}


STDAPI CMultiLanguage2::GetCodePageInfo(UINT uiCodePage, LANGID LangId, PMIMECPINFO pcpInfo)
{
    if (dwMimeSource &  MIMECONTF_MIME_REGISTRY)
    {
        if (NULL != g_pMimeDatabaseReg)
            return g_pMimeDatabaseReg->GetCodePageInfo(uiCodePage, pcpInfo);
        else
            return E_FAIL;
    }
    else
    {
        if (m_pMimeDatabase)
            return m_pMimeDatabase->GetCodePageInfo(uiCodePage, LangId, pcpInfo);
        else
            return E_FAIL;
    }
}

// Optimized for performance
// Skip unecessary resource loading
STDAPI CMultiLanguage2::GetFamilyCodePage(UINT uiCodePage, UINT *puiFamilyCodePage)
{
        HRESULT hr = S_OK;
        int idx = 0;

        if (puiFamilyCodePage)
            *puiFamilyCodePage = 0;
        else
            return E_INVALIDARG;

        DebugMsg(DM_TRACE, TEXT("CMultiLanguage2::GetFamilyCodePage called."));
        // Keep registry version IE4 implementation
        if (dwMimeSource &  MIMECONTF_MIME_REGISTRY)
        {

            if (NULL != g_pMimeDatabaseReg)
            {
                MIMECPINFO cpInfo;
                hr = g_pMimeDatabaseReg->GetCodePageInfo(uiCodePage, &cpInfo);
                if (S_OK == hr)
                    *puiFamilyCodePage = cpInfo.uiFamilyCodePage;
            }
        }
        else
        {
            while(MimeCodePage[idx].uiCodePage)
            {
                if ((uiCodePage == MimeCodePage[idx].uiCodePage) &&
                    (MimeCodePage[idx].dwFlags & dwMimeSource))
                    break;
                idx++;
            }

            if (MimeCodePage[idx].uiCodePage)
            {
                if (MimeCodePage[idx].uiFamilyCodePage)
                    *puiFamilyCodePage = MimeCodePage[idx].uiFamilyCodePage;
                else
                    *puiFamilyCodePage = uiCodePage;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        return hr;
}

STDAPI CMultiLanguage2::GetCharsetInfo(BSTR Charset, PMIMECSETINFO pcsetInfo)
{
    if (dwMimeSource &  MIMECONTF_MIME_REGISTRY)
    {
        if (NULL != g_pMimeDatabaseReg)
            return g_pMimeDatabaseReg->GetCharsetInfo(Charset, pcsetInfo);
        else
            return E_FAIL;
    }

    if (NULL != m_pMimeDatabase)
        return m_pMimeDatabase->GetCharsetInfo(Charset, pcsetInfo);
    else
        return E_FAIL;
}

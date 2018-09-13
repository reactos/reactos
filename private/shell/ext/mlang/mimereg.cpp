#include "private.h"
#ifdef UNIX
/* Convert from little endian to big endian format */
#define CONVERTLONG(a,b,c,d) (((unsigned long )a) + \
                           ((unsigned long )b << 8) + \
                           ((unsigned long )c << 16) + \
                           ((unsigned long )d << 24))
#endif /* UNIX */

//
//  Globals
//
CMimeDatabaseReg *g_pMimeDatabaseReg = NULL;

//
//  Globals
//
PRFC1766INFOA   g_pRfc1766Reg = NULL;
UINT            g_cRfc1766Reg = 0, g_cMaxRfc1766 = 0;

//
//  Functions
//
void CMimeDatabaseReg::BuildRfc1766Table(void)
{
    HKEY hKey = NULL;
    DWORD dwIndex, dwType, cInfo, cbMaxValueLen, cbLCID, cb;
    TCHAR szLCID[8], sz[MAX_RFC1766_NAME +  MAX_LOCALE_NAME + 1];

    DebugMsg(DM_TRACE, TEXT("CRfc1766::BuildRfc1766Table called."));
    EnterCriticalSection(&g_cs);
    if (NULL == g_pRfc1766Reg)
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, REGSTR_KEY_MIME_DATABASE_RFC1766, 0, KEY_READ, &hKey))
        {
            ASSERT(NULL != hKey);
            if (ERROR_SUCCESS == RegQueryInfoKey(hKey, NULL, NULL, 0, NULL, NULL, NULL, &cInfo, &cbMaxValueLen, NULL, NULL, NULL))
            {
                g_pRfc1766Reg = (PRFC1766INFOA)LocalAlloc(LPTR, sizeof(RFC1766INFOA) * cInfo);
                if (NULL != g_pRfc1766Reg)
                {
                    g_cRfc1766Reg = 0;
                    g_cMaxRfc1766 = cInfo;
                    dwIndex = 0;
                    while (g_cRfc1766Reg < g_cMaxRfc1766)
                    {
                        LONG lRet;

                        cbLCID = ARRAYSIZE(szLCID) - 2;
                        cb = sizeof(sz);
                        lRet = RegEnumValue(hKey, dwIndex++, szLCID + 2, &cbLCID, 0, &dwType, (LPBYTE)sz, &cb);
                        if (ERROR_SUCCESS == lRet)
                        {
                            int iLCID;

                            szLCID[0] = TEXT('0');
                            szLCID[1] = TEXT('x');
                            // BUGBUG: We'd better have private function for this instead of using SHLWAPI becuase 16-bit doesn't have SHLWAPI
                            // if (TRUE == StrToIntEx(szLCID, STIF_SUPPORT_HEX, &iLCID))
                            if (iLCID = HexToNum(szLCID + 2))
                            {
                                g_pRfc1766Reg[g_cRfc1766Reg].lcid = (LCID)iLCID;
                                if (REG_SZ == dwType)
                                {
                                    TCHAR *psz = sz;

                                    while (*psz)
                                    {
                                        if (TEXT(';') == *psz)
                                        {
                                            *psz = TEXT('\0');
                                            break;
                                        }
                                        psz = CharNext(psz);
                                    }
                                    lstrcpyn(g_pRfc1766Reg[g_cRfc1766Reg].szRfc1766, sz, MAX_RFC1766_NAME);
                                    lstrcpyn(g_pRfc1766Reg[g_cRfc1766Reg].szLocaleName, psz + 1, MAX_LOCALE_NAME);
                                    g_cRfc1766Reg++;
                                }
                            }
                        }
                        else if (ERROR_NO_MORE_ITEMS == lRet)
                            break;
                    }
                }
            }
            RegCloseKey(hKey);
        }
    }
    LeaveCriticalSection(&g_cs);
}

void CMimeDatabaseReg::FreeRfc1766Table(void)
{
    DebugMsg(DM_TRACE, TEXT("CRfc1766::FreeRfc1766Table called."));
    EnterCriticalSection(&g_cs);
    if (NULL != g_pRfc1766Reg)
    {
        LocalFree(g_pRfc1766Reg);
        g_pRfc1766Reg = NULL;
        g_cRfc1766Reg = g_cMaxRfc1766 = 0;
    }
    LeaveCriticalSection(&g_cs);
}

void CMimeDatabaseReg::EnsureRfc1766Table(void)
{
    // Ensure g_pRfc1766 is initialized
    if (NULL == g_pRfc1766Reg)
        BuildRfc1766Table();
}


STDAPI CMimeDatabaseReg::LcidToRfc1766A(LCID Locale, LPSTR pszRfc1766, int iMaxLength)
{
    UINT i;
    HRESULT hr = E_INVALIDARG;

    EnsureRfc1766Table();
    if (NULL != pszRfc1766 && 0 < iMaxLength)
    {
        for (i = 0; i < g_cRfc1766Reg; i++)
        {
            if (g_pRfc1766Reg[i].lcid == Locale)
                break;
        }
        if (i < g_cRfc1766Reg)
        {
            lstrcpyn(pszRfc1766, g_pRfc1766Reg[i].szRfc1766, iMaxLength);
            hr = S_OK;
        }
        else
        {
            TCHAR sz[MAX_RFC1766_NAME];

            if (GetLocaleInfoA(Locale, LOCALE_SABBREVLANGNAME, sz, ARRAYSIZE(sz)))
            {
                CharLowerA(sz);
                if (!lstrcmpA(sz, TEXT("cht")))
                    lstrcpynA(pszRfc1766, TEXT("zh-cn"), iMaxLength);
                else if (!lstrcmpA(sz, TEXT("chs")))
                    lstrcpynA(pszRfc1766, TEXT("zh-tw"), iMaxLength);
                else if (!lstrcmpA(sz, TEXT("jpn")))
                    lstrcpynA(pszRfc1766, TEXT("ja"), iMaxLength);
                else
                {
                    sz[2] = TEXT('\0');
                    lstrcpynA(pszRfc1766, sz, iMaxLength);
                }
                hr = S_OK;
            }
            else
                hr = E_FAIL;
        }
    }
    return hr;
}    

STDAPI CMimeDatabaseReg::LcidToRfc1766W(LCID Locale, LPWSTR pwszRfc1766, int nChar)
{
    HRESULT hr = E_INVALIDARG;

    if (NULL != pwszRfc1766 && 0 < nChar)
    {
        TCHAR sz[MAX_RFC1766_NAME];

        hr = LcidToRfc1766A(Locale, (LPSTR)sz, ARRAYSIZE(sz));
        if (S_OK == hr)
        {
            int i;

            for (i = 0; i < nChar - 1; i++)
            {
                pwszRfc1766[i] = (WCHAR)sz[i];
                if (L'\0' == pwszRfc1766[i])
                    break;
            }
            if (i == nChar - 1)
                pwszRfc1766[i] = L'\0';            
        }
    }
    return hr;
}    

STDAPI CMimeDatabaseReg::Rfc1766ToLcidA(PLCID pLocale, LPCSTR pszRfc1766)
{
    UINT i;
    HRESULT hr = E_INVALIDARG;

    EnsureRfc1766Table();
    if (NULL != pLocale && NULL != pszRfc1766)
    {
        for (i = 0; i < g_cRfc1766Reg; i++)
        {
            if (!lstrcmpi(g_pRfc1766Reg[i].szRfc1766, pszRfc1766))
                break;
        }
        if (i < g_cRfc1766Reg)
        {
            *pLocale = g_pRfc1766Reg[i].lcid;
            hr = S_OK;
        }
        else
        {
            if (2 < lstrlen(pszRfc1766))
            {
                TCHAR sz[3];

                sz[0] = pszRfc1766[0];
                sz[1] = pszRfc1766[1];
                sz[2] = TEXT('\0');
                for (i = 0; i < g_cRfc1766Reg; i++)
                {
                    if (!lstrcmpi(g_pRfc1766Reg[i].szRfc1766, sz))
                        break;                
                }
                if (i < g_cRfc1766Reg)
                {
                    *pLocale = g_pRfc1766Reg[i].lcid;
                    hr = S_FALSE;
                }
                else
                    hr = E_FAIL;
            }
            else
                hr = E_FAIL;
        }
    }
    return hr;
}

STDAPI CMimeDatabaseReg::Rfc1766ToLcidW(PLCID pLocale, LPCWSTR pwszRfc1766)
{
    HRESULT hr = E_INVALIDARG;

    if (NULL != pLocale && NULL != pwszRfc1766)
    {
        int i;
        TCHAR sz[MAX_RFC1766_NAME];

        for (i = 0; i < MAX_RFC1766_NAME - 1; i++)
        {
            sz[i] = (TCHAR)pwszRfc1766[i];
            if (TEXT('\0') == sz[i])
                break;
        }
        if (i == MAX_RFC1766_NAME -1)
            sz[i] = TEXT('\0');

        hr = Rfc1766ToLcidA(pLocale, (LPCSTR)sz);
    }
    return hr;
}


//
//  CMimeDatabase implementation
//
CMimeDatabaseReg::CMimeDatabaseReg()
{
    DebugMsg(DM_TRACE, TEXT("constructor of CMimeDatabase 0x%08x"), this);
    _pCodePage = NULL;
    _cCodePage = _cMaxCodePage = 0;
    _pCharset = NULL;
    _cCharset = _cMaxCharset = 0;
    _fAllCPCached = FALSE;
    InitializeCriticalSection(&_cs);
}

CMimeDatabaseReg::~CMimeDatabaseReg()
{
    DebugMsg(DM_TRACE, TEXT("destructor of CMimeDatabase 0x%08x"), this);
    FreeMimeDatabase();
    DeleteCriticalSection(&_cs);
}

void CMimeDatabaseReg::BuildCodePageMimeDatabase(void)
{
    HKEY hKey = NULL;
    DWORD cInfo, cbMaxSubKeyLen;

    DebugMsg(DM_TRACE, TEXT("CMimeDatabase::BuildCodePageMimeDatabase called."));
    // Open CodePage Mime Database Key
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, REGSTR_KEY_MIME_DATABASE_CODEPAGE, 0, KEY_READ, &hKey))
    {
        ASSERT(NULL != hKey);
        if (ERROR_SUCCESS == RegQueryInfoKey(hKey, NULL, NULL, 0, &cInfo, &cbMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL))
        {
            if (NULL == _pCodePage)
            {
                _pCodePage = (PMIMECPINFO)LocalAlloc(LPTR, sizeof(MIMECPINFO) * cInfo);
                if (NULL != _pCodePage)
                    _cMaxCodePage = cInfo;

            }
        }
        RegCloseKey(hKey);
        hKey = NULL;
    }

}

void CMimeDatabaseReg::BuildCharsetMimeDatabase(void)
{
    HKEY hKey = NULL;
    DWORD cInfo, cbMaxSubKeyLen;

    DebugMsg(DM_TRACE, TEXT("CMimeDatabase::BuildCharsetMimeDatabase called."));
    // Open Charset Mime Database Key
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, REGSTR_KEY_MIME_DATABASE_CHARSET, 0, KEY_READ, &hKey))
    {
        ASSERT(NULL != hKey);
        if (ERROR_SUCCESS == RegQueryInfoKey(hKey, NULL, NULL, 0, &cInfo, &cbMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL))
        {
            if (NULL == _pCharset)
            {
                _pCharset = (PMIMECSETINFO)LocalAlloc(LPTR, sizeof(MIMECSETINFO) * cInfo);
                if (NULL != _pCharset)
                    _cMaxCharset = cInfo;
            }
        }
        RegCloseKey(hKey);
        hKey = NULL;
    }
}

void CMimeDatabaseReg::FreeMimeDatabase(void)
{
    DebugMsg(DM_TRACE, TEXT("CMimeDatabase::FreeMimeDatabase called."));
    EnterCriticalSection(&_cs);
    if (NULL != _pCodePage)
    {
        LocalFree(_pCodePage);
        _pCodePage = NULL;
        _cCodePage = _cMaxCodePage = 0;
    }
    if (NULL != _pCharset)
    {
        LocalFree(_pCharset);
        _pCharset = NULL;
        _cCharset = _cMaxCharset = 0;
    }
    LeaveCriticalSection(&_cs);
    FreeRfc1766Table();
}

STDAPI CMimeDatabaseReg::EnumCodePageInfo(void)
{
    HKEY hKey = NULL;
    DWORD dwIndex = 0;
    MIMECPINFO CPInfo;
    TCHAR szCodePage[15];
    HRESULT hr = S_OK;

    DebugMsg(DM_TRACE, TEXT("CMimeDatabase::EnumCodePageInfo called."));
    EnterCriticalSection(&_cs);
    if (FALSE == _fAllCPCached)
    {
        if (NULL == _pCodePage)
            BuildCodePageMimeDatabase();
        if (_pCodePage)
        {
            // Open CodePage Mime Database Key
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, REGSTR_KEY_MIME_DATABASE_CODEPAGE, 0, KEY_READ, &hKey))
            {
                ASSERT(NULL != hKey);
                while (ERROR_SUCCESS == RegEnumKey(hKey, dwIndex++, szCodePage, ARRAYSIZE(szCodePage)))
                {
                    UINT uiCodePage = MLStrToInt(szCodePage);
    
                    if (0 <= FindCodePageFromCache(uiCodePage))
                        continue;

                    if (TRUE == FindCodePageFromRegistry(uiCodePage, &CPInfo))
                    {
                        _pCodePage[_cCodePage] = CPInfo;
                        _cCodePage++;
                    }
                }
                _fAllCPCached = TRUE;
                RegCloseKey(hKey);
                hKey = NULL;
            }
            if (0 < _cCodePage)
                QSortCodePageInfo(0, _cCodePage-1);

            // Fill empty font face field base on its FamilyCodePage
            for (UINT i = 0; i < _cCodePage; i++)
            {
                UINT uiFamily;
                WCHAR wszFixed[MAX_MIMEFACE_NAME], wszProp[MAX_MIMEFACE_NAME];

                uiFamily = 0;
                wszFixed[0] = wszProp[0] = TEXT('\0');

                if (TEXT('\0') == _pCodePage[i].wszFixedWidthFont[0] || TEXT('\0') == _pCodePage[i].wszProportionalFont[0])
                {
                    if (uiFamily != _pCodePage[i].uiFamilyCodePage)
                    {
                        for (UINT j = 0; j < _cCodePage; j++)
                        {
                            if (_pCodePage[i].uiFamilyCodePage == _pCodePage[j].uiCodePage)
                            {
                                uiFamily = _pCodePage[j].uiCodePage;
                                MLStrCpyNW(wszFixed, _pCodePage[j].wszFixedWidthFont, MAX_MIMEFACE_NAME);
                                MLStrCpyNW(wszProp, _pCodePage[j].wszProportionalFont, MAX_MIMEFACE_NAME);
                                break;
                            }
                        }                
                    }
                    MLStrCpyNW(_pCodePage[i].wszFixedWidthFont, wszFixed, MAX_MIMEFACE_NAME);
                    MLStrCpyNW(_pCodePage[i].wszProportionalFont, wszProp, MAX_MIMEFACE_NAME);
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    LeaveCriticalSection(&_cs);

    return hr;
}

STDAPI CMimeDatabaseReg::GetNumberOfCodePageInfo(UINT *pcCodePage)
{
    EnterCriticalSection(&_cs);
    if (NULL == _pCodePage)
        BuildCodePageMimeDatabase();
    *pcCodePage = _cMaxCodePage;
    LeaveCriticalSection(&_cs);
    return NOERROR;
}

STDAPI CMimeDatabaseReg::GetCodePageInfo(UINT uiCodePage, PMIMECPINFO pcpInfo)
{
    int idx;
    HRESULT hr = E_FAIL;

    DebugMsg(DM_TRACE, TEXT("CMimeDatabase::GetCodePageInfo called."));
    if (NULL != pcpInfo)
    {
        EnterCriticalSection(&_cs);
        if (NULL == _pCodePage)
            BuildCodePageMimeDatabase();

        if (_pCodePage)
        {
            idx = FindCodePageFromCache(uiCodePage);
            if (0 > idx)
            {
                MIMECPINFO CPInfo = {0};

                if (TRUE == FindCodePageFromRegistry(uiCodePage, &CPInfo))
                {
                    if (CPInfo.uiCodePage != CPInfo.uiFamilyCodePage)
                    {
                        idx = FindCodePageFromCache(CPInfo.uiFamilyCodePage);
                        if (0 > idx)
                        {
                            MIMECPINFO FamilyCPInfo;

                            if (TRUE == FindCodePageFromRegistry(CPInfo.uiFamilyCodePage, &FamilyCPInfo))
                            {
                                idx = _cCodePage;
                                _pCodePage[_cCodePage] = FamilyCPInfo;
                                _cCodePage++;
                            }
                        }
                        MLStrCpyNW(CPInfo.wszFixedWidthFont, _pCodePage[idx].wszFixedWidthFont, MAX_MIMEFACE_NAME);
                        MLStrCpyNW(CPInfo.wszProportionalFont, _pCodePage[idx].wszProportionalFont, MAX_MIMEFACE_NAME);
                    }
                    _pCodePage[_cCodePage] = CPInfo;
                    _cCodePage++;
                    QSortCodePageInfo(0, _cCodePage-1);
                    idx = FindCodePageFromCache(uiCodePage);
                }
            }
            if (0 <= idx)
            {
                *pcpInfo = _pCodePage[idx];
                hr = S_OK;
            }
            LeaveCriticalSection(&_cs);
        }
    }
    return hr;
}

int CMimeDatabaseReg::FindCodePageFromCache(UINT uiCodePage)
{
    UINT i;
    int iRet = -1;

    for (i = 0; i < _cCodePage; i++)
    {
        if (_pCodePage[i].uiCodePage == uiCodePage)
        {
            iRet = i;
            break;
        }
    }
    return iRet;
}

BOOL CMimeDatabaseReg::FindCodePageFromRegistry(UINT uiCodePage, PMIMECPINFO pcpInfo)
{
    HKEY hKey;
    DWORD dw, cb;
    TCHAR szKey[256],  sz[MAX_MIMECP_NAME];
    BOOL fRet = FALSE;
    static UINT s_uiCP = GetACP();

    wsprintf(szKey, TEXT("%s\\%d"), REGSTR_KEY_MIME_DATABASE_CODEPAGE, uiCodePage);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hKey))
    {
        TCHAR *psz, *pszComma;
        CHARSETINFO rCharsetInfo;

        pcpInfo->uiCodePage = uiCodePage;

        cb = sizeof(dw);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_FAMILY, 0, NULL, (LPBYTE)&dw, &cb))
            pcpInfo->uiFamilyCodePage = (UINT)dw;
        else
            pcpInfo->uiFamilyCodePage = pcpInfo->uiCodePage;

        cb = sizeof(dw);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_LEVEL, 0, NULL, (LPBYTE)&dw, &cb))
#ifdef UNIX
        {
           BYTE* px = (BYTE*)&dw;
           pcpInfo->dwFlags = CONVERTLONG(px[0], px[1], px[2], px[3]);
        }
#else
            pcpInfo->dwFlags = dw;
#endif /* UNIX */
        else
            pcpInfo->dwFlags = 0;

        cb = sizeof(sz);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_DESCRIPTION, NULL, NULL, (LPBYTE)sz, &cb))
            MultiByteToWideChar(CP_ACP, 0, sz, -1, pcpInfo->wszDescription, ARRAYSIZE(pcpInfo->wszDescription));
        else
        {
            TCHAR szDef[MAX_MIMECP_NAME];

            LoadString(g_hInst, IDS_MIME_LANG_DEFAULT, szDef, ARRAYSIZE(szDef));
            wsprintf(sz, szDef, pcpInfo->uiCodePage);
            MultiByteToWideChar(CP_ACP, 0, sz, -1, pcpInfo->wszDescription, ARRAYSIZE(pcpInfo->wszDescription));
        }

        cb = sizeof(sz);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_FIXEDWIDTHFONT, NULL, NULL, (LPBYTE)sz, &cb))
        {
            psz = sz;
            pszComma =  MLStrChr(sz, TEXT(','));
            if (NULL != pszComma)               // If there are multiple font name
            {
                if (uiCodePage != s_uiCP)
                    psz = pszComma + 1;         // Take right side(English) fontname for non-native codepage info
                else
                    *pszComma = TEXT('\0');     // Take left side(DBCS) fontname for native codepage info
            }
            if (lstrlen(psz) >= MAX_MIMEFACE_NAME)
                psz[MAX_MIMEFACE_NAME-1] = TEXT('\0');
            MultiByteToWideChar(CP_ACP, 0, psz, -1, pcpInfo->wszFixedWidthFont, ARRAYSIZE(pcpInfo->wszFixedWidthFont));
        }
        else
            pcpInfo->wszFixedWidthFont[0] = L'\0';

        cb = sizeof(sz);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_PROPORTIONALFONT, NULL, NULL, (LPBYTE)sz, &cb))
        {
            psz = sz;
            pszComma = MLStrChr(sz, TEXT(','));
            if (NULL != pszComma)               // If there are multiple font name
            {
                if (uiCodePage != s_uiCP)
                    psz = pszComma + 1;         // Take right side(English) fontname for non-native codepage info
                else
                    *pszComma = TEXT('\0');     // Take left side(DBCS) fontname for native codepage info
            }
            if (lstrlen(psz) >= MAX_MIMEFACE_NAME)
                psz[MAX_MIMEFACE_NAME-1] = TEXT('\0');
            MultiByteToWideChar(CP_ACP, 0, psz, -1, pcpInfo->wszProportionalFont, ARRAYSIZE(pcpInfo->wszProportionalFont));
        }
        else
            pcpInfo->wszProportionalFont[0] = L'\0';

        cb = sizeof(sz);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_BODYCHARSET, NULL, NULL, (LPBYTE)sz, &cb))
            MultiByteToWideChar(CP_ACP, 0, sz, -1, pcpInfo->wszBodyCharset, ARRAYSIZE(pcpInfo->wszBodyCharset));
        else
            pcpInfo->wszBodyCharset[0] = L'\0';

        cb = sizeof(sz);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_HEADERCHARSET, NULL, NULL, (LPBYTE)sz, &cb))
            MultiByteToWideChar(CP_ACP, 0, sz, -1, pcpInfo->wszHeaderCharset, ARRAYSIZE(pcpInfo->wszHeaderCharset));
        else
            MLStrCpyNW(pcpInfo->wszHeaderCharset, pcpInfo->wszBodyCharset, MAX_MIMECSET_NAME);

        cb = sizeof(sz);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_WEBCHARSET, NULL, NULL, (LPBYTE)sz, &cb))
            MultiByteToWideChar(CP_ACP, 0, sz, -1, pcpInfo->wszWebCharset, ARRAYSIZE(pcpInfo->wszWebCharset));
        else
            MLStrCpyNW(pcpInfo->wszWebCharset, pcpInfo->wszBodyCharset, MAX_MIMECSET_NAME);

        cb = sizeof(sz);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_PRIVCONVERTER, NULL, NULL, (LPBYTE)sz, &cb))
            pcpInfo->dwFlags |= MIMECONTF_PRIVCONVERTER;

        if (0 != TranslateCharsetInfo((LPDWORD)pcpInfo->uiFamilyCodePage, &rCharsetInfo, TCI_SRCCODEPAGE))
            pcpInfo->bGDICharset = (BYTE)rCharsetInfo.ciCharset;
        else
            pcpInfo->bGDICharset = DEFAULT_CHARSET;


        if (1200 == pcpInfo->uiFamilyCodePage || 50000 == pcpInfo->uiFamilyCodePage || TRUE == _IsValidCodePage(pcpInfo->uiFamilyCodePage)) // 50000 means user defined
        {
            if (TRUE == CheckFont(pcpInfo->bGDICharset))
            {
                if (pcpInfo->uiCodePage == pcpInfo->uiFamilyCodePage || TRUE == _IsValidCodePage(pcpInfo->uiCodePage))
                    pcpInfo->dwFlags |= MIMECONTF_VALID|MIMECONTF_VALID;
                else if (S_OK == IsConvertINetStringAvailable(pcpInfo->uiCodePage, pcpInfo->uiFamilyCodePage))
                    pcpInfo->dwFlags |= MIMECONTF_VALID|MIMECONTF_VALID;
            }
            else
            {
                if (pcpInfo->uiCodePage == pcpInfo->uiFamilyCodePage || TRUE == _IsValidCodePage(pcpInfo->uiCodePage))
                    pcpInfo->dwFlags |= MIMECONTF_VALID_NLS;
                else if (S_OK == IsConvertINetStringAvailable(pcpInfo->uiCodePage, pcpInfo->uiFamilyCodePage))
                    pcpInfo->dwFlags |= MIMECONTF_VALID_NLS;
            }

        }
        RegCloseKey(hKey);
        fRet = TRUE;
    }
    return fRet;
}

STDAPI CMimeDatabaseReg::GetCodePageInfoWithIndex(UINT uidx, PMIMECPINFO pcpInfo)
{
    HRESULT hr = NOERROR;

    DebugMsg(DM_TRACE, TEXT("CMimeDatabase::GetCodePageInfoWithIndex called."));
    EnterCriticalSection(&_cs);
    if (NULL == _pCodePage)
        BuildCodePageMimeDatabase();
    if (uidx < _cCodePage && _pCodePage)
        *pcpInfo = _pCodePage[uidx];
    else
        hr = E_FAIL;
    LeaveCriticalSection(&_cs);
    return hr;
}

STDAPI CMimeDatabaseReg::GetCharsetInfo(BSTR Charset, PMIMECSETINFO pcsetInfo)
{
    int idx;
    HRESULT hr = E_FAIL;

    DebugMsg(DM_TRACE, TEXT("CMimeDatabase::GetCharsetInfo called."));
    if (NULL != pcsetInfo)
    {
        EnterCriticalSection(&_cs);
        if (NULL == _pCharset)
            BuildCharsetMimeDatabase();
        if (_pCharset)
        {
            idx = FindCharsetFromCache(Charset);
            if (0 > idx)
                idx = FindCharsetFromRegistry(Charset, FALSE);
            if (0 <= idx)
            {
                *pcsetInfo = _pCharset[idx];
                hr = S_OK;
            }
        }
        LeaveCriticalSection(&_cs);
    }
    return hr;
}

int CMimeDatabaseReg::FindCharsetFromCache(BSTR Charset)
{
    int iStart, iEnd, iMiddle, iCmpResult, iRet = -1;

    iStart = 0;
    iEnd = _cCharset - 1;
    while (iStart <= iEnd)
    {
        iMiddle = (iStart + iEnd) / 2;
        iCmpResult = MLStrCmpIW(Charset, _pCharset[iMiddle].wszCharset);
        if (iCmpResult < 0)
            iEnd = iMiddle - 1;
        else if (iCmpResult > 0)
            iStart = iMiddle + 1;
        else
        {
            iRet = iMiddle;
            break;
        }
    }
    return iRet;
}

int CMimeDatabaseReg::FindCharsetFromRegistry(BSTR Charset, BOOL fFromAlias)
{
    HKEY hKey;
    TCHAR szKey[256], szCharset[MAX_MIMECSET_NAME];
    int iRet = -1;

    WideCharToMultiByte(CP_ACP, 0, Charset, -1, szCharset, ARRAYSIZE(szCharset), NULL, NULL);
    lstrcpy(szKey, REGSTR_KEY_MIME_DATABASE_CHARSET);
    lstrcat(szKey, TEXT("\\"));
    lstrcat(szKey, szCharset);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hKey))
    {
        DWORD cb, dw;
        TCHAR sz[MAX_MIMECSET_NAME];
        WCHAR wsz[MAX_MIMECSET_NAME];

        cb = sizeof(sz);
        if (FALSE == fFromAlias && ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_ALIASTO, NULL, NULL, (LPBYTE)sz, &cb))
        {
            MultiByteToWideChar(CP_ACP, 0, sz, -1, wsz, ARRAYSIZE(wsz));
            iRet = FindCharsetFromCache(wsz);
            if (0 > iRet)
                iRet = FindCharsetFromRegistry(wsz, TRUE);
            if (0 <= iRet)
            {
                MLStrCpyNW(_pCharset[_cCharset].wszCharset, Charset, MAX_MIMECSET_NAME);
                _pCharset[_cCharset].uiCodePage = _pCharset[iRet].uiCodePage;
                _pCharset[_cCharset].uiInternetEncoding = _pCharset[iRet].uiInternetEncoding;
                _cCharset++;
                QSortCharsetInfo(0, _cCharset-1);
                iRet = FindCharsetFromCache(Charset);
            }
        }
        else
        {
            MLStrCpyNW(_pCharset[_cCharset].wszCharset, Charset, MAX_MIMECSET_NAME);
            cb = sizeof(dw);
            if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_CODEPAGE, 0, NULL, (LPBYTE)&dw, &cb))
            {
                _pCharset[_cCharset].uiCodePage = (UINT)dw;
                cb = sizeof(dw);
                if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_INETENCODING, 0, NULL, (LPBYTE)&dw, &cb))
                {
                    _pCharset[_cCharset].uiInternetEncoding = (UINT)dw;
                    _cCharset++;
                    QSortCharsetInfo(0, _cCharset-1);
                    iRet = FindCharsetFromCache(Charset);
                }
            }
        }
        RegCloseKey(hKey);
    }
    return iRet;
}


BOOL CMimeDatabaseReg::CheckFont(BYTE bGDICharset)
{
    BOOL fRet = FALSE;

    DebugMsg(DM_TRACE, TEXT("CMimeDatabase::CheckFont called."));
    if (DEFAULT_CHARSET == bGDICharset)
        fRet = TRUE;
    else
    {
        HDC     hDC;
        LOGFONT lf;
        HWND    hWnd;

        hWnd = GetTopWindow(GetDesktopWindow());
        hDC = GetDC(hWnd);

        if (NULL != hDC)
        {
            lf.lfFaceName[0] = TEXT('\0');
            lf.lfPitchAndFamily = 0;
            lf.lfCharSet = bGDICharset;
            EnumFontFamiliesEx(hDC, &lf, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)&fRet, 0);
        }
        ReleaseDC(hWnd, hDC);
    }
    return fRet;
}

void CMimeDatabaseReg::QSortCodePageInfo(LONG left, LONG right)
{
    register LONG i, j;
    WCHAR k[MAX_MIMECP_NAME];
    MIMECPINFO t;

    DebugMsg(DM_TRACE, TEXT("CMimeDatabase::QSortCodePageInfo called."));
    i = left;
    j = right;
    MLStrCpyW(k, _pCodePage[(left + right) / 2].wszDescription);

    do  
    {
        while(MLStrCmpIW(_pCodePage[i].wszDescription, k) < 0 && i < right)
            i++;
        while (MLStrCmpIW(_pCodePage[j].wszDescription, k) > 0 && j > left)
            j--;

        if (i <= j)
        {
            t = _pCodePage[i];
            _pCodePage[i] = _pCodePage[j];
            _pCodePage[j] = t;
            i++; j--;
        }

    } while (i <= j);

    if (left < j)
        QSortCodePageInfo(left, j);
    if (i < right)
        QSortCodePageInfo(i, right);
}

void CMimeDatabaseReg::QSortCharsetInfo(LONG left, LONG right)
{
    register LONG i, j;
    WCHAR k[MAX_MIMECSET_NAME];
    MIMECSETINFO t;

    DebugMsg(DM_TRACE, TEXT("CMimeDatabase::QSortCharsetInfo called."));
    i = left;
    j = right;
    MLStrCpyW(k, _pCharset[(left + right) / 2].wszCharset);

    do  
    {
        while(MLStrCmpIW(_pCharset[i].wszCharset, k) < 0 && i < right)
            i++;
        while (MLStrCmpIW(_pCharset[j].wszCharset, k) > 0 && j > left)
            j--;

        if (i <= j)
        {
            t = _pCharset[i];
            _pCharset[i] = _pCharset[j];
            _pCharset[j] = t;
            i++; j--;
        }

    } while (i <= j);

    if (left < j)
        QSortCharsetInfo(left, j);
    if (i < right)
        QSortCharsetInfo(i, right);
}

// validates all cps that are in the same
// family of the given codepage
STDAPI CMimeDatabaseReg::ValidateCP(UINT uiCodePage)
{
    UINT i;

    if (NULL == _pCodePage)
        BuildCodePageMimeDatabase();
    //
    // just look into already cached codepages
    // 
    for (i = 0; i < _cCodePage; i++)
    {
        if (_pCodePage[i].uiFamilyCodePage == uiCodePage)
            _pCodePage[i].dwFlags |=  MIMECONTF_VALID|MIMECONTF_VALID_NLS;
    }
        
    return S_OK; // never fail?
}

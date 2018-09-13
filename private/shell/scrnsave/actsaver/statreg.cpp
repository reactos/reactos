#include "precomp.h"
#include "statreg.h"

LPCTSTR   rgszNeverDelete[] = //Component Catagories
{
    _T("CLSID"), _T("TYPELIB")
};

const int   cbNeverDelete = sizeof(rgszNeverDelete) / sizeof(LPCTSTR*);

LPTSTR StrChr(LPTSTR lpsz, TCHAR ch)
{
    LPTSTR p = NULL;
    while (*lpsz)
    {
        if (*lpsz == ch)
        {
            p = lpsz;
            break;
        }
        lpsz = CharNext(lpsz);
    }
    return p;
}

static HKEY WINAPI HKeyFromString(LPTSTR szToken)
{
    struct keymap
    {
        LPCTSTR lpsz;
        HKEY hkey;
    };
    static const keymap map[] = {
        {_T("HKCR"), HKEY_CLASSES_ROOT},
        {_T("HKCU"), HKEY_CURRENT_USER},
        {_T("HKLM"), HKEY_LOCAL_MACHINE},
        {_T("HKU"),  HKEY_USERS},
        {_T("HKPD"), HKEY_PERFORMANCE_DATA},
        {_T("HKDD"), HKEY_DYN_DATA},
        {_T("HKCC"), HKEY_CURRENT_CONFIG},
        {_T("HKEY_CLASSES_ROOT"), HKEY_CLASSES_ROOT},
        {_T("HKEY_CURRENT_USER"), HKEY_CURRENT_USER},
        {_T("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE},
        {_T("HKEY_USERS"), HKEY_USERS},
        {_T("HKEY_PERFORMANCE_DATA"), HKEY_PERFORMANCE_DATA},
        {_T("HKEY_DYN_DATA"), HKEY_DYN_DATA},
        {_T("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG}
    };

    for (int i=0;i<sizeof(map)/sizeof(keymap);i++)
    {
        if (!lstrcmpi(szToken, map[i].lpsz))
            return map[i].hkey;
    }
    return NULL;
}

static HKEY HKeyFromCompoundString(LPTSTR szToken, LPTSTR& szTail)
{
    if (NULL == szToken)
        return NULL;

    LPTSTR lpsz = StrChr(szToken, chDirSep);

    if (NULL == lpsz)
        return NULL;

    szTail = CharNext(lpsz);
    *lpsz = chEOS;
    HKEY hKey = HKeyFromString(szToken);
    *lpsz = chDirSep;
    return hKey;
}

static LPVOID QueryValue(HKEY hKey, LPCTSTR szValName, DWORD& dwType)
{
    DWORD dwCount = 0;

    if (RegQueryValueEx(hKey, szValName, NULL, &dwType, NULL, &dwCount) != ERROR_SUCCESS)
    {
        ASSERT(FALSE);
        return NULL;
    }

    if (!dwCount)
    {
        ASSERT(FALSE);
        return NULL;
    }

    // Not going to Check for fail on CoTaskMemAlloc & RegQueryValueEx as NULL
    // will be returned regardless if anything failed

    LPVOID pData = CoTaskMemAlloc(dwCount);
    RegQueryValueEx(hKey, szValName, NULL, &dwType, (LPBYTE) pData, &dwCount);
    return pData;
}

/////////////////////////////////////////////////////////////////////////////
//

HRESULT CRegParser::GenerateError(UINT nID)
{
    m_re.m_nID   = nID;
    m_re.m_cLines = m_cLines;
    return DISP_E_EXCEPTION;
}


CRegParser::CRegParser(CRegObject* pRegObj)
{
    m_pRegObj           = pRegObj;
    m_pchCur            = NULL;
    m_cLines            = 1;
}

BOOL CRegParser::IsSpace(TCHAR ch)
{
    switch (ch)
    {
        case chSpace:
        case chTab:
        case chCR:
        case chLF:
                return TRUE;
    }

    return FALSE;
}

void CRegParser::IncrementLinePos()
{
    m_pchCur = CharNext(m_pchCur);
    if (chLF == *m_pchCur)
        IncrementLineCount();
}

void CRegParser::SkipWhiteSpace()
{
    while(IsSpace(*m_pchCur))
        IncrementLinePos();
}

HRESULT CRegParser::NextToken(LPTSTR szToken)
{
    UINT ichToken = 0;

    SkipWhiteSpace();

    // NextToken cannot be called at EOS
    if (chEOS == *m_pchCur)
        return GenerateError(IDS_UNEXPECTED_EOS);

    // handle quoted value / key
    if (chQuote == *m_pchCur)
    {
        LPCTSTR szOrig = szToken;

        IncrementLinePos(); // Skip Quote

        while (chEOS != *m_pchCur && !EndOfVar())
        {
            if (chQuote == *m_pchCur) // If it is a quote that means we must skip it
                IncrementLinePos();   // as it has been escaped

            LPTSTR pchPrev = m_pchCur;
            IncrementLinePos();

            if (szToken + sizeof(WORD) >= MAX_VALUE + szOrig)
                return GenerateError(IDS_VALUE_TOO_LARGE);
            for (int i = 0; pchPrev+i < m_pchCur; i++, szToken++)
                *szToken = *(pchPrev+i);
        }

        if (chEOS == *m_pchCur)
        {
            ASSERT(FALSE);
            return GenerateError(IDS_UNEXPECTED_EOS);
        }

        *szToken = chEOS;
        IncrementLinePos(); // Skip end quote
    }

    else
    {   // Handle non-quoted ie parse up till first "White Space"
        while (chEOS != *m_pchCur && !IsSpace(*m_pchCur))
        {
            LPTSTR pchPrev = m_pchCur;
            IncrementLinePos();
            for (int i = 0; pchPrev+i < m_pchCur; i++, szToken++)
                *szToken = *(pchPrev+i);
        }

        *szToken = chEOS;
    }
    return S_OK;
}

static BOOL VTFromRegType(LPCTSTR szValueType, VARTYPE& vt)
{
    struct typemap
    {
        LPCTSTR lpsz;
        VARTYPE vt;
    };
    static const typemap map[] = {
        {szStringVal, VT_BSTR},
        {szDwordVal,  VT_I4}
    };

    for (int i=0;i<sizeof(map)/sizeof(typemap);i++)
    {
        if (!lstrcmpi(szValueType, map[i].lpsz))
        {
            vt = map[i].vt;
            return TRUE;
        }
    }

    return FALSE;

}

HRESULT CRegParser::AddValue(CRegKey& rkParent,LPCTSTR szValueName, LPTSTR szToken)
{
    TCHAR *     pszTypeToken;
    TCHAR *     pszValue = NULL;
    VARTYPE     vt;
    LONG        lRes = ERROR_SUCCESS;
    UINT        nIDRes = 0;
    HRESULT     hrResult;

    pszTypeToken = new TCHAR [MAX_TYPE];

    if (NULL == pszTypeToken)
        return E_OUTOFMEMORY;

    for (;;)
    {
        if (FAILED(hrResult = NextToken(pszTypeToken)))
            break;

        if (!VTFromRegType(pszTypeToken, vt))
        {
            ASSERT(FALSE);
            hrResult = GenerateError(IDS_TYPE_NOT_SUPPORTED);
            break;
        }

        SkipWhiteSpace();

        pszValue = new TCHAR [MAX_VALUE];

        if (NULL == pszValue)
        {
            hrResult = E_OUTOFMEMORY;
            break;
        }

        if (FAILED(hrResult = NextToken(pszValue)))
            break;

        switch (vt)
        {
            case VT_BSTR:
            {
                lRes = rkParent.SetValue(pszValue, szValueName);
                break;
            }

            case VT_I4:
            {
                long lVal;

                T2OLE(pszValue, poszValue);
                VarI4FromStr(poszValue, 0, 0, &lVal);
                lRes = rkParent.SetValue(lVal, szValueName);
                break;
            }
        }

        if (ERROR_SUCCESS != lRes)
        {
            nIDRes = IDS_VALUE_SET_FAILED;
            hrResult = HRESULT_FROM_WIN32(lRes);
            break;
        }

        if (FAILED(hrResult = NextToken(szToken)))
            break;

        hrResult = S_OK;
        break;
    }

    if (pszTypeToken != NULL)
        delete [] pszTypeToken;

    if (pszValue != NULL)
        delete [] pszValue;

    return hrResult;
}

BOOL CRegParser::CanForceRemoveKey(LPCTSTR szKey)
{
    for (int iNoDel = 0; iNoDel < cbNeverDelete; iNoDel++)
        if (!lstrcmpi(szKey, rgszNeverDelete[iNoDel]))
             return FALSE;                       // We cannot delete it

    return TRUE;
}

BOOL CRegParser::HasSubKeys(HKEY hkey)
{
    DWORD       cbSubKeys = 0;

    if (FAILED(RegQueryInfoKey(hkey, NULL, NULL, NULL,
                               &cbSubKeys, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL)))
    {
        ASSERT(FALSE);
        return FALSE; // REVIEW:really not a good return for this scenerio!
    }

    return cbSubKeys > 0;
}

BOOL CRegParser::HasValues(HKEY hkey)
{
    DWORD       cbValues = 0;

    LONG lResult = RegQueryInfoKey(hkey, NULL, NULL, NULL,
                                  NULL, NULL, NULL,
                                  &cbValues, NULL, NULL, NULL, NULL);
    if (ERROR_SUCCESS != lResult)
    {
        ASSERT(FALSE);
        return FALSE;
    }

    if (1 == cbValues)
    {
        DWORD cbData = 0;
        lResult = RegQueryValueEx(hkey, NULL, NULL, NULL, NULL, &cbData);

        if (ERROR_SUCCESS == lResult)
            return !cbData;
        else
            return TRUE;
    }

    return cbValues > 0;
}

HRESULT CRegParser::SkipAssignment(LPTSTR szToken)
{
    HRESULT hrResult;

    if (NULL == szToken)
        return E_POINTER;

    if (*szToken == chEquals)
    {
        TCHAR * pszValue;

        pszValue = new TCHAR [MAX_VALUE];

        if (NULL == pszValue)
            return E_OUTOFMEMORY;

        for (;;)
        {
            if (FAILED(hrResult = NextToken(szToken)))
                break;

            // Skip assignment
            SkipWhiteSpace();

            if (FAILED(hrResult = NextToken(pszValue)))
                break;

            if (FAILED(hrResult = NextToken(szToken)))
                break;

            hrResult = S_OK;
            break;
        }

        delete [] pszValue;
    }

    return hrResult;
}


HRESULT CRegParser::RegisterSubkeys(HKEY hkParent, BOOL bRegister, BOOL bInRecovery)
{
    CRegKey keyCur;
    TCHAR * pszToken;
    TCHAR * pszKey;
    LONG    lRes;
    BOOL    bDelete = TRUE;
    BOOL    bRecover = bInRecovery;
    HRESULT hrResult = S_OK;

    pszToken = new TCHAR [MAX_VALUE];
    pszKey = new TCHAR [MAX_VALUE];

    if (NULL == pszToken || NULL == pszKey)
        goto Cleanup;

    for (;;)
    {
        if (FAILED(hrResult = NextToken(pszToken)))  // Should be key name
            break;

        while (*pszToken != chRightBracket) // Continue till we see a }
        {
            BOOL bTokenDelete = !lstrcmpi(pszToken, szDelete);

            if  (
                !lstrcmpi(pszToken, szForceRemove)
                ||
                bTokenDelete
                )
            {
                if (FAILED(hrResult = NextToken(pszToken)))
                    break;

                if (bRegister)
                {
                    CRegKey rkForceRemove;

                    if (StrChr(pszToken, chDirSep) != NULL)
                    {
                        hrResult = GenerateError(IDS_COMPOUND_KEY);
                        break;
                    }

                    if (CanForceRemoveKey(pszToken))
                    {
                        rkForceRemove.Attach(hkParent);
                        rkForceRemove.RecurseDeleteKey(pszToken);
                        rkForceRemove.Detach();
                    }

                    if (bTokenDelete)
                    {
                        if (FAILED(hrResult = NextToken(pszToken)))
                            break;

                        if (FAILED(hrResult = SkipAssignment(pszToken)))
                            break;

                        goto EndCheck;
                    }
                }

            }

            if (!lstrcmpi(pszToken, szNoRemove))
            {
                bDelete = FALSE;    // set even for register
                if (FAILED(hrResult = NextToken(pszToken)))
                    break;
            }

            if (!lstrcmpi(pszToken, szValToken)) // need to add a value to hkParent
            {
                TCHAR  szValueName[MAX_PATH];

                if (FAILED(hrResult = NextToken(szValueName)))
                    break;

                if (FAILED(hrResult = NextToken(pszToken)))
                    break;

                if (*pszToken != chEquals)
                {
                    hrResult = GenerateError(IDS_EXPECTING_EQUAL);
                    break;
                }

                if (bRegister)
                {
                    CRegKey rk;

                    rk.Attach(hkParent);
                    hrResult = AddValue(rk, szValueName, pszToken);
                    rk.Detach();

                    if (FAILED(hrResult))
                        break;

                    goto EndCheck;
                }
                else
                {
                    if (!bRecover)
                        RegDeleteValue(hkParent, szValueName);

                    if (FAILED(hrResult = SkipAssignment(pszToken))) // Strip off type
                        break;

                    continue;  // can never have a subkey
                }
            }

            if (StrChr(pszToken, chDirSep) != NULL)
            {
                hrResult = GenerateError(IDS_COMPOUND_KEY);
                break;
            }

            if (bRegister)
            {
                lRes = keyCur.Open(hkParent, pszToken, KEY_ALL_ACCESS);
                if (ERROR_SUCCESS != lRes)
                {
                    // Failed all access try read only
                    lRes = keyCur.Open(hkParent, pszToken, KEY_READ);
                    if (ERROR_SUCCESS != lRes)
                    {
                        // Finally try creating it
                        lRes = keyCur.Create(hkParent, pszToken);
                        if (ERROR_SUCCESS != lRes)
                        {
                            hrResult = GenerateError(IDS_CREATE_KEY_FAILED);
                            break;
                        }
                    }
                }

                if (FAILED(hrResult = NextToken(pszToken)))
                    break;

                if (*pszToken == chEquals)
                {
                    if (FAILED(hrResult = AddValue(keyCur, NULL, pszToken))) // NULL == default
                        break;
                }
            }
            else
            {
                if (!bRecover && keyCur.Open(hkParent, pszToken) != ERROR_SUCCESS)
                    bRecover = TRUE;

                // Remember Subkey
                lstrcpyn(pszKey, pszToken, MAX_PATH);

                // If in recovery mode

                if (bRecover || HasSubKeys(keyCur) || HasValues(keyCur))
                {
                    if (FAILED(hrResult = NextToken(pszToken)))
                        break;

                    if (FAILED(hrResult = SkipAssignment(pszToken)))
                        break;

                    if (*pszToken == chLeftBracket)
                    {
                        if (FAILED(hrResult = RegisterSubkeys(keyCur.m_hKey, bRegister, bRecover)))
                            break;
                    }

                    if (!bRecover && HasSubKeys(keyCur))
                    {
                        // See if the KEY is in the NeverDelete list and if so, don't
                        if (CanForceRemoveKey(pszKey))
                            keyCur.RecurseDeleteKey(pszKey);

                        if (FAILED(hrResult = NextToken(pszToken)))
                            break;

                        continue;
                    }
                }

                if (!bRecover && keyCur.Close() != ERROR_SUCCESS)
                {
                    hrResult = GenerateError(IDS_CLOSE_KEY_FAILED);
                    break;
                }

                if (!bRecover && bDelete)
                    RegDeleteKey(hkParent, pszKey);

                if (FAILED(hrResult = NextToken(pszToken)))
                    break;

                if (FAILED(hrResult = SkipAssignment(pszToken)))
                    break;
            }

EndCheck:
            if (bRegister)
            {
                if (*pszToken == chLeftBracket)
                {
                    if (FAILED(hrResult = RegisterSubkeys(keyCur.m_hKey, bRegister, FALSE)))
                        break;

                    if (FAILED(hrResult = NextToken(pszToken)))
                        break;
                }
            }
        }

        break;
    }

Cleanup:

    if (pszKey != NULL)
        delete [] pszKey;

    if (pszToken != NULL)
        delete [] pszToken;

    return hrResult;
}

class CParseBuffer
{
public:
    int nPos;
    int nSize;
    LPTSTR p;
    CParseBuffer(int nInitial);
    ~CParseBuffer() {CoTaskMemFree(p);}
    BOOL AddChar(TCHAR ch);
    LPTSTR Detach();

};

LPTSTR CParseBuffer::Detach()
{
    LPTSTR lp = p;
    p = NULL;
    return lp;
}

CParseBuffer::CParseBuffer(int nInitial)
{
    nPos = 0;
    nSize = nInitial;
    p = (LPTSTR) CoTaskMemAlloc(nSize*sizeof(TCHAR));
}

BOOL CParseBuffer::AddChar(TCHAR ch)
{
    if (nPos == nSize) // realloc
    {
        nSize *= 2;
        p = (LPTSTR) CoTaskMemRealloc(p, nSize*sizeof(TCHAR));
    }

    if (NULL == p)
    {
        nSize = 0;
        return FALSE;
    }

    p[nPos++] = ch;
    return TRUE;
}

HRESULT CRegParser::PreProcessBuffer(LPTSTR lpszReg, LPTSTR* ppszReg)
{
    HRESULT hr = S_OK;
    ASSERT(lpszReg != NULL);
    ASSERT(ppszReg != NULL);

    if (NULL == lpszReg || NULL == ppszReg)
        return E_POINTER;

    *ppszReg = NULL;
    int nSize = lstrlen(lpszReg)*2;
    CParseBuffer pb(nSize);

    if (NULL == pb.p)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    m_pchCur = lpszReg;

    while (*m_pchCur != NULL) // look for end
    {
        if (*m_pchCur == _T('%'))
        {
            IncrementLinePos();
            if (*m_pchCur == _T('%'))
            {
                if (!pb.AddChar(*m_pchCur))
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                
            }
            else
            {
                LPTSTR lpszNext = StrChr(m_pchCur, _T('%'));
                if (lpszNext == NULL)
                {
                    hr = GenerateError(IDS_UNEXPECTED_EOS);
                    break;
                }

                int nLength = (int)(lpszNext - m_pchCur);
                if (nLength > 31)
                {
                    hr = E_FAIL;
                    break;
                }

                TCHAR buf[32];
                lstrcpyn(buf, m_pchCur, nLength+1);
                LPTSTR lpszVar = m_pRegObj->StrFromMap(buf);
                if (lpszVar == NULL)
                {
                    hr = GenerateError(IDS_NOT_IN_MAP);
                    break;
                }

                while (*lpszVar)
                {
                    if (!pb.AddChar(*lpszVar))
                    {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                    }
                    lpszVar++;
                }

                while (m_pchCur != lpszNext)
                    IncrementLinePos();
            }
        }
        else
        {
            if (!pb.AddChar(*m_pchCur))
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        IncrementLinePos();
    }
    pb.AddChar(NULL);

Cleanup:

    if (SUCCEEDED(hr))
        *ppszReg = pb.Detach();
    else
        *ppszReg = NULL;

    return hr;
}


HRESULT CRegParser::RegisterBuffer(LPTSTR szBuffer, BOOL bRegister)
{
    TCHAR   szToken[MAX_PATH];
    HRESULT hr = S_OK;

    LPTSTR szReg;

    if (FAILED(hr = PreProcessBuffer(szBuffer, &szReg)))
        return hr;

    m_pchCur = szReg;

    // Preprocess szReg

    while (chEOS != *m_pchCur)
    {
        if (FAILED(hr = NextToken(szToken)))
            break;

        HKEY hkBase;
        if ((hkBase = HKeyFromString(szToken)) == NULL)
            hr = GenerateError(IDS_BAD_HKEY);
            break;

        if (FAILED(hr = NextToken(szToken)))
            return hr;

        if (chLeftBracket != *szToken)
            hr = GenerateError(IDS_MISSING_OPENKEY_TOKEN);
            break;

        if (bRegister)
        {
            LPTSTR szRegAtRegister = m_pchCur;
            HRESULT hr = RegisterSubkeys(hkBase, bRegister);
            if (FAILED(hr))
            {
                ASSERT(FALSE);

                m_pchCur = szRegAtRegister;
                RegisterSubkeys(hkBase, FALSE);
                break;
            }
        }
        else
        {
            hr = RegisterSubkeys(hkBase, bRegister);
            if (FAILED(hr))
                break;
        }

        SkipWhiteSpace();
    }
    CoTaskMemFree(szReg);
    return hr;
}

HRESULT CExpansionVector::Add(LPCOLESTR lpszKey, LPCOLESTR lpszValue)
{
    HRESULT hr = S_OK;

    EXPANDER*   pExpand = new EXPANDER;

    pExpand->bstrKey = SysAllocString(lpszKey);
    pExpand->bstrValue = SysAllocString(lpszValue);

#ifndef OLE2ANSI
#ifndef UNICODE

    if (pExpand && pExpand->bstrKey && pExpand->bstrValue)
    {

        OLE2T(lpszValue, p);
        pExpand->lpszValue = (LPSTR) CoTaskMemAlloc((lstrlen(p)+1)*sizeof(char));

        if (pExpand->lpszValue)
            lstrcpy(pExpand->lpszValue, p);
        else
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

#endif
#endif

    if (m_cEls == m_nSize)
    {
        m_nSize*=2;
        m_p = (EXPANDER**)realloc(m_p, m_nSize*sizeof(EXPANDER*));
    }

    if (NULL == m_p)
    {
        hr = E_OUTOFMEMORY;
        m_nSize = 0;
        goto Cleanup;
    }

    m_p[m_cEls] = pExpand;
    m_cEls++;

Cleanup:

    if (FAILED(hr))
    {
    
        if (pExpand)
        {

            if (pExpand->bstrKey)
                SysFreeString(pExpand->bstrKey);

            if (pExpand->bstrValue)
                SysFreeString(pExpand->bstrValue);

            delete pExpand;

        }

    }

    return hr;

}

LPTSTR CExpansionVector::Find(LPTSTR lpszKey)
{
    for (int iExpand = 0; iExpand < m_cEls; iExpand++)
    {
        OLE2T(m_p[iExpand]->bstrKey, pszKey);
        if (!lstrcmpi(pszKey, lpszKey)) //are equal
#if defined(UNICODE) | defined(OLE2ANSI)
            return m_p[iExpand]->bstrValue;
#else
            return m_p[iExpand]->lpszValue;
#endif
    }
    return NULL;
}

HRESULT CExpansionVector::ClearReplacements()
{
    for (int iExpand = 0; iExpand < m_cEls; iExpand++)
    {
        EXPANDER* pExp = m_p[iExpand];
        SysFreeString(pExp->bstrKey);
        SysFreeString(pExp->bstrValue);
#ifndef OLE2ANSI
#ifndef UNICODE
        CoTaskMemFree(pExp->lpszValue);
#endif
#endif
        delete pExp;
    }
    m_cEls = 0;
    return S_OK;
}

HRESULT CRegObject::GenerateError(UINT nID, CRegException& re)
{
    re.m_nID    = nID;
    re.m_cLines = -1;

    return DISP_E_EXCEPTION;
}

HRESULT CRegObject::AddReplacement(LPCOLESTR lpszKey, LPCOLESTR lpszItem)
{
    m_csMap.Lock();
    HRESULT hr = m_RepMap.Add(lpszKey, lpszItem);
    m_csMap.Unlock();
    return hr;
}

HRESULT CRegObject::RegisterFromResource(BSTR bstrFileName, LPCTSTR szID,
                                         LPCTSTR szType, CRegException& re,
                                         BOOL bRegister)
{
    HRESULT     hr;
    CRegParser  parser(this);
    HINSTANCE   hInstResDll;
    HRSRC       hrscReg;
    HGLOBAL     hReg;
    DWORD       dwSize;
    LPSTR       szRegA;
    LPTSTR      szReg;

    OLE2T(bstrFileName, pszFilename);
    hInstResDll = LoadLibraryEx(pszFilename, NULL, LOAD_LIBRARY_AS_DATAFILE);

    if (hInstResDll == NULL)
    {
        ASSERT(FALSE);

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ReturnHR;
    }

    hrscReg = FindResource((HMODULE)hInstResDll, szID, szType);

    if (NULL == hrscReg)
    {
        ASSERT(FALSE);

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ReturnHR;
    }

    hReg = LoadResource((HMODULE)hInstResDll, hrscReg);

    if (NULL == hReg)
    {
        ASSERT(FALSE);

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ReturnHR;
    }

    dwSize = SizeofResource((HMODULE)hInstResDll, hrscReg);
    szRegA = (LPSTR)hReg;
    if (szRegA[dwSize] != NULL)
    {
        szRegA = (LPSTR)malloc(dwSize+1);

        if (NULL == szRegA)
        {
            hr = E_OUTOFMEMORY;
            goto ReturnHR;
        }

        memcpy(szRegA, (void*)hReg, dwSize+1);
        szRegA[dwSize] = NULL;
        szReg = A2T(szRegA);
        free(szRegA);
    }
    else
        szReg = A2T(szRegA);

    hr = parser.RegisterBuffer(szReg, bRegister);

    if (FAILED(hr))
        re = parser.GetRegException();


ReturnHR:
    if (NULL != hInstResDll)
        FreeLibrary((HMODULE)hInstResDll);

    return hr;
}

static LPCTSTR StringFromResID(VARIANT& var)
{
    CComVariant varTemp;

    if (FAILED(VariantChangeType(&varTemp, &var, VARIANT_NOVALUEPROP, VT_I2)))
        return NULL;

    return MAKEINTRESOURCE(varTemp.iVal);

}

HRESULT CRegObject::ResourceRegister(BSTR bstrFileName, VARIANT varID,
                               VARIANT varType, CRegException& re)
{
    CString strID;
    if (VT_BSTR == varID.vt)
        strID = varID.bstrVal;

    CString strType;
    if (VT_BSTR == varType.vt)
        strType = varType.bstrVal;

    return RegisterFromResource(bstrFileName,
                                (varID.vt == VT_BSTR) ? strID : StringFromResID(varID),
                                (varType.vt == VT_BSTR) ? strType : StringFromResID(varType),
                                re,
                                TRUE);
}

HRESULT CRegObject::ResourceUnregister(BSTR bstrFileName, VARIANT varID,
                               VARIANT varType, CRegException& re)
{
    CString strID;
    if (VT_BSTR == varID.vt)
        strID = varID.bstrVal;

    CString strType;
    if (VT_BSTR == varType.vt)
        strType = varType.bstrVal;

    return RegisterFromResource(bstrFileName,
                                (varID.vt == VT_BSTR) ? strID : StringFromResID(varID),
                                (varType.vt == VT_BSTR) ? strType : StringFromResID(varType),
                                re,
                                FALSE);
}


HRESULT CRegObject::RegisterWithString(BSTR bstrData, BOOL bRegister, CRegException& re)
{
    CRegParser  parser(this);

    OLE2T(bstrData, szReg);

    HRESULT hr = parser.RegisterBuffer(szReg, bRegister);

    if (FAILED(hr))
        re = parser.GetRegException();

    return hr;
}

HRESULT CRegObject::ClearReplacements()
{
    m_csMap.Lock();
    HRESULT hr = m_RepMap.ClearReplacements();
    m_csMap.Unlock();
    return hr;
}

HRESULT CRegObject::DeleteKey(BSTR bstrKey, CRegException& re)
{
    LPTSTR szTail;

    OLE2T(bstrKey, pszKey);
    HKEY hkeyRoot = HKeyFromCompoundString(pszKey, szTail);
    if (NULL == hkeyRoot)
        return GenerateError(IDS_BAD_HKEY, re);

    CRegKey key;
    key.Attach(hkeyRoot);
    LONG lRes = key.RecurseDeleteKey(szTail);
    if (ERROR_SUCCESS != lRes)
        return GenerateError(IDS_DELETE_KEY_FAILED, re);

    return S_OK;
}

HRESULT CRegObject::AddKey(BSTR keyName, CRegException& re)
{
    LPTSTR szTail;

    OLE2T(keyName, pszKeyName);
    HKEY hkeyRoot = HKeyFromCompoundString(pszKeyName, szTail);
    if (NULL == hkeyRoot)
        return GenerateError(IDS_BAD_HKEY, re);

    CRegKey key;
    LONG lRes = key.Create(hkeyRoot, szTail);
    if (ERROR_SUCCESS != lRes)
        return GenerateError(IDS_CREATE_KEY_FAILED, re);

    return S_OK;
}

HRESULT CRegObject::SetKeyValue(BSTR keyName, BSTR valueName, VARIANT value,
    CRegException& re, BOOL bCreateKey)
{
    LPTSTR szTail;

    OLE2T(keyName, pszKeyName);
    HKEY hkeyRoot = HKeyFromCompoundString(pszKeyName, szTail);
    if (hkeyRoot == NULL)
        return GenerateError(IDS_BAD_HKEY, re);

    CRegKey key;
    LONG lRes;
    if (bCreateKey)
        lRes = key.Create(hkeyRoot, szTail);
    else
        lRes = key.Open(hkeyRoot, szTail);
    if (ERROR_SUCCESS != lRes)
        return GenerateError(IDS_CREATE_KEY_FAILED, re);

    OLE2T(valueName, pval);
    if (*valueName == 0)
        pval = NULL;

    CComVariant varString;
    BSTR pb = value.bstrVal;
    BYTE*   pData;
    long    lBound, uBound;
    UINT nIDRes = IDS_CONVERT_FAILED;
    HRESULT hr = S_OK;
    switch (value.vt)
    {
        case VT_I2:
            lRes = key.SetValue(value.iVal, pval);
            break;
        case VT_I4:
            lRes = key.SetValue(value.lVal, pval);
            break;
        case VT_R4:
        case VT_R8:
        case VT_BOOL:
        case VT_DATE:
        case VT_CY:
            lRes = ERROR_SUCCESS;
            hr = DISP_E_TYPEMISMATCH;
            nIDRes = IDS_VALUE_SET_FAILED;
            break;
        case VT_BSTR:
        {
            OLE2T(pb, szpb);
            lRes = key.SetValue(szpb, pval);
            break;
        }
        case VT_SAFEARRAY|VT_UI1:
            hr = E_FAIL;
            if (SafeArrayGetDim(value.parray) != 1) // Verify 1 Dimension
                break;

            hr = SafeArrayGetLBound(value.parray, 1, &lBound);
            if (FAILED(hr))
                break;

            hr = SafeArrayGetUBound(value.parray, 1, &uBound);
            if (FAILED(hr))
                break;
            hr = SafeArrayAccessData(value.parray, (void**)&pData);
            if (FAILED(hr))
                break;

            lRes = RegSetValueEx(key, pval, 0, REG_BINARY, pData,
                (uBound - lBound) + 1);
            SafeArrayUnaccessData(value.parray);
            break;
        default:
            lRes = ERROR_INVALID_DATA;
    }

    if (ERROR_SUCCESS != lRes)
        return GenerateError(IDS_VALUE_SET_FAILED, re);
    if (FAILED(hr))
        return GenerateError(nIDRes, re);

    return S_OK;
}

HRESULT CRegObject::GetKeyValue(BSTR keyName, BSTR valueName, VARIANT* value, CRegException& re)
{
    LPTSTR      szTail;
    HRESULT     hr = S_OK;

    OLE2T(keyName, szKeyName);
    HKEY hkeyRoot = HKeyFromCompoundString(szKeyName, szTail);
    if (NULL == hkeyRoot)
        return GenerateError(IDS_BAD_HKEY, re);

    CRegKey key;
    LONG lRes = key.Open(hkeyRoot, szTail);
    if (ERROR_SUCCESS != lRes)
        return GenerateError(IDS_OPEN_KEY_FAILED, re);

    DWORD dwType;
    OLE2T(valueName, szValueName);
    LPVOID pData = QueryValue(key.m_hKey, szValueName, dwType);
    value->vt = VT_EMPTY;

    if (NULL == pData)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    switch (dwType)
    {
        case REG_DWORD:
            value->vt = VT_I4;
            value->lVal = *((long *) pData);
            break;
        case REG_SZ:
        {
            value->vt = VT_BSTR;
            T2OLE((LPTSTR)pData, posData);
            value->bstrVal = SysAllocString(posData);
            if (NULL == value->bstrVal)
                hr = E_OUTOFMEMORY;
            break;
        }
        default:
            hr = E_FAIL;
    }

Cleanup:

    if (pData)
        CoTaskMemFree(pData);

    if (FAILED(hr))
        return GenerateError(IDS_VALUE_GET_FAILED, re);

    return hr;
}

LPTSTR CRegObject::StrFromMap(LPTSTR lpszKey)
{
    m_csMap.Lock();
    LPTSTR lpsz = m_RepMap.Find(lpszKey);

    ASSERT(lpsz != NULL);

    m_csMap.Unlock();
    return lpsz;
}

HRESULT CRegObject::MemMapAndRegister(BSTR bstrFileName, BOOL bRegister,
                                      CRegException& re)
{
    CRegParser  parser(this);

    OLE2T(bstrFileName, strFilename);

    HANDLE hFile = CreateFile(strFilename, GENERIC_READ, 0, NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_READONLY,
                              NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ASSERT(FALSE);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    DWORD cbFile = GetFileSize(hFile, NULL); // No HiOrder DWORD required

    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (hMapping == NULL)
    {
        ASSERT(FALSE);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LPVOID pMap = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);

    if (pMap == NULL)
    {
        ASSERT(FALSE);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LPTSTR szReg = A2T((char*)pMap);

    if (chEOS != szReg[cbFile]) //ensure buffer is NULL terminated
    {
        ASSERT(FALSE);
        return E_FAIL; // make a real error
    }

    HRESULT hRes = parser.RegisterBuffer(szReg, bRegister);

    if (FAILED(hRes))
        re = parser.GetRegException();

    UnmapViewOfFile(pMap);
    CloseHandle(hMapping);
    CloseHandle(hFile);

    return hRes;
}

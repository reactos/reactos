//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: urlhook.cpp
//
// History:
//         9-24-96  by dli
//------------------------------------------------------------------------

#include "priv.h"
#include "sccls.h"
#include "resource.h"

#include <mluisupp.h>

// CURRENT_USER
static const TCHAR c_szSearchUrl[]     = TSZIEPATH TEXT("\\SearchUrl");


#define TF_URLSEARCHHOOK 0

// structure for the character replacement in URL searches
typedef struct _SUrlCharReplace {
    TCHAR from;
    TCHAR to[10];
} SUrlCharReplace;


class CURLSearchHook : public IURLSearchHook
{
public:
    CURLSearchHook();
    
    // *** IUnknown Methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IURLSearchHook
    virtual STDMETHODIMP Translate(LPWSTR lpwszSearchURL, DWORD cchBufferSize);
        
protected:
    // IUnknown 
    UINT _cRef;

    HRESULT _IsKeyWordSearch(LPCTSTR pcszURL);
    HRESULT _IsURLSearchable(LPTSTR pszURL, HKEY * phkeySearch, LPCTSTR * pcszQuery);
    HRESULT _ReplaceChars(HKEY hkeySearch, LPCTSTR pcszQuery, PTSTR pszReplaced, int cchReplaced);
    HRESULT _Search(HKEY hkeySearch, LPCTSTR pcszQuery, PTSTR pszTranslatedURL, DWORD cchTranslatedUrl); 
    void    _ConvertToUtf8(LPWSTR pszQuery, int cch);

}; 


#ifdef DEBUG
#define _AddRef(psz) { ++_cRef; TraceMsg(TF_URLSEARCHHOOK, "CURLSearchHook(%x)::QI(%s) is AddRefing _cRef=%lX", this, psz, _cRef); }
#else
#define _AddRef(psz)    ++_cRef
#endif


CURLSearchHook::CURLSearchHook()
    : _cRef(1)
{
        DllAddRef();
}


HRESULT CURLSearchHook::QueryInterface(REFIID riid, LPVOID * ppvObj)
{ 
    // ppvObj must not be NULL
    ASSERT(ppvObj != NULL);
    
    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;
    if (IsEqualIID(riid, IID_IUnknown))
    {    
        *ppvObj = SAFECAST(this, IUnknown *);
        TraceMsg(TF_URLSEARCHHOOK, "QI IUnknown succeeded");
    }
    else if (IsEqualIID(riid, IID_IURLSearchHook))
    {
        *ppvObj = SAFECAST(this, IURLSearchHook*);
        TraceMsg(TF_URLSEARCHHOOK, "QI IURLSEARCHHOOK succeeded");
    } 
    else
        return E_NOINTERFACE;  // Otherwise, don't delegate to HTMLObj!!
     
    
    _AddRef(TEXT("IURLSearchHook"));
    return S_OK;
}


ULONG CURLSearchHook::AddRef()
{
    _cRef++;
    TraceMsg(TF_URLSEARCHHOOK, "CURLSearchHook(%x)::AddRef called, new _cRef=%lX", this, _cRef);
    return _cRef;
}

ULONG CURLSearchHook::Release()
{
    _cRef--;
    TraceMsg(TF_URLSEARCHHOOK, "CURLSearchHook(%x)::Release called, new _cRef=%lX", this, _cRef);
    if (_cRef > 0)
        return _cRef;

    delete this;
    DllRelease();
    return 0;
}

HRESULT CURLSearchHook::_IsKeyWordSearch(LPCTSTR pcszURL)
{
    TCHAR szAcceptedRequestKey[256];
    
    LPTSTR lpsz = szAcceptedRequestKey;
    LPTSTR lpszKey = szAcceptedRequestKey;
   
    // load the accepted request keywords and compare them with what the user typed in
    MLLoadString(IDS_URL_SEARCH_KEY, szAcceptedRequestKey, ARRAYSIZE(szAcceptedRequestKey)-1);
    
    int RequestKeyLen = 0;
    while (*lpsz) {
        if (*lpsz == TEXT(' ')){ 
            if (! StrCmpNI(pcszURL, lpszKey, RequestKeyLen+1))      
                return S_OK;        
            else {
                lpsz++;
                lpszKey = lpsz;
                RequestKeyLen = 0;
            }
        }
        else {      
            lpsz++;
            RequestKeyLen++;
        }
    }
    
    return S_FALSE;
}   


// This function determines if we will do an autosearch on the string user typed in 
//
// Priorities:
// 1  ---  Key word search: search with "go", "find" and so on
// 2  ---  possible URL address: contains '.', ':', '/' and '\\', so don't search 
// 3  ---  Space triggered search.
// 4  ---  Don't search. 
HRESULT CURLSearchHook::_IsURLSearchable(LPTSTR pszURL, HKEY * phkeySearch, LPCTSTR * ppcszQuery)
{
    BOOL fExtendedChar = FALSE;
    TCHAR szRegSearchKey[MAX_PATH];
    LPTSTR pszKey = StrChr(pszURL, TEXT(' '));
    if (pszKey == NULL)
    {
        // No keyword, but if any of the characters are non-ascii, we will default
        // to search because it's likely not a url
        fExtendedChar = HasExtendedChar(pszURL);
        if (!fExtendedChar)
            return S_FALSE;

        pszKey = pszURL;
    }
    
    StrCpyN(szRegSearchKey, c_szSearchUrl, ARRAYSIZE(szRegSearchKey));
    
    if ((_IsKeyWordSearch(pszURL) == S_FALSE) && !fExtendedChar)        
    {
        // Find the end of the default Registry Subkey and 
        // append the keyword so the regkey becomes:
        // Software\Microsoft\Internet Explorer\SearchUrl\go
        ASSERT((ARRAYSIZE(c_szSearchUrl) + 1) < ARRAYSIZE(szRegSearchKey));
        PTSTR pszEnd = &szRegSearchKey[ARRAYSIZE(c_szSearchUrl) - 1];
        *pszEnd++ = TEXT('\\');
        const int cchBuf = ARRAYSIZE(szRegSearchKey) - (ARRAYSIZE(c_szSearchUrl) + 1);
        const int cchToCopy = (int) (pszKey - pszURL + 1);
        StrCpyN(pszEnd, pszURL, min(cchBuf, cchToCopy));

        // See if this is a search keyword in the registry
        if (OpenRegUSKey(szRegSearchKey, 0, KEY_READ, phkeySearch) == ERROR_SUCCESS)
        {  
            PathRemoveBlanks(pszKey);
            *ppcszQuery = pszKey;
            return S_OK;
        }

        // No keyword so use entire "url" for the search
        pszKey = pszURL;

        if (StrCSpn(pszURL, TEXT(".:/\\")) != lstrlen(pszURL))
        {
            return S_FALSE;
        }
    }
    
    // Null out the key to signal that we should use the internal hard-coded search string
    *phkeySearch = NULL;
    PathRemoveBlanks(pszKey);
    *ppcszQuery = pszKey;
    return S_OK;
}

HRESULT CURLSearchHook::_ReplaceChars(HKEY hkeySearch, LPCTSTR pcszQuery, LPTSTR pszReplaced, int cchReplaced)
{
    // The following are strings and its lengthes passed in RegEnumValue
    TCHAR szOrig[2];
    DWORD dwOrigLen;
    
    TCHAR szMatch[10];
    DWORD dwMatchLen;
    
    HDSA  hdsaReplace = NULL;
    
    // If we are using our hard-coded search url, we get the char replacements from the string table
    if (NULL == hkeySearch)
    {
        WCHAR szSub[MAX_PATH];
        if (MLLoadString(IDS_SEARCH_SUBSTITUTIONS, szSub, ARRAYSIZE(szSub)) && *szSub != NULL)
        {
            // The first char is our deliminator followed by replacement pairs (", ,+,#,%23,&,%26,?,%3F,+,%2B,=,%3d")
            WCHAR chDelim = szSub[0];
            LPWSTR pszFrom = &szSub[1];
            BOOL fDone = FALSE;
            LPWSTR pszNext;
            do
            {
                // Null terminater our source string
                LPWSTR pszTo = StrChr(pszFrom, chDelim);
                if (NULL == pszTo)
                {
                    break;
                }
                *pszTo = L'\0';

                // Null terminate the dest string
                ++pszTo;
                LPWSTR pszToEnd = StrChr(pszTo, chDelim);
                if (pszToEnd)
                {
                    *pszToEnd = L'\0';
                    pszNext = pszToEnd + 1;
                }
                else
                {
                    pszNext = NULL;
                }
        
                // If the "from" string is one char and the "to" substitution fits, store this pair
                SUrlCharReplace scr;
                if (pszFrom[1] == L'\0' && lstrlen(pszTo) < ARRAYSIZE(scr.to))
                {
                    scr.from = pszFrom[0];
                    StrCpyN(scr.to, pszTo, ARRAYSIZE(scr.to));
        
                    if (!hdsaReplace)
                        hdsaReplace = DSA_Create(SIZEOF(SUrlCharReplace), 4); 
                    if (hdsaReplace)
                        DSA_AppendItem(hdsaReplace, &scr);
                }

                pszFrom = pszNext;
            } 
            while (pszNext != NULL);
        }
    }

    // The search url is in the registry, so get the char substitutions from there
    else
    {
        DWORD dwType;
        LONG lRegEnumResult;
        DWORD dwiValue = 0; 
        do
        {
            dwOrigLen = ARRAYSIZE(szOrig);
            dwMatchLen = SIZEOF(szMatch);
            lRegEnumResult = RegEnumValue(hkeySearch, dwiValue, szOrig,
                                          &dwOrigLen, NULL, &dwType, (PBYTE)szMatch,
                                          &dwMatchLen);
            dwiValue++;         
            SUrlCharReplace         scr;
            
            if ((lRegEnumResult == ERROR_SUCCESS) && (dwType == REG_SZ) && (dwOrigLen == 1) 
                && dwMatchLen < ARRAYSIZE(scr.to))
            {
                scr.from = szOrig[0];
                StrCpyN(scr.to, szMatch, ARRAYSIZE(scr.to));
            
                if (!hdsaReplace)
                    hdsaReplace = DSA_Create(SIZEOF(SUrlCharReplace), 4); 
                if (hdsaReplace)
                    DSA_AppendItem(hdsaReplace, &scr);
            }       
        } while ((lRegEnumResult == ERROR_SUCCESS) || (lRegEnumResult == ERROR_MORE_DATA));
    }
            
            
    if (hdsaReplace)
    {
        // Replace all characters found in the registry by their matches in the search key word
        LPTSTR lpHead = pszReplaced;
        int cchHead = cchReplaced;
        int ich;
        int ihdsa;
        BOOL bCharFound;
        int querylen = lstrlen(pcszQuery);
        for (ich = 0; ich < querylen && cchHead > 1; ich++)
        {
            bCharFound = FALSE;
            // First look through the DSA array to find a match
            for (ihdsa = 0; ihdsa < DSA_GetItemCount(hdsaReplace); ihdsa++)
            {
                SUrlCharReplace *pscr;
                pscr = (SUrlCharReplace *)DSA_GetItemPtr(hdsaReplace, ihdsa);
                if (pscr && pscr->from == pcszQuery[ich])
                {
                    int szLen = lstrlen(pscr->to);
                    StrCpyN(lpHead, pscr->to, cchHead);    
                    lpHead += szLen;
                    cchHead -= szLen;
                    bCharFound = TRUE;
                    break;
                }
            }
            
            // Copy the character over if there is no replacements
            if (!bCharFound)
            {
                *lpHead = pcszQuery[ich];
                lpHead++;
                cchHead--;
            }
        }       

        if (cchHead > 0)
            *lpHead = 0;
        
        DSA_Destroy(hdsaReplace);
    }
    
    return S_OK;
    
}       

void  CURLSearchHook::_ConvertToUtf8(LPWSTR pszQuery, int cch)
{
    // Only need to covert if extended characters found
    if (HasExtendedChar(pszQuery))
    {
        ConvertToUtf8Escaped(pszQuery, cch);
    }
}

// pszTranslatedUrl is the output of this function
HRESULT CURLSearchHook::_Search(HKEY hkeySearch, LPCTSTR pcszQuery, PTSTR pszTranslatedUrl, DWORD cchTranslatedUrl)
{
    HRESULT hr = E_FAIL;

    // Get the search provider from the registry 
    DWORD dwType;
    WCHAR szProvider[MAX_PATH];
    szProvider[0] = 0;
    DWORD cbProvider = sizeof(szProvider);
    if (SHRegGetUSValue(c_szSearchUrl, L"Provider", &dwType, &szProvider, &cbProvider, FALSE, NULL, 0) != ERROR_SUCCESS ||
        dwType != REG_SZ)
    {
        szProvider[0] = 0;
    }

    TCHAR szSearchPath[MAX_URL_STRING];
    DWORD dwSearchPathLen = SIZEOF(szSearchPath);        
        
    // Find the search URL in the registry or our string table
    BOOL fSuccess;
    if (hkeySearch)
    {
        fSuccess = (RegQueryValueEx(hkeySearch, NULL, NULL, NULL, (PBYTE)szSearchPath, &dwSearchPathLen) == ERROR_SUCCESS);
    }
    else
    {

        // See if we want the hardcoded intranet or internet url
        UINT ids = (StrCmpI(szProvider, L"Intranet") == 0) ? IDS_SEARCH_INTRANETURL : IDS_SEARCH_URL;

        // Use our internal hard-coded string
        fSuccess = MLLoadString(ids, szSearchPath, ARRAYSIZE(szSearchPath));
    }

    if (fSuccess && lstrlen(szSearchPath) > 1)
    {
        // 1. Look in the registry and find all of the original characters and it's
        // matches and store them in the DSA arrays of SURlCharReplace
        // 2. Replace all of the occurences of the original characters in the 
        // URL search key word by their matches.
        // 3. Append the search URL and the search key words
        
        TCHAR szURLReplaced[MAX_URL_STRING];
        
        StrCpyN(szURLReplaced, pcszQuery, ARRAYSIZE(szURLReplaced));
        _ReplaceChars(hkeySearch, pcszQuery, szURLReplaced, ARRAYSIZE(szURLReplaced));

        //
        // If we are using our search engine, convert the string to UTF8 and escape it
        // so that it appears like normal ascii
        //
        if (NULL == hkeySearch)
        {
            _ConvertToUtf8(szURLReplaced, ARRAYSIZE(szURLReplaced));
        }

        // If this is an old-style url, there will be a %s in it for the search string.
        // Otherwise there will be the following parameters:
        //
        // http://whatever.com?p=%1&srch=%2&prov=%3&utf8
        //
        //  %1 = search string
        //  %2 = how to display results:
        //        "1" = just show me results in full window
        //        "2" = show results in full window, but redirect if possible
        //        "3" = show results in the search pane, and take me to the most
        //              likely site in the main window if there is one
        //  %3 = search provider name
        //
        LPWSTR pszParam1 = StrStr(szSearchPath, L"%1");
        if (NULL != pszParam1)
        {
            //
            // We can't use FormatMessage because on win95 it converts to ansi
            // using the system code page and the translation back is lossy.
            // So we'll replace the parameters ourselves. Arrrggg.
            //

            // First convert %1 to %s
            pszParam1[1] = L's';

            // Next replace %2 with the display option in %2 
            LPWSTR pszParam2 = StrStr(szSearchPath, L"%2");
            if (NULL != pszParam2)
            {
                DWORD dwValue;
                DWORD cbValue = sizeof(dwValue);
                if (SHRegGetUSValue(REGSTR_PATH_MAIN, L"AutoSearch", &dwType, &dwValue, &cbValue, FALSE, NULL, 0) != ERROR_SUCCESS ||
                    dwValue > 9)
                {
                    // Default to "display results in search pane and go to most likely site"
                    dwValue = 3;
                }
                *pszParam2 = (WCHAR)dwValue + L'0';
                StrCpyN(pszParam2 + 1, pszParam2 + 2, ARRAYSIZE(szSearchPath)); // can never overflow
            }

            // Finally, find the third Param and convert it to %s too
            LPWSTR pszParam3 = StrStr(szSearchPath, L"%3");
            if (pszParam3)
            {
                // Insert the provider in the third param
                WCHAR szTemp[MAX_URL_STRING];
                StrCpyN(szTemp, pszParam3 + 2, ARRAYSIZE(szTemp));
                *pszParam3 = 0;
                StrCatBuff(szSearchPath, szProvider, ARRAYSIZE(szSearchPath));
                StrCatBuff(szSearchPath, szTemp, ARRAYSIZE(szSearchPath));
            }
        }

        // Now replace the %s with the search string
        wnsprintf(pszTranslatedUrl, cchTranslatedUrl, szSearchPath, szURLReplaced);
        hr = S_OK;
    }

    if (hkeySearch)
        RegCloseKey(hkeySearch);
    return hr;
}

HRESULT CURLSearchHook::Translate(LPWSTR lpwszSearchURL, DWORD cchBufferSize)
{
    HRESULT hr = E_FAIL;
    TCHAR szSearchURL[MAX_URL_STRING];

    SHUnicodeToTChar(lpwszSearchURL, szSearchURL, ARRAYSIZE(szSearchURL));
    
    HKEY hkeySearch;
    LPCTSTR pcszQuery;
    if (_IsURLSearchable(szSearchURL, &hkeySearch, &pcszQuery) == S_OK)
    {
        hr = _Search(hkeySearch, pcszQuery, szSearchURL, ARRAYSIZE(szSearchURL));
        if (hr == S_OK)
            SHTCharToUnicode(szSearchURL, lpwszSearchURL, cchBufferSize); 
    }
    
    return hr;
}
        

#ifdef DEBUG
extern void remove_from_memlist(void *pv);
#endif

STDAPI CURLSearchHook_CreateInstance(IUnknown* pUnkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    HRESULT hr = E_OUTOFMEMORY;

    CURLSearchHook *pcush = new CURLSearchHook;

    if (pcush)
    {
#ifdef DEBUG
        //
        // HACK:(dli)
        //
        //   IURLSearchHook objects are free-threaded objects, meaning that
        // they are cacheed and shared between different IEXPLORE processes, 
        // and they are only deleted when the SHDOCVW DLL ref count is 0. 
        // So, we can remove them from the SATOSHI's memlist.
        //
        // By the way, SATOSHI has Okayed this. Don't copy this code without
        // talking to SATOSHI.
        //
        remove_from_memlist(pcush);
#endif

        *ppunk = (IUnknown *) pcush;
        hr = S_OK;
    }

    return hr;
}


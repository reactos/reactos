
/*
 * url.cpp - IUniformResourceLocator implementation for InternetShortcut class.
 */


/* Headers
 **********/

#include "priv.h"
#pragma hdrstop
#define INC_OLE2
#include "intshcut.h"


/* Module Constants
 *******************/

const TCHAR c_szURLPrefixesKey[]        = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\URL\\Prefixes");
const TCHAR c_szDefaultURLPrefixKey[]   = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\URL\\DefaultPrefix");

// DPA array that holds the IURLSearchHook Pointers
static HDPA g_hdpaHooks = NULL;

// CURRENT_USER
static const TCHAR c_szURLSearchHook[] = TSZIEPATH TEXT("\\URLSearchHooks");


/***************************** Private Functions *****************************/


int DPA_DestroyURLSearchHooksCallback(LPVOID p, LPVOID d)
{
    IURLSearchHook * psuh = (IURLSearchHook *)p;
    ASSERT(psuh);
    ATOMICRELEASET(psuh, IURLSearchHook);

    return 1; 
}

extern "C" {
    
void DestroyHdpaHooks()
{
    if (g_hdpaHooks)
    {
        ENTERCRITICAL;
        //---------------------------- Critical Section -------------------------
        HDPA hdpa = g_hdpaHooks;
        g_hdpaHooks = NULL;
        //-----------------------------------------------------------------------
        LEAVECRITICAL;
        if (hdpa)
            DPA_DestroyCallback(hdpa, DPA_DestroyURLSearchHooksCallback, 0);

    }
}

}

HRESULT InvokeURLSearchHook(IURLSearchHook * pusHook, LPCTSTR pcszQuery, LPTSTR pszResult) 
{
    HRESULT hr = E_FAIL;
    
    ASSERT(pusHook);
    WCHAR szSearchURL[MAX_URL_STRING]; 

    SHTCharToUnicode(pcszQuery, szSearchURL, ARRAYSIZE(szSearchURL));
        
    hr = pusHook->Translate(szSearchURL, ARRAYSIZE(szSearchURL));
            
    // In case the URLSearchHook worked, convert result to TCHAR
    // This includes two cases: S_OK and S_FALSE
    if (SUCCEEDED(hr))
    {
        //BUGBUG: (dli) Assuming pszResult size = MAX_URL_STRING 
        SHUnicodeToTChar(szSearchURL, pszResult, MAX_URL_STRING);
    }

    return hr;    
}


/* 
 * Returns: 
 * S_OK         Search handled completely, pszResult has the full URL to browse to. 
 * 0x00000000   Stop running any further IURLSearchHooks and pass this URL back to 
 *              the browser for browsing.
 *
 * S_FALSE      Query has been preprocessed, pszResult has the result of the preprocess, 
 * 0x00000001   further search still needed. Go on executing the rest of the IURLSearchHooks 
 *              The preprocessing steps can be: 1. replaced certain characters
 *                                              2. added more hints 
 *
 * E_ABORT      Search handled completely, stop running any further IURLSearchHooks, 
 * 0x80004004   but NO BROWSING NEEDED as a result, pszResult is a copy of pcszQuery. 
 *              BUGBUG: This is not fully implemented, yet, making IURLQualify return this
 *              involves too much change. 
 * 
 * E_FAIL       This Hook was unsuccessful. Search not handled at all, pcszQueryURL has the 
 * 0x80004005   query string. Please go on running other IURLSearchHooks. 
 * return
 */

HRESULT TryURLSearchHooks(LPCTSTR pcszQuery, LPTSTR pszResult)
{
    HRESULT hr = E_FAIL;
    
    TCHAR szNewQuery[MAX_URL_STRING];
    StrCpyN(szNewQuery, pcszQuery, ARRAYSIZE(szNewQuery));
    
    int ihdpa;
    for (ihdpa = 0; ihdpa < DPA_GetPtrCount(g_hdpaHooks); ihdpa++)
    {
        IURLSearchHook * pusHook;
        pusHook = (IURLSearchHook *) DPA_GetPtr(g_hdpaHooks, ihdpa);
        if (!pusHook)
            return E_FAIL;
        hr = InvokeURLSearchHook(pusHook, szNewQuery, pszResult);
        if ((hr == S_OK) || (hr == E_ABORT))
            break;
        else if (hr == S_FALSE)
            StrCpyN(szNewQuery, pszResult, ARRAYSIZE(szNewQuery));
    }

    return hr;
}


void InitURLSearchHooks()
{
    HDPA hdpa = DPA_Create(4);
    
    // We need to look in LOCAL_MACHINE if this registry entry doesn't exist in CURRENT_USER.
    // The installer needs to install the values into LOCAL_MACHINE so they are accessable
    // to all users.  Then anyone wanting to modify the value, will need to determine if they
    // want to add it to a specific user's CURRENT_USER or modify the LOCAL_MACHINE value to 
    // apply the change to all users.  (bryanst - #6722)
    HUSKEY hkeyHooks;
    if ((hdpa) && (SHRegOpenUSKey(c_szURLSearchHook, KEY_READ, NULL, &hkeyHooks, FALSE) == ERROR_SUCCESS))
    {    
        TCHAR szCLSID[GUIDSTR_MAX];
        DWORD dwccCLSIDLen;
        LONG lEnumReturn;
        DWORD dwiValue = 0;
        
        do {
            dwccCLSIDLen = ARRAYSIZE(szCLSID);
            lEnumReturn = SHRegEnumUSValue(hkeyHooks, dwiValue, szCLSID, &dwccCLSIDLen, 
                                       NULL, NULL, NULL, SHREGENUM_DEFAULT);
            if (lEnumReturn == ERROR_SUCCESS)
            {
                CLSID clsidHook;
                if (SUCCEEDED(SHCLSIDFromString(szCLSID, &clsidHook)))
                {
                    IURLSearchHook * pusHook;

                    HRESULT hr = CoCreateInstance(clsidHook, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, 
                                                  IID_IURLSearchHook, (LPVOID *)&pusHook);
        
                    if (SUCCEEDED(hr))
                        DPA_AppendPtr(hdpa, pusHook);
                }   
            }
            dwiValue++;            
        } while (lEnumReturn == ERROR_SUCCESS);
        
        SHRegCloseUSKey(hkeyHooks);
    }
    
    ENTERCRITICAL;
    //---------------------------- Critical Section --------------------------
    if (!g_hdpaHooks)
    {
        g_hdpaHooks = hdpa;
        hdpa = NULL;
    }
    //------------------------------------------------------------------------
    LEAVECRITICAL;
    
    if (hdpa)
        DPA_DestroyCallback(hdpa, DPA_DestroyURLSearchHooksCallback, 0);

}

    
HRESULT ApplyURLSearch(LPCTSTR pcszQuery, LPTSTR pszTranslatedUrl)
{
    if (!g_hdpaHooks)
        InitURLSearchHooks();   
    
    return TryURLSearchHooks(pcszQuery, pszTranslatedUrl);
}


/*----------------------------------------------------------
Purpose: This function qualifies a string as a URL.  Strings
         such as "www.foo.com" would have the scheme guessed
         if the correct flags are given.  Local paths are 
         converted to "file:" URLs.

         pszTranslatedURL may point to the same buffer as 
         pcszURL.

         If the given string is already a URL (not necessarily
         canonicalized, though), this function will not touch it, 
         unless UQF_CANONICALIZE is set, in which case the string 
         will be canonicalized.

Returns: S_OK or S_FALSE means we filled in pszTranslatedURL.
         S_OK means we altered the URL to qualify it too.
         various failure codes too

Cond:    --
*/
SHDOCAPI
IURLQualify(
    IN  LPCWSTR pcszURL, 
    IN  DWORD   dwFlags,         // UQF_*
    OUT LPWSTR  pszTranslatedURL,
    LPBOOL      pbWasSearchURL,
    LPBOOL      pbWasCorrected)
{
    HRESULT hres = S_FALSE;
    DWORD cchSize;

    SHSTR strOut;
    BOOL bWasCorrected = FALSE; 

    ASSERT(IS_VALID_STRING_PTR(pcszURL, -1));
    ASSERT(IS_VALID_WRITE_BUFFER(pszTranslatedURL, TCHAR, MAX_URL_STRING));

    if (pbWasSearchURL)
        *pbWasSearchURL = FALSE;

    // Special cases: URLs of the form <drive>:<filename>
    //                URLs of the form \<filename>
    // we'll assume that if the second character is a : or |, this is an url of
    // that form, and we will guess "file://" for the prefix.
    // we'll assume any url that begins with a single \ is a file: url
 
    // NOTE: We do this here because these are cases where the protocol is 
    // left off, and is likely to be incorrectly guessed, such as a 
    // relative path \data\ftp\docs, would wrongly be turned 
    // into "ftp://\data\ftp\docs".
 

    // Note: PathIsURL returns TRUE for non-canonicalized URLs too
    if (PathIsURL(pcszURL))
    {
        LPCWSTR pcszTemp = pcszURL;
        cchSize = MAX_URL_STRING;
        if (IsFlagSet(dwFlags, UQF_AUTOCORRECT))
        {
            hres = UrlFixup(pcszURL, pszTranslatedURL, cchSize);
            if (hres == S_OK)
            {
                bWasCorrected = TRUE;
                pcszTemp = pszTranslatedURL;
            }
        }

        if (dwFlags & UQF_CANONICALIZE)
            hres = UrlCanonicalize(pcszTemp, pszTranslatedURL, &cchSize, 0);
        else if (pszTranslatedURL != pcszTemp)
            StrCpyN(pszTranslatedURL, pcszTemp, MAX_URL_STRING);

        hres = S_OK;
    }
    else
    {
        // Look for file paths
        if (IsFlagClear(dwFlags, UQF_IGNORE_FILEPATHS) && (
#ifdef UNIX
            pcszURL[0] == TEXT('/') ||
#endif
            pcszURL[1] == TEXT(':') || pcszURL[1] == TEXT('|') || pcszURL[0] == TEXT('\\')))
        {
            hres = strOut.SetSize(MAX_PATH);

            if(SUCCEEDED(hres))
            {
                //  SHSTRs have a size granularity, so the size
                //  will be equal to or greater than what was set.
                //  this means we need to get it our self.
                DWORD cchOut = strOut.GetSize();
                TCHAR szCurrentDir[MAX_PATH];

                //
                //  BUGBUG - IE30 compatibility - zekel 8-Jan-97
                //  we need to GetCurrentDirectory() in order to
                //  put a default drive letter on the path
                //  if necessary.  
                //

                if(GetCurrentDirectory(ARRAYSIZE(szCurrentDir), szCurrentDir))
                    PathCombine((LPTSTR)strOut, szCurrentDir, pcszURL);
                else
                    hres = strOut.SetStr(pcszURL);

                if(SUCCEEDED(hres))
                {
                    hres = UrlCreateFromPath((LPTSTR) strOut, (LPTSTR) strOut, &cchOut, 0);
                    if (E_POINTER == hres && SUCCEEDED(hres = strOut.SetSize(cchOut)))
                    {
                        cchOut = strOut.GetSize();
                        hres = UrlCreateFromPath((LPTSTR) strOut, (LPTSTR) strOut, &cchOut, 0);
                    }
                }
            }
        }
        else if (SUCCEEDED(hres = strOut.SetSize(MAX_URL_STRING)))
        {
            //  all the Apply*() below rely on MAX_URL_STRING

            // No; begin processing general-case URLs.  Try to guess the
            // protocol or resort to the default protocol.

            DWORD cchOut = strOut.GetSize();
            if (IsFlagSet(dwFlags, UQF_GUESS_PROTOCOL))
                hres = UrlApplyScheme(pcszURL, (LPTSTR) strOut, &cchOut, URL_APPLY_GUESSSCHEME);

            //
            // Try to auto-correct the protocol
            //
            if (hres == S_FALSE &&
                IsFlagSet(dwFlags, UQF_AUTOCORRECT))
            {
                hres = UrlFixup(pcszURL, (LPTSTR)strOut, strOut.GetSize());
                bWasCorrected = (hres == S_OK);
            }

            if (hres == S_FALSE &&
                IsFlagSet(dwFlags, UQF_USE_DEFAULT_PROTOCOL)) 
            {
                hres = ApplyURLSearch(pcszURL, (LPTSTR) strOut);
                if (SUCCEEDED(hres) && pbWasSearchURL) {
                    *pbWasSearchURL = TRUE;
                }
                
                // If that fails, then tack on the default protocol
                if (FAILED(hres) || hres == S_FALSE)
                {
                    cchOut = strOut.GetSize();
                    hres = UrlApplyScheme(pcszURL, (LPTSTR) strOut, &cchOut, URL_APPLY_DEFAULT);
                }
            }

            // Did the above fail?
            if (S_FALSE == hres)
            {
                // Yes; return the real reason why the URL is bad
                hres = URL_E_INVALID_SYNTAX;
            }
            else if (dwFlags & UQF_CANONICALIZE)
            {
                // No; canonicalize
                cchSize = strOut.GetSize();
                hres = UrlCanonicalize((LPTSTR)strOut, (LPTSTR)strOut, &cchSize, 0);
            }
        }

        if (SUCCEEDED(hres))
        {
            StrCpyN(pszTranslatedURL, (LPTSTR) strOut, MAX_URL_STRING);
        }
    }

    if (pbWasCorrected)
        *pbWasCorrected = bWasCorrected;

    return hres;
}


/***************************** Exported Functions ****************************/


STDAPI
URLQualifyA(
    LPCSTR pszURL, 
    DWORD  dwFlags,         // UQF_*
    LPSTR *ppszOut)
{
    HRESULT hres;

    ASSERT(IS_VALID_STRING_PTRA(pszURL, -1));
    ASSERT(IS_VALID_WRITE_PTR(ppszOut, LPSTR));

    *ppszOut = NULL;

    WCHAR szTempTranslatedURL[MAX_URL_STRING];
    WCHAR szURL[MAX_URL_STRING];

    SHAnsiToUnicode(pszURL, szURL, ARRAYSIZE(szURL));

    hres = IURLQualify(szURL, dwFlags, szTempTranslatedURL, NULL, NULL);

    if (SUCCEEDED(hres))
    {
        CHAR szOut[MAX_URL_STRING];

        SHUnicodeToAnsi(szTempTranslatedURL, szOut, ARRAYSIZE(szOut));

        *ppszOut = StrDupA(szOut);

        if (!*ppszOut)
            hres = E_OUTOFMEMORY;
    }

    return hres;
}


STDAPI
URLQualifyW(
    LPCWSTR pszURL, 
    DWORD  dwFlags,         // UQF_*
    LPWSTR *ppszOut)
{
    HRESULT hres;

    ASSERT(IS_VALID_STRING_PTRW(pszURL, -1));
    ASSERT(IS_VALID_WRITE_PTR(ppszOut, LPWSTR));

    WCHAR szTempTranslatedURL[MAX_URL_STRING];

    hres = IURLQualify(pszURL, dwFlags, szTempTranslatedURL, NULL, NULL);

    if (SUCCEEDED(hres))
    {
        *ppszOut = StrDup(szTempTranslatedURL);

        if (!*ppszOut)
            hres = E_OUTOFMEMORY;
    }

    return hres;
}


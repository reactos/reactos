/*
 * urlexec.cpp - IUnknown implementation for Intshcut class.
 */

#include "shellprv.h"
#include <shstr.h>
#include <intshcut.h>


// URL Exec Hook

class CURLExec : public IShellExecuteHook
{
private:

    ULONG       m_cRef;

    ~CURLExec(void);    // Prevent this class from being allocated on the stack or it will fault.

public:
    CURLExec(void);

   // IShellExecuteHook methods

    STDMETHODIMP Execute(LPSHELLEXECUTEINFO pei);

    // IUnknown methods
    
    STDMETHODIMP  QueryInterface(REFIID riid, PVOID *ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
#ifdef DEBUG
    friend BOOL IsValidPCURLExec(const CURLExec * pue);
#endif
};


#ifdef DEBUG

BOOL IsValidPCURLExec(CURLExec * pue)
{
    return (IS_VALID_READ_PTR(pue, CURLExec));
}

#endif




/*----------------------------------------------------------
Purpose: This function qualifies a string as a URL.  Strings
         such as "www.foo.com" would have the scheme guessed
         if the correct flags are given.  Local paths are 
         converted to "file:" URLs.

         pszTranslatedURL may point to the same buffer as 
         pcszURL.

         NOTE:  This is identical to shdocvw's IURLQualify,
         except it doesn't have the search hook.

Returns: S_OK or S_FALSE means we filled in pszTranslatedURL.
         S_OK means we altered the URL to qualify it too.
         various failure codes too

Cond:    --
*/
STDAPI
MyIURLQualify(
    IN  LPCTSTR pcszURL, 
    IN  DWORD  dwFlags,         // UQF_*
    OUT LPTSTR  pszTranslatedURL)
{
    HRESULT hres = S_FALSE;

    SHSTR strOut;

    ASSERT(IS_VALID_STRING_PTR(pcszURL, -1));
    ASSERT(IS_VALID_STRING_PTR(pszTranslatedURL, -1));

    // Special cases: URLs of the form <drive>:<filename>
    //                URLs of the form \<filename>
    // we'll assume that if the second character is a : or |, this is an url of
    // that form, and we will guess "file://" for the prefix.
    // we'll assume any url that begins with a single \ is a file: url
 
    // NOTE: We do this here because these are cases where the protocol is 
    // left off, and is likely to be incorrectly guessed, such as a 
    // relative path \data\ftp\docs, would wrongly be turned 
    // into "ftp://\data\ftp\docs".
 

    if (PathIsURL(pcszURL))
    {
        //  BUGBUG multiple exit points for expediency
        //  of common case
        if (pszTranslatedURL != pcszURL)
            lstrcpy(pszTranslatedURL, pcszURL);

        return S_OK;
    }
    //  look for file paths
    else if (IsFlagClear(dwFlags, UQF_IGNORE_FILEPATHS) &&
        (pcszURL[1] == ':' || pcszURL[1] == '|' || pcszURL[0] == '\\'))
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
    else if (SUCCEEDED(hres = strOut.SetSize(INTERNET_MAX_URL_LENGTH)))
    {
        // No; begin processing general-case URLs.  Try to guess the
        // protocol or resort to the default protocol.

        DWORD cchOut = strOut.GetSize();
        if (IsFlagSet(dwFlags, UQF_GUESS_PROTOCOL))
            hres = UrlApplyScheme(pcszURL, (LPTSTR) strOut, &cchOut, URL_APPLY_GUESSSCHEME);

        if (hres == S_FALSE &&
            IsFlagSet(dwFlags, UQF_USE_DEFAULT_PROTOCOL)) 
        {
            cchOut = strOut.GetSize();
            hres = UrlApplyScheme(pcszURL, (LPTSTR) strOut, &cchOut, URL_APPLY_DEFAULT);
        }

        // Did the above fail?
        if (S_FALSE == hres)
        {
            // Yes; return the real reason why the URL is bad
            hres = URL_E_INVALID_SYNTAX;
        }
    }

    if (SUCCEEDED(hres))
    {
        lstrcpy(pszTranslatedURL, (LPTSTR)strOut);
    }

    return hres;
}


STDAPI
MyURLQualify(
    LPCTSTR pszURL, 
    DWORD  dwFlags,         // UQF_*
    LPTSTR *ppszOut)
{
    HRESULT hres;

    ASSERT(IS_VALID_STRING_PTR(pszURL, -1));
    ASSERT(IS_VALID_WRITE_PTR(ppszOut, LPTSTR));

    *ppszOut = NULL;

    TCHAR szTempTranslatedURL[INTERNET_MAX_URL_LENGTH];

    hres = MyIURLQualify(pszURL, dwFlags, szTempTranslatedURL);

    if (SUCCEEDED(hres))
    {
        *ppszOut = StrDup(szTempTranslatedURL);

        if (!*ppszOut)
            hres = E_OUTOFMEMORY;
    } 

    return hres;
}



//-----------------------------------------------------------------------



CURLExec::CURLExec(void) : m_cRef(1)
{
    // CURLExec objects should always be allocated

    ASSERT(IS_VALID_STRUCT_PTR(this, CURLExec));

    return;
}

CURLExec::~CURLExec(void)
{
    ASSERT(IS_VALID_STRUCT_PTR(this, CURLExec));
    
    return;
}


/*----------------------------------------------------------
Purpose: IUnknown::QueryInterface handler for CURLExec

*/
STDMETHODIMP CURLExec::QueryInterface(REFIID riid, PVOID *ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IShellExecuteHook))
    {
        *ppvObj = SAFECAST(this, IShellExecuteHook *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}


STDMETHODIMP_(ULONG) CURLExec::AddRef()
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CURLExec::Release()
{
    m_cRef--;
    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}


/*----------------------------------------------------------
Purpose: IShellExecuteHook::Execute handler for CURLExec

*/
STDMETHODIMP CURLExec::Execute(LPSHELLEXECUTEINFO pei)
{
    HRESULT hres;
    
    ASSERT(IS_VALID_STRUCT_PTR(this, CURLExec));
    ASSERT(IS_VALID_STRUCT_PTR(pei, SHELLEXECUTEINFO));
    
    if (! pei->lpVerb ||
        ! lstrcmpi(pei->lpVerb, TEXT("open")))
    {
        if (pei->lpFile && !UrlIs(pei->lpFile, URLIS_FILEURL))
        {
            LPTSTR pszURL;

            // This should succeed only for real URLs.  We should fail
            // for file paths and let the shell handle those.

            // WARNING: since this hook is called in any process that
            // calls ShellExecuteEx, we must be careful about calling
            // into shdocvw (it is delay-loaded), since it loads OLE.
            // Very piggy.
            //
            // If shdocvw isn't already loaded, defer to the local 
            // MyURLQualify.  Otherwise, call shdocvw's URLQualify.
            //
            // URLQualify offers more features (e.g., search) than the 
            // local MyURLQualify.  We'd like to take advantage of this
            // in the explorer process (say, for the Run dialog).

            
            if (GetModuleHandle(TEXT("shdocvw.dll")))
            {
                // Call the full-featured API
                hres = URLQualify(pei->lpFile, UQF_GUESS_PROTOCOL | UQF_IGNORE_FILEPATHS,
                                  &pszURL);
            }
            else
            {
                // Use the local function
                hres = MyURLQualify(pei->lpFile, UQF_GUESS_PROTOCOL | UQF_IGNORE_FILEPATHS,
                                  &pszURL);
            }
            
            if (SUCCEEDED(hres))
            {
                IUniformResourceLocator * purl;

                hres = SHCoCreateInstance(NULL, &CLSID_InternetShortcut, NULL, IID_IUniformResourceLocator, (void **)&purl);
                if (SUCCEEDED(hres))
                {
                    hres = purl->SetURL(pszURL, 0);
                    if (hres == S_OK)
                    {
                        IShellLink * psl;

                        hres = purl->QueryInterface(IID_IShellLink, (void **)&psl);
                        if (SUCCEEDED(hres))
                        {
                            URLINVOKECOMMANDINFO urlici;

                            EVAL(psl->SetShowCmd(pei->nShow) == S_OK);
                            
                            urlici.dwcbSize = SIZEOF(urlici);
                            urlici.hwndParent = pei->hwnd;
                            urlici.pcszVerb = NULL;
                            
                            urlici.dwFlags = IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB;
                            
                            if (IsFlagClear(pei->fMask, SEE_MASK_FLAG_NO_UI))
                                SetFlag(urlici.dwFlags, IURL_INVOKECOMMAND_FL_ALLOW_UI);

                            if (pei->fMask & SEE_MASK_FLAG_DDEWAIT)
                                SetFlag(urlici.dwFlags, IURL_INVOKECOMMAND_FL_DDEWAIT);
                                
                            hres = purl->InvokeCommand(&urlici);
                            
                            if (hres != S_OK)
                                SetFlag(pei->fMask, SEE_MASK_FLAG_NO_UI);

                            psl->Release();
                        }
                    }
                    purl->Release();
                }
                LocalFree(pszURL);
            }
        }
        else
            // BUGBUG (scotth): This hook only handles execution of file string, not IDList.
            hres = S_FALSE;
    }
    else
        // Unrecognized verb.
        hres = S_FALSE;
    
    if (hres == S_OK)
        pei->hInstApp = (HINSTANCE)42;  // BUGBUG (scotth): huh??
    else if (FAILED(hres))
    {
        switch (hres)
        {
        case URL_E_INVALID_SYNTAX:
        case URL_E_UNREGISTERED_PROTOCOL:
            hres = S_FALSE;
            break;
            
        case E_OUTOFMEMORY:
            pei->hInstApp = (HINSTANCE)SE_ERR_OOM;
            hres = E_FAIL;
            break;
            
        case IS_E_EXEC_FAILED:
            // Translate execution failure into "file not found".
            pei->hInstApp = (HINSTANCE)SE_ERR_FNF;
            hres = E_FAIL;
            break;
            
        default:
            // pei->lpFile is bogus.  Treat as file not found.
            pei->hInstApp = (HINSTANCE)SE_ERR_FNF;
            hres = E_FAIL;
            break;
        }
    }
    else
        ASSERT(hres == S_FALSE);
    
    ASSERT(hres == S_OK ||
        hres == S_FALSE ||
        hres == E_FAIL);
    
    return hres;
}


STDAPI CURLExec_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hres;
    
    CURLExec *pue = new CURLExec;
    if (pue)
    {
        hres = pue->QueryInterface(riid, ppvOut);
        pue->Release();
    }
    else
    {
        *ppvOut = NULL;
        hres = E_OUTOFMEMORY;
    }

    return hres;
}



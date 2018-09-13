#include "shellprv.h"
#pragma  hdrstop

#include "fstreex.h"
#include "idlcomm.h"

// returns SHAlloc() (COM Task Allocator) memory

LPTSTR SHGetCaption(HIDA hida)
{
    UINT idFormat;
    LPTSTR pszCaption = NULL;
    LPITEMIDLIST pidl;
    
    switch (HIDA_GetCount(hida))
    {
    case 0:
        return NULL;
        
    case 1:
        idFormat = IDS_ONEFILEPROP;
        break;
        
    default:
        idFormat = IDS_MANYFILEPROP;
        break;
    }
    
    pidl = HIDA_ILClone(hida, 0);
    if (pidl)
    {
        TCHAR szName[MAX_PATH];
        if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_NORMAL, szName, ARRAYSIZE(szName), NULL)))
        {
            TCHAR szTemplate[40];
            UINT uLen = LoadString(HINST_THISDLL, idFormat, szTemplate, ARRAYSIZE(szTemplate)) + lstrlen(szName) + 1;
            
            pszCaption = SHAlloc(uLen * SIZEOF(TCHAR));
            if (pszCaption)
            {
                wsprintf(pszCaption, szTemplate, (LPTSTR)szName);
            }
        }
        ILFree(pidl);
    }
    return pszCaption;
}

// This is not folder specific, and could be used for other background
// properties handlers, since all it does is bind to the parent of a full pidl
// and ask for properties
STDAPI SHPropertiesForPidl(HWND hwndOwner, LPCITEMIDLIST pidlFull, LPCTSTR pszParams)
{
    LPITEMIDLIST pidlLast;
    IShellFolder *psf;
    HRESULT hres;

    if (SHRestricted(REST_NOVIEWCONTEXTMENU)) {
        return HRESULT_FROM_WIN32(E_ACCESSDENIED);
    }
    
    hres = SHBindToIDListParent(pidlFull, &IID_IShellFolder, &psf, &pidlLast);
    if (SUCCEEDED(hres))
    {
        IContextMenu *pcm;
        hres = psf->lpVtbl->GetUIObjectOf(psf, hwndOwner, 1, &pidlLast, &IID_IContextMenu, 0, &pcm);
        if (SUCCEEDED(hres))
        {
#ifdef UNICODE
            CHAR szParameters[MAX_PATH];
#endif
            CMINVOKECOMMANDINFOEX ici = {
                SIZEOF(CMINVOKECOMMANDINFOEX),
                0L,
                hwndOwner,
#ifdef UNICODE
                "properties",
                szParameters,
#else
                c_szProperties,
                pszParams,
#endif
                NULL, SW_SHOWNORMAL
            };
#ifdef UNICODE
            if (pszParams)
                SHUnicodeToAnsi(pszParams, szParameters, ARRAYSIZE(szParameters));
            else
                ici.lpParameters = NULL;

            ici.fMask |= CMIC_MASK_UNICODE;
            ici.lpVerbW = c_szProperties;
            ici.lpParametersW = pszParams;
#endif
            // record if shift or control was being held down
            SetICIKeyModifiers(&ici.fMask);

            hres = pcm->lpVtbl->InvokeCommand(pcm, (LPCMINVOKECOMMANDINFO)&ici);
            pcm->lpVtbl->Release(pcm);
        }
        psf->lpVtbl->Release(psf);
    }

    return hres;
}

STDAPI Multi_GetAttributesOf(IShellFolder2 *psf, UINT cidl, LPCITEMIDLIST* apidl, ULONG *prgfInOut, PFNGAOCALLBACK pfnGAOCallback)
{
    HRESULT hres = NOERROR;
    UINT iidl;
    ULONG rgfOut = 0;

    for (iidl=0; iidl<cidl ; iidl++)
    {
        ULONG rgfT = *prgfInOut;
        hres = pfnGAOCallback(psf, apidl[iidl], &rgfT);
        if (FAILED(hres))
        {
            rgfOut = 0;
            break;
        }
        rgfOut |= rgfT;
    }

    *prgfInOut &= rgfOut;
    return hres;
}


// REVIEW: Why do we have this function to expose global keys?
// BUGBUG: HKEY_CURRENT_USER and HKEY_LOCAL_MACHINE are substantially different cases
//          one has to close the former, but never the latter

HKEY SHGetExplorerHkey(HKEY hkeyRoot, BOOL bCreate)
{
    ASSERT(hkeyRoot == HKEY_CURRENT_USER || hkeyRoot == HKEY_LOCAL_MACHINE);
    
    if (hkeyRoot == HKEY_CURRENT_USER) {
        HKEY hkeyExplorer = NULL;
        RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, &hkeyExplorer);
        return hkeyExplorer;
    }
    if (hkeyRoot == HKEY_LOCAL_MACHINE)
        return g_hklmExplorer;
    
    return NULL;
}

// REVIEW: Why do we have this function to wrap aroung RegCreate/Open key?
// we don't save any code size...

#define FULL_KEY_SIZE 400
HKEY SHGetExplorerSubHkey(HKEY hkeyRoot, LPCTSTR szSubKey, BOOL bCreate)
{
    HKEY hkSubKey=0;
    static TCHAR const szRegExplorer[] = REGSTR_PATH_EXPLORER  TEXT("\\");
    TCHAR szFullKey[FULL_KEY_SIZE];

    lstrcpy( szFullKey, szRegExplorer);
    lstrcpyn( szFullKey + ARRAYSIZE(szRegExplorer) - 1, szSubKey, FULL_KEY_SIZE - ARRAYSIZE(szRegExplorer));
    ASSERT(  lstrlen( szSubKey) + ARRAYSIZE(szRegExplorer) < FULL_KEY_SIZE );
    {
        LONG err;
        DWORD dwDisp;
        err = bCreate ? RegCreateKeyEx(hkeyRoot, szFullKey, 0, NULL, REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, NULL, &hkSubKey, &dwDisp) : 
                        RegOpenKeyEx  (hkeyRoot, szFullKey, 0, MAXIMUM_ALLOWED, &hkSubKey);
        if( err == ERROR_SUCCESS) {
            ASSERT( hkSubKey);
        } else {
            ASSERT( !hkSubKey);
        }
    }
    return hkSubKey;
}

BOOL _LoadErrMsg(UINT idErrMsg, LPTSTR pszErrMsg, DWORD err)
{
    TCHAR szTemplate[256];
    if (LoadString(HINST_THISDLL, idErrMsg, szTemplate, ARRAYSIZE(szTemplate)))
    {
        wsprintf(pszErrMsg, szTemplate, err);
        return TRUE;
    }
    return FALSE;
}

BOOL _VarArgsFormatMessage( LPTSTR lpBuffer, UINT cchBuffer, DWORD err, ... )
{
    BOOL fSuccess;

    va_list ArgList;

    va_start(ArgList, err);
    fSuccess = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, err, 0, lpBuffer, cchBuffer, &ArgList);
    va_end(ArgList);
    return fSuccess;
}

//
// Paremeters:
//  hwndOwner  -- owner window
//  idTemplate -- specifies template (e.g., "Can't open %2%s\n\n%1%s")
//  err        -- specifies the WIN32 error code
//  pszParam   -- specifies the 2nd parameter to idTemplate
//  dwFlags    -- flags for MessageBox
//

STDAPI_(UINT) SHSysErrorMessageBox(HWND hwndOwner, LPCTSTR pszTitle, UINT idTemplate,
                                   DWORD err, LPCTSTR pszParam, UINT dwFlags)
{
    BOOL fSuccess;
    UINT idRet = IDCANCEL;
    TCHAR szErrMsg[MAX_PATH * 2];

    //
    // FormatMessage is bogus, we don't know what to pass to it for %1,%2,%3,...
    // For most messages, lets pass the path as %1 and "" as everything else
    // For ERROR_MR_MID_NOT_FOUND (something nobody is ever supposed to see)
    // we will pass the path as %2 and everything else as "".
    //
    if (err == ERROR_MR_MID_NOT_FOUND)
    {
        fSuccess = _VarArgsFormatMessage(szErrMsg,ARRAYSIZE(szErrMsg),
                       err,c_szNULL,pszParam,c_szNULL,c_szNULL,c_szNULL);
    } 
    else 
    {
        fSuccess = _VarArgsFormatMessage(szErrMsg,ARRAYSIZE(szErrMsg),
                       err,pszParam,c_szNULL,c_szNULL,c_szNULL,c_szNULL);
    }

    if (fSuccess || _LoadErrMsg(IDS_ENUMERR_FSGENERIC, szErrMsg, err))
    {
        if (idTemplate==IDS_SHLEXEC_ERROR && (pszParam == NULL || StrStr(szErrMsg, pszParam)))
        {
            idTemplate = IDS_SHLEXEC_ERROR2;
        }

        idRet = ShellMessageBox(HINST_THISDLL, hwndOwner,
                MAKEINTRESOURCE(idTemplate),
                pszTitle, dwFlags, szErrMsg, pszParam);
    }

    return idRet;
}


STDAPI_(UINT) SHEnumErrorMessageBox(HWND hwnd, UINT idTemplate, DWORD err, LPCTSTR pszParam, BOOL fNet, UINT dwFlags)
{
    UINT idRet = IDCANCEL;
    TCHAR szErrMsg[256 + 32];

    if (hwnd == NULL)
        return idRet;

    switch(err)
    {
    case WN_SUCCESS:
    case WN_CANCEL:
        return IDCANCEL;        // Don't retry

    case ERROR_OUTOFMEMORY:
        return IDABORT;         // Out of memory!
    }

    if (fNet)
    {
        TCHAR szProvider[256];  // We don't use it.
        DWORD dwErrSize = ARRAYSIZE(szErrMsg);	     // BUGBUG (DavePl) I expect a cch here, but no docs, could be cb
        DWORD dwProvSize = ARRAYSIZE(szProvider);

        szErrMsg[0] = 0;
        MultinetGetErrorText(szErrMsg, &dwErrSize, szProvider, &dwProvSize);

        if (szErrMsg[0] == 0)
            _LoadErrMsg(IDS_ENUMERR_NETGENERIC, szErrMsg, err);

        idRet = ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(idTemplate),
                    NULL, dwFlags, szErrMsg, pszParam);
    }
    else
    {
        idRet = SHSysErrorMessageBox(hwnd, NULL, idTemplate, err, pszParam, dwFlags);
    }
    return idRet;
}

/*
 * urlassoc.c - URL Type association routines.
 */


#include "priv.h"
#include "ishcut.h"
#include <filetype.h>
#include <shlwapip.h>
#include "assocurl.h"
#include "resource.h"

#include <mluisupp.h>

#define c_szURLProtocol     TEXT("URL Protocol")
#define c_szEditFlags       TEXT("EditFlags")

#define c_szMIMETypeSubKeyFmt       TEXT("MIME\\Database\\Content Type\\%s")

#define c_szShellOpenCmdSubKeyFmt       TEXT("%s\\shell\\open\\command")
#define c_szAppOpenCmdFmt       TEXT("%s %%1")
#define c_szDefaultIconSubKeyFmt        TEXT("%s\\DefaultIcon")
#define c_szDefaultProtocolIcon     TEXT("shdocvw.dll,-105")


/***************************** Private Functions *****************************/

extern "C" {




/*
** RegisterAppAsURLProtocolHandler()
**
** Under HKEY_CLASSES_ROOT\url-protocol\shell\open\command, add default value =
** "c:\foo\bar.exe %1".
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
BOOL 
RegisterAppAsURLProtocolHandler(
    LPCTSTR pcszProtocol,
    LPCTSTR pcszApp)
{
    BOOL bResult = FALSE;
    DWORD cbShellOpen;
    LPTSTR pszShellOpen;

    ASSERT(IS_VALID_STRING_PTR(pcszProtocol, -1));
    ASSERT(IS_VALID_STRING_PTR(pcszApp, -1));

    /* (+ 1) for null terminator. */
    cbShellOpen = SIZEOF(c_szShellOpenCmdSubKeyFmt) + 
                         CbFromCch(1 + lstrlen(pcszProtocol));

    pszShellOpen = (LPTSTR)LocalAlloc(LPTR, cbShellOpen);

    if (pszShellOpen)
    {
        DWORD cbAppOpen;
        LPTSTR pszAppOpen;

        /* BUGBUG: We should quote pcszApp here only if it contains spaces. */

        /* (+ 1) for null terminator. */
        cbAppOpen = SIZEOF(c_szAppOpenCmdFmt) + 
                           CbFromCch(1 + lstrlen(pcszApp));

        pszAppOpen = (LPTSTR)LocalAlloc(LPTR, cbAppOpen);

        if (pszAppOpen)
        {
            wnsprintf(pszShellOpen, cbShellOpen / sizeof(TCHAR),
                      c_szShellOpenCmdSubKeyFmt, pcszProtocol);

            wnsprintf(pszAppOpen, cbAppOpen / sizeof(TCHAR), c_szAppOpenCmdFmt,
                      pcszApp);

            /* (+ 1) for null terminator. */
            bResult = (NO_ERROR == SHSetValue(HKEY_CLASSES_ROOT, pszShellOpen, NULL, 
                                              REG_SZ, pszAppOpen, 
                                              CbFromCch(lstrlen(pszAppOpen) + 1)));

            LocalFree(pszAppOpen);
        }

        LocalFree(pszShellOpen);
    }

    return(bResult);
}


/*
** RegisterURLProtocolDescription()
**
** Under HKEY_CLASSES_ROOT\url-protocol, add default value =
** URL:Url-protocol Protocol.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
BOOL 
RegisterURLProtocolDescription(
    LPCTSTR pcszProtocol)
{
    BOOL bResult = FALSE;
    LPTSTR pszProtocolCopy = NULL;

    ASSERT(IS_VALID_STRING_PTR(pcszProtocol, -1));

    if (Str_SetPtr(&pszProtocolCopy, pcszProtocol))
    {
        TCHAR szDescriptionFmt[MAX_PATH];

        /*
         * Convert first character of protocol to upper case for description
         * string.
         */

        *pszProtocolCopy = (TCHAR) (DWORD_PTR) CharUpper((LPTSTR)(DWORD_PTR)*pszProtocolCopy);

        if (MLLoadString(IDS_URL_DESC_FORMAT, szDescriptionFmt, SIZECHARS(szDescriptionFmt)))
        {
            TCHAR szDescription[MAX_PATH];

            if ((UINT)lstrlen(szDescriptionFmt) + (UINT)lstrlen(pszProtocolCopy)
                < SIZECHARS(szDescription))
            {
                wnsprintf(szDescription, ARRAYSIZE(szDescription), szDescriptionFmt,
                          pszProtocolCopy);                     

                /* (+ 1) for null terminator. */
                bResult = (NO_ERROR == SHSetValue(HKEY_CLASSES_ROOT, pcszProtocol, NULL, 
                                                  REG_SZ, szDescription,
                                                  CbFromCch(lstrlen(szDescription) + 1)));
            }
        }

        Str_SetPtr(&pszProtocolCopy, NULL);
    }

    return(bResult);
}


/*
** RegisterURLProtocolFlags()
**
** Under HKEY_CLASSES_ROOT\url-protocol, add EditFlags = FTA_Show.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
BOOL 
RegisterURLProtocolFlags(
    LPCTSTR pcszProtocol)
{
    DWORD dwEditFlags = FTA_Show;

    ASSERT(IS_VALID_STRING_PTR(pcszProtocol, -1));

    /* BUGBUG: What about preserving any existing EditFlags here? */

    return NO_ERROR == SHSetValue(HKEY_CLASSES_ROOT, pcszProtocol, c_szEditFlags, 
                                  REG_BINARY, &dwEditFlags, SIZEOF(dwEditFlags));
}


/*
** RegisterURLProtocol()
**
** Under HKEY_CLASSES_ROOT\url-protocol, add URL Protocol = "".
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
BOOL 
RegisterURLProtocol(
    LPCTSTR pcszProtocol)
{
    ASSERT(IS_VALID_STRING_PTR(pcszProtocol, -1));

    // REVIEW (scotth): what does this value mean??

    /* (+ 1) for null terminator. */
    return NO_ERROR == SHSetValue(HKEY_CLASSES_ROOT, pcszProtocol, c_szURLProtocol, 
                                  REG_SZ, c_szNULL, CbFromCch(1));
}


/*
** RegisterURLProtocolDefaultIcon()
**
** Under HKEY_CLASSES_ROOT\url-protocol\DefaultIcon, add default value =
** app.exe,0.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
BOOL 
RegisterURLProtocolDefaultIcon(
    LPCTSTR pcszProtocol)
{
    BOOL bResult = FALSE;
    DWORD cbAlloc;
    LPTSTR pszT;

    ASSERT(IS_VALID_STRING_PTR(pcszProtocol, -1));

    /* (+ 1) for null terminator. */
    cbAlloc = SIZEOF(c_szDefaultIconSubKeyFmt) + 
              CbFromCch(1 + lstrlen(pcszProtocol));

    pszT = (LPTSTR)LocalAlloc(LPTR, cbAlloc);

    if (pszT)
    {
        wnsprintf(pszT, cbAlloc / sizeof(TCHAR), c_szDefaultIconSubKeyFmt,
                  pcszProtocol);

        bResult = (NO_ERROR == SHSetValue(HKEY_CLASSES_ROOT, pszT, NULL, REG_SZ, 
                                          c_szDefaultProtocolIcon, 
                                          SIZEOF(c_szDefaultProtocolIcon)));

        LocalFree(pszT);
    }

    return(bResult);
}


BOOL 
AllowedToRegisterMIMEType(
    LPCTSTR pcszMIMEContentType)
{
    BOOL bResult;

    bResult = (0 != StrCmpI(pcszMIMEContentType, TEXT("application/octet-stream")) &&
               0 != StrCmpI(pcszMIMEContentType, TEXT("application/octet-string")));

    return(bResult);
}


BOOL 
RegisterMIMEAssociation(
    LPCTSTR pcszFile,
    LPCTSTR pcszMIMEContentType)
{
    BOOL bResult;
    LPCTSTR pcszExtension;

    ASSERT(IS_VALID_STRING_PTR(pcszFile, -1));
    ASSERT(IS_VALID_STRING_PTR(pcszMIMEContentType, -1));

    pcszExtension = PathFindExtension(pcszFile);

     /*
      * Don't allow association of flag unknown MIME types
      * application/octet-stream and application/octet-string.
      */

    if (EVAL(*pcszExtension) &&
        AllowedToRegisterMIMEType(pcszMIMEContentType))
    {
        bResult = (RegisterMIMETypeForExtension(pcszExtension, pcszMIMEContentType) &&
                   RegisterExtensionForMIMEType(pcszExtension, pcszMIMEContentType));
    }
    else
        bResult = FALSE;

    return(bResult);
}


BOOL 
RegisterURLAssociation(
    LPCTSTR pcszProtocol, 
    LPCTSTR pcszApp)
{
    ASSERT(IS_VALID_STRING_PTR(pcszProtocol, -1));
    ASSERT(IS_VALID_STRING_PTR(pcszApp, -1));

    return(RegisterAppAsURLProtocolHandler(pcszProtocol, pcszApp) &&
           RegisterURLProtocolDescription(pcszProtocol) &&
           RegisterURLProtocol(pcszProtocol) &&
           RegisterURLProtocolFlags(pcszProtocol) &&
           RegisterURLProtocolDefaultIcon(pcszProtocol));
}


HRESULT 
MyMIMEAssociationDialog(
    HWND hwndParent, 
    DWORD dwInFlags,
    LPCTSTR pcszFile,
    LPCTSTR pcszMIMEContentType,
    LPTSTR pszAppBuf, 
    UINT cchAppBuf)
{
    HRESULT hr;
    OPENASINFO oainfo;

    ASSERT(IS_VALID_HANDLE(hwndParent, WND));
    ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_MIMEASSOCDLG_FLAGS));
    ASSERT(IS_VALID_STRING_PTR(pcszFile, -1));
    ASSERT(IS_VALID_STRING_PTR(pcszMIMEContentType, -1));
    ASSERT(IS_VALID_WRITE_BUFFER(pszAppBuf, TCHAR, cchAppBuf));

    /* Use default file name if not supplied by caller. */

    if (cchAppBuf > 0)
        *pszAppBuf = '\0';

    oainfo.pcszFile = pcszFile;
    oainfo.pcszClass = pcszMIMEContentType;
    oainfo.dwInFlags = 0;

    if (IsFlagSet(dwInFlags, MIMEASSOCDLG_FL_REGISTER_ASSOC))
    {
        SetFlag(oainfo.dwInFlags, (OAIF_ALLOW_REGISTRATION |
                                    OAIF_REGISTER_EXT));
    }

#if 0   // BUGBUG (scotth): fix this
    hr = OpenAsDialog(hwndParent, &oainfo);
#else
    hr = E_FAIL;
#endif

    if (hr == S_OK &&
        IsFlagSet(dwInFlags, MIMEASSOCDLG_FL_REGISTER_ASSOC))
    {
        hr = RegisterMIMEAssociation(pcszFile, pcszMIMEContentType) ? S_OK
                                                                   : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
        StrCpyN(pszAppBuf, oainfo.szApp, cchAppBuf);

    ASSERT(! cchAppBuf ||
           (IS_VALID_STRING_PTR(pszAppBuf, -1) &&
            EVAL((UINT)lstrlen(pszAppBuf) < cchAppBuf)));
    ASSERT(SUCCEEDED(hr) ||
           (! cchAppBuf ||
            EVAL(! *pszAppBuf)));

    return(hr);
}


HRESULT 
MyURLAssociationDialog(
    HWND hwndParent, 
    DWORD dwInFlags,
    LPCTSTR pcszFile, 
    LPCTSTR pcszURL,
    LPTSTR pszAppBuf, 
    UINT cchAppBuf)
{
    HRESULT hr;
    LPTSTR pszProtocol;

    ASSERT(IS_VALID_HANDLE(hwndParent, WND));
    ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_URLASSOCDLG_FLAGS));
    ASSERT(IsFlagSet(dwInFlags, URLASSOCDLG_FL_USE_DEFAULT_NAME) ||
           IS_VALID_STRING_PTR(pcszFile, -1));
    ASSERT(IS_VALID_STRING_PTR(pcszURL, -1));
    ASSERT(IS_VALID_WRITE_BUFFER(pszAppBuf, TCHAR, cchAppBuf));

    /* Use URL protocol as class name. */

    if (cchAppBuf > 0)
        *pszAppBuf = '\0';

    hr = CopyURLProtocol(pcszURL, &pszProtocol, NULL);

    if (hr == S_OK)
    {
        TCHAR szInternetShortcut[MAX_PATH];
        OPENASINFO oainfo;

        /* Use default file name if not supplied by caller. */

        if (IsFlagSet(dwInFlags, URLASSOCDLG_FL_USE_DEFAULT_NAME) &&
            EVAL(MLLoadString(IDS_INTERNET_SHORTCUT,
                               szInternetShortcut,
                               SIZECHARS(szInternetShortcut))))
        {
            pcszFile = szInternetShortcut;
        }

        oainfo.pcszFile = pcszFile;
        oainfo.pcszClass = pszProtocol;
        oainfo.dwInFlags = 0;

        if (IsFlagSet(dwInFlags, URLASSOCDLG_FL_REGISTER_ASSOC))
            SetFlag(oainfo.dwInFlags, OAIF_ALLOW_REGISTRATION);

#if 0   // BUGBUG (scotth): fix this
        hr = OpenAsDialog(hwndParent, &oainfo);
#else
        hr = E_FAIL;
#endif

        if (hr == S_OK &&
            IsFlagSet(dwInFlags, URLASSOCDLG_FL_REGISTER_ASSOC))
        {
            hr = RegisterURLAssociation(pszProtocol, oainfo.szApp) ? S_OK
                                                                   : E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
            StrCpyN(pszAppBuf, oainfo.szApp, cchAppBuf);

        LocalFree(pszProtocol);
    }

    ASSERT(! cchAppBuf ||
           (IS_VALID_STRING_PTR(pszAppBuf, -1) &&
            EVAL((UINT)lstrlen(pszAppBuf) < cchAppBuf)));
    ASSERT(SUCCEEDED(hr) ||
           (! cchAppBuf ||
            EVAL(! *pszAppBuf)));

    return(hr);
}


#ifdef DEBUG

BOOL 
IsValidPCOPENASINFO(
    POPENASINFO poainfo)
{
    return(IS_VALID_READ_PTR(poainfo, OPENASINFO) &&
           IS_VALID_STRING_PTR(poainfo->pcszFile, -1) &&
           (! poainfo->pcszClass ||
            IS_VALID_STRING_PTR(poainfo->pcszClass, -1)) &&
           FLAGS_ARE_VALID(poainfo->dwInFlags, OAIF_ALL) &&
           (! *poainfo->szApp ||
            IS_VALID_STRING_PTR(poainfo->szApp, -1)));
}

#endif   /* DEBUG */


/***************************** Exported Functions ****************************/


/*----------------------------------------------------------
Purpose: Invoke the MIME-type association dialog.

Returns: standard hresult

Cond:    This API must conform to MIMEAssociationDialog semantics as
         defined in intshcut.h.  URL.DLL auto-forwards to this API in
         Nashville.

*/
STDAPI
AssociateMIME(
    HWND hwndParent,
    DWORD dwInFlags,
    LPCTSTR pcszFile,
    LPCTSTR pcszMIMEContentType,
    LPTSTR pszAppBuf,
    UINT cchAppBuf)
{
    HRESULT hr;

    /* Verify parameters. */

#ifdef EXPV
    if (IS_VALID_HANDLE(hwndParent, WND) &&
        IS_VALID_STRING_PTR(pcszFile, -1) &&
        IS_VALID_STRING_PTR(pcszMIMEContentType, -1) &&
        IS_VALID_WRITE_BUFFER(pszAppBuf, TCHAR, cchAppBuf))
    {
        if (FLAGS_ARE_VALID(dwInFlags, ALL_MIMEASSOCDLG_FLAGS))
        {
#endif
            hr = MyMIMEAssociationDialog(hwndParent, dwInFlags, pcszFile,
                                         pcszMIMEContentType, pszAppBuf,
                                         cchAppBuf);
#ifdef EXPV
        }
        else
            hr = E_FLAGS;
    }
    else
        hr = E_POINTER;
#endif

    return(hr);
}


STDAPI
AssociateMIMEA(
    HWND hwndParent,
    DWORD dwInFlags,
    LPCSTR pcszFile,
    LPCSTR pcszMIMEContentType,
    LPSTR pszAppBuf,
    UINT cchAppBuf)
{
    HRESULT hres;
    WCHAR wszFile[MAX_PATH];
    WCHAR wszMIMEType[MAX_PATH];
    LPWSTR pwszT;

    MultiByteToWideChar(CP_ACP, 0, pcszFile, -1, wszFile, SIZECHARS(wszFile));
    MultiByteToWideChar(CP_ACP, 0, pcszMIMEContentType, -1, wszMIMEType, 
                        SIZECHARS(wszMIMEType));

    *pszAppBuf = '\0';

    pwszT = (LPWSTR)LocalAlloc(LPTR, CbFromCch(cchAppBuf));
    if (pwszT)
    {
        hres = AssociateMIME(hwndParent, dwInFlags, wszFile, wszMIMEType,
                               pwszT, cchAppBuf);

        if (SUCCEEDED(hres))
        {
            WideCharToMultiByte(CP_ACP, 0, pwszT, -1, pszAppBuf, cchAppBuf, NULL, NULL);
        }

        LocalFree(pwszT);
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}



/*----------------------------------------------------------
Purpose: Invoke the URL association dialog.

Returns: standard hresult

Cond:    This API must conform to URLAssociationDialog semantics as
         defined in intshcut.h.  URL.DLL auto-forwards to this API in
         Nashville.

*/
STDAPI
AssociateURL(
    HWND hwndParent,
    DWORD dwInFlags,
    LPCTSTR pcszFile, 
    LPCTSTR pcszURL,
    LPTSTR pszAppBuf,
    UINT cchAppBuf)
{
    HRESULT hr;

    /* Verify parameters. */

#ifdef EXPV
    if (IS_VALID_HANDLE(hwndParent, WND) &&
        (IsFlagSet(dwInFlags, URLASSOCDLG_FL_USE_DEFAULT_NAME) ||
         IS_VALID_STRING_PTR(pcszFile, -1)) &&
        IS_VALID_STRING_PTR(pcszURL, -1) &&
        IS_VALID_WRITE_BUFFER(pszAppBuf, TCHAR, cchAppBuf))
    {
        if (FLAGS_ARE_VALID(dwInFlags, ALL_URLASSOCDLG_FLAGS))
        {
#endif
            hr = MyURLAssociationDialog(hwndParent, dwInFlags, pcszFile, pcszURL,
                                        pszAppBuf, cchAppBuf);
#ifdef EXPV
        }
        else
            hr = E_FLAGS;
    }
    else
        hr = E_POINTER;
#endif
 
    return(hr);
}


STDAPI
AssociateURLA(
    HWND hwndParent,
    DWORD dwInFlags,
    LPCSTR pcszFile,
    LPCSTR pcszURL,
    LPSTR pszAppBuf,
    UINT cchAppBuf)
{
    HRESULT hres;
    WCHAR wszFile[MAX_PATH];
    WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
    LPWSTR pwszT;

    MultiByteToWideChar(CP_ACP, 0, pcszFile, -1, wszFile, SIZECHARS(wszFile));
    MultiByteToWideChar(CP_ACP, 0, pcszURL, -1, wszURL, SIZECHARS(wszURL));

    *pszAppBuf = '\0';

    pwszT = (LPWSTR)LocalAlloc(LPTR, CbFromCch(cchAppBuf));
    if (pwszT)
    {
        hres = AssociateURL(hwndParent, dwInFlags, wszFile, wszURL,
                              pwszT, cchAppBuf);

        if (SUCCEEDED(hres))
        {
            WideCharToMultiByte(CP_ACP, 0, pwszT, -1, pszAppBuf, cchAppBuf, NULL, NULL);
        }

        LocalFree(pwszT);
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}


};  // extern "C"

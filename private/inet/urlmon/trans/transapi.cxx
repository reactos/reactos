//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       transapi.cxx
//
//  Contents:   API's for internal use
//
//  Classes:
//
//  Functions:
//
//  History:    4-26-96   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <trans.h>
#include "oinet.hxx"
#include <shlwapi.h>
#include <shlwapip.h>
#include <winineti.h>
#include "datasnif.hxx"
#include <winver.h> 

PerfDbgTag(tagTransApi, "Urlmon", "Log Trans API", DEB_DATA);

static char szMimeKey[]     = "MIME\\Database\\Content Type\\";
const ULONG ulMimeKeyLen    = ((sizeof(szMimeKey)/sizeof(char))-1);
LPCSTR pszDocObject         = "DocObject";
LPCSTR pszInprocServer      = "InprocServer32";
LPCSTR pszLocalServer       = "LocalServer32";

#define INTERNET_SETTING_KEY    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"

extern LPSTR        g_pszUserAgentString;
HMODULE             g_hLibPluginOcx = NULL;

HRESULT GetClassDocFileBuffer(LPVOID pbuffer, DWORD dwSize, CLSID *pclsid);

// borrow from urlmon\download to check if Doc Object handler is installed
// need CLocalComponentInfo & IsControlLocallyInstalled
#ifndef unix
#include "..\download\cdl.h"
#else
#include "../download/cdl.h"
#endif /* unix */

// DocFile properties and constants for extracting CodeBase property
#define DOCFILE_NUMPROPERTIES 3
#define DOCFILE_PROPID_CODEBASE 3
#define DOCFILE_PROPID_MAJORVERSION 4
#define DOCFILE_PROPID_MINORVERSION 5

//
// this new apis should be made public
//
STDAPI URLDownloadW(IUnknown *pUnk, LPCWSTR pwzURL, DWORD pBindInfo, IBindStatusCallback *pBSCB, DWORD dwReserved);
STDAPI URLDownloadA(IUnknown *pUnk, LPCSTR  pszURL, DWORD pBindInfo, IBindStatusCallback *pBSCB, DWORD dwReserved);

#define CF_INGNORE_SLASH 0x00000001     //ignore slash when comparing urls

typedef enum tagCLSCTXEX
{
/*
    // ole default class context values
    CLSCTX_INPROC_SERVER            = 0x0001,
    CLSCTX_INPROC_HANDLER           = 0x0002,
    CLSCTX_LOCAL_SERVER             = 0x0004,
    CLSCTX_INPROC_SERVER16          = 0x0008,
    CLSCTX_REMOTE_SERVER            = 0x0010,
    CLSCTX_INPROC_HANDLER16         = 0x0020,
    CLSCTX_INPROC_SERVERX86         = 0x0040,
    CLSCTX_INPROC_HANDLERX86        = 0x0080
*/
    // new class context values used in GetClassFileOrMime
    CLSCTX_INPROC_DOCOBJECT         = 0x0100,
    CLSCTX_LOCAL_DOCOBJECT          = 0x0200,
    CLSCTX_INPROC_CONTROL           = 0x0400,
    CLSCTX_INPROC_X_CONTROL         = 0x0800,
    CLSCTX_INPROC_PLUGIN            = 0x1000
}   CLSCTXEX;

#define CLSCTX_DOCOBJECT (CLSCTX_INPROC_DOCOBJECT|CLSCTX_LOCAL_DOCOBJECT)

const GUID CLSID_PluginHost =
{
    0x25336921, 0x03F9, 0x11cf, {0x8F, 0xD0, 0x00, 0xAA, 0x00, 0x68, 0x6F, 0x13}
};
const GUID CLSID_MsHtml =
{
    0x25336920, 0x03F9, 0x11cf, {0x8F, 0xD0, 0x00, 0xAA, 0x00, 0x68, 0x6F, 0x13}
};

const GUID FMTID_CodeBase =
{
    0xfe2d9191, 0x7fba, 0x11d0, {0xb3, 0xc2, 0x00, 0xa0, 0xc9, 0x0a, 0xea, 0x82}
};


//+---------------------------------------------------------------------------
//
//  Function:   GetClsIDInfo
//
//  Synopsis:
//
//  Arguments:  [pclsid] --     class id
//              [ClsCtxIn] --   unused
//              [pClsCtx] --    class context of class passed in
//
//  Returns:    S_OK on success
//              E_OUTOFMEMORY
//              E_FAIL
//
//  History:    7-20-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetClsIDInfo(CLSID *pclsid, DWORD ClsCtxIn, DWORD *pClsCtx)
{
    PerfDbgLog(tagTransApi, NULL, "+GetClsIDInfo");
    HRESULT hr = E_FAIL;

    TransAssert(( pClsCtx && pclsid && !IsEqualGUID(*pclsid, CLSID_NULL) ));

    LPSTR pszCls = StringAFromCLSID(pclsid);

    if (!pszCls)
    {
        hr = E_OUTOFMEMORY;
        *pClsCtx = 0;
    }
    else
    {
        CHAR  szCLSID[CLSIDSTR_MAX + 8];
        HKEY  hClsRegEntry;
        CHAR  szValue[64];
        long  lSize;
        DWORD dwClsCtx = 0;

        strcpy(szCLSID, "CLSID\\");
        strcat(szCLSID, pszCls);

        if (RegOpenKey(HKEY_CLASSES_ROOT, szCLSID, &hClsRegEntry) == ERROR_SUCCESS)
        {
            lSize = 64;
            HKEY hkeySrv32;
            if( RegOpenKey(hClsRegEntry, pszInprocServer, &hkeySrv32) 
                == ERROR_SUCCESS)
            {
                dwClsCtx |= CLSCTX_INPROC;
                RegCloseKey(hkeySrv32);
            }

            lSize = 64;
            if (RegQueryValue(hClsRegEntry, pszLocalServer, szValue, &lSize) == ERROR_SUCCESS)
            {
                dwClsCtx |= CLSCTX_LOCAL_SERVER;
            }
            lSize = 64;
            if (RegQueryValue(hClsRegEntry, pszDocObject, szValue, &lSize) == ERROR_SUCCESS)
            {
                dwClsCtx |= CLSCTX_INPROC_DOCOBJECT;
            }

            RegCloseKey(hClsRegEntry);
        }

        if (dwClsCtx)
        {
            hr = S_OK;
            *pClsCtx = dwClsCtx;
        }

        delete pszCls;
    }

    PerfDbgLog1(tagTransApi, NULL, "-GetClsIDInfo (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindFileExtension
//
//  Synopsis:
//
//  Arguments:  [pszFileName] --
//
//  Returns:
//
//  History:    2-09-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPSTR FindFileExtension(LPSTR pszFileName)
{
    PerfDbgLog1(tagTransApi, NULL, "+FindFileExtension (szFileName:%s)", pszFileName);
    LPSTR pStr = NULL;
    DWORD i;

    DWORD dwLen = strlen(pszFileName); // point to NULL
    LPSTR lpF = pszFileName + dwLen;

    if (lpF)
    {
        for (i = 0; *(lpF) != '.' && (i < dwLen); i++, lpF--)
            ;
    }

    if (i < dwLen)
    {
        pStr = lpF;
    }

    PerfDbgLog1(tagTransApi, NULL, "-FindFileExtension (pStr:%s)", pStr);
    return pStr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsDocFile
//
//  Synopsis:
//
//  Arguments:  [pBuffer] --
//
//  Returns:    S_OK            buffer is begin of docfile
//              S_FALSE         not begin of docfile
//              E_INVALIDARG    arguments incorrect
//
//  History:    2-09-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT IsDocFile(LPVOID pBuffer, DWORD cbSize)
{
    PerfDbgLog(tagTransApi, NULL, "+IsDocFile");
    HRESULT hr;

    // The byte combination that identifies that a file is a storage of
    // some kind
    BYTE SIGSTG[] = {0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1};
    BYTE CBSIGSTG = sizeof(SIGSTG);

    TransAssert(((pBuffer != NULL) &&  (cbSize != 0) ));

    if (!pBuffer  || (cbSize == 0))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = (!memcmp(pBuffer, SIGSTG, CBSIGSTG)) ? S_OK : S_FALSE;
    }

    PerfDbgLog1(tagTransApi, NULL, "-IsDocFile (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetMimeFromExt
//
//  Synopsis:
//
//  Arguments:  [pszExt] --
//              [pszMime] --
//              [cbMime] --
//
//  Returns:
//
//  History:    4-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetMimeFromExt(LPSTR pszExt, LPSTR pszMime, DWORD *pcbMime)
{
    PerfDbgLog1(tagTransApi, NULL, "+GetMimeFromExt (szExt:%s)", pszExt);
    HRESULT hr = E_FAIL;

    *pszMime = '\0';
    TransAssert((pszExt[0] == '.'));

    // the entry begins with '.' so it may be a file extension
    // query the value (which is the ProgID)

    HKEY hMimeKey = NULL;

    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, pszExt, 0, KEY_QUERY_VALUE, &hMimeKey) == ERROR_SUCCESS)
    {
        DWORD dwType = 1;
        if (RegQueryValueEx(hMimeKey, szContent, NULL, &dwType, (LPBYTE)pszMime, pcbMime) == ERROR_SUCCESS)
        {
            hr = NOERROR;
        }
        RegCloseKey(hMimeKey);
    }

    PerfDbgLog2(tagTransApi, NULL, "-GetMimeFromExt (pszMime:%s, hr:%lx)", pszMime, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetMimeFileExtension
//
//  Synopsis:
//
//  Arguments:  [pszMime] --
//              [pclsid] --
//
//  Returns:
//
//  History:    2-22-96   JohannP (Johann Posch)   Created
//
//  Notes:      BUGBUG: needs to be optimized!
//
//----------------------------------------------------------------------------
HRESULT GetMimeFileExtension(LPSTR pszMime, LPSTR pszExt, DWORD cbSize)
{
    PerfDbgLog1(tagTransApi, NULL, "+GetMimeFileExtension (MimeStr:%s)", pszMime);
    HRESULT hr = REGDB_E_CLASSNOTREG;

    HKEY hMimeKey = NULL;
    DWORD dwError;
    DWORD dwType;
    char szValue[256];
    DWORD dwValueLen = 256;
    char szKey[SZMIMESIZE_MAX + ulMimeKeyLen];

    if ((pszMime == 0) || (pszExt == 0))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pszExt = 0;

        strcpy(szKey, szMimeKey);
        strncat(szKey,pszMime, SZMIMESIZE_MAX);

        switch (RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_QUERY_VALUE, &hMimeKey))
        {
            case ERROR_SUCCESS:
                hr = NOERROR;
            break;
            // win32 will return file not found instead of bad key
            case ERROR_FILE_NOT_FOUND:
            case ERROR_BADKEY:
                hr = REGDB_E_CLASSNOTREG;
            break;
            default:
                hr = REGDB_E_READREGDB;
            break;
        }
        if (hr == NOERROR)
        {
            dwValueLen = 256;
            dwError = RegQueryValueEx(hMimeKey
                                        , szExtension
                                        , NULL
                                        , &dwType
                                        , (LPBYTE)szValue
                                        , &dwValueLen);

            if (  (dwError == ERROR_SUCCESS)
                && pszExt
                && dwValueLen
                && (dwValueLen <= cbSize) )
            {
                StrNCpy(pszExt, szValue, dwValueLen);
            }
        }
    }

    if (hMimeKey)
    {
        RegCloseKey(hMimeKey);
    }

    PerfDbgLog2(tagTransApi, NULL, "-GetMimeFileExtension (hr:%lx, szExt:%s)", hr, pszExt);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetClassMime
//
//  Synopsis:
//
//  Arguments:  [pszMime] --
//              [pclsid] --
//              [fIgnoreMimeClsid] --
//
//  Returns:
//
//  History:    2-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetClassMime(LPSTR pszMime, CLSID *pclsid, BOOL fIgnoreMimeClsid)
{
    PerfDbgLog1(tagTransApi, NULL, "+GetClassMime (MimeStr:%s)", pszMime);
    HRESULT hr = REGDB_E_CLASSNOTREG;
    DWORD dwFlags = (fIgnoreMimeClsid) ? MIMEFLAGS_IGNOREMIME_CLASSID: 0;
    HKEY hMimeKey = NULL;
    DWORD dwClsCtx;

    DWORD dwError;
    DWORD dwType;
    char szValue[256];
    DWORD dwValueLen = 256;
    char szKey[SZMIMESIZE_MAX + ulMimeKeyLen];

    if ((pszMime == 0) || (*pszMime == 0))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        strcpy(szKey, szMimeKey);
        strcat(szKey, pszMime);

        switch (RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_QUERY_VALUE, &hMimeKey))
        {
            case ERROR_SUCCESS:
                hr = NOERROR;
            break;
            // win32 will return file not found instead of bad key
            case ERROR_FILE_NOT_FOUND:
            case ERROR_BADKEY:
                hr = REGDB_E_CLASSNOTREG;
            break;
            default:
                hr = REGDB_E_READREGDB;
            break;
        }
        if (hr == NOERROR)
        {
            // if fIgnoreMimeClsid is set, ignore the CLSID entry
            // in the HKCR\MIME tree.
            if (!fIgnoreMimeClsid)
            {
                dwError = RegQueryValueEx(hMimeKey, szClassID, NULL
                                        , &dwType, (LPBYTE)szValue, &dwValueLen);

                hr = REGDB_E_CLASSNOTREG;


                if (dwError == ERROR_SUCCESS)
                {
                    WCHAR sz[256];
                    A2W(szValue,sz,256);
                    hr = CLSIDFromString(sz, pclsid);
                }

                if (hr == NOERROR)
                {
                    goto End;
                }
            }

            hr = REGDB_E_CLASSNOTREG;

            dwValueLen = 256;
            dwError = RegQueryValueEx(hMimeKey
                                        , szExtension
                                        , NULL
                                        , &dwType
                                        , (LPBYTE)szValue
                                        , &dwValueLen);

            if (dwError == ERROR_SUCCESS)
            {

                hr = GetClassFromExt(szValue,pclsid);
                //class still not known
                // try extension
            }
        }
    }

End:
    if (hMimeKey)
    {
        RegCloseKey(hMimeKey);
    }

    PerfDbgLog1(tagTransApi, NULL, "-GetClassMime (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetMimeFlags
//
//  Synopsis:
//
//  Arguments:  [pszMime] --
//              [pdwFlags] --
//
//  Returns:
//
//  History:    2-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetMimeFlags(LPCWSTR pwzMime, DWORD *pdwFlags)
{
    TransDebugOut((DEB_DATA, "API _IN GetMimeFlags (MimeStr:%ws)\n", pwzMime));
    HRESULT hr = E_FAIL;

    HKEY hMimeKey = NULL;
    DWORD dwError;
    DWORD dwType;
    DWORD dwFlags = 0;
    char szValue[256];
    DWORD dwValueLen = 256;
    char szKey[SZMIMESIZE_MAX + ulMimeKeyLen];

    if ((pwzMime == 0) || (*pwzMime == 0) || !pdwFlags)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        strcpy(szKey, szMimeKey);
        W2A(pwzMime, szKey + ulMimeKeyLen, SZMIMESIZE_MAX);

        *pdwFlags = 0;

        switch (RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_QUERY_VALUE, &hMimeKey))
        {
            case ERROR_SUCCESS:
                hr = NOERROR;
            break;
            // win32 will return file not found instead of bad key
            case ERROR_FILE_NOT_FOUND:
            case ERROR_BADKEY:
                hr = REGDB_E_CLASSNOTREG;
            break;
            default:
                hr = REGDB_E_READREGDB;
            break;
        }
        if (hr == NOERROR)
        {
            dwValueLen = sizeof(DWORD);
            dwError = RegQueryValueEx(hMimeKey, szFlags, NULL
                                    , &dwType, (LPBYTE)&dwFlags, &dwValueLen);

            hr = E_FAIL;

            if (dwError == ERROR_SUCCESS)
            {
                *pdwFlags = dwFlags;
                hr = NOERROR;
            }
        }
    }

    if (hMimeKey)
    {
        RegCloseKey(hMimeKey);
    }

    TransDebugOut((DEB_DATA, "API OUT GetMimeFlags (DWFLAGS:%lx, hr:%lx)\n", dwFlags, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetMimeInfo
//
//  Synopsis:
//
//  Arguments:  [pszMime] --
//              [pclsid] --
//              [pdwFlags] --
//              [pdwMimeFlags] --
//
//  Returns:
//
//  History:    2-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetMimeInfo(LPSTR pszMime, CLSID *pclsid, DWORD dwFlags, DWORD *pdwMimeFlags)

{
    PerfDbgLog1(tagTransApi, NULL, "+GetMimeInfo (MimeStr:%s)", pszMime);
    HRESULT hr = REGDB_E_CLASSNOTREG;

    BOOL fIgnoreMimeClsid = (dwFlags & MIMEFLAGS_IGNOREMIME_CLASSID);

    HKEY hMimeKey = NULL;
    DWORD dwError;
    DWORD dwType;
    char szValue[256];
    DWORD dwValueLen = 256;
    char szKey[SZMIMESIZE_MAX + ulMimeKeyLen];

    if ((pszMime == 0) || (*pszMime == 0))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        strcpy(szKey, szMimeKey);
        strcat(szKey, pszMime);

        switch (RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_QUERY_VALUE, &hMimeKey))
        {
            case ERROR_SUCCESS:
                hr = NOERROR;
            break;
            // win32 will return file not found instead of bad key
            case ERROR_FILE_NOT_FOUND:
            case ERROR_BADKEY:
                hr = REGDB_E_CLASSNOTREG;
            break;
            default:
                hr = REGDB_E_READREGDB;
            break;
        }
        if (hr == NOERROR)
        {
            // if fIgnoreMimeClsid is set, ignore the CLSID entry
            // in the HKCR\MIME tree.
            if (!fIgnoreMimeClsid)
            {
                dwError = RegQueryValueEx(hMimeKey, szClassID, NULL
                                        , &dwType, (LPBYTE)szValue, &dwValueLen);

                hr = REGDB_E_CLASSNOTREG;


                if (dwError == ERROR_SUCCESS)
                {
                    WCHAR sz[256];
                    A2W(szValue,sz,256);
                    hr = CLSIDFromString(sz, pclsid);
                }

                if (hr == NOERROR)
                {
                    goto End;
                }
            }

            hr = REGDB_E_CLASSNOTREG;

            dwValueLen = 256;
            dwError = RegQueryValueEx(hMimeKey
                                        , szExtension
                                        , NULL
                                        , &dwType
                                        , (LPBYTE)szValue
                                        , &dwValueLen);

            if (dwError == ERROR_SUCCESS)
            {

                hr = GetClassFromExt(szValue,pclsid);
            }

            if (pdwMimeFlags)
            {
                DWORD dwFlags = 0;
                *pdwMimeFlags = 0;
                dwValueLen = sizeof(DWORD);
                dwError = RegQueryValueEx(hMimeKey, szFlags, NULL
                                        , &dwType, (LPBYTE)&dwFlags, &dwValueLen);
                if (dwError == ERROR_SUCCESS)
                {
                    *pdwMimeFlags = dwFlags;
                }
            }
        }
    }

End:
    if (hMimeKey)
    {
        RegCloseKey(hMimeKey);
    }

    PerfDbgLog1(tagTransApi, NULL, "-GetMimeInfo (hr:%lx)", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Method:     GetClassFromExt
//
//  Synopsis:
//
//  Arguments:  [pszExt] --
//              [pclsid] --
//
//  Returns:
//
//  History:    2-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetClassFromExt(LPSTR pszExt, CLSID *pclsid)
{
    PerfDbgLog1(tagTransApi, NULL, "+GetClassFromExt (szExt:%s)", pszExt);
    HRESULT hr = REGDB_E_CLASSNOTREG;

    HKEY        hkRoot = HKEY_CLASSES_ROOT;
    char        szFileExt[MAX_PATH];
    DWORD       cbFileExt = sizeof(szFileExt);

    *pclsid = CLSID_NULL;
    char szProgID[MAX_PATH];
    LONG  cbProgID = sizeof(szProgID);

    if (pszExt[0] == '\0')
    {
        goto End;
    }

    strcpy(szFileExt,pszExt);

    TransAssert((szFileExt[0] == '.'));

    // the entry begins with '.' so it may be a file extension
    // query the value (which is the ProgID)

    if (RegQueryValue(hkRoot, szFileExt, szProgID, &cbProgID) == ERROR_SUCCESS)
    {
        // we got the value (ProgID), now query for the CLSID
        // string and convert it to a CLSID

        char szClsid[40];
        LONG  cbClsid = sizeof(szClsid);
        strcat(szProgID, "\\Clsid");

        if (RegQueryValue(HKEY_CLASSES_ROOT, szProgID, szClsid,&cbClsid) == ERROR_SUCCESS)
        {
            // make sure the clsid is valid
            cbProgID = sizeof(szProgID);
            char szClsidEntry[80];
            strcpy(szClsidEntry, "Clsid\\");
            strcat(szClsidEntry, szClsid);

            if (RegQueryValue(HKEY_CLASSES_ROOT, szClsidEntry,szProgID, &cbProgID) == ERROR_SUCCESS)
            {
                CLSID clsid;
                WCHAR sz[256];
                A2W(szClsid,sz,256);
                hr = CLSIDFromString(sz, pclsid);

                if (hr != NOERROR)
                {
                    *pclsid = CLSID_NULL;
                    hr = REGDB_E_CLASSNOTREG;
                }
            }
        }
    }
End:

    PerfDbgLog1(tagTransApi, NULL, "-GetClassFromExt (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsValidURL
//
//  Synopsis:
//
//  Arguments:  [pBC] --
//              [szURL] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    4-16-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI IsValidURL(LPBC pBC, LPCWSTR szURL, DWORD dwReserved)
{
    HRESULT hr;
    PerfDbgLog2(tagTransApi, NULL, "+IsValidURL(pBC:%lx, szURL:%ws)",pBC,szURL);
    WCHAR   wzUrlStr[MAX_URL_SIZE + 1];

    if (szURL == NULL)
    {
        hr = E_INVALIDARG;
    }
    else if (   (ConstructURL(pBC, NULL, NULL, (LPWSTR)szURL, wzUrlStr, sizeof(wzUrlStr), CU_CANONICALIZE) == NOERROR)
             && IsOInetProtocol(pBC, wzUrlStr))
    {
        hr = NOERROR;
    }
    else
    {
        hr = S_FALSE;
    }

    PerfDbgLog3(tagTransApi, NULL, "-IsValidURL(pBC:%lx, szURL:%ws, hr:%lx)",pBC,szURL,hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetUrlScheme
//
//  Synopsis:
//
//  Arguments:  pcwsz -- the URL
//              [szURL] --
//              [dwReserved] --
//
//  Returns:    URL_SCHEME_*
//
//----------------------------------------------------------------------------

DWORD
GetUrlScheme(LPCWSTR pcwzUrl)
{
    if(pcwzUrl)
    {
        PARSEDURLW puW;
        puW.cbSize = sizeof(puW);
        if(SUCCEEDED(ParseURLW(pcwzUrl, &puW)))
            return puW.nScheme;
    }
    return URL_SCHEME_INVALID;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetClassFileOrMime2
//
//  Synopsis:
//
//  Arguments:  [pBC] --
//              [wzFilename] --
//              [pBuffer] --
//              [cbSize] --
//              [pwzMime] --
//              [dwReserved] --
//              [pclsid] --
//              [fIgnoreMimeClsid] --
//
//  Returns:
//
//  History:    4-16-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI GetClassFileOrMime2(LPBC pBC, LPCWSTR pwzFilename, LPVOID pBuffer, DWORD cbSize,
                          LPCWSTR pwzMimeIn, DWORD dwReserved, CLSID *pclsid, BOOL fIgnoreMimeClsid)
{
    PerfDbgLog1(tagTransApi, NULL, "+GetClassFileOrMime(%lx)",pBC);
    HRESULT hr = REGDB_E_CLASSNOTREG;
    HRESULT hrPlugin = REGDB_E_CLASSNOTREG;
    HRESULT hrClass = REGDB_E_CLASSNOTREG;
    char szMime[SZMIMESIZE_MAX];
    char szFilename[MAX_PATH];
    LPSTR pszMime = NULL;
    LPSTR pszFilename = NULL;
    LPSTR pszExt = NULL;
    LPWSTR pwzMime = (LPWSTR)pwzMimeIn;
    DWORD cbMime = SZMIMESIZE_MAX;
    CLSID clsidPlugin = CLSID_NULL;
    BOOL  fDocFile = FALSE;
    DWORD dwClsCtx = 0;
    DWORD dwFlags = (fIgnoreMimeClsid) ? MIMEFLAGS_IGNOREMIME_CLASSID : 0;


    if (   pclsid == NULL
        || (!pwzFilename && (!pBuffer || !cbSize) && !pwzMimeIn))
    {
        hr = E_INVALIDARG;
        goto errRet;
    }
    *pclsid = CLSID_NULL;

    //sniff data here or when setting the mime
    if (pBuffer && cbSize)
    {
        fDocFile = (IsDocFile(pBuffer,cbSize) == S_OK);
        if (fDocFile)
        {
            // do not pass the buffer - no need to sniff data
            hr = FindMimeFromData(pBC, pwzFilename, NULL, 0, pwzMimeIn, 0, &pwzMime, 0);
        }
        else
        {
            hr = FindMimeFromData(pBC, pwzFilename, pBuffer, cbSize, pwzMimeIn, 0, &pwzMime, 0);
        }
    }

    if (pwzMime)
    {
        // convert the mime
        W2A(pwzMime, szMime, SZMIMESIZE_MAX);
        pszMime = szMime;
    }

    if (pwzFilename)
    {
        W2A(pwzFilename, szFilename, MAX_PATH);
        pszFilename = szFilename;
    }

    // 1. find the class based on the mime
    if (pszMime)
    {
        if (pBC)
        {
            hr = FindMediaTypeClass(pBC, szMime, pclsid, 0);
        }

        if (hr != NOERROR)
        {
            // get the class from the mime string
            hr = FindMediaTypeClassInfo(pszMime, pszFilename, pclsid, &dwClsCtx, dwFlags);
        }
    }

    // 2. find class of docfile
    if ((hr != NOERROR || IsEqualGUID(*pclsid, CLSID_NULL)) && cbSize && fDocFile)
    {
        // get class from docfile
        hr = GetClassDocFileBuffer(pBuffer, cbSize, pclsid);
    }

    // 3. use the file to find the class
    if (   (hr != NOERROR)
        && pszFilename)
    {
        pszExt = FindFileExtension(pszFilename);

        // use extension and use class mapping
        if (pszExt != NULL)
        {
            char szMimeExt[SZMIMESIZE_MAX];
            DWORD cbMimeExt = SZMIMESIZE_MAX;

            // get the mime for the file
            hr = GetMimeFromExt(pszExt,szMimeExt, &cbMimeExt);

            if (   (hr == NOERROR)
                && (   (pszMime && strcmp(pszMime, szMimeExt))
                    || !pszMime)
               )
            {
                hr = REGDB_E_CLASSNOTREG;
                if (pBC)
                {
                    // check for class mapping
                    hr = FindMediaTypeClass(pBC, szMimeExt, pclsid, 0);
                }

                if (hr != NOERROR)
                {
                    // get the class from the mime string
                    hr = FindMediaTypeClassInfo(szMimeExt, pszFilename, pclsid, &dwClsCtx, dwFlags);
                }
            }
        }

        // last call GetClassFile
        if ( hr != NOERROR && pwzFilename && (!pwzMime || fDocFile) )
        {
            hr = GetClassFile(pwzFilename, pclsid);
        }
    }

    // 4. if available check the class id and
    //    trigger check for plugin class id if needed
    if (   (hr == NOERROR)
        && !IsEqualGUID(*pclsid, CLSID_NULL))
    {

        hrClass = NOERROR;
        if (dwClsCtx == 0)
        {
            hr = GetClsIDInfo(pclsid, 0, &dwClsCtx);
        }

        if (hr == NOERROR)
        {
            if (dwClsCtx  & CLSCTX_DOCOBJECT)
            {
                // server of class is a docobject
                hrPlugin = NOERROR;
            }
            else if (dwClsCtx  & CLSCTX_INPROC)
            {
                // server of class is inproc

                // check if the class is mshtml tread it as docobject and stop
                // looking for plugin
                if (IsEqualGUID(*pclsid, CLSID_MsHtml))
                {
                    hrPlugin = NOERROR;
                }
            }
            else if (dwClsCtx  & CLSCTX_LOCAL_SERVER)
            {
                // server of class is local
            }
        }
        // else
        // class is not properly registered
        //
    }

    // 5. check if the download is for a plugin
    //    if yes get the plugin host class id
    if (hrPlugin != NOERROR)
    {
        if (pszExt == NULL && pszFilename)
        {
            pszExt = FindFileExtension(pszFilename);
        }

        // if we have a mime and/or an extension mime check if
        // this is a plugin or an ocx
        if (pszExt || pszMime)
        {
            hrPlugin = GetPlugInClsID(pszExt, NULL, pszMime, &clsidPlugin);
        }

    }
    else
    {
        hrPlugin = E_FAIL;
    }

    // 6. the plugin class use it
    if ( (hrPlugin == NOERROR) && !(dwReserved & GETCLASSFILEORMIME_IGNOREPLUGIN))
    {
        *pclsid = clsidPlugin;
        hr = hrPlugin;
    }
    // used the class found
    else
    {
        hr = hrClass;
    }

    if (pwzMime != pwzMimeIn)
    {
        delete [] pwzMime;
    }

    TransAssert((   (hr != NOERROR && IsEqualGUID(*pclsid, CLSID_NULL))
                 || (hr == NOERROR && !IsEqualGUID(*pclsid, CLSID_NULL)) ));

errRet:
    TransAssert((hr == NOERROR  || hr == REGDB_E_CLASSNOTREG  || hr == E_INVALIDARG));
    PerfDbgLog1(tagTransApi, NULL, "-GetClassFileOrMime (hr:%lx)",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetClassFileOrMime
//
//  Synopsis:
//
//  Arguments:  [pBC] --
//              [wzFilename] --
//              [pBuffer] --
//              [cbSize] --
//              [pwzMime] --
//              [dwReserved] --
//              [pclsid] --
//
//  Returns:
//
//  History:    4-16-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI GetClassFileOrMime(LPBC pBC, LPCWSTR pwzFilename, LPVOID pBuffer, DWORD cbSize,
                          LPCWSTR pwzMimeIn, DWORD dwReserved, CLSID *pclsid)
{
    return GetClassFileOrMime2(pBC, pwzFilename, pBuffer, cbSize, pwzMimeIn,
        dwReserved, pclsid, FALSE);
}



//+---------------------------------------------------------------------------
//
//  Function:   GetClassDocFileBuffer
//
//  Synopsis:
//
//  Arguments:  [pbuffer] --
//              [dwSize] --
//              [pclsid] --
//
//  Returns:
//
//  History:    2-28-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetClassDocFileBuffer(LPVOID pbuffer, DWORD dwSize, CLSID *pclsid)
{
    PerfDbgLog(tagTransApi, NULL, "+GetClassDocFileBuffer");
    HRESULT hr = E_FAIL;

    ILockBytes *pilb;
    IStorage *pstg;
    STATSTG stat;

    HGLOBAL hGlobal = 0;
    hGlobal = GlobalAlloc(GMEM_FIXED | GMEM_NODISCARD, dwSize);
    if (hGlobal)
    {
        memcpy(hGlobal, pbuffer, dwSize);

        hr = CreateILockBytesOnHGlobal(hGlobal,FALSE,&pilb);
        if (hr == NOERROR)
        {
            hr = StgOpenStorageOnILockBytes(pilb,NULL,STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                        NULL,0,&pstg);

            if (hr == NOERROR)
            {
                pstg->Stat(&stat, STATFLAG_NONAME);
                pstg->Release();
                *pclsid = stat.clsid;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        GlobalFree(hGlobal);
    }

    PerfDbgLog1(tagTransApi, NULL, "-GetClassDocFileBuffer (hr:%lx)", hr);
    return hr;
}

HRESULT GetCodeBaseFromDocFile(LPBYTE pBuffer, ULONG ulSize, LPWSTR *pwzClassStr, 
                               LPWSTR pwzBaseUrl, DWORD *lpdwVersionMS, DWORD *lpdwVersionLS);

//+---------------------------------------------------------------------------
//
//  Method:     IsHandlerAvailable
//
//  Synopsis:
//
//  Arguments:  [pClsid] --
//              [pMime] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT IsHandlerAvailable(LPWSTR pwzUrl, LPWSTR pwzMime, CLSID *pclsid, LPBYTE pBuffer, ULONG cbSize)
{
    PerfDbgLog(tagTransApi, NULL, "+GetCodeBaseFromDocFile");
    HRESULT hr = E_FAIL, hr1;
    LPWSTR szDistUnit = 0;
    BOOL fIgnoreMimeClsid = FALSE, fHasHandler = FALSE, fDocFile = FALSE;
    CLocalComponentInfo lci;
    CHAR szKey[SZMIMESIZE_MAX + ulMimeKeyLen];
    CHAR szMime[SZMIMESIZE_MAX];
    LPSTR pszUrl = 0;

    W2A(pwzMime, szMime, SZMIMESIZE_MAX);
    pszUrl = DupW2A(pwzUrl);        // can potentially be very long

    if ((pwzMime == 0) || (*pwzMime == 0))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        HKEY hMimeKey = 0;
     
        *pclsid = CLSID_NULL;

        strcpy(szKey, szMimeKey);
        strcat(szKey, szMime);

        // NOTE: different handlers may be implemented in different ways, for example.
        // the abscence of a CLSID does not imply the handler is bad or missing.

        // check if mime type exists in "Content Type" branch
        if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_QUERY_VALUE, &hMimeKey) == ERROR_SUCCESS)
        {
            fHasHandler = TRUE;
            RegCloseKey(hMimeKey);
            hMimeKey = 0;
        }
        
        // if not check if extension type has handler
        if (!fHasHandler)
        {
            LPSTR pszExt = FindFileExtension(pszUrl);

            // try for handler of extension
            if (pszExt && *pszExt == '.')
            {
                hMimeKey = 0;

                // there may be a handler for this extension already
                if (RegOpenKeyEx(HKEY_CLASSES_ROOT, pszExt, 0, KEY_QUERY_VALUE, &hMimeKey) == ERROR_SUCCESS)
                {
                    fHasHandler = TRUE;
                    RegCloseKey(hMimeKey);
                }
            }
        }

        // we haven't found a handler yet, in case of DocFile, check if CLSID associated
        // with it exists.  
        if (!fHasHandler && SUCCEEDED(IsDocFile(pBuffer,cbSize)))
        {
            DWORD dwVersionMS = 0, dwVersionLS = 0;
            LPWSTR pwzCodeBase = 0;

            hr1 = GetCodeBaseFromDocFile(pBuffer, cbSize, &pwzCodeBase, NULL, &dwVersionMS, &dwVersionLS);
            if (pwzCodeBase) {

                delete pwzCodeBase;
            }

            hr1 = GetClassDocFileBuffer(pBuffer, cbSize, pclsid);

            if (SUCCEEDED(hr1) && !IsEqualCLSID(*pclsid, CLSID_NULL))
            {
                StringFromCLSID(*pclsid, &szDistUnit);

                hr1 = IsControlLocallyInstalled(NULL, pclsid, szDistUnit, dwVersionMS, dwVersionLS, &lci, NULL);

                if (hr1 == S_OK)
                {
                    fHasHandler = TRUE;
                }

                if (szDistUnit)
                {
                    delete szDistUnit;
                }
            }
        }
    }

    if (pszUrl)
    {
        delete [] pszUrl;
    }

    if (fHasHandler)
    {
        hr = S_OK;
    }    
    else
    {
        hr = S_FALSE;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     GetCodeBaseFromDocFile
//
//  Synopsis:
//
//  Arguments:  [pBuffer] --
//              [ulSize] --
//              [pwzClassStr] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetCodeBaseFromDocFile(LPBYTE pBuffer, ULONG ulSize, LPWSTR *pwzClassStr, 
                               LPWSTR pwzBaseUrl, DWORD *lpdwVersionMS, DWORD *lpdwVersionLS)
{
    PerfDbgLog(tagTransApi, NULL, "+GetCodeBaseFromDocFile");
    HRESULT hr = E_FAIL;

    ILockBytes *pilb;
    IStorage *pstg;
    STATSTG stat;

    static ULONG cpSpec = DOCFILE_NUMPROPERTIES;
    static PROPSPEC rgpSpec[DOCFILE_NUMPROPERTIES] = { 
                                { PRSPEC_PROPID, DOCFILE_PROPID_CODEBASE },
                                { PRSPEC_PROPID, DOCFILE_PROPID_MAJORVERSION },
                                { PRSPEC_PROPID, DOCFILE_PROPID_MINORVERSION } };
            
    PROPVARIANT rgVarDisplay[DOCFILE_NUMPROPERTIES];
    
    if (!pwzClassStr || !lpdwVersionMS, !lpdwVersionLS)
    {
        hr = E_INVALIDARG;
    }
    else 
    {   
        HGLOBAL hGlobal = 0;
        hGlobal = GlobalAlloc(GMEM_FIXED | GMEM_NODISCARD, ulSize);

        *pwzClassStr = 0;

        if (hGlobal)
        {
            memcpy(hGlobal, pBuffer, ulSize);

            hr = CreateILockBytesOnHGlobal(hGlobal,FALSE,&pilb);
            if (hr == NOERROR)
            {
                hr = StgOpenStorageOnILockBytes(pilb,NULL,STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                            NULL,0,&pstg);
                if (hr == NOERROR)
                {
                    IPropertySetStorage *ppss = 0;
                    
                    hr = pstg->QueryInterface(IID_IPropertySetStorage, (void **)&ppss);
                    if (SUCCEEDED(hr)) 
                    {
                        IPropertyStorage *pps = 0;

                        //BUGBUG: there is potential for error here if data structure of
                        // PropertyStorage is not fully loaded.  since we have read only access
                        // we should be ok.

                        hr = ppss->Open(FMTID_CodeBase, STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE, (IPropertyStorage **)&pps);
                        if (SUCCEEDED(hr))
                        {
                            hr = pps->ReadMultiple(cpSpec, rgpSpec, rgVarDisplay);
                            if (SUCCEEDED(hr)) 
                            {
                                *pwzClassStr = rgVarDisplay[0].pwszVal;
                                *lpdwVersionMS = rgVarDisplay[1].ulVal;
                                *lpdwVersionLS = rgVarDisplay[2].ulVal;
                            }
                            pps->Release();
                        }
                        ppss->Release();
                    }
                    pstg->Release();
                }
                else
                {
                    hr = E_FAIL;
                }
            }
            GlobalFree(hGlobal);
        }
    }

    if (SUCCEEDED(hr) && pwzBaseUrl)
    {
        LPWSTR pwzNewClassStr = 0;
        DWORD dwLen, dwNewLen;
        HRESULT hr1;

        // we do not know what the combined maximum Url length is, so as
        // an upper bound we take twice the combined url's plus 10.
        dwLen = 2*(lstrlenW(pwzBaseUrl) + lstrlenW(*pwzClassStr)) + 10;

        pwzNewClassStr = new WCHAR[dwLen];

        if (pwzNewClassStr)
        {
            hr1 = CoInternetCombineUrl(pwzBaseUrl, *pwzClassStr, ICU_NO_ENCODE, pwzNewClassStr, dwLen, &dwNewLen, 0);
            
            if (SUCCEEDED(hr1) && dwNewLen)
            {
                delete [] *pwzClassStr;
                *pwzClassStr = pwzNewClassStr;
            }
            else
            {
                delete [] pwzNewClassStr;
            }
        }
    }

    PerfDbgLog1(tagTransApi, NULL, "-GetCodeBaseFromDocFile (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   UrlMkSetSessionOption
//
//  Synopsis:
//
//  Arguments:  [dwOption] --
//              [pBuffer] --
//              [dwBufferLength] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    5-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI UrlMkSetSessionOption(DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength, DWORD dwReserved)
{
    // BUGBUG - Function not threadsafe.
    
    PerfDbgLog(tagTransApi, NULL, "+UrlMkSetSessionOption");

    HRESULT hr;

    switch(dwOption)
    {
        // Change the User Agent string for this process.
        case URLMON_OPTION_USERAGENT:
        {
            // Validate buffer, allocate new user agent string,
            // delete old if necessary, copy and reference with
            // g_pszUserAgentString.

            if (!(pBuffer && dwBufferLength))
            {
                hr = E_INVALIDARG;
                break;
            }

            LPSTR pszTemp = new CHAR[dwBufferLength + 1];
            if (!pszTemp)
            {
                TransAssert(pszTemp && "Allocating memory for User-Agent header failed");
                hr = E_OUTOFMEMORY;
                break;
            }

            memcpy(pszTemp, pBuffer, dwBufferLength);
            pszTemp[dwBufferLength] = '\0';
            
            if (g_pszUserAgentString)
            {
                delete [] g_pszUserAgentString;
            }

            g_pszUserAgentString = pszTemp;
            hr = S_OK;
            break;
        }

        // Refresh user agent string from registry for this process.
        case URLMON_OPTION_USERAGENT_REFRESH:
        {
            // Refresh, delete old user agent string if necessary.
            // g_pszUserAgentString references refreshed string.

            LPSTR pszTemp = g_pszUserAgentString;            

            // NULL forces GetUserAgentString to refresh from registry.
            g_pszUserAgentString = NULL;
            g_pszUserAgentString = (LPSTR) GetUserAgentString();

            if (!g_pszUserAgentString)
            {
                g_pszUserAgentString = pszTemp;
                hr = S_FALSE;
                break;
            }
            
            // Need to set this on the session handle also.
            if (g_hSession)
                InternetSetOption(g_hSession, INTERNET_OPTION_USER_AGENT,
                    g_pszUserAgentString, strlen(g_pszUserAgentString));


            delete [] pszTemp;
            hr = S_OK;
            break;
        }
        
        // Set or reload proxy info from registry.
        case INTERNET_OPTION_PROXY:
        case INTERNET_OPTION_REFRESH:
        {
            // InternetSetOption does its own buffer validation.
            if (InternetSetOption(NULL, dwOption, pBuffer, dwBufferLength))
            {
                hr = S_OK;
                break;
            }

            hr = S_FALSE;
            break;
        }
                
        default:
        {
            hr = E_INVALIDARG;
            break;
        }
    }
                        
    PerfDbgLog1(tagTransApi, NULL, "-UrlMkSetSessionOption (hr:%lx)", hr);
    return hr;
}
            
            
//+---------------------------------------------------------------------------
//
//  Function:   UrlMkGetSessionOption
//
//  Synopsis:
//
//  Arguments:  [dwOption] --
//              [pBuffer] --
//              [dwBufferLength] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    5-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI UrlMkGetSessionOption(DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength, DWORD *pdwBufferLengthOut, DWORD dwReserved)
{
    PerfDbgLog(tagTransApi, NULL, "+UrlMkGetSessionOption");
    HRESULT hr = E_FAIL;

    if( !pdwBufferLengthOut )
    {
        return E_INVALIDARG;
    }

    if (dwOption == URLMON_OPTION_USERAGENT)
    {
        // get the default user agent string
        LPCSTR pszStr = GetUserAgentString();

        DWORD cLen = strlen(pszStr);
        *pdwBufferLengthOut = cLen + 1;

        hr = E_OUTOFMEMORY;

        if (cLen < dwBufferLength )
        {
            if( pBuffer )
            {
                strcpy((LPSTR)pBuffer, pszStr);
// AOL BUG 66102 - we always return E_FAIL
//                 hr = NOERROR;
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
    }
    else if (dwOption == URLMON_OPTION_URL_ENCODING)
    {
        if( !pBuffer || dwBufferLength < sizeof(DWORD) )
        {
            hr = E_INVALIDARG;
        }
        else
        {
            DWORD dwEncoding = URL_ENCODING_NONE;
            BOOL fDefault = FALSE;
            DWORD dwUrlEncodingDisableUTF8;
            DWORD dwSize = sizeof(DWORD);
            
            if( ERROR_SUCCESS == SHRegGetUSValue(
                    INTERNET_SETTING_KEY,
                    "UrlEncoding", 
                    NULL, 
                    (LPBYTE) &dwUrlEncodingDisableUTF8, 
                    &dwSize, 
                    FALSE, 
                    (LPVOID) &fDefault, 
                    sizeof(fDefault) )  )
            {
                if( dwUrlEncodingDisableUTF8)
                    dwEncoding = URL_ENCODING_DISABLE_UTF8;
                else
                    dwEncoding = URL_ENCODING_ENABLE_UTF8;
            }

            hr = NOERROR;
            *pdwBufferLengthOut = sizeof(DWORD);
            memcpy(pBuffer, (LPVOID)(&dwEncoding), sizeof(DWORD));
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    PerfDbgLog1(tagTransApi, NULL, "-UrlMkGetSessionOption (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetPlugInClsID
//
//  Synopsis:   load the plugin ocx; get the FindPluginByExtA address
//              call the FindPluginByExtA api
//
//  Arguments:  [pszExt] --
//              [szMime] --
//              [pclsid] --
//
//  Returns:    NOERROR and plugin class id if file is handled by plugin
//              REGDB_E_CLASSNOTREG otherwise
//
//  History:    7-16-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetPlugInClsID(LPSTR pszExt, LPSTR pszName, LPSTR pszMime, CLSID *pclsid)
{
    HRESULT hr = REGDB_E_CLASSNOTREG;
    PerfDbgLog3(tagTransApi, NULL, "+GetPlugInClsID (pszExt:%s, pszName:%s, pszMime:%s)", pszExt, pszName, pszMime);

    typedef BOOL (WINAPI * pfnFINDPLUGINBYEXT)(char *szExt, char *szName, char *szMime);

    static pfnFINDPLUGINBYEXT pfnFindPlugin = NULL;
    static BOOL fPluginLoaded = FALSE;
    static BOOL fGotProcAddr = FALSE;

    if (!fPluginLoaded)
    {
        g_hLibPluginOcx = LoadLibraryA("plugin.ocx");
        fPluginLoaded = TRUE;
    }

    if (g_hLibPluginOcx != NULL)
    {
        if (!fGotProcAddr)
        {
            pfnFindPlugin = (pfnFINDPLUGINBYEXT)GetProcAddress(g_hLibPluginOcx, "FindPluginByExtA");
            fGotProcAddr = TRUE;
            if (pfnFindPlugin == NULL)
            {
                DbgLog(tagTransApi, NULL, "Failed to find entry point FindPluginByExt in  plugin.ocx");
            }
        }

        if (pfnFindPlugin && pfnFindPlugin(pszExt, pszName, pszMime))
        {
            hr = S_OK;
        }
    }
    else
    {
        DbgLog(tagTransApi, NULL, "Failed to find plugin.ocx");
    }

    if (hr == S_OK)
    {
        *pclsid = CLSID_PluginHost;
    }

    PerfDbgLog1(tagTransApi, NULL, "-GetPlugInClsID (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   StringAFromCLSID
//
//  Synopsis:   returns an ansi string of given class id
//
//  Arguments:  [pclsid] -- the class id
//
//  Returns:    pointer to class id string or
//              NULL if out of memory
//
//  History:    7-20-96   JohannP (Johann Posch)   Created
//
//  Notes:      string pointer has to be deleted with delete (operator)
//
//----------------------------------------------------------------------------
LPSTR StringAFromCLSID(CLSID *pclsid)
{
    LPOLESTR pwzStr;
    LPSTR    pszStr = NULL;

    TransAssert((pclsid));

    StringFromCLSID(*pclsid, &pwzStr);

    if (pwzStr)
    {
        pszStr =  SzDupWzToSz(pwzStr, TRUE);
        delete pwzStr;
    }

    return pszStr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CLSIDFromStringA
//
//  Synopsis:
//
//  Arguments:  [pszClsid] --
//              [pclsid] --
//
//  Returns:
//
//  History:    11-20-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CLSIDFromStringA(LPSTR pszClsid, CLSID *pclsid)
{
    WCHAR sz[CLSIDSTR_MAX];
    A2W(pszClsid,sz,CLSIDSTR_MAX);
    return CLSIDFromString(sz, pclsid);
}


#include "ocidl.h"
#ifndef unix
#include "..\urlhlink\urlhlink.h"
#else
#include "../urlhlink/urlhlink.h"
#endif /* unix */
#define IS_E_PENDING(hr) (hr == E_PENDING)

//+---------------------------------------------------------------------------
//
//  Function:   URLDownloadA
//
//  Synopsis:
//
//  Arguments:  [pUnk] --
//              [szURL] --
//              [pBindInfo] --
//              [pBSCB] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    7-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI URLDownloadA(IUnknown *pUnk, LPCSTR pszURL, DWORD pBindInfo, IBindStatusCallback *pBSCB, DWORD dwReserved)
{
    PerfDbgLog1(tagTransApi, NULL, "+URLDownloadA (pszUrl:%s)", pszURL);
    HRESULT hr = NOERROR;
    LPCWSTR pwzUrl = DupA2W((LPSTR) pszURL);

    if (pwzUrl)
    {
        hr = URLDownloadW(pUnk,pwzUrl,pBindInfo,pBSCB, 0);
    }

    PerfDbgLog1(tagTransApi, NULL, "-URLDownloadA (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   URLDownloadW
//
//  Synopsis:
//
//  Arguments:  [pUnk] --
//              [pwzURL] --
//              [pBindInfo] --
//              [pBSCB] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    7-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI URLDownloadW(IUnknown *pUnk, LPCWSTR pwzURL, DWORD pBindInfo, IBindStatusCallback *pBSCB, DWORD dwReserved)
{
    PerfDbgLog1(tagTransApi, NULL, "+URLDownloadW (pwzUrl:%ws)", pwzURL);
    HRESULT             hr;
    IOleObject *        pOleObject = 0;
    IServiceProvider *  pServiceProvider = 0;
    IMoniker *          pmkr = 0;
    IBindCtx *          pBndCtx = 0;
    IBindHost *         pBindHost = 0;

    IStream * pstrm = 0;

    // Don't bother if we don't have a caller...

    if( pUnk )
    {
        // By convention the we give the caller first crack at service
        // provider. The assumption here is that if they implement it
        // they have the decency to forward QS's to their container.

        hr = pUnk->QueryInterface( IID_IServiceProvider,
                                        (void**)&pServiceProvider );

        if( FAILED(hr) )
        {
            // Ok, now try the 'slow way' : maybe the object is an 'OLE' object
            // that knows about it's client site:

            hr = pUnk->QueryInterface( IID_IOleObject, (void**)&pOleObject );

            if( SUCCEEDED(hr) )
            {
                IOleClientSite * pClientSite = 0;

                hr = pOleObject->GetClientSite(&pClientSite);

                if( SUCCEEDED(hr) )
                {
                    // Now see if we have a service provider at that site
                    hr = pClientSite->QueryInterface
                                            ( IID_IServiceProvider,
                                            (void**)&pServiceProvider );
                }

                if( pClientSite )
                    pClientSite->Release();
            }
            else
            {
                // Ok, it's not an OLE object, maybe it's one of these
                // new fangled 'ObjectWithSites':

                IObjectWithSite * pObjWithSite = 0;

                hr = pUnk->QueryInterface( IID_IObjectWithSite,
                                                    (void**)&pObjWithSite );

                if( SUCCEEDED(hr) )
                {
                    // Now see if we have a service provider at that site

                    hr = pObjWithSite->GetSite(IID_IServiceProvider,
                                                (void**)&pServiceProvider);
                }

                if( pObjWithSite )
                    pObjWithSite->Release();

            }
            if( pOleObject )
                pOleObject->Release();

        }

        // BUGBUG: In the code above we stop looking at one level up --
        //  this may be too harsh and we should loop on client sites
        // until we get to the top...

        if( !pServiceProvider )
            hr = E_UNEXPECTED;

        // Ok, we have a service provider, let's see if BindHost is
        // available. (Here there is some upward delegation going on
        // via service provider).

        if( SUCCEEDED(hr) )
            hr = pServiceProvider->QueryService( SID_SBindHost, IID_IBindHost,
                                                        (void**)&pBindHost );

        if( pServiceProvider )
            pServiceProvider->Release();

        pmkr = 0;
    }

    if (pBindHost)
    {
        // This allows the container to actually drive the download
        // by creating it's own moniker.

        hr = pBindHost->CreateMoniker( LPOLESTR(pwzURL),NULL, &pmkr,0 );



        if( SUCCEEDED(hr) )
        {
            // This allows containers to hook the download for
            // doing progress and aborting

            hr = pBindHost->MonikerBindToStorage(pmkr, NULL, pBSCB, IID_IStream,(void**)&pstrm);
        }

        pBindHost->Release();
    }
    else
    {
        // If you are here, then either the caller didn't pass
        // a 'caller' pointer or the caller is not in a BindHost
        // friendly environment.

        hr = CreateURLMoniker( 0, pwzURL, &pmkr );

        if( SUCCEEDED(hr) )
        {
            hr = CreateAsyncBindCtx( 0,pBSCB,0, &pBndCtx );
        }

        if (SUCCEEDED(hr))
        {
            hr = pmkr->BindToStorage( pBndCtx, NULL, IID_IStream, (void**)&pstrm );
        }

    }

    if( pstrm )
        pstrm->Release();

    if( pmkr )
        pmkr->Release();

    if( pBndCtx )
        pBndCtx->Release();

    PerfDbgLog1(tagTransApi, NULL, "-URLDownloadW (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindMimeFromData
//
//  Synopsis:
//
//  Arguments:  [pBC] --
//              [pwzURL] --
//              [pBuffer] --
//              [cbSize] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    3-28-97   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI FindMimeFromData(
                        LPBC pBC,                               // bind context - can be NULL
                        LPCWSTR pwzUrl,                     // url - can be null
                        LPVOID pBuffer,                     // buffer with data to sniff - can be null (pwzUrl must be valid)
                        DWORD cbSize,                   // size of buffer
                        LPCWSTR pwzMimeProposed,    // proposed mime if - can be null
                        DWORD dwMimeFlags,                  // will be determined
                        LPWSTR *ppwzMimeOut,        // the suggested mime
                        DWORD dwReserved)                   // must be 0
{
    LPCWSTR wzMimeFromData = 0;
    HRESULT hr = E_FAIL;

    PerfDbgLog1(tagTransApi, NULL, "+FindMimeFromData (sugg: %ws)", pwzMimeProposed ? pwzMimeProposed : L"NULL");
    if (   !ppwzMimeOut
        || (!pwzUrl && !pBuffer))
    {
        hr = E_INVALIDARG;
    }
    else if( pBuffer || pwzMimeProposed )
    {
        CContentAnalyzer ca;
        wzMimeFromData = ca.FindMimeFromData(pwzUrl,(char*) pBuffer, cbSize, pwzMimeProposed, dwMimeFlags);

        if (wzMimeFromData)
        {
            *ppwzMimeOut = OLESTRDuplicate(wzMimeFromData);
            if (*ppwzMimeOut)
            {
                hr = NOERROR;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

        }
    }
    else
    {
        // file extension is the only solution
        LPSTR pszExt = NULL;
        CHAR  szUrl[MAX_PATH]; 
        CHAR  szMime[MAX_PATH];
        DWORD cbMime = MAX_PATH;
        
        W2A(pwzUrl, szUrl, MAX_PATH);
        pszExt = FindFileExtension(szUrl);

        if( pszExt )
        {
            hr = GetMimeFromExt(pszExt, szMime, &cbMime);
        }
        else
        {
            hr = E_FAIL;
        }

        if( SUCCEEDED(hr) )
        {
            *ppwzMimeOut = DupA2W(szMime);    
            if (*ppwzMimeOut)
            {
                hr = NOERROR;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    PerfDbgLog1(tagTransApi, NULL, "-FindMimeFromData (actual: %ws)", wzMimeFromData ? wzMimeFromData : L"NULL" );
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoInternetParseUrl
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [ParseAction] --
//              [dwFlags] --
//              [pszResult] --
//              [cchResult] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    4-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoInternetParseUrl(
    LPCWSTR     pwzUrl,
    PARSEACTION ParseAction,
    DWORD       dwFlags,
    LPWSTR      pszResult,
    DWORD       cchResult,
    DWORD      *pcchResult,
    DWORD       dwReserved
    )
{
    PerfDbgLog(tagTransApi, NULL, "+CoInternetParseUrl");
    COInetSession *pOInetSession = 0;
    HRESULT hr = INET_E_DEFAULT_ACTION;

    if (!IsKnownProtocol(pwzUrl))
    {
        hr = GetCOInetSession(0, &pOInetSession,0);

        if (hr == NOERROR)
        {
            hr = pOInetSession->ParseUrl(pwzUrl, ParseAction, dwFlags,  pszResult, cchResult, pcchResult, dwReserved);
            pOInetSession->Release();
        }
    }
    
    if (hr == INET_E_DEFAULT_ACTION)
    {
        hr = E_FAIL;

        switch (ParseAction)
        {
        case PARSE_CANONICALIZE      :
            *pcchResult = cchResult;
            hr = UrlCanonicalizeW(pwzUrl, pszResult, pcchResult, dwFlags);
        break;
        case PARSE_SCHEMA            :
            *pcchResult = cchResult;
            hr = UrlGetPartW(pwzUrl, pszResult, pcchResult, URL_PART_SCHEME, 0);   
            break;

        break;
        case PARSE_SITE              :
        break;
        case PARSE_DOMAIN            :
            *pcchResult = cchResult;
            hr = UrlGetPartW(pwzUrl, pszResult, pcchResult, URL_PART_HOSTNAME, 0);   
            TransAssert(hr != E_POINTER);
            break;

        break;
        case PARSE_FRIENDLY          :
        break;
        case PARSE_SECURITY_URL   :
        // should return "schema:host"  for all protocols
        break;
        case PARSE_ROOTDOCUMENT      :
        {
            PARSEDURLW puW;
            puW.cbSize = sizeof(PARSEDURLW);
            if (SUCCEEDED(ParseURLW(pwzUrl, &puW)) && IsHierarchicalScheme(puW.nScheme))
            {
                DWORD cchRequired = 1;
                // The first URLGetPartW call is just to get the number of chars required for the hostname.
                // This is not as efficient but keeps the code simpler.
                if ((UrlGetPartW(pwzUrl, pszResult, &cchRequired, URL_PART_HOSTNAME, 0)) == E_POINTER)
                {
                    cchRequired += (puW.cchProtocol + 3);
                    if (cchResult >= cchRequired)
                    {
                        LPWSTR pszCopyTo = pszResult;

                        *pcchResult = cchRequired - 1;    // don't include terminating NULL char.
                        memcpy(pszCopyTo, puW.pszProtocol, puW.cchProtocol * sizeof(WCHAR));
                        pszCopyTo += puW.cchProtocol;
                        memcpy(pszCopyTo, L"://", 3 * sizeof(WCHAR));
                        pszCopyTo += 3;

                        DWORD cchHost = (DWORD) (cchResult - (pszCopyTo - pszResult));
                        hr = UrlGetPartW(pwzUrl, pszCopyTo, &cchHost, URL_PART_HOSTNAME, 0);
                    }
                    else 
                    {
                        *pcchResult = cchRequired;
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
        break;

        case PARSE_DOCUMENT          :
        break;
        case PARSE_ANCHOR            :
        break;
        case PARSE_ENCODE            :
        case PARSE_UNESCAPE          :
            *pcchResult = cchResult;
            hr = UrlUnescapeW((LPWSTR)pwzUrl, pszResult, pcchResult, dwFlags);
        break;
        case PARSE_ESCAPE            :
        case PARSE_DECODE            :
            *pcchResult = cchResult;
            hr = UrlEscapeW(pwzUrl, pszResult, pcchResult, dwFlags);
        break;
        case PARSE_PATH_FROM_URL     :
            *pcchResult = cchResult;
            hr = PathCreateFromUrlW(pwzUrl, pszResult, pcchResult, dwFlags);
        break;
        case PARSE_URL_FROM_PATH     :
            *pcchResult = cchResult;
            hr = UrlCreateFromPathW(pwzUrl, pszResult, pcchResult, dwFlags);
        break;
        case PARSE_LOCATION          :
        {
            hr = E_FAIL;
            *pcchResult = 0;
            LPCWSTR pwzStr = UrlGetLocationW(pwzUrl); //, pszResult, pcchResult, dwFlags);
            if (pwzStr)
            {
                DWORD dwlen = wcslen(pwzStr);
                if (dwlen < cchResult)
                {
                    wcscpy(pszResult, pwzStr);
                    *pcchResult = dwlen;
                    hr = NOERROR;
                }
                else
                {
                    // buffer too small
                }
            }
        }
        break;
        case PARSE_MIME              :
        default:
            hr = E_FAIL;
        }
    }

    PerfDbgLog1(tagTransApi, NULL, "-CoInternetParseUrl (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     IsHierarchicalScheme
//
//  Synopsis:
//
//  Arguments:  [dwScheme] --
//
//  Returns:
//
//  History:    6-16-1997   Sanjays (Sanjay Shenoy)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL IsHierarchicalScheme(DWORD dwScheme)
{
    BOOL bReturn;

    switch ( dwScheme )
    {
        case URL_SCHEME_HTTP:
        case URL_SCHEME_FTP:
        case URL_SCHEME_HTTPS:
        case URL_SCHEME_NEWS:
        case URL_SCHEME_GOPHER:
        case URL_SCHEME_NNTP:
        case URL_SCHEME_TELNET:
        case URL_SCHEME_SNEWS:
            bReturn = TRUE;
            break;
        default:
            bReturn = FALSE;
            break;
    }

    return bReturn;
}  

//+---------------------------------------------------------------------------
//
//  Method:     IsHierarchicalUrl
//
//  Synopsis:
//
//  Arguments:  [pwszUrl] --
//
//  Returns:
//
//  History:    6-16-1997   Sanjays (Sanjay Shenoy)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL IsHierarchicalUrl(LPCWSTR pwszUrl)
{
    DWORD dwScheme = URL_SCHEME_INVALID;

    if(pwszUrl)
    {
        PARSEDURLW pu;
        pu.cbSize = sizeof(pu);
        if(SUCCEEDED(ParseURLW(pwszUrl, &pu)))
            dwScheme = pu.nScheme;
    }
  
    return IsHierarchicalScheme(dwScheme);
}

//+---------------------------------------------------------------------------
//
//  Method:     CoInternetGetSecurityUrl
//
//  Synopsis:
//
//  Arguments:  [pwszUrl] --
//              [ppszSecUrl] --
//              [psuAction]
//              [dwReserved]
//  Returns:
//
//  History:    4-28-1997   Sanjays (Sanjay Shenoy)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT CoInternetGetSecurityUrl(
    LPCWSTR pwszUrl,
    LPWSTR *ppwszSecUrl,    // out argument.
    PSUACTION   psuAction,
    DWORD dwReserved
    )
{
    PerfDbgLog(tagTransApi, NULL, "+CoInternetGetSecurityUrl");
    COInetSession *pOInetSession = NULL;

    if (pwszUrl == NULL || ppwszSecUrl == NULL)
    {
        return E_INVALIDARG;
    }
    
    if (psuAction != PSU_DEFAULT && psuAction != PSU_SECURITY_URL_ONLY)
    {
        return E_NOTIMPL;
    }

    LPWSTR pwszSecUrl = (LPWSTR) pwszUrl;
    *ppwszSecUrl = NULL;
    BOOL bAllocSecUrl = FALSE; // Should we free pwszSecUrl?

    HRESULT hr = S_OK;

    // Step 1    
    // If this is a custom protocol, give it a chance to return back the 
    // security URL first. 

    hr = GetCOInetSession(0, &pOInetSession, 0);

    if (hr == NOERROR)
    {
        DWORD dwId;
        // It is important to loop here. The URL returned by a pluggable protocol by calling
        // PARSE_SECURITY_URL might be another pluggable protocol. 
        while (((dwId = IsKnownProtocol(pwszSecUrl)) == DLD_PROTOCOL_NONE)
                || (dwId == DLD_PROTOCOL_STREAM)) // Special case mk: hack since there could be a 
                                                  // namespace handler defined for it. 
        {
        
            // Allocate as much memory as the url. This should be a good upper limit in most all cases.
            DWORD cchIn = lstrlenW(pwszSecUrl) + 1;
            DWORD cchOut = 0;

            LPWSTR pwszTmp = new WCHAR[cchIn];

            if (pwszTmp != NULL)
                hr = pOInetSession->ParseUrl(pwszSecUrl, PARSE_SECURITY_URL, 0, pwszTmp, cchIn, &cchOut, 0);
            else 
                hr = E_OUTOFMEMORY;


            // Not enough memory.
            if (hr == S_FALSE)
            {
                // Plug prot claims it needs more memory but asks us for a buffer of a 
                // smaller size.
                TransAssert(cchIn < cchOut);
                if (cchIn >= cchOut)
                {
                    hr = E_UNEXPECTED;
                }
                else
                {
                    cchIn = cchOut;
                    delete [] pwszTmp;
                    pwszTmp = new WCHAR[cchIn];

                    if ( pwszTmp != NULL ) 
                    {                  
                        hr = pOInetSession->ParseUrl(pwszSecUrl, PARSE_SECURITY_URL, 0, pwszTmp, cchIn, &cchOut, 0);
                        TransAssert(hr != S_FALSE);
                    }                
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // If for some reason the pluggable protocol just returned back
                // the original string, don't go into an infinite loop.
                if (0 == StrCmpW(pwszSecUrl, pwszTmp))
                {
                    delete [] pwszTmp;
                    break;
                }

                if (bAllocSecUrl)
                    delete [] pwszSecUrl;

                pwszSecUrl = pwszTmp;
                bAllocSecUrl = TRUE;
            }
            else 
            {
                if (hr == INET_E_DEFAULT_ACTION || hr == E_NOTIMPL)
                {
                    // This implies the pluggable protocol just wants us to use the 
                    // base url as the security url. 
                    hr = S_OK;
                }

                delete [] pwszTmp;
                break;
            }
        }
    }
    else 
    {
        // Some protocols don't support the IInternetProtocolInfo interface.
        // We will do the best we can. 
        hr = S_OK;
    }

    // End of Step 1.                                                          
    // At this point we have the security URL. We are done if the PSUACTION 
    // indicated we should only be getting the security URL.
    
    if (psuAction == PSU_SECURITY_URL_ONLY)
    {
        if (SUCCEEDED(hr))
        {
            // If we didn't allocate memory for pwszSecUrl i.e. it is the same as the 
            // input string, we have to do that before returning it back. 
            if (!bAllocSecUrl)    
            {
                *ppwszSecUrl = new WCHAR [(lstrlenW(pwszSecUrl) + 1)];
                if (*ppwszSecUrl != NULL)
                    StrCpyW(*ppwszSecUrl, pwszSecUrl);
                else
                    hr = E_OUTOFMEMORY;
            }
            else
            {
                *ppwszSecUrl = pwszSecUrl;
            }
        }
    }
    else
    {
        TransAssert(psuAction == PSU_DEFAULT);                                 
        // Step 2.
        // If URL after Step 1 is still not well known ask the protocol handler to simplify
        // it it is well known call UrlGetPart in shlwapi.

        LPWSTR pwszRet = NULL;

        if (SUCCEEDED(hr))
        {
            if (pwszSecUrl == NULL)
            {
                TransAssert(FALSE); // This has to be due to a bug in Step 1.
                pwszSecUrl = (LPWSTR) pwszUrl;   // recover as best as we can.
            }


            // Since Step 2 is just supposed to strip off parts we can safely assume that 
            // the out string will be smaller than the input string. 

            DWORD cchIn = lstrlenW(pwszSecUrl) + 1;
            DWORD cchOut = 0;
            pwszRet = new WCHAR[cchIn];

            if (pwszRet == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else if (!IsKnownProtocol(pwszSecUrl))
            {
                TransAssert(pOInetSession);

                if (pOInetSession)
                    hr = pOInetSession->ParseUrl(pwszSecUrl, PARSE_SECURITY_DOMAIN, 0, pwszRet, cchIn, &cchOut, 0);
                else
                    hr = INET_E_DEFAULT_ACTION ; // no protocol info ==> use default

                TransAssert(hr != S_FALSE);   // Should never require more memory
                if (hr == INET_E_DEFAULT_ACTION || hr == E_NOTIMPL)
                {
                    StrCpyW(pwszRet, pwszSecUrl);
                    hr = S_OK;
                }
            }
            else  // Known protocol call shlwapi.
            {
                if (IsHierarchicalUrl(pwszSecUrl))
                {
                    hr = UrlGetPartW(pwszSecUrl, pwszRet, &cchIn, URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME);   
                    TransAssert(hr != E_POINTER);
                }
                else
                {
                    // Just copy the string from step 1, we can't do any meaningful processing.
                    hr = INET_E_DEFAULT_ACTION;
                }


                // If UrlGetPart didn't process it, just pass the original string back.
                if (!SUCCEEDED(hr))
                {
                    hr = S_OK;
                    StrCpyW(pwszRet, pwszSecUrl);
                }
            }
        }

        // End of Step 2. 
    

        if (bAllocSecUrl)
            delete [] pwszSecUrl;

        if (SUCCEEDED(hr))
        {
            TransAssert(pwszRet != NULL);
            *ppwszSecUrl = pwszRet;
        }
    }

    PerfDbgLog1(tagTransApi, NULL, "-CoInternetGetSecurityUrl (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoInternetCombineUrl
//
//  Synopsis:
//
//  Arguments:  [pwzBaseUrl] --
//              [pwzRelativeUrl] --
//              [dwFlags] --
//              [pszResult] --
//              [cchResult] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    4-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoInternetCombineUrl(
    LPCWSTR     pwzBaseUrl,
    LPCWSTR     pwzRelativeUrl,
    DWORD       dwFlags,
    LPWSTR      pszResult,
    DWORD       cchResult,
    DWORD      *pcchResult,
    DWORD       dwReserved
    )
{
    PerfDbgLog(tagTransApi, NULL, "+CoInternetCombineUrl");
    COInetSession *pOInetSession = 0;

    HRESULT hr = INET_E_DEFAULT_ACTION;

    if (!IsKnownProtocol(pwzBaseUrl))
    {
        hr = GetCOInetSession(0, &pOInetSession,0);

        if (hr == NOERROR)
        {
            hr = pOInetSession->CombineUrl(pwzBaseUrl, pwzRelativeUrl, dwFlags, pszResult, cchResult, pcchResult, dwReserved);
            pOInetSession->Release();
        }
    }
    if (hr == INET_E_DEFAULT_ACTION)
    {
        DWORD   dwRes = cchResult;

        hr = UrlCombineW(pwzBaseUrl, pwzRelativeUrl, pszResult, &dwRes, dwFlags);
        *pcchResult = dwRes;
    }

    PerfDbgLog1(tagTransApi, NULL, "-CoInternetCombineUrl (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoInternetCompareUrl
//
//  Synopsis:
//
//  Arguments:  [pwzUrl1] --
//              [pwzUrl2] --
//              [dwFlags] --
//
//  Returns:
//
//  History:    4-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoInternetCompareUrl(
    LPCWSTR pwzUrl1,
    LPCWSTR pwzUrl2,
    DWORD dwFlags
    )
{
    PerfDbgLog(tagTransApi, NULL, "+CoInternetCompareUrl");
    COInetSession *pOInetSession = 0;
    HRESULT hr = INET_E_DEFAULT_ACTION;

    if (!IsKnownProtocol(pwzUrl1))
    {
        hr = GetCOInetSession(0, &pOInetSession,0);

        if (hr == NOERROR)
        {
            hr = pOInetSession->CompareUrl(pwzUrl1, pwzUrl2, dwFlags);
            pOInetSession->Release();
        }
    }
    if (hr == INET_E_DEFAULT_ACTION)
    {
        int iRes = UrlCompareW(pwzUrl1, pwzUrl2, dwFlags & CF_INGNORE_SLASH);
        if (iRes == 0)
        {
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }

    PerfDbgLog1(tagTransApi, NULL, "-CoInternetCompareUrl (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoInternetQueryInfo
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [dwOptions] --
//              [pvBuffer] --
//              [cbBuffer] --
//              [pcbBuffer] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    4-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoInternetQueryInfo(
    LPCWSTR     pwzUrl,
    QUERYOPTION QueryOption,
    DWORD       dwQueryFlags,
    LPVOID      pvBuffer,
    DWORD       cbBuffer,
    DWORD      *pcbBuffer,
    DWORD       dwReserved
    )
{
    PerfDbgLog(tagTransApi, NULL, "+CoInternetQueryInfo");
    COInetSession *pOInetSession = 0;

    HRESULT hr = INET_E_DEFAULT_ACTION;

    if (!IsKnownProtocol(pwzUrl))
    {
        hr = GetCOInetSession(0, &pOInetSession,0);

        if (hr == NOERROR)
        {
            hr = pOInetSession->QueryInfo(pwzUrl, QueryOption, dwQueryFlags, pvBuffer, cbBuffer, pcbBuffer, dwReserved);
            pOInetSession->Release();
        }
    }
  
    if (hr == INET_E_DEFAULT_ACTION)
    {
        switch (QueryOption)
        {
        case QUERY_USES_NETWORK:
        case QUERY_USES_CACHE:
            {
                if (!pvBuffer || cbBuffer < sizeof(DWORD))
                    return E_FAIL;

                if (pcbBuffer)
                {
                    *pcbBuffer = sizeof(DWORD);
                }

                switch (GetUrlScheme(pwzUrl)) 
                {
                    case URL_SCHEME_FILE:
                    case URL_SCHEME_NEWS:
                    case URL_SCHEME_NNTP:
                    case URL_SCHEME_MK:
                    case URL_SCHEME_SHELL:
                    case URL_SCHEME_SNEWS:
                    case URL_SCHEME_LOCAL:
                        *((DWORD *)pvBuffer) = FALSE;
                        return S_OK;

                    case URL_SCHEME_FTP:
                    case URL_SCHEME_HTTP:
                    case URL_SCHEME_GOPHER:
                    case URL_SCHEME_TELNET:
                    case URL_SCHEME_WAIS:
                    case URL_SCHEME_HTTPS:
                        *((DWORD *)pvBuffer) = TRUE;
                        return S_OK;

                    default:
                        return E_FAIL;
                }
            }
            break;

         case QUERY_IS_CACHED:
         case QUERY_IS_INSTALLEDENTRY:
         case QUERY_IS_CACHED_OR_MAPPED:
            {
                char szUrl[MAX_URL_SIZE];
                DWORD dwFlags = 0;

                if(QueryOption == QUERY_IS_INSTALLEDENTRY)
                {
                    dwFlags = INTERNET_CACHE_FLAG_INSTALLED_ENTRY;
                }
                else if(QueryOption == QUERY_IS_CACHED_OR_MAPPED)
                {
                    dwFlags = INTERNET_CACHE_FLAG_ENTRY_OR_MAPPING;
                }
                // Otherwise let the flags remain as 0
                
                if (!pvBuffer || cbBuffer < sizeof(DWORD))
                    return E_FAIL;

                if (pcbBuffer)
                {
                    *pcbBuffer = sizeof(DWORD);
                }

                W2A(pwzUrl, szUrl, MAX_URL_SIZE);

                char *pchLoc = StrChr(szUrl, TEXT('#'));
                if (pchLoc)
                    *pchLoc = TEXT('\0');
        
                *((DWORD *)pvBuffer) = GetUrlCacheEntryInfoEx(szUrl, NULL, NULL, NULL, NULL, NULL, dwFlags);
                return S_OK;
            }
            break;
            
        default:
            // do not know what do to
            hr = E_FAIL;
        }
    }

    PerfDbgLog1(tagTransApi, NULL, "-CoInternetQueryInfo (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoInternetGetProtocolFlags
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [pdwFlags] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    4-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoInternetGetProtocolFlags(
    LPCWSTR     pwzUrl,
    DWORD      *pdwFlags,
    DWORD       dwReserved
    )
{
    PerfDbgLog(tagTransApi, NULL, "+CoInternetGetProtocolFlags");
    COInetSession *pOInetSession = 0;

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagTransApi, NULL, "-CoInternetGetProtocolFlags (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CoInternetCreateSecurityManager
//
//  Synopsis:
//
//  Arguments:  [pSP] --
//              [pSM] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    4-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoInternetCreateSecurityManager(IServiceProvider *pSP, IInternetSecurityManager **ppSM, DWORD dwReserved)
{
    PerfDbgLog(tagTransApi, NULL, "+CoInternetCreateUrlSecurityManager");
    HRESULT hr = NOERROR;

    hr = InternetCreateSecurityManager(0, IID_IInternetSecurityManager, (void **)ppSM, dwReserved);

    PerfDbgLog1(tagTransApi, NULL, "-CoInternetCreateUrlSecurityManager (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CoInternetCreateZoneManager
//
//  Synopsis:
//
//  Arguments:  [pSP] --
//              [ppZM] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    4-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CoInternetCreateZoneManager(IServiceProvider *pSP, IInternetZoneManager **ppZM, DWORD dwReserved)
{
    PerfDbgLog(tagTransApi, NULL, "+CoInternetCreateUrlZoneManager");
    HRESULT hr;

    hr = InternetCreateZoneManager(0, IID_IInternetZoneManager, (void **) ppZM, dwReserved);

    PerfDbgLog1(tagTransApi, NULL, "-CoInternetCreateUrlZoneManager (hr:%lx)", hr);
    return hr;
}


BOOL PDFNeedProgressiveDownload()
{
    HKEY    hClsRegEntry;
    CHAR    szInProc[CLSIDSTR_MAX + 64];
    CHAR    szPath[MAX_PATH];
    long    lSize;
    BOOL    fRet = TRUE;
    BYTE*   pVerBuffer = NULL;
    DWORD   cbFileVersionBufSize;
    DWORD   dwTemp;
    unsigned uiLength = 0;
    VS_FIXEDFILEINFO *lpVSFixedFileInfo;

    strcpy(szInProc, 
           "CLSID\\{CA8A9780-280D-11CF-A24D-444553540000}\\InProcServer32");

    if( ERROR_SUCCESS != RegOpenKey(
                    HKEY_CLASSES_ROOT, szInProc, &hClsRegEntry) )
    {
        goto Exit;
    }

    // now we are at HKCR\CLSID\xxx-yyyy\InProcServer32
    // we need to get the path to the ocx
    lSize = MAX_PATH;
    if( ERROR_SUCCESS != RegQueryValue(
                    hClsRegEntry, NULL, szPath, &lSize) )
    {
        RegCloseKey(hClsRegEntry);
        goto Exit;
    }

    // done with key
    RegCloseKey(hClsRegEntry);
    
    // we have the path now
    if((cbFileVersionBufSize = GetFileVersionInfoSize( szPath, &dwTemp)) == 0 )
    {
        goto Exit;
    }

    pVerBuffer = new BYTE[cbFileVersionBufSize];
    if( !pVerBuffer )
    {
        goto Exit;
    }

    if( !GetFileVersionInfo(szPath, 0, cbFileVersionBufSize, pVerBuffer) )
    {
        goto Exit;
    }

    if( !VerQueryValue( 
            pVerBuffer, TEXT("\\"),(LPVOID*)&lpVSFixedFileInfo, &uiLength) ) 
    {
        goto Exit;
    }

    if( lpVSFixedFileInfo->dwFileVersionMS == 0x00010003 &&
        lpVSFixedFileInfo->dwFileVersionLS < 170 )
    {
        // this is 3.0 or 3.01, we should disable progressive download
        fRet = FALSE;
    }


Exit:
    if( pVerBuffer != NULL)
        delete [] pVerBuffer;
    return fRet;    
}

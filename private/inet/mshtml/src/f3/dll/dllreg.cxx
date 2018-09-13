//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dllreg.cxx
//
//  Contents:   DllRegisterServer, DllUnRegisterServer
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_SITEGUID_H_
#define X_SITEGUID_H_
#include "siteguid.h"
#endif

#ifndef X_SITECNST_HXX_
#define X_SITECNST_HXX_
#include "sitecnst.hxx"
#endif

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include "coreguid.h"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_IIMGCTX_H_
#define X_IIMGCTX_H_
#include "iimgctx.h"
#endif

#ifndef X_ADVPUB_H_
#define X_ADVPUB_H_
#include <advpub.h>     // for RegInstall
#endif

//+------------------------------------------------------------------------
//
//  Prototypes
//
//+------------------------------------------------------------------------

HRESULT UnregisterServer(TCHAR *);

#ifndef WIN16
static DYNLIB s_dynlibADVPACK = { 0, 0, "ADVPACK.DLL" };
static DYNPROC s_dynprocREGINSTALL = { 0, &s_dynlibADVPACK, achREGINSTALL };
char g_achIEPath[MAX_PATH];     // path to iexplore.exe
#endif // ndef WIN16


//+------------------------------------------------------------------------
//
//  Function:   RegisterTypeLibraries
//
//  Synopsis:   Register the forms type libraries.
//
//-------------------------------------------------------------------------

static HRESULT
RegisterTypeLibraries()
{
    HRESULT     hr;
    ITypeLib *  pTL = NULL;
    TCHAR       ach[MAX_PATH];
    TCHAR *     pchName;

    // register the msdatsrc.tlb (databinding)
    int iLen = GetSystemDirectory(ach, MAX_PATH);
    if (iLen > 0)
    {
        _tcscat(ach, _T("\\msdatsrc.tlb"));
        hr = THR(LoadTypeLib(ach, &pTL));
        if (!hr && pTL)
        {
            hr = THR(RegisterTypeLib(pTL, ach, 0));
            pTL->Release();
        }
    }

    GetFormsTypeLibPath(ach);
    pchName = _tcsrchr(ach, '.');
    Assert(pchName);
    _tcscpy(pchName + 1, _T("tlb"));

    hr = THR(LoadTypeLib(ach, &pTL));
    if (!hr && pTL)
    {
        hr = THR(RegisterTypeLib(pTL, ach, 0));
        pTL->Release();
    }

    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Function:   UnregisterTypeLibraries
//
//  Synopsis:   Unregister the registered type libraries.
//
//-------------------------------------------------------------------------
static void
UnregisterTypeLibraries( )
{
    ITypeLib *  pTL;
    TLIBATTR *  ptla;

    if (!THR(GetFormsTypeLib(&pTL, TRUE)))
    {
        if (!THR(pTL->GetLibAttr(&ptla)))
        {
            UnRegisterTypeLib(ptla->guid,
                    ptla->wMajorVerNum,
                    ptla->wMinorVerNum,
                    ptla->lcid,
                    ptla->syskind);
            pTL->ReleaseTLibAttr(ptla);
        }
        pTL->Release();
    }
}

#ifndef WIN16
const CHAR  c_szIexploreKey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE";

//+------------------------------------------------------------------------
//
// Function:    GetIEPath
//
// Synopsis:    Queries the registry for the location of the path
//              of Internet Explorer and returns it in pszBuf.
//
// Returns:     TRUE on success
//              FALSE if path cannot be determined
//-------------------------------------------------------------------------

void
GetIEPath()
{
    BOOL fSuccess = FALSE;
    HKEY hkey;

    if (lstrlenA(g_achIEPath))
        return;

    g_achIEPath[0] = '\0';

    // Get the path of Internet Explorer
    if (NO_ERROR != RegOpenKeyA(HKEY_LOCAL_MACHINE, c_szIexploreKey, &hkey))
    {
        TraceTag((tagError, "InstallRegSet(): RegOpenKey( %s ) Failed", c_szIexploreKey)) ;
    }
    else
    {
        DWORD cbData = MAX_PATH;
        DWORD dwType;

        if (NO_ERROR != RegQueryValueExA(
                hkey,
                "",
                NULL,
                &dwType,
                (LPBYTE)g_achIEPath,
                &cbData))
        {
            TraceTag((tagError, "InstallRegSet(): RegQueryValueEx() for Iexplore path failed"));
        }
        else
        {
            fSuccess = TRUE;
        }

        RegCloseKey(hkey);
    }

    if (!fSuccess)
    {
        // Failed, just say "iexplore"
        lstrcpyA(g_achIEPath, "iexplore.exe");
    }
}

//+------------------------------------------------------------------------
//
// Function:    CallRegInstall
//
// Synopsis:    Calls the ADVPACK entry-point which executes an inf
//              file section.
//
//-------------------------------------------------------------------------

HRESULT
CallRegInstall(LPSTR szSection)
{
    HRESULT     hr = S_OK;
    STRENTRY    seReg[] = {{ "IEXPLORE", g_achIEPath} };
    STRTABLE    stReg = { 1, seReg };

    hr = THR(LoadProcedure(&s_dynprocREGINSTALL));
    if (hr)
        goto Cleanup;

    // Get the location of iexplore from the registry
    GetIEPath();

    hr = THR( ((REGINSTALL)s_dynprocREGINSTALL.pfn)(g_hInstCore, szSection, &stReg));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
// Function:    UnregisterServer
//
// Synopsis:    Delete everything from the registry that looks like ours.
//
//-------------------------------------------------------------------------

HRESULT
UnregisterServer(TCHAR *pstrDLL)
{
    RRETURN(CallRegInstall("Unreg"));
}
#endif // ndef WIN16


//+------------------------------------------------------------------------
//
// Function:    TestVersion
//
// Synopsis:    Check whether the  InprocServer32 points to a existing file
//              if not return true, otherwise check the version, if the
//              version is greater than current want to register , return
//              false, else return true
// Arguments:   [clsaaID] - specifies the class ID to register
//              [pstrExpectedVersion] - the expected version value.
//
//-------------------------------------------------------------------------
#if 0
BOOL
TestVersion(CLSID classID , TCHAR *pstrExpectedVersion)
{
    TCHAR strKey[128];
    TCHAR strValue[MAX_PATH];
    long cb;

    TCHAR *pstrFmtServer =
#ifndef _MAC
        TEXT("CLSID\\<0g>\\InprocServer32");
#else
        TEXT("CLSID\\<0g>\\InprocServer");
#endif

    TCHAR *pstrFmtVersion = TEXT("CLSID\\<0g>\\Version");

    Verify(Format(0, strKey, ARRAY_SIZE(strKey),
            pstrFmtServer, &classID) == S_OK);

    cb = sizeof(strValue);

    // If Can not Find InprocServer32 We should do register so return true
    if (RegQueryValue(HKEY_CLASSES_ROOT, strKey, strValue, &cb)
            != ERROR_SUCCESS)
        return TRUE;

    // Otherwise the server exists, Check whether it really exists

    TCHAR achPath[MAX_PATH];
    LPTSTR  lpszFilename;

    //Since the strValue includes full path, it ok to use searchpath here
    if (!SearchPath(NULL, strValue, NULL, MAX_PATH, achPath, &lpszFilename ))
        return TRUE;

    //otherwise check version
    Verify(Format(0, strKey, ARRAY_SIZE(strKey),
            pstrFmtVersion, &classID) == S_OK);

    cb = sizeof(strValue);

    // No Version Stamp also Return Success
    if (RegQueryValue(HKEY_CLASSES_ROOT, strKey, strValue, &cb)
        != ERROR_SUCCESS)
        return TRUE;

    //Otherwise compare the version
    if ( _tcsicmp(strValue, pstrExpectedVersion) <= 0)
        return TRUE;
    else
        return FALSE;
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   ShouldWeRegister
//
//  Synopsis:   Determine whether would should register a particular pic format
//
//  Returns:    TRUE - yes
//
//----------------------------------------------------------------------------

BOOL 
ShouldWeRegister(TCHAR *szFormat, TCHAR *szFileExt)
{
    TCHAR   aBuffer[MAX_PATH];
    LONG    lRet;
    HKEY    hkey = NULL;
    DWORD   cb=MAX_PATH;
    TCHAR   *lptszCmdName;

    lRet = RegOpenKeyEx(HKEY_CLASSES_ROOT,
                        szFileExt,
                        0,
                        KEY_READ,
                        &hkey);
    if( lRet == ERROR_SUCCESS )
    {
        lRet = RegQueryValueEx(hkey, NULL, 0, NULL, (LPBYTE)aBuffer, &cb);
        RegCloseKey(hkey);

        if (    lRet == ERROR_SUCCESS
            &&  cb != 0
            &&  lstrlen(aBuffer) != 0
            &&  _tcsicmp(aBuffer, szFormat) != 0)
        {
            return(FALSE);
        }
    }

    _tcscpy(aBuffer, szFormat);
    _tcscat(aBuffer, _T("\\shell\\open\\command"));
    lRet = RegOpenKeyEx(HKEY_CLASSES_ROOT,
                        aBuffer,
                        0,
                        KEY_READ,
                        &hkey);
    if( lRet != ERROR_SUCCESS )
        return(TRUE);

    // check if the key value is empty or has IEXplore already
    cb = MAX_PATH;
    lRet = RegQueryValueEx(hkey, NULL, 0, NULL, (LPBYTE)aBuffer, &cb);

    RegCloseKey(hkey);
    if (cb == 0 || lstrlen(aBuffer) == 0 || lRet != ERROR_SUCCESS)
        return(TRUE);

    PathRemoveArgs(aBuffer);
    lptszCmdName = PathFindFileName(aBuffer);
    if ( _tcsnicmp(lptszCmdName, 12, _T("iexplore.exe"), 12) == 0)
        return(TRUE);
    return( FALSE );
}


//+------------------------------------------------------------------------
//
// Function:    DllRegisterServer
//
// Synopsis:    Register objects and type libraries for this server
//              as described in the OLE Controls specification.
//
//-------------------------------------------------------------------------

extern BOOL ShouldWeRegisterCompatibilityTable();
extern BOOL ShouldWeRegisterUrlCompatibilityTable();

typedef struct
{
    TCHAR *tszName;
    LPSTR szSection;
    TCHAR *tszExt;
} PICFORMATREG;

static const PICFORMATREG aImgReg[] =
            {
                {_T("jpegfile"), "RegJPEG"  ,_T(".jpeg")},
                {_T("jpegfile"), "RegJPE"   ,_T(".jpe")},
                {_T("jpegfile"), "RegJPG"   ,_T(".jpg")},
                {_T("pngfile"),  "RegPNG"   ,_T(".png")},
                {_T("pjpegfile"),"RegPJPG"  ,_T(".jfif")},
                {_T("xbmfile"),  "RegXBM"   ,_T(".xbm")},
                {_T("giffile"),  "RegGIF"   ,_T(".gif")}
            };

STDAPI
DllRegisterServer()
{
    HRESULT hr;

    Assert(0 != _tcslen(g_achDLLCore));

    CEnsureThreadState ets;
    hr = ets._hr;
#ifndef UNIX  // CallRegInstall("Reg") has to be excuted
    if (FAILED(hr))
        return hr;
#endif

    hr = RegisterTypeLibraries();
#ifndef UNIX  // CallRegInstall("Reg") has to be excuted
    if (hr)
        goto Cleanup;
#endif

#ifndef WIN16
    hr = THR(CallRegInstall("Reg"));
    if (FAILED(hr))
        goto Cleanup;

    // The test to determine if we should register our compatibility table
    // is non trivial, so we allow clstab.cxx to implement the test since it
    // is the expert on the issue.
    if( ShouldWeRegisterCompatibilityTable() )
    {
        hr = THR(CallRegInstall("RegCompatTable"));
    }

    if (ShouldWeRegisterUrlCompatibilityTable())
    {
        hr = THR(CallRegInstall("RegUrlCompatTable"));
    }
#endif // ndef WIN16

Cleanup:
    RegFlushKey(HKEY_CLASSES_ROOT);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
// Function:    DllUnregisterServer
//
// Synopsis:    Undo the actions of DllRegisterServer.
//
//-------------------------------------------------------------------------

STDAPI
DllUnregisterServer()
{
    HRESULT hr;

    CEnsureThreadState ets;
    hr = ets._hr;
    if (FAILED(hr))
        return hr;

    UnregisterTypeLibraries();

#ifdef WIN16
    return hr;
#else
    Assert(_tcslen(g_achDLLCore));
    return UnregisterServer(g_achDLLCore);
#endif
}


#ifndef WIN16
//+------------------------------------------------------------------------
//
// Function:    DllInstall
//
// Synopsis:    Install/uninstall user settings
//
//-------------------------------------------------------------------------

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    HRESULT     hr = S_OK;
    BOOL        fPerUser = (pszCmdLine &&
                            (*pszCmdLine == L'u' || *pszCmdLine == L'U'));
    int     i;
    int     l = ARRAY_SIZE(aImgReg);

    if (fPerUser)
    {
        return S_FALSE;
    }

    if (bInstall)
    {
        hr = THR(CallRegInstall("Install"));

        for (i = 0; i < l; i ++)
        {
            if (ShouldWeRegister(aImgReg[i].tszName, aImgReg[i].tszExt))
            {
                HRESULT hr2;

                hr2 = THR(CallRegInstall(aImgReg[i].szSection));
                if (FAILED(hr2))
                {
                    hr = hr2;
                }
            }
        }
    }
    else
    {
        hr = THR(CallRegInstall("Uninstall"));
    }

    RRETURN(hr);
}
#endif // ndef WIN16

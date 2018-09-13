//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       padreg.cxx
//
//  Contents:   Register pad stuff
//
//----------------------------------------------------------------------------

#include "padhead.hxx"

#include "platform.h"

#define MSHTML_STR  _T("mshtml.dll")
#define MSHTMLED_STR _T("mshtmled.dll")

typedef HRESULT (STDAPICALLTYPE *CTLREGPROC)();
extern void DeinitDynamicLibraries();


//+------------------------------------------------------------------------
//
//  Format Strings
//
//  The following strings describe the data added to the registry.
//
//  The strings consist of major keys followed by subkey/value pairs.
//  Each set of subkey/value pairs is terminated with two nulls (that is,
//  the next subkey name is missing). The entire list is terminated
//  by three nulls (that is, the next major key is missing). Major
//  keys are allowed one value without an associated name (subkey); the
//  value of the first subkey, if the subkey name is missing, is treated
//  as the value for the major key. Since a missing subkey name usually
//  terminates the list of subkey/value pairs, the value for the major key
//  (the one without a subkey) *must* be first in the set of subkey/value
//  pairs. This works because all keys must have at least one value.
//
//  (While the values of named subkeys may be other than strings, only
//   strings are supported. To support more than strings, these structures
//   should be changed to precede the value with a single character type ID.)
//
//  To keep the strings clear, the major keys occur first, on a line
//  by themselves, followed by the pairs of subkey/value indented underneath.
//  The terminator for the subkey/value pair stands on a line by itself as well.
//
//  Each key and value string is used as a format string in the Format
//  function (see CORE\FORMAT.CXX for more info).  The arguments for
//  substitution into these strings are:
//
//      0 pstr      - Name of EXE
//      1 pclsid    - Class id
//      2 pstr      - Prog ID
//      3 pstr      - Friendly name
//      4 idr       - Icon
//
//-------------------------------------------------------------------------

#define DEFAULT_VALUE   TEXT("\0")

static TCHAR s_strCLSIDFmt[] =
    TEXT("<1g>\0")
        DEFAULT_VALUE               TEXT("<3s>\0")
        TEXT("\0")
        TEXT("LocalServer32\0")
                DEFAULT_VALUE               TEXT("<0s>\0")
        TEXT("\0")
    TEXT("ProgID\0")
        DEFAULT_VALUE               TEXT("<2s>\0")
        TEXT("\0")
    TEXT("DefaultIcon\0")
        DEFAULT_VALUE               TEXT("<0s>,-<4d>\0")
        TEXT("\0")
    TEXT("\0");

static TCHAR s_strProgIDFmt[] =
    TEXT("<2s>\0")
        DEFAULT_VALUE               TEXT("<3s>\0")
        TEXT("\0")
    TEXT("CLSID\0")
        DEFAULT_VALUE               TEXT("<1g>\0")
        TEXT("\0")
    TEXT("\0");

//+------------------------------------------------------------------------
//
//  Function:   GetPadDLLName
//
//  Synopsis:   Return a .DLL pathname for a .DLL in the same directory
//              as the pad .EXE
//
//-------------------------------------------------------------------------

void
GetPadDLLName(TCHAR * pszDLL, TCHAR * achBuf, int cchBuf)
{
    TCHAR * pName;

        achBuf[0] = 0;
    Verify(::GetModuleFileName(NULL, achBuf, cchBuf));
    pName = _tcsrchr(achBuf, FILENAME_SEPARATOR);
    if (pName)
    {
        *pName = '\0';              // Remove the name.
    }

    _tcscat(achBuf, pszDLL);
}

//+------------------------------------------------------------------------
//
// Function:    RegisterClass
//
// Synopsis:    Register a single class.
//
//-------------------------------------------------------------------------

HRESULT
RegisterOneClass(HKEY hkeyCLSID, TCHAR *pchFile, REFCLSID clsid, TCHAR *pchProgID, TCHAR *pchFriendly)
{
    HRESULT hr;
    DWORD_PTR adwArgs[10];

    adwArgs[0] = (DWORD_PTR)pchFile;
    adwArgs[1] = (DWORD_PTR)&clsid;
    adwArgs[2] = (DWORD_PTR)pchProgID;
    adwArgs[3] = (DWORD_PTR)pchFriendly;
    adwArgs[4] = IDR_PADICON;

    hr = THR(RegDbSetValues(HKEY_CLASSES_ROOT, s_strProgIDFmt, adwArgs));
    if (hr)
        goto Cleanup;

    hr = THR(RegDbSetValues(hkeyCLSID, s_strCLSIDFmt, adwArgs));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
// Function:    RegisterPad
//
// Synopsis:    Register objects for this server.
//
//-------------------------------------------------------------------------

HRESULT
RegisterPad()
{
    HRESULT hr;
    HKEY    hkeyCLSID = 0;
    TCHAR   achExe[MAX_PATH];
    HINSTANCE hInst = 0;

    GetModuleFileName(hInst, achExe, ARRAY_SIZE(achExe));

    hr = THR(RegDbOpenCLSIDKey(&hkeyCLSID));
    if (hr)
        RRETURN(hr);

    hr = THR(RegisterOneClass(
            hkeyCLSID,
            achExe,
            CLSID_Pad,
            _T("TridentPad"),
            _T("Trident Pad")));
    if (hr)
        goto Cleanup;

Cleanup:
    RegCloseKey(hkeyCLSID);
    RegFlushKey(HKEY_CLASSES_ROOT);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
// Function:    RegisterTrident
//
// Purpose:     Registers Mshtml.dll as well as Mshtmled.dll
//-------------------------------------------------------------------------

HRESULT
RegisterTrident(HWND hwnd, BOOL fDialog, BOOL fSystem)
{
    TCHAR       achBuf[MAX_PATH];
    TCHAR       achMshtmled[MAX_PATH];
    HRESULT     hr;

    if (fSystem)
    {
        Verify(GetSystemDirectory(achBuf, ARRAY_SIZE(achBuf)));
        _tcscat(achBuf, 
                _T(FILENAME_SEPARATOR_STR)
                MSHTML_STR);
	Verify(GetSystemDirectory(achMshtmled, ARRAY_SIZE(achMshtmled)));
	_tcscat(achMshtmled,
		_T(FILENAME_SEPARATOR_STR)
		MSHTMLED_STR);

    }
    else
    {
        GetPadDLLName(_T(FILENAME_SEPARATOR_STR)
                      MSHTML_STR, achBuf, ARRAY_SIZE(achBuf));
        GetPadDLLName(_T(FILENAME_SEPARATOR_STR)
	 	      MSHTMLED_STR, achMshtmled, ARRAY_SIZE(achMshtmled));
    }

    hr = RegisterDLL(achBuf);
    if (hr)
        goto Error;

    hr = RegisterDLL(achMshtmled);
    if (hr)
        goto Error;

    if (fDialog)
    {
        MessageBox(
            hwnd,
            _T("Trident MSHTML registered as HTML viewer."),
            SZ_APPLICATION_NAME,
            MB_APPLMODAL | MB_OK);
    }

Error:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
// Function:    RegisterLocalCLSIDs
//
//-------------------------------------------------------------------------

DYNLIB  g_dynlibMSHTML = { NULL, NULL, "mshtml.dll" };
DYNPROC g_dynprocMSHTMLClassObjects = { NULL, &g_dynlibMSHTML, "DllEnumClassObjects" };
DYNLIB  g_dynlibMSHTMLED = { NULL, NULL, "mshtmled.dll" };
DYNPROC g_dynprocMSHTMLEDClassObjects = { NULL, &g_dynlibMSHTMLED, "DllEnumClassObjects" };
DYNLIB  g_dynlibIEPEERS = { NULL, NULL, "iepeers.dll" };
DYNPROC g_dynprocIEPEERSClassObjects = { NULL, &g_dynlibIEPEERS, "DllEnumClassObjects" };

DYNPROC * g_adynprocDlls[] = {
                                &g_dynprocMSHTMLClassObjects,
#ifndef UNIX
                                &g_dynprocMSHTMLEDClassObjects,
#endif
                                &g_dynprocIEPEERSClassObjects
                             };

HRESULT
RegisterLocalCLSIDs()
{
    PADTHREADSTATE * pts = GetThreadState();
    int         i, j;
    HRESULT     hr = S_OK;
    CLSID       clsid;
    int         cFactory = 0;

    IUnknown ** pUnkFactory = pts->pUnkFactory;
    DWORD *     dwCookie    = pts->dwCookie;

    if (pts->fLocalReg)
        return S_OK;

    // We register class objects per thread, so hold on to
    // one instance of library per thread.

    for (i=0; i < ARRAY_SIZE(g_adynprocDlls); i++)
    {
        //
        // We do an explicit load to increment the system's refcount on the DLL.
        //
        pts->hinstDllThread[i] = LoadLibraryA(g_adynprocDlls[i]->pdynlib->achName);

        hr = THR(LoadProcedure(g_adynprocDlls[i]));
        if (hr)
        {
#if DBG == 1
            if (hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                TraceTag((tagError, "\n\n**** WARNING! Could not locally register class "
                          "factories for %hs!", g_adynprocDlls[i]->pdynlib->achName));
                TraceTagEx((tagError, TAG_NONAME, "OLE objects may be pulled from the system DLL "
                          "instead of the local DLL.\n"));
            }
#endif

            // Couldn't load this DLL. Go on to the next one.
            hr = S_OK;
            continue;
        }

        for (j = 0; cFactory < PUNKFACTORY_ARRAY_SIZE; j++, cFactory++)
        {
            hr = THR(((HRESULT (STDAPICALLTYPE *)(int, CLSID *, IUnknown **))
                    g_adynprocDlls[i]->pfn)(j,
                        &clsid,
                        &pUnkFactory[cFactory]));
            if (hr)
                break;

            hr = THR(CoRegisterClassObject(clsid,
                    pUnkFactory[cFactory],
                    CLSCTX_INPROC_SERVER,
                    REGCLS_MULTIPLEUSE,
                    &dwCookie[cFactory]));
            if (hr)
                break;
        }

        if (!OK(hr))
            RRETURN(hr);
    }

    AssertSz(cFactory < PUNKFACTORY_ARRAY_SIZE, "Ran out of room to register factories!");

    if (OK(hr))
        hr = S_OK;

    pts->fLocalReg = hr == S_OK;

    RRETURN(hr);
}

void
UnregisterLocalCLSIDs()
{
    PADTHREADSTATE * pts = GetThreadState();
    IUnknown ** pUnkFactory = pts->pUnkFactory;
    DWORD * dwCookie = pts->dwCookie;
    int i;

    for (i = 0; i < PUNKFACTORY_ARRAY_SIZE; i++)
    {
        if (dwCookie[i])
        {
            Verify(OK(CoRevokeClassObject(dwCookie[i])));
            dwCookie[i] = 0;
        }

        ClearInterface(&pUnkFactory[i]);
    }

    for (i = 0; i < NUM_LOCAL_DLLS; i++)
    {
        if (pts->hinstDllThread[i] != NULL)
        {
            FreeLibrary(pts->hinstDllThread[i]);
        }
    }

    DeinitDynamicLibraries();

    pts->fLocalReg = FALSE;
}

//+------------------------------------------------------------------------
//
// Function:    RegisterDLL
//
//-------------------------------------------------------------------------

HRESULT
RegisterDLL(LPOLESTR Path)
{
    HMODULE     hModule = NULL;
    CTLREGPROC  DLLRegisterServer;
    HRESULT     hr = E_FAIL;

    hModule = ::LoadLibraryEx(Path, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hModule)
    {
        goto WinError;
    }

    // Control found try to register.
    DLLRegisterServer = (CTLREGPROC)::GetProcAddress(hModule,
                                                       "DllRegisterServer");

    if (DLLRegisterServer == NULL)
    {
        goto WinError;
    }

    hr = THR(DLLRegisterServer());  // Register control

Cleanup:
    if (hModule)
    {
        ::FreeLibrary(hModule);
    }
#if DBG == 1
    OutputDebugString(_T("\n\rDLL "));
    OutputDebugString((hr == S_OK) ?
                        _T("registered: ") :
                        _T("NOT registered: "));
    OutputDebugString(Path);
    OutputDebugString(_T("\n\r"));
#endif
    RRETURN(hr);

WinError:
    hr = HRESULT_FROM_WIN32(::GetLastError());
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Function:   NukeKnownDLLStuff
//
//  Synopsis:   Removes standard dlls from the knowndll list.
//
//----------------------------------------------------------------------------

HRESULT
NukeKnownDLLStuff()
{
    HRESULT             hr      = S_OK;
    long                error;
    HKEY                hkey    = NULL;
    long                i;
    BOOL                fChanged = FALSE;
    static char *       g_achDLLs[] =
        { "shdocvw", "urlmon", "wininet" };

    // Check if oleaut32 is in known dlls list.
    error = TW32_NOTRACE(1, RegOpenKeyA(
            HKEY_LOCAL_MACHINE,
            "System\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs",
            &hkey));
    if (error)
    {
        // On Win95, the key is spelled differently.
        error = TW32(1, RegOpenKeyA(
                HKEY_LOCAL_MACHINE,
                "System\\CurrentControlSet\\Control\\SessionManager\\KnownDLLs",
                &hkey));

        if (error)
            goto Cleanup;
    }

    for (i = ARRAY_SIZE(g_achDLLs); i > 0; i--)
    {
        error = TW32_NOTRACE(1, RegQueryValueExA(
                hkey,
                g_achDLLs[i-1],
                NULL,
                NULL,
                NULL,
                NULL));
        if (error)
            continue;

        // Delete the value.
        error = TW32(1, RegDeleteValueA(hkey, g_achDLLs[i-1]));
        if (error)
            goto Win32Error;

        fChanged = TRUE;
    }

Cleanup:
    if (hkey)
        Verify(!RegCloseKey(hkey));

    if (!hr)
    {
        MessageBox(
            NULL,
            fChanged ?
                _T("Known DLLs nuked OK. Need Reboot.") :
                _T("Known DLLs nuked OK."),
            SZ_APPLICATION_NAME,
            MB_APPLMODAL | MB_OK);
    }
    else
    {
        MessageBox(
            NULL,
            _T("Failed nuking Known DLLs."),
            SZ_APPLICATION_NAME,
            MB_APPLMODAL | MB_OK);
    }

    RRETURN(hr);

Win32Error:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Cleanup;
}



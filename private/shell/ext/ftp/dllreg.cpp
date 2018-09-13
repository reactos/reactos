// dllreg.cpp -- autmatic registration and unregistration
//
#include "priv.h"
#include "installwv.h"

#include <advpub.h>
#include <comcat.h>
#include <msieftp.h>


// helper macros

// ADVPACK will return E_UNEXPECTED if you try to uninstall (which does a registry restore)
// on an INF section that was never installed.  We uninstall sections that may never have
// been installed, so this MACRO will quiet these errors.
#define QuietInstallNoOp(hr)   ((E_UNEXPECTED == hr) ? S_OK : hr)


const CHAR  c_szIexploreKey[]         = "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE";

/*----------------------------------------------------------
Purpose: Queries the registry for the location of the path
         of Internet Explorer and returns it in pszBuf.

Returns: TRUE on success
         FALSE if path cannot be determined

Cond:    --
*/
BOOL
GetIEPath(
    OUT LPSTR pszBuf,
    IN  DWORD cchBuf)
{
    BOOL bRet = FALSE;
    HKEY hkey;

    *pszBuf = '\0';

    // Get the path of Internet Explorer 
    if (NO_ERROR != RegOpenKeyA(HKEY_LOCAL_MACHINE, c_szIexploreKey, &hkey))  
    {
    }
    else
    {
        DWORD cbBrowser;
        DWORD dwType;

        lstrcatA(pszBuf, "\"");

        cbBrowser = CbFromCchA(cchBuf - lstrlenA(" -nohome") - 4);
        if (NO_ERROR != RegQueryValueExA(hkey, "", NULL, &dwType, 
                                         (LPBYTE)&pszBuf[1], &cbBrowser))
        {
        }
        else
        {
            bRet = TRUE;
        }

        lstrcatA(pszBuf, "\"");

        RegCloseKey(hkey);
    }

    return bRet;
}


BOOL UnregisterTypeLibrary(const CLSID* piidLibrary)
{
    TCHAR szScratch[GUIDSTR_MAX];
    HKEY hk;
    BOOL fResult = FALSE;

    // convert the libid into a string.
    //
    SHStringFromGUID(*piidLibrary, szScratch, ARRAYSIZE(szScratch));

    if (RegOpenKey(HKEY_CLASSES_ROOT, TEXT("TypeLib"), &hk) == ERROR_SUCCESS) {
        fResult = RegDeleteKey(hk, szScratch);
        RegCloseKey(hk);
    }
    
    return fResult;
}



HRESULT FtpRegTypeLib(void)
{
    HRESULT hr = S_OK;
    ITypeLib *pTypeLib;
    DWORD   dwPathLen;
    TCHAR   szTmp[MAX_PATH];
#ifdef UNICODE
    WCHAR   *pwsz = szTmp; 
#else
    WCHAR   pwsz[MAX_PATH];
#endif

    // Load and register our type library.
    //
    dwPathLen = GetModuleFileName(HINST_THISDLL, szTmp, ARRAYSIZE(szTmp));
#ifndef UNICODE
    if (SHAnsiToUnicode(szTmp, pwsz, MAX_PATH)) 
#endif
    {
        hr = LoadTypeLib(pwsz, &pTypeLib);

        if (SUCCEEDED(hr))
        {
            // call the unregister type library as we had some old junk that
            // was registered by a previous version of OleAut32, which is now causing
            // the current version to not work on NT...
            UnregisterTypeLibrary(&LIBID_MSIEFTPLib);
            hr = RegisterTypeLib(pTypeLib, pwsz, NULL);

            if (FAILED(hr))
            {
                TraceMsg(TF_WARNING, "MSIEFTP: RegisterTypeLib failed (%x)", hr);
            }
            pTypeLib->Release();
        }
        else
        {
            TraceMsg(TF_WARNING, "MSIEFTP: LoadTypeLib failed (%x)", hr);
        }
    } 
#ifndef UNICODE
    else {
        hr = E_FAIL;
    }
#endif

    return hr;
}


/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.

Returns: 
Cond:    --
*/
HRESULT CallRegInstall(HINSTANCE hinstFTP, LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            char szThisDLL[MAX_PATH];

            // Get the location of this DLL from the HINSTANCE
            if ( !EVAL(GetModuleFileNameA(hinstFTP, szThisDLL, ARRAYSIZE(szThisDLL))) )
            {
                // Failed, just say "msieftp.exe"
                lstrcpyA(szThisDLL, "msieftp.exe");
            }

            STRENTRY seReg[] = {
                { "THISDLL", szThisDLL },

                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg) - 2, seReg };

            hr = pfnri(g_hinst, szSection, &stReg);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}


STDAPI DllRegisterServer(void)
{
    HRESULT hrInternal;
    HRESULT hr;

    // Delete any old registration entries, then add the new ones.
    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
    // (The inf engine doesn't guarantee DelReg/AddReg order, that's
    // why we explicitly unreg and reg here.)
    //
    HINSTANCE hinstFTP = GetModuleHandle(TEXT("MSIEFTP.DLL"));
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    hr = CallRegInstall(hinstFTP, "FtpShellExtensionInstall");
    ASSERT(SUCCEEDED(hr));

    hrInternal = InstallWebViewFiles(hinstFTP);
    if (SUCCEEDED(hr))
        EVAL(SUCCEEDED(hr = hrInternal));   // Propogate the error and assert.

    FtpRegTypeLib();
    if (hinstAdvPack)
        FreeLibrary(hinstAdvPack);

    return hr;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;
    HINSTANCE hinstFTP = GetModuleHandle(TEXT("MSIEFTP.DLL"));

    // UnInstall the registry values
    hr = CallRegInstall(hinstFTP, "FtpShellExtensionUninstall");
    UnregisterTypeLibrary(&LIBID_MSIEFTPLib);

    return hr;
}


/*----------------------------------------------------------
Purpose: Install/uninstall user settings

Description: Note that this function has special error handling.
             The function will keep hrExternal with the worse error
             but will only stop executing util the internal error (hr)
             gets really bad.  This is because we need the external
             error to catch incorrectly authored INFs but the internal
             error to be robust in attempting to install other INF sections
             even if one doesn't make it.
*/
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK;    
}    




class CFtpInstaller     : public IFtpInstaller
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IFtpInstaller ***
    virtual STDMETHODIMP IsIEDefautlFTPClient(void);
    virtual STDMETHODIMP RestoreFTPClient(void);
    virtual STDMETHODIMP MakeIEDefautlFTPClient(void);

protected:
    CFtpInstaller();
    ~CFtpInstaller();

    friend HRESULT CFtpInstaller_Create(REFIID riid, LPVOID * ppv);

private:
    int                     m_cRef;
};




/*****************************************************************************\
    FUNCTION: CFtpInstaller_Create

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpInstaller_Create(REFIID riid, LPVOID * ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CFtpInstaller * pfi = new CFtpInstaller();

    if (EVAL(pfi))
    {
        hr = pfi->QueryInterface(riid, ppv);
        pfi->Release();
    }

    return hr;
}



/****************************************************\
    Constructor
\****************************************************/
CFtpInstaller::CFtpInstaller() : m_cRef(1)
{
    DllAddRef();
}


/****************************************************\
    Destructor
\****************************************************/
CFtpInstaller::~CFtpInstaller()
{
    DllRelease();
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CFtpInstaller::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CFtpInstaller::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CFtpInstaller::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IFtpInstaller))
    {
        *ppvObj = SAFECAST(this, IFtpInstaller*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


/***************************************************\
    Return values:
        S_OK - IE is default FTP client AND other client exists
        S_FALSE - IE not default FTP client (other client exists of course)
        E_FAIL - IE is default FTP client AND no other client exists
\***************************************************/
HRESULT CFtpInstaller::IsIEDefautlFTPClient(void)
{
    HRESULT hr = E_FAIL;
    TCHAR szDefaultFTPClient[MAX_PATH];
    DWORD cbSize = sizeof(szDefaultFTPClient);

    if (EVAL(ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, SZ_REGKEY_FTPCLASS, SZ_REGVALUE_DEFAULT_FTP_CLIENT, NULL, szDefaultFTPClient, &cbSize)))
    {
        // Are we the default client?
        if (!StrCmpI(szDefaultFTPClient, SZ_REGDATA_IE_FTP_CLIENT))
        {
            DWORD dwType;

            // Yes.  Is someone else installed?
            if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_FTPFOLDER, SZ_REGVALUE_PREVIOUS_FTP_CLIENT, &dwType, szDefaultFTPClient, &cbSize))
            {
                // Yes, so display UI so the user can switch back to them.
                hr = S_OK;
            }
        }
        else
        {
            // No, so someone else is installed and is default.  Display UI.
            hr = S_FALSE;
        }
    }

    return hr;
}


HRESULT CFtpInstaller::RestoreFTPClient(void)
{
    HRESULT hr = S_OK;
    TCHAR szDefaultFTPClient[MAX_PATH];
    DWORD cbSize = sizeof(szDefaultFTPClient);

    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_FTPFOLDER, SZ_REGVALUE_PREVIOUS_FTP_CLIENT, NULL, szDefaultFTPClient, &cbSize))
    {
        EVAL(ERROR_SUCCESS == SHSetValue(HKEY_CLASSES_ROOT, SZ_REGKEY_FTPCLASS, SZ_REGVALUE_DEFAULT_FTP_CLIENT, REG_SZ, szDefaultFTPClient, ((lstrlen(szDefaultFTPClient) + 1) * sizeof(TCHAR))));
        EVAL(ERROR_SUCCESS == SHDeleteValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_FTPFOLDER, SZ_REGVALUE_PREVIOUS_FTP_CLIENT));
    }

    return hr;
}


HRESULT BackupCurrentFTPClient(void)
{
    HRESULT hr = S_OK;
    TCHAR szDefaultFTPClient[MAX_PATH];
    DWORD cbSize = sizeof(szDefaultFTPClient);

    // Is a handler installed and is it not ours?
    if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, SZ_REGKEY_FTPCLASS, SZ_REGVALUE_DEFAULT_FTP_CLIENT, NULL, szDefaultFTPClient, &cbSize) &&
        StrCmpI(szDefaultFTPClient, SZ_REGDATA_IE_FTP_CLIENT))
    {
        // Yes, so back it up to be restored later if needed.
        EVAL(ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, SZ_REGKEY_FTPCLASS, SZ_REGVALUE_DEFAULT_FTP_CLIENT, NULL, szDefaultFTPClient, &cbSize));
        EVAL(ERROR_SUCCESS == SHSetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_FTPFOLDER, SZ_REGVALUE_PREVIOUS_FTP_CLIENT, REG_SZ, szDefaultFTPClient, ((lstrlen(szDefaultFTPClient) + 1) * sizeof(TCHAR))));
    }


    return hr;
}


HRESULT CFtpInstaller::MakeIEDefautlFTPClient(void)
{
    HRESULT hr = S_OK;

    EVAL(SUCCEEDED(BackupCurrentFTPClient()));
    HINSTANCE hinstFTP = GetModuleHandle(TEXT("MSIEFTP.DLL"));
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    hr = CallRegInstall(hinstFTP, "FtpForceAssociations");
    ASSERT(SUCCEEDED(hr));
    if (hinstAdvPack)
        FreeLibrary(hinstAdvPack);

    return hr;
}

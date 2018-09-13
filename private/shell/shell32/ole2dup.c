#include "shellprv.h"
#pragma  hdrstop

// this makes sure the DLL for the given clsid stays in memory
// this is needed because we violate COM rules and hold apparment objects
// across the lifetime of appartment threads. these objects really need
// to be free threaded (we have always treated them as such)

STDAPI_(HINSTANCE) SHPinDllOfCLSIDStr(LPCTSTR pszCLSID)
{
    CLSID clsid;

    SHCLSIDFromString(pszCLSID, &clsid);
    return SHPinDllOfCLSID(&clsid);
}


// translate string form of CLSID into binary form

STDAPI SHCLSIDFromString(LPCTSTR psz, CLSID *pclsid)
{
    *pclsid = CLSID_NULL;
    if (psz == NULL) 
        return NOERROR;
    return GUIDFromString(psz, pclsid) ? NOERROR : CO_E_CLASSSTRING;
}

BOOL _IsShellDll(LPCTSTR pszDllPath)
{
    LPCTSTR pszDllName = PathFindFileName(pszDllPath);
    return lstrcmpi(pszDllName, TEXT("shell32.dll")) == 0;
}

#ifdef WINNT

HKEY g_hklmApprovedExt = (HKEY)-1;    // not tested yet

// On NT, we must check to ensure that this CLSID exists in
// the list of approved CLSIDs that can be used in-process.
// If not, we fail the creation with ERROR_ACCESS_DENIED.
// We explicitly allow anything serviced by this DLL

BOOL _IsShellExtApproved(LPCTSTR pszClass, LPCTSTR pszDllPath)
{
    BOOL fIsApproved = TRUE;

    ASSERT(!_IsShellDll(pszDllPath));

    if (SHRestricted(REST_ENFORCESHELLEXTSECURITY))
    {
        if (g_hklmApprovedExt == (HKEY)-1)
        {
            g_hklmApprovedExt = NULL;
            RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"), &g_hklmApprovedExt);
        }

        if (g_hklmApprovedExt)
        {
            fIsApproved = SHQueryValueEx(g_hklmApprovedExt, pszClass, 0, NULL, NULL, NULL) == ERROR_SUCCESS;
            if (!fIsApproved)
            {
                HKEY hk;
                if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"), &hk) == ERROR_SUCCESS)
                {
                    fIsApproved = SHQueryValueEx(hk, pszClass, 0, NULL, NULL, NULL) == ERROR_SUCCESS;
                    RegCloseKey(hk);
                }
            }
        }
    }
    return fIsApproved;
}

#else

#define _IsShellExtApproved(x, y)  (TRUE)   // win95 does not require this

#endif // WINNT

#ifdef DEBUG
__inline BOOL IsGuimodeSetupRunning()
{
    DWORD dwSystemSetupInProgress;
    DWORD dwType;
    DWORD dwSize;
    
    dwSize = sizeof(dwSystemSetupInProgress);
    if ((SHGetValueW(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", L"SystemSetupInProgress", &dwType, (LPVOID)&dwSystemSetupInProgress, &dwSize) == ERROR_SUCCESS) &&
        (dwType == REG_DWORD))
    {
        return (BOOL)dwSystemSetupInProgress;
    }

    return FALSE;
}
#endif // DEBUG

typedef HRESULT (__stdcall *PFNDLLGETCLASSOBJECT)(REFCLSID rclsid, REFIID riid, void **ppv);

HRESULT _CreateFromDllGetClassObject(PFNDLLGETCLASSOBJECT pfn, const CLSID *pclsid, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    IClassFactory *pcf;
    HRESULT hres = pfn(pclsid, &IID_IClassFactory, &pcf);
    if (SUCCEEDED(hres))
    {
        hres = pcf->lpVtbl->CreateInstance(pcf, punkOuter, riid, ppv);
#ifdef DEBUG
        if (SUCCEEDED(hres))
        {
            // confirm that OLE can create this our objects to
            // make sure our objects are really CoCreateable

            IUnknown *punk;
            HRESULT hresTemp = CoCreateInstance(pclsid, punkOuter, CLSCTX_INPROC_SERVER, riid, &punk);
            if (SUCCEEDED(hresTemp))
            {
                punk->lpVtbl->Release(punk);
            }
            else
            {
                if (hresTemp == CO_E_NOTINITIALIZED)
                {
                    // shell32.dll works without com being inited
                    TraceMsg(TF_WARNING, "shell32 or friend object used without COM being initalized");
                }
                else if ((hresTemp != E_OUTOFMEMORY) && // stress can hit the E_OUTOFMEMORY case
                         !IsGuimodeSetupRunning())      // and we don't want to fire the assert during guimode (shell32 might not be registered yet)
                {
                    // others failures are bad
                    RIPMSG(FALSE, "CoCreate failed with %x", hresTemp);
                }
            }
        }
#endif
        pcf->lpVtbl->Release(pcf);
    }
    return hres;
}


HRESULT _CreateFromShell(const CLSID *pclsid, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return _CreateFromDllGetClassObject(DllGetClassObject, pclsid, punkOuter, riid, ppv);
}

HRESULT _CreateFromDll(LPCTSTR pszDllPath, const CLSID *pclsid, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HMODULE hmod = LoadLibraryEx(pszDllPath,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hmod)
    {
        HRESULT hres;
        PFNDLLGETCLASSOBJECT pfn = (PFNDLLGETCLASSOBJECT)GetProcAddress(hmod, "DllGetClassObject");
        if (pfn)
            hres = _CreateFromDllGetClassObject(pfn, pclsid, punkOuter, riid, ppv);
        else
            hres = E_FAIL;

        if (FAILED(hres))
            FreeLibrary(hmod);
        return hres;
    }
    return HRESULT_FROM_WIN32(GetLastError());
}

STDAPI SHGetInProcServerForClass(const CLSID *pclsid, LPTSTR pszDllPath, LPTSTR pszClass, BOOL *pbLoadWithoutCOM)
{
    TCHAR szInProcServer[GUIDSTR_MAX + 64];    // InProcServer32
    HKEY hkeyInProcServer;
    DWORD dwSize = MAX_PATH * SIZEOF(TCHAR);  // convert to count of bytes

    SHStringFromGUID(pclsid, szInProcServer, ARRAYSIZE(szInProcServer));

    lstrcpy(pszClass, szInProcServer);

    *pszDllPath = 0;

    lstrcat(szInProcServer, TEXT("\\InProcServer32"));

    if (RegOpenKeyEx(g_hkcrCLSID, szInProcServer, 0, KEY_QUERY_VALUE, &hkeyInProcServer) == ERROR_SUCCESS)
    {
        SHQueryValueEx(hkeyInProcServer, NULL, 0, NULL, (BYTE *)pszDllPath, &dwSize);

        *pbLoadWithoutCOM = SHQueryValueEx(hkeyInProcServer, TEXT("LoadWithoutCOM"), NULL, NULL, NULL, NULL) == ERROR_SUCCESS;

        RegCloseKey(hkeyInProcServer);
    }

    return *pszDllPath ? S_OK : REGDB_E_CLASSNOTREG;
}


STDAPI _SHCoCreateInstance(const CLSID * pclsid, IUnknown *punkOuter, DWORD dwCoCreateFlags, BOOL bMustBeApproved, REFIID riid, void **ppv)
{
    TCHAR szClass[GUIDSTR_MAX + 64], szDllPath[MAX_PATH];
    HRESULT hres;
    BOOL bLoadWithoutCOM;

    *ppv = NULL;

    hres = SHGetInProcServerForClass(pclsid, szDllPath, szClass, &bLoadWithoutCOM);
    if (SUCCEEDED(hres))
    {
        if (_IsShellDll(szDllPath))
        {
            hres = THR(_CreateFromShell(pclsid, punkOuter, riid, ppv));
        }
        else if (bMustBeApproved && !_IsShellExtApproved(szClass, szDllPath))
        {
            TraceMsg(TF_ERROR, "SHCoCreateInstance() %s needs to be registered under HKLM or HKCU"
                ",Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", szClass);
            hres = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        }
        else
        {
            // nt4 com validates the CLSCTX_ flags, so strip off the new bits
            // before we call on that platform.

            if (!IsOS(OS_NT5))
            {
                // BUGBUG: when we go to nt6 make sure || == OS_NT6 is added
                // these constants don't work w/ IsOS() >= OS_NT5 because OS_MEMPHIS > OS_NT5
                // and no code download is not supported there.
                dwCoCreateFlags &= ~CLSCTX_NO_CODE_DOWNLOAD;
            }
        
            hres = THR(CoCreateInstance(pclsid, punkOuter, dwCoCreateFlags, riid, ppv));

            if ((hres == CO_E_NOTINITIALIZED) && bLoadWithoutCOM)
            {
                hres = THR(_CreateFromDll(szDllPath, pclsid, punkOuter, riid, ppv));
            }

            // only RIP if this is not a secondary explorer process since secondary explorers dont init com or ole since
            // they are going to delegate to an existing process and we don't want to have to load ole for perf in that case.
            if (!IsSecondaryExplorerProcess())
            {
                RIPMSG((hres != CO_E_NOTINITIALIZED), "COM not inited for dll %s", szDllPath);
            }

            //  sometimes we need to permanently pin these objects.
            if (SUCCEEDED(hres) && (OBJCOMPATF_PINDLL & SHGetObjectCompatFlags(NULL, (CLSID *)pclsid)))
                SHPinDllOfCLSID(pclsid);
        }
    }
    else
    {
        // try shell32 for improperly registered objects
        hres = THR(_CreateFromShell(pclsid, punkOuter, riid, ppv));

#ifdef DEBUG
        //
        // check to see if we're the explorer process before complaining (to
        // avoid ripping during setup before all objects have been registered)
        //
        if (IsProcessAnExplorer() && !IsGuimodeSetupRunning())
        {
            ASSERTMSG(FAILED(hres), "object not registered (add to selfreg.inx) %s", szClass);
        }
#endif
    }

#ifdef DEBUG
    if (FAILED(hres) && (hres != E_NOINTERFACE))    // E_NOINTERFACE means riid not accepted
    {
        DWORD dwTF = IsFlagSet(g_dwBreakFlags, BF_COCREATEINSTANCE) ? TF_ALWAYS : TF_WARNING;
        TraceMsg(dwTF, "CoCreateInstance: failed (%s,%x)", szClass, hres);
    }
#endif
    return hres;
}

STDAPI SHCoCreateInstance(LPCTSTR pszCLSID, const CLSID * pclsid, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    CLSID clsid;
    if (pszCLSID)
    {
        SHCLSIDFromString(pszCLSID, &clsid);
        pclsid = &clsid;
    }
    return _SHCoCreateInstance(pclsid, punkOuter, CLSCTX_INPROC_SERVER, FALSE, riid, ppv);
}

//
// create a shell extension object, ensures that object is in the approved list
//
STDAPI SHExtCoCreateInstance(LPCTSTR pszCLSID, const CLSID *pclsid, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    CLSID clsid;
    
    if (pszCLSID)
    {
        SHCLSIDFromString(pszCLSID, &clsid);
        pclsid = &clsid;
    }

    return _SHCoCreateInstance(pclsid, punkOuter, CLSCTX_NO_CODE_DOWNLOAD | CLSCTX_INPROC_SERVER, TRUE, riid, ppv);
}

STDAPI_(BOOL) SHIsBadInterfacePtr(const void *pv, UINT cbVtbl)
{
    IUnknown const * punk = pv;
    return IsBadReadPtr(punk, SIZEOF(punk->lpVtbl)) || 
           IsBadReadPtr(punk->lpVtbl, cbVtbl) || 
           IsBadCodePtr((FARPROC)punk->lpVtbl->Release);
}

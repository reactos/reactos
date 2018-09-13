#include "stdafx.h"
#include <initguid.h>
#include "stobject.h"
#include "cfact.h"

// One lock for each running component + one lock per LockServer call
long g_cLocks = 0;
HINSTANCE g_hinstDll = NULL;
const TCHAR g_szThreadModel[] = TEXT("Both");

STDAPI DllCanUnloadNow()
{
    return (g_cLocks == 0);
}

STDAPI DllGetClassObject(const CLSID& clsid, const IID& iid, void** ppvObject)
{
    HRESULT hr = S_OK;
    BOOL fRunTrayOnConstruct;
    *ppvObject = NULL;

    if (clsid == CLSID_SysTray)
    {
        // The SysTray object is being requested - we don't actually launch the tray thread until
        // told to do so though IOleCommandTarget
        fRunTrayOnConstruct = FALSE;
    }
    else if (clsid == CLSID_SysTrayInvoker)
    {
        // The simple invoker object is being requested - the tray thread will be launched immediately
        fRunTrayOnConstruct = TRUE;
    }
    else
    {
        // We don't support this object!
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }

    // If one of the two objects we support was requested:
    if (SUCCEEDED(hr))
    {
        // Try to create the object
        CSysTrayFactory* ptrayfact = new CSysTrayFactory(fRunTrayOnConstruct);

        if (ptrayfact != NULL)
        {
            hr = ptrayfact->QueryInterface(iid, ppvObject);
            ptrayfact->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


BOOL RegisterComponent(const CLSID& clsid, const TCHAR* szProgID)
{
    // Build a CLSID string for the registry
    BOOL fSuccess = FALSE;
    TCHAR szSubkey[MAX_PATH];
    TCHAR szCLSID[GUIDSTR_MAX];
    TCHAR szModule[MAX_PATH];
    HKEY hkeyCLSID = NULL;
    HKEY hkeyInproc = NULL;
    DWORD dwDisp;
    TCHAR* pszNameOnly;

    // Try and get all the strings we need
    if (StringFromGUID2(clsid, szCLSID, ARRAYSIZE(szCLSID)) != 0)
    {
        if ((GetModuleFileName(g_hinstDll, szModule, ARRAYSIZE(szModule)) != 0) &&
            (pszNameOnly = PathFindFileName(szModule)))
        {
            if (wnsprintf(szSubkey, ARRAYSIZE(szSubkey), TEXT("CLSID\\%s"), szCLSID) > 0)
            {
                // We've built our strings, so write stuff to the registry
                if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT, szSubkey, 0, 
                    NULL, 0, KEY_WRITE, NULL, &hkeyCLSID, &dwDisp))
                {

                    RegSetValueEx(hkeyCLSID, NULL, 0, REG_SZ, (const BYTE*) szProgID, 
                        (lstrlen(szProgID) + 1) * sizeof(TCHAR));

                    if (ERROR_SUCCESS == RegCreateKeyEx(hkeyCLSID, TEXT("InprocServer32"), 
                        0, NULL, 0, KEY_SET_VALUE, NULL, &hkeyInproc, &dwDisp))
                    {

                        RegSetValueEx(hkeyInproc, NULL, 0, REG_SZ, 
                            (const BYTE*) pszNameOnly, (lstrlen(pszNameOnly) + 1) * sizeof(TCHAR));
                        RegSetValueEx(hkeyInproc, TEXT("ThreadingModel"), 0, REG_SZ, 
                            (const BYTE*) g_szThreadModel, sizeof(g_szThreadModel));
                        fSuccess = TRUE;
                    }
                }
            }
        }
    }

    if (hkeyCLSID != NULL)
        RegCloseKey(hkeyCLSID);

    if (hkeyInproc != NULL)
        RegCloseKey(hkeyInproc);

    return fSuccess;
}

BOOL UnregisterComponent(const CLSID& clsid)
{
    // Build a CLSID string for the registry
    BOOL fSuccess = FALSE;
    TCHAR szSubkey[MAX_PATH];
    TCHAR szCLSID[GUIDSTR_MAX];
    HKEY hkeyCLSID = NULL;

    // Try and get all the strings we need
    if (StringFromGUID2(clsid, szCLSID, ARRAYSIZE(szCLSID)) != 0)
    {
        if (wnsprintf(szSubkey, ARRAYSIZE(szSubkey), TEXT("CLSID\\%s"), szCLSID) > 0)
        {        
            // We've built our strings, so delete our registry stuff
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, szSubkey, 0, 
                KEY_WRITE, &hkeyCLSID))
            {
                RegDeleteKey(hkeyCLSID, TEXT("InprocServer32"));
                RegCloseKey(hkeyCLSID);
                hkeyCLSID = NULL;

                RegDeleteKey(HKEY_CLASSES_ROOT, szSubkey);
                fSuccess = TRUE;
            }
        }
    }
    if (hkeyCLSID != NULL)
        RegCloseKey(hkeyCLSID);
    
    return fSuccess;
}

BOOL RegisterShellServiceObject(const CLSID& clsid, const TCHAR* szProgID, BOOL fRegister)
{
    const static TCHAR szSubkey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad");
    BOOL fSuccess = FALSE;
    TCHAR szCLSID[GUIDSTR_MAX];
    HKEY hkey = NULL;

    // Try and get all the strings we need
    if (StringFromGUID2(clsid, szCLSID, ARRAYSIZE(szCLSID)) != 0)
    {

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubkey, 0, 
            KEY_WRITE, &hkey))
        {
            if (fRegister)
            {
                fSuccess = RegSetValueEx(hkey, szProgID, 0, REG_SZ, (const BYTE*) szCLSID,
                    (lstrlen(szCLSID) + 1) * sizeof(TCHAR));
            }
            else
            {
                fSuccess = RegDeleteValue(hkey, szProgID);
            }
        }
    }

    if (hkey != NULL)
        RegCloseKey(hkey);
    
    return fSuccess;    
}

STDAPI DllRegisterServer()
{
    BOOL fSuccess;
    fSuccess = RegisterComponent(CLSID_SysTray, TEXT("SysTray"));
    fSuccess &= RegisterComponent(CLSID_SysTrayInvoker, TEXT("SysTrayInvoker"));
    fSuccess &= RegisterShellServiceObject(CLSID_SysTray, TEXT("SysTray"), TRUE);
    return fSuccess;
}

STDAPI DllUnregisterServer()
{
    BOOL fSuccess;
    fSuccess = UnregisterComponent(CLSID_SysTray);
    fSuccess &= UnregisterComponent(CLSID_SysTrayInvoker);
    fSuccess &= RegisterShellServiceObject(CLSID_SysTray, TEXT("SysTray"), FALSE);
    return fSuccess;
}

STDAPI DllMain(HINSTANCE hModule, DWORD dwReason, void* lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // Don't have DllMain called for thread init's.
        DisableThreadLibraryCalls(hModule);
        g_hinstDll = hModule;
    }

    return TRUE;
}

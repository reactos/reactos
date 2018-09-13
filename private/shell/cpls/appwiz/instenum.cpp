//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: instenum.cpp
//
// The current order of enumeration is Legacy --> Darwin --> SMS
//
// History:
//         1-18-97  by dli
//------------------------------------------------------------------------
#include "priv.h"
#include "instenum.h"
#include "instapp.h"
#include "sccls.h"



// constructor
CEnumInstalledApps::CEnumInstalledApps(void) : _cRef(1), _bEnumLegacy(TRUE) //_bEnumDarwin(FALSE)
{
    DllAddRef();

    TraceAddRef(CEnumInstalledApps, _cRef);
    
    // Start off enumerating legacy apps, then switch to
    // enumerating darwin apps.
    ASSERT(_hkeyUninstall == NULL);
}


// destructor
CEnumInstalledApps::~CEnumInstalledApps()
{
    if (_hkeyUninstall)
    {
        RegCloseKey(_hkeyUninstall);
        _hkeyUninstall = NULL;
    }

    DllRelease();
}


// IEnumInstalledApps::QueryInterface
HRESULT CEnumInstalledApps::QueryInterface(REFIID riid, LPVOID * ppvOut)
{
    static const QITAB qit[] = {
        QITABENT(CEnumInstalledApps, IEnumInstalledApps),                  // IID_IEnumInstalledApps
        { 0 },
    };

    return QISearch(this, qit, riid, ppvOut);
}

// IUnknown::AddRef
ULONG CEnumInstalledApps::AddRef()
{
    _cRef++;
    TraceAddRef(CEnumInstalledApps, _cRef);
    return _cRef;
}

// IUnknown::Release
ULONG CEnumInstalledApps::Release()
{
    _cRef--;
    TraceRelease(CEnumInstalledApps, _cRef);
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

#define REGSTR_VAL_UNINSTALLER_WINDOWSINSTALLER TEXT("WindowsInstaller")
#define REGSTR_VAL_UNINSTALLER_SYSTEMCOMPONENT TEXT("SystemComponent")


HRESULT CEnumInstalledApps::_GetNextLegacyAppFromRegistry(IInstalledApp ** ppia)
{
    HRESULT hres = S_FALSE;
    LONG lRet;
    HKEY hkeySub = NULL;
    TCHAR szKeyName[MAX_PATH];
    DWORD dwType;
    BOOL bTryAgain;

    do
    {
        ULONG cchKeyName = ARRAYSIZE(szKeyName);
        FILETIME ftLast;

        bTryAgain = FALSE;
        
        // Start enumerationg subkeys under _hkeyUninstall
        if (RegEnumKeyEx(_hkeyUninstall, _iIndexEach, szKeyName, &cchKeyName, NULL,
                         NULL, NULL, &ftLast) == ERROR_SUCCESS)
        {
            _iIndexEach++;

            // Open the key and get the subkey name
            lRet = RegOpenKeyEx(_hkeyUninstall, szKeyName, 0, KEY_READ, &hkeySub);
            if (lRet == ERROR_SUCCESS)
            {
                TCHAR szProduct[MAX_PATH];

                // Don't enumerate system components 
                DWORD dwSysComponent = 0;
                DWORD cbSysComponent = SIZEOF(dwSysComponent);
                lRet = SHQueryValueEx(hkeySub, REGSTR_VAL_UNINSTALLER_SYSTEMCOMPONENT, 0, &dwType,
                                       (PBYTE)&dwSysComponent, &cbSysComponent); 
                if ((lRet != ERROR_SUCCESS) || (dwSysComponent != 1))
                {
                    // Don't enumerate Darwin apps, who has WindowsInstaller set to 1
                    ULONG uDarwin;
                    ULONG cbDarwin = SIZEOF(uDarwin);
                    lRet = SHQueryValueEx(hkeySub, REGSTR_VAL_UNINSTALLER_WINDOWSINSTALLER, 0, &dwType,
                                          (PBYTE)&uDarwin, &cbDarwin); 
                    if ((lRet != ERROR_SUCCESS) || (uDarwin != 1))
                    {

                        // Get the DisplayName value
                        ULONG cbProductName = SIZEOF(szProduct);
                        lRet = SHQueryValueEx(hkeySub, REGSTR_VAL_UNINSTALLER_DISPLAYNAME, 0, &dwType,
                                              (PBYTE)szProduct, &cbProductName); 
                        if (lRet == ERROR_SUCCESS)
                        {
                            TCHAR szUninstall[MAX_INFO_STRING];

                            // we proceed even if the below SHQueryValueEx fails, so we need
                            // to zero initialize
                            szUninstall[0] = 0;

                            // Get the uninstaller string
                            ULONG cbUninstall = SIZEOF(szUninstall);
                            lRet = SHQueryValueEx(hkeySub, REGSTR_VAL_UNINSTALLER_COMMANDLINE, 0, &dwType, (PBYTE)szUninstall, &cbUninstall);

                            // NOTE: We don't create CInstalledApp Object if there is no "Uninstall" key 
                            // should we just delete this from registry?
                            if (lRet == ERROR_SUCCESS)
                            {
                                // Create new CInstalledApp Object 
                                CInstalledApp * pia = new CInstalledApp(hkeySub, szKeyName, szProduct, szUninstall, _bEnumHKCU);
                                if (pia)
                                {
                                    *ppia = SAFECAST(pia, IInstalledApp *);
                                    hres = S_OK;
                                }
                                else
                                    hres = E_OUTOFMEMORY;
                                break;  // We found an app, return
                            }
                        }
                    }
                }
                
                // In failure cases, go to the next one and try again
                RegCloseKey(hkeySub);
                bTryAgain = TRUE;
                continue;
                
                // (hkeySub is owned and closed by the CInstalledApp object)
            }
        }
        else
        {
            RegCloseKey(_hkeyUninstall);
            _hkeyUninstall = NULL;
        }
    } while (bTryAgain);

    return hres;
}

// Gets the next legacy app from the registry "uninstall" key 
HRESULT CEnumInstalledApps::_GetNextLegacyApp(IInstalledApp ** ppia)
{
    HRESULT hres = S_FALSE;

    // There are three possible cases
    // 1. In the very beginning, _hkeyUninstall == NULL, _bEnumHKCU == FALSE
    //    we open up HKLM\\...\\Uninstall to enumerate
    if ((_hkeyUninstall == NULL) && (_bEnumHKCU != TRUE))
    {
        if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_UNINSTALL, 0, KEY_READ, &_hkeyUninstall))
        {
            // There is no apps in HKLM, let's try HKCU
            goto TRYHKCU;
        }
    }

    if (_hkeyUninstall)
    {
        // Enumerate the next one
        hres = _GetNextLegacyAppFromRegistry(ppia);

        // 2. When we are done with HKLM, _hkeyUninstall == NULL, _bEnumHKCU == FALSE
        //    we open up HKCU\\...\\Uninstall to enumerate
        if (hres == S_FALSE)
        {
TRYHKCU:
            if (_bEnumHKCU != TRUE)
            {
                _bEnumHKCU = TRUE;
                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_UNINSTALL, 0, KEY_READ, &_hkeyUninstall))
                {
                    _iIndexEach = 0;
                    hres = _GetNextLegacyAppFromRegistry(ppia);
                }
            }
        }
    }

    // 3. When we are done with HKCU, _hkeyUninstall == NULL, but _bEnumHKCU == TRUE
    //    we will not go on enumerating in this case.
    
    return hres;
}


HRESULT CEnumInstalledApps::_GetNextDarwinApp(IInstalledApp ** ppia)
{
    HRESULT hres = S_FALSE;
    TCHAR szProductID[GUIDSTR_MAX];

    BOOL bContinue;
    do
    {
        bContinue = FALSE;
        
        UINT uRet = MsiEnumProducts(_iIndexEach, szProductID);
        if (uRet == ERROR_SUCCESS)
        {
            BOOL bTake = TRUE; // Do we want to show this app, default to yes. 
            _iIndexEach++; // increment the counter
            
            HKEY hkeySub = NULL;
            DWORD dwType;
            TCHAR szRegKey[MAX_PATH];
            wnsprintf(szRegKey, ARRAYSIZE(szRegKey), TEXT("%s\\%s"), REGSTR_PATH_UNINSTALL, szProductID);

            // Open this key in the registry
            uRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegKey, 0, KEY_READ, &hkeySub);
            if (uRet == ERROR_SUCCESS)
            {
                // Don't enumerate system components 
                DWORD dwSysComponent = 0;
                DWORD cbSysComponent = SIZEOF(dwSysComponent);
                uRet = SHQueryValueEx(hkeySub, REGSTR_VAL_UNINSTALLER_SYSTEMCOMPONENT, 0, &dwType,
                                      (PBYTE)&dwSysComponent, &cbSysComponent);
                if ((uRet == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwSysComponent == 1))
                    bTake = FALSE;

                RegCloseKey(hkeySub);
            }

            if (bTake)
            {
                INSTALLSTATE is = MsiQueryProductState(szProductID);

                if ((is != INSTALLSTATE_DEFAULT) && (is != INSTALLSTATE_ADVERTISED))
                    bTake = FALSE;

                // NOTE: INSTALLSTATE_ADVERTISED means assigned apps
                if (bTake)
                {
                    CInstalledApp * pia = new CInstalledApp(szProductID);
                    if (pia)
                    {
                        *ppia = SAFECAST(pia, IInstalledApp *);
                        hres = S_OK;
                    }
                    else
                        hres = E_OUTOFMEMORY;
                    break;
                }
            } 
            bContinue = TRUE;
        }
        else if (uRet != ERROR_NO_MORE_ITEMS)
        {
            _iIndexEach++;
            bContinue = TRUE;
        }
        
    } while (bContinue);
    
    return hres;
}


// IEnumInstalledApps::Next
// We allow only one app at a time. 
STDMETHODIMP CEnumInstalledApps::Next(IInstalledApp ** ppia)
{
    HRESULT hres = S_FALSE;
    if (_bEnumLegacy)
    {
        hres = _GetNextLegacyApp(ppia);
        if (hres == S_FALSE)
        {
            // End of the enumeration for legacy apps
            _bEnumLegacy = FALSE;
            _iIndexEach = 0;
            goto EnumDarwinNow;
        }
    }
    else
    {    
EnumDarwinNow:
        hres = _GetNextDarwinApp(ppia);
    }
    
    return hres;
}

// IEnumInstalledApps::Reset
STDMETHODIMP CEnumInstalledApps::Reset(void)
{
    // Start off enumerating legacy apps, then switch to
    // enumerating darwin apps.
    _bEnumLegacy = TRUE;
    _iIndexEach = 0;

    return S_OK;
}


/*----------------------------------------------------------
Purpose: Create-instance function for class factory

*/
STDAPI CEnumInstalledApps_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    HRESULT hres = E_OUTOFMEMORY;
    CEnumInstalledApps * pObj = new CEnumInstalledApps();
    if (pObj)
    {
        *ppunk = SAFECAST(pObj, IEnumInstalledApps *);
        hres = S_OK;
    }

    return hres;
}

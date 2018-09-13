#include "deskcmmn.h"
#include <regstr.h>


static const TCHAR sc_szDeskAppletSoftwareKey[] = REGSTR_PATH_CONTROLSFOLDER TEXT("\\Display");



LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan)
    {
    int i;
    for (i = 0; pszScan[i] != TEXT('\0') && pszTarget[i] != TEXT('\0') &&
            CharUpper(CHARTOPSZ(pszScan[i])) ==
            CharUpper(CHARTOPSZ(pszTarget[i])); i++);

    if (pszTarget[i] == TEXT('\0')) 
        {
        // we found the substring
        return pszScan + i;
        }

    return pszScan;
    }


BOOL GetDeviceRegKey(LPCTSTR pstrDeviceKey, HKEY* phKey, BOOL* pbReadOnly)
    {
    //ASSERT(lstrlen(pstrDeviceKey) < MAX_PATH);
    
    if(lstrlen(pstrDeviceKey) >= MAX_PATH)
        return FALSE;
        
    BOOL bRet = FALSE;

    // copy to local string
    TCHAR szBuffer[MAX_PATH];
    lstrcpy(szBuffer, pstrDeviceKey);

    //
    // At this point, szBuffer has something like:
    //  \REGISTRY\Machine\System\ControlSet001\Services\Jazzg300\Device0
    //
    // To use the Win32 registry calls, we have to strip off the \REGISTRY
    // and convert \Machine to HKEY_LOCAL_MACHINE
    //

    LPTSTR pszRegistryPath = SubStrEnd(SZ_REGISTRYMACHINE, szBuffer);
    
    if(pszRegistryPath)
        {
        // Open the registry key
        bRet = (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             pszRegistryPath,
                             0,
                             KEY_ALL_ACCESS,
                             phKey) == ERROR_SUCCESS);
        if(bRet)
            {
            *pbReadOnly = FALSE;
            }
        else
            {
            bRet = (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                 pszRegistryPath,
                                 0, 
                                 KEY_READ, 
                                 phKey) == ERROR_SUCCESS);
            if (bRet)
                {
                *pbReadOnly = TRUE;
                }
            }
        }

    return bRet;
    }


int GetDisplayCPLPreference(LPCTSTR szRegVal)
{
    int val = -1;
    HKEY hk;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, sc_szDeskAppletSoftwareKey, 0, KEY_READ, &hk) == ERROR_SUCCESS)
    {
        TCHAR sz[64];
        DWORD cb = sizeof(sz);

        *sz = 0;
        if ((RegQueryValueEx(hk, szRegVal, NULL, NULL,
            (LPBYTE)sz, &cb) == ERROR_SUCCESS) && *sz)
        {
            val = (int)MyStrToLong(sz);
        }

        RegCloseKey(hk);
    }

    if (val == -1 && RegOpenKeyEx(HKEY_LOCAL_MACHINE, sc_szDeskAppletSoftwareKey, 0, KEY_READ, &hk) == ERROR_SUCCESS)
    {
        TCHAR sz[64];
        DWORD cb = sizeof(sz);

        *sz = 0;
        if ((RegQueryValueEx(hk, szRegVal, NULL, NULL,
            (LPBYTE)sz, &cb) == ERROR_SUCCESS) && *sz)
        {
            val = (int)MyStrToLong(sz);
        }

        RegCloseKey(hk);
    }

    return val;
}


int GetDynaCDSPreference()
{
//DLI: until we figure out if this command line stuff is still needed. 
//    if (g_fCommandLineModeSet)
//        return DCDSF_YES;

    int iRegVal = GetDisplayCPLPreference(REGSTR_VAL_DYNASETTINGSCHANGE);
    if (iRegVal == -1)
        iRegVal = DCDSF_DYNA; // Apply dynamically
    return iRegVal;
}


void SetDisplayCPLPreference(LPCTSTR szRegVal, int val)
{
    HKEY hk;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, sc_szDeskAppletSoftwareKey, 0, TEXT(""), 0, KEY_WRITE, NULL, &hk, NULL) ==
        ERROR_SUCCESS)
    {
        TCHAR sz[64];

        wsprintf(sz, TEXT("%d"), val);
        RegSetValueEx(hk, szRegVal, NULL, REG_SZ,
            (LPBYTE)sz, lstrlen(sz) + 1);

        RegCloseKey(hk);
    }
}


LONG WINAPI MyStrToLong(LPCTSTR sz)
{
    long l=0;

    while (*sz >= TEXT('0') && *sz <= TEXT('9'))
        l = l*10 + (*sz++ - TEXT('0'));

    return l;
}



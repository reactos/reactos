/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       REGHELP.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <string.h>
#include <regstr.h>
#include <commctrl.h>

#include <ntpoapi.h>

#include "powrprof.h"

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

extern HINSTANCE   g_hInstance;        // Global instance handle of this DLL.
extern HANDLE      g_hSemRegistry;     // Registry semaphore.
extern UINT        g_uiLastID;         // The last ID value used, per machine.

extern TCHAR c_szREGSTR_PATH_MACHINE_POWERCFG[];
extern TCHAR c_szREGSTR_VAL_LASTID[];


/*******************************************************************************
*
*  OpenCurrentUser
*
*  DESCRIPTION:
*   
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN OpenCurrentUser(PHKEY phKey)
{
#ifdef WINNT

    // Since powerprof can be called in the Winlogon context when
    // a user is being impersonated, use RegOpenCurrentUser to get HKCU.
    LONG lRet;

    if ((lRet = RegOpenCurrentUser(KEY_ALL_ACCESS, phKey)) == ERROR_SUCCESS) {
        return TRUE;
    }
    DebugPrint("RegOpenCurrentUser, failed, LastError: 0x%08X", lRet);
    return FALSE;

#else
    *phKey = HKEY_CURRENT_USER;
    return TRUE; 
#endif
}

/*******************************************************************************
*
*  CloseCurrentUser
*
*  DESCRIPTION:
*   
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN CloseCurrentUser(HKEY hKey)
{
#ifdef WINNT
    RegCloseKey(hKey);
#endif
    return TRUE;
}

/*******************************************************************************
*
*  OpenMachineUserKeys
*
*  DESCRIPTION:
*   
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN OpenMachineUserKeys(
    LPTSTR  lpszUserKeyName,
    LPTSTR  lpszMachineKeyName,
    PHKEY   phKeyUser,
    PHKEY   phKeyMachine
)
{
    BOOLEAN  bRet = FALSE;
    LONG     lRet;
    HKEY     hKeyCurrentUser;

    if (OpenCurrentUser(&hKeyCurrentUser)) {
        if ((lRet = RegOpenKey(hKeyCurrentUser,
                               lpszUserKeyName,
                               phKeyUser)) == ERROR_SUCCESS) {
    
            if ((lRet = RegOpenKey(HKEY_LOCAL_MACHINE,
                                   lpszMachineKeyName,
                                   phKeyMachine)) == ERROR_SUCCESS) {
                
                CloseCurrentUser(hKeyCurrentUser);
                return TRUE;
            }
            else {
                DebugPrint("OpenMachineUserKeys, failure opening  HKEY_LOCAL_MACHINE\\%s", lpszMachineKeyName);
            }
            RegCloseKey(*phKeyUser);
        }
        else {
            DebugPrint("OpenMachineUserKeys, failure opening HKEY_CURRENT_USER\\%s", lpszUserKeyName);
        }
        CloseCurrentUser(hKeyCurrentUser);
    }
    
    if (lRet == ERROR_SUCCESS) {
        lRet = GetLastError();
    }
    else {
        SetLastError(lRet);
    }
    DebugPrint("OpenMachineUserKeys, failed, LastError: 0x%08X", lRet);
    return FALSE;
}

/*******************************************************************************
*
*  OpenPathKeys
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN OpenPathKeys(
    LPTSTR  lpszUserKeyName,
    LPTSTR  lpszMachineKeyName,
    LPTSTR  lpszSchemeName,
    PHKEY   phKeyUser,
    PHKEY   phKeyMachine,
    BOOLEAN bMustExist,
    REGSAM  sam
)
{
    HKEY     hKeyUser, hKeyMachine;
    BOOLEAN  bRet = FALSE;
    LONG     lRet;
    DWORD    dwDisposition, dwDescSize;

    if (!OpenMachineUserKeys(lpszUserKeyName, lpszMachineKeyName,
                             &hKeyUser, &hKeyMachine)) {
        return FALSE;
    }

    if ((lRet = RegCreateKeyEx(hKeyUser,
                               lpszSchemeName,
                               0,
                               TEXT(""),
                               REG_OPTION_NON_VOLATILE,
                               sam,
                               NULL,
                               phKeyUser,
                               &dwDisposition)) == ERROR_SUCCESS) {

        if (!bMustExist || (dwDisposition == REG_OPENED_EXISTING_KEY)) {

            if ((lRet = RegCreateKeyEx(hKeyMachine,
                                       lpszSchemeName,
                                       0,
                                       TEXT(""),
                                       REG_OPTION_NON_VOLATILE,
                                       sam,
                                       NULL,
                                       phKeyMachine,
                                       &dwDisposition)) == ERROR_SUCCESS) {

                if (!bMustExist ||
                    (dwDisposition == REG_OPENED_EXISTING_KEY)) {
                    bRet = TRUE;
                }
            }
            else {
               RegCloseKey(*phKeyUser);
               DebugPrint("OpenPathKeys, unable to create machine key %s\\%s", lpszMachineKeyName, lpszSchemeName);
            }
        }
    }
    else {
       DebugPrint("OpenPathKeys, unable to create user key %s\\%s", lpszUserKeyName, lpszSchemeName);
    }
    RegCloseKey(hKeyUser);
    RegCloseKey(hKeyMachine);

    if (!bRet) {
        if (lRet == ERROR_SUCCESS) {
            lRet = GetLastError();
        }
        else {
            SetLastError(lRet);
        }
        DebugPrint("OpenPathKeys, failed, LastError: 0x%08X", lRet);
    }
    return bRet;
}

/*******************************************************************************
*
*  TakeRegSemaphore
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN TakeRegSemaphore(VOID)
{
    if (WaitForSingleObject(g_hSemRegistry, SEMAPHORE_TIMEOUT) != WAIT_OBJECT_0) {
        ReleaseSemaphore(g_hSemRegistry, 1, NULL);
        DebugPrint("WaitForSingleObject, failed");
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    return TRUE;
}

/*******************************************************************************
*
*  ReadPowerValueOptional
*
*  DESCRIPTION:
*   Value may not exist.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ReadPowerValueOptional(
    HKEY    hKey,
    LPTSTR  lpszPath,
    LPTSTR  lpszValueName,
    LPTSTR  lpszValue,
    LPDWORD lpdwSize
)
{
    HKEY     hKeyPath;
    BOOLEAN  bRet = FALSE;
    DWORD    dwSize;
    LONG     lRet;

    if ((lRet = RegOpenKey(hKey,
                           lpszPath,
                           &hKeyPath)) != ERROR_SUCCESS) {
        goto RPVO_exit;
    }

    if ((lRet = RegQueryValueEx(hKeyPath,
                                lpszValueName,
                                NULL,
                                NULL,
                                (PBYTE) lpszValue,
                                lpdwSize)) == ERROR_SUCCESS) {
            bRet = TRUE;
    }

    RegCloseKey(hKeyPath);

RPVO_exit:
    return bRet;
}

/*******************************************************************************
*
*  ReadPowerIntOptional
*
*  DESCRIPTION:
*   Integer value may not exist.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ReadPowerIntOptional(
    HKEY    hKey,
    LPTSTR  lpszPath,
    LPTSTR  lpszValueName,
    PINT    piVal
)
{
    HKEY     hKeyPath;
    BOOLEAN  bRet = FALSE;
    DWORD    dwSize;
    TCHAR    szNum[NUM_DEC_DIGITS];
    LONG     lRet;

    if ((lRet = RegOpenKey(hKey,
                           lpszPath,
                           &hKeyPath)) != ERROR_SUCCESS) {
        goto RPVO_exit;
    }

    dwSize = sizeof(szNum);
    if ((lRet = RegQueryValueEx(hKeyPath,
                                lpszValueName,
                                NULL,
                                NULL,
                                (PBYTE) szNum,
                                &dwSize)) == ERROR_SUCCESS) {
        if (MyStrToInt(szNum, piVal)) {
            bRet = TRUE;
        }
    }

    RegCloseKey(hKeyPath);

RPVO_exit:
    return bRet;
}

/*******************************************************************************
*
*  CreatePowerValue
*
*  DESCRIPTION:
*   Value may not exist.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN CreatePowerValue(
    HKEY    hKey,
    LPCTSTR  lpszPath,
    LPCTSTR  lpszValueName,
    LPCTSTR  lpszValue
)
{
   DWORD    dwDisposition, dwDescSize;
   HKEY     hKeyPath;
   BOOLEAN  bRet = FALSE;
   DWORD    dwSize;
   LONG     lRet;
    
    // Wait on/take the registry semaphore.
    if (!TakeRegSemaphore()) {
        return FALSE;
    }

    if ((lRet = RegCreateKeyEx(hKey,
                               lpszPath,
                               0,
                               TEXT(""),
                               REG_OPTION_NON_VOLATILE,
                               KEY_WRITE,
                               NULL,
                               &hKeyPath,
                               &dwDisposition)) == ERROR_SUCCESS) {

       if (lpszValue) {
           dwSize = (lstrlen(lpszValue) + 1) * sizeof(TCHAR);
           if ((lRet = RegSetValueEx(hKeyPath,
                                     lpszValueName,
                                     0,
                                     REG_SZ,
                                     (PBYTE) lpszValue,
                                     dwSize)) == ERROR_SUCCESS) {
               bRet = TRUE;
           }
       }
       else {
           SetLastError(ERROR_INVALID_PARAMETER);
       }
       RegCloseKey(hKeyPath);
    }

    ReleaseSemaphore(g_hSemRegistry, 1, NULL);
    return bRet;
}


/*******************************************************************************
*
*  ReadWritePowerValue
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ReadWritePowerValue(
    HKEY    hKey,
    LPTSTR  lpszPath,
    LPTSTR  lpszValueName,
    LPTSTR  lpszValue,
    LPDWORD lpdwSize,
    BOOLEAN bWrite,
    BOOLEAN bTakeSemaphore
)
{
    HKEY     hKeyPath;
    BOOLEAN  bRet = FALSE;
    DWORD    dwSize;
    LONG     lRet;

    if ((lRet = RegOpenKey(hKey,
                           lpszPath,
                           &hKeyPath)) != ERROR_SUCCESS) {
        goto RWPV_exit;
    }

    // Wait on/take the registry semaphore.
    if (bTakeSemaphore) {
        if (!TakeRegSemaphore()) {
            return FALSE;
        }
    }

    if (bWrite) {
        // Write current case.
        if (lpszValue) {
            dwSize = (lstrlen(lpszValue) + 1) * sizeof(TCHAR);
            if ((lRet = RegSetValueEx(hKeyPath,
                                      lpszValueName,
                                      0,
                                      REG_SZ,
                                      (PBYTE) lpszValue,
                                      dwSize)) == ERROR_SUCCESS) {
                bRet = TRUE;
            }
        }
        else {
            SetLastError(ERROR_INVALID_PARAMETER);
        }
    }
    else {
        // Read current case.
        if ((lRet = RegQueryValueEx(hKeyPath,
                                    lpszValueName,
                                    NULL,
                                    NULL,
                                    (PBYTE) lpszValue,
                                    lpdwSize)) == ERROR_SUCCESS) {
            bRet = TRUE;
        }
    }

    if (bTakeSemaphore) {
        ReleaseSemaphore(g_hSemRegistry, 1, NULL);
    }
    RegCloseKey(hKeyPath);

RWPV_exit:
    if (!bRet) {
        if (lRet != ERROR_SUCCESS) {
            SetLastError(lRet);
        }
        else {
            lRet = GetLastError();
        }

        // Access denied is a valid result. 
        if (lRet != ERROR_ACCESS_DENIED) {
            DebugPrint("ReadWritePowerValue, failed, lpszValueName: %s, LastError: 0x%08X", lpszValueName, lRet);
        }
    }
    return bRet;
}

/*******************************************************************************
*
*  ReadPwrPolicyEx
*
*  DESCRIPTION:
*   Supports ReadPwrScheme and ReadGlobalPwrPolicy
*
*  PARAMETERS:
*   lpdwDescSize - Pointer to size of optional description buffer.
*   lpszDesc     - Optional description buffer.
*
*******************************************************************************/

BOOLEAN ReadPwrPolicyEx(
    LPTSTR  lpszUserKeyName,
    LPTSTR  lpszMachineKeyName,
    LPTSTR  lpszSchemeName,
    LPTSTR  lpszDesc,
    LPDWORD lpdwDescSize,
    LPVOID  lpvUser,
    DWORD   dwcbUserSize,
    LPVOID  lpvMachine,
    DWORD   dwcbMachineSize
)
{
    HKEY     hKeyUser, hKeyMachine;
    DWORD    dwType, dwSize;
    BOOLEAN  bRet = FALSE;
    LONG     lRet;

    if ((!lpszUserKeyName || !lpszMachineKeyName) ||
        (!lpszSchemeName  || !lpvUser || !lpvMachine) ||
        (!lpdwDescSize    && lpszDesc) ||
        (lpdwDescSize     && !lpszDesc)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto RPPE_exit;
    }

    // Wait on/take the registry semaphore.
    if (!TakeRegSemaphore()) {
        return FALSE;
    }

    if (!OpenPathKeys(lpszUserKeyName, lpszMachineKeyName, lpszSchemeName,
                      &hKeyUser, &hKeyMachine, TRUE, KEY_READ)) {
        ReleaseSemaphore(g_hSemRegistry, 1, NULL);
        return FALSE;
    }

    dwSize = dwcbUserSize;
    lRet = RegQueryValueEx(hKeyUser,
                           TEXT("Policies"),
                           NULL,
                           &dwType,
                           (PBYTE) lpvUser,
                           &dwSize);

    if ((lRet == ERROR_SUCCESS) && (dwType == REG_BINARY)) {
        dwSize = dwcbMachineSize;
        lRet = RegQueryValueEx(hKeyMachine,
                               TEXT("Policies"),
                               NULL,
                               &dwType,
                               (PBYTE) lpvMachine,
                               &dwSize);
    }

    if (lRet == ERROR_SUCCESS) {
        if (dwType == REG_BINARY) {
            if (lpdwDescSize) {
                if ((lRet = RegQueryValueEx(hKeyUser,
                                            TEXT("Description"),
                                            NULL,
                                            &dwType,
                                            (PBYTE) lpszDesc,
                                            lpdwDescSize)) == ERROR_SUCCESS) {
                    bRet = TRUE;
                }
            }
            else {
                bRet = TRUE;
            }
        }
        else {
            SetLastError(ERROR_INVALID_DATATYPE);
        }
    }

    RegCloseKey(hKeyUser);
    RegCloseKey(hKeyMachine);
    ReleaseSemaphore(g_hSemRegistry, 1, NULL);

RPPE_exit:
    if (!bRet) {
        DebugPrint("ReadPwrPolicyEx, failed, LastError: 0x%08X",
                 (lRet == ERROR_SUCCESS) ? GetLastError():lRet);
        if (lRet != ERROR_SUCCESS) {
            DebugPrint("  lpszUserKeyName: %s, lpszSchemeName: %s", lpszUserKeyName, lpszSchemeName);
        }
    }
    return bRet;
}

/*******************************************************************************
*
*  WritePwrPolicyEx
*
*  DESCRIPTION:
*   Supports WritePwrScheme and
*   WriteGlobalPwrPolicy
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN WritePwrPolicyEx(
    LPTSTR  lpszUserKeyName,
    LPTSTR  lpszMachineKeyName,
    PUINT   puiID,
    LPTSTR  lpszName,
    LPTSTR  lpszDescription,
    LPVOID  lpvUser,
    DWORD   dwcbUserSize,
    LPVOID  lpvMachine,
    DWORD   dwcbMachineSize
)
{
    HKEY     hKeyUser, hKeyMachine;
    LONG     lRet;
    DWORD    dwDisposition, dwSize;
    TCHAR    szNum[NUM_DEC_DIGITS];
    LPTSTR   lpszKeyName;

    if ((!lpszUserKeyName || !lpszMachineKeyName || !lpvUser || !lpvMachine) ||
        (!puiID && !lpszName)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto WPPE_exit;
    }

    // If a scheme ID was passed
    if (puiID) {
        if (*puiID == NEWSCHEME) {
            *puiID = ++g_uiLastID;
            wsprintf(szNum, TEXT("%d"), *puiID);
            if (!ReadWritePowerValue(HKEY_LOCAL_MACHINE,
                                     c_szREGSTR_PATH_MACHINE_POWERCFG,
                                     c_szREGSTR_VAL_LASTID,
                                     szNum, &dwSize, TRUE, TRUE)) {
                return FALSE;
            }
        }
        else {
            wsprintf(szNum, TEXT("%d"), *puiID);
        }
        lpszKeyName = szNum;
    }
    else {
        lpszKeyName = lpszName;
    }

    // Wait on/take the registry semaphore.
    if (!TakeRegSemaphore()) {
        return FALSE;
    }

    if (!OpenPathKeys(lpszUserKeyName, lpszMachineKeyName, lpszKeyName,
                      &hKeyUser, &hKeyMachine, FALSE, KEY_WRITE)) {
        ReleaseSemaphore(g_hSemRegistry, 1, FALSE);
        return FALSE;
    }

    // Write the binary policies data
    if ((lRet = RegSetValueEx(hKeyUser,
                              TEXT("Policies"),
                              0,
                              REG_BINARY,
                              (PBYTE) lpvUser,
                              dwcbUserSize)) == ERROR_SUCCESS) {

        // Write the binary policies data
        if ((lRet = RegSetValueEx(hKeyMachine,
                                  TEXT("Policies"),
                                  0,
                                  REG_BINARY,
                                  (PBYTE) lpvMachine,
                                  dwcbMachineSize)) == ERROR_SUCCESS) {

            // Write the name text if an ID was provided.
            if (lpszName && puiID) {
                dwSize = (lstrlen(lpszName) + 1) * sizeof(TCHAR);
                lRet = RegSetValueEx(hKeyUser, TEXT("Name"), 0,
                                     REG_SZ, (PBYTE) lpszName, dwSize);
            }

            // Write the description text.
            if (lpszDescription && (lRet == ERROR_SUCCESS)) {
                dwSize = (lstrlen(lpszDescription) + 1) * sizeof(TCHAR);
                lRet =  RegSetValueEx(hKeyUser, TEXT("Description"), 0,
                                      REG_SZ, (PBYTE) lpszDescription, dwSize);
            }
        }
    }
    RegCloseKey(hKeyUser);
    RegCloseKey(hKeyMachine);
    ReleaseSemaphore(g_hSemRegistry, 1, NULL);

WPPE_exit:
    if (lRet != ERROR_SUCCESS) {
        DebugPrint("WritePwrPolicyEx, failed, LastError: 0x%08X",
                 (lRet == ERROR_SUCCESS) ? GetLastError():lRet);
        DebugPrint("  lpszUserKeyName: %s, lpszKeyName: %s", lpszUserKeyName, lpszKeyName);
        return FALSE;
    }
    return TRUE;
}

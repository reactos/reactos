/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       POWRPROF.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*   User power management profile maintenance library. Implements persistent
*   power mamagement data storage. To minimize registry storage and simplify
*   user power profile management, power scheme's are divided into two parts,
*   GLOBAL_POWER_POLICY and POWER_POLICY:
*
*   User Level              Registry Storage
*   GLOBAL_POWER_POLICY =                               - Common scheme data.
*                           GLOBAL_MACHINE_POWER_POLICY - Per machine data.
*                        +  GLOBAL_USER_POWER_POLICY    - Per user data.
*
*   POWER_POLICY        =                              - Unique scheme data.
*                           MACHINE_POWER_POLICY       - Per machine data.
*                         + USER_POWER_POLICY          - Per user data.
*
*   The interface to the power policy manager is by AC and DC
*   SYSTEM_POWER_POLICY which is formed by merging the above structures.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <initguid.h>
#include <devguid.h>
#include <string.h>
#include <regstr.h>
#include <commctrl.h>

#include <ntpoapi.h>

#include <setupapi.h>
#include <syssetup.h>
#include <setupbat.h>

#include "powrprof.h"


/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

HINSTANCE   g_hInstance;        // Global instance handle of this DLL.
HANDLE      g_hSemRegistry;     // Registry semaphore.
UINT        g_uiLastID;         // The last ID value used, per machine.

// Variables and definitions to manage dynamic link to  NtPowerInformation.
typedef NTSTATUS (NTAPI *PFNNTPOWERINFORMATION)(POWER_INFORMATION_LEVEL, PVOID, ULONG, PVOID, ULONG);
HINSTANCE               g_hmodNTDLL;
PFNNTPOWERINFORMATION   g_pfnNtPowerInformation;
PFNNTINITIATEPWRACTION  g_pfnNtInitiatePwrAction;

#ifdef WINNT
// Global administrator power policy variables. Initialize to allow everything.
BOOLEAN g_bAdminOverrideActive = FALSE;
ADMINISTRATOR_POWER_POLICY g_app =
{
    // Meaning of power action "sleep" Min, Max.
    PowerSystemSleeping1, PowerSystemHibernate,

    // Video policies Min, Max.
    0, -1,

    // Disk spindown policies Min, Max.
    0, -1
};
#endif

// Debug strings for Power Policy Manager POWER_INFORMATION_LEVEL:
#ifdef DEBUG
LPTSTR lpszInfoLevel[] =
{
    TEXT("SystemPowerPolicyAc"),
    TEXT("SystemPowerPolicyDc"),
    TEXT("VerifySystemPolicyAc"),
    TEXT("VerifySystemPolicyDc"),
    TEXT("SystemPowerCapabilities"),
    TEXT("SystemBatteryState"),
    TEXT("SystemPowerStateHandler"),
    TEXT("ProcessorStateHandler"),
    TEXT("SystemPowerPolicyCurrent"),
    TEXT("AdministratorPowerPolicy"),
    TEXT("SystemReserveHiberFile"),
    TEXT("ProcessorInformation"),
    TEXT("SystemPowerInformation")
};

int g_iShowValidationChanges;
int g_iShowCapabilities;
int g_iShowSetPPM;
#endif

// Global value for storing a single registry value name/path. Multithread
// protection is provided by the Registry semaphore.
TCHAR g_szRegValue[REGSTR_MAX_VALUE_LENGTH];

// Global semaphore name.
const TCHAR c_szSemRegistry[] = TEXT("PowerProfileRegistrySemaphore");

// Strings used to access the registry. REGSTR_* string constants can be
// found in sdk\inc\regstr.h, USER strings are under HKEY_CURRENT_USER,
// MACHINE strings are under HKEY_LOCAL_MACHINE.

TCHAR c_szREGSTR_PATH_MACHINE_POWERCFG[]  = REGSTR_PATH_CONTROLSFOLDER TEXT("\\PowerCfg");
TCHAR c_szREGSTR_PATH_USER_POWERCFG[]     = REGSTR_PATH_CONTROLPANEL TEXT("\\PowerCfg");

TCHAR c_szREGSTR_PATH_MACHINE_POWERCFG_POLICIES[] = REGSTR_PATH_CONTROLSFOLDER TEXT("\\PowerCfg\\PowerPolicies");
TCHAR c_szREGSTR_PATH_USER_POWERCFG_POLICIES[]    = REGSTR_PATH_CONTROLPANEL   TEXT("\\PowerCfg\\PowerPolicies");

TCHAR c_szREGSTR_VAL_GLOBALPOWERPOLICY[]  = TEXT("GlobalPowerPolicy");
TCHAR c_szREGSTR_VAL_CURRENTPOWERPOLICY[] = TEXT("CurrentPowerPolicy");

// These values are provided to help OEM's meet disk drive warranty requirements.
TCHAR c_szREGSTR_VAL_SPINDOWNMAX[]        = TEXT("DiskSpinDownMax");
TCHAR c_szREGSTR_VAL_SPINDOWNMIN[]        = TEXT("DiskSpinDownMin");

// These values are provided to support administrator power policies.
TCHAR c_szREGSTR_VAL_ADMINMAXVIDEOTIMEOUT[]       = TEXT("AdminMaxVideoTimeout");
TCHAR c_szREGSTR_VAL_ADMINMAXSLEEP[]              = TEXT("AdminMaxSleep");

// This value manages the policy ID's.
TCHAR c_szREGSTR_VAL_LASTID[] = TEXT("LastID");

// This value turns on debug logging of PPM Validation Changes
#ifdef DEBUG
TCHAR c_szREGSTR_VAL_SHOWVALCHANGES[] = TEXT("ShowValidationChanges");
TCHAR c_szREGSTR_VAL_SHOWCAPABILITIES[] = TEXT("ShowCapabilities");
TCHAR c_szREGSTR_VAL_SHOWSETPPM[] = TEXT("ShowSetPPM");
#endif

/*******************************************************************************
*
*               P U B L I C   E N T R Y   P O I N T S
*
*******************************************************************************/

/*******************************************************************************
*
*  DllInitialize
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN DllInitialize(IN PVOID hmod, IN ULONG ulReason, IN PCONTEXT pctx OPTIONAL)
{

    UNREFERENCED_PARAMETER(pctx);

    switch (ulReason) {

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hmod);
            g_hInstance = hmod;

#ifdef DEBUG
            // Get the debug optional settings from HKCU.
            ReadOptionalDebugSettings();
#endif
            // Create the registry semaphore.
            g_hSemRegistry  = MyCreateSemaphore(c_szSemRegistry);

            // Link to NtPowerInformation
            g_hmodNTDLL = LoadLibrary(TEXT("NTDLL.DLL"));
            if (!g_hmodNTDLL) {
                DebugPrint( "Unable to load NTDLL.DLL");
                return FALSE;
            }
            else {
                g_pfnNtPowerInformation =
                    (PFNNTPOWERINFORMATION)GetProcAddress(g_hmodNTDLL,
                                                          "NtPowerInformation");
                if (!g_pfnNtPowerInformation) {
                    DebugPrint( "Unable to resolve address: NtPowerInformation");
                    return FALSE;
                }
                g_pfnNtInitiatePwrAction =
                    (PFNNTINITIATEPWRACTION)GetProcAddress(g_hmodNTDLL,
                                                          "NtInitiatePowerAction");
                if (!g_pfnNtPowerInformation) {
                    DebugPrint( "Unable to resolve address: NtPowerInformation");
                    return FALSE;
                }
            }

#ifdef WINNT
            // Initialize an administrator power policy.
            InitAdmin(&g_app);
#endif
            // One time registry related initialization.
            if (!RegistryInit(&g_uiLastID)) {
                return FALSE;
            }
            break;

        case DLL_PROCESS_DETACH:
            CloseHandle(g_hSemRegistry);
            g_hSemRegistry = NULL;
            break;
    }
    return TRUE;
}

/*******************************************************************************
*
*  IsAdminOverrideActive
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN IsAdminOverrideActive(PADMINISTRATOR_POWER_POLICY papp)
{
#ifdef WINNT
    if ((g_bAdminOverrideActive) && (papp)) {
        memcpy(papp, &g_app, sizeof(g_app));
    }
    return g_bAdminOverrideActive;
#else
    return FALSE;
#endif
}

/*******************************************************************************
*
*  IsPwrSuspendAllowed
*
*  DESCRIPTION:
*   Called by Explorer to determine whether suspend is supported.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN IsPwrSuspendAllowed(VOID)
{
    SYSTEM_POWER_CAPABILITIES   spc;

    if (GetPwrCapabilities(&spc)) {
        if (spc.SystemS1 || spc.SystemS2 || spc.SystemS3) {
            return TRUE;
        }
    }
    return FALSE;
}

/*******************************************************************************
*
*  IsPwrHibernateAllowed
*
*  DESCRIPTION:
*   Called by Explorer to determine whether hibernate is supported.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN IsPwrHibernateAllowed(VOID)
{
    SYSTEM_POWER_CAPABILITIES   spc;

    if (GetPwrCapabilities(&spc)) {
        if (spc.SystemS4 && spc.HiberFilePresent) {
            return TRUE;
        }
    }
    return FALSE;
}

/*******************************************************************************
*
*  IsPwrShutdownAllowed
*
*  DESCRIPTION:
*   Called by Explorer to determine whether shutdown is supported.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN IsPwrShutdownAllowed(VOID)
{
    SYSTEM_POWER_CAPABILITIES   spc;

    if (GetPwrCapabilities(&spc)) {
        if (spc.SystemS5) {
            return TRUE;
        }
    }
    return FALSE;
}

/*******************************************************************************
*
*  CanUserWritePwrScheme
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN CanUserWritePwrScheme(VOID)
{
    DWORD   dwSize;
    TCHAR   szNum[NUM_DEC_DIGITS];
    LONG    lErr;

    // Read in the last ID this value must be present.
    dwSize = sizeof(szNum);
    if (ReadWritePowerValue(HKEY_LOCAL_MACHINE,
                            c_szREGSTR_PATH_MACHINE_POWERCFG,
                            c_szREGSTR_VAL_LASTID,
                            szNum, &dwSize, FALSE, TRUE)) {

        // Write the value back out, this may fail if user doesn't have write access.
        if (ReadWritePowerValue(HKEY_LOCAL_MACHINE,
                                c_szREGSTR_PATH_MACHINE_POWERCFG,
                                c_szREGSTR_VAL_LASTID,
                                szNum, &dwSize, TRUE, TRUE)) {
            return TRUE;
        }
        else {
            lErr = GetLastError();
            if (lErr != ERROR_ACCESS_DENIED) {
                DebugPrint( "CanUserWritePwrScheme, Unable to write last ID, Error: %d", lErr);
            }
        }
    }
    else {
        lErr = GetLastError();
        DebugPrint( "CanUserWritePwrScheme, Unable to fetch last ID, Error: %d", lErr);
    }
    return FALSE;
}

/*******************************************************************************
*
*  GetPwrDiskSpindownRange
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN GetPwrDiskSpindownRange(PUINT puiMax, PUINT puiMin)
{
    if (!puiMax || !puiMin) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (ReadPowerIntOptional(HKEY_LOCAL_MACHINE,
                             c_szREGSTR_PATH_MACHINE_POWERCFG,
                             c_szREGSTR_VAL_SPINDOWNMAX,
                             puiMax) &&
        ReadPowerIntOptional(HKEY_LOCAL_MACHINE,
                             c_szREGSTR_PATH_MACHINE_POWERCFG,
                             c_szREGSTR_VAL_SPINDOWNMIN,
                             puiMin)) {
            return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
*
*  EnumPwrSchemes
*
*  DESCRIPTION:
*   Calls back the PWRSCHEMESENUMPROC with the ID, a pointer to the name,
*   the size in bytes of the name, a pointer to the description, the size in
*   bytes of the description, a pointer to the power policies and a user
*   defined value. Returns ERROR_SUCCESS on success, else error code. Callback
*   data is not allocated and is only valid during the scope of the callback.
*
*   Note: No calls to any other API's in this library should be made during
*   the call back to PWRSCHEMESENUMPROC. The registry semaphore is held at
*   this time and a deadlock will result.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN EnumPwrSchemes(
    PWRSCHEMESENUMPROC  lpfn,
    LPARAM              lParam
)
{
    HKEY            hKeyPolicyUser, hKeyPolicyMachine;
    HKEY            hKeyUser    = INVALID_HANDLE_VALUE;
    HKEY            hKeyMachine = INVALID_HANDLE_VALUE;
    DWORD           dwDescSize;
    DWORD           dwSize, dwNameSize, dwIndex = 0;
    BOOLEAN         bOneCallBackOk = FALSE;
    LONG            lRet = ERROR_SUCCESS;
    LPTSTR          lpszDescBuf, lpszDesc;
    TCHAR           szNameBuf[MAX_NAME_LEN+1];
    FILETIME        ft;
    UINT            uiID;

    MACHINE_POWER_POLICY    mpp;
    USER_POWER_POLICY       upp;
    POWER_POLICY            pp;

    if (!lpfn) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto WESPSP_exit;
    }

    // Wait on/take the registry semaphore.
    if (!TakeRegSemaphore()) {
        return FALSE;
    }

    // Allocate a description buffer.
    lpszDescBuf = LocalAlloc(0, (MAX_DESC_LEN + 1) * sizeof(TCHAR));
    if (!lpszDescBuf) {
        goto WESPSP_exit;
    }

    if (!OpenMachineUserKeys(c_szREGSTR_PATH_USER_POWERCFG_POLICIES,
                             c_szREGSTR_PATH_MACHINE_POWERCFG_POLICIES,
                             &hKeyUser, &hKeyMachine)) {
        ReleaseSemaphore(g_hSemRegistry, 1, NULL);
        return FALSE;
    }

    // Enumerate the schemes
    while (lRet == ERROR_SUCCESS) {
        dwSize = REGSTR_MAX_VALUE_LENGTH - 1;
        if ((lRet = RegEnumKeyEx(hKeyUser,
                                 dwIndex,
                                 g_szRegValue,
                                 &dwSize,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &ft)) == ERROR_SUCCESS) {

            // Open the Policies Key. The key name is the policies ID.
            lpszDesc = NULL;
            if (MyStrToInt(g_szRegValue, &uiID)) {
                if ((lRet = RegOpenKeyEx(hKeyUser,
                                         g_szRegValue,
                                         0,
                                         KEY_READ,
                                         &hKeyPolicyUser)) == ERROR_SUCCESS) {

                    if ((lRet = RegOpenKeyEx(hKeyMachine,
                                             g_szRegValue,
                                             0,
                                             KEY_READ,
                                             &hKeyPolicyMachine)) == ERROR_SUCCESS) {

                        // Get the friendly name..
                        dwNameSize = MAX_NAME_SIZE;
                        if ((lRet = RegQueryValueEx(hKeyPolicyUser,
                                                    TEXT("Name"),
                                                    NULL,
                                                    NULL,
                                                    (PBYTE) szNameBuf,
                                                    &dwNameSize)) == ERROR_SUCCESS) {

                            // Descriptions are optional.
                            dwDescSize = MAX_DESC_SIZE;
                            if ((lRet = RegQueryValueEx(hKeyPolicyUser,
                                                        TEXT("Description"),
                                                        NULL,
                                                        NULL,
                                                        (PBYTE) lpszDescBuf,
                                                        &dwDescSize)) == ERROR_SUCCESS) {
                                lpszDesc = lpszDescBuf;
                            }

                            // Read the user and machine policies.
                            dwSize = sizeof(upp);
                            if ((lRet = RegQueryValueEx(hKeyPolicyUser,
                                                        TEXT("Policies"),
                                                        NULL,
                                                        NULL,
                                                        (PBYTE) &upp,
                                                        &dwSize)) == ERROR_SUCCESS) {

                                dwSize = sizeof(mpp);
                                if ((lRet = RegQueryValueEx(hKeyPolicyMachine,
                                                            TEXT("Policies"),
                                                            NULL,
                                                            NULL,
                                                            (PBYTE) &mpp,
                                                            &dwSize)) == ERROR_SUCCESS) {


                                    // Merge the user and machine policies.
                                    if (MergePolicies(&upp, &mpp, &pp)) {

                                        // Call the enumerate proc.
                                        if (!lpfn(uiID,
                                                  dwNameSize, szNameBuf,
                                                  dwDescSize, lpszDesc,
                                                  &pp, lParam)) {
                                            RegCloseKey(hKeyPolicyMachine);
                                            RegCloseKey(hKeyPolicyUser);
                                            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                                            break;
                                        }
                                        else {
                                            bOneCallBackOk = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                        RegCloseKey(hKeyPolicyMachine);
                    }
                    RegCloseKey(hKeyPolicyUser);
                }
            }
        }
        dwIndex++;
    }

    RegCloseKey(hKeyUser);
    RegCloseKey(hKeyMachine);
    ReleaseSemaphore(g_hSemRegistry, 1, NULL);
    if (lpszDescBuf) {
        LocalFree(lpszDescBuf);
    }

WESPSP_exit:
    if (lRet != ERROR_NO_MORE_ITEMS) {
        DebugPrint( "EnumPwrSchemes, failed, LastError: 0x%08X",
                 (lRet == ERROR_SUCCESS) ? GetLastError():lRet);
    }
    return bOneCallBackOk;
}

/*******************************************************************************
*
*  ReadGlobalPwrPolicy
*
*  DESCRIPTION:
*   Function reads the users global power policy profile and returns it.
*   If there is no such profile FALSE is returned. A global power policy
*   profile is per user, and contains values which apply to all of a users
*   power policies.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ReadGlobalPwrPolicy(
    PGLOBAL_POWER_POLICY  pgpp
)
{
    GLOBAL_MACHINE_POWER_POLICY gmpp;
    GLOBAL_USER_POWER_POLICY    gupp;

    if (ReadPwrPolicyEx(c_szREGSTR_PATH_USER_POWERCFG,
                        c_szREGSTR_PATH_MACHINE_POWERCFG,
                        c_szREGSTR_VAL_GLOBALPOWERPOLICY,
                        NULL,
                        NULL,
                        &gupp,
                        sizeof(gupp),
                        &gmpp,
                        sizeof(gmpp))) {

        return MergeGlobalPolicies(&gupp, &gmpp, pgpp);
    }
    return FALSE;

}

/*******************************************************************************
*
*  WritePwrScheme
*
*  DESCRIPTION:
*   Function to write a users power policy profile.  If the profile already
*   exists it is replaced.  Otherwise a new profile is created.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN WritePwrScheme(
    PUINT           puiID,
    LPTSTR          lpszSchemeName,
    LPTSTR          lpszDescription,
    PPOWER_POLICY   lpScheme
)
{
    MACHINE_POWER_POLICY    mpp;
    USER_POWER_POLICY       upp;

    if (SplitPolicies(lpScheme, &upp, &mpp)) {
        return WritePwrPolicyEx(c_szREGSTR_PATH_USER_POWERCFG_POLICIES,
                                c_szREGSTR_PATH_MACHINE_POWERCFG_POLICIES,
                                puiID,
                                lpszSchemeName,
                                lpszDescription,
                                &upp,
                                sizeof(upp),
                                &mpp,
                                sizeof(mpp));
    }
    return FALSE;
}

/*******************************************************************************
*
*  WriteGlobalPwrPolicy
*
*  DESCRIPTION:
*   Function to write a users global power policy profile.  If the profile
*   already exists it is replaced.  Otherwise a new profile is created.
*   A global power policy profile is per user, and contains values which
*   apply to all of a users power policies. Otherwise a new profile is created.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN WriteGlobalPwrPolicy (
    PGLOBAL_POWER_POLICY   pgpp
)
{
    GLOBAL_MACHINE_POWER_POLICY gmpp;
    GLOBAL_USER_POWER_POLICY    gupp;

    if (SplitGlobalPolicies(pgpp, &gupp, &gmpp)) {
        return WritePwrPolicyEx(c_szREGSTR_PATH_USER_POWERCFG,
                                c_szREGSTR_PATH_MACHINE_POWERCFG,
                                NULL,
                                c_szREGSTR_VAL_GLOBALPOWERPOLICY,
                                NULL,
                                &gupp,
                                sizeof(gupp),
                                &gmpp,
                                sizeof(gmpp));
    }
    return FALSE;
}

/*******************************************************************************
*
*  DeletePwrScheme
*
*  DESCRIPTION:
*   Function to delete a users power policy profile. An attempt to delete the
*   currently active power policy profile will fail with last error set to
*   ERROR_ACCESS_DENIED.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN DeletePwrScheme(UINT uiID)
{
    HKEY    hKeyUser;
    DWORD   dwSize = REGSTR_MAX_VALUE_LENGTH * sizeof(TCHAR);
    BOOLEAN bRet = FALSE;
    LONG    lRet = ERROR_SUCCESS;
    TCHAR   szNum[NUM_DEC_DIGITS];
    int     iCurrent;
    HKEY     hKeyCurrentUser;

    // Wait on/take the registry semaphore.
    if (!TakeRegSemaphore()) {
        return FALSE;
    }

    if (OpenCurrentUser(&hKeyCurrentUser)) {

        // Don't allow the currently active power policy profile to be deleted.
        if (ReadWritePowerValue(hKeyCurrentUser,
                                c_szREGSTR_PATH_USER_POWERCFG,
                                c_szREGSTR_VAL_CURRENTPOWERPOLICY,
                                g_szRegValue, &dwSize, FALSE, FALSE) &&
            MyStrToInt(g_szRegValue, &iCurrent)) {

            if (uiID != (UINT) iCurrent) {

                // For now we only delete the user portion of a policy. We may
                // want a ref count on the machine portion which allows deletion
                // of the machine portion when no user portion references it.
                if ((lRet = RegOpenKey(hKeyCurrentUser,
                                       c_szREGSTR_PATH_USER_POWERCFG_POLICIES,
                                       &hKeyUser)) == ERROR_SUCCESS) {

                    wsprintf(szNum, TEXT("%d"), uiID);

                    if ((lRet = RegDeleteKey(hKeyUser, szNum)) == ERROR_SUCCESS) {
                       bRet = TRUE;
                    }
                    RegCloseKey(hKeyUser);
                }
            }
            else {
                SetLastError(ERROR_ACCESS_DENIED);
            }
        }
        CloseCurrentUser(hKeyCurrentUser);
    }

    ReleaseSemaphore(g_hSemRegistry, 1, NULL);

    if (!bRet) {
        DebugPrint( "DeletePwrScheme, failed, LastError: 0x%08X",
                 (lRet == ERROR_SUCCESS) ? GetLastError():lRet);
    }
    return bRet;
}

/*******************************************************************************
*
*  GetActivePwrScheme
*
*  DESCRIPTION:
*   Retrieves the ID of the currently active power policy profile. This value
*   is set by SetActivePwrScheme.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN
GetActivePwrScheme(PUINT puiID)
{
    BOOLEAN bRet = FALSE;
    TCHAR   szNum[NUM_DEC_DIGITS];
    DWORD   dwSize = SIZE_DEC_DIGITS;
    HKEY    hKey;

    if (OpenCurrentUser(&hKey)) {
        if (ReadWritePowerValue(hKey,
                                c_szREGSTR_PATH_USER_POWERCFG,
                                c_szREGSTR_VAL_CURRENTPOWERPOLICY,
                                szNum, &dwSize, FALSE, TRUE) &&
            MyStrToInt(szNum, puiID)) {
            bRet = TRUE;
        }
        CloseCurrentUser(hKey);
    }
    return bRet;
}

/*******************************************************************************
*
*  SetActivePwrScheme
*
*  DESCRIPTION:
*   Set the currently active power policy profile.
*
*  PARAMETERS:
*   uiID           - ID of the new active power scheme.
*   lpGlobalPolicy - Optional global policies to merge with active power scheme.
*   lpPowerPolicy  - Optional power policies to merge with active power scheme.
*
*******************************************************************************/

BOOLEAN
SetActivePwrScheme(
    UINT                    uiID,
    PGLOBAL_POWER_POLICY    pgpp,
    PPOWER_POLICY           ppp
)
{
    DWORD                       dwSize;
    NTSTATUS                    ntsRetVal;
    TCHAR                       szNum[NUM_DEC_DIGITS];
    POWER_POLICY                pp;
    GLOBAL_POWER_POLICY         gpp;
    MACHINE_POWER_POLICY        mpp;
    USER_POWER_POLICY           upp;
    GLOBAL_MACHINE_POWER_POLICY gmpp;
    GLOBAL_USER_POWER_POLICY    gupp;
    SYSTEM_POWER_POLICY         sppAc, sppDc;

    HKEY     hKeyCurrentUser;
    BOOLEAN  bRet = FALSE;

    // If a new scheme is not passed, fetch the target scheme.
    if (!ppp) {
        if (!ReadPwrScheme(uiID, &pp)) {
            return FALSE;
        }
        ppp = &pp;
    }

    // If a new global policy is not passed, fetch the target global policy.
    if (!pgpp) {
        if (!ReadGlobalPwrPolicy(&gpp)) {
            return FALSE;
        }
        pgpp = &gpp;
    }

    // Merge global policy and user scheme if a global policy was passed.
    if (!MergeToSystemPowerPolicies(pgpp, ppp, &sppAc, &sppDc)) {
        return FALSE;
    }

    // Write out what was requested to the registry.
    SplitPolicies(ppp, &upp, &mpp);
    if (!WritePwrPolicyEx(c_szREGSTR_PATH_USER_POWERCFG_POLICIES,
                          c_szREGSTR_PATH_MACHINE_POWERCFG_POLICIES,
                          &uiID,
                          NULL,
                          NULL,
                          &upp,
                          sizeof(upp),
                          &mpp,
                          sizeof(mpp))) {
        return FALSE;
    }

    SplitGlobalPolicies(pgpp, &gupp, &gmpp);
    if (!WritePwrPolicyEx(c_szREGSTR_PATH_USER_POWERCFG,
                          c_szREGSTR_PATH_MACHINE_POWERCFG,
                          NULL,
                          c_szREGSTR_VAL_GLOBALPOWERPOLICY,
                          NULL,
                          &gupp,
                          sizeof(gupp),
                          &gmpp,
                          sizeof(gmpp))) {
        return FALSE;
    }

    // Call down to the power policy manager to set the scheme.
    ntsRetVal = CallNtSetValidateAcDc(FALSE, &sppAc, &sppAc, &sppDc, &sppDc);

    if (ntsRetVal == STATUS_SUCCESS) {
        if (OpenCurrentUser(&hKeyCurrentUser)) {

            // On success, set the current active power scheme in the registry.
            wsprintf(szNum, TEXT("%d"), uiID);
            bRet =  ReadWritePowerValue(hKeyCurrentUser,
                                       c_szREGSTR_PATH_USER_POWERCFG,
                                       c_szREGSTR_VAL_CURRENTPOWERPOLICY,
                                       szNum, NULL, TRUE, TRUE);

            CloseCurrentUser(hKeyCurrentUser);
        }
    }
    return bRet;
}

/*******************************************************************************
*
*  LoadCurrentPwrScheme
*
*  DESCRIPTION:
*   A Memphis only cover to call SetActivePwrScheme using RunDLL32 calling
*   convention. Do not change parameter list.
*
*  PARAMETERS:
*
*******************************************************************************/

void WINAPI LoadCurrentPwrScheme(
    HWND hwnd,
    HINSTANCE hAppInstance,
    LPSTR lpszCmdLine,
    int nCmdShow)
{
    UINT uiID;

    if (GetActivePwrScheme(&uiID)) {
        SetActivePwrScheme(uiID, NULL, NULL);
    }
}

/*******************************************************************************
*
*  MergeLegacyPwrScheme
*
*  DESCRIPTION:
*   A Memphis only call to merge legacy power management registry info into the
*   currently active power scheme.
*   Called using the RunDLL32 calling convention. Do not change parameter list.
*
*  PARAMETERS:
*
*******************************************************************************/

void WINAPI MergeLegacyPwrScheme(
    HWND hwnd,
    HINSTANCE hAppInstance,
    LPSTR lpszCmdLine,
    int nCmdShow)
{
    DWORD                       dwSize, dwLegacy;
    POWER_POLICY                pp;
    GLOBAL_POWER_POLICY         gpp;
    UINT                        uiID;
    HKEY                        hKeyCurrentUser;

    // Get the active power scheme from the registry.
    if (!GetActivePwrScheme(&uiID)) {
        return;
    }
    if (!ReadPwrScheme(uiID, &pp)) {
        return;
    }
    if (!ReadGlobalPwrPolicy(&gpp)) {
        return;
    }

    if (OpenCurrentUser(&hKeyCurrentUser)) {
        // Get the legacy video monitor power down information.
        if (ReadPowerIntOptional(hKeyCurrentUser,
                                 REGSTR_PATH_SCREENSAVE,
                                 REGSTR_VALUE_POWEROFFACTIVE,
                                 &pp.user.VideoTimeoutAc)) {
            DebugPrint( "MergeLegacyPwrScheme, found legacy %s: %d", REGSTR_VALUE_POWEROFFACTIVE, pp.user.VideoTimeoutAc);
            pp.user.VideoTimeoutDc = pp.user.VideoTimeoutAc;
        }
        CloseCurrentUser(hKeyCurrentUser);
    }


    // Get the legacy disk spin down information.
    if (ReadPowerIntOptional(HKEY_LOCAL_MACHINE,
                             REGSTR_PATH_FILESYSTEM,
                             REGSTR_VAL_ACDRIVESPINDOWN,
                             &pp.user.SpindownTimeoutAc)) {
        DebugPrint( "MergeLegacyPwrScheme, found legacy %s: %d", REGSTR_VAL_ACDRIVESPINDOWN, pp.user.SpindownTimeoutAc);
    }

    if (ReadPowerIntOptional(HKEY_LOCAL_MACHINE,
                               REGSTR_PATH_FILESYSTEM,
                               REGSTR_VAL_BATDRIVESPINDOWN,
                               &pp.user.SpindownTimeoutDc)) {
        DebugPrint( "MergeLegacyPwrScheme, found legacy %s: %d", REGSTR_VAL_BATDRIVESPINDOWN, pp.user.SpindownTimeoutDc);
    }

    // Get the legacy battery meter information.
    dwSize = sizeof(dwLegacy);
    if (ReadPowerValueOptional(HKEY_LOCAL_MACHINE,
                               REGSTR_PATH_VPOWERD,
                               REGSTR_VAL_VPOWERDFLAGS,
                               (LPTSTR)&dwLegacy, &dwSize)) {
        if (dwLegacy & VPDF_SHOWMULTIBATT) {
            gpp.user.GlobalFlags |= EnableSysTrayBatteryMeter;
        }
        else {
            gpp.user.GlobalFlags &= ~EnableSysTrayBatteryMeter;
        }
        DebugPrint( "MergeLegacyPwrScheme, found legacy %s: %X", REGSTR_VAL_VPOWERDFLAGS, dwLegacy);
    }

    // Write out the modified active power scheme.
    if (!WriteGlobalPwrPolicy(&gpp)) {
        return;
    }

    WritePwrScheme(&uiID, NULL, NULL, &pp);
}

/*******************************************************************************
*
*  GetPwrCapabilities
*
*  DESCRIPTION:
*   Get the system power capabilities from the Power Policy Manager.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN GetPwrCapabilities(PSYSTEM_POWER_CAPABILITIES lpspc)
{
    NTSTATUS ntsRetVal = STATUS_SUCCESS;

    if (!lpspc) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ntsRetVal = CallNtPowerInformation(SystemPowerCapabilities, NULL, 0, lpspc,
                                       sizeof(SYSTEM_POWER_CAPABILITIES));

    if (ntsRetVal == STATUS_SUCCESS) {
#ifdef DEBUG
        if (g_iShowCapabilities) {
            DumpSystemPowerCapabilities("GetPwrCapabilities, returned:", lpspc);
        }
#ifdef SIM_BATTERY
        lpspc->SystemBatteriesPresent = TRUE;
#endif
#endif
        return TRUE;
    }
    else {
        return FALSE;
    }
}

/*******************************************************************************
*
*  CallNtPowerInformation
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

NTSTATUS CallNtPowerInformation(
    POWER_INFORMATION_LEVEL InformationLevel,
    PVOID InputBuffer OPTIONAL,
    ULONG InputBufferLength,
    PVOID OutputBuffer OPTIONAL,
    ULONG OutputBufferLength
)
{
    NTSTATUS ntsRetVal;


#ifdef WINNT
    DWORD dwOldState, dwStatus;
    DWORD dwErrorSave;
    LPCTSTR PrivilegeName;

    if (InformationLevel == SystemReserveHiberFile) {
        PrivilegeName = SE_CREATE_PAGEFILE_NAME;
    } else {
        PrivilegeName = SE_SHUTDOWN_NAME;
    }

    SetLastError(0);
    dwStatus = SetPrivilegeAttribute(PrivilegeName, SE_PRIVILEGE_ENABLED,
                                     &dwOldState);
    dwErrorSave = GetLastError();
#endif

    ntsRetVal = g_pfnNtPowerInformation(InformationLevel,
                                        InputBuffer, InputBufferLength,
                                        OutputBuffer, OutputBufferLength);

#ifdef WINNT
    //
    // If we were able to set the privilege, then reset it.
    //
    if (NT_SUCCESS(dwStatus) && dwErrorSave == 0) {
        SetPrivilegeAttribute(PrivilegeName, dwOldState, NULL);
    }
    else {
        DebugPrint( "CallNtPowerInformation, SetPrivilegeAttribute failed: 0x%08X", GetLastError());
    }
#endif

#ifdef DEBUG
    if (ntsRetVal != STATUS_SUCCESS) {
        DebugPrint( "NtPowerInformation, %s, failed: 0x%08X", lpszInfoLevel[InformationLevel], ntsRetVal);
    }
    else {
        if (g_iShowSetPPM && InputBuffer) {
            if ((InformationLevel == SystemPowerPolicyAc) ||
                (InformationLevel == SystemPowerPolicyDc)) {
                DumpSystemPowerPolicy("NtPowerInformation, Set to PPM, InputBuffer", InputBuffer);
            }
        }
    }
#endif

    return ntsRetVal;
}

/*******************************************************************************
*
*  SetSuspendState
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN SetSuspendState(
    BOOLEAN bHibernate,
    BOOLEAN bForce,
    BOOLEAN bWakeupEventsDisabled)
{
    NTSTATUS ntsRetVal;
    POWER_ACTION pa;
    ULONG Flags;

#ifdef WINNT
    DWORD dwOldState, dwStatus;
    DWORD dwErrorSave;

    SetLastError(0);
    dwStatus = SetPrivilegeAttribute(SE_SHUTDOWN_NAME, SE_PRIVILEGE_ENABLED,
                                     &dwOldState);
    dwErrorSave = GetLastError();
#endif

    if (bHibernate) {
        pa = PowerActionHibernate;
    }
    else {
        pa = PowerActionSleep;
    }

    Flags = POWER_ACTION_QUERY_ALLOWED | POWER_ACTION_UI_ALLOWED;

    if (bForce) {
        Flags |= POWER_ACTION_CRITICAL;
    }

    if (bWakeupEventsDisabled) {
        Flags |= POWER_ACTION_DISABLE_WAKES;
    }

    ntsRetVal = g_pfnNtInitiatePwrAction(pa, PowerSystemSleeping1, Flags, FALSE);

#ifdef WINNT
    //
    // If we were able to set the privilege, then reset it.
    //
    if (NT_SUCCESS(dwStatus) && dwErrorSave == 0) {
        SetPrivilegeAttribute(SE_SHUTDOWN_NAME, dwOldState, NULL);
    }
    else {
        DebugPrint( "SetSuspendState, SetPrivilegeAttribute failed: 0x%08X", GetLastError());
    }
#endif

    if (ntsRetVal == STATUS_SUCCESS) {
        return TRUE;
    }
    else {
        DebugPrint( "NtInitiatePowerAction, failed: 0x%08X", ntsRetVal);
        return FALSE;
    }
}

/*******************************************************************************
*
*                 P R I V A T E   F U N C T I O N S
*
*******************************************************************************/

/*******************************************************************************
*
*  ValidatePowerPolicies
*
*  DESCRIPTION:
*   Call down to the power policy manager to validate power policies.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ValidatePowerPolicies(
    PGLOBAL_POWER_POLICY    pgpp,
    PPOWER_POLICY           ppp
)
{
    POWER_POLICY        ppValid;
    GLOBAL_POWER_POLICY gppValid;
    SYSTEM_POWER_POLICY sppAc, sppDc;

    // Get current power policy data from the PPM.
    if (!GetCurrentPowerPolicies(&gppValid, &ppValid)) {
        return FALSE;
    }

    if (!pgpp) {
        pgpp = &gppValid;
    }

    if (!ppp) {
        ppp = &ppValid;
    }

    // Merge policy and global policy data.
    if (!MergeToSystemPowerPolicies(pgpp, ppp, &sppAc, &sppDc)) {
        return FALSE;
    }

    if (!ValidateSystemPolicies(&sppAc, &sppDc)) {
        return FALSE;
    }

    return SplitFromSystemPowerPolicies(&sppAc, &sppDc, pgpp, ppp);
}

/*******************************************************************************
*
*  ValidateSystemPolicies
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ValidateSystemPolicies(
    PSYSTEM_POWER_POLICY psppAc,
    PSYSTEM_POWER_POLICY psppDc
)
{
    DWORD               dwLastErr;
    NTSTATUS            ntsRetVal;

    // Call down to the power policy manager to validate the scheme.
    ntsRetVal = CallNtSetValidateAcDc(TRUE, psppAc, psppAc, psppDc, psppDc);

    // Map any PPM errors to winerror.h values
    switch (ntsRetVal) {
        case STATUS_SUCCESS:
            return TRUE;

        case STATUS_PRIVILEGE_NOT_HELD:
            dwLastErr = ERROR_ACCESS_DENIED;
            break;

        case STATUS_INVALID_PARAMETER:
            dwLastErr = ERROR_INVALID_DATA;
            break;

        default:
            dwLastErr = ERROR_GEN_FAILURE;
            break;
    }
    SetLastError(dwLastErr);
    return FALSE;
}


/*******************************************************************************
*
*  GetCurrentPowerPolicies
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN GetCurrentPowerPolicies(PGLOBAL_POWER_POLICY pgpp, PPOWER_POLICY ppp)
{
    SYSTEM_POWER_POLICY sppAc, sppDc;

    if (!GetCurrentSystemPowerPolicies(&sppAc, &sppDc)) {
        return FALSE;
    }

    return SplitFromSystemPowerPolicies(&sppAc, &sppDc, pgpp, ppp);
}

/*******************************************************************************
*
*  GetCurrentSystemPowerPolicies
*
*  DESCRIPTION:
*   Call down to the power policy manager to get the current system power
*   policies.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN GetCurrentSystemPowerPolicies(
    PSYSTEM_POWER_POLICY psppAc,
    PSYSTEM_POWER_POLICY psppDc
)
{
    NTSTATUS            ntsRetVal;

    // Call down to the power policy manager to get system power policies.
    ntsRetVal = CallNtSetValidateAcDc(FALSE, NULL, psppAc, NULL, psppDc);

    if (ntsRetVal == STATUS_SUCCESS) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

#ifdef WINNT
/*******************************************************************************
*
*  SetPrivilegeAttribute
*
*  DESCRIPTION:
*   This routine sets the security attributes for a given privilege.
*
*  PARAMETERS:
*   PrivilegeName - Name of the privilege we are manipulating.
*   NewPrivilegeAttribute - The new attribute value to use.
*   OldPrivilegeAttribute - Pointer to receive the old privilege value.
*                           OPTIONAL.
*
*******************************************************************************/

DWORD SetPrivilegeAttribute(
    LPCTSTR PrivilegeName,
    DWORD   NewPrivilegeAttribute,
    DWORD   *OldPrivilegeAttribute
)
{
    LUID             PrivilegeValue;
    TOKEN_PRIVILEGES TokenPrivileges, OldTokenPrivileges;
    DWORD            ReturnLength;
    HANDLE           TokenHandle;

    // First, find out the LUID Value of the privilege

    if(!LookupPrivilegeValue(NULL, PrivilegeName, &PrivilegeValue)) {
        return GetLastError();
    }

    // Get the token handle
    if (!OpenThreadToken (GetCurrentThread(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          FALSE, &TokenHandle)) {
        if (!OpenProcessToken (GetCurrentProcess(),
                               TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                               &TokenHandle)) {
            return GetLastError();
        }
    }

    // Set up the privilege set we will need
    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = PrivilegeValue;
    TokenPrivileges.Privileges[0].Attributes = NewPrivilegeAttribute;

    ReturnLength = sizeof(TOKEN_PRIVILEGES);
    if (!AdjustTokenPrivileges(TokenHandle, FALSE,
                               &TokenPrivileges, sizeof(TOKEN_PRIVILEGES),
                               &OldTokenPrivileges, &ReturnLength)) {
        CloseHandle(TokenHandle);
        return GetLastError();
    }
    else {
        if (OldPrivilegeAttribute != NULL) {

            //
            //  If the privilege changed, store the old value.  If it did
            //  not change, store the value passed in.
            //

            if( OldTokenPrivileges.PrivilegeCount != 0 ) {

                *OldPrivilegeAttribute = OldTokenPrivileges.Privileges[0].Attributes;

            } else {

                *OldPrivilegeAttribute = NewPrivilegeAttribute;
            }
        }
        CloseHandle(TokenHandle);
        return NO_ERROR;
    }
}
#endif


/*******************************************************************************
*
*  CallNtSetValidateAcDc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

NTSTATUS CallNtSetValidateAcDc(
    BOOLEAN bValidate,
    PVOID InputBufferAc OPTIONAL,
    PVOID OutputBufferAc OPTIONAL,
    PVOID InputBufferDc OPTIONAL,
    PVOID OutputBufferDc OPTIONAL
)
{
    NTSTATUS                ntsRetVal;
    POWER_INFORMATION_LEVEL pil, pilAc, pilDc;

#ifdef DEBUG
    SYSTEM_POWER_POLICY sppOrgAc, sppOrgDc;
#endif

#ifdef WINNT
    DWORD dwOldState, dwStatus;
    DWORD dwErrorSave;

    SetLastError(0);
    dwStatus = SetPrivilegeAttribute(SE_SHUTDOWN_NAME, SE_PRIVILEGE_ENABLED,
                                     &dwOldState);
    dwErrorSave = GetLastError();
#endif

    if (bValidate) {
        pil = pilAc = VerifySystemPolicyAc;
        pilDc = VerifySystemPolicyDc;
    }
    else {
        pil = pilAc = SystemPowerPolicyAc;
        pilDc = SystemPowerPolicyDc;
    }

#ifdef DEBUG
    if (InputBufferAc) {
        memcpy(&sppOrgAc, InputBufferAc, sizeof(SYSTEM_POWER_POLICY));
    }
    if (InputBufferDc) {
        memcpy(&sppOrgDc, InputBufferDc, sizeof(SYSTEM_POWER_POLICY));
    }
#endif

    ntsRetVal = g_pfnNtPowerInformation(pilAc,
                                        InputBufferAc,
                                        sizeof(SYSTEM_POWER_POLICY),
                                        OutputBufferAc,
                                        sizeof(SYSTEM_POWER_POLICY));

    if (ntsRetVal == STATUS_SUCCESS) {
        pil = pilDc;
        ntsRetVal = g_pfnNtPowerInformation(pilDc,
                                            InputBufferDc,
                                            sizeof(SYSTEM_POWER_POLICY),
                                            OutputBufferDc,
                                            sizeof(SYSTEM_POWER_POLICY));
    }

#ifdef WINNT
    //
    // If we were able to set the privilege, then reset it.
    //
    if (NT_SUCCESS(dwStatus) && dwErrorSave == 0) {
        SetPrivilegeAttribute(SE_SHUTDOWN_NAME, dwOldState, NULL);

#ifdef DEBUG
        if (InputBufferAc && OutputBufferAc) {
            DifSystemPowerPolicies("PPM modified AC policies",
                                    &sppOrgAc, OutputBufferAc);
        }
        if (InputBufferDc && OutputBufferDc) {
            DifSystemPowerPolicies("PPM modified DC policies",
                                   &sppOrgDc, OutputBufferDc);
        }
#endif

    }
    else {
        DebugPrint( "SetSuspendState, SetPrivilegeAttribute failed: 0x%08X", GetLastError());
    }
#endif

#ifdef DEBUG
    if (ntsRetVal != STATUS_SUCCESS) {
        DebugPrint( "NtPowerInformation, %s, failed: 0x%08X", lpszInfoLevel[pil], ntsRetVal);
        switch (pil) {
            case SystemPowerPolicyAc:
            case VerifySystemPolicyAc:
                DumpSystemPowerPolicy("InputBufferAc", InputBufferAc);
                break;

            case SystemPowerPolicyDc:
            case VerifySystemPolicyDc:
                DumpSystemPowerPolicy("InputBufferDc", InputBufferDc);
                break;
        }
    }
    else {
        if (g_iShowSetPPM && InputBufferAc && InputBufferDc && !bValidate) {
            DumpSystemPowerPolicy("CallNtSetValidateAcDc, Set AC to PPM", InputBufferAc);
            DumpSystemPowerPolicy("CallNtSetValidateAcDc, Set DC to PPM", InputBufferDc);
        }
    }
#endif
    return ntsRetVal;
}

/*******************************************************************************
*
*  ReadPwrScheme
*
*  DESCRIPTION:
*   Function reads the specified user power policy profile and returns
*   it.  If there is no such profile FALSE is returned.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ReadPwrScheme(
    UINT            uiID,
    PPOWER_POLICY   ppp
)
{
    MACHINE_POWER_POLICY    mpp;
    USER_POWER_POLICY       upp;
    TCHAR                   szNum[NUM_DEC_DIGITS];

    wsprintf(szNum, TEXT("%d"), uiID);

    if (ReadPwrPolicyEx(c_szREGSTR_PATH_USER_POWERCFG_POLICIES,
                        c_szREGSTR_PATH_MACHINE_POWERCFG_POLICIES,
                        szNum,
                        NULL,
                        0,
                        &upp,
                        sizeof(upp),
                        &mpp,
                        sizeof(mpp))) {

        return MergePolicies(&upp, &mpp, ppp);
    }
    return FALSE;
}

/*******************************************************************************
*
*  MyStrToInt
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN MyStrToInt(LPCTSTR lpSrc, PINT pi)
{

#define ISDIGIT(c)  ((c) >= TEXT('0') && (c) <= TEXT('9'))

    int n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == TEXT('-')) {
        bNeg = TRUE;
        lpSrc++;
    }

    if (!ISDIGIT(*lpSrc))  {
        DebugPrint( "MyStrToInt, non-integer string: %s", lpSrc);
        return FALSE;
    }

    while (ISDIGIT(*lpSrc)) {
        n *= 10;
        n += *lpSrc - TEXT('0');
        lpSrc++;
    }

    if (bNeg) {
        *pi = -n;
    }
    else {
        *pi = n;
    }
    return TRUE;
}

#ifndef WINNT
/*******************************************************************************
*
*  DM32IsPCMCIAPresent
*
*  DESCRIPTION:
*   Memphis only test for existence of PCMCIA slots. From Jason Cobb.
*
*  PARAMETERS:
*
*******************************************************************************/

DEFINE_GUID(GUID_DEVICEINTERFACE_PCMCIA,0x4d36e977L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18);

BOOLEAN DM32IsPCMCIAPresent(VOID)
{
    HDEVINFO        hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    BOOLEAN         bRet = FALSE;

    hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVICEINTERFACE_PCMCIA,
                                   NULL, NULL, 0);

    if (hDevInfo != INVALID_HANDLE_VALUE) {
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if (SetupDiEnumDeviceInfo(hDevInfo, 0, &DeviceInfoData)) {
            bRet = TRUE;
        }
        else {
            DebugPrint( "DM32IsPCMCIAPresent, SetupDiEnumDeviceInfo on PCMCIA failed");
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
    else {
        DebugPrint( "DM32IsPCMCIAPresent, SetupDiGetClassDevs on PCMCIA failed");
    }
    return bRet;
}
#endif

/*******************************************************************************
*
*  RegistryInit
*
*  DESCRIPTION:
*   Do DLL load time registry related initialization.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN RegistryInit(PUINT puiLastId)
{
    DWORD               dwSize;
    TCHAR               szNum[NUM_DEC_DIGITS];
    UINT                uiCurPwrScheme;

    // Read in the last ID this value must be present.
    dwSize = sizeof(szNum);
    if (!ReadWritePowerValue(HKEY_LOCAL_MACHINE,
                             c_szREGSTR_PATH_MACHINE_POWERCFG,
                             c_szREGSTR_VAL_LASTID,
                             szNum, &dwSize, FALSE, TRUE) ||
        !MyStrToInt(szNum, &g_uiLastID)) {
        DebugPrint( "RegistryInit, Unable to fetch last ID, registry is corrupt");
        return FALSE;
    }

#ifndef WINNT
    // For Memphis only we check the HKCU current scheme entry. If it's not
    // valid a new entry is written. If the machine has a PCMCIA slot we set
    // Portable as the current scheme, otherwise it's Home/Office. Per RobMCK.

    if (!GetActivePwrScheme(&uiCurPwrScheme)) {
        DebugPrint( "RegistryInit, unable to validate currrent scheme");
        if (DM32IsPCMCIAPresent()) {
            uiCurPwrScheme = 1;
        }
        else {
            uiCurPwrScheme = 0;
        }
        wsprintf(szNum, TEXT("%d"), uiCurPwrScheme);
        DebugPrint( "RegistryInit, attempting to write new current ID: %s", szNum);
        return ReadWritePowerValue(HKEY_CURRENT_USER,
                                   c_szREGSTR_PATH_USER_POWERCFG,
                                   c_szREGSTR_VAL_CURRENTPOWERPOLICY,
                                   szNum, NULL, TRUE, TRUE);
    }
#endif
    return TRUE;
}

#ifdef DEBUG
/*******************************************************************************
*
*  ReadOptionalDebugSettings
*
*  DESCRIPTION:
*   Debug only. Get the debug settings from HKCU registry entries into globals.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID ReadOptionalDebugSettings(VOID)
{
    HKEY     hKeyCurrentUser;

    if (OpenCurrentUser(&hKeyCurrentUser)) {
        // Optional debug logging of PPM policy validation changes.
        ReadPowerIntOptional(hKeyCurrentUser,
                             c_szREGSTR_PATH_USER_POWERCFG,
                             c_szREGSTR_VAL_SHOWVALCHANGES,
                             &g_iShowValidationChanges);

        // Optional debug logging of PPM capabilities.
        ReadPowerIntOptional(hKeyCurrentUser,
                             c_szREGSTR_PATH_USER_POWERCFG,
                             c_szREGSTR_VAL_SHOWCAPABILITIES,
                             &g_iShowCapabilities);

        // Optional debug logging of setting new policy to PPM.
        ReadPowerIntOptional(hKeyCurrentUser,
                             c_szREGSTR_PATH_USER_POWERCFG,
                             c_szREGSTR_VAL_SHOWSETPPM,
                             &g_iShowSetPPM);

        CloseCurrentUser(hKeyCurrentUser);
    }
}
#endif

/*******************************************************************************
*
*  MyCreateSemaphore
*
*  DESCRIPTION:
*   CreateSemaphore .
*
*  PARAMETERS:
*
*******************************************************************************/

HANDLE MyCreateSemaphore(LPCTSTR c_szSem)
{
#ifdef WINNT
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION );

    SetSecurityDescriptorDacl(&sd,
                              TRUE,     // Dacl present
                              NULL,     // NULL Dacl
                              FALSE);   // Not defaulted

    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;
    sa.nLength = sizeof(sa);

    return CreateSemaphore(&sa, 1, 1, c_szSem);
#else
    return CreateSemaphore(NULL, 1, 1, c_szSem);
#endif
}

#ifdef WINNT
/*******************************************************************************
*
*  InitAdmin
*
*  DESCRIPTION:
*   For NT only, initialize an administrator power policy which
*   supports an optional administrative override of certain
*   power policy settings. The PowerCfg.Cpl and PPM will use these
*   override values during validation.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID InitAdmin(PADMINISTRATOR_POWER_POLICY papp)
{
    INT         i;
    NTSTATUS    ntsRetVal;
    HKEY        hKeyCurrentUser;

    if (OpenCurrentUser(&hKeyCurrentUser)) {
        if (ReadPowerIntOptional(hKeyCurrentUser,
                                 c_szREGSTR_PATH_USER_POWERCFG,
                                 c_szREGSTR_VAL_ADMINMAXSLEEP,
                                 &i)) {
                g_app.MaxSleep = (SYSTEM_POWER_STATE) i;
                g_bAdminOverrideActive = TRUE;
        }

        if (ReadPowerIntOptional(hKeyCurrentUser,
                                 c_szREGSTR_PATH_USER_POWERCFG,
                                 c_szREGSTR_VAL_ADMINMAXVIDEOTIMEOUT,
                                 &i)) {
                g_app.MaxVideoTimeout = i;
                g_bAdminOverrideActive = TRUE;
        }
        CloseCurrentUser(hKeyCurrentUser);
    }

    // If an administration override was set, call down to the power
    // policy manager to set the administrator policy.
    if (g_bAdminOverrideActive) {
        ntsRetVal = CallNtPowerInformation(AdministratorPowerPolicy,
                                           &g_app,
                                           sizeof(ADMINISTRATOR_POWER_POLICY),
                                           &g_app,
                                           sizeof(ADMINISTRATOR_POWER_POLICY));
        if (ntsRetVal != STATUS_SUCCESS) {
            DebugPrint( "DllInitialize, Set AdministratorPowerPolicy failed: 0x%08X", ntsRetVal);
        }
    }
}
#endif

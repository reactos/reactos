/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            base/system/winlogon/winlogon.c
 * PURPOSE:         Logon
 * PROGRAMMERS:     Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Filip Navara
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "winlogon.h"

#include <ndk/cmfuncs.h>

/* GLOBALS ******************************************************************/

HINSTANCE hAppInstance;
PWLSESSION WLSession = NULL;

/* FUNCTIONS *****************************************************************/

static
BOOL
StartServicesManager(VOID)
{
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    LPCWSTR ServiceString = L"services.exe";
    BOOL res;

    /* Start the service control manager (services.exe) */
    ZeroMemory(&StartupInfo, sizeof(STARTUPINFOW));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpTitle = NULL;
    StartupInfo.dwFlags = 0;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = 0;

    TRACE("WL: Creating new process - %S\n", ServiceString);

    res = CreateProcessW(ServiceString,
                         NULL,
                         NULL,
                         NULL,
                         FALSE,
                         DETACHED_PROCESS,
                         NULL,
                         NULL,
                         &StartupInfo,
                         &ProcessInformation);
    if (!res)
    {
        ERR("WL: Failed to execute services (error %lu)\n", GetLastError());
        return FALSE;
    }

    TRACE("WL: Created new process - %S\n", ServiceString);

    CloseHandle(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hProcess);

    TRACE("WL: StartServicesManager() done.\n");

    return TRUE;
}


static
BOOL
StartLsass(VOID)
{
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    LPCWSTR ServiceString = L"lsass.exe";
    BOOL res;

    /* Start the local security authority subsystem (lsass.exe) */
    ZeroMemory(&StartupInfo, sizeof(STARTUPINFOW));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpTitle = NULL;
    StartupInfo.dwFlags = 0;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = 0;

    TRACE("WL: Creating new process - %S\n", ServiceString);

    res = CreateProcessW(ServiceString,
                         NULL,
                         NULL,
                         NULL,
                         FALSE,
                         DETACHED_PROCESS,
                         NULL,
                         NULL,
                         &StartupInfo,
                         &ProcessInformation);

    TRACE("WL: Created new process - %S\n", ServiceString);

    CloseHandle(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hProcess);

    return res;
}


static
VOID
WaitForLsass(VOID)
{
    HANDLE hEvent;
    DWORD dwError;

    hEvent = CreateEventW(NULL,
                          TRUE,
                          FALSE,
                          L"LSA_RPC_SERVER_ACTIVE");
    if (hEvent == NULL)
    {
        dwError = GetLastError();
        TRACE("WL: Failed to create the notification event (Error %lu)\n", dwError);

        if (dwError == ERROR_ALREADY_EXISTS)
        {
            hEvent = OpenEventW(SYNCHRONIZE,
                                FALSE,
                                L"LSA_RPC_SERVER_ACTIVE");
            if (hEvent == NULL)
            {
               ERR("WL: Could not open the notification event (Error %lu)\n", GetLastError());
               return;
            }
        }
    }

    TRACE("WL: Wait for the LSA server!\n");
    WaitForSingleObject(hEvent, INFINITE);
    TRACE("WL: LSA server running!\n");

    CloseHandle(hEvent);
}


static
VOID
UpdateTcpIpInformation(VOID)
{
    LONG lError;
    HKEY hKey = NULL;
    DWORD dwType, dwSize;
    PWSTR pszBuffer;
    WCHAR szBuffer[128] = L"";

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                           0,
                           KEY_QUERY_VALUE | KEY_SET_VALUE,
                           &hKey);
    if (lError != ERROR_SUCCESS)
    {
        ERR("WL: RegOpenKeyExW(\"HKLM\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters\") failed (error %lu)\n", lError);
        return;
    }

    /*
     * Read the "NV Hostname" value and copy it into the "Hostname" value.
     */

    pszBuffer = szBuffer;
    dwSize = ARRAYSIZE(szBuffer);

    lError = RegQueryValueExW(hKey,
                              L"NV Hostname",
                              NULL,
                              &dwType,
                              (LPBYTE)pszBuffer,
                              &dwSize);
    if (((lError == ERROR_INSUFFICIENT_BUFFER) || (lError == ERROR_MORE_DATA)) && (dwType == REG_SZ))
    {
        pszBuffer = HeapAlloc(GetProcessHeap(), 0, dwSize);
        if (pszBuffer)
        {
            lError = RegQueryValueExW(hKey,
                                      L"NV Hostname",
                                      NULL,
                                      &dwType,
                                      (LPBYTE)pszBuffer,
                                      &dwSize);
        }
        else
        {
            ERR("WL: Could not reallocate memory for pszBuffer\n");
        }
    }
    if ((lError == ERROR_SUCCESS) && (dwType == REG_SZ))
    {
        TRACE("NV Hostname is '%S'.\n", pszBuffer);

        lError = RegSetValueExW(hKey,
                                L"Hostname",
                                0,
                                REG_SZ,
                                (LPBYTE)pszBuffer,
                                dwSize);
        if (lError != ERROR_SUCCESS)
            ERR("WL: RegSetValueExW(\"Hostname\") failed (error %lu)\n", lError);
    }

    /*
     * Read the "NV Domain" value and copy it into the "Domain" value.
     */

    // pszBuffer = szBuffer;
    // dwSize = ARRAYSIZE(szBuffer);

    lError = RegQueryValueExW(hKey,
                              L"NV Domain",
                              NULL,
                              &dwType,
                              (LPBYTE)pszBuffer,
                              &dwSize);
    if (((lError == ERROR_INSUFFICIENT_BUFFER) || (lError == ERROR_MORE_DATA)) && (dwType == REG_SZ))
    {
        if (pszBuffer != szBuffer)
        {
            PWSTR pszNewBuffer;
            pszNewBuffer = HeapReAlloc(GetProcessHeap(), 0, pszBuffer, dwSize);
            if (pszNewBuffer)
            {
                pszBuffer = pszNewBuffer;
            }
            else
            {
                HeapFree(GetProcessHeap(), 0, pszBuffer);
                pszBuffer = NULL;
            }
        }
        else
        {
            pszBuffer = HeapAlloc(GetProcessHeap(), 0, dwSize);
        }
        if (pszBuffer)
        {
            lError = RegQueryValueExW(hKey,
                                      L"NV Domain",
                                      NULL,
                                      &dwType,
                                      (LPBYTE)pszBuffer,
                                      &dwSize);
        }
        else
        {
            ERR("WL: Could not reallocate memory for pszBuffer\n");
        }
    }
    if ((lError == ERROR_SUCCESS) && (dwType == REG_SZ))
    {
        TRACE("NV Domain is '%S'.\n", pszBuffer);

        lError = RegSetValueExW(hKey,
                                L"Domain",
                                0,
                                REG_SZ,
                                (LPBYTE)pszBuffer,
                                dwSize);
        if (lError != ERROR_SUCCESS)
            ERR("WL: RegSetValueExW(\"Domain\") failed (error %lu)\n", lError);
    }

    if (pszBuffer != szBuffer)
        HeapFree(GetProcessHeap(), 0, pszBuffer);

    RegCloseKey(hKey);
}


static
BOOL
InitKeyboardLayouts(VOID)
{
    WCHAR wszKeyName[12], wszKLID[10];
    DWORD dwSize = sizeof(wszKLID), dwType, i = 1;
    HKEY hKey;
    UINT Flags;
    BOOL bRet = FALSE;

    /* Open registry key with preloaded layouts */
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        while (TRUE)
        {
            /* Read values with integer names only */
            swprintf(wszKeyName, L"%d", i++);
            if (RegQueryValueExW(hKey, wszKeyName, NULL, &dwType, (LPBYTE)wszKLID, &dwSize) != ERROR_SUCCESS)
            {
                /* There is no more entries */
                break;
            }

            /* Only REG_SZ values are valid */
            if (dwType != REG_SZ)
            {
                ERR("Wrong type: %ws!\n", wszKLID);
                continue;
            }

            /* Load keyboard layout with given locale id */
            Flags = KLF_SUBSTITUTE_OK;
            if (i > 1)
                Flags |= KLF_NOTELLSHELL|KLF_REPLACELANG;
            else // First layout
                Flags |= KLF_ACTIVATE; // |0x40000000
            if (!LoadKeyboardLayoutW(wszKLID, Flags))
            {
                ERR("LoadKeyboardLayoutW(%ws) failed!\n", wszKLID);
                continue;
            }
            else
            {
                /* We loaded at least one layout - success */
                bRet = TRUE;
            }
        }

        /* Close the key now */
        RegCloseKey(hKey);
    }
    else
        WARN("RegOpenKeyExW(Keyboard Layout\\Preload) failed!\n");

    if (!bRet)
    {
        /* If we failed, load US keyboard layout */
        if (LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE | KLF_SUBSTITUTE_OK | KLF_REPLACELANG | KLF_SETFORPROCESS))
            bRet = TRUE;
    }

    return bRet;
}


BOOL
DisplayStatusMessage(
     IN PWLSESSION Session,
     IN HDESK hDesktop,
     IN UINT ResourceId)
{
    WCHAR StatusMsg[MAX_PATH];

    if (Session->Gina.Version < WLX_VERSION_1_3)
        return TRUE;

    if (Session->SuppressStatus)
        return TRUE;

    if (LoadStringW(hAppInstance, ResourceId, StatusMsg, MAX_PATH) == 0)
        return FALSE;

    return Session->Gina.Functions.WlxDisplayStatusMessage(Session->Gina.Context, hDesktop, 0, NULL, StatusMsg);
}


BOOL
RemoveStatusMessage(
    IN PWLSESSION Session)
{
    if (Session->Gina.Version < WLX_VERSION_1_3)
        return TRUE;

    return Session->Gina.Functions.WlxRemoveStatusMessage(Session->Gina.Context);
}


static
INT_PTR
CALLBACK
GinaLoadFailedWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndDlg, IDOK);
                    return TRUE;
            }
            break;
        }

        case WM_INITDIALOG:
        {
            int len;
            WCHAR templateText[MAX_PATH], text[MAX_PATH];

            len = GetDlgItemTextW(hwndDlg, IDC_GINALOADFAILED, templateText, MAX_PATH);
            if (len)
            {
                wsprintfW(text, templateText, (LPWSTR)lParam);
                SetDlgItemTextW(hwndDlg, IDC_GINALOADFAILED, text);
            }

            SetFocus(GetDlgItem(hwndDlg, IDOK));
            return TRUE;
        }

        case WM_CLOSE:
        {
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
    }

    return FALSE;
}


int
WINAPI
WinMain(
    IN HINSTANCE hInstance,
    IN HINSTANCE hPrevInstance,
    IN LPSTR lpCmdLine,
    IN int nShowCmd)
{
#if 0
    LSA_STRING ProcessName, PackageName;
    HANDLE LsaHandle;
    LSA_OPERATIONAL_MODE Mode;
    BOOLEAN Old;
    ULONG AuthenticationPackage;
    NTSTATUS Status;
#endif
    ULONG HardErrorResponse;
    MSG Msg;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    hAppInstance = hInstance;

    /* Make us critical */
    RtlSetProcessIsCritical(TRUE, NULL, FALSE);
    RtlSetThreadIsCritical(TRUE, NULL, FALSE);

    /* Update the cached TCP/IP Information in the registry */
    UpdateTcpIpInformation();

    if (!RegisterLogonProcess(GetCurrentProcessId(), TRUE))
    {
        ERR("WL: Could not register logon process\n");
        NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, NULL, OptionOk, &HardErrorResponse);
        ExitProcess(1);
    }

    WLSession = (PWLSESSION)HeapAlloc(GetProcessHeap(), 0, sizeof(WLSESSION));
    if (!WLSession)
    {
        ERR("WL: Could not allocate memory for winlogon instance\n");
        NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, NULL, OptionOk, &HardErrorResponse);
        ExitProcess(1);
    }

    ZeroMemory(WLSession, sizeof(WLSESSION));
    WLSession->DialogTimeout = 120; /* 2 minutes */

    /* Initialize the dialog tracking list */
    InitDialogListHead();

    if (!CreateWindowStationAndDesktops(WLSession))
    {
        ERR("WL: Could not create window station and desktops\n");
        NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, NULL, OptionOk, &HardErrorResponse);
        ExitProcess(1);
    }

    LockWorkstation(WLSession);

    /* Load default keyboard layouts */
    if (!InitKeyboardLayouts())
    {
        ERR("WL: Could not preload keyboard layouts\n");
        NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, NULL, OptionOk, &HardErrorResponse);
        ExitProcess(1);
    }

    if (!StartRpcServer())
    {
        ERR("WL: Could not start the RPC server\n");
        NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, NULL, OptionOk, &HardErrorResponse);
        ExitProcess(1);
    }

    if (!StartServicesManager())
    {
        ERR("WL: Could not start services.exe\n");
        NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, NULL, OptionOk, &HardErrorResponse);
        ExitProcess(1);
    }

    if (!StartLsass())
    {
        ERR("WL: Failed to start lsass.exe service (error %lu)\n", GetLastError());
        NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, NULL, OptionOk, &HardErrorResponse);
        ExitProcess(1);
    }

    /* Wait for the LSA server */
    WaitForLsass();

    /* Init Notifications */
    InitNotifications();

    /* Check for pending setup: if so, run it and reboot when done */
__debugbreak();
    g_setupType = GetSetupType();
    // CheckForSetup();

    /* Load and initialize gina */
    if (!GinaInit(WLSession))
    {
        ERR("WL: Failed to initialize Gina\n");
        // FIXME: Retrieve the real name of the GINA DLL we were trying to load.
        // It is known only inside the GinaInit function...
        DialogBoxParam(hAppInstance, MAKEINTRESOURCE(IDD_GINALOADFAILED), GetDesktopWindow(), GinaLoadFailedWindowProc, (LPARAM)L"msgina.dll");
        HandleShutdown(WLSession, WLX_SAS_ACTION_SHUTDOWN_REBOOT);
        ExitProcess(1);
    }

    DisplayStatusMessage(WLSession, WLSession->WinlogonDesktop, IDS_REACTOSISSTARTINGUP);

#if 0
    /* Connect to NetLogon service (lsass.exe) */
    /* Real winlogon uses "Winlogon" */
    RtlInitUnicodeString((PUNICODE_STRING)&ProcessName, L"Winlogon");
    Status = LsaRegisterLogonProcess(&ProcessName, &LsaHandle, &Mode);
    if (Status == STATUS_PORT_CONNECTION_REFUSED)
    {
        /* Add the 'SeTcbPrivilege' privilege and try again */
        Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, TRUE, &Old);
        if (!NT_SUCCESS(Status))
        {
            ERR("RtlAdjustPrivilege() failed with error %lu\n", LsaNtStatusToWinError(Status));
            return 1;
        }

        Status = LsaRegisterLogonProcess(&ProcessName, &LsaHandle, &Mode);
    }

    if (!NT_SUCCESS(Status))
    {
        ERR("LsaRegisterLogonProcess() failed with error %lu\n", LsaNtStatusToWinError(Status));
        return 1;
    }

    RtlInitUnicodeString((PUNICODE_STRING)&PackageName, MICROSOFT_KERBEROS_NAME_W);
    Status = LsaLookupAuthenticationPackage(LsaHandle, &PackageName, &AuthenticationPackage);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaLookupAuthenticationPackage() failed with error %lu\n", LsaNtStatusToWinError(Status));
        LsaDeregisterLogonProcess(LsaHandle);
        return 1;
    }
#endif

    CallNotificationDlls(WLSession, StartupHandler);

    /* Create a hidden window to get SAS notifications */
    if (!InitializeSAS(WLSession))
    {
        ERR("WL: Failed to initialize SAS\n");
        ExitProcess(2);
    }

    // DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_PREPARENETWORKCONNECTIONS);
    // DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_APPLYINGCOMPUTERSETTINGS);

    /* Display logged out screen */
    WLSession->LogonState = STATE_INIT;
    RemoveStatusMessage(WLSession);


    /* Check for pending setup: if so, run it and reboot when done */
__debugbreak();
    // g_setupType = GetSetupType();
    /*****CheckForSetup();******/


    /*
     * Simulate an initial Ctrl-Alt-Del key press.
     * FIXME: This should quite probably be done only if some registry
     * setting is set. However, since our current Ctrl-Alt-Del hotkey
     * handling in Win32k is broken, we need to use this hack!
     */
    // PostMessageW(WLSession->SASWindow, WLX_WM_SAS, WLX_SAS_TYPE_CTRL_ALT_DEL, 0);

    (void)LoadLibraryW(L"sfc_os.dll");

    /* Tell kernel that CurrentControlSet is good (needed
     * to support Last good known configuration boot) */
    NtInitializeRegistry(CM_BOOT_FLAG_ACCEPTED | 1);

    /* Message loop for the SAS window */
    while (GetMessageW(&Msg, WLSession->SASWindow, 0, 0))
    {
        TranslateMessage(&Msg);
        DispatchMessageW(&Msg);
    }

    CleanupNotifications();

    /* We never go there */

    return 0;
}

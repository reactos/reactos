/*
 * PROJECT:     ReactOS System Setup
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 *              or GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Addon installation management
 * COPYRIGHT:   Copyright 2026 Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "precomp.h"

#undef LF_FACESIZE
#include <shlobj.h>
#include <shlwapi.h>

typedef enum _RappsConsent {
    NOT_ASKED,
    APPROVED,
    DENIED
} RappsConsent;

typedef struct _ADDON_INSTALL_DATA
{
    PCWSTR Title;
    PCWSTR AddonPath;
    PCWSTR CreateProcessFormatString;
    PCWSTR RappsId;
} ADDON_INSTALL_DATA, *PADDON_INSTALL_DATA;
typedef const ADDON_INSTALL_DATA* PCADDON_INSTALL_DATA;

/* TODO: Move this out of code and into an .inf or something. */
static const ADDON_INSTALL_DATA Addons[] = {
#ifdef _M_IX86
    {L"Wine Gecko", L"%SystemRoot%\\wine_gecko-2.40-x86.msi", L"msiexec.exe /i \"%s\" /qn /norestart", L"gecko"},
    {L"WineVDM", L"%SystemRoot%\\winevdm_setup.exe", L"\"%s\" /VERYSILENT", L"winevdm"},
#endif
    {NULL, NULL, NULL}
};

HRESULT
RunCommandAndWait(
    _In_ PWCHAR Command)
{
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    DWORD ExitCode = 0;

    if (CreateProcessW(NULL, Command, NULL, NULL, FALSE,
                       0, NULL, NULL, &si, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &ExitCode);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        if (ExitCode == 0)
            return S_OK;
    }

    return HRESULT_FROM_WIN32(GetLastError());
}

static HRESULT
InstallAddon(
    _In_ PCADDON_INSTALL_DATA pInstallData,
    _Inout_ RappsConsent* Consent,
    _In_ PITEMSDATA pItemsData,
    _Inout_ PINSTALLITEM_NOTIFY pNotify)
{
    HRESULT hr;
    WCHAR Command[MAX_PATH], ExpandedAddonPath[MAX_PATH];

    pNotify->Progress++;
    pNotify->CurrentItem = pInstallData->Title;
    SendMessage(pItemsData->hwndDlg, PM_STEP_START, 0, (LPARAM)pNotify);

    ExpandEnvironmentStringsW(pInstallData->AddonPath,
                              ExpandedAddonPath,
                              ARRAYSIZE(ExpandedAddonPath));

    /* Attempt to install addon from local installer. */
    if (!DoesFileExist(ExpandedAddonPath))
        goto rapps_install;

    hr = StringCchPrintfW(Command, ARRAYSIZE(Command),
                          pInstallData->CreateProcessFormatString, ExpandedAddonPath);
    if (!SUCCEEDED(hr))
        goto done;

    hr = RunCommandAndWait(Command);
    if (SUCCEEDED(hr))
    {
        /* We successfully installed the addon locally! Try removing it from disk and finish. */
        DeleteFileW(ExpandedAddonPath);
        goto done;
    }

rapps_install:
    /* Local installer doesn't exist or failed. Try installing through Rapps. */
    if (*Consent == NOT_ASKED)
    {
        WCHAR szMessage[256], szCaption[64];
        LoadStringW(hDllInstance, IDS_INSTALLADDONSMESSAGE, szMessage, ARRAYSIZE(szMessage));
        LoadStringW(hDllInstance, IDS_INSTALLADDONSCAPTION, szCaption, ARRAYSIZE(szCaption));
        int MsgBox = MessageBoxW(NULL, szMessage, szCaption,
                                 MB_YESNO | MB_ICONINFORMATION);
        *Consent = (MsgBox == IDYES) ? APPROVED : DENIED;
    }

    if (*Consent == DENIED)
    {
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        goto done;
    }

    hr = StringCchPrintfW(Command, ARRAYSIZE(Command), L"rapps.exe /install /S %s", pInstallData->RappsId);
    if (!SUCCEEDED(hr))
        goto done;

    hr = RunCommandAndWait(Command);

done:
    /* We don't set an error in pNotify because the user chose to cancel rapps install.
     * Showing an error may make the user think the installation of the operating system failed. */
    if (!SUCCEEDED(hr) && hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
        pNotify->LastError = HRESULT_CODE(hr);
    SendMessage(pItemsData->hwndDlg, PM_STEP_END, 0, (LPARAM)pNotify);
    return hr;
}

HRESULT
InstallOptionalComponents(
    _In_ PITEMSDATA pItemsData)
{
    HRESULT hr = S_OK;
    PSETUPDATA pSetupData = pItemsData->pSetupData;
    INSTALLITEM_NOTIFY Notify = {0};
    RappsConsent Consent = NOT_ASKED;

    /* The last element in Addons is null, don't count it as a step. */
    SendMessage(pItemsData->hwndDlg, PM_ITEM_START, 2, (LPARAM)(ARRAYSIZE(Addons) - 1));

    if (pSetupData->UnattendSetup)
        Consent = pSetupData->RappsDownload ? APPROVED : DENIED;

    for (DWORD i = 0; i < ARRAYSIZE(Addons); i++)
    {
        if (Addons[i].AddonPath == NULL ||
            Addons[i].CreateProcessFormatString == NULL ||
            Addons[i].RappsId == NULL)
        {
            continue;
        }

        hr = InstallAddon(&Addons[i], &Consent, pItemsData, &Notify);

        /* Cancelling rapps install is not an error. Reset hr to S_OK. */
        if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            hr = S_OK;

        if (!SUCCEEDED(hr))
        {
            WCHAR szMessage[256], szCaption[64];
            LoadStringW(hDllInstance, IDS_INSTALLADDONSFAILEDMESSAGE, szMessage, ARRAYSIZE(szMessage));
            LoadStringW(hDllInstance, IDS_INSTALLADDONSCAPTION, szCaption, ARRAYSIZE(szCaption));
            MessageBoxW(NULL, szMessage, szCaption,
                        MB_OK | MB_ICONWARNING | MB_TOPMOST);
            break;
        }
    }

    SendMessage(pItemsData->hwndDlg, PM_ITEM_END, 2, HRESULT_CODE(hr));
    return hr;
}

/**
 * @brief
 * Cleans up all temporary files and directories used for add-ons download and installation.
 *
 * These are the RAPPS default Download directory and its settings directory,
 * see base/applications/rapps/appdb.cpp and settings.cpp files.
 **/
BOOL
CleanupAddonsTempFiles(VOID)
{
#if 0
    HKEY hKey;
    LONG rc;
    DWORD dwType, dwSize;
#endif
    WCHAR szTempDir[MAX_PATH];

#if 0
    /* Open the RAPPS settings key and retrieve the download directory value */
    rc = RegOpenKeyExW(HKEY_CURRENT_USER,
                       L"Software\\ReactOS\\RApps",
                       0,
                       KEY_QUERY_VALUE,
                       &hKey);
    if (rc != ERROR_SUCCESS)
    {
        //DPRINT1("RegOpenKeyExW failed (Error %lu)\n", rc);
        return FALSE;
    }

    rc = RegQueryValueExW(hKey,
                          L"szDownloadDir",
                          NULL,
                          &dwType,
                          (PBYTE)pszPath, // NULL,
                          &dwSize);
    if (rc != ERROR_SUCCESS ||
        (dwType != REG_SZ && dwType != REG_EXPAND_SZ) ||
        dwSize == 0 ||
        dwSize % sizeof(WCHAR) != 0 /**/ ||
        (dwSize > cchPathSize * sizeof(WCHAR)) /**/)
    {
        if (cchPathSize > 1)
            *pszPath = UNICODE_NULL;
        goto done;
    }

#if 0
    /* Reserve space for data */
    Buffer = HeapAlloc(GetProcessHeap(), 0, dwSize);
    if (!Buffer)
        goto done;
    ZeroMemory(Buffer, dwSize);

    rc = RegQueryValueExW(hKey,
                          L"szDownloadDir",
                          NULL,
                          NULL,
                          (PBYTE)Buffer,
                          &dwSize);
    if (rc != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, Buffer);
        dwSize = 0;
    }
#endif

done:
    RegCloseKey(hKey);
    return (rc == ERROR_SUCCESS);
#else

    // FIXME: Since there's currently no existing way RAPPS could tell us
    // which paths it used to store its temporary files, nor any switch to
    // request it to cleanup these files, we hardcode below the different
    // places where RAPPS is known to store its files by default, and we
    // clean those places.

    /* Cleanup the RAPPS default Download directory */
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szTempDir)))
    {
        DWORD dwSize = GetEnvironmentVariableW(L"SystemDrive", szTempDir, _countof(szTempDir));
        if ((dwSize == 0) || (dwSize > _countof(szTempDir)))
            wcscpy(szTempDir, L"C:");
    }
    if (PathAppendW(szTempDir, L"\\RAPPS Downloads"))
        RemoveDirectoryPath(szTempDir);

    /* Cleanup the RAPPS settings directory (that also contains the downloaded database directory) */
    if (SHGetSpecialFolderPathW(NULL, szTempDir, CSIDL_LOCAL_APPDATA, TRUE) &&
        PathAppendW(szTempDir, L"RApps" /*RAPPS_NAME*/))
    {
        RemoveDirectoryPath(szTempDir);
    }

    // TODO: Should we want to delete the "HKLM\\SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\ReactOS Application Manager"
    // registry key?

    return TRUE;
#endif
}

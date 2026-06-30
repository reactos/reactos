/*
 * PROJECT:     ReactOS System Setup
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 *              or GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Addon installation management
 * COPYRIGHT:   Copyright 2026 Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "precomp.h"
#include <strsafe.h>

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

BOOL DoesFileExist(
    _In_ PCWSTR path)
{
    DWORD attr = GetFileAttributesW(path);
    return (attr != INVALID_FILE_ATTRIBUTES &&
            !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

HRESULT
InstallAddon(
    _In_    PCADDON_INSTALL_DATA pInstallData,
    _Inout_ RappsConsent* Consent,
    _In_    PITEMSDATA pItemsData,
    _Inout_ PREGISTRATIONNOTIFY pNotify)
{
    HRESULT hr;
    WCHAR Command[MAX_PATH], ExpandedAddonPath[MAX_PATH];
    WCHAR szMessage[256], szCaption[64];

    pNotify->Progress++;
    pNotify->CurrentItem = pInstallData->Title;
    SendMessage(pItemsData->hwndDlg, PM_STEP_START, 3, (LPARAM)pNotify);

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
        LoadStringW(hDllInstance, IDS_INSTALLADDONSMESSAGE, szMessage, ARRAYSIZE(szMessage));
        LoadStringW(hDllInstance, IDS_INSTALLADDONSCAPTION, szCaption, ARRAYSIZE(szCaption));
        int MsgBox = MessageBoxW(NULL,
                                 szMessage,
                                 szCaption,
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
    SendMessage(pItemsData->hwndDlg, PM_STEP_END, 3, (LPARAM)pNotify);
    return hr;
}

HRESULT
InstallOptionalComponents(
    _In_ PITEMSDATA pItemsData)
{
    HRESULT hr;
    PSETUPDATA pSetupData;
    WCHAR szMessage[256], szCaption[64];
    REGISTRATIONNOTIFY Notify = { 0 };
    RappsConsent Consent = NOT_ASKED;

    /* The last element in Addons is null, don't count it as a step. */
    SendMessage(pItemsData->hwndDlg, PM_ITEM_START, 3, (LPARAM)(ARRAYSIZE(Addons) - 1));
    pSetupData = (PSETUPDATA)GetWindowLongPtr(pItemsData->hwndDlg, GWLP_USERDATA);

    if (pSetupData->UnattendSetup)
        Consent = pSetupData->RappsDownload ? APPROVED : DENIED;

    for (DWORD i = 0; i < ARRAYSIZE(Addons); i++)
    {
        if (Addons[i].AddonPath == NULL
            || Addons[i].CreateProcessFormatString == NULL
            || Addons[i].RappsId == NULL)
            continue;

        hr = InstallAddon(&Addons[i], &Consent, pItemsData, &Notify);

        /* Cancelling rapps install is not an error. Reset hr to S_OK. */
        if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            hr = S_OK;

        if (!SUCCEEDED(hr))
        {
            LoadStringW(hDllInstance, IDS_INSTALLADDONSFAILEDMESSAGE, szMessage, ARRAYSIZE(szMessage));
            LoadStringW(hDllInstance, IDS_INSTALLADDONSCAPTION, szCaption, ARRAYSIZE(szCaption));

            MessageBoxW(NULL,
                        szMessage,
                        szCaption,
                        MB_OK | MB_ICONWARNING | MB_TOPMOST);

            break;
        }
    }

    SendMessage(pItemsData->hwndDlg, PM_ITEM_END, 3, HRESULT_CODE(hr));
    return hr;
}

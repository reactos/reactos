/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/hardprof.c
 * PURPOSE:     Modify hardware profiles
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

#include <debug.h>

#define PROFILE_NAME_LENGTH 80

typedef struct _PROFILE
{
    WCHAR szFriendlyName[PROFILE_NAME_LENGTH];
    WCHAR szName[5];
    DWORD dwProfileNumber;
    DWORD dwPreferenceOrder;
} PROFILE, *PPROFILE;

typedef struct _PROFILEDATA
{
    DWORD dwProfileCount;
    DWORD dwLastProfile;
    DWORD dwSelectedProfile;
    DWORD dwSelectedProfileIndex;
    PPROFILE pProfiles;
} PROFILEDATA, *PPROFILEDATA;

typedef struct _PROFILENAMES
{
    WCHAR szSourceName[PROFILE_NAME_LENGTH];
    WCHAR szDestinationName[PROFILE_NAME_LENGTH];
    PPROFILEDATA pProfileData;
} PROFILENAMES, *PPROFILENAMES;


static
BOOL
IsProfileNameInUse(
    PPROFILENAMES pProfileNames,
    BOOL bIgnoreCurrent)
{
    DWORD i;

    for (i = 0; i < pProfileNames->pProfileData->dwProfileCount; i++)
    {
        if (bIgnoreCurrent == TRUE && i == pProfileNames->pProfileData->dwSelectedProfileIndex)
            continue;

        if (wcscmp(pProfileNames->pProfileData->pProfiles[i].szFriendlyName, pProfileNames->szDestinationName) == 0)
            return TRUE;
    }

    return FALSE;
}


static
INT_PTR
CALLBACK
CopyProfileDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PPROFILENAMES pProfileNames;

    pProfileNames = (PPROFILENAMES)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
            pProfileNames = (PPROFILENAMES)lParam;

            /* Set the old name */
            SetDlgItemText(hwndDlg, IDC_COPYPROFILEFROM, pProfileNames->szSourceName);

            /* Set the new name */
            SendDlgItemMessageW(hwndDlg, IDC_COPYPROFILETO, EM_SETLIMITTEXT, PROFILE_NAME_LENGTH - 1, 0);
            SetDlgItemText(hwndDlg, IDC_COPYPROFILETO, pProfileNames->szDestinationName);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    GetDlgItemText(hwndDlg,
                                   IDC_COPYPROFILETO,
                                   pProfileNames->szDestinationName,
                                   PROFILE_NAME_LENGTH);
                    if (IsProfileNameInUse(pProfileNames, FALSE))
                        ResourceMessageBox(hApplet,
                                           NULL,
                                           MB_OK | MB_ICONERROR,
                                           IDS_HWPROFILE_WARNING,
                                           IDS_HWPROFILE_ALREADY_IN_USE);
                    else
                        EndDialog(hwndDlg, IDOK);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}


static
VOID
CopyHardwareProfile(
    HWND hwndDlg,
    PPROFILEDATA pProfileData)
{
    PROFILENAMES ProfileNames;
    PPROFILE pProfile, pNewProfiles, pNewProfile;
    WCHAR szBuffer[80];

    pProfile = &pProfileData->pProfiles[pProfileData->dwSelectedProfileIndex];

    LoadStringW(hApplet, IDS_HWPROFILE_PROFILE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR));

    wcscpy(ProfileNames.szSourceName, pProfile->szFriendlyName);
    swprintf(ProfileNames.szDestinationName, L"%s %lu", szBuffer, pProfileData->dwProfileCount);

    ProfileNames.pProfileData = pProfileData;

    if (DialogBoxParam(hApplet,
                       MAKEINTRESOURCE(IDD_COPYPROFILE),
                       hwndDlg,
                       (DLGPROC)CopyProfileDlgProc,
                       (LPARAM)&ProfileNames) != IDOK)
        return;

    /* Apply new name only if it has been changed */
    if (wcscmp(ProfileNames.szSourceName, ProfileNames.szDestinationName) == 0)
        return;

    /* Allocate memory for the new profile */
    pNewProfiles = HeapReAlloc(GetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               pProfileData->pProfiles,
                               (pProfileData->dwProfileCount + 1) * sizeof(PROFILE));
    if (pNewProfiles == NULL)
    {
        DPRINT1("HeapReAlloc() failed!\n");
        return;
    }

    pProfileData->dwProfileCount++;
    pProfileData->pProfiles = pNewProfiles;

    pNewProfile = &pProfileData->pProfiles[pProfileData->dwProfileCount - 1];

    CopyMemory(pNewProfile, pProfile, sizeof(PROFILE));

    wcscpy(pNewProfile->szFriendlyName, ProfileNames.szDestinationName);

    pNewProfile->dwProfileNumber = ++pProfileData->dwLastProfile;
    swprintf(pNewProfile->szName, L"%04lu", pNewProfile->dwProfileNumber);

    pNewProfile->dwPreferenceOrder = pNewProfile->dwProfileNumber;

    SendDlgItemMessageW(hwndDlg, IDC_HRDPROFLSTBOX, LB_ADDSTRING, 0, (LPARAM)pNewProfile->szFriendlyName);
}


static
INT_PTR
CALLBACK
RenameProfileDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PPROFILENAMES pProfileNames;

    pProfileNames = (PPROFILENAMES)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
            pProfileNames = (PPROFILENAMES)lParam;

            /* Set the old name */
            SetDlgItemText(hwndDlg, IDC_RENPROFEDITFROM, pProfileNames->szSourceName);

            /* Set the new name */
            SendDlgItemMessageW(hwndDlg, IDC_RENPROFEDITTO, EM_SETLIMITTEXT, PROFILE_NAME_LENGTH - 1, 0);
            SetDlgItemText(hwndDlg, IDC_RENPROFEDITTO, pProfileNames->szDestinationName);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    GetDlgItemText(hwndDlg,
                                   IDC_RENPROFEDITTO,
                                   pProfileNames->szDestinationName,
                                   PROFILE_NAME_LENGTH);
                    if (IsProfileNameInUse(pProfileNames, TRUE))
                        ResourceMessageBox(hApplet,
                                           NULL,
                                           MB_OK | MB_ICONERROR,
                                           IDS_HWPROFILE_WARNING,
                                           IDS_HWPROFILE_ALREADY_IN_USE);
                    else
                        EndDialog(hwndDlg, IDOK);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}


static
VOID
RenameHardwareProfile(
    HWND hwndDlg,
    PPROFILEDATA pProfileData)
{
    PROFILENAMES ProfileNames;
    PPROFILE pProfile;
    WCHAR szBuffer[80];

    pProfile = &pProfileData->pProfiles[pProfileData->dwSelectedProfileIndex];

    LoadStringW(hApplet, IDS_HWPROFILE_PROFILE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR));

    wcscpy(ProfileNames.szSourceName, pProfile->szFriendlyName);
    swprintf(ProfileNames.szDestinationName, L"%s %lu", szBuffer, pProfileData->dwProfileCount);

    ProfileNames.pProfileData = pProfileData;

    if (DialogBoxParam(hApplet,
                       MAKEINTRESOURCE(IDD_RENAMEPROFILE),
                       hwndDlg,
                       (DLGPROC)RenameProfileDlgProc,
                       (LPARAM)&ProfileNames) != IDOK)
        return;

    /* Apply new name only if it has been changed */
    if (wcscmp(pProfile->szFriendlyName, ProfileNames.szDestinationName) == 0)
        return;

    /* Replace the profile name in the profile list */
    wcscpy(pProfile->szFriendlyName, ProfileNames.szDestinationName);

    /* Replace the profile name in the listbox */
    SendDlgItemMessageW(hwndDlg, IDC_HRDPROFLSTBOX, LB_DELETESTRING, pProfileData->dwSelectedProfileIndex, 0);
    SendDlgItemMessageW(hwndDlg, IDC_HRDPROFLSTBOX, LB_INSERTSTRING, pProfileData->dwSelectedProfileIndex, (LPARAM)pProfile->szFriendlyName);
}


static
VOID
DeleteHardwareProfile(
    HWND hwndDlg,
    PPROFILEDATA pProfileData)
{
    WCHAR szMessage[256];
    WCHAR szBuffer[128];
    WCHAR szCaption[80];
    PPROFILE pProfiles;
    PPROFILE pProfile;

    pProfile = &pProfileData->pProfiles[pProfileData->dwSelectedProfileIndex];

    LoadStringW(hApplet, IDS_HWPROFILE_CONFIRM_DELETE_TITLE, szCaption, sizeof(szCaption) / sizeof(WCHAR));
    LoadStringW(hApplet, IDS_HWPROFILE_CONFIRM_DELETE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR));
    swprintf(szMessage, szBuffer, pProfile->szFriendlyName);

    if (MessageBox(NULL,
                   szMessage,
                   szCaption,
                   MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    SendDlgItemMessageW(hwndDlg, IDC_HRDPROFLSTBOX, LB_DELETESTRING, pProfileData->dwSelectedProfileIndex, 0);

    if (pProfileData->dwSelectedProfileIndex != pProfileData->dwProfileCount - 1)
    {
        RtlMoveMemory(&pProfileData->pProfiles[pProfileData->dwSelectedProfileIndex],
                      &pProfileData->pProfiles[pProfileData->dwSelectedProfileIndex + 1],
                      (pProfileData->dwProfileCount - pProfileData->dwSelectedProfileIndex - 1) * sizeof(PPROFILE));
    }
    else
    {
        pProfileData->dwSelectedProfileIndex--;
    }

    pProfiles = HeapReAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            pProfileData->pProfiles,
                            (pProfileData->dwProfileCount - 1) * sizeof(PROFILE));
    if (pProfiles == NULL)
    {
        DPRINT1("HeapReAlloc() failed!\n");
        return;
    }

    pProfileData->dwProfileCount--;
    pProfileData->pProfiles = pProfiles;

    SendDlgItemMessageW(hwndDlg, IDC_HRDPROFLSTBOX, LB_SETCURSEL, pProfileData->dwSelectedProfileIndex, 0);
}


static
DWORD
GetUserWaitInterval(VOID)
{
    DWORD dwWaitInterval = 30;
    DWORD dwSize;
    HKEY hKey;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"System\\CurrentControlSet\\Control\\IDConfigDB",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey))
        return dwWaitInterval;

    dwSize = sizeof(DWORD);
    RegQueryValueExW(hKey,
                     L"UserWaitInterval",
                     NULL,
                     NULL,
                     (LPBYTE)&dwWaitInterval,
                     &dwSize);

    RegCloseKey(hKey);

    return dwWaitInterval;
}


static
VOID
SetUserWaitInterval(DWORD dwWaitInterval)
{
    HKEY hKey;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"System\\CurrentControlSet\\Control\\IDConfigDB",
                      0,
                      KEY_SET_VALUE,
                      &hKey))
        return;

    RegSetValueExW(hKey,
                   L"UserWaitInterval",
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwWaitInterval,
                   sizeof(DWORD));

    RegCloseKey(hKey);
}


static
BOOL
GetProfileCount(LPDWORD lpProfileCount)
{
    HKEY hKey;
    LONG lError;

    *lpProfileCount = 0;

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"System\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles",
                           0,
                           KEY_READ,
                           &hKey);
    if (lError != ERROR_SUCCESS)
        return FALSE;

    lError = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, lpProfileCount,
                              NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    RegCloseKey(hKey);

    if (lError != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}


static
VOID
GetProfile(
    HWND hwndDlg,
    HKEY hKey,
    LPWSTR lpName,
    DWORD dwProfileNumber,
    PPROFILE pProfile)
{
    HKEY hProfileKey;
    DWORD dwSize;
    LONG lError;

    lError = RegOpenKeyExW(hKey,
                           lpName,
                           0,
                           KEY_READ,
                           &hProfileKey);
    if (lError != ERROR_SUCCESS)
        return;

    dwSize = PROFILE_NAME_LENGTH * sizeof(WCHAR);
    lError = RegQueryValueExW(hProfileKey,
                              L"FriendlyName",
                              NULL,
                              NULL,
                              (LPBYTE)pProfile->szFriendlyName,
                              &dwSize);
    if (lError == ERROR_SUCCESS)
    {
        DPRINT1("Profile: %S\n", pProfile->szFriendlyName);
    }

    dwSize = sizeof(DWORD);
    lError = RegQueryValueExW(hProfileKey,
                              L"PreferenceOrder",
                              NULL,
                              NULL,
                              (LPBYTE)pProfile->dwPreferenceOrder,
                              &dwSize);
    if (lError == ERROR_SUCCESS)
    {
        DPRINT1("PreferenceOrder: %lu\n", pProfile->dwPreferenceOrder);
    }

    pProfile->dwProfileNumber = dwProfileNumber;

    SendDlgItemMessageW(hwndDlg, IDC_HRDPROFLSTBOX, LB_ADDSTRING, 0, (LPARAM)pProfile->szFriendlyName);

    RegCloseKey(hProfileKey);
}


static
BOOL
GetProfiles(HWND hwndDlg)
{
    PPROFILEDATA pProfileData;
    WCHAR szName[8];
    DWORD dwNameLength;
    DWORD dwProfileNumber;
    DWORD dwIndex = 0;
    HKEY hKey;
    LONG lError;

    pProfileData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PROFILEDATA));
    if (pProfileData == NULL)
        return FALSE;

    pProfileData->dwLastProfile = (DWORD)-1;
    pProfileData->dwSelectedProfileIndex = (DWORD)-1;

    if (!GetProfileCount(&pProfileData->dwProfileCount))
    {
        HeapFree(GetProcessHeap(), 0, pProfileData);
        return FALSE;
    }

    pProfileData->pProfiles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        pProfileData->dwProfileCount * sizeof(PROFILE));
    if (pProfileData->pProfiles == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pProfileData);
        return FALSE;
    }

    SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pProfileData);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"System\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles",
                      0,
                      KEY_READ,
                      &hKey) != ERROR_SUCCESS)
        return FALSE;

    for (dwIndex = 0; dwIndex < pProfileData->dwProfileCount; dwIndex++)
    {
        dwNameLength = 8;
        lError = RegEnumKeyExW(hKey,
                               dwIndex,
                               szName,
                               &dwNameLength,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        if (lError != ERROR_SUCCESS)
            break;

        dwProfileNumber = wcstoul(szName, NULL, 10);
        DPRINT("Profile name: %S\n", szName);
        DPRINT("Profile number: %lu\n", dwProfileNumber);

        if ((pProfileData->dwLastProfile == (DWORD)-1) ||
            (pProfileData->dwLastProfile < dwProfileNumber))
            pProfileData->dwLastProfile = dwProfileNumber;

        DPRINT("Last Profile number: %lu\n", pProfileData->dwLastProfile);

        GetProfile(hwndDlg, hKey, szName, dwProfileNumber, &pProfileData->pProfiles[dwIndex]);
    }

    RegCloseKey(hKey);

    return TRUE;
}


static
BOOL
OnInitDialog(HWND hwndDlg)
{
    DWORD dwWaitInterval;

    DPRINT("OnInitDialog()\n");

    SendMessage(GetDlgItem(hwndDlg, IDC_HRDPROFUP),
                BM_SETIMAGE,(WPARAM)IMAGE_ICON,
                (LPARAM)(HANDLE)LoadIcon(hApplet, MAKEINTRESOURCE(IDI_UP)));
    SendMessage(GetDlgItem(hwndDlg, IDC_HRDPROFDWN),
                BM_SETIMAGE,(WPARAM)IMAGE_ICON,
                (LPARAM)(HANDLE)LoadIcon(hApplet, MAKEINTRESOURCE(IDI_DOWN)));

    if (!GetProfiles(hwndDlg))
        return FALSE;

    SendDlgItemMessageW(hwndDlg, IDC_HRDPROFUPDWN, UDM_SETRANGE, (WPARAM)0, (LPARAM)MAKELONG((SHORT)500, 0));

    dwWaitInterval = GetUserWaitInterval();
    if (dwWaitInterval == (DWORD)-1)
    {
        CheckDlgButton(hwndDlg, IDC_HRDPROFWAIT, BST_CHECKED);
        SendDlgItemMessageW(hwndDlg, IDC_HRDPROFUPDWN, UDM_SETPOS, 0, 30);
        EnableWindow(GetDlgItem(hwndDlg, IDC_HRDPROFEDIT), FALSE);
    }
    else
    {
        CheckDlgButton(hwndDlg, IDC_HRDPROFSELECT, BST_CHECKED);
        SendDlgItemMessageW(hwndDlg, IDC_HRDPROFUPDWN, UDM_SETPOS, 0, dwWaitInterval);
    }

    return TRUE;
}


static
VOID
OnOk(HWND hwndDlg)
{
    DWORD dwWaitInterval;

    if (IsDlgButtonChecked(hwndDlg, IDC_HRDPROFWAIT) == BST_CHECKED)
    {
        dwWaitInterval = (DWORD)-1;
    }
    else
    {
        dwWaitInterval = LOWORD(SendDlgItemMessageW(hwndDlg, IDC_HRDPROFUPDWN, UDM_GETPOS, 0, 0));
    }

    SetUserWaitInterval(dwWaitInterval);
}


/* Property page dialog callback */
INT_PTR
CALLBACK
HardProfDlgProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    PPROFILEDATA pProfileData;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    pProfileData = (PPROFILEDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return OnInitDialog(hwndDlg);

        case WM_DESTROY:
            if (pProfileData != NULL)
            {
                if (pProfileData->pProfiles != NULL)
                    HeapFree(GetProcessHeap(), 0, pProfileData->pProfiles);
                HeapFree(GetProcessHeap(), 0, pProfileData);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_HRDPROFCOPY:
                    CopyHardwareProfile(hwndDlg, pProfileData);
                    break;

                case IDC_HRDPROFRENAME:
                    RenameHardwareProfile(hwndDlg, pProfileData);
                    break;

                case IDC_HRDPROFDEL:
                    DeleteHardwareProfile(hwndDlg, pProfileData);
                    break;

                case IDC_HRDPROFWAIT:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_HRDPROFEDIT), FALSE);
                    return TRUE;

                case IDC_HRDPROFSELECT:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_HRDPROFEDIT), TRUE);
                    return TRUE;

                case IDC_HRDPROFLSTBOX:
                    if (HIWORD(wParam) == LBN_SELCHANGE)
                    {
                        pProfileData->dwSelectedProfileIndex = (DWORD)SendDlgItemMessage(hwndDlg, IDC_HRDPROFLSTBOX, LB_GETCURSEL, 0, 0);

                        EnableWindow(GetDlgItem(hwndDlg, IDC_HRDPROFCOPY), (pProfileData->dwSelectedProfileIndex != (DWORD)-1) ? TRUE : FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_HRDPROFRENAME), (pProfileData->dwSelectedProfileIndex != (DWORD)-1) ? TRUE : FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_HRDPROFDEL), (pProfileData->dwSelectedProfileIndex != (DWORD)-1) ? TRUE : FALSE);
                    }
                    return TRUE;

                case IDOK:
                    OnOk(hwndDlg);

                case IDCANCEL:
                    EndDialog(hwndDlg, LOWORD(wParam));
                    return TRUE;
            }
            break;
    }

    return FALSE;
}

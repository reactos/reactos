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
    HWND hwndProfileDlg;
} PROFILEDATA, *PPROFILEDATA;


static
VOID
OnCopyProfileInit(HWND hwndDlg,
                  PPROFILEDATA pProfileData,
                  UINT idFrom,
                  UINT idTo)
{
    WCHAR szNewProfileName[PROFILE_NAME_LENGTH];

    SetDlgItemText(hwndDlg, idFrom, pProfileData->pProfiles[pProfileData->dwSelectedProfileIndex].szFriendlyName);

    swprintf(szNewProfileName, L"Profile %lu", pProfileData->dwProfileCount);
    SendDlgItemMessageW(hwndDlg, idTo, EM_SETLIMITTEXT, PROFILE_NAME_LENGTH - 1, 0);
    SetDlgItemText(hwndDlg, idTo, szNewProfileName);
}


static
VOID
CopyProfile(
    HWND hwndDlg,
    PPROFILEDATA pProfileData)
{
    PPROFILE pProfiles;
//    PPROFILE pSrcProfile
    PPROFILE pDstProfile;

    /* Allocate memory for the new profile */
    pProfiles = HeapReAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            pProfileData->pProfiles,
                            (pProfileData->dwProfileCount + 1) * sizeof(PROFILE));
    if (pProfiles == NULL)
    {
        DPRINT1("HeapReAlloc() failed!\n");
        return;
    }

    pProfileData->dwProfileCount++;
    pProfileData->pProfiles = pProfiles;

//    pSrcProfile = &pProfileData->pProfiles[pProfileData->dwSelectedProfileIndex];
    pDstProfile = &pProfileData->pProfiles[pProfileData->dwProfileCount - 1];

    GetDlgItemText(hwndDlg,
                   IDC_COPYPROFILETO,
                   pDstProfile->szFriendlyName,
                   PROFILE_NAME_LENGTH);

    pDstProfile->dwProfileNumber = ++pProfileData->dwLastProfile;
    swprintf(pDstProfile->szName, L"%04lu", pDstProfile->dwProfileNumber);

    pDstProfile->dwPreferenceOrder = pDstProfile->dwProfileNumber;

    SendDlgItemMessageW(pProfileData->hwndProfileDlg, IDC_HRDPROFLSTBOX, LB_ADDSTRING, 0, (LPARAM)pDstProfile->szFriendlyName);
}


static
INT_PTR
CALLBACK
CopyProfileDlgProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PPROFILEDATA pProfileData;

    pProfileData = (PPROFILEDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
            pProfileData = (PPROFILEDATA)lParam;
            OnCopyProfileInit(hwndDlg, pProfileData, IDC_COPYPROFILEFROM, IDC_COPYPROFILETO);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    CopyProfile(hwndDlg, pProfileData);
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
RenameProfile(
    HWND hwndDlg,
    PPROFILEDATA pProfileData)
{
    PPROFILE pProfile;

    pProfile = &pProfileData->pProfiles[pProfileData->dwSelectedProfileIndex];

    GetDlgItemText(hwndDlg,
                   IDC_RENPROFEDITTO,
                   pProfile->szFriendlyName,
                   PROFILE_NAME_LENGTH);

    /* Replace the listbox string */
    SendDlgItemMessageW(pProfileData->hwndProfileDlg, IDC_HRDPROFLSTBOX, LB_DELETESTRING, pProfileData->dwSelectedProfileIndex, 0);
    SendDlgItemMessageW(pProfileData->hwndProfileDlg, IDC_HRDPROFLSTBOX, LB_INSERTSTRING, pProfileData->dwSelectedProfileIndex, (LPARAM)pProfile->szFriendlyName);
}


static
INT_PTR
CALLBACK
RenameProfileDlgProc(HWND hwndDlg,
                     UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam)
{
    PPROFILEDATA pProfileData;

    pProfileData = (PPROFILEDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
            pProfileData = (PPROFILEDATA)lParam;
            OnCopyProfileInit(hwndDlg, pProfileData, IDC_RENPROFEDITFROM, IDC_RENPROFEDITTO);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    RenameProfile(hwndDlg, pProfileData);
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

    pProfileData->hwndProfileDlg = hwndDlg;
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
                    DialogBoxParam(hApplet,
                                   MAKEINTRESOURCE(IDD_COPYPROFILE),
                                   hwndDlg,
                                   (DLGPROC)CopyProfileDlgProc,
                                   (LPARAM)pProfileData);
                    break;

                case IDC_HRDPROFRENAME:
                    DialogBoxParam(hApplet,
                                   MAKEINTRESOURCE(IDD_RENAMEPROFILE),
                                   hwndDlg,
                                   (DLGPROC)RenameProfileDlgProc,
                                   (LPARAM)pProfileData);
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

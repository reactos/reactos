/*
 * PROJECT:     Ports installer library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\win32\msports\parallel.c
 * PURPOSE:     Parallel Port property functions
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

#include "precomp.h"


typedef struct _PORT_DATA
{
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;

    WCHAR szPortName[16];
    DWORD dwPortNumber;
    DWORD dwFilterResourceMethod;
    DWORD dwLegacy;

} PORT_DATA, *PPORT_DATA;


static
VOID
GetUsedPorts(
    PDWORD pPortMap)
{
    WCHAR szDeviceName[64];
    WCHAR szDosDeviceName[64];
    DWORD dwIndex, dwType, dwPortNumber;
    DWORD dwDeviceNameSize, dwDosDeviceNameSize;
    PWSTR ptr;
    HKEY hKey;
    DWORD dwError;

    *pPortMap = 0;

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"Hardware\\DeviceMap\\PARALLEL PORTS",
                            0,
                            KEY_READ,
                            &hKey);
    if (dwError == ERROR_SUCCESS)
    {
        for (dwIndex = 0; ; dwIndex++)
        {
            dwDeviceNameSize = ARRAYSIZE(szDeviceName);
            dwDosDeviceNameSize = sizeof(szDosDeviceName);
            dwError = RegEnumValueW(hKey,
                                    dwIndex,
                                    szDeviceName,
                                    &dwDeviceNameSize,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szDosDeviceName,
                                    &dwDosDeviceNameSize);
            if (dwError != ERROR_SUCCESS)
                break;

            if (dwType == REG_SZ)
            {
                TRACE("%S --> %S\n", szDeviceName, szDosDeviceName);
                if (_wcsnicmp(szDosDeviceName, L"\\DosDevices\\LPT", wcslen(L"\\DosDevices\\LPT")) == 0)
                {
                     ptr = szDosDeviceName + wcslen(L"\\DosDevices\\LPT");
                     if (wcschr(ptr, L'.') == NULL)
                     {
                         TRACE("Device number %S\n", ptr);
                         dwPortNumber = wcstoul(ptr, NULL, 10);
                         if (dwPortNumber != 0)
                         {
                             *pPortMap |=(1 << dwPortNumber);
                         }
                     }
                }
            }
        }

        RegCloseKey(hKey);
    }

    TRACE("Done\n");
}


static
VOID
ReadPortSettings(
    PPORT_DATA pPortData)
{
    DWORD dwSize;
    HKEY hKey;
    DWORD dwError;

    TRACE("ReadPortSettings(%p)\n", pPortData);

    pPortData->dwFilterResourceMethod = 1; /* Never use an interrupt */
    pPortData->dwLegacy = 0;               /* Disabled */
    pPortData->dwPortNumber = 0;           /* Unknown */

    hKey = SetupDiOpenDevRegKey(pPortData->DeviceInfoSet,
                                pPortData->DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DEV,
                                KEY_READ);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        dwSize = sizeof(pPortData->szPortName);
        dwError = RegQueryValueExW(hKey,
                                   L"PortName",
                                   NULL,
                                   NULL,
                                  (PBYTE)pPortData->szPortName,
                                  &dwSize);
        if (dwError != ERROR_SUCCESS)
        {
            ERR("RegQueryValueExW failed (Error %lu)\n", dwError);
        }

        dwSize = sizeof(pPortData->dwFilterResourceMethod);
        dwError = RegQueryValueExW(hKey,
                                   L"FilterResourceMethod",
                                   NULL,
                                   NULL,
                                   (PBYTE)&pPortData->dwFilterResourceMethod,
                                   &dwSize);
        if (dwError != ERROR_SUCCESS)
        {
            ERR("RegQueryValueExW failed (Error %lu)\n", dwError);
        }

        RegCloseKey(hKey);
    }

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\CurrentControlSet\\Services\\Parport\\Parameters",
                            0,
                            KEY_READ,
                            &hKey);
    if (dwError == ERROR_SUCCESS)
    {
        dwSize = sizeof(pPortData->dwLegacy);
        dwError = RegQueryValueExW(hKey,
                                   L"ParEnableLegacyZip",
                                   NULL,
                                   NULL,
                                   (PBYTE)&pPortData->dwLegacy,
                                   &dwSize);
        if (dwError != ERROR_SUCCESS)
        {
            ERR("RegQueryValueExW failed (Error %lu)\n", dwError);
        }

        RegCloseKey(hKey);
    }
}


static
DWORD
ChangePortNumber(
    _In_ PPORT_DATA pPortData,
    _In_ DWORD dwNewPortNumber)
{
    WCHAR szDeviceDescription[256];
    WCHAR szFriendlyName[256];
    WCHAR szNewPortName[16];
    HKEY hDeviceKey = INVALID_HANDLE_VALUE;
    DWORD dwError;

    /* We are done if old and new port number are the same */
    if (pPortData->dwPortNumber == dwNewPortNumber)
        return ERROR_SUCCESS;

    /* Build the new port name */
    swprintf(szNewPortName, L"LPT%lu", dwNewPortNumber);

    /* Open the devices hardware key */
    hDeviceKey = SetupDiOpenDevRegKey(pPortData->DeviceInfoSet,
                                      pPortData->DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DEV,
                                      KEY_READ | KEY_WRITE);
    if (hDeviceKey == INVALID_HANDLE_VALUE)
    {
        return GetLastError();
    }

    /* Set the new 'PortName' value */
    dwError = RegSetValueExW(hDeviceKey,
                             L"PortName",
                             0,
                             REG_SZ,
                             (LPBYTE)szNewPortName,
                             (wcslen(szNewPortName) + 1) * sizeof(WCHAR));
    if (dwError != ERROR_SUCCESS)
        goto done;

    /* Save the new port name and number */
    wcscpy(pPortData->szPortName, szNewPortName);
    pPortData->dwPortNumber = dwNewPortNumber;

    /* Get the device description... */
    if (SetupDiGetDeviceRegistryProperty(pPortData->DeviceInfoSet,
                                         pPortData->DeviceInfoData,
                                         SPDRP_DEVICEDESC,
                                         NULL,
                                         (PBYTE)szDeviceDescription,
                                         sizeof(szDeviceDescription),
                                         NULL))
    {
        /* ... and use it to build a new friendly name */
        swprintf(szFriendlyName,
                 L"%s (%s)",
                 szDeviceDescription,
                 szNewPortName);
    }
    else
    {
        /* ... or build a generic friendly name */
        swprintf(szFriendlyName,
                 L"Parallel Port (%s)",
                 szNewPortName);
    }

    /* Set the friendly name for the device */
    SetupDiSetDeviceRegistryProperty(pPortData->DeviceInfoSet,
                                     pPortData->DeviceInfoData,
                                     SPDRP_FRIENDLYNAME,
                                     (PBYTE)szFriendlyName,
                                     (wcslen(szFriendlyName) + 1) * sizeof(WCHAR));

done:
    if (hDeviceKey != INVALID_HANDLE_VALUE)
        RegCloseKey(hDeviceKey);

    return dwError;
}


static
VOID
WritePortSettings(
    HWND hwnd,
    PPORT_DATA pPortData)
{
    SP_PROPCHANGE_PARAMS PropChangeParams;
    DWORD dwDisposition;
    DWORD dwFilterResourceMethod;
    DWORD dwLegacy;
    DWORD dwPortNumber;
    DWORD dwPortMap;
    HKEY hKey;
    BOOL bChanged = FALSE;
    DWORD dwError;

    TRACE("WritePortSettings(%p)\n", pPortData);

    dwFilterResourceMethod = 1;
    if (Button_GetCheck(GetDlgItem(hwnd, IDC_TRY_INTERRUPT)) == BST_CHECKED)
        dwFilterResourceMethod = 0;
    else if (Button_GetCheck(GetDlgItem(hwnd, IDC_NEVER_INTERRUPT)) == BST_CHECKED)
        dwFilterResourceMethod = 1;
    else if (Button_GetCheck(GetDlgItem(hwnd, IDC_ANY_INTERRUPT)) == BST_CHECKED)
        dwFilterResourceMethod = 2;

    if (dwFilterResourceMethod != pPortData->dwFilterResourceMethod)
    {
        hKey = SetupDiOpenDevRegKey(pPortData->DeviceInfoSet,
                                    pPortData->DeviceInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DEV,
                                    KEY_WRITE);
        if (hKey != INVALID_HANDLE_VALUE)
        {
            dwError = RegSetValueExW(hKey,
                                     L"FilterResourceMethod",
                                     0,
                                     REG_DWORD,
                                     (PBYTE)&dwFilterResourceMethod,
                                     sizeof(dwFilterResourceMethod));
            RegCloseKey(hKey);
            if (dwError == ERROR_SUCCESS)
            {
                pPortData->dwFilterResourceMethod = dwFilterResourceMethod;
                bChanged = TRUE;
            }
        }
    }

    dwLegacy = 0;
    if (Button_GetCheck(GetDlgItem(hwnd, IDC_PARALLEL_LEGACY)) == BST_CHECKED)
        dwLegacy = 1;

    if (dwLegacy != pPortData->dwLegacy)
    {
        dwError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                  L"SYSTEM\\CurrentControlSet\\Services\\Parport\\Parameters",
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_WRITE,
                                  NULL,
                                  &hKey,
                                  &dwDisposition);
        if (dwError == ERROR_SUCCESS)
        {
            dwError = RegSetValueExW(hKey,
                                     L"ParEnableLegacyZip",
                                     0,
                                     REG_DWORD,
                                     (LPBYTE)&dwLegacy,
                                     sizeof(dwLegacy));
            RegCloseKey(hKey);

            if (dwError == ERROR_SUCCESS)
            {
                pPortData->dwLegacy = dwLegacy;
                bChanged = TRUE;
            }
        }
    }

    dwPortNumber = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_PARALLEL_NAME));
    if (dwPortNumber != LB_ERR)
    {
        dwPortNumber++;
        if (dwPortNumber != pPortData->dwPortNumber)
        {
            GetUsedPorts(&dwPortMap);

            if (dwPortMap & 1 << dwPortNumber)
            {
                ERR("Port LPT%lu is already in use!\n", dwPortNumber);
            }
            else
            {
                ChangePortNumber(pPortData,
                                 dwPortNumber);
                bChanged = TRUE;
            }
        }
    }

    if (bChanged)
    {
        /* Notify the system */
        PostMessageW(HWND_BROADCAST,
                     WM_WININICHANGE,
                     0,
                     (LPARAM)pPortData->szPortName);

        /* Notify the installer (and device) */
        PropChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
        PropChangeParams.Scope = DICS_FLAG_GLOBAL;
        PropChangeParams.StateChange = DICS_PROPCHANGE;

        SetupDiSetClassInstallParams(pPortData->DeviceInfoSet,
                                     pPortData->DeviceInfoData,
                                     (PSP_CLASSINSTALL_HEADER)&PropChangeParams,
                                     sizeof(SP_PROPCHANGE_PARAMS));

        SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                  pPortData->DeviceInfoSet,
                                  pPortData->DeviceInfoData);
    }
}


static
BOOL
OnInitDialog(HWND hwnd,
             WPARAM wParam,
             LPARAM lParam)
{
    WCHAR szBuffer[256];
    WCHAR szPortInUse[64];
    PPORT_DATA pPortData;
    HWND hwndControl;
    DWORD dwPortMap;
    UINT i;

    TRACE("OnInitDialog()\n");

    pPortData = (PPORT_DATA)((LPPROPSHEETPAGEW)lParam)->lParam;
    if (pPortData == NULL)
    {
        ERR("pPortData is NULL\n");
        return FALSE;
    }

    SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pPortData);

    /* Read and parse the port settings */
    ReadPortSettings(pPortData);

    /* Set the 'Filter Resource Method' radiobuttons */
    hwndControl = GetDlgItem(hwnd,
                             IDC_TRY_INTERRUPT + pPortData->dwFilterResourceMethod);
    if (hwndControl)
        Button_SetCheck(hwndControl, BST_CHECKED);

    /* Disable the 'Enable legacy PNP detection' checkbox */
    hwndControl = GetDlgItem(hwnd, IDC_PARALLEL_LEGACY);
    if (hwndControl)
    {
        Button_SetCheck(GetDlgItem(hwnd, IDC_PARALLEL_LEGACY),
                        pPortData->dwLegacy ? BST_CHECKED : BST_UNCHECKED);
    }

    LoadStringW(hInstance, IDS_PORT_IN_USE, szPortInUse, ARRAYSIZE(szPortInUse));

    /* Fill the 'LPT Port Number' combobox */
    hwndControl = GetDlgItem(hwnd, IDC_PARALLEL_NAME);
    if (hwndControl)
    {
        GetUsedPorts(&dwPortMap);

        for (i = 1; i < 4; i++)
        {
            swprintf(szBuffer, L"LPT%lu", i);

            if ((dwPortMap & (1 << i)) && (pPortData->dwPortNumber != i))
                wcscat(szBuffer, szPortInUse);

            ComboBox_AddString(hwndControl, szBuffer);

            if (_wcsicmp(pPortData->szPortName, szBuffer) == 0)
                pPortData->dwPortNumber = i;
        }

        if (pPortData->dwPortNumber != 0)
        {
            ComboBox_SetCurSel(hwndControl, pPortData->dwPortNumber - 1);
        }
    }

    return TRUE;
}


static
VOID
OnNotify(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam)
{
    PPORT_DATA pPortData;

    TRACE("OnNotify()\n");

    pPortData = (PPORT_DATA)GetWindowLongPtr(hwnd, DWLP_USER);
    if (pPortData == NULL)
    {
        ERR("pPortData is NULL\n");
        return;
    }

    if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
    {
        TRACE("PSN_APPLY!\n");
        WritePortSettings(hwnd, pPortData);
    }
}


static
VOID
OnDestroy(
    HWND hwnd)
{
    PPORT_DATA pPortData;

    TRACE("OnDestroy()\n");

    pPortData = (PPORT_DATA)GetWindowLongPtr(hwnd, DWLP_USER);
    if (pPortData == NULL)
    {
        ERR("pPortData is NULL\n");
        return;
    }

    HeapFree(GetProcessHeap(), 0, pPortData);
    SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)NULL);
}


static
INT_PTR
CALLBACK
ParallelSettingsDlgProc(HWND hwnd,
                        UINT uMsg,
                        WPARAM wParam,
                        LPARAM lParam)
{
    TRACE("ParallelSettingsDlgProc()\n");

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return OnInitDialog(hwnd, wParam, lParam);

        case WM_NOTIFY:
            OnNotify(hwnd, wParam, lParam);
            break;

        case WM_DESTROY:
            OnDestroy(hwnd);
            break;
    }

    return FALSE;
}


BOOL
WINAPI
ParallelPortPropPageProvider(PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
                             LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
                             LPARAM lParam)
{
    PROPSHEETPAGEW PropSheetPage;
    HPROPSHEETPAGE hPropSheetPage;
    PPORT_DATA pPortData;

    TRACE("ParallelPortPropPageProvider(%p %p %lx)\n",
          lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);

    pPortData = HeapAlloc(GetProcessHeap(),
                          HEAP_ZERO_MEMORY,
                          sizeof(PORT_DATA));
    if (pPortData == NULL)
    {
        ERR("Port data allocation failed!\n");
        return FALSE;
    }

    pPortData->DeviceInfoSet = lpPropSheetPageRequest->DeviceInfoSet;
    pPortData->DeviceInfoData = lpPropSheetPageRequest->DeviceInfoData;

    if (lpPropSheetPageRequest->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
    {
        TRACE("SPPSR_ENUM_ADV_DEVICE_PROPERTIES\n");

        PropSheetPage.dwSize = sizeof(PROPSHEETPAGEW);
        PropSheetPage.dwFlags = 0;
        PropSheetPage.hInstance = hInstance;
        PropSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PARALLELSETTINGS);
        PropSheetPage.pfnDlgProc = ParallelSettingsDlgProc;
        PropSheetPage.lParam = (LPARAM)pPortData;
        PropSheetPage.pfnCallback = NULL;

        hPropSheetPage = CreatePropertySheetPageW(&PropSheetPage);
        if (hPropSheetPage == NULL)
        {
            ERR("CreatePropertySheetPageW() failed!\n");
            return FALSE;
        }

        if (!(*lpfnAddPropSheetPageProc)(hPropSheetPage, lParam))
        {
            ERR("lpfnAddPropSheetPageProc() failed!\n");
            DestroyPropertySheetPage(hPropSheetPage);
            return FALSE;
        }
    }

    TRACE("Done!\n");

    return TRUE;
}

/* EOF */

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
    DWORD dwFilterResourceMethod;
} PORT_DATA, *PPORT_DATA;


static
VOID
ReadPortSettings(
    PPORT_DATA pPortData)
{
    DWORD dwSize;
    HKEY hKey;
    LONG lError;

    TRACE("ReadPortSettings(%p)\n", pPortData);

    pPortData->dwFilterResourceMethod = 1; /* Never use an interrupt */

    hKey = SetupDiOpenDevRegKey(pPortData->DeviceInfoSet,
                                pPortData->DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DEV,
                                KEY_READ);
    if (hKey == INVALID_HANDLE_VALUE)
    {
        ERR("SetupDiOpenDevRegKey() failed\n");
        return;
    }

    dwSize = sizeof(pPortData->szPortName);
    lError = RegQueryValueExW(hKey,
                              L"PortName",
                              NULL,
                              NULL,
                              (PBYTE)pPortData->szPortName,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed (Error %lu)\n", lError);
    }

    dwSize = sizeof(pPortData->dwFilterResourceMethod);
    lError = RegQueryValueExW(hKey,
                              L"FilterResourceMethod",
                              NULL,
                              NULL,
                              (PBYTE)&pPortData->dwFilterResourceMethod,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed (Error %lu)\n", lError);
    }

    RegCloseKey(hKey);
}


static
VOID
WritePortSettings(
    HWND hwnd,
    PPORT_DATA pPortData)
{
    DWORD dwFilterResourceMethod;
    HKEY hKey;
    LONG lError;

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
        if (hKey == INVALID_HANDLE_VALUE)
        {
            ERR("SetupDiOpenDevRegKey() failed\n");
            return;
        }

        lError = RegSetValueExW(hKey,
                                L"FilterResourceMethod",
                                0,
                                REG_DWORD,
                                (PBYTE)&dwFilterResourceMethod,
                                sizeof(dwFilterResourceMethod));
        if (lError != ERROR_SUCCESS)
        {
            ERR("RegSetValueExW failed (Error %lu)\n", lError);
        }

        RegCloseKey(hKey);

        pPortData->dwFilterResourceMethod = dwFilterResourceMethod;
    }

}


static
BOOL
OnInitDialog(HWND hwnd,
             WPARAM wParam,
             LPARAM lParam)
{
    PPORT_DATA pPortData;
    HWND hwndControl;
    WCHAR szBuffer[256];
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
    EnableWindow(hwndControl, FALSE);

    /* Fill the 'LPT Port Number' combobox */
    hwndControl = GetDlgItem(hwnd, IDC_PARALLEL_NAME);
    if (hwndControl)
    {
        for (i = 0; i < 3; i++)
        {
            swprintf(szBuffer, L"LPT%lu", i + 1);
            ComboBox_AddString(hwndControl, szBuffer);
        }

        /* Disable it */
        EnableWindow(hwndControl, FALSE);
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

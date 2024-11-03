/*
 * PROJECT:     Ports installer library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\win32\msports\serial.c
 * PURPOSE:     Serial Port property functions
 * COPYRIGHT:   Copyright 2011 Eric Kohl
 */

#include "precomp.h"


typedef struct _PORT_DATA
{
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;

    WCHAR szPortName[16];
    INT nBaudRateIndex;
    INT nParityIndex;
    INT nDataBitsIndex;
    INT nStopBitsIndex;
    INT nFlowControlIndex;

    BOOL bChanged;
} PORT_DATA, *PPORT_DATA;

#define DEFAULT_BAUD_RATE_INDEX    11
#define DEFAULT_DATA_BITS_INDEX     4
#define DEFAULT_PARITY_INDEX        2
#define DEFAULT_STOP_BITS_INDEX     0
#define DEFAULT_FLOW_CONTROL_INDEX  2

DWORD BaudRates[] = {75, 110, 134, 150, 300, 600, 1200, 1800, 2400, 4800,
                     7200, 9600, 14400, 19200, 38400, 57600, 115200, 128000};
PWSTR Paritys[] = {L"e", L"o", L"n", L"m", L"s"};
PWSTR DataBits[] = {L"4", L"5", L"6", L"7", L"8"};
PWSTR StopBits[] = {L"1", L"1.5", L"2"};
PWSTR FlowControls[] = {L"x", L"p"};


static
VOID
FillComboBox(
    HWND hwnd,
    PWSTR szBuffer)
{
    PWSTR pStart, pEnd;

    pStart = szBuffer;
    for (;;)
    {
        pEnd = wcschr(pStart, L',');
        if (pEnd != NULL)
            *pEnd = UNICODE_NULL;

        ComboBox_AddString(hwnd, pStart);

        if (pEnd == NULL)
            break;

        pStart = pEnd + 1;
    }
}


static
VOID
ReadPortSettings(
    PPORT_DATA pPortData)
{
    WCHAR szPortData[32];
    WCHAR szParity[4];
    WCHAR szDataBits[4];
    WCHAR szStopBits[4];
    WCHAR szFlowControl[4];
    DWORD dwType, dwSize;
    DWORD dwBaudRate = 0;
    HKEY hKey;
    INT n, i;
    LONG lError;

    TRACE("ReadPortSettings(%p)\n", pPortData);

    pPortData->nBaudRateIndex = DEFAULT_BAUD_RATE_INDEX; /* 9600 */
    pPortData->nParityIndex   = DEFAULT_PARITY_INDEX;    /* None */
    pPortData->nDataBitsIndex = DEFAULT_DATA_BITS_INDEX; /* 8 Data Bits */
    pPortData->nStopBitsIndex = DEFAULT_STOP_BITS_INDEX; /* 1 Stop Bit */
    pPortData->nFlowControlIndex = DEFAULT_FLOW_CONTROL_INDEX; /* None */
    pPortData->bChanged = FALSE;

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
    RegCloseKey(hKey);

    if (lError != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed (Error %lu)\n", lError);
        return;
    }

    wcscat(pPortData->szPortName, L":");
    TRACE("PortName: '%S'\n", pPortData->szPortName);

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Ports",
                           0,
                           KEY_READ,
                           &hKey);
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed (Error %lu)\n", lError);
        return;
    }

    dwSize = sizeof(szPortData);
    lError = RegQueryValueExW(hKey,
                              pPortData->szPortName,
                              NULL,
                              &dwType,
                              (LPBYTE)szPortData,
                              &dwSize);
    RegCloseKey(hKey);

    if (lError != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed (Error %lu)\n", lError);
        return;
    }

    if ((dwType != REG_SZ) || (dwSize > sizeof(szPortData)))
    {
        ERR("Wrong type or size\n");
        return;
    }

    TRACE("szPortData: '%S'\n", szPortData);

    /* Replace commas by spaces */
    for (i = 0; i < wcslen(szPortData); i++)
    {
        if (szPortData[i] == L',')
            szPortData[i] = L' ';
    }

    TRACE("szPortData: '%S'\n", szPortData);

    /* Parse the port settings */
    n = swscanf(szPortData,
                L"%lu %3s %3s %3s %3s",
                &dwBaudRate,
                &szParity,
                &szDataBits,
                &szStopBits,
                &szFlowControl);

    TRACE("dwBaudRate: %lu\n", dwBaudRate);
    TRACE("szParity: '%S'\n", szParity);
    TRACE("szDataBits: '%S'\n", szDataBits);
    TRACE("szStopBits: '%S'\n", szStopBits);
    TRACE("szFlowControl: '%S'\n", szFlowControl);

    if (n > 0)
    {
        for (i = 0; i < ARRAYSIZE(BaudRates); i++)
        {
            if (dwBaudRate == BaudRates[i])
                pPortData->nBaudRateIndex = i;
        }
    }

    if (n > 1)
    {
        for (i = 0; i < ARRAYSIZE(Paritys); i++)
        {
            if (_wcsicmp(szParity, Paritys[i]) == 0)
                pPortData->nParityIndex = i;
        }
    }

    if (n > 2)
    {
        for (i = 0; i < ARRAYSIZE(DataBits); i++)
        {
            if (_wcsicmp(szDataBits, DataBits[i]) == 0)
                pPortData->nDataBitsIndex = i;
        }
    }

    if (n > 3)
    {
        for (i = 0; i < ARRAYSIZE(StopBits); i++)
        {
            if (_wcsicmp(szStopBits, StopBits[i]) == 0)
                pPortData->nStopBitsIndex = i;
        }
    }

    if (n > 4)
    {
        for (i = 0; i < ARRAYSIZE(FlowControls); i++)
        {
            if (_wcsicmp(szFlowControl, FlowControls[i]) == 0)
                pPortData->nFlowControlIndex = i;
        }
    }
}


static
VOID
WritePortSettings(
    HWND hwnd,
    PPORT_DATA pPortData)
{
    SP_PROPCHANGE_PARAMS PropChangeParams;
    WCHAR szPortData[32];
    HWND hwndControl;
    INT nBaudRateIndex;
    INT nDataBitsIndex;
    INT nParityIndex;
    INT nStopBitsIndex;
    INT nFlowControlIndex;
    HKEY hKey;
    LONG lError;

    TRACE("WritePortSettings(%p)\n", pPortData);

    if (pPortData->bChanged == FALSE)
    {
        TRACE("Nothing changed. Done!\n");
        return;
    }

    nBaudRateIndex = pPortData->nBaudRateIndex;
    nDataBitsIndex = pPortData->nDataBitsIndex;
    nParityIndex = pPortData->nParityIndex;
    nStopBitsIndex = pPortData->nStopBitsIndex;
    nFlowControlIndex = pPortData->nFlowControlIndex;

    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_BITSPERSECOND);
    if (hwndControl)
    {
        nBaudRateIndex = ComboBox_GetCurSel(hwndControl);
    }

    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_DATABITS);
    if (hwndControl)
    {
        nDataBitsIndex = ComboBox_GetCurSel(hwndControl);
    }

    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_PARITY);
    if (hwndControl)
    {
        nParityIndex = ComboBox_GetCurSel(hwndControl);
    }

    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_STOPBITS);
    if (hwndControl)
    {
        nStopBitsIndex = ComboBox_GetCurSel(hwndControl);
    }

    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_FLOWCONTROL);
    if (hwndControl)
    {
        nFlowControlIndex = ComboBox_GetCurSel(hwndControl);
    }

    swprintf(szPortData,
             L"%lu,%s,%s,%s",
             BaudRates[nBaudRateIndex],
             Paritys[nParityIndex],
             DataBits[nDataBitsIndex],
             StopBits[nStopBitsIndex]);
    if (nFlowControlIndex < 2)
    {
        wcscat(szPortData, L",");
        wcscat(szPortData, FlowControls[nFlowControlIndex]);
    }

    TRACE("szPortData: '%S'\n", szPortData);

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Ports",
                           0,
                           KEY_WRITE,
                           &hKey);
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed (Error %lu)\n", lError);
        return;
    }

    lError = RegSetValueExW(hKey,
                            pPortData->szPortName,
                            0,
                            REG_SZ,
                            (LPBYTE)szPortData,
                            (wcslen(szPortData) + 1) * sizeof(WCHAR));

    RegCloseKey(hKey);

    if (lError != ERROR_SUCCESS)
    {
        ERR("RegSetValueExW failed (Error %lu)\n", lError);
        return;
    }

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

    TRACE("Done!\n");
}


static
BOOL
OnInitDialog(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam)
{
    PPORT_DATA pPortData;
    WCHAR szBuffer[256];
    UINT i;
    HWND hwndControl;

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

    /* Fill the 'Bits per second' combobox */
    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_BITSPERSECOND);
    if (hwndControl)
    {
        for (i = 0; i < ARRAYSIZE(BaudRates); i++)
        {
            _ultow(BaudRates[i], szBuffer, 10);
            ComboBox_AddString(hwndControl, szBuffer);
        }

        ComboBox_SetCurSel(hwndControl, pPortData->nBaudRateIndex);
    }

    /* Fill the 'Data bits' combobox */
    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_DATABITS);
    if (hwndControl)
    {
        for (i = 0; i < ARRAYSIZE(DataBits); i++)
        {
            ComboBox_AddString(hwndControl, DataBits[i]);
        }

        ComboBox_SetCurSel(hwndControl, pPortData->nDataBitsIndex);
    }

    /* Fill the 'Parity' combobox */
    LoadStringW(hInstance, IDS_PARITY, szBuffer, ARRAYSIZE(szBuffer));

    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_PARITY);
    if (hwndControl)
    {
        FillComboBox(hwndControl, szBuffer);
        ComboBox_SetCurSel(hwndControl, pPortData->nParityIndex);
    }

    /* Fill the 'Stop bits' combobox */
    LoadStringW(hInstance, IDS_STOPBITS, szBuffer, ARRAYSIZE(szBuffer));

    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_STOPBITS);
    if (hwndControl)
    {
        FillComboBox(hwndControl, szBuffer);
        ComboBox_SetCurSel(hwndControl, pPortData->nStopBitsIndex);
    }

    /* Fill the 'Flow control' combobox */
    LoadStringW(hInstance, IDS_FLOWCONTROL, szBuffer, ARRAYSIZE(szBuffer));

    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_FLOWCONTROL);
    if (hwndControl)
    {
        FillComboBox(hwndControl, szBuffer);
        ComboBox_SetCurSel(hwndControl, pPortData->nFlowControlIndex);
    }

    /* Disable the 'Advanced' button */
    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_ADVANCED);
    if (hwndControl)
        EnableWindow(hwndControl, FALSE);

    return TRUE;
}


static
VOID
RestoreDefaultValues(
    HWND hwnd,
    PPORT_DATA pPortData)
{
    HWND hwndControl;

    /* Reset the 'Bits per second' combobox */
    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_BITSPERSECOND);
    if (hwndControl)
    {
        ComboBox_SetCurSel(hwndControl, DEFAULT_BAUD_RATE_INDEX);
    }

    /* Reset the 'Data bits' combobox */
    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_DATABITS);
    if (hwndControl)
    {
        ComboBox_SetCurSel(hwndControl, DEFAULT_DATA_BITS_INDEX);
    }

    /* Reset the 'Parity' combobox */
    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_PARITY);
    if (hwndControl)
    {
        ComboBox_SetCurSel(hwndControl, DEFAULT_PARITY_INDEX);
    }

    /* Reset the 'Stop bits' combobox */
    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_STOPBITS);
    if (hwndControl)
    {
        ComboBox_SetCurSel(hwndControl, DEFAULT_STOP_BITS_INDEX);
    }

    /* Reset the 'Flow control' combobox */
    hwndControl = GetDlgItem(hwnd, IDC_SERIAL_FLOWCONTROL);
    if (hwndControl)
    {
        ComboBox_SetCurSel(hwndControl, DEFAULT_FLOW_CONTROL_INDEX);
    }

    pPortData->bChanged = TRUE;
}


static
VOID
OnCommand(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam)
{
    PPORT_DATA pPortData;

    TRACE("OnCommand()\n");

    pPortData = (PPORT_DATA)GetWindowLongPtr(hwnd, DWLP_USER);
    if (pPortData == NULL)
    {
        ERR("pPortData is NULL\n");
        return;
    }

    switch (LOWORD(wParam))
    {
        case IDC_SERIAL_BITSPERSECOND:
        case IDC_SERIAL_DATABITS:
        case IDC_SERIAL_PARITY:
        case IDC_SERIAL_STOPBITS:
        case IDC_SERIAL_FLOWCONTROL:
            if (HIWORD(wParam) == CBN_SELCHANGE ||
                HIWORD(wParam) == CBN_EDITCHANGE)
            {
                pPortData->bChanged = TRUE;
            }
            break;

//        case IDC_SERIAL_ADVANCED:

        case IDC_SERIAL_RESTORE:
            RestoreDefaultValues(hwnd, pPortData);
            break;

        default:
            break;
    }
}


static
VOID
OnNotify(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam)
{
    PPORT_DATA pPortData;

    TRACE("OnCommand()\n");

    pPortData = (PPORT_DATA)GetWindowLongPtr(hwnd, DWLP_USER);
    if (pPortData == NULL)
    {
        ERR("pPortData is NULL\n");
        return;
    }

    if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
    {
        FIXME("PSN_APPLY!\n");
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
SerialSettingsDlgProc(HWND hwnd,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam)
{
    TRACE("SerialSettingsDlgProc()\n");

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return OnInitDialog(hwnd, wParam, lParam);

        case WM_COMMAND:
            OnCommand(hwnd, wParam, lParam);
            break;

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
SerialPortPropPageProvider(
    PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    LPARAM lParam)
{
    PROPSHEETPAGEW PropSheetPage;
    HPROPSHEETPAGE hPropSheetPage;
    PPORT_DATA pPortData;

    TRACE("SerialPortPropPageProvider(%p %p %lx)\n",
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
        PropSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SERIALSETTINGS);
        PropSheetPage.pfnDlgProc = SerialSettingsDlgProc;
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

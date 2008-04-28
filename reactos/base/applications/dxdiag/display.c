/*
 * PROJECT:     ReactX Diagnosis Application
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/dxdiag/display.c
 * PURPOSE:     ReactX diagnosis display page
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald
 *
 */

#include "precomp.h"
#include <initguid.h>
#include <devguid.h>

static
BOOL
InitializeDialog(HWND hwndDlg)
{
    HDEVINFO hInfo;
    SP_DEVICE_INTERFACE_DATA InterfaceData;
    SP_DEVINFO_DATA InfoData;
    DWORD dwIndex = 0;
    WCHAR szText[100];
    WCHAR szFormat[30];
    HKEY hKey;
    DWORD dwMemory;
    DEVMODE DevMode;

    DISPLAY_DEVICEW DispDevice;

    /* query display device adapter */
    ZeroMemory(&DispDevice, sizeof(DISPLAY_DEVICEW));
    DispDevice.cb = sizeof(DISPLAY_DEVICEW);
    if (!EnumDisplayDevicesW(NULL, 0, &DispDevice, 0))
        return FALSE;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, &DispDevice.DeviceKey[18], 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return FALSE;

    if (GetRegValue(hKey, NULL, L"HardwareInformation.ChipType", REG_BINARY, szText, sizeof(szText)))
    {
        /* set chip type */
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_ADAPTER_CHIP, WM_SETTEXT, 0, (LPARAM)szText);
    }

    if (GetRegValue(hKey, NULL, L"HardwareInformation.DacType", REG_BINARY, szText, sizeof(szText)))
    {
        /* set DAC type */
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_ADAPTER_DAC, WM_SETTEXT, 0, (LPARAM)szText);
    }

    if (GetRegValue(hKey, NULL, L"HardwareInformation.MemorySize", REG_BINARY, &dwMemory, sizeof(dwMemory)))
    {
        /* set chip memory size */
        if (dwMemory > (1048576))
        {
            /* buggy ATI driver requires that */
            dwMemory /= 1048576;
        }
        szFormat[0] = L'\0';
        if (LoadStringW(hInst, IDS_FORMAT_ADAPTER_MEM, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
            szFormat[(sizeof(szFormat)/sizeof(WCHAR))-1] = L'\0';
        swprintf(szText, szFormat, dwMemory);
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_ADAPTER_MEM, WM_SETTEXT, 0, (LPARAM)szText);
    }

    /* retrieve current display mode */
    DevMode.dmSize = sizeof(DEVMODE);
    if (EnumDisplaySettingsW(DispDevice.DeviceName, ENUM_CURRENT_SETTINGS, &DevMode))
    {
        szFormat[0] = L'\0';
        if (LoadStringW(hInst, IDS_FORMAT_ADAPTER_MODE, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
            szFormat[(sizeof(szFormat)/sizeof(WCHAR))-1] = L'\0';
        swprintf(szText, szFormat, DevMode.dmPelsWidth, DevMode.dmPelsHeight, DevMode.dmBitsPerPel, DevMode.dmDisplayFrequency);
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_ADAPTER_MODE, WM_SETTEXT, 0, (LPARAM)szText);
    }

    /* query attached monitor */
    wcscpy(szText, DispDevice.DeviceName);
    ZeroMemory(&DispDevice, sizeof(DISPLAY_DEVICEW));
    DispDevice.cb = sizeof(DISPLAY_DEVICEW);
    if (EnumDisplayDevicesW(szText, 0, &DispDevice, 0))
    {
         /* set monitor name */
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_ADAPTER_MONITOR, WM_SETTEXT, 0, (LPARAM)DispDevice.DeviceString);
    }


    hInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, NULL, hwndDlg, DIGCF_PRESENT|DIGCF_PROFILE);
    if (hInfo == INVALID_HANDLE_VALUE)
        return FALSE;

    do
    {
        ZeroMemory(&InterfaceData, sizeof(InterfaceData));
        InterfaceData.cbSize = sizeof(InterfaceData);
        InfoData.cbSize = sizeof(InfoData);

        if (SetupDiEnumDeviceInfo(hInfo, dwIndex, &InfoData))
        {
            /* set device name */
            if (SetupDiGetDeviceRegistryPropertyW(hInfo, &InfoData, SPDRP_DEVICEDESC, NULL, (PBYTE)szText, sizeof(szText), NULL))
                SendDlgItemMessageW(hwndDlg, IDC_STATIC_ADAPTER_ID, WM_SETTEXT, 0, (LPARAM)szText);

            /* set the manufacturer name */
            if (SetupDiGetDeviceRegistryPropertyW(hInfo, &InfoData, SPDRP_MFG, NULL, (PBYTE)szText, sizeof(szText), NULL))
                SendDlgItemMessageW(hwndDlg, IDC_STATIC_ADAPTER_VENDOR, WM_SETTEXT, 0, (LPARAM)szText);

            /* FIXME
             * we currently enumerate only the first adapter 
             */
            break;
        }

        if (GetLastError() == ERROR_NO_MORE_ITEMS)
            break;

        dwIndex++;
    }while(TRUE);


    SetupDiDestroyDeviceInfoList(hInfo);
    return TRUE;
}

INT_PTR CALLBACK
DisplayPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    PDXDIAG_CONTEXT pContext = (PDXDIAG_CONTEXT)GetWindowLongPtr(hDlg, DWLP_USER);
    switch (message) 
    {
        case WM_INITDIALOG:
        {
            pContext = (PDXDIAG_CONTEXT) lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pContext);
            SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            InitializeDialog(hDlg);
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_BUTTON_TESTDD:
                    /* FIXME log result errors */
                    ShowWindow(pContext->hMainDialog, SW_HIDE);
                    StartDDTest(pContext->hMainDialog, hInst, IDS_DDPRIMARY_DESCRIPTION, IDS_DDPRIMARY_RESULT, 1);
                    StartDDTest(pContext->hMainDialog, hInst, IDS_DDOFFSCREEN_DESCRIPTION, IDS_DDOFFSCREEN_RESULT, 2);
                    StartDDTest(pContext->hMainDialog, hInst, IDS_DDFULLSCREEN_DESCRIPTION, IDS_DDFULLSCREEN_RESULT, 3);
                    ShowWindow(pContext->hMainDialog, SW_SHOW);
                    /* FIXME resize window */
                    break;
            }
            break;
        }
    }

    return FALSE;
}

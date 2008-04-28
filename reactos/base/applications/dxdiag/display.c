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

BOOL
GetFileModifyTime(LPCWSTR pFullPath, WCHAR * szTime, int szTimeSize)
{
    HANDLE hFile;
    FILETIME AccessTime;
    SYSTEMTIME SysTime, LocalTime;
    UINT Length;

    hFile = CreateFileW(pFullPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (!hFile)
        return FALSE;

    if (!GetFileTime(hFile, NULL, NULL, &AccessTime))
    {
        CloseHandle(hFile);
        return FALSE;
    }
    CloseHandle(hFile);

    if (!FileTimeToSystemTime(&AccessTime, &SysTime))
        return FALSE;

    if (!SystemTimeToTzSpecificLocalTime(NULL, &SysTime, &LocalTime))
        return FALSE;

    Length = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &LocalTime, NULL, szTime, szTimeSize);
    szTime[Length-1] = L' ';
    return GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT, &LocalTime, NULL, &szTime[Length], szTimeSize-Length);
}



static UINT WINAPI
DriverFilesCallback(IN PVOID Context,
                              IN UINT Notification,
                              IN UINT_PTR Param1,
                              IN UINT_PTR Param2)
{
    LPCWSTR pFile;
    LPWSTR pBuffer;
    LPCWSTR pFullPath = (LPCWSTR)Param1;
    WCHAR szVer[30];
    LRESULT Length, fLength;
    HWND * hDlgCtrls = (HWND *)Context;

    if (wcsstr(pFullPath, L"\\DRIVERS\\"))
    {
        /* exclude files from drivers dir to have failsafe file version/date information */
        return NO_ERROR;
    }

    pFile = wcsrchr(pFullPath, L'\\');
    if (!pFile)
       return NO_ERROR;

    pFile++;
    fLength = wcslen(pFile) + 1;

    Length = SendMessageW(hDlgCtrls[0], WM_GETTEXTLENGTH, 0, 0) + 1;
    pBuffer = HeapAlloc(GetProcessHeap(), 0, (Length + fLength) * sizeof(WCHAR));
    if (!pBuffer)
        return ERROR_OUTOFMEMORY;

    Length = SendMessageW(hDlgCtrls[0], WM_GETTEXT, Length, (LPARAM)pBuffer);
    if (Length)
    {
        pBuffer[Length++] = L',';
    }
    else
    {
        /* set file version */
        if (GetFileVersion(pFullPath, szVer))
            SendMessageW(hDlgCtrls[1], WM_SETTEXT, 0, (LPARAM)szVer);
        /* set file time */
        if (GetFileModifyTime(pFullPath, szVer, sizeof(szVer)/sizeof(WCHAR)))
            SendMessageW(hDlgCtrls[2], WM_SETTEXT, 0, (LPARAM)szVer);
    }

    wcscpy(&pBuffer[Length], pFile);
    SendMessageW(hDlgCtrls[0], WM_SETTEXT, 0, (LPARAM)pBuffer);
    HeapFree(GetProcessHeap(), 0, pBuffer);
    return NO_ERROR;
}

VOID
EnumerateDrivers(PVOID Context, HDEVINFO hList, PSP_DEVINFO_DATA pInfoData)
{
    HSPFILEQ hQueue;
    SP_DEVINSTALL_PARAMS DeviceInstallParams = {0};
    SP_DRVINFO_DATA DriverInfoData;
    DWORD Result;

    DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);
    if (!SetupDiGetDeviceInstallParamsW(hList, pInfoData, &DeviceInstallParams))
        return;

    DeviceInstallParams.FlagsEx |= (DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);
    if (!SetupDiSetDeviceInstallParams(hList, pInfoData, &DeviceInstallParams))
        return;

    if (!SetupDiBuildDriverInfoList(hList, pInfoData, SPDIT_CLASSDRIVER))
        return;

    DriverInfoData.cbSize = sizeof(DriverInfoData);
    if (!SetupDiEnumDriverInfoW(hList, pInfoData, SPDIT_CLASSDRIVER, 0, &DriverInfoData))
        return;

    DriverInfoData.cbSize = sizeof(DriverInfoData);
    if (!SetupDiSetSelectedDriverW(hList, pInfoData, &DriverInfoData))
         return;

    hQueue = SetupOpenFileQueue();
    if (hQueue == (HSPFILEQ)INVALID_HANDLE_VALUE)
        return;

    DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);
    if (!SetupDiGetDeviceInstallParamsW(hList, pInfoData, &DeviceInstallParams))
    {
        SetupCloseFileQueue(hQueue);
        return;
    }

    DeviceInstallParams.FileQueue = hQueue;
    DeviceInstallParams.Flags |= DI_NOVCP;

    if (!SetupDiSetDeviceInstallParamsW(hList, pInfoData, &DeviceInstallParams))
    {
        SetupCloseFileQueue(hQueue);
        return;
    }

    if(!SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES, hList, pInfoData))
    {
        SetupCloseFileQueue(hQueue);
        return;
    }


    /* enumerate the driver files */
    SetupScanFileQueueW(hQueue, SPQ_SCAN_USE_CALLBACK, NULL, DriverFilesCallback, Context, &Result);
    SetupCloseFileQueue(hQueue);
}


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
    HWND hDlgCtrls[3];
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

    if (GetRegValue(hKey, NULL, L"HardwareInformation.MemorySize", REG_BINARY, (LPWSTR)&dwMemory, sizeof(dwMemory)))
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
            hDlgCtrls[0] = GetDlgItem(hwndDlg, IDC_STATIC_ADAPTER_DRIVER);
            hDlgCtrls[1] = GetDlgItem(hwndDlg, IDC_STATIC_ADAPTER_VERSION);
            hDlgCtrls[2] = GetDlgItem(hwndDlg, IDC_STATIC_ADAPTER_DATE);
            EnumerateDrivers(hDlgCtrls, hInfo, &InfoData);
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

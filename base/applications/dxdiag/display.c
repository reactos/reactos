/*
 * PROJECT:     ReactX Diagnosis Application
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/dxdiag/display.c
 * PURPOSE:     ReactX diagnosis display page
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald
 *
 */

#include "precomp.h"

#include <d3d9.h>

BOOL
GetFileModifyTime(LPCWSTR pFullPath, WCHAR * szTime, int szTimeSize)
{
    HANDLE hFile;
    FILETIME AccessTime;
    SYSTEMTIME SysTime, LocalTime;
    UINT Length;
    TIME_ZONE_INFORMATION TimeInfo;

    hFile = CreateFileW(pFullPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (!hFile)
        return FALSE;

    if (!GetFileTime(hFile, NULL, NULL, &AccessTime))
    {
        CloseHandle(hFile);
        return FALSE;
    }
    CloseHandle(hFile);

    if(!GetTimeZoneInformation(&TimeInfo))
        return FALSE;

    if (!FileTimeToSystemTime(&AccessTime, &SysTime))
        return FALSE;

    if (!SystemTimeToTzSpecificLocalTime(&TimeInfo, &SysTime, &LocalTime))
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
    WCHAR szVer[60];
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
        if (GetFileVersion(pFullPath, szVer, sizeof(szVer)/sizeof(WCHAR)))
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
void
DisplayPageSetDeviceDetails(HWND * hDlgCtrls, LPCGUID classGUID, LPGUID * deviceGUID)
{
    HDEVINFO hInfo;
    DWORD dwIndex = 0;
    SP_DEVINFO_DATA InfoData;
    WCHAR szText[100];

    /* create the setup list */
    hInfo = SetupDiGetClassDevsW(classGUID, NULL, NULL, DIGCF_PRESENT|DIGCF_PROFILE);
    if (hInfo == INVALID_HANDLE_VALUE)
        return;

    do
    {
        ZeroMemory(&InfoData, sizeof(InfoData));
        InfoData.cbSize = sizeof(InfoData);

        if (SetupDiEnumDeviceInfo(hInfo, dwIndex, &InfoData))
        {
            /* set device name */
            if (SetupDiGetDeviceRegistryPropertyW(hInfo, &InfoData, SPDRP_DEVICEDESC, NULL, (PBYTE)szText, sizeof(szText), NULL))
                SendMessageW(hDlgCtrls[0], WM_SETTEXT, 0, (LPARAM)szText);

            /* set the manufacturer name */
            if (SetupDiGetDeviceRegistryPropertyW(hInfo, &InfoData, SPDRP_MFG, NULL, (PBYTE)szText, sizeof(szText), NULL))
                SendMessageW(hDlgCtrls[1], WM_SETTEXT, 0, (LPARAM)szText);

            /* FIXME
             * we currently enumerate only the first adapter
             */
            EnumerateDrivers(&hDlgCtrls[2], hInfo, &InfoData);
            break;
        }

        if (GetLastError() == ERROR_NO_MORE_ITEMS)
            break;

        dwIndex++;
    }while(TRUE);

    /* destroy the setup list */
    SetupDiDestroyDeviceInfoList(hInfo);
}


static
BOOL
InitializeDialog(HWND hwndDlg, PDISPLAY_DEVICEW pDispDevice)
{
    WCHAR szText[100];
    WCHAR szFormat[30];
    HKEY hKey;
    HWND hDlgCtrls[5];
    DWORD dwMemory;
    DEVMODEW DevMode;
    IDirect3D9 * ppObj = NULL;
    D3DADAPTER_IDENTIFIER9 Identifier;
    HRESULT hResult;

    szText[0] = L'\0';

    /* fix wine */
    //ppObj = Direct3DCreate9(D3D_SDK_VERSION);
    if (ppObj)
    {
        hResult = IDirect3D9_GetAdapterIdentifier(ppObj, D3DADAPTER_DEFAULT , 2/*D3DENUM_WHQL_LEVEL*/, &Identifier);
        if (hResult == D3D_OK)
        {

            if (Identifier.WHQLLevel)
            {
                /* adapter is WHQL certified */
                LoadStringW(hInst, IDS_OPTION_YES, szText, sizeof(szText)/sizeof(WCHAR));
            }
            else
            {
                LoadStringW(hInst, IDS_NOT_APPLICABLE, szText, sizeof(szText)/sizeof(WCHAR));
            }
        }
        IDirect3D9_Release(ppObj);
    }
    else
    {
        LoadStringW(hInst, IDS_NOT_APPLICABLE, szText, sizeof(szText)/sizeof(WCHAR));
    }
    szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
    SendDlgItemMessageW(hwndDlg, IDC_STATIC_ADAPTER_LOGO, WM_SETTEXT, 0, (LPARAM)szText);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, &pDispDevice->DeviceKey[18], 0, KEY_READ, &hKey) != ERROR_SUCCESS)
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
        wsprintfW(szText, szFormat, dwMemory);
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_ADAPTER_MEM, WM_SETTEXT, 0, (LPARAM)szText);
    }

    /* retrieve current display mode */
    DevMode.dmSize = sizeof(DEVMODEW);
    if (EnumDisplaySettingsW(pDispDevice->DeviceName, ENUM_CURRENT_SETTINGS, &DevMode))
    {
        szFormat[0] = L'\0';
        if (LoadStringW(hInst, IDS_FORMAT_ADAPTER_MODE, szFormat, sizeof(szFormat)/sizeof(WCHAR)))
            szFormat[(sizeof(szFormat)/sizeof(WCHAR))-1] = L'\0';
        wsprintfW(szText, szFormat, DevMode.dmPelsWidth, DevMode.dmPelsHeight, DevMode.dmBitsPerPel, DevMode.dmDisplayFrequency);
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_ADAPTER_MODE, WM_SETTEXT, 0, (LPARAM)szText);
    }

    /* query attached monitor */
    wcscpy(szText, pDispDevice->DeviceName);
    ZeroMemory(pDispDevice, sizeof(DISPLAY_DEVICEW));
    pDispDevice->cb = sizeof(DISPLAY_DEVICEW);
    if (EnumDisplayDevicesW(szText, 0, pDispDevice, 0))
    {
         /* set monitor name */
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_ADAPTER_MONITOR, WM_SETTEXT, 0, (LPARAM)pDispDevice->DeviceString);
    }

    hDlgCtrls[0] = GetDlgItem(hwndDlg, IDC_STATIC_ADAPTER_ID);
    hDlgCtrls[1] = GetDlgItem(hwndDlg, IDC_STATIC_ADAPTER_VENDOR);
    hDlgCtrls[2] = GetDlgItem(hwndDlg, IDC_STATIC_ADAPTER_DRIVER);
    hDlgCtrls[3] = GetDlgItem(hwndDlg, IDC_STATIC_ADAPTER_VERSION);
    hDlgCtrls[4] = GetDlgItem(hwndDlg, IDC_STATIC_ADAPTER_DATE);

    DisplayPageSetDeviceDetails(hDlgCtrls, &GUID_DEVCLASS_DISPLAY, NULL);
    return TRUE;
}

void InitializeDisplayAdapters(PDXDIAG_CONTEXT pContext)
{
    DISPLAY_DEVICEW DispDevice;
    HWND * hDlgs;
    HWND hwndDlg;
    WCHAR szDisplay[20];
    WCHAR szText[30];
    DWORD dwOffset = 0;

    while(TRUE)
    {
        ZeroMemory(&DispDevice, sizeof(DISPLAY_DEVICEW));
        DispDevice.cb = sizeof(DISPLAY_DEVICEW);
        if (!EnumDisplayDevicesW(NULL, pContext->NumDisplayAdapter + dwOffset, &DispDevice, 0))
            return;

        /* skip devices not attached to the desktop and mirror drivers */
        if (!(DispDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) || (DispDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
        {
            dwOffset++;
            continue;
        }
        if (pContext->NumDisplayAdapter)
            hDlgs = HeapReAlloc(GetProcessHeap(), 0, pContext->hDisplayWnd, (pContext->NumDisplayAdapter + 1) * sizeof(HWND));
        else
            hDlgs = HeapAlloc(GetProcessHeap(), 0, (pContext->NumDisplayAdapter + 1) * sizeof(HWND));

        if (!hDlgs)
            break;

        pContext->hDisplayWnd = hDlgs;
        hwndDlg = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_DISPLAY_DIALOG), pContext->hMainDialog, DisplayPageWndProc, (LPARAM)pContext); EnableDialogTheme(hwndDlg);
        if (!hwndDlg)
           break;

        /* initialize the dialog */
        InitializeDialog(hwndDlg, &DispDevice);

        szDisplay[0] = L'\0';
        LoadStringW(hInst, IDS_DISPLAY_DIALOG, szDisplay, sizeof(szDisplay)/sizeof(WCHAR));
        szDisplay[(sizeof(szDisplay)/sizeof(WCHAR))-1] = L'\0';

        wsprintfW (szText, L"%s %u", szDisplay, pContext->NumDisplayAdapter + 1);
        InsertTabCtrlItem(GetDlgItem(pContext->hMainDialog, IDC_TAB_CONTROL), pContext->NumDisplayAdapter + 1, szText);

        hDlgs[pContext->NumDisplayAdapter] = hwndDlg;
        pContext->NumDisplayAdapter++;
    }


}


INT_PTR CALLBACK
DisplayPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    PDXDIAG_CONTEXT pContext = (PDXDIAG_CONTEXT)GetWindowLongPtr(hDlg, DWLP_USER);
    switch (message)
    {
        case WM_INITDIALOG:
        {
            pContext = (PDXDIAG_CONTEXT) lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pContext);
            SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_BUTTON_TESTDD:
                case IDC_BUTTON_TEST3D:
                    GetWindowRect(pContext->hMainDialog, &rect);
                    /* FIXME log result errors */
                    if (IDC_BUTTON_TESTDD == LOWORD(wParam))
                        DDTests();
                    else if (IDC_BUTTON_TEST3D == LOWORD(wParam))
                        D3DTests();
                    SetWindowPos(pContext->hMainDialog, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
                    break;
            }
            break;
        }
    }

    return FALSE;
}

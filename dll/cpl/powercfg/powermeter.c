/*
 * PROJECT:         ReactOS Power Configuration Applet
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/powercfg/powermeter.c
 * PURPOSE:         hibernate tab of applet
 * PROGRAMMERS:     Alexander Wurzinger (Lohnegrim at gmx dot net)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  Martin Rottensteiner
 *                  Dmitry Chapyshev (lentind@yandex.ru)
 */

#include "powercfg.h"

static int SelectedBattery = 0;
static HWND hwndDlgDetail = 0;

typedef struct
{
    WCHAR Name[200];
    WCHAR UniqueID[200];
    WCHAR Manufacturer[200];
    CHAR Chem[5];
    BOOL ACOnline;
    BOOL Charging;
    BOOL Critical;
    BYTE BatteryLifePercent;
    DWORD BatteryLifeTime;
} POWER_METER_INFO, *PPOWER_METER_INFO;

static
VOID
PowerMeterInfo_UpdateGlobalStats(PPOWER_METER_INFO ppmi)
{
    SYSTEM_POWER_STATUS sps;

    if (GetSystemPowerStatus(&sps))
    {
        ppmi->ACOnline = sps.ACLineStatus != 0;
        ppmi->Charging = sps.BatteryFlag & 8;
        ppmi->Critical = sps.BatteryFlag & 4;
        ppmi->BatteryLifePercent = sps.BatteryLifePercent;
        ppmi->BatteryLifeTime = sps.BatteryLifeTime;
    }
}

static
BOOL
PowerMeterInfo_UpdateBatteryStats(PPOWER_METER_INFO ppmi)
{
    HDEVINFO hDevInfo;
    SP_DEVICE_INTERFACE_DATA InfoData;
    DWORD dwIndex;
    DWORD dwSize;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W InterfaceData;
    HANDLE hDevice;
    DWORD dwWait;
    DWORD dwReceived;
    BATTERY_QUERY_INFORMATION bqi = {0};
    BATTERY_INFORMATION bi = {0};
    BATTERY_WAIT_STATUS bws = {0};
    BATTERY_STATUS bs;

    hDevInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_BATTERY, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        DPRINT1("SetupDiGetClassDevsW failed with %x\n", GetLastError());
        return FALSE;
    }

    InfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    dwIndex = SelectedBattery;
    if (!SetupDiEnumDeviceInterfaces(hDevInfo, 0, &GUID_DEVCLASS_BATTERY, dwIndex, &InfoData))
    {
        DPRINT("SetupDiEnumDeviceInterfaces failed with %x\n", GetLastError());
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return FALSE;
    }

    dwSize = 0;
    InterfaceData = NULL;
    if (!SetupDiGetInterfaceDeviceDetailW(hDevInfo, &InfoData, InterfaceData, dwSize, &dwSize, NULL))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            DPRINT1("SetupDiGetInterfaceDeviceDetailW failed with %x\n", GetLastError());
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return FALSE;
        }
    }
    InterfaceData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
    if (!InterfaceData)
    {
        DPRINT1("HeapAlloc failed with %x\n", GetLastError());
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return FALSE;
    }
    InterfaceData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
    if (!SetupDiGetInterfaceDeviceDetailW(hDevInfo, &InfoData, InterfaceData, dwSize, &dwSize, NULL))
    {
        DPRINT1("SetupDiGetInterfaceDeviceDetailW failed with %x\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, InterfaceData);
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return FALSE;
    }

    DPRINT("Opening battery %S\n", InterfaceData->DevicePath);
    hDevice = CreateFileW(
        InterfaceData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        DPRINT1("CreateFileW failed with %x\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, InterfaceData);
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return FALSE;
    }

    dwWait = 0;
    if (DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_TAG,
                        &dwWait, sizeof(DWORD), &bqi.BatteryTag, sizeof(bqi.BatteryTag),
                        &dwReceived,
                        NULL))
    {

        bqi.InformationLevel = BatteryDeviceName;
        if (DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_INFORMATION,
                            &bqi, sizeof(bqi), ppmi->Name, sizeof(ppmi->Name),
                            &dwReceived,
                            NULL))
        {
            ppmi->Name[dwReceived / sizeof(WCHAR)] = 0;
        }
        else
        {
            ppmi->Name[0] = 0;
        }

        bqi.InformationLevel = BatteryUniqueID;
        if (DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_INFORMATION,
                            &bqi, sizeof(bqi), ppmi->UniqueID, sizeof(ppmi->UniqueID),
                            &dwReceived, NULL))
        {
            ppmi->UniqueID[dwReceived / sizeof(WCHAR)] = 0;
        }
        else
        {
            ppmi->UniqueID[0] = 0;
        }

        bqi.InformationLevel = BatteryInformation;
        if (DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi), &bi, sizeof(bi), &dwReceived, NULL))
        {
            RtlCopyMemory(ppmi->Chem, bi.Chemistry, sizeof(bi.Chemistry));
            ppmi->Chem[4] = 0;
        }
        else
        {
            ppmi->Chem[0] = 0;
        }

        bws.BatteryTag = bqi.BatteryTag;
        if (DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_STATUS, &bws, sizeof(bws), &bs, sizeof(bs), &dwReceived, NULL))
        {
            ppmi->ACOnline = bs.PowerState & BATTERY_POWER_ON_LINE;
            ppmi->Charging = bs.PowerState & BATTERY_CHARGING && !(bs.PowerState & BATTERY_DISCHARGING);
            ppmi->Critical = bs.PowerState & BATTERY_CRITICAL;
            ppmi->BatteryLifePercent = 100 * bs.Capacity / bi.FullChargedCapacity;
            ppmi->BatteryLifeTime = -1;
        }

        bqi.InformationLevel = BatteryManufactureName;
        if (DeviceIoControl(
                hDevice, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi), ppmi->Manufacturer, sizeof(ppmi->Manufacturer),
                &dwReceived, NULL))
        {
            ppmi->Manufacturer[dwReceived / sizeof(WCHAR)] = 0;
        }
        else
        {
            ppmi->Manufacturer[0] = 0;
        }
    }
    HeapFree(GetProcessHeap(), 0, InterfaceData);
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return TRUE;
}

static
VOID
PowerMeterDetail_UpdateStats(HWND hwndDlg)
{
    POWER_METER_INFO pmi;
    WCHAR Status[200];
    WCHAR Buffer[200];

    PowerMeterInfo_UpdateBatteryStats(&pmi);

    SetDlgItemTextW(hwndDlg, IDC_BATTERYNAME, pmi.Name);
    SetDlgItemTextW(hwndDlg, IDC_BATTERYUNIQUEID, pmi.UniqueID);
    SetDlgItemTextA(hwndDlg, IDC_BATTERYCHEMISTRY, pmi.Chem);

    Status[0] = UNICODE_NULL;
    if (pmi.ACOnline)
    {
        if (LoadString(hApplet, IDS_ONLINE, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
        {
            wcscpy(Status, Buffer);
        }
    }
    if (pmi.Charging)
    {
        if (LoadString(hApplet, IDS_CHARGING, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
        {
            if (Status[0] != UNICODE_NULL)
            {
                wcscat(Status, L", ");
            }
            wcscat(Status, Buffer);
        }
    }
    else
    {
        if (LoadString(hApplet, IDS_DISCHARGING, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
        {
            if (Status[0] != UNICODE_NULL)
            {
                wcscat(Status, L", ");
            }
            wcscat(Status, Buffer);
        }
    }
    // TODO BATTERY_CRITICAL
    SetDlgItemTextW(hwndDlg, IDC_BATTERYPOWERSTATE, Status);

    SetDlgItemTextW(hwndDlg, IDC_BATTERYMANUFACTURER, pmi.Manufacturer);
}


static
VOID
PowerMeterDetail_InitDialog(HWND hwndDlg)
{
    WCHAR FormatBuffer[200];
    WCHAR Buffer[200];

    if (LoadString(hApplet, IDS_DETAILEDBATTERY, FormatBuffer, sizeof(Buffer) / sizeof(WCHAR)))
    {
        wsprintf(Buffer, FormatBuffer, SelectedBattery + 1);
        SetWindowTextW(hwndDlg, Buffer);
    }
    PowerMeterDetail_UpdateStats(hwndDlg);
}

INT_PTR
CALLBACK
PowerMeterDetailDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            PowerMeterDetail_InitDialog(hwndDlg);
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_REFRESH)
            {
                PowerMeterDetail_InitDialog(hwndDlg);
                break;
            }
            else if (LOWORD(wParam) == IDOK)
            {
                DestroyWindow(hwndDlgDetail);
                hwndDlgDetail = 0;
                return TRUE;
            }

    }
    return FALSE;
}


static
VOID
PowerMeter_InitDialog(HWND hwndDlg)
{
    POWER_METER_INFO pmi;
    WCHAR Buffer[200];

    PowerMeterInfo_UpdateGlobalStats(&pmi);
    if (pmi.ACOnline)
    {
        if (LoadString(hApplet, IDS_ONLINE, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
        {
            SetDlgItemTextW(hwndDlg, IDC_POWERSOURCE, Buffer);
        }
    }
    else
    {
        if (LoadString(hApplet, IDS_OFFLINE, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
        {
            SetDlgItemTextW(hwndDlg, IDC_POWERSOURCE, Buffer);
        }
    }
    if (pmi.Charging)
    {
        if (LoadString(hApplet, IDS_CHARGING, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
        {
            SetDlgItemTextW(hwndDlg, IDC_BATTERYCHARGING0 + SelectedBattery, Buffer);
        }
    }
    else
    {
        if (LoadString(hApplet, IDS_DISCHARGING, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
        {
            SetDlgItemTextW(hwndDlg, IDC_BATTERYCHARGING0 + SelectedBattery, Buffer);
        }
    }
    wsprintf(Buffer, L"%d %%", pmi.BatteryLifePercent);
    SetDlgItemTextW(hwndDlg, IDC_POWERSTATUS, Buffer);

    for (SelectedBattery = 0; SelectedBattery < 8; SelectedBattery++)
    {
        if (!PowerMeterInfo_UpdateBatteryStats(&pmi))
            break;

        ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERY0 + SelectedBattery), SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, IDI_BATTERYDETAIL0 + SelectedBattery), SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERYPERCENT0 + SelectedBattery), SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERYCHARGING0 + SelectedBattery), SW_SHOW);
        if (pmi.Charging)
        {
            if (LoadString(hApplet, IDS_CHARGING, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
            {
                SetDlgItemTextW(hwndDlg, IDC_BATTERYCHARGING0 + SelectedBattery, Buffer);
            }
        }
        else
        {
            if (LoadString(hApplet, IDS_DISCHARGING, Buffer, sizeof(Buffer) / sizeof(WCHAR)))
            {
                SetDlgItemTextW(hwndDlg, IDC_BATTERYCHARGING0 + SelectedBattery, Buffer);
            }
        }
        wsprintf(Buffer, L"%d %%", pmi.BatteryLifePercent);
        SetDlgItemTextW(hwndDlg, IDC_BATTERYPERCENT0 + SelectedBattery, Buffer);
    }
    for (; SelectedBattery < 8; SelectedBattery++)
    {
        ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERY0 + SelectedBattery), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDI_BATTERYDETAIL0 + SelectedBattery), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERYPERCENT0 + SelectedBattery), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERYCHARGING0 + SelectedBattery), SW_HIDE);
    }
}

/* Property page dialog callback */
INT_PTR
CALLBACK
PowerMeterDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            PowerMeter_InitDialog(hwndDlg);
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) >= IDI_BATTERYDETAIL0 && LOWORD(wParam) <= IDI_BATTERYDETAIL7)
            {
                if (!IsWindow(hwndDlgDetail))
                {
                    SelectedBattery = LOWORD(wParam) - IDI_BATTERYDETAIL0;
                    hwndDlgDetail =
                        CreateDialog(hApplet, MAKEINTRESOURCE(IDD_POWERMETERDETAILS), hwndDlg, PowerMeterDetailDlgProc);
                    ShowWindow(hwndDlgDetail, SW_SHOW);
                }

            }
            break;
    }
    return FALSE;
}

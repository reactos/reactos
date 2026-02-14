/*
 * PROJECT:     ReactOS Power Configuration Applet
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Meter & Battery tab
 * COPYRIGHT:   Copyright 2025 Johannes Anderwald <johannes.anderwald@reactos.org>
 *              Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "powercfg.h"
#include <debug.h>

static UINT SelectedBattery = 0;  ///< ID of battery currently shown in the details dialog.
static HWND hwndDlgDetail = NULL; ///< Single modeless battery details dialog.

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
PowerMeterInfo_UpdateGlobalStats(
    _In_ PPOWER_METER_INFO ppmi)
{
    SYSTEM_POWER_STATUS sps;

    ZeroMemory(ppmi, sizeof(*ppmi));

    if (GetSystemPowerStatus(&sps))
    {
        ppmi->ACOnline = (sps.ACLineStatus != 0);
        ppmi->Charging = !!(sps.BatteryFlag & BATTERY_FLAG_CHARGING);
        ppmi->Critical = !!(sps.BatteryFlag & BATTERY_FLAG_CRITICAL);
        ppmi->BatteryLifePercent = sps.BatteryLifePercent;
        ppmi->BatteryLifeTime = sps.BatteryLifeTime;
    }
}

static
BOOL
PowerMeterInfo_UpdateBatteryStats(
    _In_ PPOWER_METER_INFO ppmi,
    _In_ UINT BatteryId)
{
    HDEVINFO hDevInfo;
    SP_DEVICE_INTERFACE_DATA InfoData;
    DWORD dwSize;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W InterfaceData;
    HANDLE hDevice;
    DWORD dwWait;
    DWORD dwReceived;
    BATTERY_QUERY_INFORMATION bqi = {0};
    BATTERY_INFORMATION bi = {0};
    BATTERY_WAIT_STATUS bws = {0};
    BATTERY_STATUS bs;

    ZeroMemory(ppmi, sizeof(*ppmi));

    hDevInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_BATTERY, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        DPRINT1("SetupDiGetClassDevsW failed with %x\n", GetLastError());
        return FALSE;
    }

    InfoData.cbSize = sizeof(InfoData);
    if (!SetupDiEnumDeviceInterfaces(hDevInfo, 0, &GUID_DEVCLASS_BATTERY, BatteryId, &InfoData))
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
    InterfaceData->cbSize = sizeof(*InterfaceData);
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
                        &dwWait, sizeof(dwWait), &bqi.BatteryTag, sizeof(bqi.BatteryTag),
                        &dwReceived, NULL))
    {
        bqi.InformationLevel = BatteryDeviceName;
        if (DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_INFORMATION,
                            &bqi, sizeof(bqi), ppmi->Name, sizeof(ppmi->Name),
                            &dwReceived, NULL))
        {
            ppmi->Name[dwReceived / sizeof(WCHAR)] = UNICODE_NULL;
        }
        else
        {
            ppmi->Name[0] = UNICODE_NULL;
        }

        bqi.InformationLevel = BatteryUniqueID;
        if (DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_INFORMATION,
                            &bqi, sizeof(bqi), ppmi->UniqueID, sizeof(ppmi->UniqueID),
                            &dwReceived, NULL))
        {
            ppmi->UniqueID[dwReceived / sizeof(WCHAR)] = UNICODE_NULL;
        }
        else
        {
            ppmi->UniqueID[0] = UNICODE_NULL;
        }

        bqi.InformationLevel = BatteryInformation;
        if (DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_INFORMATION,
                            &bqi, sizeof(bqi), &bi, sizeof(bi),
                            &dwReceived, NULL))
        {
            RtlCopyMemory(ppmi->Chem, bi.Chemistry, sizeof(bi.Chemistry));
            ppmi->Chem[4] = ANSI_NULL;
        }
        else
        {
            ppmi->Chem[0] = ANSI_NULL;
        }

        bws.BatteryTag = bqi.BatteryTag;
        if (DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_STATUS,
                            &bws, sizeof(bws), &bs, sizeof(bs),
                            &dwReceived, NULL))
        {
            ppmi->ACOnline = !!(bs.PowerState & BATTERY_POWER_ON_LINE);
            ppmi->Charging = (bs.PowerState & BATTERY_CHARGING) && !(bs.PowerState & BATTERY_DISCHARGING);
            ppmi->Critical = !!(bs.PowerState & BATTERY_CRITICAL);
            ppmi->BatteryLifePercent = 100 * bs.Capacity / bi.FullChargedCapacity;
            ppmi->BatteryLifeTime = BATTERY_LIFE_UNKNOWN;
        }

        bqi.InformationLevel = BatteryEstimatedTime;
        if (!DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_INFORMATION,
                             &bqi, sizeof(bqi), &ppmi->BatteryLifeTime, sizeof(ppmi->BatteryLifeTime),
                             &dwReceived, NULL))
        {
            ppmi->BatteryLifeTime = BATTERY_LIFE_UNKNOWN; // == BATTERY_UNKNOWN_TIME;
        }

        bqi.InformationLevel = BatteryManufactureName;
        if (DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_INFORMATION,
                            &bqi, sizeof(bqi), ppmi->Manufacturer, sizeof(ppmi->Manufacturer),
                            &dwReceived, NULL))
        {
            ppmi->Manufacturer[dwReceived / sizeof(WCHAR)] = UNICODE_NULL;
        }
        else
        {
            ppmi->Manufacturer[0] = UNICODE_NULL;
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

    PowerMeterInfo_UpdateBatteryStats(&pmi, SelectedBattery);

    SetDlgItemTextW(hwndDlg, IDC_BATTERYNAME, pmi.Name);
    SetDlgItemTextW(hwndDlg, IDC_BATTERYUNIQUEID, pmi.UniqueID);
    SetDlgItemTextA(hwndDlg, IDC_BATTERYCHEMISTRY, pmi.Chem);

    Status[0] = UNICODE_NULL;
    if (pmi.ACOnline)
    {
        if (LoadString(hApplet, IDS_ONLINE, Buffer, _countof(Buffer)))
        {
            wcscpy(Status, Buffer);
        }
    }
    if (pmi.Charging)
    {
        if (LoadString(hApplet, IDS_CHARGING, Buffer, _countof(Buffer)))
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
        if (LoadString(hApplet, IDS_DISCHARGING, Buffer, _countof(Buffer)))
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

    if (LoadString(hApplet, IDS_DETAILEDBATTERY, FormatBuffer, _countof(FormatBuffer)))
    {
        StringCchPrintfW(Buffer, _countof(Buffer), FormatBuffer, SelectedBattery + 1);
        SetWindowTextW(hwndDlg, Buffer);
    }
    PowerMeterDetail_UpdateStats(hwndDlg);
}

static VOID
CenterWindow(_In_ HWND hWnd)
{
    HWND hWndParent;
    RECT rcParent, rcWindow;
    SIZE size;
    POINT pos;
    MONITORINFO mi;
    HMONITOR hMonitor;

    hWndParent = GetParent(hWnd);
    if (hWndParent == NULL)
        hWndParent = GetDesktopWindow();

    GetWindowRect(hWndParent, &rcParent);
    GetWindowRect(hWnd, &rcWindow);
    size.cx = rcWindow.right - rcWindow.left;
    size.cy = rcWindow.bottom - rcWindow.top;

    mi.cbSize = sizeof(mi);
    // hMonitor = MonitorFromRect(rcParent, MONITOR_DEFAULTTONEAREST);
    hMonitor = MonitorFromWindow(hWndParent, MONITOR_DEFAULTTONEAREST);
    GetMonitorInfoW(hMonitor, &mi);

    /* Center the window with respect to its parent. Formula:
     * with: pL = parent.left, pR = parent.right, w = window width,
     * X = pL + (pR - pL - w) / 2; and similar for Y, top, bottom, height. */
    pos.x = (rcParent.right + rcParent.left - size.cx) / 2;
    pos.y = (rcParent.bottom + rcParent.top - size.cy) / 2;

    /* Clip it to the monitor work area, so that top-left stays visible */
    pos.x = max(mi.rcWork.left, min(pos.x, mi.rcWork.right - size.cx));
    pos.y = max(mi.rcWork.top, min(pos.y, mi.rcWork.bottom - size.cy));

    SetWindowPos(hWnd, HWND_TOP, pos.x, pos.y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
}

INT_PTR
CALLBACK
PowerMeterDetailDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            PowerMeterDetail_InitDialog(hwndDlg);
            CenterWindow(hwndDlg);
            hwndDlgDetail = hwndDlg;
            return TRUE;
        }

        case WM_ACTIVATE:
        {
            /* Refresh the dialog on activation */
            if (LOWORD(wParam) != WA_INACTIVE)
                PowerMeterDetail_InitDialog(hwndDlg);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDC_REFRESH:
                PowerMeterDetail_InitDialog(hwndDlg);
                break;

            case IDOK:
            case IDCANCEL:
                hwndDlgDetail = NULL;
                // DestroyWindow(hwndDlgDetail);
                EndDialog(hwndDlg, 0);
                return TRUE;

            default:
                break;
            }
    }
    return FALSE;
}

static DWORD WINAPI
PowerMeterDetailDlgThread(PVOID pParam)
{
    // HWND hwndDlg = (HWND)pParam;

    /* Use DialogBoxW() instead of CreateDialogW(), so that we can directly
     * use the default dialog message loop implementation furnished for modal
     * dialog boxes, instead of doing it ourselves for a modeless dialog.
     * But, set the parent to NULL so that we behave "like" a modeless dialog. */
    // hwndDlgDetail =
    DialogBoxW(hApplet, MAKEINTRESOURCEW(IDD_POWERMETERDETAILS), NULL/*hwndDlg*/, PowerMeterDetailDlgProc);
    return 0;
}


static
HICON
GetBattIcon(
    _In_ HINSTANCE hInst,
    _In_ PPOWER_METER_INFO ppmi)
{
    UINT uIDIcon = IDI_BAT_UNKNOWN;

    // gGPP.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].BatteryLevel
    if (ppmi->Critical || (0 <= ppmi->BatteryLifePercent && ppmi->BatteryLifePercent < 5)) // FIXME: Find which percentage is critical?
        uIDIcon = (ppmi->Charging ? IDI_BAT_CHARGE_CRIT : IDI_BAT_DISCHRG_CRIT);
    else if (5 <= ppmi->BatteryLifePercent && ppmi->BatteryLifePercent < 33)
        uIDIcon = (ppmi->Charging ? IDI_BAT_CHARGE_LOW  : IDI_BAT_DISCHRG_LOW);
    else if (33 <= ppmi->BatteryLifePercent && ppmi->BatteryLifePercent < 66)
        uIDIcon = (ppmi->Charging ? IDI_BAT_CHARGE_HALF : IDI_BAT_DISCHRG_HALF);
    else if (66 <= ppmi->BatteryLifePercent && ppmi->BatteryLifePercent < 100)
        uIDIcon = (ppmi->Charging ? IDI_BAT_CHARGE_HIGH : IDI_BAT_DISCHRG_HIGH);
    else if (ppmi->BatteryLifePercent >= 100)
        uIDIcon = (ppmi->Charging ? IDI_BAT_CHARGE_FULL : IDI_BAT_DISCHRG_FULL);

    return LoadIconW(hApplet, MAKEINTRESOURCEW(uIDIcon));
}

static
VOID
PowerMeter_InitDialog(HWND hwndDlg)
{
    POWER_METER_INFO pmi;
    HICON hIcon;
    UINT BatteryId;
    WCHAR szPercent[5];
    WCHAR Buffer[200];

    *szPercent = UNICODE_NULL;
    LoadString(hApplet, IDS_PERCENT, szPercent, _countof(szPercent));

    PowerMeterInfo_UpdateGlobalStats(&pmi);
    if (pmi.ACOnline)
    {
        hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDI_AC));
        SendDlgItemMessageW(hwndDlg, IDC_IPOWERSOURCE, STM_SETICON, (WPARAM)hIcon, 0);

        if (LoadString(hApplet, IDS_ONLINE, Buffer, _countof(Buffer)))
            SetDlgItemTextW(hwndDlg, IDC_POWERSOURCE, Buffer);
    }
    else
    {
        hIcon = GetBattIcon(hApplet, &pmi);
        SendDlgItemMessageW(hwndDlg, IDC_IPOWERSOURCE, STM_SETICON, (WPARAM)hIcon, 0);
        // DeleteObject(hIcon);

        if (LoadString(hApplet, IDS_OFFLINE, Buffer, _countof(Buffer)))
            SetDlgItemTextW(hwndDlg, IDC_POWERSOURCE, Buffer);
    }
    StringCchPrintfW(Buffer, _countof(Buffer), szPercent, pmi.BatteryLifePercent);
    SetDlgItemTextW(hwndDlg, IDC_POWERSTATUS, Buffer);

    /* Check whether to show only the composite battery gauge, or the individual battery details */
    if (IsDlgButtonChecked(hwndDlg, IDC_SHOWDETAILS) != BST_CHECKED)
    {
        /* Hide the individual battery controls */
        for (BatteryId = 0; BatteryId < 8; ++BatteryId)
        {
            ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERY0 + BatteryId), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDI_BATTERYDETAIL0 + BatteryId), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERYPERCENT0 + BatteryId), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERYCHARGING0 + BatteryId), SW_HIDE);
        }
        ShowWindow(GetDlgItem(hwndDlg, IDC_CLICKBATTINFO), SW_HIDE);

        /* Show the composite battery gauge and set it up (0 to 100%) */
        ShowWindow(GetDlgItem(hwndDlg, IDC_BATTPROGRESS), SW_SHOW);
        SendDlgItemMessageW(hwndDlg, IDC_BATTPROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendDlgItemMessageW(hwndDlg, IDC_BATTPROGRESS, PBM_SETPOS, min(pmi.BatteryLifePercent, 100), 0);
        return;
    }

    /* Hide the composite battery gauge */
    ShowWindow(GetDlgItem(hwndDlg, IDC_BATTPROGRESS), SW_HIDE);

    /* Show or hide the individual battery controls */
    ShowWindow(GetDlgItem(hwndDlg, IDC_CLICKBATTINFO), SW_SHOW);
    for (BatteryId = 0; BatteryId < 8; ++BatteryId)
    {
        if (!PowerMeterInfo_UpdateBatteryStats(&pmi, BatteryId))
            break;

        ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERY0 + BatteryId), SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, IDI_BATTERYDETAIL0 + BatteryId), SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERYPERCENT0 + BatteryId), SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERYCHARGING0 + BatteryId), SW_SHOW);

        hIcon = GetBattIcon(hApplet, &pmi);
        SendDlgItemMessageW(hwndDlg, IDI_BATTERYDETAIL0 + BatteryId, STM_SETICON, (WPARAM)hIcon, 0);
        // DeleteObject(hIcon);

        if (pmi.Charging)
        {
            if (LoadString(hApplet, IDS_CHARGING, Buffer, _countof(Buffer)))
                SetDlgItemTextW(hwndDlg, IDC_BATTERYCHARGING0 + BatteryId, Buffer);
        }
        else
        {
            if (LoadString(hApplet, IDS_DISCHARGING, Buffer, _countof(Buffer)))
                SetDlgItemTextW(hwndDlg, IDC_BATTERYCHARGING0 + BatteryId, Buffer);
        }
        StringCchPrintfW(Buffer, _countof(Buffer), szPercent, pmi.BatteryLifePercent);
        SetDlgItemTextW(hwndDlg, IDC_BATTERYPERCENT0 + BatteryId, Buffer);
    }
    for (; BatteryId < 8; ++BatteryId)
    {
        ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERY0 + BatteryId), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDI_BATTERYDETAIL0 + BatteryId), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERYPERCENT0 + BatteryId), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_BATTERYCHARGING0 + BatteryId), SW_HIDE);
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
            if (LOWORD(wParam) == IDC_SHOWDETAILS)
            {
                PowerMeter_InitDialog(hwndDlg);
                break;
            }
            if (LOWORD(wParam) >= IDI_BATTERYDETAIL0 && LOWORD(wParam) <= IDI_BATTERYDETAIL7)
            {
                SelectedBattery = LOWORD(wParam) - IDI_BATTERYDETAIL0;
                if (!IsWindow(hwndDlgDetail))
                {
                    HANDLE Thread = CreateThread(NULL, 0,
                                                 PowerMeterDetailDlgThread,
                                                 (PVOID)hwndDlg,
                                                 0, NULL);
                    if (Thread)
                        CloseHandle(Thread);
                }
                else
                {
                    /* Activate the existing window */
                    ShowWindow(hwndDlgDetail, SW_SHOWNA);
                    SwitchToThisWindow(hwndDlgDetail, TRUE);
                }
            }
            break;
    }
    return FALSE;
}

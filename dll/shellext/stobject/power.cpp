/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/stobject/power.cpp
 * PURPOSE:     Power notification icon handler
 * PROGRAMMERS: Eric Kohl <eric.kohl@reactos.org>
                Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 *              David Quintana <gigaherz@gmail.com>
 */

#include "precomp.h"

#include <devguid.h>
#include <winioctl.h>
#include <powrprof.h>
#include <windows.h>
#include <batclass.h>

#define GBS_HASBATTERY 0x1
#define GBS_ONBATTERY  0x2

int br_icons[5] = { IDI_BATTCAP0, IDI_BATTCAP1, IDI_BATTCAP2, IDI_BATTCAP3, IDI_BATTCAP4 }; // battery mode icons.
int bc_icons[5] = { IDI_BATTCHA0, IDI_BATTCHA1, IDI_BATTCHA2, IDI_BATTCHA3, IDI_BATTCHA4 }; // charging mode icons.

typedef struct _PWRSCHEMECONTEXT
{
    HMENU hPopup;
    UINT uiFirst;
    UINT uiLast;
} PWRSCHEMECONTEXT, *PPWRSCHEMECONTEXT;

CString  g_strTooltip;
static float g_batCap = 0;
static HICON g_hIconBattery = NULL;
static BOOL g_IsRunning = FALSE;

/*++
* @name GetBatteryState
*
* Enumerates the available battery devices and provides the remaining capacity.
*
* @param cap
*        If no error occurs, then this will contain average remaining capacity.
* @param dwResult
*        Helps in making battery type checks.
*       {
*           Returned value includes GBS_HASBATTERY if the system has a non-UPS battery,
*           and GBS_ONBATTERY if the system is running on a battery.
*           dwResult & GBS_ONBATTERY means we have not yet found AC power.
*           dwResult & GBS_HASBATTERY means we have found a non-UPS battery.
*       }
*
* @return The error code.
*
*--*/
static HRESULT GetBatteryState(float& cap, DWORD& dwResult)
{
    cap = 0;
    dwResult = GBS_ONBATTERY;

    HDEVINFO hdev = SetupDiGetClassDevs(&GUID_DEVCLASS_BATTERY, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (INVALID_HANDLE_VALUE == hdev)
        return E_HANDLE;

    // Limit search to 100 batteries max
    for (int idev = 0, count = 0; idev < 100; idev++)
    {
        SP_DEVICE_INTERFACE_DATA did = { 0 };
        did.cbSize = sizeof(did);

        if (SetupDiEnumDeviceInterfaces(hdev, 0, &GUID_DEVCLASS_BATTERY, idev, &did))
        {
            DWORD cbRequired = 0;

            SetupDiGetDeviceInterfaceDetail(hdev, &did, 0, 0, &cbRequired, 0);
            if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
            {
                PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);
                if (pdidd)
                {
                    pdidd->cbSize = sizeof(*pdidd);
                    if (SetupDiGetDeviceInterfaceDetail(hdev, &did, pdidd, cbRequired, &cbRequired, 0))
                    {
                        // Enumerated a battery.  Ask it for information.
                        HANDLE hBattery = CreateFile(pdidd->DevicePath, GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                        if (INVALID_HANDLE_VALUE != hBattery)
                        {
                            // Ask the battery for its tag.
                            BATTERY_QUERY_INFORMATION bqi = { 0 };

                            DWORD dwWait = 0;
                            DWORD dwOut;

                            if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_TAG, &dwWait, sizeof(dwWait), &bqi.BatteryTag,
                                sizeof(bqi.BatteryTag), &dwOut, NULL) && bqi.BatteryTag)
                            {
                                // With the tag, you can query the battery info.
                                BATTERY_INFORMATION bi = { 0 };
                                bqi.InformationLevel = BatteryInformation;

                                if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi), &bi,
                                    sizeof(bi), &dwOut, NULL))
                                {
                                    // Only non-UPS system batteries count
                                    if (bi.Capabilities & BATTERY_SYSTEM_BATTERY)
                                    {
                                        if (!(bi.Capabilities & BATTERY_IS_SHORT_TERM))
                                            dwResult |= GBS_HASBATTERY;

                                        // Query the battery status.
                                        BATTERY_WAIT_STATUS bws = { 0 };
                                        bws.BatteryTag = bqi.BatteryTag;

                                        BATTERY_STATUS bs;
                                        if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_STATUS, &bws, sizeof(bws),
                                            &bs, sizeof(bs), &dwOut, NULL))
                                        {
                                            if (bs.PowerState & BATTERY_POWER_ON_LINE)
                                                dwResult &= ~GBS_ONBATTERY;

                                            // Take average of total capacity of batteries detected!
                                            cap = cap*(count)+(float)bs.Capacity / bi.FullChargedCapacity * 100;
                                            cap /= count + 1;
                                            count++;
                                        }
                                    }
                                }
                            }
                            CloseHandle(hBattery);
                        }
                    }
                    LocalFree(pdidd);
                }
            }
        }
        else  if (ERROR_NO_MORE_ITEMS == GetLastError())
        {
            break;  // Enumeration failed - perhaps we're out of items
        }
    }
    SetupDiDestroyDeviceInfoList(hdev);

    //  Final cleanup:  If we didn't find a battery, then presume that we
    //  are on AC power.

    if (!(dwResult & GBS_HASBATTERY))
        dwResult &= ~GBS_ONBATTERY;

    return S_OK;
}

/*++
* @name Quantize
* 
* This function quantizes the mentioned quantity to nearest level.
* 
* @param p
*        Should be a quantity in percentage.
* @param lvl
*        Quantization level (this excludes base level 0, which will always be present), default is 10.
*
* @return Nearest quantized level, can be directly used as array index based on context.
*
*--*/
static UINT Quantize(float p, UINT lvl = 10)
{
    int i = 0;
    float f, q = (float)100 / lvl, d = q / 2;
    for (f = 0; f < p; f += q, i++);

    if ((f - d) <= p)
        return i;
    else
        return i - 1;
/* 
 @remarks This function uses centred/symmetric logic for quantization.
 For the case of lvl = 4, You will get following integer levels if given (p) value falls in between the range partitions:
     0    <= p <  12.5 : returns 0; (corresponding to 0% centre)
     12.5 <= p <  37.5 : returns 1; (corresponding to 25% centre)
     37.5 <= p <  62.5 : returns 2; (corresponding to 50% centre)
     62.5 <= p <  87.5 : returns 3; (corresponding to 75% centre)
     87.5 <= p <= 100  : returns 4; (corresponding to 100% centre)
*/
}

/*++
* @name DynamicLoadIcon
*
* Returns the respective icon as per the current battery capacity.
* It also does the work of setting global parameters of battery capacity and tooltips.
*
* @param hinst
*        A handle to a instance of the module.
*
* @return The handle to respective battery icon.
*
*--*/
static HICON DynamicLoadIcon(HINSTANCE hinst)
{    
    HICON hBatIcon;
    float cap = 0;
    DWORD dw = 0;
    UINT index = -1;
    HRESULT hr = GetBatteryState(cap, dw);

    if (!FAILED(hr) && (dw & GBS_HASBATTERY))
    {
        index = Quantize(cap, 4);
        g_batCap = cap;
    }
    else
    {
        g_batCap = 0;
        hBatIcon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_BATTCAP_ERR));
        g_strTooltip.LoadStringW(IDS_PWR_UNKNOWN_REMAINING);
        return hBatIcon;
    }

    if (dw & GBS_ONBATTERY)
    {
        hBatIcon = LoadIcon(hinst, MAKEINTRESOURCE(br_icons[index]));
        g_strTooltip.Format(IDS_PWR_PERCENT_REMAINING, cap);
    }
    else
    {
        hBatIcon = LoadIcon(hinst, MAKEINTRESOURCE(bc_icons[index])); 
        g_strTooltip.Format(IDS_PWR_CHARGING, cap);
    }

    return hBatIcon;
}

HRESULT STDMETHODCALLTYPE Power_Init(_In_ CSysTray * pSysTray)
{ 
    TRACE("Power_Init\n");
    g_hIconBattery = DynamicLoadIcon(g_hInstance);
    g_IsRunning = TRUE;

    return pSysTray->NotifyIcon(NIM_ADD, ID_ICON_POWER, g_hIconBattery, g_strTooltip);
}

HRESULT STDMETHODCALLTYPE Power_Update(_In_ CSysTray * pSysTray)
{
    TRACE("Power_Update\n");
    g_hIconBattery = DynamicLoadIcon(g_hInstance);

    return pSysTray->NotifyIcon(NIM_MODIFY, ID_ICON_POWER, g_hIconBattery, g_strTooltip);
}

HRESULT STDMETHODCALLTYPE Power_Shutdown(_In_ CSysTray * pSysTray)
{
    TRACE("Power_Shutdown\n");
    g_IsRunning = FALSE;

    return pSysTray->NotifyIcon(NIM_DELETE, ID_ICON_POWER, NULL, NULL);
}

static void _RunPower()
{
    ShellExecuteW(NULL, NULL, L"powercfg.cpl", NULL, NULL, SW_SHOWNORMAL);
}

static void _ShowContextMenu(CSysTray * pSysTray)
{
    CString strOpen((LPCSTR)IDS_PWR_PROPERTIES);
    HMENU hPopup = CreatePopupMenu();
    AppendMenuW(hPopup, MF_STRING, IDS_PWR_PROPERTIES, strOpen);

    SetForegroundWindow(pSysTray->GetHWnd());
    DWORD flags = TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN | TPM_BOTTOMALIGN;
    POINT pt;
    GetCursorPos(&pt);

    DWORD id = TrackPopupMenuEx(hPopup, flags,
        pt.x, pt.y,
        pSysTray->GetHWnd(), NULL);

    switch (id)
    {
        case IDS_PWR_PROPERTIES:
            _RunPower();
            break;
    }
    DestroyMenu(hPopup);
}

static
BOOLEAN
CALLBACK
PowerSchemesEnumProc(
    UINT uiIndex,
    DWORD dwName,
    LPWSTR sName,
    DWORD dwDesc,
    LPWSTR sDesc,
    PPOWER_POLICY pp,
    LPARAM lParam)
{
    PPWRSCHEMECONTEXT PowerSchemeContext = (PPWRSCHEMECONTEXT)lParam;

    if (AppendMenuW(PowerSchemeContext->hPopup, MF_STRING, uiIndex + 1, sName))
    {
        if (PowerSchemeContext->uiFirst == 0)
            PowerSchemeContext->uiFirst = uiIndex + 1;

        PowerSchemeContext->uiLast = uiIndex + 1;
    }

    return TRUE;
}

static
VOID
ShowPowerSchemesPopupMenu(
    CSysTray *pSysTray)
{
    PWRSCHEMECONTEXT PowerSchemeContext = {NULL, 0, 0};
    UINT uiActiveScheme;
    DWORD id;
    POINT pt;
    PowerSchemeContext.hPopup = CreatePopupMenu();
    EnumPwrSchemes(PowerSchemesEnumProc, (LPARAM)&PowerSchemeContext);

    if (GetActivePwrScheme(&uiActiveScheme))
    {
        CheckMenuRadioItem(PowerSchemeContext.hPopup,
                           PowerSchemeContext.uiFirst,
                           PowerSchemeContext.uiLast,
                           uiActiveScheme + 1,
                           MF_BYCOMMAND);
    }

    SetForegroundWindow(pSysTray->GetHWnd());
    GetCursorPos(&pt);
    
    id = TrackPopupMenuEx(PowerSchemeContext.hPopup,
                          TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                          pt.x,
                          pt.y,
                          pSysTray->GetHWnd(),
                          NULL);

    DestroyMenu(PowerSchemeContext.hPopup);

    if (id != 0)
        SetActivePwrScheme(id - 1, NULL, NULL);
}

HRESULT STDMETHODCALLTYPE Power_Message(_In_ CSysTray * pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult)
{
    TRACE("Power_Message uMsg=%d, wParam=%x, lParam=%x\n", uMsg, wParam, lParam);

    switch (uMsg)
    {
        case WM_USER + 220:
            TRACE("Power_Message: WM_USER+220\n");
            if (wParam == 1)
            {
                if (lParam == FALSE)
                    return Power_Init(pSysTray);
                else
                    return Power_Shutdown(pSysTray);
            }
            return S_FALSE;

        case WM_USER + 221:
            TRACE("Power_Message: WM_USER+221\n");
            if (wParam == 1)
            {
                lResult = (LRESULT)g_IsRunning;
                return S_OK;
            }
            return S_FALSE;

        case ID_ICON_POWER:
            Power_Update(pSysTray);

            switch (lParam)
            {
                case WM_LBUTTONDOWN:
                    break;

                case WM_LBUTTONUP:
                    ShowPowerSchemesPopupMenu(pSysTray);
                    break;

                case WM_LBUTTONDBLCLK:
                    _RunPower();
                    break;

                case WM_RBUTTONDOWN:
                    break;

                case WM_RBUTTONUP:
                    _ShowContextMenu(pSysTray);
                    break;

                case WM_RBUTTONDBLCLK:
                    break;

                case WM_MOUSEMOVE:
                    break;
            }
            return S_OK;

        default:
            TRACE("Power_Message received for unknown ID %d, ignoring.\n");
            return S_FALSE;
    }

    return S_FALSE;
}

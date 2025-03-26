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

int br_icons[5] = { IDI_BATTCAP0, IDI_BATTCAP1, IDI_BATTCAP2, IDI_BATTCAP3, IDI_BATTCAP4 }; // battery mode icons.
int bc_icons[5] = { IDI_BATTCHA0, IDI_BATTCHA1, IDI_BATTCHA2, IDI_BATTCHA3, IDI_BATTCHA4 }; // charging mode icons.

typedef struct _PWRSCHEMECONTEXT
{
    HMENU hPopup;
    UINT uiFirst;
    UINT uiLast;
} PWRSCHEMECONTEXT, *PPWRSCHEMECONTEXT;

CString  g_strTooltip;
static HICON g_hIconBattery = NULL;

#define HOUR_IN_SECS    3600
#define MIN_IN_SECS     60

/*++
* @name Quantize
*
* This function quantizes the mentioned quantity to nearest level.
*
* @param p
*        Should be a quantity in percentage.
*
* @return Nearest quantized level, can be directly used as array index based on context.
*
 @remarks This function uses centred/symmetric logic for quantization.
 For the case of lvl = 4, You will get following integer levels if given (p) value falls in between the range partitions:
     0    <= p <  12.5 : returns 0; (corresponding to 0% centre)
     12.5 <= p <  37.5 : returns 1; (corresponding to 25% centre)
     37.5 <= p <  62.5 : returns 2; (corresponding to 50% centre)
     62.5 <= p <  87.5 : returns 3; (corresponding to 75% centre)
     87.5 <= p <= 100  : returns 4; (corresponding to 100% centre)
 *--*/
static UINT Quantize(BYTE p)
{
    if (p <= 12)
        return 0;
    else if (p > 12 && p <= 37)
        return 1;
    else if (p > 37 && p <= 62)
        return 2;
    else if (p > 62 && p <= 87)
        return 3;
    else
        return 4;
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
    SYSTEM_POWER_STATUS PowerStatus;
    HICON hBatIcon;
    UINT uiHour, uiMin;
    UINT index = -1;

    if (!GetSystemPowerStatus(&PowerStatus) ||
        PowerStatus.ACLineStatus == AC_LINE_UNKNOWN ||
        PowerStatus.BatteryFlag == BATTERY_FLAG_UNKNOWN)
    {
        hBatIcon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_BATTCAP_ERR));
        g_strTooltip.LoadStringW(IDS_PWR_UNKNOWN_REMAINING);
        return hBatIcon;
    }

    if (((PowerStatus.BatteryFlag & BATTERY_FLAG_NO_BATTERY) == 0) &&
        (PowerStatus.BatteryLifePercent == BATTERY_PERCENTAGE_UNKNOWN))
    {
        hBatIcon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_BATTCAP_ERR));
        g_strTooltip.LoadStringW(IDS_PWR_UNKNOWN_REMAINING);
    }
    else if (((PowerStatus.BatteryFlag & BATTERY_FLAG_NO_BATTERY) == 0) &&
        ((PowerStatus.BatteryFlag & BATTERY_FLAG_CHARGING) == BATTERY_FLAG_CHARGING))
    {
        index = Quantize(PowerStatus.BatteryLifePercent);
        hBatIcon = LoadIcon(hinst, MAKEINTRESOURCE(bc_icons[index]));
        g_strTooltip.Format(IDS_PWR_CHARGING, PowerStatus.BatteryLifePercent);
    }
    else if (((PowerStatus.BatteryFlag & BATTERY_FLAG_NO_BATTERY) == 0) &&
             ((PowerStatus.BatteryFlag & BATTERY_FLAG_CHARGING) == 0))
    {
        index = Quantize(PowerStatus.BatteryLifePercent);
        hBatIcon = LoadIcon(hinst, MAKEINTRESOURCE(br_icons[index]));

        if (PowerStatus.BatteryLifeTime != BATTERY_UNKNOWN_TIME)
        {
            uiHour = PowerStatus.BatteryLifeTime / HOUR_IN_SECS;
            uiMin = (PowerStatus.BatteryLifeTime % HOUR_IN_SECS) / MIN_IN_SECS;

            if (uiHour != 0)
            {
                g_strTooltip.Format(IDS_PWR_HOURS_REMAINING, uiHour, uiMin, PowerStatus.BatteryLifePercent);
            }
            else
            {
                g_strTooltip.Format(IDS_PWR_MINUTES_REMAINING, uiMin, PowerStatus.BatteryLifePercent);
            }
        }
        else
        {
            g_strTooltip.Format(IDS_PWR_PERCENT_REMAINING, PowerStatus.BatteryLifePercent);
        }
    }
    else
    {
        hBatIcon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_POWER_AC));
        g_strTooltip.LoadStringW(IDS_PWR_AC);
    }

    return hBatIcon;
}

HRESULT STDMETHODCALLTYPE Power_Init(_In_ CSysTray * pSysTray)
{
    TRACE("Power_Init\n");
    g_hIconBattery = DynamicLoadIcon(g_hInstance);

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
    SetMenuDefaultItem(hPopup, IDS_PWR_PROPERTIES, FALSE);

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
            if (wParam == POWER_SERVICE_FLAG)
            {
                if (lParam)
                {
                    pSysTray->EnableService(POWER_SERVICE_FLAG, TRUE);
                    return Power_Init(pSysTray);
                }
                else
                {
                    pSysTray->EnableService(POWER_SERVICE_FLAG, FALSE);
                    return Power_Shutdown(pSysTray);
                }
            }
            return S_FALSE;

        case WM_USER + 221:
            TRACE("Power_Message: WM_USER+221\n");
            if (wParam == POWER_SERVICE_FLAG)
            {
                lResult = (LRESULT)pSysTray->IsServiceEnabled(POWER_SERVICE_FLAG);
                return S_OK;
            }
            return S_FALSE;

        case WM_TIMER:
            if (wParam == POWER_TIMER_ID)
            {
                KillTimer(pSysTray->GetHWnd(), POWER_TIMER_ID);
                ShowPowerSchemesPopupMenu(pSysTray);
            }
            break;

        case ID_ICON_POWER:
            Power_Update(pSysTray);

            switch (lParam)
            {
                case WM_LBUTTONDOWN:
                    SetTimer(pSysTray->GetHWnd(), POWER_TIMER_ID, GetDoubleClickTime(), NULL);
                    break;

                case WM_LBUTTONUP:
                    break;

                case WM_LBUTTONDBLCLK:
                    KillTimer(pSysTray->GetHWnd(), POWER_TIMER_ID);
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

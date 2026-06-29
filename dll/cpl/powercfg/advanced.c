/*
 * PROJECT:     ReactOS Power Configuration Applet
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Advanced tab
 * COPYRIGHT:   Copyright 2006 Alexander Wurzinger <lohnegrim@gmx.net>
 *              Copyright 2006 Johannes Anderwald <johannes.anderwald@reactos.org>
 *              Copyright 2006 Martin Rottensteiner <2005only@pianonote.at>
 *              Copyright 2019 Eric Kohl <eric.kohl@reactos.org>
 */

#include "powercfg.h"

static POWER_ACTION g_SystemBatteries[3];
static POWER_ACTION g_PowerButton[5];
static POWER_ACTION g_SleepButton[5];


static
VOID
SetSystrayPowerIconState(BOOL bEnabled)
{
    HWND hwndTaskBar;

    hwndTaskBar = FindWindowW(L"SystemTray_Main", NULL);
    if (hwndTaskBar == NULL)
        return;

    SendMessageW(hwndTaskBar, WM_USER + 220, 1, bEnabled);
}

static
BOOL
GetSystrayPowerIconState(VOID)
{
    HWND hwndTaskBar;

    hwndTaskBar = FindWindowW(L"SystemTray_Main", NULL);
    if (hwndTaskBar == NULL)
        return FALSE;

    return (BOOL)SendMessageW(hwndTaskBar, WM_USER + 221, 1, 0);
}

static VOID
AddItem(HWND hDlgCtrl, INT ResourceId, LPARAM lParam, POWER_ACTION * lpAction)
{
    TCHAR szBuffer[MAX_PATH];
    LRESULT Index;

    if (LoadString(hApplet, ResourceId, szBuffer, _countof(szBuffer)) < _countof(szBuffer))
    {
        Index = SendMessage(hDlgCtrl, CB_INSERTSTRING, -1, (LPARAM)szBuffer);
        if (Index != CB_ERR)
        {
            SendMessage(hDlgCtrl, CB_SETITEMDATA, (WPARAM)Index, lParam);
            lpAction[Index] = (POWER_ACTION)lParam;
        }
    }
}

static INT
FindActionIndex(POWER_ACTION * lpAction, DWORD dwActionSize, POWER_ACTION poAction)
{
    INT Index;

    for (Index = 0; Index < (INT)dwActionSize; Index++)
    {
        if (lpAction[Index] == poAction)
            return Index;
    }

    return -1;
}

static BOOLEAN
IsBatteryUsed(VOID)
{
    SYSTEM_BATTERY_STATE sbs;

    if (CallNtPowerInformation(SystemBatteryState, NULL, 0, &sbs, sizeof(sbs)) == STATUS_SUCCESS)
        return (sbs.BatteryPresent && !sbs.AcOnLine);
    return FALSE;
}

POWER_ACTION
GetPowerActionFromPolicy(
    POWER_ACTION_POLICY *Policy,
    PSYSTEM_POWER_CAPABILITIES spc,
    BOOL bIsLid)
{
    POWER_ACTION poAction = PowerActionNone;
    /*

    TCHAR szBuffer[MAX_PATH];

    // Note: Windows XP SP2+ does not return the PowerAction code
    // for PowerActionWarmEject + PowerActionShutdown but sets it
    // to PowerActionNone and sets the Flags & EventCode

     _stprintf(szBuffer, L"Action: %x EventCode %x Flags %x",Policy->Action, Policy->EventCode, Policy->Flags);
     MessageBoxW(NULL, szBuffer, NULL, MB_OK);

    */

    if (Policy->Action == PowerActionNone)
    {
        if (Policy->Flags == (POWER_ACTION_UI_ALLOWED | POWER_ACTION_QUERY_ALLOWED))
        {
            if (Policy->EventCode == POWER_FORCE_TRIGGER_RESET)
                poAction = PowerActionNone;
            else if (Policy->EventCode == POWER_USER_NOTIFY_BUTTON)
                poAction = PowerActionWarmEject;
            else if (Policy->EventCode == POWER_USER_NOTIFY_SHUTDOWN)
                poAction = PowerActionShutdown;
        }
    }
    else
    {
        poAction = Policy->Action;
        if ((poAction == PowerActionHibernate) && !IS_PWR_HIBERNATE_ALLOWED(spc))
            poAction = PowerActionSleep;
        if ((poAction == PowerActionSleep) && !IS_PWR_SUSPEND_ALLOWED(spc))
        {
            if (bIsLid)
                poAction = PowerActionNone;
            else
                poAction = PowerActionShutdown;
        }
    }

    return poAction;
}

VOID
ShowCurrentPowerActionPolicy(HWND hDlgCtrl,
                             POWER_ACTION *lpAction,
                             DWORD dwActionSize,
                             POWER_ACTION_POLICY *Policy,
                             PSYSTEM_POWER_CAPABILITIES spc,
                             BOOL bIsLid)
{
    int poActionIndex;
    POWER_ACTION poAction;

    poAction = GetPowerActionFromPolicy(Policy, spc, bIsLid);
    poActionIndex = FindActionIndex(lpAction, dwActionSize, poAction);

    if (poActionIndex < 0)
    {
        return;
    }

    SendMessage(hDlgCtrl, CB_SETCURSEL, (WPARAM)poActionIndex, (LPARAM)0);
}

BOOLEAN
SaveCurrentPowerActionPolicy(IN HWND hDlgCtrl,
                             OUT POWER_ACTION_POLICY *Policy)
{
    LRESULT Index;
    LRESULT ItemData;

    Index = SendMessage(hDlgCtrl, CB_GETCURSEL, 0, 0);
    if (Index == CB_ERR)
        return FALSE;

    ItemData = SendMessage(hDlgCtrl, CB_GETITEMDATA, (WPARAM)Index, 0);
    if (ItemData == CB_ERR)
        return FALSE;

    switch(ItemData)
    {
        case PowerActionNone:
            Policy->Action = PowerActionNone;
            Policy->EventCode = POWER_FORCE_TRIGGER_RESET;
            break;

        case PowerActionWarmEject:
            Policy->Action = PowerActionNone;
            Policy->EventCode = POWER_USER_NOTIFY_BUTTON;
            break;

        case PowerActionShutdown:
            Policy->Action = PowerActionNone;
            Policy->EventCode = POWER_USER_NOTIFY_SHUTDOWN;
            break;

        case PowerActionSleep:
        case PowerActionHibernate:
            Policy->Action = (POWER_ACTION)ItemData;
            Policy->EventCode = 0;
            break;

        default:
            return FALSE;
    }

    Policy->Flags = (POWER_ACTION_UI_ALLOWED | POWER_ACTION_QUERY_ALLOWED);

    return TRUE;
}


//-------------------------------------------------------------------

VOID
ShowCurrentPowerActionPolicies(
    HWND hwndDlg,
    PSYSTEM_POWER_CAPABILITIES spc)
{
    if (!IsBatteryUsed())
    {
        ShowCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_LIDCLOSE),
                                     g_SystemBatteries,
                                     sizeof(g_SystemBatteries) / sizeof(POWER_ACTION),
                                     &gGPP.user.LidCloseAc,
                                     spc,
                                     TRUE);

        ShowCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_POWERBUTTON),
                                     g_PowerButton,
                                     sizeof(g_PowerButton) / sizeof(POWER_ACTION),
                                     &gGPP.user.PowerButtonAc,
                                     spc,
                                     FALSE);

        ShowCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_SLEEPBUTTON),
                                     g_SleepButton,
                                     sizeof(g_SleepButton) / sizeof(POWER_ACTION),
                                     &gGPP.user.SleepButtonAc,
                                     spc,
                                     FALSE);
    }
    else
    {
        ShowCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_LIDCLOSE),
                                     g_SystemBatteries,
                                     sizeof(g_SystemBatteries) / sizeof(POWER_ACTION),
                                     &gGPP.user.LidCloseDc,
                                     spc,
                                     TRUE);

        ShowCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_POWERBUTTON),
                                     g_PowerButton,
                                     sizeof(g_PowerButton) / sizeof(POWER_ACTION),
                                     &gGPP.user.PowerButtonDc,
                                     spc,
                                     FALSE);

        ShowCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_SLEEPBUTTON),
                                     g_SleepButton,
                                     sizeof(g_SleepButton) / sizeof(POWER_ACTION),
                                     &gGPP.user.SleepButtonDc,
                                     spc,
                                     FALSE);
    }
}

VOID
Adv_InitDialog(
    HWND hwndDlg)
{
    HWND hList1, hList2, hList3;
    BOOLEAN bSuspend, bHibernate, bShutdown;
    BOOL bEnabled;
    SYSTEM_POWER_CAPABILITIES spc;

    bEnabled = GetSystrayPowerIconState();

    if (bEnabled)
        gGPP.user.GlobalFlags |= EnableSysTrayBatteryMeter;
    else
        gGPP.user.GlobalFlags &= ~EnableSysTrayBatteryMeter;

    CheckDlgButton(hwndDlg,
        IDC_SYSTRAYBATTERYMETER,
        bEnabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg,
        IDC_PASSWORDLOGON,
        gGPP.user.GlobalFlags & EnablePasswordLogon ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg,
        IDC_VIDEODIMDISPLAY,
        gGPP.user.GlobalFlags & EnableVideoDimDisplay ? BST_CHECKED : BST_UNCHECKED);

    GetPwrCapabilities(&spc);

    bSuspend = IS_PWR_SUSPEND_ALLOWED(&spc);
    bHibernate = IS_PWR_HIBERNATE_ALLOWED(&spc);
    bShutdown = IS_PWR_POWEROFF_ALLOWED(&spc);

    hList1 = GetDlgItem(hwndDlg, IDC_LIDCLOSE);
    SendMessage(hList1, CB_RESETCONTENT, 0, 0);

    ZeroMemory(g_SystemBatteries, sizeof(g_SystemBatteries));
    if (spc.SystemBatteriesPresent)
    {
        AddItem(hList1, IDS_PowerActionNone1, (LPARAM)PowerActionNone, g_SystemBatteries);

        if (bSuspend)
            AddItem(hList1, IDS_PowerActionSleep, (LPARAM)PowerActionSleep, g_SystemBatteries);

        if (bHibernate)
            AddItem(hList1, IDS_PowerActionHibernate, (LPARAM)PowerActionHibernate, g_SystemBatteries);
    }
    else
    {
        ShowWindow(GetDlgItem(hwndDlg, IDC_VIDEODIMDISPLAY), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_SLIDCLOSE), SW_HIDE);
        ShowWindow(hList1, SW_HIDE);
    }

    hList2 = GetDlgItem(hwndDlg, IDC_POWERBUTTON);
    SendMessage(hList2, CB_RESETCONTENT, 0, 0);

    ZeroMemory(g_PowerButton, sizeof(g_PowerButton));
    if (spc.PowerButtonPresent)
    {
        AddItem(hList2, IDS_PowerActionNone1, (LPARAM)PowerActionNone, g_PowerButton);
        AddItem(hList2, IDS_PowerActionWarmEject, (LPARAM)PowerActionWarmEject, g_PowerButton);

        if (bSuspend)
            AddItem(hList2, IDS_PowerActionSleep, (LPARAM)PowerActionSleep, g_PowerButton);

        if (bHibernate)
            AddItem(hList2, IDS_PowerActionHibernate, (LPARAM)PowerActionHibernate, g_PowerButton);

        if (bShutdown)
            AddItem(hList2, IDS_PowerActionShutdown, (LPARAM)PowerActionShutdown, g_PowerButton);
    }
    else
    {
        ShowWindow(GetDlgItem(hwndDlg, IDC_SPOWERBUTTON), SW_HIDE);
        ShowWindow(hList2, SW_HIDE);
    }

    hList3 = GetDlgItem(hwndDlg, IDC_SLEEPBUTTON);
    SendMessage(hList3, CB_RESETCONTENT, 0, 0);

    ZeroMemory(g_SleepButton, sizeof(g_SleepButton));
    if (spc.SleepButtonPresent)
    {
        AddItem(hList3, IDS_PowerActionNone1, (LPARAM)PowerActionNone, g_SleepButton);
        AddItem(hList3, IDS_PowerActionWarmEject, (LPARAM)PowerActionWarmEject, g_SleepButton);

        if (bSuspend)
            AddItem(hList3, IDS_PowerActionSleep, (LPARAM)PowerActionSleep, g_SleepButton);

        if (bHibernate)
            AddItem(hList3, IDS_PowerActionHibernate, (LPARAM)PowerActionHibernate, g_SleepButton);

        if (bShutdown)
            AddItem(hList3, IDS_PowerActionShutdown, (LPARAM)PowerActionShutdown, g_SleepButton);
    }
    else
    {
        ShowWindow(GetDlgItem(hwndDlg, IDC_SSLEEPBUTTON), SW_HIDE);
        ShowWindow(hList3, SW_HIDE);
    }

    if (ReadGlobalPwrPolicy(&gGPP))
        ShowCurrentPowerActionPolicies(hwndDlg, &spc);
}

static VOID
Adv_SaveData(HWND hwndDlg)
{
    BOOL bSystrayBatteryMeter;
    BOOL bPasswordLogon;
    BOOL bVideoDimDisplay;

    bSystrayBatteryMeter =
        (IsDlgButtonChecked(hwndDlg, IDC_SYSTRAYBATTERYMETER) == BST_CHECKED);

    bPasswordLogon =
        (IsDlgButtonChecked(hwndDlg, IDC_PASSWORDLOGON) == BST_CHECKED);

    bVideoDimDisplay =
        (IsDlgButtonChecked(hwndDlg, IDC_VIDEODIMDISPLAY) == BST_CHECKED);

    if (bSystrayBatteryMeter)
    {
        if (!(gGPP.user.GlobalFlags & EnableSysTrayBatteryMeter))
        {
            gGPP.user.GlobalFlags = gGPP.user.GlobalFlags + EnableSysTrayBatteryMeter;
        }
    }
    else
    {
        if ((gGPP.user.GlobalFlags & EnableSysTrayBatteryMeter))
        {
            gGPP.user.GlobalFlags = gGPP.user.GlobalFlags - EnableSysTrayBatteryMeter;
        }
    }

    if (bPasswordLogon)
    {
        if (!(gGPP.user.GlobalFlags & EnablePasswordLogon))
        {
            gGPP.user.GlobalFlags = gGPP.user.GlobalFlags + EnablePasswordLogon;
        }
    }
    else
    {
        if ((gGPP.user.GlobalFlags & EnablePasswordLogon))
        {
            gGPP.user.GlobalFlags = gGPP.user.GlobalFlags - EnablePasswordLogon;
        }
    }

    if (bVideoDimDisplay)
    {
        if (!(gGPP.user.GlobalFlags & EnableVideoDimDisplay))
        {
            gGPP.user.GlobalFlags = gGPP.user.GlobalFlags + EnableVideoDimDisplay;
        }
    }
    else
    {
        if ((gGPP.user.GlobalFlags & EnableVideoDimDisplay))
        {
            gGPP.user.GlobalFlags = gGPP.user.GlobalFlags - EnableVideoDimDisplay;
        }
    }

    if (!IsBatteryUsed())
    {
#if 0
        SaveCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_LIDCLOSE), &gGPP.user.LidCloseAc);
#endif
        SaveCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_POWERBUTTON), &gGPP.user.PowerButtonAc);
        SaveCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_SLEEPBUTTON), &gGPP.user.SleepButtonAc);
    }
    else
    {
#if 0
        SaveCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_LIDCLOSE), &gGPP.user.LidCloseDc);
        SaveCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_POWERBUTTON), &gGPP.user.PowerButtonDc);
        SaveCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_SLEEPBUTTON), &gGPP.user.SleepButtonDc);
#endif
    }

    if (!WriteGlobalPwrPolicy(&gGPP))
    {
        MessageBox(hwndDlg, L"WriteGlobalPwrPolicy failed", NULL, MB_OK);
    }

    SetSystrayPowerIconState(bSystrayBatteryMeter);

//    Adv_InitDialog(hwndDlg);
}

/* Property page dialog callback */
INT_PTR CALLBACK
AdvancedDlgProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            Adv_InitDialog(hwndDlg);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_SYSTRAYBATTERYMETER:
                case IDC_PASSWORDLOGON:
                case IDC_VIDEODIMDISPLAY:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_LIDCLOSE:
                case IDC_POWERBUTTON:
                case IDC_SLEEPBUTTON:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_APPLY:
                    Adv_SaveData(hwndDlg);
                    return TRUE;

                case PSN_SETACTIVE:
                    Adv_InitDialog(hwndDlg);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}

/*
 * PROJECT:     ReactOS Power Configuration Applet
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Alarms tab
 * COPYRIGHT:   Copyright 2006 Alexander Wurzinger <lohnegrim@gmx.net>
 *              Copyright 2006 Johannes Anderwald <johannes.anderwald@reactos.org>
 *              Copyright 2006 Martin Rottensteiner <2005only@pianonote.at>
 */

#include "powercfg.h"

static BOOLEAN
Ala_InitData(HWND hwndDlg)
{
    TCHAR szAction[MAX_PATH];
    TCHAR szText[MAX_PATH];
    TCHAR szSound[MAX_PATH];
    TCHAR szMessage[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    TCHAR szBatteryLevel[MAX_PATH];
    TCHAR szProgram[MAX_PATH];

    if (!ReadGlobalPwrPolicy(&gGPP))
        return FALSE;

    if (gGPP.user.DischargePolicy[DISCHARGE_POLICY_LOW].Enable)
    {
        CheckDlgButton(hwndDlg, IDC_ALARM1,
            gGPP.user.DischargePolicy[DISCHARGE_POLICY_LOW].Enable ? BST_CHECKED : BST_UNCHECKED);

        if (LoadString(hApplet, IDS_PERCENT, szTemp, _countof(szTemp)))
        {
            _stprintf(szBatteryLevel, szTemp, gGPP.user.DischargePolicy[DISCHARGE_POLICY_LOW].BatteryLevel);
            SetDlgItemText(hwndDlg, IDC_ALARMVALUE1, szBatteryLevel);
        }

        SendDlgItemMessage(hwndDlg, IDC_ALARMBAR1,
                           TBM_SETRANGE,
                           (WPARAM)TRUE,
                           (LPARAM)MAKELONG(0, 100));
        SendDlgItemMessage(hwndDlg, IDC_ALARMBAR1,
                           TBM_SETTICFREQ,
                           (WPARAM)TRUE,
                           (LPARAM)20);
        SendDlgItemMessage(hwndDlg, IDC_ALARMBAR1,
                           TBM_SETPOS,
                           (WPARAM)TRUE,
                           (LPARAM)gGPP.user.DischargePolicy[DISCHARGE_POLICY_LOW].BatteryLevel);

        if (LoadString(hApplet, gGPP.user.DischargePolicy[DISCHARGE_POLICY_LOW].PowerPolicy.Action+IDS_PowerActionNone1, szAction, _countof(szAction)))
        {
            SetDlgItemText(hwndDlg, IDC_ALARMAKTION1, szAction);
        }

        ZeroMemory(szMessage, sizeof(szMessage));
        LoadString(hApplet, IDS_NOACTION, szMessage, _countof(szMessage));

        if (LOWORD(gGPP.user.DischargePolicy[DISCHARGE_POLICY_LOW].PowerPolicy.EventCode) & POWER_LEVEL_USER_NOTIFY_TEXT)
        {
            if (LOWORD(gGPP.user.DischargePolicy[DISCHARGE_POLICY_LOW].PowerPolicy.EventCode) & POWER_LEVEL_USER_NOTIFY_SOUND)
            {
                if (LoadString(hApplet, IDS_SOUND, szSound, _countof(szSound)) && LoadString(hApplet, IDS_TEXT, szText, _countof(szText)))
                    _stprintf(szMessage, _T("%s, %s"), szSound, szText);
            }
            else
            {
                if (LoadString(hApplet, IDS_TEXT, szText, _countof(szText)))
                    _stprintf(szMessage, _T("%s"), szText);
            }
        }
        else
        {
            if (LOWORD(gGPP.user.DischargePolicy[DISCHARGE_POLICY_LOW].PowerPolicy.EventCode) & POWER_LEVEL_USER_NOTIFY_SOUND)
            {
                if (LoadString(hApplet, IDS_SOUND, szSound, _countof(szSound)))
                    _stprintf(szMessage, _T("%s"), szSound);
            }
        }

        SetDlgItemText(hwndDlg, IDC_ALARMMSG1, szMessage);

        if (LoadString(hApplet, IDS_PowerActionNone2, szProgram, _countof(szProgram)))
        {
            SetDlgItemText(hwndDlg, IDC_ALARMPROG1, szProgram);
        }
    }

    if (gGPP.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].Enable)
    {
        CheckDlgButton(hwndDlg, IDC_ALARM2,
            gGPP.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].Enable ? BST_CHECKED : BST_UNCHECKED);

        if (LoadString(hApplet, IDS_PERCENT, szTemp, _countof(szTemp)))
        {
            _stprintf(szBatteryLevel, szTemp, gGPP.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].BatteryLevel);
            SetDlgItemText(hwndDlg, IDC_ALARMVALUE2, szBatteryLevel);
        }

        SendDlgItemMessage(hwndDlg, IDC_ALARMBAR2,
                           TBM_SETRANGE,
                           (WPARAM)TRUE,
                           (LPARAM)MAKELONG(0, 100));
        SendDlgItemMessage(hwndDlg, IDC_ALARMBAR2,
                           TBM_SETPOS,
                           (WPARAM)TRUE,
                           (LPARAM)gGPP.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].BatteryLevel);

        if (LoadString(hApplet, gGPP.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].PowerPolicy.Action+IDS_PowerActionNone1, szAction, _countof(szAction)))
        {
            SetDlgItemText(hwndDlg, IDC_ALARMAKTION2, szAction);
        }

        ZeroMemory(szMessage, sizeof(szMessage));
        LoadString(hApplet, IDS_NOACTION, szMessage, _countof(szMessage));

        if (LOWORD(gGPP.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].PowerPolicy.EventCode) & POWER_LEVEL_USER_NOTIFY_TEXT)
        {
            if (LOWORD(gGPP.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].PowerPolicy.EventCode) & POWER_LEVEL_USER_NOTIFY_SOUND)
            {
                if (LoadString(hApplet, IDS_TEXT, szText, _countof(szText)) && LoadString(hApplet, IDS_SOUND, szSound, _countof(szSound)))
                    _stprintf(szMessage, _T("%s, %s"), szSound, szText);
            }
            else
            {
                if (LoadString(hApplet, IDS_TEXT, szText, _countof(szText)))
                    _stprintf(szMessage, _T("%s"), szText);
            }
        }
        else
        {
            if (LOWORD(gGPP.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].PowerPolicy.EventCode) & POWER_LEVEL_USER_NOTIFY_SOUND)
            {
                if (LoadString(hApplet, IDS_SOUND, szSound, _countof(szSound)))
                    _stprintf(szMessage, _T("%s"), szSound);
            }
        }

        SetDlgItemText(hwndDlg, IDC_ALARMMSG2, szMessage);

        if (LoadString(hApplet, IDS_PowerActionNone2, szProgram, _countof(szProgram)))
        {
            SetDlgItemText(hwndDlg, IDC_ALARMPROG2, szProgram);
        }
    }

    return TRUE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
AlarmsDlgProc(HWND hwndDlg,
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            if (!Ala_InitData(hwndDlg))
            {
                // TODO:
                // Handle initialization error
            }
            return TRUE;

        default:
            break;
    }

    return FALSE;
}

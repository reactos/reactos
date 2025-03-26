/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/timedate/dateandtime.c
 * PURPOSE:     Date & Time property page
 * COPYRIGHT:   Copyright 2004-2007 Eric Kohl
 *              Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2006 Thomas Weidenmueller <w3seek@reactos.com>
 *
 */

#include "timedate.h"

static WNDPROC pOldWndProc = NULL;

BOOL
SystemSetLocalTime(LPSYSTEMTIME lpSystemTime)
{
    HANDLE hToken;
    DWORD PrevSize;
    TOKEN_PRIVILEGES priv, previouspriv;
    BOOL Ret = FALSE;

    /*
     * Enable the SeSystemtimePrivilege privilege
     */

    if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                         &hToken))
    {
        priv.PrivilegeCount = 1;
        priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (LookupPrivilegeValueW(NULL,
                                  SE_SYSTEMTIME_NAME,
                                  &priv.Privileges[0].Luid))
        {
            if (AdjustTokenPrivileges(hToken,
                                      FALSE,
                                      &priv,
                                      sizeof(previouspriv),
                                      &previouspriv,
                                      &PrevSize) &&
                GetLastError() == ERROR_SUCCESS)
            {
                /*
                 * We successfully enabled it, we're permitted to change the time.
                 * Call SetLocalTime twice to ensure correct results.
                 */
                Ret = SetLocalTime(lpSystemTime) &&
                      SetLocalTime(lpSystemTime);

                /*
                 * For the sake of security, restore the previous status again
                 */
                if (previouspriv.PrivilegeCount > 0)
                {
                    AdjustTokenPrivileges(hToken,
                                          FALSE,
                                          &previouspriv,
                                          0,
                                          NULL,
                                          0);
                }
            }
        }
        CloseHandle(hToken);
    }

    return Ret;
}


static VOID
SetLocalSystemTime(HWND hwnd)
{
    SYSTEMTIME Time;

    if (DateTime_GetSystemtime(GetDlgItem(hwnd,
                                          IDC_TIMEPICKER),
                               &Time) == GDT_VALID &&
        SendMessageW(GetDlgItem(hwnd,
                                IDC_MONTHCALENDAR),
                     MCCM_GETDATE,
                     (WPARAM)&Time,
                     0))
    {
        SystemSetLocalTime(&Time);

        SetWindowLongPtrW(hwnd,
                          DWLP_MSGRESULT,
                          PSNRET_NOERROR);

        SendMessageW(GetDlgItem(hwnd,
                                IDC_MONTHCALENDAR),
                     MCCM_RESET,
                     (WPARAM)&Time,
                     0);

        /* Broadcast the time change message */
        SendMessageW(HWND_BROADCAST,
                     WM_TIMECHANGE,
                     0,
                     0);
    }
}


static VOID
SetTimeZoneName(HWND hwnd)
{
    TIME_ZONE_INFORMATION TimeZoneInfo;
    WCHAR TimeZoneString[128];
    WCHAR TimeZoneText[128];
    WCHAR TimeZoneName[128];
    DWORD TimeZoneId;

    TimeZoneId = GetTimeZoneInformation(&TimeZoneInfo);

    LoadStringW(hApplet, IDS_TIMEZONETEXT, TimeZoneText, 128);

    switch (TimeZoneId)
    {
        case TIME_ZONE_ID_STANDARD:
        case TIME_ZONE_ID_UNKNOWN:
            wcscpy(TimeZoneName, TimeZoneInfo.StandardName);
            break;

        case TIME_ZONE_ID_DAYLIGHT:
            wcscpy(TimeZoneName, TimeZoneInfo.DaylightName);
            break;

        case TIME_ZONE_ID_INVALID:
        default:
            LoadStringW(hApplet, IDS_TIMEZONEINVALID, TimeZoneName, 128);
            break;
    }

    wsprintfW(TimeZoneString, TimeZoneText, TimeZoneName);
    SendDlgItemMessageW(hwnd, IDC_TIMEZONE, WM_SETTEXT, 0, (LPARAM)TimeZoneString);
}


static VOID
FillMonthsComboBox(HWND hCombo)
{
    SYSTEMTIME LocalDate = {0};
    WCHAR szBuf[64];
    INT i;
    UINT Month;

    GetLocalTime(&LocalDate);

    SendMessageW(hCombo,
                 CB_RESETCONTENT,
                 0,
                 0);

    for (Month = 1;
         Month <= 13;
         Month++)
    {
        i = GetLocaleInfoW(LOCALE_USER_DEFAULT,
                           ((Month < 13) ? LOCALE_SMONTHNAME1 + Month - 1 : LOCALE_SMONTHNAME13),
                           szBuf,
                           sizeof(szBuf) / sizeof(szBuf[0]));
        if (i > 1)
        {
            i = (INT)SendMessageW(hCombo,
                                  CB_ADDSTRING,
                                  0,
                                  (LPARAM)szBuf);
            if (i != CB_ERR)
            {
                SendMessageW(hCombo,
                             CB_SETITEMDATA,
                             (WPARAM)i,
                             Month);

                if (Month == (UINT)LocalDate.wMonth)
                {
                    SendMessageW(hCombo,
                                 CB_SETCURSEL,
                                 (WPARAM)i,
                                 0);
                }
            }
        }
    }
}


static WORD
GetCBSelectedMonth(HWND hCombo)
{
    INT i;
    WORD Ret = (WORD)-1;

    i = (INT)SendMessageW(hCombo,
                          CB_GETCURSEL,
                          0,
                          0);
    if (i != CB_ERR)
    {
        i = (INT)SendMessageW(hCombo,
                              CB_GETITEMDATA,
                              (WPARAM)i,
                              0);

        if (i >= 1 && i <= 13)
            Ret = (WORD)i;
    }

    return Ret;
}


static VOID
ChangeMonthCalDate(HWND hMonthCal,
                   WORD Day,
                   WORD Month,
                   WORD Year)
{
    SendMessageW(hMonthCal,
                 MCCM_SETDATE,
                 MAKEWPARAM(Day,
                            Month),
                 MAKELPARAM(Year,
                            0));
}

static VOID
AutoUpdateMonthCal(HWND hwndDlg,
                   PNMMCCAUTOUPDATE lpAutoUpdate)
{
    UNREFERENCED_PARAMETER(lpAutoUpdate);

    /* Update the controls */
    FillMonthsComboBox(GetDlgItem(hwndDlg,
                                  IDC_MONTHCB));
}


static INT_PTR CALLBACK
DTPProc(HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:
            /* Stop the timer when the user is about to change the time */
            if ((wParam != VK_LEFT) & (wParam != VK_RIGHT))
                KillTimer(GetParent(hwnd), ID_TIMER);
            break;
    }

    return CallWindowProcW(pOldWndProc, hwnd, uMsg, wParam, lParam);
}

/* Property page dialog callback */
INT_PTR CALLBACK
DateTimePageProc(HWND hwndDlg,
         UINT uMsg,
         WPARAM wParam,
         LPARAM lParam)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            FillMonthsComboBox(GetDlgItem(hwndDlg,
                                          IDC_MONTHCB));

            /* Set range and current year */
            SendMessageW(GetDlgItem(hwndDlg, IDC_YEAR), UDM_SETRANGE, 0, MAKELONG ((short) 9999, (short) 1900));
            SendMessageW(GetDlgItem(hwndDlg, IDC_YEAR), UDM_SETPOS, 0, MAKELONG( (short) st.wYear, 0));

            pOldWndProc = (WNDPROC)SetWindowLongPtrW(GetDlgItem(hwndDlg, IDC_TIMEPICKER), GWLP_WNDPROC, (LONG_PTR)DTPProc);

            SetTimer(hwndDlg, ID_TIMER, 1000 - st.wMilliseconds, NULL);
            break;

        case WM_TIMER:
            SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEPICKER), DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &st);

            // Reset timeout.
            SetTimer(hwndDlg, ID_TIMER, 1000 - st.wMilliseconds, NULL);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_MONTHCB:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        ChangeMonthCalDate(GetDlgItem(hwndDlg,
                                                      IDC_MONTHCALENDAR),
                                                      (WORD) -1,
                                                      GetCBSelectedMonth((HWND)lParam),
                                                      (WORD) -1);
                    }
                    break;
            }
            break;

        case WM_CTLCOLORSTATIC:
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_YEARTEXT))
                return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->idFrom)
            {
                case IDC_YEAR:
                    switch (lpnm->code)
                    {
                        case UDN_DELTAPOS:
                        {
                            SHORT wYear;
                            LPNMUPDOWN updown = (LPNMUPDOWN)lpnm;
                            wYear = (SHORT)SendMessageW(GetDlgItem(hwndDlg, IDC_YEAR), UDM_GETPOS, 0, 0);
                            /* Enable the 'Apply' button */
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            ChangeMonthCalDate(GetDlgItem(hwndDlg,
                                               IDC_MONTHCALENDAR),
                                               (WORD) -1,
                                               (WORD) -1,
                                               (WORD) (wYear + updown->iDelta));
                        }
                        break;
                    }
                    break;

                case IDC_TIMEPICKER:
                    switch (lpnm->code)
                    {
                        case DTN_DATETIMECHANGE:
                            /* Stop the timer */
                            KillTimer(hwndDlg, ID_TIMER);

                            /* Tell the clock to stop ticking */
                            SendDlgItemMessageW(hwndDlg, IDC_CLOCKWND, CLM_STOPCLOCK,
                                                0, 0);

                            // TODO: Set the clock to the input time.

                            /* Enable the 'Apply' button */
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            break;
                    }
                    break;

                case IDC_MONTHCALENDAR:
                    switch (lpnm->code)
                    {
                        case MCCN_SELCHANGE:
                            /* Enable the 'Apply' button */
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            break;

                        case MCCN_AUTOUPDATE:
                            AutoUpdateMonthCal(hwndDlg,
                                                 (PNMMCCAUTOUPDATE)lpnm);
                            break;
                    }
                    break;

                default:
                    switch (lpnm->code)
                    {
                        case PSN_SETACTIVE:
                            SetTimeZoneName(hwndDlg);
                            break;

                        case PSN_APPLY:
                            SetLocalSystemTime(hwndDlg);

                            /* Tell the clock to start ticking */
                            SendDlgItemMessageW(hwndDlg, IDC_CLOCKWND, CLM_STARTCLOCK,
                                                0, 0);

                            SetTimer(hwndDlg, ID_TIMER, 1000 - st.wMilliseconds, NULL);
                            return TRUE;
                    }
                    break;
            }
        }
        break;

        case WM_TIMECHANGE:
            /* FIXME: We don't get this message as we're not a top-level window... */
            SendMessageW(GetDlgItem(hwndDlg,
                                    IDC_MONTHCALENDAR),
                         MCCM_RESET,
                         0,
                         0);
            break;

        case WM_DESTROY:
            KillTimer(hwndDlg, ID_TIMER);
            break;
    }

    return FALSE;
}

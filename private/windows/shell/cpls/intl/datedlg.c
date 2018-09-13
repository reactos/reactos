/*++

Copyright (c) 1994-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    datedlg.c

Abstract:

    This module implements the date property sheet for the Regional
    Settings applet.

Revision History:

--*/



//
//  Include Files.
//

#include "intl.h"
#include <windowsx.h>
#include <commctrl.h>
#include "intlhlp.h"
#include "maxvals.h"
#include "winnlsp.h"



//
//  Context Help Ids.
//

static int aDateHelpIds[] =
{
    IDC_GROUPBOX1,             IDH_COMM_GROUPBOX,
    IDC_GROUPBOX2,             IDH_COMM_GROUPBOX,
    IDC_GROUPBOX3,             IDH_COMM_GROUPBOX,
    IDC_SAMPLE1,               IDH_INTL_DATE_SHORTSAMPLE,
    IDC_SAMPLELBL1,            IDH_INTL_DATE_SHORTSAMPLE,
    IDC_SAMPLE1A,              IDH_INTL_DATE_SHORTSAMPLE_ARABIC,
    IDC_SAMPLELBL1A,           IDH_INTL_DATE_SHORTSAMPLE_ARABIC,
    IDC_SHORT_DATE_STYLE,      IDH_INTL_DATE_SHORTSTYLE,
    IDC_SEPARATOR,             IDH_INTL_DATE_SEPARATOR,
    IDC_SAMPLE2,               IDH_INTL_DATE_LONGSAMPLE,
    IDC_SAMPLELBL2,            IDH_INTL_DATE_LONGSAMPLE,
    IDC_SAMPLE2A,              IDH_INTL_DATE_LONGSAMPLE_ARABIC,
    IDC_SAMPLELBL2A,           IDH_INTL_DATE_LONGSAMPLE_ARABIC,
    IDC_LONG_DATE_STYLE,       IDH_INTL_DATE_LONGSTYLE,
    IDC_CALENDAR_TYPE_TEXT,    IDH_INTL_DATE_CALENDARTYPE,
    IDC_CALENDAR_TYPE,         IDH_INTL_DATE_CALENDARTYPE,
    IDC_TWO_DIGIT_YEAR_LOW,    IDH_INTL_DATE_TWO_DIGIT_YEAR,
    IDC_TWO_DIGIT_YEAR_HIGH,   IDH_INTL_DATE_TWO_DIGIT_YEAR,
    IDC_TWO_DIGIT_YEAR_ARROW,  IDH_INTL_DATE_TWO_DIGIT_YEAR,
    IDC_ADD_HIJRI_DATE,        IDH_INTL_DATE_ADD_HIJRI_DATE,
    IDC_ADD_HIJRI_DATE_TEXT,   IDH_INTL_DATE_ADD_HIJRI_DATE,

    0, 0
};




//
//  Global Variables.
//

TCHAR szNLS_LongDate[SIZE_128];
TCHAR szNLS_ShortDate[SIZE_128];

static const TCHAR c_szInternational[] = TEXT("Control Panel\\International");
static const TCHAR c_szAddHijriDate[]  = TEXT("AddHijriDate");
static const TCHAR c_szAddHijriDateTemp[] = TEXT("AddHijriDateTemp");
static const PTSTR c_szAddHijriDateValues[] =
{
  TEXT("AddHijriDate-2"),
  TEXT("AddHijriDate"),
  TEXT(""),
  TEXT("AddHijriDate+1"),
  TEXT("AddHijriDate+2")
};

static const TCHAR c_szTwoDigitYearKey[] = TEXT("Software\\Policies\\Microsoft\\Control Panel\\International\\Calendars\\TwoDigitYearMax");


void Date_InitializeHijriDateComboBox(
    HWND hDlg);



////////////////////////////////////////////////////////////////////////////
//
//  Date_EnumerateDates
//
//  Enumerates the appropriate dates for the chosen calendar.
//
////////////////////////////////////////////////////////////////////////////

void Date_EnumerateDates(
    HWND hDlg,
    DWORD dwDateFlag)
{
    DWORD dwLocaleFlag;
    int nItemId;
    DWORD dwIndex;
    DWORD dwCalNum = 0;
    TCHAR szBuf[SIZE_128];
    HWND hCtrlDate;
    HWND hCtrlCal = GetDlgItem(hDlg, IDC_CALENDAR_TYPE);


    //
    //  Initialize variables according to the dwDateFlag parameter.
    //
    if (dwDateFlag == CAL_SSHORTDATE)
    {
        dwLocaleFlag = LOCALE_SSHORTDATE;
        nItemId = IDC_SHORT_DATE_STYLE;
    }
    else           // CAL_SLONGDATE
    {
        dwLocaleFlag = LOCALE_SLONGDATE;
        nItemId = IDC_LONG_DATE_STYLE;
    }
    hCtrlDate = GetDlgItem(hDlg, nItemId);

    //
    //  Initialize to reset the contents for the appropriate combo box.
    //
    if (!Set_List_Values(hDlg, nItemId, 0))
    {
        return;
    }

    //
    //  Reset the contents of the combo box.
    //
    ComboBox_ResetContent(hCtrlDate);

    //
    //  Get the currently selected calendar id.
    //
    dwIndex = ComboBox_GetCurSel(hCtrlCal);
    if (dwIndex != CB_ERR)
    {
        dwCalNum = (DWORD)ComboBox_GetItemData(hCtrlCal, dwIndex);
    }

    //
    //  Enumerate the dates for the currently selected calendar.
    //
    EnumCalendarInfo(EnumProc, UserLocaleID, dwCalNum, dwDateFlag);
    dwIndex = ComboBox_GetCount(hCtrlCal);
    if ((dwIndex == 0) || (dwIndex == CB_ERR))
    {
        EnumCalendarInfo(EnumProc, UserLocaleID, CAL_GREGORIAN, dwDateFlag);
    }

    //
    //  Add (if necesary) and select the current user setting in the
    //  combo box.
    //
    dwIndex = 0;
    if (GetLocaleInfo(UserLocaleID, dwLocaleFlag, szBuf, SIZE_128))
    {
        if ((dwIndex = ComboBox_FindStringExact(hCtrlDate, -1, szBuf)) == CB_ERR)
        {
            //
            //  Need to add this entry to the combo box.
            //
            Set_List_Values(0, 0, szBuf);
            if ((dwIndex = ComboBox_FindStringExact(hCtrlDate, -1, szBuf)) == CB_ERR)
            {
                dwIndex = 0;
            }
        }
    }
    else
    {
        MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
    }
    Set_List_Values(0, nItemId, 0);

    Localize_Combobox_Styles(hDlg, nItemId, dwLocaleFlag);
    ComboBox_SetCurSel(hCtrlDate, dwIndex);
}


////////////////////////////////////////////////////////////////////////////
//
//  Date_GetTwoDigitYearRangeFromPolicy
//
//  Read the two digit year from the Policy registry.
//
////////////////////////////////////////////////////////////////////////////

BOOL Date_GetTwoDigitYearRangeFromPolicy(
    CALID CalId)
{
    HKEY hKey;
    BYTE buf[MAX_PATH];
    TCHAR szCalId[MAX_PATH];
    DWORD dwResultLen = sizeof(buf), dwType;
    BOOL bRet = FALSE;


    //
    //  Convert CalendarId to a string.
    //
    wsprintf(szCalId, TEXT("%d"), CalId);

    if (RegOpenKey( HKEY_CURRENT_USER,
                    c_szTwoDigitYearKey,
                    &hKey ) == ERROR_SUCCESS)
    {
        if ((RegQueryValueEx( hKey,
                              szCalId,
                              NULL,
                              &dwType,
                              &buf[0],
                              &dwResultLen ) == ERROR_SUCCESS) &&
            (dwType == REG_SZ) &&
            (dwResultLen > 2))
        {
            bRet = TRUE;
        }

        RegCloseKey(hKey);
    }

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Date_GetTwoDigitYearRange
//
//  Fills in the two digit year range controls.
//
////////////////////////////////////////////////////////////////////////////

void Date_GetTwoDigitYearRange(
    HWND hDlg,
    CALID CalId)
{
    HWND hwndYearHigh = GetDlgItem(hDlg, IDC_TWO_DIGIT_YEAR_HIGH);
    HWND hwndScroll = GetDlgItem(hDlg, IDC_TWO_DIGIT_YEAR_ARROW);
    DWORD YearHigh, YearHighDefault;

    //
    //  Enable the high range control.
    //
    EnableWindow(hwndYearHigh, TRUE);
    EnableWindow(hwndScroll, TRUE);

    //
    //  Get the default two digit year upper boundary.
    //
    if (!GetCalendarInfo( LOCALE_USER_DEFAULT,
                          CalId,
                          CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER |
                            CAL_NOUSEROVERRIDE,
                          NULL,
                          0,
                          &YearHighDefault ))
    {
        YearHighDefault = 0;
    }

    //
    //  Disable the two digit year upper boundary control if it is
    //  enforced by a policy or if the default value is 99 or less.
    //
    if ((Date_GetTwoDigitYearRangeFromPolicy(CalId)) ||
        (YearHighDefault <= 99))
    {
        //
        //  Disable the two digit year max controls.
        //
        EnableWindow(hwndScroll, FALSE);
        EnableWindow(hwndYearHigh, FALSE);
    }

    //
    //  Get the two digit year upper boundary.  If the default is less
    //  than or equal to 99, then use the default value and ignore the
    //  registry.  This is done for calendars like the Japanese Era
    //  calendar where it doesn't make sense to have a sliding window.
    //
    if (YearHighDefault <= 99)
    {
        YearHigh = YearHighDefault;
    }
    else if (!GetCalendarInfo( LOCALE_USER_DEFAULT,
                               CalId,
                               CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                               NULL,
                               0,
                               &YearHigh ) ||
             (YearHigh < 99) || (YearHigh > 9999))
    {
        YearHigh = (YearHighDefault >= 99) ? YearHighDefault : 2029;
    }

    //
    //  Set the range on the controls.
    //
    SendMessage(hwndScroll, UDM_SETRANGE, 0, MAKELPARAM(9999, 99));
    SendMessage(hwndScroll, UDM_SETBUDDY, (WPARAM)hwndYearHigh, 0L);

    //
    //  Set the values of the controls.
    //
    SetDlgItemInt(hDlg, IDC_TWO_DIGIT_YEAR_LOW, (UINT)(YearHigh - 99), FALSE);
    SendMessage(hwndScroll, UDM_SETPOS, 0, MAKELONG((short)YearHigh, 0));
}


////////////////////////////////////////////////////////////////////////////
//
//  Date_SetTwoDigitYearMax
//
//  Sets the two digit year max value in the registry.
//
////////////////////////////////////////////////////////////////////////////

BOOL Date_SetTwoDigitYearMax(
    HWND hDlg,
    CALID CalId)
{
    TCHAR szYear[SIZE_64];

    //
    //  Get the max year.
    //
    szYear[0] = 0;
    if (GetWindowText( GetDlgItem(hDlg, IDC_TWO_DIGIT_YEAR_HIGH),
                       szYear,
                       SIZE_64 ) != 0)
    {
        //
        //  Set the two digit year upper boundary.
        //
        return (SetCalendarInfo( LOCALE_USER_DEFAULT,
                                 CalId,
                                 CAL_ITWODIGITYEARMAX,
                                 szYear ));
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Date_ChangeYear
//
//  Changes the lower bound based on the upper bound value.
//
////////////////////////////////////////////////////////////////////////////

void Date_ChangeYear(
    HWND hDlg)
{
    DWORD YearHigh;
    BOOL bSuccess;

    //
    //  Get the two digit year upper boundary.
    //
    YearHigh = GetDlgItemInt(hDlg, IDC_TWO_DIGIT_YEAR_HIGH, &bSuccess, FALSE);

    if ((!bSuccess) || (YearHigh < 99) || (YearHigh > 9999))
    {
        //
        //  Invalid value, so set the lower control to 0.
        //
        SetDlgItemInt(hDlg, IDC_TWO_DIGIT_YEAR_LOW, 0, FALSE);
    }
    else
    {
        //
        //  Set the value of the lower control.
        //
        SetDlgItemInt(hDlg, IDC_TWO_DIGIT_YEAR_LOW, (UINT)(YearHigh - 99), FALSE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Date_DisplaySample
//
//  Updates the date samples.  It formats the date based on the user's
//  current locale settings.
//
////////////////////////////////////////////////////////////////////////////

void Date_DisplaySample(
    HWND hDlg)
{
    TCHAR szBuf[MAX_SAMPLE_SIZE];
    BOOL bNoError = TRUE;


    if (GetDateFormat( UserLocaleID,
                       (bShowRtL
                         ? DATE_LTRREADING
                         : 0) | DATE_SHORTDATE,
                       NULL,
                       NULL,
                       szBuf,
                       MAX_SAMPLE_SIZE ))
    {
        SetDlgItemText(hDlg, IDC_SAMPLE1, szBuf);
    }
    else
    {
        MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
        bNoError = FALSE;
    }

    //
    //  Show or hide the Arabic info based on the current user locale id.
    //
    ShowWindow(GetDlgItem(hDlg, IDC_SAMPLELBL1A), bShowRtL ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, IDC_SAMPLE1A), bShowRtL ? SW_SHOW : SW_HIDE);
    if (bShowRtL)
    {
        if (GetDateFormat( UserLocaleID,
                           DATE_RTLREADING | DATE_SHORTDATE,
                           NULL,
                           NULL,
                           szBuf,
                           MAX_SAMPLE_SIZE ))
        {
            SetDlgItemText(hDlg, IDC_SAMPLE1A, szBuf);
            SetDlgItemRTL(hDlg, IDC_SAMPLE1A);
        }
        else
        {
            MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
            bNoError = FALSE;
        }
    }


    if (GetDateFormat( UserLocaleID,
                       (bShowRtL
                         ? DATE_LTRREADING
                         : 0) | DATE_LONGDATE,
                       NULL,
                       NULL,
                       szBuf,
                       MAX_SAMPLE_SIZE ))
    {
        SetDlgItemText(hDlg, IDC_SAMPLE2, szBuf);
    }
    else if (bNoError)
    {
        MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
    }

    //
    //  Show or hide the Right to left info based on the current user locale id.
    //
    ShowWindow(GetDlgItem(hDlg, IDC_SAMPLELBL2A), bShowRtL ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, IDC_SAMPLE2A), bShowRtL ? SW_SHOW : SW_HIDE);
    if (bShowRtL)
    {
        if (GetDateFormat( UserLocaleID,
                           DATE_RTLREADING | DATE_LONGDATE,
                           NULL,
                           NULL,
                           szBuf,
                           MAX_SAMPLE_SIZE ))
        {
            SetDlgItemText(hDlg, IDC_SAMPLE2A, szBuf);
            SetDlgItemRTL(hDlg, IDC_SAMPLE2A);
        }
        else if (bNoError)
        {
            MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Date_ClearValues
//
//  Reset each of the list boxes in the date property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Date_ClearValues(
    HWND hDlg)
{
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_SHORT_DATE_STYLE));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_LONG_DATE_STYLE));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_SEPARATOR));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_CALENDAR_TYPE));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_TWO_DIGIT_YEAR_LOW));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_TWO_DIGIT_YEAR_HIGH));
}


////////////////////////////////////////////////////////////////////////////
//
//  Date_EnableHijriComboBox
//
//  Enables/Disables Show/Hides the Hijri date advance combo where necessary
//
////////////////////////////////////////////////////////////////////////////

void Date_EnableHijriComboBox(
    HWND hDlg,
    BOOL Status)
{
    HWND hAddHijriDateCB = GetDlgItem(hDlg, IDC_ADD_HIJRI_DATE);
    HWND hAddHijriDateText = GetDlgItem(hDlg, IDC_ADD_HIJRI_DATE_TEXT);
    INT iCount;


    //
    // if the combo box is empty then disable it
    //
    iCount = (INT)SendMessage(hAddHijriDateCB, CB_GETCOUNT, 0L, 0L);
    if ((iCount == CB_ERR) || (iCount <= 0L))
    {
        Status = FALSE;
    }

    EnableWindow(hAddHijriDateCB, Status);
    ShowWindow(hAddHijriDateCB, bShowArabic ? SW_SHOW : SW_HIDE );

    EnableWindow(hAddHijriDateText, Status);
    ShowWindow(hAddHijriDateText, bShowArabic ? SW_SHOW : SW_HIDE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Date_SetValues
//
//  Initialize all of the controls in the date property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Date_SetValues(
    HWND hDlg)
{
    TCHAR szBuf[SIZE_128];
    int i, nItem;
    HWND hCtrl;
    LONG CalId;

    //
    //  Initialize the dropdown box for the current locale setting for the
    //  date separator.
    //
    DropDown_Use_Locale_Values(hDlg, LOCALE_SDATE, IDC_SEPARATOR);

    //
    //  Initialize and Lock function.  If it succeeds, call enum function to
    //  enumerate all possible values for the list box via a call to EnumProc.
    //  EnumProc will call Set_List_Values for each of the string values it
    //  receives.  When the enumeration of values is complete, call
    //  Set_List_Values to clear the dialog item specific data and to clear
    //  the lock on the function.  Perform this set of operations for:
    //  Calendar Type, Short Date Sytle, and Long Date Style.
    //
    if (Set_List_Values(hDlg, IDC_CALENDAR_TYPE, 0))
    {
        hCtrl = GetDlgItem(hDlg, IDC_CALENDAR_TYPE);
        EnumCalendarInfo(EnumProc, UserLocaleID, ENUM_ALL_CALENDARS, CAL_SCALNAME);
        Set_List_Values(0, IDC_CALENDAR_TYPE, 0);
        EnumCalendarInfo(EnumProc, UserLocaleID, ENUM_ALL_CALENDARS, CAL_ICALINTVALUE);
        Set_List_Values(0, IDC_CALENDAR_TYPE, 0);
        if (GetLocaleInfo(UserLocaleID, LOCALE_ICALENDARTYPE, szBuf, SIZE_128))
        {
            CalId = StrToLong(szBuf);

            nItem = ComboBox_GetCount(hCtrl);
            for (i = 0; i < nItem; i++)
            {
                if (ComboBox_GetItemData(hCtrl, i) == CalId)
                {
                    break;
                }
            }
            ComboBox_SetCurSel(hCtrl, (i < nItem) ? i : 0);

            //
            //  Enable/disable the Add Hijri date check box.
            //
            Date_InitializeHijriDateComboBox(hDlg);
            Date_EnableHijriComboBox(hDlg, (CalId == CAL_HIJRI));

            //
            //  Set the two digit year range.
            //
            Date_GetTwoDigitYearRange(hDlg, (CALID)CalId);

            //
            //  Subtract 1 from calendar value because calendars are one
            //  based, not zero based like all other locale values.
            //
        }
        else
        {
            MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
        }

        //
        //  If more than one selection, enable dropdown box.
        //  Otherwise, disable it.
        //
        if (ComboBox_GetCount(hCtrl) > 1)
        {
            EnableWindow(GetDlgItem(hDlg, IDC_CALENDAR_TYPE_TEXT), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_CALENDAR_TYPE), TRUE);
            ShowWindow(GetDlgItem(hDlg, IDC_CALENDAR_TYPE_TEXT), SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_CALENDAR_TYPE), SW_SHOW);
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, IDC_CALENDAR_TYPE_TEXT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CALENDAR_TYPE), FALSE);
            ShowWindow(GetDlgItem(hDlg, IDC_CALENDAR_TYPE_TEXT), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_CALENDAR_TYPE), SW_HIDE);
        }
    }
    Date_EnumerateDates(hDlg, CAL_SSHORTDATE);
    Date_EnumerateDates(hDlg, CAL_SLONGDATE);

    //
    //  Display the current sample that represents all of the locale settings.
    //
    Date_DisplaySample(hDlg);
}



////////////////////////////////////////////////////////////////////////////
//
//  Date_SetHijriDate
//
//  Saves the Hijri date advance amount to the registry.
//
////////////////////////////////////////////////////////////////////////////

void Date_SetHijriDate(
    HWND hHijriComboBox)
{
    HKEY hKey;
    INT iIndex;


    //
    // Get the string index to set
    //
    iIndex = (INT)SendMessage(hHijriComboBox, CB_GETCURSEL, 0L, 0L);

    if (iIndex == CB_ERR)
        return;

    iIndex = (INT)SendMessage(hHijriComboBox, CB_GETITEMDATA, (WPARAM)iIndex, 0L);

    if (iIndex != CB_ERR)
    {
        if (RegOpenKeyEx( HKEY_CURRENT_USER,
                          c_szInternational,
                          0,
                          KEY_READ | KEY_WRITE,
                          &hKey ) == ERROR_SUCCESS)
        {

            RegSetValueEx( hKey,
                           c_szAddHijriDate,
                           0,
                           REG_SZ,
                           (LPBYTE)c_szAddHijriDateValues[iIndex],
                           (lstrlen(c_szAddHijriDateValues[iIndex]) + 1) * sizeof(TCHAR) );

            RegCloseKey(hKey);
        }
    }

    return;
}

////////////////////////////////////////////////////////////////////////////
//
//  Date_ApplySettings
//
//  For every control that has changed (that affects the Locale settings),
//  call Set_Locale_Values to update the user locale information.  Notify
//  the parent of changes and reset the change flag stored in the property
//  sheet page structure appropriately.  Redisplay the date sample if
//  bRedisplay is TRUE.
//
////////////////////////////////////////////////////////////////////////////

BOOL Date_ApplySettings(
    HWND hDlg,
    BOOL bRedisplay)
{
    TCHAR szBuf[SIZE_128];
    HKEY hKey;
    CALID CalId = 0;
    DWORD dwRecipients;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    LPARAM Changes = lpPropSheet->lParam;
    HWND hwndYearHigh = GetDlgItem(hDlg, IDC_TWO_DIGIT_YEAR_HIGH);

    if (Changes & DC_ShortFmt)
    {
        //
        //  szNLS_ShortDate is set in Date_ValidatePPS.
        //
        if (!Set_Locale_Values( hDlg,
                                LOCALE_SSHORTDATE,
                                IDC_SHORT_DATE_STYLE,
                                TEXT("sShortDate"),
                                FALSE,
                                0,
                                0,
                                szNLS_ShortDate ))
        {
            return (FALSE);
        }

        //
        //  If the date separator field has also been changed by the user,
        //  then don't update now.  It will be updated below.
        //
        if (!(Changes & DC_SDate))
        {
            //
            //  Since the short date style changed, reset date separator
            //  list box.
            //
            ComboBox_ResetContent(GetDlgItem(hDlg, IDC_SEPARATOR));
            DropDown_Use_Locale_Values(hDlg, LOCALE_SDATE, IDC_SEPARATOR);
            if (!Set_Locale_Values( hDlg,
                                    LOCALE_SDATE,
                                    IDC_SEPARATOR,
                                    TEXT("sDate"),
                                    FALSE,
                                    0,
                                    0,
                                    NULL ))
            {
                return (FALSE);
            }
        }
    }
    if (Changes & DC_LongFmt)
    {
        //
        //  szNLS_LongDate is set in Date_ValidatePPS.
        //
        if (!Set_Locale_Values( hDlg,
                                LOCALE_SLONGDATE,
                                IDC_LONG_DATE_STYLE,
                                TEXT("sLongDate"),
                                FALSE,
                                0,
                                0,
                                szNLS_LongDate ))
        {
            return (FALSE);
        }
    }
    if (Changes & DC_SDate)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_SDATE,
                                IDC_SEPARATOR,
                                TEXT("sDate"),
                                FALSE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }

        //
        //  Since the date separator changed, reset the short date style
        //  list box.
        //
        Date_EnumerateDates(hDlg, CAL_SSHORTDATE);
    }
    if (Changes & DC_Calendar)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_ICALENDARTYPE,
                                IDC_CALENDAR_TYPE,
                                0,
                                TRUE,
                                1,
                                0,
                                NULL ))
        {
            return (FALSE);
        }

        if (GetLocaleInfo(UserLocaleID, LOCALE_ICALENDARTYPE, szBuf, SIZE_128))
        {
            CalId = StrToLong(szBuf);
            Date_InitializeHijriDateComboBox(hDlg);
            Date_EnableHijriComboBox(hDlg, (CalId == CAL_HIJRI));
        }
    }

    if (Changes & DC_Arabic_Calendar)
    {
        Date_SetHijriDate( GetDlgItem(hDlg, IDC_ADD_HIJRI_DATE) );
    }

    if (Changes & DC_TwoDigitYearMax)
    {
        if ((CalId == 0) &&
            GetLocaleInfo(UserLocaleID, LOCALE_ICALENDARTYPE, szBuf, SIZE_128))
        {
            CalId = StrToLong(szBuf);
        }
        if (!Date_SetTwoDigitYearMax(hDlg, CalId))
        {
            //
            // Make sure that the API failed due to a reason other than
            // the upper year two digit max is <= 99. This can easily
            // be checked by seeing if the control is enabled or not.
            //
            if (IsWindowEnabled(hwndYearHigh))
            {
                return (FALSE);
            }
        }
    }

    PropSheet_UnChanged(GetParent(hDlg), hDlg);
    lpPropSheet->lParam = DC_EverChg;

    //
    //  Broadcast the message that the international settings in the
    //  registry have changed.
    //
    dwRecipients = BSM_APPLICATIONS | BSM_ALLDESKTOPS;
    BroadcastSystemMessage( BSF_FORCEIFHUNG | BSF_IGNORECURRENTTASK |
                              BSF_NOHANG | BSF_NOTIMEOUTIFNOTHUNG,
                            &dwRecipients,
                            WM_WININICHANGE,
                            0,
                            (LPARAM)szIntl );

    //
    //  Display the current sample that represents all of the locale settings.
    //
    if (bRedisplay)
    {
        Date_DisplaySample(hDlg);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Date_ValidatePPS
//
//  Validate each of the combo boxes whose values are constrained.
//  If any of the input fails, notify the user and then return FALSE
//  to indicate validation failure.
//
////////////////////////////////////////////////////////////////////////////

BOOL Date_ValidatePPS(
    HWND hDlg,
    LPARAM Changes)
{
    //
    //  If nothing has changed, return TRUE immediately.
    //
    if (Changes <= DC_EverChg)
    {
        return (TRUE);
    }

    //
    //  If the date separator has changed, ensure that there are no digits
    //  and no invalid characters contained in the new separator.
    //
    if (Changes & DC_SDate &&
        Item_Has_Digits_Or_Invalid_Chars( hDlg,
                                          IDC_SEPARATOR,
                                          FALSE,
                                          szInvalidSDate ))
    {
        No_Numerals_Error(hDlg, IDC_SEPARATOR, IDS_LOCALE_DATE_SEP);
        return (FALSE);
    }

    //
    //  If the short date style has changed, ensure that there are only
    //  characters in this set " dHhMmsty,-./:;\", the separator string,
    //  and text enclosed in single quotes.
    //
    if (Changes & DC_ShortFmt)
    {
        if (NLSize_Style( hDlg,
                          IDC_SHORT_DATE_STYLE,
                          szNLS_ShortDate,
                          LOCALE_SSHORTDATE ) ||
            Item_Check_Invalid_Chars( hDlg,
                                      szNLS_ShortDate,
                                      szSDateChars,
                                      IDC_SEPARATOR,
                                      FALSE,
                                      szSDCaseSwap,
                                      IDC_SHORT_DATE_STYLE ))
        {
            Invalid_Chars_Error(hDlg, IDC_SHORT_DATE_STYLE, IDS_LOCALE_SDATE);
            return (FALSE);
        }
    }

    //
    //  If the long date style has changed, ensure that there are only
    //  characters in this set " dgHhMmsty,-./:;\", the separator string,
    //  and text enclosed in single quotes.
    //
    if (Changes & DC_LongFmt)
    {
        if (NLSize_Style( hDlg,
                          IDC_LONG_DATE_STYLE,
                          szNLS_LongDate,
                          LOCALE_SLONGDATE ) ||
            Item_Check_Invalid_Chars( hDlg,
                                      szNLS_LongDate,
                                      szLDateChars,
                                      IDC_SEPARATOR,
                                      FALSE,
                                      szLDCaseSwap,
                                      IDC_LONG_DATE_STYLE ))
        {
            Invalid_Chars_Error(hDlg, IDC_LONG_DATE_STYLE, IDS_LOCALE_LDATE);
            return (FALSE);
        }
    }

    //
    //  If the two digit year has changed, make sure the value is between
    //  99 and 9999 (if the window is still enabled).
    //
    if (Changes & DC_TwoDigitYearMax)
    {
        DWORD YearHigh;
        BOOL bSuccess;

        if (IsWindowEnabled(GetDlgItem(hDlg, IDC_TWO_DIGIT_YEAR_HIGH)))
        {
            YearHigh = GetDlgItemInt( hDlg,
                                      IDC_TWO_DIGIT_YEAR_HIGH,
                                      &bSuccess,
                                      FALSE );

            if ((!bSuccess) || (YearHigh < 99) || (YearHigh > 9999))
            {
                TCHAR szBuf[SIZE_128];

                LoadString(hInstance, IDS_LOCALE_YEAR_ERROR, szBuf, SIZE_128);
                MessageBox(hDlg, szBuf, NULL, MB_OK | MB_ICONINFORMATION);
                SetFocus(GetDlgItem(hDlg, IDC_TWO_DIGIT_YEAR_HIGH));
                return (FALSE);
            }
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Date_InitializeHijriDateComboBox
//
//  Initialize the HijriDate advance combo box.
//
////////////////////////////////////////////////////////////////////////////

void Date_InitializeHijriDateComboBox(
    HWND hDlg)
{
    HWND hHijriDate = GetDlgItem(hDlg, IDC_ADD_HIJRI_DATE);
    HKEY hKey;
    TCHAR szBuf[128];
    TCHAR szCurrentValue[128];
    INT iIndex;
    DWORD dwCtr, dwNumEntries, DataLen;
    static BOOL Initialized = FALSE;

    //
    //  If already initialized, then return.
    //
    if (Initialized)
    {
        return;
    }

    //
    //  Clear contents.
    //
    SendMessage( hHijriDate,
                 CB_RESETCONTENT,
                 0L,
                 0L);

    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      c_szInternational,
                      0,
                      KEY_READ | KEY_WRITE,
                      &hKey ) == ERROR_SUCCESS)
    {

        //
        //  Read the default/current value.
        //
        if (RegQueryValueEx( hKey,
                             c_szAddHijriDate,
                             NULL,
                             NULL,
                             (LPBYTE)szCurrentValue,
                             &DataLen ) != ERROR_SUCCESS)
        {
            szCurrentValue[0] = TEXT('\0');
        }

        dwNumEntries = (sizeof(c_szAddHijriDateValues) / sizeof(PTSTR));
        for (dwCtr = 0; dwCtr < dwNumEntries; dwCtr++)
        {
            //
            //  Fill the combo box.
            //
            if (RegSetValueEx( hKey,
                               c_szAddHijriDateTemp,
                               0,
                               REG_SZ,
                               (LPBYTE)c_szAddHijriDateValues[dwCtr],
                               (lstrlen(c_szAddHijriDateValues[dwCtr]) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS)
            {
                //
                //  0x80000000 is a private flag to make GetDateFormat read
                //  the HijriDate setting from the temp reg value.
                //
                if (GetDateFormat( MAKELCID(MAKELANGID(LANG_ARABIC,
                                                       SUBLANG_DEFAULT),
                                            SORT_DEFAULT),
                                   DATE_ADDHIJRIDATETEMP | DATE_LONGDATE |
                                     DATE_RTLREADING,
                                   NULL,
                                   NULL,
                                   szBuf,
                                   sizeof(szBuf) / sizeof(TCHAR)))
                {
                    iIndex = (INT)SendMessage(hHijriDate, CB_ADDSTRING, 0L, (LPARAM)szBuf);
                    if (iIndex != CB_ERR)
                    {
                        SendMessage(hHijriDate, CB_SETITEMDATA, iIndex, (LPARAM)dwCtr);

                        if (!lstrcmp(szCurrentValue, c_szAddHijriDateValues[dwCtr]))
                        {
                            SendMessage(hHijriDate, CB_SETCURSEL, iIndex, 0L);
                        }
                    }
                }
            }
        }

        //
        //  Delete the value after we're done.
        //
        RegDeleteValue(hKey, c_szAddHijriDateTemp);

        RegCloseKey(hKey);
    }

    //
    //  Set to initialized.
    //
    Initialized = TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  Date_InitPropSheet
//
//  The extra long value for the property sheet page is used as a set of
//  state or change flags for each of the list boxes in the property sheet.
//  Initialize this value to 0.  Call Date_SetValues with the property
//  sheet handle and the value TRUE (to indicate that the Positive Value
//  button should also be initialized) to initialize all of the property
//  sheet controls.
//
////////////////////////////////////////////////////////////////////////////

void Date_InitPropSheet(
    HWND hDlg,
    LPARAM lParam)
{
    //
    //  The lParam holds a pointer to the property sheet page, save it
    //  for later reference.
    //
    SetWindowLongPtr(hDlg, DWLP_USER, lParam);

    //
    //  Set the values.
    //
    Date_SetValues(hDlg);
    szNLS_ShortDate[0] = szNLS_LongDate[0] = 0;

    ComboBox_LimitText(GetDlgItem(hDlg, IDC_SEPARATOR),        MAX_SDATE);
    ComboBox_LimitText(GetDlgItem(hDlg, IDC_SHORT_DATE_STYLE), MAX_FORMAT);
    ComboBox_LimitText(GetDlgItem(hDlg, IDC_LONG_DATE_STYLE),  MAX_FORMAT);

    Edit_LimitText(GetDlgItem(hDlg, IDC_TWO_DIGIT_YEAR_LOW),   MAX_YEAR);
    Edit_LimitText(GetDlgItem(hDlg, IDC_TWO_DIGIT_YEAR_HIGH),  MAX_YEAR);

    //
    //  Set the Add Hijri Date combo box appropriately.
    //
    if (bShowArabic)
    {
        Date_InitializeHijriDateComboBox(hDlg);
    }

    //
    //  Make sure the Apply button is off.
    //
    PropSheet_UnChanged(GetParent(hDlg), hDlg);
    if (lParam)
    {
        ((LPPROPSHEETPAGE)lParam)->lParam = DC_EverChg;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  DateDlgProc
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK DateDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    NMHDR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    DWORD dwIndex;
    HWND hCtrl;

    switch (message)
    {
        case ( WM_INITDIALOG ) :
        {
            Date_InitPropSheet(hDlg, lParam);
            break;
        }
        case ( WM_DESTROY ) :
        {
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aDateHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aDateHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            if (!lpPropSheet)
            {
                break;
            }

            switch ( LOWORD(wParam) )
            {
                case ( IDC_SHORT_DATE_STYLE ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= DC_ShortFmt;
                    }
                    break;
                }
                case ( IDC_LONG_DATE_STYLE ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= DC_LongFmt;
                    }
                    break;
                }
                case ( IDC_SEPARATOR ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= DC_SDate;
                    }
                    break;
                }
                case ( IDC_CALENDAR_TYPE ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= DC_Calendar;

                        hCtrl = GetDlgItem(hDlg, IDC_CALENDAR_TYPE);
                        dwIndex = ComboBox_GetCurSel(hCtrl);
                        if (dwIndex != CB_ERR)
                        {
                            dwIndex = (DWORD)ComboBox_GetItemData(hCtrl, dwIndex);
                            Date_InitializeHijriDateComboBox(hDlg);
                            Date_EnableHijriComboBox(hDlg, (dwIndex == CAL_HIJRI) );
                            Date_GetTwoDigitYearRange(hDlg, (CALID)dwIndex);
                        }

                        Date_EnumerateDates(hDlg, CAL_SSHORTDATE);
                        Date_EnumerateDates(hDlg, CAL_SLONGDATE);
                    }
                    break;
                }
                case ( IDC_ADD_HIJRI_DATE ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= DC_Arabic_Calendar;
                    }
                    break;
                }
                case ( IDC_TWO_DIGIT_YEAR_HIGH ) :
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                    {
                        Date_ChangeYear(hDlg);
                        lpPropSheet->lParam |= DC_TwoDigitYearMax;
                    }
                    break;
                }
            }

            //
            //  Turn on ApplyNow button.
            //
            if (lpPropSheet->lParam > DC_EverChg)
            {
                PropSheet_Changed(GetParent(hDlg), hDlg);
            }

            break;
        }
        case ( WM_NOTIFY ) :
        {
            lpnm = (NMHDR *)lParam;
            switch (lpnm->code)
            {
                case ( PSN_SETACTIVE ) :
                {
                    //
                    //  If there has been a change in the regional Locale
                    //  setting, clear all of the current info in the
                    //  property sheet, get the new values, and update the
                    //  appropriate registry values.
                    //
                    if (Verified_Regional_Chg & Process_Date)
                    {
                        Verified_Regional_Chg &= ~Process_Date;
                        Date_ClearValues(hDlg);
                        Date_SetValues(hDlg);
                        lpPropSheet->lParam = 0;
                    }
                    break;
                }
                case ( PSN_KILLACTIVE ) :
                {
                    //
                    //  Validate the entries on the property page.
                    //
                    SetWindowLongPtr( hDlg,
                                   DWLP_MSGRESULT,
                                   !Date_ValidatePPS( hDlg,
                                                      lpPropSheet->lParam ) );
                    break;
                }
                case ( PSN_APPLY ) :
                {
                    //
                    //  Apply the settings.
                    //
                    if (Date_ApplySettings(hDlg, TRUE))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                        //
                        //  Zero out the DC_EverChg bit.
                        //
                        lpPropSheet->lParam = 0;
                    }
                    else
                    {
                        SetWindowLongPtr( hDlg,
                                       DWLP_MSGRESULT,
                                       PSNRET_INVALID_NOCHANGEPAGE );
                    }

                    break;
                }
                case ( PSN_HASHELP ) :
                {
                    //
                    //  Disable help until MS provides the files and details.
                    //
                    //  FALSE is the default return value.
                    //
                //  SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                //  SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                    break;
                }
                case ( PSN_HELP ) :
                {
                    //
                    //  Call win help with the applets help file using the
                    //  "generic help button" topic.
                    //
                    //  Disable until MS provides the files and details.
                //  WinHelp(hDlg, txtHelpFile, HELP_CONTEXT, IDH_GENERIC_HELP_BUTTON);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( WM_VSCROLL ) :
        {
            if ((GET_WM_VSCROLL_CODE(wParam, lParam) == SB_ENDSCROLL) &&
                ((HWND)SendMessage( GET_WM_VSCROLL_HWND(wParam, lParam),
                                   UDM_GETBUDDY,
                                   0,
                                   0L ) == GetDlgItem(hDlg, IDC_TWO_DIGIT_YEAR_HIGH)))
            {
                DWORD YearHigh;

                //
                //  Get the high year.
                //
                YearHigh = (DWORD)SendDlgItemMessage( hDlg,
                                                      IDC_TWO_DIGIT_YEAR_ARROW,
                                                      UDM_GETPOS,
                                                      0,
                                                      0L );

                //
                //  Set the low year based on the high year.
                //
                SetDlgItemInt( hDlg,
                               IDC_TWO_DIGIT_YEAR_LOW,
                               (UINT)(YearHigh - 99),
                               FALSE );

                //
                //  Mark it as changed.
                //
                lpPropSheet->lParam |= DC_TwoDigitYearMax;

                //
                //  Turn on ApplyNow button.
                //
                PropSheet_Changed(GetParent(hDlg), hDlg);
            }

            break;
        }
        default :
        {
            return (FALSE);
        }
   }

   return (TRUE);
}

/*++

Copyright (c) 1994-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    curdlg.c

Abstract:

    This module implements the currency property sheet for the Regional
    Settings applet.

Revision History:

--*/



//
//  Include Files.
//

#include "intl.h"
#include <windowsx.h>
#include "intlhlp.h"
#include "maxvals.h"




//
//  Context Help Ids.
//
static int aCurrencyHelpIds[] =
{
    IDC_SAMPLELBL1,         IDH_INTL_CURR_POSVALUE,
    IDC_SAMPLE1,            IDH_INTL_CURR_POSVALUE,
    IDC_SAMPLELBL2,         IDH_INTL_CURR_NEGVALUE,
    IDC_SAMPLE2,            IDH_INTL_CURR_NEGVALUE,
    IDC_SAMPLELBL3,         IDH_COMM_GROUPBOX,
    IDC_POS_CURRENCY_SYM,   IDH_INTL_CURR_POSOFSYMBOL,
    IDC_CURRENCY_SYMBOL,    IDH_INTL_CURR_SYMBOL,
    IDC_NEG_NUM_FORMAT,     IDH_INTL_CURR_NEGNUMFMT,
    IDC_DECIMAL_SYMBOL,     IDH_INTL_CURR_DECSYMBOL,
    IDC_NUM_DECIMAL_DIGITS, IDH_INTL_CURR_DIGITSAFTRDEC,
    IDC_DIGIT_GROUP_SYMBOL, IDH_INTL_CURR_DIGITGRPSYMBOL,
    IDC_NUM_DIGITS_GROUP,   IDH_INTL_CURR_DIGITSINGRP,

    0, 0
};





////////////////////////////////////////////////////////////////////////////
//
//  Currency_DisplaySample
//
//  Updates the currency sample.  It formats the currency based on the
//  user's current locale settings.  It displays either a positive value
//  or a negative value based on the Positive/Negative radio buttons.
//
////////////////////////////////////////////////////////////////////////////

void Currency_DisplaySample(
    HWND hDlg)
{
    TCHAR szBuf[MAX_SAMPLE_SIZE];
    int nCharCount;

    //
    //  Get the string representing the currency format for the positive sample
    //  currency and, if the the value is valid, display it.  Perform the same
    //  operations for the negative currency sample.
    //
    nCharCount = GetCurrencyFormat( UserLocaleID,
                                    0,
                                    szSample_Number,
                                    NULL,
                                    szBuf,
                                    MAX_SAMPLE_SIZE );
    if (nCharCount)
    {
        SetDlgItemText(hDlg, IDC_SAMPLE1, szBuf);
    }
    else
    {
        MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
    }

    nCharCount = GetCurrencyFormat( UserLocaleID,
                                    0,
                                    szNegSample_Number,
                                    NULL,
                                    szBuf,
                                    MAX_SAMPLE_SIZE );
    if (nCharCount)
    {
        SetDlgItemText(hDlg, IDC_SAMPLE2, szBuf);
    }
    else
    {
        MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Currency_ClearValues
//
//  Reset each of the list boxes in the currency property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Currency_ClearValues(
    HWND hDlg)
{
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_CURRENCY_SYMBOL));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_POS_CURRENCY_SYM));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_NEG_NUM_FORMAT));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_DECIMAL_SYMBOL));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_NUM_DECIMAL_DIGITS));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_DIGIT_GROUP_SYMBOL));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_NUM_DIGITS_GROUP));
}


////////////////////////////////////////////////////////////////////////////
//
//  Currency_SetValues
//
//  Initialize all of the controls in the currency property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Currency_SetValues(
    HWND hDlg)
{
    HWND hCtrl1, hCtrl2;
    TCHAR szBuf[SIZE_128];
    int Index;
    const nMax_Array_Fill = (cInt_Str >= 10 ? 10 : cInt_Str);
    CURRENCYFMT cfmt;
    TCHAR szThousandSep[SIZE_128];
    TCHAR szEmpty[]  = TEXT("");
    TCHAR szSample[] = TEXT("123456789");

    //
    //  Initialize the dropdown box for the current locale setting for:
    //      Currency Symbol
    //      Currency Decimal Symbol
    //      Currency Grouping Symbol
    //
    DropDown_Use_Locale_Values(hDlg, LOCALE_SCURRENCY, IDC_CURRENCY_SYMBOL);
    DropDown_Use_Locale_Values(hDlg, LOCALE_SMONDECIMALSEP, IDC_DECIMAL_SYMBOL);
    DropDown_Use_Locale_Values(hDlg, LOCALE_SMONTHOUSANDSEP, IDC_DIGIT_GROUP_SYMBOL);

    //
    //  Fill in the Number of Digits after Decimal Symbol drop down list
    //  with the values of 0 through 10.  Get the user locale value and
    //  make it the current selection.  If GetLocaleInfo fails, simply
    //  select the first item in the list.
    //
    hCtrl1 = GetDlgItem(hDlg, IDC_NUM_DECIMAL_DIGITS);
    hCtrl2 = GetDlgItem(hDlg, IDC_NUM_DIGITS_GROUP);
    for (Index = 0; Index < nMax_Array_Fill; Index++)
    {
        ComboBox_InsertString(hCtrl1, -1, aInt_Str[Index]);
    }

    if (GetLocaleInfo(UserLocaleID, LOCALE_ICURRDIGITS, szBuf, SIZE_128))
    {
        ComboBox_SelectString(hCtrl1, -1, szBuf);
    }
    else
    {
        ComboBox_SetCurSel(hCtrl1, 0);
    }

    //
    //  Fill in the Number of Digits in "Thousands" Grouping's drop down
    //  list with the appropriate options.  Get the user locale value and
    //  make it the current selection.  If GetLocaleInfo fails, simply
    //  select the first item in the list.
    //
    cfmt.NumDigits = 0;                // no decimal in sample string
    cfmt.LeadingZero = 0;              // no decimal in sample string
    cfmt.lpDecimalSep = szEmpty;       // no decimal in sample string
    cfmt.NegativeOrder = 0;            // not a negative value
    cfmt.PositiveOrder = 0;            // prefix, no separation
    cfmt.lpCurrencySymbol = szEmpty;   // no currency symbol
    cfmt.lpThousandSep = szThousandSep;
    GetLocaleInfo(UserLocaleID, LOCALE_SMONTHOUSANDSEP, szThousandSep, SIZE_128);

    cfmt.Grouping = 0;
    if (GetCurrencyFormat(UserLocaleID, 0, szSample, &cfmt, szBuf, SIZE_128))
    {
        ComboBox_InsertString(hCtrl2, -1, szBuf);
    }
    cfmt.Grouping = 3;
    if (GetCurrencyFormat(UserLocaleID, 0, szSample, &cfmt, szBuf, SIZE_128))
    {
        ComboBox_InsertString(hCtrl2, -1, szBuf);
    }
    cfmt.Grouping = 32;
    if (GetCurrencyFormat(UserLocaleID, 0, szSample, &cfmt, szBuf, SIZE_128))
    {
        ComboBox_InsertString(hCtrl2, -1, szBuf);
    }

    if (GetLocaleInfo(UserLocaleID, LOCALE_SMONGROUPING, szBuf, SIZE_128) &&
        (szBuf[0]))
    {
        //
        //  Since only the values 0, 3;0, and 3;2;0 are allowed, simply
        //  ignore the ";#"s for subsequent groupings.
        //
        Index = 0;
        if (szBuf[0] == TEXT('3'))
        {
            if ((szBuf[1] == CHAR_SEMICOLON) && (szBuf[2] == TEXT('2')))
            {
                Index = 2;
            }
            else
            {
                Index = 1;
            }
        }
        else
        {
            //
            //  We used to allow the user to set #;0, where # is a value from
            //  0 - 9.  If it's 0, then fall through so that Index is 0.
            //
            if ((szBuf[0] > CHAR_ZERO) && (szBuf[0] <= CHAR_NINE) &&
                ((szBuf[1] == 0) || (lstrcmp(szBuf + 1, TEXT(";0")) == 0)))
            {
                cfmt.Grouping = szBuf[0] - CHAR_ZERO;
                if (GetCurrencyFormat(UserLocaleID, 0, szSample, &cfmt, szBuf, SIZE_128))
                {
                    Index = ComboBox_InsertString(hCtrl2, -1, szBuf);
                    if (Index >= 0)
                    {
                        ComboBox_SetItemData( hCtrl2,
                                              Index,
                                              (LPARAM)((DWORD)cfmt.Grouping) );
                    }
                    else
                    {
                        Index = 0;
                    }
                }
            }
        }
        ComboBox_SetCurSel(hCtrl2, Index);
    }
    else
    {
        ComboBox_SetCurSel(hCtrl2, 0);
    }

    //
    //  Initialize and Lock function.  If it succeeds, call enum function to
    //  enumerate all possible values for the list box via a call to EnumProc.
    //  EnumProc will call Set_List_Values for each of the string values it
    //  receives.  When the enumeration of values is complete, call
    //  Set_List_Values to clear the dialog item specific data and to clear the
    //  lock on the function.  Perform this set of operations for:
    //  Position of Currency Symbol and Negative Currency Format.
    //
    if (Set_List_Values(hDlg, IDC_POS_CURRENCY_SYM, 0))
    {
        EnumPosCurrency(EnumProcEx, UserLocaleID, 0);

        Set_List_Values(0, IDC_POS_CURRENCY_SYM, 0);
        if (GetLocaleInfo(UserLocaleID, LOCALE_ICURRENCY, szBuf, SIZE_128))
        {
            ComboBox_SetCurSel( GetDlgItem(hDlg, IDC_POS_CURRENCY_SYM),
                                StrToLong(szBuf) );
        }
        else
        {
            MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
        }
    }
    if (Set_List_Values(hDlg, IDC_NEG_NUM_FORMAT, 0))
    {
        EnumNegCurrency(EnumProcEx, UserLocaleID, 0);
        Set_List_Values(0, IDC_NEG_NUM_FORMAT, 0);
        if (GetLocaleInfo(UserLocaleID, LOCALE_INEGCURR, szBuf, SIZE_128))
        {
            ComboBox_SetCurSel( GetDlgItem(hDlg, IDC_NEG_NUM_FORMAT),
                                StrToLong(szBuf) );
        }
        else
        {
            MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
        }
    }

    //
    //  Display the current sample that represents all of the locale settings.
    //
    Currency_DisplaySample(hDlg);
}


////////////////////////////////////////////////////////////////////////////
//
//  Currency_ApplySettings
//
//  For every control that has changed (that affects the Locale settings),
//  call Set_Locale_Values to update the user locale information.  Notify
//  the parent of changes and reset the change flag stored in the property
//  sheet page structure appropriately.  Redisplay the currency sample
//  if bRedisplay is TRUE.
//
////////////////////////////////////////////////////////////////////////////

BOOL Currency_ApplySettings(
    HWND hDlg,
    BOOL bRedisplay)
{
    DWORD dwRecipients;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    LPARAM Changes = lpPropSheet->lParam;

    if (Changes & CC_SCurrency)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_SCURRENCY,
                                IDC_CURRENCY_SYMBOL,
                                TEXT("sCurrency"),
                                FALSE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & CC_CurrSymPos)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_ICURRENCY,
                                IDC_POS_CURRENCY_SYM,
                                TEXT("iCurrency"),
                                TRUE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & CC_NegCurrFmt)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_INEGCURR,
                                IDC_NEG_NUM_FORMAT,
                                TEXT("iNegCurr"),
                                TRUE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & CC_SMonDec)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_SMONDECIMALSEP,
                                IDC_DECIMAL_SYMBOL,
                                0,
                                FALSE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & CC_ICurrDigits)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_ICURRDIGITS,
                                IDC_NUM_DECIMAL_DIGITS,
                                TEXT("iCurrDigits"),
                                TRUE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & CC_SMonThousand)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_SMONTHOUSANDSEP,
                                IDC_DIGIT_GROUP_SYMBOL,
                                0,
                                FALSE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & CC_DMonGroup)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_SMONGROUPING,
                                IDC_NUM_DIGITS_GROUP,
                                0,
                                TRUE,
                                0,
                                TEXT(";0"),
                                NULL ))
        {
            return (FALSE);
        }
    }

    PropSheet_UnChanged(GetParent(hDlg), hDlg);
    lpPropSheet->lParam = CC_EverChg;

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
        Currency_ClearValues(hDlg);
        Currency_SetValues(hDlg);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Currency_ValidatePPS
//
//  Validate each of the combo boxes whose values are constrained.
//  If any of the input fails, notify the user and then return FALSE
//  to indicate validation failure.
//
////////////////////////////////////////////////////////////////////////////

BOOL Currency_ValidatePPS(
    HWND hDlg,
    LPARAM Changes)
{
    //
    //  If nothing has changed, return TRUE immediately.
    //
    if (Changes <= CC_EverChg)
    {
        return (TRUE);
    }

    //
    //  If the currency symbol has changed, ensure that there are no digits
    //  contained in the new symbol.
    //
    if ((Changes & CC_SCurrency) &&
        Item_Has_Digits(hDlg, IDC_CURRENCY_SYMBOL, FALSE))
    {
        No_Numerals_Error(hDlg, IDC_CURRENCY_SYMBOL, IDS_LOCALE_CURR_SYM);
        return (FALSE);
    }

    //
    //  If the currency's decimal symbol has changed, ensure that there are
    //  no digits contained in the new symbol.
    //
    if ((Changes & CC_SMonDec) &&
        Item_Has_Digits(hDlg, IDC_DECIMAL_SYMBOL, FALSE))
    {
        No_Numerals_Error(hDlg, IDC_DECIMAL_SYMBOL, IDS_LOCALE_CDECIMAL_SYM);
        return (FALSE);
    }

    //
    //  If the currency's thousands grouping symbol has changed, ensure that
    //  there are no digits contained in the new symbol.
    //
    if ((Changes & CC_SMonThousand) &&
        Item_Has_Digits(hDlg, IDC_DIGIT_GROUP_SYMBOL, FALSE))
    {
        No_Numerals_Error(hDlg, IDC_DIGIT_GROUP_SYMBOL, IDS_LOCALE_CGROUP_SYM);
        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Currency_InitPropSheet
//
//  The extra long value for the property sheet page is used as a set of
//  state or change flags for each of the list boxes in the property sheet.
//  Initialize this value to 0.  Call Currency_SetValues with the property
//  sheet handle to initialize all of the property sheet controls.  Limit
//  the length of the text in some of the ComboBoxes.
//
////////////////////////////////////////////////////////////////////////////

void Currency_InitPropSheet(
    HWND hDlg,
    LPARAM lParam)
{
    //
    //  The lParam holds a pointer to the property sheet page, save it
    //  for later reference.
    //
    SetWindowLongPtr(hDlg, DWLP_USER, lParam);

    Currency_SetValues(hDlg);

    ComboBox_LimitText(GetDlgItem(hDlg, IDC_CURRENCY_SYMBOL),    MAX_SCURRENCY);
    ComboBox_LimitText(GetDlgItem(hDlg, IDC_DECIMAL_SYMBOL),     MAX_SMONDECSEP);
    ComboBox_LimitText(GetDlgItem(hDlg, IDC_DIGIT_GROUP_SYMBOL), MAX_SMONTHOUSEP);
}


////////////////////////////////////////////////////////////////////////////
//
//  CurrencyDlgProc
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK CurrencyDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    NMHDR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));

    switch (message)
    {
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
                    if (Verified_Regional_Chg & Process_Curr)
                    {
                        Verified_Regional_Chg &= ~Process_Curr;
                        Currency_ClearValues(hDlg);
                        Currency_SetValues(hDlg);
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
                                   !Currency_ValidatePPS( hDlg,
                                                          lpPropSheet->lParam) );
                    break;
                }
                case ( PSN_APPLY ) :
                {
                    //
                    //  Apply the settings.
                    //
                    if (Currency_ApplySettings(hDlg, TRUE))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                        //
                        //  Zero out the CC_EverChg bit.
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
                    //  Disable until MS provides the files and details.
                    //
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
        case ( WM_INITDIALOG ) :
        {
            Currency_InitPropSheet(hDlg, lParam);
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
                     (DWORD_PTR)(LPTSTR)aCurrencyHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aCurrencyHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch ( LOWORD(wParam) )
            {
                case ( IDC_CURRENCY_SYMBOL ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= CC_SCurrency;
                    }
                    break;
                }
                case ( IDC_POS_CURRENCY_SYM ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= CC_CurrSymPos;
                    }
                    break;
                }
                case ( IDC_NEG_NUM_FORMAT ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= CC_NegCurrFmt;
                    }
                    break;
                }
                case ( IDC_DECIMAL_SYMBOL ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= CC_SMonDec;
                    }
                    break;
                }
                case ( IDC_NUM_DECIMAL_DIGITS ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= CC_ICurrDigits;
                    }
                    break;
                }
                case ( IDC_DIGIT_GROUP_SYMBOL ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= CC_SMonThousand;
                    }
                    break;
                }
                case ( IDC_NUM_DIGITS_GROUP ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= CC_DMonGroup;
                    }
                    break;
                }
            }

            //
            //  Turn on ApplyNow button.
            //
            if (lpPropSheet->lParam > CC_EverChg)
            {
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

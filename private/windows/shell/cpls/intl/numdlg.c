/*++

Copyright (c) 1994-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    numdlg.c

Abstract:

    This module implements the number property sheet for the Regional
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
//  Constant Declarations.
//

#define MAX_DIGIT_SUBST           2
#define CHAR_MAX_DIGIT_SUBST      TEXT('2')

#define EUROPEAN_DIGITS           TEXT("0123456789")
#define LPK_EUROPEAN_DIGITS       TEXT("\x206f\x0030\x0031\x0032\x0033\x0034\x0035\x0036\x0037\x0038\x0039")

#define LANGUAGE_GROUPS_KEY       TEXT("System\\CurrentControlSet\\Control\\Nls\\Language Groups")




//
//  Context Help Ids.
//

static int aNumberHelpIds[] =
{
    IDC_SAMPLELBL3,          IDH_COMM_GROUPBOX,
    IDC_SAMPLELBL1,          IDH_INTL_NUM_POSVALUE,
    IDC_SAMPLE1,             IDH_INTL_NUM_POSVALUE,
    IDC_SAMPLELBL2,          IDH_INTL_NUM_NEGVALUE,
    IDC_SAMPLE2,             IDH_INTL_NUM_NEGVALUE,

    IDC_SAMPLELBL1A,         IDH_INTL_NUM_POSVALUE_ARABIC,
    IDC_SAMPLE1A,            IDH_INTL_NUM_POSVALUE_ARABIC,
    IDC_SAMPLELBL2A,         IDH_INTL_NUM_NEGVALUE_ARABIC,
    IDC_SAMPLE2A,            IDH_INTL_NUM_NEGVALUE_ARABIC,

    IDC_DECIMAL_SYMBOL,      IDH_INTL_NUM_DECSYMBOL,
    IDC_NUM_DECIMAL_DIGITS,  IDH_INTL_NUM_DIGITSAFTRDEC,
    IDC_DIGIT_GROUP_SYMBOL,  IDH_INTL_NUM_DIGITGRPSYMBOL,
    IDC_NUM_DIGITS_GROUP,    IDH_INTL_NUM_DIGITSINGRP,
    IDC_NEG_SIGN,            IDH_INTL_NUM_NEGSIGNSYMBOL,
    IDC_NEG_NUM_FORMAT,      IDH_INTL_NUM_NEGNUMFORMAT,
    IDC_SEPARATOR,           IDH_INTL_NUM_LISTSEPARATOR,
    IDC_DISPLAY_LEAD_0,      IDH_INTL_NUM_DISPLEADZEROS,
    IDC_MEASURE_SYS,         IDH_INTL_NUM_MEASUREMNTSYS,
    IDC_NATIVE_DIGITS_TEXT,  IDH_INTL_NUM_NATIVE_DIGITS,
    IDC_NATIVE_DIGITS,       IDH_INTL_NUM_NATIVE_DIGITS,
    IDC_DIGIT_SUBST_TEXT,    IDH_INTL_NUM_DIGIT_SUBST,
    IDC_DIGIT_SUBST,         IDH_INTL_NUM_DIGIT_SUBST,

    0, 0
};




//
//  Global Variables.
//

#define MAX_LANG_GROUPS                16
#define MAX_DIGITS_PER_LG              2
static const int c_szDigitsPerLangGroup[MAX_LANG_GROUPS][MAX_DIGITS_PER_LG] =
{
    0,  0,    // 0  = (invalid)
    0,  0,    // 1  = Western Europe (added by code, see Number_SetValues(..))
    0,  0,    // 2  = Central Europe
    0,  0,    // 3  = Baltic
    0,  0,    // 4  = Greek
    0,  0,    // 5  = Cyrillic
    0,  0,    // 6  = Turkish
    0,  0,    // 7  = Japanese
    0,  0,    // 8  = Korean
    0,  0,    // 9  = Traditional Chinese
    0,  0,    // 10 = Simplified Chinese
    12, 0,    // 11 = Thai
    0,  0,    // 12 = Hebrew
    1,  2,    // 13 = Arabic
    0,  0,    // 14 = Vietnamese
    3,  8     // 15 = Indian (NT5 supports only Devenagari and Tamil (i.e. fonts and kbd))
};
static const LPTSTR c_szNativeDigits[15] =
{
    TEXT("0123456789"),                                                    // European
    TEXT("\x0660\x0661\x0662\x0663\x0664\x0665\x0666\x0667\x0668\x0669"),  // Arabic-Indic
    TEXT("\x06f0\x06f1\x06f2\x06f3\x06f4\x06f5\x06f6\x06f7\x06f8\x06f9"),  // Extended Arabic-Indic
    TEXT("\x0966\x0967\x0968\x0969\x096a\x096b\x096c\x096d\x096e\x096f"),  // Devanagari
    TEXT("\x09e6\x09e7\x09e8\x09e9\x09ea\x09eb\x09ec\x09ed\x09ee\x09ef"),  // Bengali
    TEXT("\x0a66\x0a67\x0a68\x0a69\x0a6a\x0a6b\x0a6c\x0a6d\x0a6e\x0a6f"),  // Gurmukhi
    TEXT("\x0ae6\x0ae7\x0ae8\x0ae9\x0aea\x0aeb\x0aec\x0aed\x0aee\x0aef"),  // Gujarati
    TEXT("\x0b66\x0b67\x0b68\x0b69\x0b6a\x0b6b\x0b6c\x0b6d\x0b6e\x0b6f"),  // Oriya
    TEXT("\x0030\x0be7\x0be8\x0be9\x0bea\x0beb\x0bec\x0bed\x0bee\x0bef"),  // Tamil
    TEXT("\x0c66\x0c67\x0c68\x0c69\x0c6a\x0c6b\x0c6c\x0c6d\x0c6e\x0c6f"),  // Telugu
    TEXT("\x0ce6\x0ce7\x0ce8\x0ce9\x0cea\x0ceb\x0cec\x0ced\x0cee\x0cef"),  // Kannada
    TEXT("\x0d66\x0d67\x0d68\x0d69\x0d6a\x0d6b\x0d6c\x0d6d\x0d6e\x0d6f"),  // Malayalam
    TEXT("\x0e50\x0e51\x0e52\x0e53\x0e54\x0e55\x0e56\x0e57\x0e58\x0e59"),  // Thai
    TEXT("\x0ed0\x0ed1\x0ed2\x0ed3\x0ed4\x0ed5\x0ed6\x0ed7\x0ed8\x0ed9"),  // Lao
    TEXT("\x0f20\x0f21\x0f22\x0f23\x0f24\x0f25\x0f26\x0f27\x0f28\x0f29")   // Tibetan
};





////////////////////////////////////////////////////////////////////////////
//
//  Number_IsEuropeanDigits
//
////////////////////////////////////////////////////////////////////////////

BOOL Number_IsEuropeanDigits(
    TCHAR *pNum)
{
    int Ctr;
    int Length = lstrlen(pNum);

    for (Ctr = 0; Ctr < Length; Ctr++)
    {
        if (!((pNum[Ctr] >= TEXT('0')) && (pNum[Ctr] <= TEXT('9'))))
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Number_GetDigitSubstitution
//
////////////////////////////////////////////////////////////////////////////

int Number_GetDigitSubstitution()
{
    TCHAR szBuf[10];
    int cch;

    //
    //  Get the digit substitution.
    //
    if ((cch = GetLocaleInfo(UserLocaleID, LOCALE_IDIGITSUBSTITUTION, szBuf, 10)) &&
        (cch == 2) &&
        ((szBuf[0] >= CHAR_ZERO) && (szBuf[0] <= CHAR_MAX_DIGIT_SUBST)))
    {
        return (szBuf[0] - CHAR_ZERO);
    }

    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Number_DisplaySample
//
//  Update the Number sample.  Format the number based on the user's
//  current locale settings.  Display either a positive value or a
//  negative value based on the Positive/Negative radio buttons.
//
////////////////////////////////////////////////////////////////////////////

void Number_DisplaySample(
    HWND hDlg)
{
    TCHAR szBuf[MAX_SAMPLE_SIZE];
    int nCharCount;

    //
    //  Show or hide the Arabic info based on the current user locale id.
    //
    ShowWindow(GetDlgItem(hDlg, IDC_SAMPLELBL1A), bShowArabic ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, IDC_SAMPLE1A), bShowArabic ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, IDC_SAMPLELBL2A), bShowArabic ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, IDC_SAMPLE2A), bShowArabic ? SW_SHOW : SW_HIDE);

    //
    //  Get the string representing the number format for the positive sample
    //  number and, if the the value is valid, display it.  Perform the same
    //  operations for the negative sample.
    //
    nCharCount = GetNumberFormat( UserLocaleID,
                                  0,
                                  szSample_Number,
                                  NULL,
                                  szBuf,
                                  MAX_SAMPLE_SIZE );
    if (nCharCount)
    {
        SetDlgItemText(hDlg, IDC_SAMPLE1, szBuf);
        if (bShowArabic)
        {
            SetDlgItemText(hDlg, IDC_SAMPLE1A, szBuf);
            SetDlgItemRTL(hDlg, IDC_SAMPLE1A);
        }
    }
    else
    {
        MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
    }

    nCharCount = GetNumberFormat( UserLocaleID,
                                  0,
                                  szNegSample_Number,
                                  NULL,
                                  szBuf,
                                  MAX_SAMPLE_SIZE );
    if (nCharCount)
    {
        SetDlgItemText(hDlg, IDC_SAMPLE2, szBuf);
        if (bShowArabic)
        {
            SetDlgItemText(hDlg, IDC_SAMPLE2A, szBuf);
            SetDlgItemRTL(hDlg, IDC_SAMPLE2A);
        }
    }
    else
    {
        MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Number_ClearValues
//
//  Reset each of the list boxes in the number property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Number_ClearValues(
    HWND hDlg)
{
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_DECIMAL_SYMBOL));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_NEG_SIGN));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_SEPARATOR));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_DIGIT_GROUP_SYMBOL));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_NUM_DECIMAL_DIGITS));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_NUM_DIGITS_GROUP));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_DISPLAY_LEAD_0));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_NEG_NUM_FORMAT));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_MEASURE_SYS));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_NATIVE_DIGITS));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_DIGIT_SUBST));
}


////////////////////////////////////////////////////////////////////////////
//
//  Number_SetValues
//
//  Initialize all of the controls in the number property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Number_SetValues(
    HWND hDlg)
{
    HWND hCtrl1, hCtrl2;
    HKEY hKey;
    int Index, Ctr1, Ctr2;
    DWORD cbData;
    TCHAR szBuf[SIZE_128];
    const nMax_Array_Fill = (cInt_Str >= 10 ? 10 : cInt_Str);
    NUMBERFMT nfmt;
    TCHAR szThousandSep[SIZE_128];
    TCHAR szEmpty[]  = TEXT("");
    TCHAR szSample[] = TEXT("123456789");
    BOOL bShow;

    //
    //  ----------------------------------------------------------------------
    //  Initialize the dropdown box for the current locale setting for:
    //      Decimal Symbol
    //      Positive Sign
    //      Negative Sign
    //      List Separator
    //      Grouping Symbol
    //  ----------------------------------------------------------------------
    //
    DropDown_Use_Locale_Values(hDlg, LOCALE_SDECIMAL, IDC_DECIMAL_SYMBOL);
    DropDown_Use_Locale_Values(hDlg, LOCALE_SNEGATIVESIGN, IDC_NEG_SIGN);
    DropDown_Use_Locale_Values(hDlg, LOCALE_SLIST, IDC_SEPARATOR);
    DropDown_Use_Locale_Values(hDlg, LOCALE_STHOUSAND, IDC_DIGIT_GROUP_SYMBOL);

    //
    //  ----------------------------------------------------------------------
    //  Fill in the Number of Digits after Decimal Symbol drop down list
    //  with the values of 0 through 10.  Get the user locale value and
    //  make it the current selection.  If GetLocaleInfo fails, simply
    //  select the first item in the list.
    //  ----------------------------------------------------------------------
    //
    hCtrl1 = GetDlgItem(hDlg, IDC_NUM_DECIMAL_DIGITS);
    hCtrl2 = GetDlgItem(hDlg, IDC_NUM_DIGITS_GROUP);
    for (Index = 0; Index < nMax_Array_Fill; Index++)
    {
        ComboBox_InsertString(hCtrl1, -1, aInt_Str[Index]);
    }

    if (GetLocaleInfo(UserLocaleID, LOCALE_IDIGITS, szBuf, SIZE_128))
    {
        ComboBox_SelectString(hCtrl1, -1, szBuf);
    }
    else
    {
        ComboBox_SetCurSel(hCtrl1, 0);
    }

    //
    //  ----------------------------------------------------------------------
    //  Fill in the Number of Digits in "Thousands" Grouping's drop down
    //  list with the appropriate options.  Get the user locale value and
    //  make it the current selection.  If GetLocaleInfo fails, simply
    //  select the first item in the list.
    //  ----------------------------------------------------------------------
    //
    nfmt.NumDigits = 0;                // no decimal in sample string
    nfmt.LeadingZero = 0;              // no decimal in sample string
    nfmt.lpDecimalSep = szEmpty;       // no decimal in sample string
    nfmt.NegativeOrder = 0;            // not a negative value
    nfmt.lpThousandSep = szThousandSep;
    GetLocaleInfo(UserLocaleID, LOCALE_STHOUSAND, szThousandSep, SIZE_128);

    nfmt.Grouping = 0;
    GetNumberFormat(UserLocaleID, 0, szSample, &nfmt, szBuf, SIZE_128);
    ComboBox_InsertString(hCtrl2, -1, szBuf);

    nfmt.Grouping = 3;
    GetNumberFormat(UserLocaleID, 0, szSample, &nfmt, szBuf, SIZE_128);
    ComboBox_InsertString(hCtrl2, -1, szBuf);

    nfmt.Grouping = 32;
    GetNumberFormat(UserLocaleID, 0, szSample, &nfmt, szBuf, SIZE_128);
    ComboBox_InsertString(hCtrl2, -1, szBuf);

    if (GetLocaleInfo(UserLocaleID, LOCALE_SGROUPING, szBuf, SIZE_128) &&
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
                nfmt.Grouping = szBuf[0] - CHAR_ZERO;
                if (GetNumberFormat(UserLocaleID, 0, szSample, &nfmt, szBuf, SIZE_128))
                {
                    Index = ComboBox_InsertString(hCtrl2, -1, szBuf);
                    if (Index >= 0)
                    {
                        ComboBox_SetItemData( hCtrl2,
                                              Index,
                                              (LPARAM)((DWORD)nfmt.Grouping) );
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
    //  ----------------------------------------------------------------------
    //  Initialize and Lock function.  If it succeeds, call enum function to
    //  enumerate all possible values for the list box via a call to EnumProc.
    //  EnumProc will call Set_List_Values for each of the string values it
    //  receives.  When the enumeration of values is complete, call
    //  Set_List_Values to clear the dialog item specific data and to clear
    //  the lock on the function.  Perform this set of operations for:
    //  Display Leading Zeros, Negative Number Format, and Measurement Systems.
    //  ----------------------------------------------------------------------
    //
    if (Set_List_Values(hDlg, IDC_DISPLAY_LEAD_0, 0))
    {
        EnumLeadingZeros(EnumProcEx, UserLocaleID, 0);
        Set_List_Values(0, IDC_DISPLAY_LEAD_0, 0);
        if (GetLocaleInfo(UserLocaleID, LOCALE_ILZERO, szBuf, SIZE_128))
        {
            ComboBox_SetCurSel( GetDlgItem(hDlg, IDC_DISPLAY_LEAD_0),
                                StrToLong(szBuf) );
        }
        else
        {
            MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
        }
    }
    if (Set_List_Values(hDlg, IDC_NEG_NUM_FORMAT, 0))
    {
        EnumNegNumFmt(EnumProcEx, UserLocaleID, 0);
        Set_List_Values(0, IDC_NEG_NUM_FORMAT, 0);
        if (GetLocaleInfo(UserLocaleID, LOCALE_INEGNUMBER, szBuf, SIZE_128))
        {
            ComboBox_SetCurSel( GetDlgItem(hDlg, IDC_NEG_NUM_FORMAT),
                                StrToLong(szBuf) );
        }
        else
        {
            MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
        }
    }
    if (Set_List_Values(hDlg, IDC_MEASURE_SYS, 0))
    {
        EnumMeasureSystem(EnumProc, UserLocaleID, 0);
        Set_List_Values(0, IDC_MEASURE_SYS, 0);
        if (GetLocaleInfo(UserLocaleID, LOCALE_IMEASURE, szBuf, SIZE_128))
        {
            ComboBox_SetCurSel( GetDlgItem(hDlg, IDC_MEASURE_SYS),
                                StrToLong(szBuf) );
        }
        else
        {
            MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
        }
    }

    //
    //  ----------------------------------------------------------------------
    //  Fill in the "Native Digits" dropdown and set the current selection.
    //  Only show this combo box if there is more than one entry in the list.
    //  ----------------------------------------------------------------------
    //
    hCtrl1 = GetDlgItem(hDlg, IDC_NATIVE_DIGITS);
    ComboBox_AddString( hCtrl1,
                        bLPKInstalled
                          ? LPK_EUROPEAN_DIGITS
                          : EUROPEAN_DIGITS );
    ComboBox_SetCurSel(hCtrl1, 0);

    //
    //  Go through the language groups to see which ones have extra native
    //  digits options.
    //
    //  Entry 0 in c_szNativeDigits is the European option.  If any entries
    //  in c_szDigitsPerLangGroup are 0 (European), then ignore them as the
    //  European option is always enabled.
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      LANGUAGE_GROUPS_KEY,
                      0L,
                      KEY_READ,
                      &hKey ) == ERROR_SUCCESS)
    {
        for (Ctr1 = 1; Ctr1 < MAX_LANG_GROUPS; Ctr1++)
        {
            //
            //  This assumes that if the first entry of
            //  c_szDigitsPerLangGroup is 0, then all other entries are 0.
            //
            if (c_szDigitsPerLangGroup[Ctr1][0] != 0)
            {
                //
                //  See if the language group is installed.
                //
                cbData = 0;
                wsprintf(szBuf, TEXT("%x"), Ctr1);
                RegQueryValueEx(hKey, szBuf, NULL, NULL, NULL, &cbData);
                if (cbData > sizeof(TCHAR))
                {
                    //
                    //  Installed, so add the native digit options to
                    //  the combo box.
                    //
                    for (Ctr2 = 0; Ctr2 < MAX_DIGITS_PER_LG; Ctr2++)
                    {
                        if ((Index = c_szDigitsPerLangGroup[Ctr1][Ctr2]) != 0)
                        {
                            if (ComboBox_FindStringExact(
                                            hCtrl1,
                                            -1,
                                            c_szNativeDigits[Index] ) == CB_ERR)
                            {
                                ComboBox_AddString( hCtrl1,
                                                    c_szNativeDigits[Index] );
                            }
                        }
                    }
                }
            }
        }
        RegCloseKey(hKey);
    }

    //
    //  Add the current user's Native Digits option if it's not already
    //  in the combo box.
    //
    if (GetLocaleInfo( UserLocaleID,
                       LOCALE_SNATIVEDIGITS,
                       szBuf,
                       SIZE_128 ) &&
        (!Number_IsEuropeanDigits(szBuf)))
    {
        if ((Index = ComboBox_FindStringExact(hCtrl1, -1, szBuf)) == CB_ERR)
        {
            Index = ComboBox_AddString(hCtrl1, szBuf);
        }
        if (Index != CB_ERR)
        {
            ComboBox_SetCurSel(hCtrl1, Index);
        }
    }

    //
    //  Add the default Native Digits option for the user's chosen locale
    //  if it's not already in the combo box.
    //
    if (GetLocaleInfo( UserLocaleID,
                       LOCALE_SNATIVEDIGITS | LOCALE_NOUSEROVERRIDE,
                       szBuf,
                       SIZE_128 ) &&
        (!Number_IsEuropeanDigits(szBuf)))
    {
        if (ComboBox_FindStringExact(hCtrl1, -1, szBuf) == CB_ERR)
        {
            ComboBox_AddString(hCtrl1, szBuf);
        }
    }

    //
    //  Disable the control if there is only 1 entry in the list.
    //
    bShow = ComboBox_GetCount(hCtrl1) > 1;
    EnableWindow(GetDlgItem(hDlg, IDC_NATIVE_DIGITS_TEXT), bShow);
    EnableWindow(GetDlgItem(hDlg, IDC_NATIVE_DIGITS), bShow);
    ShowWindow(GetDlgItem(hDlg, IDC_NATIVE_DIGITS_TEXT), bShow ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, IDC_NATIVE_DIGITS), bShow ? SW_SHOW : SW_HIDE);

    //
    //  ----------------------------------------------------------------------
    //  Fill in the "Digit Substitution" dropdown and set the current
    //  selection.  Only show this combo box if a language pack is installed.
    //  ----------------------------------------------------------------------
    //
    hCtrl1 = GetDlgItem(hDlg, IDC_DIGIT_SUBST);
    for (Index = 0; Index <= MAX_DIGIT_SUBST; Index++)
    {
        LoadString(hInstance, IDS_DIGIT_SUBST_CONTEXT + Index, szBuf, SIZE_128);
        ComboBox_InsertString(hCtrl1, Index, szBuf);
    }

    ComboBox_SetCurSel( hCtrl1,
                        Number_GetDigitSubstitution() );

    EnableWindow(GetDlgItem(hDlg, IDC_DIGIT_SUBST_TEXT), bLPKInstalled);
    EnableWindow(hCtrl1, bLPKInstalled);
    ShowWindow(GetDlgItem(hDlg, IDC_DIGIT_SUBST_TEXT), bLPKInstalled ? SW_SHOW : SW_HIDE);
    ShowWindow(hCtrl1, bLPKInstalled ? SW_SHOW : SW_HIDE);

    //
    //  ----------------------------------------------------------------------
    //  Display the current sample that represents all of the locale settings.
    //  ----------------------------------------------------------------------
    //
    Number_DisplaySample(hDlg);
}


////////////////////////////////////////////////////////////////////////////
//
//  Number_ApplySettings
//
//  For every control that has changed (that affects the Locale settings),
//  call Set_Locale_Values to update the user locale information.
//  Notify the parent of changes and reset the change flag stored in the
//  property sheet page structure appropriately.  Redisplay the number
//  sample if bRedisplay is TRUE.
//
////////////////////////////////////////////////////////////////////////////

BOOL Number_ApplySettings(
    HWND hDlg,
    BOOL bRedisplay)
{
    DWORD dwRecipients;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    LPARAM Changes = lpPropSheet->lParam;

    if (Changes & NC_DSymbol)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_SDECIMAL,
                                IDC_DECIMAL_SYMBOL,
                                TEXT("sDecimal"),
                                FALSE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & NC_NSign)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_SNEGATIVESIGN,
                                IDC_NEG_SIGN,
                                0,
                                FALSE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
        Verified_Regional_Chg |= Process_Curr;
    }
    if (Changes & NC_SList)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_SLIST,
                                IDC_SEPARATOR,
                                TEXT("sList"),
                                FALSE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & NC_SThousand)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_STHOUSAND,
                                IDC_DIGIT_GROUP_SYMBOL,
                                TEXT("sThousand"),
                                FALSE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & NC_IDigits)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_IDIGITS,
                                IDC_NUM_DECIMAL_DIGITS,
                                TEXT("iDigits"),
                                TRUE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & NC_DGroup)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_SGROUPING,
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
    if (Changes & NC_LZero)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_ILZERO,
                                IDC_DISPLAY_LEAD_0,
                                TEXT("iLzero"),
                                TRUE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & NC_NegFmt)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_INEGNUMBER,
                                IDC_NEG_NUM_FORMAT,
                                0,
                                TRUE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & NC_Measure)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_IMEASURE,
                                IDC_MEASURE_SYS,
                                TEXT("iMeasure"),
                                TRUE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & NC_NativeDigits)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_SNATIVEDIGITS,
                                IDC_NATIVE_DIGITS,
                                TEXT("sNativeDigits"),
                                FALSE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & NC_DigitSubst)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_IDIGITSUBSTITUTION,
                                IDC_DIGIT_SUBST,
                                TEXT("NumShape"),
                                TRUE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }

    PropSheet_UnChanged(GetParent(hDlg), hDlg);
    lpPropSheet->lParam = NC_EverChg;

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
        Number_ClearValues(hDlg);
        Number_SetValues(hDlg);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Number_ValidatePPS
//
//  Validate each of the combo boxes whose values are constrained.
//  If any of the input fails, notify the user and then return FALSE
//  to indicate validation failure.
//
////////////////////////////////////////////////////////////////////////////

BOOL Number_ValidatePPS(
    HWND hDlg,
    LPARAM Changes)
{
    //
    //  If nothing has changed, return TRUE immediately.
    //
    if (Changes <= NC_EverChg)
    {
        return (TRUE);
    }

    //
    //  If the decimal symbol has changed, ensure that there are no digits
    //  contained in the new symbol.
    //
    if (Changes & NC_DSymbol &&
        Item_Has_Digits(hDlg, IDC_DECIMAL_SYMBOL, FALSE))
    {
        No_Numerals_Error(hDlg, IDC_DECIMAL_SYMBOL, IDS_LOCALE_DECIMAL_SYM);
        return (FALSE);
    }

    //
    //  If the negative sign symbol has changed, ensure that there are no
    //  digits contained in the new symbol.
    //
    if (Changes & NC_NSign &&
        Item_Has_Digits(hDlg, IDC_NEG_SIGN, TRUE))
    {
        No_Numerals_Error(hDlg, IDC_NEG_SIGN, IDS_LOCALE_NEG_SIGN);
        return (FALSE);
    }

    //
    //  If the thousands grouping symbol has changed, ensure that there
    //  are no digits contained in the new symbol.
    //
    if (Changes & NC_SThousand &&
        Item_Has_Digits(hDlg, IDC_DIGIT_GROUP_SYMBOL, FALSE))
    {
        No_Numerals_Error(hDlg, IDC_DIGIT_GROUP_SYMBOL, IDS_LOCALE_GROUP_SYM);
        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Number_InitPropSheet
//
//  The extra long value for the property sheet page is used as a set of
//  state or change flags for each of the list boxes in the property sheet.
//  Initialize this value to 0.  Call Number_SetValues with the property
//  sheet handle to initialize all of the property sheet controls.
//  Constrain the size of certain ComboBox text sizes.
//
////////////////////////////////////////////////////////////////////////////

void Number_InitPropSheet(
    HWND hDlg,
    LPARAM lParam)
{
    //
    //  The lParam holds a pointer to the property sheet page, save it for
    //  later reference.
    //
    SetWindowLongPtr(hDlg, DWLP_USER, lParam);
    Number_SetValues(hDlg);

    ComboBox_LimitText(GetDlgItem(hDlg, IDC_NEG_SIGN),           MAX_SNEGSIGN);
    ComboBox_LimitText(GetDlgItem(hDlg, IDC_DECIMAL_SYMBOL),     MAX_SDECIMAL);
    ComboBox_LimitText(GetDlgItem(hDlg, IDC_DIGIT_GROUP_SYMBOL), MAX_STHOUSAND);
    ComboBox_LimitText(GetDlgItem(hDlg, IDC_SEPARATOR),          MAX_SLIST);
}


////////////////////////////////////////////////////////////////////////////
//
//  NumberDlgProc
//
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK NumberDlgProc(
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
                    if (Verified_Regional_Chg & Process_Num)
                    {
                        Verified_Regional_Chg &= ~Process_Num;
                        Number_ClearValues(hDlg);
                        Number_SetValues(hDlg);
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
                                   !Number_ValidatePPS( hDlg,
                                                        lpPropSheet->lParam ) );
                    break;
                }
                case ( PSN_APPLY ) :
                {
                    //
                    //  Apply the settings.
                    //
                    if (Number_ApplySettings(hDlg, TRUE))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                        //
                        //  Zero out the NC_EverChg bit.
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
                    //  Call winhelp with the applets help file using the
                    //  "generic help button" topic.
                    //
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
            Number_InitPropSheet(hDlg, lParam);
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
                     (DWORD_PTR)(LPTSTR)aNumberHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aNumberHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDC_DECIMAL_SYMBOL ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= NC_DSymbol;
                    }
                    break;
                }
                case ( IDC_NEG_SIGN ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= NC_NSign;
                    }
                    break;
                }
                case ( IDC_SEPARATOR ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= NC_SList;
                    }
                    break;
                }
                case ( IDC_DIGIT_GROUP_SYMBOL ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= NC_SThousand;
                    }
                    break;
                }
                case ( IDC_NUM_DECIMAL_DIGITS ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= NC_IDigits;
                    }
                    break;
                }
                case ( IDC_NUM_DIGITS_GROUP ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= NC_DGroup;
                    }
                    break;
                }
                case ( IDC_DISPLAY_LEAD_0 ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= NC_LZero;
                    }
                    break;
                }
                case ( IDC_NEG_NUM_FORMAT ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= NC_NegFmt;
                    }
                    break;
                }
                case ( IDC_MEASURE_SYS ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= NC_Measure;
                    }
                    break;
                }
                case ( IDC_NATIVE_DIGITS ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= NC_NativeDigits;
                    }
                    break;
                }
                case ( IDC_DIGIT_SUBST ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= NC_DigitSubst;
                    }
                    break;
                }
            }

            //
            //  Turn on ApplyNow button.
            //
            if (lpPropSheet->lParam > NC_EverChg)
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

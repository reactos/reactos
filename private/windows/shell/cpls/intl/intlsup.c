/*++

Copyright (c) 1994-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    intlsup.c

Abstract:

    This module implements the support information for the Regional
    Settings applet.

Revision History:

--*/



//
//  Include Files.
//

#include "intl.h"
#include <windowsx.h>
#include <tchar.h>




//
//  Global Variables.
//

#ifdef UNICODE
#define NUM_CURRENCY_SYMBOLS      2
LPWSTR pCurrencySymbols[] =
{
    L"$",
    L"\x20ac"
};
#endif

#define NUM_DATE_SEPARATORS       3
LPTSTR pDateSeparators[] =
{
    TEXT("/"),
    TEXT("-"),
    TEXT(".")
};


#define NUM_NEG_NUMBER_FORMATS    5
LPTSTR pNegNumberFormats[] =
{
    TEXT("(1.1)"),
    TEXT("-1.1"),
    TEXT("- 1.1"),
    TEXT("1.1-"),
    TEXT("1.1 -")
};

#define NUM_POS_CURRENCY_FORMATS  4
LPTSTR pPosCurrencyFormats[] =
{
    TEXT("¤1.1"),
    TEXT("1.1¤"),
    TEXT("¤ 1.1"),
    TEXT("1.1 ¤")
};

#define NUM_NEG_CURRENCY_FORMATS  16
LPTSTR pNegCurrencyFormats[] =
{
    TEXT("(¤1.1)"),
    TEXT("-¤1.1"),
    TEXT("¤-1.1"),
    TEXT("¤1.1-"),
    TEXT("(1.1¤)"),
    TEXT("-1.1¤"),
    TEXT("1.1-¤"),
    TEXT("1.1¤-"),
    TEXT("-1.1 ¤"),
    TEXT("-¤ 1.1"),
    TEXT("1.1 ¤-"),
    TEXT("¤ 1.1-"),
    TEXT("¤ -1.1"),
    TEXT("1.1- ¤"),
    TEXT("(¤ 1.1)"),
    TEXT("(1.1 ¤)")
};





////////////////////////////////////////////////////////////////////////////
//
//  StrToLong
//
//  Returns the long integer value stored in the string.  Since these
//  values are coming back form the NLS API as ordinal values, do not
//  worry about double byte characters.
//
////////////////////////////////////////////////////////////////////////////

LONG StrToLong(
    LPTSTR szNum)
{
    LONG Rtn_Val = 0;

    while (*szNum)
    {
        Rtn_Val = (Rtn_Val * 10) + (*szNum - CHAR_ZERO);
        szNum++;
    }
    return (Rtn_Val);
}


////////////////////////////////////////////////////////////////////////////
//
//  TransNum
//
//  Converts a number string to a dword value (in hex).
//
////////////////////////////////////////////////////////////////////////////

DWORD TransNum(
    LPTSTR lpsz)
{
    DWORD dw = 0L;
    TCHAR c;

    while (*lpsz)
    {
        c = *lpsz++;

        if (c >= TEXT('A') && c <= TEXT('F'))
        {
            c -= TEXT('A') - 0xa;
        }
        else if (c >= TEXT('0') && c <= TEXT('9'))
        {
            c -= TEXT('0');
        }
        else if (c >= TEXT('a') && c <= TEXT('f'))
        {
            c -= TEXT('a') - 0xa;
        }
        else
        {
            break;
        }
        dw *= 0x10;
        dw += c;
    }
    return (dw);
}


////////////////////////////////////////////////////////////////////////////
//
//  Item_Has_Digits
//
//  Return true if the combo box specified by item in the property sheet
//  specified by the dialog handle contains any digits.
//
////////////////////////////////////////////////////////////////////////////

BOOL Item_Has_Digits(
    HWND hDlg,
    int nItemId,
    BOOL Allow_Empty)
{
    TCHAR szBuf[SIZE_128];
    LPTSTR lpszBuf = szBuf;
    HWND hCtrl = GetDlgItem(hDlg, nItemId);
    int dwIndex = ComboBox_GetCurSel(hCtrl);

    //
    //  If there is no selection, get whatever is in the edit box.
    //
    if (dwIndex == CB_ERR)
    {
        dwIndex = GetDlgItemText(hDlg, nItemId, szBuf, SIZE_128);
        if (dwIndex)
        {
            //
            //  Get text succeeded.
            //
            szBuf[dwIndex] = 0;
        }
        else
        {
            //
            //  Get text failed.
            //
            dwIndex = CB_ERR;
        }
    }
    else
    {
        ComboBox_GetLBText(hCtrl, dwIndex, szBuf);
    }

    if (dwIndex != CB_ERR)
    {
        while (*lpszBuf)
        {
#ifndef UNICODE
            if (IsDBCSLeadByte(*lpszBuf))
            {
                //
                //  Skip 2 bytes in the array.
                //
                lpszBuf += 2;
            }
            else
#endif
            {
                if ((*lpszBuf >= CHAR_ZERO) && (*lpszBuf <= CHAR_NINE))
                {
                    return (TRUE);
                }
                lpszBuf++;
            }
        }
        return (FALSE);
    }

    //
    //  The data retrieval failed.
    //  If !Allow_Empty, just return TRUE.
    //
    return (!Allow_Empty);
}


////////////////////////////////////////////////////////////////////////////
//
//  Item_Has_Digits_Or_Invalid_Chars
//
//  Return true if the combo box specified by item in the property sheet
//  specified by the dialog handle contains any digits or any of the
//  given invalid characters.
//
////////////////////////////////////////////////////////////////////////////

BOOL Item_Has_Digits_Or_Invalid_Chars(
    HWND hDlg,
    int nItemId,
    BOOL Allow_Empty,
    LPTSTR pInvalid)
{
    TCHAR szBuf[SIZE_128];
    LPTSTR lpszBuf = szBuf;
    HWND hCtrl = GetDlgItem(hDlg, nItemId);
    int dwIndex = ComboBox_GetCurSel(hCtrl);

    //
    //  If there is no selection, get whatever is in the edit box.
    //
    if (dwIndex == CB_ERR)
    {
        dwIndex = GetDlgItemText(hDlg, nItemId, szBuf, SIZE_128);
        if (dwIndex)
        {
            //
            //  Get text succeeded.
            //
            szBuf[dwIndex] = 0;
        }
        else
        {
            //
            //  Get text failed.
            //
            dwIndex = CB_ERR;
        }
    }
    else
    {
        dwIndex = ComboBox_GetLBText(hCtrl, dwIndex, szBuf);
    }

    if (dwIndex != CB_ERR)
    {
        while (*lpszBuf)
        {
#ifndef UNICODE
            if (IsDBCSLeadByte(*lpszBuf))
            {
                //
                //  Skip 2 bytes in the array.
                //
                lpszBuf += 2;
            }
            else
#endif
            {
                if ( ((*lpszBuf >= CHAR_ZERO) && (*lpszBuf <= CHAR_NINE)) ||
                     (_tcschr(pInvalid, *lpszBuf)) )
                {
                    return (TRUE);
                }
                lpszBuf++;
            }
        }
        return (FALSE);
    }

    //
    //  The data retrieval failed.
    //  If !Allow_Empty, just return TRUE.
    //
    return (!Allow_Empty);
}


////////////////////////////////////////////////////////////////////////////
//
//  Item_Check_Invalid_Chars
//
//  Return true if the input string contains any characters that are not in
//  lpCkChars or in the string contained in the check id control combo box.
//  If there is an invalid character and the character is contained in
//  lpChgCase, change the invalid character's case so that it will be a
//  vaild character.
//
////////////////////////////////////////////////////////////////////////////

BOOL Item_Check_Invalid_Chars(
    HWND hDlg,
    LPTSTR lpszBuf,
    LPTSTR lpCkChars,
    int nCkIdStr,
    BOOL Allow_Empty,
    LPTSTR lpChgCase,
    int nItemId)
{
    TCHAR szCkBuf[SIZE_128];
    LPTSTR lpCCaseChar;
    LPTSTR lpszSaveBuf = lpszBuf;
    int nCkBufLen;
    BOOL bInQuote = FALSE;
    BOOL UpdateEditTest = FALSE;
    HWND hCtrl = GetDlgItem(hDlg, nCkIdStr);
    DWORD dwIndex = ComboBox_GetCurSel(hCtrl);
    BOOL TextFromEditBox = (ComboBox_GetCurSel(GetDlgItem(hDlg, nItemId)) == CB_ERR);

    if (!lpszBuf)
    {
        return (!Allow_Empty);
    }

    if (dwIndex != CB_ERR)
    {
        nCkBufLen = ComboBox_GetLBText(hCtrl, dwIndex, szCkBuf);
        if (nCkBufLen == CB_ERR)
        {
            nCkBufLen = 0;
        }
    }
    else
    {
        //
        //  No selection, so pull the string from the edit portion.
        //
        nCkBufLen = GetDlgItemText(hDlg, nCkIdStr, szCkBuf, SIZE_128);
        szCkBuf[nCkBufLen] = 0;
    }

    while (*lpszBuf)
    {
#ifndef UNICODE
        if (IsDBCSLeadByte(*lpszBuf))
        {
            //
            //  If the the text is in the midst of a quote, skip it.
            //  Otherwise, if there is a string from the check ID to
            //  compare, determine if the current string is equal to the
            //  string in the combo box.  If it is not equal, return true
            //  (there are invalid characters).  Otherwise, skip the entire
            //  length of the "check" combo box's string in lpszBuf.
            //
            if (bInQuote)
            {
                lpszBuf += 2;
            }
            else if (nCkBufLen &&
                     lstrlen(lpszBuf) >= nCkBufLen)
            {
                if (CompareString( UserLocaleID,
                                   0,
                                   szCkBuf,
                                   nCkBufLen,
                                   lpszBuf,
                                   nCkBufLen ) != CSTR_EQUAL)
                {
                    //
                    //  Invalid DB character.
                    //
                    return (TRUE);
                }
                lpszBuf += nCkBufLen;
            }
        }
        else
#endif
        {
            if (bInQuote)
            {
                bInQuote = (*lpszBuf != CHAR_QUOTE);
                lpszBuf++;
            }
            else if (_tcschr(lpCkChars, *lpszBuf))
            {
                lpszBuf++;
            }
            else if (TextFromEditBox &&
                     (lpCCaseChar = _tcschr(lpChgCase, *lpszBuf), lpCCaseChar))
            {
                *lpszBuf = lpCkChars[lpCCaseChar - lpChgCase];
                UpdateEditTest = TRUE;
                lpszBuf++;
            }
            else if (*lpszBuf == CHAR_QUOTE)
            {
                lpszBuf++;
                bInQuote = TRUE;
            }
            else if ( (nCkBufLen) &&
                      (lstrlen(lpszBuf) >= nCkBufLen) &&
                      (CompareString( UserLocaleID,
                                      0,
                                      szCkBuf,
                                      nCkBufLen,
                                      lpszBuf,
                                      nCkBufLen ) == CSTR_EQUAL) )
            {
                lpszBuf += nCkBufLen;
            }
            else
            {
                //
                //  Invalid character.
                //
                return (TRUE);
            }
        }
    }

    //
    //  Parsing passed.
    //  If there are unmatched quotes return TRUE.  Otherwise, return FALSE.
    //  If the edit text changed, update edit box only if returning true.
    //
    if (!bInQuote && UpdateEditTest)
    {
        return (!SetDlgItemText(hDlg, nItemId, lpszSaveBuf));
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  No_Numerals_Error
//
//  Display the no numerals allowed in "some control" error.
//
////////////////////////////////////////////////////////////////////////////

void No_Numerals_Error(
    HWND hDlg,
    int nItemId,
    int iStrId)
{
    TCHAR szBuf[SIZE_128];
    TCHAR szBuf2[SIZE_128];
    TCHAR szErrorMessage[256];

    LoadString(hInstance, IDS_LOCALE_NO_NUMS_IN, szBuf, SIZE_128);
    LoadString(hInstance, iStrId, szBuf2, SIZE_128);
    wsprintf(szErrorMessage, szBuf, szBuf2);
    MessageBox(hDlg, szErrorMessage, NULL, MB_OK | MB_ICONINFORMATION);
    SetFocus(GetDlgItem(hDlg, nItemId));
}


////////////////////////////////////////////////////////////////////////////
//
//  Invalid_Chars_Error
//
//  Display the invalid chars in "some style" error.
//
////////////////////////////////////////////////////////////////////////////

void Invalid_Chars_Error(
    HWND hDlg,
    int nItemId,
    int iStrId)
{
    TCHAR szBuf[SIZE_128];
    TCHAR szBuf2[SIZE_128];
    TCHAR szErrorMessage[256];

    LoadString(hInstance, IDS_LOCALE_STYLE_ERR, szBuf, SIZE_128);
    LoadString(hInstance, iStrId, szBuf2, SIZE_128);
    wsprintf(szErrorMessage, szBuf, szBuf2);
    MessageBox(hDlg, szErrorMessage, NULL, MB_OK | MB_ICONINFORMATION);
    SetFocus(GetDlgItem(hDlg, nItemId));
}


////////////////////////////////////////////////////////////////////////////
//
//  Localize_Combobox_Styles
//
//  Transform either all date or time style, as indicated by LCType, in
//  the indicated combobox from a value that the NLS will provide to a
//  localized value.
//
////////////////////////////////////////////////////////////////////////////

void Localize_Combobox_Styles(
    HWND hDlg,
    int nItemId,
    LCTYPE LCType)
{
    BOOL bInQuote = FALSE;
    BOOL Map_Char = TRUE;
    TCHAR szBuf1[SIZE_128];
    TCHAR szBuf2[SIZE_128];
    LPTSTR lpszInBuf = szBuf1;
    LPTSTR lpszOutBuf = szBuf2;
    HWND hCtrl = GetDlgItem(hDlg, nItemId);
    DWORD ItemCnt = ComboBox_GetCount(hCtrl);
    DWORD Position = 0;
    DWORD dwIndex;

    if (!Styles_Localized)
    {
        return;
    }

    while (Position < ItemCnt)
    {
        //
        //  Could check character count with CB_GETLBTEXTLEN to make sure
        //  that the item text will fit in 128, but max values for these
        //  items is 79 chars.
        //
        dwIndex = ComboBox_GetLBText(hCtrl, Position, szBuf1);
        if (dwIndex != CB_ERR)
        {
            lpszInBuf = szBuf1;
            lpszOutBuf = szBuf2;
            while (*lpszInBuf)
            {
                Map_Char = TRUE;
#ifndef UNICODE
                if (IsDBCSLeadByte(*lpszInBuf))
                {
                    //
                    //  Copy any double byte character straight through.
                    //
                    *lpszOutBuf++ = *lpszInBuf++;
                    *lpszOutBuf++ = *lpszInBuf++;
                }
                else
#endif
                {
                    if (*lpszInBuf == CHAR_QUOTE)
                    {
                        bInQuote = !bInQuote;
                        *lpszOutBuf++ = *lpszInBuf++;
                    }
                    else
                    {
                        if (!bInQuote)
                        {
                            if (LCType == LOCALE_STIMEFORMAT ||
                                LCType == LOCALE_SLONGDATE)
                            {
                                Map_Char = FALSE;
                                if (CompareString( UserLocaleID,
                                                   0,
                                                   lpszInBuf,
                                                   1,
                                                   TEXT("H"),
                                                   1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStyleH[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStyleH))
                                    {
                                        *lpszOutBuf++ = szStyleH[1];
                                    }
#endif
                                }
                                else if (CompareString( UserLocaleID,
                                                        0,
                                                        lpszInBuf,
                                                        1,
                                                        TEXT("h"),
                                                        1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStyleh[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStyleh))
                                    {
                                        *lpszOutBuf++ = szStyleh[1];
                                    }
#endif
                                }
                                else if (CompareString( UserLocaleID,
                                                        0,
                                                        lpszInBuf,
                                                        1,
                                                        TEXT("m"),
                                                        1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStylem[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStylem))
                                    {
                                        *lpszOutBuf++ = szStylem[1];
                                    }
#endif
                                }
                                else if (CompareString( UserLocaleID,
                                                        0,
                                                        lpszInBuf,
                                                        1,
                                                        TEXT("s"),
                                                        1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStyles[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStyles))
                                    {
                                        *lpszOutBuf++ = szStyles[1];
                                    }
#endif
                                }
                                else if (CompareString( UserLocaleID,
                                                        0,
                                                        lpszInBuf,
                                                        1,
                                                        TEXT("t"),
                                                        1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStylet[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStylet))
                                    {
                                        *lpszOutBuf++ = szStylet[1];
                                    }
#endif
                                }
                                else
                                {
                                    Map_Char = TRUE;
                                }
                            }
                            if (LCType == LOCALE_SSHORTDATE ||
                                (LCType == LOCALE_SLONGDATE && Map_Char))
                            {
                                Map_Char = FALSE;
                                if (CompareString( UserLocaleID,
                                                   0,
                                                   lpszInBuf,
                                                   1,
                                                   TEXT("d"),
                                                   1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStyled[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStyled))
                                    {
                                        *lpszOutBuf++ = szStyled[1];
                                    }
#endif
                                }
                                else if (CompareString( UserLocaleID,
                                                        0,
                                                        lpszInBuf,
                                                        1,
                                                        TEXT("M"),
                                                        1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStyleM[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStyleM))
                                    {
                                        *lpszOutBuf++ = szStyleM[1];
                                    }
#endif
                                }
                                else if (CompareString( UserLocaleID,
                                                        0,
                                                        lpszInBuf,
                                                        1,
                                                        TEXT("y"),
                                                        1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStyley[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStyley))
                                    {
                                        *lpszOutBuf++ = szStyley[1];
                                    }
#endif
                                }
                                else
                                {
                                    Map_Char = TRUE;
                                }
                            }
                        }

                        if (Map_Char)
                        {
                            *lpszOutBuf++ = *lpszInBuf++;
                        }
                        else
                        {
                            lpszInBuf++;
                        }
                    }
                }
            }

            //
            //  Append null to localized string.
            //
            *lpszOutBuf = 0;

            ComboBox_DeleteString(hCtrl, Position);
            ComboBox_InsertString(hCtrl, Position, szBuf2);
        }
        Position++;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  NLSize_Style
//
//  Transform either date or time style, as indicated by LCType, from a
//  localized value to one that the NLS API will recognize.
//
////////////////////////////////////////////////////////////////////////////

BOOL NLSize_Style(
    HWND hDlg,
    int nItemId,
    LPTSTR lpszOutBuf,
    LCTYPE LCType)
{
    BOOL bInQuote = FALSE;
    BOOL Map_Char = TRUE;
    TCHAR szBuf[SIZE_128];
    LPTSTR lpszInBuf = szBuf;
    LPTSTR lpNLSChars1;
    LPTSTR lpNLSChars2;
    HWND hCtrl = GetDlgItem(hDlg, nItemId);
    DWORD dwIndex = ComboBox_GetCurSel(hCtrl);
    BOOL TextFromEditBox = dwIndex == CB_ERR;
    int Cmp_Size;
#ifndef UNICODE
    BOOL Is_Dbl = FALSE;
#endif

    //
    //  If there is no selection, get whatever is in the edit box.
    //
    if (TextFromEditBox)
    {
        dwIndex = GetDlgItemText(hDlg, nItemId, szBuf, SIZE_128);
        if (dwIndex)
        {
            //
            //  Get text succeeded.
            //
            szBuf[dwIndex] = 0;
        }
        else
        {
            //
            //  Get text failed.
            //
            dwIndex = (DWORD)CB_ERR;
        }
    }
    else
    {
        dwIndex = ComboBox_GetLBText(hCtrl, dwIndex, szBuf);
    }

    if (!Styles_Localized)
    {
        lstrcpy(lpszOutBuf, lpszInBuf);
        return (FALSE);
    }

    switch (LCType)
    {
        case ( LOCALE_STIMEFORMAT ) :
        {
            lpNLSChars1 = szTLetters;
            lpNLSChars2 = szTCaseSwap;
            break;
        }
        case ( LOCALE_SLONGDATE ) :
        {
            lpNLSChars1 = szLDLetters;
            lpNLSChars2 = szLDCaseSwap;
            break;
        }
        case ( LOCALE_SSHORTDATE ) :
        {
            lpNLSChars1 = szSDLetters;
            lpNLSChars2 = szSDCaseSwap;
            break;
        }
    }

    while (*lpszInBuf)
    {
        Map_Char = TRUE;
#ifdef UNICODE
        Cmp_Size = 1;
#else
        Is_Dbl = IsDBCSLeadByte(*lpszInBuf);
        Cmp_Size = Is_Dbl ? 2 : 1;
#endif

        if (*lpszInBuf == CHAR_QUOTE)
        {
            bInQuote = !bInQuote;
            *lpszOutBuf++ = *lpszInBuf++;
        }
        else
        {
            if (!bInQuote)
            {
                if (LCType == LOCALE_STIMEFORMAT || LCType == LOCALE_SLONGDATE)
                {
                    Map_Char = FALSE;
                    if (CompareString( UserLocaleID,
                                       0,
                                       lpszInBuf,
                                       Cmp_Size,
                                       szStyleH,
                                       -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_CAP_H;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            szStyleh,
                                            -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_SML_H;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            szStylem,
                                            -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_SML_M;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            szStyles,
                                            -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_SML_S;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            szStylet,
                                            -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_SML_T;
                    }
                    else
                    {
                        Map_Char = TRUE;
                    }
                }
                if (LCType == LOCALE_SSHORTDATE ||
                    (LCType == LOCALE_SLONGDATE && Map_Char))
                {
                    Map_Char = FALSE;
                    if (CompareString( UserLocaleID,
                                       0,
                                       lpszInBuf,
                                       Cmp_Size,
                                       szStyled,
                                       -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_SML_D;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            szStyleM,
                                            -1) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_CAP_M;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            szStyley,
                                            -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_SML_Y;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            TEXT("g"),
                                            -1) == CSTR_EQUAL)
                    {
                        //
                        //  g is not localized, but it's legal.
                        //
                        *lpszOutBuf++ = CHAR_SML_G;
                    }
                    else
                    {
                        Map_Char = TRUE;
                    }
                }
            }

            if (Map_Char)
            {
                //
                //  Just copy chars in quotes or chars that are not
                //  recognized. Leave the char checking to the other
                //  function.  However, do check for NLS standard chars
                //  that were not supposed to be here due to localization.
                //
                if ( !bInQuote &&
#ifndef UNICODE
                     !Is_Dbl &&
#endif
                     (CompareString( UserLocaleID,
                                     0,
                                     lpszInBuf,
                                     Cmp_Size,
                                     TEXT(" "),
                                     -1 ) != CSTR_EQUAL) &&
                     ( _tcschr(lpNLSChars1, *lpszInBuf) ||
                       _tcschr(lpNLSChars2, *lpszInBuf) ) )
                {
                    return (TRUE);
                }
                *lpszOutBuf++ = *lpszInBuf++;
#ifndef UNICODE
                if (Is_Dbl)
                {
                    //
                    //  Copy 2nd byte.
                    //
                    *lpszOutBuf++ = *lpszInBuf++;
                }
#endif
            }
#ifndef UNICODE
            else if (Is_Dbl)
            {
                lpszInBuf += 2;
            }
#endif
            else
            {
                lpszInBuf++;
            }
        }
    }

    //
    //  Append null to localized string.
    //
    *lpszOutBuf = 0;

    return (FALSE);
}


#ifndef WINNT

////////////////////////////////////////////////////////////////////////////
//
//  SDate3_1_Compatibility
//
//  There is a requirement to keep windows 3.1 compatibility in the
//  registry (win.ini).  Only allow 1 or 2 'M's, 1 or 2 'd's, and
//  2 or 4 'y's.  The remainder of the date style is compatible.
//
////////////////////////////////////////////////////////////////////////////

void SDate3_1_Compatibility(
    LPTSTR lpszBuf,
    int Buf_Size)
{
    BOOL bInQuote = FALSE;
    int Index, Del_Cnt;
    int Len = lstrlen(lpszBuf);
    int MCnt = 0;                 // running total of Ms
    int dCnt = 0;                 // running total of ds
    int yCnt = 0;                 // running total of ys

    while (*lpszBuf)
    {
#ifndef UNICODE
        if (IsDBCSLeadByte(*lpszBuf))
        {
            lpszBuf += 2;
        }
        else
#endif
        {
            if (bInQuote)
            {
                bInQuote = (*lpszBuf != CHAR_QUOTE);
                lpszBuf++;
            }
            else if (*lpszBuf == CHAR_CAP_M)
            {
                if (MCnt++ < 2)
                {
                    lpszBuf++;
                }
                else
                {
                    //
                    //  At least 1 extra M.  Move all of the chars, including
                    //  null, up by Del_Cnt.
                    //
                    Del_Cnt = 1;
                    Index = 1;
                    while (lpszBuf[Index++] == CHAR_CAP_M)
                    {
                        Del_Cnt++;
                    }
                    for (Index = 0; Index <= Len - Del_Cnt + 1; Index++)
                    {
                        lpszBuf[Index] = lpszBuf[Index + Del_Cnt];
                    }
                    Len -= Del_Cnt;
                }
            }
            else if (*lpszBuf == CHAR_SML_D)
            {
                if (dCnt++ < 2)
                {
                    lpszBuf++;
                }
                else
                {
                    //
                    //  At least 1 extra d.  Move all of the chars, including
                    //  null, up by Del_Cnt.
                    //
                    Del_Cnt = 1;
                    Index = 1;
                    while (lpszBuf[Index++] == CHAR_SML_D)
                    {
                        Del_Cnt++;
                    }
                    for (Index = 0; Index <= Len - Del_Cnt + 1; Index++)
                    {
                        lpszBuf[Index] = lpszBuf[Index + Del_Cnt];
                    }
                    Len -= Del_Cnt;
                }
            }
            else if (*lpszBuf == CHAR_SML_Y)
            {
                if (yCnt == 0 || yCnt == 2)
                {
                    if (lpszBuf[1] == CHAR_SML_Y)
                    {
                        lpszBuf += 2;
                        yCnt += 2;
                    }
                    else if (Len < Buf_Size - 1)
                    {
                        //
                        //  Odd # of ys & room for one more.
                        //  Move the remaining text down by 1 (the y will
                        //  be copied).
                        //
                        //  Use Del_Cnt for unparsed string length.
                        //
                        Del_Cnt = lstrlen(lpszBuf);
                        for (Index = Del_Cnt + 1; Index > 0; Index--)
                        {
                            lpszBuf[Index] = lpszBuf[Index - 1];
                        }
                    }
                    else
                    {
                        //
                        //  No room, move all of the chars, including null,
                        //  up by 1.
                        //
                        for (Index = 0; Index <= Len; Index++)
                        {
                            lpszBuf[Index] = lpszBuf[Index + 1];
                        }
                        Len--;
                    }
                }
                else
                {
                    //
                    //  At least 1 extra y.  Move all of the chars, including
                    //  null, up by Del_Cnt.
                    //
                    Del_Cnt = 1;
                    Index = 1;
                    while (lpszBuf[Index++] == CHAR_SML_Y)
                    {
                        Del_Cnt++;
                    }
                    for (Index = 0; Index <= Len - Del_Cnt + 1; Index++)
                    {
                        lpszBuf[Index] = lpszBuf[Index + Del_Cnt];
                    }
                    Len -= Del_Cnt;
                }
            }
            else if (*lpszBuf == CHAR_QUOTE)
            {
                lpszBuf++;
                bInQuote = TRUE;
            }
            else
            {
                lpszBuf++;
            }
        }
    }
}

#endif


////////////////////////////////////////////////////////////////////////////
//
//  Set_Locale_Values
//
//  Set_Locale_Values is called for each LCType that has either been
//  directly modified via a user change, or indirectly modified by the user
//  changing the regional locale setting.  When a dialog handle is available,
//  Set_Locale_Values will pull the new value of the LCType from the
//  appropriate list box (this is a direct change), register it in the
//  locale database, and then update the registry string.  If no dialog
//  handle is available, it will simply update the registry string based on
//  the locale registry.  If the registration succeeds, return true.
//  Otherwise, return false.
//
////////////////////////////////////////////////////////////////////////////

BOOL Set_Locale_Values(
    HWND hDlg,
    LCTYPE LCType,
    int nItemId,
    LPTSTR lpIniStr,
    BOOL bValue,
    int Ordinal_Offset,
    LPTSTR Append_Str,
    LPTSTR NLS_Str)
{
    DWORD dwIndex;
    BOOL bSuccess = TRUE;
    TCHAR szBuf[SIZE_128 + 1];
    LPTSTR pBuf = szBuf;
    HWND hCtrl;

    if (NLS_Str)
    {
        //
        //  Use a non-localized string.
        //
        lstrcpy(pBuf, NLS_Str);
        bSuccess = SetLocaleInfo(UserLocaleID, LCType, pBuf);
    }
    else if (hDlg)
    {
        //
        //  Get the new value from the list box.
        //
        hCtrl = GetDlgItem(hDlg, nItemId);
        dwIndex = ComboBox_GetCurSel(hCtrl);

        //
        //  If there is no selection, get whatever is in the edit box.
        //
        if (dwIndex == CB_ERR)
        {
            dwIndex = GetDlgItemText(hDlg, nItemId, pBuf, SIZE_128);
            if (dwIndex)
            {
                //
                //  Get text succeeded.
                //
                pBuf[dwIndex] = 0;
            }
            else
            {
                //
                //  Get text failed.
                //  Allow the AM/PM symbols to be set as empty strings.
                //  Otherwise, fail.
                //
                if ((LCType == LOCALE_S1159) || (LCType == LOCALE_S2359))
                {
                    pBuf[0] = 0;
                }
                else
                {
                    bSuccess = FALSE;
                }
            }
        }
        else if (bValue)
        {
            //
            //  Need string representation of ordinal locale value.
            //
            if (nItemId == IDC_CALENDAR_TYPE)
            {
                dwIndex = (DWORD)ComboBox_GetItemData(hCtrl, dwIndex);
            }
            else
            {
                //
                //  Ordinal_Offset is required since calendar is 1 based,
                //  not 0 based.
                //
                dwIndex += Ordinal_Offset;
            }

            //
            //  Special case the grouping string.
            //
            if (nItemId == IDC_NUM_DIGITS_GROUP)
            {
                switch (dwIndex)
                {
                    case ( 0 ) :
                    {
                        lstrcpy(pBuf, TEXT("0"));
                        break;
                    }
                    case ( 1 ) :
                    {
                        lstrcpy(pBuf, TEXT("3"));
                        break;
                    }
                    case ( 2 ) :
                    {
                        lstrcpy(pBuf, TEXT("3;2"));
                        break;
                    }
                    case ( 3 ) :
                    {
                        wsprintf( pBuf,
                                  TEXT("%d"),
                                  ComboBox_GetItemData(hCtrl, dwIndex) );
                        break;
                    }
                }
            }
            else if (dwIndex < cInt_Str)
            {
                lstrcpy(pBuf, aInt_Str[dwIndex]);
            }
            else
            {
                wsprintf(pBuf, TEXT("%d"), dwIndex);
            }
        }
        else
        {
            //
            //  Get actual value of locale data.
            //
            bSuccess = (ComboBox_GetLBText(hCtrl, dwIndex, pBuf) != CB_ERR);
        }

        if (bSuccess)
        {
            //
            //  If edit text, index value or selection text succeeds...
            //
            if (Append_Str)
            {
                lstrcat(pBuf, Append_Str);
            }

            //
            //  If this is sNativeDigits, the LPK is installed, and the
            //  first char is 0x206f (nominal digit shapes), then do not
            //  store the first char in the registry.
            //
            if ((LCType == LOCALE_SNATIVEDIGITS) &&
                (bLPKInstalled) &&
                (pBuf[0] == TEXT('\x206f')))
            {
                pBuf++;
            }
            bSuccess = SetLocaleInfo(UserLocaleID, LCType, pBuf);
        }
    }

    if (lpIniStr && bSuccess)
    {
        //
        //  Set the registry string to the string that is stored in the list
        //  box.  If there is no dialog handle, get the required string
        //  locale value from the NLS function.  Write the associated string
        //  into the registry.
        //
        if (!hDlg && !NLS_Str)
        {
            GetLocaleInfo( UserLocaleID,
                           LCType | LOCALE_NOUSEROVERRIDE,
                           pBuf,
                           SIZE_128 );
        }

#ifndef WINNT
        //
        //  There is a requirement to keep windows 3.1 compatibility in the
        //  win.ini.  There are some win32 short date formats that are
        //  incompatible with exisiting win 3.1 apps... modify these styles.
        //
        if (LCType == LOCALE_SSHORTDATE)
        {
            SDate3_1_Compatibility(pBuf, SIZE_128);
        }
#endif

        //
        //  Check the value whether it is empty or not.
        //
        switch (LCType)
        {
            case ( LOCALE_STHOUSAND ) :
            case ( LOCALE_SDECIMAL ) :
            case ( LOCALE_SDATE ) :
            case ( LOCALE_STIME ) :
            case ( LOCALE_SLIST ) :
            {
                CheckEmptyString(pBuf);
                break;
            }
        }
        WriteProfileString(szIntl, lpIniStr, pBuf);
    }
    else if (!bSuccess)
    {
        LoadString(hInstance, IDS_LOCALE_SET_ERROR, szBuf, SIZE_128);
        MessageBox(hDlg, szBuf, NULL, MB_OK | MB_ICONINFORMATION);
        SetFocus(GetDlgItem(hDlg, nItemId));
        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Set_List_Values
//
//  Set_List_Values is called several times for each drop down list which is
//  populated via an enum function.  The first call to this function should
//  be with a valid dialog handle, valid dialog item ID, and null string
//  value.  If the function is not already in use, it will clear the list box
//  and store the handle and id information for the subsequent calls to this
//  function that will be made by the enumeration function.  The calls from
//  the enumeration function will add the specified string values to the
//  list box.  When the enumeration function is complete, this function
//  should be called with a null dialog handle, the valid dialog item id,
//  and a null string value.  This will clear all of the state information,
//  including the lock flag.
//
////////////////////////////////////////////////////////////////////////////

BOOL Set_List_Values(
    HWND hDlg,
    int nItemId,
    LPTSTR lpValueString)
{
    static BOOL bLock, bString;
    static HWND hDialog;
    static int nDItemId, nID;

    if (!lpValueString)
    {
        //
        //  Clear the lock if there is no dialog handle and the item IDs
        //  match.
        //
        if (bLock && !hDlg && (nItemId == nDItemId))
        {
            if (nItemId != IDC_CALENDAR_TYPE)
            {
                hDialog = 0;
                nDItemId = 0;
                bLock = FALSE;
            }
            else
            {
                if (bString)
                {
                    hDialog = 0;
                    nDItemId = 0;
                    bLock = FALSE;
                    bString = FALSE;
                }
                else
                {
                    nID = 0;
                    bString = TRUE;
                }
            }
            return (TRUE);
        }

        //
        //  Return false, for failure, if the function is locked or if the
        //  handle or ID parameters are null.
        //
        if (bLock || !hDlg || !nItemId)
        {
            return (FALSE);
        }

        //
        //  Prepare for subsequent calls to populate the list box.
        //
        bLock = TRUE;
        hDialog = hDlg;
        nDItemId = nItemId;
    }
    else if (bLock && hDialog && nDItemId)
    {
        //
        //  Add the string to the list box.
        //
        if (!bString)
        {
            ComboBox_InsertString( GetDlgItem(hDialog, nDItemId),
                                   -1,
                                   lpValueString );
        }
        else
        {
            ComboBox_SetItemData( GetDlgItem(hDialog, nDItemId),
                                  nID++,
                                  StrToLong(lpValueString) );
        }
    }
    else
    {
        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DropDown_Use_Locale_Values
//
//  Get the user locale value for the locale type specifier.  Add it to
//  the list box and make this value the current selection.  If the user
//  locale value for the locale type is different than the system value,
//  add the system value to the list box.  If the user default is different
//  than the user override, add the user default.
//
////////////////////////////////////////////////////////////////////////////

void DropDown_Use_Locale_Values(
    HWND hDlg,
    LCTYPE LCType,
    int nItemId)
{
    TCHAR szBuf[SIZE_128];
    TCHAR szCmpBuf1[SIZE_128];
    TCHAR szCmpBuf2[SIZE_128];
    HWND hCtrl = GetDlgItem(hDlg, nItemId);
    int ctr;

    if (GetLocaleInfo(UserLocaleID, LCType, szBuf, SIZE_128))
    {
        ComboBox_SetCurSel(hCtrl, ComboBox_InsertString(hCtrl, -1, szBuf));

        //
        //  If the system setting is different, add it to the list box.
        //
        if (GetLocaleInfo( SysLocaleID,
                           LCType | LOCALE_NOUSEROVERRIDE,
                           szCmpBuf1,
                           SIZE_128 ))
        {
            if (CompareString( UserLocaleID,
                               0,
                               szCmpBuf1,
                               -1,
                               szBuf,
                               -1 ) != CSTR_EQUAL)
            {
                ComboBox_InsertString(hCtrl, -1, szCmpBuf1);
            }
        }

        //
        //  If the default user locale setting is different than the user
        //  overridden setting and different than the system setting, add
        //  it to the list box.
        //
        if (GetLocaleInfo( UserLocaleID,
                           LCType | LOCALE_NOUSEROVERRIDE,
                           szCmpBuf2,
                           SIZE_128 ))
        {
            if (CompareString(UserLocaleID, 0, szCmpBuf2, -1, szBuf, -1) != CSTR_EQUAL &&
                CompareString(UserLocaleID, 0, szCmpBuf2, -1, szCmpBuf1, -1) != CSTR_EQUAL)
            {
                ComboBox_InsertString(hCtrl, -1, szCmpBuf2);
            }
        }
    }
    else
    {
        //
        //  Failed to get user value, try for system value.  If system value
        //  fails, display a message box indicating that there was a locale
        //  problem.
        //
        if (GetLocaleInfo( SysLocaleID,
                           LCType | LOCALE_NOUSEROVERRIDE,
                           szBuf,
                           SIZE_128 ))
        {
            ComboBox_SetCurSel(hCtrl, ComboBox_InsertString(hCtrl, -1, szBuf));
        }
        else
        {
            MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
        }
    }

    //
    //  If it's the date separator, then we want slash, dot, and dash in
    //  the list in addition to the user and system settings (if different).
    //
    if (LCType == LOCALE_SDATE)
    {
        for (ctr = 0; ctr < NUM_DATE_SEPARATORS; ctr++)
        {
            if (ComboBox_FindStringExact( hCtrl,
                                          -1,
                                          pDateSeparators[ctr] ) == CB_ERR)
            {
                ComboBox_InsertString(hCtrl, -1, pDateSeparators[ctr]);
            }
        }
    }

#ifdef UNICODE
    //
    //  If it's the currency symbol, then we want the Euro symbol and dollar
    //  sign in the list in addition to the user and system settings (if
    //  different).
    //
    if (LCType == LOCALE_SCURRENCY)
    {
        for (ctr = 0; ctr < NUM_CURRENCY_SYMBOLS; ctr++)
        {
            if (ComboBox_FindStringExact( hCtrl,
                                          -1,
                                          pCurrencySymbols[ctr] ) == CB_ERR)
            {
                ComboBox_InsertString(hCtrl, -1, pCurrencySymbols[ctr]);
            }
        }
    }
#endif
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumProc
//
//  This call back function calls Set_List_Values assuming that whatever
//  code called the NLS enumeration function (or dummied enumeration
//  function) has properly set up Set_List_Values for the list box
//  population.
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK EnumProc(
    LPTSTR lpValueString)
{
    return (Set_List_Values(0, 0, lpValueString));
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumProcEx
//
//  This call back function calls Set_List_Values assuming that whatever
//  code called the enumeration function has properly set up
//  Set_List_Values for the list box population.
//  Also, this function fixes the string passed in to contain the correct
//  decimal separator and negative sign, if appropriate.
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK EnumProcEx(
    LPTSTR lpValueString,
    LPTSTR lpDecimalString,
    LPTSTR lpNegativeString,
    LPTSTR lpSymbolString)
{
    TCHAR szString[SIZE_128];
    LPTSTR pStr, pValStr, pTemp;


    //
    //  Simplify things if we have a NULL string.
    //
    if (lpDecimalString && (*lpDecimalString == CHAR_NULL))
    {
        lpDecimalString = NULL;
    }
    if (lpNegativeString && (*lpNegativeString == CHAR_NULL))
    {
        lpNegativeString = NULL;
    }
    if (lpSymbolString && (*lpSymbolString == CHAR_NULL))
    {
        lpSymbolString = NULL;
    }

    //
    //  See if we need to do any substitutions.
    //
    if (lpDecimalString || lpNegativeString || lpSymbolString)
    {
        pValStr = lpValueString;
        pStr = szString;

        while (*pValStr)
        {
            if (lpDecimalString && (*pValStr == CHAR_DECIMAL))
            {
                //
                //  Substitute the current user decimal separator.
                //
                pTemp = lpDecimalString;
                while (*pTemp)
                {
                    *pStr = *pTemp;
                    pStr++;
                    pTemp++;
                }
            }
            else if (lpNegativeString && (*pValStr == CHAR_HYPHEN))
            {
                //
                //  Substitute the current user negative sign.
                //
                pTemp = lpNegativeString;
                while (*pTemp)
                {
                    *pStr = *pTemp;
                    pStr++;
                    pTemp++;
                }
            }
            else if (lpSymbolString && (*pValStr == CHAR_INTL_CURRENCY))
            {
                //
                //  Substitute the current user currency symbol.
                //
                pTemp = lpSymbolString;
                while (*pTemp)
                {
                    *pStr = *pTemp;
                    pStr++;
                    pTemp++;
                }
            }
            else
            {
                //
                //  Simply copy the character.
                //
                *pStr = *pValStr;
                pStr++;
            }
            pValStr++;
        }
        *pStr = CHAR_NULL;

        return (Set_List_Values(0, 0, szString));
    }
    else
    {
        return (Set_List_Values(0, 0, lpValueString));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumLeadingZeros
//
////////////////////////////////////////////////////////////////////////////

BOOL EnumLeadingZeros(
    LEADINGZEROS_ENUMPROC lpLeadingZerosEnumProc,
    LCID LCId,
    DWORD dwFlags)
{
    TCHAR szBuf[SIZE_128];
    TCHAR szDecimal[SIZE_128];

    //
    //  If there is no enum proc, return false to indicate a failure.
    //
    if (!lpLeadingZerosEnumProc)
    {
        return (FALSE);
    }

    //
    //  Get the Decimal Separator for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SDECIMAL, szDecimal, SIZE_128) ||
        ((szDecimal[0] == CHAR_DECIMAL) && (szDecimal[1] == CHAR_NULL)))
    {
        szDecimal[0] = CHAR_NULL;
    }

    //
    //  Call enum proc with the NO string.  Check to make sure the
    //  enum proc requests continuation.
    //
    LoadString(hInstance, IDS_NO_LZERO, szBuf, SIZE_128);
    if (!lpLeadingZerosEnumProc(szBuf, szDecimal, NULL, NULL))
    {
        return (TRUE);
    }

    //
    //  Call enum proc with the YES string.
    //
    LoadString(hInstance, IDS_LZERO, szBuf, SIZE_128);
    lpLeadingZerosEnumProc(szBuf, szDecimal, NULL, NULL);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumNegNumFmt
//
////////////////////////////////////////////////////////////////////////////

BOOL EnumNegNumFmt(
    NEGNUMFMT_ENUMPROC lpNegNumFmtEnumProc,
    LCID LCId,
    DWORD dwFlags)
{
    TCHAR szBuf[SIZE_128];
    TCHAR szDecimal[SIZE_128];
    TCHAR szNeg[SIZE_128];
    int ctr;

    //
    //  If there is no enum proc, return false to indicate a failure.
    //
    if (!lpNegNumFmtEnumProc)
    {
        return (FALSE);
    }

    //
    //  Get the Decimal Separator for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SDECIMAL, szDecimal, SIZE_128) ||
        ((szDecimal[0] == CHAR_DECIMAL) && (szDecimal[1] == CHAR_NULL)))
    {
        szDecimal[0] = CHAR_NULL;
    }

    //
    //  Get the Negative Sign for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SNEGATIVESIGN, szNeg, SIZE_128) ||
        ((szNeg[0] == CHAR_HYPHEN) && (szNeg[1] == CHAR_NULL)))
    {
        szNeg[0] = CHAR_NULL;
    }

    //
    //  Call enum proc with each format string.  Check to make sure
    //  the enum proc requests continuation.
    //
    for (ctr = 0; ctr < NUM_NEG_NUMBER_FORMATS; ctr++)
    {
        if (!lpNegNumFmtEnumProc( pNegNumberFormats[ctr],
                                  szDecimal,
                                  szNeg,
                                  NULL ))
        {
            return (TRUE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumMeasureSystem
//
////////////////////////////////////////////////////////////////////////////

BOOL EnumMeasureSystem(
    MEASURESYSTEM_ENUMPROC lpMeasureSystemEnumProc,
    LCID LCId,
    DWORD dwFlags)
{
    TCHAR szBuf[SIZE_128];

    //
    //  If there is no enum proc, return false to indicate a failure.
    //
    if (!lpMeasureSystemEnumProc)
    {
        return (FALSE);
    }

    //
    //  Call enum proc with the metric string.  Check to make sure the
    //  enum proc requests continuation.
    //
    LoadString(hInstance, IDS_METRIC, szBuf, SIZE_128);
    if (!lpMeasureSystemEnumProc(szBuf))
    {
        return (TRUE);
    }

    //
    //  Call enum proc with the U.S. string.
    //
    LoadString(hInstance, IDS_US, szBuf, SIZE_128);
    lpMeasureSystemEnumProc(szBuf);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumPosCurrency
//
////////////////////////////////////////////////////////////////////////////

BOOL EnumPosCurrency(
    POSCURRENCY_ENUMPROC lpPosCurrencyEnumProc,
    LCID LCId,
    DWORD dwFlags)
{
    TCHAR szBuf[SIZE_128];
    TCHAR szDecimal[SIZE_128];
    TCHAR szSymbol[SIZE_128];
    int ctr;

    //
    //  If there is no enum proc, return false to indicate a failure.
    //
    if (!lpPosCurrencyEnumProc)
    {
        return (FALSE);
    }

    //
    //  Get the Decimal Separator for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SMONDECIMALSEP, szDecimal, SIZE_128) ||
        ((szDecimal[0] == CHAR_DECIMAL) && (szDecimal[1] == CHAR_NULL)))
    {
        szDecimal[0] = CHAR_NULL;
    }

    //
    //  Get the Currency Symbol for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SCURRENCY, szSymbol, SIZE_128) ||
        ((szSymbol[0] == CHAR_INTL_CURRENCY) && (szSymbol[1] == CHAR_NULL)))
    {
        szSymbol[0] = CHAR_NULL;
    }

    //
    //  Call enum proc with each format string.  Check to make sure the
    //  enum proc requests continuation.
    //
    for (ctr = 0; ctr < NUM_POS_CURRENCY_FORMATS; ctr++)
    {
        if (!lpPosCurrencyEnumProc( pPosCurrencyFormats[ctr],
                                    szDecimal,
                                    NULL,
                                    szSymbol ))
        {
            return (TRUE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumNegCurrency
//
////////////////////////////////////////////////////////////////////////////

BOOL EnumNegCurrency(
    NEGCURRENCY_ENUMPROC lpNegCurrencyEnumProc,
    LCID LCId,
    DWORD dwFlags)
{
    TCHAR szBuf[SIZE_128];
    TCHAR szDecimal[SIZE_128];
    TCHAR szNeg[SIZE_128];
    TCHAR szSymbol[SIZE_128];
    int ctr;

    //
    //  If there is no enum proc, return false to indicate a failure.
    //
    if (!lpNegCurrencyEnumProc)
    {
        return (FALSE);
    }

    //
    //  Get the Decimal Separator for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SMONDECIMALSEP, szDecimal, SIZE_128) ||
        ((szDecimal[0] == CHAR_DECIMAL) && (szDecimal[1] == CHAR_NULL)))
    {
        szDecimal[0] = CHAR_NULL;
    }

    //
    //  Get the Negative Sign for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SNEGATIVESIGN, szNeg, SIZE_128) ||
        ((szNeg[0] == CHAR_HYPHEN) && (szNeg[1] == CHAR_NULL)))
    {
        szNeg[0] = CHAR_NULL;
    }

    //
    //  Get the Currency Symbol for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SCURRENCY, szSymbol, SIZE_128) ||
        ((szSymbol[0] == CHAR_INTL_CURRENCY) && (szSymbol[1] == CHAR_NULL)))
    {
        szSymbol[0] = CHAR_NULL;
    }

    //
    //  Call enum proc with each format string.  Check to make sure the
    //  enum proc requests continuation.
    //
    for (ctr = 0; ctr < NUM_NEG_CURRENCY_FORMATS; ctr++)
    {
        if (!lpNegCurrencyEnumProc( pNegCurrencyFormats[ctr],
                                    szDecimal,
                                    szNeg,
                                    szSymbol ))
        {
            return (TRUE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CheckEmptyString
//
//  If lpStr is empty, then it fills it with a null ("") string.
//  If lpStr is filled only by space, fills with a blank (" ") string.
//
////////////////////////////////////////////////////////////////////////////

void CheckEmptyString(
    LPTSTR lpStr)
{
    LPTSTR lpString;
    WORD wStrCType[64];

    if (!(*lpStr))
    {
        //
        //  Put "" string in buffer.
        //
        lstrcpy(lpStr, TEXT("\"\""));
    }
    else
    {
        for (lpString = lpStr; *lpString; lpString = CharNext(lpString))
        {
            GetStringTypeEx( LOCALE_USER_DEFAULT,
                             CT_CTYPE1,
                             lpString,
                             1,
                             wStrCType);

            if (wStrCType[0] != CHAR_SPACE)
            {
                return;
            }
        }

        //
        //  Put " " string in buffer.
        //
        lstrcpy(lpStr, TEXT("\" \""));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  SetDlgItemRTL
//
////////////////////////////////////////////////////////////////////////////

void SetDlgItemRTL(
    HWND hDlg,
    UINT uItem)
{
    HWND hItem = GetDlgItem(hDlg, uItem);
    DWORD dwExStyle = GetWindowLong(hItem, GWL_EXSTYLE);

    SetWindowLong(hItem, GWL_EXSTYLE, dwExStyle | WS_EX_RTLREADING);
}

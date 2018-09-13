//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       filesize.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "filesize.h"
#include <resource.h>

//-----------------------------------------------------------------------------
// class FileSize
//-----------------------------------------------------------------------------
//
// Static array of file size "order" string IDs.
// These IDs identify the "%1 KB", "%1 MB", "%1 GB" resource strings.
//
CArray<int> FileSize::m_rgiOrders(IDS_ORDER_EB - IDS_ORDER_BYTES + 1);

FileSize::FileSize(
    ULONGLONG ullSize
    ) : m_ullSize(ullSize)
{
    if (0 < m_rgiOrders.Count())
    {
        //
        // Initialize the static array of file size "order" string IDs.
        //
        for (int i = IDS_ORDER_BYTES; i <= IDS_ORDER_EB; i++)
        {
            m_rgiOrders[i - IDS_ORDER_BYTES] = i;
        }
    }
}


//
// FileSize assignment.
//
FileSize& 
FileSize::operator = (
    const FileSize& rhs
    )
{
    if (this != &rhs)
    {
        m_ullSize = rhs.m_ullSize;
    }
    return *this;
}


//
// The following code for converting a file size value to a text
// string (i.e. "10.5 MB") was taken from shell32.dll so that file size
// values would match those displayed in shell views.  The code isn't
// my normal style but I left it "as is" so I wouldn't break it. [brianau]
//
const int MAX_INT64_SIZE        = 30;
const int MAX_COMMA_NUMBER_SIZE = MAX_INT64_SIZE + 10;

//
// Convert a ULONGLONG file size value to a text string.
//
void 
FileSize::CvtSizeToText(
    ULONGLONG n, 
    LPTSTR pszBuffer
    ) const
{
    TCHAR     szTemp[MAX_INT64_SIZE];
    ULONGLONG iChr;

    iChr = 0;

    do {
        szTemp[iChr++] = (TCHAR)(TEXT('0') + (TCHAR)(n % 10));
        n = n / 10;
    } while (n != 0);

    do {
        iChr--;
        *pszBuffer++ = szTemp[iChr];
    } while (iChr != 0);

    *pszBuffer++ = '\0';
}


//
// Convert a string to an integer (taken from shlwapi.dll).
//
int
FileSize::StrToInt(
    LPCTSTR lpSrc
    ) const
{
    int n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == TEXT('-')) {
        bNeg = TRUE;
        lpSrc++;
    }

    while (IsDigit(*lpSrc)) {
        n *= 10;
        n += *lpSrc - TEXT('0');
        lpSrc++;
    }
    return bNeg ? -n : n;
}


//
// Add commas where necessary to a number with more than 3 digits.
//
LPTSTR 
FileSize::AddCommas(
    ULONGLONG n, 
    LPTSTR pszResult,
    int cchResult
    ) const
{
    TCHAR  szTemp[MAX_COMMA_NUMBER_SIZE];
    TCHAR  szSep[5];
    NUMBERFMT nfmt;

    nfmt.NumDigits=0;
    nfmt.LeadingZero=0;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szSep, ARRAYSIZE(szSep));
    nfmt.Grouping = StrToInt(szSep);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder= 0;

    CvtSizeToText(n, szTemp);

    if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt, pszResult, cchResult) == 0)
        lstrcpy(pszResult, szTemp);

    return pszResult;
}


//
// Format a file size value as a text string suitable for viewing.
//
void
FileSize::Format(
    ULONGLONG ullSize,
    CString *pstrFS
    ) const
{
    DBGASSERT((NULL != pstrFS));

    int i;
    ULONGLONG wInt;
    ULONGLONG dw64 = ullSize;
    UINT wLen, wDec;
    TCHAR szTemp[MAX_COMMA_NUMBER_SIZE], szFormat[5];

    if (dw64 < 1000) 
    {
        wsprintf(szTemp, TEXT("%d"), (DWORD)(dw64));
        i = 0;
    }
    else
    {
        int cOrders = m_rgiOrders.Count();
        for (i = 1; i < cOrders - 1 && dw64 >= 1000L * 1024L; dw64 >>= 10, i++);
            /* do nothing */

        wInt = dw64 >> 10;
        AddCommas(wInt, szTemp, ARRAYSIZE(szTemp));
        wLen = lstrlen(szTemp);
        if (wLen < 3)
        {
            wDec = ((DWORD)(dw64 - wInt * 1024L)) * 1000 / 1024;
            // At this point, wDec should be between 0 and 1000
            // we want get the top one (or two) digits.
            wDec /= 10;
            if (wLen == 2)
                wDec /= 10;

            // Note that we need to set the format before getting the
            // intl char.
            lstrcpy(szFormat, TEXT("%02d"));

            szFormat[2] = (TCHAR)(TEXT('0') + 3 - wLen);
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                    szTemp+wLen, ARRAYSIZE(szTemp)-wLen);
            wLen = lstrlen(szTemp);
            wLen += wsprintf(szTemp+wLen, szFormat, wDec);
        }
    }
    CString strOrderFmt(g_hInstance, m_rgiOrders[i]);
    pstrFS->Format((LPCTSTR)strOrderFmt, (LPCTSTR)szTemp);
}


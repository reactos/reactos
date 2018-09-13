//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       numconv.cxx
//
//  Contents:   Numeral String Conversions
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_NUMCONV_HXX_
#define X_NUMCONV_HXX_
#include "numconv.hxx"
#endif

#if defined(UNIX) || defined(_MAC)
// AR: Varma: BUGBUG: Remove after MW support.

/***
*wchar_t *_wcsrev(string) - reverse a wide-character string in place
*
*Purpose:
*       Reverses the order of characters in the string.  The terminating
*       null character remains in place (wide-characters).
*
*Entry:
*       wchar_t *string - string to reverse
*
*Exit:
*       returns string - now with reversed characters
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl _wcsrev (
        wchar_t * string
        )
{
        wchar_t *start = string;
        wchar_t *left = string;
        wchar_t ch;

        while (*string++)                 /* find end of string */
                ;
        string -= 2;

        while (left < string)
        {
                ch = *left;
                *left++ = *string;
                *string-- = ch;
        }

        return(start);
}

#endif // UNIX

//+----------------------------------------------------------------------
//
//  Function:   RomanNumberHelper( pn, nDivisor, szBuffer, szRomanLetter )
//
//              Computes the roman numeral for a single decimal-digit
//              equivalent.
//
//              *pn contains the current value.  We determine the string
//              that corresponds to a single decimal digit.
//
//              nDivisor is the power of 10 for which we are computing
//              the substring.
//
//              szBuffer is the pointer into the buffer into which the
//              string is written.
//
//              szRomanLetter is a three character buffer which contains
//              the 1-digit, the 5-digit, and the 10-digit, in that
//              order.  For example, when computing the unit position,
//              szRomanLetter is "ivx".
//
//  Returns:    The value of *pn is changed to the modulo of the original.
//              The roman 'digit' is written into szBuffer.
//              The updated string pointer into szBuffer is returned.
//
//-----------------------------------------------------------------------

static TCHAR * RomanNumberHelper(
    LONG *pn,
    LONG nDivisor,
    TCHAR * szBuffer,
    TCHAR * szRomanLetter )
{
    LONG digit = *pn / nDivisor;
    *pn %= nDivisor;

    if (digit > 4 && digit < 9)
        *szBuffer++ = szRomanLetter[1];

    switch (digit%5)
    {
        case 3:
            *szBuffer++ = szRomanLetter[0];
        case 2:
            *szBuffer++ = szRomanLetter[0];
        case 4:
        case 1:
            *szBuffer++ = szRomanLetter[0];
            break;
    }

    if (4 == digit)
        *szBuffer++ = szRomanLetter[1];
    else if (9 == digit)
        *szBuffer++ = szRomanLetter[2];

    return szBuffer;
}
                       
//+----------------------------------------------------------------------
//
//  Function:   RomanNumber( n, szBuffer, achRomanChars )
//
//              Convert a long value to its roman numeral equivalent.
//              We can process any value between 1 and 3999.
//              We call RomanNumberHelper() for each power of 10.
//
//  Returns:    A roman numeral string in achRomanChars[].
//
//-----------------------------------------------------------------------

static void RomanNumber(
    LONG    n,
    TCHAR * szBuffer,
    TCHAR * achRomanChars )
{
    // Emulate IE behavior -- numbers in excess of 3999 just render as
    // arabic numerals.  Netscape does weird module + offset; 4000 is
    // rendered as ii, etc.

    if (n < 1 || n > 3999)
    {
        NumberToNumeral(n, szBuffer);
    }
    else
    {
        TCHAR *p = szBuffer;

        p = RomanNumberHelper( &n, 1000, p, achRomanChars );
        p = RomanNumberHelper( &n, 100, p, achRomanChars + 3 );
        p = RomanNumberHelper( &n, 10, p, achRomanChars + 6 );
        p = RomanNumberHelper( &n, 1, p, achRomanChars + 9 );

        *p++ = _T('.');
        *p = _T('\0');
    }
}
                          
//+----------------------------------------------------------------------
//
//  Function:   NumberToRomanUpper( n, achBuffer[NUMCONV_STRLEN] )
//
//              Convert a long value to it's roman numeral equivalent.
//              The letters used here are in lowercase.
//
//  Returns:    Returns a roman number in achBuffer.
//
//-----------------------------------------------------------------------

void NumberToRomanLower(LONG n, TCHAR achBuffer[NUMCONV_STRLEN])
{
    RomanNumber(n, achBuffer, _T("m??cdmxlcivx"));
}

//+----------------------------------------------------------------------
//
//  Function:   NumberToRomanUpper( n, achBuffer[NUMCONV_STRLEN] )
//
//              Convert a long value to it's roman numeral equivalent.
//              The letters used here are in uppercase.
//
//  Returns:    Returns a roman number in achBuffer.
//
//-----------------------------------------------------------------------

void NumberToRomanUpper(LONG n, TCHAR achBuffer[NUMCONV_STRLEN])
{
    RomanNumber(n, achBuffer, _T("M??CDMXLCIVX"));
}

//+----------------------------------------------------------------------
//
//  Function:   AlphaNumber( n, szBuffer, chBase )
//
//              A helper function for NumberToRoman(Upper|Lower).
//              Pass in either 'a' or 'A' in chBase to get an
//              'alphabetic' number of n.
//
//              Zero are represented as @. (a hack.)
//              Negative numbers are represented as would decimal
//              numbers, ie with a preceeding minus sign (another hack.)
//
//  Returns:    Returns an 'alphabetic' number in szBuffer.
//
//-----------------------------------------------------------------------

static void AlphaNumber(
    LONG    n,
    TCHAR * szBuffer,
    TCHAR   chBase )
{
    TCHAR *p = szBuffer;

    if (n)
    {
        LONG m = abs(n);

        // It is easier to compute from the least-significant 'digit',
        // so we generate the string backwards, and then reverse it
        // at the end.
        
        *p++ = '.';

        while (m)
        {
            m--;
            *p++ = (TCHAR)(chBase + (m % 26));
            m /= 26;        
        }

        if (n < 0)
        {
            // A nerdly hack to represent negative numbers.

            *p++ = _T('-');
        }

        *p = _T('\0');

        _tcsrev(szBuffer);

        
    }
    else
    {
        // A nerdly hack to represent zero.

        *p++ = _T('@');
        *p++ = _T('.');
        *p++ = _T('\0');
    }
}
                          
//+----------------------------------------------------------------------
//
//  Function:   NumberToAlphaUpper( n, achBuffer[NUMCONV_STRLEN] )
//
//              Convert a long value to a 'alphabetic' string.  An
//              alphabetic string is a,b,..z,aa,ab,..,ba,..,zz,aaa, etc.
//              The letters used here will be in lowercase.
//
//  Returns:    A string in achBuffer.
//
//-----------------------------------------------------------------------

void NumberToAlphaLower(LONG n, TCHAR achBuffer[NUMCONV_STRLEN])
{
    AlphaNumber(n, achBuffer, _T('a'));
}

//+----------------------------------------------------------------------
//
//  Function:   NumberToAlphaUpper( n, achBuffer[NUMCONV_STRLEN] )
//
//              Convert a long value to a 'alphabetic' string.  An
//              alphabetic string is A,B,..Z,AA,AB,..,BA,..,ZZ,AAA, etc.
//              The letters used here will be in uppercase.
//
//  Returns:    A string in achBuffer.
//
//-----------------------------------------------------------------------

void NumberToAlphaUpper(LONG n, TCHAR achBuffer[NUMCONV_STRLEN])
{
    AlphaNumber(n, achBuffer, _T('A'));
}

//+----------------------------------------------------------------------
//
//  Function:   NumberToNumeral( n, achBuffer[NUMCONV_STRLEN] )
//
//              Convert a long value to a numeric string.
//
//  Returns:    A string in achBuffer.
//
//-----------------------------------------------------------------------

void NumberToNumeral(LONG n, TCHAR achBuffer[NUMCONV_STRLEN])
{
    _ltot(n, achBuffer, 10);
    _tcscat(achBuffer, _T("."));
}

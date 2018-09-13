/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    datetime.c

Abstract:

    This file contains the API functions that form properly formatted date
    and time strings for a given locale.

    APIs found in this file:
      GetTimeFormatW
      GetDateFormatW

Revision History:

    05-31-91    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nls.h"




//
//  Constant Declarations.
//

#define MAX_DATETIME_BUFFER  256            // max size of buffer

#define NLS_CHAR_LTR_MARK    L'\x200e'      // left to right reading order mark
#define NLS_CHAR_RTL_MARK    L'\x200f'      // right to left reading order mark

#define NLS_HEBREW_JUNE      6              // month of June (Hebrew lunar)




//
//  Forward Declarations.
//

BOOL
IsValidTime(
    LPSYSTEMTIME lpTime);

BOOL
IsValidDate(
    LPSYSTEMTIME lpDate);

WORD
GetCalendarYear(
    LPWORD *ppRange,
    CALID CalNum,
    PCALENDAR_VAR pCalInfo,
    WORD Year,
    WORD Month,
    WORD Day);

int
ParseTime(
    PLOC_HASH pHashN,
    LPSYSTEMTIME pLocalTime,
    LPWSTR pFormat,
    LPWSTR pTimeStr,
    DWORD dwFlags);

int
ParseDate(
    PLOC_HASH pHashN,
    DWORD dwFlags,
    LPSYSTEMTIME pLocalDate,
    LPWSTR pFormat,
    LPWSTR pDateStr,
    CALID CalNum,
    PCALENDAR_VAR pCalInfo,
    BOOL fLunarLeap);

DWORD
GetAbsoluteDate(
    WORD Year,
    WORD Month,
    WORD Day);

void
GetHijriDate(
    LPSYSTEMTIME pDate,
    DWORD dwFlags);

LONG
GetAdvanceHijriDate(
    DWORD dwFlags);

DWORD
DaysUpToHijriYear(
    DWORD HijriYear);

BOOL
GetHebrewDate(
    LPSYSTEMTIME pDate,
    LPBOOL pLunarLeap);

BOOL
IsValidDateForHebrew(
    WORD Year,
    WORD Month,
    WORD Day);

BOOL
NumberToHebrewLetter(
    DWORD Number,
    LPWSTR szHebrewNum,
    int cchSize);





//-------------------------------------------------------------------------//
//                            INTERNAL MACROS                              //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  NLS_COPY_UNICODE_STR
//
//  Copies a zero terminated string from pSrc to the pDest buffer.  The
//  pDest pointer is advanced to the end of the string.
//
//  DEFINED AS A MACRO.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_COPY_UNICODE_STR( pDest,                                       \
                              pSrc )                                       \
{                                                                          \
    LPWSTR pTmp;             /* temp pointer to source */                  \
                                                                           \
                                                                           \
    pTmp = pSrc;                                                           \
    while (*pTmp)                                                          \
    {                                                                      \
        *pDest = *pTmp;                                                    \
        pDest++;                                                           \
        pTmp++;                                                            \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  NLS_PAD_INT_TO_UNICODE_STR
//
//  Converts an integer value to a unicode string and stores it in the
//  buffer provided with the appropriate number of leading zeros.  The
//  pResultBuf pointer is advanced to the end of the string.
//
//  DEFINED AS A MACRO.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_PAD_INT_TO_UNICODE_STR( Value,                                 \
                                    Base,                                  \
                                    Padding,                               \
                                    pResultBuf )                           \
{                                                                          \
    UNICODE_STRING ObString;                     /* value string */        \
    WCHAR pBuffer[MAX_SMALL_BUF_LEN];            /* ptr to buffer */       \
    UINT LpCtr;                                  /* loop counter */        \
                                                                           \
                                                                           \
    /*                                                                     \
     *  Set up unicode string structure.                                   \
     */                                                                    \
    ObString.Length = MAX_SMALL_BUF_LEN * sizeof(WCHAR);                   \
    ObString.MaximumLength = MAX_SMALL_BUF_LEN * sizeof(WCHAR);            \
    ObString.Buffer = pBuffer;                                             \
                                                                           \
    /*                                                                     \
     *  Get the value as a string.  If there is an error, then do nothing. \
     */                                                                    \
    if (!RtlIntegerToUnicodeString(Value, Base, &ObString))                \
    {                                                                      \
        /*                                                                 \
         *  Pad the string with the appropriate number of zeros.           \
         */                                                                \
        for (LpCtr = GET_WC_COUNT(ObString.Length);                        \
             LpCtr < Padding;                                              \
             LpCtr++, pResultBuf++)                                        \
        {                                                                  \
            *pResultBuf = NLS_CHAR_ZERO;                                   \
        }                                                                  \
                                                                           \
        /*                                                                 \
         *  Copy the string to the result buffer.                          \
         *  The pResultBuf pointer will be advanced in the macro.          \
         */                                                                \
        NLS_COPY_UNICODE_STR(pResultBuf, ObString.Buffer);                 \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  NLS_STRING_TO_INTEGER
//
//  Converts a string to an integer value.
//
//  DEFINED AS A MACRO.
//
//  10-19-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_STRING_TO_INTEGER( CalNum,                                     \
                               pCalId )                                    \
{                                                                          \
    UNICODE_STRING ObUnicodeStr;       /* value string */                  \
                                                                           \
                                                                           \
    /*                                                                     \
     *  No need to check return value since the calendar number            \
     *  will be validated after this anyway.                               \
     */                                                                    \
    RtlInitUnicodeString(&ObUnicodeStr, pCalId);                           \
    RtlUnicodeStringToInteger(&ObUnicodeStr, 10, &CalNum);                 \
}


////////////////////////////////////////////////////////////////////////////
//
//  NLS_INSERT_BIDI_MARK
//
//  Based on the user's bidi mark preference, it either adds a
//  left to right mark or a right to left mark.
//  The pDest pointer is advanced to the next position.
//
//  DEFINED AS A MACRO.
//
//  12-03-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_INSERT_BIDI_MARK(pDest, dwFlags)                               \
{                                                                          \
    if (dwFlags & (DATE_LTRREADING | DATE_RTLREADING))                     \
    {                                                                      \
        if (dwFlags & DATE_RTLREADING)                                     \
        {                                                                  \
            *pDest = NLS_CHAR_RTL_MARK;                                    \
        }                                                                  \
        else                                                               \
        {                                                                  \
            *pDest = NLS_CHAR_LTR_MARK;                                    \
        }                                                                  \
        pDest++;                                                           \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  NLS_GREGORIAN_LEAP_YEAR
//
//  True if the given Gregorian year is a leap year.  False otherwise.
//
//  A year is a leap year if it is divisible by 4 and is not a century
//  year (multiple of 100) or if it is divisible by 400.
//
//  DEFINED AS A MACRO.
//
//  12-04-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_GREGORIAN_LEAP_YEAR(Year)                                      \
    ((Year % 4 == 0) && ((Year % 100 != 0) || (Year % 400 == 0)))


////////////////////////////////////////////////////////////////////////////
//
//  NLS_HIJRI_LEAP_YEAR
//
//  True if the given Hijri year is a leap year.  False otherwise.
//
//  A year is a leap year if it is the 2nd, 5th, 7th, 10th, 13th, 16th,
//  18th, 21st, 24th, 26th, or 29th year of a 30-year cycle.
//
//  DEFINED AS A MACRO.
//
//  12-04-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_HIJRI_LEAP_YEAR(Year)                                          \
    ((((Year * 11) + 14) % 30) < 11)




//-------------------------------------------------------------------------//
//                             API ROUTINES                                //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  GetTimeFormatW
//
//  Returns a properly formatted time string for the given locale.  It uses
//  either the system time or the specified time.  This call also indicates
//  how much memory is necessary to contain the desired information.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WINAPI GetTimeFormatW(
    LCID Locale,
    DWORD dwFlags,
    CONST SYSTEMTIME *lpTime,
    LPCWSTR lpFormat,
    LPWSTR lpTimeStr,
    int cchTime)

{
    PLOC_HASH pHashN;                       // ptr to LOC hash node
    SYSTEMTIME LocalTime;                   // local time structure
    LPWSTR pFormat;                         // ptr to time format string
    int Length = 0;                         // number of characters written
    WCHAR pString[MAX_DATETIME_BUFFER];     // ptr to temporary buffer
    WCHAR pTemp[MAX_REG_VAL_SIZE];          // temp buffer


    //
    //  Invalid Parameter Check:
    //    - validate LCID
    //    - count is negative
    //    - NULL data pointer AND count is not zero
    //
    VALIDATE_LOCALE(Locale, pHashN, FALSE);
    if ( (pHashN == NULL) ||
         (cchTime < 0) ||
         ((lpTimeStr == NULL) && (cchTime != 0)) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Invalid Flags Check:
    //    - flags other than valid ones
    //    - lpFormat not NULL AND NoUserOverride flag is set
    //
    if ( (dwFlags & GTF_INVALID_FLAG) ||
         ((lpFormat != NULL) && (dwFlags & LOCALE_NOUSEROVERRIDE)) )
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (0);
    }

    //
    //  Set pFormat to point at the proper format string.
    //
    if (lpFormat == NULL)
    {
        //
        //  Get either the user's time format from the registry or
        //  the default time format from the locale file.
        //  This string may be a null string.
        //
        if (!(dwFlags & LOCALE_NOUSEROVERRIDE) &&
            GetUserInfo( Locale,
                         LOCALE_STIMEFORMAT,
                         pNlsUserInfo->sTimeFormat,
                         NLS_VALUE_STIMEFORMAT,
                         pTemp,
                         FALSE ))
        {
            pFormat = pTemp;
        }
        else
        {
            pFormat = (LPWORD)(pHashN->pLocaleHdr) +
                      pHashN->pLocaleHdr->STimeFormat;
        }
    }
    else
    {
        //
        //  Use the format string given by the caller.
        //
        pFormat = (LPWSTR)lpFormat;
    }

    //
    //  Get the current local system time if one is not given.
    //
    if (lpTime != NULL)
    {
        //
        //  Time is given by user.  Store in local structure and
        //  validate it.
        //
        LocalTime.wHour         = lpTime->wHour;
        LocalTime.wMinute       = lpTime->wMinute;
        LocalTime.wSecond       = lpTime->wSecond;
        LocalTime.wMilliseconds = lpTime->wMilliseconds;

        if (!IsValidTime(&LocalTime))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return (0);
        }
    }
    else
    {
        GetLocalTime(&LocalTime);
    }

    //
    //  Parse the time format string.
    //
    Length = ParseTime( pHashN,
                        &LocalTime,
                        pFormat,
                        pString,
                        dwFlags );

    //
    //  Check cchTime for size of given buffer.
    //
    if (cchTime == 0)
    {
        //
        //  If cchTime is 0, then we can't use lpTimeStr.  In this
        //  case, we simply want to return the length (in characters) of
        //  the string to be copied.
        //
        return (Length);
    }
    else if (cchTime < Length)
    {
        //
        //  The buffer is too small for the string, so return an error
        //  and zero bytes written.
        //
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Copy the time string to lpTimeStr and null terminate it.
    //  Return the number of characters copied.
    //
    wcsncpy(lpTimeStr, pString, Length);
    return (Length);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetDateFormatW
//
//  Returns a properly formatted date string for the given locale.  It uses
//  either the system date or the specified date.  The user may specify
//  either the short date format or the long date format.  This call also
//  indicates how much memory is necessary to contain the desired information.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WINAPI GetDateFormatW(
    LCID Locale,
    DWORD dwFlags,
    CONST SYSTEMTIME *lpDate,
    LPCWSTR lpFormat,
    LPWSTR lpDateStr,
    int cchDate)

{
    PLOC_HASH pHashN;                       // ptr to LOC hash node
    LPWSTR pFormat;                         // ptr to format string
    SYSTEMTIME LocalDate;                   // local date structure
    int Length = 0;                         // number of characters written
    WCHAR pString[MAX_DATETIME_BUFFER];     // ptr to temporary buffer
    BOOL fAltCalendar;                      // if alternate cal flag set
    LPWSTR pOptCal;                         // ptr to optional calendar
    PCAL_INFO pCalInfo;                     // ptr to calendar info
    CALID CalNum = 0;                       // calendar number
    ULONG CalDateOffset;                    // offset to calendar data
    ULONG LocDateOffset;                    // offset to locale data
    LPWSTR pRegStr;                         // ptr to registry string to get
    LPWSTR pValue;                          // ptr to registry value to get
    WCHAR pTemp[MAX_REG_VAL_SIZE];          // temp buffer
    BOOL fLunarLeap = FALSE;                // if Hebrew Lunar leap year
    LCTYPE LCType;


    //
    //  Invalid Parameter Check:
    //    - validate LCID
    //    - count is negative
    //    - NULL data pointer AND count is not zero
    //
    VALIDATE_LOCALE(Locale, pHashN, FALSE);
    if ( (pHashN == NULL) ||
         (cchDate < 0) ||
         ((lpDateStr == NULL) && (cchDate != 0)) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Invalid Flags Check:
    //    - flags other than valid ones
    //    - more than one of either ltr reading or rtl reading
    //    - lpFormat not NULL AND flags not zero
    //
    if ( (dwFlags & GDF_INVALID_FLAG) ||
         (MORE_THAN_ONE(dwFlags, GDF_SINGLE_FLAG)) ||
         ((lpFormat != NULL) &&
          (dwFlags & (DATE_SHORTDATE | DATE_LONGDATE |
                      DATE_YEARMONTH | LOCALE_NOUSEROVERRIDE))) )
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return (0);
    }

    //
    //  See if the alternate calendar should be used.
    //
    if (fAltCalendar = (dwFlags & DATE_USE_ALT_CALENDAR))
    {
        //
        //  Get the default optional calendar.
        //
        pOptCal = (LPWORD)(pHashN->pLocaleHdr) +
                  pHashN->pLocaleHdr->IOptionalCal;

        //
        //  If there is an optional calendar, store the calendar id.
        //
        if (((POPT_CAL)pOptCal)->CalId != CAL_NO_OPTIONAL)
        {
            CalNum = ((POPT_CAL)pOptCal)->CalId;
        }
    }

    //
    //  If there was no alternate calendar, then try (in order):
    //     - the user's calendar type
    //     - the system default calendar type
    //
    if (CalNum == 0)
    {
        //
        //  Get the user's calendar type.
        //
        if ( !(dwFlags & LOCALE_NOUSEROVERRIDE) &&
             GetUserInfo( Locale,
                          LOCALE_ICALENDARTYPE,
                          pNlsUserInfo->iCalType,
                          NLS_VALUE_ICALENDARTYPE,
                          pTemp,
                          TRUE ) &&
             (pOptCal = IsValidCalendarTypeStr( pHashN, pTemp )) )
        {
            CalNum = ((POPT_CAL)pOptCal)->CalId;
        }
        else
        {
            //
            //  Get the system default calendar type.
            //
            NLS_STRING_TO_INTEGER( CalNum,
                                   pHashN->pLocaleFixed->szICalendarType );
        }
    }

    //
    //  Get the pointer to the appropriate calendar information.
    //
    if (GetCalendar(CalNum, &pCalInfo))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    //
    //  Set pFormat to point at the proper format string.
    //
    if (lpFormat == NULL)
    {
        //
        //  Find out which flag is set and save the appropriate
        //  information.
        //
        switch (dwFlags & (DATE_SHORTDATE | DATE_LONGDATE | DATE_YEARMONTH))
        {
            case ( 0 ) :
            case ( DATE_SHORTDATE ) :
            {
                //
                //  Get the offset values for the shortdate.
                //
                CalDateOffset = (ULONG)FIELD_OFFSET(CALENDAR_VAR, SShortDate);
                LocDateOffset = (ULONG)FIELD_OFFSET(LOCALE_VAR, SShortDate);
                pRegStr = pNlsUserInfo->sShortDate;
                pValue = NLS_VALUE_SSHORTDATE;
                LCType = LOCALE_SSHORTDATE;

                break;
            }
            case ( DATE_LONGDATE ) :
            {
                //
                //  Get the offset values for the longdate.
                //
                CalDateOffset = (ULONG)FIELD_OFFSET(CALENDAR_VAR, SLongDate);
                LocDateOffset = (ULONG)FIELD_OFFSET(LOCALE_VAR, SLongDate);
                pRegStr = pNlsUserInfo->sLongDate;
                pValue = NLS_VALUE_SLONGDATE;
                LCType = LOCALE_SLONGDATE;

                break;
            }
            case ( DATE_YEARMONTH ) :
            {
                //
                //  Get the offset values for the year/month.
                //
                CalDateOffset = (ULONG)FIELD_OFFSET(CALENDAR_VAR, SYearMonth);
                LocDateOffset = (ULONG)FIELD_OFFSET(LOCALE_VAR, SYearMonth);
                pRegStr = pNlsUserInfo->sYearMonth;
                pValue = NLS_VALUE_SYEARMONTH;
                LCType = LOCALE_SYEARMONTH;

                break;
            }
            default :
            {
                SetLastError(ERROR_INVALID_FLAGS);
                return (0);
            }
        }

        //
        //  Get the proper format string for the given locale.
        //  This string may be a null string.
        //
        pFormat = NULL;
        if (fAltCalendar && (CalNum != CAL_GREGORIAN))
        {
            pFormat = (LPWORD)pCalInfo +
                      *((LPWORD)((LPBYTE)(pCalInfo) + CalDateOffset));

            if (*pFormat == 0)
            {
                pFormat = NULL;
            }
        }

        if (pFormat == NULL)
        {
            if (!(dwFlags & LOCALE_NOUSEROVERRIDE) &&
                GetUserInfo(Locale, LCType, pRegStr, pValue, pTemp, TRUE))
            {
                pFormat = pTemp;
            }
            else
            {
                pFormat = (LPWORD)pCalInfo +
                          *((LPWORD)((LPBYTE)(pCalInfo) + CalDateOffset));

                if (*pFormat == 0)
                {
                    pFormat = (LPWORD)(pHashN->pLocaleHdr) +
                              *((LPWORD)((LPBYTE)(pHashN->pLocaleHdr) +
                                         LocDateOffset));
                }
            }
        }
    }
    else
    {
        //
        //  Use the format string given by the caller.
        //
        pFormat = (LPWSTR)lpFormat;
    }

    //
    //  Get the current local system date if one is not given.
    //
    if (lpDate != NULL)
    {
        //
        //  Date is given by user.  Store in local structure and
        //  validate it.
        //
        LocalDate.wYear      = lpDate->wYear;
        LocalDate.wMonth     = lpDate->wMonth;
        LocalDate.wDayOfWeek = lpDate->wDayOfWeek;
        LocalDate.wDay       = lpDate->wDay;

        if (!IsValidDate(&LocalDate))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return (0);
        }
    }
    else
    {
        GetLocalTime(&LocalDate);
    }

    //
    //  See if we're dealing with the Hijri or the Hebrew calendar.
    //
    if (CalNum == CAL_HIJRI)
    {
        GetHijriDate(&LocalDate, dwFlags);
    }
    else if (CalNum == CAL_HEBREW)
    {
        if (!GetHebrewDate(&LocalDate, &fLunarLeap))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return (0);
        }
    }

    //
    //  Parse the date format string.
    //
    Length = ParseDate( pHashN,
                        dwFlags,
                        &LocalDate,
                        pFormat,
                        pString,
                        CalNum,
                        (PCALENDAR_VAR)pCalInfo,
                        fLunarLeap );

    //
    //  Check cchDate for size of given buffer.
    //
    if (cchDate == 0)
    {
        //
        //  If cchDate is 0, then we can't use lpDateStr.  In this
        //  case, we simply want to return the length (in characters) of
        //  the string to be copied.
        //
        return (Length);
    }
    else if (cchDate < Length)
    {
        //
        //  The buffer is too small for the string, so return an error
        //  and zero bytes written.
        //
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Copy the date string to lpDateStr and null terminate it.
    //  Return the number of characters copied.
    //
    wcsncpy(lpDateStr, pString, Length);
    return (Length);
}




//-------------------------------------------------------------------------//
//                           INTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  IsValidTime
//
//  Returns TRUE if the given time is valid.  Otherwise, it returns FALSE.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL IsValidTime(
    LPSYSTEMTIME pTime)

{
    //
    //  Check for invalid time values.
    //
    if ( (pTime->wHour > 23) ||
         (pTime->wMinute > 59) ||
         (pTime->wSecond > 59) ||
         (pTime->wMilliseconds > 999) )
    {
        return (FALSE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsValidDate
//
//  Returns TRUE if the given date is valid.  Otherwise, it returns FALSE.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL IsValidDate(
    LPSYSTEMTIME pDate)

{
    LARGE_INTEGER Time;           // time as a large integer
    TIME_FIELDS TimeFields;       // time fields structure


    //
    //  Set up time fields structure with the given date.
    //  Only want to check the DATE values, so pass in a valid time.
    //
    TimeFields.Year         = pDate->wYear;
    TimeFields.Month        = pDate->wMonth;
    TimeFields.Day          = pDate->wDay;
    TimeFields.Hour         = 0;
    TimeFields.Minute       = 0;
    TimeFields.Second       = 0;
    TimeFields.Milliseconds = 0;

    //
    //  Check for invalid date values.
    //
    //  NOTE:  This routine ignores the Weekday field.
    //
    if (!RtlTimeFieldsToTime(&TimeFields, &Time))
    {
        return (FALSE);
    }

    //
    //  Make sure the given day of the week is valid for the given date.
    //
    RtlTimeToTimeFields(&Time, &TimeFields);
    pDate->wDayOfWeek = TimeFields.Weekday;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetCalendarYear
//
//  Adjusts the given year to the given calendar's year.
//
//  10-15-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

WORD GetCalendarYear(
    LPWORD *ppRange,
    CALID CalNum,
    PCALENDAR_VAR pCalInfo,
    WORD Year,
    WORD Month,
    WORD Day)

{
    LPWORD pRange;                // ptr to range position
    LPWORD pEndRange;             // ptr to the end of the range


    //
    //  Initialize range pointer.
    //
    *ppRange = NULL;

    //
    //  Adjust the year based on the given calendar
    //
    switch (CalNum)
    {
        case ( 0 ) :
        case ( CAL_GREGORIAN ) :
        case ( CAL_GREGORIAN_US ) :
        default :
        {
            //
            //  Year value is not changed.
            //
            break;
        }
        case ( CAL_JAPAN ) :
        case ( CAL_TAIWAN ) :
        {
            //
            //  Get pointer to ranges.
            //
            pRange = ((LPWORD)pCalInfo) + pCalInfo->SEraRanges;
            pEndRange = ((LPWORD)pCalInfo) + pCalInfo->SShortDate;

            //
            //  Find the appropriate range.
            //
            while (pRange < pEndRange)
            {
                if ((Year > ((PERA_RANGE)pRange)->Year) ||
                    ((Year == ((PERA_RANGE)pRange)->Year) &&
                     ((Month > ((PERA_RANGE)pRange)->Month) ||
                      ((Month == ((PERA_RANGE)pRange)->Month) &&
                       (Day >= ((PERA_RANGE)pRange)->Day)))))
                {
                    break;
                }

                pRange += ((PERA_RANGE)pRange)->Offset;
            }

            //
            //  Make sure the year is within the given ranges.  If it
            //  is not, then leave the year in the Gregorian format.
            //
            if (pRange < pEndRange)
            {
                //
                //  Convert the year to the appropriate Era year.
                //     Year = Year - EraYear + 1
                //
                Year = Year - ((PERA_RANGE)pRange)->Year + 1;

                //
                //  Save the pointer to the range.
                //
                *ppRange = pRange;
            }

            break;
        }
        case ( CAL_KOREA ) :
        case ( CAL_THAI ) :
        {
            //
            //  Get the first range.
            //
            pRange = ((LPWORD)pCalInfo) + pCalInfo->SEraRanges;

            //
            //  Add the year offset to the given year.
            //     Year = Year + EraYear
            //
            Year += ((PERA_RANGE)pRange)->Year;

            //
            //  Save the range.
            //
            *ppRange = pRange;

            break;
        }
    }

    //
    //  Return the year.
    //
    return (Year);
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseTime
//
//  Parses the time format string and puts the properly formatted
//  local time into the given string buffer.  It returns the number of
//  characters written to the string buffer.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseTime(
    PLOC_HASH pHashN,
    LPSYSTEMTIME pLocalTime,
    LPWSTR pFormat,
    LPWSTR pTimeStr,
    DWORD dwFlags)

{
    LPWSTR pPos;                       // ptr to pTimeStr current position
    LPWSTR pLastPos;                   // ptr to pTimeStr last valid position
    LPWSTR pLastFormatPos;             // ptr to pFormat last parsed string
    int Repeat;                        // number of repetitions of same letter
    WORD wHour;                        // hour
    WCHAR wchar;                       // character in format string
    LPWSTR pAMPM;                      // ptr to AM/PM designator
    WCHAR pTemp[MAX_REG_VAL_SIZE];     // temp buffer
    BOOL bInQuote;                     // are we in a quoted string or not ?


    //
    //  Initialize position pointer.
    //
    pPos = pTimeStr;
    pLastPos = pPos;
    pLastFormatPos = pFormat;

    //
    //  Parse through loop and store the appropriate time information
    //  in the pTimeStr buffer.
    //
    while (*pFormat)
    {
        switch (*pFormat)
        {
            case ( L'h' ) :
            {
                //
                //  Check for forced 24 hour time format.
                //
                wHour = pLocalTime->wHour;
                if (!(dwFlags & TIME_FORCE24HOURFORMAT))
                {
                    //
                    //  Use 12 hour format.
                    //
                    if (!(wHour %= 12))
                    {
                        wHour = 12;
                    }
                }

                //
                //  Get the number of 'h' repetitions in the format string.
                //
                pFormat++;
                for (Repeat = 0; (*pFormat == L'h'); Repeat++, pFormat++)
                    ;

                switch (Repeat)
                {
                    case ( 0 ) :
                    {
                        //
                        //  Use NO leading zero for the hour.
                        //  The pPos pointer will be advanced in the macro.
                        //
                        NLS_PAD_INT_TO_UNICODE_STR( wHour,
                                                    10,
                                                    1,
                                                    pPos );

                        break;
                    }

                    case ( 1 ) :
                    default :
                    {
                        //
                        //  Use leading zero for the hour.
                        //  The pPos pointer will be advanced in the macro.
                        //
                        NLS_PAD_INT_TO_UNICODE_STR( wHour,
                                                    10,
                                                    2,
                                                    pPos );

                        break;
                    }
                }

                //
                //  Save the last position in case one of the NO_xxx
                //  flags is set.
                //
                pLastPos = pPos;
                pLastFormatPos = pFormat;

                break;
            }

            case ( L'H' ) :
            {
                //
                //  Get the number of 'H' repetitions in the format string.
                //
                pFormat++;
                for (Repeat = 0; (*pFormat == L'H'); Repeat++, pFormat++)
                    ;

                switch (Repeat)
                {
                    case ( 0 ) :
                    {
                        //
                        //  Use NO leading zero for the hour.
                        //  The pPos pointer will be advanced in the macro.
                        //
                        NLS_PAD_INT_TO_UNICODE_STR( pLocalTime->wHour,
                                                    10,
                                                    1,
                                                    pPos );

                        break;
                    }

                    case ( 1 ) :
                    default :
                    {
                        //
                        //  Use leading zero for the hour.
                        //  The pPos pointer will be advanced in the macro.
                        //
                        NLS_PAD_INT_TO_UNICODE_STR( pLocalTime->wHour,
                                                    10,
                                                    2,
                                                    pPos );

                        break;
                    }
                }

                //
                //  Save the last position in case one of the NO_xxx
                //  flags is set.
                //
                pLastPos = pPos;
                pLastFormatPos = pFormat;

                break;
            }

            case ( L'm' ) :
            {
                //
                //  Get the number of 'm' repetitions in the format string.
                //
                pFormat++;
                for (Repeat = 0; (*pFormat == L'm'); Repeat++, pFormat++)
                    ;

                //
                //  If the flag TIME_NOMINUTESORSECONDS is set, then
                //  skip over the minutes.
                //
                if (dwFlags & TIME_NOMINUTESORSECONDS)
                {
                    //
                    //  Reset position pointer to last postion and break
                    //  out of this case statement.
                    //
                    //  This will remove any separator(s) between the
                    //  hours and minutes.
                    //

                    //
                    // 1- Go backward and leave only quoted text
                    // 2- Go forward and remove everything till hitting {hHt}
                    //
                    bInQuote = FALSE;
                    while (pFormat != pLastFormatPos)
                    {
                        if (*pLastFormatPos == NLS_CHAR_QUOTE)
                        {
                            bInQuote = !bInQuote;
                            pLastFormatPos++;
                            continue;
                        }
                        if (bInQuote)
                        {
                            *pLastPos = *pLastFormatPos;
                            pLastPos++;
                        }
                        pLastFormatPos++;
                    }

                    bInQuote = FALSE;
                    while (*pFormat)
                    {
                        if (*pLastFormatPos == NLS_CHAR_QUOTE)
                        {
                            bInQuote = !bInQuote;
                        }

                        if (!bInQuote)
                        {
                            if (*pFormat == L' ')
                            {
                                *pLastPos++ = *pFormat;
                            }
                            else
                            {
                                if ((*pFormat == L'h') ||
                                    (*pFormat == L'H') ||
                                    (*pFormat == L't'))
                                {
                                    break;
                                }
                            }
                        }
                        pFormat++;
                    }

                    pPos = pLastPos;
                    break;
                }

                switch (Repeat)
                {
                    case ( 0 ) :
                    {
                        //
                        //  Use NO leading zero for the minute.
                        //  The pPos pointer will be advanced in the macro.
                        //
                        NLS_PAD_INT_TO_UNICODE_STR( pLocalTime->wMinute,
                                                    10,
                                                    1,
                                                    pPos );

                        break;
                    }

                    case ( 1 ) :
                    default :
                    {
                        //
                        //  Use leading zero for the minute.
                        //  The pPos pointer will be advanced in the macro.
                        //
                        NLS_PAD_INT_TO_UNICODE_STR( pLocalTime->wMinute,
                                                    10,
                                                    2,
                                                    pPos );

                        break;
                    }
                }

                //
                //  Save the last position in case one of the NO_xxx
                //  flags is set.
                //
                pLastPos = pPos;
                pLastFormatPos = pFormat;

                break;
            }

            case ( L's' ) :
            {
                //
                //  Get the number of 's' repetitions in the format string.
                //
                pFormat++;
                for (Repeat = 0; (*pFormat == L's'); Repeat++, pFormat++)
                    ;

                //
                //  If the flag TIME_NOMINUTESORSECONDS and/or TIME_NOSECONDS
                //  is set, then skip over the seconds.
                //
                if (dwFlags & (TIME_NOMINUTESORSECONDS | TIME_NOSECONDS))
                {
                    //
                    //  Reset position pointer to last postion and break
                    //  out of this case statement.
                    //
                    //  This will remove any separator(s) between the
                    //  minutes and seconds.
                    //

                    //
                    // 1- Go backward and leave only quoted text
                    // 2- Go forward and remove everything till hitting {hmHt}
                    //
                    bInQuote = FALSE;
                    while (pFormat != pLastFormatPos)
                    {
                        if (*pLastFormatPos == NLS_CHAR_QUOTE)
                        {
                            bInQuote = !bInQuote;
                            pLastFormatPos++;
                            continue;
                        }
                        if (bInQuote)
                        {
                            *pLastPos = *pLastFormatPos;
                            pLastPos++;
                        }
                        pLastFormatPos++;
                    }

                    bInQuote = FALSE;
                    while (*pFormat)
                    {
                        if (*pLastFormatPos == NLS_CHAR_QUOTE)
                        {
                            bInQuote = !bInQuote;
                        }

                        if (!bInQuote)
                        {
                            if (*pFormat == L' ')
                            {
                                *pLastPos++ = *pFormat;
                            }
                            else
                            {
                                if ((*pFormat == L'h') ||
                                    (*pFormat == L'H') ||
                                    (*pFormat == L't') ||
                                    (*pFormat == L'm'))
                                {
                                    break;
                                }
                            }
                        }
                        pFormat++;
                    }

                    pPos = pLastPos;
                    break;
                }

                switch (Repeat)
                {
                    case ( 0 ) :
                    {
                        //
                        //  Use NO leading zero for the second.
                        //  The pPos pointer will be advanced in the macro.
                        //
                        NLS_PAD_INT_TO_UNICODE_STR( pLocalTime->wSecond,
                                                    10,
                                                    1,
                                                    pPos );

                        break;
                    }

                    case ( 1 ) :
                    default :
                    {
                        //
                        //  Use leading zero for the second.
                        //  The pPos pointer will be advanced in the macro.
                        //
                        NLS_PAD_INT_TO_UNICODE_STR( pLocalTime->wSecond,
                                                    10,
                                                    2,
                                                    pPos );

                        break;
                    }
                }

                //
                //  Save the last position in case one of the NO_xxx
                //  flags is set.
                //
                pLastPos = pPos;
                pLastFormatPos = pFormat;

                break;
            }

            case ( L't' ) :
            {
                //
                //  Get the number of 't' repetitions in the format string.
                //
                pFormat++;
                for (Repeat = 0; (*pFormat == L't'); Repeat++, pFormat++)
                    ;

                //
                //  If the flag TIME_NOTIMEMARKER is set, then skip over
                //  the time marker info.
                //
                if (dwFlags & TIME_NOTIMEMARKER)
                {
                    //
                    //  Reset position pointer to last postion.
                    //
                    //  This will remove any separator(s) between the
                    //  time (hours, minutes, seconds) and the time
                    //  marker.
                    //
                    pPos = pLastPos;
                    pLastFormatPos = pFormat;

                    //
                    //  Increment the format pointer until it reaches
                    //  an h, H, m, or s.  This will remove any
                    //  separator(s) following the time marker.
                    //
                    while ( (wchar = *pFormat) &&
                            (wchar != L'h') &&
                            (wchar != L'H') &&
                            (wchar != L'm') &&
                            (wchar != L's') )
                    {
                        pFormat++;
                    }

                    //
                    //  Break out of this case statement.
                    //
                    break;
                }
                else
                {
                    //
                    //  Get AM/PM designator.
                    //  This string may be a null string.
                    //
                    if (pLocalTime->wHour < 12)
                    {
                        if (!(dwFlags & LOCALE_NOUSEROVERRIDE) &&
                            GetUserInfo( pHashN->Locale,
                                         LOCALE_S1159,
                                         pNlsUserInfo->s1159,
                                         NLS_VALUE_S1159,
                                         pTemp,
                                         FALSE ))
                        {
                            pAMPM = pTemp;
                        }
                        else
                        {
                            pAMPM = (LPWORD)(pHashN->pLocaleHdr) +
                                    pHashN->pLocaleHdr->S1159;
                        }
                    }
                    else
                    {
                        if (!(dwFlags & LOCALE_NOUSEROVERRIDE) &&
                            GetUserInfo( pHashN->Locale,
                                         LOCALE_S2359,
                                         pNlsUserInfo->s2359,
                                         NLS_VALUE_S2359,
                                         pTemp,
                                         FALSE ))
                        {
                            pAMPM = pTemp;
                        }
                        else
                        {
                            pAMPM = (LPWORD)(pHashN->pLocaleHdr) +
                                    pHashN->pLocaleHdr->S2359;
                        }
                    }

                    if (*pAMPM == 0)
                    {
                        //
                        //  Reset position pointer to last postion and break
                        //  out of this case statement.
                        //
                        //  This will remove any separator(s) between the
                        //  time (hours, minutes, seconds) and the time
                        //  marker.
                        //
                        pPos = pLastPos;
                        pLastFormatPos = pFormat;

                        break;
                    }
                }

                switch (Repeat)
                {
                    case ( 0 ) :
                    {
                        //
                        //  One letter of AM/PM designator.
                        //
                        *pPos = *pAMPM;
                        pPos++;

                        break;
                    }

                    case ( 1 ) :
                    default :
                    {
                        //
                        //  Use entire AM/PM designator string.
                        //  The pPos pointer will be advanced in the macro.                 \
                        //
                        NLS_COPY_UNICODE_STR(pPos, pAMPM);

                        break;
                    }
                }

                //
                //  Save the last position in case one of the NO_xxx
                //  flags is set.
                //
                pLastPos = pPos;
                pLastFormatPos = pFormat;

                break;
            }

            case ( NLS_CHAR_QUOTE ) :
            {
                //
                //  Any text enclosed within single quotes should be left
                //  in the time string in its exact form (without the
                //  quotes), unless it is an escaped single quote ('').
                //
                pFormat++;
                while (*pFormat)
                {
                    if (*pFormat != NLS_CHAR_QUOTE)
                    {
                        //
                        //  Still within the single quote, so copy
                        //  the character to the buffer.
                        //
                        *pPos = *pFormat;
                        pFormat++;
                        pPos++;
                    }
                    else
                    {
                        //
                        //  Found another quote, so skip over it.
                        //
                        pFormat++;

                        //
                        //  Make sure it's not an escaped single quote.
                        //
                        if (*pFormat == NLS_CHAR_QUOTE)
                        {
                            //
                            //  Escaped single quote, so just write the
                            //  single quote.
                            //
                            *pPos = *pFormat;
                            pFormat++;
                            pPos++;
                        }
                        else
                        {
                            //
                            //  Found the end quote, so break out of loop.
                            //
                            break;
                        }
                    }
                }

                break;
            }

            default :
            {
                //
                //  Store the character in the buffer.  Should be the
                //  separator, but copy it even if it isn't.
                //
                *pPos = *pFormat;
                pFormat++;
                pPos++;

                break;
            }
        }
    }

    //
    //  Zero terminate the string.
    //
    *pPos = 0;

    //
    //  Return the number of characters written to the buffer, including
    //  the null terminator.
    //
    return ((int)((pPos - pTimeStr) + 1));
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseDate
//
//  Parses the date format string and puts the properly formatted
//  local date into the given string buffer.  It returns the number of
//  characters written to the string buffer.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseDate(
    PLOC_HASH pHashN,
    DWORD dwFlags,
    LPSYSTEMTIME pLocalDate,
    LPWSTR pFormat,
    LPWSTR pDateStr,
    CALID CalNum,
    PCALENDAR_VAR pCalInfo,
    BOOL fLunarLeap)

{
    LPWSTR pPos;                  // ptr to pDateStr current position
    LPWSTR pTemp;                 // ptr to temp position in format string
    int Repeat;                   // number of repetitions of same letter
    LPWORD pIncr;                 // ptr to increment amount (day, month)
    WORD Incr;                    // increment amount
    BOOL fDayExists = FALSE;      // numeric day precedes or follows month
    WORD Year;                    // year value
    LPWORD pRange = NULL;         // ptr to era ranges
    LPWORD pInfo;                 // ptr to locale or calendar info
    LPWORD pInfoC;                // ptr to calendar info
    WCHAR szHebrew[10];           // buffer for Hebrew


    //
    //  Initialize position pointer.
    //
    pPos = pDateStr;

    //
    //  Parse through loop and store the appropriate date information
    //  in the pDateStr buffer.
    //
    while (*pFormat)
    {
        switch (*pFormat)
        {
            case ( L'd' ) :
            {
                //
                //  Insert the layout direction flag, if requested.
                //
                NLS_INSERT_BIDI_MARK(pPos, dwFlags);

                //
                //  Get the number of 'd' repetitions in the format string.
                //
                pFormat++;
                for (Repeat = 0; (*pFormat == L'd'); Repeat++, pFormat++)
                    ;

                switch (Repeat)
                {
                    case ( 0 ) :
                    case ( 1 ) :
                    {
                        //
                        //  Set flag for day preceding month.  The flag
                        //  will be used when the MMMM case follows the
                        //  d or dd case.
                        //
                        fDayExists = TRUE;

                        //
                        //  Special case the Hebrew calendar.
                        //
                        if (CalNum == CAL_HEBREW)
                        {
                            //
                            //  Convert Day number to Hebrew letter and
                            //  write it to the buffer.
                            //
                            NumberToHebrewLetter( pLocalDate->wDay,
                                                  szHebrew,
                                                  10 );
                            NLS_COPY_UNICODE_STR(pPos, szHebrew);
                            break;
                        }

                        //
                        //  Repeat Value:
                        //    0 : Use NO leading zero for the day of the month
                        //    1 : Use leading zero for the day of the month
                        //  The pPos pointer will be advanced in the macro.
                        //
                        NLS_PAD_INT_TO_UNICODE_STR( pLocalDate->wDay,
                                                    10,
                                                    (UINT)(Repeat + 1),
                                                    pPos );

                        break;
                    }

                    case ( 2 ) :
                    {
                        //
                        //  Set flag for day preceding month to be FALSE.
                        //
                        fDayExists = FALSE;

                        //
                        //  Get the abbreviated name for the day of the
                        //  week.
                        //  The pPos pointer will be advanced in the macro.
                        //
                        //  NOTE: LocalTime structure uses:
                        //           0 = Sun, 1 = Mon, etc.
                        //        Locale file uses:
                        //           SAbbrevDayName1 = Mon, etc.
                        //
                        if (pCalInfo->IfNames)
                        {
                            pInfo = (LPWORD)pCalInfo;
                            pIncr = &(pCalInfo->SAbbrevDayName1);
                        }
                        else
                        {
                            pInfo = (LPWORD)(pHashN->pLocaleHdr);
                            pIncr = &(pHashN->pLocaleHdr->SAbbrevDayName1);
                        }
                        pIncr += (((pLocalDate->wDayOfWeek) + 6) % 7);

                        //
                        //  Copy the abbreviated day name.
                        //
                        NLS_COPY_UNICODE_STR( pPos,
                                              ((LPWORD)(pInfo) + *pIncr) );

                        break;
                    }

                    case ( 3 ) :
                    default :
                    {
                        //
                        //  Set flag for day preceding month to be FALSE.
                        //
                        fDayExists = FALSE;

                        //
                        //  Get the full name for the day of the week.
                        //  The pPos pointer will be advanced in the macro.                 \
                        //
                        //  NOTE: LocalTime structure uses:
                        //           0 = Sunday, 1 = Monday, etc.
                        //        Locale file uses:
                        //           SAbbrevDayName1 = Monday, etc.
                        //
                        if (pCalInfo->IfNames)
                        {
                            pInfo = (LPWORD)pCalInfo;
                            pIncr = &(pCalInfo->SDayName1);
                        }
                        else
                        {
                            pInfo = (LPWORD)(pHashN->pLocaleHdr);
                            pIncr = &(pHashN->pLocaleHdr->SDayName1);
                        }
                        pIncr += (((pLocalDate->wDayOfWeek) + 6) % 7);

                        //
                        //  Copy the abbreviated day name.
                        //
                        NLS_COPY_UNICODE_STR( pPos,
                                              ((LPWORD)(pInfo) + *pIncr) );

                        break;
                    }
                }

                break;
            }

            case ( L'M' ) :
            {
                //
                //  Insert the layout direction flag, if requested.
                //
                NLS_INSERT_BIDI_MARK(pPos, dwFlags);

                //
                //  Get the number of 'M' repetitions in the format string.
                //
                pFormat++;
                for (Repeat = 0; (*pFormat == L'M'); Repeat++, pFormat++)
                    ;

                switch (Repeat)
                {
                    case ( 0 ) :
                    case ( 1 ) :
                    {
                        //
                        //  Special case the Hebrew calendar.
                        //
                        if (CalNum == CAL_HEBREW)
                        {
                            //
                            //  Convert Month number to Hebrew letter and
                            //  write it to the buffer.
                            //
                            NumberToHebrewLetter( pLocalDate->wMonth,
                                                  szHebrew,
                                                  10 );
                            NLS_COPY_UNICODE_STR(pPos, szHebrew);
                            break;
                        }

                        //
                        //  Repeat Value:
                        //    0 : Use NO leading zero for the month
                        //    1 : Use leading zero for the month
                        //  The pPos pointer will be advanced in the macro.
                        //
                        NLS_PAD_INT_TO_UNICODE_STR( pLocalDate->wMonth,
                                                    10,
                                                    (UINT)(Repeat + 1),
                                                    pPos );

                        break;
                    }

                    case ( 2 ) :
                    case ( 3 ) :
                    default :
                    {
                        //
                        //  Check for abbreviated or full month name.
                        //
                        if (Repeat == 2)
                        {
                            pInfoC = &(pCalInfo->SAbbrevMonthName1);
                            pInfo  = &(pHashN->pLocaleHdr->SAbbrevMonthName1);
                        }
                        else
                        {
                            pInfoC = &(pCalInfo->SMonthName1);
                            pInfo  = &(pHashN->pLocaleHdr->SMonthName1);
                        }

                        //
                        //  Get the abbreviated name of the month.
                        //  The pPos pointer will be advanced in the macro.
                        //
                        if (pCalInfo->IfNames)
                        {
                            if ((CalNum == CAL_HEBREW) &&
                                (!fLunarLeap) &&
                                (pLocalDate->wMonth > NLS_HEBREW_JUNE))
                            {
                                //
                                //  Go passed Addar_B.
                                //
                                pIncr = (pInfoC) +
                                        (pLocalDate->wMonth);
                            }
                            else
                            {
                                pIncr = (pInfoC) +
                                        (pLocalDate->wMonth - 1);
                            }

                            //
                            //  Copy the abbreviated month name.
                            //
                            NLS_COPY_UNICODE_STR( pPos,
                                           ((LPWORD)(pCalInfo) + *pIncr) );
                        }
                        else
                        {
                            pIncr = (pInfo) +
                                    (pLocalDate->wMonth - 1);

                            //
                            //  If we don't already have a numeric day
                            //  preceding the month name, then check for
                            //  a numeric day following the month name.
                            //
                            if (!fDayExists)
                            {
                                pTemp = pFormat;
                                while (*pTemp)
                                {
                                    if ((*pTemp == L'g') || (*pTemp == L'y'))
                                    {
                                        break;
                                    }
                                    if (*pTemp == L'd')
                                    {
                                        for (Repeat = 0;
                                             (*pTemp == L'd');
                                             Repeat++, pTemp++)
                                            ;
                                        if ((Repeat == 1) || (Repeat == 2))
                                        {
                                            fDayExists = TRUE;
                                        }
                                        break;
                                    }
                                    pTemp++;
                                }
                            }

                            //
                            //  Check for numeric day immediately preceding
                            //  or following the month name.
                            //
                            if (fDayExists)
                            {
                                Incr = *pIncr + 1 +
                                       NlsStrLenW(((LPWORD)(pHashN->pLocaleHdr) +
                                                  *pIncr));

                                if (Incr != *(pIncr + 1))
                                {
                                    //
                                    //  Copy the special month name -
                                    //  2nd one in list.
                                    //
                                    NLS_COPY_UNICODE_STR( pPos,
                                           ((LPWORD)(pHashN->pLocaleHdr) + Incr) );

                                    break;
                                }
                            }

                            //
                            //  Just copy the month name.
                            //
                            NLS_COPY_UNICODE_STR( pPos,
                                           ((LPWORD)(pHashN->pLocaleHdr) + *pIncr) );
                        }

                        break;
                    }
                }

                //
                //  Set flag for day preceding month to be FALSE.
                //
                fDayExists = FALSE;

                break;
            }

            case ( L'y' ) :
            {
                //
                //  Insert the layout direction flag, if requested.
                //
                NLS_INSERT_BIDI_MARK(pPos, dwFlags);

                //
                //  Get the number of 'y' repetitions in the format string.
                //
                pFormat++;
                for (Repeat = 0; (*pFormat == L'y'); Repeat++, pFormat++)
                    ;

                //
                //  Get proper year for calendar.
                //
                if (pCalInfo->NumRanges)
                {
                    if (!pRange)
                    {
                        //
                        //  Adjust the year for the given calendar.
                        //
                        Year = GetCalendarYear( &pRange,
                                                CalNum,
                                                pCalInfo,
                                                pLocalDate->wYear,
                                                pLocalDate->wMonth,
                                                pLocalDate->wDay );
                    }
                }
                else
                {
                    Year = pLocalDate->wYear;
                }

                //
                //  Special case the Hebrew calendar.
                //
                if (CalNum == CAL_HEBREW)
                {
                    //
                    //  Convert Year number to Hebrew letter and
                    //  write it to the buffer.
                    //
                    NumberToHebrewLetter(Year, szHebrew, 10);
                    NLS_COPY_UNICODE_STR(pPos, szHebrew);
                }
                else
                {
                    //
                    //  Write the year string to the buffer.
                    //
                    switch (Repeat)
                    {
                        case ( 0 ) :
                        case ( 1 ) :
                        {
                            //
                            //  1-digit century or 2-digit century.
                            //  The pPos pointer will be advanced in the macro.
                            //
                            NLS_PAD_INT_TO_UNICODE_STR( (Year % 100),
                                                        10,
                                                        (UINT)(Repeat + 1),
                                                        pPos );

                            break;
                        }

                        case ( 2 ) :
                        case ( 3 ) :
                        default :
                        {
                            //
                            //  Full century.
                            //  The pPos pointer will be advanced in the macro.
                            //
                            NLS_PAD_INT_TO_UNICODE_STR( Year,
                                                        10,
                                                        2,
                                                        pPos );

                            break;
                        }
                    }
                }

                //
                //  Set flag for day preceding month to be FALSE.
                //
                fDayExists = FALSE;

                break;
            }

            case ( L'g' ) :
            {
                //
                //  Insert the layout direction flag, if requested.
                //
                NLS_INSERT_BIDI_MARK(pPos, dwFlags);

                //
                //  Get the number of 'g' repetitions in the format string.
                //
                //  NOTE: It doesn't matter how many g repetitions
                //        there are.  They all mean 'gg'.
                //
                pFormat++;
                while (*pFormat == L'g')
                {
                    pFormat++;
                }

                //
                //  Copy the era string for the current calendar.
                //
                if (pCalInfo->NumRanges)
                {
                    //
                    //  Make sure we have the pointer to the
                    //  appropriate range.
                    //
                    if (!pRange)
                    {
                        //
                        //  Get the pointer to the correct range and
                        //  adjust the year for the given calendar.
                        //
                        Year = GetCalendarYear( &pRange,
                                                CalNum,
                                                pCalInfo,
                                                pLocalDate->wYear,
                                                pLocalDate->wMonth,
                                                pLocalDate->wDay );
                    }

                    //
                    //  Copy the era string to the buffer, if one exists.
                    //
                    if (pRange)
                    {
                        NLS_COPY_UNICODE_STR( pPos,
                             ((PERA_RANGE)pRange)->pYearStr +
                             NlsStrLenW(((PERA_RANGE)pRange)->pYearStr) + 1 );
                    }
                }

                //
                //  Set flag for day preceding month to be FALSE.
                //
                fDayExists = FALSE;

                break;
            }

            case ( NLS_CHAR_QUOTE ) :
            {
                //
                //  Insert the layout direction flag, if requested.
                //
                NLS_INSERT_BIDI_MARK(pPos, dwFlags);

                //
                //  Any text enclosed within single quotes should be left
                //  in the date string in its exact form (without the
                //  quotes), unless it is an escaped single quote ('').
                //
                pFormat++;
                while (*pFormat)
                {
                    if (*pFormat != NLS_CHAR_QUOTE)
                    {
                        //
                        //  Still within the single quote, so copy
                        //  the character to the buffer.
                        //
                        *pPos = *pFormat;
                        pFormat++;
                        pPos++;
                    }
                    else
                    {
                        //
                        //  Found another quote, so skip over it.
                        //
                        pFormat++;

                        //
                        //  Make sure it's not an escaped single quote.
                        //
                        if (*pFormat == NLS_CHAR_QUOTE)
                        {
                            //
                            //  Escaped single quote, so just write the
                            //  single quote.
                            //
                            *pPos = *pFormat;
                            pFormat++;
                            pPos++;
                        }
                        else
                        {
                            //
                            //  Found the end quote, so break out of loop.
                            //
                            break;
                        }
                    }
                }

                break;
            }

            default :
            {
                //
                //  Store the character in the buffer.  Should be the
                //  separator, but copy it even if it isn't.
                //
                *pPos = *pFormat;
                pFormat++;
                pPos++;

                break;
            }
        }
    }

    //
    //  Zero terminate the string.
    //
    *pPos = 0;

    //
    //  Return the number of characters written to the buffer, including
    //  the null terminator.
    //
    return ((int)((pPos - pDateStr) + 1));
}




//-------------------------------------------------------------------------//
//                     MIDDLE EAST CALENDAR ROUTINES                       //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  GetAbsoluteDate
//
//  Gets the Absolute date for the given Gregorian date.
//
//  Computes:
//      Number of Days in Prior Years (both common and leap years) +
//      Number of Days in Prior Months of Current Year +
//      Number of Days in Current Month
//
//  12-04-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

DWORD GetAbsoluteDate(
    WORD Year,
    WORD Month,
    WORD Day)

{
    DWORD AbsoluteDate = 0;            // absolute date
    DWORD GregMonthDays[13] = {0,31,59,90,120,151,181,212,243,273,304,334,365};


    //
    //  Check to see if the current year is a Gregorian leap year.
    //  If so, add a day.
    //
    if (NLS_GREGORIAN_LEAP_YEAR(Year) && (Month > 2))
    {
        AbsoluteDate++;
    }

    //
    //  Add the Number of Days in the Prior Years.
    //
    if (Year = Year - 1)
    {
        AbsoluteDate += ((Year * 365L) + (Year / 4L) - (Year / 100L) + (Year / 400L));
    }

    //
    //  Add the Number of Days in the Prior Months of the Current Year.
    //
    AbsoluteDate += GregMonthDays[Month - 1];

    //
    //  Add the Number of Days in the Current Month.
    //
    AbsoluteDate += (DWORD)Day;

    //
    //  Return the absolute date.
    //
    return (AbsoluteDate);
}




//-------------------------------------------------------------------------//
//                         HIJRI CALENDAR ROUTINES                         //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  GetHijriDate
//
//  Converts the given Gregorian date to its equivalent Hijri (Islamic)
//  date.
//
//  Rules for the Hijri calendar:
//    - The Hijri calendar is a strictly Lunar calendar.
//    - Days begin at sunset.
//    - Islamic Year 1 (Muharram 1, 1 A.H.) is equivalent to absolute date
//        227015 (Friday, July 16, 622 C.E. - Julian).
//    - Leap Years occur in the 2, 5, 7, 10, 13, 16, 18, 21, 24, 26, & 29th
//        years of a 30-year cycle.  Year = leap iff ((11y+14) mod 30 < 11).
//    - There are 12 months which contain alternately 30 and 29 days.
//    - The 12th month, Dhu al-Hijjah, contains 30 days instead of 29 days
//        in a leap year.
//    - Common years have 354 days.  Leap years have 355 days.
//    - There are 10,631 days in a 30-year cycle.
//    - The Islamic months are:
//        1.  Muharram   (30 days)     7.  Rajab          (30 days)
//        2.  Safar      (29 days)     8.  Sha'ban        (29 days)
//        3.  Rabi I     (30 days)     9.  Ramadan        (30 days)
//        4.  Rabi II    (29 days)     10. Shawwal        (29 days)
//        5.  Jumada I   (30 days)     11. Dhu al-Qada    (30 days)
//        6.  Jumada II  (29 days)     12. Dhu al-Hijjah  (29 days) {30}
//
//  12-04-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void GetHijriDate(
    LPSYSTEMTIME pDate,
    DWORD dwFlags)

{
    DWORD AbsoluteDate;                // absolute date
    DWORD HijriYear;                   // Hijri year
    DWORD HijriMonth;                  // Hijri month
    DWORD HijriDay;                    // Hijri day
    DWORD NumDays;                     // number of days
    DWORD HijriMonthDays[13] = {0,30,59,89,118,148,177,207,236,266,295,325,355};


    //
    //  Get the absolute date.
    //
    AbsoluteDate = GetAbsoluteDate(pDate->wYear, pDate->wMonth, pDate->wDay);

    //
    //  See how much we need to backup or advance
    //
    (LONG)AbsoluteDate += GetAdvanceHijriDate(dwFlags);

    //
    //  Calculate the Hijri Year.
    //
    HijriYear = ((AbsoluteDate - 227013L) * 30L / 10631L) + 1;

    if (AbsoluteDate <= DaysUpToHijriYear(HijriYear))
    {
        HijriYear--;
    }
    else if (AbsoluteDate > DaysUpToHijriYear(HijriYear + 1))
    {
        HijriYear++;
    }

    //
    //  Calculate the Hijri Month.
    //
    HijriMonth = 1;
    NumDays = AbsoluteDate - DaysUpToHijriYear(HijriYear);
    while ((HijriMonth <= 12) && (NumDays > HijriMonthDays[HijriMonth - 1]))
    {
        HijriMonth++;
    }
    HijriMonth--;

    //
    //  Calculate the Hijri Day.
    //
    HijriDay = NumDays - HijriMonthDays[HijriMonth - 1];

    //
    //  Save the Hijri date and return.
    //
    pDate->wYear  = (WORD)HijriYear;
    pDate->wMonth = (WORD)HijriMonth;
    pDate->wDay   = (WORD)HijriDay;
}


////////////////////////////////////////////////////////////////////////////
//
//  GetAdvanceHijriDate
//
//  Gets the AddHijriDate value from the registry.
//
//  12-04-96    JulieB    Created.
//  05-15-99    SamerA    Support +/-3 Advance Hijri Date
////////////////////////////////////////////////////////////////////////////

LONG GetAdvanceHijriDate(
    DWORD dwFlags)
{
    LONG lAdvance = 0L;                                 // advance hijri date
    HANDLE hKey = NULL;                                 // handle to intl key
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull;          // ptr to query info
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];               // ptr to static buffer
    BOOL IfAlloc = FALSE;                               // if buffer was allocated
    WCHAR wszAddHijriRegValue[] = L"AddHijriDate";      // registry value
    WCHAR wszAddHijriTempValue[] = L"AddHijriDateTemp"; // temp registry to use (intl.cpl use)
    INT AddHijriStringLength;
    PWSTR pwszValue;
    LONG lData;
    UNICODE_STRING ObUnicodeStr;
    ULONG rc = 0L;                                 // result code


    //
    //  Open the Control Panel International registry key.
    //
    OPEN_CPANEL_INTL_KEY(hKey, lAdvance, KEY_READ);

    //
    //  Query the registry for the AddHijriDate value.
    //
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
    rc = QueryRegValue( hKey,
                        (dwFlags & DATE_ADDHIJRIDATETEMP) ?
                        wszAddHijriTempValue :
                        wszAddHijriRegValue,
                        &pKeyValueFull,
                        MAX_KEY_VALUE_FULLINFO,
                        &IfAlloc );

    //
    //  Close the registry key.
    //
    CLOSE_REG_KEY(hKey);

    //
    // Get the base value length without the NULL terminating char.
    //
    AddHijriStringLength = (sizeof(wszAddHijriRegValue) / sizeof(WCHAR)) - 1;

    //
    //  See if the AddHijriDate value is present.
    //
    if (rc != NO_ERROR)
    {
        return (lAdvance);
    }

    //
    //  See if the AddHijriDate data is present, and
    //  if so parse the Advance Hijri amount
    //
    pwszValue = GET_VALUE_DATA_PTR(pKeyValueFull);

    if ((pKeyValueFull->DataLength > 2) &&
        (wcsncmp(pwszValue, wszAddHijriRegValue, AddHijriStringLength) == 0))
    {
        RtlInitUnicodeString( &ObUnicodeStr,
                              &pwszValue[AddHijriStringLength]);

        if (NT_SUCCESS(RtlUnicodeStringToInteger(&ObUnicodeStr,
                                                 10,
                                                 &lData)))
        {
            if ((lData > -3L) && (lData < 3L))
            {
                //
                // AddHijriDate and AddHijriDate-1 both means means -1
                //
                if (lData == 0L)
                {
                    lAdvance = -1L;
                }
                else
                {
                    lAdvance = lData;
                }
            }
        }
    }

    //
    //  Free the buffer used for the query.
    //
    if (IfAlloc)
    {
        NLS_FREE_MEM(pKeyValueFull);
    }

    //
    //  Return the result.
    //
    return (lAdvance);
}


////////////////////////////////////////////////////////////////////////////
//
//  DaysUpToHijriYear
//
//  Gets the total number of days (absolute date) up to the given Hijri
//  Year.
//
//  12-04-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

DWORD DaysUpToHijriYear(
    DWORD HijriYear)

{
    DWORD NumDays;           // number of absolute days
    DWORD NumYear30;         // number of years up to current 30 year cycle
    DWORD NumYearsLeft;      // number of years into 30 year cycle


    //
    //  Compute the number of years up to the current 30 year cycle.
    //
    NumYear30 = ((HijriYear - 1) / 30) * 30;

    //
    //  Compute the number of years left.  This is the number of years
    //  into the 30 year cycle for the given year.
    //
    NumYearsLeft = HijriYear - NumYear30 - 1;

    //
    //  Compute the number of absolute days up to the given year.
    //
    NumDays = ((NumYear30 * 10631L) / 30L) + 227013L;
    while (NumYearsLeft)
    {
        NumDays += 354L + NLS_HIJRI_LEAP_YEAR(NumYearsLeft);
        NumYearsLeft--;
    }

    //
    //  Return the number of absolute days.
    //
    return (NumDays);
}




//-------------------------------------------------------------------------//
//                         HEBREW CALENDAR ROUTINES                        //
//-------------------------------------------------------------------------//


//
//  Jewish Era in use today is dated from the supposed year of the
//  Creation with its beginning in 3761 B.C.
//
#define NLS_LUNAR_ERA_DIFF   3760


//
//  Hebrew Translation Table.
//
CONST BYTE HebrewTable[] =
{
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,7,3,17,3,
    0,4,11,2,21,6,1,3,13,2,
    25,4,5,3,16,2,27,6,9,1,
    20,2,0,6,11,3,23,4,4,2,
    14,3,27,4,8,2,18,3,28,6,
    11,1,22,5,2,3,12,3,25,4,
    6,2,16,3,26,6,8,2,20,1,
    0,6,11,2,24,4,4,3,15,2,
    25,6,8,1,19,2,29,6,9,3,
    22,4,3,2,13,3,25,4,6,3,
    17,2,27,6,7,3,19,2,31,4,
    11,3,23,4,5,2,15,3,25,6,
    6,2,19,1,29,6,10,2,22,4,
    3,3,14,2,24,6,6,1,17,3,
    28,5,8,3,20,1,32,5,12,3,
    22,6,4,1,16,2,26,6,6,3,
    17,2,0,4,10,3,22,4,3,2,
    14,3,24,6,5,2,17,1,28,6,
    9,2,19,3,31,4,13,2,23,6,
    3,3,15,1,27,5,7,3,17,3,
    29,4,11,2,21,6,3,1,14,2,
    25,6,5,3,16,2,28,4,9,3,
    20,2,0,6,12,1,23,6,4,2,
    14,3,26,4,8,2,18,3,0,4,
    10,3,21,5,1,3,13,1,24,5,
    5,3,15,3,27,4,8,2,19,3,
    29,6,10,2,22,4,3,3,14,2,
    26,4,6,3,18,2,28,6,10,1,
    20,6,2,2,12,3,24,4,5,2,
    16,3,28,4,8,3,19,2,0,6,
    12,1,23,5,3,3,14,3,26,4,
    7,2,17,3,28,6,9,2,21,4,
    1,3,13,2,25,4,5,3,16,2,
    27,6,9,1,19,3,0,5,11,3,
    23,4,4,2,14,3,25,6,7,1,
    18,2,28,6,9,3,21,4,2,2,
    12,3,25,4,6,2,16,3,26,6,
    8,2,20,1,0,6,11,2,22,6,
    4,1,15,2,25,6,6,3,18,1,
    29,5,9,3,22,4,2,3,13,2,
    23,6,4,3,15,2,27,4,7,3,
    19,2,31,4,11,3,21,6,3,2,
    15,1,25,6,6,2,17,3,29,4,
    10,2,20,6,3,1,13,3,24,5,
    4,3,16,1,27,5,7,3,17,3,
    0,4,11,2,21,6,1,3,13,2,
    25,4,5,3,16,2,29,4,9,3,
    19,6,30,2,13,1,23,6,4,2,
    14,3,27,4,8,2,18,3,0,4,
    11,3,22,5,2,3,14,1,26,5,
    6,3,16,3,28,4,10,2,20,6,
    30,3,11,2,24,4,4,3,15,2,
    25,6,8,1,19,2,29,6,9,3,
    22,4,3,2,13,3,25,4,7,2,
    17,3,27,6,9,1,21,5,1,3,
    11,3,23,4,5,2,15,3,25,6,
    6,2,19,1,29,6,10,2,22,4,
    3,3,14,2,24,6,6,1,18,2,
    28,6,8,3,20,4,2,2,12,3,
    24,4,4,3,16,2,26,6,6,3,
    17,2,0,4,10,3,22,4,3,2,
    14,3,24,6,5,2,17,1,28,6,
    9,2,21,4,1,3,13,2,23,6,
    5,1,15,3,27,5,7,3,19,1,
    0,5,10,3,22,4,2,3,13,2,
    24,6,4,3,15,2,27,4,8,3,
    20,4,1,2,11,3,22,6,3,2,
    15,1,25,6,7,2,17,3,29,4,
    10,2,21,6,1,3,13,1,24,5,
    5,3,15,3,27,4,8,2,19,6,
    1,1,12,2,22,6,3,3,14,2,
    26,4,6,3,18,2,28,6,10,1,
    20,6,2,2,12,3,24,4,5,2,
    16,3,28,4,9,2,19,6,30,3,
    12,1,23,5,3,3,14,3,26,4,
    7,2,17,3,28,6,9,2,21,4,
    1,3,13,2,25,4,5,3,16,2,
    27,6,9,1,19,6,30,2,11,3,
    23,4,4,2,14,3,27,4,7,3,
    18,2,28,6,11,1,22,5,2,3,
    12,3,25,4,6,2,16,3,26,6,
    8,2,20,4,30,3,11,2,24,4,
    4,3,15,2,25,6,8,1,18,3,
    29,5,9,3,22,4,3,2,13,3,
    23,6,6,1,17,2,27,6,7,3,
    20,4,1,2,11,3,23,4,5,2,
    15,3,25,6,6,2,19,1,29,6,
    10,2,20,6,3,1,14,2,24,6,
    4,3,17,1,28,5,8,3,20,4,
    1,3,12,2,22,6,2,3,14,2,
    26,4,6,3,17,2,0,4,10,3,
    20,6,1,2,14,1,24,6,5,2,
    15,3,28,4,9,2,19,6,1,1,
    12,3,23,5,3,3,15,1,27,5,
    7,3,17,3,29,4,11,2,21,6,
    1,3,12,2,25,4,5,3,16,2,
    28,4,9,3,19,6,30,2,12,1,
    23,6,4,2,14,3,26,4,8,2,
    18,3,0,4,10,3,22,5,2,3,
    14,1,25,5,6,3,16,3,28,4,
    9,2,20,6,30,3,11,2,23,4,
    4,3,15,2,27,4,7,3,19,2,
    29,6,11,1,21,6,3,2,13,3,
    25,4,6,2,17,3,27,6,9,1,
    20,5,30,3,10,3,22,4,3,2,
    14,3,24,6,5,2,17,1,28,6,
    9,2,21,4,1,3,13,2,23,6,
    5,1,16,2,27,6,7,3,19,4,
    30,2,11,3,23,4,3,3,14,2,
    25,6,5,3,16,2,28,4,9,3,
    21,4,2,2,12,3,23,6,4,2,
    16,1,26,6,8,2,20,4,30,3,
    11,2,22,6,4,1,14,3,25,5,
    6,3,18,1,29,5,9,3,22,4,
    2,3,13,2,23,6,4,3,15,2,
    27,4,7,3,20,4,1,2,11,3,
    21,6,3,2,15,1,25,6,6,2,
    17,3,29,4,10,2,20,6,3,1,
    13,3,24,5,4,3,17,1,28,5,
    8,3,18,6,1,1,12,2,22,6,
    2,3,14,2,26,4,6,3,17,2,
    28,6,10,1,20,6,1,2,12,3,
    24,4,5,2,15,3,28,4,9,2,
    19,6,33,3,12,1,23,5,3,3,
    13,3,25,4,6,2,16,3,26,6,
    8,2,20,4,30,3,11,2,24,4,
    4,3,15,2,25,6,8,1,18,6,
    33,2,9,3,22,4,3,2,13,3,
    25,4,6,3,17,2,27,6,9,1,
    21,5,1,3,11,3,23,4,5,2,
    15,3,25,6,6,2,19,4,33,3,
    10,2,22,4,3,3,14,2,24,6,
    6,1,99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,99,99,
    99,99
};


//
//  The lunar calendar has 6 different variations of month lengths
//  within a year.
//
CONST BYTE LunarMonthLen[7][14] =
{
    0,00,00,00,00,00,00,00,00,00,00,00,00,0,
    0,30,29,29,29,30,29,30,29,30,29,30,29,0,     // 3 common year variations
    0,30,29,30,29,30,29,30,29,30,29,30,29,0,
    0,30,30,30,29,30,29,30,29,30,29,30,29,0,
    0,30,29,29,29,30,30,29,30,29,30,29,30,29,    // 3 leap year variations
    0,30,29,30,29,30,30,29,30,29,30,29,30,29,
    0,30,30,30,29,30,30,29,30,29,30,29,30,29
};




////////////////////////////////////////////////////////////////////////////
//
//  GetHebrewDate
//
//  Converts the given Gregorian date to its equivalent Hebrew date.
//
//  Rules for the Hebrew calendar:
//    - The Hebrew calendar is both a Lunar (months) and Solar (years)
//        calendar, but allows for a week of seven days.
//    - Days begin at sunset.
//    - Leap Years occur in the 3, 6, 8, 11, 14, 17, & 19th years of a
//        19-year cycle.  Year = leap iff ((7y+1) mod 19 < 7).
//    - There are 12 months in a common year and 13 months in a leap year.
//    - In a common year, the 12th month, Adar, has 29 days.  In a leap
//        year, the 12th month, Adar I, has 30 days and the 13th month,
//        Adar II, has 29 days.
//    - Common years have 353-355 days.  Leap years have 383-385 days.
//    - The Hebrew new year (Rosh HaShanah) begins on the 1st of Tishri,
//        the 7th month in the list below.
//        - The new year may not begin on Sunday, Wednesday, or Friday.
//        - If the new year would fall on a Tuesday and the conjunction of
//            the following year were at midday or later, the new year is
//            delayed until Thursday.
//        - If the new year would fall on a Monday after a leap year, the
//            new year is delayed until Tuesday.
//    - The length of the 8th and 9th months vary from year to year,
//        depending on the overall length of the year.
//        - The length of a year is determined by the dates of the new
//            years (Tishri 1) preceding and following the year in question.
//        - The 8th month is long (30 days) if the year has 355 or 385 days.
//        - The 9th month is short (29 days) if the year has 353 or 383 days.
//    - The Hebrew months are:
//        1.  Nisan      (30 days)     7.  Tishri         (30 days)
//        2.  Iyyar      (29 days)     8.  Heshvan        (29 or 30 days)
//        3.  Sivan      (30 days)     9.  Kislev         (29 or 30 days)
//        4.  Tammuz     (29 days)     10. Teveth         (29 days)
//        5.  Av         (30 days)     11. Shevat         (30 days)
//        6.  Elul       (29 days)    {12. Adar I         (30 days)}
//                                     12. {13.} Adar {II}(29 days)
//
//  12-04-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL GetHebrewDate(
    LPSYSTEMTIME pDate,
    LPBOOL pLunarLeap)

{
    WORD Year, Month, Day;             // initial year, month, day
    WORD WeekDay;                      // day of the week
    BYTE LunarYearCode;                // lunar year code
    BYTE LunarMonth, LunarDay;         // lunar month and day for Jan 1
    DWORD Absolute1600;                // absolute date 1/1/1600
    DWORD AbsoluteDate;                // absolute date - absolute date 1/1/1600
    LONG NumDays;                      // number of days since 1/1
    CONST BYTE *pLunarMonthLen;        // ptr to lunar month length array


    //
    //  Save the Gregorian date values.
    //
    Year = pDate->wYear;
    Month = pDate->wMonth;
    Day = pDate->wDay;

    //
    //  Make sure we have a valid Gregorian date that will fit into our
    //  Hebrew conversion limits.
    //
    if (!IsValidDateForHebrew(Year, Month, Day))
    {
        return (FALSE);
    }

    //
    //  Get the offset into the LunarMonthLen array and the lunar day
    //  for January 1st.
    //
    LunarYearCode = HebrewTable[(Year - 1500) * 2 + 1];
    LunarDay      = HebrewTable[(Year - 1500) * 2];

    //
    //  See if it's a Lunar leap year.
    //
    *pLunarLeap = (LunarYearCode >= 4);

    //
    //  Get the Lunar Month.
    //
    switch (LunarDay)
    {
        case ( 0 ) :                   // 1/1 is on Shvat 1
        {
            LunarMonth = 5;
            LunarDay = 1;
            break;
        }
        case ( 30 ) :                  // 1/1 is on Kislev 30
        {
            LunarMonth = 3;
            break;
        }
        case ( 31 ) :                  // 1/1 is on Shvat 2
        {
            LunarMonth = 5;
            LunarDay = 2;
            break;
        }
        case ( 32 ) :                  // 1/1 is on Shvat 3
        {
            LunarMonth = 5;
            LunarDay = 3;
            break;
        }
        case ( 33 ) :                  // 1/1 is on Kislev 29
        {
            LunarMonth = 3;
            LunarDay = 29;
            break;
        }
        default :                      // 1/1 is on Tevet
        {
            LunarMonth = 4;
            break;
        }
    }

    //
    //  Store the values for the start of the new year - 1/1.
    //
    pDate->wYear  = Year + NLS_LUNAR_ERA_DIFF;
    pDate->wMonth = (WORD)LunarMonth;
    pDate->wDay   = (WORD)LunarDay;

    //
    //  Get the absolute date from 1/1/1600.
    //
    Absolute1600 = GetAbsoluteDate(1600, 1, 1);
    AbsoluteDate = GetAbsoluteDate(Year, Month, Day) - Absolute1600;

    //
    //  Compute and save the day of the week (Sunday = 0).
    //
    WeekDay = (WORD)(AbsoluteDate % 7);
    pDate->wDayOfWeek = (WeekDay) ? (WeekDay - 1) : 6;

    //
    //  If the requested date was 1/1, then we're done.
    //
    if ((Month == 1) && (Day == 1))
    {
        return (TRUE);
    }

    //
    //  Calculate the number of days between 1/1 and the requested date.
    //
    NumDays = (LONG)(AbsoluteDate - (GetAbsoluteDate(Year, 1, 1) - Absolute1600));

    //
    //  If the requested date is within the current lunar month, then
    //  we're done.
    //
    pLunarMonthLen = &(LunarMonthLen[LunarYearCode][0]);
    if ((NumDays + (LONG)LunarDay) <= (LONG)(pLunarMonthLen[LunarMonth]))
    {
        pDate->wDay += (WORD)NumDays;
        return (TRUE);
    }

    //
    //  Adjust for the current partial month.
    //
    pDate->wMonth++;
    pDate->wDay = 1;

    //
    //  Adjust the Lunar Month and Year (if necessary) based on the number
    //  of days between 1/1 and the requested date.
    //
    //  Assumes Jan 1 can never translate to the last Lunar month, which
    //  is true.
    //
    NumDays -= (LONG)(pLunarMonthLen[LunarMonth] - LunarDay);
    if (NumDays == 1)
    {
        return (TRUE);
    }

    //
    //  Get the final Hebrew date.
    //
    do
    {
        //
        //  See if we're on the correct Lunar month.
        //
        if (NumDays <= (LONG)(pLunarMonthLen[pDate->wMonth]))
        {
            //
            //  Found the right Lunar month.
            //
            pDate->wDay += (WORD)(NumDays - 1);
            return (TRUE);
        }
        else
        {
            //
            //  Adjust the number of days and move to the next month.
            //
            NumDays -= (LONG)(pLunarMonthLen[pDate->wMonth++]);

            //
            //  See if we need to adjust the Year.
            //  Must handle both 12 and 13 month years.
            //
            if ((pDate->wMonth > 13) || (pLunarMonthLen[pDate->wMonth] == 0))
            {
                //
                //  Adjust the Year.
                //
                pDate->wYear++;
                LunarYearCode = HebrewTable[(Year + 1 - 1500) * 2 + 1];
                pLunarMonthLen = &(LunarMonthLen[LunarYearCode][0]);

                //
                //  Adjust the Month.
                //
                pDate->wMonth = 1;

                //
                //  See if this new Lunar year is a leap year.
                //
                *pLunarLeap = (LunarYearCode >= 4);
            }
        }
    } while (NumDays > 0);

    //
    //  Return success.
    //
    return (TRUE);
}



////////////////////////////////////////////////////////////////////////////
//
//  IsValidDateForHebrew
//
//  Checks to be sure the given Gregorian date is valid.  This validation
//  requires that the year be between 1600 and 2239.  If it is, it
//  returns TRUE.  Otherwise, it returns FALSE.
//
//  12-04-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL IsValidDateForHebrew(
    WORD Year,
    WORD Month,
    WORD Day)

{
    WORD GregMonthLen[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};


    //
    //  Make sure the Year is between 1600 and 2239.
    //
    if ((Year < 1600) || (Year > 2239))
    {
        return (FALSE);
    }

    //
    //  Make sure the Month is between 1 and 12.
    //
    if ((Month < 1) || (Month > 12))
    {
        return (FALSE);
    }

    //
    //  See if it's a Gregorian leap year.  If so, make sure February
    //  is allowed to have 29 days.
    //
    if (NLS_GREGORIAN_LEAP_YEAR(Year))
    {
        GregMonthLen[2] = 29;
    }

    //
    //  Make sure the Day is within the correct range for the given Month.
    //
    if ((Day < 1) || (Day > GregMonthLen[Month]))
    {
        return (FALSE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  NumberToHebrewLetter
//
//  Converts the given number to Hebrew letters according to the numeric
//  value of each Hebrew letter.  Basically, this converts the lunar year
//  and the lunar month to letters.
//
//  The character of a year is described by three letters of the Hebrew
//  alphabet, the first and third giving, respectively, the days of the
//  weeks on which the New Year occurs and Passover begins, while the
//  second is the initial of the Hebrew word for defective, normal, or
//  complete.
//
//  Defective Year : Both Heshvan and Kislev are defective (353 or 383 days)
//  Normal Year    : Heshvan is defective, Kislev is full  (354 or 384 days)
//  Complete Year  : Both Heshvan and Kislev are full      (355 or 385 days)
//
//  12-04-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL NumberToHebrewLetter(
    DWORD Number,
    LPWSTR szHebrew,
    int cchSize)

{
    WCHAR szHundreds[4];               // temp buffer for hundreds
    WCHAR cTens, cUnits;               // tens and units chars
    DWORD Hundreds, Tens;              // hundreds and tens values
    WCHAR szTemp[10];                  // temp buffer
    LPWSTR pTemp = szTemp;             // temp ptr to temp buffer
    int Length, Ctr;                   // loop counters


    //
    //  Sanity check.
    //
    if (cchSize > 10)
    {
        return (FALSE);
    }

    //
    //  Adjust the number if greater than 5000.
    //
    if (Number > 5000)
    {
        Number -= 5000;
    }

    //
    //  Clear out the temp buffer.
    //
    RtlZeroMemory(szHundreds, 4 * sizeof(WCHAR));

    //
    //  Get the Hundreds.
    //
    Hundreds = Number / 100;

    if (Hundreds)
    {
        Number -= Hundreds * 100;

        if (Hundreds > 3)
        {
            szHundreds[2] = L'\x05ea';      // Hebrew Letter Tav
            Hundreds -= 4;
        }

        if (Hundreds > 3)
        {
            szHundreds[1] = L'\x05ea';      // Hebrew Letter Tav
            Hundreds -= 4;
        }

        if (Hundreds > 0)
        {
            if (!szHundreds[1])
            {
                szHundreds[1] = (WCHAR)(L'\x05e6' + Hundreds);
            }
            else
            {
                szHundreds[0] = (WCHAR)(L'\x05e6' + Hundreds);
            }
        }

        if (!szHundreds[1])
        {
            szHundreds[0] = szHundreds[2];
        }
        else
        {
            if (!szHundreds[0])
            {
                szHundreds[0] = szHundreds[1];
                szHundreds[1] = szHundreds[2];
                szHundreds[2] = 0;
            }
        }
    }

    //
    //  Get the Tens.
    //
    Tens = Number / 10;

    if (Tens)
    {
        Number -= Tens * 10;

        switch (Tens)
        {
            case ( 1 ) :
            {
                cTens = L'\x05d9';          // Hebrew Letter Yod
                break;
            }
            case ( 2 ) :
            {
                cTens = L'\x05db';          // Hebrew Letter Kaf
                break;
            }
            case ( 3 ) :
            {
                cTens = L'\x05dc';          // Hebrew Letter Lamed
                break;
            }
            case ( 4 ) :
            {
                cTens = L'\x05de';          // Hebrew Letter Mem
                break;
            }
            case ( 5 ) :
            {
                cTens = L'\x05e0';          // Hebrew Letter Nun
                break;
            }
            case ( 6 ) :
            {
                cTens = L'\x05e1';          // Hebrew Letter Samekh
                break;
            }
            case ( 7 ) :
            {
                cTens = L'\x05e2';          // Hebrew Letter Ayin
                break;
            }
            case ( 8 ) :
            {
                cTens = L'\x05e4';          // Hebrew Letter Pe
                break;
            }
            case ( 9 ) :
            {
                cTens = L'\x05e6';          // Hebrew Letter Tsadi
                break;
            }
        }
    }
    else
    {
        cTens = 0;
    }

    //
    //  Get the Units.
    //
    cUnits = (WCHAR)(Number ? (L'\x05d0' + Number - 1) : 0);

    if ((cUnits == L'\x05d4') &&            // Hebrew Letter He
        (cTens == L'\x05d9'))               // Hebrew Letter Yod
    {
        cUnits = L'\x05d5';                 // Hebrew Letter Vav
        cTens  = L'\x05d8';                 // Hebrew Letter Tet
    }

    if ((cUnits == L'\x05d5') &&            // Hebrew Letter Vav
        (cTens == L'\x05d9'))               // Hebrew Letter Yod
    {
        cUnits = L'\x05d6';                 // Hebrew Letter Zayin
        cTens  = L'\x05d8';                 // Hebrew Letter Tet
    }

    //
    //  Clear out the temp buffer.
    //
    RtlZeroMemory(pTemp, cchSize * sizeof(WCHAR));

    //
    //  Copy the appropriate info to the given buffer.
    //
    if (cUnits)
    {
        *pTemp++ = cUnits;
    }

    if (cTens)
    {
        *pTemp++ = cTens;
    }

    NlsStrCpyW(pTemp, szHundreds);

    if (NlsStrLenW(szTemp) > 1)
    {
        RtlMoveMemory(szTemp + 2, szTemp + 1, NlsStrLenW(szTemp + 1) * sizeof(WCHAR));
        szTemp[1] = L'"';
    }
    else
    {
        szTemp[1] = szTemp[0];
        szTemp[0] = L'\'';
    }

    //
    //  Reverse the final string and store it in the given buffer.
    //
    Length = NlsStrLenW(szTemp) - 1;
    for (Ctr = 0; Length >= 0; Ctr++)
    {
        szHebrew[Ctr] = szTemp[Length];
        Length--;
    }
    szHebrew[Ctr] = 0;

    //
    //  Return success.
    //
    return (TRUE);
}

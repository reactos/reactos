/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    httptime.cxx

Abstract:

    This file contains routines to get various timestamps from an http response
    header.

    We handle only the three standards-mandated date forms, since these are used by
    the vast majority of sites out there on the WWW. Handling additional date forms
    adds to the overhead of these functions, so unless a new form makes headway, we
    will keep these functions simple.

    Contents:
        FGetHttpExpiryTime
        FGetHttpLastModifiedTime
        FParseHttpDate
        FHttpDateTimeToFiletime
        FFileTimetoHttpDateTime
        HttpDateToSystemTime
        HttpTimeFromSystemTime
        (FInternalParseHttpDate)
        (MapDayMonthToDword)

Author:

    Shishir Pardikar (shishirp) 06-Jan-1996

Revision History:

    06-Jan-1996 rfirth
        Created this header

    12-Dec-1997 arthurbi
        Rewrote the date parser to reduce allocs, and other bad stuff.

--*/

#include <wininetp.h>
#include "httpp.h"
#include "httptime.h"

//
// external prototypes
//

/********************* Local data *******************************************/
/******************** HTTP date format strings ******************************/

// Month
static const char cszJan[]="Jan";
static const char cszFeb[]="Feb";
static const char cszMar[]="Mar";
static const char cszApr[]="Apr";
static const char cszMay[]="May";
static const char cszJun[]="Jun";
static const char cszJul[]="Jul";
static const char cszAug[]="Aug";
static const char cszSep[]="Sep";
static const char cszOct[]="Oct";
static const char cszNov[]="Nov";
static const char cszDec[]="Dec";

// DayOfWeek in rfc1123 or asctime format
static const char cszSun[]="Sun";
static const char cszMon[]="Mon";
static const char cszTue[]="Tue";
static const char cszWed[]="Wed";
static const char cszThu[]="Thu";
static const char cszFri[]="Fri";
static const char cszSat[]="Sat";

// List of weekdays for rfc1123 or asctime style date
static const char *rgszWkDay[7] =
   {
        cszSun,cszMon,cszTue,cszWed,cszThu,cszFri,cszSat
   };

// list of month strings for all date formats
static const char *rgszMon[12] =
   {
        cszJan,cszFeb,cszMar,cszApr,cszMay,cszJun,
        cszJul,cszAug,cszSep,cszOct,cszNov,cszDec
   };

/******************** HTTP date format strings ******************************/

/* Http date format: Sat, 29 Oct 1994 19:43:00 GMT */
const char cszHttpDateFmt[]="%s, %02i %s %02i %02i:%02i:%02i GMT";

/****************************************************************************/


/******************************** Local Functions ***************************/

BOOL
FHttpDateTimeToFiletime(
    LPCSTR pcszStr,      // input datetime string
    LPCSTR *rgszWkDay,   // day of week strings
    LPCSTR *rgszMon,     // month strings
    LPCSTR pcszSep,      // seperators
    UINT dateId,         // date format
    FILETIME *lpft       // output filetime in GMT
    );


BOOL 
FInternalParseHttpDate(
    OUT FILETIME *lpft,
    OUT SYSTEMTIME *lpSysTime,
    IN  LPCSTR lpInputBuffer
    );


/****************************************************************************/






//+---------------------------------------------------------------------------
//
//  Function: FGetHttpExpiryTime
//
//  Synopsis:
//
//  Arguments:
//
//
//  Returns: TRUE if successful. lpft contains the datetime in FILETIME format
//
//
//  Notes:
//
//
//----------------------------------------------------------------------------

BOOL FGetHttpExpiryTime(HINTERNET hRequest, FILETIME *lpFt)
   {
        BOOL fRet=FALSE;
   char buff[256];
   DWORD dwBuffLen;

   dwBuffLen = sizeof(buff);
   if (HttpQueryInfo(hRequest, HTTP_QUERY_EXPIRES, buff, &dwBuffLen, NULL))
      {
      fRet = FParseHttpDate(lpFt, buff);
      }

        return fRet;
   }

//+---------------------------------------------------------------------------
//
//  Function: FGetHttpLastModifiedTime
//
//  Synopsis:
//
//  Arguments:
//
//
//  Returns: TRUE if successful. lpft contains the datetime in FILETIME format
//
//
//  Notes:
//
//
//----------------------------------------------------------------------------

BOOL FGetHttpLastModifiedTime(HINTERNET hRequest, FILETIME *lpFt)
   {
        BOOL fRet=FALSE;
   char buff[256];
   DWORD dwBuffLen;

   dwBuffLen = sizeof(buff);
   if (HttpQueryInfo(hRequest, HTTP_QUERY_LAST_MODIFIED, buff, &dwBuffLen, NULL))
      {
      fRet = FParseHttpDate(lpFt, buff);
      }

        return fRet;
   }

DWORD
inline
MapDayMonthToDword(
    LPCSTR lpszDay
    )
/*++

Routine Description:

    Looks at the first three bytes of string to determine if we're looking
        at a Day of the Week, or Month, or "GMT" string.  Is inlined so that
        the compiler can optimize this code into the caller FInternalParseHttpDate.

Arguments:

    lpszDay - a string ptr to the first byte of the string in question.

Return Value:

    DWORD
    Success - The Correct date token, 0-6 for day of the week, 1-14 for month, etc

    Failure - DATE_TOKEN_ERROR

--*/

{
    switch ( MAKE_UPPER(*lpszDay) ) // make uppercase
    {
        case 'A':
            switch ( MAKE_UPPER(*(lpszDay+1)) )
            {
                case 'P':
                    return DATE_TOKEN_APRIL;
                case 'U':
                    return DATE_TOKEN_AUGUST;

            }
            return DATE_TOKEN_ERROR;

        case 'D':
            return DATE_TOKEN_DECEMBER;

        case 'F':
            switch ( MAKE_UPPER(*(lpszDay+1)) )
            {
                case 'R':
                    return DATE_TOKEN_FRIDAY;
                case 'E':
                    return DATE_TOKEN_FEBRUARY;
            }

            return DATE_TOKEN_ERROR;

        case 'G':
            return DATE_TOKEN_GMT;

        case 'M':

            switch ( MAKE_UPPER(*(lpszDay+1)) )
            {
                case 'O':
                    return DATE_TOKEN_MONDAY;
                case 'A':
                    switch (MAKE_UPPER(*(lpszDay+2)) )
                    {
                        case 'R':
                            return DATE_TOKEN_MARCH;
                        case 'Y':
                            return DATE_TOKEN_MAY;
                    }

                    // fall through to error
            }

            return DATE_TOKEN_ERROR;            

        case 'N':
            return DATE_TOKEN_NOVEMBER;

        case 'J':

            switch (MAKE_UPPER(*(lpszDay+1)) )
            {
                case 'A':
                    return DATE_TOKEN_JANUARY;

                case 'U':
                    switch (MAKE_UPPER(*(lpszDay+2)) )
                    {
                        case 'N':
                            return DATE_TOKEN_JUNE;
                        case 'L':
                            return DATE_TOKEN_JULY;
                    }

                    // fall through to error
            }

            return DATE_TOKEN_ERROR;

        case 'O':
            return DATE_TOKEN_OCTOBER;

        case 'S':
            
            switch (MAKE_UPPER(*(lpszDay+1)) )
            {
                case 'A':
                    return DATE_TOKEN_SATURDAY;
                case 'U':
                    return DATE_TOKEN_SUNDAY;
                case 'E':
                    return DATE_TOKEN_SEPTEMBER;
            }

            return DATE_TOKEN_ERROR;


        case 'T':
            switch (MAKE_UPPER(*(lpszDay+1)) )
            {    
                case 'U':
                    return DATE_TOKEN_TUESDAY;
                case 'H':
                    return DATE_TOKEN_THURSDAY;
            }

            return DATE_TOKEN_ERROR;

        case 'U':
            return DATE_TOKEN_GMT;
            
        case 'W':
            return DATE_TOKEN_WEDNESDAY;

    }

    return DATE_TOKEN_ERROR;
}

BOOL 
FInternalParseHttpDate(
    OUT FILETIME *lpft,
    OUT SYSTEMTIME *lpSysTime,
    IN  LPCSTR lpInputBuffer
    )
/*++

Routine Description:

    Parses through a ANSI, RFC850, or RFC1123 date format and covents it into
     a FILETIME/SYSTEMTIME time format.  

    Important this a time-critical function and should only be changed 
     with the intention of optimizing or a critical need work item.

Arguments:

    lpft - Ptr to FILETIME structure.  Used to store converted result.
            Must be NULL if not intended to be used !!!

    lpSysTime - Ptr to SYSTEMTIME struture. Used to return Systime if needed.

    lpcszDateStr - Const Date string to parse.

Return Value:

    BOOL
    Success - TRUE

    Failure - FALSE

--*/

{
    int i = 0, iLastLettered = -1;
    BOOL fIsANSIDateFormat = FALSE;
    DWORD rgdwDateParseResults[MAX_DATE_ENTRIES];
    SYSTEMTIME  sSysTime;
    FILETIME    ftTime;
    BOOL fRet = TRUE;

    DEBUG_ENTER((DBG_HTTP,
                Bool,
                "FInternalParseHttpDate",
                "%x %.10q",
                lpft,
                lpInputBuffer
                ));

    //
    // Date Parsing v2 (1 more to go), and here is how it works... 
    //  We take a date string and churn through it once, converting
    //  integers to integers, Month,Day, and GMT strings into integers,
    //  and all is then placed IN order in a temp array. 
    //
    // At the completetion of the parse stage, we simple look at 
    //  the data, and then map the results into the correct 
    //  places in the SYSTIME structure.  Simple, No allocations, and
    //  No dirting the data.   
    //
    // The end of the function does something munging and pretting
    //  up of the results to handle the year 2000, and TZ offsets
    //  Note: do we need to fully handle TZs anymore?
    //

    while ( *lpInputBuffer && i < MAX_DATE_ENTRIES)
    {
        if ( *lpInputBuffer >= '0' && *lpInputBuffer <= '9' )
        {
            //
            // we have a numerical entry, scan through it and convent to DWORD
            //

            rgdwDateParseResults[i] = 0;

            do {
                rgdwDateParseResults[i] *= BASE_DEC;
                rgdwDateParseResults[i] += (DWORD) (*lpInputBuffer - '0');
                lpInputBuffer++;
            } while ( *lpInputBuffer && *lpInputBuffer >= '0' && *lpInputBuffer <= '9' );

            i++; // next token
        }
        else if ( (*lpInputBuffer >= 'A' && *lpInputBuffer <= 'Z') ||
             (*lpInputBuffer >= 'a' && *lpInputBuffer <= 'z') )
        {
            //
            // we have a string, should be a day, month, or GMT
            //   lets skim to the end of the string
            //
            
            rgdwDateParseResults[i] = 
                MapDayMonthToDword(lpInputBuffer);

            iLastLettered = i;

            // We want to ignore the possibility of a time zone such as PST or EST in a non-standard
            // date format such as "Thu Dec 17 16:01:28 PST 1998" (Notice that the year is _after_ the time zone
            if ((rgdwDateParseResults[i] == DATE_TOKEN_ERROR) 
                && 
                !(fIsANSIDateFormat && (i==DATE_ANSI_INDEX_YEAR)))
            {
                fRet = FALSE;
#ifdef DEBUG
                dprintf("FInternalParseHttpDate: Invalid Date Format, could not parse %s\n", lpInputBuffer);
#endif
                
                goto quit;
            }

            //
            // At this point if we have a vaild string
            //  at this index, we know for sure that we're
            //  looking at a ANSI type DATE format.
            //

            if ( i == DATE_ANSI_INDEX_MONTH )
            {
                fIsANSIDateFormat = TRUE;
            }

            //
            // Read past the end of the current set of alpha characters,
            //  as MapDayMonthToDword only peeks at a few characters
            //

            do {
                lpInputBuffer++;
            } while ( *lpInputBuffer && 
                        ( (*lpInputBuffer >= 'A' && *lpInputBuffer <= 'Z') ||
                          (*lpInputBuffer >= 'a' && *lpInputBuffer <= 'z') ) );

            i++; // next token
        }
        else
        {
            //
            // For the generic case its either a space, comma, semi-colon, etc.
            //  the point is we really don't care, nor do we need to waste time
            //  worring about it (the orginal code did).   The point is we 
            //  care about the actual date information, So we just advance to the 
            //  next lexume.
            //

            lpInputBuffer++;        
        }
    }

    //
    // We're finished parsing the string, now take the parsed tokens
    //  and turn them to the actual structured information we care about.
    //  So we build lpSysTime from the Array, using a local if none is passed in.
    //

    if ( lpSysTime == NULL )
    {
        lpSysTime = &sSysTime;
    }

    lpSysTime->wDayOfWeek    = (WORD)rgdwDateParseResults[DATE_INDEX_DAY_OF_WEEK];
    lpSysTime->wMilliseconds =  0;

    if ( fIsANSIDateFormat )
    {
        lpSysTime->wDay    = (WORD)rgdwDateParseResults[DATE_ANSI_INDEX_DAY];
        lpSysTime->wMonth  = (WORD)rgdwDateParseResults[DATE_ANSI_INDEX_MONTH];
        lpSysTime->wHour   = (WORD)rgdwDateParseResults[DATE_ANSI_INDEX_HRS];
        lpSysTime->wMinute = (WORD)rgdwDateParseResults[DATE_ANSI_INDEX_MINS];
        lpSysTime->wSecond = (WORD)rgdwDateParseResults[DATE_ANSI_INDEX_SECS];
        if (iLastLettered != DATE_ANSI_INDEX_YEAR)
        {
            lpSysTime->wYear   = (WORD)rgdwDateParseResults[DATE_ANSI_INDEX_YEAR];
        }
        else
        {
            // Warning! This is a hack to get around the toString/toGMTstring fiasco (where the timezone is
            // appended at the end. (See above)
            lpSysTime->wYear   = (WORD)rgdwDateParseResults[DATE_INDEX_TZ];
         }
    }
    else
    {
        lpSysTime->wDay    = (WORD)rgdwDateParseResults[DATE_1123_INDEX_DAY];
        lpSysTime->wMonth  = (WORD)rgdwDateParseResults[DATE_1123_INDEX_MONTH];
        lpSysTime->wYear   = (WORD)rgdwDateParseResults[DATE_1123_INDEX_YEAR];
        lpSysTime->wHour   = (WORD)rgdwDateParseResults[DATE_1123_INDEX_HRS];
        lpSysTime->wMinute = (WORD)rgdwDateParseResults[DATE_1123_INDEX_MINS];
        lpSysTime->wSecond = (WORD)rgdwDateParseResults[DATE_1123_INDEX_SECS];
    }

    //
    // Normalize the year, 90 == 1990, handle the year 2000, 02 == 2002
    //  This is Year 2000 handling folks!!!  We get this wrong and 
    //  we all look bad. 
    //

    if (lpSysTime->wYear < 100) {
        lpSysTime->wYear += ((lpSysTime->wYear < 80) ? 2000 : 1900);
    }

    //
    // if we got misformed time, then plug in the current time
    // !lpszHrs || !lpszMins || !lpszSec
    //

    if ( i < 4) 
    {
        SYSTEMTIME  sCurSysTime;

        // this is a bad date; logging.
        DEBUG_PRINT(HTTP,
                    INFO,
                    ("*** Received a malformed date: %s\n", lpInputBuffer
                    ));

        GetSystemTime(&sCurSysTime);

        if ( i < 2 )
        {
            //
            // If we really screwed up the parsing, then
            //  just use the current time. 
            //

            *lpSysTime = sCurSysTime;
        }
        else
        {
            lpSysTime->wHour = sCurSysTime.wHour;
            lpSysTime->wMinute = sCurSysTime.wMinute;
            lpSysTime->wSecond = sCurSysTime.wSecond;
        }
    }


    if ((lpSysTime->wDay > 31)
    || (lpSysTime->wHour > 23)
    || (lpSysTime->wMinute > 59)
    || (lpSysTime->wSecond > 59)) 
    {
        fRet = FALSE;
        DEBUG_PRINT(HTTP,
                    INFO,
                    ("*** Received a malformed date: %s\n", lpInputBuffer
                    ));
        goto quit;
    }

    // Hack: we want the system time to be accurate. This is _suhlow_
    // The time passed in is in the local time zone; we have to convert this into GMT.
    
    if (iLastLettered==DATE_ANSI_INDEX_YEAR)
    {
        i--;
        
        FILETIME ft1, ft2;

        fRet = 
            SystemTimeToFileTime(lpSysTime, &ft1);

        if (fRet)
        {
            fRet = LocalFileTimeToFileTime(&ft1, &ft2);
            if (fRet)
            {
                fRet = FileTimeToSystemTime(&ft2, lpSysTime);
            }

        }
        
        if (!fRet)
        {
            DEBUG_PRINT(HTTP,
                    INFO,
                    ("*** Received a malformed date: %s\n", lpInputBuffer
                    ));
            goto quit;
        }
    }


    //
    // If FILETIME Ptr passed in/or we have an Offset to another Time Zone
    //   then convert to FILETIME for necessity/convenience
    //

    if ( lpft ||
         (i > DATE_INDEX_TZ &&
          rgdwDateParseResults[DATE_INDEX_TZ] != DATE_TOKEN_GMT))
    {

        if ( lpft == NULL )
        {
            lpft = &ftTime;
        }

        fRet = 
            SystemTimeToFileTime(lpSysTime, lpft);

        if ( ! fRet )
        {
           DEBUG_PRINT(HTTP,
                    INFO,
                    ("*** Received a malformed date: %s\n", lpInputBuffer
                    ));
           goto quit;
        }

        if (i > DATE_INDEX_TZ &&
            rgdwDateParseResults[DATE_INDEX_TZ] != DATE_TOKEN_GMT) 
        {
            // time zones are a very expensive operation, I want to know if this is a common case.
            DEBUG_PRINT(HTTP,
                        INFO,
                        ("*** Received a time zone: %d\n", (int) rgdwDateParseResults[DATE_INDEX_TZ]
                        ));

            //
            // if we received +/-nnnn as offset (hhmm), modify the output FILETIME
            //

            LONGLONG delta;
            BOOL negative;
            int offset;

            offset = (int) rgdwDateParseResults[DATE_INDEX_TZ];

            //
            // BUGBUG - some sites return +0000 instead of GMT. Presumably, this is
            //          an offset from GMT (== 0). What are the units? What are the
            //          boundaries (-12 hours to +12 hours? In seconds? (43200
            //          seconds in 12 hours, so can't be this)
            //

            //
            // BUGBUG - must handle negatives...and (-1 == GMT)
            //

            if (offset < 0) {
                negative = TRUE;
                offset = -offset;
            } else {
                negative = FALSE;
            }

            //
            // hours and minutes as 100nSec intervals
            //

            delta = (((offset / 100) * 60)
                    + (offset % 100)) * 60 * 10000000;
            if (negative) {
                delta = -delta;
            }
            AddLongLongToFT(lpft,delta);

            //
            // Chk to see if we Need to turn the offseted 
            //   FILETIME back into SYSTEMTIME.
            //

            if ( lpSysTime == &sSysTime )
            {
                fRet = FileTimeToSystemTime(lpft, lpSysTime);
            }
        }
    }

quit:

    DEBUG_LEAVE(fRet);

    return fRet;
}

PUBLIC 
BOOL 
FParseHttpDate(
    OUT FILETIME *lpft,
    IN  LPCSTR lpInputBuffer
    )

/*++

Routine Description:

    Parses through a ANSI, RFC850, or RFC1123 date format and covents it into
     a FILETIME time format.  

Arguments:

    lpft - Ptr to FILETIME structure.  Used to store converted result.

    lpcszDateStr - Const Date string to parse.

Return Value:

    BOOL
    Success - TRUE

    Failure - FALSE

--*/

{
    return FInternalParseHttpDate(
                lpft,                
                NULL, // SYSTEMTIME
                lpInputBuffer
                );
}



//+---------------------------------------------------------------------------
//
//  Function: FFileTimetoHttpDateTime
//
//  Synopsis:
//
//  Arguments:
//
//
//  Returns: TRUE if successful. lpft contains the datetime in FILETIME format
//
//
//  Notes:
//
//
//----------------------------------------------------------------------------
BOOL FFileTimetoHttpDateTime(
    FILETIME *lpft,       // output filetime in GMT
    LPSTR   lpszBuff,
    LPDWORD lpdwSize
    )
{
    SYSTEMTIME  sSysTime;

    INET_ASSERT (*lpdwSize >= HTTP_DATE_SIZE);

    if (FileTimeToSystemTime(lpft, &sSysTime)) {
        *lpdwSize = wsprintf(lpszBuff, cszHttpDateFmt
                , rgszWkDay[sSysTime.wDayOfWeek]
                , sSysTime.wDay
                , rgszMon[sSysTime.wMonth-1]
                , sSysTime.wYear
                , sSysTime.wHour
                , sSysTime.wMinute
                , sSysTime.wSecond);
        return (TRUE);
    }
    return (FALSE);
}


BOOL
HttpDateToSystemTime(
    IN LPSTR lpszHttpDate,
    OUT LPSYSTEMTIME lpSystemTime
    )

/*++

Routine Description:

    Takes a HTTP time/date string of the format "Sat, 6 Jan 1996 21:22:04 GMT"
    and converts it to a SYSTEMTIME structure

Arguments:

    lpszHttpDate    - pointer to time string to convert

    lpSystemTime    - pointer to converted time

Return Value:

    BOOL
        TRUE    - string converted

        FALSE   - couldn't convert string

--*/

{
    return FInternalParseHttpDate(
                NULL, // FILETIME               
                lpSystemTime, 
                (LPCSTR)lpszHttpDate
                );
}


INTERNETAPI
BOOL
WINAPI
InternetTimeFromSystemTimeA(
    IN  CONST SYSTEMTIME *pst,  // input GMT time
    IN  DWORD dwRFC,            // RFC format: must be FORMAT_RFC1123
    OUT LPSTR lpszTime,         // output string buffer
    IN  DWORD cbTime            // output buffer size
    )
/*++

Routine Description:

    Converts system time to a time string fromatted in the specified RFC format


Arguments:

    pst:    points to the SYSTEMTIME to be converted

    dwRFC:  RFC number of the format in which the result string should be returned

    lpszTime: buffer to return the string in

    cbTime: size of lpszTime buffer

Return Value:

    BOOL
        TRUE    - string converted

        FALSE   - couldn't convert string, GetLastError returns windows error code

--*/
{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "InternetTimeFromSystemTimeA",
                     "%#x, %d, %#x, %d",
                     pst,
                     dwRFC,
                     lpszTime,
                     cbTime
                     ));

    DWORD dwErr;
    BOOL fResult = FALSE;
    
    if (   dwRFC != INTERNET_RFC1123_FORMAT
        || IsBadReadPtr (pst, sizeof(*pst))
        || IsBadWritePtr (lpszTime, cbTime)
       )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    } 
    else if (cbTime < INTERNET_RFC1123_BUFSIZE)
    {
        dwErr = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        wsprintf (lpszTime, cszHttpDateFmt,
            rgszWkDay[pst->wDayOfWeek],
            pst->wDay,
            rgszMon[pst->wMonth-1],
            pst->wYear,
            pst->wHour,
            pst->wMinute,
            pst->wSecond);
        fResult = TRUE;
    }

    if (!fResult)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

INTERNETAPI
BOOL
WINAPI
InternetTimeToSystemTimeA(
    IN  LPCSTR lpcszTimeString,
    OUT SYSTEMTIME *lpSysTime,
    IN  DWORD dwReserved
)
/*++

Routine Description:

    API. Takes a HTTP time/date string of the formats that we deal with
    and converts it to a SYSTEMTIME structure

Arguments:

    lpcszTimeString     - pointer to a null terminated date/time string to convert

    lpSysTime           - pointer to converted time

    dwreserved          - Reserved

Return Value:

    BOOL
        TRUE    - string converted

        FALSE   - couldn't convert string, GetLastError returns windows error code

--*/
{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "InternetTimeFromSystemTimeA",
                     "%q, %#x, %#x",
                     lpcszTimeString,
                     lpSysTime,
                     dwReserved
                     ));
    BOOL fRet = FALSE;;
    DWORD dwErr;

    if (IsBadWritePtr (lpSysTime, sizeof(*lpSysTime)) ||
        IsBadStringPtr(lpcszTimeString, 0xffff))
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        fRet = FInternalParseHttpDate(NULL, lpSysTime, (LPCSTR)lpcszTimeString);
        if (!fRet)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }

    if (!fRet)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fRet);
    return (fRet);
}

INTERNETAPI
BOOL
WINAPI
InternetTimeFromSystemTimeW(
    IN  CONST SYSTEMTIME *pst,  // input GMT time
    IN  DWORD dwRFC,            // RFC format: must be FORMAT_RFC1123
    OUT LPWSTR lpszTime,         // output string buffer
    IN  DWORD cbTime            // output buffer size
    )
/*++

Routine Description:

    Converts system time to a time string fromatted in the specified RFC format


Arguments:

    pst:    points to the SYSTEMTIME to be converted

    dwRFC:  RFC number of the format in which the result string should be returned

    lpszTime: buffer to return the string in

    cbTime: size of lpszTime buffer in bytes

Return Value:

    BOOL
        TRUE    - string converted

        FALSE   - couldn't convert string, GetLastError returns windows error code

--*/
{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "InternetTimeFromSystemTimeW",
                     "%#x, %d, %#x, %d",
                     pst,
                     dwRFC,
                     lpszTime,
                     cbTime
                     ));

    DWORD dwErr = ERROR_SUCCESS;;
    BOOL fResult = FALSE;
    MEMORYPACKET mpTime;
    DWORD ccSize;
    
    if (!(cbTime >= INTERNET_RFC1123_BUFSIZE*sizeof(WCHAR)))
    {
        dwErr = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }
    if (!lpszTime)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    mpTime.psStr = (LPSTR)ALLOC_BYTES(cbTime);
    if (!mpTime.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    mpTime.dwAlloc = cbTime;
    
    fResult = InternetTimeFromSystemTimeA(pst, dwRFC, mpTime.psStr, cbTime);

    if (fResult)
    {
        ccSize = MultiByteToWideChar(CP_ACP, 0, mpTime.psStr, -1, NULL, 0);
        if (cbTime<=ccSize*sizeof(WCHAR))
        {
            fResult = FALSE;
            dwErr = ERROR_INSUFFICIENT_BUFFER;;
        }
        else
        {
            MultiByteToWideChar(CP_ACP, 0, mpTime.psStr, -1, lpszTime, cbTime/sizeof(WCHAR));
        }
    }

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

INTERNETAPI
BOOL
WINAPI
InternetTimeToSystemTimeW(
    IN  LPCWSTR lpcszTimeString,
    OUT SYSTEMTIME *lpSysTime,
    IN  DWORD dwReserved
)
/*++

Routine Description:

    API. Takes a HTTP time/date string of the formats that we deal with
    and converts it to a SYSTEMTIME structure

Arguments:

    lpcszTimeString     - pointer to a null terminated date/time string to convert

    lpSysTime           - pointer to converted time

    dwreserved          - Reserved

Return Value:

    BOOL
        TRUE    - string converted

        FALSE   - couldn't convert string, GetLastError returns windows error code

--*/
{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "InternetTimeFromSystemTimeW",
                     "%wq, %#x, %#x",
                     lpcszTimeString,
                     lpSysTime,
                     dwReserved
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpTime;

    if (!lpcszTimeString)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(lpcszTimeString, 0, mpTime);
    if (!mpTime.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(lpcszTimeString, mpTime);
    fResult = InternetTimeToSystemTimeA(mpTime.psStr, lpSysTime, dwReserved);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}



//
// Time functions
//
//


#include "priv.h"
#include "w95wraps.h"
#include "ids.h"

#include <mluisupp.h>

#ifndef DATE_LTRREADING
#define DATE_LTRREADING           0x00000010    //BUGBUG 
#define DATE_RTLREADING           0x00000020    //BUGBUG 
#endif

/*-------------------------------------------------------------------------
Purpose: Calls GetDateFormat and tries to replace the day with 
         a relative reference like "Today" or "Yesterday".

         Returns the count of characters written to pszBuf.
*/
int GetRelativeDateFormat(
    DWORD dwDateFlags, 
    DWORD * pdwFlags, 
    SYSTEMTIME * pstDate, 
    LPWSTR pszBuf, 
    int cchBuf)
{
    int cch;

    ASSERT(pdwFlags);
    ASSERT(pstDate);
    ASSERT(pszBuf);

    // Assume that no relative date is applied, so clear the bit
    // for now.
    *pdwFlags &= ~FDTF_RELATIVE;
    
    // Get the Win32 date format.  (GetDateFormat's return value includes
    // the null terminator.)
    cch = GetDateFormat(LOCALE_USER_DEFAULT, dwDateFlags, pstDate, NULL, pszBuf, cchBuf);
    if (0 < cch)
    {
        SYSTEMTIME stCurrentTime;
        int iDay = 0;   // 1 = today, -1 = yesterday, 0 = neither today nor yesterday.

        // Now see if the date merits a replacement to "Yesterday" or "Today".
        
        GetLocalTime(&stCurrentTime);      // get the current date 

        // Does it match the current day?
        if (pstDate->wYear == stCurrentTime.wYear   && 
            pstDate->wMonth == stCurrentTime.wMonth &&
            pstDate->wDay == stCurrentTime.wDay)
        {
            // Yes
            iDay = 1;
        }
        else 
        {
            // No; maybe it matches yesterday    
            FILETIME ftYesterday;
            SYSTEMTIME stYesterday;

            // Compute yesterday's date by converting to FILETIME,
            // subtracting one day, then converting back.
            SystemTimeToFileTime(&stCurrentTime, &ftYesterday);
            FILETIMEtoInt64(ftYesterday) -= FT_ONEDAY;
            FileTimeToSystemTime(&ftYesterday, &stYesterday);

            // Does it match yesterday?
            if (pstDate->wYear == stYesterday.wYear   && 
                pstDate->wMonth == stYesterday.wMonth &&
                pstDate->wDay == stYesterday.wDay)
            {
                // Yes
                iDay = -1;
            }
        }

        // Should we try replacing the day?
        if (0 != iDay)
        {
            // Yes
            TCHAR szDayOfWeek[32];
            LPTSTR pszModifier;
            int cchDayOfWeek;

            cchDayOfWeek = MLLoadString((IDS_DAYSOFTHEWEEK + pstDate->wDayOfWeek), 
                                      szDayOfWeek, SIZECHARS(szDayOfWeek));

            // Search for the day of week text in the string we got back.
            // Depending on the user's regional settings, there might not
            // be a day in the long-date format...
            
            pszModifier = StrStr(pszBuf, szDayOfWeek);

            if (pszModifier)
            {
                // We found the day in the string, so replace it with
                // "Today" or "Yesterday"
                TCHAR szTemp[64];
                TCHAR szRelativeDay[32];
                int cchRelativeDay;

                // Save the tail end (the part after the "Monday" string) 
                lstrcpyn(szTemp, &pszModifier[cchDayOfWeek], SIZECHARS(szTemp));
                
                // Load the appropriate string ("Yesterday" or "Today").
                // If the string is empty (localizers might need to do this
                // if this logic isn't locale-friendly), don't bother doing
                // anything.
                cchRelativeDay = MLLoadString((1 == iDay) ? IDS_TODAY : IDS_YESTERDAY, 
                                            szRelativeDay, SIZECHARS(szRelativeDay));
                if (0 < cchRelativeDay)
                {
                    // Make sure that we have enough room for the replacement
                    // (cch already accounts for the null terminator)
                    if (cch - cchDayOfWeek + cchRelativeDay <= cchBuf)
                    {
                        // copy the friendly name over the day of the week
                        lstrcpy(pszModifier, szRelativeDay);

                        // put back the tail end
                        lstrcat(pszModifier, szTemp);
                        cch = cch - cchDayOfWeek + cchRelativeDay;

                        *pdwFlags |= FDTF_RELATIVE;
                    }
                }
            }
        }
    }

    return cch;
}

#define LRM 0x200E // UNICODE Left-to-right mark control character
#define RLM 0x200F // UNICODE Left-to-right mark control character

/*-------------------------------------------------------------------------
Purpose: Constructs a displayname form of the file time.

         *pdwFlags may be NULL, in which case FDTF_DEFAULT is assumed.  Other
         valid flags are:

            FDTF_DEFAULT    "3/29/98 7:48 PM"
            FDTF_SHORTTIME  "7:48 PM"
            FDTF_SHORTDATE  "3/29/98"
            FDTF_LONGDATE   "Monday, March 29, 1998"
            FDTF_LONGTIME   "7:48:33 PM"
            FDTF_RELATIVE   only works with FDTF_LONGDATE.  If possible, 
                            replace the day with "Yesterday" or "Today":
                            "Yesterday, March 29, 1998"

         This function updates *pdwFlags to indicate which sections of the
         string were actually set.  For example, if FDTF_RELATIVE is passed
         in, but no relative date conversion was performed, then FDTF_RELATIVE
         is cleared before returning.

         If the date is the magic "Sorry, I don't know what date it is" value
         that FAT uses, then we return an empty string.

*/
STDAPI_(int) SHFormatDateTimeW(const FILETIME * pft, DWORD * pdwFlags, LPWSTR pszBuf, UINT cchBuf)
{
    int cchBufSav = cchBuf;
    DWORD dwFlags;
    FILETIME ft;

    ASSERT(IS_VALID_READ_PTR(pft, FILETIME));
    ASSERT(IS_VALID_WRITE_BUFFER(pszBuf, WCHAR, cchBuf));
    ASSERT(NULL == pdwFlags || IS_VALID_WRITE_PTR(pdwFlags, DWORD));

    FileTimeToLocalFileTime(pft, &ft);

    if (FILETIMEtoInt64(*pft) == FT_NTFS_UNKNOWNGMT ||
        FILETIMEtoInt64(  ft) == FT_FAT_UNKNOWNLOCAL)
    {
        // This date is uninitialized.  Don't show a bogus "10/10/72" string.
        if (0 < cchBuf)
            *pszBuf = 0;
        dwFlags = 0;
    }
    else if (0 < cchBuf)
    {
        int cch;
        SYSTEMTIME st;
        DWORD dwDateFlags = DATE_SHORTDATE;     // default
        DWORD dwTimeFlags = TIME_NOSECONDS;     // default

        if (pdwFlags)
            dwFlags = *pdwFlags;
        else
            dwFlags = FDTF_DEFAULT;
            
        // Initialize the flags we're going to use
        if (dwFlags & FDTF_LONGDATE)
            dwDateFlags = DATE_LONGDATE;
        else
            dwFlags &= ~FDTF_RELATIVE;      // can't show relative dates w/o long dates

        if (dwFlags & FDTF_LTRDATE)
            dwDateFlags |= DATE_LTRREADING;
        else if(dwFlags & FDTF_RTLDATE)
            dwDateFlags |= DATE_RTLREADING;

        if (dwFlags & FDTF_LONGTIME)
            dwTimeFlags &= ~TIME_NOSECONDS;
            
        FileTimeToSystemTime(&ft, &st);

        cchBuf--;       // Account for null terminator first
        
        if (dwFlags & (FDTF_LONGDATE | FDTF_SHORTDATE))
        {
            // Get the date
            if (dwFlags & FDTF_RELATIVE)
                cch = GetRelativeDateFormat(dwDateFlags, &dwFlags, &st, pszBuf, cchBuf);
            else
                cch = GetDateFormat(LOCALE_USER_DEFAULT, dwDateFlags, &st, NULL, pszBuf, cchBuf);

            if (0 < cch)
                cch--;      // (null terminator was counted above, so don't count it again)
            else
                dwFlags &= ~(FDTF_LONGDATE | FDTF_SHORTDATE);   // no date, so clear these bits
            cchBuf -= cch;
            pszBuf += cch;

            // Are we tacking on the time too?
            if (dwFlags & (FDTF_SHORTTIME | FDTF_LONGTIME))
            {
                // Yes; for long dates, separate with a comma, otherwise
                // separate with a space.
                if (dwFlags & FDTF_LONGDATE)
                {
                    WCHAR szT[8];
                    
                    cch = MLLoadString(IDS_LONGDATE_SEP, szT, SIZECHARS(szT));
                    lstrcatn(pszBuf, szT, cchBuf);
                    cchBuf -= cch;
                    pszBuf += cch;
                }
                else
                {
                    *pszBuf++ = TEXT(' ');
                    *pszBuf = 0;          // (in case GetTimeFormat doesn't add anything)
                    cchBuf--;
                }
                // [msadek]; need to insert strong a Unicode control character to simulate
                // a strong run in the opposite base direction to enforce
                // correct display of concatinated string in all cases
                if(dwFlags & FDTF_RTLDATE)
                {
                    *pszBuf++ = LRM; // simulate an opposite run
                    *pszBuf++ = RLM; // force RTL display of the time part.
                    *pszBuf = 0;
                    cchBuf -= 2;
                }
                else if(dwFlags & FDTF_LTRDATE)
                {
                    *pszBuf++ = RLM; // simulate an opposite run
                    *pszBuf++ = LRM; // force LTR display of the time part.
                    *pszBuf = 0;
                    cchBuf -= 2;                    
                }
            }
        }

        if (dwFlags & (FDTF_SHORTTIME | FDTF_LONGTIME))
        {
            // Get the time
            cch = GetTimeFormat(LOCALE_USER_DEFAULT, dwTimeFlags, &st, NULL, pszBuf, cchBuf);
            if (0 < cch)
                cch--;      // (null terminator was counted above, so don't count it again)
            else
                dwFlags &= ~(FDTF_LONGTIME | FDTF_SHORTTIME);   // no time, so clear these bits
            cchBuf -= cch;
        }
    }

    if (pdwFlags)
        *pdwFlags = dwFlags;
        
    return cchBufSav - cchBuf;
}


/*-------------------------------------------------------------------------
Purpose: See the defn for SHFormatDateTimeW.
*/
STDAPI_(int) SHFormatDateTimeA(const FILETIME * pft, DWORD * pdwFlags, LPSTR pszBuf, UINT cchBuf)
{
    int cchRet;
    WCHAR wsz[256];

    cchRet = SHFormatDateTimeW(pft, pdwFlags, wsz, SIZECHARS(wsz));
    if (0 < cchRet)
    {
        cchRet = SHUnicodeToAnsi(wsz, pszBuf, cchBuf);
    }
    else if (0 < cchBuf)
        *pszBuf = 0;
        
    return cchRet;
}


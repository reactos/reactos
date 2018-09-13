#include "ctlspriv.h"
#include "scdttime.h"

// BUGBUG? remove references to 1750 and 1752 as they
// are not needed -- the minimal year we allow is 1753
// to avoid such problems. (We don't care about
// pre-revised-Gregorian dates! If you want to develop
// a history application, then you can deal with these problems)

int mpcdymoAccum[13] =
{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

/*
 -    LIncrWord
 -
 *    Purpose:
 *        Increment (or decrement) an integer by a specified amount,
 *        given the constraints nMic and nMac.
 *        Returns the amount of carry into the following (or preceding)
 *        field, or zero if none.
 *
 *        Intended for use with incrementing date/times.
 *
 *    Arguments:
 *        pn        Pointer to integer to be modified.
 *        nDelta    Amount by which to modify *pn; may be positive,
 *                negative or zero.
 *        nMic    Minimum value for *pn;  if decrementing below this,
 *                a carry is performed.
 *        nMac    Maximum value for *pn;  if incrementing above this,
 *                a carry is performed.
 *
 *    Returns:
 *        Zero if modification done within constraints, otherwise the
 *        amount of carry (positive in incrementing, negative if
 *        decrementing).
 *
 */
LONG LIncrWord(WORD *pn, LONG nDelta, int nMic, int nMac)
    {
    LONG lNew, lIncr;

    lIncr = 0;
    lNew = *pn + nDelta;

    while (lNew >= nMac)
        {
        lNew -= nMac - nMic;
        lIncr++;
        }

    if (!lIncr)
        {
        while (lNew < nMic)
            {
            lNew += nMac - nMic;
            lIncr--;
            }
        }

    *pn = (WORD)lNew;

    return(lIncr);
    }

void IncrSystemTime(SYSTEMTIME *pstSrc, SYSTEMTIME *pstDest, LONG nDelta, LONG flag)
    {
    int cdyMon;

    if (pstSrc != pstDest)
        *pstDest = *pstSrc;

    switch (flag)
        {
        case INCRSYS_SECOND:
            if (!(nDelta = LIncrWord(&pstDest->wSecond, nDelta, 0, 60)))
                break;

        case INCRSYS_MINUTE:
            if (!(nDelta = LIncrWord(&pstDest->wMinute, nDelta, 0, 60)))
                break;

        case INCRSYS_HOUR:
            if (!(nDelta = LIncrWord(&pstDest->wHour, nDelta, 0, 24)))
                break;

        case INCRSYS_DAY:
IDTday:
            if (nDelta >= 0)
                {
                cdyMon = GetDaysForMonth(pstDest->wYear, pstDest->wMonth);
                while (pstDest->wDay + nDelta > cdyMon)
                    {
                    nDelta -= cdyMon + 1 - pstDest->wDay;
                    pstDest->wDay = 1;
                    IncrSystemTime(pstDest, pstDest, 1, INCRSYS_MONTH);
                    cdyMon = GetDaysForMonth(pstDest->wYear, pstDest->wMonth);
                    }
                }
            else
                {
                while (pstDest->wDay <= -nDelta)
                    {
                    nDelta += pstDest->wDay;
                    IncrSystemTime(pstDest, pstDest, -1, INCRSYS_MONTH);
                    cdyMon = GetDaysForMonth(pstDest->wYear, pstDest->wMonth);
                    pstDest->wDay = (WORD) cdyMon;
                    }
                }

            pstDest->wDay += (WORD)nDelta;
            break;

        case INCRSYS_MONTH:
            if (!(nDelta = LIncrWord(&pstDest->wMonth, nDelta, 1, 13)))
                {
                cdyMon = GetDaysForMonth(pstDest->wYear, pstDest->wMonth);
                if (pstDest->wDay > cdyMon)
                    pstDest->wDay = (WORD) cdyMon;
                break;
                }

        case INCRSYS_YEAR:
            pstDest->wYear += (WORD)nDelta;
            cdyMon = GetDaysForMonth(pstDest->wYear, pstDest->wMonth);
            if (pstDest->wDay > cdyMon)
                pstDest->wDay = (WORD) cdyMon;
            break;

        case INCRSYS_WEEK:
            nDelta *= 7;
            goto IDTday;
            break;
        }
    }

CmpDate(const SYSTEMTIME *pst1, const SYSTEMTIME *pst2)
    {
    int iRet;

    if (pst1->wYear < pst2->wYear)
        iRet = -1;
    else if (pst1->wYear > pst2->wYear)
        iRet = 1;
    else if (pst1->wMonth < pst2->wMonth)
        iRet = -1;
    else if (pst1->wMonth > pst2->wMonth)
        iRet = 1;
    else if (pst1->wDay < pst2->wDay)
        iRet = -1;
    else if (pst1->wDay > pst2->wDay)
        iRet = 1;
    else
        iRet = 0;

    return(iRet);
    }

CmpSystemtime(const SYSTEMTIME *pst1, const SYSTEMTIME *pst2)
    {
    int iRet;

    if (pst1->wYear < pst2->wYear)
        iRet = -1;
    else if (pst1->wYear > pst2->wYear)
        iRet = 1;
    else if (pst1->wMonth < pst2->wMonth)
        iRet = -1;
    else if (pst1->wMonth > pst2->wMonth)
        iRet = 1;
    else if (pst1->wDay < pst2->wDay)
        iRet = -1;
    else if (pst1->wDay > pst2->wDay)
        iRet = 1;
    else if (pst1->wHour < pst2->wHour)
        iRet = -1;
    else if (pst1->wHour > pst2->wHour)
        iRet = 1;
    else if (pst1->wMinute < pst2->wMinute)
        iRet = -1;
    else if (pst1->wMinute > pst2->wMinute)
        iRet = 1;
    else if (pst1->wSecond < pst2->wSecond)
        iRet = -1;
    else if (pst1->wSecond > pst2->wSecond)
        iRet = 1;
    else
        iRet = 0;

    return(iRet);
    }

/*
 -    CdyBetweenYmd
 -
 *    Purpose:
 *        Calculate the number of days between two dates as expressed
 *        in YMD's.
 *
 *    Parameters:
 *        pymdStart        start day of range.
 *        pymdEnd            end day of range.
 *
 *    Returns:
 *        Number of days between two dates.  The number
 *        of days does not include the starting day, but does include
 *        the last day. ie 1/24/1990-1/25/1990 = 1 day.
 */
DWORD DaysBetweenDates(const SYSTEMTIME *pstStart, const SYSTEMTIME *pstEnd)
    {
    DWORD cday;
    WORD yr;

    // Calculate number of days between the start month/day and the
    // end month/day as if they were in the same year - since cday
    // is unsigned, cday could be really large if the end month/day
    // is before the start month.day.
    // This will be cleared up when we account for the days between
    // the years.
    ASSERT(pstEnd->wMonth >= 1 && pstEnd->wMonth <= 12);
    cday = mpcdymoAccum[pstEnd->wMonth - 1] - mpcdymoAccum[pstStart->wMonth - 1] +
             pstEnd->wDay - pstStart->wDay;
    yr = pstStart->wYear;

    // Check to see if the start year is before the end year,
    // and if the end month is after February and
    // if the end year is a leap year, then add an extra day
    // for to account for Feb. 29 in the end year.
    if ( ((yr < pstEnd->wYear) || (pstStart->wMonth <= 2)) &&
         pstEnd->wMonth > 2 &&
        (pstEnd->wYear & 03) == 0 &&
        (pstEnd->wYear <= 1750 || pstEnd->wYear % 100 != 0 || pstEnd->wYear % 400 == 0))
        {
        cday++;
        }

    // Now account for the leap years in between the start and end dates
    // as well as accounting for the days in each year.
    if (yr < pstEnd->wYear)
        {
        // If the start date is before march and the start year is
        // a leap year then add an extra day to account for Feb. 29.
        if ( pstStart->wMonth <= 2 &&
            (yr & 03) == 0 &&
            (yr <= 1750 || yr % 100 != 0 || yr % 400 == 0))
            {
            cday++;
            }

        // Account for the days in each year (disregarding leap years).
        cday += 365;
        yr++;

        // Keep on accounting for the days in each year including leap
        // years until we reach the end year.
        while (yr < pstEnd->wYear)
            {
            cday += 365;
            if ((yr & 03) == 0 && (yr <= 1750 || yr % 100 != 0 || yr % 400 == 0))
                cday++;
            yr++;
            }
        }

    return(cday);
    }

/*
 -    DowStartOfYrMo
 -
 *    Purpose:
 *        Find the day of the week the indicated month begins on
 *
 *    Parameters:
 *        yr        year, must be > 0
 *        mo        month, number 1-12
 *
 *    Returns:
 *        day of the week (0-6) on which the month begins
 *        (0 = Sunday, 1 = Monday etc.)
 */
int GetStartDowForMonth(int yr, int mo)
    {
    int dow;

    // we want monday = 0, sunday = 6
    // dow = 6 + (yr - 1) + ((yr - 1) >> 2);
    dow = 5 + (yr - 1) + ((yr - 1) >> 2);
    if (yr > 1752)
        dow += ((yr - 1) - 1600) / 400 - ((yr - 1) - 1700) / 100 - 11;
    else if (yr == 1752 && mo > 9)
        dow -= 11;
    dow += mpcdymoAccum[mo - 1];
    if (mo > 2 && (yr & 03) == 0 && (yr <= 1750 || yr % 100 != 0 || yr % 400 == 0))
        dow++;
    dow %= 7;

    return(dow);
    }

int DowFromDate(const SYSTEMTIME *pst)
    {
    int dow;

    dow = GetStartDowForMonth(pst->wYear, pst->wMonth);
    dow = (dow + pst->wDay - 1) % 7;

    return(dow);
    }

int GetDaysForMonth(int yr, int mo)
    {
    int cdy;

    if (yr == 1752 && mo == 9)
        return(19);
    cdy = mpcdymoAccum[mo] - mpcdymoAccum[mo - 1];
    if (mo == 2 && (yr & 03) == 0 && (yr <= 1750 || yr % 100 != 0 || yr % 400 == 0))
        cdy++;

    return(cdy);
    }

/*
 -    NweekNumber
 -
 *    Purpose:
 *        Calculates week number in which a given date occurs, based
 *        on a specified start-day of week.
 *        Adjusts based on how a calendar would show this week
 *        (ie. week 53 is probably week 1 on the calendar).
 *
 *    Arguments:
 *        pdtm            Pointer to date in question
 *        dowStartWeek    Day-of-week on which weeks starts (0 - 6).
 *
 *    Returns:
 *        Week number of the year, in which *pdtr occurs.
 *
 */
// TODO: this currently ignores woyFirst
// it uses the 1st week containing 4+ days as the first week (woyFirst = 2)
// need to make appropriate changes so it handles woyFirst = 0 and = 1...
int GetWeekNumber(const SYSTEMTIME *pst, int dowFirst, int woyFirst)
    {
    int day, ddow, ddowT, nweek;
    SYSTEMTIME st;
    
    st.wYear = pst->wYear;
    st.wMonth = 1;
    st.wDay = 1;

    ddow = GetStartDowForMonth(st.wYear, st.wMonth) - dowFirst;
    if (ddow < 0)
        ddow += 7;

    if (pst->wMonth == 1 && pst->wDay < 8 - ddow)
        {
        nweek = 0;
        }
    else
        {
        if (ddow)
            st.wDay = 8 - ddow;

        nweek = (DaysBetweenDates(&st, pst) / 7) + 1;
        }
    if (ddow && ddow <= 3)
        nweek++;

    // adjust if necessary for calendar
    if (!nweek)
        {
        if (!ddow)
            return(1);

        // check what week Dec 31 is on
        st.wYear--;
        st.wMonth = 12;
        st.wDay = 31;
        return(GetWeekNumber(&st, dowFirst, woyFirst));
        }
    else if (nweek >= 52)
        {
        ddowT = (GetStartDowForMonth(pst->wYear, pst->wMonth) +
                    pst->wDay - 1 + 7 -    dowFirst) % 7;
        day = pst->wDay + (7 - ddowT);
        if (day > 31 + 4)
            nweek = 1;
        }

    return(nweek);
    }

// ignores day of week and time-related fields...
// BUGBUG also validate years in range
BOOL IsValidDate(const SYSTEMTIME *pst)
    {
    int cDay;

    if (pst && pst->wMonth >= 1 && pst->wMonth <= 12)
        {
        cDay = GetDaysForMonth(pst->wYear, pst->wMonth);
        if (pst->wDay >= 1 && pst->wDay <= cDay)
            return(TRUE);
        }
    return(FALSE);
    }

// ignores milliseconds and date-related fields...
BOOL IsValidTime(const SYSTEMTIME *pst)
    {
    return(pst->wHour <= 23 &&
            pst->wMinute <= 59 &&
            pst->wSecond <= 59);
    }

// ignores day of week
BOOL IsValidSystemtime(const SYSTEMTIME *pst)
    {
    if (pst && pst->wMonth >= 1 && pst->wMonth <= 12)
        {
        int cDay = GetDaysForMonth(pst->wYear, pst->wMonth);
        if (pst->wDay >= 1 &&
            pst->wDay <= cDay &&
            pst->wHour <= 23 &&
            pst->wMinute <= 59 &&
            pst->wSecond <= 59 &&
            pst->wMilliseconds < 1000)
            return(TRUE);
        }
    return(FALSE);
    }

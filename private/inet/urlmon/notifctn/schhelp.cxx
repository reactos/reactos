//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       schhelp.cxx
//
//  Contents:   helper APIs for the scheduling notifications
//
//  Classes:
//
//  Functions:
//
//  History:    12-17-1996   JohannP (Johann Posch)   Moved from schedule.cpp
//
//----------------------------------------------------------------------------
#include <notiftn.h>
#include "notfmgr.hxx"
#include "../utils/cvar.hxx"

#ifndef xTASK_EVENT_TRIGGER_AT_NET_IDLE
#define xTASK_EVENT_TRIGGER_AT_NET_IDLE   8
#endif

#define TF_THISMODULE TF_SCHEDULER

// Time definitions
const WORD  JOB_DAYS_PER_WEEK           = 7;
const WORD  JOB_MAX_DAY                 = 31;
const WORD  JOB_DAYS_PER_MONTHMAX       = JOB_MAX_DAY;
const WORD  JOB_MONTHS_PER_YEAR         = 12;
const WORD  JOB_MINS_PER_HOUR           = 60;
const WORD  JOB_HOURS_PER_DAY           = 24;
const WORD  JOB_MIN_MONTH               = 1;
const WORD  JOB_MAX_MONTH               = 12;

const WORD  JOB_MILLISECONDS_PER_SECOND = 1000;
const WORD  JOB_MILLISECONDS_PER_MINUTE = 60 * JOB_MILLISECONDS_PER_SECOND;
const DWORD FILETIMES_PER_MILLISECOND   = 10000;
const DWORD FILETIMES_PER_MINUTE        = FILETIMES_PER_MILLISECOND * JOB_MILLISECONDS_PER_MINUTE;
__int64     FILETIMES_PER_DAY;

const BYTE g_rgMonthDays[] =
{
    0,  // make January's index 1
    31, // January
    28, // February
    31, // March
    30, // April
    31, // May
    30, // June
    31, // July
    31, // August
    30, // September
    31, // October
    30, // November
    31  // December
};

// random number seed
#define RANDNUM_MAX 0x7fff

// prototypes for local functions:
void    AddMinutesToFileTime(LPFILETIME pft, DWORD Minutes);
void    AddDaysToFileTime(LPFILETIME pft, WORD Days);
HRESULT MonthDays(WORD wMonth, WORD wYear, WORD *pwDays);
BOOL    IsLeapYear(WORD wYear);

// Function to convert wide to multibyte chars.  Debug only.  Used in
// GetRunTimes.
#ifdef DBG
int MyOleStrToStrN(LPTSTR psz, int cchMultiByte, LPCOLESTR pwsz)
{
    return WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz,
                    cchMultiByte, NULL, NULL);
}
#endif

LONG InitializeRandomSeed()
{
    SYSTEMTIME  st;
    GetSystemTime(&st);
    return (LONG)st.wMilliseconds;
}

//+-------------------------------------------------------------------------
//
//  Function:   randnum
//
//  Synopsys:   Generate random number based on seed. (copied from crt)
//
//+-------------------------------------------------------------------------
int randnum (void)
{
    static long holdrand = InitializeRandomSeed();
    return(holdrand = ((holdrand * 214013L + 2531011L) >> 16) & RANDNUM_MAX);
}

//+-------------------------------------------------------------------------
//
//  Function:   IsLeapYear
//
//  Synopsis:   Determines if a given year is a leap year.
//
//  Arguments:  [wYear]  - the year
//
//  Returns:    boolean value: TRUE == leap year
//
//  History:    05-05-93 EricB
//
// BUGBUG: kevinro sez that there were no regular leap years prior to 1904!
//--------------------------------------------------------------------------
BOOL
IsLeapYear(WORD wYear)
{
    return wYear % 4 == 0 && wYear % 100 != 0 || wYear % 400 == 0;
}

//+----------------------------------------------------------------------------
//
//  Function:   MonthDays
//
//  Synopsis:   Returns the number of days in the indicated month.
//
//  Arguments:  [wMonth] - Index of the month in question where January = 1
//                         through December equalling 12.
//              [yYear]  - If non-zero, then leap year adjustment for February
//                         will be applied.
//              [pwDays] - The place to return the number of days in the
//                         indicated month.
//
//  Returns:    S_OK or E_INVALIDARG
//
//  History:    10-29-93 EricB
//
//-----------------------------------------------------------------------------
HRESULT
MonthDays(WORD wMonth, WORD wYear, WORD *pwDays)
{
    if (wMonth < JOB_MIN_MONTH || wMonth > JOB_MAX_MONTH)
    {
        return E_INVALIDARG;
    }
    *pwDays = g_rgMonthDays[wMonth];
    //
    // If February, adjust for leap years
    //
    if (wMonth == 2 && wYear != 0)
    {
        if (IsLeapYear(wYear))
        {
            (*pwDays)++;
        }
    }
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Functions:  AddDaysToFileTime
//
//  Synopsis:   Convert the days value to filetime units and add it to
//              the filetime.
//
//-----------------------------------------------------------------------------
void
AddDaysToFileTime(LPFILETIME pft, WORD Days)
{
    if (!Days)
    {
        return; // Nothing to do.
    }
    //
    // ft = ft + Days * FILETIMES_PER_DAY;
    //
    ULARGE_INTEGER uli, uliSum;
    uli.LowPart  = pft->dwLowDateTime;
    uli.HighPart = pft->dwHighDateTime;
#ifndef unix
    uliSum.QuadPart = uli.QuadPart + (__int64)Days * FILETIMES_PER_DAY;
#else
    U_QUAD_PART(uliSum) = U_QUAD_PART(uli) + (__int64)Days * FILETIMES_PER_DAY;
#endif /* unix */
    pft->dwLowDateTime  = uliSum.LowPart;
    pft->dwHighDateTime = uliSum.HighPart;
}

//+----------------------------------------------------------------------------
//
//  Functions:  AddMinutesToFileTime
//
//  Synopsis:   Convert the minutes value to filetime units and add it to
//              the filetime.
//
//-----------------------------------------------------------------------------
void AddMinutesToFileTime(LPFILETIME pft, DWORD Minutes)
{
    if (!Minutes)
    {
        return; // Nothing to do.
    }
    //
    // ft = ft + Minutes * FILETIMES_PER_MINUTE;
    //
    ULARGE_INTEGER uli, uliSum;
    uli.LowPart  = pft->dwLowDateTime;
    uli.HighPart = pft->dwHighDateTime;
#ifndef unix
    uliSum.QuadPart = uli.QuadPart + (__int64)Minutes * FILETIMES_PER_MINUTE;
#else
    U_QUAD_PART(uliSum) = U_QUAD_PART(uli) + (__int64)Minutes + FILETIMES_PER_MINUTE;
#endif /* unix */
    pft->dwLowDateTime  = uliSum.LowPart;
    pft->dwHighDateTime = uliSum.HighPart;
}

//+----------------------------------------------------------------------------
//
//  Function:   ValidateTrigger
//
//  Synopsis:   Ensures a task trigger has a valid configuration
//
//  Arguments:  ptt             Task trigger to inspect
//
//  Returns:    S_OK:           Task trigger is valid
//              E_INVALIDARG:   Task trigger is invalid
//              E_NOTIMPL:      Task trigger is valid but not supported
//
//  Notes:      Following fields are validated in GetRunTimes:
//              - wBegin[Day|Month|Year]
//              - wEnd[Day|Month|Year]
//-----------------------------------------------------------------------------

// all the possible flags for a task trigger
#define TASK_TRIGGER_FLAGS \
        (  TASK_TRIGGER_FLAG_HAS_END_DATE         \
         | TASK_TRIGGER_FLAG_KILL_AT_DURATION_END \
         | TASK_TRIGGER_FLAG_DISABLED             \
        )

HRESULT ValidateTrigger(TASK_TRIGGER *ptt)
{
    if(NULL == ptt)
    {
        return E_INVALIDARG;
    }

    // validate size of structure, Reserved1, and Reserved2 fields
    if (   ptt->cbTriggerSize < sizeof(TASK_TRIGGER)
        || ptt->Reserved1
        || ptt->Reserved2
       )
        return E_INVALIDARG;

    // make sure wStartHour and wStartMinute are valid
    if (   ptt->wStartHour > 23
        || ptt->wStartMinute > 59
       )
        return E_INVALIDARG;

    // validate flags
    if (ptt->rgFlags & ~TASK_TRIGGER_FLAGS)
        return E_INVALIDARG;

    // we don't support kill at duration end
    if (ptt->rgFlags & TASK_TRIGGER_FLAG_KILL_AT_DURATION_END)
        return E_NOTIMPL;

    // we screwed us up by assigning TASK_FLAG_DISABLED to the task trigger
    // type of the manual group!
    if (ptt->rgFlags & TASK_TRIGGER_FLAG_DISABLED)
        return S_OK;

    // assume success
    HRESULT hres = S_OK;

    // validate TriggerType and Type
    switch(ptt->TriggerType) {
    case TASK_TIME_TRIGGER_ONCE:
        break;
    case TASK_TIME_TRIGGER_DAILY:
        if(0 == ptt->Type.Daily.DaysInterval)
            hres = E_INVALIDARG;
        break;
    case TASK_TIME_TRIGGER_WEEKLY:
        if(   0 == ptt->Type.Weekly.WeeksInterval
           || 0 == ptt->Type.Weekly.rgfDaysOfTheWeek
          )
            hres = E_INVALIDARG;
        break;
    case TASK_TIME_TRIGGER_MONTHLYDATE:
        if(   0 == ptt->Type.MonthlyDate.rgfDays
           || 0 == ptt->Type.MonthlyDate.rgfMonths
          )
            hres = E_INVALIDARG;
        break;
    case TASK_TIME_TRIGGER_MONTHLYDOW:
        if(   ptt->Type.MonthlyDOW.wWhichWeek < TASK_FIRST_WEEK
           || ptt->Type.MonthlyDOW.wWhichWeek > TASK_LAST_WEEK
           || 0 == ptt->Type.MonthlyDOW.rgfDaysOfTheWeek
           || 0 == ptt->Type.MonthlyDOW.rgfMonths
          )
            hres = E_INVALIDARG;
        break;
    case TASK_EVENT_TRIGGER_ON_IDLE:
    case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
    case TASK_EVENT_TRIGGER_AT_LOGON:
        // we don't support event triggering
        hres = E_NOTIMPL;
        break;
    default:
        // invalid TriggerType field
        hres = E_INVALIDARG;
        break;
    }

    return hres;
}

//+----------------------------------------------------------------------------
//
//  Function:   ValidateTaskData
//
//  Synopsis:   Ensures a TASK_DATA structure is valid
//
//  Arguments:  ptd             structure to inspect
//
//  Returns:    S_OK:           structure is valid
//              E_INVALIDARG:   structure is invalid
//
//  Notes:      - dwPriority is not validated because only set by not mgr
//                not read
//              - dwDuration is not validated because not mgr never looks at
//                it - any value is valid anyway unless an arbitrary maximum
//                is imposed
//              - nParallelTasks is not validated because it's never looked at
//-----------------------------------------------------------------------------

// valid task flags
#define TASK_FLAGS                                  \
        (                                           \
           TASK_FLAG_INTERACTIVE                    \
         | TASK_FLAG_DELETE_WHEN_DONE               \
         | TASK_FLAG_DISABLED                       \
         | TASK_FLAG_START_ONLY_IF_IDLE             \
         | TASK_FLAG_KILL_ON_IDLE_END               \
         | TASK_FLAG_DONT_START_IF_ON_BATTERIES     \
         | TASK_FLAG_KILL_IF_GOING_ON_BATTERIES     \
         | TASK_FLAG_RUN_ONLY_IF_DOCKED             \
         | TASK_FLAG_HIDDEN                         \
         | TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET   \
        )

// flags that are actually implemented
#define TASK_FLAGS_IMPL                             \
        (                                           \
           TASK_FLAG_START_ONLY_IF_IDLE             \
         | TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET   \
        )

HRESULT ValidateTaskData(TASK_DATA *ptd)
{
    // NULL task data is valid
    if(NULL == ptd)
        return S_OK;

    // validate size and reserved
    if(   ptd->cbSize < sizeof(TASK_DATA)
       || ptd->dwReserved
      )
        return E_INVALIDARG;

    // validate flags
    if(ptd->dwTaskFlags & ~TASK_FLAGS)
        return E_INVALIDARG;

    if(ptd->dwTaskFlags & ~TASK_FLAGS_IMPL)
        return E_NOTIMPL;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetRunTimes
//
//  Synopsis:   Return a list of run times for this trigger that fall between
//              the bracketing times; pstBracketBegin is inclusive,
//              pstBracketEnd is exclusive.
//
//  Arguments:  [jt]              - Inspected trigger.
//              [td]              - the task data structur
//              [pstBracketBegin] - the start of the bracketing period
//              [pstBracketEnd]   - the end of the bracketing period, may
//                                  be NULL
//              [pCount]          - the number of CRun elements in the
//                                  list; non-zero input values set an upper
//                                  limit on the size of the returned list
//              [pRunList]        - the returned list of run time objects, can
//                                  be NULL if just checking to see if there
//                                  will be *any* runs.
//              [ptszJobName],
//              [dwJobFlags],
//              [dwMaxRunTime]    - the last 3 params are used for the CRun
//                                  objects as their member data.
//
//  Returns:    S_OK: all of the runs requested by *pCount on entry have been
//                    found. If pstBrackedEnd is non-NULL, then this also
//                    indicates that the number of runs found is all of the
//                    runs between the run brackets.
//              S_FALSE: there are more runs between the bracketing times
//                       than the amount asked for by *pCount on entry. This
//                       only applies when pstBrackedEnd is non-NULL.
//              SCHED_S_EVENT_TRIGGER: this is an event trigger.
//              SCHED_S_TASK_NO_VALID_TRIGGERS: the trigger is disabled or
//                                              not set.
//
//  Notes:      The trigger time list is callee allocated and caller freed. The
//              caller must use delete to free this list.
//-----------------------------------------------------------------------------
HRESULT GetRunTimes(TASK_TRIGGER & jt,
                    TASK_DATA    * ptd,
                    LPSYSTEMTIME   pstBracketBegin,
                    LPSYSTEMTIME   pstBracketEnd,
                    WORD *         pCount,
                    FILETIME *     pRunList)
{
    HRESULT hr = S_OK;
    DWORD dwRet;

    // Can't initialize __int64 globals without crt
    FILETIMES_PER_DAY = (__int64)FILETIMES_PER_MINUTE *
                        (__int64)JOB_MINS_PER_HOUR *
                        (__int64)JOB_HOURS_PER_DAY;

    //
    // Make sure we have a valid trigger
    //
    hr = ValidateTrigger(&jt);
    if(FAILED(hr))
        return hr;

    //
    // Event triggers don't have set run times.
    //
    switch (jt.TriggerType)
    {
    case TASK_EVENT_TRIGGER_ON_IDLE:
    case xTASK_EVENT_TRIGGER_AT_NET_IDLE:
    case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
    case TASK_EVENT_TRIGGER_AT_LOGON:
        // Not yet implemented:
        // case TASK_EVENT_TRIGGER_ON_APM_RESUME:

        *pCount = 0;

        return SCHED_S_EVENT_TRIGGER;
    }

    //
    // Return if this trigger is disabled.
    //
    if (jt.rgFlags & TASK_TRIGGER_FLAG_DISABLED)
    {
        *pCount = 0;
        return SCHED_S_TASK_NO_VALID_TRIGGERS;
    }
    SYSTEMTIME st = {0};
    //
    // Convert to FILETIMEs and check if the trigger lifetime intersects the
    // requested run bracket.
    // If there is a trigger end date, then one of three conditions holds:
    // a. *pstBracketBegin > jt.End{Month/Day/Year}
    //    result, no runs
    // b. *pstBracketBegin < jt.End{Month/Day/Year} < *pstBracketEnd
    //    result, return all runs between *pstBracketBegin and
    //    jt.End{Month/Day/Year}
    // c. jt.End{Month/Day/Year} > *pstBracketEnd
    //    result, return all runs between *pstBracketBegin and *pstBracketEnd
    // In addition, if there is a bracket end we check:
    // d. *pstBracketEnd < jt.Begin{Month/Day/Year}
    //    result, no runs
    //
    FILETIME ftTriggerBegin, ftTriggerEnd, ftBracketBegin, ftBracketEnd;

    if (!SystemTimeToFileTime(pstBracketBegin, &ftBracketBegin))
    {
        dwRet = GetLastError();
        *pCount = 0;
        return HRESULT_FROM_WIN32(dwRet);
    }

    st.wYear   = jt.wBeginYear;
    st.wMonth  = jt.wBeginMonth;
    st.wDay    = jt.wBeginDay;
    st.wHour   = jt.wStartHour;
    st.wMinute = jt.wStartMinute;

    if (!SystemTimeToFileTime(&st, &ftTriggerBegin))
    {
        dwRet = GetLastError();
        *pCount = 0;
        return HRESULT_FROM_WIN32(dwRet);
    }

    st.wHour   = 23;    // set to the last hour of the day.
    st.wMinute = 59;    // set to the last minute of the day.
    st.wSecond = 59;    // set to the last second of the day.

    if (jt.rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE)
    {
        st.wYear  = jt.wEndYear;
        st.wMonth = jt.wEndMonth;
        st.wDay   = jt.wEndDay;

        if (!SystemTimeToFileTime(&st, &ftTriggerEnd))
        {
            dwRet = GetLastError();
            *pCount = 0;
            return HRESULT_FROM_WIN32(dwRet);
        }

        if (CompareFileTime(&ftTriggerEnd, &ftBracketBegin) < 0)
        {
            //
            // Trigger end time is before the run bracket begin time (case a.).
            //
            *pCount = 0;
            return SCHED_S_TASK_NO_MORE_RUNS;
        }
    }
    else    // no trigger end date.
    {
        //
        // Create an end date that is reasonably large.
        //
        st.wMonth = 12;
        st.wDay = 31;
        st.wYear = 2200;

        if (!SystemTimeToFileTime(&st, &ftTriggerEnd))
        {
            dwRet = GetLastError();
            *pCount = 0;
            return HRESULT_FROM_WIN32(dwRet);
        }
    }

    if (pstBracketEnd)
    {
        if (!SystemTimeToFileTime(pstBracketEnd, &ftBracketEnd))
        {
            dwRet = GetLastError();
            *pCount = 0;
            return HRESULT_FROM_WIN32(dwRet);
        }

        if (CompareFileTime(&ftTriggerBegin, &ftBracketEnd) > 0)
        {
            //
            // The trigger start date is after the bracket end date, there are
            // no runs (case d.).
            //
            *pCount = 0;
            return S_OK;
        }

        if (CompareFileTime(&ftTriggerEnd, &ftBracketEnd) < 0)
        {
            //
            // Trigger end is before bracket end, so set bracket end to
            // trigger end (case b.).
            //
            ftBracketEnd = ftTriggerEnd;
        }
    }
    else
    {
        //
        // No bracket end, so use trigger end (case c.).
        //
        ftBracketEnd = ftTriggerEnd;
    }

    //
    // Save the passed in limit before modifying the count.
    //
    WORD cLimit;
    if (*pCount > 0)
    {
        cLimit = *pCount;
        *pCount = 0;
    }
    else
    {
        cLimit = TASK_MAX_RUN_TIMES; // USHRT_MAX;
    }

    FILETIME ftRun, ftDurationStart, ftDurationEnd;
    WORD rgfRunDOW[JOB_DAYS_PER_WEEK], i;
    WORD rgfDaysOfMonth[JOB_DAYS_PER_MONTHMAX];
    WORD rgfMonths[JOB_MONTHS_PER_YEAR];
    WORD wDay, wBeginDOW, wCurDOW,  wCurDay, wLastDOM, wCurMonth, wCurYear;
    WORD cRunDOW, iRunDOW, IndexStart;
    BOOL fWrapped;
    fWrapped = FALSE;

    //
    // Calculate the trigger's first run time.
    //
    switch (jt.TriggerType)
    {
    case TASK_TIME_TRIGGER_ONCE:
        // fall through to daily:

    case TASK_TIME_TRIGGER_DAILY:
        //
        // The first run time is the trigger begin time.
        //
        ftRun = ftTriggerBegin;
        break;

    case TASK_TIME_TRIGGER_WEEKLY:
        //
        // At jobs clear the DOW bits, so make sure we don't have an expired
        // At job.
        //
        if (jt.Type.Weekly.rgfDaysOfTheWeek == 0)
        {
            *pCount = 0;
            return S_OK;
        }

        //
        // See what day of the week the trigger begin day is. SYSTEMTIME
        // defines Sunday = 0, Monday = 1, etc.
        //
        FileTimeToSystemTime(&ftTriggerBegin, &st);
        wBeginDOW = st.wDayOfWeek;
        //
        // Convert the trigger data run day bit array into a boolean array
        // so that the results can be compared with the SYSTEMTIME value.
        // This array will also be used in the main loop.
        //
        for (i = 0; i < JOB_DAYS_PER_WEEK; i++)
        {
            rgfRunDOW[i] = (jt.Type.Weekly.rgfDaysOfTheWeek >> i) & 0x1;
        }
        //
        // Find the first set day-of-the-week after the trigger begin day.
        //
        for (i = 0; i < JOB_DAYS_PER_WEEK; i++)
        {
            wCurDOW = wBeginDOW + i;
            if (wCurDOW >= JOB_DAYS_PER_WEEK)
            {
                wCurDOW -= JOB_DAYS_PER_WEEK;
            }
            if (rgfRunDOW[wCurDOW])
            {
                ftRun = ftTriggerBegin;
                AddDaysToFileTime(&ftRun, i);
                break;
            }
        }
        break;

    case TASK_TIME_TRIGGER_MONTHLYDATE:
        //
        // At jobs clear the days bits, so make sure we don't have an expired
        // At job.
        //
        if (jt.Type.MonthlyDate.rgfDays == 0)
        {
            *pCount = 0;
            return S_OK;
        }

        //
        // Convert the bit fields to boolean arrays.
        // These arrays will also be used in the main loop.
        //
        for (i = 0; i < JOB_DAYS_PER_MONTHMAX; i++)
        {
            rgfDaysOfMonth[i] = (WORD)(jt.Type.MonthlyDate.rgfDays >> i) & 0x1;
        }
        for (i = 0; i < JOB_MONTHS_PER_YEAR; i++)
        {
            rgfMonths[i] = (jt.Type.MonthlyDate.rgfMonths >> i) & 0x1;
        }

        wCurDay = jt.wBeginDay;
        wCurMonth = jt.wBeginMonth;
        wCurYear = jt.wBeginYear;
        BOOL fDayOverflow, fDayFound;
        fDayFound = FALSE;
        do
        {
            MonthDays(wCurMonth, wCurYear, &wLastDOM);
            //
            // Find the first run day after the trigger start day, including
            // the trigger start day.
            //
            for (i = 0; i < wLastDOM; i++)
            {
                if (wCurDay > wLastDOM)
                {
                    //
                    // Adjust for wrapping.
                    //
                    wCurDay = 1;
                    fWrapped = TRUE;
                }
                if (rgfDaysOfMonth[wCurDay - 1])
                {
                    fDayFound = TRUE;
                    break;
                }
                wCurDay++;
            }
            //
            // Find the first run month.
            //
            for (i = 0; i < JOB_MONTHS_PER_YEAR; i++)
            {
                if (wCurMonth > JOB_MONTHS_PER_YEAR)
                {
                    wCurMonth = 1;
                    wCurYear++;
                }
                //
                // Check for run month match. Note that rgfMonths is zero based
                // and wCurMonth is one based.
                //
                if (rgfMonths[wCurMonth - 1])
                {
                    if (fWrapped && !i)
                    {
                        //
                        // Even though we have a match for run month, the run
                        // date for the first month has passed, so move on to
                        // the next run month.
                        //
                        fWrapped = FALSE;
                        continue;
                    }
                    break;
                }
                wCurMonth++;
            }
            //
            // Check for days overflow.
            //
            MonthDays(wCurMonth, wCurYear, &wLastDOM);
            if (wCurDay > wLastDOM)
            {
                //
                // Note that this clause would be entered infinitely if there
                // were no valid dates. ITask::SetTrigger validates the data to
                // ensure that there are valid dates.
                //
                fDayOverflow = TRUE;
                fDayFound = FALSE;
                wCurDay = 1;
                wCurMonth++;
                if (wCurMonth > JOB_MONTHS_PER_YEAR)
                {
                    wCurMonth = 1;
                    wCurYear++;
                }
            }
            else
            {
                fDayOverflow = FALSE;
            }
        } while (fDayOverflow & !fDayFound);

        break;

    case TASK_TIME_TRIGGER_MONTHLYDOW:
        //
        // Convert the bit fields to boolean arrays.
        // These arrays will also be used in the main loop.
        //
        cRunDOW = 0;
        for (i = 0; i < JOB_DAYS_PER_WEEK; i++)
        {
            if ((jt.Type.MonthlyDOW.rgfDaysOfTheWeek >> i) & 0x1)
            {
                cRunDOW++;
                rgfRunDOW[i] = TRUE;
            }
            else
            {
                rgfRunDOW[i] = FALSE;
            }
        }
        for (i = 0; i < JOB_MONTHS_PER_YEAR; i++)
        {
            rgfMonths[i] = (jt.Type.MonthlyDOW.rgfMonths >> i) & 0x1;
        }
        //
        // See if the trigger start month is in rgfMonths and if not
        // move to the first month in rgfMonths after jt.BeginMonth.
        //
        wCurMonth = jt.wBeginMonth;
        wCurYear = jt.wBeginYear;
        BOOL fInStartMonth;
        IndexStart = 0;
        CheckNextMonth:
        for (i = IndexStart; i < (JOB_MONTHS_PER_YEAR + IndexStart); i++)
        {
            //
            // Check for run month match. Note that rgfMonths is zero based
            // and wCurMonth is one based.
            //
            if (rgfMonths[wCurMonth - 1])
            {
                break;
            }

            wCurMonth++;
            if (wCurMonth > JOB_MONTHS_PER_YEAR)
            {
                wCurMonth -= JOB_MONTHS_PER_YEAR;
                wCurYear++;
            }
        }

        fInStartMonth = i == 0;

        //
        // See what day of the week the first day of the month is.
        //
        st.wMonth = wCurMonth;
        st.wDay = 1;
        st.wYear = wCurYear;

        //
        // Convert to FILETIME and back to SYSTEMTIME to get wDayOfWeek.
        //
        SystemTimeToFileTime(&st, &ftRun);
        FileTimeToSystemTime(&ftRun, &st);
        wBeginDOW = st.wDayOfWeek;

        //
        // Find the first run DayOftheWeek. If it is before the start
        // day, find the next and so on until after the start day.
        //
        iRunDOW = cRunDOW;

        for (i = 0; i < JOB_DAYS_PER_WEEK; i++)
        {
            wCurDOW = wBeginDOW + i;
            wCurDay = 1 + i;

            if (wCurDOW >= JOB_DAYS_PER_WEEK)
            {
                wCurDOW -= JOB_DAYS_PER_WEEK;
            }

            if (rgfRunDOW[wCurDOW])
            {
                iRunDOW--;
                wCurDay += (jt.Type.MonthlyDOW.wWhichWeek - 1)
                           * JOB_DAYS_PER_WEEK;

                MonthDays(wCurMonth, wCurYear, &wLastDOM);

                if (wCurDay > wLastDOM)
                {
                    //
                    // This case can be reached if
                    // jt.Type.MonthlyDOW.wWhichWeek == TASK_LAST_WEEK
                    // which means to always run on the last occurance of
                    // this day for the month.
                    //
                    wCurDay -= JOB_DAYS_PER_WEEK;
                }

                if (fInStartMonth && wCurDay < jt.wBeginDay)
                {
                    if (iRunDOW)
                    {
                        //
                        // There are more runs this month, so check those.
                        //
                        continue;
                    }
                    else
                    {
                        //
                        // Start with the next run month.
                        //
                        IndexStart++;
                        goto CheckNextMonth;
                    }
                }
                break;
            }
        }
        wDay = 1 + i;
        break;

    default:
        return E_FAIL;
    }

    if (jt.TriggerType == TASK_TIME_TRIGGER_MONTHLYDATE ||
        jt.TriggerType == TASK_TIME_TRIGGER_MONTHLYDOW)
    {
        st.wYear   = wCurYear;
        st.wMonth  = wCurMonth;
        st.wDay    = wCurDay;
        st.wHour   = jt.wStartHour;
        st.wMinute = jt.wStartMinute;
        st.wSecond = st.wMilliseconds = 0;
        SystemTimeToFileTime(&st, &ftRun);
    }

    //
    // Set the initial duration period endpoints.
    //
    // ftDurationEnd = ftDurationStart + jt.MinutesDuration
    //                 * FILETIMES_PER_MINUTE;
    //
    ftDurationStart = ftDurationEnd = ftRun;
    AddMinutesToFileTime(&ftDurationEnd, jt.MinutesDuration);
    BOOL fPassedDurationEnd = FALSE;

    //
    // Main loop. Find all of the runs after the initial run.
    //
    do
    {
        //
        // If the run falls within the run bracket, add it to the list.
        //
        if (CompareFileTime(&ftRun, &ftBracketBegin) >= 0 &&
            CompareFileTime(&ftRun, &ftBracketEnd) < 0)
        {
            //
            // Check the count and return if at the limit.
            //
            if (*pCount >= cLimit)
            {
                //
                // If pstBracketEnd is non-NULL, return S_FALSE to indicate
                // that there are more runs within the run brackets.
                //
                return (pstBracketEnd != NULL) ? S_FALSE : S_OK;
            }

            FILETIME ftEnd = {0, 0};
            if (jt.rgFlags & TASK_TRIGGER_FLAG_KILL_AT_DURATION_END)
            {
                ftEnd = ftDurationEnd;
            }

            if (pRunList != NULL)
            {
                // add random minutes if necessary
                if(jt.wRandomMinutesInterval) {

                    // use smaller of MinutesInterval and wRandom... for
                    // random interval.  This ensures that we pick a time
                    // before the next repeated time.
                    DWORD dwRandTime;
                    if(jt.MinutesInterval && (jt.MinutesInterval < jt.wRandomMinutesInterval))
                        dwRandTime = jt.MinutesInterval;
                    else
                        dwRandTime = jt.wRandomMinutesInterval;

                    // pick time in our range
                    dwRandTime = (dwRandTime * randnum()) / RANDNUM_MAX;

                    AddMinutesToFileTime(&ftRun, dwRandTime);
                }

                // save time in run list
                pRunList[(*pCount)] = ftRun;
            }

            (*pCount)++;
        }

        //
        // If the run is past the bracket end, return.
        //
        if (CompareFileTime(&ftRun, &ftBracketEnd) >= 0)
        {
            return S_OK;
        }

        //
        // Calculate the next run time.
        //

        //
        // If there is minutes repetition (MinutesInterval non-zero), then
        // compute all of the runs in the duration period.
        //

        if (jt.MinutesInterval)
        {
            //
            // Add the minutes interval.
            //
            AddMinutesToFileTime(&ftRun, jt.MinutesInterval);

            //
            // See if we are at the end of this duration period.
            //
            if (CompareFileTime(&ftDurationEnd, &ftRun) <= 0)
            {
                fPassedDurationEnd = TRUE;
            }
        }

        //
        // If there is no minutes repetition (MinutesInterval is zero) or we
        // have passed the end of the duration period, then calculate the next
        // duration start (which is also the next run).
        //
        if (!jt.MinutesInterval || fPassedDurationEnd)
        {
            switch (jt.TriggerType)
            {
            case TASK_TIME_TRIGGER_ONCE:
                return S_OK;

            case TASK_TIME_TRIGGER_DAILY:
                //
                // ftNextRun = ftCurRun + DaysInterval * FILETIMES_PER_DAY;
                //
                AddDaysToFileTime(&ftDurationStart, jt.Type.Daily.DaysInterval);
                break;

            case TASK_TIME_TRIGGER_WEEKLY:
                fWrapped = FALSE;
                //
                // Find the next DayOfWeek to run on.
                //
                for (i = 1; i <= JOB_DAYS_PER_WEEK; i++)
                {
                    wCurDOW++;
                    if (wCurDOW >= JOB_DAYS_PER_WEEK)
                    {
                        //
                        // We have wrapped into the next week.
                        //
                        wCurDOW -= JOB_DAYS_PER_WEEK;
                        fWrapped = TRUE;
                    }
                    if (rgfRunDOW[wCurDOW])
                    {
                        AddDaysToFileTime(&ftDurationStart, i);
                        break;
                    }
                }

                if (fWrapped)
                {
                    //
                    // Starting a new week, so add the weeks increment.
                    //
                    AddDaysToFileTime(&ftDurationStart,
                                      (jt.Type.Weekly.WeeksInterval - 1)
                                      * JOB_DAYS_PER_WEEK);
                }
                break;

            case TASK_TIME_TRIGGER_MONTHLYDATE:
                BOOL fDayFound;
                fWrapped = FALSE;
                fDayFound = FALSE;
                //
                // Find the next day to run.
                //
                do
                {
                    MonthDays(wCurMonth, wCurYear, &wLastDOM);
                    for (i = 1; i <= wLastDOM; i++)
                    {
                        wCurDay++;
                        if (wCurDay > wLastDOM)
                        {
                            //
                            // Adjust for wrapping.
                            //
                            wCurDay = 1;
                            fWrapped = TRUE;
                            wCurMonth++;
                            if (wCurMonth > JOB_MONTHS_PER_YEAR)
                            {
                                wCurMonth = 1;
                                wCurYear++;
                            }
                            MonthDays(wCurMonth, wCurYear, &wLastDOM);
                        }
                        if (rgfDaysOfMonth[wCurDay - 1])
                        {
                            fDayFound = TRUE;
                            break;
                        }
                    }
                    if (fWrapped || !fDayFound)
                    {
                        //
                        // The prior month is done, find the next month.
                        //
                        for (i = 1; i <= JOB_MONTHS_PER_YEAR; i++)
                        {
                            if (wCurMonth > JOB_MONTHS_PER_YEAR)
                            {
                                wCurMonth = 1;
                                wCurYear++;
                            }
                            if (rgfMonths[wCurMonth - 1])
                            {
                                fWrapped = FALSE;
                                break;
                            }
                            wCurMonth++;
                        }
                    }
                } while (!fDayFound);
                break;

            case TASK_TIME_TRIGGER_MONTHLYDOW:
                if (!iRunDOW)
                {
                    //
                    // All of the runs for the current month are done, find the
                    // next month.
                    //
                    for (i = 0; i < JOB_MONTHS_PER_YEAR; i++)
                    {
                        wCurMonth++;
                        if (wCurMonth > JOB_MONTHS_PER_YEAR)
                        {
                            wCurMonth = 1;
                            wCurYear++;
                        }
                        if (rgfMonths[wCurMonth - 1])
                        {
                            break;
                        }
                    }
                    //
                    // See what day of the week the first day of the month is.
                    //
                    st.wMonth = wCurMonth;
                    st.wDay = wDay = 1;
                    st.wYear = wCurYear;
                    SystemTimeToFileTime(&st, &ftRun);
                    FileTimeToSystemTime(&ftRun, &st);
                    wCurDOW = st.wDayOfWeek;
                    iRunDOW = cRunDOW;
                    //
                    // Start at the first run DOW for this next month.
                    //
                    IndexStart = 0;
                }
                else
                {
                    //
                    // Start at the next run DOW for the current month.
                    //
                    IndexStart = 1;
                }

                //
                // Find the next DayOfWeek to run on.
                //
                for (i = IndexStart; i <= JOB_DAYS_PER_WEEK; i++)
                {
                    if (i > 0)
                    {
                        wCurDOW++;
                        wDay++;
                    }
                    if (wCurDOW >= JOB_DAYS_PER_WEEK)
                    {
                        wCurDOW -= JOB_DAYS_PER_WEEK;
                    }
                    if (rgfRunDOW[wCurDOW])
                    {
                        //
                        // Found a run DayOfWeek.
                        //
                        iRunDOW--;
                        wCurDay = wDay + (jt.Type.MonthlyDOW.wWhichWeek - 1)
                                  * JOB_DAYS_PER_WEEK;
                        WORD wLastDOM;
                        MonthDays(wCurMonth, wCurYear, &wLastDOM);
                        if (wCurDay > wLastDOM)
                        {
                            //
                            // This case can be reached if
                            // jt.Type.MonthlyDOW.wWhichWeek == JOB_LAST_WEEK
                            // which means to always run on the last occurance
                            // of this day for the month.
                            //
                            wCurDay -= JOB_DAYS_PER_WEEK;
                        }
                        break;
                    }
                }
                break;

            default:
                return E_FAIL;
            }

            if (jt.TriggerType == TASK_TIME_TRIGGER_MONTHLYDATE ||
                jt.TriggerType == TASK_TIME_TRIGGER_MONTHLYDOW)
            {
                st.wYear   = wCurYear;
                st.wMonth  = wCurMonth;
                st.wDay    = wCurDay;
                st.wHour   = jt.wStartHour;
                st.wMinute = jt.wStartMinute;
                st.wSecond = st.wMilliseconds = 0;
                SystemTimeToFileTime(&st, &ftDurationStart);
            }

            //
            // Calc the next duration period endpoints.
            //
            ftRun = ftDurationEnd = ftDurationStart;

            AddMinutesToFileTime(&ftDurationEnd, jt.MinutesDuration);

            fPassedDurationEnd = FALSE;
        }

    } while (*pCount < TASK_MAX_RUN_TIMES);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetResourceString
//
//  Synopsis:
//
//  Arguments:  [iResId] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPWSTR GetResourceString(UINT iResId)
{
    LPWSTR pwzStr = 0;
    char  szFormatString[MAX_NOTF_FORMAT_STRING_LENGTH];
    int cLen = LoadString(g_hInst, iResId, szFormatString, MAX_NOTF_FORMAT_STRING_LENGTH);


    if (cLen)
    {
        pwzStr = DupA2W(szFormatString);
    }

    return pwzStr;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteDWORD
//
//  Synopsis:
//
//  Arguments:  [pNotObj] --
//              [pwzStr]  --
//              [dwWord]  --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT WriteDWORD(LPNOTIFICATION pNotObj, LPWSTR pwzStr,  DWORD dwWord)
{
    HRESULT hr = E_FAIL;
    
    NotfAssert((pNotObj && pwzStr));
    COleVariant CVar((LONG) dwWord, VT_I4);

    hr = pNotObj->Write(pwzStr,CVar, 0);

    return hr;
}

HRESULT WriteGUID(LPNOTIFICATION pNotObj, LPCWSTR szName, GUID *pGuid)
{
    NotfAssert((pNotObj && szName && pGuid));
    
    WCHAR   wszCookie[GUIDSTR_MAX];

    StringFromGUID2(*pGuid, wszCookie, sizeof(wszCookie));
    return WriteOLESTR(pNotObj, szName, wszCookie);
}

HRESULT WriteOLESTR(LPNOTIFICATION pNotObj, LPCWSTR szName, LPCWSTR szVal)
{
    NotfAssert((pNotObj && szName && szVal));

    VARIANT Val;

    Val.vt = VT_BSTR;
    Val.bstrVal = SysAllocString(szVal);

    HRESULT hr = pNotObj->Write(szName, Val, 0);

    SysFreeString(Val.bstrVal);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetRandTime
//
//  Synopsis:
//
//  Arguments:  [wStartMins] --
//              [wEndMins] --
//              [wInc] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

//
//  Following function has no protecttion over irregular wInc. Might break
//  if the default values change.   dZhang
//

DWORD GetRandTime(DWORD wStartMins, DWORD wEndMins, DWORD wInc)
{
    DWORD wRange;
    DWORD wIncrements;

    if (wStartMins > wEndMins)
    {
        wRange = ((1440 - wStartMins) + wEndMins);
    }
    else
    {
        wRange = (wEndMins - wStartMins);
    }

    wIncrements = ((wRange / wInc) + 1);

    return (wStartMins + (randnum() % wIncrements) * wInc);
}




//+---------------------------------------------------------------------------
//
//  Function:   CheckNextRunDate
//
//  Synopsis:   check if the tasktrigger contains a valid trigger time
//
//  Arguments:  [pTaskTrigger] --
//              [CFileTime] --
//              [pDate] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CheckNextRunDate(PTASK_TRIGGER pTaskTrigger, PTASK_DATA pTaskData, CFileTime *pDate)
{
    NotfDebugOut((DEB_SCHEDLST, "_IN CheckNextRunDate\n"));
    NotfAssert((pTaskTrigger && pDate));
    HRESULT hr;
    SYSTEMTIME stBegin;  // now
    WORD cDate = 1;

    GetLocalTime(&stBegin);

    hr = GetRunTimes(*pTaskTrigger, pTaskData, &stBegin,  NULL, &cDate, pDate);
    if (cDate == 0)
    {
        // passed the time it was supposed to be triggered
        *pDate = GetCurrentDtTime();
        hr = E_FAIL;
    }

    NotfDebugOut((DEB_SCHEDLST, "OUT CheckNextRunDate (hr:%lx)\n", hr));
    return hr;
}


BOOL AreWeTheDefaultProcess()
{
    BOOL rc = FALSE;
    CDestinationPort defDest;

    if (g_pGlobalNotfMgr->GetDefaultDestinationPort(&defDest) == S_OK)
    {
        HWND defHwnd = defDest.GetPort();
        DWORD dwDefProcessID;
        GetWindowThreadProcessId(defHwnd, &dwDefProcessID);
        if (dwDefProcessID == GetCurrentProcessId())
        {
            rc = TRUE;
        }
    }

    return rc;
}

BOOL IsThereADefaultProcess()
{
    CDestinationPort defDest;

    return (g_pGlobalNotfMgr->GetDefaultDestinationPort(&defDest) == S_OK) ? TRUE : FALSE;
}

void PostSyncDefProcNotifications()
{
    if (!AreWeTheDefaultProcess())
    {
        CDestinationPort defDest;
        
        if (g_pGlobalNotfMgr->GetDefaultDestinationPort(&defDest) == S_OK)
        {
            PostMessage(defDest.GetPort(), WM_SYNC_DEF_PROC_NOTIFICATIONS, 0, 0);
        }
    }
}

HRESULT ReadFromStream(IStream *pStream, void *pv, ULONG cbRequired)
{
    HRESULT hr = E_FAIL;
    ULONG cbRead;


    if (pStream)
    {
        hr = pStream->Read(pv, cbRequired, &cbRead);

        if (SUCCEEDED(hr) && (cbRead != cbRequired))
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

HRESULT WriteToStream(IStream *pStream, void *pv, ULONG cbRequired)
{
    HRESULT hr = E_FAIL;
    ULONG cbRead;


    if (pStream)
    {
        hr = pStream->Write(pv, cbRequired, &cbRead);

        if (SUCCEEDED(hr) && (cbRead != cbRequired))
        {
            hr = E_FAIL;
        }
    }

    return hr;
}


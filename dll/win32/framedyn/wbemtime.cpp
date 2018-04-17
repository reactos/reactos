/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     WBEMTime class implementation
 * COPYRIGHT:   Copyright 2018 Jin Juanshi (2033537949@qq.com)
 */

#include <wbemtime.h>
#include <stdio.h>
#include <time.h>
#include <strsafe.h>

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR (SECONDS_PER_MINUTE * 60)
#define SECONDS_PER_DAY (SECONDS_PER_HOUR * 24)

#define MICROSECONDS_OFFSET 10
#define MILLISECONDS_OFFSET (MICROSECONDS_OFFSET * 1000)
#define SECONDS_OFFSET (MILLISECONDS_OFFSET * 1000)

#define DAYS_BETWEEN_TIME_T_AND_FILETIME 134774 //Between 01-01-1601 and 01-01-1970, there are exactly 134774 days.

static __inline void time_t_to_FILETIME(time_t t, LPFILETIME lpFileTime)
{
    ULARGE_INTEGER ulTime;
    ulTime.QuadPart = ((ULONGLONG)t + (ULONGLONG)DAYS_BETWEEN_TIME_T_AND_FILETIME * SECONDS_PER_DAY) * SECONDS_OFFSET;
    lpFileTime->dwLowDateTime = ulTime.LowPart;
    lpFileTime->dwHighDateTime = ulTime.HighPart;
}

static __inline void tm_to_SYSTEMTIME(const struct tm *lpTM, SYSTEMTIME *lpSystemTime)
{
    lpSystemTime->wYear = lpTM->tm_year + 1900;
    lpSystemTime->wMonth = lpTM->tm_mon + 1;
    lpSystemTime->wDay = lpTM->tm_mday;
    lpSystemTime->wHour = lpTM->tm_hour;
    lpSystemTime->wMinute = lpTM->tm_min;
    lpSystemTime->wSecond = lpTM->tm_sec;
}

static __inline void SYSTEMTIME_to_tm(SYSTEMTIME *lpSystemTime, struct tm *lpTM)
{
    lpTM->tm_year = lpSystemTime->wYear - 1900;
    lpTM->tm_mon = lpSystemTime->wMonth - 1;
    lpTM->tm_mday = lpSystemTime->wDay;
    lpTM->tm_hour = lpSystemTime->wHour;
    lpTM->tm_min = lpSystemTime->wMinute;
    lpTM->tm_sec = lpSystemTime->wSecond;
}

WBEMTimeSpan::WBEMTimeSpan()
{
    m_Time = INVALID_TIME;
}

WBEMTimeSpan::WBEMTimeSpan(const BSTR bstrDMTFFormat)
{
    *this = bstrDMTFFormat;
}

// constructor creates a time span object from an integer number of elapsed days.
WBEMTimeSpan::WBEMTimeSpan(
    int iDays,
    int iHours,
    int iMinutes,
    int iSeconds,
    int iMSec,
    int iUSec,
    int iNSec
)
{
    m_Time = iDays * SECONDS_PER_DAY + iHours * SECONDS_PER_HOUR + iMinutes * SECONDS_PER_MINUTE + iSeconds;
    m_Time *= SECONDS_OFFSET;
    m_Time += iMSec * MILLISECONDS_OFFSET + iUSec * MICROSECONDS_OFFSET + iNSec / 100;
}

// add operator adds one time span to another, placing the sum in a new WBEMTimeSpan object returned by the method.
WBEMTimeSpan WBEMTimeSpan::operator+(const WBEMTimeSpan &uAdd) const
{
    WBEMTimeSpan rt;

    if (IsOk() && uAdd.IsOk())
    {
        rt.m_Time = m_Time + uAdd.m_Time;
    }

    return rt;
}

// add-and-assign operator adds one time span to another. The operation create a new time span that contains the resulting time.
const WBEMTimeSpan& WBEMTimeSpan::operator+=(const WBEMTimeSpan &uAdd)
{
    if (IsOk() && uAdd.IsOk())
    {
        m_Time += uAdd.m_Time;
    }
    else
    {
        m_Time = INVALID_TIME;
    }

    return *this;
}

// subtract operator (–) subtracts a time span from the object on which the method is executed. The result is a new time span that contains the difference in time.
WBEMTimeSpan WBEMTimeSpan::operator-(const WBEMTimeSpan &uSub) const
{
    WBEMTimeSpan rt;

    if (IsOk() && uSub.IsOk() && m_Time >= uSub.m_Time)
    {
        rt.m_Time = m_Time - uSub.m_Time;
    }

    return rt;
}

// subtract and assign operator (–=).
const WBEMTimeSpan& WBEMTimeSpan::operator-=(const WBEMTimeSpan &uSub)
{
    if (IsOk() && uSub.IsOk() && m_Time >= uSub.m_Time)
    {
        m_Time -= uSub.m_Time;
    }
    else
    {
        m_Time = INVALID_TIME;
    }

    return *this;
}

// Converts a BSTR time interval value to a WBEMTimeSpan object in CIM date and time format.
const WBEMTimeSpan& WBEMTimeSpan::operator=(const BSTR bstrDMTFFormat)
{
    m_Time = INVALID_TIME;

    if (bstrDMTFFormat != NULL && wcslen(bstrDMTFFormat) == 25 && bstrDMTFFormat[14] == L'.' && bstrDMTFFormat[21] == L':')
    {
        int iDays, iHours, iMinutes, iSeconds, iUSec;
        if (swscanf(bstrDMTFFormat, L"%8d%2d%2d%2d.%6d:000", &iDays, &iHours, &iMinutes, &iSeconds, &iUSec) == 5)
        {
            WBEMTimeSpan t(iDays, iHours, iMinutes, iSeconds, 0, iUSec, 0);
            m_Time = t.m_Time;
        }
    }

    return *this;
}

BOOL WBEMTimeSpan::operator==(const WBEMTimeSpan &a) const
{
    return m_Time == a.m_Time;
}

BOOL WBEMTimeSpan::operator!=(const WBEMTimeSpan &a) const
{
    return m_Time != a.m_Time;
}

BOOL WBEMTimeSpan::operator<(const WBEMTimeSpan &a) const
{
    return m_Time < a.m_Time;
}

BOOL WBEMTimeSpan::operator<=(const WBEMTimeSpan &a) const
{
    return m_Time <= a.m_Time;
}

BOOL WBEMTimeSpan::operator>(const WBEMTimeSpan &a) const
{
    return m_Time > a.m_Time;
}

BOOL WBEMTimeSpan::operator>=(const WBEMTimeSpan &a) const
{
    return m_Time >= a.m_Time;
}

// gets the time span as a BSTR in Date and Time format.
BSTR WBEMTimeSpan::GetBSTR(void) const throw (CHeap_Exception)
{
    if (!IsOk())
    {
        return NULL;
    }

    ULONGLONG totalSeconds = m_Time / SECONDS_OFFSET;
    ULONGLONG iDays = totalSeconds / SECONDS_PER_DAY;
    int iHours = (totalSeconds % SECONDS_PER_DAY) / SECONDS_PER_HOUR;
    int iMinutes = (totalSeconds % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE;
    int iSeconds = totalSeconds % SECONDS_PER_MINUTE;
    int iUSec = (m_Time % SECONDS_OFFSET) / MICROSECONDS_OFFSET;

    BSTR ret = (BSTR)SysAllocStringLen(NULL, 26);
    if (StringCchPrintfW(ret, 26, L"%08I64i%02d%02d%02d.%06ld:000", iDays, iHours, iMinutes, iSeconds, iUSec) != S_OK)
    {
        return NULL;
    }
    return ret;
}

bool WBEMTimeSpan::IsOk() const
{
    return m_Time != INVALID_TIME;
}

ULONGLONG WBEMTimeSpan::GetTime() const
{
    return m_Time;
}

void  WBEMTimeSpan::Clear(void)
{
    m_Time = INVALID_TIME;
}

WBEMTimeSpan::WBEMTimeSpan(const FILETIME &ft)
{
    *this = ft;
}

WBEMTimeSpan::WBEMTimeSpan(const time_t & t)
{
    *this = t;
}

const WBEMTimeSpan& WBEMTimeSpan::operator=(const FILETIME &ft)
{
    ULARGE_INTEGER ulTime;
    ulTime.LowPart = ft.dwLowDateTime;
    ulTime.HighPart = ft.dwHighDateTime;
    m_Time = ulTime.QuadPart;
    return *this;
}

const WBEMTimeSpan& WBEMTimeSpan::operator=(const time_t &t)
{
    if (t < 0)
    {
        m_Time = INVALID_TIME;
    }
    else
    {
        m_Time = t * SECONDS_OFFSET;
    }
    return *this;
}

BOOL WBEMTimeSpan::Gettime_t(time_t *ptime_t) const
{
    if (IsOk())
    {
        *ptime_t = m_Time / SECONDS_OFFSET;
        return TRUE;
    }
    return FALSE;
}

BOOL WBEMTimeSpan::GetFILETIME(FILETIME *pst) const
{
    if (IsOk())
    {
        ULARGE_INTEGER ulTime;
        ulTime.QuadPart = m_Time;
        pst->dwLowDateTime = ulTime.LowPart;
        pst->dwHighDateTime = ulTime.HighPart;

        return TRUE;
    }
    return FALSE;
}

WBEMTime::WBEMTime()
{
    m_uTime = INVALID_TIME;
}

WBEMTime::WBEMTime(const BSTR bstrDMTFFormat)
{
    *this = bstrDMTFFormat;
}

WBEMTime::WBEMTime(const SYSTEMTIME &st)
{
    *this = st;
}

WBEMTime::WBEMTime(const FILETIME &ft)
{
    *this = ft;
}

WBEMTime::WBEMTime(const struct tm &tmin)
{
    *this = tmin;
}

WBEMTime::WBEMTime(const time_t &t)
{
    *this = t;
}

WBEMTime WBEMTime::operator+(const WBEMTimeSpan &uAdd) const
{
    WBEMTime rt;

    if (IsOk() && uAdd.IsOk())
    {
        rt.m_uTime = m_uTime + uAdd.m_Time;
    }

    return rt;
}

const WBEMTime& WBEMTime::operator+=(const WBEMTimeSpan &ts)
{
    if (IsOk() && ts.IsOk())
    {
        m_uTime += ts.m_Time;
    }
    else
    {
        m_uTime = INVALID_TIME;
    }

    return *this;
}

WBEMTimeSpan WBEMTime::operator-(const WBEMTime &sub)
{
    WBEMTimeSpan rt;

    if (IsOk() && sub.IsOk() && m_uTime >= sub.m_uTime)
    {
        rt.m_Time = m_uTime - sub.m_uTime;
    }

    return rt;
}

WBEMTime WBEMTime::operator-(const WBEMTimeSpan &sub) const
{
    WBEMTime rt;

    if (IsOk() && sub.IsOk() && m_uTime >= sub.m_Time)
    {
        rt.m_uTime = m_uTime - sub.m_Time;
    }

    return rt;
}

const WBEMTime& WBEMTime::operator-=(const WBEMTimeSpan &sub)
{
    if (IsOk() && sub.IsOk() && m_uTime >= sub.m_Time)
    {
        m_uTime -= sub.m_Time;
    }
    else
    {
        m_uTime = INVALID_TIME;
    }

    return *this;
}

// assignment operator overload method takes a CIM date time format parameter.
const WBEMTime& WBEMTime::operator=(const BSTR bstrDMTFFormat)
{
    m_uTime = INVALID_TIME;

    if (bstrDMTFFormat != NULL && wcslen(bstrDMTFFormat) == 25 && bstrDMTFFormat[14] == L'.')
    {
        if (bstrDMTFFormat[21] == L'+' || bstrDMTFFormat[21] == L'-')
        {
            SetDMTF(bstrDMTFFormat);
        }
    }

    return *this;
}

// assignment operators have been overloaded to facilitate conversions between various Windows and ANSI C run-time time formats.
const WBEMTime& WBEMTime::operator=(const SYSTEMTIME &st)
{
    FILETIME ft;

    if (SystemTimeToFileTime(&st, &ft))
    {
        *this = ft;
    }
    else
    {
        m_uTime = INVALID_TIME;
    }

    return *this;
}

// assignment operator overload method takes a FILETIME parameter. The WBEMTime class assignment operators have been overloaded to facilitate conversions between various Windows and ANSI C run-time time formats.
const WBEMTime& WBEMTime::operator=(const FILETIME &ft)
{
    ULARGE_INTEGER ulTime;
    ulTime.LowPart = ft.dwLowDateTime;
    ulTime.HighPart = ft.dwHighDateTime;
    m_uTime = ulTime.QuadPart;
    return *this;
}

// assignment operators have been overloaded to facilitate conversions between various Windows and ANSI C run-time time formats.
const WBEMTime& WBEMTime::operator=(const struct tm &tmin)
{
    SYSTEMTIME sysTime;

    tm_to_SYSTEMTIME(&tmin, &sysTime);

    *this = sysTime;

    return *this;
}

// assignment operators have been overloaded to facilitate conversions between various Windows and ANSI C run-time time formats.
const WBEMTime& WBEMTime::operator=(const time_t &t)
{
    FILETIME ft;
    time_t_to_FILETIME(t, &ft);

    *this = ft;
    return *this;
}

BOOL WBEMTime::operator==(const WBEMTime &a) const
{
    return m_uTime == a.m_uTime;
}

BOOL WBEMTime::operator!=(const WBEMTime &a) const
{
    return m_uTime != a.m_uTime;
}

BOOL WBEMTime::operator<(const WBEMTime &a) const
{
    return m_uTime < a.m_uTime;
}

BOOL WBEMTime::operator<=(const WBEMTime &a) const
{
    return m_uTime <= a.m_uTime;
}

BOOL WBEMTime::operator>(const WBEMTime &a) const
{
    return m_uTime > a.m_uTime;
}

BOOL WBEMTime::operator>=(const WBEMTime &a) const
{
    return m_uTime >= a.m_uTime;
}

// gets the time as a BSTR value in CIM Date and Time Format.
BSTR WBEMTime::GetBSTR(void) const throw (CHeap_Exception)
{
    return GetDMTF(false);
}

// gets the time as an ANSI C run-time struct tm structure.
BOOL WBEMTime::GetStructtm(struct tm *ptm) const
{
    if (ptm == NULL)
    {
        return FALSE;
    }

    SYSTEMTIME sysTime;
    if (!GetSYSTEMTIME(&sysTime) || sysTime.wYear < 1900)
    {
        return FALSE;
    }

    SYSTEMTIME_to_tm(&sysTime, ptm);

    return TRUE;
}

// gets the time as an MFC SYSTEMTIME structure.
BOOL WBEMTime::GetSYSTEMTIME(SYSTEMTIME *pst) const
{
    FILETIME ft;
    if (!GetFILETIME(&ft))
    {
        return FALSE;
    }

    return FileTimeToSystemTime(&ft, pst);
}

// gets the time as an MFC FILETIME structure.
BOOL WBEMTime::GetFILETIME(FILETIME *pst) const
{
    if (pst == NULL || !IsOk())
    {
        return FALSE;
    }

    ULARGE_INTEGER ulTime;
    ulTime.QuadPart = m_uTime;
    pst->dwLowDateTime = ulTime.LowPart;
    pst->dwHighDateTime = ulTime.HighPart;

    return TRUE;
}

// sets the time in the WBEMTime object. The time is given by its BSTR parameter in Date and Time Format. A date and time argument earlier than midnight January 1, 1601 is not valid.
BOOL WBEMTime::SetDMTF(const BSTR wszText)
{
    int iYears, iMonths, iDays, iHours, iMinutes, iSeconds, iUSecs, iOffsetInMinutes;
    WCHAR cSign;

    if (swscanf(wszText, L"%4d%2d%2d%2d%2d%2d.%6d%c%3d", &iYears, &iMonths, &iDays, &iHours, &iMinutes, &iSeconds, &iUSecs, &cSign, &iOffsetInMinutes) != 9)
    {
        return FALSE;
    }

    SYSTEMTIME sysTime;
    sysTime.wYear = iYears;
    sysTime.wMonth = iMonths;
    sysTime.wDay = iDays;
    sysTime.wHour = iHours;
    sysTime.wMinute = iMinutes;
    sysTime.wSecond = iSeconds;
    *this = sysTime;

    m_uTime += iUSecs * MICROSECONDS_OFFSET;
    if (cSign == '+')
    {
        m_uTime += iOffsetInMinutes * SECONDS_PER_MINUTE * SECONDS_OFFSET;
    }
    else
    {
        m_uTime -= iOffsetInMinutes * SECONDS_PER_MINUTE * SECONDS_OFFSET;
    }

    return TRUE;
}

// converts a BSTR value to CIM Date and Time Format.
BSTR WBEMTime::GetDMTF(BOOL bLocal) const throw (CHeap_Exception)
{
    if (!IsOk())
    {
        return NULL;
    }

    SYSTEMTIME sysTime;
    if (!GetSYSTEMTIME(&sysTime))
    {
        return NULL;
    }

    int offsetInMinutes = 0;
    WCHAR cSign = L'+';

    if (bLocal && m_uTime >= (ULONGLONG)12 * SECONDS_PER_HOUR * SECONDS_OFFSET)
    {
        offsetInMinutes = GetLocalOffsetForDate(&sysTime);

        if (offsetInMinutes < 0)
        {
            cSign = L'-';
            offsetInMinutes = -offsetInMinutes;
            WBEMTime t = *this - WBEMTimeSpan(0, 0, offsetInMinutes, 0);
            t.GetSYSTEMTIME(&sysTime);
        }
        else
        {
            WBEMTime t = *this + WBEMTimeSpan(0, 0, offsetInMinutes, 0);
            t.GetSYSTEMTIME(&sysTime);
        }
    }

    BSTR ret = (BSTR)SysAllocStringLen(NULL, 26);
    if (StringCchPrintfW(
        ret,
        26,
        L"%04.4d%02.2d%02.2d%02.2d%02.2d%02.2d.%06.6d%c%03.3ld",
        sysTime.wYear,
        sysTime.wMonth,
        sysTime.wDay,
        sysTime.wHour,
        sysTime.wMinute,
        sysTime.wSecond,
        (int)((m_uTime % SECONDS_OFFSET) / MICROSECONDS_OFFSET),
        cSign,
        offsetInMinutes) != S_OK)
    {
        return NULL;
    }

    return ret;
}

void WBEMTime::Clear(void)
{
    m_uTime = INVALID_TIME;
}

bool WBEMTime::IsOk() const
{
    return m_uTime != INVALID_TIME;
}

ULONGLONG WBEMTime::GetTime() const
{
    return m_uTime;
}

// gets a DMTF date in a CIM Date and Time Format from a FAT that does not have time zones.
BSTR WBEMTime::GetDMTFNonNtfs(void) const
{
    FILETIME fileTime, localFileTime;
    if (!GetFILETIME(&fileTime))
    {
        return NULL;
    }

    if (!FileTimeToLocalFileTime(&fileTime, &localFileTime))
    {
        return NULL;
    }

    WBEMTime localWBEMTime = localFileTime;
    BSTR str = localWBEMTime.GetDMTF(false);
    if (!str)
    {
        return NULL;
    }
    str[21] = L'+';
    str[22] = L'*';
    str[23] = L'*';
    str[24] = L'*';

    return str;
}

// returns the offset in minutes (+ or –) between GMT and local time for the ANSI C tm structure supplied in the argument.
LONG WBEMTime::GetLocalOffsetForDate(const struct tm *tmin)
{
    SYSTEMTIME sysTime;
    tm_to_SYSTEMTIME(tmin, &sysTime);

    return GetLocalOffsetForDate(&sysTime);
}

// returns the offset in minutes (+ or –) between GMT and local time for the SYSTEMTIME supplied in the argument.
LONG WBEMTime::GetLocalOffsetForDate(const SYSTEMTIME *pst)
{
    struct _TIME_ZONE_INFORMATION timeZoneInfo;
    GetTimeZoneInformation(&timeZoneInfo);

    return -timeZoneInfo.Bias;
}

// returns the offset in minutes (+ or –) between GMT and local time for the FILETIME supplied in the argument.
LONG WBEMTime::GetLocalOffsetForDate(const FILETIME *pft)
{
    SYSTEMTIME sysTime;
    FileTimeToSystemTime(pft, &sysTime);

    return GetLocalOffsetForDate(&sysTime);
}

// returns the offset in minutes (+ or –) between GMT and local time for the ANSI C time_t structure supplied in the argument.
LONG WBEMTime::GetLocalOffsetForDate(const time_t &t)
{
    FILETIME ft;
    time_t_to_FILETIME(t, &ft);

    return GetLocalOffsetForDate(&ft);
}

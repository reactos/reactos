// PROJECT:        ReactOS ATL CTime, CFileTime, CTimeSpan, CFileTimeSpan
// LICENSE:        Public Domain
// PURPOSE:        Provides compatibility to Microsoft ATL
// PROGRAMMERS:    Benedikt Freisen

#ifndef __ATLTIME_H__
#define __ATLTIME_H__

// WARNING: Untested code

#pragma once

#include <atlcore.h>
#include <windows.h>
#include <atlstr.h>
#include <time.h>
#include <oledb.h>

namespace ATL
{

class CTimeSpan
{
    __time64_t m_nSpan;
public:
    CTimeSpan() throw()
    {
        // leave uninitialized
    }

    CTimeSpan(__time64_t time) throw()
    {
        m_nSpan = time;
    }

    CTimeSpan(LONG lDays, int nHours, int nMins, int nSecs) throw()
    {
        ATLASSERT(lDays >= 0 && nHours >= 0 && nHours <= 23 && nMins >= 0 && nMins <= 59 && nSecs >= 0 && nSecs <= 59);
        m_nSpan = ((((LONGLONG)lDays) * 24 + nHours) * 60 + nMins) * 60 + nSecs;
    }

    CString Format(LPCSTR pFormat) const
    {
        struct tm time;
        _localtime64_s(&time, &m_nSpan);
        CStringA strTime;
        strftime(strTime.GetBuffer(256), 256, pFormat, &time);
        strTime.ReleaseBuffer();
        return CString(strTime);
    }

    CString Format(LPCTSTR pszFormat) const
    {
        struct tm time;
        _localtime64_s(&time, &m_nSpan);
        CString strTime;
#ifdef UNICODE
        wcsftime(strTime.GetBuffer(256), 256, pszFormat, &time);
#else
        strftime(strTime.GetBuffer(256), 256, pszFormat, &time);
#endif
        strTime.ReleaseBuffer();
        return strTime;
    }

    CString Format(UINT nID) const
    {
        struct tm time;
        _localtime64_s(&time, &m_nSpan);
        CString strFormat;
        strFormat.LoadString(nID);
        CString strTime;
#ifdef UNICODE
        wcsftime(strTime.GetBuffer(256), 256, strFormat, &time);
#else
        strftime(strTime.GetBuffer(256), 256, strFormat, &time);
#endif
        strTime.ReleaseBuffer();
        return strTime;
    }

    LONGLONG GetTotalHours() const throw()
    {
        return m_nSpan / 60 / 60;
    }

    LONGLONG GetTotalMinutes() const throw()
    {
        return m_nSpan / 60;
    }

    LONGLONG GetTotalSeconds() const throw()
    {
        return m_nSpan;
    }

    LONGLONG GetDays() const throw()
    {
        return m_nSpan / 60 / 60 / 24;
    }

    LONG GetHours() const throw()
    {
        return GetTotalHours() - GetDays() * 24;
    }

    LONG GetMinutes() const throw()
    {
        return GetTotalMinutes() - GetTotalHours() * 60;
    }

    LONG GetSeconds() const throw()
    {
        return GetTotalSeconds() - GetTotalMinutes() * 60;
    }

    __time64_t GetTimeSpan() const throw()
    {
        return m_nSpan;
    }

//     CArchive& Serialize64(CArchive& ar)  // MFC only
//     {
//         // TODO
//     }

};

class CTime
{
    __time64_t m_nTime;
public:
    CTime() throw()
    {
        // leave uninitialized
    }

    CTime(__time64_t time) throw()
    {
        m_nTime = time;
    }

    CTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = -1)
    {
        struct tm time;
        time.tm_year = nYear;
        time.tm_mon = nMonth;
        time.tm_mday = nDay;
        time.tm_hour = nHour;
        time.tm_min = nMin;
        time.tm_sec = nSec;
        time.tm_isdst = nDST;
        m_nTime = _mktime64(&time);
    }

    CTime(WORD wDosDate, WORD wDosTime, int nDST = -1)
    {
        FILETIME ft;
        DosDateTimeToFileTime(wDosDate, wDosTime, &ft);
        SYSTEMTIME st;
        FileTimeToSystemTime(&ft, &st);
        struct tm time;
        time.tm_year = st.wYear;
        time.tm_mon = st.wMonth;
        time.tm_wday = st.wDayOfWeek;
        time.tm_hour = st.wHour;
        time.tm_min = st.wMinute;
        time.tm_sec = st.wSecond;
        time.tm_isdst = nDST;
        m_nTime = _mktime64(&time);
    }

    CTime(const SYSTEMTIME& st, int nDST = -1) throw()
    {
        struct tm time;
        time.tm_year = st.wYear;
        time.tm_mon = st.wMonth;
        time.tm_wday = st.wDayOfWeek;
        time.tm_hour = st.wHour;
        time.tm_min = st.wMinute;
        time.tm_sec = st.wSecond;
        time.tm_isdst = nDST;
        m_nTime = _mktime64(&time);
    }

    CTime(const FILETIME& ft, int nDST = -1)
    {
        SYSTEMTIME st;
        FileTimeToSystemTime(&ft, &st);
        struct tm time;
        time.tm_year = st.wYear;
        time.tm_mon = st.wMonth;
        time.tm_wday = st.wDayOfWeek;
        time.tm_hour = st.wHour;
        time.tm_min = st.wMinute;
        time.tm_sec = st.wSecond;
        time.tm_isdst = nDST;
        m_nTime = _mktime64(&time);
    }

    CTime(const DBTIMESTAMP& dbts, int nDST = -1) throw()
    {
        struct tm time;
        time.tm_year = dbts.year;
        time.tm_mon = dbts.month;
        time.tm_hour = dbts.hour;
        time.tm_min = dbts.minute;
        time.tm_sec = dbts.second;
        time.tm_isdst = nDST;
        m_nTime = _mktime64(&time);
    }

    CString Format(LPCTSTR pszFormat) const
    {
        struct tm time;
        _localtime64_s(&time, &m_nTime);
        CString strTime;
#ifdef UNICODE
        wcsftime(strTime.GetBuffer(256), 256, pszFormat, &time);
#else
        strftime(strTime.GetBuffer(256), 256, pszFormat, &time);
#endif
        strTime.ReleaseBuffer();
        return strTime;
    }

    CString Format(UINT nFormatID) const
    {
        struct tm time;
        _localtime64_s(&time, &m_nTime);
        CString strFormat;
        strFormat.LoadString(nFormatID);
        CString strTime;
#ifdef UNICODE
        wcsftime(strTime.GetBuffer(256), 256, strFormat, &time);
#else
        strftime(strTime.GetBuffer(256), 256, strFormat, &time);
#endif
        strTime.ReleaseBuffer();
        return strTime;
    }

    CString FormatGmt(LPCTSTR pszFormat) const
    {
        struct tm time;
        _gmtime64_s(&time, &m_nTime);
        CString strTime;
#ifdef UNICODE
        wcsftime(strTime.GetBuffer(256), 256, pszFormat, &time);
#else
        strftime(strTime.GetBuffer(256), 256, pszFormat, &time);
#endif
        strTime.ReleaseBuffer();
        return strTime;
    }

    CString FormatGmt(UINT nFormatID) const
    {
        struct tm time;
        _gmtime64_s(&time, &m_nTime);
        CString strFormat;
        strFormat.LoadString(nFormatID);
        CString strTime;
#ifdef UNICODE
        wcsftime(strTime.GetBuffer(256), 256, strFormat, &time);
#else
        strftime(strTime.GetBuffer(256), 256, strFormat, &time);
#endif
        strTime.ReleaseBuffer();
        return strTime;
    }

    bool GetAsDBTIMESTAMP(DBTIMESTAMP& dbts) const throw()
    {
        struct tm time;
        _gmtime64_s(&time, &m_nTime);
        dbts.year = time.tm_year;
        dbts.month = time.tm_mon;
        dbts.day = time.tm_mday;
        dbts.hour = time.tm_hour;
        dbts.minute = time.tm_min;
        dbts.second = time.tm_sec;
        dbts.fraction = 0;
        return true;  // TODO: error handling?
    }

    bool GetAsSystemTime(SYSTEMTIME& st) const throw()
    {
        struct tm time;
        _gmtime64_s(&time, &m_nTime);
        st.wYear = time.tm_year;
        st.wMonth = time.tm_mon;
        st.wDayOfWeek = time.tm_wday;
        st.wDay = time.tm_mday;
        st.wHour = time.tm_hour;
        st.wMinute = time.tm_min;
        st.wSecond = time.tm_sec;
        st.wMilliseconds = 0;
        return true;  // TODO: error handling?
    }

    static CTime WINAPI GetCurrentTime() throw()
    {
        __time64_t time;
        _time64(&time);
        return CTime(time);
    }

    int GetDay() const throw()
    {
        struct tm time;
        _localtime64_s(&time, &m_nTime);
        return time.tm_mday;
    }

    int GetDayOfWeek() const throw()
    {
        struct tm time;
        _localtime64_s(&time, &m_nTime);
        return time.tm_wday;
    }

    struct tm* GetGmtTm(struct tm* ptm) const
    {
        _gmtime64_s(ptm, &m_nTime);
        return ptm;
    }

    int GetHour() const throw()
    {
        struct tm time;
        _localtime64_s(&time, &m_nTime);
        return time.tm_hour;
    }

    struct tm* GetLocalTm(struct tm* ptm) const
    {
        _localtime64_s(ptm, &m_nTime);
        return ptm;
    }

    int GetMinute() const throw()
    {
        struct tm time;
        _localtime64_s(&time, &m_nTime);
        return time.tm_min;
    }

    int GetMonth() const throw()
    {
        struct tm time;
        _localtime64_s(&time, &m_nTime);
        return time.tm_mon;
    }

    int GetSecond() const throw()
    {
        struct tm time;
        _localtime64_s(&time, &m_nTime);
        return time.tm_sec;
    }

    __time64_t GetTime() const throw()
    {
        return m_nTime;
    }

    int GetYear()
    {
        struct tm time;
        _localtime64_s(&time, &m_nTime);
        return time.tm_year;
    }

//     CArchive& Serialize64(CArchive& ar)  // MFC only
//     {
//         // TODO
//     }

    CTime operator+(CTimeSpan timeSpan) const throw()
    {
        return CTime(m_nTime + timeSpan.GetTimeSpan());
    }

    CTime operator-(CTimeSpan timeSpan) const throw()
    {
        return CTime(m_nTime - timeSpan.GetTimeSpan());
    }

    CTimeSpan operator-(CTime time) const throw()
    {
        return CTimeSpan(m_nTime - time.GetTime());
    }

    CTime& operator+=(CTimeSpan span) throw()
    {
        m_nTime += span.GetTimeSpan();
        return *this;
    }

    CTime& operator-=(CTimeSpan span) throw()
    {
        m_nTime -= span.GetTimeSpan();
        return *this;
    }

    CTime& operator=(__time64_t time) throw()
    {
        m_nTime = time;
        return *this;
    }

    bool operator==(CTime time) const throw()
    {
        return m_nTime == time.GetTime();
    }

    bool operator!=(CTime time) const throw()
    {
        return m_nTime != time.GetTime();
    }

    bool operator<(CTime time) const throw()
    {
        return m_nTime < time.GetTime();
    }

    bool operator>(CTime time) const throw()
    {
        return m_nTime > time.GetTime();
    }

    bool operator<=(CTime time) const throw()
    {
        return m_nTime <= time.GetTime();
    }

    bool operator>=(CTime time) const throw()
    {
        return m_nTime >= time.GetTime();
    }

};

class CFileTimeSpan
{
    LONGLONG m_nSpan;
public:
    CFileTimeSpan() throw()
    {
        m_nSpan = 0;
    }

    CFileTimeSpan(const CFileTimeSpan& span) throw()
    {
        m_nSpan = span.GetTimeSpan();
    }

    CFileTimeSpan(LONGLONG nSpan) throw()
    {
        m_nSpan = nSpan;
    }

    LONGLONG GetTimeSpan() const throw()
    {
        return m_nSpan;
    }

    void SetTimeSpan(LONGLONG nSpan) throw()
    {
        m_nSpan = nSpan;
    }

    CFileTimeSpan operator-(CFileTimeSpan span) const throw()
    {
        return CFileTimeSpan(m_nSpan - span.GetTimeSpan());
    }

    bool operator!=(CFileTimeSpan span) const throw()
    {
        return m_nSpan != span.GetTimeSpan();
    }

    CFileTimeSpan operator+(CFileTimeSpan span) const throw()
    {
        return CFileTimeSpan(m_nSpan + span.GetTimeSpan());
    }

    CFileTimeSpan& operator+=(CFileTimeSpan span) throw()
    {
        m_nSpan += span.GetTimeSpan();
        return *this;
    }

    bool operator<(CFileTimeSpan span) const throw()
    {
        return m_nSpan < span.GetTimeSpan();
    }

    bool operator<=(CFileTimeSpan span) const throw()
    {
        return m_nSpan <= span.GetTimeSpan();
    }

    CFileTimeSpan& operator=(const CFileTimeSpan& span) throw()
    {
        m_nSpan = span.GetTimeSpan();
        return *this;
    }

    CFileTimeSpan& operator-=(CFileTimeSpan span) throw()
    {
        m_nSpan -= span.GetTimeSpan();
        return *this;
    }

    bool operator==(CFileTimeSpan span) const throw()
    {
        return m_nSpan == span.GetTimeSpan();
    }

    bool operator>(CFileTimeSpan span) const throw()
    {
        return m_nSpan > span.GetTimeSpan();
    }

    bool operator>=(CFileTimeSpan span) const throw()
    {
        return m_nSpan >= span.GetTimeSpan();
    }

};

class CFileTime : public FILETIME
{
public:
    static const ULONGLONG Millisecond = 10000;
    static const ULONGLONG Second = Millisecond * 1000;
    static const ULONGLONG Minute = Second * 60;
    static const ULONGLONG Hour = Minute * 60;
    static const ULONGLONG Day = Hour * 24;
    static const ULONGLONG Week = Day * 7;

    CFileTime() throw()
    {
        this->dwLowDateTime = 0;
        this->dwHighDateTime = 0;
    }

    CFileTime(const FILETIME& ft) throw()
    {
        this->dwLowDateTime = ft.dwLowDateTime;
        this->dwHighDateTime = ft.dwHighDateTime;
    }

    CFileTime(ULONGLONG nTime) throw()
    {
        this->dwLowDateTime = (DWORD) nTime;
        this->dwHighDateTime = nTime >> 32;
    }

    static CFileTime GetCurrentTime() throw()
    {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        return CFileTime(ft);
    }

    ULONGLONG GetTime() const throw()
    {
        return ((ULONGLONG)this->dwLowDateTime) | (((ULONGLONG)this->dwHighDateTime) << 32);
    }

    CFileTime LocalToUTC() const throw()
    {
        FILETIME ft;
        LocalFileTimeToFileTime(this, &ft);
        return CFileTime(ft);
    }

    void SetTime(ULONGLONG nTime) throw()
    {
        this->dwLowDateTime = (DWORD) nTime;
        this->dwHighDateTime = nTime >> 32;
    }

    CFileTime UTCToLocal() const throw()
    {
        FILETIME ft;
        FileTimeToLocalFileTime(this, &ft);
        return CFileTime(ft);
    }

    CFileTime operator-(CFileTimeSpan span) const throw()
    {
        return CFileTime(this->GetTime() - span.GetTimeSpan());
    }

    CFileTimeSpan operator-(CFileTime ft) const throw()
    {
        return CFileTimeSpan(this->GetTime() - ft.GetTime());
    }

    bool operator!=(CFileTime ft) const throw()
    {
        return this->GetTime() != ft.GetTime();
    }

    CFileTime operator+(CFileTimeSpan span) const throw()
    {
        return CFileTime(this->GetTime() + span.GetTimeSpan());
    }

    CFileTime& operator+=(CFileTimeSpan span) throw()
    {
        this->SetTime(this->GetTime() + span.GetTimeSpan());
        return *this;
    }

    bool operator<(CFileTime ft) const throw()
    {
        return this->GetTime() < ft.GetTime();
    }

    bool operator<=(CFileTime ft) const throw()
    {
        return this->GetTime() <= ft.GetTime();
    }

    CFileTime& operator=(const FILETIME& ft) throw()
    {
        this->dwLowDateTime = ft.dwLowDateTime;
        this->dwHighDateTime = ft.dwHighDateTime;
        return *this;
    }

    CFileTime& operator-=(CFileTimeSpan span) throw()
    {
        this->SetTime(this->GetTime() - span.GetTimeSpan());
        return *this;
    }

    bool operator==(CFileTime ft) const throw()
    {
        return this->GetTime() == ft.GetTime();
    }

    bool operator>(CFileTime ft) const throw()
    {
        return this->GetTime() > ft.GetTime();
    }

    bool operator>=(CFileTime ft) const throw()
    {
        return this->GetTime() >= ft.GetTime();
    }

};

}  // namespace ATL

#endif

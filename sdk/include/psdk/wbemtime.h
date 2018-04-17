#pragma once

#ifndef _WBEMTIME_H
#define _WBEMTIME_H

#include <windows.h>
#include <provexce.h>

#define INVALID_TIME 0xffffffffffffffff

// This class holds time span values. 

class WBEMTimeSpan
{
private:

    ULONGLONG m_Time;
    friend class WBEMTime;

public:

    WBEMTimeSpan();
    WBEMTimeSpan(const BSTR bstrDMTFFormat);
    WBEMTimeSpan(
        int iDays,
        int iHours,
        int iMinutes,
        int iSeconds,
        int iMSec = 0,
        int iUSec = 0,
        int iNSec = 0
    );

    WBEMTimeSpan operator+(const WBEMTimeSpan &uAdd) const;
    const WBEMTimeSpan &operator+=(const WBEMTimeSpan &uAdd);

    WBEMTimeSpan operator-(const WBEMTimeSpan &uSub) const;
    const WBEMTimeSpan &operator-=(const WBEMTimeSpan &uSub);

    const WBEMTimeSpan &operator=(const BSTR pDMTFFormat);

    BOOL operator==(const WBEMTimeSpan &a) const;
    BOOL operator!=(const WBEMTimeSpan &a) const;
    BOOL operator<(const WBEMTimeSpan &a) const;
    BOOL operator<=(const WBEMTimeSpan &a) const;
    BOOL operator>(const WBEMTimeSpan &a) const;
    BOOL operator>=(const WBEMTimeSpan &a) const;

    BSTR GetBSTR(void) const throw (CHeap_Exception);

    bool IsOk() const;
    ULONGLONG GetTime() const;
    void Clear(void);

    // These are all deprecated
    WBEMTimeSpan(const FILETIME &ft);
    WBEMTimeSpan(const time_t &t);
    const WBEMTimeSpan &operator=(const FILETIME &ft);
    const WBEMTimeSpan &operator=(const time_t &t);
    BOOL Gettime_t(time_t *ptime_t) const;
    BOOL GetFILETIME(FILETIME *pst) const;
};

// This class holds time values. 

class WBEMTime
{
public:

    WBEMTime();
    WBEMTime(const BSTR bstrDMTFFormat);
    WBEMTime(const SYSTEMTIME &st);
    WBEMTime(const FILETIME &ft);
    WBEMTime(const struct tm &tmin);
    WBEMTime(const time_t &t);

    WBEMTime operator+(const WBEMTimeSpan &uAdd) const;
    const WBEMTime &operator+=(const WBEMTimeSpan &ts);

    WBEMTimeSpan operator-(const WBEMTime &sub);

    WBEMTime operator-(const WBEMTimeSpan &sub) const;
    const WBEMTime &operator-=(const WBEMTimeSpan &sub);

    const WBEMTime &operator=(const BSTR bstrDMTFFormat);
    const WBEMTime &operator=(const SYSTEMTIME &st);
    const WBEMTime &operator=(const FILETIME &ft);
    const WBEMTime &operator=(const struct tm &tmin);
    const WBEMTime &operator=(const time_t &t);

    BOOL operator==(const WBEMTime &a) const;
    BOOL operator!=(const WBEMTime &a) const;
    BOOL operator<(const WBEMTime &a) const;
    BOOL operator<=(const WBEMTime &a) const;
    BOOL operator>(const WBEMTime &a) const;
    BOOL operator>=(const WBEMTime &a) const;

    BSTR GetBSTR(void) const throw (CHeap_Exception);
    BOOL GetStructtm(struct tm *ptm) const;
    BOOL Gettime_t(time_t *ptime_t) const;
    BOOL GetSYSTEMTIME(SYSTEMTIME *pst) const;
    BOOL GetFILETIME(FILETIME *pst) const;

    BOOL SetDMTF(const BSTR wszText);
    BSTR GetDMTF(BOOL bLocal = FALSE) const throw (CHeap_Exception);

    BSTR GetDMTFNonNtfs(void) const;

    void Clear(void);

    bool IsOk() const;
    ULONGLONG GetTime() const;

    static LONG WINAPI GetLocalOffsetForDate(const struct tm *tmin);
    static LONG WINAPI GetLocalOffsetForDate(const SYSTEMTIME *pst);
    static LONG WINAPI GetLocalOffsetForDate(const FILETIME *pft);
    static LONG WINAPI GetLocalOffsetForDate(const time_t &t);

private:
    ULONGLONG m_uTime;
};

#endif

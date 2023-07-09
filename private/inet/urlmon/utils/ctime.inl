// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFX.H

#ifdef _AFX_INLINE

// CTime and CTimeSpan
_AFX_INLINE CTimeSpan::CTimeSpan()
        { }
_AFX_INLINE CTimeSpan::CTimeSpan(time_t time)
        { m_timeSpan = time; }
_AFX_INLINE CTimeSpan::CTimeSpan(LONG lDays, int nHours, int nMins, int nSecs)
        { m_timeSpan = nSecs + 60* (nMins + 60* (nHours + 24* lDays)); }
_AFX_INLINE CTimeSpan::CTimeSpan(const CTimeSpan& timeSpanSrc)
        { m_timeSpan = timeSpanSrc.m_timeSpan; }
_AFX_INLINE const CTimeSpan& CTimeSpan::operator=(const CTimeSpan& timeSpanSrc)
        { m_timeSpan = timeSpanSrc.m_timeSpan; return *this; }
_AFX_INLINE LONG CTimeSpan::GetDays() const
        { return (LONG)(m_timeSpan / (24*3600L)); }
_AFX_INLINE LONG CTimeSpan::GetTotalHours() const
        { return (LONG)(m_timeSpan/3600); }
_AFX_INLINE int CTimeSpan::GetHours() const
        { return (int)(GetTotalHours() - GetDays()*24); }
_AFX_INLINE LONG CTimeSpan::GetTotalMinutes() const
        { return (LONG)(m_timeSpan/60); }
_AFX_INLINE int CTimeSpan::GetMinutes() const
        { return (int)(GetTotalMinutes() - GetTotalHours()*60); }
_AFX_INLINE LONG_PTR CTimeSpan::GetTotalSeconds() const
        { return m_timeSpan; }
_AFX_INLINE int CTimeSpan::GetSeconds() const
        { return (int)(GetTotalSeconds() - GetTotalMinutes()*60); }
_AFX_INLINE CTimeSpan CTimeSpan::operator-(CTimeSpan timeSpan) const
        { return CTimeSpan(m_timeSpan - timeSpan.m_timeSpan); }
_AFX_INLINE CTimeSpan CTimeSpan::operator+(CTimeSpan timeSpan) const
        { return CTimeSpan(m_timeSpan + timeSpan.m_timeSpan); }
_AFX_INLINE const CTimeSpan& CTimeSpan::operator+=(CTimeSpan timeSpan)
        { m_timeSpan += timeSpan.m_timeSpan; return *this; }
_AFX_INLINE const CTimeSpan& CTimeSpan::operator-=(CTimeSpan timeSpan)
        { m_timeSpan -= timeSpan.m_timeSpan; return *this; }
_AFX_INLINE BOOL CTimeSpan::operator==(CTimeSpan timeSpan) const
        { return m_timeSpan == timeSpan.m_timeSpan; }
_AFX_INLINE BOOL CTimeSpan::operator!=(CTimeSpan timeSpan) const
        { return m_timeSpan != timeSpan.m_timeSpan; }
_AFX_INLINE BOOL CTimeSpan::operator<(CTimeSpan timeSpan) const
        { return m_timeSpan < timeSpan.m_timeSpan; }
_AFX_INLINE BOOL CTimeSpan::operator>(CTimeSpan timeSpan) const
        { return m_timeSpan > timeSpan.m_timeSpan; }
_AFX_INLINE BOOL CTimeSpan::operator<=(CTimeSpan timeSpan) const
        { return m_timeSpan <= timeSpan.m_timeSpan; }
_AFX_INLINE BOOL CTimeSpan::operator>=(CTimeSpan timeSpan) const
        { return m_timeSpan >= timeSpan.m_timeSpan; }


_AFX_INLINE CTime::CTime()
        { }
_AFX_INLINE CTime::CTime(time_t time)
        { m_time = time; }
_AFX_INLINE CTime::CTime(const CTime& timeSrc)
        { m_time = timeSrc.m_time; }
_AFX_INLINE const CTime& CTime::operator=(const CTime& timeSrc)
        { m_time = timeSrc.m_time; return *this; }
_AFX_INLINE const CTime& CTime::operator=(time_t t)
        { m_time = t; return *this; }
_AFX_INLINE time_t CTime::GetTime() const
        { return m_time; }
_AFX_INLINE int CTime::GetYear() const
        { return (GetLocalTm(NULL)->tm_year) + 1900; }
_AFX_INLINE int CTime::GetMonth() const
        { return GetLocalTm(NULL)->tm_mon + 1; }
_AFX_INLINE int CTime::GetDay() const
        { return GetLocalTm(NULL)->tm_mday; }
_AFX_INLINE int CTime::GetHour() const
        { return GetLocalTm(NULL)->tm_hour; }
_AFX_INLINE int CTime::GetMinute() const
        { return GetLocalTm(NULL)->tm_min; }
_AFX_INLINE int CTime::GetSecond() const
        { return GetLocalTm(NULL)->tm_sec; }
_AFX_INLINE int CTime::GetDayOfWeek() const
        { return GetLocalTm(NULL)->tm_wday + 1; }
_AFX_INLINE CTimeSpan CTime::operator-(CTime time) const
        { return CTimeSpan(m_time - time.m_time); }
_AFX_INLINE CTime CTime::operator-(CTimeSpan timeSpan) const
        { return CTime(m_time - timeSpan.m_timeSpan); }
_AFX_INLINE CTime CTime::operator+(CTimeSpan timeSpan) const
        { return CTime(m_time + timeSpan.m_timeSpan); }
_AFX_INLINE const CTime& CTime::operator+=(CTimeSpan timeSpan)
        { m_time += timeSpan.m_timeSpan; return *this; }
_AFX_INLINE const CTime& CTime::operator-=(CTimeSpan timeSpan)
        { m_time -= timeSpan.m_timeSpan; return *this; }
_AFX_INLINE BOOL CTime::operator==(CTime time) const
        { return m_time == time.m_time; }
_AFX_INLINE BOOL CTime::operator!=(CTime time) const
        { return m_time != time.m_time; }
_AFX_INLINE BOOL CTime::operator<(CTime time) const
        { return m_time < time.m_time; }
_AFX_INLINE BOOL CTime::operator>(CTime time) const
        { return m_time > time.m_time; }
_AFX_INLINE BOOL CTime::operator<=(CTime time) const
        { return m_time <= time.m_time; }
_AFX_INLINE BOOL CTime::operator>=(CTime time) const
        { return m_time >= time.m_time; }

/////////////////////////////////////////////////////////////////////////////
#endif //_AFX_INLINE

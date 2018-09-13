//typedef long time_t;
#include <time.h>

class COleDateTimeSpan;
class COleDateTime;

/////////////////////////////////////////////////////////////////////////////
// COleDateTime class helpers

#define AFX_OLE_DATETIME_ERROR (-1)

/////////////////////////////////////////////////////////////////////////////
// COleDateTime class

class COleDateTime
{
    // Constructors
public:
    static COleDateTime PASCAL GetCurrentTime();

    COleDateTime();

    COleDateTime(const COleDateTime& dateSrc);
    COleDateTime(const VARIANT& varSrc);
    COleDateTime(DATE dtSrc);

    COleDateTime(time_t timeSrc);
    COleDateTime(const SYSTEMTIME& systimeSrc);
    COleDateTime(const FILETIME& filetimeSrc);

    COleDateTime(int nYear, int nMonth, int nDay,
                 int nHour, int nMin, int nSec);
    COleDateTime(WORD wDosDate, WORD wDosTime);

    // Attributes
public:
    enum DateTimeStatus
    {
        valid = 0,
        invalid = 1,    // Invalid date (out of range, etc.)
        null = 2        // Literally has no value
    };

    DATE m_dt;
    DateTimeStatus m_status;

    void SetStatus(DateTimeStatus status);
    DateTimeStatus GetStatus() const;

    int GetYear() const;
    int GetMonth() const;       // month of year (1 = Jan)
    int GetDay() const;         // day of month (0-31)
    int GetHour() const;        // hour in day (0-23)
    int GetMinute() const;      // minute in hour (0-59)
    int GetSecond() const;      // second in minute (0-59)
    int GetDayOfWeek() const;   // 1=Sun, 2=Mon, ..., 7=Sat
    int GetDayOfYear() const;   // days since start of year, Jan 1 = 1

    // Operations
public:
    const COleDateTime& operator=(const COleDateTime& dateSrc);
    const COleDateTime& operator=(const VARIANT& varSrc);
    const COleDateTime& operator=(DATE dtSrc);

    const COleDateTime& operator=(const time_t& timeSrc);
    const COleDateTime& operator=(const SYSTEMTIME& systimeSrc);
    const COleDateTime& operator=(const FILETIME& filetimeSrc);

    BOOL operator==(const COleDateTime& date) const;
    BOOL operator!=(const COleDateTime& date) const;
    BOOL operator<(const COleDateTime& date) const;
    BOOL operator>(const COleDateTime& date) const;
    BOOL operator<=(const COleDateTime& date) const;
    BOOL operator>=(const COleDateTime& date) const;

    // DateTime math
    COleDateTime operator+(const COleDateTimeSpan& dateSpan) const;
    COleDateTime operator-(const COleDateTimeSpan& dateSpan) const;
    const COleDateTime& operator+=(const COleDateTimeSpan dateSpan);
    const COleDateTime& operator-=(const COleDateTimeSpan dateSpan);

    // DateTimeSpan math
    COleDateTimeSpan operator-(const COleDateTime& date) const;

    operator DATE() const;

    BOOL SetDateTime(int nYear, int nMonth, int nDay,
                     int nHour, int nMin, int nSec);
    BOOL SetDate(int nYear, int nMonth, int nDay);
    BOOL SetTime(int nHour, int nMin, int nSec);
    BOOL ParseDateTime(LPCTSTR lpszDate, DWORD dwFlags = 0,
                       LCID lcid = LANG_USER_DEFAULT);

    // formatting
    CString Format(DWORD dwFlags = 0, LCID lcid = LANG_USER_DEFAULT) const;
    CString Format(LPCTSTR lpszFormat) const;
    CString Format(UINT nFormatID) const;

    // Implementation
protected:
    void CheckRange();
    friend COleDateTimeSpan;
};

// COleDateTime diagnostics and serialization
#ifdef _DEBUG
CDumpContext& AFXAPI operator<<(CDumpContext& dc, COleDateTime dateSrc);
#endif
//CArchive& AFXAPI operator<<(CArchive& ar, COleDateTime dateSrc);
//CArchive& AFXAPI operator>>(CArchive& ar, COleDateTime& dateSrc);

/////////////////////////////////////////////////////////////////////////////
// COleDateTimeSpan class
class COleDateTimeSpan
{
    // Constructors
public:
    COleDateTimeSpan();

    COleDateTimeSpan(double dblSpanSrc);
    COleDateTimeSpan(const COleDateTimeSpan& dateSpanSrc);
    COleDateTimeSpan(long lDays, int nHours, int nMins, int nSecs);

    // Attributes
public:
    enum DateTimeSpanStatus
    {
        valid = 0,
        invalid = 1,    // Invalid span (out of range, etc.)
        null = 2        // Literally has no value
    };

    double m_span;
    DateTimeSpanStatus m_status;

    void SetStatus(DateTimeSpanStatus status);
    DateTimeSpanStatus GetStatus() const;

    double GetTotalDays() const;    // span in days (about -3.65e6 to 3.65e6)
    double GetTotalHours() const;   // span in hours (about -8.77e7 to 8.77e6)
    double GetTotalMinutes() const; // span in minutes (about -5.26e9 to 5.26e9)
    double GetTotalSeconds() const; // span in seconds (about -3.16e11 to 3.16e11)

    long GetDays() const;       // component days in span
    long GetHours() const;      // component hours in span (-23 to 23)
    long GetMinutes() const;    // component minutes in span (-59 to 59)
    long GetSeconds() const;    // component seconds in span (-59 to 59)

    // Operations
public:
    const COleDateTimeSpan& operator=(double dblSpanSrc);
    const COleDateTimeSpan& operator=(const COleDateTimeSpan& dateSpanSrc);

    BOOL operator==(const COleDateTimeSpan& dateSpan) const;
    BOOL operator!=(const COleDateTimeSpan& dateSpan) const;
    BOOL operator<(const COleDateTimeSpan& dateSpan) const;
    BOOL operator>(const COleDateTimeSpan& dateSpan) const;
    BOOL operator<=(const COleDateTimeSpan& dateSpan) const;
    BOOL operator>=(const COleDateTimeSpan& dateSpan) const;

    // DateTimeSpan math
    COleDateTimeSpan operator+(const COleDateTimeSpan& dateSpan) const;
    COleDateTimeSpan operator-(const COleDateTimeSpan& dateSpan) const;
    const COleDateTimeSpan& operator+=(const COleDateTimeSpan dateSpan);
    const COleDateTimeSpan& operator-=(const COleDateTimeSpan dateSpan);
    COleDateTimeSpan operator-() const;

    operator double() const;

    void SetDateTimeSpan(long lDays, int nHours, int nMins, int nSecs);

    // formatting
    CString Format(LPCTSTR pFormat) const;
    CString Format(UINT nID) const;

    // Implementation
public:
    void CheckRange();
    friend COleDateTime;
};

// COleDateTimeSpan diagnostics and serialization
#ifdef _DEBUG
CDumpContext& AFXAPI operator<<(CDumpContext& dc,COleDateTimeSpan dateSpanSrc);
#endif
//CArchive& AFXAPI operator<<(CArchive& ar, COleDateTimeSpan dateSpanSrc);
//CArchive& AFXAPI operator>>(CArchive& ar, COleDateTimeSpan& dateSpanSrc);

#define _AFXDISP_INLINE inline

#ifdef _AFX_INLINE

// COleDateTime
_AFXDISP_INLINE COleDateTime::COleDateTime()
{ m_dt = 0; SetStatus(valid);}
_AFXDISP_INLINE COleDateTime::COleDateTime(const COleDateTime& dateSrc)
{ m_dt = dateSrc.m_dt; m_status = dateSrc.m_status;}
_AFXDISP_INLINE COleDateTime::COleDateTime(const VARIANT& varSrc)
{ *this = varSrc;}
_AFXDISP_INLINE COleDateTime::COleDateTime(DATE dtSrc)
{ m_dt = dtSrc; SetStatus(valid);}
_AFXDISP_INLINE COleDateTime::COleDateTime(time_t timeSrc)
{ *this = timeSrc;}
_AFXDISP_INLINE COleDateTime::COleDateTime(const SYSTEMTIME& systimeSrc)
{ *this = systimeSrc;}
_AFXDISP_INLINE COleDateTime::COleDateTime(const FILETIME& filetimeSrc)
{ *this = filetimeSrc;}
_AFXDISP_INLINE COleDateTime::COleDateTime(int nYear, int nMonth, int nDay,
                                           int nHour, int nMin, int nSec)
{ SetDateTime(nYear, nMonth, nDay, nHour, nMin, nSec);}
_AFXDISP_INLINE COleDateTime::COleDateTime(WORD wDosDate, WORD wDosTime)
{ m_status = DosDateTimeToVariantTime(wDosDate, wDosTime, &m_dt) ?
             valid : invalid;}
_AFXDISP_INLINE const COleDateTime& COleDateTime::operator=(const COleDateTime& dateSrc)
{ m_dt = dateSrc.m_dt; m_status = dateSrc.m_status; return *this;}
_AFXDISP_INLINE COleDateTime::DateTimeStatus COleDateTime::GetStatus() const
{ return m_status;}
_AFXDISP_INLINE void COleDateTime::SetStatus(DateTimeStatus status)
{ m_status = status;}
_AFXDISP_INLINE BOOL COleDateTime::operator==(const COleDateTime& date) const
{ return (m_status == date.m_status && m_dt == date.m_dt);}
_AFXDISP_INLINE BOOL COleDateTime::operator!=(const COleDateTime& date) const
{ return (m_status != date.m_status || m_dt != date.m_dt);}
_AFXDISP_INLINE const COleDateTime& COleDateTime::operator+=(
                                                            const COleDateTimeSpan dateSpan)
{ *this = *this + dateSpan; return *this;}
_AFXDISP_INLINE const COleDateTime& COleDateTime::operator-=(
                                                            const COleDateTimeSpan dateSpan)
{ *this = *this - dateSpan; return *this;}
_AFXDISP_INLINE COleDateTime::operator DATE() const
{ return m_dt;}
_AFXDISP_INLINE COleDateTime::SetDate(int nYear, int nMonth, int nDay)
{ return SetDateTime(nYear, nMonth, nDay, 0, 0, 0);}
_AFXDISP_INLINE COleDateTime::SetTime(int nHour, int nMin, int nSec)
// Set date to zero date - 12/30/1899
{ return SetDateTime(1899, 12, 30, nHour, nMin, nSec);}

// COleDateTimeSpan
_AFXDISP_INLINE COleDateTimeSpan::COleDateTimeSpan()
{ m_span = 0; SetStatus(valid);}
_AFXDISP_INLINE COleDateTimeSpan::COleDateTimeSpan(double dblSpanSrc)
{ m_span = dblSpanSrc; SetStatus(valid);}
_AFXDISP_INLINE COleDateTimeSpan::COleDateTimeSpan(
                                                  const COleDateTimeSpan& dateSpanSrc)
{ m_span = dateSpanSrc.m_span; m_status = dateSpanSrc.m_status;}
_AFXDISP_INLINE COleDateTimeSpan::COleDateTimeSpan(
                                                  long lDays, int nHours, int nMins, int nSecs)
{ SetDateTimeSpan(lDays, nHours, nMins, nSecs);}
_AFXDISP_INLINE COleDateTimeSpan::DateTimeSpanStatus COleDateTimeSpan::GetStatus() const
{ return m_status;}
_AFXDISP_INLINE void COleDateTimeSpan::SetStatus(DateTimeSpanStatus status)
{ m_status = status;}
_AFXDISP_INLINE double COleDateTimeSpan::GetTotalDays() const
{ ASSERT(GetStatus() == valid); return m_span;}
_AFXDISP_INLINE double COleDateTimeSpan::GetTotalHours() const
{ ASSERT(GetStatus() == valid); return m_span * 24;}
_AFXDISP_INLINE double COleDateTimeSpan::GetTotalMinutes() const
{ ASSERT(GetStatus() == valid); return m_span * 24 * 60;}
_AFXDISP_INLINE double COleDateTimeSpan::GetTotalSeconds() const
{ ASSERT(GetStatus() == valid); return m_span * 24 * 60 * 60;}
_AFXDISP_INLINE long COleDateTimeSpan::GetDays() const
{ ASSERT(GetStatus() == valid); return (long)m_span;}
_AFXDISP_INLINE BOOL COleDateTimeSpan::operator==(
                                                 const COleDateTimeSpan& dateSpan) const
{ return (m_status == dateSpan.m_status &&
          m_span == dateSpan.m_span);}
_AFXDISP_INLINE BOOL COleDateTimeSpan::operator!=(
                                                 const COleDateTimeSpan& dateSpan) const
{ return (m_status != dateSpan.m_status ||
          m_span != dateSpan.m_span);}
_AFXDISP_INLINE BOOL COleDateTimeSpan::operator<(
                                                const COleDateTimeSpan& dateSpan) const
{ ASSERT(GetStatus() == valid);
    ASSERT(dateSpan.GetStatus() == valid);
    return m_span < dateSpan.m_span;}
_AFXDISP_INLINE BOOL COleDateTimeSpan::operator>(
                                                const COleDateTimeSpan& dateSpan) const
{ ASSERT(GetStatus() == valid);
    ASSERT(dateSpan.GetStatus() == valid);
    return m_span > dateSpan.m_span;}
_AFXDISP_INLINE BOOL COleDateTimeSpan::operator<=(
                                                 const COleDateTimeSpan& dateSpan) const
{ ASSERT(GetStatus() == valid);
    ASSERT(dateSpan.GetStatus() == valid);
    return m_span <= dateSpan.m_span;}
_AFXDISP_INLINE BOOL COleDateTimeSpan::operator>=(
                                                 const COleDateTimeSpan& dateSpan) const
{ ASSERT(GetStatus() == valid);
    ASSERT(dateSpan.GetStatus() == valid);
    return m_span >= dateSpan.m_span;}
_AFXDISP_INLINE const COleDateTimeSpan& COleDateTimeSpan::operator+=(
                                                                    const COleDateTimeSpan dateSpan)
{ *this = *this + dateSpan; return *this;}
_AFXDISP_INLINE const COleDateTimeSpan& COleDateTimeSpan::operator-=(
                                                                    const COleDateTimeSpan dateSpan)
{ *this = *this - dateSpan; return *this;}
_AFXDISP_INLINE COleDateTimeSpan COleDateTimeSpan::operator-() const
{ return -this->m_span;}
_AFXDISP_INLINE COleDateTimeSpan::operator double() const
{ return m_span;}

#endif //_AFX_INLINE

 


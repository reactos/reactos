class CTimeSpan;                      // time/date difference
class CTime;                          // absolute time/date


/////////////////////////////////////////////////////////////////////////////
// CTimeSpan and CTime

//typedef long time_t;
#include <time.h>
class CTimeSpan
{
public:

    // Constructors
    CTimeSpan();
    CTimeSpan(time_t time);
    CTimeSpan(LONG lDays, int nHours, int nMins, int nSecs);

    CTimeSpan(const CTimeSpan& timeSpanSrc);
    const CTimeSpan& operator=(const CTimeSpan& timeSpanSrc);

    // Attributes
    // extract parts
    LONG GetDays() const;   // total # of days
    LONG GetTotalHours() const;
    int GetHours() const;
    LONG GetTotalMinutes() const;
    int GetMinutes() const;
    time_t GetTotalSeconds() const;
    int GetSeconds() const;

    // Operations
    // time math
    CTimeSpan operator-(CTimeSpan timeSpan) const;
    CTimeSpan operator+(CTimeSpan timeSpan) const;
    const CTimeSpan& operator+=(CTimeSpan timeSpan);
    const CTimeSpan& operator-=(CTimeSpan timeSpan);
    BOOL operator==(CTimeSpan timeSpan) const;
    BOOL operator!=(CTimeSpan timeSpan) const;
    BOOL operator<(CTimeSpan timeSpan) const;
    BOOL operator>(CTimeSpan timeSpan) const;
    BOOL operator<=(CTimeSpan timeSpan) const;
    BOOL operator>=(CTimeSpan timeSpan) const;

    CString Format(LPCTSTR pFormat) const;

#ifdef _ALL_CTIME_FORMATS_
    #ifdef _UNICODE
    // for compatibility with MFC 3.x
    CString Format(LPCSTR pFormat) const;
    #endif
    CString Format(LPCTSTR pFormat) const;
    CString Format(UINT nID) const;
#endif //_ALL_CTIME_FORMATS_

    // serialization
#ifdef _DEBUG
    friend CDumpContext& AFXAPI operator<<(CDumpContext& dc,CTimeSpan timeSpan);
#endif
    friend CArchive& AFXAPI operator<<(CArchive& ar, CTimeSpan timeSpan);
    friend CArchive& AFXAPI operator>>(CArchive& ar, CTimeSpan& rtimeSpan);

private:
    time_t m_timeSpan;
    friend class CTime;
};

class CTime
{
public:

    // Constructors
    static CTime PASCAL GetCurrentTime();

    CTime();
    CTime(time_t time);
    CTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec,
          int nDST = -1);
    CTime(WORD wDosDate, WORD wDosTime, int nDST = -1);
    CTime(const CTime& timeSrc);

    CTime(const SYSTEMTIME& sysTime, int nDST = -1);
    CTime(const FILETIME& fileTime, int nDST = -1);
    const CTime& operator=(const CTime& timeSrc);
    const CTime& operator=(time_t t);

    // Attributes
    struct tm* GetGmtTm(struct tm* ptm = NULL) const;
    struct tm* GetLocalTm(struct tm* ptm = NULL) const;

    time_t GetTime() const;
    int GetYear() const;
    int GetMonth() const;       // month of year (1 = Jan)
    int GetDay() const;         // day of month
    int GetHour() const;
    int GetMinute() const;
    int GetSecond() const;
    int GetDayOfWeek() const;   // 1=Sun, 2=Mon, ..., 7=Sat

    // Operations
    // time math
    CTimeSpan operator-(CTime time) const;
    CTime operator-(CTimeSpan timeSpan) const;
    CTime operator+(CTimeSpan timeSpan) const;
    const CTime& operator+=(CTimeSpan timeSpan);
    const CTime& operator-=(CTimeSpan timeSpan);
    BOOL operator==(CTime time) const;
    BOOL operator!=(CTime time) const;
    BOOL operator<(CTime time) const;
    BOOL operator>(CTime time) const;
    BOOL operator<=(CTime time) const;
    BOOL operator>=(CTime time) const;

    CString Format(LPCTSTR pFormat) const;

#ifdef _ALL_CTIME_FORMATS_
    // formatting using "C" strftime
    CString FormatGmt(LPCTSTR pFormat) const;
    CString Format(UINT nFormatID) const;
    CString FormatGmt(UINT nFormatID) const;

    #ifdef _UNICODE
    // for compatibility with MFC 3.x
    CString Format(LPCSTR pFormat) const;
    CString FormatGmt(LPCSTR pFormat) const;
    #endif
#endif //_ALL_CTIME_FORMATS_

    // serialization
#ifdef _DEBUG
    friend CDumpContext& AFXAPI operator<<(CDumpContext& dc, CTime time);
#endif
    friend CArchive& AFXAPI operator<<(CArchive& ar, CTime time);
    friend CArchive& AFXAPI operator>>(CArchive& ar, CTime& rtime);

private:
    time_t m_time;
};

#include "ctime.inl"



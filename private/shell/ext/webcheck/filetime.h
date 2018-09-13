#ifndef _FILETIME_H_
#define _FILETIME_H_

//**************************************************************************
//  NOTE:  This file is duplicated in urlmon and webcheck.  If you make 
//         changes please sync them!
//**************************************************************************

#define MAKEINT64(lo, hi)   ((__int64)(((DWORD)(lo)) | ((__int64)((DWORD)(hi))) << 32))

#ifndef LODWORD
#define LODWORD(i)          ((DWORD)(i))
#endif

#ifndef HIDWORD
#define HIDWORD(i)          ((DWORD)(((__int64)(i) >> 32) & 0xFFFFFFFF))
#endif

// This is Monday, January 1, 1601 at 12:00:00 am
#define MIN_FILETIME            0i64

// This is Thursday, September 14, 30828 at 2:48:05 am
#define MAX_FILETIME            0x7FFFFFFFFFFFFFFFi64

//  For clarity since FILETIME is expressed as 100-nanosecond intervals
#define ONE_SECOND_IN_FILETIME  10000000i64
#define ONE_MSEC_IN_FILEITME    10000i64
#define ONE_MINUTE_IN_FILETIME  (ONE_SECOND_IN_FILETIME * 60i64)

struct CFileTime : public FILETIME
{
    // Constructors
    CFileTime()             { *this = 0; }
    CFileTime(const FILETIME& f)  { *this = f; }
    CFileTime(const CFileTime& f) { *this = f; }
    CFileTime(__int64 i)    { *this = i; }

    // Assignment operators
    inline CFileTime& operator = (const FILETIME& f)
    {   
        dwLowDateTime = f.dwLowDateTime;
        dwHighDateTime = f.dwHighDateTime;
        return *this;
    }

    inline CFileTime& operator = (const CFileTime& f)
    {   
        dwLowDateTime = f.dwLowDateTime;
        dwHighDateTime = f.dwHighDateTime;
        return *this;
    }

    inline CFileTime& operator = (__int64 i)
    {   
        dwLowDateTime = LODWORD(i);
        dwHighDateTime = HIDWORD(i);
        return *this;
    }

    // Comparison operators
    inline BOOL operator == (__int64 i)
    {   
        return MAKEINT64(dwLowDateTime, dwHighDateTime) == i;
    }

    inline BOOL operator > (__int64 i)
    {   
        return MAKEINT64(dwLowDateTime, dwHighDateTime) > i;
    }

    inline BOOL operator < (__int64 i)
    {   
        return MAKEINT64(dwLowDateTime, dwHighDateTime) < i;
    }

    inline BOOL operator != (__int64 i)
    {   
        return !(*this == i);
    }

    inline BOOL operator >= (__int64 i)
    {   
        return !(*this < i);
    }

    inline BOOL operator <= (__int64 i)
    {   
        return !(*this > i);
    }

    inline BOOL operator == (const FILETIME& f)
    {   
        return *this == MAKEINT64(f.dwLowDateTime, f.dwHighDateTime);
    }

    inline BOOL operator > (const FILETIME& f)
    {   
        return *this > MAKEINT64(f.dwLowDateTime, f.dwHighDateTime);
    }

    inline BOOL operator < (const FILETIME& f)
    {   
        return *this < MAKEINT64(f.dwLowDateTime, f.dwHighDateTime);
    }

    inline BOOL operator != (const FILETIME& f)
    {   
        return !(*this == f);
    }

    inline BOOL operator >= (const FILETIME& f)
    {   
        return !(*this < f);
    }

    inline BOOL operator <= (const FILETIME& f)
    {   
        return !(*this > f);
    }

    // Arithemetic operators
    inline CFileTime operator + (__int64 i)
    {
        return CFileTime(MAKEINT64(dwLowDateTime, dwHighDateTime) + i);
    }
    
    inline CFileTime operator += (__int64 i)
    {
        *this = *this + i;
        return *this;
    }

    inline CFileTime operator - (__int64 i)
    {
        return CFileTime(MAKEINT64(dwLowDateTime, dwHighDateTime) - i);
    }
    
    inline CFileTime operator -= (__int64 i)
    {
        *this = *this - i;
        return *this;
    }

    inline CFileTime operator * (__int64 i)
    {
        return CFileTime(MAKEINT64(dwLowDateTime, dwHighDateTime) * i);
    }
    
    inline CFileTime operator *= (__int64 i)
    {
        *this = *this * i;
        return *this;
    }

    inline CFileTime operator / (__int64 i)
    {
        return CFileTime(MAKEINT64(dwLowDateTime, dwHighDateTime) / i);
    }
    
    inline CFileTime operator /= (__int64 i)
    {
        *this = *this / i;
        return *this;
    }

    inline CFileTime operator + (const FILETIME& f)
    {
        return *this + MAKEINT64(f.dwLowDateTime, f.dwHighDateTime);
    }
    
    inline CFileTime operator += (const FILETIME& f)
    {
        *this = *this + f;
        return *this;
    }

    inline CFileTime operator - (const FILETIME& f)
    {
        return *this - MAKEINT64(f.dwLowDateTime, f.dwHighDateTime);
    }
    
    inline CFileTime operator -= (const FILETIME& f)
    {
        *this = *this - f;
        return *this;
    }

    inline CFileTime operator * (const FILETIME& f)
    {
        return *this * MAKEINT64(f.dwLowDateTime, f.dwHighDateTime);
    }
    
    inline CFileTime operator *= (const FILETIME& f)
    {
        *this = *this * f;
        return *this;
    }

    inline CFileTime operator / (const FILETIME& f)
    {
        return *this / MAKEINT64(f.dwLowDateTime, f.dwHighDateTime);
    }
    
    inline CFileTime operator /= (const FILETIME& f)
    {
        *this = *this / f;
        return *this;
    }
};

//
//  Conversions 
//  NOTE: We can't do want operator __int64() since what causes to many
//  ambiguous situations that the compiler just can't handle.
//
inline  __int64 FileTimeToInt64(const FILETIME& f)
{
    return MAKEINT64(f.dwLowDateTime, f.dwHighDateTime);
}


#endif


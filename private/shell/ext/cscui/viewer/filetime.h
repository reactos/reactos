#ifndef _INC_CSCVIEW_FILETIME_H
#define _INC_CSCVIEW_FILETIME_H

#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _INC_CSCVIEW_STRCLASS_H
#   include "strclass.h"
#endif

//
// Trivial class to represent a file-time value.
// Allows comparison of file times using standard == and != operators.
//
class FileTime
{
    public:
        explicit FileTime(const FILETIME& ft)
            { FileTimeToLocalFileTime(&ft, &m_time); }

        explicit FileTime(const WIN32_FIND_DATA& fd)
            { FileTimeToLocalFileTime(&fd.ftLastWriteTime, &m_time); }

        FileTime(void)
            { m_time.dwLowDateTime = m_time.dwHighDateTime = 0; }

        operator FILETIME() const
            { return m_time; }

        int Compare(const FileTime& rhs) const
            { return CompareFileTime(&m_time, &rhs.m_time); }

        FILETIME GetTime(void) const
            { return m_time; }

        void GetString(CString *pstr) const;

    private:
        FILETIME m_time;
        //
        // Default bitwise copy semantics are OK.
        //

        friend bool operator == (const FileTime& a, const FileTime& b);
        friend bool operator != (const FileTime& a, const FileTime& b);
        friend bool operator <  (const FileTime& a, const FileTime& b);
        friend bool operator >  (const FileTime& a, const FileTime& b);
        friend bool operator <= (const FileTime& a, const FileTime& b);
        friend bool operator >= (const FileTime& a, const FileTime& b);
};

//
// The various comparison operators for FileTime objects.
//
inline bool operator == (const FileTime& a, const FileTime& b)
{
    return 0 == a.Compare(b);
}

inline bool operator != (const FileTime& a, const FileTime& b)
{
    return !(a == b);
}

inline bool operator < (const FileTime& a, const FileTime& b)
{
    return 0 > a.Compare(b);
}

inline bool operator <= (const FileTime& a, const FileTime& b)
{
    return (a < b) || (a == b);
}

inline bool operator > (const FileTime& a, const FileTime& b)
{
    return !(a < b) && !(a == b);
}

inline bool operator >= (const FileTime& a, const FileTime& b)
{
    return (a > b) || (a == b);
}

#endif //_INC_CSCVIEW_FILETIME_H

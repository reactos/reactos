//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       filesize.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_FILESIZE_H
#define _INC_CSCVIEW_FILESIZE_H

#ifndef _INC_CSCVIEW_STRCLASS_H
#   include "strclass.h"
#endif
#ifndef _INC_CSCVIEW_CARRAY_H
#   include "carray.h"
#endif

//
// Simple class to convert a file size value to a string formatted for display.
// The display format is the same used by the shell (i.e. "10.5 MB")
//
class FileSize
{
    public:
        explicit FileSize(ULONGLONG ullSize = 0);
        ~FileSize(void) { }

        FileSize(const FileSize& rhs)
            { *this = rhs; }

        FileSize& operator = (const FileSize& rhs);

        int Compare(const FileSize& rhs) const
            { return *this > rhs ? 1 : (*this == rhs ? 0 : -1); }

        operator ULONGLONG() const
            { return m_ullSize; }

        ULONGLONG GetSize(void) const
            { return m_ullSize; }

        void GetString(CString *pstr) const
            { DBGASSERT((NULL != pstr)); Format(m_ullSize, pstr); }

    private:
        ULONGLONG          m_ullSize;   // Size as a number.
        static CArray<int> m_rgiOrders; // Array of format string res IDs.

        void Format(ULONGLONG ullSize, CString *pstr) const;
        void CvtSizeToText(ULONGLONG n, LPTSTR pszBuffer) const;
        int StrToInt(LPCTSTR lpSrc) const;
        LPTSTR AddCommas(ULONGLONG n, LPTSTR pszResult, int cchResult) const;

        bool IsDigit(TCHAR ch) const
            { return (ch >= TEXT('0') && ch <= TEXT('9')); }

        friend bool operator == (const FileSize& a, const FileSize& b);
        friend bool operator != (const FileSize& a, const FileSize& b);
        friend bool operator <  (const FileSize& a, const FileSize& b);
        friend bool operator <= (const FileSize& a, const FileSize& b);
        friend bool operator >  (const FileSize& a, const FileSize& b);
        friend bool operator >= (const FileSize& a, const FileSize& b);
};

//
// The various comparison operators for FileSize objects.
//
inline bool operator == (const FileSize& a, const FileSize& b)
{ 
    return a.m_ullSize == b.m_ullSize;
}

inline bool operator != (const FileSize& a, const FileSize& b)
{ 
    return !(a == b);
}

inline bool operator < (const FileSize& a, const FileSize& b)
{ 
    return a.m_ullSize < b.m_ullSize;
}

inline bool operator <= (const FileSize& a, const FileSize& b)
{ 
    return (a < b) || (a == b);
}

inline bool operator > (const FileSize& a, const FileSize& b)
{ 
    return !(a < b) && !(a == b);
}

inline bool operator >= (const FileSize& a, const FileSize& b)
{ 
    return (a > b) || (a == b);
}

#endif // _INC_CSCVIEW_FILESIZE_H

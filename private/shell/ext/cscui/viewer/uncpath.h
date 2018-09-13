#ifndef _INC_CSCVIEW_UNCPATH_H
#define _INC_CSCVIEW_UNCPATH_H

#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _INC_CSCVIEW_STRCLASS_H
#   include <strclass.h>
#endif

//
// Trivial class that parses a UNC path string and is able to return
// the various pieces.
//
class UNCPath
{
    public:
        explicit UNCPath(LPCTSTR pszUNCPath);
        UNCPath(void) { }

        UNCPath& operator = (LPCTSTR pszPath);

        bool operator == (const UNCPath& rhs) const;
        bool operator < (const UNCPath& rhs) const;
        bool operator != (const UNCPath& rhs) const
            { return !(*this == rhs); }

        void GetFullPath(CString *pstrOut) const; // "\\worf\ntspecs\brianau\doc\foo.txt"

        CString m_strServer;  // Server name string. "worf"
        CString m_strShare;   // Share name string.  "ntspecs"
        CString m_strPath;    // Path name string.   "\brianau\doc"
        CString m_strFile;    // File name string.   "foo.txt"
};

#endif //_INC_CSCVIEW_UNCPATH_H

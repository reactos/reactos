#ifndef _INC_DSKQUOTA_PATHSTR_H
#define _INC_DSKQUOTA_PATHSTR_H


#ifndef _INC_DSKQUOTA_UTILS_H
#   include "utils.h"
#endif
#ifndef _INC_SHLWAPI
#   include <shlwapi.h>
#endif
#ifndef _INC_DSKQUOTA_STRCLASS_H
#   include "strclass.h"
#endif


class CPath : public CString
{
    public:
        CPath(void) { }
        explicit CPath(LPCTSTR pszRoot, LPCTSTR pszDir = NULL, LPCTSTR pszFile = NULL, LPCTSTR pszExt = NULL);
        CPath(const CPath& rhs);
        CPath& operator = (const CPath& rhs);
        CPath& operator = (LPCTSTR rhs);

        ~CPath(void) { }

        //
        // Component replacement.
        //
        void SetRoot(LPCTSTR pszRoot);
        void SetPath(LPCTSTR pszPath);
        void SetDirectory(LPCTSTR pszDir);
        void SetFileSpec(LPCTSTR pszFileSpec);
        void SetExtension(LPCTSTR pszExt);
        //
        // Component query
        //
        bool GetRoot(CPath *pOut) const;
        bool GetPath(CPath *pOut) const;
        bool GetDirectory(CPath *pOut) const;
        bool GetFileSpec(CPath *pOut) const;
        bool GetExtension(CPath *pOut) const;
        //
        // Component removal
        //
        void RemoveRoot(void);
        void RemovePath(void);
        void RemoveFileSpec(void);
        void RemoveExtension(void);
        void StripToRoot(void);

        bool Append(LPCTSTR psz);

        //
        // DOS drive letter support.
        //
        bool BuildRoot(int iDrive);
        int GetDriveNumber(void) const;

        //
        // Type identification.
        //
        bool IsDirectory(void) const;
        bool IsFileSpec(void) const;
        bool IsPrefix(LPCTSTR pszPrefix) const;
        bool IsRelative(void) const;
        bool IsRoot(void) const;
        bool IsSameRoot(LPCTSTR pszPath) const;
        bool IsUNC(void) const;
        bool IsUNCServer(void) const;
        bool IsUNCServerShare(void) const;
        bool IsURL(void) const;

        //
        // Miscellaneous formatting.
        //
        bool MakePretty(void);
        void QuoteSpaces(void);
        void UnquoteSpaces(void);
        void RemoveBlanks(void);
        void AddBackslash(void);
        void RemoveBackslash(void);
        bool Canonicalize(void);
        bool Compact(HDC hdc, int cxPixels);
        bool CommonPrefix(LPCTSTR pszPath1, LPCTSTR pszPath2);
        bool Exists(void) const;

    private:
        template <class T>
        T& MAX(const T& a, const T& b)
            { return a > b ? a : b; }

};


class CPathIter
{
    public:
        CPathIter(const CPath& path);
        ~CPathIter(void) { }

        bool Next(CPath *pOut);
        void Reset(void);

    private:
        CPath  m_path;
        LPTSTR m_pszCurrent;
};


inline bool 
CPath::Exists(
    void
    ) const
{
    return boolify(::PathFileExists((LPCTSTR)*this));
}


inline bool 
CPath::IsDirectory(
    void
    ) const
{
    return boolify(::PathIsDirectory((LPCTSTR)*this));
}

inline bool 
CPath::IsFileSpec(
    void
    ) const
{
    return boolify(::PathIsFileSpec((LPCTSTR)*this));
}

inline bool 
CPath::IsPrefix(
    LPCTSTR pszPrefix
    ) const
{
    return boolify(::PathIsPrefix(pszPrefix, (LPCTSTR)*this));
}


inline bool 
CPath::IsRelative(
    void
    ) const
{
    return boolify(::PathIsRelative((LPCTSTR)*this));
}

inline bool 
CPath::IsRoot(
    void
    ) const
{
    return boolify(::PathIsRoot((LPCTSTR)*this));
}


inline bool 
CPath::IsSameRoot(
    LPCTSTR pszPath
    ) const
{
    return boolify(::PathIsSameRoot(pszPath, (LPCTSTR)*this));
}


inline bool 
CPath::IsUNC(
    void
    ) const
{
    return boolify(::PathIsUNC((LPCTSTR)*this));
}

inline bool 
CPath::IsUNCServer(
    void
    ) const
{
    return boolify(::PathIsUNCServer((LPCTSTR)*this));
}


inline bool 
CPath::IsUNCServerShare(
    void
    ) const
{
    return boolify(::PathIsUNCServerShare((LPCTSTR)*this));
}

inline bool 
CPath::IsURL(
    void
    ) const
{
    return boolify(::PathIsURL((LPCTSTR)*this));
}

inline bool 
CPath::MakePretty(
    void
    )
{
    return boolify(::PathMakePretty((LPTSTR)*this));
}

inline int
CPath::GetDriveNumber(
    void
    ) const
{
    return ::PathGetDriveNumber(*this);
}


#endif // _INC_DSKQUOTA_PATHSTR_H






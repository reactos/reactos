#ifndef _INC_DSKQUOTA_STRCLASS_H
#define _INC_DSKQUOTA_STRCLASS_H
///////////////////////////////////////////////////////////////////////////////
/*  File: strclass.h

    Description: Typical class to handle strings.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _INC_STDIO
#   include <stdio.h>
#endif

#ifndef _INC_STDARG
#   include <stdarg.h>    // For va_list stuff.
#endif

#ifndef _INC_DSKQUOTA_EXCEPT_H
#   include "except.h"
#endif

#undef StrCpyA

//
// class String implements a reference-counted string class with all 
// the typical string class operations.
//
class CString
{
    public:
        CString(VOID);
        explicit CString(INT cch);
        explicit CString(LPCSTR pszA);
        explicit CString(LPCWSTR pszW);
        CString(const CString& rhs);
        CString(HINSTANCE hInst, INT idMsg, ...);
        virtual ~CString(VOID);

        //
        // Basic operations.
        // - Assignment.
        // - Concatenation.
        // - Comparison.
        // - Array subscript.
        //
        CString& operator =  (const CString& rhs);
        CString& operator =  (LPCSTR rhsA);
        CString& operator =  (LPCWSTR rhsW);
        CString  operator +  (const CString& rhs) const;
        CString  operator +  (LPCSTR rhsA) const;
        CString  operator +  (LPCWSTR rhsW) const;
        friend CString operator + (LPCSTR pszA, const CString& s);
        friend CString operator + (LPCWSTR pszW, const CString& s);
        CString& operator += (const CString& rhs);
        BOOL     operator == (const CString& rhs) const;
        BOOL     operator != (const CString& rhs) const;
        BOOL     operator <  (const CString& rhs) const;
        BOOL     operator <= (const CString& rhs) const;
        BOOL     operator >  (const CString& rhs) const;
        BOOL     operator >= (const CString& rhs) const;
        TCHAR    operator[]  (INT index) const;
        TCHAR&   operator[]  (INT index);

        //
        // Type conversion.  Give read access to nul terminated c-string.
        //
        operator LPCTSTR(VOID) const
            { return m_pValue->m_psz; }

        operator LPCTSTR(VOID)
            { return m_pValue->m_psz; }

        operator LPTSTR(VOID)
            { CopyOnWrite(); return m_pValue->m_psz; }

        //
        // Same thing as (LPCTSTR) conversion but in function form.
        //
        LPCTSTR Cstr(void) const
            { return m_pValue->m_psz; }

        //
        // Return a pointer to a specifically-sized buffer.
        //
        LPTSTR GetBuffer(INT cchMax = -1);
        void ReleaseBuffer(void);

        //
        // Trim trailing or leading whitespace.
        //
        void Rtrim(void);
        void Ltrim(void);
        void Trim(void)
            { Ltrim(); Rtrim(); }

        //
        // Character location.
        //
        INT First(TCHAR ch) const;
        INT Last(TCHAR ch) const;

        //
        // Extract a substring.
        //
        CString SubString(INT iFirst, INT cch = -1);

        //
        // Convert characters to upper/lower case.
        //
        VOID ToUpper(INT iFirst = 0, INT cch = -1);
        VOID ToLower(INT iFirst = 0, INT cch = -1);

        //
        // Load string from resource or message table.
        // Supports FormatMessage-style variable arg formatting.
        //
        BOOL Format(HINSTANCE hInst, UINT idFmt, ...);
        BOOL Format(LPCTSTR pszFmt, ...);
        BOOL Format(HINSTANCE hInst, UINT idFmt, va_list *pargs);
        BOOL Format(LPCTSTR pszFmt, va_list *pargs);

        //
        // Minimum size display rect.
        //
        bool GetDisplayRect(HDC hdc, LPRECT prc) const;

        //
        // Expand any embedded environment strings.
        //
        VOID ExpandEnvironmentStrings(VOID);

        //
        // Compare with a normal 'C' string.
        //
        INT Compare(LPCWSTR rhsW) const;
        INT Compare(LPCSTR rhsA) const;
        INT CompareNoCase(LPCWSTR rhsW) const;
        INT CompareNoCase(LPCSTR rhsA) const;
    
        //
        // Clear a string's contents.  Leaves in new-object state.
        //
        VOID Empty(VOID);
        //
        // Does the object have no content?
        //
        BOOL IsEmpty(VOID) const;
        //
        // Length of string, excluding nul terminator.
        //
        INT Length(VOID) const;
        INT LengthBytes(VOID) const
            { return Length() * sizeof(TCHAR); }

        VOID Size(INT cch);
        INT Size(VOID) const
            { return m_pValue->m_cchAlloc; }
        INT SizeBytes(VOID) const
            { return m_pValue->m_cchAlloc * sizeof(TCHAR); }

        VOID DebugOut(BOOL bNewline = TRUE) const;

        //
        // Replacements for standard string functions.
        // The Ansi versions are DBCS-aware.
        //
        static LPSTR StrCpyA(LPSTR pszDest, LPCSTR pszSrc);
        static LPWSTR StrCpyW(LPWSTR pszDest, LPCWSTR pszSrc);
        static INT StrLenA(LPCSTR psz);
        static INT StrLenW(LPCWSTR psz);
        static LPSTR StrCpyNA(LPSTR pszDest, LPCSTR pszSrc, INT cch);
        static LPWSTR StrCpyNW(LPWSTR pszDest, LPCWSTR pszSrc, INT cch);

    private:
        //
        // class StringValue contains actual string data and a reference count.
        // Class CString has a pointer to one of these.  If a CString
        // object is initialized with or assigned another CString, their StringValue
        // pointers reference the same StringValue object.  The StringValue
        // object maintains a reference count to keep track of how many
        // CString objects reference it.  The CString object implements 
        // copy-on-write so that when it is modified, a private copy of the
        // StringValue is created so other CString objects are left unmodified.
        //
        struct StringValue
        {
            LPTSTR m_psz;      // Ptr to nul-term character string.
            INT    m_cchAlloc; // Number of characters allocated in buffer.
            LONG   m_cRef;     // Number of CString objects referencing this value.
            mutable INT m_cch; // Number of characters in buffer (excl nul term).
        
            StringValue(VOID);
            StringValue(INT cch);
            StringValue(LPCSTR pszA);
            StringValue(LPCWSTR pszW);
            ~StringValue(VOID);

            INT Length(VOID) const;

            static LPSTR WideToAnsi(LPCWSTR pszW, INT *pcch = NULL);
            static LPWSTR AnsiToWide(LPCSTR pszA, INT *pcch = NULL);
            static LPWSTR Dup(LPCWSTR pszW, INT len = 0);
            static LPSTR Dup(LPCSTR pszA, INT len = 0);
            static LPTSTR Concat(StringValue *psv1, StringValue *psv2);

        };

        StringValue *m_pValue; // Pointer to string representation.

        BOOL ValidIndex(INT index) const;
        VOID CopyOnWrite(VOID);
        inline bool IsWhiteSpace(TCHAR ch) const;
};


inline bool
CString::IsWhiteSpace(
    TCHAR ch
    ) const
{
    return (TEXT(' ')  == ch ||
            TEXT('\t') == ch ||
            TEXT('\n') == ch);
}


inline BOOL
CString::ValidIndex(
    INT index
    ) const
{
    return (0 <= index && index < Length());
}


inline BOOL
CString::operator != (
    const CString& rhs
    ) const
{ 
    return !(this->operator == (rhs)); 
}


inline BOOL
CString::operator <= (
    const CString& rhs
    ) const
{
    return (*this < rhs || *this == rhs);
}

inline BOOL
CString::operator > (
    const CString& rhs
    ) const
{
    return !(*this <= rhs);
}

inline BOOL
CString::operator >= (
    const CString& rhs
    ) const
{
    return !(*this < rhs);
}

inline LPWSTR
CString::StrCpyW(
    LPWSTR pszDest, 
    LPCWSTR pszSrc
    )
{
    return lstrcpyW(pszDest, pszSrc);
}


inline LPSTR
CString::StrCpyA(
    LPSTR pszDest, 
    LPCSTR pszSrc
    )
{
    return lstrcpyA(pszDest, pszSrc);
}

inline INT 
CString::StrLenW(
    LPCWSTR psz
    )
{
    return lstrlenW(psz);
}


inline INT 
CString::StrLenA(
    LPCSTR psz
    )
{
    return lstrlenA(psz);
}


inline LPWSTR
CString::StrCpyNW(
    LPWSTR pszDest, 
    LPCWSTR pszSrc, 
    INT cch
    )
{
    return lstrcpynW(pszDest, pszSrc, cch);
}

inline LPSTR
CString::StrCpyNA(
    LPSTR pszDest, 
    LPCSTR pszSrc, 
    INT cch
    )
{
    return lstrcpynA(pszDest, pszSrc, cch);
}

CString
operator + (const LPCWSTR pszW, const CString& s);
                                                
CString
operator + (const LPCSTR pszA, const CString& s);

#endif // _INC_DSKQUOTA_STRCLASS_H


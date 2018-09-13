//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       strclass.cpp
//
//--------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
/*  File: strclass.cpp

    Description: Typical class to handle strings.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "strclass.h"


#ifdef StrCpy
#   undef StrCpy
#endif
#ifdef StrCpyN
#   undef StrCpyN
#endif
#ifdef StrLen
#   undef StrLen
#endif

#ifdef UNICODE
#   define StrCpy   StrCpyW
#   define StrCpyN  StrCpyNW
#   define StrLen   StrLenW
#else
#   define StrCpy   StrCpyA
#   define StrCpyN  StrCpyNA
#   define StrLen   StrLenA
#endif // UNICODE

const INT MAX_RESOURCE_STR_LEN = 4097;


CString::CString(
    VOID
    ) : m_pValue(new StringValue())
{

}

CString::CString(
    INT cch
    ) : m_pValue(new StringValue(cch))
{

}

CString::CString(                                                           
    LPCSTR pszA
    ) : m_pValue(new StringValue(pszA))
{

}

CString::CString(
    LPCWSTR pszW
    ) : m_pValue(new StringValue(pszW))
{

}

CString::CString(
    const CString& rhs
    ) : m_pValue(rhs.m_pValue)
{
    InterlockedIncrement(&(m_pValue->m_cRef));
}


CString::CString(
    HINSTANCE hInst,
    INT idMsg,
    ...
    ) : m_pValue(NULL)
{
    LPTSTR pszMsg = NULL;
    va_list args;

    va_start(args, idMsg);

    Format(hInst, idMsg, &args);

    va_end(args);
}

CString::~CString(
    VOID
    )
{
    if (0 == InterlockedDecrement(&(m_pValue->m_cRef)))
        delete m_pValue;
}

//
// Length of string, excluding nul terminator.
//
INT 
CString::Length(
    VOID
    ) const throw()
{ 
    return m_pValue->Length();
}


VOID
CString::Empty(
    VOID
    )
{
    if (0 == InterlockedDecrement(&(m_pValue->m_cRef)))
    {
        delete m_pValue;
        m_pValue = NULL;
    }

    m_pValue = new StringValue();
}

BOOL
CString::IsEmpty(
    VOID
    ) const throw()
{
    return (NULL != m_pValue && 0 == m_pValue->Length());
}



CString&
CString::operator = (
    const CString& rhs
    )
{
    if (m_pValue != rhs.m_pValue) // Chk for assignment to *this.
    {
        if (0 == InterlockedDecrement(&(m_pValue->m_cRef)))
            delete m_pValue;

        m_pValue = rhs.m_pValue;
        InterlockedIncrement(&(m_pValue->m_cRef));
    }
    return *this;
}

CString&
CString::operator = (
    LPCWSTR rhsW
    )
{
    if (0 == InterlockedDecrement(&(m_pValue->m_cRef)))
    {
        delete m_pValue;
        m_pValue = NULL;
    }

    m_pValue = new StringValue(rhsW);
    return *this;
}    


CString&
CString::operator = (
    LPCSTR rhsA
    )
{
    if (0 == InterlockedDecrement(&(m_pValue->m_cRef)))
    {
        delete m_pValue;
        m_pValue = NULL;
    }

    m_pValue = new StringValue(rhsA);
    return *this;
}    


CString
CString::operator + (
    const CString& rhs
    ) const
{
    CString strNew;
    LPTSTR pszTemp = NULL;
    try
    {
        pszTemp = StringValue::Concat(m_pValue, rhs.m_pValue);
        strNew = pszTemp;
    }
    catch(...)
    {
        DBGERROR((TEXT("C++ exception caught in CString::operator +")));
        delete[] pszTemp;
        throw;
    }
    delete[] pszTemp;

    return strNew;
}



CString&
CString::operator += (
    const CString& rhs
    )
{
    LPTSTR pszTemp = NULL;
    try
    {
        pszTemp = StringValue::Concat(m_pValue, rhs.m_pValue);
        *this = pszTemp;
    }
    catch(...)
    {
        DBGERROR((TEXT("C++ exception caught in CString::operator +=")));
        delete[] pszTemp;
        throw;
    }
    delete[] pszTemp;
    return *this;
}


BOOL
CString::operator == (
    const CString& rhs
    ) const throw()
{
    return (0 == lstrcmp(m_pValue->m_psz, rhs.m_pValue->m_psz));
}


INT
CString::Compare(
    LPCWSTR rhsW
    ) const
{
    LPCTSTR psz;
#ifndef UNICODE
    StringValue Value(rhsW);
    psz = Value.m_psz;
#else
    psz = rhsW;
#endif
    return lstrcmp(m_pValue->m_psz, psz);
}


INT
CString::Compare(
    LPCSTR rhsA
    ) const
{
    LPCTSTR psz;
#ifdef UNICODE
    StringValue Value(rhsA);
    psz = Value.m_psz;
#else
    psz = rhsA;
#endif
    return lstrcmp(m_pValue->m_psz, psz);
}

INT
CString::CompareNoCase(
    LPCWSTR rhsW
    ) const
{
    LPCTSTR psz;
#ifndef UNICODE
    StringValue Value(rhsW);
    psz = Value.m_psz;
#else
    psz = rhsW;
#endif
    return lstrcmpi(m_pValue->m_psz, psz);
}


INT
CString::CompareNoCase(
    LPCSTR rhsA
    ) const
{
    LPCTSTR psz;
#ifdef UNICODE
    StringValue Value(rhsA);
    psz = Value.m_psz;
#else
    psz = rhsA;
#endif
    return lstrcmpi(m_pValue->m_psz, psz);
}

BOOL
CString::operator < (
    const CString& rhs
    ) const
{
    return (0 > lstrcmp(m_pValue->m_psz, rhs.m_pValue->m_psz));
}


TCHAR 
CString::operator[](
    INT index
    ) const
{
    if (!ValidIndex(index))
        throw CException(ERROR_INVALID_INDEX);

    return m_pValue->m_psz[index];
}


TCHAR& 
CString::operator[](
    INT index
    )
{
    if (!ValidIndex(index))
        throw CException(ERROR_INVALID_INDEX);

    CopyOnWrite();
    return m_pValue->m_psz[index];
}

//
// BUGBUG:  What about DBCS?
//
INT
CString::First(
    TCHAR ch
    ) const throw()
{
    INT len = Length();
    for (INT i = 0; i < len; i++)
    {
        if (ch == this->operator[](i))
            return i;
    }
    return -1;
}


//
// BUGBUG:  What about DBCS?
//
INT
CString::Last(
    TCHAR ch
    ) const throw()
{
    INT len = Length();
    for (INT i = len - 1; i >= 0; i--)
    {
        if (ch == this->operator[](i))
            return i;
    }
    return -1;
}

CString 
CString::SubString(
    INT iFirst, 
    INT cch
    )
{

    if (!ValidIndex(iFirst))
        throw CException(ERROR_INVALID_INDEX);

    INT cchToEnd = Length() - iFirst;

    if (-1 == cch || cch > cchToEnd)
        cch = cchToEnd;

    LPTSTR pszTemp = new TCHAR[cch + 1];
    CString::StrCpyN(pszTemp, m_pValue->m_psz + iFirst, cch + 1);
    return CString(pszTemp);
}


VOID 
CString::ToUpper(
    INT iFirst, 
    INT cch
    )
{
    if (!ValidIndex(iFirst))
        throw CException(ERROR_INVALID_INDEX);

    CopyOnWrite();
    INT cchToEnd = Length() - iFirst;

    if (-1 == cch || cch > cchToEnd)
        cch = cchToEnd;

    CharUpperBuff(m_pValue->m_psz + iFirst, cch);
}


VOID 
CString::ToLower(
    INT iFirst, 
    INT cch
    )
{
    if (!ValidIndex(iFirst))
        throw CException(ERROR_INVALID_INDEX);

    CopyOnWrite();
    INT cchToEnd = Length() - iFirst;

    if (-1 == cch || cch > cchToEnd)
        cch = cchToEnd;

    CharLowerBuff(m_pValue->m_psz + iFirst, cch);
}


VOID
CString::Size(
    INT cch
    )
{
    StringValue *m_psv = new StringValue(cch + 1);
    CString::StrCpyN(m_psv->m_psz, m_pValue->m_psz, cch);

    if (m_pValue->m_cRef > 1)
    {
        InterlockedDecrement(&(m_pValue->m_cRef));
    }
    else
    {
        delete m_pValue;
    }
    m_pValue = m_psv;
}


VOID
CString::CopyOnWrite(
    VOID
    )
{
    //
    // Only need to copy if ref cnt > 1.
    //
    if (m_pValue->m_cRef > 1)
    {
        LPTSTR pszTemp = m_pValue->m_psz;
        InterlockedDecrement(&(m_pValue->m_cRef));
        m_pValue = NULL;
        m_pValue = new StringValue(pszTemp);
    }
}
 


BOOL
CString::Format(
    LPCTSTR pszFmt,
    ...
    )
{
    BOOL bResult;
    va_list args;
    va_start(args, pszFmt);
    bResult = Format(pszFmt, &args);
    va_end(args);

    return bResult;
}


BOOL
CString::Format(
    LPCTSTR pszFmt,
    va_list *pargs
    )
{
    BOOL bResult = FALSE;
    TCHAR szBuffer[MAX_RESOURCE_STR_LEN];
    INT cchLoaded;

    cchLoaded = ::FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                                pszFmt,
                                0,
                                0,
                                szBuffer,
                                ARRAYSIZE(szBuffer),
                                pargs);

    if (0 < cchLoaded)
    {
        if (NULL != m_pValue && 0 == InterlockedDecrement(&(m_pValue->m_cRef)))
            delete m_pValue;

        m_pValue = NULL;
        m_pValue = new StringValue(szBuffer);

        bResult = TRUE;
    }
    else
    {
        DWORD dwLastError = GetLastError();
        if (ERROR_SUCCESS != dwLastError)
        {
            DBGERROR((TEXT("CString::Format failed with error 0x%08X"), dwLastError));
            throw CException(dwLastError);
        }
    }

    return bResult;
}


BOOL
CString::Format(
    HINSTANCE hInst,
    UINT idFmt,
    ...
    )
{
    BOOL bResult;
    va_list args;
    va_start(args, idFmt);
    bResult = Format(hInst, idFmt, &args);
    va_end(args);
    return bResult;
}


BOOL
CString::Format(
    HINSTANCE hInst,
    UINT idFmt,
    va_list *pargs
    )
{
    BOOL bResult = FALSE;

    TCHAR szFmtStr[MAX_RESOURCE_STR_LEN]; // Buffer for format string (if needed).
    INT cchLoaded;

    //
    // Try to load the format string as a string resource.
    //
    cchLoaded = ::LoadString(hInst, idFmt, szFmtStr, ARRAYSIZE(szFmtStr));

    if (0 < cchLoaded)
    {
        //
        // The format string was in a string resource.
        // Now format it with the arg list.
        //
        bResult = Format(szFmtStr, pargs);
    }
    else
    {
        TCHAR szBuffer[MAX_RESOURCE_STR_LEN];

        //
        // The format string may be in a message resource.
        // Note that if it is, the resulting formatted string will
        // be automatically attached to m_psz by ::FormatMessage.
        //
        cchLoaded = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | 
                                    FORMAT_MESSAGE_FROM_HMODULE,
                                    hInst,
                                    (DWORD)idFmt,
                                    LANGIDFROMLCID(GetThreadLocale()),
                                    (LPTSTR)szBuffer,
                                    ARRAYSIZE(szBuffer),
                                    pargs);

        if (0 < cchLoaded)
        {
            if (NULL != m_pValue && 0 == InterlockedDecrement(&(m_pValue->m_cRef)))
                delete m_pValue;

            m_pValue = NULL;
            m_pValue = new StringValue(szBuffer);

            bResult = TRUE;
        }
        else
        {
            DWORD dwLastError = GetLastError();
            if (ERROR_SUCCESS != dwLastError)
            {
                DBGERROR((TEXT("CString::Format failed with error 0x%08X"), dwLastError));
                throw CException(dwLastError);
            }
        }
    }
    return bResult;
}


void
CString::FormatSysError(
    UINT idSysError
    )
{
    LPTSTR pszBuffer = NULL;
    ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | 
                    FORMAT_MESSAGE_ALLOCATE_BUFFER,
                    NULL,
                    idSysError,
                    0,
                    (LPTSTR)&pszBuffer,
                    1,
                    NULL);

    if (NULL != pszBuffer)
    {
        *this = pszBuffer;
        LocalFree(pszBuffer);
    }
    else
    {
        //
        // Don't throw a resource exception.  Some code may pass in an error
        // value that just doesn't have corresponding message text.  Since we can't
        // predict what error codes may come in, just set the string to blank and
        // display some debugger spew.
        //
        DBGERROR((TEXT("CString::FormatSysError failed with error 0x%08X for code %d"), 
                 GetLastError(), idSysError));
        Empty();
    }
}


BOOL 
CString::FormatNumber(
    int n, 
    NUMBERFMT *pFmt
    )
{
    NUMBERFMT fmt;
    TCHAR szSep[5];
    if (NULL == pFmt)
    {
        fmt.NumDigits     = 0;
        fmt.LeadingZero   = 0;
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szSep, ARRAYSIZE(szSep));
        fmt.Grouping      = szSep[0] - TEXT('0');
        DBGASSERT((0 <= fmt.Grouping && fmt.Grouping <= 9));
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
        fmt.lpDecimalSep  = fmt.lpThousandSep = szSep;
        fmt.NegativeOrder = 0;
        pFmt = &fmt;
    }
    TCHAR szNum[40];
    wsprintf(szNum, TEXT("%d"), n);
    int iResult = GetNumberFormat(LOCALE_USER_DEFAULT, 0, szNum, pFmt, GetBuffer(40), 40);
    ReleaseBuffer();
    if (0 == iResult)
    {
        DBGERROR((TEXT("CString::FormatNumber failed with error %d"), GetLastError()));
    }
    return 0 != iResult;
}


LPTSTR 
CString::GetBuffer(
    INT cchMax
    )
{
    if (-1 == cchMax)
        cchMax = m_pValue->m_cchAlloc;

    CopyOnWrite();
    if (cchMax > m_pValue->m_cchAlloc)
    {
        //
        // Extend the buffer, copying original contents to dest.
        //
        StringValue *pv = new StringValue(cchMax);

        StrCpyN(pv->m_psz, m_pValue->m_psz, cchMax);

        if (0 == InterlockedDecrement(&(m_pValue->m_cRef)))
            delete m_pValue;

        m_pValue = pv;

        LPTSTR pszEnd = m_pValue->m_psz + m_pValue->m_cchAlloc - 1;
        if (pszEnd >= m_pValue->m_psz && TEXT('\0') != *(pszEnd))
        {
            //
            // Ensure it's nul terminated.
            //
            *(pszEnd) = TEXT('\0');
        }
    }

    return m_pValue->m_psz;
}


VOID
CString::ReleaseBuffer(
    void
    ) throw()
{
    //
    // Update the string length member after client has had free access
    // to internal buffer.
    //
    m_pValue->m_cch = StrLen(m_pValue->m_psz);
}


void
CString::Rtrim(
    void
    ) throw()
{
    LPTSTR s = GetBuffer();  // Doesn't expand buffer so no exception thrown.
    int len = Length();

    while(0 < --len && IsWhiteSpace(s[len]))
        s[len] = TEXT('\0');

    ReleaseBuffer();
}

void
CString::Ltrim(
    void
    ) throw()
{
    LPTSTR s0;
    LPTSTR s = s0 = GetBuffer();   // Doesn't expand buffer so no exception thrown.

    while(*s && IsWhiteSpace(*s))
        s++;
    while(*s)
        *s0++ = *s++;
    *s0 = TEXT('\0');

    ReleaseBuffer();
}



VOID
CString::ExpandEnvironmentStrings(
    VOID
    )
{
    DWORD cchBuffer = 0;  // Size of expansion buffer.
    DWORD cchPath   = 0;  // Count of actual chars in expanded buffer.

    CString strExpanded;  // Expansion buffer.

    //
    // If necessary, keep increasing expansion buffer size until entire
    // expanded string fits.
    //
    do
    {
        cchBuffer += MAX_PATH;

        cchPath = ::ExpandEnvironmentStrings(*this,
                                             strExpanded.GetBuffer(cchBuffer),
                                             cchBuffer);
    }
    while(0 != cchPath && cchPath > cchBuffer);
    ReleaseBuffer();

    *this = strExpanded;
}



VOID
CString::DebugOut(
    BOOL bNewline
    ) const
{ 
    OutputDebugString(m_pValue->m_psz);
    if (bNewline)
        OutputDebugString(TEXT("\n\r"));
}


CString::StringValue::StringValue(
    VOID
    ) throw()
      : m_psz(new TCHAR[1]),
        m_cchAlloc(1),
        m_cch(0),
        m_cRef(1)
{
    *m_psz = TEXT('\0');
}


CString::StringValue::StringValue(
    LPCSTR pszA
    ) : m_psz(NULL),
        m_cchAlloc(0),
        m_cch(0),
        m_cRef(1)
{
#ifdef UNICODE
    m_psz = AnsiToWide(pszA, &m_cchAlloc);
    m_cch = StrLenW(m_psz);
#else
    m_cch = CString::StrLenA(pszA);
    m_psz = Dup(pszA, m_cch + 1);
    m_cchAlloc = m_cch + 1;
#endif    
}

CString::StringValue::StringValue(
    LPCWSTR pszW
    ) : m_psz(NULL),
        m_cchAlloc(0),
        m_cch(0), 
        m_cRef(1)
{
#ifdef UNICODE
    m_cch = CString::StrLenW(pszW);
    m_psz = Dup(pszW, m_cch + 1);
    m_cchAlloc = m_cch + 1;
#else
    m_psz  = WideToAnsi(pszW, &m_cchAlloc);
    m_cch  = StrLenA(m_psz);
#endif
}

CString::StringValue::StringValue(
    INT cch
    ) : m_psz(NULL),
        m_cchAlloc(0),
        m_cch(0),
        m_cRef(0)
{
    m_psz      = Dup(TEXT(""), cch);
    m_cRef     = 1;
    m_cchAlloc = cch;
}


CString::StringValue::~StringValue(
    VOID
    ) throw()
{
    delete[] m_psz;
}

LPWSTR
CString::StringValue::AnsiToWide(
    LPCSTR pszA,
    INT *pcch
    )
{
    INT cchW    = 0;
    LPWSTR pszW = NULL;

    cchW = MultiByteToWideChar(CP_ACP,
                               0,
                               pszA,
                               -1,
                               NULL,
                               0);

    pszW = new WCHAR[cchW];
    MultiByteToWideChar(CP_ACP,
                        0,
                        pszA,
                        -1,
                        pszW,
                        cchW);

    if (NULL != pcch)
        *pcch = cchW;

    return pszW;
}

LPSTR
CString::StringValue::WideToAnsi(
    LPCWSTR pszW,
    INT *pcch
    )
{
    INT cchA   = 0;
    LPSTR pszA = NULL;

    cchA = WideCharToMultiByte(CP_ACP,
                               0,
                               pszW,
                               -1,
                               NULL,
                               0,
                               NULL,
                               NULL);

    pszA = new CHAR[cchA];
    WideCharToMultiByte(CP_ACP,
                        0,
                        pszW,
                        -1,
                        pszA,
                        cchA,
                        NULL,
                        NULL);

    if (NULL != pcch)
        *pcch = cchA;

    return pszA;
}


INT 
CString::StringValue::Length(
    VOID
    ) const throw()
{
    if (0 == m_cch && NULL != m_psz)
    {
        m_cch = StrLen(m_psz);
    }
    return m_cch;
}


LPWSTR
CString::StringValue::Dup(
    LPCWSTR pszW,
    INT cch
    )
{
    if (0 == cch)
        cch = CString::StrLenW(pszW) + 1;

    LPWSTR pszNew = new WCHAR[cch];
    CString::StrCpyW(pszNew, pszW);
    return pszNew;
}


LPSTR
CString::StringValue::Dup(
    LPCSTR pszA,
    INT cch
    )
{
    if (0 == cch)
        cch = CString::StrLenA(pszA) + 1;

    LPSTR pszNew = new CHAR[cch];
    CString::StrCpyA(pszNew, pszA);
    return pszNew;
}
   

LPTSTR 
CString::StringValue::Concat(
    CString::StringValue *psv1,
    CString::StringValue *psv2
    )
{
    LPTSTR pszTemp = NULL;
    INT len1 = psv1->Length();
    INT len2 = psv2->Length();

    pszTemp = new TCHAR[len1 + len2 + 1];
    CString::StrCpy(pszTemp, psv1->m_psz);
    CString::StrCpy(pszTemp + len1, psv2->m_psz);

    return pszTemp;
}

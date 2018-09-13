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


//
// Disable "'operator ->' is not a UDT or reference to a UDT" warning.
// This is caused when we create an autoptr to a non-UDT.  It's meaningless
// since there's no reason to call operator-> on a non-UDT autoptr.
//
#pragma warning (disable : 4284)

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
    ) const
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
    ) const
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
        delete[] pszTemp;
        throw;
    }
    delete[] pszTemp;
    return *this;
}


BOOL
CString::operator == (
    const CString& rhs
    ) const
{
    return (0 == lstrcmp(m_pValue->m_psz, rhs.m_pValue->m_psz));
}


INT
CString::Compare(
    LPCWSTR rhsW
    ) const
{
    StringValue Value(rhsW);
    return lstrcmp(m_pValue->m_psz, Value.m_psz);
}


INT
CString::Compare(
    LPCSTR rhsA
    ) const
{
    StringValue Value(rhsA);
    return lstrcmp(m_pValue->m_psz, Value.m_psz);
}

INT
CString::CompareNoCase(
    LPCWSTR rhsW
    ) const
{
    StringValue Value(rhsW);
    return lstrcmpi(m_pValue->m_psz, Value.m_psz);
}


INT
CString::CompareNoCase(
    LPCSTR rhsA
    ) const
{
    StringValue Value(rhsA);
    return lstrcmpi(m_pValue->m_psz, Value.m_psz);
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
        throw CMemoryException(CMemoryException::index);

    return m_pValue->m_psz[index];
}


TCHAR&
CString::operator[](
    INT index
    )
{
    if (!ValidIndex(index))
        throw CMemoryException(CMemoryException::index);

    CopyOnWrite();
    return m_pValue->m_psz[index];
}

INT
CString::First(
    TCHAR ch
    ) const
{
    LPCTSTR psz = m_pValue->m_psz;
    LPCTSTR pszLast = psz;
    INT i = 0;
    while(psz && *psz)
    {
        if (ch == *psz)
            return i;

        psz = CharNext(psz);
        i += (INT)(psz - pszLast);
        pszLast = psz;
    }
    return -1;
}


INT
CString::Last(
    TCHAR ch
    ) const
{
    INT iLast = -1;
    INT i = 0;
    LPCTSTR psz = m_pValue->m_psz;
    LPCTSTR pszPrev = psz;
    while(psz && *psz)
    {
        if (ch == *psz)
            iLast = i;

        psz = CharNext(psz);
        i += (INT)(psz - pszPrev);
        pszPrev = psz;
    }
    return iLast;
}



CString
CString::SubString(
    INT iFirst,
    INT cch
    )
{

    if (!ValidIndex(iFirst))
        throw CMemoryException(CMemoryException::index);

    INT cchToEnd = Length() - iFirst;

    if (-1 == cch || cch > cchToEnd)
        return CString(m_pValue->m_psz + iFirst);

    LPTSTR pszTemp = new TCHAR[cch + 1];
    if (NULL == pszTemp)
        throw CAllocException();

    CString::StrCpyN(pszTemp, m_pValue->m_psz + iFirst, cch + 1);
    CString strTemp(pszTemp);
    delete[] pszTemp;

    return strTemp;
}


VOID
CString::ToUpper(
    INT iFirst,
    INT cch
    )
{
    if (!ValidIndex(iFirst))
        throw CMemoryException(CMemoryException::index);

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
        throw CMemoryException(CMemoryException::index);

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
            throw CResourceException(CResourceException::string, NULL, 0);
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
                throw CResourceException(CResourceException::string, hInst, idFmt);
            }
        }
    }
    return bResult;
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
    )
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
    )
{
    LPTSTR s = GetBuffer();
    int len = Length();

    while(0 < --len && IsWhiteSpace(s[len]))
        s[len] = TEXT('\0');

    ReleaseBuffer();
}


void
CString::Ltrim(
    void
    )
{
    LPTSTR s0;
    LPTSTR s = s0 = GetBuffer();

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


bool
CString::GetDisplayRect(
    HDC hdc,
    LPRECT prc
    ) const
{
    return (0 != DrawText(hdc, Cstr(), Length(), prc, DT_CALCRECT));
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
    ) : m_psz(new TCHAR[1]),
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
    )
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
    if (NULL == pszW)
        throw CAllocException();

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
    if (NULL == pszA)
        throw CAllocException();

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
    ) const
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
    if (NULL == pszNew)
        throw CAllocException();

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
    if (NULL == pszNew)
        throw CMemoryException(CMemoryException::alloc);

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
    if (NULL == pszTemp)
        throw CMemoryException(CMemoryException::alloc);

    CString::StrCpy(pszTemp, psv1->m_psz);
    CString::StrCpy(pszTemp + len1, psv2->m_psz);

    return pszTemp;
}


#pragma warning (default : 4284)

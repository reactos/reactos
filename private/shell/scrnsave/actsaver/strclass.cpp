/////////////////////////////////////////////////////////////////////////////
// STRCLASS.CPP
//
// Implementation of CString.
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     07/26/96    Created
// jaym     08/26/96    Added LoadString
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "strclass.h"

/////////////////////////////////////////////////////////////////////////////
// Design constants
/////////////////////////////////////////////////////////////////////////////
#define MAX_LOADSTRING_LEN  128
    // Maximum length of a LoadString string. NOTE: The Windows imposed max
    // string length is 4K. But, for performance reasons, we have choosen a
    // much smaller number which relates to this applications largest string
    // resource. [jm]

/////////////////////////////////////////////////////////////////////////////
// CString
/////////////////////////////////////////////////////////////////////////////
CString::CString()
{
    Initialize();
}

CString::CString
(
    int cSize
)
{
    Initialize();

    if (cSize > 0)
        Initialize(cSize);
}

CString::CString
(
    char * str
)
{
    Initialize();

    if (str)
    {
        int cSize = lstrlen(str);

        if (Initialize(cSize))
            lstrcpy(m_str, str);
    }
}

CString::CString
(
    BSTR & bstr
)
{
    int cSize = ::SysStringLen(bstr)*2;

    Initialize();

    if (Initialize(cSize))
        ::WideCharToMultiByte(CP_ACP, 0, bstr, -1, m_str, cSize+1, NULL, NULL);
}

CString::CString
(
    CString & str
)
{
    int cSize = str.GetSize();

    Initialize();

    if (Initialize(cSize))
        str.GetString(m_str);
}

/////////////////////////////////////////////////////////////////////////////
// CString::Initialize
/////////////////////////////////////////////////////////////////////////////
BOOL CString::Initialize
(
    int cSize
)
{
    if (m_cLen > 0)
        Empty();

    m_str = new char [cSize+1];
    if (m_str)
    {
        m_cLen = cSize+1;
        return TRUE;
    }
    else
        Initialize();

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CString::operator= (CString)
/////////////////////////////////////////////////////////////////////////////
CString & CString::operator=
(
    CString & str
)
{
    Empty();

    int cSize = str.GetSize();

    if (Initialize(cSize))
        str.GetString(m_str);

    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// CString::operator= (BSTR)
/////////////////////////////////////////////////////////////////////////////
CString & CString::operator=
(
    BSTR & bstr
)
{
    Empty();

    int cSize = ::SysStringLen(bstr)*2;

    if (Initialize(cSize))
        ::WideCharToMultiByte(CP_ACP, 0, bstr, -1, m_str, cSize+1, NULL, NULL);

    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// CString::operator= (char *)
/////////////////////////////////////////////////////////////////////////////
CString & CString::operator=
(
    char * psz
)
{
    EVAL(SetString(psz));

    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// CString::operator= (const char *)
/////////////////////////////////////////////////////////////////////////////
CString & CString::operator=
(
    const char * psz
)
{
    EVAL(SetString(psz));

    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// CString::operator+=
/////////////////////////////////////////////////////////////////////////////
CString & CString::operator+=
(
    LPCTSTR psz 
)
{
    int     cAddLen;
    char *  pNew;

    if ((cAddLen = lstrlen(psz)) == 0)
        return *this;

    pNew = new char [m_cLen+cAddLen+1];

    if (NULL == pNew)
        return *this;

    if (m_str != NULL)
    {
        lstrcpy(pNew, m_str);
        delete [] m_str;
    }

    lstrcat(pNew, psz);
    m_cLen += cAddLen+1;
    m_str = pNew;

    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// CString::operator+=
/////////////////////////////////////////////////////////////////////////////
CString & CString::operator+=
(
    char ch
)
{
    char * pNew;

    pNew = new char [m_cLen+1];

    if (m_str != NULL)
    {
        lstrcpy(pNew, m_str);
        delete [] m_str;
    }
    else
        return *this;

    pNew[m_cLen++] = ch;
    pNew[m_cLen] = '\0';
    m_str = pNew;

    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// CString::Empty
/////////////////////////////////////////////////////////////////////////////
void CString::Empty
(
)
{
    if (m_cLen > 0)
        delete [] m_str;

    Initialize();
}

/////////////////////////////////////////////////////////////////////////////
// CString::SetString
/////////////////////////////////////////////////////////////////////////////
BOOL CString::SetString
(
    const char * strIn
)
{
    Empty();

    int cSize = lstrlen(strIn);

    if (Initialize(cSize))
    {
            lstrcpy(m_str, strIn);
            return TRUE;
    }
    else
        return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CString::AllocSysString
/////////////////////////////////////////////////////////////////////////////
BSTR CString::AllocSysString
(
) const
{
#if defined(_UNICODE) || defined(OLE2ANSI)
    BSTR bstr = ::SysAllocString(m_str);
#else
    int nLen = ::MultiByteToWideChar(CP_ACP, 0, m_str, m_cLen-1, NULL, NULL);
    BSTR bstr = ::SysAllocStringLen(NULL, nLen);
    if (bstr != NULL)
        ::MultiByteToWideChar(CP_ACP, 0, m_str, m_cLen-1, bstr, nLen);
#endif

    return bstr;
}

/////////////////////////////////////////////////////////////////////////////
// CString::SetSysString
/////////////////////////////////////////////////////////////////////////////
BSTR CString::SetSysString
(
    BSTR * pbstr
) const
{
#if defined(_UNICODE) || defined(OLE2ANSI)
    ::SysReAllocString(pbstr, m_str))
#else
    int nLen = ::MultiByteToWideChar(CP_ACP, 0, m_str, m_cLen-1, NULL, NULL);
    if (::SysReAllocStringLen(pbstr, NULL, nLen))
        MultiByteToWideChar(CP_ACP, 0, m_str, m_cLen-1, *pbstr, nLen);
#endif

    ASSERT(*pbstr != NULL);
    return *pbstr;
}

/////////////////////////////////////////////////////////////////////////////
// CString::LoadString
/////////////////////////////////////////////////////////////////////////////
BOOL CString::LoadString
(
    UINT nID
)
{
    Initialize(MAX_LOADSTRING_LEN);

    return (::LoadString(   _pModule->GetResourceInstance(),
                            nID,
                            m_str,
                            MAX_LOADSTRING_LEN) != 0);
}

/////////////////////////////////////////////////////////////////////////////
// CString::GetBuffer
/////////////////////////////////////////////////////////////////////////////
LPTSTR CString::GetBuffer
(
    int nMinBufLength
)
{
    ASSERT(nMinBufLength >= 0);

    if (nMinBufLength > m_cLen)
    {
        // We have to grow the buffer
        char *  pOldData = m_str;
        int     nOldLen = m_cLen;

        if (nMinBufLength < nOldLen)
            nMinBufLength = nOldLen;

        m_str = new char [nMinBufLength+1];

        if (NULL == m_str)
        {
            m_str = pOldData;
            return NULL;
        }

        if (pOldData != NULL)
        {
            memcpy(m_str, pOldData, (nOldLen)*sizeof(TCHAR));
            delete [] pOldData;
        }

        m_cLen = nMinBufLength+1;
    }

    return m_str;
}

/////////////////////////////////////////////////////////////////////////////
// CString::ReleaseBuffer
/////////////////////////////////////////////////////////////////////////////
void CString::ReleaseBuffer
(
    int nNewLength
)
{
    if (nNewLength == -1)
        nNewLength = lstrlen(m_str); // Zero terminated

    ASSERT(nNewLength <= m_cLen);
    m_str[nNewLength] = '\0';
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// MakeWideStrFromAnsi
/////////////////////////////////////////////////////////////////////////////
LPWSTR MakeWideStrFromAnsi
(
    LPSTR psz,
    BYTE  bType
)
{
    LPWSTR pwsz;
    int i;

    if (psz == NULL)
        return NULL;

    // Compute the length of the required BSTR
    if ((i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0)) <= 0)
        return NULL;

    // Allocate the widestr, +1 for terminating null
    switch (bType) 
    {
        case STR_BSTR:
        {
            pwsz = (LPWSTR) SysAllocStringLen(NULL, i-1); // SysAllocStringLen adds
            break;
        }

        case STR_OLESTR:
        {
            pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));
            break;
        }

        default:
        {
            TraceMsg(DM_ERROR, "Bogus String Type!");
            return NULL;
        }
    }

    if (pwsz == NULL)
        return NULL;

    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;

    return pwsz;
}

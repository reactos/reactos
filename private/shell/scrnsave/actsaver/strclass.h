/////////////////////////////////////////////////////////////////////////////
// STRCLASS.H
//
// Declaration of CString.
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     07/26/96    Created
// jaym     06/10/97    Added TCharSysAllocString and MakeWideStrFromAnsi
/////////////////////////////////////////////////////////////////////////////
#ifndef __STRCLASS_H__
#define __STRCLASS_H__

/////////////////////////////////////////////////////////////////////////////
// CString
/////////////////////////////////////////////////////////////////////////////
class CString
{
// Construction/Destruction
public:
    CString();
    CString(int cSize);
    CString(char * str);
    CString(BSTR & bstr);
    CString(CString & str);

    virtual ~CString()
    { Empty(); }

// Data
protected:
    int     m_cLen;
    char *  m_str;

public:
// Operators
    CString & operator=(CString & str);
    CString & operator=(BSTR & bstr);
    CString & operator=(char * psz);
    CString & operator=(const char * pcsz);

    CString & operator+=(LPCTSTR lpsz);
    CString & operator+=(char ch);
    char operator[](int nIndex)
    {
        ASSERT(nIndex >= 0);
        ASSERT(nIndex < lstrlen(m_str));
        return m_str[nIndex];
    }

    BOOL operator==(CString & str)
    { return (lstrcmpi(m_str, str.m_str) == 0); }

// Casts
    operator LPCTSTR() const
    { return (LPCTSTR)m_str; }

    operator LPTSTR() const
    { return (LPTSTR)m_str; }

// String functions
    void GetString(char * strOut)
    { lstrcpy(strOut, m_str); }

    void GetString(char * strOut, int cSize)
    { lstrcpyn(strOut, m_str, cSize); }

    BOOL SetString(const char * strIn);
    BSTR SetSysString(BSTR * pbstr) const;

    BSTR AllocSysString() const;

    BOOL LoadString(UINT nID);

    LPTSTR GetBuffer(int nMinBufLength);
    void ReleaseBuffer(int nNewLength = -1);

    int GetLength()
    { return ((m_str != NULL) ? lstrlen(m_str) : 0); }

    int GetSize()
    { return m_cLen; }

    void Empty();

// Utility functions
private:
    void Initialize()
    { m_cLen = 0; m_str = NULL; }

    BOOL Initialize(int length);

    CString & ConcatStr(LPCTSTR psz, int cAddLen);
};

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////
LPWSTR MakeWideStrFromAnsi(LPSTR psz, BYTE bType);

#ifdef UNICODE
#define TCharSysAllocString(psz) (BSTR)SysAllocString((psz))
#else
#define TCharSysAllocString(psz) (BSTR)MakeWideStrFromAnsi((psz), STR_BSTR)
#endif

#endif  // __STRCLASS_H__

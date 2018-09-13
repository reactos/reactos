/******************************************************************************

  Header File:  String Array.H

  This provides a relatively simple C++ class for manipulating an array of
  character strings.  In this project, we use it for lists of associated
  devices, or potential associated devices, etc.  I'm not currently sorting
  this list.

  The class declaration may look a bit bizarre.  Since most of the arrays
  will in fact be rather small, I picked a decent size.  When they get bigger,
  I'll chain them internally and use recursion to perform any needed function.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:

  11-01-96  a-robkj@microsoft.com- original version
  12-04-96  a-robkj@microsoft.com   Added LoadString and IsEmpty to CString

******************************************************************************/

#if !defined(STRING_ARRAY)

#if defined(UNICODE)

#define LPCOSTR LPCSTR
#define LPOSTR  LPSTR
#define OCHAR   CHAR

#if !defined(_UNICODE)
#define _UNICODE
#endif

#else

#define LPCOSTR LPCWSTR
#define LPOSTR  LPWSTR
#define OCHAR   WCHAR

#endif
#include    <tchar.h>

#define STRING_ARRAY

class CString {
    LPTSTR  m_acContents;
    LPOSTR  m_acConverted;
    BOOL    m_bConverted;
    void    Flip(LPCWSTR lpstrIn, LPSTR& lpstrOut);
    void    Flip(LPCSTR lpstrIn, LPWSTR& lpstrOut);

public:
    CString();
    CString(const CString& csRef);
    CString(LPCTSTR lpstrRef);
    CString(LPCOSTR lpstrRef);

    ~CString();

    BOOL    IsEmpty() const { return !m_acContents || !m_acContents[0]; }
    void    Empty();

    operator LPCTSTR() const { return m_acContents; }
    operator LPTSTR() const { return m_acContents; }
    operator LPARAM() const { return (LPARAM) m_acContents; }
    operator LPCOSTR();
    const CString& operator = (const CString& csSrc);
    const CString& operator = (LPCTSTR lpstrSrc);
    const CString& operator = (LPCOSTR lpstrSrc);
    CString NameOnly() const;
    CString NameAndExtension() const;
    void    Load(int id, HINSTANCE hiWhere = NULL);
    void    Load(HWND hwnd);
    void    LoadAndFormat(int id,
                          HINSTANCE hiWhere,
                          BOOL bSystemMessage,
                          DWORD dwNumMsg,
                          va_list *argList);
    BOOL    IsEqualString(CString& csRef1);

    friend CString operator + (const CString& csRef1, LPCTSTR lpstrRef2);
};

class CStringArray {
    CString         m_aStore[20];
    CStringArray    *m_pcsaNext;
    unsigned        m_ucUsed;

    const unsigned ChunkSize() const { 
        return sizeof m_aStore / sizeof m_aStore[0];
    }

    LPCTSTR Borrow();

public:

    CStringArray();
    ~CStringArray();

    unsigned    Count() const { return m_ucUsed; }

    //  Add an item
    void        Add(LPCTSTR lpstrNew);

    CString&    operator [](unsigned u) const;

    void        Remove(unsigned u);
    void        Empty();

    //  Return index of string in array- array count if not present

    unsigned    Map(LPCTSTR lpstrRef);
};

class CUintArray {
    unsigned        m_aStore[20];
    CUintArray      *m_pcuaNext;
    unsigned        m_ucUsed;

    const unsigned ChunkSize() const { 
        return sizeof m_aStore / sizeof m_aStore[0];
    }

    unsigned    Borrow();

public:

    CUintArray();
    ~CUintArray();

    unsigned    Count() const { return m_ucUsed; }

    //  Add an item
    void    Add(unsigned u);

    unsigned    operator [](unsigned u) const;

    void    Remove(unsigned u);
    void    Empty();
};

#endif

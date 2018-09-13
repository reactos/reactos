/******************************************************************************

  Header File:  String Array.CPP

  Implements the String Array class- see the related header for the declaration
  of this class.

  This class will do arrays in chunks- if the total array exceeds the size of
  one chunk, we chain more instances together, then use recursion to do the
  work.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:

  11-01-96  a-robkj@microsoft.com- original version
  12-04-96  a-robkj@microsoft.com   Added LoadString and IsEmpty to CString
                                    Also fixed bug in Remove where
                                    u > ChunkSize (wasn't exiting)
  12-11-96  a-robkj@microsoft.com   Let CString do ANSI/UNICODE conversions
                                    automagically to ease some API issues
  01-07-97  KjelgaardR@acm.org  Fixed CStringArray::Empty and CUintArray::Empty
            to NULL pointer to next chunk after deleting it.  Led to GP faults
            if we needed to use the chunk again.

******************************************************************************/

#include    "ICMUI.H"

//  Convert a UNICODE string to a new ANSI buffer

void    CString::Flip(LPCWSTR lpstrIn, LPSTR& lpstrOut) {
    if  (!lpstrIn) {
        lpstrOut = NULL;
        return;
    }
    int iLength = WideCharToMultiByte(CP_ACP, 0, lpstrIn, -1, NULL, 0, NULL,
        NULL);

    if  (!iLength) {
        lpstrOut = NULL;
        return;
    }

    lpstrOut = (LPSTR) malloc(++iLength);
    if(lpstrOut) {
        WideCharToMultiByte(CP_ACP, 0, lpstrIn, -1, lpstrOut, iLength, NULL,
            NULL);
    }
}

//  Convert an ANSI string to a new UNICODE buffer

void    CString::Flip(LPCSTR lpstrIn, LPWSTR& lpstrOut) {
    if  (!lpstrIn) {
        lpstrOut = NULL;
        return;
    }

    int iLength = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpstrIn, -1,
        NULL, 0);

    if  (!iLength) {
        lpstrOut = NULL;
        return;
    }

    lpstrOut = (LPWSTR) malloc(++iLength * sizeof (WCHAR));
    if(lpstrOut) {
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpstrIn, -1, lpstrOut,
            iLength);
    }
}

//  Empty the string, and free all memory.

void    CString::Empty() {
    if  (m_acContents)
        free(m_acContents);

    if  (m_acConverted)
        free(m_acConverted);

    m_acContents = NULL;
    m_acConverted = NULL;
    m_bConverted = FALSE;
}

//  Compare with other CString

BOOL    CString::IsEqualString(CString& csRef1)
{
    if (IsEmpty() || csRef1.IsEmpty())
        return (FALSE);

    return (_tcsicmp(m_acContents,(LPTSTR)csRef1) == 0);
}

CString::CString() {
    m_acContents = NULL;
    m_acConverted = NULL;
    m_bConverted = FALSE;
}

CString::CString(const CString& csRef) {
    m_acContents = csRef.m_acContents ? _tcsdup(csRef.m_acContents) : NULL;
    m_acConverted = NULL;
    m_bConverted = FALSE;
}

CString::CString(LPCTSTR lpstrRef) {
    m_acContents = lpstrRef ? _tcsdup(lpstrRef) : NULL;
    m_acConverted = NULL;
    m_bConverted = FALSE;
}

CString::CString(LPCOSTR lpstrRef) {
    m_acConverted = NULL;
    m_bConverted = FALSE;

    if  (!lpstrRef) {
        m_acContents = NULL;
        return;
    }

    Flip(lpstrRef, m_acContents);
}

//  Class destructor

CString::~CString() {
    Empty();
}

//  Report string in non-native encoding

CString::operator LPCOSTR() {
    if  (!m_bConverted) {
        Flip(m_acContents, m_acConverted);
        m_bConverted = TRUE;
    }
    return  m_acConverted;
}

const CString& CString::operator =(const CString& csSrc) {
    Empty();
    m_acContents = csSrc.m_acContents ? _tcsdup(csSrc.m_acContents) : NULL;
    return  *this;
}

const CString& CString::operator =(LPCTSTR lpstrSrc) {
    Empty();
    m_acContents = lpstrSrc ? _tcsdup(lpstrSrc) : NULL;
    return  *this;
}

const CString& CString::operator =(LPCOSTR lpstrSrc) {
    Empty();
    Flip(lpstrSrc, m_acContents);
    return  *this;
}

CString CString::NameOnly() const {
    TCHAR   acName[_MAX_FNAME];

    if  (!m_acContents)
        return  *this;

    _tsplitpath(m_acContents, NULL, NULL, acName, NULL);

    return  acName;
}

CString CString::NameAndExtension() const {
    TCHAR   acName[_MAX_FNAME], acExtension[_MAX_EXT];

    if  (!m_acContents)
        return  *this;

    _tsplitpath(m_acContents, NULL, NULL, acName, acExtension);

    lstrcat(acName, acExtension);

    return  acName;
}

void    CString::Load(int id, HINSTANCE hi) {

    if  (!hi)
        hi = CGlobals::Instance();

    TCHAR   acWork[MAX_PATH];
    LoadString(hi, id, acWork, MAX_PATH);
    *this = acWork;
}

//  03-20-1997  Bob_Kjelgaard@Prodigy.Net   Part of RAID 22289.
//  Add a method for loading text from a windows handle

void    CString::Load(HWND hwnd) {
    Empty();

    int iccNeeded = GetWindowTextLength(hwnd);
    if  (!iccNeeded)
        return;
    m_acContents = (LPTSTR) malloc(++iccNeeded * sizeof (TCHAR));
    if(m_acContents) {
      GetWindowText(hwnd, m_acContents, iccNeeded);
    }
}

void    CString::LoadAndFormat(int id, HINSTANCE hiWhere, BOOL bSystemMessage,
                               DWORD dwNumMsg, va_list *argList) {
    Empty();

    TCHAR   acWork[1024];
    CString csTemplate;
    LPTSTR  lpSource;
    DWORD   dwFlags;

    if (bSystemMessage) {
        lpSource = NULL;
        dwFlags = FORMAT_MESSAGE_FROM_SYSTEM;
    } else {
        csTemplate.Load(id);
        lpSource = csTemplate;
        dwFlags = FORMAT_MESSAGE_FROM_STRING;
        id = 0;
    }

    if (FormatMessage(dwFlags,lpSource, id, 0, acWork, 1024, argList)) {
        *this = acWork;
    }
}

CString operator +(const CString& csRef, LPCTSTR lpstrRef) {
    if  (!lpstrRef || !*lpstrRef)
        return  csRef;

    if  (csRef.IsEmpty())
        return  lpstrRef;

    CString csReturn;

    csReturn.m_acContents = (LPTSTR) malloc((1 + lstrlen(csRef.m_acContents) +
        lstrlen(lpstrRef)) * sizeof(TCHAR));
    if(csReturn.m_acContents) {
        lstrcat(lstrcpy(csReturn.m_acContents, csRef.m_acContents), lpstrRef);
    }

    return  csReturn;
}

//  CStringArray classes- these manage an array of strings,
//  but the methods are geared to list-style management.

//  Borrow first element from next chunk

LPCTSTR CStringArray::Borrow() {

    LPCTSTR lpstrReturn = m_aStore[0];

    memcpy((LPSTR) m_aStore, (LPSTR) (m_aStore + 1),
        (ChunkSize() - 1) * sizeof m_aStore[0]);

    if  (m_ucUsed > ChunkSize())
        m_aStore[ChunkSize() - 1] = m_pcsaNext -> Borrow();
    else
        m_aStore[ChunkSize() - 1] = (LPCTSTR) NULL;

    m_ucUsed--;

    if  (m_ucUsed <= ChunkSize() && m_pcsaNext) {
        delete  m_pcsaNext;
        m_pcsaNext = NULL;
    }

    return  lpstrReturn;
}

//  ctor

CStringArray::CStringArray() {
    m_ucUsed = 0;
    m_pcsaNext = NULL;
}

//  dtor

CStringArray::~CStringArray() {
    Empty();
}

//  Empty the list/array

void    CStringArray::Empty() {

    if  (!m_ucUsed) return;

    if  (m_pcsaNext) {
        delete  m_pcsaNext;
        m_pcsaNext = NULL;
    }
    m_ucUsed = 0;
}

unsigned    CStringArray::Map(LPCTSTR lpstrRef) {

    for (unsigned u = 0; u < m_ucUsed; u++)
        if  (!lstrcmpi(operator[](u), lpstrRef))
            break;

    return  u;
}

//  Add an item

void    CStringArray::Add(LPCTSTR lpstrNew) {

    if  (m_ucUsed < ChunkSize()) {
        m_aStore[m_ucUsed++] = lpstrNew;
        return;
    }

    //  Not enough space!  Add another record, if there isn't one

    if  (!m_pcsaNext)
        m_pcsaNext = new CStringArray;

    //  Add the string to the next array (recursive call!)

    if  (m_pcsaNext) {
        m_pcsaNext -> Add(lpstrNew);
        m_ucUsed++;
    }
}

//  define an indexing operator

CString&    CStringArray::operator [](unsigned u) const {
    _ASSERTE(u < m_ucUsed);

    return  u < ChunkSize() ?
        (CString&)m_aStore[u] : m_pcsaNext -> operator[](u - ChunkSize());
}

//  Remove the string at some index, shifting the rest down one slot

void    CStringArray::Remove(unsigned u) {

    if  (u > m_ucUsed)
        return;

    if  (u >= ChunkSize()) {
        m_pcsaNext -> Remove(u - ChunkSize());
        return;
    }

    memmove((LPSTR) (m_aStore + u), (LPSTR) (m_aStore + u + 1),
        (ChunkSize() - (u + 1)) * sizeof m_aStore[0]);

    if  (m_ucUsed > ChunkSize())
        m_aStore[ChunkSize() - 1] = m_pcsaNext -> Borrow();
    else
        m_aStore[ChunkSize() - 1] = (LPCTSTR) NULL;

    m_ucUsed--;

    if  (m_ucUsed <= ChunkSize() && m_pcsaNext) {
        delete  m_pcsaNext;
        m_pcsaNext = NULL;
    }
}

//  CUintArray class- this manages an array/list of unsigned integers
//  The implementation is quite similar to the CStringArray's.  Why
//  bother to do it different, after all?

unsigned    CUintArray::Borrow() {

    unsigned    uReturn = m_aStore[0];

    memcpy((LPSTR) m_aStore, (LPSTR) (m_aStore + 1),
        (ChunkSize() - 1) * sizeof m_aStore[0]);

    if  (m_ucUsed > ChunkSize())
        m_aStore[ChunkSize() - 1] = m_pcuaNext -> Borrow();
    else
        m_aStore[ChunkSize() - 1] = 0;

    m_ucUsed--;

    if  (m_ucUsed <= ChunkSize() && m_pcuaNext) {
        delete  m_pcuaNext;
        m_pcuaNext = NULL;
    }

    return  uReturn;
}

CUintArray::CUintArray() {
    m_ucUsed = 0;
    m_pcuaNext = NULL;
}

CUintArray::~CUintArray() {
    Empty();
}

void    CUintArray::Empty() {

    if  (!m_ucUsed) return;

    if  (m_pcuaNext) {
        delete  m_pcuaNext;
        m_pcuaNext = NULL;
    }
    m_ucUsed = 0;
}

//  Add an item
void    CUintArray::Add(unsigned uNew) {

    if  (m_ucUsed < ChunkSize()) {
        m_aStore[m_ucUsed++] = uNew;
        return;
    }

    //  Not enough space!  Add another record, if there isn't one

    if  (!m_pcuaNext)
        m_pcuaNext = new CUintArray;

    //  Add the item to the next array (recursive call!)

    if  (m_pcuaNext) {
        m_pcuaNext -> Add(uNew);
        m_ucUsed++;
    }
}

unsigned    CUintArray::operator [](unsigned u) const {
    return  u < m_ucUsed ? u < ChunkSize() ?
        m_aStore[u] : m_pcuaNext -> operator[](u - ChunkSize()) : 0;
}

void    CUintArray::Remove(unsigned u) {

    if  (u > m_ucUsed)
        return;

    if  (u >= ChunkSize()) {
        m_pcuaNext -> Remove(u - ChunkSize());
        return;
    }

    memmove((LPSTR) (m_aStore + u), (LPSTR) (m_aStore + u + 1),
        (ChunkSize() - (u + 1)) * sizeof m_aStore[0]);

    if  (m_ucUsed > ChunkSize())
        m_aStore[ChunkSize() - 1] = m_pcuaNext -> Borrow();
    else
        m_aStore[ChunkSize() - 1] = 0;

    m_ucUsed--;

    if  (m_ucUsed <= ChunkSize() && m_pcuaNext) {
        delete  m_pcuaNext;
        m_pcuaNext = NULL;
    }
}


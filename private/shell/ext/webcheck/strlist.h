//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
#ifndef _STRINGLST_H
#define _STRINGLST_H

// Helper functions to properly create CWCStringList.
class CWCStringList;
CWCStringList *CreateCWCStringList(int iInitBufSize=4096);

// BSTR is DWORD length followed by null-term OLECHAR (WCHAR) data
//
//----------------------------------------------------------------------------
// CWCStringList is used to store array of non-duplicate strings. Used for
//   dependency and link storage.
//
// Limitations:
//  1) Strings can only be added and never removed from list
//  2) No duplicate strings can ever be stored
//
// Stores all strings in one big block of memory
// It's efficient at ensuring there are no duplicate strings.
// Scalable. Uses hash. Expands to the limit of memory.
//
// Usage:
//  Create the class. Call Init() and destroy if it fails.
//  Add the strings with AddString
//  Use NumStrings() and GetString() to iterate through all of the stored strings.
//
//  The state can be saved and restored with IPersistStream operations
//
//  We take up memory when we're loaded. Don't initialize one of these objects
//  until you're going to use it.
//----------------------------------------------------------------------------

const int STRING_HASH_SIZE = 127;      // should be prime

const TCHAR PARSE_STRING_DELIM = TEXT('\n');     // To separate URLs

// We're not an OLE object but support IPersistStream members to make saving
//  & restoring easier
class CWCStringList {
public:
    CWCStringList();
virtual ~CWCStringList();

    // Return from AddString
    enum { STRLST_FAIL=0, STRLST_DUPLICATE=1, STRLST_ADDED=2 };

// iInitBufSize is minimum starting buffer size, or -1 for default
virtual BOOL Init(int iInitBufSize=-1);

virtual int   AddString(LPCWSTR lpwstr, DWORD_PTR dwData = 0, int *piNum = NULL);
virtual DWORD_PTR GetStringData(int iNum) { return 0; }
virtual void  SetStringData(int iNum, DWORD_PTR dw) { return; }

    int     NumStrings() { return m_iNumStrings; }

    // iLen must be length in characters of string, not counting null term.
    // -1 if unknown.
    BOOL    FindString(LPCWSTR lpwstr, int iLen, int *piNum=NULL);

    // Returns const pointer to within stringlist's memory
    LPCWSTR GetString    (int iNum)
                {
                    ASSERT(iNum < m_iNumStrings);
                    return m_psiStrings[iNum].lpwstr;
                }

    // Returns length of string in characters
    int     GetStringLen (int iNum)
                { 
                    ASSERT(iNum < m_iNumStrings);
                    return m_psiStrings[iNum].iLen;
                }

    // Returns new BSTR. Free with SysFreeString when you're done.
    BSTR    GetBSTR     (int iNum);

    // IUnknown members
//  STDMETHODIMP         QueryInterface(REFIID riid, void **punk);
//  STDMETHODIMP_(ULONG) AddRef(void);
//  STDMETHODIMP_(ULONG) Release(void);

    // IPersistStream members
//  STDMETHODIMP         GetClassID(CLSID *pClassID);
    STDMETHODIMP         IsDirty(void);         // Always returns TRUE
    STDMETHODIMP         Load(IStream *pStm);
    STDMETHODIMP         Save(IStream *pStm, BOOL fClearDirty);
    STDMETHODIMP         GetSizeMax(ULARGE_INTEGER *pcbSize);

    enum { DEFAULT_INIT_BUF_SIZE = 4096 };

protected:
    void    CleanUp();
    void    Clear();
    void    Reset();

    BOOL    InitializeFromBuffer();

    BOOL    m_fValid;                   // Are our buffers initialized?
    int     m_iNumStrings;              // # of strings so far.
    int     m_iMaxStrings;              // # of elements in m_psiStrings

private:
    typedef struct tagStringIndex {
        LPCWSTR         lpwstr;  // pointer to string text in m_pBuffer
        int             iLen;    // length of this string in characters w/o null term
        tagStringIndex* psiNext; // index of next string with same hash value
    } STRING_INDEX, *PSTRING_INDEX, *LPSTRING_INDEX;

    LPSTR   m_pBuffer;                  // Holds all strings
    int     m_iBufEnd;                  // Last byte used in buffer
    int     m_iBufSize;

    LPSTRING_INDEX  m_psiStrings;               // dynamically allocated array
    LPSTRING_INDEX  m_Hash[STRING_HASH_SIZE];   // hash table (array of ptrs within m_psiStrings)
    int             m_iLastHash;                // used to avoid recalculating hashes

    BOOL InsertToHash(LPCWSTR lpwstr, int iLen, BOOL fAlreadyHashed);
    int Hash(LPCWSTR lpwstr, int iLen)
    {
        unsigned long hash=0;

        while (iLen--)
        {
            hash = (hash<<5) + hash + *lpwstr++;
        }

        return m_iLastHash = (int)(hash % STRING_HASH_SIZE);
    }

#ifdef DEBUG
    void SpewHashStats(BOOL fVerbose);
#endif
};

// Helper macros to create the string lists
inline CWCStringList *CreateCWCStringList(int iInitBufSize)
{
    CWCStringList *pRet = new CWCStringList();
    if (pRet->Init(iInitBufSize))
    {
        return pRet;
    }
    delete pRet;
    return NULL;
}

// CWCDwordStringList stores an extra DWORD of data along with each string.
// This data does not get persisted

class CWCDwordStringList : public CWCStringList {

public:
    CWCDwordStringList();
    ~CWCDwordStringList();

    // these are all virtual
    BOOL    Init(int iInitBufSize=-1);
    int     AddString(LPCWSTR psz, DWORD_PTR dwData = 0, int *piNum = NULL);
    DWORD_PTR GetStringData(int iNum) { return m_pData[iNum]; }
    void    SetStringData(int iNum, DWORD_PTR dw) { m_pData[iNum] = dw; }

private:
    DWORD_PTR *m_pData;      // data our caller wants attached to the strings
};

#endif // _STRINGLST_H

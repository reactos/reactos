//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
#include "private.h"
#include "strlist.h"

#define TF_THISMODULE   TF_STRINGLIST

//----------------------------------------------------------------------------
// CWCStringList

CWCStringList::CWCStringList()
{
    // We may be able to remove this Clear() call if we guarantee that
    //  1) the new operator zero inits
    //  2) we don't use this class on the stack
    Clear();
}

CWCStringList::~CWCStringList()
{
#ifdef DEBUG
    if (m_iNumStrings > 100)
        SpewHashStats(TRUE);
#endif

    CleanUp();
}

// Clean up any allocated memory
void CWCStringList::CleanUp()
{
    if (!m_fValid)
        return;

    if (m_pBuffer)
    {
        MemFree(m_pBuffer);
        m_pBuffer = NULL;
    }

    if (m_psiStrings)
    {
        MemFree(m_psiStrings);
        m_psiStrings = NULL;
    }

    m_fValid = FALSE;
}

// Clear our internal structures to prepare to be initialized. Assumes we
//  have no allocated memory (call CleanUp())
void CWCStringList::Clear()
{
    m_fValid = FALSE;
    m_iBufEnd = m_iBufSize = m_iNumStrings = m_iMaxStrings = 0;
    m_pBuffer = NULL;
    m_psiStrings = NULL;
    ZeroMemory(m_Hash, sizeof(m_Hash));
}

void CWCStringList::Reset()
{
    if (m_fValid || m_pBuffer || m_psiStrings)
    {
        CleanUp();
        Clear();
    }
}

BOOL CWCStringList::Init(int iInitBufSize)
{
    if (m_fValid)
    {
        DBG("WCStringList::Init called when already initialized");
        Reset();
    }

    if (iInitBufSize <= 0)
    {
        iInitBufSize = DEFAULT_INIT_BUF_SIZE;
    }

    m_iMaxStrings = iInitBufSize >> 5;  // this is relatively arbitrary but doesn't matter much

    m_pBuffer = (LPSTR)MemAlloc(LMEM_FIXED, iInitBufSize);
    m_psiStrings = (LPSTRING_INDEX)MemAlloc(LMEM_FIXED, m_iMaxStrings * sizeof(STRING_INDEX));

    if ((NULL == m_psiStrings) ||
        (NULL == m_pBuffer))
    {
        DBG_WARN("Init() memory allocation failed");

        CleanUp();
        return FALSE;
    }

    *m_pBuffer = 0;

    m_iBufSize = iInitBufSize;
    m_iBufEnd = 0;
    m_fValid = TRUE;

    return TRUE;
}


// Sets up our internal data structures (hash and string_index)
// Sets m_iBufEnd.
// We must already be Init()ialized and the data in m_pBuffer
BOOL CWCStringList::InitializeFromBuffer()
{
    LPCWSTR pNext;
    int iLen;

    if (!m_fValid)
        return FALSE;

    pNext = (LPCWSTR)m_pBuffer;

    while (((LPSTR)pNext-m_pBuffer) < m_iBufSize)
    {
        iLen = lstrlenW(pNext);
        InsertToHash(pNext, iLen, FALSE);
        pNext += iLen+1;
    }

    m_iBufEnd = (int)((LPSTR)pNext - m_pBuffer);

    return TRUE;
}


//
// IPersistStream members
//
// We save
// DWORD containing total length in bytes that follows. Will be
//  multiple of four; may have 0-4 extra pad bytes on the end.
// String data.
//
// Smallest data we store is 4 bytes of zeroes. We still end up taking
//  memory when we get restored. Don't instantiate one of these objects
//  until you're going to use it.
STDMETHODIMP CWCStringList::IsDirty(void)
{
    DBG("CWCStringList::IsDirty returning S_OK (true) as always");

    return S_OK;
}

STDMETHODIMP CWCStringList::Load(IStream *pStm)
{
    HRESULT hr;
    ULONG   cbRead;
    DWORD   dwDataSize;

    DBG("CWCStringList::Load");

    if (NULL==pStm)
        return E_POINTER;

    // Clean up our object
    Reset();

    // Load our data
    hr = pStm->Read(&dwDataSize, sizeof(DWORD), &cbRead);
    if (FAILED(hr) || cbRead != sizeof(DWORD))
        return STG_E_READFAULT;

    if (0 == dwDataSize)
    {
        if (!Init(512))     // Start with small buffer since we're empty
            return E_OUTOFMEMORY;
        return S_OK;
    }

    if (!Init(dwDataSize))
        return E_OUTOFMEMORY;

    ASSERT(dwDataSize <= (DWORD)m_iBufSize);

    // Read in the string data
    hr = pStm->Read(m_pBuffer, dwDataSize, &cbRead);
    if (FAILED(hr) || cbRead != dwDataSize)
        return STG_E_READFAULT;

    // Set up hash tables etc.
    InitializeFromBuffer();

    DBG("CWCStringList::Load success");

    return NOERROR;
}

STDMETHODIMP CWCStringList::Save(IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr;
    ULONG   cbWritten;
    DWORD   dwDataSize, dwZero=0;
    DWORD   dwZeroPad;

    DBG("CWCStringList::Save");

    if (NULL==pStm)
        return E_POINTER;

    // First write our data
    dwDataSize = (m_iBufEnd+3) & 0xFFFFFFFC; // multiple of four

    if ((0 == m_iBufSize) || (0 == m_iNumStrings))
    {
        dwDataSize = 0;
    }

    hr = pStm->Write(&dwDataSize, sizeof(DWORD), &cbWritten);
    if (FAILED(hr) || sizeof(DWORD) != cbWritten)
        return STG_E_WRITEFAULT;

    if (dwDataSize > 0)
    {
        hr = pStm->Write(m_pBuffer, m_iBufSize, &cbWritten);
        if (FAILED(hr) || sizeof(DWORD) != cbWritten)
            return STG_E_WRITEFAULT;

        dwZeroPad = dwDataSize - m_iBufSize;

        ASSERT(dwZeroPad<4);
        if (dwZeroPad && dwZeroPad<4)
        {
            hr = pStm->Write(&dwZero, dwZeroPad, &cbWritten);
            if (FAILED(hr) || (dwZeroPad != cbWritten))
                return STG_E_WRITEFAULT;
        }
    }

    DBG("CWCStringList::Save success");

    return NOERROR;
}

STDMETHODIMP CWCStringList::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    DBG("CWCStringList::GetSizeMax");

    if (NULL==pcbSize)
        return E_POINTER;

    pcbSize->LowPart = 0;
    pcbSize->HighPart = 0;

    pcbSize->LowPart = m_iBufEnd + 8;

    return NOERROR;
}

// Returns a BSTR
BSTR CWCStringList::GetBSTR(int iNum)
{
    LPCWSTR lpStr = GetString(iNum);

    return SysAllocStringLen(lpStr, GetStringLen(iNum));
}

// Returns FALSE if string is not found
// Places string index (for GetString()) in *piNum only if string is found.
BOOL CWCStringList::FindString(LPCWSTR lpwstr, int iLen, int *piNum/*=NULL*/)
{
    int             iHash;
    LPSTRING_INDEX  psi;

    if (!lpwstr)
        return FALSE;

    if (iLen < 0)
        iLen = lstrlenW(lpwstr);

    iHash = Hash(lpwstr, iLen);
    for (psi = m_Hash[iHash]; psi; psi = psi->psiNext)
    {
        if ((psi->iLen == iLen) && memcmp(psi->lpwstr, lpwstr, iLen * sizeof(WCHAR)) == 0)
        {
            if (piNum)
                *piNum = (int) (psi-m_psiStrings);
            return TRUE;        // String is a duplicate
        }
    }

    return FALSE;
}

// returns STRLST_FAIL on failure,
//         STRLST_DUPLICATE if the string already existed, and
//         STRLST_ADDED if it's new
int CWCStringList::AddString(LPCWSTR lpwstr, DWORD_PTR dwData /*=NULL*/, int *piNum /*=NULL*/)
{
    int iSize, iLen;

    if (!lpwstr)
        return STRLST_FAIL;

    iLen = lstrlenW(lpwstr);

    if (!m_fValid || !m_pBuffer)
    {
        DBG_WARN("WCStringList: AddString() called with invalid instance");
        return STRLST_FAIL;
    }

    if (dwData != 0)
        DBG_WARN("Value for dwData passed into CWCStringList::AddString");

    if (FindString(lpwstr, iLen, piNum))
        return STRLST_DUPLICATE;        // String is a duplicate

    // iSize will be size in bytes including null term
    iSize = (iLen+1)*sizeof(WCHAR);

    // Append string to current buffer
    if (iSize >= (m_iBufSize - m_iBufEnd))
    {
        int iOldBufSize = m_iBufSize;

        // Grow buffer.
        m_iBufSize *= 2;     // This way the number of reallocs drops off logarithmically
        if (m_iBufEnd + iSize > m_iBufSize)
        {
            DBG("StringList special growing size");
            m_iBufSize = m_iBufEnd + iSize;
        }

        TraceMsg(TF_THISMODULE,"StringList growing to size %d",m_iBufSize);

        LPSTR pBuf = (LPSTR)MemReAlloc((HLOCAL)m_pBuffer, m_iBufSize, LMEM_MOVEABLE);
        if (!pBuf)
        {
            m_iBufSize = iOldBufSize;
            DBG_WARN("WCStringList: ReAlloc() failure");
            // Realloc failure: our old memory is still present
            return 0;
        }
        // Let's be clever and fix all our pointers instead of getting faults
        if (m_pBuffer != pBuf)
        {
            int i;
            LPSTRING_INDEX psi;
            for (i=0, psi=&m_psiStrings[0]; i<m_iNumStrings; i++, psi++)
            {
                psi->lpwstr = (LPWSTR)(((LPSTR)psi->lpwstr - m_pBuffer) + pBuf);
            }

            m_pBuffer = pBuf;
        }
    }

    if (piNum)
        *piNum = m_iNumStrings;

    LPWSTR pBufEnd = (LPWSTR)(m_pBuffer + m_iBufEnd);

    StrCpyW(pBufEnd, lpwstr);
    if (!InsertToHash(pBufEnd, iLen, TRUE))
        return 0;
    m_iBufEnd += iSize;

    return STRLST_ADDED;           // indicate we added a new string
}


BOOL CWCStringList::InsertToHash(LPCWSTR lpwstr, int iLen, BOOL fAlreadyHashed)
{
    int iHash = fAlreadyHashed ? m_iLastHash : Hash(lpwstr, iLen);

    // grow psiStrings if needed
    ASSERT(m_iNumStrings <= m_iMaxStrings);
    if (m_iNumStrings >= m_iMaxStrings)
    {
        m_iMaxStrings *= 2;
        TraceMsg(TF_THISMODULE, "StringList growing max strings to %d", m_iMaxStrings);
        LPSTRING_INDEX psiBuf = (LPSTRING_INDEX)MemReAlloc((HLOCAL)m_psiStrings,
            m_iMaxStrings * sizeof(STRING_INDEX), LMEM_MOVEABLE);
        if (!psiBuf)
        {
            // Realloc failure: Old memory still present
            DBG_WARN("WCStringList::InsertToHash() ReAlloc failure");
            m_iMaxStrings /= 2;
            return FALSE;
        }
        // More cleverness
        if (m_psiStrings != psiBuf)
        {
            int i;
            LPSTRING_INDEX psi, *ppsi;

            for (i=0, psi=psiBuf; i<m_iNumStrings; i++, psi++)
            {
                if (psi->psiNext)
                    psi->psiNext = (psi->psiNext - m_psiStrings) + psiBuf;
            }
            for (i=0, ppsi=m_Hash; i<STRING_HASH_SIZE; i++, ppsi++)
            {
                if (*ppsi)
                    *ppsi = (*ppsi - m_psiStrings) + psiBuf;
            }

            m_psiStrings = psiBuf;
        }
    }

    m_psiStrings[m_iNumStrings].lpwstr  = lpwstr;
    m_psiStrings[m_iNumStrings].iLen    = iLen;
    m_psiStrings[m_iNumStrings].psiNext = m_Hash[iHash];
    m_Hash[iHash] = &m_psiStrings[m_iNumStrings];
    m_iNumStrings++;
    return TRUE;
}


#ifdef DEBUG
// WARNING: this clobbers the hash
void CWCStringList::SpewHashStats(BOOL fVerbose)
{
/*
    int i;
    for (i = 0; i < STRING_HASH_SIZE; ++i)
    {
        int c = 0;

        for (tagStringIndex *p = m_Hash[i]; p; p = p->psiNext)
            ++c;

        if (c)
            TraceMsg(TF_THISMODULE,"%10d%12d", i, c);
    }
*/

 
    TraceMsg(TF_THISMODULE,"### Hash size: %d       Num. entries:%7d", STRING_HASH_SIZE, m_iNumStrings);
    int i,n;
    if (fVerbose)
    {
        TraceMsg(TF_THISMODULE," # of entries    # of keys with that many");
        for (i=0,n=0; n<STRING_HASH_SIZE; i++)
        {
            int k=0;
            for (int j=0; j<STRING_HASH_SIZE; j++)
            {
                int c=0;
                for (tagStringIndex* p=m_Hash[j]; p; p=p->psiNext)
                    c++;
                if (c == i)
                    k++;
            }
            if (k)
            {
                TraceMsg(TF_THISMODULE,"%10d%12d", i, k);
                n += k;
            }
        }
    }

/*
    int total=0;
    if (fVerbose)
    {
        TraceMsg(TF_THISMODULE," length   # of strings with that length",1);
        for (i=0,n=0; n<m_iNumStrings; i++)
        {
            int k=0;
            for (int j=0; j<m_iNumStrings; j++)
            {
                if (m_psiStrings[j].iLen == i)
                    k++;
            }
            if (k)
            {
                if (fVerbose)
                    TraceMsg(TF_THISMODULE,"%5d%10d", i, k);
                n += k;
                total += k*(k+1)/2;
            }
        }
    }
    TraceMsg(TF_THISMODULE,"### Average compares without hash * 100:%5d", total*100/m_iNumStrings);

    total=0;
    for (i=0; i<STRING_HASH_SIZE; i++)
    {
        for (tagStringIndex* p=m_Hash[i]; p; p=p->psiNext)
        {
            if (p->iLen < 0) continue;
            int n=1;
            for (tagStringIndex* q=p->psiNext; q; q=q->psiNext)
            {
                if (p->iLen == q->iLen)
                {
                    n++;
                    q->iLen = -1;
                }
            }
            total += n*(n+1)/2;
        }
    }
    TraceMsg(TF_THISMODULE,"### Average compares with hash * 100:%8d", total*100/m_iNumStrings);
*/
}
#endif

//----------------------------------------------------------------------------
// CWCDwordStringList
CWCDwordStringList::CWCDwordStringList() : CWCStringList()
{
}

CWCDwordStringList::~CWCDwordStringList()
{
    if (m_pData)
        MemFree((HLOCAL)m_pData);
}

BOOL CWCDwordStringList::Init(int iInitBufSize/*=-1*/)
{
    if (!CWCStringList::Init(iInitBufSize))
        return FALSE;

    m_pData = (DWORD_PTR*)MemAlloc(LMEM_FIXED, m_iMaxStrings * sizeof(DWORD));
    if (NULL == m_pData)
        return FALSE;

    return TRUE;
}

int CWCDwordStringList::AddString(LPCWSTR psz, DWORD_PTR dwData/*=0*/, int* piNum/*=NULL*/)
{
    int iOldMaxStrings = m_iMaxStrings;     // track changes in m_iMaxStrings by this call:
    int iNum;
    int iResult = CWCStringList::AddString(psz, 0, &iNum);

    if (iResult == 0)
        return 0;

    if (iOldMaxStrings != m_iMaxStrings)    // make sure we have enough data space
    {
        DWORD_PTR *pData;
//      TraceMsg(TF_THISMODULE, "DwordStringList expanding dwords to %d", m_iMaxStrings);
        pData = (DWORD_PTR*)MemReAlloc((HLOCAL)m_pData, m_iMaxStrings * sizeof(DWORD),
            LMEM_MOVEABLE);
        ASSERT(pData);
        if (!pData)
        {
            DBG_WARN("Realloc failure in DwordStringList");
            MemFree(m_pData);
            m_pData = NULL;         // This is bad
            return 0;
        }

        m_pData = pData;
    }

    if (iResult == 2)       // only set data value if this is a new string
    {
        m_pData[iNum] = dwData;
    }

    if (piNum)
        *piNum = iNum;
    return iResult;
}

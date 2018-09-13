/* mime64 */
/* MIME base64 encoder/decoder by Karl Hahn  hahn@lds.loral.com  3-Aug-94 */
/* Modified into an API by georgep@microsoft.com 8-Jan-96 */

#include "priv.h"

#include "mime64.h"

#define INVALID_CHAR (ULONG)-2
#define IGNORE_CHAR (ULONG)-1

extern "C" void DllAddRef();
extern "C" void DllRelease();

class CRefDll
{
public:
    CRefDll() {DllAddRef();}
    ~CRefDll() {DllRelease();}
} ;


HRESULT CopyTo(IStream* pstmIn, 
    /* [unique][in] */ IStream *pstm,
    /* [in] */ ULARGE_INTEGER cb,
    /* [out] */ ULARGE_INTEGER *pcbRead,
    /* [out] */ ULARGE_INTEGER *pcbWritten)
{
    if (cb.HighPart != 0)
        return E_INVALIDARG;

    DWORD dwBytes = cb.LowPart;
    DWORD dwStep = dwBytes;
    if (dwStep >= 0x8000)
        dwStep = 0x8000;

    LPVOID pv = GlobalAlloc(GPTR, dwStep);
    if (!pv)
        return E_OUTOFMEMORY;

    DWORD dwTotRead = 0;
    DWORD dwTotWrite = 0;
    HRESULT hres = NOERROR;

    for (dwBytes; dwBytes!=0; )
    {
        DWORD dwThisRead = dwStep;
        if (dwThisRead > dwBytes)
        {
            dwThisRead = dwBytes;
        }

        if (NOERROR!=pstmIn->Read(pv, dwThisRead, &dwThisRead) || !dwThisRead)
        {
            // Must be the end of the file
            break;
        }
        dwTotRead += dwThisRead;

        DWORD dwWrite;
        hres = pstm->Write(pv, dwThisRead, &dwWrite);
        if (FAILED(hres))
        {
            break;
        }
        dwTotWrite += dwWrite;

        if (dwWrite != dwThisRead)
        {
            hres = E_UNEXPECTED;
            break;
        }

        dwBytes -= dwThisRead;
    }

    GlobalFree(pv);

    if (pcbRead)
    {
        pcbRead->HighPart = 0;
        pcbRead->LowPart = dwTotRead;
    }

    if (pcbWritten)
    {
        pcbWritten->HighPart = 0;
        pcbWritten->LowPart = dwTotWrite;
    }

    return(hres);
}

#undef new  // Hack! need to resolve this (edwardp)

class CStreamMem : public IStream
{
private:
    CStreamMem(UINT cbSize) : m_cbSize(cbSize), m_cRef(1), m_cbPos(0) {}
    void* operator new(size_t cbClass, UINT cbSize);

public:
    static CStreamMem *Construct(UINT cbSize);
    LPVOID GetPtr() { return(m_vData); }
    void SetSize(UINT cbSize) { m_cbSize = cbSize; }

    // IUnknown
        virtual STDMETHODIMP QueryInterface( 
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject);
        
        virtual STDMETHODIMP_(ULONG) AddRef(void)
        { return(++m_cRef); }
        
        virtual STDMETHODIMP_(ULONG) Release(void);

    // IStream
        virtual STDMETHODIMP Read( 
            /* [out] */ void *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbRead);
        
        virtual STDMETHODIMP Write( 
            /* [size_is][in] */ const void *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbWritten)
        { return(E_NOTIMPL); }
        
        virtual STDMETHODIMP Seek( 
            /* [in] */ LARGE_INTEGER dlibMove,
            /* [in] */ DWORD dwOrigin,
            /* [out] */ ULARGE_INTEGER *plibNewPosition);
        
        virtual STDMETHODIMP SetSize( 
            /* [in] */ ULARGE_INTEGER libNewSize)
        { return(E_NOTIMPL); }
        
        virtual STDMETHODIMP CopyTo( 
            /* [unique][in] */ IStream *pstm,
            /* [in] */ ULARGE_INTEGER cb,
            /* [out] */ ULARGE_INTEGER *pcbRead,
            /* [out] */ ULARGE_INTEGER *pcbWritten)
        { return (::CopyTo(this, pstm, cb, pcbRead, pcbWritten)); }
        
        virtual STDMETHODIMP Commit( 
            /* [in] */ DWORD grfCommitFlags)
        { return(E_NOTIMPL); }
        
        virtual STDMETHODIMP Revert( void)
        { return(E_NOTIMPL); }
        
        virtual STDMETHODIMP LockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType)
        { return(E_NOTIMPL); }
        
        virtual STDMETHODIMP UnlockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType)
        { return(E_NOTIMPL); }
        
        virtual STDMETHODIMP Stat( 
            /* [out] */ STATSTG *pstatstg,
            /* [in] */ DWORD grfStatFlag)
        { return(E_NOTIMPL); }
        
        virtual STDMETHODIMP Clone( 
            /* [out] */ IStream **ppstm)
        { return(E_NOTIMPL); }

private:
    CRefDll m_cRefDll;

    ULONG m_cRef;

    UINT m_cbSize;
    UINT m_cbPos;

    // Must be the last field in the class
    BYTE m_vData[1];
} ;


void* CStreamMem::operator new(size_t cbClass, UINT cbSize)
{
    return(::operator new(cbClass + cbSize - 1));
}


CStreamMem *CStreamMem::Construct(UINT cbSize)
{
    return(new(cbSize) CStreamMem(cbSize));
}


STDMETHODIMP CStreamMem::QueryInterface( 
    /* [in] */ REFIID riid,
    /* [out] */ void **ppvObject)
{
    if (riid==IID_IUnknown || riid==IID_IStream)
    {
        AddRef();
        *ppvObject = (LPVOID)(IStream*)this;
        return(NOERROR);
    }

    *ppvObject = NULL;
    return(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CStreamMem::Release( void)
{
    --m_cRef;
    if (!m_cRef)
    {
        delete this;
        return(0);
    }

    return(m_cRef);
}


// IStream
STDMETHODIMP CStreamMem::Read( 
    /* [out] */ void *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbRead)
{
    if (pcbRead)
    {
        *pcbRead = 0;
    }

    if (m_cbPos >= m_cbSize)
    {
        return(S_FALSE);
    }

    ULONG cbRest = m_cbSize - m_cbPos;
    if (cb > cbRest)
    {
        cb = cbRest;
    }

    CopyMemory(pv, m_vData + m_cbPos, cb);
    m_cbPos += cb;

    if (pcbRead)
    {
        *pcbRead = cb;
    }

    return(S_OK);
}


STDMETHODIMP CStreamMem::Seek( 
    /* [in] */ LARGE_INTEGER dlibMove,
    /* [in] */ DWORD dwOrigin,
    /* [out] */ ULARGE_INTEGER *plibNewPosition)
{
    LONG lOffset = (LONG)dlibMove.LowPart;

    // Make sure we are only using 32 bits
    if (dlibMove.HighPart==0 && lOffset>=0)
    {
    }
    else if (dlibMove.HighPart==-1 && lOffset<0)
    {
    }
    else
    {
        return(E_INVALIDARG);
    }

    switch (dwOrigin)
    {
    case STREAM_SEEK_SET:
        break;

    case STREAM_SEEK_CUR:
        lOffset = (LONG)m_cbPos + lOffset;
        break;

    case STREAM_SEEK_END:
        lOffset = (LONG)m_cbSize + lOffset;
        break;

    default:
        return(E_INVALIDARG);
    }

    // Check the new offset is in range (NOTE: it is valid to seek past "end of file")
    if (lOffset < 0)
    {
        return(E_INVALIDARG);
    }

    // Store the new offset and return it
    m_cbPos = (ULONG)lOffset;

    if (plibNewPosition)
    {
        plibNewPosition->HighPart = 0;
        plibNewPosition->LowPart = m_cbPos;
    }

    return(S_OK);
}


ULONG _BinaryFromASCII2( unsigned char alpha )
{
    switch (alpha)
    {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
        return(IGNORE_CHAR);

    default:
        if      ( (alpha >= 'A') && (alpha <= 'Z') )
        {
            return (int)(alpha - 'A');
        }
        else if ( (alpha >= 'a') && (alpha <= 'z') )
        {
            return 26 + (int)(alpha - 'a');
        }
        else if ( (alpha >= '0') && (alpha <= '9' ) )
        {
            return 52 + (int)(alpha - '0');
        }

        return(INVALID_CHAR);

    case '+':
        return 62;

    case '/':
        return 63;
    }
}

#if 0
struct _BinASCIIData
{
    BOOL m_bInited;
    ULONG m_anBinary[256];
} g_cBinASCIIData = { FALSE } ;

void _InitTables()
{
    if (g_cBinASCIIData.m_bInited)
    {
        return;
    }

    for (int i=0; i<256; ++i)
    {
        // Note this is thread-safe, since we always set to the same value
        g_cBinASCIIData.m_anBinary[i] = _BinaryFromASCII2((unsigned char)i);
    }

    // Set after initing other values to make thread-safe
    g_cBinASCIIData.m_bInited = TRUE;
}


inline ULONG _BinaryFromASCII( unsigned char alpha )
{
    return(g_cBinASCIIData.m_anBinary[alpha]);
}


HRESULT Mime64Decode(LPCMSTR pStrData, IStream **ppstm)
{
    *ppstm = NULL;

    _InitTables();

    // Waste some bytes so I don't have to worry about overflow
    CStreamMem *pstm = CStreamMem::Construct((lstrlen(pStrData)*3)/4 + 2);
    if (!pstm)
    {
        return(E_OUTOFMEMORY);
    }

    LPBYTE pData = (LPBYTE)pstm->GetPtr();
    int cbData = 0;

    int shift = 0;
    unsigned long accum = 0;

    BOOL quit = FALSE;

    // This loop will ignore white space, but quit at any other invalid characters
    for ( ; ; ++pStrData)
    {
        unsigned long value = _BinaryFromASCII(*pStrData);

        if ( value < 64 )
        {
            accum <<= 6;
            shift += 6;
            accum |= value;

            if ( shift >= 8 )
            {
                shift -= 8;
                value = accum >> shift;
                pData[cbData++] = (BYTE)value & 0xFF;
            }
        }
        else if (IGNORE_CHAR == value)
        {
            continue;
        }
        else
        {
            break;
        }
    }

    pstm->SetSize(cbData);
    *ppstm = pstm;

    return(NOERROR);
}
#endif

#define CHARS_PER_LINE 60


class COutputChars
{
public:
    COutputChars(LPMSTR pStrData) : m_pStrData(pStrData), m_cbLine(0) {}
    void AddChar(MCHAR cAdd)
    {
        *m_pStrData++ = cAdd;
        if (++m_cbLine == CHARS_PER_LINE)
        {
            *m_pStrData++ = '\n';
            m_cbLine = 0;
        }
    }
    LPMSTR GetPtr() { return(m_pStrData); }

private:
    LPMSTR m_pStrData;
    UINT m_cbLine;
} ;


HRESULT Mime64Encode(LPBYTE pData, UINT cbData, IStream **ppstm)
{
    static char const alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
                      "0123456789+/";

    *ppstm = NULL;

    // Waste some bytes so I don't have to worry about overflow
    // The 81/80 is to add a '\n' at the end of every 80 characters
    CStreamMem *pstm = CStreamMem::Construct((((cbData*4)/3 + 4)
        *(CHARS_PER_LINE+1)/CHARS_PER_LINE+2)*sizeof(MCHAR));
    if (!pstm)
    {
        return(E_OUTOFMEMORY);
    }

    COutputChars cStrData((LPMSTR)pstm->GetPtr());
    LPMSTR pSaveData = cStrData.GetPtr();

    int shift = 0;
    int save_shift = 0;
    unsigned long accum = 0;
    int index = 0;
    unsigned char blivit;
    unsigned long value;
    BOOL quit = FALSE;

    while ( ( cbData ) || (shift != 0) )
    {
        if ( ( cbData ) && ( quit == FALSE ) )
        {
            blivit = *pData++;
            --cbData;
        }
        else
        {
            quit = TRUE;
            save_shift = shift;
            blivit = 0;
        }

        if ( (quit == FALSE) || (shift != 0) )
        {
            value = (unsigned long)blivit;
            accum <<= 8;
            shift += 8;
            accum |= value;
        } /* ENDIF */

        while ( shift >= 6 )
        {
            shift -= 6;
            value = (accum >> shift) & 0x3Fl;
            blivit = alphabet[value];

            cStrData.AddChar(blivit);

            if ( quit != FALSE )
            {
                shift = 0;
            }
        }
    }

    if ( save_shift == 2 )
    {
        cStrData.AddChar('=');
        cStrData.AddChar('=');
    }
    else if ( save_shift == 4 )
    {
        cStrData.AddChar('=');
    }

    cStrData.AddChar('\n');
    cStrData.AddChar('\0');

    pstm->SetSize((int)(cStrData.GetPtr()-pSaveData) * sizeof(MCHAR));
    *ppstm = pstm;

    return(NOERROR);
}

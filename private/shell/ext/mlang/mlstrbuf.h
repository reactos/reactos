// MLStrBuf.h : Declaration and implementation of the IMLangStringBufW/A classes

#ifndef __MLSTRBUF_H_
#define __MLSTRBUF_H_

#include <mlang.h>

/////////////////////////////////////////////////////////////////////////////
// CMLStrBufTempl

template <class CHTYPE, class IMLSB, class MEM, class ACCESS>
class CMLStrBufTempl : public IMLSB, public MEM, public ACCESS
{
public:
    CMLStrBufTempl(CHTYPE* psz = NULL, long cch = 0, void* pv = NULL, long cb = 0) : ACCESS(psz, cch, pv, cb)
#ifdef DEBUG
        {m_nLockCount = 0;}
#else
        {}
#endif
    ~CMLStrBufTempl(void) {ASSERT(!m_nLockCount);}

// IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)(void) {return AddRefI();}
    STDMETHOD_(ULONG, Release)(void) {return ReleaseI();}
// IMLangStringBufW/A
    STDMETHOD(GetStatus)(long* plFlags, long* pcchBuf);
    STDMETHOD(LockBuf)(long cchOffset, long cchMaxLock, CHTYPE** ppszBuf, long* pcchBuf);
    STDMETHOD(UnlockBuf)(const CHTYPE* pszBuf, long cchOffset, long cchWrite);
    STDMETHOD(Insert)(long cchOffset, long cchMaxInsert, long* pcchActual) {ASSERT(!m_nLockCount); return InsertI(cchOffset, cchMaxInsert, pcchActual);}
    STDMETHOD(Delete)(long cchOffset, long cchDelete) {ASSERT(!m_nLockCount); return DeleteI(cchOffset, cchDelete);}

protected:
#ifdef DEBUG
    int m_nLockCount;
#endif
};

template <class CHTYPE, class IMLSB, class MEM, class ACCESS>
HRESULT CMLStrBufTempl<CHTYPE, IMLSB, MEM, ACCESS>::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        (sizeof(CHTYPE) == sizeof(CHAR)  && IsEqualIID(riid, IID_IMLangStringBufA)) ||
        (sizeof(CHTYPE) == sizeof(WCHAR) && IsEqualIID(riid, IID_IMLangStringBufW)))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

template <class CHTYPE, class IMLSB, class MEM, class ACCESS>
HRESULT CMLStrBufTempl<CHTYPE, IMLSB, MEM, ACCESS>::GetStatus(long* plFlags, long* pcchBuf)
{
    if (plFlags)
        *plFlags = GetFlags();

    if (pcchBuf)
        *pcchBuf = m_cchStr;

    return S_OK;
}

template <class CHTYPE, class IMLSB, class MEM, class ACCESS>
HRESULT CMLStrBufTempl<CHTYPE, IMLSB, MEM, ACCESS>::LockBuf(long cchOffset, long cchMaxLock, CHTYPE** ppszBuf, long* pcchBuf)
{
    ASSERT(cchOffset >= 0 && cchOffset < m_cchStr);
    ASSERT(cchMaxLock >= 1 && cchMaxLock <= m_cchStr - cchOffset);
#ifdef DEBUG
    m_nLockCount++;
#endif

    if (ppszBuf)
        *ppszBuf = m_pszBuf + cchOffset;

    if (pcchBuf)
        *pcchBuf = cchMaxLock;

    return S_OK;
}

template <class CHTYPE, class IMLSB, class MEM, class ACCESS>
HRESULT CMLStrBufTempl<CHTYPE, IMLSB, MEM, ACCESS>::UnlockBuf(const CHTYPE* pszBuf, long cchOffset, long cchWrite)
{
    ASSERT(m_nLockCount > 0);
    ASSERT(cchOffset >= 0  && pszBuf + cchOffset >= m_pszBuf && pszBuf + cchOffset < m_pszBuf + m_cchStr);
    ASSERT(cchWrite == 0);
#ifdef DEBUG
    m_nLockCount--;
#endif

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CMLStrBufConst

template <class CHTYPE>
class CMLStrBufConst
{
protected:
    CMLStrBufConst(CHTYPE* psz, long cch, void*, long) : m_pszBuf(psz), m_cchStr(cch) {}
    long GetFlags(void) const {return MLSTR_READ;}
    HRESULT InsertI(long cchOffset, long cchMaxInsert, long* pcchActual) {ASSERT(FALSE); if (pcchActual) *pcchActual = 0; return E_FAIL;}
    HRESULT DeleteI(long cchOffset, long cchDelete) {ASSERT(FALSE); return E_FAIL;}

    CHTYPE* const m_pszBuf;
    const long m_cchStr;
};

/////////////////////////////////////////////////////////////////////////////
// CMLStrBufVariable

template <class CHTYPE>
class CMLStrBufVariable
{
protected:
    CMLStrBufVariable(CHTYPE* psz, long cch, void* pv, long cb) {m_pszBuf = (CHTYPE*)pv; m_cchBuf = cb / sizeof(CHTYPE); m_cchOffset = psz - m_pszBuf; m_cchStr = cch;}
    ~CMLStrBufVariable(void) {if (m_pszBuf) MemFree(m_pszBuf);}
    long GetFlags(void) const {return MLSTR_READ | MLSTR_WRITE;}
    HRESULT InsertI(long cchOffset, long cchMaxInsert, long* pcchActual);
    HRESULT DeleteI(long cchOffset, long cchDelete);

    virtual LPVOID MemAlloc(ULONG) {return NULL;}
    virtual LPVOID MemRealloc(LPVOID, ULONG) {return NULL;}
    virtual void MemFree(LPVOID) {}
    virtual long RoundBufSize(long cchStr) {return (cchStr + 15) / 16;}

    CHTYPE* m_pszBuf;
    long m_cchBuf;
    long m_cchOffset;
    long m_cchStr;
};

template <class CHTYPE>
HRESULT CMLStrBufVariable<CHTYPE>::InsertI(long cchOffset, long cchMaxInsert, long* pcchActual)
{
    ASSERT(cchOffset >= 0 && cchOffset < m_cchStr);
    ASSERT(cchMaxInsert >= 0);

    long lShiftLeft = 0;
    long lShiftRight = 0;

    if (cchOffset < m_cchStr - cchOffset &&
        cchMaxInsert <= m_cchOffset)
    {
        lShiftLeft = cchMaxInsert;
    }
    else if (cchMaxInsert <= m_cchBuf - m_cchOffset - m_cchStr)
    {
        lShiftRight = cchMaxInsert;
    }
    else if (cchMaxInsert <= m_cchOffset)
    {
        lShiftLeft = cchMaxInsert;
    }
    else if (cchMaxInsert <= m_cchBuf - m_cchStr)
    {
        lShiftLeft = m_cchOffset;
        lShiftRight = cchMaxInsert - m_cchOffset;
    }
    else
    {
        void* pBuf;
        const long cchNew = RoundBufSize(m_cchOffset + m_cchStr + cchMaxInsert);

        if (!m_pszBuf)
            pBuf = MemAlloc(sizeof(*m_pszBuf) * cchNew);
        else
            pBuf = MemRealloc(m_pszBuf, sizeof(*m_pszBuf) * cchNew);

        if (pBuf)
        {
            m_pszBuf = (WCHAR*)pBuf;
            m_cchBuf = cchNew;
            lShiftRight = cchMaxInsert;
        }
        else
        {
            lShiftRight = m_cchBuf - m_cchOffset - m_cchStr;
            lShiftLeft = cchMaxInsert - lShiftRight;

            if (!pcchActual)
                return E_OUTOFMEMORY;
        }
    }

    if (lShiftLeft > 0)
    {
        if (cchOffset)
            ::memmove(m_pszBuf + m_cchOffset - lShiftLeft, m_pszBuf + m_cchOffset, sizeof(*m_pszBuf) * cchOffset);
        m_cchOffset -= lShiftLeft;
        m_cchStr += lShiftLeft;
    }

    if (lShiftRight > 0)
    {
        if (m_cchStr - cchOffset)
            ::memmove(m_pszBuf + m_cchOffset + lShiftRight, m_pszBuf + m_cchOffset, sizeof(*m_pszBuf) * (m_cchStr - cchOffset));
        m_cchStr += lShiftRight;
    }

    if (pcchActual)
        *pcchActual = lShiftLeft + lShiftRight;

    return S_OK;
}

template <class CHTYPE>
HRESULT CMLStrBufVariable<CHTYPE>::DeleteI(long cchOffset, long cchDelete)
{
    ASSERT(cchOffset >= 0 && cchOffset < m_cchStr);
    ASSERT(cchDelete >= 0 && cchDelete < m_cchStr - cchOffset);

    long cchShrink = m_cchBuf - RoundBufSize(RoundBufSize(m_cchStr - cchDelete) + 1);
    cchShrink = max(cchShrink, 0);

    const long cchRight = m_cchStr - cchOffset - cchDelete;
    if (cchOffset < cchRight && m_cchBuf - m_cchOffset - m_cchStr >= cchShrink)
    {
        if (cchOffset)
            ::memmove(m_pszBuf + m_cchOffset + cchDelete, m_pszBuf + m_cchOffset, sizeof(*m_pszBuf) * cchOffset);
        m_cchOffset += cchDelete;
        m_cchStr -= cchDelete;
    }
    else if (m_cchBuf - m_cchOffset - m_cchStr + cchDelete >= cchShrink)
    {
        if (cchRight)
            ::memmove(m_pszBuf + m_cchOffset + cchOffset, m_pszBuf + m_cchOffset + cchDelete, sizeof(*m_pszBuf) * cchRight);
        m_cchStr -= cchDelete;
    }
    else
    {
        if (cchOffset)
            ::memmove(m_pszBuf, m_pszBuf + m_cchOffset, sizeof(*m_pszBuf) * cchOffset);
        if (cchRight)
            ::memmove(m_pszBuf + cchOffset, m_pszBuf + m_cchOffset + cchDelete, sizeof(*m_pszBuf) * cchRight);

        m_cchOffset = 0;
        m_cchStr -= cchDelete;
    }

    if (cchShrink)
    {
        void* pBuf = MemRealloc(m_pszBuf, sizeof(*m_pszBuf) * (m_cchBuf - cchShrink));

        if (pBuf)
        {
            m_pszBuf = (WCHAR*)pBuf;
            m_cchBuf -= cchShrink;
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CMLStrBufStack

class CMLStrBufStack
{
protected:
#ifdef DEBUG
    inline CMLStrBufStack(void) {m_cRef = 0;}
#else
    inline CMLStrBufStack(void) {}
#endif
    inline ~CMLStrBufStack(void) {ASSERT(!m_cRef);}

    ULONG AddRefI(void)
    {
#ifdef DEBUG
        m_cRef++;
        return m_cRef;
#else
        ASSERT(FALSE);
        return 0;
#endif
    }

    ULONG ReleaseI(void)
    {
#ifdef DEBUG
        m_cRef--;
        ASSERT(m_cRef >= 0);
        return m_cRef;
#else
        ASSERT(FALSE);
        return 0;
#endif
    }

#ifdef DEBUG
    int m_cRef;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CMLStrBufHeap

class CMLStrBufHeap
{
protected:
    inline CMLStrBufHeap(void) {m_cRef = 0;}
    inline ~CMLStrBufHeap(void) {ASSERT(!m_cRef);}
    ULONG AddRefI(void) {m_cRef++; return m_cRef;}
    ULONG ReleaseI(void) {m_cRef--; const int cRef = m_cRef; if (!cRef) delete this; return cRef;}

    int m_cRef;
};

typedef CMLStrBufTempl<WCHAR, IMLangStringBufW, CMLStrBufStack, CMLStrBufConst<WCHAR> > CMLStrBufConstStackW;
typedef CMLStrBufTempl<CHAR,  IMLangStringBufA, CMLStrBufStack, CMLStrBufConst<CHAR>  > CMLStrBufConstStackA;
typedef CMLStrBufTempl<WCHAR, IMLangStringBufW, CMLStrBufStack, CMLStrBufVariable<WCHAR> > CMLStrBufStackW;
typedef CMLStrBufTempl<CHAR,  IMLangStringBufA, CMLStrBufStack, CMLStrBufVariable<CHAR>  > CMLStrBufStackA;
typedef CMLStrBufTempl<WCHAR, IMLangStringBufW, CMLStrBufHeap,  CMLStrBufConst<WCHAR> > CMLStrBufConstW;
typedef CMLStrBufTempl<CHAR,  IMLangStringBufA, CMLStrBufHeap,  CMLStrBufConst<CHAR>  > CMLStrBufConstA;
typedef CMLStrBufTempl<WCHAR, IMLangStringBufW, CMLStrBufHeap,  CMLStrBufVariable<WCHAR> > CMLStrBufW;
typedef CMLStrBufTempl<CHAR,  IMLangStringBufA, CMLStrBufHeap,  CMLStrBufVariable<CHAR>  > CMLStrBufA;

#ifdef UNICODE
typedef CMLStrBufConstStackW CMLStrBufConstStackT;
typedef CMLStrBufStackW CMLStrBufStackT;
typedef CMLStrBufConstW CMLStrBufConstT;
typedef CMLStrBufW CMLStrBufT;
#else
typedef CMLStrBufConstStackA CMLStrBufConstStackT;
typedef CMLStrBufStackA CMLStrBufStackT;
typedef CMLStrBufConstA CMLStrBufConstT;
typedef CMLStrBufA CMLStrBufT;
#endif

#endif //__MLSTRBUF_H_

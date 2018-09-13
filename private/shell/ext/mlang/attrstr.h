// AttrStr.h : Declaration of the CMLStrAttrStrCommon

#ifndef __ATTRSTR_H_
#define __ATTRSTR_H_

#include "mlatl.h"
#include "mlstrbuf.h"

/////////////////////////////////////////////////////////////////////////////
// CMLStrAttrStrCommon
class CMLStrAttrStrCommon
{
    typedef HRESULT (CMLStrAttrStrCommon::*PFNUNLOCKPROC)(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen);

public:
    CMLStrAttrStrCommon(void);

protected:
    class CLockInfo
    {
    protected:
        class CLockInfoEntry
        {
        public:
            void* m_psz;
            PFNUNLOCKPROC m_pfnUnlockProc;
            long m_lFlags;
            UINT m_uCodePage;
            long m_lPos;
            long m_lLen;
            long m_cchPos;
            long m_cchLen;
        };

    public:
        CLockInfo(CMLStrAttrStrCommon* pCommon) : m_pCommon(pCommon)
        {
            m_nLockCount = 0;
            m_pLockArray = NULL;
        }
        ~CLockInfo(void)
        {
            UnlockAll();
        }
        HRESULT UnlockAll(void);
        HRESULT StartLock(BOOL fWrite)
        {
            if (fWrite && !m_nLockCount)
                m_nLockCount = -1; // Negative means write lock
            else if (!fWrite && m_nLockCount >= 0)
                m_nLockCount++;
            else
                return MLSTR_E_ACCESSDENIED;
            return S_OK;
        }
        HRESULT EndLock(BOOL fWrite)
        {
            ASSERT(m_nLockCount);
            if (fWrite)
                m_nLockCount = 0;
            else
                m_nLockCount--;
            return S_OK;
        }
        HRESULT Lock(PFNUNLOCKPROC pfnUnlockProc, long lFlags, UINT uCodePage, void* psz, long lPos, long lLen, long cchPos, long cchLen);
        HRESULT Find(const void* psz, long cch, void** ppKey);
        HRESULT Unlock(void* pKey, const void* psz, long cch, long* pcchActual, long* plActualLen);
        long GetFlags(void* pKey) {return ((CLockInfoEntry*)pKey)->m_lFlags;}
        UINT GetCodePage(void* pKey) {return ((CLockInfoEntry*)pKey)->m_uCodePage;}
        long GetPos(void* pKey) {return ((CLockInfoEntry*)pKey)->m_lPos;}
        long GetLen(void* pKey) {return ((CLockInfoEntry*)pKey)->m_lLen;}
        long GetCChPos(void* pKey) {return ((CLockInfoEntry*)pKey)->m_cchPos;}
        long GetCChLen(void* pKey) {return ((CLockInfoEntry*)pKey)->m_cchLen;}

    protected:
        CMLStrAttrStrCommon* const m_pCommon;
        int m_nLockCount;
        CLockInfoEntry* m_pLockArray;
    };

    class CMLStrBufStandardW : public CMLStrBufW
    {
    protected:
        LPVOID MemAlloc(ULONG cb) {return ::CoTaskMemAlloc(cb);}
        LPVOID MemRealloc(LPVOID pv, ULONG cb) {return ::CoTaskMemRealloc(pv, cb);}
        void MemFree(LPVOID pv) {::CoTaskMemFree(pv);}
        long RoundBufSize(long cchStr);
    };

public:
    class CLock
    {
    public:
        CLock(BOOL fWrite, CMLStrAttrStrCommon* pCommon, HRESULT& hr) : m_fWrite(fWrite), m_pCommon(pCommon) {m_fLocked = (SUCCEEDED(hr) && SUCCEEDED(hr = m_pCommon->GetLockInfo()->StartLock(m_fWrite)));}
        ~CLock(void) {if (m_fLocked) m_pCommon->GetLockInfo()->EndLock(m_fWrite);}
        HRESULT FallThrough(void) {m_fLocked = FALSE; return S_OK;} // Don't call EndLock in destructor
    protected:
        const BOOL m_fWrite;
        CMLStrAttrStrCommon* const m_pCommon;
        BOOL m_fLocked;
    };

    HRESULT PrepareMLStrBuf(void);
    HRESULT SetStrBufCommon(void* pMLStrX, long lDestPos, long lDestLen, UINT uCodePage, IMLangStringBufW* pSrcBufW, IMLangStringBufA* pSrcBufA, long* pcchActual, long* plActualLen);
    HRESULT UnlockStrCommon(const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen);
    HRESULT CheckThread(void) {return (m_dwThreadID == ::GetCurrentThreadId()) ? S_OK : E_FAIL;}
    HRESULT RegularizePosLen(long* plPos, long* plLen);
    HRESULT GetLen(long cchOffset, long cchLen, long* plLen);
    HRESULT GetCCh(long cchOffset, long lLen, long* pcchLen);
    static HRESULT CalcLenW(const WCHAR*, long cchLen, long* plLen) {if (plLen) *plLen = cchLen; return S_OK;}
    static HRESULT CalcLenA(UINT uCodePage, const CHAR*,  long cchLen, long* plLen);
    static HRESULT CalcCChW(const WCHAR*, long lLen, long* pcchLen) {if (pcchLen) *pcchLen = lLen; return S_OK;}
    static HRESULT CalcCChA(UINT uCodePage, const CHAR*,  long lLen, long* pcchLen);
    static HRESULT CalcBufSizeW(long lLen, long* pcchSize) {if (pcchSize) *pcchSize = lLen; return S_OK;}
    static HRESULT CalcBufSizeA(long lLen, long* pcchSize) {if (pcchSize) *pcchSize = lLen * 2; return S_OK;}
    static HRESULT ConvAStrToWStr(UINT uCodePage, const CHAR* pszSrc, long cchSrc, WCHAR* pszDest, long cchDest, long* pcchActualA, long* pcchActualW, long* plActualLen);
    static HRESULT ConvWStrToAStr(BOOL fCanStopAtMiddle, UINT uCodePage, const WCHAR* pszSrc, long cchSrc, CHAR* pszDest, long cchDest, long* pcchActualA, long* pcchActualW, long* plActualLen);
    IMLangStringBufW* GetMLStrBufW(void) const {return m_pMLStrBufW;}
    void SetMLStrBufW(IMLangStringBufW* pBuf) {m_pMLStrBufW = pBuf;}
    IMLangStringBufA* GetMLStrBufA(void) const {return m_pMLStrBufA;}
    void SetMLStrBufA(IMLangStringBufA* pBuf) {m_pMLStrBufA = pBuf;}
    UINT GetCodePage(void) const {return m_uCodePage;}
    void SetCodePage(UINT uCodePage) {m_uCodePage = uCodePage;}
    long GetBufFlags(void) const {return m_lBufFlags;}
    void SetBufFlags(long lBufFlags) {m_lBufFlags = lBufFlags;}
    long GetBufCCh(void) const {return m_cchBuf;}
    void SetBufCCh(long cchBuf) {m_cchBuf = cchBuf;}
    LCID GetLocale(void) const {return m_locale;}
    void SetLocale(LCID locale) {m_locale = locale;}
    CLockInfo* GetLockInfo(void) {return &m_LockInfo;}
    HRESULT MemAlloc(ULONG cb, void** ppv) {void* pv = ::CoTaskMemAlloc(cb); if (ppv) *ppv = pv; return (pv) ? S_OK : E_OUTOFMEMORY;}
    HRESULT MemFree(void* pv) {::CoTaskMemFree(pv); return S_OK;}
    HRESULT UnlockWStrDirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen);
    HRESULT UnlockWStrIndirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen);
    HRESULT UnlockAStrDirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen);
    HRESULT UnlockAStrIndirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen);

protected:
    ~CMLStrAttrStrCommon(void);
    virtual IMLStrAttr* GetMLStrAttr(void) = 0;

    DWORD m_dwThreadID;

    IMLangStringBufW* m_pMLStrBufW;
    IMLangStringBufA* m_pMLStrBufA;
    UINT m_uCodePage;
    long m_lBufFlags;
    long m_cchBuf;

    LCID m_locale;

    CLockInfo m_LockInfo;
};

#endif //__ATTRSTR_H_

#ifndef NEWMLSTR

// MLStr.h : Declaration of the CMLStr

#ifndef __MLSTR_H_
#define __MLSTR_H_

#ifdef ASTRIMPL
#include "mlstrw.h"
#endif
#include "mlstra.h"
#ifdef ASTRIMPL
#include "mlstrbuf.h"
#endif

#define MAX_LOCK_COUNT                  4

// Error Code
#define FACILITY_MLSTR                  0x0A15
#define MLSTR_E_ACCESSDENIED            MAKE_HRESULT(1, FACILITY_MLSTR, 1002)
#define MLSTR_E_TOOMANYNESTOFLOCK       MAKE_HRESULT(1, FACILITY_MLSTR, 1003)
#define MLSTR_E_STRBUFNOTAVAILABLE      MAKE_HRESULT(1, FACILITY_MLSTR, 1004)

/////////////////////////////////////////////////////////////////////////////
// CMLStr
class ATL_NO_VTABLE CMLStr :
    public CComObjectRoot,
    public CComCoClass<CMLStr, &CLSID_CMLangString>,
#ifdef ASTRIMPL
    public IMLangString
#else
    public IMLangStringWStr
#endif
{
    typedef HRESULT (CMLStr::*PFNUNLOCKPROC)(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen);

public:
    CMLStr(void);

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMLStr)
        COM_INTERFACE_ENTRY(IMLangString)
#ifdef ASTRIMPL
        COM_INTERFACE_ENTRY_TEAR_OFF(IID_IMLangStringWStr, CMLStrW)
#else
        COM_INTERFACE_ENTRY(IMLangStringWStr)
#endif
        COM_INTERFACE_ENTRY_TEAR_OFF(IID_IMLangStringAStr, CMLStrA)
    END_COM_MAP()

public:
// IMLangString
    STDMETHOD(Sync)(/*[in]*/ BOOL fNoAccess);
    STDMETHOD(GetLength)(/*[out, retval]*/ long* plLen);
    STDMETHOD(SetMLStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ IUnknown* pSrcMLStr, /*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen);
    STDMETHOD(GetMLStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ IUnknown* pUnkOuter, /*[in]*/ DWORD dwClsContext, /*[in]*/ const IID* piid, /*[out]*/ IUnknown** ppDestMLStr, /*[out]*/ long* plDestPos, /*[out]*/ long* plDestLen);
#ifndef ASTRIMPL
// IMLangStringWStr
    STDMETHOD(SetWStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in, size_is(cchSrc)]*/ const WCHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(SetStrBufW)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ IMLangStringBufW* pSrcBuf, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetWStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[out, size_is(cchDest)]*/ WCHAR* pszDest, /*[in]*/ long cchDest, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetStrBufW)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcMaxLen, /*[out]*/ IMLangStringBufW** ppDestBuf, /*[out]*/ long* plDestLen);
    STDMETHOD(LockWStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ long lFlags, /*[in]*/ long cchRequest, /*[out, size_is(,*pcchDest)]*/ WCHAR** ppszDest, /*[out]*/ long* pcchDest, /*[out]*/ long* plDestLen);
    STDMETHOD(UnlockWStr)(/*[in, size_is(cchSrc)]*/ const WCHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
#endif
    STDMETHOD(SetLocale)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ LCID locale);
    STDMETHOD(GetLocale)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcMaxLen, /*[out]*/ LCID* plocale, /*[out]*/ long* plLocalePos, /*[out]*/ long* plLocaleLen);

#ifdef ASTRIMPL
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
        CLockInfo(CMLStr* pMLStr) : m_pMLStr(pMLStr)
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
        CMLStr* const m_pMLStr;
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
#endif

public:
// Called from CMLStrW and CMLStrA
#ifdef ASTRIMPL
    class CLock
    {
    public:
        CLock(BOOL fWrite, CMLStr* pMLStr, HRESULT& hr) : m_fWrite(fWrite), m_pMLStr(pMLStr) {m_fLocked = (SUCCEEDED(hr) && SUCCEEDED(hr = m_pMLStr->GetLockInfo()->StartLock(m_fWrite)));}
        ~CLock(void) {if (m_fLocked) m_pMLStr->GetLockInfo()->EndLock(m_fWrite);}
        HRESULT FallThrough(void) {m_fLocked = FALSE; return S_OK;} // Don't call EndLock in destructor
    protected:
        const BOOL m_fWrite;
        CMLStr* const m_pMLStr;
        BOOL m_fLocked;
    };
#endif

    HRESULT PrepareMLStrBuf(void);
    HRESULT SetStrBufCommon(void* pMLStrX, long lDestPos, long lDestLen, UINT uCodePage, IMLangStringBufW* pSrcBufW, IMLangStringBufA* pSrcBufA, long* pcchActual, long* plActualLen);
#ifdef ASTRIMPL
    HRESULT UnlockStrCommon(const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen);
#endif
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
#ifdef ASTRIMPL
    CLockInfo* GetLockInfo(void) {return &m_LockInfo;}
#else
    BOOL IsLocked(void) const {return (m_lLockFlags != 0);}
    BOOL IsDirectLock(void) const {return m_fDirectLock;}
    void SetDirectLockFlag(BOOL fDirectLock) {m_fDirectLock = fDirectLock;}
    long GetLockFlags(void) const {return m_lLockFlags;}
    void SetLockFlags(long lFlags) {m_lLockFlags = lFlags;}
#endif
    HRESULT MemAlloc(ULONG cb, void** ppv) {void* pv = ::CoTaskMemAlloc(cb); if (ppv) *ppv = pv; return (pv) ? S_OK : E_OUTOFMEMORY;}
    HRESULT MemFree(void* pv) {::CoTaskMemFree(pv); return S_OK;}
#ifdef ASTRIMPL
    HRESULT UnlockWStrDirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen);
    HRESULT UnlockWStrIndirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen);
    HRESULT UnlockAStrDirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen);
    HRESULT UnlockAStrIndirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen);
#endif

protected:
    ~CMLStr(void);
#ifndef ASTRIMPL
    static HRESULT ConvertMLStrBufAToWStr(UINT uCodePage, IMLangStringBufA* pMLStrBufA, long cchSrcPos, long cchSrcLen, WCHAR* pszBuf, long cchBuf, long* pcchActual);
    static HRESULT ConvertWStrToMLStrBufA(const WCHAR* pszSrc, long cchSrc, UINT uCodePage, IMLangStringBufA* pMLStrBufA, long cchDestPos, long cchDestLen);
#endif

    DWORD m_dwThreadID;

    IMLangStringBufW* m_pMLStrBufW;
    IMLangStringBufA* m_pMLStrBufA;
    UINT m_uCodePage;
    long m_lBufFlags;
    long m_cchBuf;

    LCID m_locale;

#ifdef ASTRIMPL
    CLockInfo m_LockInfo;
#else
    BOOL m_fDirectLock;
    long m_lLockFlags;

    WCHAR* m_pszLockBuf;
    long m_cchLockPos;
    long m_cchLockLen;
    long m_lLockPos;
    long m_lLockLen;
#endif
};

#endif //__MLSTR_H_

#else // NEWMLSTR

// MLStr.h : Declaration of the CMLStr

#ifndef __MLSTR_H_
#define __MLSTR_H_

#include "mlstrw.h" // IMLangStringWStrImpl
#include "mlstra.h" // IMLangStringAStrImpl
#include "util.h"

/////////////////////////////////////////////////////////////////////////////
// CMLStr
class ATL_NO_VTABLE CMLStr :
    public CComObjectRoot,
    public CComCoClass<CMLStr, &CLSID_CMLangString>,
    public IMLangString,
    public IMLStrAttrNotifySink,
    public IConnectionPointContainerImpl<CMLStr>,
    public IConnectionPointImpl<CMLStr, &IID_IMLangStringNotifySink>
{
public:
    CMLStr();

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMLStr)
        COM_INTERFACE_ENTRY(IMLangString)
        COM_INTERFACE_ENTRY_TEAR_OFF(IID_IMLangStringWStr, CMLStrW)
        COM_INTERFACE_ENTRY_TEAR_OFF(IID_IMLangStringAStr, CMLStrA)
        COM_INTERFACE_ENTRY(IMLStrAttrNotifySink)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP()

    BEGIN_CONNECTION_POINT_MAP(CMLStr)
        CONNECTION_POINT_ENTRY(IID_IMLangStringNotifySink)
    END_CONNECTION_POINT_MAP()

public:
// IMLangString
    STDMETHOD(LockMLStr)(/*[in]*/ long lPos, /*[in]*/ long lLen, /*[in]*/ DWORD dwFlags, /*[out]*/ DWORD* pdwCookie, /*[out]*/ long* plActualPos, /*[out]*/ long* plActualLen);
    STDMETHOD(UnlockMLStr)(/*[in]*/ DWORD dwCookie);
    STDMETHOD(GetLength)(/*[out, retval]*/ long* plLen);
    STDMETHOD(SetMLStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ IUnknown* pSrcMLStr, /*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen);
    STDMETHOD(RegisterAttr)(/*[in]*/ IUnknown* pUnk, /*[out]*/ DWORD* pdwCookie);
    STDMETHOD(UnregisterAttr)(/*[in]*/ DWORD dwCookie);
    STDMETHOD(EnumAttr)(/*[out]*/ IEnumUnknown** ppEnumUnk);
    STDMETHOD(FindAttr)(/*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[out]*/ IUnknown** ppUnk);
// IMLStrAttrNotifySink
    STDMETHOD(OnRequestEdit)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ long lNewLen, /*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[in]*/ IUnknown* pUnk);
    STDMETHOD(OnCanceledEdit)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ long lNewLen, /*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[in]*/ IUnknown* pUnk);
    STDMETHOD(OnChanged)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ long lNewLen, /*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[in]*/ IUnknown* pUnk);

//---------------------------------------------------------------------------
protected:
    struct LOCKINFO
    {
        long lPos;
        long lLen;
        DWORD dwFlags;
        DWORD dwThrd;
    };
//---------------------------------------------------------------------------
protected:
    class CLockList : public CMLListFast
    {
    protected:
        struct CCell : public CMLListFast::CCell
        {
            LOCKINFO m_linfo;
        };

    public:
        inline CLockList(void) : CMLListFast(sizeof(CCell), sizeof(CCell) * 8) {}
        inline HRESULT SetLock(void* pv, long lPos, long lLen, DWORD dwFlags, DWORD dwThrd)
        {
            ((CCell*)pv)->m_linfo.lPos = lPos;
            ((CCell*)pv)->m_linfo.lLen = lLen;
            ((CCell*)pv)->m_linfo.dwFlags = dwFlags;
            ((CCell*)pv)->m_linfo.dwThrd = dwThrd;
            return S_OK;
        }
        inline HRESULT GetLockInfo(void* pv, LOCKINFO** pplinfo)
        {
            *pplinfo = &((CCell*)pv)->m_linfo;
            return S_OK;
        }
    };
//---------------------------------------------------------------------------
protected:
    class CAttrList : public CMLListLru
    {
    protected:
        struct CCell : public CMLListLru::CCell
        {
            IMLStrAttr* m_pAttr;
            DWORD m_dwCookie;
        };

    public:
        inline CAttrList(void) : CMLListLru(sizeof(CCell), sizeof(CCell) * 8) {}
        inline IMLStrAttr* GetAttr(void* pv) {return ((CCell*)pv)->m_pAttr;}
        inline void SetAttr(void* pv, IMLStrAttr* pAttr) {((CCell*)pv)->m_pAttr = pAttr;}
        inline DWORD GetCookie(void* pv) const {return ((CCell*)pv)->m_dwCookie;}
        inline void SetCookie(void* pv, DWORD dwCookie) {((CCell*)pv)->m_dwCookie = dwCookie;}
    };
//---------------------------------------------------------------------------
// IEnumUnknown object for IMLangString::EnumAttr()
protected:
    class ATL_NO_VTABLE CEnumAttr :
        public CComObjectRoot,
        public IEnumUnknown
    {
    public:
        CEnumAttr(void);
        ~CEnumAttr(void);
        void Init(CMLStr* pMLStr);

        BEGIN_COM_MAP(CEnumAttr)
            COM_INTERFACE_ENTRY(IEnumUnknown)
        END_COM_MAP()

        STDMETHOD(Next)(ULONG celt, IUnknown** rgelt, ULONG* pceltFetched);
        STDMETHOD(Skip)(ULONG celt);
        STDMETHOD(Reset)(void);
        STDMETHOD(Clone)(IEnumUnknown** ppEnum);

    protected:
        CMLStr* m_pMLStr;
        void* m_pv;
    };
    friend class CEnumAttr;
//---------------------------------------------------------------------------
// Fire notification to all of IMLangStringNotifySink advised.
protected:
    class CFire : public CFireConnection<IMLangStringNotifySink, &IID_IMLangStringNotifySink>
    {
    public:
        inline CFire(HRESULT& rhr, CMLStr* const pMLStr) :
            CFireConnection<IMLangStringNotifySink, &IID_IMLangStringNotifySink>(rhr)
        {
            if (SUCCEEDED(*m_phr) &&
                FAILED(*m_phr = pMLStr->EnumConnections(&m_pEnumConn)))
            {
                m_pEnumConn = NULL;
            }
        }
    };
//---------------------------------------------------------------------------

protected:
    ~CMLStr(void);
    HRESULT CheckAccessValidation(long lPos, long lLen, DWORD dwFlags, DWORD dwThrd, long* plActualPos, long* plActualLen);

    inline HRESULT StartEndConnectionAttr(IUnknown* const pUnk, DWORD* const pdwCookie, DWORD dwCookie)
    {
        return ::StartEndConnection(pUnk, &IID_IMLStrAttrNotifySink, (IMLStrAttrNotifySink*)this, pdwCookie, dwCookie);
    }

protected:
    long m_lLen;
    CLockList m_lock;
    CAttrList m_attr;
    HANDLE m_hUnlockEvent;
    int m_cWaitUnlock;
    HANDLE m_hZeroEvent;
};

#endif //__MLSTR_H_

#endif // NEWMLSTR

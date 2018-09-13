#ifndef NEWMLSTR

// MLStrW.h : Declaration of the CMLStrW

#ifndef __MLSTRW_H_
#define __MLSTRW_H_

#include "mlatl.h"
#include "mlstrbuf.h"

class CMLStr;

/////////////////////////////////////////////////////////////////////////////
// CMLStrW
class ATL_NO_VTABLE CMLStrW :
    public CComTearOffObjectBase<CMLStr>,
    public IMLangStringWStr
{
    typedef CComObject<CMLStr>* POWNER;

public:
    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMLStrW)
        COM_INTERFACE_ENTRY(IMLangString)
        COM_INTERFACE_ENTRY(IMLangStringWStr)
    END_COM_MAP()

public:
// IMLangString
    STDMETHOD(Sync)(/*[in]*/ BOOL fNoAccess);
    STDMETHOD(GetLength)(/*[out, retval]*/ long* plLen);
    STDMETHOD(SetMLStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ IUnknown* pSrcMLStr, /*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen);
    STDMETHOD(GetMLStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ IUnknown* pUnkOuter, /*[in]*/ DWORD dwClsContext, /*[in]*/ const IID* piid, /*[out]*/ IUnknown** ppDestMLStr, /*[out]*/ long* plDestPos, /*[out]*/ long* plDestLen);
// IMLangStringWStr
    STDMETHOD(SetWStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in, size_is(cchSrc)]*/ const WCHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(SetStrBufW)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ IMLangStringBufW* pSrcBuf, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetWStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[out, size_is(cchDest)]*/ WCHAR* pszDest, /*[in]*/ long cchDest, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetStrBufW)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcMaxLen, /*[out]*/ IMLangStringBufW** ppDestBuf, /*[out]*/ long* plDestLen);
    STDMETHOD(LockWStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ long lFlags, /*[in]*/ long cchRequest, /*[out, size_is(,*pcchDest)]*/ WCHAR** ppszDest, /*[out]*/ long* pcchDest, /*[out]*/ long* plDestLen);
    STDMETHOD(UnlockWStr)(/*[in, size_is(cchSrc)]*/ const WCHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(SetLocale)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ LCID locale);
    STDMETHOD(GetLocale)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcMaxLen, /*[out]*/ LCID* plocale, /*[out]*/ long* plLocalePos, /*[out]*/ long* plLocaleLen);

protected:
    POWNER GetOwner(void) const {return m_pOwner;}
};

#endif //__MLSTRW_H_

#else // NEWMLSTR

// MLStrW.h : Declaration of the CMLStrW

#ifndef __MLSTRW_H_
#define __MLSTRW_H_

#include "mlatl.h"

class CMLStr;

/////////////////////////////////////////////////////////////////////////////
// CMLStrW
class ATL_NO_VTABLE CMLStrW :
    public CComTearOffObjectBase<CMLStr>,
    public IMLangStringWStr
{
    typedef CComObject<CMLStr>* POWNER;

public:
    CMLStrW(void);

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMLStrW)
        COM_INTERFACE_ENTRY(IMLangString)
        COM_INTERFACE_ENTRY(IMLangStringWStr)
    END_COM_MAP()

public:
// IMLangString
    STDMETHOD(LockMLStr)(/*[in]*/ long lPos, /*[in]*/ long lLen, /*[in]*/ DWORD dwFlags, /*[out]*/ DWORD* pdwCookie, /*[out]*/ long* plActualPos, /*[out]*/ long* plActualLen);
    STDMETHOD(UnlockMLStr)(/*[in]*/ DWORD dwCookie);
    STDMETHOD(GetLength)(/*[out, retval]*/ long* plLen);
    STDMETHOD(SetMLStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ IUnknown* pSrcMLStr, /*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen);
    STDMETHOD(RegisterAttr)(/*[in]*/ IUnknown* pUnk, /*[out]*/  DWORD* pdwCookie);
    STDMETHOD(UnregisterAttr)(/*[in]*/ DWORD dwCookie);
    STDMETHOD(EnumAttr)(/*[out]*/ IEnumUnknown** ppEnumUnk);
    STDMETHOD(FindAttr)(/*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[out]*/ IUnknown** ppUnk);
// IMLangStringWStr
    STDMETHOD(SetWStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in, size_is(cchSrc)]*/ const WCHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(SetStrBufW)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ IMLangStringBufW* pSrcBuf, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetWStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[out, size_is(cchDest)]*/ WCHAR* pszDest, /*[in]*/ long cchDest, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetStrBufW)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcMaxLen, /*[out]*/ IMLangStringBufW** ppDestBuf, /*[out]*/ long* plDestLen);
    STDMETHOD(LockWStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ long lFlags, /*[in]*/ long cchRequest, /*[out, size_is(,*pcchDest)]*/ WCHAR** ppszDest, /*[out]*/ long* pcchDest, /*[out]*/ long* plDestLen);
    STDMETHOD(UnlockWStr)(/*[in, size_is(cchSrc)]*/ const WCHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(SetLocale)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ LCID locale);
    STDMETHOD(GetLocale)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcMaxLen, /*[out]*/ LCID* plocale, /*[out]*/ long* plLocalePos, /*[out]*/ long* plLocaleLen);

protected:
    ~CMLStrW(void);
    POWNER GetOwner(void) const {return m_pOwner;}
    inline HRESULT GetAttrWStr(IMLStrAttrWStr** ppAttr);
    inline HRESULT GetAttrLocale(IMLStrAttrLocale** ppAttr);
    HRESULT GetAttrWStrReal(IMLStrAttrWStr** ppAttr);
    HRESULT GetAttrLocaleReal(IMLStrAttrLocale** ppAttr);

    CRITICAL_SECTION m_cs;
    IMLStrAttrWStr* m_pAttrWStr;
    IMLStrAttrLocale* m_pAttrLocale;
    DWORD m_dwAttrWStrCookie; // Returned by RegisterAttr
    DWORD m_dwAttrLocaleCookie; // Returned by RegisterAttr
};

/////////////////////////////////////////////////////////////////////////////
// CMLStrW inline-line functions
HRESULT CMLStrW::GetAttrWStr(IMLStrAttrWStr** ppAttr)
{
    if (m_pAttrWStr)
    {
        if (ppAttr)
            *ppAttr = m_pAttrWStr;

        return S_OK;
    }
    else
    {
        return GetAttrWStrReal(ppAttr);
    }
}

HRESULT CMLStrW::GetAttrLocale(IMLStrAttrLocale** ppAttr)
{
    if (m_pAttrLocale)
    {
        if (ppAttr)
            *ppAttr = m_pAttrLocale;

        return S_OK;
    }
    else
    {
        return GetAttrLocaleReal(ppAttr);
    }
}

#endif //__MLSTRW_H_

#endif // NEWMLSTR

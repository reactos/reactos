#ifndef NEWMLSTR

// MLStrA.h : Declaration of the CMLStrA

#ifndef __MLSTRA_H_
#define __MLSTRA_H_

#include "mlatl.h"

class CMLStr;

/////////////////////////////////////////////////////////////////////////////
// CMLStrA
class ATL_NO_VTABLE CMLStrA :
    public CComTearOffObjectBase<CMLStr>,
    public IMLangStringAStr
{
    typedef CComObject<CMLStr>* POWNER;

public:
#ifdef ASTRIMPL
    CMLStrA(void);
#endif

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMLStrA)
        COM_INTERFACE_ENTRY(IMLangString)
        COM_INTERFACE_ENTRY(IMLangStringAStr)
    END_COM_MAP()

public:
// IMLangString
    STDMETHOD(Sync)(/*[in]*/ BOOL fNoAccess);
    STDMETHOD(GetLength)(/*[out, retval]*/ long* plLen);
    STDMETHOD(SetMLStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ IUnknown* pSrcMLStr, /*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen);
    STDMETHOD(GetMLStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ IUnknown* pUnkOuter, /*[in]*/ DWORD dwClsContext, /*[in]*/ const IID* piid, /*[out]*/ IUnknown** ppDestMLStr, /*[out]*/ long* plDestPos, /*[out]*/ long* plDestLen);
// IMLangStringAStr
    STDMETHOD(SetAStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ UINT uCodePage, /*[in, size_is(cchSrc)]*/ const CHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(SetStrBufA)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ UINT uCodePage, /*[in]*/ IMLangStringBufA* pSrcBuf, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetAStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ UINT uCodePageIn, /*[out]*/ UINT* puCodePageOut, /*[out, size_is(cchDest)]*/ CHAR* pszDest, /*[in]*/ long cchDest, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetStrBufA)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcMaxLen, /*[out]*/ UINT* puDestCodePage, /*[out]*/ IMLangStringBufA** ppDestBuf, /*[out]*/ long* plDestLen);
    STDMETHOD(LockAStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ long lFlags, /*[in]*/ UINT uCodePageIn, /*[in]*/ long cchRequest, /*[out]*/ UINT* puCodePageOut, /*[out, size_is(,*pcchDest)]*/ CHAR** ppszDest, /*[out]*/ long* pcchDest, /*[out]*/ long* plDestLen);
    STDMETHOD(UnlockAStr)(/*[in, size_is(cchSrc)]*/ const CHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(SetLocale)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ LCID locale);
    STDMETHOD(GetLocale)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcMaxLen, /*[out]*/ LCID* plocale, /*[out]*/ long* plLocalePos, /*[out]*/ long* plLocaleLen);

protected:
#ifdef ASTRIMPL
    ~CMLStrA(void);
#endif
    POWNER GetOwner(void) const {return m_pOwner;}
#ifdef ASTRIMPL
    HRESULT PrepareMLangCodePages(void)
    {
        HRESULT hr = S_OK;
        if (!m_pMLCPs)
            hr = ::CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMLangCodePages, (void**)&m_pMLCPs);
        return hr;
    }
    IMLangCodePages* GetMLangCodePages(void) const {return m_pMLCPs;}

    IMLangCodePages* m_pMLCPs;
#endif
};

#endif //__MLSTRA_H_

#else // NEWMLSTR

// MLStrA.h : Declaration of the CMLStrA

#ifndef __MLSTRA_H_
#define __MLSTRA_H_

#include "mlatl.h"

class CMLStr;

/////////////////////////////////////////////////////////////////////////////
// CMLStrA
class ATL_NO_VTABLE CMLStrA :
    public CComTearOffObjectBase<CMLStr>,
    public IMLangStringAStr
{
    typedef CComObject<CMLStr>* POWNER;

public:
    CMLStrA(void);

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMLStrA)
        COM_INTERFACE_ENTRY(IMLangString)
        COM_INTERFACE_ENTRY(IMLangStringAStr)
    END_COM_MAP()

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
// IMLangStringAStr
    STDMETHOD(SetAStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ UINT uCodePage, /*[in, size_is(cchSrc)]*/ const CHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(SetStrBufA)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ UINT uCodePage, /*[in]*/ IMLangStringBufA* pSrcBuf, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetAStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ UINT uCodePageIn, /*[out]*/ UINT* puCodePageOut, /*[out, size_is(cchDest)]*/ CHAR* pszDest, /*[in]*/ long cchDest, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetStrBufA)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcMaxLen, /*[out]*/ UINT* puDestCodePage, /*[out]*/ IMLangStringBufA** ppDestBuf, /*[out]*/ long* plDestLen);
    STDMETHOD(LockAStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ long lFlags, /*[in]*/ UINT uCodePageIn, /*[in]*/ long cchRequest, /*[out]*/ UINT* puCodePageOut, /*[out, size_is(,*pcchDest)]*/ CHAR** ppszDest, /*[out]*/ long* pcchDest, /*[out]*/ long* plDestLen);
    STDMETHOD(UnlockAStr)(/*[in, size_is(cchSrc)]*/ const CHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(SetLocale)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ LCID locale);
    STDMETHOD(GetLocale)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcMaxLen, /*[out]*/ LCID* plocale, /*[out]*/ long* plLocalePos, /*[out]*/ long* plLocaleLen);

protected:
    ~CMLStrA(void);
    POWNER GetOwner(void) const {return m_pOwner;}
    inline HRESULT GetAttrAStr(IMLStrAttrAStr** ppAttr);
    inline HRESULT GetAttrLocale(IMLStrAttrLocale** ppAttr);
    HRESULT GetAttrAStrReal(IMLStrAttrAStr** ppAttr);
    HRESULT GetAttrLocaleReal(IMLStrAttrLocale** ppAttr);

    CRITICAL_SECTION m_cs;
    IMLStrAttrAStr* m_pAttrAStr;
    IMLStrAttrLocale* m_pAttrLocale;
    DWORD m_dwAttrAStrCookie; // Returned by RegisterAttr
    DWORD m_dwAttrLocaleCookie; // Returned by RegisterAttr
};

/////////////////////////////////////////////////////////////////////////////
// CMLStrA inline-line functions
HRESULT CMLStrA::GetAttrAStr(IMLStrAttrAStr** ppAttr)
{
    if (m_pAttrAStr)
    {
        if (ppAttr)
            *ppAttr = m_pAttrAStr;

        return S_OK;
    }
    else
    {
        return GetAttrAStrReal(ppAttr);
    }
}

HRESULT CMLStrA::GetAttrLocale(IMLStrAttrLocale** ppAttr)
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

#endif //__MLSTRA_H_

#endif // NEWMLSTR

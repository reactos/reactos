// AttrStrW.h : Declaration of the CMLStrAttrWStr

#ifndef __ATTRSTRW_H_
#define __ATTRSTRW_H_

#include "mlatl.h"
#include "attrstr.h"
#include "mlstrbuf.h"

/////////////////////////////////////////////////////////////////////////////
// CMLStrAttrWStr
class ATL_NO_VTABLE CMLStrAttrWStr :
    public CMLStrAttrStrCommon,
    public CComObjectRoot,
    public CComCoClass<CMLStrAttrWStr, &CLSID_CMLStrAttrWStr>,
    public IMLStrAttrWStr,
    public IMLangStringNotifySink,
    public IConnectionPointContainerImpl<CMLStrAttrWStr>,
    public IConnectionPointImpl<CMLStrAttrWStr, &IID_IMLStrAttrNotifySink>
{
    typedef CMLStrAttrWStr* POWNER;

public:
    CMLStrAttrWStr(void);

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMLStrAttrWStr)
        COM_INTERFACE_ENTRY(IMLStrAttr)
        COM_INTERFACE_ENTRY(IMLStrAttrWStr)
        COM_INTERFACE_ENTRY(IMLangStringNotifySink)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP()

    BEGIN_CONNECTION_POINT_MAP(CMLStrAttrWStr)
        CONNECTION_POINT_ENTRY(IID_IMLStrAttrNotifySink)
    END_CONNECTION_POINT_MAP()

public:
// IMLStrAttr
    STDMETHOD(SetClient)(/*[in]*/ IUnknown* pUnk);
    STDMETHOD(GetClient)(/*[out]*/ IUnknown** ppUnk);
    STDMETHOD(QueryAttr)(/*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[out]*/ IUnknown** ppUnk, /*[out]*/ long* lConf);
    STDMETHOD(GetAttrInterface)(/*[out]*/ IID* pIID, /*[out]*/ LPARAM* plParam);
    STDMETHOD(SetMLStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ IUnknown* pSrcMLStr, /*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen);
// IMLStrAttrWStr
    STDMETHOD(SetWStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in, size_is(cchSrc)]*/ const WCHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(SetStrBufW)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ IMLangStringBufW* pSrcBuf, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetWStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[out, size_is(cchDest)]*/ WCHAR* pszDest, /*[in]*/ long cchDest, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetStrBufW)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcMaxLen, /*[out]*/ IMLangStringBufW** ppDestBuf, /*[out]*/ long* plDestLen);
    STDMETHOD(LockWStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ long lFlags, /*[in]*/ long cchRequest, /*[out, size_is(,*pcchDest)]*/ WCHAR** ppszDest, /*[out]*/ long* pcchDest, /*[out]*/ long* plDestLen);
    STDMETHOD(UnlockWStr)(/*[in, size_is(cchSrc)]*/ const WCHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
// IMLangStringNotifySink
    STDMETHOD(OnRegisterAttr)(/*[in]*/ IUnknown* pUnk);
    STDMETHOD(OnUnregisterAttr)(/*[in]*/ IUnknown* pUnk);
    STDMETHOD(OnRequestEdit)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ long lNewLen, /*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[in]*/ IUnknown* pUnk);
    STDMETHOD(OnCanceledEdit)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ long lNewLen, /*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[in]*/ IUnknown* pUnk);
    STDMETHOD(OnChanged)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ long lNewLen, /*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[in]*/ IUnknown* pUnk);

protected:
    ~CMLStrAttrWStr(void);
    IMLStrAttr* GetMLStrAttr(void) {return this;}
    HRESULT StartEndConnectionMLStr(IUnknown* const pUnk, BOOL fStart);

    IMLangString* m_pMLStr;
    DWORD m_dwMLStrCookie;
};

#endif //__ATTRSTRW_H_

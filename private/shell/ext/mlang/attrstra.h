// AttrStrA.h : Declaration of the CMLStrAttrAStr

#ifndef __ATTRSTRA_H_
#define __ATTRSTRA_H_

#include "mlatl.h"
#include "attrstr.h"

/////////////////////////////////////////////////////////////////////////////
// CMLStrAttrAStr
class ATL_NO_VTABLE CMLStrAttrAStr :
    public CMLStrAttrStrCommon,
    public CComObjectRoot,
    public CComCoClass<CMLStrAttrAStr, &CLSID_CMLStrAttrAStr>,
    public IMLStrAttrAStr,
    public IMLangStringNotifySink,
    public IConnectionPointContainerImpl<CMLStrAttrAStr>,
    public IConnectionPointImpl<CMLStrAttrAStr, &IID_IMLStrAttrNotifySink>
{
    typedef CMLStrAttrAStr* POWNER;

public:
    CMLStrAttrAStr(void);

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMLStrAttrAStr)
        COM_INTERFACE_ENTRY(IMLStrAttr)
        COM_INTERFACE_ENTRY(IMLStrAttrAStr)
        COM_INTERFACE_ENTRY(IMLangStringNotifySink)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP()

    BEGIN_CONNECTION_POINT_MAP(CMLStrAttrAStr)
        CONNECTION_POINT_ENTRY(IID_IMLStrAttrNotifySink)
    END_CONNECTION_POINT_MAP()

public:
// IMLStrAttr
    STDMETHOD(SetClient)(/*[in]*/ IUnknown* pUnk);
    STDMETHOD(GetClient)(/*[out]*/ IUnknown** ppUnk);
    STDMETHOD(QueryAttr)(/*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[out]*/ IUnknown** ppUnk, /*[out]*/ long* lConf);
    STDMETHOD(GetAttrInterface)(/*[out]*/ IID* pIID, /*[out]*/ LPARAM* plParam);
    STDMETHOD(SetMLStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ IUnknown* pSrcMLStr, /*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen);
// IMLStrAttrAStr
    STDMETHOD(SetAStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ UINT uCodePage, /*[in, size_is(cchSrc)]*/ const CHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(SetStrBufA)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ UINT uCodePage, /*[in]*/ IMLangStringBufA* pSrcBuf, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetAStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ UINT uCodePageIn, /*[out]*/ UINT* puCodePageOut, /*[out, size_is(cchDest)]*/ CHAR* pszDest, /*[in]*/ long cchDest, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
    STDMETHOD(GetStrBufA)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcMaxLen, /*[out]*/ UINT* puDestCodePage, /*[out]*/ IMLangStringBufA** ppDestBuf, /*[out]*/ long* plDestLen);
    STDMETHOD(LockAStr)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ long lFlags, /*[in]*/ UINT uCodePageIn, /*[in]*/ long cchRequest, /*[out]*/ UINT* puCodePageOut, /*[out, size_is(,*pcchDest)]*/ CHAR** ppszDest, /*[out]*/ long* pcchDest, /*[out]*/ long* plDestLen);
    STDMETHOD(UnlockAStr)(/*[in, size_is(cchSrc)]*/ const CHAR* pszSrc, /*[in]*/ long cchSrc, /*[out]*/ long* pcchActual, /*[out]*/ long* plActualLen);
// IMLangStringNotifySink
    STDMETHOD(OnRegisterAttr)(/*[in]*/ IUnknown* pUnk);
    STDMETHOD(OnUnregisterAttr)(/*[in]*/ IUnknown* pUnk);
    STDMETHOD(OnRequestEdit)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ long lNewLen, /*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[in]*/ IUnknown* pUnk);
    STDMETHOD(OnCanceledEdit)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ long lNewLen, /*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[in]*/ IUnknown* pUnk);
    STDMETHOD(OnChanged)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ long lNewLen, /*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[in]*/ IUnknown* pUnk);

protected:
    ~CMLStrAttrAStr(void);
    IMLStrAttr* GetMLStrAttr(void) {return this;}
    HRESULT PrepareMLangCodePages(void)
    {
        HRESULT hr = S_OK;
        if (!m_pMLCPs)
            hr = ::CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMLangCodePages, (void**)&m_pMLCPs);
        return hr;
    }
    IMLangCodePages* GetMLangCodePages(void) const {return m_pMLCPs;}
    HRESULT StartEndConnectionMLStr(IUnknown* const pUnk, BOOL fStart);

    IMLangCodePages* m_pMLCPs;
    IMLangString* m_pMLStr;
    DWORD m_dwMLStrCookie;
};

#endif //__ATTRSTRA_H_

// AttrLoc.h : Declaration of the CMLStrAttrLocale

#ifndef __ATTRLOC_H_
#define __ATTRLOC_H_

#include "mlatl.h"

/////////////////////////////////////////////////////////////////////////////
// CMLStrAttrLocale
class ATL_NO_VTABLE CMLStrAttrLocale :
    public CComObjectRoot,
    public CComCoClass<CMLStrAttrLocale, &CLSID_CMLStrAttrLocale>,
    public IConnectionPointContainerImpl<CMLStrAttrLocale>,
    public IConnectionPointImpl<CMLStrAttrLocale, &IID_IMLStrAttrNotifySink>,
    public IMLStrAttrLocale
{
public:
    CMLStrAttrLocale();

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMLStrAttrLocale)
        COM_INTERFACE_ENTRY(IMLStrAttr)
        COM_INTERFACE_ENTRY(IMLStrAttrLong)
        COM_INTERFACE_ENTRY(IMLStrAttrLocale)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP()

    BEGIN_CONNECTION_POINT_MAP(CMLStrAttrLocale)
        CONNECTION_POINT_ENTRY(IID_IMLStrAttrNotifySink)
    END_CONNECTION_POINT_MAP()

public:
// IMLStrAttr
    STDMETHOD(SetClient)(/*[in]*/ IUnknown* pUnk);
    STDMETHOD(GetClient)(/*[out]*/ IUnknown** ppUnk);
    STDMETHOD(QueryAttr)(/*[in]*/ REFIID riid, /*[in]*/ LPARAM lParam, /*[out]*/ IUnknown** ppUnk, /*[out]*/ long* lConf);
    STDMETHOD(GetAttrInterface)(/*[out]*/ IID* pIID, /*[out]*/ LPARAM* plParam);
    STDMETHOD(SetMLStr)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ IUnknown* pSrcMLStr, /*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen);
// IMLStrAttrLong
    STDMETHOD(SetLong)(/*[in]*/ long lDestPos, /*[in]*/ long lDestLen, /*[in]*/ long lValue);
    STDMETHOD(GetLong)(/*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[out]*/ long* plValue, /*[out]*/ long* plActualPos, /*[out]*/ long* plActualLen);
// IMLStrAttrLocale
    // Nothing

protected:
    long m_lLen;
    LCID m_lcid;
};

#endif //__ATTRLOC_H_

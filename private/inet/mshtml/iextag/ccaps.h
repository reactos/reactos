// CCaps.h : Declaration of the CClientCaps

#ifndef __CCAPS_H_
#define __CCAPS_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CClientCaps
class ATL_NO_VTABLE CClientCaps : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CClientCaps, &CLSID_ClientCaps>,
    public IDispatchImpl<IClientCaps, &IID_IClientCaps, &LIBID_IEXTagLib>,

    public IElementBehavior
{
public:
    CClientCaps()
    {
        m_pSite = NULL;
        ppwszComponents = NULL;
    }
    ~CClientCaps()
    {
        if (m_pSite)
            m_pSite->Release();
        if(ppwszComponents)
        {
            clearComponentRequest();
            delete [] ppwszComponents;
        }
    }

DECLARE_REGISTRY_RESOURCEID(IDR_CLIENTCAPS)
DECLARE_NOT_AGGREGATABLE(CClientCaps)

BEGIN_COM_MAP(CClientCaps)
    COM_INTERFACE_ENTRY(IClientCaps)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IElementBehavior)
END_COM_MAP()

// IClientCaps
public:
    STDMETHOD(get_javaEnabled)(/*[out, retval]*/ VARIANT_BOOL *pVal);
    STDMETHOD(get_cookieEnabled)(/*[out, retval]*/ VARIANT_BOOL *pVal);
    STDMETHOD(get_cpuClass)(/*[out, retval]*/ BSTR * p);      
    STDMETHOD(get_systemLanguage)(/*[out, retval]*/ BSTR * p);
    STDMETHOD(get_userLanguage)(/*[out, retval]*/ BSTR * p);  
    STDMETHOD(get_platform)(/*[out, retval]*/ BSTR * p);      
    STDMETHOD(get_connectionSpeed)(/*[out, retval]*/ long * p);
    STDMETHOD(get_onLine)(/*[out, retval]*/ VARIANT_BOOL * p);
    STDMETHOD(get_colorDepth)(/*[out, retval]*/ long * p);    
    STDMETHOD(get_bufferDepth)(/*[out, retval]*/ long * p);   
    STDMETHOD(get_width)(/*[out, retval]*/ long * p);         
    STDMETHOD(get_height)(/*[out, retval]*/ long * p);        
    STDMETHOD(get_availHeight)(/*[out, retval]*/ long * p);   
    STDMETHOD(get_availWidth)(/*[out, retval]*/ long * p); 
    STDMETHOD(get_connectionType)(/*[out, retval]*/ BSTR * p); 
    STDMETHOD(getComponentVersion)(/*[in]*/ BSTR bstrName, /*[in]*/ BSTR bstrType, /*[out,retval]*/ BSTR *pbstrVer);
    STDMETHOD(isComponentInstalled)(/*[in]*/ BSTR bstrName, /*[in]*/ BSTR bstrType, /*[in,optional]*/ BSTR bStrVer, /*[out,retval]*/ VARIANT_BOOL *p);
    STDMETHOD(compareVersions)(/*[in]*/ BSTR bstrVer1, /*[in]*/ BSTR bstrVer2, /*[out,retval]*/long *p); 
    STDMETHOD(addComponentRequest)(/*[in]*/ BSTR bstrName, /*[in]*/ BSTR bstrType, /*[in, optional]*/ BSTR bstrVer);
    STDMETHOD(doComponentRequest)(/*[out]*/ VARIANT_BOOL * pVal);
    STDMETHOD(clearComponentRequest)();

    //IHTMLPeerElement methods
    STDMETHOD(Init)(IElementBehaviorSite *pSite);
    STDMETHOD(Notify)(LONG lNotify, VARIANT * pVarNotify);
    STDMETHOD(Detach)() { return S_OK; };

private:
    STDMETHOD(GetHTMLWindow)(/* out */ IHTMLWindow2 **ppWindow);
    STDMETHOD(GetHTMLDocument)(/* out */IHTMLDocument2 **ppDoc);
    STDMETHOD(GetClientInformation)(/* out */IOmNavigator **ppClientInformation);
    STDMETHOD(GetScreen)(/* out */ IHTMLScreen **ppScreen);
    STDMETHOD(GetVersion)(BSTR bstrName, BSTR bstrType, LPDWORD pdwMS, LPDWORD pdwLS);

private: // helpers functions to convert between version strings and DWORD's
    static HRESULT GetVersionFromString(LPCOLESTR psz, LPDWORD pdwMS, LPDWORD pdwLS);
    static HRESULT GetStringFromVersion(DWORD dwMS, DWORD dwLS, BSTR *pbstrVersion);

private:
    IElementBehaviorSite * m_pSite;
    int iComponentNum;
    int iComponentCap;
    LPWSTR * ppwszComponents;

};

#endif //__CCAPS_H_

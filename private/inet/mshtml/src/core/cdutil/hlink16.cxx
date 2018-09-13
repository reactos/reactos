//+------------------------------------------------------------------------
//
//  File:       chlink.cxx
//  Created :   27th June 1997
//  Contents:   Implementing IHlink.
//              Done for storing and retrieving the 
//              bookmark fragment of an URL
//              Constructor - initializes the bookmark "_szLocation"
//              GetMonikerReference - Currently used for retrieving the bookmark
//              HlinkCreateFromMoniker - Instantiates the CHlink class   
//-------------------------------------------------------------------------


#include "headers.hxx"

#ifndef X_HLINK_H_
#define X_HLINK_X_
#include "hlink.h"        // for std hyperlink object
#endif

class CHlink : public IHlink
{
public:
    CHlink();
    CHlink(IMoniker* pmk, LPCTSTR pszLocation, LPCSTR pszFriedlyName);
    ~CHlink();

    // *** IUnknown methods ***
    virtual HRESULT STDMETHODCALLTYPE  QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) ;
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // *** IOleWindow methods ***
        virtual HRESULT STDMETHODCALLTYPE SetHlinkSite( 
            /* [unique][in] */ IHlinkSite __RPC_FAR *pihlSite,
            /* [in] */ DWORD dwSiteData) ;
        
        virtual HRESULT STDMETHODCALLTYPE GetHlinkSite( 
            /* [out] */ IHlinkSite __RPC_FAR *__RPC_FAR *ppihlSite,
            /* [out] */ DWORD __RPC_FAR *pdwSiteData) ;

        virtual HRESULT STDMETHODCALLTYPE GetMonikerReference( 
            /* [in] */ DWORD dwWhichRef,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppimkTarget,
            /* [out] */ LPWSTR __RPC_FAR *ppwzLocation);

        virtual HRESULT STDMETHODCALLTYPE GetStringReference( 
            /* [in] */ DWORD dwWhichRef,
            /* [out] */ LPWSTR __RPC_FAR *ppwzTarget,
            /* [out] */ LPWSTR __RPC_FAR *ppwzLocation);

        virtual HRESULT STDMETHODCALLTYPE GetFriendlyName( 
            /* [in] */ DWORD grfHLFNAMEF,
            /* [out] */ LPWSTR __RPC_FAR *ppwzFriendlyName);

            virtual HRESULT STDMETHODCALLTYPE Navigate( 
            /* [in] */ DWORD grfHLNF,
            /* [unique][in] */ LPBC pibc,
            /* [unique][in] */ IBindStatusCallback __RPC_FAR *pibsc,
            /* [unique][in] */ IHlinkBrowseContext __RPC_FAR *pihlbc);

  
    virtual HRESULT STDMETHODCALLTYPE SetMonikerReference( 
            /* [in] */ DWORD grfHLSETF,
            /* [unique][in] */ IMoniker __RPC_FAR *pimkTarget,
            /* [unique][in] */ LPCWSTR pwzLocation) ;
        
    virtual HRESULT STDMETHODCALLTYPE SetStringReference( 
            /* [in] */ DWORD grfHLSETF,
            /* [unique][in] */ LPCWSTR pwzTarget,
            /* [unique][in] */ LPCWSTR pwzLocation) ;
                
    virtual HRESULT STDMETHODCALLTYPE SetFriendlyName( 
            /* [unique][in] */ LPCWSTR pwzFriendlyName) ;
        
        
    virtual HRESULT STDMETHODCALLTYPE SetTargetFrameName( 
            /* [unique][in] */ LPCWSTR pwzTargetFrameName) ;
        
    virtual HRESULT STDMETHODCALLTYPE GetTargetFrameName( 
            /* [out] */ LPWSTR __RPC_FAR *ppwzTargetFrameName) ;
        
    virtual HRESULT STDMETHODCALLTYPE GetMiscStatus( 
            /* [out] */ DWORD __RPC_FAR *pdwStatus);
        
    virtual HRESULT STDMETHODCALLTYPE SetAdditionalParams( 
            /* [unique][in] */ LPCWSTR pwzAdditionalParams) ;
        
    virtual HRESULT STDMETHODCALLTYPE GetAdditionalParams( 
            /* [out] */ LPWSTR __RPC_FAR *ppwzAdditionalParams);


protected:
    UINT	_cRef;
    IHlinkSite* _pihlSite;
    IMoniker*   _pmk;
    TCHAR	_szLocation[MAX_PATH];
};

CHlink::CHlink(IMoniker* pmk, LPCTSTR pszLocation, LPCSTR pszFriedlyName)
		    : _cRef(1), _pihlSite(NULL), _pmk(pmk)
{
    if (_pmk) {
	_pmk->AddRef();
    }
    if (pszLocation) {
	lstrcpy(_szLocation, pszLocation);
    } else {
	_szLocation[0] = '\0';
    }
}


CHlink::~CHlink()
{
    if (_pmk) {
	_pmk->Release();
    }

    if (_pihlSite) {
	_pihlSite->Release();
    }
}

HRESULT CHlink::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
     if( IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IHlink))
    {
        *ppvObj = (IHlink *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
    
}

ULONG CHlink::AddRef(void)
{
    return ++_cRef;
}

ULONG CHlink::Release(void)
{
    if (--_cRef > 0) {
	return _cRef;
    }

    delete this;
    return 0;
}

HRESULT CHlink::SetHlinkSite( 
            /* [unique][in] */ IHlinkSite __RPC_FAR *pihlSite,
            /* [in] */ DWORD dwSiteData) 
{
    if (_pihlSite) {
	_pihlSite->Release();
    }

    _pihlSite = pihlSite;

    if (_pihlSite) {
	_pihlSite->AddRef();
    }

    return S_OK;
}

HRESULT CHlink::GetHlinkSite( 
            /* [out] */ IHlinkSite __RPC_FAR *__RPC_FAR *ppihlSite,
            /* [out] */ DWORD __RPC_FAR *pdwSiteData) 
{
    *ppihlSite = _pihlSite;
    if (_pihlSite) {
	_pihlSite->AddRef();
    }

    return S_OK;
}

HRESULT CHlink::GetMonikerReference( 
            /* [in] */ DWORD dwWhichRef,
            /* [out] */ IMoniker **ppimkTarget,
            /* [out] */ LPWSTR __RPC_FAR *ppwzLocation)
{
    if (ppimkTarget) {
	*ppimkTarget = _pmk;
	if (_pmk) {
	    _pmk->AddRef();
	}
    }

    if (ppwzLocation) 
    {
        *ppwzLocation = SysAllocString ( _szLocation ) ;
    }
    
    return S_OK;
}

HRESULT CHlink::GetStringReference( 
            /* [in] */ DWORD dwWhichRef,
            /* [out] */ LPWSTR __RPC_FAR *ppwzTarget,
            /* [out] */ LPWSTR __RPC_FAR *ppwzLocation)
{
    return E_NOTIMPL;
}

HRESULT CHlink::GetFriendlyName( 
            /* [in] */ DWORD grfHLFNAMEF,
            /* [out] */ LPWSTR __RPC_FAR *ppwzFriendlyName){
    return E_NOTIMPL;
}

HRESULT CHlink::Navigate( 
            /* [in] */ DWORD grfHLNF,
            /* [unique][in] */ LPBC pibc,
            /* [unique][in] */ IBindStatusCallback __RPC_FAR *pibsc,
            /* [unique][in] */ IHlinkBrowseContext __RPC_FAR *pihlbc)
{
       return E_NOTIMPL ;
}


HRESULT CHlink::SetMonikerReference( 
            /* [in] */ DWORD grfHLSETF,
            /* [unique][in] */ IMoniker __RPC_FAR *pimkTarget,
            /* [unique][in] */ LPCWSTR pwzLocation)
{
 return E_NOTIMPL ;   
}
        
HRESULT CHlink::SetStringReference( 
            /* [in] */ DWORD grfHLSETF,
            /* [unique][in] */ LPCWSTR pwzTarget,
            /* [unique][in] */ LPCWSTR pwzLocation)
{
    return E_NOTIMPL ;
}
                
HRESULT CHlink::SetFriendlyName( 
            /* [unique][in] */ LPCWSTR pwzFriendlyName)
{
    return E_NOTIMPL ;
}
        
HRESULT CHlink::SetTargetFrameName( 
            /* [unique][in] */ LPCWSTR pwzTargetFrameName)
{
    return E_NOTIMPL ;
}
        
HRESULT CHlink::GetTargetFrameName( 
            /* [out] */ LPWSTR __RPC_FAR *ppwzTargetFrameName)
{
    return E_NOTIMPL ;
}
        
HRESULT CHlink::GetMiscStatus( 
            /* [out] */ DWORD __RPC_FAR *pdwStatus)
{
    return E_NOTIMPL ;
}
        
HRESULT CHlink::SetAdditionalParams( 
            /* [unique][in] */ LPCWSTR pwzAdditionalParams)
{
    return E_NOTIMPL ;
}
        
HRESULT CHlink::GetAdditionalParams( 
            /* [out] */ LPWSTR __RPC_FAR *ppwzAdditionalParams)
{
    return E_NOTIMPL ;
}

//#ifdef WIN16
STDAPI HlinkCreateFromMoniker(
             IMoniker * pimkTrgt,
             LPCWSTR pwzLocation,
             LPCWSTR pwzFriendlyName,
             IHlinkSite * pihlsite,
             DWORD dwSiteData,
             IUnknown * piunkOuter,
             REFIID riid,
             void ** ppvObj)
               
{
    if ( pimkTrgt == NULL)
        return E_FAIL;
                          
		CHlink *ptemp = NULL;
        
        ptemp = new CHlink ( pimkTrgt, pwzLocation, pwzFriendlyName) ; 
        if ( !ptemp )
        	return E_OUTOFMEMORY ;

        *ppvObj = NULL ;                          

        *ppvObj = (void * )ptemp ;
        

    return S_OK ;

}
//#endif
#ifndef _URLCALLBACK_HXX
#define _URLCALLBACK_HXX
/*
 * @(#) URLCallback.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "urlstream.hxx"
#include <urlmon.h>

//typedef _tsreference<IBindStatusCallback> TSRIBindStatusCallback;
//typedef _tsreference<IBinding> TSRBinding;
typedef _reference<IBindStatusCallback> RIBindStatusCallback;
typedef _reference<IBinding> RBinding;

class URLStream;
typedef _reference<URLStream> RURLStream;
class URLCallback;
typedef _reference<URLCallback> RURLCallback;

class URLCallback : public _unknown<IBindStatusCallback, &IID_IBindStatusCallback>,
                    public IHttpNegotiate
{
public:
    ////////////////////////////////////////////////////////////
    // Constructor & Destructor
    //    
    URLCallback(URLStream* s);
    virtual ~URLCallback();

public:

    ////////////////////////////////////////////////////////////
    // IUnknown interface
    //
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);
    ULONG STDMETHODCALLTYPE AddRef( void);
    ULONG STDMETHODCALLTYPE Release( void);

    ////////////////////////////////////////////////////////////
    // IURLCallback Interface
    // 
    HRESULT STDMETHODCALLTYPE OnStartBinding(
        /* [in] */ DWORD grfBSCOption,
        /* [in] */ IBinding *pib);

    HRESULT STDMETHODCALLTYPE GetPriority(
       /* [out] */ LONG *pnPriority);

    HRESULT STDMETHODCALLTYPE OnLowResource(
        /* [in] */ DWORD reserved);

    HRESULT STDMETHODCALLTYPE OnProgress(
        /* [in] */ ULONG ulProgress,
        /* [in] */ ULONG ulProgressMax,
        /* [in] */ ULONG ulStatusCode,
        /* [in] */ LPCWSTR szStatusText);

    HRESULT STDMETHODCALLTYPE OnStopBinding(
        /* [in] */ HRESULT hresult,
        /* [in] */ LPCWSTR szError);

    HRESULT STDMETHODCALLTYPE GetBindInfo(
        /* [out] */ DWORD *grfBINDF,
        /* [unique][out][in] */ BINDINFO *pbindinfo);

    HRESULT STDMETHODCALLTYPE OnDataAvailable(
        /* [in] */ DWORD grfBSCF,
        /* [in] */ DWORD dwSize,
        /* [in] */ FORMATETC *pformatetc,
        /* [in] */ STGMEDIUM *pstgmed);

    HRESULT STDMETHODCALLTYPE OnObjectAvailable(
        /* [in] */ REFIID iid,
        /* [iid_is][in] */ IUnknown *punknown);


    ////////////////////////////////////////////////////////////
    // IHttpNegotiate Interface
    // 
    HRESULT STDMETHODCALLTYPE BeginningTransaction( 
        /* [in] */ LPCWSTR szURL,
        /* [unique][in] */ LPCWSTR szHeaders,
        /* [in] */ DWORD dwReserved,
        /* [out] */ LPWSTR __RPC_FAR *pszAdditionalHeaders);
    
    HRESULT STDMETHODCALLTYPE OnResponse( 
        /* [in] */ DWORD dwResponseCode,
        /* [unique][in] */ LPCWSTR szResponseHeaders,
        /* [unique][in] */ LPCWSTR szRequestHeaders,
        /* [out] */ LPWSTR __RPC_FAR *pszAdditionalRequestHeaders);

    void SetPreviousCallback(IBindStatusCallback* pbs) { _pbs = pbs; }
    void Reset();
    void Abort();

private:

    HRESULT getMediaType(char* szContentType, long len);

private:

    RURLStream  _pStream;
//  TSRIBindStatusCallback _pbs;
//  TSRBinding _binding;
    RIBindStatusCallback _pbs;
    RBinding _binding;
    bool _fFinishedBinding;
    LPWSTR _pszwActualURL;
    LONG _lRefs;
    bool _fGetMimeType;
};


/*----------------------------------------------------------------------------
        IsStatusOK 
        Checks for status codes that are considered "normal" from an urlmon 
        perspective i.e. no special action or error code paths should be taken
        in these cases.
-----------------------------------------------------------------------------*/

inline BOOL IsStatusOk(DWORD dwStatus)
{
    return ((dwStatus == HTTP_STATUS_OK) || (dwStatus == HTTP_STATUS_RETRY_WITH));
}

#endif _URLCALLBACK_HXX

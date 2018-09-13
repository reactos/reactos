/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _CALLBACK_HXX
#define _CALLBACK_HXX

#ifndef UNIX
#include <comdef.h>
#endif // UNIX
#include "utils.hxx"
#include "mimedownload.hxx"

class ViewerFactory;
class Viewer;

//=======================================================================
class CallbackWrapper : public _unknown<IBindStatusCallback, &IID_IBindStatusCallback>
{
public:
    CallbackWrapper();
    virtual ~CallbackWrapper();

    void SetPreviousCallback(IBindStatusCallback* pbs);

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

private:
    IBindStatusCallback* _pbs;
};

//=======================================================================
class CallbackMonitor : public _unknown<IBindStatusCallback, &IID_IBindStatusCallback>
{
public:
    CallbackMonitor();
    virtual ~CallbackMonitor();

    void SetPreviousCallback(IBindStatusCallback* pbs);

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

private:
    IBindStatusCallback* _pbs;
#if MIMEASYNC
    BOOL _fAsync;
public:
    void TurnOnAsync(void) { _fAsync = TRUE; };
#endif
};

//=======================================================================
class CBinding : public _unknown<IBinding, &IID_IBinding>
{
public:
    CBinding();
    virtual ~CBinding();

    virtual HRESULT STDMETHODCALLTYPE Abort(void);
    virtual HRESULT STDMETHODCALLTYPE Suspend(void);
    virtual HRESULT STDMETHODCALLTYPE Resume(void);

    virtual HRESULT STDMETHODCALLTYPE SetPriority( 
        /* [in] */ LONG nPriority);
    
    virtual HRESULT STDMETHODCALLTYPE GetPriority( 
        /* [out] */ LONG __RPC_FAR *pnPriority);
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetBindResult( 
        /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
        /* [out] */ DWORD __RPC_FAR *pdwResult,
        /* [out] */ LPOLESTR __RPC_FAR *pszResult,
        /* [out][in] */ DWORD __RPC_FAR *pdwReserved);

    void SetAbortCB(Viewer *pViewer, ViewerFactory *pNodeFactory);

    IUnknown* getTrident();

private:
    Viewer *_pViewer;
    ViewerFactory *_pNF;
};

#endif
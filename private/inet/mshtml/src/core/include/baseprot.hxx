//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       baseprot.hxx
//
//  Contents:   Base class for pluggable protocols
//
//  History:    02-12-1997   AnandRa (Anand Ramakrishna)    Created
//
//----------------------------------------------------------------------------

#ifndef I_BASEPROT_HXX_
#define I_BASEPROT_HXX_
#pragma INCMSG("--- Beg 'baseprot.hxx'")

#ifndef X_URLMON_H_
#define X_URLMON_H_
#pragma INCMSG("--- Beg <urlmon.h>")
#include <urlmon.h>
#pragma INCMSG("--- End <urlmon.h>")
#endif

#define BIND_ASYNC 1

BOOL HasSecureContext(const TCHAR *pchUrl);


class CBaseProtocolCF : public CBaseCF,
                        public IInternetProtocolInfo
{
typedef CBaseCF super;

public:
    // constructor
    CBaseProtocolCF(FNCREATE *pfnCreate) : CBaseCF(pfnCreate) {}

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG, AddRef) (void)
        { return super::AddRef(); }
    STDMETHOD_(ULONG, Release) (void)
        { return super::Release(); }
        
    // IInternetProtocolInfo methods
    STDMETHODIMP ParseUrl(LPCWSTR     pwzUrl,
                          PARSEACTION ParseAction,
                          DWORD       dwFlags,
                          LPWSTR      pwzResult,
                          DWORD       cchResult,
                          DWORD *     pcchResult,
                          DWORD       dwReserved);
    STDMETHODIMP CombineUrl(LPCWSTR     pwzBaseUrl,
                            LPCWSTR     pwzRelativeUrl,
                            DWORD       dwFlags,
                            LPWSTR      pwzResult,
                            DWORD       cchResult,
                            DWORD *     pcchResult,
                            DWORD       dwReserved);
    STDMETHODIMP CompareUrl(LPCWSTR     pwzUrl1,
                            LPCWSTR     pwzUrl2,
                            DWORD       dwFlags);
    STDMETHODIMP QueryInfo(LPCWSTR         pwzUrl,
                           QUERYOPTION     QueryOption,
                           DWORD           dwQueryFlags,
                           LPVOID          pBuffer,
                           DWORD           cbBuffer,
                           DWORD *         pcbBuf,
                           DWORD           dwReserved);

    // Misc methods
    HRESULT UnwrapSpecialUrl(LPCWSTR pchUrl, CStr &cstrUnwrappedUrl);
};

class CBaseProtocol : public CBase,
                      public IInternetProtocol, 
                      public IServiceProvider
{
typedef CBase super;

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))

public:
    
    // CBase overrides
    DECLARE_AGGREGATED_IUNKNOWN(CBaseProtocol)

    // ctor/dtor
    CBaseProtocol(IUnknown *pUnkOuter);
    ~CBaseProtocol();
    
    virtual void Passivate();
    IUnknown * PunkOuter() { return _pUnkOuter; }

    // Real workers.  Just override this one method in derived classes.
    virtual HRESULT ParseAndBind();

    DECLARE_PRIVATE_QI_FUNCS(super)

    // IInternetProtocol methods
    STDMETHOD(Start)(
            LPCWSTR szUrl,
            IInternetProtocolSink *pProtSink,
            IInternetBindInfo *pOIBindInfo,
            DWORD grfSTI,
            HANDLE_PTR dwReserved);
    STDMETHOD(Continue)(PROTOCOLDATA *pStateInfo);
    STDMETHOD(Abort)(HRESULT hrReason,DWORD dwOptions);
    STDMETHOD(Terminate)(DWORD dwOptions);
    STDMETHOD(Suspend)();
    STDMETHOD(Resume)();
    STDMETHOD(Read)(void *pv,ULONG cb,ULONG *pcbRead);
    STDMETHOD(Seek)(
            LARGE_INTEGER dlibMove,
            DWORD dwOrigin,
            ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(LockRequest)(DWORD dwOptions);
    STDMETHOD(UnlockRequest)();

    //IServiceProvider methods
    STDMETHOD(QueryService)(REFGUID rsid, REFIID riid, void ** ppvObj);

    // Data members
    IUnknown *              _pUnkOuter;
    CStr                    _cstrURL;           // The url
    IInternetProtocolSink * _pProtSink;         // The protocol sink
    IInternetBindInfo *     _pOIBindInfo;       // The Bind info
    DWORD                   _bscf;
    DWORD                   _grfBindF;
    DWORD                   _grfSTI;
    BINDINFO                _bindinfo;
    IStream *               _pStm;              // Stream of output data
#if DBG == 1
    DWORD                   _dwTID;             // Thread id
#endif

    // Bitfields
    unsigned                _fAborted:1;        // Whether we're already aborted.
};

#pragma INCMSG("--- End 'baseprot.hxx'")
#else
#pragma INCMSG("*** Dup 'baseprot.hxx'")
#endif

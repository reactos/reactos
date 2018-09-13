//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       namespb.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-20-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _NAMESPACEOHSERV_HXX_
#define _NAMESPACEOHSERV_HXX_

#include "protbase.hxx"


class COhServNameSp : public CBaseProtocol
{
public:

    STDMETHODIMP Start(LPCWSTR szUrl,IOInetProtocolSink *pProtSink,
                        IOInetBindInfo *pOIBindInfo, DWORD grfSTI, DWORD dwReserved);

    STDMETHODIMP Continue(PROTOCOLDATA *pStateInfo);

    STDMETHODIMP Read(void *pv,ULONG cb,ULONG *pcbRead);

    STDMETHODIMP Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,
                ULARGE_INTEGER *plibNewPosition);

    STDMETHODIMP LockRequest(DWORD dwOptions);

    STDMETHODIMP UnlockRequest();

    STDMETHODIMP ParseAndStart(BOOL fBind = TRUE);

public:
    COhServNameSp(REFCLSID rclsid, IUnknown *pUnkOuter, IUnknown **ppUnkInner);
    virtual ~COhServNameSp();

};


#endif // _NAMESPACEOHSERV_HXX_


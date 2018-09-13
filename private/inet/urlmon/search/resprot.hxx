//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       protbase.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _RESPROT_HXX_
#define _RESPROT_HXX_

#define offsetof(s,m) (size_t)&(((s *)0)->m)
#define GETPPARENT(pmemb, struc, membname) ((struc FAR *)(((char FAR *)(pmemb))-offsetof(struc, membname)))

#define MAX_URL_SIZE    INTERNET_MAX_URL_LENGTH

class CResProtocol : public CBaseProtocol
{
public:

    STDMETHODIMP Start(LPCWSTR szUrl,IOInetProtocolSink *pProtSink,
                        IOInetBindInfo *pOIBindInfo, DWORD grfSTI, DWORD dwReserved);

    STDMETHODIMP Continue(PROTOCOLDATA *pStateInfo);

    STDMETHODIMP Read(void *pv,ULONG cb,ULONG *pcbRead);

    STDMETHODIMP Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,
                ULARGE_INTEGER *plibNewPosition);

public:
    CResProtocol(REFCLSID rclsid, IUnknown *pUnkOuter, IUnknown **ppUnkInner);
    virtual ~CResProtocol();

private:
    STDMETHODIMP Bind();

private:


};

#endif // _RESPROT_HXX_

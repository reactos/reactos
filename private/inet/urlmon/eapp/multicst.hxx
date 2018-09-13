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
#ifndef _MULTICST_HXX_
#define _MULTICST_HXX_

#define offsetof(s,m) (size_t)&(((s *)0)->m)
#define GETPPARENT(pmemb, struc, membname) ((struc FAR *)(((char FAR *)(pmemb))-offsetof(struc, membname)))

#define MAX_URL_SIZE    INTERNET_MAX_URL_LENGTH

class CMulticastProtocol : public CBaseProtocol
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

public:
    CMulticastProtocol(REFCLSID rclsid, IUnknown *pUnkOuter, IUnknown **ppUnkInner);
    virtual ~CMulticastProtocol();

private:
    STDMETHODIMP GetResource(LPCWSTR pwzFileName, LPCWSTR pwzResName, LPCWSTR pwzResType, LPCWSTR pwzMime);
    STDMETHODIMP ParseAndBind(BOOL fBind = TRUE);

private:

    LPWSTR  _wzRID;       // String resource id to use
    LPWSTR  _wzResName;   // Resource dll name
    LPWSTR  _wzDispName;  // Full display name

    HINSTANCE   _hInst;
    ULONG   _cbBuffer;
    LPVOID  _pBuffer;
    HGLOBAL _hgbl;
    ULONG   _cbPos;

};

#endif // _MULTICST_HXX_

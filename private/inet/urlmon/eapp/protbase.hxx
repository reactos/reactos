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
#ifndef _PROTBASE_HXX_
#define _PROTBASE_HXX_

#ifdef unix
#include <stddef.h> // For 'offsetof.'
#else
#define offsetof(s,m) (SIZE_T)&(((s *)0)->m)
#endif /* unix */

#define GETPPARENT(pmemb, struc, membname) ((struc FAR *)(((char FAR *)(pmemb))-offsetof(struc, membname)))

#define MAX_URL_SIZE    INTERNET_MAX_URL_LENGTH

// this will be in a common header file
#define S_NEEDMOREDATA                                ((HRESULT)0x00000002L)
#define BSCF_ASYNCDATANOTIFICATION  0x00010000
#define BSCF_DATAFULLYAVAILABLE     0x00020000

class CHttpNegotiate : public IHttpNegotiate
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    // *** IHttpNegotiate methods ***
    STDMETHOD(BeginningTransaction) (
        LPCWSTR szURL,
        LPCWSTR szHeaders,
        DWORD dwReserved,
        LPWSTR *pszAdditionalHeaders);

    STDMETHOD(OnResponse) (
        DWORD dwResponseCode,
        LPCWSTR szResponseHeaders,
        LPCWSTR szRequestHeaders,
        LPWSTR *pszAdditionalRequestHeaders);


    CHttpNegotiate(IHttpNegotiate *pNeg)
    {
        if (pNeg)
        {
            _pHttpNegDelegate = pNeg;
            _pHttpNegDelegate->AddRef();
        }
    }
    ~CHttpNegotiate()
    {
        if (_pHttpNegDelegate)
        {
            _pHttpNegDelegate->Release();
        }
    }

protected:
    CRefCount       _CRefs;          // the total refcount of this object

    IHttpNegotiate      *_pHttpNegDelegate;
};

class CBaseProtocol : public IOInetProtocol, public IOInetThreadSwitch, public IOInetPriority, public IServiceProvider
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP Start(LPCWSTR szUrl,IOInetProtocolSink *pProtSink,
                       IOInetBindInfo *pOIBindInfo,DWORD grfSTI, DWORD_PTR dwReserved);

    STDMETHODIMP Continue(PROTOCOLDATA *pStateInfo);

    STDMETHODIMP Abort(HRESULT hrReason,DWORD dwOptions);

    STDMETHODIMP Terminate(DWORD dwOptions);

    STDMETHODIMP Suspend();

    STDMETHODIMP Resume();

    STDMETHODIMP Read(void *pv,ULONG cb,ULONG *pcbRead);

    STDMETHODIMP Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,
                        ULARGE_INTEGER *plibNewPosition);

    STDMETHODIMP LockRequest(DWORD dwOptions);

    STDMETHODIMP UnlockRequest();

    // IOInetPriority
    STDMETHODIMP SetPriority(LONG nPriority);

    STDMETHODIMP GetPriority(LONG * pnPriority);

    // IOInetThreadSwitch
    STDMETHODIMP Prepare();

    STDMETHODIMP Continue();

    //IServiceProvider
    STDMETHODIMP QueryService(REFGUID rsid, REFIID riid, void ** ppvObj);
    STDMETHODIMP ObtainService(REFGUID rsid, REFIID riid, void ** ppvObj);



public:
    CBaseProtocol(REFCLSID rclsid, IUnknown *pUnkOuter, IUnknown **ppUnkInner);
    virtual ~CBaseProtocol();

    BOOL OpenTempFile();
    BOOL CloseTempFile();


    BOOL IsApartmentThread()
    {
       EProtAssert((_dwThreadID != 0));
       return (_dwThreadID == GetCurrentThreadId());
    }


protected:
    CRefCount       _CRefs;          // the total refcount of this object
    DWORD           _dwThreadID;
    LPWSTR          _pwzUrl;

    IOInetProtocolSink  *_pProtSink;
    IOInetBindInfo      *_pOIBindInfo;
    IServiceProvider    *_pServProvDelegate;
    IHttpNegotiate      *_pHttpNeg;

    REFCLSID        _pclsidProtocol;

    DWORD           _bscf;
    DWORD           _grfBindF;
    DWORD           _grfSTI;
    BINDINFO        _BndInfo;
    WCHAR           _wzFullURL[MAX_URL_SIZE + 1];
    HANDLE          _hFile;
    char            _szTempFile[MAX_PATH];


    class CPrivUnknown : public IUnknown
    {
    public:
        STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (void);
        STDMETHOD_(ULONG,Release) (void);

        ~CPrivUnknown() {}
        CPrivUnknown() : _CRefs() {}

    private:
        CRefCount   _CRefs;          // the total refcount of this object
    };

    friend class CPrivUnknown;
    CPrivUnknown     _Unknown;

    IUnknown        *_pUnkOuter;

    // the next punk is the punkinner in
    // another protocol gets loaded by us
    // the inner object if the protocol supports aggregation
    IUnknown        *_pUnkInner;
    // the protocol pointer if another protocol gets loaded
    IOInetProtocol  *_pProt;


    STDMETHODIMP_(ULONG) PrivAddRef()
    {
        return _Unknown.AddRef();
    }
    STDMETHODIMP_(ULONG) PrivRelease()
    {
        return _Unknown.Release();
    }

};

LPWSTR OLESTRDuplicate(LPWSTR ws);
LPSTR DupW2A(const WCHAR *pwz);

HRESULT CreateAPP(REFCLSID rclsid, IUnknown *pUnkOuter, REFIID riid, IUnknown **ppUnk);

#define RES_STATE_BIND 4711

#endif // _PROTBASE_HXX_

 


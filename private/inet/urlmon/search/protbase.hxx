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

#define offsetof(s,m) (size_t)&(((s *)0)->m)
#define GETPPARENT(pmemb, struc, membname) ((struc FAR *)(((char FAR *)(pmemb))-offsetof(struc, membname)))

#define MAX_URL_SIZE    INTERNET_MAX_URL_LENGTH

// this will be in a common header file
#define S_NEEDMOREDATA                                ((HRESULT)0x00000002L)
#define BSCF_ASYNCDATANOTIFICATION  0x00010000
#define BSCF_DATAFULLYAVAILABLE     0x00020000


class CBaseProtocol : public IOInetProtocol, public IOInetThreadSwitch, public IOInetPriority
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP Start(LPCWSTR szUrl,IOInetProtocolSink *pProtSink,
                       IOInetBindInfo *pOIBindInfo,DWORD grfSTI,DWORD dwReserved);

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

public:
    CBaseProtocol(REFCLSID rclsid, IUnknown *pUnkOuter, IUnknown **ppUnkInner);
    virtual ~CBaseProtocol();

    BOOL OpenTempFile();
    BOOL CloseTempFile();


    BOOL IsApartmentThread()
    {
       TransAssert((_dwThreadID != 0));
       return (_dwThreadID == GetCurrentThreadId());
    }


protected:
    CRefCount       _CRefs;          // the total refcount of this object
    DWORD           _dwThreadID;
    LPTSTR          _pszUrl;
    TCHAR           _szNewUrl[MAX_URL_SIZE + 1];

    IOInetProtocolSink  *_pProtSink;
    IOInetBindInfo      *_pOIBindInfo;
    REFCLSID        _pclsidProtocol;

    DWORD           _bscf;
    DWORD           _grfBindF;
    BINDINFO        _BndInfo;

    IOInetProtocol  *_pProt;

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
LPWSTR DupA2W(const LPSTR psz);

HRESULT CreateAPP(REFCLSID rclsid, IUnknown *pUnkOuter, REFIID riid, IUnknown **ppUnk);

inline void W2A(LPCWSTR lpwszWide, LPSTR lpszAnsi, int cchAnsi)
{
    WideCharToMultiByte(CP_ACP,0,lpwszWide,-1,lpszAnsi,cchAnsi,NULL,NULL);
}
inline void A2W(LPSTR lpszAnsi,LPWSTR lpwszWide, int cchAnsi)
{
    MultiByteToWideChar(CP_ACP,0,lpszAnsi,-1,lpwszWide,cchAnsi);
}


#ifndef ARRAYSIZE
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#endif


#endif // _PROTBASE_HXX_


//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1996                    **
//*********************************************************************

#ifndef _BINDCB_H_

// the CStubBindStatusCallback implements IBindStatusCallback and
// IHttpNegotiate.  We use it to make a "fake" bind status callback
// object when we have headers and post data we would like to apply
// to a navigation.  We supply this IBindStatusCallback object, and
// the URL moniker asks us for headers and post data and use those in
// the transaction.

class CStubBindStatusCallback : public IBindStatusCallback,
                                       IHttpNegotiate,
                                       IMarshal
{
private:
    UINT      _cRef;         // ref count on this COM object
    LPCTSTR    _pszHeaders;  // headers to use
    HGLOBAL   _hszPostData;  // post data to use
    DWORD     _cbPostData;   // size of post data
    BOOL      _bFrameIsOffline : 1; // Indicates if Offline property is set
    BOOL      _bFrameIsSilent : 1;  // Indicates if Silent property is set
    BOOL      _bHyperlink : 1;  // This is a hyperlink or top level request
    DWORD     _grBindFlags; //  optional additional bindinfo flags

public:
    CStubBindStatusCallback(LPCWSTR pwzHeaders,LPCBYTE pPostData,DWORD cbPostData,
                            VARIANT_BOOL bFrameIsOffline, VARIANT_BOOL bFrameIsSilent, BOOL bHyperlink, DWORD grBindFlags);
    ~CStubBindStatusCallback();

    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IBindStatusCallback ***
    virtual STDMETHODIMP OnStartBinding(
        /* [in] */ DWORD grfBSCOption,
        /* [in] */ IBinding *pib);

    virtual STDMETHODIMP GetPriority(
        /* [out] */ LONG *pnPriority);

    virtual STDMETHODIMP OnLowResource(
        /* [in] */ DWORD reserved);

    virtual STDMETHODIMP OnProgress(
        /* [in] */ ULONG ulProgress,
        /* [in] */ ULONG ulProgressMax,
        /* [in] */ ULONG ulStatusCode,
        /* [in] */ LPCWSTR szStatusText);

    virtual STDMETHODIMP OnStopBinding(
        /* [in] */ HRESULT hresult,
        /* [in] */ LPCWSTR szError);

    virtual STDMETHODIMP GetBindInfo(
        /* [out] */ DWORD *grfBINDINFOF,
        /* [unique][out][in] */ BINDINFO *pbindinfo);

    virtual STDMETHODIMP OnDataAvailable(
        /* [in] */ DWORD grfBSCF,
        /* [in] */ DWORD dwSize,
        /* [in] */ FORMATETC *pformatetc,
        /* [in] */ STGMEDIUM *pstgmed);

    virtual STDMETHODIMP OnObjectAvailable(
        /* [in] */ REFIID riid,
        /* [iid_is][in] */ IUnknown *punk);

    /* *** IHttpNegotiate ***  */
    virtual STDMETHODIMP BeginningTransaction(LPCWSTR szURL, LPCWSTR szHeaders,
            DWORD dwReserved, LPWSTR __RPC_FAR *pszAdditionalHeaders);

    virtual STDMETHODIMP OnResponse(DWORD dwResponseCode, LPCWSTR szResponseHeaders,
            LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders);

    // IMarshal methods
                    
    STDMETHODIMP GetUnmarshalClass(REFIID riid,void *pvInterface,
        DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,CLSID *pCid);
    STDMETHODIMP GetMarshalSizeMax(REFIID riid,void *pvInterface,
        DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,DWORD *pSize);
    STDMETHODIMP MarshalInterface(IStream *pistm,REFIID riid,
                                void *pvInterface,DWORD dwDestContext,
                                void *pvDestContext,DWORD mshlflags);
    STDMETHODIMP UnmarshalInterface(IStream *pistm,REFIID riid,void ** ppvObj);
    STDMETHODIMP ReleaseMarshalData(IStream *pStm);
    STDMETHODIMP DisconnectObject(DWORD dwReserved);

    // helper methods
    STDMETHODIMP _FreeHeadersAndPostData();
    BOOL _CanMarshalIID(REFIID riid);
    HRESULT _ValidateMarshalParams(REFIID riid,void *pvInterface,
                    DWORD dwDestContext,void *pvDestContext,DWORD mshlflags);

};

// BUGBUG:
// private flags between shdocvw and mshtml
// -> should be done via bind context
//
#define BINDF_INLINESGETNEWESTVERSION   0x10000000
#define BINDF_INLINESRESYNCHRONIZE      0x20000000
#define BINDF_CONTAINER_NOWRITECACHE    0x40000000


// global helper functions
BOOL fOnProxy();
HRESULT BuildBindInfo(DWORD *grfBINDF,BINDINFO *pbindinfo,HGLOBAL hszPostData,
    DWORD cbPostData, BOOL bFrameIsOffline, BOOL bFrameIsSilent, BOOL bHyperlink, LPUNKNOWN pUnkForRelease);
HRESULT BuildAdditionalHeaders(LPCTSTR pszOurExtraHeaders,LPCWSTR * ppwzCombinedHeadersOut);
HRESULT CStubBindStatusCallback_Create(LPCWSTR pwzHeaders, LPCBYTE pPostData,
    DWORD cbPostData, VARIANT_BOOL bFrameIsOffline, VARIANT_BOOL bFrameIsSilent,BOOL bHyperlink,
    DWORD grBindFlags,
    CStubBindStatusCallback ** ppBindStatusCallback);
HRESULT GetHeadersAndPostData(IBindStatusCallback * pBindStatusCallback,
    LPTSTR * ppszHeaders, STGMEDIUM * pstgPostData, DWORD * pdwPostData, BOOL * pfUseCache);
HRESULT GetTopLevelBindStatusCallback(IServiceProvider * psp,
    IBindStatusCallback ** ppBindStatusCallback);
HRESULT GetTopLevelPendingBindStatusCallback(IServiceProvider * psp,
    IBindStatusCallback ** ppBindStatusCallback);

#endif // _BINDCB_H_

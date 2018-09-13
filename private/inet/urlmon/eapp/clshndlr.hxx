#ifndef _CLSHNDLR_HXX_
#define _CLSHNDLR_HXX_

enum INSTALLSTATE
{
    installingNone,
    installingDone,
    installingBoth,
    installingDocObject,
    installingHandler
};

class CClassInstallFilterSink;
    
class CInstallBindInfo : public IOInetBindInfo
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IOInetBindInfo methods
    STDMETHODIMP GetBindInfo(DWORD *grfBINDF, BINDINFO * pbindinfo);
    STDMETHODIMP GetBindString(ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched);

    CInstallBindInfo();
    ~CInstallBindInfo();

private:
    DWORD _CRefs;
};


class CClassInstallFilter : public IOInetProtocol
                            ,public IOInetProtocolSink
                            ,public IServiceProvider
                            ,public IInternetHostSecurityManager
{
    friend class CClassInstallFilterSink;

public:

    CClassInstallFilter();
    ~CClassInstallFilter();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IOInetProtocol methods
    STDMETHODIMP Start(LPCWSTR szUrl,IOInetProtocolSink *pProtSink,
                       IOInetBindInfo *pOIBindInfo,DWORD grfSTI,DWORD_PTR dwReserved);
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

    // IOInetProtocolSink methods
    STDMETHODIMP Switch(PROTOCOLDATA *pStateInfo);
    STDMETHODIMP ReportProgress(ULONG ulStatusCode, LPCWSTR szStatusText);
    STDMETHODIMP ReportData(DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax);
    STDMETHODIMP ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult);

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID rsid, REFIID riid, void ** ppvObj);

    INSTALLSTATE GetInstallState() 
    {
        return _state;
    };

    void SetInstallState(INSTALLSTATE state)
    {
        _state = state;
    };

    // IInternetHostSecurityManager
    STDMETHODIMP GetSecurityId(BYTE *pbSecurityId, DWORD *pcbSecurityId,
                               DWORD_PTR dwReserved);
    
    STDMETHODIMP ProcessUrlAction(DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy,
                                  BYTE *pContext, DWORD cbContext, DWORD dwFlags,
                                  DWORD dwReserved);
    
    STDMETHODIMP QueryCustomPolicy(REFGUID guidKey, BYTE **ppPolicy,
                                   DWORD *pcbPolicy, BYTE *pContext,
                                   DWORD cbContext, DWORD dwReserved);

private:
    HRESULT InstallerReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult);
    
    CClassInstallFilterSink* _pInstallSink;
    IOInetProtocolSink* _pProtSnk;
    IOInetProtocol* _pCDLnetProtocol;
    IOInetProtocol* _pProt;
    LPWSTR  _pwzCDLURL;
    INSTALLSTATE _state;
    BOOL _bAddRef;

    DWORD _grfBSCF;
    ULONG _ulProgress;
    ULONG _ulProgressMax;

    DWORD _dwTotalSize;

    // cached ReportData if main DocObject finishes downloading before install handler
    BOOL _fDocObjectDone;

    DWORD _hrResult;
    DWORD _dwError;
    LPWSTR _wzResult;
    BOOL _fReportResult;

    DWORD _CRefs;
    LPWSTR _pwzUrl;
    LPWSTR _pwzClsId;
    LPWSTR _pwzMime;
    WCHAR _pwzDocBase[INTERNET_MAX_URL_LENGTH];

    IInternetSecurityManager *_pSecMgr;

};

class CClassInstallFilterSink : public IOInetProtocolSink
                                ,public IServiceProvider
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IOInetProtocolSink methods
    STDMETHODIMP Switch(PROTOCOLDATA *pStateInfo);
    STDMETHODIMP ReportProgress(ULONG ulStatusCode, LPCWSTR szStatusText);
    STDMETHODIMP ReportData( DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax);
    STDMETHODIMP ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult);

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID rsid, REFIID riid, void ** ppvObj);

    CClassInstallFilterSink(CClassInstallFilter *pInstallFilter);
    ~CClassInstallFilterSink();

private:
    CClassInstallFilter *_pInstallFilter;
    DWORD _dwRef;
    BOOL _bDone;

};
 
#endif // _CLSHNDLR_HXX_

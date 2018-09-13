
#define PP_PRE_SWITCH   0x00000001
#define PP_POST_SWITCH  0x00000002
#define PP_DELETE       0x00000004

class COInetProt : public IOInetProtocol
                 , public IOInetProtocolSink
                 , public IServiceProvider
                 , public IOInetPriority

{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    //
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

    //
    // IOInetProtocolSink methods
    STDMETHODIMP Switch(PROTOCOLDATA *pStateInfo);

    STDMETHODIMP ReportProgress(ULONG ulStatusCode, LPCWSTR szStatusText);

    STDMETHODIMP ReportData( DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax);

    STDMETHODIMP ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult);
    
    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID rsid, REFIID iid, void ** ppvObj);

    // IOInetPriority
    STDMETHODIMP SetPriority(LONG nPriority);

    STDMETHODIMP GetPriority(LONG * pnPriority);
    
        
public:

    STDMETHODIMP Create(COInetProt **ppCProtHandler);
    STDMETHODIMP OnDataReceived(DWORD *pgrfBSC, DWORD *pcbBytesAvailable, DWORD *pdwTotalSize); 
    
    void SetServiceProvider(IServiceProvider *pSrvPrv)
    {   
        if (pSrvPrv)
        {
            pSrvPrv->AddRef();
        }
        if (_pSrvPrv)
        {
            _pSrvPrv->Release();
        }
        _pSrvPrv = pSrvPrv;
    }
    
    void SetProtocolSink(IOInetProtocolSink *pProtSnk)
    {   
        if (pProtSnk)
        {
            pProtSnk->AddRef();
        }
        if (_pProtSnk)
        {
            _pProtSnk->Release();
        }
        _pProtSnk = pProtSnk;
    }
    void SetProtocol(IOInetProtocol *pProt)
    {
        if (pProt)
        {
            pProt->AddRef();
        }
        if (_pProt)
        {
            _pProt->Release();
        }

        _pProt = pProt;
    }
    HRESULT GetProtocol( IOInetProtocol **ppProt)
    {   
        if (_pProt)
        {
            _pProt->AddRef();
            *ppProt = _pProt;
        }
        
        return (_pProt) ? NOERROR : E_NOINTERFACE;
    }

    void SetHandler(IOInetProtocol *pProt, IOInetProtocolSink *pProtSnk)
    {   
        if (_pProtHandler)
        {
            _pProtHandler->Release();
        }
        _pProtHandler = pProt;
        if (_pProtSnkHandler)
        {
            _pProtSnkHandler->Release();
        }
        _pProtSnkHandler = pProtSnk;
        if (_pProtHandler)
        {
            _pProtHandler->AddRef();
        }
        if (_pProtSnkHandler)
        {
            _pProtSnkHandler->AddRef();
        }
    }

    STDMETHODIMP Initialize(CTransaction *pCTrans, IServiceProvider *pSrvPrv, DWORD dwMode, DWORD dwOptions, IUnknown *pUnk, IOInetProtocol *pProt, IOInetProtocolSink *pProtSnk, LPWSTR pwzUrl = 0);
    static HRESULT Create(IUnknown *pUnk, IOInetProtocolSink *pProtSnk, COInetProt **pCOInetProtHndler)
    {
        HRESULT hr = NOERROR;

        
        if (!pUnk || !pProtSnk)
        {
            hr = E_INVALIDARG;
        }
        else 
        {
            *pCOInetProtHndler = new COInetProt();
            if (*pCOInetProtHndler == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        
        return hr;
    }

    DWORD   GetOInetBindFlags()
    {
        return _dwOInetBdgFlags;
    }

    void    SetOInetBindFlags(DWORD dwBdgFlags)
    {
        _dwOInetBdgFlags = dwBdgFlags;
    }
    
    COInetProt() : _CRefs()
    {
        _pUnk = 0;
        _pProt = 0;
        _pProtSnk = 0;
        _pCTrans = 0;
        _pSrvPrv = 0;
        _dwMode = 0;
        _dwOInetBdgFlags = 0;
        _pBuffer = 0;           // DNLD_BUFFER_SIZE  size buffer
        _cbBufferSize = 0;
        _cbTotalBytesRead = 0;
        _cbBufferFilled = 0;    //how much of the buffer is in use
        _cbDataSniffMin = 0;
        _cbBytesReported = 0;
        _fDocFile = 0;
        _fMimeVerified = 0;
        _fMimeReported = 0;
        _pwzFileName = 0;
        _pwzMimeSuggested = 0;
        _fDelete = 0;
        _pProtHandler = 0;
        _pProtSnkHandler = 0;
        _fNeedMoreData = 0;
        _fGotHandler = 0;
        _fWaitOnHandler = 0;
        _pwzUrl = 0;
        _nPriority = THREAD_PRIORITY_NORMAL;
    }
    
    ~COInetProt()
    {
        delete [] _pwzFileName;
        delete [] _pwzMimeSuggested;
        delete [] _pBuffer;
        delete [] _pwzUrl;
    }
    
private:
    CRefCount            _CRefs;        // the total refcount of this object
    CMutexSem            _mxs;              // used in Read, Seek, Abort and package list in case of apartment threaded
    
    IUnknown            *_pUnk;
    IOInetProtocol      *_pProt;            // the prot the filter reads from
    IOInetProtocolSink  *_pProtSnk;         // the prot report progress 
    IServiceProvider    *_pSrvPrv;
    CTransaction        *_pCTrans;
    LONG                 _nPriority;
    
    DWORD               _dwOInetBdgFlags;
    DWORD               _dwMode;            //
    LPBYTE              _pBuffer;           // DNLD_BUFFER_SIZE  size buffer
    ULONG               _cbBufferSize;
    ULONG               _cbTotalBytesRead;
    ULONG               _cbBufferFilled;    //how much of the buffer is in use
    //ULONG               _cbDataSize;
    ULONG               _cbDataSniffMin;
    ULONG               _cbBytesReported;   // how much was reported
    
    BOOL                _fDocFile       : 1;
    BOOL                _fMimeVerified  : 1;
    BOOL                _fMimeReported  : 1;
    BOOL                _fDelete        : 1;

    BOOL                _fNeedMoreData  : 1;
    BOOL                _fGotHandler    : 1;
    BOOL                _fWaitOnHandler : 1;

    LPWSTR              _pwzFileName;
    LPWSTR              _pwzMimeSuggested;
    LPWSTR              _pwzUrl;

    IOInetProtocol      *_pProtHandler;
    IOInetProtocolSink  *_pProtSnkHandler;
};


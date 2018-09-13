
#define PP_PRE_SWITCH   0x00000001
#define PP_POST_SWITCH  0x00000002
#define PP_DELETE       0x00000004



#define PP_PRE_SWITCH   0x00000001
#define PP_POST_SWITCH  0x00000002
#define PP_DELETE       0x00000004
#define DATASNIFSIZEDOCFILE_MIN 2048

class CMimeHandlerTest1 : public CBaseProtocol
                //: public IOInetProtocol
                 , public IOInetProtocolSink
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    //
    //IOInetProtocol methods
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

    //
    // IOInetProtocolSink methods
    STDMETHODIMP Switch(PROTOCOLDATA *pStateInfo);

    STDMETHODIMP ReportProgress(ULONG ulStatusCode, LPCWSTR szStatusText);

    STDMETHODIMP ReportData( DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax);

    STDMETHODIMP ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult);
        
public:

    STDMETHODIMP Create(CMimeHandlerTest1 **ppCProtHandler);
    STDMETHODIMP OnDataReceived(DWORD *pgrfBSC, DWORD *pcbBytesAvailable, DWORD *pdwTotalSize); 
    
    void SetProtSink(IOInetProtocolSink *pProtSnk)
    {   
        TransAssert((pProtSnk));
        /*
        if (pProtSnk)
        {
            pProtSnk->AddRef();
        }
        if (_pProtSnk)
        {
            _pProtSnk->Release();
        }
        */
        _pProtSnk = pProtSnk;
    }
    void SetDelegate(IUnknown *pUnk, IOInetProtocolSink *pProtSnk)
    {   
        TransAssert((pProtSnk && pUnk));
        /*
        if (pProtSnk)
        {
            pProtSnk->AddRef();
        }
        if (pUnk)
        {
            pUnk->AddRef();
        }
        if (_pProtSnk)
        {
            _pProtSnk->Release();
        }
        if (_pUnk)
        {
            _pUnk->Release();
        }
        */
        _pProtSnk = pProtSnk;
        _pUnk = pUnk;


    }

    STDMETHODIMP Initialize(DWORD dwMode, DWORD dwOptions, IUnknown *pUnk, IOInetProtocol *pProt, IOInetProtocolSink *pProtSnk);
    CMimeHandlerTest1(REFCLSID rclsid, IUnknown *pUnkOuter, IUnknown **ppUnkInner);
    virtual ~CMimeHandlerTest1()
    {
        if (_pProt)
        {
            _pProt->Release();
        }
        if (_pProtSnk)
        {
            _pProtSnk->Release();
        }

        delete [] _pwzFileName;
        delete [] _pwzMimeSuggested;
        delete [] _pBuffer;

    }
    
private:
    CRefCount            _CRefs;        // the total refcount of this object
    CMutexSem            _mxs;              // used in Read, Seek, Abort and package list in case of apartment threaded
    
    IUnknown            *_pUnk;
    IOInetProtocol      *_pProt;            // the prot the filter reads from
    IOInetProtocolSink  *_pProtSnk;         // the prot report progress 
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
    BOOL                _fDelete        : 1;
    LPWSTR              _pwzFileName;
    LPWSTR              _pwzMimeSuggested;

};


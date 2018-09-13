#ifndef _CDLPROT_HXX_
#define _CDLPROT_HXX_

#define MAX_URL_SIZE    INTERNET_MAX_URL_LENGTH

#define CDL_STATE_BIND              4712
#define VALUE_POUND_CHAR            '#'


typedef struct CodeDownloadDataTag CodeDownloadData;

class CCdlProtocol : public CBaseProtocol
{
    public:
        STDMETHODIMP Start(LPCWSTR pwzUrl,
                           IOInetProtocolSink *pIOInetProtocolSink,
                           IOInetBindInfo *pIOInetBindInfo,
                           DWORD grfSTI,
                           DWORD_PTR dwReserved);
    
        STDMETHODIMP Continue(PROTOCOLDATA *pStateInfo);
    
        STDMETHODIMP Abort(HRESULT hrReason, DWORD dwOptions);
    
        STDMETHODIMP Read(void *pv,ULONG cb,ULONG *pcbRead);
    
        STDMETHODIMP Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,
                    ULARGE_INTEGER *plibNewPosition);
    
        STDMETHODIMP LockRequest(DWORD dwOptions);
    
        STDMETHODIMP UnlockRequest();
        
        void SetDataPending(BOOL fPending);

        REFCLSID GetClsid() { return _clsidReport; }

        HRESULT RegisterIUnknown(IUnknown *pUnk);
    
    public:
        CCdlProtocol(REFCLSID rclsid, IUnknown *pUnkOuter, IUnknown **ppUnkInner);
        virtual ~CCdlProtocol();
    
        void ClearCodeDLBSC() { _pCodeDLBSC = NULL; }

    private:
        STDMETHODIMP ParseURL();
        STDMETHODIMP StartDownload(CodeDownloadData &cdldata);
    
    private:
        CCodeDLBSC     *_pCodeDLBSC;
        IUnknown       *_pUnk;
        IBindCtx       *_pbc;
        BOOL           _fDataPending;
        BOOL           _fNotStarted;
        BOOL           _fGetClassObject;
        CLSID          _clsidReport;
        IID            _iid;
};

#endif // _CDLPROT_HXX_

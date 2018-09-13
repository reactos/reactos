#ifndef __CDLBSC_INCLUDED__
#define __CDLBSC_INCLUDED__

class CCdlProtocol;

class CCodeDLBSC : public IBindStatusCallback
                  ,public IServiceProvider
{
    public:
        CCodeDLBSC(IOInetProtocolSink *pIOInetProtocolSink,
                   IOInetBindInfo *pIOInetBindInfo,
                   CCdlProtocol *pCDLProtocol,
                   BOOL fGetClassObject);
        virtual ~CCodeDLBSC();
        HRESULT Abort();

        // IUnknown methods
        STDMETHODIMP QueryInterface( REFIID ridd, void **ppv );
        STDMETHODIMP_( ULONG ) AddRef();
        STDMETHODIMP_( ULONG ) Release();
    
        // IBindStatusCallback methods
        STDMETHODIMP GetBindInfo(DWORD *grfBINDINFOF, BINDINFO *pbindinfo);
        STDMETHODIMP OnStartBinding(DWORD grfBSCOption, IBinding *pib);
        STDMETHODIMP GetPriority(LONG *pnPriority);
        STDMETHODIMP OnProgress(ULONG ulProgress, ULONG ulProgressMax,
                                ULONG ulStatusCode, LPCWSTR szStatusText);
        STDMETHODIMP OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
                                      FORMATETC *pformatetc,
                                      STGMEDIUM *pstgmed);
        STDMETHODIMP OnObjectAvailable(REFIID riid, IUnknown *punk);
        STDMETHODIMP OnLowResource(DWORD dwReserved);
        STDMETHODIMP OnStopBinding(HRESULT hresult, LPCWSTR szError);

        // IServiceProvider
        STDMETHODIMP QueryService(REFGUID rsid, REFIID riid, void ** ppvObj);

    protected:
        IBinding                   *_pIBinding; // ibinding from code dl'er
        IOInetProtocolSink         *_pOInetProtocolSink;
        IOInetBindInfo             *_pIOInetBindInfo;
        CCdlProtocol               *_pCDLProtocol;
        IUnknown                   *_pUnk;
        BOOL                        _fGetClassObject;
        DWORD                       _cRef;

};

#endif

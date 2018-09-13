#ifndef __LOGO3BSC_INCLUDED__
#define __LOGO3BSC_INCLUDED__

#define TIMEOUT_INTERVAL                     1000 // milliseconds

void CALLBACK TimeOutProc(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime);

class Logo3CodeDLBSC : public IBindStatusCallback
{
    public:
        Logo3CodeDLBSC(CSoftDist *pSoftDist, IBindStatusCallback *pClientBSC,
                       LPSTR szCodeBase, LPWSTR wzDistUnit);
        virtual ~Logo3CodeDLBSC();

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

        // Helper Functions
        void SetBindCtx(IBindCtx *pbc);

#ifdef LOGO3_SUPPORT_AUTOINSTALL
        void TimeOut();
#endif

    private:
        STDMETHODIMP RecordPrecacheValue(HRESULT hr);

    protected:
        IBinding                   *_pIBinding; // ibinding from code dl'er
        IBindStatusCallback        *_pClientBSC; 
        IBindCtx                   *_pbc;
        CSoftDist                  *_pSoftDist;
        DWORD                       _cRef;
        LPSTR                       _szCodeBase;
        LPSTR                       _szDistUnit;
        BOOL                        _bPrecacheOnly;
        Cwvt                        _wvt;

#ifdef LOGO3_SUPPORT_AUTOINSTALL
        UINT                        _uiTimer;
        HANDLE                      _hProc;
#endif
};

#endif

#ifndef __CDLBSC_INCLUDED__
#define __CDLBSC_INCLUDED__

class CDLAgentBSC : public IBindStatusCallback, public IServiceProvider,
                    public IInternetHostSecurityManager
{
    public:
        CDLAgentBSC(CCDLAgent *pcdlagent, DWORD dwMaxSizeKB, BOOL fSilentOperation, LPWSTR szCDFURL);
        virtual ~CDLAgentBSC();
        HRESULT Abort();
        HRESULT Pause();
        HRESULT Resume();

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

        // IInternetHostSecurityManager
        STDMETHODIMP GetSecurityId(BYTE *pbSecurityId, DWORD *pcbSecurityId,
                                   DWORD_PTR dwReserved);
        
        STDMETHODIMP ProcessUrlAction(DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy,
                                      BYTE *pContext, DWORD cbContext, DWORD dwFlags,
                                      DWORD dwReserved);
        
        STDMETHODIMP QueryCustomPolicy(REFGUID guidKey, BYTE **ppPolicy,
                                       DWORD *pcbPolicy, BYTE *pContext,
                                       DWORD cbContext, DWORD dwReserved);
    
    
    protected:
        IBinding                   *m_pIBinding; // ibinding from code dl'er
        CCDLAgent                  *m_pCdlAgent;
        DWORD                       m_cRef;
        BOOL                        m_fSilentOperation;
        DWORD                       m_dwMaxSize;
        WCHAR                       m_pwzCDFBase[INTERNET_MAX_URL_LENGTH];
        IInternetSecurityManager   *m_pSecMgr;
};

#endif

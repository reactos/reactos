#ifndef __CDLDELEGATE_INCLUDED__
#define __CDLDELEGATE_INCLUDED__

STDMETHODIMP AsyncDLCodeInstall(CBinding *pCBinding,
                                IBindStatusCallback *pBSC,
                                IBinding **ppIBinding,
                                CCodeDownloadInfo *pCDLInfo);

class CCDLDelegate : public IBindStatusCallback, public IBinding,
                     public IWindowForBindingUI {
    public:
        CCDLDelegate(CBinding *pCBinding, IBindStatusCallback *pBSC);
        virtual ~CCDLDelegate();

        // IUnknown methods
        STDMETHODIMP QueryInterface(REFIID ridd, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();
    
        // IBindStatusCallback methods
        STDMETHODIMP GetBindInfo(DWORD *grfBINDINFOF, BINDINFO *pbindinfo);
        STDMETHODIMP OnStartBinding(DWORD grfBSCOption, IBinding *pib);
        STDMETHODIMP OnProgress(ULONG ulProgress, ULONG ulProgressMax,
                                ULONG ulStatusCode, LPCWSTR szStatusText);
        STDMETHODIMP OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
                                     FORMATETC *pformatetc,
                                     STGMEDIUM *pstgmed);
        STDMETHODIMP OnObjectAvailable(REFIID riid, IUnknown *punk);
        STDMETHODIMP OnLowResource(DWORD dwReserved);
        STDMETHODIMP OnStopBinding(HRESULT hresult, LPCWSTR szError);
        STDMETHODIMP GetPriority(LONG *pnPriority);

        // IBinding methods
        STDMETHODIMP Abort(void);
        STDMETHODIMP Suspend(void);
        STDMETHODIMP Resume(void);
        STDMETHODIMP SetPriority(LONG nPriority);
        STDMETHODIMP GetBindResult(CLSID *pclsidProtocol, DWORD *pdwBindResult,
                                   LPWSTR *pszBindResult, DWORD *dwReserved);

        // IWindowForBindingUI methods
        STDMETHODIMP GetWindow(REFGUID rguidReason, HWND __RPC_FAR *phwnd);

    protected:
        CBinding                   *_pCBinding; // CBinding from urlmon (inital dl)
        IBindStatusCallback        *_pBSC;      // BSC from SHDOCVW
        IBinding                   *_pBinding;  // ibinding from cdl moniker
        DWORD                       _cRef;

};

#endif

#ifndef __SILENT_BINDSTATUS__
#define __SILENT_BINDSTATUS__

#include <urlmki.h>

class CSilentCodeDLSink : public IBindStatusCallback, 
                          public ICodeInstall
{
public:
    CSilentCodeDLSink();
    ~CSilentCodeDLSink();

    // Helper function
    HRESULT WaitTillNotified();
    VOID Abort();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IBindStatusCallback
    STDMETHODIMP OnStartBinding(
        /* [in] */ DWORD grfBSCOption,
        /* [in] */ IBinding *pib);
    STDMETHODIMP GetPriority(
        /* [out] */ LONG *pnPriority);
    STDMETHODIMP OnLowResource(
        /* [in] */ DWORD reserved);
    STDMETHODIMP OnProgress(
        /* [in] */ ULONG ulProgress,
        /* [in] */ ULONG ulProgressMax,
        /* [in] */ ULONG ulStatusCode,
        /* [in] */ LPCWSTR szStatusText);
    STDMETHODIMP OnStopBinding(
        /* [in] */ HRESULT hresult,
        /* [in] */ LPCWSTR szError);
    STDMETHODIMP GetBindInfo(
        /* [out] */ DWORD *grfBINDINFOF,
        /* [unique][out][in] */ BINDINFO *pbindinfo);
    STDMETHODIMP OnDataAvailable(
        /* [in] */ DWORD grfBSCF,
        /* [in] */ DWORD dwSize,
        /* [in] */ FORMATETC *pformatetc,
        /* [in] */ STGMEDIUM *pstgmed);
    STDMETHODIMP OnObjectAvailable(
        /* [in] */ REFIID riid,
        /* [iid_is][in] */ IUnknown *punk);

    // ICodeInstall methods
    STDMETHODIMP GetWindow(
                    REFGUID rguidReason,
        /* [out] */ HWND *phwnd);
    STDMETHODIMP OnCodeInstallProblem(
                   ULONG ulStatusCode, 
                   LPCWSTR szDestination, 
                   LPCWSTR szSource, 
                   DWORD dwReserved);

protected:

    BOOL            m_fAbort;
    DWORD           m_cRef;
    IBinding*       m_pBinding;
    HANDLE			m_hOnStopBindingEvt;	// Handle to manual reset events
};

#endif

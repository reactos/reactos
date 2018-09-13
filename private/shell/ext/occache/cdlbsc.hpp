#ifndef __CDLBSC_INCLUDED__
#define __CDLBSC_INCLUDED__

#include "init.h"
#include <urlmon.h>

#define DOWNLOAD_PROGRESS  0x9001
#define DOWNLOAD_COMPLETE  0x9002

class CodeDownloadBSC : public IBindStatusCallback, public IWindowForBindingUI {
    public:
        CodeDownloadBSC( HWND hwnd, HWND hdlg, LPITEMIDLIST pidlUpdate = NULL );
        virtual ~CodeDownloadBSC();
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

        // IWindowForBindingUI
        STDMETHODIMP GetWindow( REFGUID rguidReason, HWND __RPC_FAR *phwnd ); 

        HWND            _hdlg;          // progress dialog

    protected:
        IBinding        *_pIBinding; // ibinding from code dl'er
        DWORD           _cRef;
        HWND            _hwnd;          // owner window
        LPITEMIDLIST    _pidlUpdate;    // pidl for item we are updating
};

#endif

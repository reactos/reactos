#include <urlmon.h>
#include "cdlbsc.hpp"
#include "resource.h"

CodeDownloadBSC::CodeDownloadBSC( HWND hwnd, HWND hdlg, LPITEMIDLIST pidlUpdate )
{
    _cRef = 1;
    _pIBinding = NULL;
    _hwnd = hwnd;
    _pidlUpdate = pidlUpdate;
    _hdlg = hdlg;  
}

CodeDownloadBSC::~CodeDownloadBSC()
{
    if ( _pidlUpdate )
        ILFree( _pidlUpdate );
}

HRESULT CodeDownloadBSC::Abort()
{
    return _pIBinding->Abort();
}

/*
 *
 * IUnknown Methods
 *
 */

STDMETHODIMP CodeDownloadBSC::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT          hr = E_NOINTERFACE;

    *ppv = NULL;
    if (riid == IID_IUnknown || riid == IID_IBindStatusCallback)
    {
        *ppv = (IBindStatusCallback *)this;
    }
    else if ( riid == IID_IWindowForBindingUI )
    {
         *ppv = (IWindowForBindingUI *)this;
    }

    if (*ppv != NULL)
    {
        ((IUnknown *)*ppv)->AddRef();
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP_(ULONG) CodeDownloadBSC::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CodeDownloadBSC::Release()
{
    if (--_cRef)
    {
        return _cRef;
    }
    delete this;

    return 0;
}

/*
 *
 * IBindStatusCallback Methods
 *
 */

STDMETHODIMP CodeDownloadBSC::OnStartBinding(DWORD grfBSCOption, IBinding *pib)
{
    if (_pIBinding != NULL)
    {
        _pIBinding->Release();
    }
    _pIBinding = pib;

    if (_pIBinding != NULL)
    {
        _pIBinding->AddRef();
    }

    return S_OK;
}

STDMETHODIMP CodeDownloadBSC::OnStopBinding(HRESULT hresult, LPCWSTR szError)
{
    if ( _hdlg != NULL )
        PostMessage(_hdlg, WM_COMMAND, DOWNLOAD_COMPLETE,
                    SUCCEEDED(hresult) ? TRUE : FALSE);

    if ( SUCCEEDED(hresult) && _pidlUpdate )
    {
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST, _pidlUpdate, NULL);
        SHChangeNotifyHandleEvents();
    }
    return S_OK;
}

STDMETHODIMP CodeDownloadBSC::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    if ( _hdlg != NULL )
        PostMessage(_hdlg, WM_COMMAND, DOWNLOAD_COMPLETE, TRUE );
    return S_OK;
}

STDMETHODIMP CodeDownloadBSC::GetPriority(LONG *pnPriority)
{
    return S_OK;
}

STDMETHODIMP CodeDownloadBSC::OnLowResource(DWORD dwReserved)
{
    return S_OK;
}  

STDMETHODIMP CodeDownloadBSC::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
                                    ULONG ulStatusCode,
                                    LPCWSTR szStatusText)
{
    if ( _hdlg != NULL )
    {
        // convert progress to a percentage - 0->100
        LPARAM lprog;
        if ( ulStatusCode == BINDSTATUS_ENDDOWNLOADDATA )
            lprog = 100;
        else
            lprog = (ulProgressMax != 0)? (ulProgress * 100) / ulProgressMax : 0;
        PostMessage(_hdlg, WM_COMMAND, DOWNLOAD_PROGRESS, lprog );
    }

    return S_OK;
}


STDMETHODIMP CodeDownloadBSC::GetBindInfo(DWORD *pgrfBINDF, BINDINFO *pbindInfo)
{
   // *pgrfBINDF |= BINDF_SILENTOPERATION;
    return S_OK;
}

STDMETHODIMP CodeDownloadBSC::OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
                                         FORMATETC *pformatetc,
                                         STGMEDIUM *pstgmed)
{
    return S_OK;
}

STDMETHODIMP CodeDownloadBSC::GetWindow( REFGUID rguidReason, HWND __RPC_FAR *phwnd )
{
    *phwnd = _hwnd;
    return S_OK;
}



#ifndef __DOWNLOAD_H_
#define __DOWNLOAD_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
//
// CDownloadBehavior
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CDownloadBehavior : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDownloadBehavior, &CLSID_CDownloadBehavior>,
    public IDispatchImpl<IDownloadBehavior, &IID_IDownloadBehavior, &LIBID_IEXTagLib>,

    public IElementBehavior
{
public:

    //
    // methods
    //

    CDownloadBehavior ();
    ~CDownloadBehavior ();

    //
    // IElementBehavior
    //

    STDMETHOD(Init)(IElementBehaviorSite *pSite);
    STDMETHOD(Notify)(LONG lEvent, VARIANT * pVarNotify);
    STDMETHOD(Detach)() { return S_OK; };

    //
    // IDownloadBehavior
    //

    STDMETHOD(startDownload)(BSTR bstrUrl, IDispatch * pdispCallback);

    //
    // wiring
    //

DECLARE_REGISTRY_RESOURCEID(IDR_DOWNLOADBEHAVIOR)
DECLARE_NOT_AGGREGATABLE(CDownloadBehavior)

BEGIN_COM_MAP(CDownloadBehavior)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(IDownloadBehavior)
END_COM_MAP()

    //
    // data
    //

    IElementBehaviorSite *  _pSite;
};

/////////////////////////////////////////////////////////////////////////////
//
// CDownload
//
/////////////////////////////////////////////////////////////////////////////

class CDownload : public CBindStatusCallback<CDownloadBehavior>
{
public:

    //
    // methods
    //

    ~CDownload();

    static HRESULT Download(BSTR bstrURL, IDispatch * pdispCallback, IUnknown * pUnkContainer);

    STDMETHOD(OnDataAvailable)(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed);

    void OnFinalDataAvailable (BYTE * pBytes, DWORD dwSize);

    static HRESULT InvokeCallback(LPTSTR pchString, IDispatch * pdispCallback);

    //
    // data
    //

    IDispatch *     _pdispCallback;
};

#endif // __DOWNLOAD_H_

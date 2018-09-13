#include "headers.hxx"

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_TRIAPI_HXX_
#define X_TRIAPI_HXX_
#include "triapi.hxx"
#endif

MtDefine(CTridentAPI, Mem, "CTridentAPI")


HRESULT
InternalShowModalDialog(HWND        hwndParent,
                        IMoniker *  pMk,
                        VARIANT *   pvarArgIn,
                        TCHAR *     pchOptions,
                        VARIANT *   pvarArgOut,
                        IUnknown *  punk,
                        COptionsHolder *pOptionsHolder,
                        DWORD       dwFlags);

CTridentAPI::CTridentAPI()
{
    _ulRefs = 1;
}

STDMETHODIMP CTridentAPI::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr;
    
    if ((IID_ITridentAPI == riid) || (IID_IUnknown == riid))
    {
        *ppvObj = (ITridentAPI *)this;
        AddRef();
        hr = S_OK;
    }
    else
    {
        hr = E_NOINTERFACE;
        *ppvObj = NULL;
    }

    return hr;
}

STDMETHODIMP CTridentAPI::ShowHTMLDialog(
                                         HWND hwndParent,
                                         IMoniker *pMk,
                                         VARIANT *pvarArgIn,
                                         WCHAR *pchOptions,
                                         VARIANT *pvarArgOut,
                                         IUnknown *punkHost
                                         )
{
    return InternalShowModalDialog(hwndParent,
                                   pMk,
                                   pvarArgIn,
                                   pchOptions,
                                   pvarArgOut,
                                   punkHost,
                                   NULL,
                                   0);  // by pasing 0, this is a trusted dialog.  this is correct (carled)
}


//+------------------------------------------------------------------------
//
//  Function:   CreateTridentAPI
//
//  Synopsis:   Creates a new Trident API helper object to avoid lots of
//              pesky exports.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CreateTridentAPI(
        IUnknown * pUnkOuter,
        IUnknown **ppUnk)
{
    HRESULT hr;
    
    if (NULL == pUnkOuter)
    {
        CTridentAPI *pTriAPI = new CTridentAPI;

        *ppUnk = pTriAPI;

        hr = pTriAPI ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        *ppUnk = NULL;
        hr = CLASS_E_NOAGGREGATION;
    }

    return hr;
}



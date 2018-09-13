// ClientCaps.cpp : Implementation of CClientCaps
#include "headers.h"
#include "iextag.h"

#include "utils.hxx"

#include "download.h"

/////////////////////////////////////////////////////////////////////////////
//
// CDownload
//
/////////////////////////////////////////////////////////////////////////////

CDownload::~CDownload()
{
    ReleaseInterface (_pdispCallback);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CDownload::Download(
    BSTR        bstrURL,
    IDispatch * pdispCallback,
    IUnknown *  pUnkContainer)
{
    HRESULT                 hr;
    CComObject<CDownload> * pDownload;

    hr = CComObject<CDownload>::CreateInstance(&pDownload);
    if (hr)
        goto Cleanup;

    pDownload->_pdispCallback = pdispCallback;
    pdispCallback->AddRef();

    pDownload->StartAsyncDownload(NULL, NULL, bstrURL, pUnkContainer, TRUE);
    // hr can now be S_OK, S_ASYNC, or failure

    // ignore hr
#if 0
    if (FAILED(hr))
    {
        hr = InvokeCallback(NULL, pdispCallback);
    }
#endif

Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// the implementation is similar to CBindStatusCallback::OnDataAvailable,
//  except it waits for the last chunk of data and only then calls OnFinalDataAvailable

HRESULT
CDownload::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    HRESULT             hr = S_OK;
    DWORD               dwActuallyRead;
    BYTE *              pBytes = NULL;
    CComPtr<IStream>    pStream;

    ATLTRACE(_T("CBindStatusCallback::OnDataAvailable\n"));

    if (!(BSCF_LASTDATANOTIFICATION & grfBSCF))
        goto Cleanup;

    ATLTRACE(_T("CBindStatusCallback::OnDataAvailable FINAL\n"));

    if (pstgmed->tymed != TYMED_ISTREAM)
        goto Cleanup;

    pStream = pstgmed->pstm;
    if (!pStream)
        goto Cleanup;

    pBytes = new BYTE[dwSize + 1];
    if (pBytes == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pStream->Read(pBytes, dwSize, &dwActuallyRead);
    if (hr)
        goto Cleanup;

    pBytes[dwActuallyRead] = 0;
    if (0 < dwActuallyRead)
    {
        OnFinalDataAvailable(pBytes, dwActuallyRead);
    }

Cleanup:
    delete[] pBytes;

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
CDownload::OnFinalDataAvailable (BYTE * pBytes, DWORD dwSize)
{
    LPTSTR      pchString = NULL;
    ULONG       cchString;

    //
    // get unicode string
    //

    // get length
    cchString = MultiByteToWideChar(CP_ACP, 0, (LPSTR) pBytes, dwSize, NULL, 0);
    if (!cchString)
        goto Cleanup;

    pchString = new TCHAR[cchString + 1];
    if (!pchString)
        goto Cleanup;

    // convert now
    MultiByteToWideChar(CP_ACP, 0, (LPSTR) pBytes, dwSize, pchString, cchString);
    pchString[cchString] = 0;

    //
    // invoke the callback function
    //

    InvokeCallback(pchString, _pdispCallback);

Cleanup:
    ClearInterface (&_pdispCallback);

    delete pchString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT
CDownload::InvokeCallback(LPTSTR pchString, IDispatch * pdispCallback)
{
    HRESULT     hr = S_OK;

    if (pdispCallback)
    {
        VARIANT     varArg;
        DISPPARAMS  dispparams = {&varArg, NULL, 1, 0};
        EXCEPINFO   excepinfo;
        UINT        nArgErr;

        if (pchString)
        {
            V_VT(&varArg) = VT_BSTR;
            V_BSTR(&varArg) = SysAllocString(pchString);
            if (!V_BSTR(&varArg))
                return E_OUTOFMEMORY;
        }
        else
        {
            VariantClear(&varArg);
            V_VT(&varArg) = VT_NULL;
        }

        hr = pdispCallback->Invoke(
            DISPID_VALUE, IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_METHOD, &dispparams, NULL, &excepinfo, &nArgErr);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// CDownloadBehavior
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

CDownloadBehavior::CDownloadBehavior()
{
    memset (this, 0, sizeof(*this));
}

/////////////////////////////////////////////////////////////////////////////

CDownloadBehavior::~CDownloadBehavior()
{
    ReleaseInterface(_pSite);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CDownloadBehavior::Init(IElementBehaviorSite *pSite)
{
    if (!pSite)
        return E_INVALIDARG;

    _pSite = pSite;
    _pSite->AddRef();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CDownloadBehavior::Notify(LONG lEvent, VARIANT * pVarNotify)
{
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CDownloadBehavior::startDownload(BSTR bstrUrl, IDispatch * pdispCallback)
{
    HRESULT hr;

    if (!bstrUrl || !pdispCallback)
        return E_INVALIDARG;

    if (!AccessAllowed(bstrUrl, _pSite))
        return E_ACCESSDENIED;

    hr = CDownload::Download(bstrUrl, pdispCallback, _pSite);

    return hr;
}

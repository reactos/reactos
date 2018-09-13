#include "headers.hxx"

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_EXTDL_HXX_
#define X_EXTDL_HXX_
#include "extdl.hxx"
#endif

MtDefine(CExternalDownload, Utilities, "CExternalDownload")

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Helper:     UnicodeStringFromAnsiStream
//
//+---------------------------------------------------------------------------

HRESULT
UnicodeStringFromAnsiStream(LPTSTR * ppch, IStream * pStream)
{
    HRESULT     hr;
    STATSTG     statstg;
    ULONG       ulLen;
    ULONG       ulLenRead;
    LPSTR       pchAnsi = NULL;
    LPTSTR      pchEnd;
    ULONG       cch;

    Assert (ppch);

    //
    // get stream length
    //

    hr = THR(pStream->Stat(&statstg, STATFLAG_NONAME));
    if (hr)
        goto Cleanup;

    ulLen = statstg.cbSize.LowPart;
    if (statstg.cbSize.HighPart || 0xFFFFFFFF == ulLen)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // read stream into buffer
    //

    pchAnsi = new char[ulLen + 1];
    if (!pchAnsi)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pStream->Read(pchAnsi, ulLen, &ulLenRead));
    if (hr)
        goto Cleanup;
    if (ulLen != ulLenRead)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // convert ANSI to unicode and normalize CRs
    //

    // get length
    cch = MultiByteToWideChar(CP_ACP, 0, pchAnsi, ulLen, NULL, 0);

    (*ppch) = new TCHAR[cch + 1];
    if (!(*ppch))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // convert now
    Verify(cch == (ULONG) MultiByteToWideChar(CP_ACP, 0, pchAnsi, ulLen, (*ppch), cch));

    pchEnd = (*ppch) + cch;
    cch -= NormalizerChar((*ppch), &pchEnd);
    (*ppch)[cch] = _T('\0');


Cleanup:
    delete pchAnsi;

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
// CExternalDownload
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CExternalDownload::CExternalDownload
//
//  Synopsis:   constructor
//
//-------------------------------------------------------------------------

CExternalDownload::CExternalDownload()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CExternalDownload::~CExternalDownload
//
//  Synopsis:   destructor
//
//-------------------------------------------------------------------------

CExternalDownload::~CExternalDownload()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CExternalDownload::Download
//
//  (static)
//
//-------------------------------------------------------------------------

HRESULT
CExternalDownload::Download(
    LPTSTR      pchUrl,
    IDispatch * pdispCallbackFunction,
    CDoc *      pDoc,
    CElement *  pElement)
{
    HRESULT             hr;
    CExternalDownload * pDownload;

    pDownload = new CExternalDownload();
    if (!pDownload)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pDownload->Init(pchUrl, pdispCallbackFunction, pDoc, pElement));

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExternalDownload::Download
//
//-------------------------------------------------------------------------

HRESULT
CExternalDownload::Init (
    LPTSTR      pchUrl,
    IDispatch * pdispCallbackFunction,
    CDoc *      pDoc,
    CElement *  pElement)
{
    HRESULT         hr;
    TCHAR   cBuf[pdlUrlLen];
    LPTSTR          pchExpandedUrl = cBuf;
    CBitsCtx *      pBitsCtx = NULL;

    //
    // general init
    //

    _pDoc = pDoc;
    ReplaceInterface (&_pdispCallbackFunction, pdispCallbackFunction);

    //
    // launch download
    //

    hr = THR(_pDoc->ExpandUrl(pchUrl, ARRAY_SIZE(cBuf), pchExpandedUrl, pElement));
    if (hr)
        goto Cleanup;

    hr = THR(_pDoc->NewDwnCtx(DWNCTX_BITS, pchExpandedUrl, NULL, (CDwnCtx **)&pBitsCtx));
    if (hr)
        goto Cleanup;

    SetBitsCtx(pBitsCtx);


Cleanup:
    if (pBitsCtx)
        pBitsCtx->Release();

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CExternalDownload::Done
//
//-------------------------------------------------------------------------

HRESULT
CExternalDownload::Done ()
{
    SetBitsCtx(NULL);

    ClearInterface (&_pdispCallbackFunction);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CExternalDownload::SetBitsCtx
//
//+---------------------------------------------------------------------------

void
CExternalDownload::SetBitsCtx(CBitsCtx * pBitsCtx)
{
    if (_pBitsCtx)
    {
        _pBitsCtx->Disconnect();
        _pBitsCtx->Release();
    }
    _pBitsCtx = pBitsCtx;

    if (_pBitsCtx)
    {
        _pBitsCtx->AddRef();

        if (_pBitsCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
        {
            OnDwnChan(_pBitsCtx);
        }
        else
        {
            _pBitsCtx->SetProgSink(_pDoc->GetProgSink());
            _pBitsCtx->SetCallback(OnDwnChanCallback, this);
            _pBitsCtx->SelectChanges(DWNCHG_COMPLETE, 0, TRUE);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CExternalDownload::OnDwnChan
//
//+---------------------------------------------------------------------------

void
CExternalDownload::OnDwnChan(CDwnChan * pDwnChan)
{
    HRESULT     hr;
    IStream *   pStream = NULL;
    LPTSTR      pchString = NULL;

    if (_pBitsCtx->GetState() & DWNLOAD_COMPLETE)
    {
        // If unsecure download, may need to remove lock icon on Doc
        if (_pDoc)
        {
            _pDoc->OnSubDownloadSecFlags(_pBitsCtx->GetUrl(), _pBitsCtx->GetSecFlags());
        }
        
        //
        // data downloaded successfully - get it
        //

        hr = THR(_pBitsCtx->GetStream(&pStream));
        if (hr)
            goto Cleanup;

        hr = THR(UnicodeStringFromAnsiStream(&pchString, pStream));
        if (hr)
            goto Cleanup;
    }
    else
        hr = E_FAIL;

    //
    // invoke the callback function
    //

    if (_pdispCallbackFunction)
    {
        VARIANT     varArg;
        DISPPARAMS  dispparams = {&varArg, NULL, 1, 0};
        EXCEPINFO   excepinfo;
        UINT        nArgErr;

        // if the string was downloaded successfully
        if (S_OK == hr)
        {
            // invoke with VT_BSTR and the string
            V_VT(&varArg) = VT_BSTR;
            hr = THR(FormsAllocString (STRVAL(pchString), &V_BSTR(&varArg)));
            if (hr)
                goto Cleanup;
        }
        else
        {
            // otherwise invoke with VT_NULL
            VariantInit(&varArg);
            V_VT(&varArg) = VT_NULL;
        }

        IGNORE_HR(_pdispCallbackFunction->Invoke(
            DISPID_VALUE, IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_METHOD, &dispparams, NULL, &excepinfo, &nArgErr));
    }

    //
    // finalize
    //

    if (_pBitsCtx && (_pBitsCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_STOPPED | DWNLOAD_ERROR)))
    {
        _pBitsCtx->SetProgSink(NULL);
        
        IGNORE_HR(Done());
        delete this;
    }

    // ! here the instance of class is deallocated from memory, be carefull not to use it

Cleanup:
    delete pchString;

    ReleaseInterface(pStream);
}

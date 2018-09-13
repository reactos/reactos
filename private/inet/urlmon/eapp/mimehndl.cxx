

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       mimehndl.cxx
//
//  Contents:   Class that performs the download of a particular request.
//
//  Classes:
//
//  Functions:
//
//  History:    12-04-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <eapp.h>


PerfDbgTag(tagCTransaction,    "Urlmon", "Log CTransaction",        DEB_TRANS)
    DbgTag(tagCTransactionErr, "Urlmon", "Log CTransaction Errors", DEB_TRANS|DEB_ERROR)
   
typedef struct _tagPROTOCOLFILTERDATA
{
    DWORD               cbSize;
    IOInetProtocolSink *pProtocolSink;  // out parameter
    IOInetProtocol     *pProtocol;      // in parameter
    IUnknown           *pUnk;
    DWORD               dwFilterFlags;  
} PROTOCOLFILTERDATA;


CMimeHandlerTest1::CMimeHandlerTest1(REFCLSID rclsid, IUnknown *pUnkOuter, IUnknown **ppUnkInner) : CBaseProtocol(rclsid, pUnkOuter, ppUnkInner)
{
    _pUnk = 0;
    _pProt = 0;
    _pProtSnk = 0;
    _dwMode = 0;
    _dwOInetBdgFlags = 0;
    _pBuffer = 0;           // DNLD_BUFFER_SIZE  size buffer
    _cbBufferSize = 0;
    _cbTotalBytesRead = 0;
    _cbBufferFilled = 0;    //how much of the buffer is in use
    _cbDataSniffMin = 0;
    _cbBytesReported = 0;
    _fDocFile = 0;
    _fMimeVerified = 0;
    _pwzFileName = 0;
    _pwzMimeSuggested = 0;
    _fDelete = 0;

}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::QueryInterface");

    *ppvObj = NULL;

    {
        if (   (riid == IID_IUnknown)
            || (riid == IID_IOInetProtocol))
        {
            *ppvObj = (IOInetProtocol *) this;
            AddRef();
        }
        else if (riid == IID_IOInetProtocolSink)
        {
            *ppvObj = (IOInetProtocolSink *) this;
            AddRef();
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }


    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::QueryInterface (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CMimeHandlerTest1::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeHandlerTest1::AddRef(void)
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::AddRef");

    LONG lRet;
  
    {
        lRet = ++_CRefs;
    }
    
    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::AddRef (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CMimeHandlerTest1::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeHandlerTest1::Release(void)
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::Release");

    LONG lRet;
    {
        lRet = --_CRefs;
        if (_CRefs == 0 && _fDelete)
        {
            delete this;
        }
    }

    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::Release (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::Start
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [pTrans] --
//              [pOIBindInfo] --
//              [grfSTI] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::Start(LPCWSTR pwzUrl, IOInetProtocolSink *pOInetProtSnk, IOInetBindInfo *pOIBindInfo,
                          DWORD grfSTI, DWORD dwReserved)
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::Start\n");
    HRESULT hr = NOERROR;
    PROTOCOLFILTERDATA *pFilterData = 0;

    TransAssert((pOIBindInfo && pOInetProtSnk));
    if (dwReserved)
    {
        pFilterData = (PROTOCOLFILTERDATA *)dwReserved;
    }
    if (pFilterData && pOInetProtSnk)
    {
        TransAssert((pOIBindInfo && pOInetProtSnk));
        _pProt = pFilterData->pProtocol;
        if (_pProt)
        {
            _pProt->AddRef();
        }
        else
        {
            hr = E_FAIL;
        }
        _pProtSnk = pOInetProtSnk;
        _pProtSnk->AddRef();
    }
    else
    {
        hr = E_FAIL;
    }

    PerfDbgLog1(tagCTransaction, this, "+CMimeHandlerTest1::Start (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::Continue
//
//  Synopsis:
//
//  Arguments:  [pStateInfoIn] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::Continue(PROTOCOLDATA *pStateInfoIn)
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::Continue\n");

    HRESULT hr = _pProt->Continue(pStateInfoIn);

    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::Continue (hr:%lx)\n",hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::Abort
//
//  Synopsis:
//
//  Arguments:  [hrReason] --
//              [dwOptions] --
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::Abort(HRESULT hrReason, DWORD dwOptions)
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::Abort\n");
    HRESULT hr = NOERROR;

    hr = _pProt->Abort(hrReason, dwOptions);

    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::Abort (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::Terminate
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::Terminate(DWORD dwOptions)
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::Terminate\n");
    HRESULT hr = NOERROR;

    TransAssert((_pProt));
    //IOInetProtocol *pProt = _pProt;
    
    hr = _pProt->Terminate(dwOptions);
    //pProt->Release();
    //_pProt = 0;

    _pProtSnk->Release();
    _pProtSnk = 0;
    
    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::Terminate (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::Suspend
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::Suspend()
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::Suspend\n");

    HRESULT hr = _pProt->Suspend();

    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::Suspend (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::Resume
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::Resume()
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::Resume\n");

    HRESULT hr = _pProt->Resume();

    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::Resume (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::Read
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//              [ULONG] --
//              [pcbRead] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::Read(void *pBuffer, ULONG cbBuffer,ULONG *pcbRead)
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::Read\n");
    HRESULT     hr = E_FAIL;

    BOOL fRead = TRUE;
    DWORD dwCopy = 0;
    DWORD dwCopyNew = 0;

    if (   (_dwOInetBdgFlags & (PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP))
        && (_cbBufferFilled))
    {
        fRead = FALSE;

        // copy data form the local buffer to the provide buffer
        if (cbBuffer < _cbBufferFilled)
        {
            dwCopy = cbBuffer;
            memcpy(pBuffer, _pBuffer, cbBuffer);
            // move the memory to the front
            memcpy(_pBuffer, _pBuffer + cbBuffer, _cbBufferFilled - cbBuffer);
            _cbBufferFilled -= cbBuffer;
            hr = S_OK;
        }
        else if (cbBuffer == _cbBufferFilled)
        {
            dwCopy = _cbBufferFilled;
            memcpy(pBuffer, _pBuffer, _cbBufferFilled);
            _cbBufferFilled = 0;
            hr = S_OK;
        }
        else
        {
            //
            // user buffer is greater than what is available in
            //
            dwCopy = _cbBufferFilled;
            memcpy(pBuffer, _pBuffer, _cbBufferFilled);
            _cbBufferFilled = 0;
            fRead = TRUE;
            hr = E_PENDING;
        }
    }

    if (fRead)
    {
        if (_pProt)
        {
            hr = _pProt->Read( ((LPBYTE)pBuffer) + dwCopy, cbBuffer - dwCopy, &dwCopyNew);
            _cbTotalBytesRead += dwCopyNew;
        }
        else
        {
            hr = S_FALSE;
        }
    }

    if (pcbRead)
    {
        *pcbRead = dwCopy + dwCopyNew;
    }
 

    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::Read (hr:%lx)\n",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::Seek
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [ULARGE_INTEGER] --
//              [plibNewPosition] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::Seek\n");

    HRESULT hr = _pProt->Seek(dlibMove, dwOrigin, plibNewPosition);

    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::Seek (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::LockRequest
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::LockRequest(DWORD dwOptions)
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::LockRequest\n");

    HRESULT hr = hr = _pProt->LockRequest(dwOptions);

    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::LockRequest (hr:%lx)\n",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::UnlockRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::UnlockRequest()
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::UnlockRequest\n");
    HRESULT hr = NOERROR;

    hr = _pProt->UnlockRequest();

    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::UnlockRequest (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::OnDataReceived
//
//  Synopsis:
//
//  Arguments:  [grfBSC] --
//              [cbBytesAvailable] --
//              [dwTotalSize] --
//              [pcbNewAvailable] --
//
//  Returns:
//
//  History:    4-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
#if 0
STDMETHODIMP CMimeHandlerTest1::OnDataReceived(DWORD *pgrfBSC, DWORD *pcbBytesAvailable, DWORD *pdwTotalSize) //, DWORD *pcbNewAvailable)
{
    PerfDbgLog3(tagCTransaction, this, "+CMimeHandlerTest1::OnDataReceived (grfBSC:%lx,  cbBytesAvailable:%ld, _cbTotalBytesRead:%ld)",
                                    *pgrfBSC, *pcbBytesAvailable, _cbTotalBytesRead);
    HRESULT hr = NOERROR;
    DWORD grfBSC = *pgrfBSC;
    DWORD cbBytesAvailable = *pcbBytesAvailable; 
    DWORD dwTotalSize = *pdwTotalSize;
    DWORD *pcbNewAvailable = &cbBytesAvailable;
    
    *pcbNewAvailable = cbBytesAvailable;

    if (_dwOInetBdgFlags & (PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP))
    {
        DWORD dwNewData = 0;
        TransAssert((_pProt && _cbDataSniffMin));

        // _cbTotalBytesRead = # of bytes read so far
        if (_cbTotalBytesRead < _cbDataSniffMin)
        {
            // no bytes read so far
            TransAssert((_cbTotalBytesRead < _cbDataSniffMin));
            // read data into buffer and report progess
            do
            {
                hr = _pProt->Read(_pBuffer + _cbBufferFilled, _cbBufferSize - _cbBufferFilled, &dwNewData);
                _cbTotalBytesRead += dwNewData;
                _cbBufferFilled += dwNewData;
            } while ((hr == S_OK) && (_cbTotalBytesRead < _cbDataSniffMin));

            // now check if this is docfile
            // if so download at least 2k
            if (!_fDocFile && _cbBufferFilled && (IsDocFile(_pBuffer, _cbBufferFilled) == S_OK))
            {
                _fDocFile = TRUE;
                _cbDataSniffMin =  (dwTotalSize && dwTotalSize < DATASNIFSIZEDOCFILE_MIN) ? dwTotalSize : DATASNIFSIZEDOCFILE_MIN;
            }

            if ((hr == E_PENDING) && (_cbTotalBytesRead < _cbDataSniffMin))
            {
                // do not report anything - wait until we get more data
                // a request is pending at this time
                // need more data to sniff properly
                hr  = S_NEEDMOREDATA;
            }
            else if (hr == NOERROR || hr == E_PENDING)
            {
                TransAssert((_cbTotalBytesRead != 0));

                // report the data we have in the buffer or
                // the available #
                DWORD cbBytesReport =  (cbBytesAvailable > _cbTotalBytesRead) ? cbBytesAvailable : _cbTotalBytesRead + 1;

                if (dwTotalSize && ((cbBytesReport > dwTotalSize)))
                {
                    cbBytesReport =  dwTotalSize;
                }
                *pcbNewAvailable = cbBytesReport;
            }
            else if (hr == S_FALSE)
            {
                // end of stream
                *pgrfBSC |=  (BSCF_LASTDATANOTIFICATION & BSCF_DATAFULLYAVAILABLE);
                *pcbBytesAvailable = *pdwTotalSize =  _cbTotalBytesRead;
            }
            
            if (   (!_fMimeVerified)
                && (   (*pcbNewAvailable >= _cbDataSniffMin)
                    || (hr == S_FALSE)) )
            {
                // enough data or end of stream
                _fMimeVerified = TRUE;
                LPCWSTR  pwzStr = FindMimeFromDataIntern(_pwzFileName,_pBuffer, _cbBufferFilled, _pwzMimeSuggested, 0);
                _pProtSnk->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, pwzStr);
                
                if (_pwzMimeSuggested != pwzStr)
                {
                    if (_pwzMimeSuggested)
                    {
                        delete [] _pwzMimeSuggested;
                    }
                    if (pwzStr)
                    {
                        _pwzMimeSuggested = OLESTRDuplicate((LPWSTR)pwzStr);
                    }
                }

                if (   _fDocFile
                    && (_dwOInetBdgFlags & PI_DOCFILECLSIDLOOKUP))
                {
                    // find the class id and send it on
                    CLSID clsid;

                    HRESULT hr1 = GetClassDocFileBuffer(_pBuffer, _cbBufferFilled, &clsid);
                    if (hr1 == NOERROR)
                    {
                        LPOLESTR pwzStrClsId;
                        StringFromCLSID(clsid, &pwzStrClsId);
                        _pProtSnk->ReportProgress(BINDSTATUS_CLASSIDAVAILABLE, pwzStrClsId);

                        delete [] pwzStrClsId;
                    }
                }
            }
            hr = NOERROR;
        }
        //TransAssert((cbBytesAvailable <= *pcbNewAvailable));
        if (cbBytesAvailable > *pcbNewAvailable)
        {
            *pcbNewAvailable = cbBytesAvailable;
        }
        if (dwTotalSize && (dwTotalSize < *pcbNewAvailable))
        {
            *pcbNewAvailable = dwTotalSize;
        }
    }
    
    {
        CLock lck(_mxs);
        _cbBytesReported = *pcbNewAvailable;
        *pdwTotalSize = dwTotalSize;
    }


    PerfDbgLog2(tagCTransaction, this, "-CMimeHandlerTest1::OnDataReceived (hr:%lx, _cbBufferFilled:%lx)", hr, _cbBufferFilled);
    return hr;
}
#endif // 0
//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::Switch
//
//  Synopsis:
//
//  Arguments:  [pStateInfo] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::Switch(PROTOCOLDATA *pStateInfo)
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::Switch");
    HRESULT hr = NOERROR;

    hr = _pProtSnk->Switch(pStateInfo);
   
    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::Switch (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::ReportProgress
//
//  Synopsis:
//
//  Arguments:  [NotMsg] --
//              [szStatusText] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::ReportProgress(ULONG NotMsg, LPCWSTR pwzStatusText)
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::ReportProgress");
    HRESULT hr = NOERROR;

    switch (NotMsg)
    {
    case BINDSTATUS_MIMETYPEAVAILABLE:
        if (_dwOInetBdgFlags & (PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP))
        {
            // report the mime later after sniffing data
            _pwzMimeSuggested = OLESTRDuplicate(pwzStatusText);
        }
        else
        {
            hr = _pProtSnk->ReportProgress(NotMsg, pwzStatusText);
        }

    break;

    case BINDSTATUS_CACHEFILENAMEAVAILABLE :
        _pwzFileName = OLESTRDuplicate(pwzStatusText);

    default:
    {
        hr = _pProtSnk->ReportProgress(NotMsg, pwzStatusText);
        
    }

    } // end switch
    
    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::ReportProgress (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::ReportData
//
//  Synopsis:
//
//  Arguments:  [grfBSCF] --
//              [ULONG] --
//              [ulProgressMax] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::ReportData(DWORD grfBSCF, ULONG ulProgress,ULONG ulProgressMax)
{
    PerfDbgLog3(tagCTransaction, this, "+CMimeHandlerTest1::ReportData(grfBSCF:%lx, ulProgress:%ld, ulProgressMax:%ld)",
                                       grfBSCF, ulProgress, ulProgressMax);
    HRESULT hr = NOERROR;
    /*
    if (   (_dwOInetBdgFlags & (PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP)
        && (OnDataReceived(&grfBSCF, &ulProgress, &ulProgressMax) == NOERROR)) )
    {
        hr = _pProtSnk->ReportData( grfBSCF, ulProgress, ulProgressMax);
    }
    else
    */
    {
        hr = _pProtSnk->ReportData( grfBSCF, ulProgress, ulProgressMax);
    }

    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::ReportData (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeHandlerTest1::ReportResult
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [dwError] --
//              [wzResult] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMimeHandlerTest1::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult)
{
    PerfDbgLog(tagCTransaction, this, "+CMimeHandlerTest1::ReportResult");
    HRESULT hr = NOERROR;

    hr = _pProtSnk->ReportResult(hrResult, dwError, wzResult);
    
    PerfDbgLog1(tagCTransaction, this, "-CMimeHandlerTest1::ReportResult (hr:%lx)", hr);
    return hr;
}

STDMETHODIMP CMimeHandlerTest1::Initialize(DWORD dwMode, DWORD dwOptions, IUnknown *pUnk, IOInetProtocol *pProt, IOInetProtocolSink *pProtSnk)
{
    HRESULT hr = NOERROR;
    _dwMode = dwMode;
    _pUnk = pUnk;
    _pProt = pProt;
    _pProtSnk = pProtSnk;
    if (_pProtSnk)
    {
        _pProtSnk->AddRef();
    }
    if (_pUnk)
    {
        _pUnk->AddRef();
    }
    
    _dwOInetBdgFlags = dwOptions;
    if (_dwOInetBdgFlags & (PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP))
    {
        _cbBufferSize = DATASNIFSIZEDOCFILE_MIN; 
        _pBuffer = (LPBYTE) new BYTE[_cbBufferSize];

        if (!_pBuffer)
        {
            _cbBufferSize = 0;
            hr = E_OUTOFMEMORY;
        }
        _cbDataSniffMin = _cbBufferSize;
    }
    TransAssert((_pUnk && _pProt && _pProtSnk));
    return hr;
}


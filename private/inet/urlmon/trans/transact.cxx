//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       transact.cxx
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
#include <trans.h>
#include "oinet.hxx"

static CHAR gwzAcceptEncHeaders[] = "Accept-Encoding: gzip, deflate";
extern BOOL g_bHasMimeHandlerForTextHtml;

PerfDbgExtern(tagCTransaction);
    DbgExtern(tagCTransactionErr);

HRESULT GetClassDocFileBuffer(LPVOID pbuffer, DWORD dwSize, CLSID *pclsid);
extern DWORD g_dwSettings;

#if DBG==1

#else
#define USE_NOTIFICATION_EXCEPTION_FILTER   //Not in this lifetime
#endif //


#define szHKSniffFlag   "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
#define szHKSniffValue  "SniffDocFile"

BOOL IsSpecialUrl(WCHAR *pchURL);

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    PerfDbgLog(tagCTransaction, this, "+CTransaction::QueryInterface");

    *ppvObj = NULL;
    hr = _pUnkOuter->QueryInterface(riid, ppvObj);

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::QueryInterface (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CTransaction::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CTransaction::AddRef(void)
{
    LONG lRet = ++_CRefs;
    PerfDbgLog1(tagCTransaction, this, "CTransaction::AddRef (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CTransaction::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CTransaction::Release(void)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::Release");

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        delete this;
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::Release (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::Switch
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
STDMETHODIMP CTransaction::Switch(PROTOCOLDATA *pStateInfo)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::Switch");
    HRESULT hr = NOERROR;

    DWORD grfFlags = pStateInfo->grfFlags;
    AddRef();

    if (IsFreeThreaded())
    {
        if (grfFlags & PD_FORCE_SWITCH)
        {
            CTransPacket * pCTP = new CTransPacket(pStateInfo);

            if (pCTP)
            {
                hr = _pClntProtSink->Switch(pCTP);
            }
        }
        else
        {
            // handle request on this thread
            _pProt->Continue(pStateInfo);
        }
    }
    else
    {
        CTransPacket *pCTP = new CTransPacket(pStateInfo);

        if (pCTP)
        {
            AddCTransPacket(pCTP);

            if ((grfFlags & PI_FORCE_ASYNC) || !IsApartmentThread())
            {
                _cPostedMsg++;
                AddRef();
                /****
                PerfDbgLog4(tagCTransaction, this, "CINet:%lx === PostMessage (Msg:%x) WM_TRANS_PACKET - dwCurrentSize:%ld, dwTotalSize:%ld",
                    _pProt, XDBG(++_wTotalPostedMsg,0), pCTP->_dwCurrentSize, pCTP->_dwTotalSize);
                ****/
                PostMessage(_hwndNotify, WM_TRANS_PACKET, (WPARAM) (WPARAM)GetTotalPostedMsgId(), (LPARAM)this);
            }
            else
            {
                OnINetCallback();
            }
        }
    }
    Release();

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::Switch (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::ReportProgress
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
STDMETHODIMP CTransaction::ReportProgress(ULONG NotMsg, LPCWSTR szStatusText)
{
    AddRef();
    PerfDbgLog(tagCTransaction, this, "+CTransaction::ReportProgress");
    HRESULT hr = NOERROR;

    if (PreDispatch() != S_OK)
    {
        // nothing to do any more!
    }
    else if (IsFreeThreaded())
    {
        // handle request on this thread
        hr = DispatchReport((BINDSTATUS) NotMsg, _grfBSCF, _ulCurrentSize, _ulTotalSize, (LPWSTR)szStatusText, 0);
    }
    else
    {
        CTransPacket *pCTP = new CTransPacket( (BINDSTATUS) NotMsg, NOERROR, szStatusText, _ulCurrentSize, _ulTotalSize);

        if (pCTP)
        {
            //BUGBUG: this is a hack where small doc files are loaded in one swipe and
            //the class install filter is not loaded because an BINDSTATUS_ENDDOWNLOAD
            //already occurs in transaction packet list.
            if (NotMsg == BINDSTATUS_CLASSINSTALLLOCATION)
            {
                AddCTransPacket(pCTP, FALSE);
            }
            else
            {
                AddCTransPacket(pCTP);
            }

            if (!IsApartmentThread())
            {
                _cPostedMsg++;
                AddRef();
                PerfDbgLog4(tagCTransaction, this, "CINet:%lx === PostMessage (Msg:%x) WM_TRANS_PACKET - dwCurrentSize:%ld, dwTotalSize:%ld",
                                _pProt, XDBG(++_wTotalPostedMsg,0), pCTP->_dwCurrentSize, pCTP->_dwTotalSize);
                PostMessage(_hwndNotify, WM_TRANS_PACKET, (WPARAM)GetTotalPostedMsgId(), (LPARAM)this);

            }
            else
            {
                OnINetCallback();
            }
        }
    }

    PostDispatch();

    Release();
    PerfDbgLog1(tagCTransaction, this, "-CTransaction::ReportProgress (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::ReportData
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
STDMETHODIMP CTransaction::ReportData(DWORD grfBSCF, ULONG ulProgress,ULONG ulProgressMax)
{
    PerfDbgLog3(tagCTransaction, this, "+CTransaction::ReportData(grfBSCF:%lx, ulProgress:%ld, ulProgressMax:%ld)",
                                       grfBSCF, ulProgress, ulProgressMax);

    HRESULT hr = NOERROR;
    BOOL fAsync = FALSE;

    BINDSTATUS bndStatus = BINDSTATUS_DOWNLOADINGDATA;
    AddRef();

    if (   (PreDispatch() == S_OK)
        && (   (_ulCurrentSize <= ulProgress)
             || (    (grfBSCF & BSCF_LASTDATANOTIFICATION)
                 && !(_grfBSCF & BSCF_LASTDATANOTIFICATION))
             || (grfBSCF & BSCF_ASYNCDATANOTIFICATION)
            ))
    {

        if (grfBSCF & BSCF_FIRSTDATANOTIFICATION)
        {
            bndStatus = BINDSTATUS_BEGINDOWNLOADDATA;
            _grfBSCF |= BSCF_FIRSTDATANOTIFICATION;
        }

        if (grfBSCF & BSCF_LASTDATANOTIFICATION)
        {
            bndStatus = BINDSTATUS_ENDDOWNLOADDATA;
            _grfBSCF |= BSCF_LASTDATANOTIFICATION;
        }

        if (grfBSCF & BSCF_DATAFULLYAVAILABLE)
        {
            _grfBSCF |= BSCF_DATAFULLYAVAILABLE;
        }

        if (grfBSCF & BSCF_ASYNCDATANOTIFICATION)
        {
            fAsync = TRUE;
        }

        _ulCurrentSize = ulProgress;
        _ulTotalSize  = ulProgressMax;
        TransAssert((   (_ulTotalSize == 0)
                     || (_ulCurrentSize <= _ulTotalSize) ));

        if (IsFreeThreaded())
        {
            hr = DispatchReport(bndStatus, _grfBSCF, _ulCurrentSize, _ulTotalSize, 0, 0);
        }
        else
        {
            CTransPacket *pCTP = new CTransPacket(bndStatus, NOERROR, NULL, _ulCurrentSize, _ulTotalSize);

            if (pCTP)
            {
                AddCTransPacket(pCTP);

                if (!IsApartmentThread() || fAsync)
                {
                    _cPostedMsg++;
                    AddRef();
                    PerfDbgLog4(tagCTransaction, this, "CINet:%lx === PostMessage (Msg:%x) WM_TRANS_PACKET - dwCurrentSize:%ld, dwTotalSize:%ld",
                                    _pProt, XDBG(++_wTotalPostedMsg,0), pCTP->_dwCurrentSize, pCTP->_dwTotalSize);
                    PostMessage(_hwndNotify, WM_TRANS_PACKET, (WPARAM) (WPARAM)GetTotalPostedMsgId(), (LPARAM)this);

                }
                else
                {
                    OnINetCallback();
                }
            }
        }
    }

    PostDispatch();

    Release();

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::ReportData (hr:%lx)", hr);
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
STDMETHODIMP CTransaction::OnDataReceived(DWORD *pgrfBSC, DWORD *pcbBytesAvailable, DWORD *pdwTotalSize) //, DWORD *pcbNewAvailable)
{
    PerfDbgLog3(tagCTransaction, this, "+CTransaction::OnDataReceived (grfBSC:%lx,  cbBytesAvailable:%ld, _cbTotalBytesRead:%ld)",
                                    *pgrfBSC, *pcbBytesAvailable, _cbTotalBytesRead);
    HRESULT hr = NOERROR;
    DWORD grfBSC = *pgrfBSC;
    DWORD cbBytesAvailable = *pcbBytesAvailable;
    DWORD dwTotalSize = *pdwTotalSize;
    DWORD *pcbNewAvailable = &cbBytesAvailable;

    *pcbNewAvailable = cbBytesAvailable;

    do
    {
        // check if mimeverification was requested
        //
        if (!(_dwOInetBdgFlags & (PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP)) )
        {
            break;
        }
        // get the buffer
        //
        if (!_pBuffer)
        {
            _cbBufferSize = DATASNIFSIZEDOCFILE_MIN; //DATASNIFSIZE_MIN;
            _pBuffer = (LPBYTE) new BYTE[_cbBufferSize];
        }
        if (!_pBuffer)
        {
            hr = E_OUTOFMEMORY;
            break;
        }


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
                LPWSTR  pwzStr = 0;
                FindMimeFromData(NULL, _pwzFileName,_pBuffer, _cbBufferFilled, _pwzMimeSuggested, 0, &pwzStr, 0);

                if (pwzStr)
                {
                    _pClntProtSink->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, pwzStr);
                } 
                else
                {
                    TransAssert((!_pwzMimeSuggested));
                }

                if (pwzStr)
                {
                    delete [] _pwzMimeSuggested;
                    _pwzMimeSuggested = pwzStr;
                    pwzStr = 0;
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
                        if (pwzStrClsId)
                        {
                           _pClntProtSink->ReportProgress(BINDSTATUS_CLASSIDAVAILABLE, pwzStrClsId);
                        }
                        delete [] pwzStrClsId;
                    }
                }
                delete [] pwzStr;

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
    
        break;
    } while (TRUE);

    {
        CLock lck(_mxs);
        _cbBytesReported = *pcbNewAvailable;
        *pdwTotalSize = dwTotalSize;
    }

    PerfDbgLog2(tagCTransaction, this, "-CTransaction::OnDataReceived (hr:%lx, _cbBufferFilled:%lx)", hr, _cbBufferFilled);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::ReportResult
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
STDMETHODIMP CTransaction::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult)
{
    PerfDbgLog3(tagCTransaction, this, "+CTransaction::ReportResult [hr:%lX, dwError;%lX (%ld)]", hrResult, dwError, dwError);
    HRESULT hr = NOERROR;

    AddRef();

    BOOL fReport = FALSE;
    
    {   //BEGIN SYNC BLOCK
        CLock lck(_mxsBind);
        if (!_fResultReported)
        {
            _fResultReported = TRUE;
            fReport = TRUE;
        }
    }   // END SYNC BLOCK

    if (fReport)
    {
        _hrResult = hrResult;
        _dwResult = dwError;
        _pwzResult = OLESTRDuplicate((LPWSTR)wzResult);

        if (IsFreeThreaded())
        {

            // handle request on this thread
            hr = DispatchReport(BINDSTATUS_RESULT, _grfBSCF, _ulCurrentSize, _ulTotalSize, _pwzResult, _dwResult, _hrResult);
        }
        else
        {
            CTransPacket *pCTP = new CTransPacket( (BINDSTATUS) ((_hrResult == NOERROR) ? BINDSTATUS_RESULT : BINDSTATUS_ERROR),
                                                    _hrResult, _pwzResult, _ulCurrentSize, _ulTotalSize, _dwResult);

            if (pCTP)
            {
                AddCTransPacket(pCTP);

                if (!IsApartmentThread() || _fForceAsyncReportResult )
                {
                    _cPostedMsg++;
                    AddRef();
                    PerfDbgLog4(tagCTransaction, this, "CINet:%lx === PostMessage (Msg:%x) WM_TRANS_PACKET - dwCurrentSize:%ld, dwTotalSize:%ld",
                                    _pProt, XDBG(++_wTotalPostedMsg,0), pCTP->_dwCurrentSize, pCTP->_dwTotalSize);
                    PostMessage(_hwndNotify, WM_TRANS_PACKET, (WPARAM) (WPARAM)GetTotalPostedMsgId(), (LPARAM)this);

                }
                else
                {
                    OnINetCallback();
                }
            }
        }
    }
    else
    {
        // should not happen with our protocols
        hr = E_FAIL;
    }
    
    Release();

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::ReportResult (hr:%lx)", hr);
    return hr;
}


// protocolinfo methods

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::Authenticate
//
//  Synopsis:
//
//  Arguments:  [phwnd] --
//              [LPWSTR] --
//              [pszPassword] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::Authenticate(HWND* phwnd, LPWSTR *pszUsername,LPWSTR *pszPassword)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::Authenticate");
    HRESULT hr = E_FAIL;
    IAuthenticate *pBasicAuth = 0;

    AddRef();
    hr = QueryService(IID_IAuthenticate,IID_IAuthenticate, (void **) &pBasicAuth);

    if ((hr == NOERROR) && pBasicAuth)
    {
         hr = pBasicAuth->Authenticate(phwnd, pszUsername,pszPassword);
    }
    else
    {
        UrlMkAssert((pBasicAuth == NULL));
        *phwnd = 0;
        *pszUsername = 0;
        *pszPassword = 0;
    }
    Release();

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::Authenticate (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::QueryOption
//
//  Synopsis:
//
//  Arguments:  [dwOption] --
//              [pBuffer] --
//              [pcbBuf] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::QueryOption(DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::QueryOption");
    HRESULT hr = E_FAIL;
    if (_pProt)
    {
        if (!_pInetInfo)
        {
            hr = _pProt->QueryInterface(IID_IWinInetInfo, (void **) &_pInetInfo);
            TransAssert(( (hr == NOERROR && _pInetInfo) || (hr != NOERROR && !_pInetInfo) ));
            if ((hr == NOERROR) && _pUnkInner)
            {
                Release();
            }

        }

        if (_pInetInfo)
        {

            hr = _pInetInfo->QueryOption(dwOption, pBuffer, pcbBuf);
        }
    }
    PerfDbgLog1(tagCTransaction, this, "-CTransaction::QueryOption (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::QueryInfo
//
//  Synopsis:
//
//  Arguments:  [dwOption] --
//              [pBuffer] --
//              [pcbBuf] --
//              [pdwFlags] --
//              [pdwReserved] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::QueryInfo(DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf, DWORD *pdwFlags, DWORD *pdwReserved)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::QueryInfo");
    HRESULT hr = E_FAIL;
    if (_pProt)
    {
        if (!_pInetHttpInfo)
        {
            hr = _pProt->QueryInterface(IID_IWinInetHttpInfo, (void **) &_pInetHttpInfo);
            TransAssert(( (hr == NOERROR && _pInetHttpInfo) || (hr != NOERROR && !_pInetHttpInfo) ));
            if ((hr == NOERROR) && _pUnkInner)
            {
                Release();
            }

        }
        if (_pInetHttpInfo)
        {
            hr = _pInetHttpInfo->QueryInfo(dwOption, pBuffer, pcbBuf, pdwFlags, pdwReserved);
        }
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::QueryInfo (hr:%lx)", hr);
    return hr;
}

//IOInetBindInfo methods
//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::GetBindInfo
//
//  Synopsis:
//
//  Arguments:  [pdwBINDF] --
//              [pbindinfo] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::GetBindInfo(DWORD *pdwBINDF, BINDINFO *pbindinfo)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::GetBindInfo");
    HRESULT hr = NOERROR;

    TransAssert((pdwBINDF && pbindinfo));

    TransAssert((_pClntBindInfo));
    hr = _pClntBindInfo->GetBindInfo(pdwBINDF, pbindinfo);

    if (SUCCEEDED(hr))
    {
        PerfDbgLog(tagCTransaction, this, "---CTrans::BINDF_FROMURLMON---");
        *pdwBINDF |= BINDF_FROMURLMON;
    }

    // never do a post on redirect
    if (_pwzRedirectUrl)
    {
        if( pbindinfo && (pbindinfo->dwBindVerb == BINDVERB_POST) )
        {
            pbindinfo->dwBindVerb = BINDVERB_GET;
        }
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::GetBindInfo (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::GetBindString
//
//  Synopsis:
//
//  Arguments:  [ulStringType] --
//              [ppwzStr] --
//              [cEl] --
//              [pcElFetched] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::GetBindString(ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::GetBindString");
    HRESULT hr = INET_E_USE_DEFAULT_SETTING;

    // we handles the encoding here
   
    if (    _fEncodingHandlerEnabled  
         && ulStringType == BINDSTRING_ACCEPT_ENCODINGS
         && ppwzStr
         && cEl )
    {
        LPWSTR pwzAcpHeaders = NULL;
        pwzAcpHeaders = DupA2W(gwzAcceptEncHeaders);
        
        if( pwzAcpHeaders ) 
        {
            *ppwzStr =  pwzAcpHeaders;
            *pcElFetched = 1;
            hr = NOERROR;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            *pcElFetched = 0;
        }
    }
    else
    {

        TransAssert((_pClntBindInfo));
    
        hr = _pClntBindInfo->GetBindString(ulStringType, ppwzStr, cEl, pcElFetched);

        PerfDbgLog1(tagCTransaction, this, "-CTransaction::GetBindString (hr:%lx)", hr);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::QueryService
//
//  Synopsis:
//
//  Arguments:  [rsid] --
//              [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT IUnknown_QueryService(IUnknown* punk, REFGUID rsid, REFIID riid, void ** ppvObj);

HRESULT CTransaction::QueryService(REFGUID rsid, REFIID riid, void ** ppvObj)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::QueryService");
    HRESULT     hr = NOERROR;
    VDATETHIS(this);
    TransAssert((ppvObj));

    hr = IUnknown_QueryService(_pClntProtSink, rsid, riid, ppvObj);

    TransAssert(( ((hr == E_NOINTERFACE) && !*ppvObj)  || ((hr == NOERROR) && *ppvObj) ));

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::QueryService (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::CTransaction
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CTransaction::CTransaction(DWORD grfFlags, LPBYTE pByte, ULONG cbSizeBuffer, IUnknown *pUnkOuter)
                          :  _CRefs(), _cPacketsInList(0), _cPostedMsg(0)
                          ,  _CProtEmbed(), _CProtEncoding(), _COInetProtPost(), _CProtClsInstaller()
{
    _flags = 0;

    if (!pUnkOuter)
    {
        pUnkOuter = &_Unknown;
    }
    _pUnkOuter = pUnkOuter;


    _pCTransNext = NULL;
    _pClntProtSink = NULL;
    _hwndNotify = NULL;
    _pCTransMgr = GetThreadTransactionMgr();
    _State = TransSt_None;
    _cBdgRefs = 0;
    _pCTPHead = NULL;
    _pCTPTail = NULL;
    _pCTPCur = NULL;
    _fDispatch = FALSE;
    _ThreadTransferState = TS_None;
    _fResultReported = FALSE;
    _fTerminated = FALSE;
    _fTerminating = FALSE;
    _fResultDispatched = FALSE;
    _fResultReceived = TRUE;
    _hrResult = NOERROR;
    _dwResult = 0;
    _dwPacketsTotal = 0;
    _grfInternalFlags = 0;
    _pProt = NULL;
    _clsidProtocol = CLSID_NULL;
    _pBndInfo = NULL;
    _pInetInfo = NULL;
    _pInetHttpInfo = NULL;
    _pBndCtx = NULL;
    _dwThreadId = GetCurrentThreadId();
    _dwProcessId = GetCurrentProcessId();
    _grfBSCF = 0;
    _pUnkInner = 0;
    _dwOInetBdgFlags = grfFlags;

    _pBuffer = pByte;                   // DNLD_BUFFER_SIZE  size buffer
    _cbBufferSize  = cbSizeBuffer;
    _cbTotalBytesRead = 0 ;
    _cbBufferFilled = 0;                //how much of the buffer is in use
    _pwzUrl = 0;
    _pwzRedirectUrl = 0;
    _dwTerminateOptions = 0;
    _cbDataSniffMin = DATASNIFSIZE_MIN;
    _fDocFile = FALSE;
    _fMimeVerified = FALSE;
    _fAttached = FALSE;
    _fLocked = FALSE;
    _fModalLoopRunning = FALSE;
    _fDispatchData = FALSE;
    _fMimeHandlerEnabled = TRUE;
    _fEncodingHandlerEnabled = TRUE;
    _fClsInstallerHandlerEnabled = TRUE;
    _fMimeHandlerLoaded = FALSE;
    _pwzFileName = 0;
    _pwzMimeSuggested = 0; 
    _fProtEmbed = TRUE;
    _fStarting = FALSE;
    _fReceivedAbort = FALSE;
    _fReceivedTerminate = FALSE;
    _hrAbort = NOERROR;
    _dwAbort = 0;
    _dwDispatchLevel = 0;
    _fReceivedTerminate = FALSE;
    _fForceAsyncReportResult = FALSE;
    _dwTerminateOptions = 0;

    
#if DBG==1
    _wTotalPostedMsg = 0;
#endif
    _pClntBindInfo = 0;

     _nPriority = THREAD_PRIORITY_NORMAL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::~CTransaction
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CTransaction::~CTransaction()
{
    PerfDbgLog1(tagCTransaction, this, "+CTransaction::~CTransaction [Url:%ws]", _pwzUrl);

    if (_fLocked && _pProt)
    {
        _pProt->UnlockRequest();
    }
        
    if (_pBuffer)
    {
        delete [] _pBuffer;
    }

    if (_pwzRedirectUrl)
    {
        delete [] _pwzRedirectUrl;
    }

    if (_pwzUrl)
    {
        delete [] _pwzUrl;
    }

    if (_pwzFileName)
    {
        delete [] _pwzFileName;
    }

    if (_pwzMimeSuggested)
    {
        delete [] _pwzMimeSuggested;
    }

    if (_pwzProtClsId)
    {
        delete [] _pwzProtClsId;
    }

    if (_pwzResult)
    {
        delete [] _pwzResult;
    }

    // Remove ourselves from the global internet transaction list.
    // We should always be in the list.
    if (_pCTransMgr)
    {
        _pCTransMgr->RemoveTransaction(this);
        _pCTransMgr = NULL;
    }

    // Release any leftover packets

    while (_pCTPHead)
    {
        CTransPacket * pCTP = _pCTPHead;
        _pCTPHead = pCTP->GetNext();
        delete pCTP;
    }

    if (_pBndCtx)
    {
        _pBndCtx->Release();
    }

    if (_pClntBindInfo)
    {
        _pClntBindInfo->Release();
        _pClntBindInfo = 0;
    }

    if (_pClntProtSink)
    {
        _pClntProtSink->Release();
    }

    if (_pProt && !_pUnkInner)
    {
        _pProt->Release();
    }
    _pProt = NULL;

    if (_pInetInfo && !_pUnkInner)
    {
        _pInetInfo->Release();
    }
    _pInetInfo = NULL;

    if (_pInetHttpInfo && !_pUnkInner)
    {
        _pInetHttpInfo->Release();
    }
    _pInetHttpInfo = NULL;

    if (_pUnkInner)
    {
        PerfDbgLog1(tagCTransaction, this, "+CTransaction::~CTransaction release pUnkInner (pCINet:%lx)", _pProt);
        _pUnkInner->Release();
        _pUnkInner = NULL;
    }

    if (_pBndInfo)
    {
        //  BINDINFO_FIX(laszlog) 8-18-96

#if DBG == 1
        if (_pBndInfo->stgmedData.tymed != TYMED_NULL)
        {
            PerfDbgLog1(tagCTransaction, this, "+CTransaction::~CTransaction ReleaseStgMedium (%lx)", _pBndInfo->stgmedData);
        }
#endif

        ReleaseBindInfo(_pBndInfo);

        delete _pBndInfo;
    }

    PerfDbgLog(tagCTransaction, this, "-CTransaction::~CTransaction");
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::Create
//
//  Synopsis:   Creates and initializes a new transaction object
//
//  Arguments:  [pCBdg] --
//              [fConvertData] --
//              [ppCTrans] --
//
//  Returns:
//
//  History:    12-07-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::Create(IBindCtx *pBC, DWORD grfFlags, IUnknown *pUnkOuter, IUnknown **ppUnk, CTransaction **ppCTrans)
{
    PerfDbgLog(tagCTransaction, NULL, "+CTransaction::Create");

    HRESULT hr = NOERROR;
    TransAssert((ppCTrans != NULL));

    // Create the object
    CTransaction *pCTrans = new CTransaction(grfFlags, NULL, 0, pUnkOuter);

    if (pCTrans)
    {
        // notification window is needed for apartment threaded case
        if  (    (grfFlags & OIBDG_APARTMENTTHREADED )
             &&  (pCTrans->GetNotificationWnd() == NULL))
        {
            delete pCTrans;
            *ppCTrans = NULL;
            hr = E_FAIL;
        }
        else
        {
            // set the cbinding assosiated with it
            pCTrans->SetState(TransSt_Initialized);
            *ppCTrans = pCTrans;
        }

        {
            // pCTrans has refcount of 1 now
            // get the pUnkInner; pUnkInner does not addref pUnkOuter
            if (pUnkOuter && ppUnk && pCTrans)
            {
                *ppUnk = pCTrans->GetIUnkInner();
                // addref the outer object since releasing pCINet will go cause a release on pUnkOuter
                PProtAssert((*ppUnk));
            }
        }

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    PerfDbgLog1(tagCTransaction, NULL, "-CTransaction::Create (hr:%lx)", hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::GetTransBindInfo
//
//  Synopsis:   Get the transaction bindinfo
//              Called to pass bindinfo on in IBSC::GetBindInfo and
//              also called by CINet
//
//  Arguments:
//
//  Returns:
//
//  History:    1-16-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BINDINFO *CTransaction::GetTransBindInfo()
{

    if (_pBndInfo == NULL)
    {
        _pBndInfo = new BINDINFO;

        if (_pBndInfo)
        {
            _pBndInfo->cbSize = sizeof(BINDINFO);
            _pBndInfo->szExtraInfo = 0;
            _pBndInfo->grfBindInfoF = 0;
        }
    }
    else
    {
        _pBndInfo->cbSize = sizeof(BINDINFO);
    }

    return _pBndInfo;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::RestartOperation
//
//  Synopsis:   Starts the asycn transaction operation
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-07-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::RestartOperation(LPWSTR pwzURL, DWORD dwCase)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::RestartOperation");

    HRESULT     hr = NOERROR;
    VDATETHIS(this);
    COInetSession *pCOInetSession = 0;
    IOInetProtocol *pProt = 0;
    
    AddRef();

    do
    {

        if (GetState() == TransSt_OperationFinished)
        {
            hr = E_FAIL;
            break;
        }
        
        //
        // case: Start of Transaction
        //

        //TransAssert((_pClntProtSink == NULL));
        BOOL fFirstCreated = FALSE;

        if (dwCase != 0x00000001)
        {
            hr = E_INVALIDARG;
            break;
        }

        // We must have at least one node in the client request linked list
        TransAssert((_pClntProtSink != NULL));
        TransAssert((_pUnkInner == 0));
        TransAssert((_pProt));

        hr = QueryService(IID_IOInetProtocol, IID_IOInetProtocol ,(void **)&pProt);

        if (hr != NOERROR)
        {
            if ((hr = GetCOInetSession(0,&pCOInetSession,0)) == NOERROR)
            {
                hr = pCOInetSession->CreateFirstProtocol(pwzURL, (IOInetBindInfo *) this, &_pUnkInner, &pProt, &_clsidProtocol);
                fFirstCreated = TRUE;
                if ((hr == NOERROR) && _pUnkInner)
                {
                    TransAssert((pProt));
                    pProt->Release();
                }
            }
        }
        else
        {
            // bugbug: find the correct cls id here.
            _clsidProtocol = CLSID_FtpProtocol;
        }

        if (hr != NOERROR)
        {
            TransAssert((!pProt && !_pUnkInner));
            pProt = 0;
            _pUnkInner = 0;
            break;
        }

        BOOL fNext;
        TransAssert((pProt));
        do  // loop over protocols
        {
            fNext = FALSE;
            TransAssert((hr == NOERROR));
            
            // Start the download operation
            TransAssert((pProt != NULL));
 
            {
                delete [] _pwzProtClsId;
                _pwzProtClsId =  0;

                HRESULT hr1 = StringFromCLSID(_clsidProtocol, &_pwzProtClsId);
                if (SUCCEEDED(hr1))
                {
                    _pClntProtSink->ReportProgress(BINDSTATUS_PROTOCOLCLASSID, _pwzProtClsId);
                }
            }

            SetState(TransSt_OperationStarted);


            if (_fProtEmbed)
            {
                _CProtEmbed.SetProtocol(pProt);
            }
            else
            {
                _pProt = pProt;
            }
            _fResultReported = FALSE;

            hr = pProt->Start(pwzURL, this, (IOInetBindInfo *)this, 0,0);

            if (hr == E_PENDING)
            {
                hr = NOERROR;
            }
            else if (hr == INET_E_USE_DEFAULT_PROTOCOLHANDLER)
            {
                fNext = TRUE;

                if (!_pUnkInner)
                {
                    pProt->Release();
                }

                if( _fProtEmbed )
                {
                    _CProtEmbed.SetProtocol(NULL);
                }

                pProt = 0;
                if (_pUnkInner)
                {
                    _pUnkInner->Release();
                    _pUnkInner = 0;
                }

                // bugbug: need to reset the protocol inside the embed protocol handler
                

                if (!fFirstCreated)
                {
                    hr = pCOInetSession->CreateFirstProtocol(pwzURL, (IOInetBindInfo *) this, &_pUnkInner, &pProt, &_clsidProtocol);
                    fFirstCreated = TRUE;
                }
                else
                {
                    hr = pCOInetSession->CreateNextProtocol(pwzURL, (IOInetBindInfo *) this, &_pUnkInner, &pProt, &_clsidProtocol);
                }

                if (hr != NOERROR)
                {
                    TransAssert((!pProt && !_pUnkInner));
                    pProt = 0;
                    _pUnkInner = 0;
                    fNext = FALSE;

                }
                else if (_pUnkInner)
                {
                    // release the extra addref - aggregation
                    Release();
                }
            }
            else if (hr != NOERROR)
            {
                // do not allow pending packages be dispatched
                // any more
                fNext = FALSE;
                _fDispatch = TRUE;
                
                if (pProt)
                {
                    if( hr == INET_E_REDIRECT_TO_DIR && _pwzRedirectUrl)
                    {
                        _pClntProtSink->ReportResult(hr, 0, _pwzRedirectUrl);
                    }
                    else
                    {
                        _pClntProtSink->ReportResult(_hrResult, _dwResult, 0);
                    }
                    _fResultDispatched = TRUE;
                    Terminate(0);
                }
            }

        } while  (fNext == TRUE);

        if (   (_dwOInetBdgFlags & PI_SYNCHRONOUS)
            && SUCCEEDED(hr) 
            && (_fModalLoopRunning == FALSE))
        {
            // complet the binding in case of sychronous bind
            TransAssert((_dwOInetBdgFlags & OIBDG_APARTMENTTHREADED));
            _fModalLoopRunning = TRUE;
            hr = CompleteOperation(_dwOInetBdgFlags & BDGFLAGS_ATTACHED);
            _fModalLoopRunning = FALSE;
        }
  
        break;
    } while (TRUE);

    if (pCOInetSession)
    {
        pCOInetSession->Release();
    }

    Release();

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::RestartOperation (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::Redirect
//
//  Synopsis:   creates a new cinet object
//
//  Arguments:  [szUrl] --
//
//  Returns:
//
//  History:    7-17-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::Redirect(LPWSTR pwzUrl)
{
    PerfDbgLog1(tagCTransaction, this, "+CTransaction::Redirect (szUrlL:%ws)", pwzUrl);
    HRESULT hr = NOERROR;

    TransAssert((pwzUrl != NULL));
    CLSID clsid;

    // Check to see if a non-"special" URL is redirecting to a "special" URL
    //
    if (!IsSpecialUrl(_pwzUrl)         // The original URL
        && IsSpecialUrl(pwzUrl))       // The redirected URL
    {
        // Check the registry workaround
        //
        static DWORD bAllowRedirectToScript = 2;

        if (bAllowRedirectToScript == 2)
        {
            // Read the key and set the static
            BOOL fDefault = FALSE;
            DWORD dwSize = sizeof(DWORD);
            
            SHRegGetUSValue(
                    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
                    "AllowRedirectToScript", 
                    NULL, 
                    (LPBYTE) &bAllowRedirectToScript, 
                    &dwSize, 
                    FALSE, 
                    (LPVOID) &fDefault, 
                    sizeof(fDefault));
        }

        // If it's not allowed, abort the transaction
        //
        if (!bAllowRedirectToScript)
        {
            // Can't redirect to a scripting URL.
            //
            hr = Abort(E_ACCESSDENIED, 0);
            return E_ACCESSDENIED;
        }
    }


    {
        COInetSession *pCOInetSession = 0;
        if ((hr = GetCOInetSession(0,&pCOInetSession,0)) == NOERROR)
        {
            hr = pCOInetSession->FindOInetProtocolClsID(pwzUrl, &clsid);
            pCOInetSession->Release();
        }
    }

    if (hr == NOERROR)
    {
        //
        // remove remaining packages from the queue
        //
        {
            CTransPacket *pCTP = 0;

            while ((pCTP = GetNextCTransPacket()) != NULL)
            {
                // delete the data now
                delete pCTP;
            }
        }

        IOInetProtocol *pCINetOld = _pProt;

        if (_pInetInfo && !_pUnkInner)
        {
            _pInetInfo->Release();
        }
        _pInetInfo = NULL;

        if (_pInetHttpInfo && !_pUnkInner)
        {
            _pInetHttpInfo->Release();
        }
        _pInetHttpInfo = NULL;

        //
        // no post on redirect
        if (_pBndInfo && (_pBndInfo->dwBindVerb == BINDVERB_POST))
        {
            _pBndInfo->dwBindVerb = BINDVERB_GET;
        }
        if (_fProtEmbed)
        {
            IOInetProtocol *pProt = 0;

            hr = _CProtEmbed.GetProtocol(&pProt);
            if (hr == NOERROR)
            {
                pProt->Terminate(0);
                pProt->Release();
            }
            _CProtEmbed.SetProtocol(0);
            if (_pUnkInner)
            {
                //PerfDbgLog2(tagCTransaction, this, "+CTransaction::~CTransaction Nulling (pCINet:%lx, hServer:%lx)", _pProt, _pProt->_hServer);
                _pUnkInner->Release();
                _pUnkInner = NULL;
            }
        }
        else
        {
            _pProt->Terminate(0);

            if (_pProt && !_pUnkInner)
            {
                //PerfDbgLog2(tagCTransaction, this, "+CTransaction::~CTransaction Nulling (pCINet:%lx, hServer:%lx)", _pProt, _pProt->_hServer);
                _pProt->Release();
            }
            _pProt = NULL;

            if (_pUnkInner)
            {
                //PerfDbgLog2(tagCTransaction, this, "+CTransaction::~CTransaction Nulling (pCINet:%lx, hServer:%lx)", _pProt, _pProt->_hServer);
                _pUnkInner->Release();
                _pUnkInner = NULL;
            }
        }
        
        SetState(TransSt_Initialized);
        hr = RestartOperation(pwzUrl,0x00000001);
         
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::Redirect(hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::AddClientOInet
//
//  Synopsis:   Adds clients OInet interfaces.
//
//  Arguments:  [pCBdg] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::AddClientOInet(IOInetProtocolSink *pOInetProtSink, IOInetBindInfo *pOInetBindInfo)
{
    PerfDbgLog2(tagCTransaction, this, "+CTransaction::AddClientOInet, pProtocolSink:%lx, pBindInfo:%lx", pOInetProtSink, pOInetBindInfo);

    TransAssert((pOInetProtSink && pOInetBindInfo));
    
    RemoveClientOInet();

    if (_fProtEmbed)
    {
        _CProtEmbed.SetProtocolSink(pOInetProtSink);
        _pClntProtSink = &_CProtEmbed;
        _cBdgRefs++;
    }
    else
    {
        _cBdgRefs++;
        _pClntProtSink = pOInetProtSink;
        pOInetProtSink->AddRef();
    }
    
    _pClntBindInfo = pOInetBindInfo;
    pOInetBindInfo->AddRef();

    PerfDbgLog(tagCTransaction, this, "-CTransaction::AddClientOInet");
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::RemoveClientOInet
//
//  Synopsis:   Removes OInetProtocolSink and OInetBindInfo interfaces. 
//
//  Arguments:  
//
//  Returns:
//
//  History:    2-02-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::RemoveClientOInet()
{
    PerfDbgLog1(tagCTransaction, this, "+CTransaction::RemoveClientOInet _pClntProtSink:%lx", _pClntProtSink);
    HRESULT hr = NOERROR;
    BOOL fRelease = FALSE;

    if (_fProtEmbed && _cBdgRefs)
    {
        _CProtEmbed.SetProtocolSink(0);
        _CProtEmbed.SetServiceProvider(0);
        _pClntProtSink = 0;
        _cBdgRefs--;
    }
    else if (_pClntProtSink)
    {
        _pClntProtSink->Release();
        _pClntProtSink = 0;
        _cBdgRefs--;
        fRelease = TRUE;
    }
    
    if (_pClntBindInfo)
    {
        _pClntBindInfo->Release();
        _pClntBindInfo = 0;
    }

    TransAssert((_pClntProtSink == 0 && _cBdgRefs == 0));

    hr = (fRelease) ? S_OK : S_FALSE;

    PerfDbgLog2(tagCTransaction, this, "-CTransaction::RemoveClientOInet (_pClntProtSink:%lx, hr:%lx)", _pClntProtSink, hr);
    return hr;
}

#if 0
//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::RemoveAllCBindings
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-02-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::RemoveAllCBindings()
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::RemoveAllCBindings");
    HRESULT hr = NOERROR;
    CBinding *pCBdg;
    CBinding *pCBdgNext;

    TransAssert((_pClntProtSink == 0 &&    _cBdgRefs == 0));
    /*
    
    TransAssert((   (_cBdgRefs == 0 && _pClntProtSink == NULL)
                 || (_cBdgRefs != 0 && _pClntProtSink != NULL) ));

    if (_pClntProtSink)
    {
        _pClntProtSink->Release();
        _pClntProtSink = 0;
        _cBdgRefs--;

    }

    // the list should be empty now
    DbgLog1(tagCTransaction, this, "=== CTransaction::RemoveAllCBindings Removing transaction, cRefs:%ld", _cBdgRefs);
    TransAssert((_cBdgRefs == 0));
    TransAssert((_pCTransMgr));

    // remove the transaction if no cbindings left
    //_pCTransMgr->RemoveTransaction(this);
    //_pCTransMgr = NULL;

    _pClntProtSink = NULL;
    */
    
    PerfDbgLog(tagCTransaction, this, "-CTransaction::RemoveAllCBindings");
    return hr;
}
#endif // 0
//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::MyPeekMessage
//
//  Synopsis:   This function is called whenever we want to do a PeekMessage.
//              It has special handling for WM_QUIT messages.
//
//  Arguments:  [pMsg] - message structure
//              [hWnd] - window to peek on
//              [min/max] - min and max message numbers
//              [wFlag] - peek flags
//
//  Returns:    TRUE  - a message is available
//              FALSE - no messages available
//
//  Algorithm:
//
//  History:    7-26-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::MyPeekMessage(MSG *pMsg, HWND hwnd, UINT min, UINT max, WORD wFlag)
{
    HRESULT hr = S_OK;
    BOOL fRet = PeekMessage(pMsg, hwnd, min, max, wFlag);
    if (fRet)
    {
        PerfDbgLog3(tagCTransaction, this, "MyPeekMessage: hwnd:%x, msg:%x time:%x", pMsg->hwnd, pMsg->message, pMsg->time);
        if (pMsg->message == WM_QUIT)
        {
            PostQuitMessage((int)pMsg->wParam);
            hr  = S_FALSE;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::CompleteOperation
//
//  Synopsis:   Simple modal loop
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-13-95   JohannP (Johann Posch)   Created
//
//  Notes:      BUGBUG: NOT COMPLETE YET!
//
//----------------------------------------------------------------------------
HRESULT CTransaction::CompleteOperation(BOOL fNested)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::CompleteOperation");

    MSG msg;
    HRESULT hr = NO_ERROR;

    CUrlMkTls tls(hr);
    LONG cDispatchLevel;
    BOOL fDispatch = _fDispatch;

    if (hr == NOERROR)
    {
        HRESULT hr1 = NOERROR;
        HRESULT hrPeek;

        CModalLoop MsgFlter(&hr1);

        AddRef();
        cDispatchLevel = tls->cDispatchLevel;

        TransAssert((cDispatchLevel >= 0));
        tls->cDispatchLevel++;

        if (fNested)
        {
            fDispatch = _fDispatch;
            _fDispatch = FALSE;
        }

        // run the modal loop in case we have a IMessageFilter
        if (hr1 == NOERROR)
        {
            DWORD dwWakeReason = WAIT_TIMEOUT;
            DWORD dwInput = QS_ALLINPUT;
            DWORD dwWaitTime = 1000;
            DWORD dwIBSCLevel = (tls->cDispatchLevel > 1)
                                    ? IBSCLEVEL_TOPLEVEL : IBSCLEVEL_NESTED;

            while (GetState() != TransSt_OperationFinished)
            {
                dwWakeReason = MsgWaitForMultipleObjects(0, 0, FALSE, dwWaitTime, dwInput);

                if (   (dwWakeReason == (WAIT_OBJECT_0 + 0))
                    || (dwWakeReason == WAIT_TIMEOUT))
                {
                    DWORD dwStatus = GetQueueStatus(QS_ALLINPUT);


                    // some messages are in the queue
                    if (dwStatus)
                    {

                        while ((hrPeek = MyPeekMessage(&msg, _hwndNotify, WM_USER, WM_TRANS_LAST, PM_REMOVE | PM_NOYIELD)) == S_OK)
                        {
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                        if (hrPeek == S_FALSE)
                        {
                            hr  = S_FALSE;
                            goto End;
                        }
                        // call MessagePending on
                        MsgFlter.HandlePendingMessage(dwStatus,cDispatchLevel,0);

                    }
                }
            }
        }
        else
        {
            // just run a dispatch loop for our own messages
            while (GetState() != TransSt_OperationFinished)
            {
                // wake up every 5 seconds
                // this is needed since our msg might be dispatched somewhere else
                // on the stack inside the protocol
                DWORD dwWaitTime = 5000;
                DWORD dwWakeReason = MsgWaitForMultipleObjects(0, 0, FALSE, dwWaitTime, QS_ALLINPUT);


                if (dwWakeReason == (WAIT_OBJECT_0 + 0))
                {
                    while ((hrPeek = MyPeekMessage(&msg, _hwndNotify, WM_USER, WM_TRANS_LAST, PM_REMOVE | PM_NOYIELD)) == S_OK)
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                    if (hrPeek == S_FALSE)
                    {
                        hr  = S_FALSE;
                        goto End;
                    }
                }
                else if (dwWakeReason == WAIT_TIMEOUT)
                {
                    // nothing to do here
                }
            }
        }

        hr = GetHResult();

        //
        // dispatch all the other notification messages
        //
        while ((hrPeek = MyPeekMessage(&msg, _hwndNotify, WM_USER, WM_TRANS_LAST, PM_REMOVE | PM_NOYIELD)) == S_OK)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    End:
        if (fNested)
        {
            _fDispatch = fDispatch;
        }

        // reset the dispatch level
        tls->cDispatchLevel--;
        Release();
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::CompleteOperation (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::PrepareThreadTransfer
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::PrepareThreadTransfer()
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::PrepareThreadTransfer");
    TransAssert(( IsApartmentThread() ));
    TransAssert((_ThreadTransferState == TS_None));

    HRESULT hr = E_OUTOFMEMORY;

    if (   _ThreadTransferState == TS_None
        && _pCTPCur)
    {
        // make a copy of the current packet
        // this packet will be send to the new
        // thread once the transfer completed

        _pCTPTransfer = new CTransPacket( (BINDSTATUS) BINDSTATUS_INTERNAL);

        if (_pCTPTransfer)
        {
            hr = NOERROR;
            *_pCTPTransfer = *_pCTPCur;

            _ThreadTransferState = TS_Prepared;

            if (_pProt)
            {
                IOInetThreadSwitch *pOInetThS;
                HRESULT hr1 = _pProt->QueryInterface(IID_IOInetThreadSwitch,(void **) &pOInetThS);

                if (hr1 == NOERROR)
                {
                    TransAssert((pOInetThS));
                    pOInetThS->Prepare();
                    pOInetThS->Release();
                }
            }
        }
    }

    PerfDbgLog(tagCTransaction, this, "-CTransaction::PrepareThreadTransfer");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::ThreadTransfer
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::ThreadTransfer()
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::ThreadTransfer");
    TransAssert(( !IsApartmentThread() ));
    TransAssert((_ThreadTransferState == TS_Prepared));

    HRESULT hr = NOERROR;

    if (_ThreadTransferState == TS_Prepared)
    {
        _hwndNotify = GetThreadNotificationWnd();

        // check the threadID and set the new window
        _dwThreadId = GetCurrentThreadId();

        _ThreadTransferState = TS_Completed;
        TransAssert((_pCTPTransfer));
        if (_pCTPTransfer)
        {
            CTransPacket *pCTP = _pCTPTransfer;
            _pCTPTransfer = NULL;

            AddCTransPacket(pCTP, FALSE);
            {
                _cPostedMsg++;
                AddRef();
                PerfDbgLog4(tagCTransaction, this, "CINet:%lx === CTransaction::ThreadTransfer (Msg:%x) WM_TRANS_PACKET - dwCurrentSize:%ld, dwTotalSize:%ld",
                    _pProt, XDBG(++_wTotalPostedMsg,0), pCTP->_dwCurrentSize, pCTP->_dwTotalSize);
                #if DBG==1
                PostMessage(_hwndNotify, WM_TRANS_PACKET, (WPARAM)_wTotalPostedMsg, (LPARAM)this);
                #else
                PostMessage(_hwndNotify, WM_TRANS_PACKET, (WPARAM) 0, (LPARAM)this);
                #endif

            }
            {
                IOInetThreadSwitch *pOInetThS;
                HRESULT hr1 =_pProt->QueryInterface(IID_IOInetThreadSwitch,(void **) &pOInetThS);

                if (hr1 == NOERROR)
                {
                    TransAssert((pOInetThS));
                    pOInetThS->Continue();
                    pOInetThS->Release();
                }
            }
        }
    }

    PerfDbgLog(tagCTransaction, this, "-CTransaction::ThreadTransfer");
    return hr;
}


#ifdef UNUSED
//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::TransmitPaket
//
//  Synopsis:
//
//  Arguments:  [uiMsg] --
//              [pdld] --
//
//  Returns:
//
//  History:    12-08-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CTransaction::TransmitPacket(BINDSTATUS NMsg, CINet * pCINet, LPCWSTR szStr,DWORD cbAvailable, DWORD cbTotal)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::TransmitPacket");

    HRESULT hr = NOERROR;

    _dwPacketsTotal++;

    CTransPacket *pCTP = new CTransPacket(NMsg, NOERROR, szStr,cbAvailable,cbTotal);

    if (pCTP)
    {
        if (cbAvailable)
        {
            pCTP->_dwCurrentSize = cbAvailable;
        }
        if (cbTotal)
        {
            pCTP->_dwTotalSize = cbTotal;
        }

        AddCTransPacket(pCTP);

        if ( pCTP->IsLastNotMsg() || !IsApartmentThread() ||  pCTP->IsAsyncNotMsg() )
        {
            _cPostedMsg++;
            AddRef();
            PerfDbgLog4(tagCTransaction, this, "CINet:%lx === CTransaction::TransmitPacket (Msg:%x) WM_TRANS_PACKET - dwCurrentSize:%ld, dwTotalSize:%ld",
                _pProt, XDBG(++_wTotalPostedMsg,0), pCTP->_dwCurrentSize, pCTP->_dwTotalSize);
            #if DBG==1
            PostMessage(_hwndNotify, WM_TRANS_PACKET, (WPARAM)_wTotalPostedMsg, (LPARAM)this);
            #else
            PostMessage(_hwndNotify, WM_TRANS_PACKET, (WPARAM) 0, (LPARAM)this);
            #endif

        }
        else
        {
            OnINetCallback();
        }

    }
    else
    {
        // post message indicating the packet could not be allocated
        PostMessage(_hwndNotify, WM_TRANS_OUTOFMEMORY, (WPARAM)NULL, (LPARAM)this);
        hr = E_OUTOFMEMORY;
    }

    PerfDbgLog(tagCTransaction, this, "-CTransaction::TransmitPacket");
    return hr;
}
#endif //UNUSED

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::DispatchReport
//
//  Synopsis:
//
//  Arguments:  [NotMsg] --
//              [grfBSCF] --
//              [dwCurrentSize] --
//              [dwTotalSize] --
//              [pwzStr] --
//              [dwError] --
//              [hrReport] --
//
//  Returns:
//
//  History:    4-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::DispatchReport(BINDSTATUS NotMsg, DWORD grfBSCF, DWORD dwCurrentSize, DWORD dwTotalSize, LPCWSTR pwzStr, DWORD dwError, HRESULT hresult)
                                               
{
    PerfDbgLog1(tagCTransaction, this, "+CTransaction::DispatchReport [NotMsg:%lx]", NotMsg);
    HRESULT hr = NOERROR;
    IOInetProtocolSink *pCBdg = _pClntProtSink;
    static const WCHAR *wzClsStr = L"Class Install Handler ";

    if (_fTerminated || !_pClntProtSink)
    {
        // nothing to do
        if (   !_fTerminated
            && (   (BINDSTATUS_RESULT == NotMsg)
                || (BINDSTATUS_ERROR == NotMsg)) )
        {
           _fResultDispatched =  TRUE;
           Terminate(0);
        }
    }
    else
    {
        // clean out the old transfer state
        //
        if (_ThreadTransferState == TS_Completed)
        {
            _ThreadTransferState = TS_None;
        }

        switch (NotMsg)
        {
        case BINDSTATUS_BEGINDOWNLOADDATA:
            if (_dwOInetBdgFlags & PI_DATAPROGRESS)
            {
                pCBdg->ReportProgress(NotMsg, pwzStr);
            }
        case BINDSTATUS_DOWNLOADINGDATA:
        case BINDSTATUS_ENDDOWNLOADDATA:
        {
            //
            // check amount of data an verify mime is requested
            //
            if (OnDataReceived(&grfBSCF, &dwCurrentSize, &dwTotalSize) == NOERROR)
            {
                if (   (NotMsg == BINDSTATUS_ENDDOWNLOADDATA)
                    && (_dwOInetBdgFlags & PI_DATAPROGRESS))
                {
                    pCBdg->ReportProgress(NotMsg, pwzStr);
                }
                hr = pCBdg->ReportData(grfBSCF, dwCurrentSize, dwTotalSize);
            }
        }
        break;
        case BINDSTATUS_REDIRECTING     :
        {
            // report the progress on the redirect url
            TransAssert((pwzStr));
            hr = pCBdg->ReportProgress(NotMsg, pwzStr);
            SetRedirectUrl((LPWSTR)pwzStr);
        }
        break;
        case BINDSTATUS_RESULT:
        case BINDSTATUS_ERROR:
        {
            if (   (hresult == INET_E_REDIRECTING)
                && (pwzStr))
            {
                // Note: it is legal here NOT to have a redirect url
                // if wininet does redirct and it fails it  will reprot this error.
                
                // report progress on redirect and to the redirect
                hr = pCBdg->ReportProgress(BINDSTATUS_REDIRECTING, pwzStr);
                SetRedirectUrl((LPWSTR)pwzStr);
                hr = Redirect((LPWSTR)pwzStr);
                if (   (hr != NOERROR) 
                    && (hr != E_PENDING)
                    && !_fResultDispatched)
                {
                    _fResultDispatched = TRUE;
                    hr = pCBdg->ReportResult(hr, 0, 0);
                }
            }
            else
            {
                _fResultDispatched = TRUE;
                hr = pCBdg->ReportResult(_hrResult, _dwResult, pwzStr);
            }
            
        }
        break;

        case BINDSTATUS_ENCODING:
        {
            /****
            // load the encode filter here
            TransAssert((pwzStr));

            if (_fEncodingHandlerEnabled && pwzStr)
            {
                  // disable data sniff on _CProtEmbed
                  DWORD dwEmbedBndFlags = _CProtEmbed.GetOInetBindFlags();
                  dwEmbedBndFlags &= (~PI_MIMEVERIFICATION & ~PI_DOCFILECLSIDLOOKUP & ~PI_CLASSINSTALL);
                  _CProtEmbed.SetOInetBindFlags(dwEmbedBndFlags);
                 _CProtEncoding.Initialize(this, 0, PP_PRE_SWITCH, _dwOInetBdgFlags, 0, _pProt, _pClntProtSink, 0);
                //hr = LoadHandler(pwzStr, &_CProtEncoding, 0);
                LoadHandler(pwzStr, &_CProtEncoding, 0);
            }
            ***/
        }
        break;

        case BINDSTATUS_CLASSINSTALLLOCATION:
            TransAssert((pwzStr));
            if (_fClsInstallerHandlerEnabled && pwzStr)
            {
                _CProtClsInstaller.Initialize(this, 0, PP_PRE_SWITCH, _dwOInetBdgFlags, 0, _pProt, _pClntProtSink, 0);
                LPWSTR pwzClsURL = 0;
                
                pwzClsURL = new WCHAR[lstrlenW(_pwzUrl) + lstrlenW(pwzStr) + lstrlenW(wzClsStr) + 2]; // +1 for NULL, +1 for another NULL after _pwzUrl

                if (pwzClsURL)
                {
                    StrCpyW(pwzClsURL, wzClsStr);

                    StrCatW(pwzClsURL, _pwzUrl);
                    StrCatW(pwzClsURL, L" ");
                    StrCatW(pwzClsURL, pwzStr);

                    pwzClsURL[lstrlenW(wzClsStr) - 1] = L'\0';
                    pwzClsURL[lstrlenW(_pwzUrl) + lstrlenW(wzClsStr)] = L'\0';
                    
                    hr = LoadHandler(pwzClsURL, &_CProtClsInstaller, 0);
                
                    delete [] pwzClsURL;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            // tell sink to stop waiting on handler (since we're broken)
            if (FAILED(hr))
            {   
                pCBdg->ReportProgress(BINDSTATUS_ENDDOWNLOADCOMPONENTS,NULL);   
            }

        break;

        case BINDSTATUS_CLASSIDAVAILABLE:
            TransAssert((pwzStr));
            hr = pCBdg->ReportProgress(NotMsg, pwzStr);
        break;
        
        case BINDSTATUS_MIMETYPEAVAILABLE:
            TransAssert((pwzStr));
            if ( _fMimeHandlerEnabled && pwzStr && _fProtEmbed && 
                 !_fMimeHandlerLoaded)
            {
                //
                // load a mime filter with the embedded prot class
                if( StrCmpNIW( pwzStr, L"text/html", 9) )
                {
                    hr = LoadHandler(pwzStr, &_CProtEmbed, 0);
                    if (hr == NOERROR)
                    {
                        pCBdg->ReportProgress(BINDSTATUS_LOADINGMIMEHANDLER, NULL);
                        _fMimeHandlerLoaded = TRUE;
                    }
                }
                else
                {
                    //
                    // special treatment for text/html to speed up the
                    // main IE download path 
                    //
                    if(g_bHasMimeHandlerForTextHtml)
                    {
                        hr = LoadHandler(pwzStr, &_CProtEmbed, 0);
                        if (hr == NOERROR)
                        {
                            pCBdg->ReportProgress(BINDSTATUS_LOADINGMIMEHANDLER, NULL);
                            _fMimeHandlerLoaded = TRUE;
                        }
                        else
                        {
                            g_bHasMimeHandlerForTextHtml = FALSE;
                        }
                    }
                    
                }

            }

            if (_dwOInetBdgFlags & (PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP))
            {
                // report the mime later after sniffing data
                _pwzMimeSuggested = OLESTRDuplicate((LPWSTR)pwzStr);
            }
            else
            {
                hr = pCBdg->ReportProgress(NotMsg, pwzStr);
            }

        break;

        case BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE:
        {
            TransAssert((pwzStr));
            if (_fMimeHandlerEnabled && pwzStr && _fProtEmbed && !_fMimeHandlerLoaded)
            {
                // load a mime filter with the embedded prot class
                hr = LoadHandler(pwzStr, &_CProtEmbed, 0);
                if (hr == NOERROR)
                {
                    pCBdg->ReportProgress(BINDSTATUS_LOADINGMIMEHANDLER, NULL);
                    _fMimeHandlerLoaded = TRUE;
                }

            }

            // disable datasniff on _CProtEmbed so we are able to pass through
            DWORD dwEmbedBndFlags = _CProtEmbed.GetOInetBindFlags();
            dwEmbedBndFlags &= (~PI_MIMEVERIFICATION &~PI_DOCFILECLSIDLOOKUP &~PI_CLASSINSTALL);
            _CProtEmbed.SetOInetBindFlags(dwEmbedBndFlags);

            // the mime filter may already updated the mime type
            // if we are already verified, then ignore this one
            if( _fMimeHandlerLoaded && _fMimeVerified && _pwzMimeSuggested )
            {
                hr = pCBdg->ReportProgress( 
                    BINDSTATUS_MIMETYPEAVAILABLE, _pwzMimeSuggested);
            }
            else
            {
                _pwzMimeSuggested = OLESTRDuplicate((LPWSTR)pwzStr);
                _fMimeVerified = TRUE;

                // we should only report BINDSTATUS_MIMETYPEAVAILABLE 
                // (BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE stops at here)
                hr = pCBdg->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, pwzStr);
            }
        }    
        break;

        case BINDSTATUS_CACHEFILENAMEAVAILABLE :
            TransAssert((pwzStr));
            _pwzFileName = OLESTRDuplicate((LPWSTR)pwzStr);

        default:
        {
            hr = pCBdg->ReportProgress(NotMsg, pwzStr);
        }

        } // end switch
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::DispatchReport (hr:%lx)", hr);
    return hr;
}

STDMETHODIMP CTransaction::PreDispatch()
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::PreDispatch");
    HRESULT hr = S_OK;

    {   // single access block
        CLock lck(_mxsBind);
        if (_fTerminated || _fTerminating)
        {
            hr = S_FALSE;
        }
        else
        {
            _dwDispatchLevel++;
        }
    }
    
    PerfDbgLog1(tagCTransaction, this, "-CTransaction::PreDispatch (hr:%lx)", hr);
    return hr;
}

STDMETHODIMP CTransaction::PostDispatch()
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::PostDispatch");
    HRESULT hr = NOERROR;
    BOOL fCallTerminate = FALSE;


    {   // single access block
        CLock lck(_mxsBind);
        _dwDispatchLevel--;
        fCallTerminate = _fReceivedTerminate;
        _fReceivedTerminate = FALSE;
    }

    if (fCallTerminate)
    {
        Terminate(_dwTerminateOptions);
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::PostDispatch (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::DispatchPacket
//
//  Synopsis:
//
//  Arguments:  [pCTPIn] --
//
//  Returns:
//
//  History:    2-02-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::DispatchPacket(CTransPacket *pCTPIn)
{
    PerfDbgLog2(tagCTransaction, this, "+CTransaction::DispatchPacket [pCTPIn:%lx, NotMsg:%lx]", pCTPIn, pCTPIn->GetNotMsg());
    HRESULT hr = NOERROR;
    
    // the packet should be in the list
    TransAssert((_fDispatch == TRUE));
    TransAssert((_pCTPCur == NULL && pCTPIn));
    _pCTPCur = pCTPIn;

    hr = DispatchReport(pCTPIn->GetNotMsg(), _grfBSCF, pCTPIn->_dwCurrentSize, pCTPIn->_dwTotalSize, pCTPIn->_pwzStr, pCTPIn->_dwResult, pCTPIn->_hrResult);

    _pCTPCur = NULL;

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::DispatchPacket (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::OnINetCallback
//
//  Synopsis:
//
//  Arguments:  [pCTPIn] --
//
//  Returns:
//
//  History:    12-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::OnINetCallback(BOOL fFromMsgQueue)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::OnINetCallback");
    HRESULT hrRelease = NOERROR;
    CTransPacket *pCTP = NULL;

    if (fFromMsgQueue)
    {
        _cPostedMsg--;
    }

    // the packet should be in the list
    if (   IsApartmentThread()
        && (_fDispatch == FALSE)
        && GotCTransPacket() )
    {
        _fDispatch = TRUE;
        BOOL fDispatch = (_ThreadTransferState == TS_None) || (_ThreadTransferState == TS_Completed);

        ThreadSwitchState TSState = _ThreadTransferState;

        // multiple packets migth be dispatched -
        // therefor the list of packets might be empty

        while (   fDispatch
               && ((pCTP = GetNextCTransPacket()) != NULL))
        {
            if (pCTP->GetNotMsg() == BINDSTATUS_INTERNAL)
            {
                OnINetInternalCallback(pCTP);
            }
            else
            {
                DispatchPacket(pCTP);
            }

            if (   (TSState == TS_None)
                && (_ThreadTransferState == TS_Prepared))
            {
                // do not dispatch any further packages on this thread
                // wait until the transfer completes
                fDispatch = FALSE;
            }

            // delete the data now
            delete pCTP;
        }

        _fDispatch = FALSE;
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::OnINetCallback (hr:%lx)", hrRelease);
    return hrRelease;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::OnINetInternalCallback
//
//  Synopsis:
//
//  Arguments:  [dwState] --
//              [fFromMsgQueue] --
//
//  Returns:
//
//  History:    3-05-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::OnINetInternalCallback(CTransPacket *pCTPIn)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::OnINetInternalCallback");
    HRESULT hr = NOERROR;
    TransAssert((IsApartmentThread()));
    TransAssert((_pProt));

    _cPostedMsg--;
    _pProt->Continue(pCTPIn);

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::OnINetInternalCallback (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   UrlMonInvokeExceptionFilter
//
//  Synopsis:
//
//  Arguments:  [lCode] --
//              [lpep] --
//
//  Returns:
//
//  History:    2-14-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LONG UrlMonInvokeExceptionFilter( DWORD lCode, LPEXCEPTION_POINTERS lpep )
{
#if DBG == 1
    DbgLog2(tagCTransactionErr, NULL, "Exception 0x%x at address 0x%x",
               lCode, lpep->ExceptionRecord->ExceptionAddress);
    DebugBreak();
#endif

    return EXCEPTION_EXECUTE_HANDLER;
}

//+---------------------------------------------------------------------------
//
//  Function:   TransactionWndProc
//
//  Synopsis:   the transaction callback function
//
//  Arguments:  [hWnd] --
//              [WPARAM] --
//              [wParam] --
//              [lParam] --
//
//  Returns:
//
//  History:    12-02-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LRESULT CALLBACK TransactionWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if ((msg >= WM_TRANS_FIRST && msg <= WM_TRANS_LAST))
    {
        switch (msg)
        {
        case WM_TRANS_PACKET:
        {
           DWORD dwFault;
           CTransaction  *pCTrans = (CTransaction *) lParam;
#ifdef WITH_EXCEPTION
           _try
#endif //WITH_EXCEPTION
            {
                TransAssert((pCTrans != NULL));
                PerfDbgLog1(tagCTransaction, pCTrans, "+CTransaction::TransactionWndProc (Msg:%x)", wParam);

                pCTrans->OnINetCallback(TRUE);

                if (pCTrans->Release() == 0)
                {
                    DbgLog(tagCTransaction, pCTrans, "=== CTransaction::TransactionWndProc Last Release!");
                    pCTrans = 0;
                }

                PerfDbgLog1(tagCTransaction, pCTrans, "-CTransaction::TransactionWndProc (Msg:%x) WM_TRANS_PACKET", wParam);
            }
#ifdef WITH_EXCEPTION
            _except(UrlMonInvokeExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
            {
                dwFault = GetExceptionCode();

                #if DBG == 1
                //
                // UrlMon catches exceptions when the client generates them. This is so we can
                // cleanup properly, and allow urlmon to continue.
                //
                if (   dwFault == STATUS_ACCESS_VIOLATION
                    || dwFault == 0xC0000194 /*STATUS_POSSIBLE_DEADLOCK*/
                    || dwFault == 0xC00000AA /*STATUS_INSTRUCTION_MISALIGNMENT*/
                    || dwFault == 0x80000002 /*STATUS_DATATYPE_MISALIGNMENT*/ )
                {
                    WCHAR iidName[256];
                    iidName[0] = 0;
                    char achProgname[256];
                    achProgname[0] = 0;

                    GetModuleFileNameA(NULL,achProgname,sizeof(achProgname));
                    DbgLog2(tagCTransactionErr, pCTrans,
                                   "UrlMon has caught a fault 0x%08x on behalf of application %s",
                                   dwFault, achProgname);

                }
                #endif
            }
#ifdef unix
            __endexcept
#endif /* unix */
#endif //WITH_EXCEPTION
        }
        break;
        case WM_TRANS_NOPACKET:
        case WM_TRANS_OUTOFMEMORY:
            // tell the
            break;
        case WM_TRANS_INTERNAL:
            {
                TransAssert((FALSE));
            }
            break;

        }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::AddCTransPacket
//
//  Synopsis:
//
//  Arguments:  [pCTP] --
//
//  Returns:
//
//  History:    12-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CTransaction::AddCTransPacket(CTransPacket *pCTP, BOOL fTail)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::AddCTransPacket");
    CLock lck(_mxs);
    //TransAssert((pCTP != NULL && (pCTP->GetNotMsg() > Notify_None && pCTP->GetNotMsg() <= Notify_Internal) ));

    if (fTail)
    {

        if (_pCTPHead == NULL)
        {
            TransAssert((_pCTPTail == NULL));
            TransAssert((_cPacketsInList == 0));
            _pCTPHead = pCTP;
            _pCTPTail = pCTP;
            _pCTPTail->SetNext(NULL);

        }
        else
        {
            _pCTPTail->SetNext(pCTP);
            _pCTPTail = pCTP;
        }
    }
    else
    {
        // add it at the front
        if (_pCTPHead == NULL)
        {
            TransAssert((_pCTPTail == NULL));
            TransAssert((_cPacketsInList == 0));
            _pCTPHead = pCTP;
            _pCTPTail = pCTP;
            _pCTPTail->SetNext(NULL);

        }
        else
        {
            pCTP->SetNext(_pCTPHead);
            _pCTPHead = pCTP;
        }
    }
    _cPacketsInList++;
    TransAssert((_cPacketsInList > 0));

    PerfDbgLog3(tagCTransaction, this, "-CTransaction::AddCTransPacket [pCTP(%lx)[%lx], cPackets:%ld] ",
        pCTP, pCTP->GetNotMsg(),_cPacketsInList);
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::GetNextCTransPacket
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CTransPacket *CTransaction::GetNextCTransPacket()
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::GetNextCTransPacket");
    CLock lck(_mxs);
    CTransPacket *pCTP;

    pCTP = _pCTPHead;
    if (_pCTPHead == NULL)
    {
        TransAssert((_pCTPTail == NULL));
        TransAssert((_cPacketsInList == 0));
    }
    else if (_pCTPHead == _pCTPTail)
    {
        TransAssert((_pCTPTail->GetNext() == NULL));
        // only one packet in fifo
        _pCTPHead = _pCTPTail = NULL;
    }
    else
    {
        _pCTPHead = _pCTPHead->GetNext();
    }

    if (pCTP)
    {
        _cPacketsInList--;
    }

    //TransAssert((   (pCTP == NULL) || (pCTP != NULL && (pCTP->GetNotMsg() > Notify_None && pCTP->GetNotMsg() <= Notify_Internal)) ));

    TransAssert((   ((pCTP == NULL) && (_cPacketsInList == 0))
                 || ((pCTP != NULL) && (_cPacketsInList >= 0)) ));

    /*
    TransAssert((   ((pCTP == NULL) && (_cPacketsInList == 0))
                 || ((pCTP != NULL) && (_cPacketsInList >= 0))
                 || ((pCTP != NULL) && (_cPacketsInList == 0) && ((pCTP->GetNotMsg() == Notify_None) || (pCTP->GetNotMsg() == Notify_Error )))
               ));
    */


    PerfDbgLog2(tagCTransaction, this, "-CTransaction::GetNextCTransPacket [pCTP(%lx), cPackets:%ld] ",
        pCTP, _cPacketsInList);
    return pCTP;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::GotCTransPacket
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CTransaction::GotCTransPacket()
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::GotCTransPacket");
    CLock lck(_mxs);
    BOOL fGot = _pCTPHead ? true : false;
    PerfDbgLog(tagCTransaction, this, "-CTransaction::GotCTransPacket");
    return fGot;
}

CTransPacket::CTransPacket(PROTOCOLDATA *pSI)
{
    TransAssert((pSI));
    grfFlags    = pSI->grfFlags;
    dwState     = pSI->dwState ;
    pData       = pSI->pData   ;
    cbData      = pSI->cbData  ;

    _dwCurrentSize  = 0;
    _dwTotalSize    = 0;
    _dwResult       = 0;
    _pwzStr         = 0;
    _hrResult       = 0;
    _NotMsg         = (BINDSTATUS) BINDSTATUS_INTERNAL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransPacket::CTransPacket
//
//  Synopsis:
//
//  Arguments:  [NMsg] --
//              [hrRet] --
//              [szStr] --
//              [cbAvailable] --
//              [cbTotal] --
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CTransPacket::CTransPacket(BINDSTATUS NMsg, HRESULT hrRet, LPCWSTR szStr, DWORD cbAvailable, DWORD cbTotal, DWORD dwResult)
{
    PerfDbgLog1(tagCTransaction, this, "+CTransPacket::CTransPacket (NMsg:%lx)", NMsg);

    _dwCurrentSize = cbAvailable;
    _dwTotalSize = cbTotal;
    _hrResult = hrRet;
    _pCTPNext = NULL;
    _NotMsg = NMsg;
    _pwzStr = NULL;
    _dwResult = dwResult;

    if (szStr)
    {
        _pwzStr = OLESTRDuplicate( (LPWSTR)szStr );
    }

    dwState = _NotMsg;
    pData = this;
    cbData = sizeof(CTransPacket);
    grfFlags = 0;

    PerfDbgLog(tagCTransaction, this, "-CTransPacket::CTransPacket");
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransPacket::~CTransPacket
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CTransPacket::~CTransPacket()
{
    PerfDbgLog(tagCTransaction, this, "+CTransPacket::~CTransPacket");

    if (_pwzStr)
    {
        delete _pwzStr;
    }

    PerfDbgLog(tagCTransaction, this, "-CTransPacket::~CTransPacket");
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::CPrivUnknown::QueryInterface
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
STDMETHODIMP CTransaction::CPrivUnknown::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    PerfDbgLog(tagCTransaction, this, "+CTransaction::CPrivUnknown::QueryInterface");
    CTransaction *pCTrans = GETPPARENT(this, CTransaction, _Unknown);

    *ppvObj = NULL;
    if ((riid == IID_IUnknown) || (riid == IID_IOInetProtocolSink) )
    {
        *ppvObj = pCTrans;
        pCTrans->AddRef();
    }
    else if (riid == IID_IOInetBindInfo)
    {
        *ppvObj = (IOInetBindInfo *) pCTrans;
        pCTrans->AddRef();
    }
    else if (riid == IID_IServiceProvider)
    {
        *ppvObj = (IServiceProvider *) pCTrans;
        pCTrans->AddRef();
    }
    else if (riid == IID_IAuthenticate)
    {
        *ppvObj = (IAuthenticate *) pCTrans;
        pCTrans->AddRef();
    }
    else if (riid == IID_IOInetProtocol)
    {
        *ppvObj = (IOInetProtocol *) pCTrans;
        pCTrans->AddRef();
    }
    else if (riid == IID_IOInetPriority)
    {
        *ppvObj = (IOInetPriority *) pCTrans;
        pCTrans->AddRef();
    }
    else if (pCTrans->_pUnkInner)
    {
        hr = pCTrans->_pUnkInner->QueryInterface(riid, ppvObj);
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::CPrivUnknown::QueryInterface (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CTransaction::CPrivUnknown::AddRef
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
STDMETHODIMP_(ULONG) CTransaction::CPrivUnknown::AddRef(void)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::CPrivUnknown::AddRef");

    LONG lRet = ++_CRefs;

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::CPrivUnknown::AddRef (cRefs:%ld)", lRet);
    return lRet;
}
//+---------------------------------------------------------------------------
//
//  Function:   CTransaction::Release
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
STDMETHODIMP_(ULONG) CTransaction::CPrivUnknown::Release(void)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::CPrivUnknown::Release");

    CTransaction *pCTransaction = GETPPARENT(this, CTransaction, _Unknown);

    LONG lRet = --_CRefs;

    if (lRet == 0)
    {
        delete pCTransaction;
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::CPrivUnknown::Release (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::OnAttach
//
//  Synopsis:
//
//  Arguments:  [pwzURL] --
//              [pOInetBindInfo] --
//              [pOInetProtSink] --
//              [riid] --
//              [grfOptions] --
//              [pClsidProtocol] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::OnAttach(LPCWSTR pwzURL, IOInetBindInfo *pOInetBindInfo, IOInetProtocolSink *pOInetProtSink, DWORD grfOptions, DWORD_PTR dwReserved)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::OnAttach");
    HRESULT     hr = NOERROR;
    VDATETHIS(this);
    do
    {
        hr = NOERROR;
        if (_pwzProtClsId)
        {
            pOInetProtSink->ReportProgress(BINDSTATUS_PROTOCOLCLASSID, _pwzProtClsId);
        }

        if (_pwzRedirectUrl)
        {
            pOInetProtSink->ReportProgress(BINDSTATUS_REDIRECTING,_pwzRedirectUrl);
        }

        if (   (grfOptions & (PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP))
            && _fMimeVerified
            && _pwzMimeSuggested)
        {
            pOInetProtSink->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, _pwzMimeSuggested);
        }

        if (_fMimeHandlerLoaded)
        {
            pOInetProtSink->ReportProgress(BINDSTATUS_LOADINGMIMEHANDLER, NULL);
        }                        

        if (_pwzFileName)
        {
            pOInetProtSink->ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE, _pwzFileName);
        }

        // report data now
        if (!IsApartmentThread())
        {
            ThreadTransfer();
        }
        else if (_pCTPCur)
        {
            HRESULT hr1;
            hr1 = DispatchReport(_pCTPCur->GetNotMsg(),_grfBSCF, _pCTPCur->_dwCurrentSize, _pCTPCur->_dwTotalSize, _pCTPCur->_pwzStr, 0);
        }
        
        break;
    } while (TRUE);

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::OnAttach (hr:%lx)", hr);
    return hr;
}


//
//  IOInetProtocol
//
//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::Start
//
//  Synopsis:
//
//  Arguments:  [pwzURL] --
//              [pOInetProtSink] --
//              [pOInetBindInfo] --
//              [grfOptions] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::Start(LPCWSTR pwzURL, IOInetProtocolSink *pOInetProtSink,
                                 IOInetBindInfo *pOInetBindInfo, DWORD grfOptions, DWORD_PTR dwReserved)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::Start");
    HRESULT     hr = NOERROR;
    VDATETHIS(this);
    COInetSession *pCOInetSession = 0;
    
    AddRef();

    do
    {
        if (   !pwzURL
            || !pOInetBindInfo
            || !pOInetProtSink
           )
        {
            hr = E_INVALIDARG;
            break;
        }
        if (GetState() == TransSt_OperationFinished)
        {
            hr = E_FAIL;
            break;
        }
        
        //
        //  case Attachment
        //
        if (GetState() == TransSt_OperationStarted)
        {
            {   // single access block
                CLock lck(_mxsBind);

                TransAssert((grfOptions & BDGFLAGS_ATTACHED));
                TransAssert((_grfInternalFlags & BDGFLAGS_PARTIAL));
                TransAssert((!_fAttached));
   
                _fAttached = TRUE;
                _grfInternalFlags |= BDGFLAGS_ATTACHED;

                if (_fProtEmbed)
                {
                    // no sniffing in this object
                    _dwOInetBdgFlags = (grfOptions & ~(PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP | PI_CLASSINSTALL));
                }
                else
                {
                    _dwOInetBdgFlags = (grfOptions | PI_DATAPROGRESS);
                }


                AddClientOInet(pOInetProtSink, pOInetBindInfo);
         
            }
            
            hr = OnAttach(pwzURL, pOInetBindInfo, pOInetProtSink, grfOptions, dwReserved);
            break;
        }

                if (grfOptions & PI_NOMIMEHANDLER)
                        _fMimeHandlerEnabled = FALSE;

        //
        // case: Start of Transaction
        //

        TransAssert((_pClntProtSink == NULL));
        BOOL fFirstCreated = FALSE;

        {   // single access block
            CLock lck(_mxsBind);

            int cchUrlLen;

            cchUrlLen  = wcslen(pwzURL) + 1;
            _pwzUrl = (LPWSTR) new WCHAR [cchUrlLen];
            if (!_pwzUrl)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            wcscpy(_pwzUrl, pwzURL);
            
            TransAssert((_pClntProtSink == 0 && _cBdgRefs == 0));
            
            AddClientOInet(pOInetProtSink, pOInetBindInfo);

            if (grfOptions & BDGFLAGS_PARTIAL)
            {
                _grfInternalFlags |= BDGFLAGS_PARTIAL;
            }

            if (_fProtEmbed)
            {
                // no sniffing in this object
                _dwOInetBdgFlags = (grfOptions & ~(PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP | PI_CLASSINSTALL));
            }
            else
            {
                _dwOInetBdgFlags = (grfOptions | PI_DATAPROGRESS);
            }

        } // end single access

        // 
        // OE4( yet another shipped-so-we-can't-fix product ) will assume
        // ReportResult is always Async (urlmon post a message), because
        // they will do TWO reportResult, if we don't do Async, Terminate
        // will called before the first ReportResult returns, which cleans
        // up everything, when OE4 calls back with second ReportResult, they
        // will fault in urlmon.
        // 
        LONG dwMS = sizeof("mhtml:") - 1;
        if( wcslen(_pwzUrl) >= dwMS && !StrCmpNIW(_pwzUrl, L"mhtml:", dwMS) ) 
        {
            _fForceAsyncReportResult = TRUE;
        }
   
    

        // We must have at least one node in the client request linked list
        TransAssert((_pClntProtSink != NULL));

        hr = QueryService(IID_IOInetProtocol, IID_IOInetProtocol ,(void **)&_pProt);
        // work around InfoViewer bug(IE4 RAID #53224), 
        // they will return NOERROR with _pProt=NULL on QueryService()
        if( hr == NOERROR && _pProt == NULL)
        {
            hr = E_NOINTERFACE;
        }

        if (hr != NOERROR)
        {
            if ((hr = GetCOInetSession(0,&pCOInetSession,0)) == NOERROR)
            {
                hr = pCOInetSession->CreateFirstProtocol(pwzURL, (IOInetBindInfo *) this, &_pUnkInner, &_pProt, &_clsidProtocol);
                fFirstCreated = TRUE;
                if ((hr == NOERROR) && _pUnkInner)
                {
                    TransAssert((_pProt));
                    _pProt->Release();
                }
            }
        }
        else
        {
            // bugbug: find the correct cls id here.
            _clsidProtocol = CLSID_FtpProtocol;
        }

        if (hr != NOERROR)
        {
            TransAssert((!_pProt && !_pUnkInner));
            _pProt = 0;
            _pUnkInner = 0;
            break;
        }

        BOOL fNext;
        BOOL fProtEmbedded = FALSE; // embed only once
        TransAssert((_pProt));
        do  // loop over protocols
        {
            fNext = FALSE;
            TransAssert((hr == NOERROR));
            
            // Start the download operation
            TransAssert((_pProt != NULL));
            TransAssert(( !IsEqualIID(GetProtocolClassID(),CLSID_NULL) ));

            {
                delete [] _pwzProtClsId;
                _pwzProtClsId =  0;

                HRESULT hr1 = StringFromCLSID(_clsidProtocol, &_pwzProtClsId);
                if (SUCCEEDED(hr1))
                {
                    pOInetProtSink->ReportProgress(BINDSTATUS_PROTOCOLCLASSID, _pwzProtClsId);
                }
            }

            SetState(TransSt_OperationStarted);


            IOInetProtocol* pProtNotAgged = NULL; 
            if (_fProtEmbed && !fProtEmbedded)
            {
                _CProtEmbed.Initialize(this, 0, PP_PRE_SWITCH, grfOptions, 0, _pProt, pOInetProtSink, (LPWSTR )pwzURL);
                _pClntProtSink = (IOInetProtocolSink *)&_CProtEmbed;
                
                if (_pUnkInner)
                {
                    // release the protocol we loaded
                    _pProt->Release();
                }
                else
                {
                    // hold on to the non-aggregrated original prot, 
                    // we will need to release it in case of we don't use 
                    // this protocol 
                    pProtNotAgged = _pProt; 
                }

                _pProt = &_CProtEmbed;
                fProtEmbedded = TRUE;
                if (_pUnkInner)
                {
                    // extra addref for second pointer to this class
                    _pProt->AddRef();
                }
            
            }

            // Just before starting the transaction give it the priority.

            IOInetPriority * pOInetPriority = NULL;
            if (_pProt->QueryInterface(IID_IOInetPriority, (void **) &pOInetPriority) == S_OK)
            {
                pOInetPriority->SetPriority(_nPriority);
                pOInetPriority->Release();
            }

            {   // single access block
                CLock lck(_mxsBind);
                _fStarting = TRUE;
            }

            hr = _pProt->Start(pwzURL, this, (IOInetBindInfo *)this, 0,0);

            {   // single access block
                CLock lck(_mxsBind);
                _fStarting = FALSE;
            }

            if (_fReceivedAbort && (hr != NOERROR))
            {
                Abort(_hrAbort, _dwAbort);
            }
            else if (hr == E_PENDING)
            {
                hr = NOERROR;
            }
            else if (hr == INET_E_USE_DEFAULT_PROTOCOLHANDLER)
            {
                fNext = TRUE;
                AddRef();

                if (!_pUnkInner)
                {
                    _pProt->Release();
                }
                if (fProtEmbedded)
                {
                    if(pProtNotAgged)
                    {
                        pProtNotAgged->Release(); 
                        pProtNotAgged = 0;
                    }
                    _CProtEmbed.SetProtocol(NULL);
                }
                fProtEmbedded = FALSE;
                _pProt = 0;

                if (_pUnkInner)
                {
                    _pUnkInner->Release();
                    _pUnkInner = 0;
                }

                // bugbug: need to reset the protocol inside the embed protocol handler

                if (!fFirstCreated)
                {
                    hr = pCOInetSession->CreateFirstProtocol(pwzURL, (IOInetBindInfo *) this, &_pUnkInner, &_pProt, &_clsidProtocol);
                    fFirstCreated = TRUE;
                }
                else
                {
                    hr = pCOInetSession->CreateNextProtocol(pwzURL, (IOInetBindInfo *) this, &_pUnkInner, &_pProt, &_clsidProtocol);
                }
                
                if (hr != NOERROR)
                {
                    TransAssert((!_pProt && !_pUnkInner));
                    _pProt = 0;
                    _pUnkInner = 0;
                    fNext = FALSE;

                }
                else if (_pUnkInner)
                {
                    // release the extra addref - aggregation
                    Release();
                }

            }
            else if (hr != NOERROR)
            {
                // do not allow pending packages be dispatched
                // any more
                fNext = FALSE;
                _fDispatch = TRUE;
                if (_pProt && !_fTerminated)
                {
                    _fResultDispatched = TRUE;

                    if (fProtEmbedded)
                    {
                        _pClntProtSink->ReportResult(_hrResult, _dwResult, 0);
                    }
                    else
                    {
                        pOInetProtSink->ReportResult(_hrResult, _dwResult, 0);
                    }
                    Terminate(0);
                }
            }

            if(pProtNotAgged)
            {
                pProtNotAgged->Release(); 
                pProtNotAgged = 0;
            }
            
        } while  (fNext == TRUE);

        if (   (grfOptions & PI_SYNCHRONOUS)
            && SUCCEEDED(hr) 
            && (_fModalLoopRunning == FALSE))
        {
            // complet the binding in case of sychronous bind
            TransAssert((grfOptions & OIBDG_APARTMENTTHREADED));
            _fModalLoopRunning = TRUE;
            hr = CompleteOperation(grfOptions & BDGFLAGS_ATTACHED);
            _fModalLoopRunning = FALSE;
        }

        break;
    } while (TRUE);

    if (pCOInetSession)
    {
        pCOInetSession->Release();
    }

    Release();

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::Start (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::Continue
//
//  Synopsis:
//
//  Arguments:  [pStateInfo] --
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::Continue(PROTOCOLDATA *pStateInfo)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::Continue");
    VDATETHIS(this);

    HRESULT hr = NOERROR;

    hr = _pProt->Continue(pStateInfo);

    delete (CTransPacket *)pStateInfo;

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::Continue (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::Abort
//
//  Synopsis:
//
//  Arguments:  [hrReason] --
//              [dwOptions] --
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::Abort(HRESULT hrReason, DWORD dwOptions)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::Abort");
    VDATETHIS(this);

    HRESULT hr = NOERROR;
    BOOL fAbort = FALSE;
    {   // single access block

        CLock lck(_mxsBind);

        if (!_fAborted && !_fTerminated && _pProt && (_ThreadTransferState == TS_None))
        {
            _fReceivedAbort = TRUE;
            _hrAbort = hrReason;
            _dwAbort = dwOptions;
            // terminate might complete async!
            if (hrReason == NOERROR)
            {
                hrReason = E_ABORT;
            }
            if (!_fStarting)
            {
                _fAborted = TRUE;
                fAbort = TRUE;
            }
        }
    } // end single access

    if (fAbort)
    {
        hr = _pProt->Abort(hrReason, dwOptions);
        if( hr == INET_E_RESULT_DISPATCHED )
        {
            //
            // CINet already dispatch the ReportResult and it
            // is currently in the CTrans's msg queue, we need to 
            // reset those flag so that when the ReportResult
            // finally get processed, we won't get confused 
            // (Terminate gets called)
            //  
            _fAborted = FALSE;
            _fReceivedAbort = FALSE;
        }
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::Abort (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::Terminate
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::Terminate(DWORD dwOptions)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::Terminate");
    VDATETHIS(this);
    HRESULT hr = E_FAIL;
    BOOL fTotalTerminate = FALSE;

    do 
    {

        {   // single access block
            CLock lck(_mxsBind);

            if (_fTerminated)
            {
                break;
            }

            if (_dwDispatchLevel)
            {
                _fReceivedTerminate = TRUE;
                _dwTerminateOptions = dwOptions;
                break;
            }
            else
            {
                _fTerminating = TRUE;
            }
            fTotalTerminate = _fTerminated;

            TransAssert((  (dwOptions == BDGFLAGS_PARTIAL)
                         || (dwOptions == BDGFLAGS_ATTACHED)
                         || (dwOptions == 0) ));
                         
            //TransAssert((_grfInternalFlags & BDGFLAGS_PARTIAL));

            if (dwOptions & BDGFLAGS_PARTIAL)
            {
                // remove the first sink
                // 
                if (!(_grfInternalFlags & BDGFLAGS_ATTACHED))
                {
                    // the new sink is not yet attached
                    TransAssert((_cBdgRefs == 1));
                    RemoveClientOInet();

                    // now make sure we shut down this on case no thread transfer is going on
                    if (_ThreadTransferState == TS_None)
                    {
                        _fAborted = TRUE;
                    }
                    
                }
                else
                {
                   TransAssert((   ((_cBdgRefs == 1) && (_pClntProtSink != 0)) 
                                || ((_cBdgRefs == 0) && (_pClntProtSink == 0)) ));
                }
            }
            else if (dwOptions & BDGFLAGS_ATTACHED)
            {
                TransAssert((_cBdgRefs == 1));
                RemoveClientOInet();
            }
            else
            {
                // nothing to do here
            }
            
        }

        if (   !fTotalTerminate
            && (_fResultDispatched || _fAborted))
        {
            fTotalTerminate = TRUE;
            
            hr = NOERROR;
            RemoveClientOInet();

            //
            // release pointers from the APP
            //
            if (_pInetInfo && !_pUnkInner)
            {
                _pInetInfo->Release();
                _pInetInfo = NULL;
            }

            if (_pInetHttpInfo && !_pUnkInner)
            {
                _pInetHttpInfo->Release();
                _pInetHttpInfo = NULL;
            }
                    
            if (_pProt)
            {
                _pProt->Terminate(0);
            }

            if (_pBndInfo)
            {
                //
                #if DBG == 1
                if (_pBndInfo->stgmedData.tymed != TYMED_NULL)
                {
                    PerfDbgLog1(tagCTransaction, this, "+CTransaction::Terminate ReleaseStgMedium (%lx)", _pBndInfo->stgmedData);
                }
                #endif
                ReleaseBindInfo(_pBndInfo);
                delete _pBndInfo;
                _pBndInfo = NULL;
            }
            if (_fProtEmbed && _pUnkInner && !_fLocked)
            {
                // remove extra refcount
                _pProt->Release();
            }
            
            //
            //
            SetState(TransSt_OperationFinished);

            TransAssert((_cBdgRefs == 0));
            //TransAssert(( _fAborted || (!_fAborted && (_cPacketsInList == 0)) ));
        }
        
        {   // single access block
            CLock lck(_mxsBind);
            _fTerminating = FALSE;
            if (fTotalTerminate)
            {
                _fTerminated = TRUE;
            }
        }
        
        break;
    } while (TRUE);

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::Terminate (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::Suspend
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::Suspend()
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::Suspend");
    VDATETHIS(this);
    CLock lck(_mxsBind);

    HRESULT hr = NOERROR;

    if( _pProt )
    {
        hr = _pProt->Suspend();
    }
    else
    {
        hr = E_NOTIMPL;
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::Suspend (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::Resume
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::Resume()
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::Resume");
    VDATETHIS(this);
    CLock lck(_mxsBind);

    HRESULT hr = NOERROR;

    hr = _pProt->Resume();

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::Resume (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::Read
//
//  Synopsis:
//
//  Arguments:  [pBuffer] --
//              [cbBuffer] --
//              [pcbRead] --
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::Read(void *pBuffer, ULONG cbBuffer, ULONG *pcbRead)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::Read");
    HRESULT     hr = E_FAIL;
    VDATETHIS(this);
    CLock lck(_mxsBind);

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

    if (fRead == TRUE)
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

    PerfDbgLog4(tagCTransaction, this, "-CTransaction::Read (hr:%lx, cbRead:%lx, _cbTotalBytesRead:%lx, _cbBytesReported:%lx)",
                                        hr, *pcbRead, _cbTotalBytesRead, _cbBytesReported);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::Seek
//
//  Synopsis:
//
//  Arguments:  [dlibMove] --
//              [dwOrigin] --
//              [plibNewPosition] --
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::Seek");
    VDATETHIS(this);
    CLock lck(_mxsBind);

    HRESULT hr = NOERROR;

    hr = _pProt->Seek(dlibMove, dwOrigin, plibNewPosition);

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::Seek (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::LockRequest
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::LockRequest(DWORD dwOptions)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::LockRequest");
    VDATETHIS(this);
    CLock lck(_mxsBind);

    HRESULT hr = NOERROR;

    hr = _pProt->LockRequest(dwOptions);
    if (SUCCEEDED(hr))
    {
        _fLocked = TRUE;
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::LockRequest (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::UnlockRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::UnlockRequest()
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::UnlockRequest");
    VDATETHIS(this);
    CLock lck(_mxsBind);

    HRESULT hr = NOERROR;

    hr = _pProt->UnlockRequest();
    if (SUCCEEDED(hr))
    {
        _fLocked = FALSE;
        if (_fProtEmbed && _pUnkInner)
        {
            // remove extra refcount to the protocol
            _pProt->Release();
        }
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::UnlockRequest (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::SetPriority
//
//  Synopsis:
//
//  Arguments:  [nPriority] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::SetPriority(LONG nPriority)
{
    PerfDbgLog1(tagCTransaction, this, "+CTransaction::SetPriority (%ld)", nPriority);

    HRESULT hr = S_OK;

    _nPriority = nPriority;

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::SetPriority (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::GetPriority
//
//  Synopsis:
//
//  Arguments:  [pnPriority] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::GetPriority(LONG * pnPriority)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::GetPriority");

    HRESULT hr;

    if (!pnPriority)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pnPriority = _nPriority;
        hr = S_OK;
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction::GetPriority (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::LoadHandler
//
//  Synopsis:
//
//  Arguments:  [pwzStr] --
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransaction::LoadHandler(LPCWSTR pwzURL, COInetProt *pCProtHndl, DWORD dwMode)
{
    PerfDbgLog(tagCTransaction, this, "+CTransaction::LoadHandler");

    HRESULT     hr = NOERROR;
    VDATETHIS(this);
    
    IOInetProtocol      *pProtHandler = 0;
    IOInetProtocolSink  *pProtSnkHandler = 0;

    IOInetProtocolSink  *pProtSnkHandlerToMe = 0;
    IOInetProtocol      *pProtHandlerToMe = 0;
    
    COInetSession       *pCOInetSession = 0;
    CLSID                clsidHandler;

    PROTOCOLFILTERDATA FilterData = {sizeof(PROTOCOLFILTERDATA), 0 ,0, 0,0};
    
    hr = E_FAIL;

    do
    {
        if (!pwzURL || !pCProtHndl)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (!_fProtEmbed)
        {
            // can not load a handler without the embedded object
            break;
        }

        if ((hr = GetCOInetSession(0,&pCOInetSession,0)) != NOERROR)
        {
            break;
        }
        hr = pCOInetSession->CreateHandler(pwzURL, 0, 0, &pProtHandler, &clsidHandler);
        if (FAILED(hr))
        {
            break;
        }
        //
        // get the interfaces for the handler
        //
        TransAssert((pProtHandler));

        FilterData.pProtocolSink = 0;
        FilterData.pProtocol = 0;
        FilterData.pUnk = 0;
        FilterData.dwFilterFlags = 0;

        if (_dwOInetBdgFlags & PI_PASSONBINDCTX)
        {
            // do not need to addref pointer
            FilterData.pUnk = _pBndCtx;
        }
        

        hr = pProtHandler->QueryInterface(IID_IOInetProtocolSink, (void **) &pProtSnkHandler);
        if (hr != NOERROR)
        {
            break;
        }
        // set up the handler now
        {
            DWORD dwOptions = 0;
            TransAssert((pProtSnkHandler));
            pProtHandlerToMe = (IOInetProtocol *) pCProtHndl;
            pProtSnkHandlerToMe = (IOInetProtocolSink *) pCProtHndl;
            FilterData.pProtocol = pProtHandlerToMe;
            _pClntProtSink = pProtSnkHandler;
            _pProt = pProtHandler; 
        }
        
        hr = pProtHandler->Start(pwzURL, pProtSnkHandlerToMe, (IOInetBindInfo *)this, PI_FILTER_MODE | PI_FORCE_ASYNC, (DWORD_PTR) &FilterData);

        if (hr == NOERROR)
        {
        }   
        else if (hr == E_PENDING)
        {   
            // send on the first junk of data
        
            hr = NOERROR;
        }
        else
        {
            pProtHandler->Release();
            pProtHandler = 0;
        }

        break;
    } while (TRUE);

    if (pCOInetSession)
    {
        pCOInetSession->Release();
    }

    if (pProtHandler)
    {
        pProtHandler->Release();
    }

    PerfDbgLog1(tagCTransaction, this, "-CTransaction:: (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::UpdateVerifiedMimeType
//
//  Synopsis:
//
//  Arguments:  [pwzMime] --
//
//  Returns:
//
//  History:    5-20-1998   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CTransaction::UpdateVerifiedMimeType(LPCWSTR pwzMime)
{
    if( pwzMime )
    {

        if( _pwzMimeSuggested )
        {
            delete [] _pwzMimeSuggested;
        }

        _pwzMimeSuggested = OLESTRDuplicate((LPWSTR)pwzMime);
        _fMimeVerified = TRUE;
    }
}

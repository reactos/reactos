
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CMimeFt.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//----------------------------------------------------------------------------
#include <eapp.h>

PerfDbgTag(tagMft,   "Pluggable MF ", "Log CMimeFt", DEB_PROT)
    DbgTag(tagMftErr,"Pluggable MF", "Log CMimeFt Errors", DEB_PROT|DEB_ERROR)

PerfDbgTag(tagPF,   "Pluggable MF ", "Log Perf", DEB_PROT)



#ifndef unix

#define JOB_MINS_PER_HOUR               60i64
#define JOB_HOURS_PER_DAY               24i64
#define JOB_MILLISECONDS_PER_SECOND     1000i64
#define JOB_MILLISECONDS_PER_MINUTE     (60i64 * JOB_MILLISECONDS_PER_SECOND)
#define FILETIMES_PER_MILLISECOND       10000i64
#define FILETIMES_PER_MINUTE            (FILETIMES_PER_MILLISECOND * JOB_MILLISECONDS_PER_MINUTE)
#define FILETIMES_PER_DAY               (FILETIMES_PER_MINUTE * JOB_MINS_PER_HOUR * JOB_HOURS_PER_DAY)

#else
#define JOB_MINS_PER_HOUR               60LL
#define JOB_HOURS_PER_DAY               24LL
#define JOB_MILLISECONDS_PER_SECOND     1000LL
#define JOB_MILLISECONDS_PER_MINUTE     (60LL * JOB_MILLISECONDS_PER_SECOND)
#define FILETIMES_PER_MILLISECOND       10000LL
#define FILETIMES_PER_MINUTE            (FILETIMES_PER_MILLISECOND * JOB_MILLISECONDS_PER_MINUTE)
#define FILETIMES_PER_DAY               (FILETIMES_PER_MINUTE * JOB_MINS_PER_HOUR * JOB_HOURS_PER_DAY)

#endif /* unix */


void
AddDaysToFileTime(LPFILETIME pft, WORD Days)
{
    if (!Days)
    {
        return; // Nothing to do.
    }

    //
    // ft = ft + Days * FILETIMES_PER_DAY;
    //
    ULARGE_INTEGER uli, uliSum;
    uli.LowPart  = pft->dwLowDateTime;
    uli.HighPart = pft->dwHighDateTime;
#ifndef unix
    uliSum.QuadPart = uli.QuadPart + (__int64)Days * FILETIMES_PER_DAY;
#else
    U_QUAD_PART(uliSum) = U_QUAD_PART(uli) + (__int64)Days * FILETIMES_PER_DAY;
#endif /* unix */
    pft->dwLowDateTime  = uliSum.LowPart;
    pft->dwHighDateTime = uliSum.HighPart;
}

// helper from transapi.cxx
HRESULT GetMimeFileExtension(LPSTR, LPSTR, DWORD);

//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::QueryInterface
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::QueryInterface(REFIID riid, void **ppvObj)
{
    PerfDbgLog(tagMft, this, "+CMimeFt::QueryInterface");

    VDATEPTROUT(ppvObj, void*);
    VDATETHIS(this);

    HRESULT hr = NOERROR;
    *ppvObj = NULL;

    if( (riid == IID_IUnknown) ||
        (riid == IID_IOInetProtocol) || 
        (riid == IID_IOInetProtocolRoot)) 
    { 
        *ppvObj = (IOInetProtocol*) this;
        AddRef();
    }
    else if (riid == IID_IOInetProtocolSink ) 
    {
        *ppvObj = (IOInetProtocolSink*) this;
        AddRef();
    }
    else if (riid == IID_IOInetProtocolSinkStackable ) 
    {
        *ppvObj = (IOInetProtocolSinkStackable*) this;
        AddRef();
    }
    else
        hr = E_NOINTERFACE;

    PerfDbgLog1(tagMft, this, "-CMimeFt::QueryInterface(hr:%1x)", hr);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::AddRef
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) 
CMimeFt::AddRef(void) 
{
    PerfDbgLog(tagMft, this, "+CMimeFt::AddRef");

    LONG lRet = ++_CRefs;

    PerfDbgLog1(tagMft, this, "-CMimeFt::AddRef (cRef:%1d)", lRet);
    return lRet;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::Release
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) 
CMimeFt::Release(void) 
{
    PerfDbgLog(tagMft, this, "+CMimeFt::Release");
    LONG lRet = --_CRefs;
    if( !lRet )
        delete this;

    PerfDbgLog1(tagMft, this, "-CMimeFt::Release (cRef:%1d)", lRet);
    return lRet;    
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::Start
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [pProtSink] --
//              [pOIBindInfo] --
//              [grfSTI] --
//              [dwReserved] --
//
//  Returns:    E_PENDING indicating the current filter is not empty
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::Start(
    LPCWSTR pwzUrl,
    IOInetProtocolSink *pProtSink,
    IOInetBindInfo *pOIBindInfo,
    DWORD grfPI,
    DWORD_PTR dwReserved)
{
    PerfDbgLog(tagMft, this, "+CMimeFt::Start");
    PerfDbgLog(tagPF, this, "*************CMimeFt::Start");
    PProtAssert( pwzUrl && (!_pProtSink) && pProtSink && dwReserved );
    HRESULT hr = NOERROR;

    PProtAssert((grfPI & PI_FILTER_MODE));
    if( !(grfPI & PI_FILTER_MODE) )
        hr = E_INVALIDARG;

    // download or upload? from the rrfPI flag...

    if( !hr )
    {
        //get the Prot pointer here
        PROTOCOLFILTERDATA* FiltData = (PROTOCOLFILTERDATA*) dwReserved;
        _pProt = FiltData->pProtocol;
        _pProt->AddRef();
    
        //get the sink pointer here 
        _pProtSink = pProtSink;
        _pProtSink->AddRef();

        // create or reload the data filter 
        if( _pDF )
        {
            _pDF->Release(); 
            _pDF = NULL;
        }

        //
        // this piece needs some more work 
        // 1. pEFtFac will be CoCreated according to reg setting 
        // 2. We will keep a linked list for existing EFFactory
        //
        IEncodingFilterFactory* pEFtFac = new CEncodingFilterFactory;
        if( pEFtFac )
        {
            // should we use enum or string value?
            hr = pEFtFac->GetDefaultFilter( pwzUrl, L"text", &_pDF);
            PProtAssert(_pDF);

            // don't need the factory anymore, 
            // but later, we will keep it on the list
            pEFtFac->Release(); 
    
            // reset all internal counter
            _ulCurSizeFmtIn = 0;
            _ulCurSizeFmtOut = 0;
            _ulTotalSizeFmtIn = 0;
            _ulTotalSizeFmtOut = 0;
            _ulOutAvailable = 0;
            _ulContentLength = 0;


            hr = _pProtSink->ReportProgress(BINDSTATUS_DECODING, pwzUrl);
        }
        else
            hr = E_OUTOFMEMORY;

        BOOL fNeedCache = TRUE;

        if( hr == NOERROR )
        {
            DWORD       dwBINDF;
            BINDINFO    bindInfo;
            bindInfo.cbSize = sizeof(BINDINFO);
            hr = pOIBindInfo->GetBindInfo(&dwBINDF, &bindInfo);

            // not generate cache file if user specifies BINDF_NOWRITECACHE
            if( hr == NOERROR)
            {
                if( dwBINDF & BINDF_NOWRITECACHE )
                {
                    fNeedCache = FALSE;
                }
                //  BINDINFO_FIX(LaszloG) 8/15/96
                ReleaseBindInfo(&bindInfo);
            }
        }

        if( hr == NOERROR && fNeedCache )
        {
            // Create cache file entry

            // 1. get url name
            ULONG ulCount;
            LPWSTR rgwzStr[1] = {NULL};
            LPSTR pszURL = NULL;
            

            hr = pOIBindInfo->GetBindString(
                BINDSTRING_URL, (LPWSTR*)rgwzStr, 1, &ulCount);
            if( hr == NOERROR && ulCount == 1 )
            {
                pszURL = DupW2A(rgwzStr[0]);        
                if( pszURL )
                {
                    strncpy(_szURL, pszURL, MAX_PATH);
                }
                else
                {
                    _szURL[0] = '\0';
                }
            }

            delete [] rgwzStr[0];
            delete [] pszURL;

            /******** move to ::Read to get accurate file ext ******
            // 2. get cache file name and create file handle
            if( hr == NOERROR && _szURL[0] != '\0' )
            {
                LPSTR pszExt = NULL;
                pszExt = FindFileExtension(_szURL); 
                if( pszExt && *pszExt == '.' )
                {
                    // FindFileExtion will return ".htm" but wininet is
                    // expecting "htm", so remove the extra "." 
                    pszExt++; 
                }
                if( CreateUrlCacheEntry(_szURL, 0, pszExt, _szFileName, 0) )
                {
                    if( _szFileName[0] != '\0' )
                    {
                        _hFile = CreateFile(_szFileName, 
                            GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
                            NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                        if( _hFile == INVALID_HANDLE_VALUE )
                        {
                            _hFile = NULL;
                        }
                    }
                }   
            }

            // Report the CACHE file name
            LPWSTR pwzFileName = NULL;
            if( _szFileName[0] != '\0' )
            {
                pwzFileName = DupA2W(_szFileName);
            }

            if( pwzFileName )
            {
                ReportProgress(
                    BINDSTATUS_CACHEFILENAMEAVAILABLE, pwzFileName);

                delete [] pwzFileName;
            }
            ******** move to ::Read to get accurate file ext ******/

        }
    }

    PerfDbgLog1(tagMft, this, "-CMimeFt::Start (hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::Continue
//
//  Synopsis:
//
//  Arguments:  [pStateInfo] --
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::Continue( PROTOCOLDATA *pStateInfo) 
{
    PerfDbgLog(tagMft, this, "+CMimeFt::Continue");
    PProtAssert( _pProt );
    HRESULT hr = _pProt->Continue(pStateInfo);

    PerfDbgLog1(tagMft, this, "-CMimeFt::Continue(hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::Abort
//
//  Synopsis:
//
//  Arguments:  [hrReason] --
//              [dwOptions] --
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::Abort( HRESULT hrReason, DWORD dwOptions)
{
    PerfDbgLog(tagMft, this, "+CMimeFt::Abort");
    PProtAssert( _pProt );

    HRESULT hr = _pProt->Abort(hrReason, dwOptions);

    PerfDbgLog1(tagMft, this, "-CMimeFt::Abort(hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::Terminate
//
//  Synopsis:
//
//  Arguments: [dwOptions] --
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::Terminate( DWORD dwOptions)
{
    PerfDbgLog(tagMft, this, "+CMimeFt::Terminate");
    PProtAssert( _pProt );
    HRESULT hr = NOERROR;

    // release the sink
    if( _pProtSink )
    {
        _pProtSink->Release();
        //_pProtSink = NULL;
    }

    // unload the data filter 
    if( _pDF )
    {
        _pDF->Release(); 
        _pDF = NULL;
    }

    
    // get expire and lastmodified time from wininet
    INTERNET_CACHE_TIMESTAMPS   st;
    DWORD cbSt = sizeof(st);
    memset(&st, 0, cbSt);
    if (_pProt)
    {
        IWinInetHttpInfo* pWin = NULL;
        hr = _pProt->QueryInterface(IID_IWinInetHttpInfo, (void**)&pWin);
        if( hr == NOERROR && pWin )
        {
            pWin->QueryOption( INTERNET_OPTION_CACHE_TIMESTAMPS, &st, &cbSt);
            pWin->Release();
        }
    }

    // close the file handle
    if( _hFile )
    {
        CloseHandle(_hFile);
        _hFile = NULL;
    }

    // Commit ( or delete) cache file entry
    if( _szFileName[0] != '\0' && _szURL[0] != '\0' )
    {
        char szHeader[256];
        char szMime[64];

        W2A(_pwzMimeSuggested, szMime, 64); 
        wsprintf(szHeader, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n", 
                 _ulContentLength, szMime);          
 
        DWORD dwLen = strlen(szHeader);
        CommitUrlCacheEntry( _szURL, _szFileName, st.ftExpires, 
            st.ftLastModified, NORMAL_CACHE_ENTRY, 
            (LPBYTE)szHeader, dwLen, NULL, 0);

    }

    if (_pProt)
    {
        hr = _pProt->Terminate(dwOptions);
    }

    PerfDbgLog1(tagMft, this, "-CMimeFt::Terminate(hr:%1x)", hr);
    PerfDbgLog(tagPF, this, "*************CMimeFt::Terminate");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::Suspend
//
//  Synopsis:
//
//  Arguments: [none]
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::Suspend()
{
    PerfDbgLog(tagMft, this, "+CMimeFt::Suspend");
    PProtAssert( _pProt );
    HRESULT hr = _pProt->Suspend();

    PerfDbgLog1(tagMft, this, "-CMimeFt::Suspend(hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::Resume
//
//  Synopsis:
//
//  Arguments:  [none]
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::Resume()
{
    PerfDbgLog(tagMft, this, "+CMimeFt::Resume");
    PProtAssert( _pProt );

    HRESULT hr = _pProt->Resume();
    PerfDbgLog1(tagMft, this, "-CMimeFt::Suspend(hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::Read
//
//  Synopsis:   The real read is implemented in SmartRead, which also
//              serves as a data sniffer
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:         
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    HRESULT hr = NOERROR;
    hr = SmartRead(pv, cb, pcbRead, FALSE);

    // do delay report result
    if( hr == S_FALSE && _fDelayReport )
    {
        ReportResult(_hrResult, _dwError, _wzResult);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::SmartRead
//
//  Synopsis:   implementation of ::Read, we add one parameter indicating 
//              if this read is for data sniffing purpose, 
//              (the goal datasniff read is to ReportProgress(MIME), 
//               data is not returned to the user buffer, this is to overcome
//               the problem that if you do data sniffing in the real Read and
//               report progress from a free thread, the message won't get 
//               to the apartment thread until it finishs the read, which 
//               might be too late for BindToObject)  
//
//  Arguments:
//
//  Returns:
//
//  History:    11-24-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:         
//
//----------------------------------------------------------------------------
HRESULT
CMimeFt::SmartRead(void *pv, ULONG cb, ULONG *pcbRead, BOOL fSniff)
{
    PerfDbgLog(tagMft, this, "+CMimeFt::Read");
    if(fSniff)
    {
        PerfDbgLog(tagMft, this, "+CMimeFt::Read - sniffing");
    }
    else
    {
        PProtAssert( pv && _pOutBuf && pcbRead );
    }
    HRESULT hr = NOERROR;
    
    LONG    lRead = 0;
    HRESULT hrRead = E_FAIL;
    HRESULT hrCode = NOERROR;
    BOOL    fPullData = FALSE;

    // no data, I have to pull more
    if( _ulOutAvailable == 0 ) 
    {
        if( _ulInBufferLeft == 0) 
        {
            hrRead = _pProt->Read(_pInBuf, FT_IBUF_SIZE-2, (ULONG*)&lRead);  
            PerfDbgLog2(tagMft, this, " CMimeFt::Read-Pull %u bytes (hr %1x)", 
                lRead, hrRead);
            fPullData = TRUE;
        }
        else
        {
            lRead = _ulInBufferLeft;
        }        

    }

    if( lRead ) 
    {
        LONG    lInUsed = 0;
        LONG    lOutUsed = 0;
        BYTE*   pInBuf = _pInBuf; 

        if( _bEncoding )
            hrCode = _pDF->DoEncode(
                        0,
                        FT_IBUF_SIZE,
                        pInBuf, 
                        FT_OBUF_SIZE,
                        _pOutBuf, 
                        lRead,
                        &lInUsed,
                        &lOutUsed,
                        0);
        else
        {
            hrCode = _pDF->DoDecode(
                        0,
                        FT_IBUF_SIZE,
                        pInBuf,
                        FT_OBUF_SIZE,
                        _pOutBuf,
                        lRead,
                        &lInUsed, 
                        &lOutUsed,
                        0); 
        }

        if( hrCode != NOERROR )
        {
            // error msg from winerr.h
            _pProtSink->ReportResult(hr, CRYPT_E_BAD_ENCODE, NULL);

            // make sure we clean up the filter
            _ulOutAvailable = 0;
            _ulInBufferLeft = 0;

            PerfDbgLog(tagMft, this, " CMimeFt::Read-Encode/Decode Failed");
        }
        else
        {

            // update 
            _ulCurSizeFmtOut += lOutUsed;
            _ulOutAvailable = lOutUsed;

            // do we get all the data?
            if( lInUsed < lRead )
            {
                // move mem to front
                memcpy(_pInBuf, _pInBuf+lInUsed, lRead-lInUsed);
                _ulInBufferLeft = lRead - lInUsed;
            }
            else
            {
                _ulInBufferLeft = 0;
            }


            if( !_bMimeVerified )
            {
                _bMimeVerified = TRUE;
                LPWSTR pwzStr = NULL;
                LPWSTR pwzFileName = NULL;
                if( _szFileName[0] != '\0' )
                {
                    pwzFileName = DupA2W(_szFileName);
                }

                FindMimeFromData(NULL, pwzFileName, _pOutBuf, lOutUsed, 
                        _pwzMimeSuggested, 0, &pwzStr, 0);

                pwzFileName = NULL;

                if( !_bMimeReported && !_bEncoding ) 
                {
                    _bMimeReported = TRUE;
                    if( pwzStr )
                    {
                        hr = _pProtSink->ReportProgress(
                            BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, pwzStr);
                    }
                    else
                    {
                        hr = _pProtSink->ReportProgress(
                            BINDSTATUS_MIMETYPEAVAILABLE, L"text/html");
                    }
                }
                
                if( pwzStr )
                {
                    delete [] _pwzMimeSuggested;
                    _pwzMimeSuggested = pwzStr;
                    pwzStr = NULL;
                }

                //------- moved from ::Start to get accurate file ext
                if( hr == NOERROR && _szURL[0] != '\0' )
                {
                    LPSTR pszExt = NULL;
                    pszExt = FindFileExtension(_szURL); 

                    // HACK... need some API to tell if ext is valid...
                    // here we assume ext with 3 chars 
                    // including the ".", it will be 4 char
                    if( pszExt && strlen(pszExt) > 5 )
                    {
                        LPSTR pszMime = DupW2A(_pwzMimeSuggested);
                        GetMimeFileExtension( pszMime, pszExt, 20);
                        delete [] pszMime;
                    }


                    if( pszExt && *pszExt == '.' )
                    {
                        // FindFileExtion will return ".htm" but wininet is
                        // expecting "htm", so remove the extra "." 
                        pszExt++; 
                    }
                    if( CreateUrlCacheEntry(_szURL, 0, pszExt, _szFileName, 0) )
                    {
                        if( _szFileName[0] != '\0' )
                        {
                            _hFile = CreateFile(_szFileName, 
                                GENERIC_WRITE, 
                                FILE_SHARE_WRITE | FILE_SHARE_READ,
                                NULL,CREATE_ALWAYS, 
                                FILE_ATTRIBUTE_NORMAL, NULL);
                            if( _hFile == INVALID_HANDLE_VALUE )
                            {
                                _hFile = NULL;
                            }
                        }
                    }   
                }

                // Report the CACHE file name
                if( _szFileName[0] != '\0' )
                {
                    pwzFileName = DupA2W(_szFileName);
                }

                if( pwzFileName )
                {
                    ReportProgress(
                        BINDSTATUS_CACHEFILENAMEAVAILABLE, pwzFileName);

                    delete [] pwzFileName;
                }
                //------- moved from ::Start to get accurate file ext
            }

            // write to the file
            if( _hFile )
            {
                DWORD dwBytesWritten;
                WriteFile(_hFile, _pOutBuf, lOutUsed, &dwBytesWritten, NULL);
                if( lOutUsed != (LONG)dwBytesWritten )
                {
                    // write failed, clean up everything
                    CloseHandle(_hFile);
                    _hFile = NULL;
                    DeleteUrlCacheEntry( _szURL );
                    _szFileName[0] = '\0';
                    _szURL[0] = '\0';
                }
                
            }

            PerfDbgLog2(tagMft, this, 
                " CMimeFt::Read-Encode/Decode %u bytes-> %u bytes", 
                lInUsed, lOutUsed);
        }   
    }
    else
        PerfDbgLog(tagMft, this, " CMimeFt::Read-in buffer empty"); 


    // copy over (only for the purpose of non-sniffing)
    if( !fSniff && (hrCode == NOERROR) && _ulOutAvailable )
    {
        if(  cb >= _ulOutAvailable )
        {
            // client has bigger buffer
            memcpy(pv, _pOutBuf, _ulOutAvailable);
            *pcbRead = _ulOutAvailable;
            _ulOutAvailable = 0;
            hr = S_OK;

            PerfDbgLog1(tagMft, this, 
                " CMimeFt::Read-enough buffer copied %u bytes", *pcbRead);

        }
        else
        {
            // client have smaller buffer
            memcpy(pv, _pOutBuf, cb);
            *pcbRead = cb;

            // move mem to front
            memcpy(_pOutBuf, _pOutBuf+cb, _ulOutAvailable-cb);

            _ulOutAvailable -= cb;
            hr = S_OK;

            PerfDbgLog1(tagMft, this, 
                " CMimeFt::Read-not enough buffer copied %u bytes", cb);
        }

        // keep the total (content-length)
        _ulContentLength  += *pcbRead;    
    }


    // If we pulled the data, we should report that hr
    if( fPullData )
        hr = hrRead;

    // if encode-decode error occurs, we report it 
    if( hrCode != NOERROR )
        hr = hrCode;

    // if all data are gone and LASTNOTIFICATION, we should return S_FALSE 
    if(_grfBSCF & BSCF_LASTDATANOTIFICATION && (_ulOutAvailable == 0) ) 
    {
        PerfDbgLog(tagMft, this, 
            " CMimeFt::Read-Last Notification, set hr --> 1" );
        hr = S_FALSE;
    }


    PerfDbgLog1(tagMft, this, "-CMimeFt::Read (hr:%1x)", hr);
    return hr;
}

 
//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::Seek
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::Seek( 
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER *plibNewPosition)
{
    PerfDbgLog(tagMft, this, "+CMimeFt::Seek");

    // Seek will be available later
    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagMft, this, "-CMimeFt::Seek (hr:%1x)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::LockRequest
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::LockRequest(DWORD dwOptions)
{
    PerfDbgLog(tagMft, this, "+CMimeFt::LockRequest");
    PProtAssert(_pProt);

    HRESULT hr = _pProt->LockRequest(dwOptions);

    PerfDbgLog1(tagMft, this, "-CMimeFt::LockRequest(hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::UnlockRequest
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::UnlockRequest()
{
    PerfDbgLog(tagMft, this, "+CMimeFt::UnLockRequest");
    PProtAssert(_pProt);
    HRESULT hr = _pProt->UnlockRequest();

    PerfDbgLog1(tagMft, this, "-CMimeFt::UnLockRequest(hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::Switch
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::Switch(PROTOCOLDATA *pStateInfo)
{
    PerfDbgLog(tagMft, this, "+CMimeFt::Switch");
    PProtAssert(_pProtSink);

    HRESULT hr = _pProtSink->Switch(pStateInfo);

    PerfDbgLog1(tagMft, this, "-CMimeFt::Switch (hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::ReportProgress
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::ReportProgress( ULONG ulStatusCode, LPCWSTR szStatusText)
{
    PerfDbgLog(tagMft, this, "+CMimeFt::ReportProgress");
    PProtAssert(_pProtSink);     
    HRESULT hr = NOERROR;

    if( ulStatusCode == BINDSTATUS_MIMETYPEAVAILABLE )
    {
        delete [] _pwzMimeSuggested;
        _pwzMimeSuggested = OLESTRDuplicate(szStatusText);
        if( !_pwzMimeSuggested )
        {
            hr = E_OUTOFMEMORY;    
        }
    }
    else
    {
        hr = _pProtSink->ReportProgress(ulStatusCode, szStatusText);
    }

    PerfDbgLog1(tagMft, this, "-CMimeFt::ReportProgress(hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::ReportData
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:      E_PENDING returned if the filter is not empty   
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::ReportData( DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    PerfDbgLog(tagMft, this, "+CMimeFt::ReportData");
    HRESULT hr = NOERROR;
    
    // Do a data sniffing Read to get the mime type out of the buffer
    // reason for _fSniffed, _fSniffInProgress  and _fDelayReport 
    // is that if the data sniffing SmartRead() reaches EOF, the protocol
    // will do ReportResult() which we want to delay until the Real Read
    // finishs
    if( !_fSniffed && !_fSniffInProgress )
    {
        _fSniffInProgress = TRUE;
        hr = SmartRead( NULL, 0, 0, TRUE);
        _fSniffInProgress = FALSE;
        _fSniffed = TRUE;
    }

    if( _fSniffed )
    {
        hr = _pProtSink->ReportData(grfBSCF, ulProgress, ulProgressMax);
    }

    PerfDbgLog1(tagMft, this, "-CMimeFt::ReportData(hr:%1x)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::ReportResult
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult)
{
    PerfDbgLog(tagMft, this, "+CMimeFt::ReportResult");

    // REVISIT 
    PProtAssert(_pProtSink);     
    HRESULT hr = NOERROR;

    if( _fSniffInProgress )
    {
        // keep it and report it after read completes
        _hrResult = hrResult;
        _dwError = dwError;
        _wzResult = wzResult; // should be OLEDuplicateStr()
    
        _fDelayReport = TRUE;
    }
    else
    {
        hr = _pProtSink->ReportResult(hrResult, dwError, wzResult);
    }

    PerfDbgLog1(tagMft, this, "-CMimeFt::ReportResult(hr:%1x)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::SwitchSink
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    11-24-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::SwitchSink(IOInetProtocolSink* pSink)
{
    PerfDbgLog(tagMft, this, "+CMimeFt::SwitchSink");

    // REVISIT 
    PProtAssert(_pProtSink && pSink);     
    HRESULT hr = NOERROR;

    // keep track the existing sink (support for Commit/Rollback)
    // the release of the old sink will be done at the Commit time
    _pProtSinkOld = _pProtSink;
 
    // -----------------------------------------------------------
    // BUG: remove this block once enable the Commit-Rollback func
    // release the old sink
    //
    if( _pProtSinkOld )
    {
        _pProtSinkOld->Release();
    }
    // -----------------------------------------------------------

    // Change the sink
    _pProtSink = pSink;
    _pProtSink->AddRef();

    PerfDbgLog1(tagMft, this, "-CMimeFt::SwitchSink(hr:%1x)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::CommitSwitch
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    11-24-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:      Commit the sink switch, what we are doing here is
//              Release the old sink (old sink is coming from the
//              Start method and AddRef'ed there
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CMimeFt::CommitSwitch()
{
    PerfDbgLog(tagMft, this, "+CMimeFt::CommitSwitch");

    // release the old sink
    //if( _pProtSinkOld )
    //{
    //    _pProtSinkOld->Release();
    //}

    // reset
    //_pProtSinkOld = NULL;

    PerfDbgLog(tagMft, this, "-CMimeFt::CommitSwitch");
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::RollbackSwitch
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    11-24-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:      Error occured (most possibly the StackFilter() call failed, 
//              we have to rollback the  sink switch, what we are doing 
//              here is releasing the switched sink (AddRef'ed at SwitchSink
//              time), and set the original sink back, no ref count work on
//              the original sink 
//
//----------------------------------------------------------------------------
STDMETHODIMP
CMimeFt::RollbackSwitch()
{
    PerfDbgLog(tagMft, this, "+CMimeFt::RollbackSwitch");

    // copy the old sink back, release the new sink
    // (new sink is AddRef'ed at SwitchSink time)
    //if( _pProtSink )
    //{
    //    _pProtSink->Release();
    //} 
    //_pProtSink = _pProtSinkOld;

    // reset
    //_pProtSinkOld = NULL;
    PerfDbgLog(tagMft, this, "-CMimeFt::RollbackSwitch");

    return NOERROR;
}





//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::Create
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CMimeFt::Create(CMimeFt** ppv)
{
    PerfDbgLog(tagMft, NULL, "+CMimeFt::Create");
    HRESULT hr = NOERROR;

    // pProt can not be NULL
    PProtAssert(ppv);     
    *ppv = NULL;

    *ppv = new CMimeFt;
    if( *ppv == NULL )
        hr = E_OUTOFMEMORY;
    else
        hr = (*ppv)->CreateBuffer();

    PerfDbgLog1(tagMft, NULL, "-CMimeFt::Create(hr:%1x)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::CMimeFt
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CMimeFt::CMimeFt()
    : _CRefs()
{
    PerfDbgLog(tagMft, this, "+CMimeFt::CMimeFt");

    _pProtSink = NULL;
    _pProt = NULL;
    _pProtSinkOld = NULL;
    _pDF = NULL;
    _ulCurSizeFmtIn = 0;
    _ulCurSizeFmtOut = 0;
    _ulTotalSizeFmtIn = 0;
    _ulTotalSizeFmtOut = 0;
    _ulOutAvailable = 0;
    _ulInBufferLeft = 0;
    _pInBuf = NULL;
    _pOutBuf = NULL;

    _grfBSCF = 0x00;
    _bMimeReported = FALSE;
    _bMimeVerified = FALSE;
    _szFileName[0] = '\0';
    _szURL[0] = '\0';
    _hFile = NULL;
    _pwzMimeSuggested = NULL;

    _fDelayReport = FALSE;
    _fSniffed = FALSE;
    _fSniffInProgress = FALSE;
    _hrResult = NOERROR;
    _dwError = 0;
    _wzResult = NULL;

    DllAddRef();

    PerfDbgLog(tagMft, this, "-CMimeFt::CMimeFt");
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::CreateBuffer
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-30-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CMimeFt::CreateBuffer()
{
    PerfDbgLog(tagMft, this, "+CMimeFt::CreateBuffer");
    HRESULT hr = NOERROR;
    _pInBuf  = new BYTE[FT_IBUF_SIZE]; 	        
    _pOutBuf = new BYTE[FT_OBUF_SIZE];

    PProtAssert(_pInBuf && _pOutBuf);     
    if( !_pInBuf || !_pOutBuf )
        hr = E_OUTOFMEMORY;

    PerfDbgLog1(tagMft, this, "-CMimeFt::CreateBuffer(hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMimeFt::~CMimeFt
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CMimeFt::~CMimeFt()
{
    PerfDbgLog(tagMft, this, "+CMimeFt::~CMimeFt");

    if( _pInBuf )
        delete [] _pInBuf;

    if( _pOutBuf )
        delete [] _pOutBuf;

    delete [] _pwzMimeSuggested;

    if( _pProt )
        _pProt->Release(); 

    if( _pDF)
        _pDF->Release();
    
    if( _hFile )
        CloseHandle(_hFile);
    

    DllRelease();

    PerfDbgLog(tagMft, this, "-CMimeFt::~CMimeFt");
}



/*
 * @(#)URLStream.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "urlstream.hxx"
#include "urlcallback.hxx"
#include "../encoder/encodingstream.hxx"
#include "../encoder/charencoder.hxx"
#define CRITICALSECTIONLOCK CSLock lock(&_cs);


//----------------------------------------------------------------
HRESULT STDMETHODCALLTYPE 
CreateEncodingStream( 
    /* [in] */ IStream __RPC_FAR *pStm,
    /* [in] */ const WCHAR __RPC_FAR *pwszEncoding,
    /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStm)
{
    UINT codepage, size, len = lstrlen(pwszEncoding);

    if (CharEncoder::getCharsetInfo(pwszEncoding, &codepage, &size) == -2)
    {
        return XML_E_INVALIDENCODING;
    }
    Encoding* encoding = Encoding::newEncoding(pwszEncoding, len, 
        true, // BUGBUG -- this should be platform dependent.
        (CP_UCS_2 == codepage || CP_UCS_4 == codepage));
    if (encoding == NULL)
        return E_OUTOFMEMORY;
    IStream* estm = EncodingStream::newEncodingStream(pStm, encoding);
    if (estm == NULL)
        return E_OUTOFMEMORY;
    *ppStm = estm;
    return S_OK;
}

//----------------------------------------------------------------
HRESULT STDMETHODCALLTYPE 
CreateURLStream( 
    /* [in] */ const WCHAR __RPC_FAR *pszUrl,
    /* [in] */ const WCHAR __RPC_FAR *pszBaseUrl,
    /* [in] */ BOOL write, // true for write, false for read.
    /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStm)
{
    URLStream* ps =  new URLStream(NULL);
    if (ps == NULL)
        return E_OUTOFMEMORY;
 
    URL url;

    HRESULT hr;
    
    checkerr( url.set(pszUrl, pszBaseUrl) );

    checkerr ( ps->Open(&url, write ? URLStream::WRITE : URLStream::ASYNCREAD) );

    checkerr( ps->QueryInterface(IID_IStream, (void **)ppStm) );

    ps->Release(); // done with it.

    return S_OK;
error:
    delete ps;
    return hr;
}

extern HRESULT CreateSecurityManager(IInternetSecurityManager ** ppUnk);

//----------------------------------------------------------------
BOOL IsSafeByZone(DWORD dwBaseZoneID, DWORD dwActualZoneID)
{
    // This function returns TRUE if the baseZone is more secure than the
    // zone we are trying to fetch data from, and FALSE otherwise.
    // For example: http://intranet/page.htm would be able to access data from 
    // http://internet/data.asp but not the other way around.
    // This table maps the 5 predefined zones to a trust level according
    // to the security spec.  We don't know how to stack rank user defined zones 
    // so we ignore them for now.
    static const DWORD s_MaxZone = 4;
    static DWORD s_TrustLevel[s_MaxZone] = {
        0,  // URLZONE_LOCAL_MACHINE
        2,  // URLZONE_INTRANET     // cfranks wanted intranet to be LESS
        1,  // URLZONE_TRUSTED      // trusted than TRUSTED zones.
        3,  // URLZONE_INTERNET
// And UNTRUSTED are RESTRICTED SITES which should always FAIL to download.
//        4,  // URLZONE_UNTRUSTED  
    };
    // Now also, if the zone ID's are not URLZONE_UNTRUSTED and they are equal
    // then this is also allowed.  This allows user defined Zone's to work.
    if ((dwBaseZoneID != URLZONE_UNTRUSTED && dwBaseZoneID == dwActualZoneID) || 
        (dwActualZoneID < s_MaxZone && dwBaseZoneID < s_MaxZone &&
            s_TrustLevel[dwActualZoneID] >= s_TrustLevel[dwBaseZoneID]))
    {
        return TRUE;
    }
    return FALSE;
}

//----------------------------------------------------------------
// Internet Security helper function.
HRESULT UrlOpenAllowed(LPCWSTR pwszUrl, LPCWSTR pwszBaseUrl, BOOL fDTD)
{
    BYTE    reqSid[MAX_SIZE_SECURITY_ID],
            baseSid[MAX_SIZE_SECURITY_ID];
    DWORD   cbReqSid,
            cbBaseSide;
    DWORD   dwBaseZoneID = URLZONE_UNTRUSTED;
    DWORD   dwActualZoneID = URLZONE_UNTRUSTED;
    DWORD   policy = URLPOLICY_DISALLOW;

    bool result = false;

    static IInternetSecurityManager *g_pSecMgr = NULL;
    HRESULT hr;

    hr = CreateSecurityManager(&g_pSecMgr);
    if (hr)
        return hr;

    // We don't return an error if the MapUrlToZone calls fail, because this 
    // will just result in the IsSafeByZone calls failing - which will result
    // in tighter security.

    if (SUCCEEDED(g_pSecMgr->MapUrlToZone(pwszBaseUrl, &dwBaseZoneID, 0)))
    {
        if (dwBaseZoneID == URLZONE_LOCAL_MACHINE)
        {
            //
            // Special case for pages loaded from the local machine -
            // In order to make development and demos convenient we allow
            // loading pages across zones, iff the user enables running unsafe
            // activeX controls in that zone.
            //

            if (g_pSecMgr->ProcessUrlAction(pwszUrl,
                    URLACTION_ACTIVEX_OVERRIDE_SCRIPT_SAFETY,
                    (BYTE *) &policy,
                    sizeof(policy),
                    NULL,
                    0,
                    0,
                    0) == S_OK)
            {
                result = true;
                goto cleanup;
            }
        }            
    }

    if (SUCCEEDED(g_pSecMgr->MapUrlToZone(pwszUrl, &dwActualZoneID, 0)))
    {
        // DTD access is allowed if it is safe purely by ZONE ID -- independent
        // of matching security ID's.
        if (fDTD && IsSafeByZone(dwBaseZoneID, dwActualZoneID))
        {
            result = true;
            goto cleanup;
        }
    }
    
    //
    // Normal check - both URL's must be from the same domain
    //

    cbReqSid = LENGTH(reqSid);
    cbBaseSide = LENGTH(baseSid);
    if (SUCCEEDED(g_pSecMgr->GetSecurityId(pwszUrl, reqSid, &cbReqSid, 0)))
    {
        if (SUCCEEDED(g_pSecMgr->GetSecurityId(pwszBaseUrl, baseSid, &cbBaseSide, 0)))
        {
            result = (cbReqSid == cbBaseSide) &&
                    (::memcmp(reqSid, baseSid, cbReqSid) == 0);
        }
    }

    // New URLACTION_CROSS_DOMAIN_DATA that allows the user to explicity permit
    // cross domain data access for a given zone - so long as it is also safe
    // according to the Zone ID's.
    if (! result && IsSafeByZone(dwBaseZoneID, dwActualZoneID))
    {
        DWORD urlScheme = URL::getScheme(pwszUrl);
        DWORD baseScheme = URL::getScheme(pwszBaseUrl);
        if (!(urlScheme == INTERNET_SCHEME_HTTP && baseScheme == INTERNET_SCHEME_HTTPS) &&
            !(urlScheme == INTERNET_SCHEME_HTTPS && baseScheme == INTERNET_SCHEME_HTTP)) 
        {
            if (g_pSecMgr->ProcessUrlAction(pwszBaseUrl,    // see if cross-domain data
                URLACTION_CROSS_DOMAIN_DATA,    // access is allowed for that zone.
                (BYTE *) &policy,
                sizeof(policy),
                NULL,
                0,
                0,
                0) == S_OK)            
            {
                // User has explicitly enabled fetching data from different domains for
                // this zone.
                result = true;
                goto cleanup;
            }
        }
    }

cleanup:
    return result ? S_OK : E_ACCESSDENIED;
}

#ifdef _DEBUG
static LONG g_cURLStreams = 0;
#endif

//-----------------------------------------------------------------------------------
URLStream::URLStream(URLDownloadTask* task, bool fdtd)
{
#ifdef _DEBUG
    InterlockedIncrement(&g_cURLStreams);
#endif

    _hFile = INVALID_HANDLE_VALUE;
    _pUrl = NULL;
    _ulStatus = S_OK;
    _pTask = task;
    _nMode = 0;
    _fDTD = fdtd;
    InitializeCriticalSection(&_cs);
}

//-----------------------------------------------------------------------------------
URLStream::~URLStream()
{
#ifdef _DEBUG
    InterlockedDecrement(&g_cURLStreams);
#endif

    Reset();
    delete _pUrl;
    _pUrl = NULL;
    DeleteCriticalSection(&_cs);
}

//-----------------------------------------------------------------------------------
void URLStream::Reset()
{
    _pTask = NULL;
    if (_pCallback) 
    {
        _pCallback->Reset();
        _pCallback = null;
    }
    _pStream = NULL;
    _ulStatus = S_OK;
    if (_hFile != INVALID_HANDLE_VALUE)
    {
        if (_fIsOutput)
            ::FlushFileBuffers(_hFile);
        ::CloseHandle(_hFile);
        _hFile = INVALID_HANDLE_VALUE;
    }
}

//-----------------------------------------------------------------------------------
void URLStream::Abort() // stop a download.
{
    if (_pCallback)
        _pCallback->Abort();
    _pCallback = NULL;
    Reset();
}

//-----------------------------------------------------------------------------------
HRESULT URLStream::Open(URL * url, Mode flag)
{
    HRESULT hr;
    URLDownloadTask* task = _pTask;
    Reset();
    _pTask = task;

    _pUrl = new_ne URL(*url);
    if (!_pUrl)
    {
        return E_OUTOFMEMORY;
    }

    checkhr2 ( OpenAllowed(NULL) );

    _nMode = flag;

    if (_pUrl->isFile() && flag != ASYNCREAD) // use URLMON to do async loads.
    {
        _fIsLocal = true;
        return OpenFile(flag);
    }
    else
    {
        _fIsLocal = false;
        return OpenURL(NULL, NULL, flag);
    }
}

//-----------------------------------------------------------------------------------
HRESULT URLStream::Open(IMoniker * pmk, LPBC lpbc, URL * url, Mode flag)
{
    HRESULT hr;
    URLDownloadTask* task = _pTask;
    Reset();
    _pTask = task;

    _fIsLocal = false;

    _pUrl = new_ne URL(*url);
    if (!_pUrl)
    {
        return E_OUTOFMEMORY;
    }

    checkhr2 ( OpenAllowed(NULL) );

    return OpenURL(pmk, lpbc, flag);
}

//-----------------------------------------------------------------------------------
char* WideToAscii(const WCHAR* string)
{
    long length = StrLen(string);

    // find out how big the resulting string will be...
    int len = WideCharToMultiByte(CP_ACP, 0, string, length, NULL, 0, NULL, NULL);

    char * result = new_ne char[ len + 1 ];
    if (result == NULL)
        return NULL;

    WideCharToMultiByte(CP_ACP, 0, string, length, result, len, NULL, NULL);

    result[ len ] = '\0'; // NULL terminate it.
    return result;
}

//-----------------------------------------------------------------------------------
HRESULT URLStream::OpenFile(Mode flag)
{
    HRESULT hr = S_OK;
    DWORD access, disp;
    char *fname = NULL;

    TCHAR * filename = _pUrl->getFile();    
    if (!filename)
        return E_OUTOFMEMORY;

    // First try CreateFileW, and let the w9x ansi wrapper thunk down 
    // Then fall back to CreateFileA if that fails because we might be
    // on IE4 and/or Win95, where the wrapper and/or UNICODE implementations
    // aren't available

    _fIsOutput = (flag != ASYNCREAD && flag != SYNCREAD);
    access = _fIsOutput ? GENERIC_WRITE : GENERIC_READ;
    disp = _fIsOutput && flag != APPEND ? CREATE_ALWAYS : OPEN_EXISTING;

    _hFile = ::CreateFileW(filename,
        access, 
        FILE_SHARE_READ, 
        NULL, 
        disp, 
        FILE_ATTRIBUTE_NORMAL, 
        0);

    if (_hFile == INVALID_HANDLE_VALUE)
    {
        fname = WideToAscii(filename);
        if (fname == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }

        _hFile = ::CreateFileA(fname,
            access, 
            FILE_SHARE_READ, 
            NULL, 
            disp, 
            FILE_ATTRIBUTE_NORMAL, 
            0);
    }

    // now seek to end for appendage
    if (_hFile != INVALID_HANDLE_VALUE && _fIsOutput && flag == APPEND)
    {
        int i = ::SetFilePointer(_hFile, 0, NULL, FILE_END);
        if (i == 0xFFFFFFF)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            goto error;
        }
    }

    // error reporting
    if (_hFile == INVALID_HANDLE_VALUE)
    {
        hr = ::GetLastError();

        // Fix bug 43789: Turn the file not found error ERROR_FILE_NOT_FOUND
        // to a URLMON error number: INET_E_OBJECT_NOT_FOUND
        // so that we have consistent error messages for local files and http files 
        // when file is not found
        // In some cases, ::GetLastError returns 0 when a file does not exist
        // To get around this, we check and return the right error code
        if (hr == 0 || hr == ERROR_FILE_NOT_FOUND)
        {
            hr = INET_E_OBJECT_NOT_FOUND;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(hr);
        }
    }

error:
    delete[] fname;
    delete[] filename;
    return hr;
}

//-----------------------------------------------------------------------------------
HRESULT URLStream::OpenURL(IMoniker * pmk, LPBC lpbc, Mode flag)
{
    if (flag == APPEND || flag == WRITE)
        return E_INVALIDARG;

    HRESULT hr;
    IBindStatusCallback* bcs = NULL;
    IStream* pStream = NULL;
    bool nopmk = (pmk == NULL);

    URLCallback* icallback = new_ne URLCallback(this);
    if (icallback == NULL)
        return E_OUTOFMEMORY;
    _pCallback = icallback;
    icallback->Release(); // smart pointer has it now.

    IBindCtx* pbc = lpbc;
    if (pbc == NULL)
    {
        checkerr(CreateBindCtx(0, (IBindCtx **) &pbc));
    }
    else
        pbc->AddRef(); // since we release it in cleanup.

    checkerr(RegisterBindStatusCallback(pbc, icallback, &bcs, 0));    

    icallback->SetPreviousCallback(bcs);  
    if (bcs)
        bcs->Release();

    if (nopmk)
    {
        checkerr(CreateURLMoniker(_pUrl->getBaseMoniker(), _pUrl->getResolved(), (IMoniker **)&pmk));
    }
    else
        pmk->AddRef(); //   // since we release it in cleanup.

    // The callback will now call back and set the stream when
    // the stream is available.  Until then, the status is
    // E_PENDING.  If the callback finds an error of some sort
    // then the status will be set accordingly.
    _ulStatus = E_PENDING;

    checkerr(pmk->BindToStorage(pbc, NULL, IID_IStream, (void **)&pStream));

    if (pStream != NULL)
    {
        if (_pStream == NULL)
            _pStream = pStream;
        pStream->Release();
    }

    hr = S_OK;
cleanup:
    if (pbc)
        pbc->Release();
    if (pmk)
        pmk->Release();
    return hr;

error:
    _pCallback = NULL;
    goto cleanup;
}

//-----------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE URLStream::Commit(DWORD grfCommitFlags)
{
    if ((IStream*)_pStream != NULL)
    {
        _pStream->Commit(grfCommitFlags);
    }
    else
    {
        if (_hFile != INVALID_HANDLE_VALUE && _fIsOutput)
        {
            ::FlushFileBuffers(_hFile);
        }
    }
    return S_OK;
}

//-----------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE URLStream::Read(void * pv, ULONG cb, ULONG * pcbRead)
{
    // bug 55610
    CRITICALSECTIONLOCK;

    if (cb == 0)
    {
        return S_FALSE;
    }
    if (_fIsLocal)
    {
        DWORD dw;
        if (!::ReadFile(_hFile, pv, cb, &dw, NULL))
        {
            return HRESULT_FROM_WIN32(::GetLastError());
        }
        if (pcbRead != NULL)
            *pcbRead = dw;
        return (dw == 0) ? S_FALSE : S_OK;
    }
    else
    {
        if ((IStream*)_pStream == NULL)
            return _ulStatus;
        else if (_ulStatus != S_OK && _ulStatus != E_PENDING && _ulStatus != S_FALSE)
            return _ulStatus; // URLCallBack must have called SetStatus with some error then.
        else
        {
            _ulStatus = _pStream->Read(pv, cb, pcbRead);
            return _ulStatus;
        }
    }
}

/**
 * Writes an array of chars to a local file
 */
//-----------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE URLStream::Write(void const * pv, ULONG cb, ULONG * pcbWritten)
{
    DWORD dw;
    if (!::WriteFile(_hFile, pv, cb, &dw, NULL))
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    if (pcbWritten != NULL)
        *pcbWritten = dw;
    return S_OK;
}

////////////////////////////////////////////////////////////
// Internet Security
////////////////////////////////////////////////////////////
HRESULT URLStream::OpenAllowed(LPCWSTR pwszUrl)
{
    //
    // Return true if there isn't a secure base URL.
    //
    TCHAR* pwszBaseURL = GetURL()->getSecureBase();

    if (pwszBaseURL == NULL || 0 == *pwszBaseURL)
        return S_OK;

    if (pwszUrl == NULL)
    {
        pwszUrl = GetURL()->getResolved();
    }

    if (pwszUrl == NULL || 0 == *pwszUrl)
        return S_OK;

    return UrlOpenAllowed(pwszUrl, pwszBaseURL, _fDTD);
}

void    
URLStream::SetStatus(HRESULT hr) 
{
    CRITICALSECTIONLOCK;
    _ulStatus = hr; 
}
    
HRESULT 
URLStream::GetStatus() 
{ 
    CRITICALSECTIONLOCK;
    return _ulStatus; 
}

void    
URLStream::SetStream(IStream * s) 
{ 
    CRITICALSECTIONLOCK;
    _pStream = s; 
}

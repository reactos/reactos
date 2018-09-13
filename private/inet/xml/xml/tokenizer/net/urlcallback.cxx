/*
 * @(#) URLCallback.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "urlcallback.hxx"

#ifdef _DEBUG
static LONG g_cURLCallbacks = 0;
#endif

URLCallback::URLCallback(URLStream * s)
{
#ifdef _DEBUG
    InterlockedIncrement(&g_cURLCallbacks);
#endif

    _pStream = s;
    _fFinishedBinding = false;
    _pszwActualURL = NULL;
    _lRefs = 1;
    _fGetMimeType = false;
}

URLCallback::~URLCallback()
{
#ifdef _DEBUG
    InterlockedDecrement(&g_cURLCallbacks);
#endif

    Reset();
    _pbs = NULL;
    _binding = NULL;
}

//////////////////////////////////////////////////////////////////////////////
// IUnknown Interface
//
HRESULT STDMETHODCALLTYPE 
URLCallback::QueryInterface(REFIID riid, void ** ppvObject)
{
    STACK_ENTRY_IUNKNOWN(this);

	HRESULT hr = S_OK;
	if (riid == IID_IHttpNegotiate)
	{
		*ppvObject = static_cast<IHttpNegotiate*>(this);		
		AddRef();
	}
	else
	{
		hr = _unknown<IBindStatusCallback, &IID_IBindStatusCallback>::QueryInterface(riid, ppvObject);
	}
	return hr;
}

ULONG STDMETHODCALLTYPE 
URLCallback::AddRef( void)
{
    STACK_ENTRY_IUNKNOWN(this);
    return InterlockedIncrement(&_lRefs); // these are never RENTAL objects !
}

ULONG STDMETHODCALLTYPE 
URLCallback::Release( void)
{
    STACK_ENTRY_IUNKNOWN(this);
    LONG refs = InterlockedDecrement(&_lRefs); // these are never RENTAL objects !
    if (refs == 0)
    {
        TRY
        {
            delete this;
        }
        CATCH
        {
        }
        ENDTRY
    }
    return refs;
}

void 
URLCallback::Reset()
{
    _pStream = NULL;
    delete _pszwActualURL;
    _pszwActualURL = NULL;
    _fGetMimeType = false;
}

void 
URLCallback::Abort()
{
    if (_binding && ! _fFinishedBinding)
    {
        _binding->Abort();
        if (_pbs)
       {
            _pbs->OnStopBinding(E_FAIL, L"");
       }
    }
    _binding = NULL;
    Reset();
}

////////////////////////////////////////////////////////////
// IURLCallback Interface
// 
HRESULT STDMETHODCALLTYPE URLCallback::OnStartBinding(
    /* [in] */ DWORD grfBSCOption,
    /* [in] */ IBinding *pib)
{
    _binding = pib;
    _fFinishedBinding = false;
    if (_pbs)
    {
        _pbs->OnStartBinding(grfBSCOption, pib);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE URLCallback::GetPriority(
   /* [out] */ LONG *pnPriority)
{
    *pnPriority = NORMAL_PRIORITY_CLASS;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE URLCallback::OnLowResource(
    /* [in] */ DWORD reserved)
{
    return E_NOTIMPL;
}

#define MAX_MIMETYPE_LENGTH 128

HRESULT STDMETHODCALLTYPE URLCallback::OnProgress(
    /* [in] */ ULONG ulProgress,
    /* [in] */ ULONG ulProgressMax,
    /* [in] */ ULONG ulStatusCode,
    /* [in] */ LPCWSTR szStatusText)
{
    HRESULT hr = S_OK;

    if (!_fGetMimeType)
    {
        IWinInetHttpInfo * pWinInetHttpInfo = NULL;

        _fGetMimeType = true;
        hr = _binding->QueryInterface(IID_IWinInetHttpInfo, (void**)&pWinInetHttpInfo);
        if (S_OK == hr && NULL != pWinInetHttpInfo)
        {
            char * szContent = new char[MAX_MIMETYPE_LENGTH];
            if (szContent)
            {
                ULONG l = MAX_MIMETYPE_LENGTH;
                hr = pWinInetHttpInfo->QueryInfo(HTTP_QUERY_CONTENT_TYPE, szContent, &l, NULL, NULL);
                if (S_OK == hr)
                {
                    hr = getMediaType(szContent, (long)l);
                }
                delete [] szContent;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            pWinInetHttpInfo->Release();
        }
    }
    
    if (BINDSTATUS_REDIRECTING == ulStatusCode)
    {
        delete _pszwActualURL;
        _pszwActualURL = ::StringDup(szStatusText);
        if (_pszwActualURL == NULL)
            hr = E_OUTOFMEMORY;
    }

    if (_pbs)
    {
        _pbs->OnProgress(ulProgress, ulProgressMax, ulStatusCode, szStatusText);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE URLCallback::OnStopBinding(
    /* [in] */ HRESULT hresult,
    /* [in] */ LPCWSTR szError)
{
    HRESULT hr = S_OK;
    _binding = NULL;
    _fFinishedBinding = true;
    if (hresult != 0 && _pStream != NULL)
    {
        if (_pStream->GetStatus() == E_PENDING)
            _pStream->SetStatus(hresult);
    }

    // the mimeviewer uses this to force the download to happen as part of it's worker thread
    // by return E_PENDING from this call (see CallbackMonitor object)
    // we want to avoid invoking the parser (which happens in HandleData) directly from
    // the bind status callback, so we can control things such as how much data is executed
    // and to perform error reporting on failure, etc.
    if (_pbs)
    {
        hr = _pbs->OnStopBinding(hresult, szError);
        if (FAILED(hr))
            goto error;
    }
    
    if (_pStream != NULL)
    {
        // tell the download task that we're done.
        URLDownloadTask* task = _pStream->GetTask();
        if (task != NULL)
        {
            hr = task->HandleData(_pStream,true);
        }
    }
    return hr;

error:
    if (_pStream != NULL)
        _pStream->SetStatus(hr);
    return (hr == E_PENDING) ? S_OK : E_ABORT; // must return this to abort the download unless we're not done
                                               // but if we returned E_PENDING don't let URLMON know, from it's
                                               // point of view we succeeded
}

HRESULT STDMETHODCALLTYPE URLCallback::GetBindInfo(
    /* [out] */ DWORD *grfBINDF,
    /* [unique][out][in] */ BINDINFO *pbindinfo)
{
    *grfBINDF = BINDF_RESYNCHRONIZE | BINDF_PULLDATA; 
    if (_pStream != NULL && _pStream->GetMode() == URLStream::ASYNCREAD)
    {
        *grfBINDF  |= BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE;
    }
//  pbbindinfo->cbSize should already set by caller.
    ULONG size = pbindinfo->cbSize;
    memset(pbindinfo, 0, size);
    pbindinfo->cbSize = size;
    // default method is GET.  Valid ones are _GET, _PUT, _POST, _CUSTOM
    pbindinfo->dwBindVerb = BINDVERB_GET;
    // currently, we don't have any custom verbs, extrainfo, or info flags
    ZeroMemory(&pbindinfo->stgmedData,sizeof(pbindinfo->stgmedData));
    return S_OK;
}


HRESULT STDMETHODCALLTYPE URLCallback::OnDataAvailable(
    /* [in] */ DWORD grfBSCF,
    /* [in] */ DWORD dwSize,
    /* [in] */ FORMATETC *pformatetc,
    /* [in] */ STGMEDIUM *pstgmed)
{
    HRESULT hr  = S_OK;

    if (grfBSCF & BSCF_FIRSTDATANOTIFICATION)
    {
        // verify stream
        if (pstgmed->tymed != TYMED_ISTREAM)
        {
            hr = E_FAIL;
            goto error; // BUGBUG: Invalid stream
        }
        // security checking
        if (_pStream != NULL)
            hr = _pStream->OpenAllowed(_pszwActualURL);

        if (FAILED(hr))
        {
            goto error;
        }

        if (_pStream != NULL)
            _pStream->SetStream(pstgmed->pstm);
    }

    if ((grfBSCF & BSCF_LASTDATANOTIFICATION) && _pStream != NULL)
    {
        if (_pStream->GetStatus() == E_PENDING)
            _pStream->SetStatus(S_OK);
    }

    // the mimeviewer uses this to force the download to happen as part of it's worker thread
    // by return E_PENDING from this call (See Callback Monitor object)
    // we want to avoid invoking the parser (which happens in HandleData) directly from
    // the bind status callback, so we can control things such as how much data is executed
    // and to perform error reporting on failure, etc.
    if (_pbs)
    {
        hr = _pbs->OnDataAvailable(grfBSCF, dwSize, pformatetc, pstgmed);
        if (FAILED(hr))
            goto error;
    }

    if (_pStream != NULL)
    {
        URLDownloadTask* task = _pStream->GetTask();
        if (task != NULL)
        {
            hr = task->HandleData(_pStream,false);
        }
    }

    return hr;

error:
    if (_pStream != NULL)
        _pStream->SetStatus(hr);
    return (hr == E_PENDING) ? S_OK : E_ABORT; // must return this to abort the download unless we're not done
                                               // but if we returned E_PENDING don't let URLMON know, from it's
                                               // point of view we succeeded
}

HRESULT STDMETHODCALLTYPE URLCallback::OnObjectAvailable(
    /* [in] */ REFIID iid,
    /* [iid_is][in] */ IUnknown *punknown)
{
    return E_NOTIMPL;
}


////////////////////////////////////////////////////////////
// IHttpNegotiate Interface
// 

//
// A helper function to construct a header name
//
WCHAR *
MakeHeaders(WCHAR ** buf, WCHAR * pwszName, WCHAR * pwszContent)
{
    WCHAR * pwszEnd = L"\r\n";
    long len0 = *buf ? lstrlen(*buf) : 0;
    long len1 = lstrlen(pwszName);
    long len2 = lstrlen(pwszContent);
    long len3 = lstrlen(pwszEnd);
    WCHAR * pwszHeader = new_ne WCHAR[len0 + len1 + len2 + len3 + 1];
    if (pwszHeader != NULL)
    {
        if (*buf)
        {
            ::memcpy(pwszHeader, *buf, len0 * sizeof(WCHAR));
        }
        ::memcpy(&pwszHeader[len0], pwszName, len1 * sizeof(WCHAR));
        ::memcpy(&pwszHeader[len0 + len1], pwszContent, len2 * sizeof(WCHAR));
        ::memcpy(&pwszHeader[len0 + len1 + len2], pwszEnd, len3 * sizeof(WCHAR));
        pwszHeader[len0 + len1 + len2 + len3] = 0;
    }

    delete [] *buf;
    *buf = pwszHeader;

    return *buf;
}


HRESULT STDMETHODCALLTYPE 
URLCallback::BeginningTransaction( 
    /* [in] */ LPCWSTR szURL,
    /* [unique][in] */ LPCWSTR szHeaders,
    /* [in] */ DWORD dwReserved,
    /* [out] */ LPWSTR __RPC_FAR *pszAdditionalHeaders)
{
    Assert(pszAdditionalHeaders);
    HRESULT hr = S_OK;
    WCHAR * pwszHeaders = NULL;
    WCHAR * pwszReferer = _pStream->GetURL() ? _pStream->GetURL()->getBase() : NULL;

    *pszAdditionalHeaders = NULL;

    if (pwszReferer)
    {
        MakeHeaders(&pwszHeaders, L"REFERER: ", _pStream->GetURL()->getBase());

        if (pwszHeaders != NULL)
        {
            *pszAdditionalHeaders = (LPWSTR)CoTaskMemAlloc((lstrlen(pwszHeaders) + 1) * sizeof(WCHAR));
            if (*pszAdditionalHeaders != NULL)
            {
                wcscpy(*pszAdditionalHeaders, pwszHeaders);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            delete [] pwszHeaders;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE 
URLCallback::OnResponse( 
    /* [in] */ DWORD dwResponseCode,
    /* [unique][in] */ LPCWSTR szResponseHeaders,
    /* [unique][in] */ LPCWSTR szRequestHeaders,
    /* [out] */ LPWSTR __RPC_FAR *pszAdditionalRequestHeaders)
{
    Assert(pszAdditionalRequestHeaders);
    HRESULT hr = IsStatusOk(dwResponseCode) ? S_OK : S_FALSE;
    *pszAdditionalRequestHeaders = NULL;
    return hr;
}


WCHAR *
AsciiToWCHAR(char * sz, int cch, int * len)
{
    int cwch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, cch, NULL, 0);
    WCHAR * pwsz = new WCHAR[cwch + 1];
    if (pwsz)
    {
        cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, cch, pwsz, cwch);
        pwsz[cwch] = 0;
        *len = cwch;
    }
    return pwsz;
}

//
// szContentType may or may not contain charset parameter, so the format would be
// "text/xml; charset=utf-16" or "text/xml"
//
HRESULT
URLCallback::getMediaType(char * szContentType, long len)
{
    HRESULT hr = S_OK;
    URLDownloadTask* task = NULL;
    char ch;
    char * p = szContentType;
    int i = 0, l, l1;

    if (_pStream != NULL)
        task = _pStream->GetTask();

    if (!task || !len)
        goto Cleanup;

    do 
    {
        ch = *p++;
        i++;
    } while (i < len && ch != ';');

    if (ch == ';')
        l = i - 1;
    else 
        l = i;

    if (l > 0)
    {
        WCHAR * pwsz = AsciiToWCHAR(szContentType, l, &l1);
        if (pwsz)
        {
            task->SetMimeType(_pStream, pwsz, l1);
            delete [] pwsz;
        }
        else
        {
            return E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    l = len - i;
    if (l > 0)
    {
        // Get optional charset
        while (l > 0 && (ch == ' ' || ch == ';'))
        {
            l--;
            ch = *p++;
        }

        if ((ch == 'c' || ch == 'C') &&
            (l > 7) && (StrCmpNIA(p, "harset=", 7) == 0))
        {
            p += 7;
            l -= 7;

            if (l > 0)
            {
                WCHAR * pwsz = AsciiToWCHAR(p, l, &l1);
                if (pwsz)
                {
                    task->SetCharset(_pStream, pwsz, l1);
                    delete [] pwsz;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }

Cleanup:
    return hr;
}


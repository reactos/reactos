// CLSID_CWebViewMimeFilter
//
// Mime filter for Web View (.htx) content. Does substitutions on:
//
//   %TEMPLATEDIR%
//   %THISDIRPATH%
//   %THISDIRNAME%
//

#include "stdafx.h"
#pragma hdrstop
//#include "clsobj.h"

#define MAX_VARIABLE_NAME_SIZE 15 // see _Expand

// urlmon uses a 2K buffer size, so match that in retail. To force
// extra iterations and reallocations, use a smaller buffer size
// in debug. To further save on reallocations, we don't read the
// entire buffer to leave room for growth.
#ifdef DEBUG
#define BUFFER_SIZE 512
#define BUFFER_ALLOC_SIZE BUFFER_SIZE
#else
#define BUFFER_SIZE 0x2000
#define BUFFER_ALLOC_SIZE (BUFFER_SIZE+2*MAX_PATH)
#endif
#define BUFFER_SIZE_INC MAX_VARIABLE_NAME_SIZE*2 // must be > MAX_VARIABLE_NAME_SIZE

#define TF_EXPAND 0 // show strings as they are expanded in our mime filter?

class CWebViewMimeFilter : public IInternetProtocol
                         , public IInternetProtocolSink
                         , public IServiceProvider
{
public:
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IInternetProtocol methods
    STDMETHOD(Start)(
            LPCWSTR szUrl,
            IInternetProtocolSink *pProtSink,
            IInternetBindInfo *pOIBindInfo,
            DWORD grfSTI,
            DWORD dwReserved);
    STDMETHOD(Continue)(PROTOCOLDATA *pStateInfo);
    STDMETHOD(Abort)(HRESULT hrReason,DWORD dwOptions);
    STDMETHOD(Terminate)(DWORD dwOptions);
    STDMETHOD(Suspend)();
    STDMETHOD(Resume)();
    STDMETHOD(Read)(void *pv,ULONG cb,ULONG *pcbRead);
    STDMETHOD(Seek)(
            LARGE_INTEGER dlibMove,
            DWORD dwOrigin,
            ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(LockRequest)(DWORD dwOptions);
    STDMETHOD(UnlockRequest)();

    // IInternetProtocolSink methods
    STDMETHOD(Switch)(PROTOCOLDATA * pProtocolData);
    STDMETHOD(ReportProgress)(ULONG ulStatusCode, LPCWSTR pwszStatusText);
    STDMETHOD(ReportData)(DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax);
    STDMETHOD(ReportResult)(HRESULT hrResult, DWORD dwError, LPCWSTR pwszResult);

    // IServiceProvider methods
    STDMETHOD(QueryService)(REFGUID rsid, REFIID riid, void ** ppvObj);

private:
    CWebViewMimeFilter();
    ~CWebViewMimeFilter();
    friend HRESULT CWebViewMimeFilter_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);

    LPBYTE _StrCmp(LPBYTE pSrc, LPCSTR pAnsi, LPWSTR pUnicode);
    LPBYTE _StrChr(LPBYTE pSrc, char chA, WCHAR chW);
    int    _StrLen(LPBYTE pStr);

    void _QueryForDVCMDID(int dvcmdid, LPBYTE pDst, int cbDst);
    HRESULT _IncreaseBuffer(ULONG cbIncrement, LPBYTE * pp1, LPBYTE * pp2);
    int _Expand(LPBYTE pszVar, LPBYTE * ppszExp);
    HRESULT _ReadAndExpandBuffer();

    int _cRef;

    LPBYTE _pBuf;       // our buffer
    ULONG _cbBufSize;   // size of the buffer
    ULONG _nCharSize;   // sizeof(char) or sizeof(WCHAR) depending on data type
    ULONG _cbBuf;       // count of bytes read into the buffer
    ULONG _cbSeek;      // offset to seek position
    BYTE  _szTemplateDirPath[MAX_PATH];
    ULONG _cbTemplateDirPath;
    BYTE  _szThisDirPath[MAX_PATH];
    ULONG _cbThisDirPath;
    BYTE  _szThisDirName[MAX_PATH];
    ULONG _cbThisDirname;

    IInternetProtocol*         _pProt;             // incoming
    IInternetProtocolSink*     _pProtSink;         // outgoing
};

CWebViewMimeFilter::CWebViewMimeFilter()
{
    _cRef = 1;
}

CWebViewMimeFilter::~CWebViewMimeFilter()
{
    ATOMICRELEASE(_pProt);

    if (_pBuf)
    {
        LocalFree(_pBuf);
        _pBuf = NULL;
        _cbBufSize = 0;
    }

    ASSERT(NULL == _pProtSink);
}

HRESULT CWebViewMimeFilter_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    // aggregation checking is handled in class factory

    HRESULT hres;
    CWebViewMimeFilter* pObj;

    pObj = new CWebViewMimeFilter();
    if (pObj)
    {
        hres = pObj->QueryInterface(riid, ppvOut);
        pObj->Release();
    }
    else
    {
        *ppvOut = NULL;
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

ULONG CWebViewMimeFilter::AddRef(void)
{
    _cRef++;
    return _cRef;
}


ULONG CWebViewMimeFilter::Release(void)
{
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;

    return 0;
}

HRESULT CWebViewMimeFilter::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CWebViewMimeFilter, IInternetProtocol),
        QITABENTMULTI(CWebViewMimeFilter, IInternetProtocolRoot, IInternetProtocol),
        QITABENT(CWebViewMimeFilter, IInternetProtocolSink),
        QITABENT(CWebViewMimeFilter, IServiceProvider),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

// IInternetProtocol methods
HRESULT CWebViewMimeFilter::Start(
        LPCWSTR szUrl,
        IInternetProtocolSink *pProtSink,
        IInternetBindInfo *pOIBindInfo,
        DWORD grfSTI,
        DWORD dwReserved)
{
    HRESULT hr;

    if (!(EVAL(grfSTI & PI_FILTER_MODE)))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // get the Prot pointer here
        PROTOCOLFILTERDATA* FiltData = (PROTOCOLFILTERDATA*) dwReserved;
        ASSERT(NULL == _pProt);
        _pProt = FiltData->pProtocol;
        _pProt->AddRef();

        // hold onto the sink as well
        ASSERT(NULL == _pProtSink);
        _pProtSink = pProtSink;
        _pProtSink->AddRef();

        // this filter converts text/webviewhtml to text/html
        _pProtSink->ReportProgress(BINDSTATUS_FILTERREPORTMIMETYPE, L"text/html");
        
        hr = S_OK;
    }

    return hr;
}
HRESULT CWebViewMimeFilter::Continue(PROTOCOLDATA *pStateInfo)
{
    ASSERT(_pProt);
    return _pProt->Continue(pStateInfo);
}
HRESULT CWebViewMimeFilter::Abort(HRESULT hrReason,DWORD dwOptions)
{
    ATOMICRELEASE(_pProtSink); // probably to remove ref cycle

    ASSERT(_pProt);
    return _pProt->Abort(hrReason, dwOptions);
}
HRESULT CWebViewMimeFilter::Terminate(DWORD dwOptions)
{
    ATOMICRELEASE(_pProtSink); // probably to remove ref cycle

    return _pProt->Terminate(dwOptions);
}
HRESULT CWebViewMimeFilter::Suspend()
{
    return _pProt->Suspend();
}
HRESULT CWebViewMimeFilter::Resume()
{
    return _pProt->Resume();
}

LPBYTE CWebViewMimeFilter::_StrCmp(LPBYTE pSrc, LPCSTR pAnsi, LPWSTR pUnicode)
{
    if (SIZEOF(char) == _nCharSize)
    {
        return (LPBYTE)lstrcmpA(pAnsi, (LPSTR)pSrc);
    }
    else
    {
        ASSERT(_nCharSize == SIZEOF(WCHAR));

        return (LPBYTE)StrCmpW(pUnicode, (LPWSTR)pSrc);
    }
}

LPBYTE CWebViewMimeFilter::_StrChr(LPBYTE pSrc, char chA, WCHAR chW)
{
    if (SIZEOF(char) == _nCharSize)
    {
        return (LPBYTE)StrChrA((LPSTR)pSrc, chA);
    }
    else
    {
        return (LPBYTE)StrChrW((LPWSTR)pSrc, chW);
    }
}

int CWebViewMimeFilter::_StrLen(LPBYTE pStr)
{
    if (SIZEOF(char) == _nCharSize)
    {
        return lstrlenA((LPSTR)pStr);
    }
    else
    {
        return lstrlenW((LPWSTR)pStr);
    }
}

/*
 * UnicodeToHTMLEscapeStringAnsi
 *
 * Takes a unicode string as the input source and translates it into an ansi string that mshtml can process.  Characters > 127 will be
 * translated into an html escape sequence that has the following syntax:  "&#xxxxx;" where xxxxx is the string representation of the decimal
 * integer which is the value for the unicode character.  In this manner we are able to generate HTML text which represent UNICODE characters.
 */
#define MAX_HTML_ESCAPE_SEQUENCE 9  // longest string representation of a 16 bit integer is 65535.  So, entire composite escape string has:
                                    // 2 for "&#" + maximum of 5 digits + 1 for ";" + 1 for NULL terminator = 9 characters
void UnicodeToHTMLEscapeStringAnsi(LPWSTR pstrSrc, LPSTR pstrDest, int cbDest)
{
    while (*pstrSrc && (cbDest >= MAX_HTML_ESCAPE_SEQUENCE))
    {
        int iLen;
        ULONG ul = MAKELONG(*pstrSrc, 0);

        // We can optimize the common ansi characters to avoid generating the long escape sequence.  This allows us to fit
        // longer paths in the buffer.
        if (ul < 128)
        {
            *pstrDest = (CHAR)*pstrSrc;
            iLen = 1;
        }
        else
        {
            iLen = wsprintfA(pstrDest, "&#%lu;", ul);
        }
        pstrDest += iLen;
        cbDest -= iLen;
        pstrSrc++;
    }
    *pstrDest = 0;
}

void CWebViewMimeFilter::_QueryForDVCMDID(int dvcmdid, LPBYTE pDst, int cbDst)
{
    IOleCommandTarget * pct;
    if (SUCCEEDED(QueryService(SID_DefView, IID_IOleCommandTarget, (LPVOID*)&pct)))
    {
        VARIANT v = {0};

        if (S_OK == pct->Exec(&CGID_DefView, dvcmdid, 0, NULL, &v))
        {
            if (v.vt == VT_BSTR)
            {
                if (SIZEOF(char) == _nCharSize)
                {
                    UnicodeToHTMLEscapeStringAnsi(v.bstrVal, (LPSTR)pDst, cbDst);
                }
                else
                {
                    ASSERT(_nCharSize == SIZEOF(WCHAR));
            
                    StrCpyNW((LPWSTR)pDst, v.bstrVal, cbDst/sizeof(WCHAR));
                }
            }

            VariantClear(&v);
        }
        pct->Release();
    }
}

int CWebViewMimeFilter::_Expand(LPBYTE pszVar, LPBYTE * ppszExp)
{
    if (!_StrCmp(pszVar, "TEMPLATEDIR", L"TEMPLATEDIR"))
    {
        if (!_szTemplateDirPath[0])
        {
#ifdef UNICODE
            WCHAR wszBuf[MAX_PATH];
            GetWindowsDirectory(wszBuf, ARRAYSIZE(wszBuf));
            WideCharToMultiByte(CP_ACP, 0, wszBuf, -1, (LPSTR)_szTemplateDirPath, sizeof(_szTemplateDirPath)/sizeof(WCHAR), NULL, NULL);
#else
            GetWindowsDirectory((LPSTR)_szTemplateDirPath, sizeof(_szTemplateDirPath)/sizeof(char));
#endif
            StrNCatA((LPSTR)_szTemplateDirPath, "\\WEB", sizeof(_szTemplateDirPath)/sizeof(char));

            if (SIZEOF(WCHAR)==_nCharSize)
            {
                char szBuf[MAX_PATH];
                lstrcpyA(szBuf, (LPSTR)_szTemplateDirPath);
                _cbTemplateDirPath = MultiByteToWideChar(CP_ACP, 0, szBuf, -1,
                        (LPWSTR)_szTemplateDirPath, sizeof(_szTemplateDirPath)/sizeof(WCHAR));
            }
        }
        *ppszExp = _szTemplateDirPath;
    }
    else if (!_StrCmp(pszVar, "THISDIRPATH", L"THISDIRPATH"))
    {
        if (!_szThisDirPath[0])
        {
            _QueryForDVCMDID(DVCMDID_GETTHISDIRPATH, _szThisDirPath, SIZEOF(_szThisDirPath));
        }
        *ppszExp = _szThisDirPath;
    }
    else if (!_StrCmp(pszVar, "THISDIRNAME", L"THISDIRNAME"))
    {
        if (!_szThisDirName[0])
        {
            _QueryForDVCMDID(DVCMDID_GETTHISDIRNAME, _szThisDirName, SIZEOF(_szThisDirName));
        }
        *ppszExp = _szThisDirName;
    }
    else
    {
        *ppszExp = (LPBYTE)(L""); // just in case memcpy doesn't like a null pointer
    }

    return _StrLen(*ppszExp);
}

//
//  Ensure room for at least cbIncrement more bytes at the end of the buffer.
//  If the memory gets moved or realloced, *pp1 and *pp2 are adjusted to
//  point to the corresponding bytes at their new location(s).
//
HRESULT CWebViewMimeFilter::_IncreaseBuffer(ULONG cbIncrement, LPBYTE * pp1, LPBYTE * pp2)
{
    HRESULT hr = S_OK;

    // first check if there's room at the beginning of the buffer
    if (_cbSeek >= cbIncrement)
    {
        MoveMemory(_pBuf, _pBuf + _cbSeek, _cbBuf - _cbSeek);
        _cbBuf -= _cbSeek;

        if (pp1)
            *pp1 = *pp1 - _cbSeek;
        if (pp2)
            *pp2 = *pp2 - _cbSeek;

        _cbSeek = 0;
    }
    else
    {
        // not enough room, so allocate more memory
        LPBYTE p = (LPBYTE)LocalReAlloc(_pBuf, _cbBufSize + cbIncrement, LMEM_MOVEABLE);
        if (!p)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            if (pp1)
                *pp1 = p + (int)(*pp1 - _pBuf);
            if (pp2)
                *pp2 = p + (int)(*pp2 - _pBuf);
    
            _pBuf = p;
            _cbBufSize += cbIncrement;
        }
    }

    return hr;
}

HRESULT CWebViewMimeFilter::_ReadAndExpandBuffer()
{
    HRESULT hr;

    _cbBuf = 0;
    _cbSeek = 0;

    if (!_pBuf)
    {
        _pBuf = (LPBYTE)LocalAlloc(LPTR, BUFFER_ALLOC_SIZE);
        if (!_pBuf)
            return E_OUTOFMEMORY;

        _cbBufSize = BUFFER_ALLOC_SIZE;
    }

    // As strings expand, our buffer grows. If we keep reading in the
    // max amount, we'll keep reallocating the more variable expansions
    // we do. By only reading in BUFFER_SIZE, our _pBuf will grow only
    // a few times and then all the variable expansions should fit
    // in the extra room generated. NOTE: for debug builds, always
    // read the most we can, so we reallocate more often.
#ifdef DEBUG
    #define BUFFER_READ_SIZE (_cbBufSize)
#else
    #define BUFFER_READ_SIZE BUFFER_SIZE
#endif
    hr = _pProt->Read(_pBuf, BUFFER_READ_SIZE-SIZEOF(WCHAR), &_cbBuf); // make sure we have room for NULL
    if (SUCCEEDED(hr) && _cbBuf > 0)
    {
        LPBYTE pchSeek = _pBuf;
        LPBYTE pchEnd;

        if (!_nCharSize)
        {
            // scan buffer and figure out if it's unicode or ansi
            //
            // since we'll always be looking at html and the html header
            // is standard ansi chars, every other byte will be null if
            // we have a unicode buffer. i'm sure 3 checks are enough,
            // so we'll require 8 characters...
            if (_cbBuf > 6 &&
                0 == _pBuf[1] &&
                0 == _pBuf[3] &&
                0 == _pBuf[5])
            {
                TraceMsg(TF_EXPAND, "WebView MIME filter - buffer is UNICODE");
                _nCharSize = SIZEOF(WCHAR);
            }
            else
            {
                TraceMsg(TF_EXPAND, "WebView MIME filter - buffer is ANSI");
                _nCharSize = SIZEOF(char);
            }
        }

        // The string had better be null-terminated, for not only are we
        // going to do a StrChr, but our loop control relies on it!
        // The buffer might have leftover goo from a previous go-round, so
        // ensure that the nulls are there.
        _pBuf[_cbBuf] = _pBuf[_cbBuf+1] = 0;

#ifdef DEBUG
        if (SIZEOF(char)==_nCharSize)
            TraceMsg(TF_EXPAND, "Read A[%hs]", _pBuf);
        else
            TraceMsg(TF_EXPAND, "Read W[%ls]", _pBuf);
#endif

        do {
            LPBYTE pchStart = pchSeek;

            // Assert that the string is still properly null-terminated
            // because we're going to be doing StrChr soon.
            ASSERT(_pBuf[_cbBuf] == 0);
            ASSERT(_pBuf[_cbBuf+1] == 0);

            pchSeek = _StrChr(pchSeek, '%', L'%');
            if (!pchSeek)
                break;

            pchEnd = _StrChr(pchSeek+_nCharSize, '%', L'%');
            if (!pchEnd)
            {
                // no terminator. if there's plenty of space to the end of
                // this buffer then there can't be a clipped variable
                // name to expand.
                if (_cbBuf - (pchSeek - _pBuf) > MAX_VARIABLE_NAME_SIZE*_nCharSize)
                    break;

                // there may be a real variable here we need to expand,
                // so increase our buffer size and read some more data.
                //
                // we may get re-allocated, so update pchStart!
                hr = _IncreaseBuffer(BUFFER_SIZE_INC, &pchStart, NULL);
                if (FAILED(hr))
                    break;
                pchSeek = pchStart;

                // read in more info -- this will be enough to complete
                // any partial variable name expansions
                DWORD dwTmp;
                ASSERT(_cbBufSize - _cbBuf - SIZEOF(WCHAR) > 0);
                hr = _pProt->Read(_pBuf + _cbBuf, _cbBufSize-_cbBuf-SIZEOF(WCHAR), &dwTmp);
                if (FAILED(hr) || dwTmp == 0)
                    break;
                _cbBuf += dwTmp;
                // Ensure proper null termination
                _pBuf[_cbBuf] = _pBuf[_cbBuf+1] = 0;
                continue;
            }


            // figure out what to expand to
            LPBYTE pszExp;
            BYTE b[2];

            b[0] = pchEnd[0];
            b[1] = pchEnd[1];
            pchEnd[0] = 0;
            pchEnd[1] = 0;
            int cbExp = _Expand(pchSeek + _nCharSize, &pszExp);
            pchEnd[0] = b[0];
            pchEnd[1] = b[1];

            if (!cbExp)
            {
                // if it's not a recognized variable, use the bytes as they are
                pchSeek = pchEnd;
                continue;
            }

            // cbVar = number of bytes being replaced (sizeof("%VARNAME%"))
            // pchSeek points to the starting percent sign and pchEnd to
            // the trailing percent sign, so we need to add one more
            // _nCharSize to include the trailing percent sign itself.
            int cbVar = (int)(pchEnd - pchSeek) + _nCharSize;

            if (_cbBuf - cbVar + cbExp  > _cbBufSize - SIZEOF(WCHAR))
            {
                hr = _IncreaseBuffer((_cbBuf - cbVar + cbExp) - (_cbBufSize - SIZEOF(WCHAR)), &pchSeek, &pchEnd);
                if (FAILED(hr))
                    break;
            }

            // move the bytes around!
            // cbSeek = the number of bytes before the first percent sign
            int cbSeek = (int)(pchSeek - _pBuf);
            ASSERT(_cbBuf - cbVar + cbExp <= _cbBufSize - SIZEOF(WCHAR));
            // Move the stuff after the %VARNAME% to its final home
            // Don't forget to move the artificial trailing NULLs too!
            MoveMemory(pchSeek + cbExp, pchEnd + _nCharSize, _cbBuf - cbSeek - cbVar + SIZEOF(WCHAR));

            // Insert the expansion
            MoveMemory(pchSeek, pszExp, cbExp);

            // on to the rest of the buffer...
            pchSeek = pchEnd + _nCharSize;
            _cbBuf = _cbBuf - cbVar + cbExp;

        } while (*pchSeek);

#ifdef DEBUG
        if (SIZEOF(char)==_nCharSize)
            TraceMsg(TF_EXPAND, "---> A[%s]", _pBuf);
        else
            TraceMsg(TF_EXPAND, "---> W[%hs]", _pBuf);
#endif
    }
    else
    {
        // we're at end of stream
        hr = S_FALSE;
    }

    return hr;
}


HRESULT CWebViewMimeFilter::Read(void *pv,ULONG cb,ULONG *pcbRead)
{
    HRESULT hr = S_OK;

    if (!_pProt)
    {
        hr = E_FAIL;
    }
    else
    {
        *pcbRead = 0;

        while (cb)
        {
            // if our buffer is empty, fill it
            if (_cbSeek == _cbBuf)
            {
                hr = _ReadAndExpandBuffer();
            }

            // do we have any data to copy?
            int cbLeft = _cbBuf - _cbSeek;
            if (SUCCEEDED(hr) && cbLeft > 0)
            {
                ULONG cbCopy = min(cb, (ULONG)cbLeft);

                memcpy(pv, &_pBuf[_cbSeek], cbCopy);

                pv = (LPVOID)(((LPBYTE)pv) + cbCopy);
                cb -= cbCopy;
                *pcbRead += cbCopy;
                _cbSeek += cbCopy;

                // do not return S_FALSE if some bytes were left unread
                if (cbCopy < (ULONG)cbLeft)
                    hr = S_OK;
            }
            else
            {
                ASSERT(FAILED(hr) || hr == S_FALSE);

                // nothing left to copy
                break;
            }
        }
    }
    return hr;
}
HRESULT CWebViewMimeFilter::Seek(
        LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition)
{
    return E_NOTIMPL;
}
HRESULT CWebViewMimeFilter::LockRequest(DWORD dwOptions)
{
    return S_OK;
}
HRESULT CWebViewMimeFilter::UnlockRequest()
{
    return S_OK;
}

// IInternetProtocolSink methods
HRESULT CWebViewMimeFilter::Switch(PROTOCOLDATA * pProtocolData)
{
    if (_pProtSink)
        return _pProtSink->Switch(pProtocolData);
    return E_FAIL;
}
HRESULT CWebViewMimeFilter::ReportProgress(ULONG ulStatusCode, LPCWSTR pwszStatusText)
{
    if (_pProtSink)
        return _pProtSink->ReportProgress(ulStatusCode, pwszStatusText);
    return E_FAIL;
}
HRESULT CWebViewMimeFilter::ReportData(DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    if (_pProtSink)
        return _pProtSink->ReportData(grfBSCF, ulProgress, ulProgressMax);
    return E_FAIL;
}
HRESULT CWebViewMimeFilter::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR pwszResult)
{
    if (_pProtSink)
        return _pProtSink->ReportResult(hrResult, dwError, pwszResult);
    return E_FAIL;
}


//IServiceProvider methods
HRESULT CWebViewMimeFilter::QueryService(REFGUID rsid, REFIID riid, void ** ppvObj)
{
    HRESULT             hr;
    IServiceProvider *  pSP;

    *ppvObj = NULL;

    hr = _pProtSink->QueryInterface(IID_IServiceProvider, (void **)&pSP);
    if (SUCCEEDED(hr))
    {
        hr = pSP->QueryService(rsid, riid, ppvObj);
        pSP->Release();
    }

    return hr;
}


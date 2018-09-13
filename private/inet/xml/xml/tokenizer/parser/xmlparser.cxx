/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "xmlparser.hxx"
#include "xmlstream.hxx"
#include "../net/urlstream.hxx"
#include "datatype.hxx"
#include "chartype.hxx"

// For message formatting...
#include "core/lang/string.hxx"
#include "core/util/resources.hxx"

#include <objbase.h>

DeclareTag(tagParserCallback, "XML Parser", "callbacks");
DeclareTag(tagParserError, "XML Parser", "errors");
#define CRITICALSECTIONLOCK CSLock lock(&_cs);

extern HRESULT UrlOpenAllowed(LPCWSTR pwszUrl, LPCWSTR pwszBaseUrl, BOOL fDTD);

#ifdef UNIX
#ifndef POSSIBLY_FOSSIL_CODE
// This was a hack to fix this calling the wrong delete
// REVIEW BUGBUG
void XMLParser::operator delete( void * p )
{
    MemFree( p );
}
#endif // FOSSIL_CODE
#endif // UNIX

//extern char* WideToAscii(const WCHAR* string);

// --------------------------------------------------------------------
// A little helper class for setting a boolean flag and clearing it
// on destruction.
class BoolLock
{
    bool* _pFlag;
public:
    BoolLock(bool* pFlag)
    {
        _pFlag = pFlag;
        *pFlag = true;
    }
    ~BoolLock()
    {
        *_pFlag = false;
    }
};

#define breakhr(a) hr = (a); if (hr != S_OK) break;
extern HINSTANCE g_hInstance;

extern "C"{

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

};

// Since we cannot use the SHLWAPI wnsprintfA function...
int DecimalToBuffer(long value, char* buffer, int j, long maxdigits)
{
    long max = 1;
    for (int k = 0; k < maxdigits; k++)
        max = max * 10;
    if (value > (max*10)-1)
        value = (max*10)-1;
    max = max/10;
    for (int i = 0; i < maxdigits; i++)
    {
        long digit = (value / max);
        value -= (digit * max);
        max /= 10;
        buffer[i+j] = '0' + (char)digit;
    }
    buffer[i+j]=0;

    return i+j;
}

int StrToBuffer(const WCHAR* str, WCHAR* buffer, int j)
{
    while (*str != NULL)
    {
        buffer[j++] = *str++;
    }
    return j;
}

/*********
WCHAR* StringAdd(WCHAR* a, WCHAR* b)
{
    long len1 = _tcslen(a);
    long len2 = _tcslen(b);
    WCHAR* result = new_ne WCHAR[len1+len2+1];
    if (result != NULL)
    {
        ::memcpy(result, a, len1 * sizeof(WCHAR*));
        ::memcpy(&result[len1], b, len2 * sizeof(WCHAR*));
        result[len1+len2] = 0;
    }
    return result;
}
************/

#define PUSHNODEINFO(pNodeInfo)\
    if (_cNodeInfoAllocated == _cNodeInfoCurrent)\
    {\
        checkhr2(GrowNodeInfo());\
    }\
    _paNodeInfo[_cNodeInfoCurrent++] = _pCurrent;

#if 0
/************************  code bloat unacceptable ******************************
// ============ BEGIN HACK
// These are copied from messages.mc.  I have to do this because including messages.h
// would clash with <xmlparser.h>
//#define XML_I_LINENUMBER                 0x400CE590L
//#define XML_I_LINEPOSITION               0x400CE591L
// ============ END HACK

//==============================================================================
// This function calls FormatMessageA and converts the result to unicode.
// This is so that it works on Win95 !!
// For convenience the va_list of arguments are WCHAR arguments and this function
// converts them to ascii for you.
typedef char* PCHAR;

WCHAR* FormatMessageInternal(
    LPCVOID lpSource,  // pointer to message source
    DWORD dwMessageId,  // requested message identifier
    WCHAR* arg1, ...)  // variable list of args.
{
    // Caller assumes the arguments are wide chars, so now we have to convert
    // them to ascii.
    char* temp = NULL;
    DWORD rc = 0;
    WCHAR* result = NULL;
    PCHAR* args = NULL;
    int argcount = 0;
    int argsize = 0;
    va_list arglist;

    DWORD dwFlags = (lpSource == NULL) ? FORMAT_MESSAGE_FROM_SYSTEM : FORMAT_MESSAGE_FROM_HMODULE;
    dwFlags |= FORMAT_MESSAGE_ALLOCATE_BUFFER;

    // First we count number of args
    va_start(arglist, arg1);
    for (WCHAR* s = arg1; s; s = va_arg(arglist, WCHAR *))
    {
        argcount++;
    }
    va_end(arglist);
    va_start(arglist, arg1);

    // Then we pack the args into an array and convert them to ascii.
    if (argcount > 0)
    {
        dwFlags |= FORMAT_MESSAGE_ARGUMENT_ARRAY;
        args = new_ne PCHAR[argcount+1];
        if (args == NULL)
            goto CleanUp;

        argcount=0;
        for (WCHAR* s = arg1; s; s = va_arg(arglist, WCHAR *))
        {
            args[argcount] = WideToAscii(s);
            if (args[argcount] == NULL)
                goto CleanUp; // out of memory.
            argcount++;
        }
        args[argcount] = NULL; // NULL terminate the array of args.
    }

    rc = ::FormatMessageA(dwFlags, lpSource, dwMessageId,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPSTR)&temp, 0, args);

    if (rc > 0)
    {
        int length = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, temp, rc, NULL, 0);

        result = new_ne WCHAR[length+1];
        if (result == NULL)
            goto CleanUp;

        rc = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, temp, rc, result, length);
        result[rc] = 0; // NULL terminate
    }

CleanUp:
    va_end(arglist);
    if (temp) ::LocalFree(temp);

    // Now delete the array of arguments that we converted to ascii.
    while (argcount > 0)
    {
        delete[] args[--argcount];
    }
    delete [] args;
    return result;
}
**********************/
#endif

const USHORT STACK_INCREMENT=10;

//------------------------------------------------------------------------
XMLParser::XMLParser()
:   _pDownloads(1), _pStack(STACK_INCREMENT), 
    _reThreadModel(MultiThread) // make sure this is thead safe (as it always has been in the past).
{
    ctorInit();
}

#ifdef RENTAL_MODEL
XMLParser::XMLParser(RentalEnum re)
: _pDownloads(1), _pStack(STACK_INCREMENT), _reThreadModel(re)
{
    ctorInit();
}
#endif

void
XMLParser::ctorInit()
{
    InitializeCriticalSection(&_cs);

    _pTokenizer = NULL;
    _pCurrent = NULL;
    _lCurrentElement = 0;
    _paNodeInfo = NULL;
    _cNodeInfoAllocated = _cNodeInfoCurrent = 0;
    _pdc = NULL;
    _usFlags = 0;
    _fCaseInsensitive = false;
    _bstrError = NULL;
//    _fTokenizerChanged = false;
    _fRunEntryCount = 0;
    _pszSecureBaseURL = NULL;
    _pszCurrentURL = NULL;
    _pszBaseURL = NULL;
    _fInLoading = false;
    _fInsideRun = false;
    _fFoundDTDAttribute = false;
    _cAttributes = 0;
    _pRoot = NULL;
    _fAttemptedURL = NULL;
    _fLastError = 0;
    _fStopped = false;
    _fSuspended = false;
    _fStarted = false;
    _fWaiting = false;
    _fIgnoreEncodingAttr = false;
    _dwSafetyOptions = 0;

    ::IncrementComponents();

    // rest of initialization done in the init() method.

    //EnableTag(tagParserCallback, TRUE);
    //EnableTag(tagParserError, TRUE);
}

XMLParser::~XMLParser()
{
    {
        CRITICALSECTIONLOCK;
        Reset();

        // Cleanup tagname buffers in context for good this time...
        for (long i = _pStack.size()-1; i>=0; i--)
        {
            MY_XML_NODE_INFO* pNodeInfo = _pStack[i];
            if (pNodeInfo->_pwcTagName != NULL)
            {
                delete [] pNodeInfo->_pwcTagName;
                pNodeInfo->_pwcTagName = NULL;
                pNodeInfo->_ulBufLen = 0;
            }
            // NULL out the node pointer in case it point's to a GC'd object :-)
            pNodeInfo->pNode = NULL;
        }
        delete _pszSecureBaseURL;
        delete _pszCurrentURL;

        delete[] _paNodeInfo;
        ::DecrementComponents();
    }
    DeleteCriticalSection(&_cs);
}

HRESULT STDMETHODCALLTYPE
XMLParser::QueryInterface(REFIID riid, void ** ppvObject)
{
    STACK_ENTRY;
    // Since this one class implements both IXMLNodeSource and
    // IXMLParser, we must override QueryInterface since the
    // IUnknown template doesn't know about the IXMLNodeSource
    // interface.

    HRESULT hr = S_OK;
    if (riid == IID_IXMLNodeSource || riid == IID_Parser)
    {
        *ppvObject = static_cast<IXMLNodeSource*>(this);        
        AddRef();
    }
    else
    {
        hr = _unknown<IXMLParser, &IID_IXMLParser>::QueryInterface(riid, ppvObject);
    }
    return hr;
}

ULONG STDMETHODCALLTYPE
XMLParser::AddRef( void)
{
    STACK_ENTRY;
    return _unknown<IXMLParser, &IID_IXMLParser>::AddRef();
}

ULONG STDMETHODCALLTYPE
XMLParser::Release( void)
{
    STACK_ENTRY;
    return _unknown<IXMLParser, &IID_IXMLParser>::Release();
}

#define checknull(a) if (!(a)) { hr = E_OUTOFMEMORY; goto error; }

HRESULT STDMETHODCALLTYPE
XMLParser::SetURL(
            /* [in] */ const WCHAR* pszBaseUrl,
            /* [in] */ const WCHAR* pszRelativeUrl,
            /* [in] */ BOOL async)
{
    CRITICALSECTIONLOCK;
    STACK_ENTRY_MODEL(_reThreadModel);

    return PushURL(pszBaseUrl, pszRelativeUrl,
                (async == TRUE),// async
                true,           // new tokenizer
                false,          // is dtd
                false,          // is entity
                false           // is parameter entity
             );
}

HRESULT
XMLParser::PushURL(
    /* [in] */ const WCHAR* pszBaseUrl,
    /* [in] */ const WCHAR* pszRelativeUrl,
    /* [in] */ bool async,
    /* [in] */ bool tokenizer,
    /* [in] */ bool dtd,
    /* [in] */ bool fentity,
    /* [in] */ bool fpe)
{
    HRESULT hr = S_OK;
    URL url;
    URLStream* stream = NULL;
    const WCHAR* pszSecureBaseURL = NULL;

    if (NULL == pszRelativeUrl)
    {
        hr = E_INVALIDARG;
        goto cleanup;
    }

    if (_pDownloads.used() == 0)
        init();

    if (pszBaseUrl == NULL)
    {
        pszBaseUrl = _pszBaseURL;
    }

    stream = new_ne URLStream(this, dtd);
    if (stream == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    if (tokenizer)
    {
        checkerr(PushTokenizer(stream));
        if (dtd)
        {
            _pTokenizer->SetDTD(true);
        }
    }
    else
    {
        checkerr(PushDownload(stream, NULL));
    }
    _pdc->_fAsync = async;
    _pdc->_fDTD = dtd;
    _pdc->_fEntity = fentity;
    _pdc->_fPEReference = fpe;

    checknull(_pdc->_pURLStream);

    checkerr(url.set(pszRelativeUrl, _pszCurrentURL, pszBaseUrl));

    pszSecureBaseURL = getSecureBaseURL();
    if (! pszSecureBaseURL && _dwSafetyOptions) 
        pszSecureBaseURL = pszBaseUrl;
    checkerr(url.setSecureBase( pszSecureBaseURL )); // can be different from pszBaseUrl.

    checkerr(SetCurrentURL(url.getResolved()));

    _fInLoading = true;
    // WARNING: this must be done last because sometimes the callback bounces
    // right back immediately and calls Run() on the parser !!
    hr = _pdc->_pURLStream->Open(&url, async ? URLStream::ASYNCREAD : URLStream::SYNCREAD);
    _fInLoading = false;
    if (FAILED(hr))
    {
        _fAttemptedURL = ::SysAllocString(url.getRelative()); // save this for error info.
        goto error;
    }

    hr = S_OK;
    goto cleanup;
error:
    PopDownload();
cleanup:
    if (stream) stream->Release();
    _fLastError = hr;
    return hr;
}

HRESULT STDMETHODCALLTYPE
XMLParser::Load(
        /* [in] */ BOOL fFullyAvailable,
        /* [in] */ IMoniker __RPC_FAR *pimkName,
        /* [in] */ LPBC pibc,
        /* [in] */ DWORD grfMode)
{
    STACK_ENTRY_MODEL(_reThreadModel);
    CRITICALSECTIONLOCK;

    HRESULT hr = S_OK;
    URL url;
    URLStream* stream = NULL;

    if (NULL == pimkName)
    {
        hr = E_INVALIDARG;
        goto cleanup;
    }

    if (_pDownloads.used()== 0)
        init();


    // In this case we register a special call back to pump
    // the parser during download.  Typically in this case the
    // caller also has a registered callback in order to monitor
    // the download progress.
    stream = new_ne URLStream(this);
    if (stream == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    checkerr(PushTokenizer(stream));

    checknull(_pdc->_pURLStream);

    checkerr(ExtractURL(pimkName, pibc, &url));

    _fInLoading = true;
    _pdc->_fAsync = true;

    // WARNING: this must be done last because sometimes the callback bounces
    // right back immediately and calls Run() on the parser !!
    hr = _pdc->_pURLStream->Open(pimkName, pibc, &url, URLStream::ASYNCREAD);

    _fInLoading = false;
    if (FAILED(hr))
    {
        _fAttemptedURL = ::SysAllocString(url.getRelative()); // save this for error info.
        goto error;
    }
    goto cleanup;
error:
    PopDownload();
cleanup:
    if (stream) stream->Release();
    return hr;
}

HRESULT STDMETHODCALLTYPE
XMLParser::LoadDTD(
            /* [in] */ const WCHAR* pszBaseUrl,
            /* [in] */ const WCHAR* pszRelativeUrl)
{
    HRESULT hr;
    STACK_ENTRY_MODEL(_reThreadModel);
    CRITICALSECTIONLOCK;
    BOOL async = FALSE;
    // If previous download is async, then make this one async too.
    if (_pdc && _pdc->_fAsync)
        async = TRUE;
    //
    // Have to do this first because PushURL may invoke another downloading and calls back to the parser immediately
    //
    _cDTD++;
    hr = _pFactory->NotifyEvent(this, XMLNF_STARTDTD);
    if (SUCCEEDED(hr))
    {
        hr = PushURL(pszBaseUrl, pszRelativeUrl,
                (async == TRUE),// async
                true,           // new tokenizer
                true,           // is dtd
                false,          // is entity
                false           // is parameter entity
             );
        _fTokenizerChanged = false;
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE
XMLParser::SetInput(
            /* [in] */ IUnknown __RPC_FAR *pStm)
{
    if (pStm == NULL)
        return E_INVALIDARG;

    STACK_ENTRY_MODEL(_reThreadModel);
    CRITICALSECTIONLOCK;
    if (_pDownloads.used() == 0)
        init();
    HRESULT hr = S_OK;

    checkhr2(PushTokenizer(NULL));

    // Get the url path
    // Continue even if we cannot get it
    STATSTG stat;
    IStream * pStream = NULL;
    memset(&stat, 0, sizeof(stat));
    hr = pStm->QueryInterface(IID_IStream, (void**)&pStream);
    if (SUCCEEDED(hr))
    {
        hr = pStream->Stat(&stat, 0);
        if (SUCCEEDED(hr) && stat.pwcsName != NULL)
        {
            SetCurrentURL(stat.pwcsName);

            WCHAR* pszSecureBaseURL = getSecureBaseURL();
            if (*stat.pwcsName != 0 && pszSecureBaseURL != NULL && *pszSecureBaseURL != 0)
            {
                // Then we can apply security to IStream also - but first we have
                // to turn the pwcsName into a valid URL.
                // BUGBUG - If we can't make a valid URL then we can't verify security.
                URL url;
                url.set(stat.pwcsName,NULL,NULL);
                hr = UrlOpenAllowed(url.getResolved(), pszSecureBaseURL, FALSE);
            }

            CoTaskMemFree(stat.pwcsName);
        }
        else
        {
            hr = S_OK; // we don't care if stat fails.
        }
        if (SUCCEEDED(hr))
        {
            hr = PushStream(pStream, false);
        }
        pStream->Release();
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE
XMLParser::PushData(
#ifdef UNIX
            //
            // The IDL compiler under Unix is outputting a
            // /* [in] */ const unsigned char *pData,
            // instead of
            // /* [in] */ const char *pData,
            // See also xmlparser.hxx
            //
            /* [in] */ const unsigned char *pData,
#else
            /* [in] */ const char __RPC_FAR *pData,
#endif
            /* [in] */ ULONG ulChars,
            /* [in] */ BOOL fLastBuffer)
{
    STACK_ENTRY_MODEL(_reThreadModel);
    CRITICALSECTIONLOCK;
    HRESULT hr;

    if (NULL == pData && (ulChars != 0))
    {
        return E_INVALIDARG;
    }

    if (_pTokenizer == NULL)
    {
        init();
        checkhr2(PushTokenizer(NULL));
    }
    return _pTokenizer->AppendData((const BYTE*)pData, ulChars, fLastBuffer);
}

HRESULT STDMETHODCALLTYPE
XMLParser::LoadEntity(
    /* [in] */ const WCHAR* pszBaseUrl,
    /* [in] */ const WCHAR* pszRelativeUrl,
    /* [in] */ BOOL fpe)
{
    STACK_ENTRY_MODEL(_reThreadModel);
    CRITICALSECTIONLOCK;
    HRESULT hr;
    BOOL async = FALSE;
    // If previous download is async, then make this one async too.
    if (_pdc && _pdc->_fAsync)
        async = TRUE;
    // then text is just a string that needs to be parsed.
    hr = PushURL(pszBaseUrl, pszRelativeUrl,
                (async == TRUE),// async
                (fpe != TRUE),  // new tokenizer
                (fpe == TRUE),  // is dtd
                true,           // is entity
                (fpe == TRUE)   // is parameter entity
             );
    if (hr == S_OK)
    {
        hr = _pFactory->NotifyEvent(this, XMLNF_STARTENTITY);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
XMLParser::ParseEntity(
    /* [in] */ const WCHAR* pwcText,
    /* [in] */ ULONG ulLen,
    /* [in] */ BOOL fpe)
{
    STACK_ENTRY_MODEL(_reThreadModel);
    CRITICALSECTIONLOCK;
    HRESULT hr;

    if (!fpe)
    {
        // Then we need a new tokenizer to parse it.
        checkhr2(PushTokenizer(NULL));
        _pdc->_fEntity = true;
        _pdc->_fPEReference = (fpe == TRUE);

        checkhr2(_pTokenizer->AppendData((const BYTE*)s_ByteOrderMark, sizeof(s_ByteOrderMark), FALSE));
        hr = _pTokenizer->AppendData((const BYTE*)pwcText, ulLen * sizeof(WCHAR), TRUE);
    }
    else
    {
        hr = _pTokenizer->InsertData((const WCHAR *)pwcText, ulLen, true);
    }
    if (hr == S_OK)
    {
        hr = _pFactory->NotifyEvent(this, XMLNF_STARTENTITY);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE
XMLParser::ExpandEntity(
    /* [in] */ const WCHAR* pwcText,
    /* [in] */ ULONG ulLen)
{
    STACK_ENTRY_MODEL(_reThreadModel);
    CRITICALSECTIONLOCK;

    return _pTokenizer->InsertData((const WCHAR *)pwcText, ulLen, false);
}

HRESULT STDMETHODCALLTYPE
XMLParser::SetRoot(
            /* [in] */ PVOID pRoot)
{
    STACK_ENTRY;
    CRITICALSECTIONLOCK;
    _pRoot = pRoot;
    _pNode = pRoot;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
XMLParser::GetRoot(
    /* [in] */ PVOID __RPC_FAR * ppRoot)
{
    STACK_ENTRY;
    CRITICALSECTIONLOCK;
    if (ppRoot == NULL) return E_INVALIDARG;
    *ppRoot = _pRoot;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
XMLParser::SetFactory(
            /* [in] */ IXMLNodeFactory __RPC_FAR *pNodeFactory)
{
    STACK_ENTRY;
    CRITICALSECTIONLOCK;
    _pFactory = pNodeFactory;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
XMLParser::GetFactory(
            /* [out] */ IXMLNodeFactory** ppNodeFactory)
{
    if (ppNodeFactory == NULL) return E_INVALIDARG;
    if (_pFactory)
    {
        *ppNodeFactory = _pFactory;
        (*ppNodeFactory)->AddRef();
    }
    else
    {
        *ppNodeFactory = NULL;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
XMLParser::Run(
            /* [in] */ long lChars)
{
    STACK_ENTRY_MODEL(_reThreadModel);
    CRITICALSECTIONLOCK;

    XML_NODE_INFO   info;
    XML_NODE_INFO*  aNodeInfo[1];

    HRESULT         hr = 0;
    USHORT          numRecs;

    bool            fIsAttribute = false;
    WCHAR           wc;
    bool            stop;

    if (_fSuspended)
        _fSuspended = FALSE; // caller must want to resume.

    if (_pFactory == NULL)
    {
        return E_FAIL;
    }

    if (_fStopped)
    {
        return XML_E_STOPPED;
    }

    if (_pTokenizer == NULL)
    {
        if (_fLastError != S_OK)
            return _fLastError;
        else
            // must be _fStarted == false
            return XMLPARSER_IDLE;
    }

    // Check for recurrsive entry and whether caller actually
    // wants anything parsed.
    if (_fInsideRun || lChars == 0)
        return E_PENDING;

    BoolLock flock(&_fInsideRun);

    if (_fLastError != 0)
    {
        // one more chance to cleanup the parser stack.
        hr = _fLastError;
        goto cleanup_stack;
    }

    if (! _fStarted)
    {
        _fStarted = true;
        hr = _pFactory->NotifyEvent(this, XMLNF_STARTDOCUMENT);
        if (_fStopped)      // watch for onReadyStateChange handlers 
            return S_OK;    // fussing with the parser state.
    }

    _fWaiting = false;
    if (_fPendingBeginChildren)
    {
        _fPendingBeginChildren = false;
        hr = _pFactory->BeginChildren(this, (XML_NODE_INFO*)_pCurrent);
    }
    if (_fPendingEndChildren)
    {
        _fPendingEndChildren = false;
        hr = _pFactory->EndChildren(this, TRUE, (XML_NODE_INFO*)_pCurrent);
        if (!hr)
            hr = pop(); // no match needed
    }

    info.dwSize = sizeof(XML_NODE_INFO);
    info.dwType = XMLStream::XML_PENDING;
    info.dwSubType = 0;
    info.pwcText = NULL;
    info.ulLen = 0;
    info.ulNsPrefixLen = 0;
    info.pNode = NULL;
    info.pReserved = NULL;
    aNodeInfo[0] = &info;

more:
    _fRunEntryCount++; // count of callers inside this loop...

    while (hr == 0 && ! _fSuspended)
    {
        info.dwSubType = 0;

        // The XMLStream error codes have been aligned with the
        // XMLParser error code so no mapping is necessary.
        hr = _pTokenizer->GetNextToken(&info.dwType, (const WCHAR  **)&info.pwcText, (long*)&info.ulLen, (long*)&info.ulNsPrefixLen);
        if (hr == E_PENDING)
        {
            _fWaiting = true;
            break;
        }

        if (! _fFoundNonWS &&
                info.dwType != XMLStream::XML_PENDING &&
                info.dwType != XML_WHITESPACE &&
                info.dwType != XML_XMLDECL)
        {
            _fFoundNonWS = true;
        }

        // Now the NodeType is the same as the XMLToken value.  We set
        // this up by aligning the two enums.
        switch (info.dwType)
        {
        case 0:
            if (hr == XML_E_INVALIDSWITCH  && _fIgnoreEncodingAttr)
            {
                hr = 0; // ignore it and continue on.
            }
            break;
            // --------- Container Nodes -------------------
        case XML_XMLDECL:
            if (_fFoundNonWS && ! _fIE4Mode)  // IE4 allowed this...
            {
                hr = XML_E_BADXMLDECL;
                break;
            }
//            _fFoundNonWS = true;
            goto containers;

        case XML_XMLSPACE:
        case XML_XMLLANG:
            info.dwSubType = info.dwType;        // pass this as a subtype.
            info.dwType = XML_ATTRIBUTE;
            // fall through
        case XML_ATTRIBUTE:
            fIsAttribute = true;
            goto containers;

        case XML_VERSION:
            info.dwSubType = info.dwType;
            info.dwType = XML_ATTRIBUTE;
            _fGotVersion = true;
            fIsAttribute = true;
            goto containers;

        case XML_STANDALONE:
        case XML_ENCODING:
            if (! _fGotVersion && _pDownloads.used() == 1)
            {
                hr = XML_E_EXPECTING_VERSION;
                break;
            }
            if (info.dwType == XML_STANDALONE)
            {
                if (_pDownloads.used() > 1)
                {
                    hr = XML_E_UNEXPECTED_STANDALONE;
                    break;
                }
            }
            info.dwSubType = info.dwType;
            info.dwType = XML_ATTRIBUTE;
            fIsAttribute = true;
            goto containers;

        case XML_NS:
            info.dwSubType = info.dwType;
            info.dwType = XML_ATTRIBUTE;
            fIsAttribute = true;
            {
                MY_XML_NODE_INFO* ptr = _pStack[_lCurrentElement];// tell namespacenodefactory
                ptr->pReserved = (void*)0x1;               // that we found an XML_NS attribute.
            }
            goto containers;

        case XML_ELEMENT:
        case XML_PI:
containers:
            if (_fRootLevel)
            {
                // Special rules apply for root level tags.
                if (info.dwType == XML_ELEMENT)
                {
                     // This is a root level element.
                     if (! _fFoundRoot)
                     {
                         _fFoundRoot = true;
                     }
                     else
                     {
                         hr = XML_E_MULTIPLEROOTS;
                         break;
                     }
                }
                else if (info.dwType != XML_PI &&
                         info.dwType != XML_XMLDECL &&
                         info.dwType != XML_DOCTYPE)
                {
                    hr = XML_E_INVALIDATROOTLEVEL;
                    break;
                }
            }

            info.fTerminal = FALSE;

            if (fIsAttribute)
            {
                breakhr( pushAttribute(info));
                fIsAttribute = false;
            }
            else
            {
                breakhr( push(info));
            }
            break;

            // --------- DTD Container Nodes -------------------
        case XML_DOCTYPE:
            if (_fSeenDocType)
            {
                hr = (HRESULT)XML_E_DUPLICATEDOCTYPE;
                break;
            }
            _fSeenDocType = true;
            if (_cDTD)
            {
                hr = XML_E_DOCTYPE_IN_DTD;
                break;
            }
            else if (_fFoundRoot)
            {
                hr = XML_E_DOCTYPE_OUTSIDE_PROLOG;
                break;
            }
            goto dtdcontainers;

        case XML_SYSTEM:
        case XML_PUBLIC:
        case XML_NDATA:
            info.dwSubType = info.dwType;
            info.dwType = XML_DTDATTRIBUTE;
            fIsAttribute = true;
            goto dtdcontainers;

            // Put all DTD related container nodes here...
        case XML_PENTITYDECL:
            info.dwSubType = info.dwType;        // make this a subtype of
            info.dwType = XML_ENTITYDECL; // XML_ENTITYDECL
                // fall through
        case XML_ENTITYDECL:
        case XML_ELEMENTDECL:
        case XML_ATTLISTDECL:
        case XML_NOTATION:
        case XML_GROUP:
        case XML_INCLUDESECT:
            if (_cDTD == 0)
            {
                hr = (HRESULT)XML_E_DTDELEMENT_OUTSIDE_DTD;
                break;
            }

dtdcontainers:
            // Cannot buffer attributes for DTD declarations because of PARAMETER
            // ENTITIES
            if (_fFoundDTDAttribute)
            {
                breakhr(popDTDAttribute());
            }
            info.fTerminal = FALSE;
            if (fIsAttribute)
            {
                fIsAttribute = false;
                // NOTE - this code is setup in a way to make it possible for the
                // node factory to return E_PENDING to force the Run to return
                // but then be able to call Run() later on and pick up where it
                // left off.  This means all the state info needed to re-enter Run
                // must be saved away.
                hr =  _pFactory->CreateNode(this, _pNode, 1, aNodeInfo);
                HRESULT hr2 = pushDTDAttribute(info);
                if (FAILED(hr2)) hr = hr2; // this is more serious that E_PENDING.
                _pNode = info.pNode;
                info.pNode = NULL;
            }
            else
            {
                hr =  _pFactory->CreateNode(this, _pNode, 1, aNodeInfo);
                HRESULT hr2 = push(info);
                if (FAILED(hr2)) hr = hr2; // this is more serious than E_PENDING.
                _pNode = info.pNode;
                info.pNode = NULL;
            }
            break;

            // --------- Terminal Nodes -------------------
        case XML_AT_CDATA:
        case XML_AT_ID:
        case XML_AT_IDREF:
        case XML_AT_IDREFS:
        case XML_AT_ENTITY:
        case XML_AT_ENTITIES:
        case XML_AT_NMTOKEN:
        case XML_AT_NMTOKENS:
        case XML_AT_NOTATION:
            // Make these subtypes of XML_ATTTYPE.
            info.dwSubType = info.dwType;
            info.dwType = XML_ATTTYPE;
            goto dtdterminals;

        case XML_AT_REQUIRED:
        case XML_AT_IMPLIED:
        case XML_AT_FIXED:
            // Make these subtypes of XML_ATTPRESENCE.
            info.dwSubType = info.dwType;
            info.dwType = XML_ATTPRESENCE;
            goto dtdterminals;

        case XML_EMPTY:
        case XML_ANY:
        case XML_MIXED:
        case XML_SEQUENCE:
        case XML_CHOICE:
        case XML_STAR:
        case XML_PLUS:
        case XML_QUESTIONMARK:
            info.dwSubType = info.dwType;  // make these subtypes of XML_MODEL
            info.dwType = XML_MODEL;
            goto dtdterminals;

            // Put all DTD terminal nodes here...
        case XML_IGNORESECT:
        case XML_PEREF:
        case XML_ATTDEF:
dtdterminals:
            if (_cDTD == 0)
            {
                hr = (HRESULT)XML_E_DTDELEMENT_OUTSIDE_DTD;
                break;
            }
            // goto terminals;
            // fall through

        case XML_PCDATA:
        case XML_CDATA:
terminals:
            // Special rules apply for root level tags.
            if (_fRootLevel)
            {
                hr = XML_E_INVALIDATROOTLEVEL;
                break;
            }
            // fall through
        case XML_COMMENT:
        case XML_WHITESPACE:
tcreatenode:
            info.fTerminal = TRUE;
            if (_cAttributes != 0)
            {
                // We are inside the attribute list, so we need to push this.
                hr = pushAttributeValue(info);
                break;
            }
            hr = _pFactory->CreateNode(this, _pNode, 1, aNodeInfo);
            info.pNode = NULL;
            break;

        case XML_NAME:
        case XML_NMTOKEN:
        case XML_STRING:
        case XML_DTDSUBSET:
            info.fTerminal = TRUE;
            hr = _pFactory->CreateNode(this, _pNode, 1, aNodeInfo);
            info.pNode = NULL;
            if (info.dwType == XML_DTDSUBSET)
            {
                _cDTD--;
                breakhr(_pFactory->NotifyEvent(this, XMLNF_ENDDTDSUBSET));
            }
            break;

        case XML_ENTITYREF:
            if (_fRootLevel)
            {
                hr = XML_E_INVALIDATROOTLEVEL;
                break;
            }

            // We handle builtin entities and char entities in xmlstream
            // so these must be user defined entity, so treat it like a regular terminal node.
            goto terminals;
            break;

        case XMLStream::XML_BUILTINENTITYREF:
        case XMLStream::XML_HEXENTITYREF:
        case XMLStream::XML_NUMENTITYREF:
            // pass real entityref type as subtype so we can publish these
            // subtypes eventually.
            info.dwSubType = info.dwType; // XML_ENTITYREF;
            info.dwType = XML_PCDATA;

            if (_cAttributes == 0)
            {
                goto tcreatenode;
            }

            // We are inside the attribute list, so we need to push this.
            info.fTerminal = TRUE;
            hr = pushAttributeValue(info);
            if (SUCCEEDED(hr))
            {
                hr = CopyText(_pCurrent);
            }
            break;

        case XMLStream::XML_TAGEND:
            numRecs = 1+_cAttributes;
            if (_cAttributes != 0)  // this is safe because _rawstack does NOT reclaim
            {                       // the popped stack entries.
                popAttributes();
            }
            hr = _pFactory->CreateNode(this, _pNode, numRecs, (XML_NODE_INFO **)&_paNodeInfo[_lCurrentElement]);
            _pNode = _pCurrent->pNode;
            if (FAILED(hr))
            {
                _fPendingBeginChildren = true;
                break;
            }
            breakhr( _pFactory->BeginChildren(this, (XML_NODE_INFO*)_pCurrent));
            break;

            // The ENDXMLDECL is like EMPTYENDTAGs since we've been
            // buffering up their attributes, and we have still got to call CreateNode.
        case XMLStream::XML_ENDXMLDECL:
            _fGotVersion = false; // reset back to initial state.
            // fall through.
        case XMLStream::XML_EMPTYTAGEND:
            numRecs = 1+_cAttributes;
            if (_cAttributes != 0)
            {
                popAttributes();
            }
            hr = _pFactory->CreateNode(this, _pNode, numRecs, (XML_NODE_INFO **)&_paNodeInfo[_lCurrentElement]);
            if (FAILED(hr))
            {
                _fPendingEndChildren = true;
                break;
            }
            breakhr(_pFactory->EndChildren(this, TRUE, (XML_NODE_INFO*)_pCurrent));
            breakhr(pop()); // no match needed
            break;

        case XMLStream::XML_ENDDECL:
            if (_fFoundDTDAttribute)
            {
                breakhr(popDTDAttribute());
            }
            breakhr(_pFactory->EndChildren(this, TRUE, (XML_NODE_INFO*)_pCurrent));
            breakhr(pop()); // no match needed
            break;

        // ---------------- end of a container -----------------
        case XMLStream::XML_ENDPI:
            info.dwType = XML_CDATA;
            info.dwSubType = XML_PI;
            info.fTerminal = TRUE;
            breakhr(pushAttribute(info));
            hr = _pFactory->CreateNode(this, _pNode, 2, (XML_NODE_INFO **)&_paNodeInfo[_cNodeInfoCurrent-2]);
            popAttribute(); // pop the PCDATA node.
            if (FAILED(hr))
            {
                _fPendingEndChildren = true;
                break;
            }
            breakhr(_pFactory->EndChildren(this, FALSE, (XML_NODE_INFO*)_pCurrent));
            breakhr(pop()); // pop the XML_PI node.
            break;

        case XMLStream::XML_CLOSEPAREN:
        case XMLStream::XML_ENDCONDSECT:
            breakhr(_pFactory->EndChildren(this, FALSE, (XML_NODE_INFO*)_pCurrent));
            hr = pop(); // no tagname to match.
            break;

        case XMLStream::XML_ENDTAG:
            if (_pStack.used() == 0)
            {
                hr = XML_E_UNEXPECTEDENDTAG;
            }
            else
            {
                XML_NODE_INFO* pCurrent = (XML_NODE_INFO*)_pCurrent; // save current record
                breakhr(pop(info.pwcText, info.ulLen)); // check tag/match
                breakhr(_pFactory->EndChildren(this, FALSE, (XML_NODE_INFO*)pCurrent));
            }
            break;
        case XMLStream::XML_STARTDTDSUBSET:
            _cDTD++;
            if (_fFoundDTDAttribute)
            {
                popDTDAttribute();
            }
            breakhr( _pFactory->NotifyEvent(this, XMLNF_STARTDTDSUBSET));
            break;
        case XMLStream::XML_ENDPROLOG:
            // For top level document only, (not for DTD's or
            // entities), call EndProlog on the node factory.
            if (_fRootLevel &&
                    ! _pdc->_fEntity && ! _pdc->_fDTD)
                breakhr( _pFactory->NotifyEvent(this, XMLNF_ENDPROLOG));
            break;

        default:
            hr = E_FAIL;
            break;
        }
    }
    _fRunEntryCount--;

    stop = false;
    if (hr == XML_E_ENDOFINPUT)
    {
        hr = S_OK;
        bool inDTD = _pdc->_fDTD;
        bool inEntity = _pdc->_fEntity;
        bool inPEReference = _pdc->_fPEReference;

        if (inEntity && _pdc->_fDepth != _pStack.used())
        {
            // Entity itself was unbalanced.
            hr = ReportUnclosedTags(_pdc->_fDepth);
        }
        else if (PopDownload() == S_OK)
        {
            // then we must have just finished a DTD and we still have more to do
            // BUGBUG -- need to check that entity is well formed, i.e. no tags
            // left open.

            if (!inPEReference)
            {
                if (inEntity)
                {
                    hr = _pFactory->NotifyEvent(this, XMLNF_ENDENTITY);
                }
                else if (inDTD)
                {
                    hr = _pFactory->NotifyEvent(this, XMLNF_ENDDTD);
                    _cDTD--;
                }
            }
            if (FAILED(hr))
            {
                goto cleanup_stack;
            }

            // In a synchronous DTD download, there is another parser
            // parser Run() call on the stack above us, so let's return
            // back to that Run method so we don't complete the parsing
            // out from under it.
            if (_fRunEntryCount > 0)
                return S_OK;

            if (_fStopped)
                return S_OK;
            goto more;
        }
        else
        {
            if (_pStack.used() > 0)
            {
                hr = ReportUnclosedTags(0);
            }
            else if (! _fFoundRoot)
            {
                hr = XML_E_MISSINGROOT;
            }
            stop = true;
        }
    }

cleanup_stack:

    if (hr != S_OK && hr != E_PENDING)
    {
        stop = true;
        _fLastError = hr;

        // Pass all the XML_NODE_INFO structs to the Error function so the client
        // gets a chance to cleanup the PVOID pNode fields.
        HRESULT edr = _pFactory->Error(this, hr,
            (USHORT)(_paNodeInfo ? _lCurrentElement+1 : 0), (XML_NODE_INFO**)_paNodeInfo);
        if (edr != 0)
            _fLastError = hr;
    }

    if (stop && ! _fStopped)
    {
        TraceTag((tagParserError, "Parser stopping with hr %x", hr));
        _fLastError = hr;
        _fStopped = true;
        _fStarted = false;
        HRESULT edr;
        edr = _pFactory->NotifyEvent(this, XMLNF_ENDDOCUMENT);
        if (edr != 0)
        {
            hr = edr; // allow factory to change error code (except to S_OK)
            if (S_OK == _fLastError)
            {
                // Make sure the node factory always finds out about errors.
                edr = _pFactory->Error(this, hr, 0, NULL);
                if (edr != 0)
                    hr = edr;
            }
            _fLastError = hr;
        }
    }
    return hr;
}

HRESULT
XMLParser::popAttributes()
{
    // Now I pop all the attributes that were pushed for this tag.
    // I know we have at least one attribute.
    HRESULT hr;
    while (_cAttributes > 0)
    {
        popAttribute(); // no match needed
    }
    Assert(_pStack.used() == _lCurrentElement+1);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
XMLParser::GetParserState( void)
{
    CRITICALSECTIONLOCK;

    if (_fLastError != 0)
        return XMLPARSER_ERROR;

    if (_fStopped)
        return XMLPARSER_STOPPED;

    if (_fSuspended)
        return XMLPARSER_SUSPENDED;

    if (! _fStarted)
        return XMLPARSER_IDLE;

    if (_fWaiting)
        return XMLPARSER_WAITING;

    return XMLPARSER_BUSY;
}

HRESULT STDMETHODCALLTYPE
XMLParser::Abort(
            /* [in] */ BSTR bstrErrorInfo)
{
    int i;
    STACK_ENTRY_MODEL(_reThreadModel);

    // Have to set these before Critical Section to notify Run()
    _fStopped = true;
    _fSuspended = true; // force Run to terminate...

    CRITICALSECTIONLOCK;
    TraceTag((tagParserError, "Parser aborted - %ls", bstrErrorInfo));

    //BUGBUG: may need to check bstrErrorInfo is NULL or not 
    //        and the returned result so that we can report 
    //        E_OUTOFMEMORY error
    if (_bstrError) ::SysFreeString(_bstrError);
    _bstrError = ::SysAllocString(bstrErrorInfo);

    // abort all downloads
    for (i=_pDownloads.used()-1;  i>=0;  --i)
    {
        URLStream* stm = _pDownloads[i]->_pURLStream;
        if (stm)
            stm->Abort();
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
XMLParser::Suspend( void)
{
    _fSuspended = true; // force Run to suspend
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
XMLParser::Reset( void)
{
    STACK_ENTRY;
    CRITICALSECTIONLOCK;

    init();

    delete _pszCurrentURL;
    _pszCurrentURL = NULL;
    delete _pszBaseURL;
    _pszBaseURL = NULL;
    _pRoot = NULL;
    _pFactory = NULL;
    _pNode = NULL;
    if (_bstrError != NULL) ::SysFreeString(_bstrError);
    _bstrError = NULL;
    if (_fAttemptedURL != NULL) ::SysFreeString(_fAttemptedURL);
    _fAttemptedURL = NULL;
    return S_OK;
}

ULONG STDMETHODCALLTYPE
XMLParser::GetLineNumber( void)
{
    CRITICALSECTIONLOCK;
    if (_pTokenizer) return _pTokenizer->GetLine();
    else return 0;
}

ULONG STDMETHODCALLTYPE
XMLParser::GetLinePosition( void)
{
    CRITICALSECTIONLOCK;
    if (_pTokenizer) return _pTokenizer->GetLinePosition();
    else return 0;
}

ULONG STDMETHODCALLTYPE
XMLParser::GetAbsolutePosition( void)
{
    CRITICALSECTIONLOCK;
    if (_pTokenizer) return _pTokenizer->GetInputPosition();
    else return 0;
}


HRESULT STDMETHODCALLTYPE
XMLParser::GetLineBuffer(
            /* [out] */ const WCHAR __RPC_FAR *__RPC_FAR *ppwcBuf,
            /* [out] */ ULONG __RPC_FAR *pulLen,
            /* [out] */ ULONG __RPC_FAR *pulStartPos)
{
    if (pulLen == NULL || pulStartPos == NULL) return E_INVALIDARG;

    STACK_ENTRY;
    CRITICALSECTIONLOCK;
    if (_pTokenizer)
    {
        return _pTokenizer->GetLineBuffer(ppwcBuf, pulLen, pulStartPos);
    }
    *ppwcBuf = NULL;
    *pulLen = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
XMLParser::GetLastError( void)
{
    return _fLastError;
}

HRESULT STDMETHODCALLTYPE
XMLParser::GetErrorInfo(
            /* [out] */ BSTR __RPC_FAR *pbstrErrorInfo)
{
    HRESULT hr = S_OK;
    WCHAR* buffer = NULL;

    STACK_ENTRY_MODEL(_reThreadModel);
    CRITICALSECTIONLOCK;

    *pbstrErrorInfo = NULL;

    HRESULT errorid = GetLastError();
    if (_bstrError)
    {
        *pbstrErrorInfo = ::SysAllocString(_bstrError);
        if(*pbstrErrorInfo == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        goto DONE;
    }
    else if ((ULONG)errorid <= 0xC00CEFFF && (ULONG)errorid >= 0xC00CE000)
    {
        // MSXML error msg.
        TRY
        {
            String* s = Resources::FormatMessage(errorid, NULL);
            *pbstrErrorInfo = s->getBSTR();
            if(*pbstrErrorInfo == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            goto DONE;
            // buffer = ::FormatMessageInternal(g_hInstance, errorid, NULL);
        }
        CATCH
        {
            hr = ERESULT;
            goto DONE;
        }
        ENDTRY
    }
    else
    {
        // MSXML error msg.
        // System error
#if 0
/**************
        buffer = ::FormatMessageInternal(NULL, errorid, NULL);

        if (buffer == NULL && HRESULT_FACILITY(errorid) == FACILITY_INTERNET)
        {
            // maybe it's URLMON...
            HINSTANCE h = ::GetModuleHandleA("URLMON.DLL");
            buffer = ::FormatMessageInternal(h, errorid, NULL);

            if (_fAttemptedURL)
            {
                WCHAR* buf2 = ::FormatMessageInternal(g_hInstance, XML_E_RESOURCE,
                    _fAttemptedURL, NULL);

                if (buffer == NULL)
                {
                    buffer = buf2;
                }
                else
                {
                    WCHAR* buf3 = StringAdd(buffer, buf2);
                    delete[] buf2;
                    if (buf3 != NULL)
                    {
                        delete [] buffer;
                        buffer = buf3;
                    }
                }
            }
        }
**************/
#endif
        TRY
        {
            String* s = Resources::FormatSystemMessage(errorid);
            if (_fAttemptedURL)
            {
                String* rs = Resources::FormatMessage(XML_E_RESOURCE,
                    String::newString(_fAttemptedURL), NULL);
                s = String::add(s,rs,null);
            }
            *pbstrErrorInfo = s->getBSTR();
            if(*pbstrErrorInfo == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            goto DONE;
        }
        CATCH
        {
            hr = ERESULT;
            goto DONE;
        }
        ENDTRY
    }
    if (buffer == NULL)
    {
        *pbstrErrorInfo = NULL;
        hr = S_FALSE;
        goto DONE;
    }
    *pbstrErrorInfo = ::SysAllocString(buffer);
    delete[] buffer;
    if(*pbstrErrorInfo == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
DONE:
    return hr;
}

HRESULT STDMETHODCALLTYPE
XMLParser::SetFlags(
            /* [in] */ ULONG lFlags)
{
    _usFlags = (unsigned short)lFlags;
    _fCaseInsensitive = (lFlags & XMLFLAG_CASEINSENSITIVE) != 0;
    _fIE4Mode = (_usFlags & XMLFLAG_IE4QUIRKS) != 0;

    if (_pTokenizer != NULL)
        _pTokenizer->SetFlags(_usFlags);
    return S_OK;
}

ULONG STDMETHODCALLTYPE
XMLParser::GetFlags( )
{
    return _usFlags;
}

HRESULT STDMETHODCALLTYPE
XMLParser::GetURL(
            /* [out] */ const WCHAR __RPC_FAR *__RPC_FAR *ppwcBuf)
{
    if (ppwcBuf == NULL)
        return E_INVALIDARG;

    URL* url;

    //BUGBUG: should CoTaskAlloc ??
    if (_pszCurrentURL != NULL)
    {
        *ppwcBuf = _pszCurrentURL;
    }
    else if (_pdc != NULL && _pdc->_pURLStream && ((url = _pdc->_pURLStream->GetURL()) != NULL))
    {
        *ppwcBuf = url->getResolved();
    }
    else
    {
        *ppwcBuf = NULL;
    }
    return S_OK;
}


HRESULT STDMETHODCALLTYPE
XMLParser::SetSecureBaseURL(
    /* [in] */ const WCHAR* pszBaseUrl)
{
    WCHAR* newBaseUrl = NULL;
    if (pszBaseUrl != NULL)
    {
        newBaseUrl = ::StringDup(pszBaseUrl);
        if (newBaseUrl == NULL)
            return E_OUTOFMEMORY;
    }
    delete _pszSecureBaseURL;
    _pszSecureBaseURL = newBaseUrl;
    
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
XMLParser::GetSecureBaseURL(
    /* [out] */ const WCHAR __RPC_FAR *__RPC_FAR *ppwcBuf)
{
    *ppwcBuf = _pszSecureBaseURL;
    return S_OK;
}


HRESULT 
XMLParser::SetCurrentURL(
    /* [in] */ const WCHAR* pszCurrentUrl)
{
    WCHAR* newCurrentUrl = NULL;
    if (pszCurrentUrl != NULL)
    {
        newCurrentUrl = ::StringDup(pszCurrentUrl);
        if (newCurrentUrl == NULL)
            return E_OUTOFMEMORY;
    }
    delete _pszCurrentURL;
    _pszCurrentURL = newCurrentUrl;
    
    return S_OK;
}

HRESULT 
XMLParser::SetBaseURL(
    /* [in] */ const WCHAR* pszBaseUrl)
{
    WCHAR* newBaseUrl = NULL;
    if (pszBaseUrl != NULL)
    {
        newBaseUrl = ::StringDup(pszBaseUrl);
        if (newBaseUrl == NULL)
            return E_OUTOFMEMORY;
    }
    delete _pszBaseURL;
    _pszBaseURL = newBaseUrl;
    
    return S_OK;
}

HRESULT
XMLParser::HandleData(URLStream* pStm, bool last)
{
    HRESULT hr = S_OK;

    // According to email with danpoz on 1/19/99 There is a limit of 2 keep-alive 
    // connections per process and you cannot tell URLMON to NOT use keep-alive. 
    // You can find the current keep-alive limit using :
    //      InternetQueryOption(INTERNET_OPTION_MAX_CONNS_PER_SERVER).

    // So we have to continue pumping downloads - even if the parser is suspended.  
    // For example, if we download an XML document that in turn loads a DTD and the 
    // DTD loads an external entity - WININET will hang waiting for an available 
    // connection if the main XML document download or DTD download never completes.  
    // Even worse, is in the synchronous case we have to download ALL the data before 
    // parsing otherwise we attempt multiple downloads on the same thread which will 
    // always hang because there's no way to pump the prior downloads - since we're 
    // doing all this on the one thread.

    // Also if this bounces back synchronously then this is a local file !
    // in which case we do NOT need to do this expensive buffering.
    if (! _fInLoading || 0 != (_usFlags & XMLFLAG_RUNBUFFERONLY))
    {
        Download* download = FindDownload(pStm);
        if (download && download->_pEncodingStream)
        {
            hr = download->_pEncodingStream->BufferData();
            if (FAILED(hr) && E_PENDING != hr)
            {
                // must tell the encoding stream to pass the error onto the bufferedstream then.
                download->_pEncodingStream->setReadStream(true);
            }
        }

        // If this download failed, then we need to call Run() in order to notify
        // the Document that it failed.
        if ((S_OK != hr || last || _pdc->_fAsync) &&    // can parse if this is the last buffer or it's async 
            ! _fSuspended &&                // and not if this parser is actually suspended
            download &&           // and only if the download has officially been Pushed.
            _pdc == download)     // and only if the "current" download matches the one for this HandleData
        {
            AddRef();   // stabilize

            hr = Run(-1);
            TraceTag((tagParserCallback, "HandleData got hr %x from Run", hr));

            if (hr != S_OK && hr != E_PENDING && hr != XML_E_STOPPED && hr != XML_E_SUSPENDED)
            {
                _fLastError = hr;
                hr = E_ABORT;
            }
            Release();
        }
    }
Cleanup:
    return hr;
}

XMLParser::Download* 
XMLParser::FindDownload(URLStream* pStream)
{
    // Find the EncodingStream that wraps this URLStream and tell it
    // to buffer up the data so that we do not BLOCK the sockets.  If we
    // do not do this then we will eventually hang in WININET while waiting
    // for a socket to free up.  WININET enforces a limit to the number of
    // concurrent downloads allowed to ensure we are a good internet citizen.
    for (long i = _pDownloads.used()-1; i >= 0; i--)
    {
        Download* download = _pDownloads[i];
        if (download->_pURLStream == pStream)
        {
            return download;
        }
    }
    return null;
}

//------------ PRIVATE METHODS --------------------------------------------------
HRESULT
XMLParser::PushTokenizer(
                URLStream* stream)
{
    _pTokenizer = new_ne XMLStream(this);
    if (_pTokenizer == NULL)
        return E_OUTOFMEMORY;

    _pTokenizer->SetFlags(_usFlags);
//    _fTokenizerChanged = true;

    HRESULT hr= PushDownload(stream, _pTokenizer);
    if (FAILED(hr))
    {
        delete _pTokenizer;
        _pTokenizer = NULL;
        return hr;
    }
    return S_OK;
}

HRESULT
XMLParser::PushDownload(URLStream* stream, XMLStream* tokenizer)
{
    // NOTE: tokenizer can be null, in the case of a parameter entity download.

    _pdc = _pDownloads.push();
    if (_pdc == NULL)
    {
        return E_OUTOFMEMORY;
    }
    if (_pDownloads.used() > 1)
        _fRootLevel = false;

    _pdc->_pTokenizer = tokenizer;
    _pdc->_fDTD = false;
    _pdc->_fEntity = false;
    _pdc->_fAsync = false;
    _pdc->_fFoundNonWS = _fFoundNonWS;
    _pdc->_fFoundRoot = _fFoundRoot;
    _pdc->_fSeenDocType = _fSeenDocType;
    _pdc->_fRootLevel = _fRootLevel;
    _pdc->_fDepth = _pStack.used();

    _fFoundNonWS = false;
    _fFoundRoot = false;

    _fRootLevel = (_pStack.used() == 0 && _pDownloads.used() == 1);

    HRESULT hr = S_OK;
    if (stream != NULL)
    {
        _pdc->_pURLStream = stream;
        hr = PushStream(stream,
            tokenizer == NULL); // this is a parameter entity stream if "tokenizer" is NULL.
    }
    return hr;
}

HRESULT 
XMLParser::PushStream(IStream* pStm, bool fpe)
{
    EncodingStream* stream = (EncodingStream*)EncodingStream::newEncodingStream(pStm); // refcount = 1
    if (stream == NULL)
        return E_OUTOFMEMORY;

    if (_usFlags & XMLFLAG_RUNBUFFERONLY)
        stream->setReadStream(false);

    _pdc->_pEncodingStream = stream;
    stream->Release(); // Smart pointer is holding a ref

    HRESULT hr = _pTokenizer->PushStream(stream, fpe);
    if (hr == E_PENDING)
    {
        _fWaiting = true;
    }
    return hr;
}

HRESULT
XMLParser::PopDownload()
{
    // NOTE: tokenizer can be null, in the case of a parameter entity download.
    HRESULT hr = S_OK;

    if (_pdc != NULL)
    {
        if (_pdc->_pTokenizer)
        {
            _pdc->_pTokenizer->Reset();
            delete _pdc->_pTokenizer;
            _pdc->_pTokenizer = NULL;
        }
        _pdc->_pEncodingStream = NULL;
        if (_pdc->_pURLStream)
            _pdc->_pURLStream->Reset();
        _pdc->_pURLStream = NULL;
        // restore saved value of foundnonws.
        _fFoundNonWS = _pdc->_fFoundNonWS;
        _pdc = _pDownloads.pop();
    }
    if (_pdc != NULL)
    {
        if (_pdc->_pTokenizer != NULL)
        {
            _pTokenizer = _pdc->_pTokenizer;
        }
        if (_pdc->_pURLStream != NULL)
        {
            hr = SetCurrentURL(_pdc->_pURLStream->GetURL()->getResolved());
        }
    }
    else
    {
        _pTokenizer = NULL;
        hr = S_FALSE;
    }

    if (_pStack.used() == 0 && _pDownloads.used() == 1)
        _fRootLevel = true;

    return hr;
}

HRESULT
XMLParser::GrowNodeInfo()
{
    USHORT newsize = _cNodeInfoAllocated + STACK_INCREMENT;
    MY_XML_NODE_INFO** pNewArray = new_ne PMY_XML_NODE_INFO[newsize];
    if (pNewArray == NULL)
        return E_OUTOFMEMORY;
    // Now since STACK_INCREMENT is the same for _pStack then _pStack
    // has also re-allocated.  Therefore we need to re-initialize all
    // the pointers in this array - since they point into the _pStack's memory.
    for (int i = _pStack.used() - 1; i >= 0; i--)
    {
        pNewArray[i] = _pStack[i];
    }
    delete[] _paNodeInfo;
    _paNodeInfo = pNewArray;
    _cNodeInfoAllocated = newsize;
    return S_OK;
}

HRESULT
XMLParser::GrowBuffer(PMY_XML_NODE_INFO pNodeInfo, long newlen)
{
    delete [] pNodeInfo->_pwcTagName;
    pNodeInfo->_pwcTagName = NULL;
    // add 50 characters to avoid too many reallocations.
    pNodeInfo->_pwcTagName = new_ne WCHAR[ newlen ];
    if (pNodeInfo->_pwcTagName == NULL)
        return E_OUTOFMEMORY;
    pNodeInfo->_ulBufLen = newlen;
    return S_OK;
}

HRESULT
XMLParser::push(XML_NODE_INFO& info)
{
    HRESULT hr;
    _lCurrentElement = _pStack.used();

    _pCurrent = _pStack.push();
    if (_pCurrent == NULL)
        return E_OUTOFMEMORY;

    *((XML_NODE_INFO*)_pCurrent) = info;
    PUSHNODEINFO(_pCurrent);

    _fRootLevel = false;

    // Save the tag name into the private buffer so it sticks around until the
    // close tag </foo> which could be anywhere down the road after the
    // BufferedStream been overwritten

    // THIS CODE IS OPTIMIZED FOR PERFORMANCE WHICH IS WHY IT IS NOT
    // CALLING THE CopyText METHOD.
    if (_pCurrent->_ulBufLen < info.ulLen+1)
    {
        checkhr2(GrowBuffer(_pCurrent, info.ulLen + 50));
    }
    Assert(info.ulLen >= 0);
    ::memcpy(_pCurrent->_pwcTagName, info.pwcText, info.ulLen*sizeof(WCHAR));
    _pCurrent->_pwcTagName[info.ulLen] = L'\0';

    // And make the XML_NODE_INFO point to private buffer.
    _pCurrent->pwcText = _pCurrent->_pwcTagName;

    return S_OK;
}

HRESULT
XMLParser::pushAttribute(XML_NODE_INFO& info)
{
    HRESULT hr;
    if (_cAttributes != 0)
    {
        // Attributes are special in that they are supposed to be unique.
        // So here we actually check this.
        for (long i = _pStack.used()-1; i > _lCurrentElement; i--)
        {
            XML_NODE_INFO* ptr = _pStack[i];

            if (ptr->dwType != XML_ATTRIBUTE)
                continue; // ignore attribute values.

            if (ptr->ulLen != info.ulLen)
            {
                continue; // we're ok with this one
            }

            // Optimized for the normal case where there is no match
            if (::memcmp(ptr->pwcText, info.pwcText, info.ulLen*sizeof(TCHAR)) == 0)
            {
                if (! _fCaseInsensitive)
                {
                    return XML_E_DUPLICATEATTRIBUTE;
                }
                else if (StrCmpNI(ptr->pwcText, info.pwcText, info.ulLen) == 0)
                {
                    // Duplicate attributes are allowed in IE4 mode!!
                    // But only the latest one shows up
                    // So we have to delete the previous duplication
                    if (_fIE4Mode)
                    {
// This is some code, so we comment it out for now
#ifdef NEVER
                        long j = i + 1;
                        int k;
                        // Find the next attribute
                        while (j < _pStack.used())
                        {
                            XML_NODE_INFO * pInfo = _pStack[j];
                            if (pInfo->dwType == XML_ATTRIBUTE)
                                break;
                            else
                                j++;
                        }
                        // move the stack content
                        for (k = 0; k < _pStack.used() - j; k++)
                        {
                            *((XML_NODE_INFO*)_pStack[k + i]) = *((XML_NODE_INFO*)_pStack[k + j]);
                        }
                        // pop
                        for (k = j - i; k > 0; k --)
                            _pCurrent = _pStack.pop();
                        _cNodeInfoCurrent -= j - i;
                        _cAttributes -= j - i;
#endif
                        break;
                    }
                    else
                    {
                        return XML_E_DUPLICATEATTRIBUTE;
                    }
                }
            }
        }
    }

    _cAttributes++;

    _pCurrent = _pStack.push();
    if (_pCurrent == NULL)
        return E_OUTOFMEMORY;

    *((XML_NODE_INFO*)_pCurrent) = info;
    PUSHNODEINFO(_pCurrent);

    return S_OK;
}

HRESULT
XMLParser::pushAttributeValue(XML_NODE_INFO& info)
{
    HRESULT hr;
    // Attributes are saved in the BufferedStream so we can point to the
    // real text in the buffered stream instead of copying it !!

    _pCurrent = _pStack.push();
    if (_pCurrent == NULL)
        return E_OUTOFMEMORY;

    // store attribute value quote character in the pReserved field.
    info.pReserved = (PVOID)_pTokenizer->getAttrValueQuoteChar();

    *((XML_NODE_INFO*)_pCurrent) = info;
    PUSHNODEINFO(_pCurrent);

    // this is really the count of nodes on the stack, not just attributes.
    _cAttributes++;
    return S_OK;
}

HRESULT
XMLParser::pushDTDAttribute(XML_NODE_INFO& info)
{
    HRESULT hr;
    _pCurrent = _pStack.push();
    if (_pCurrent == NULL)
        return E_OUTOFMEMORY;

    *((XML_NODE_INFO*)_pCurrent) = info;
    PUSHNODEINFO(_pCurrent);

    _fFoundDTDAttribute = true;

    return S_OK;
}

HRESULT
XMLParser::pop(const WCHAR* tag, ULONG len)
{
    HRESULT hr = S_OK;

    if (_pCurrent == NULL || _pStack.used() == 0)
    {
        hr = XML_E_UNEXPECTEDENDTAG;
        goto Cleanup;
    }
    if (len != 0)
    {
        if (_pCurrent->ulLen != len)
        {
            hr = XML_E_ENDTAGMISMATCH;
        }
        // Optimized for the normal case where there is no match
        else if (::memcmp(_pCurrent->pwcText, tag, len*sizeof(TCHAR)) != 0)
        {
            if (! _fCaseInsensitive)
            {
                hr = XML_E_ENDTAGMISMATCH;
            }
            else if (::StrCmpNI(_pCurrent->pwcText, tag, len) != 0)
            {
                hr = XML_E_ENDTAGMISMATCH;
            }
        }
        if (hr)
        {
            TRY
            {
                String* s = Resources::FormatMessage(hr, String::newString(_pCurrent->pwcText, 0, _pCurrent->ulLen),
                                                         String::newString(tag, 0, len), NULL);
                _bstrError = s->getBSTR();
            }
            CATCH
            {
                hr = ERESULT;
            }
            ENDTRY
            goto Cleanup;
        }
    }

    // We don't delete the fTagName because we're going to reuse this field
    // later to avoid lots of memory allocations.

    _pCurrent = _pStack.pop();
    _cNodeInfoCurrent--;

    if (_pCurrent == 0)
    {
        _pNode = _pRoot;
        if (_pDownloads.used() == 1)
            _fRootLevel = true;
    }
    else
    {
        _pNode = _pCurrent->pNode;
    }

Cleanup:
    return hr;
}

HRESULT XMLParser::pop()
{
    // We don't delete the fTagName because we're going to reuse this field
    // later to avoid lots of memory allocations.

    _pCurrent = _pStack.pop();
    _cNodeInfoCurrent--;

    if (_pCurrent == 0)
    {
        _pNode = _pRoot;
        if (_pDownloads.used() == 1)
            _fRootLevel = true;
    }
    else
    {
        _pNode = _pCurrent->pNode;
    }
    return S_OK;
}

void XMLParser::popAttribute()
{
    Assert(_pStack.used() > 0);

    _pCurrent = _pStack.pop();
    _cNodeInfoCurrent--;

    Assert(_pCurrent != 0);

    _cAttributes--;

}

HRESULT
XMLParser::popDTDAttribute()
{
    HRESULT hr;
    Assert(_pStack.used() > 0);

    hr = _pFactory->EndChildren(this, TRUE, (XML_NODE_INFO*)_pCurrent);

    _fFoundDTDAttribute = false;

    _pCurrent = _pStack.pop();
    _cNodeInfoCurrent--;

    Assert(_pCurrent != 0);

    _pNode = _pCurrent->pNode;

    return hr;
}


HRESULT
XMLParser::CopyText(PMY_XML_NODE_INFO pNodeInfo)
{
    HRESULT hr = S_OK;
    if (pNodeInfo->_pwcTagName != pNodeInfo->pwcText)
    {
        ULONG len = pNodeInfo->ulLen;

        // Copy the current text into the buffer.
        if (pNodeInfo->_ulBufLen < len+1)
        {
            checkhr2(GrowBuffer(pNodeInfo, len + 50));
        }
        if (len > 0)
        {
            ::memcpy(pNodeInfo->_pwcTagName, pNodeInfo->pwcText, len*sizeof(WCHAR));
        }
        pNodeInfo->_pwcTagName[len] = L'\0';

        // And make the XML_NODE_INFO point to private buffer.
        pNodeInfo->pwcText = pNodeInfo->_pwcTagName;
    }
    return S_OK;
}

HRESULT
XMLParser::CopyContext()
{
    // For performance reasons we try not to copy the data for attributes
    // and their values when we push them on the stack.  We can do this
    // because the tokenizer tries to freeze the internal buffers while
    // parsing attributes and thereby guarentee that the pointers stay
    // good.  But occasionally the BufferedStream has to reallocate when
    // the attributes are right at the end of the buffer.

    long last = _pStack.used();
    for (long i = _cAttributes; i > 0 ; i--)
    {
        long index = last - i;
        MY_XML_NODE_INFO* ptr = _pStack[index];
        CopyText(ptr);
    }
    return S_OK;
}

HRESULT XMLParser::ReportUnclosedTags(int start)
{
    HRESULT hr = XML_E_UNCLOSEDTAG;
    // Build a string containing the list of unclosed tags and format an error
    // message containing this text.
    int tags = _pStack.used();

    WCHAR* buffer = NULL;
    WCHAR* msgbuf = NULL;
    unsigned long size = 0;
    unsigned long used = 0;

    for (long i = start; i < tags; i++)
    {
        XML_NODE_INFO* ptr = _pStack[i];
        if (ptr->dwType == XML_ATTRIBUTE)
            break;

        if (used + ptr->ulLen + 3 > size) // +3 for '<','>' and '\0'
        {
            long newsize = used + ptr->ulLen + 500;
            WCHAR* newbuf = new_ne WCHAR[newsize];
            if (newbuf == NULL)
            {
                goto nomem;
            }
            if (buffer != NULL)
            {
                ::memcpy(newbuf, buffer, used);
                delete[] buffer;
            }

            size = newsize;
            buffer = newbuf;
        }
        if (i > start)
        {
            buffer[used++] = ',';
            buffer[used++] = ' ';
        }
        ::memcpy(&buffer[used], ptr->pwcText, sizeof(WCHAR) * ptr->ulLen);
        used += ptr->ulLen;
        buffer[used] = '\0';
    }

//    msgbuf = ::FormatMessageInternal(g_hInstance, XML_E_UNCLOSEDTAG, buffer, NULL);
    TRY
    {
        String* s = Resources::FormatMessage(XML_E_UNCLOSEDTAG,
            String::newString(buffer), NULL);
        _bstrError = s->getBSTR();
        goto cleanup;
    }
    CATCH
    {
        hr = ERESULT;
        goto done;
    }
    ENDTRY

    if (msgbuf == NULL)
        goto nomem;

    if (_bstrError) ::SysFreeString(_bstrError);
    _bstrError = ::SysAllocString(msgbuf);
    if (_bstrError == NULL)
        goto nomem;

    goto cleanup;

nomem:
    hr = E_OUTOFMEMORY;
cleanup:
    delete [] buffer;
    delete [] msgbuf;
done:
    return hr;
}

HRESULT XMLParser::init()
{
    CRITICALSECTIONLOCK;

    _fLastError = 0;
    _fStopped = false;
    _fSuspended = false;
    _pNode = _pRoot;
    _fStarted = false;
    _fStopped = false;
    _fWaiting = false;
    _fFoundRoot = false;
    _fFoundNonWS = false;
    _pTokenizer = NULL;
    _fGotVersion = false;
    _cDTD = 0;
    _fSeenDocType = false;
    _fRootLevel = true;
    _cAttributes = 0;
    _fFoundDTDAttribute = false;
    _fPendingBeginChildren = false;
    _fPendingEndChildren = false;

    while (_pCurrent != NULL)
    {
        _pCurrent = _pStack.pop();
    }

    _cNodeInfoCurrent = 0;
    _lCurrentElement = 0;

    // cleanup downloads
    while (_pdc != NULL)
    {
        PopDownload();
    }

    _pCurrent = NULL;
    return S_OK;
}

/**
 *Gets the URL from the specified IMoniker and binding context
 */
HRESULT XMLParser::ExtractURL(IMoniker* pmk, LPBC pbc, URL* pURL)
{
    LPOLESTR pOleStr = NULL;
    HRESULT hr = S_OK;
    long len = 0;

    checkerr(pmk->GetDisplayName(pbc, pmk, &pOleStr));
    len = ::StrLen(pOleStr);
    // Fix bugs in what GetDisplayName returns...
    if (len > 4 && ::StrNCmpI(pOleStr, _T("file"), 4) == 0)
    {
        TCHAR* buf = new_ne TCHAR[ len + 10 ];
        ::StrCpy(buf, _T("file:///"));
        LPOLESTR ptr = &pOleStr[4];
        if (*ptr == L':') ptr++;
        while (*ptr != NULL && *ptr == L'/') ptr++;
        TCHAR* temp = &buf[7];
        while (*ptr != NULL)
        {
            if (*ptr == L'\\')
                *temp = L'/';
            else
                *temp = *ptr;
            ptr++;
            temp++;
        }
        *temp = '\0'; // NULL terminate it !!
        hr = pURL->set(buf, NULL);
        delete[] buf;
    }
    else
        hr = pURL->set(pOleStr, NULL);

error:
    if (pOleStr)
        CoTaskMemFree(pOleStr);
    return hr;
}

void 
XMLParser::SetMimeType(URLStream* pStm, const WCHAR * pwszMimeType, int length) 
{
    Download* download = FindDownload(pStm);
    if (download && download->_pEncodingStream)
    {
        download->_pEncodingStream->SetMimeType(pwszMimeType, length);
    }
}


void 
XMLParser::SetCharset(URLStream* pStm, const WCHAR * pwszCharset, int length) 
{
    Download* download = FindDownload(pStm);
    if (download && download->_pEncodingStream)
    {
        download->_pEncodingStream->SetCharset(pwszCharset, length);
    }
}


HRESULT 
XMLParser::ErrorCallback(HRESULT hr)
{
    Assert(hr == XMLStream::XML_DATAAVAILABLE ||
           hr == XMLStream::XML_DATAREALLOCATE);

    if (hr == XMLStream::XML_DATAREALLOCATE)
    {
        // This is more serious.  We have to actually save away the
        // context because the buffers are about to be reallocated.
        checkhr2(CopyContext());
    }
    checkhr2(_pFactory->NotifyEvent(this, XMLNF_DATAAVAILABLE));
    return hr;
}



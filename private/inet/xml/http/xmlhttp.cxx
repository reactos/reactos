/*
 -  XMLHTTP.CPP
 -
 *  Purpose: XML HTTP Request object definition
 *
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 *  References:
 *
 *
 */

#include "core.hxx"
#pragma hdrstop

#include <shlguid.h>
#include <ocidl.h>      // READYSTATE_*
#include "../xml/tokenizer/encoder/encodingstream.hxx"
#include "../xml/tokenizer/net/url.hxx"
#include "xmlhttp.hxx"

#pragma warning(disable: 4102)

#define SafeRelease(p) \
{ \
    if (p) \
        (p)->Release();\
    (p) = NULL;\
}

// External MSXML helpers
extern HRESULT
CreateVector(VARIANT * pVar, const BYTE * pData, LONG cElems);

extern HRESULT
UrlOpenAllowed(LPCWSTR pwszUrl, LPCWSTR pwszBaseUrl, BOOL fDTD);

extern HRESULT
CreateDOMDocument(REFIID iid, void ** ppvObj);

// Helper function to save a document and return the encoded bytes.
// Defined in xml\om\docstream.cxx
extern HRESULT 
SaveDocument(IDispatch * pDispatch, BYTE** ppData, DWORD* pdwSize);


/***************************
/* A cheaper version of this function is implemented in xml/om/docstream.cxx
/* Let's keep this here in case XMLHTTP is ever split out to a separate
/* DLL.

HRESULT SaveDocument(IDispatch * pDispatch, BYTE** ppData, DWORD* pdwSize)
{
    HRESULT             hr = S_OK;
    BOOL                fRetCode = FALSE;
    BSTR                bstrBody = NULL;
    IPersistStreamInit *pPS = NULL;
    IStream  *          pStm = NULL;
    HGLOBAL             hGlobal = NULL;
    BYTE*               pBytes = NULL;
    DWORD               dwSize = 0;
    STATSTG             statStg;

    if (!pDispatch)
        goto ErrorInvalidArg;
        
    // Save document to a memory stream.
    hr = pDispatch->QueryInterface(IID_IPersistStreamInit, (void**)&pPS);

    if (FAILED(hr))
        goto Error;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStm);

    if (FAILED(hr))
        goto Error;

    hr = pPS->Save(pStm, TRUE);
    
    if (FAILED(hr))
        goto Error;

    // Copy out the raw encoded bytes.
    hr = GetHGlobalFromStream(pStm, &hGlobal);

    if (FAILED(hr))
        goto Error;

    pStm->Stat(&statStg, STATFLAG_NONAME);
    dwSize = statStg.cbSize.QuadPart;

    pBytes = (BYTE*)new_ne char[dwSize];

    if (pBytes == NULL)
        goto ErrorOutOfMemory;

    ::memcpy(pBytes, *((char**)hGlobal), dwSize);
    
Cleanup:
    SafeRelease(pPS);
    SafeRelease(pStm);

    *ppData = pBytes;
    *pdwSize = dwSize;
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

Error:
    goto Cleanup;
}
*************************************/


#define TYPE_XML                "xml"
#define TYPE_TEXT               "text/"
#define TYPE_APPLICATION        "application/" // like application/x-netcdf
#define TYPE_TEXTXML            TYPE_TEXT TYPE_XML
#define TYPE_APPXML             TYPE_APPLICATION TYPE_XML
#define USER_AGENT_BUFSIZE      64
#define HTTP_HEADER_CONTENTTYPE "Content-Type"
#define HTTP_HEADER_CONTENTLEN  "Content-Length"
#define HTTP_HEADER_REFERRER    "Referer"

#define WM_HTTP_INVOKE          WM_USER

/*
 -  CreateXMLHTTPRequest
 -
 *  Purpose:
 *      Helper to create a CXMLHTTP object
 *
 */

HRESULT STDMETHODCALLTYPE
CreateXMLHTTPRequest(REFIID iid, void **ppvObj)
{
    STACK_ENTRY;
    HRESULT hr;

    TRY
    {
        CXMLHTTP * pXmlHttp = new CXMLHTTP();
        XMLHTTPWrapper *pWrapper = new XMLHTTPWrapper(pXmlHttp);


        // Finally, get the desired interface
        hr = pWrapper->QueryInterface(iid, ppvObj);
        pWrapper->Release();
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


/*
 -  CXMLHTTP::SetErrorInfo
 -
 *  Purpose:
 *      Correctly propagate errors through IDispatch
 *
 */

void
CXMLHTTP::SetErrorInfo(
    HRESULT     hr,
    DWORD       dwLastError)
{
    if (FAILED(hr) && dwLastError != 0)
    {
        String* msg = Resources::FormatSystemMessage(dwLastError);
        if (msg != null)
        {
            Exception* e = Exception::newException(HRESULT_FROM_WIN32(dwLastError), msg);
            e->throwE();
        }
        Exception::throwE(hr);
    }
}

/*
 -  GetBSTRFromVariant
 -
 *  Purpose:
 *      Convert a VARIANT to a BSTR
 *
 */

BSTR GetBSTRFromVariant(VARIANT varVariant)
{
    VARIANT varTemp;
    HRESULT hr;
    BSTR    bstrResult = NULL;

    if (V_VT(&varVariant) != VT_EMPTY && V_VT(&varVariant) != VT_NULL &&
                V_VT(&varVariant) != VT_ERROR)
    {
        VariantInit(&varTemp);

        hr = VariantChangeType(
                &varTemp,
                &varVariant,
                VARIANT_NOVALUEPROP,
                VT_BSTR);

        if (FAILED(hr))
            goto Error;
        
        bstrResult = V_BSTR(&varTemp); // take over ownership of BSTR
    }

Cleanup:
    // DON'T clear the variant because we stole the BSTR
    //VariantClear(&varTemp);
    return bstrResult;

Error:
    goto Cleanup;
}


/*
 -  GetBoolFromVariant
 -
 *  Purpose:
 *      Convert a VARIANT to a Boolean
 *
 */

BOOL GetBoolFromVariant(VARIANT varVariant, BOOL fDefault)
{
    HRESULT hr;
    BOOL    fResult = fDefault;

    if (V_VT(&varVariant) != VT_EMPTY && V_VT(&varVariant) != VT_NULL &&
        V_VT(&varVariant) != VT_ERROR)
    {
        VARIANT varTemp;
        VariantInit(&varTemp);
        hr = VariantChangeType(
                &varTemp,
                &varVariant,
                VARIANT_NOVALUEPROP,
                VT_BOOL);
    
        if (FAILED(hr))
            goto Cleanup;
        
        fResult = V_BOOL(&varTemp) == VARIANT_TRUE ? TRUE : FALSE;
    }

Cleanup:
    return fResult;
}


/*
 -  BSTRToUTF8
 -
 *  Purpose:
 *      Convert a BSTR to UTF-8
 *
 */

HRESULT 
BSTRToUTF8(char ** psz, DWORD * pcbUTF8, BSTR bstr)
{
    Assert(psz);
    Assert(bstr);
    Assert(pcbUTF8);
    
    UINT cch = lstrlenW(bstr);
    DWORD dwMode = 0;
    
    *pcbUTF8 = 0;
    *psz = NULL;
    
    if (cch == 0)
        return S_OK;

    // Allocate the maximum buffer the UTF-8 string could be
    *psz = new_ne char [(cch * 3) + 1];
    
    if (!*psz)
        return E_OUTOFMEMORY;

    *pcbUTF8 = cch * 3;

    CharEncoder::wideCharToUtf8(&dwMode,
                    CP_UNDEFINED,
                    bstr,
                    &cch,
                    (BYTE*)*psz,
                    (UINT*)pcbUTF8);

	(*psz)[*pcbUTF8] = 0;
    return S_OK;
}



/*
 -  BSTRToAscii
 -
 *  Purpose:
 *      Convert a BSTR to an ascii string
 *
 */

HRESULT 
BSTRToAscii(char ** psz, BSTR bstr)
{
    int cch;

    *psz = NULL;

    if (!bstr)
        return S_OK;

    // Determine how big the ascii string will be
    cch = WideCharToMultiByte(CP_ACP, 0, bstr, lstrlenW(bstr),
                                NULL, 0, NULL, NULL);

    *psz = new_ne char[cch + 1];

    if (!*psz)
        return E_OUTOFMEMORY;

    // Determine how big the ascii string will be
    cch = WideCharToMultiByte(CP_ACP, 0, bstr, lstrlenW(bstr),
                                *psz, cch, NULL, NULL);

    (*psz)[cch] = 0;
    
    return S_OK;
}


/*
 -  AsciiToBSTR
 -
 *  Purpose:
 *      Convert an ascii string to a BSTR
 *
 */

HRESULT 
AsciiToBSTR(BSTR * pbstr, char * sz)
{
    int cch = strlen(sz);
    int cwch;

    // Determine how big the ascii string will be
    cwch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, cch,
                                NULL, 0);

    *pbstr = SysAllocStringLen(NULL, cwch);

    if (!*pbstr)
        return E_OUTOFMEMORY;

    // Determine how big the ascii string will be
    cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, cch,
                                *pbstr, cwch);

    return S_OK;
}


/*
 -  IDispatch Wrapper Functions
 -
 *
 */

DISPATCHINFO _dispatchexport<CXMLHTTP, IXMLHttpRequest, &LIBID_MSXML, ORD_MSXML,
                             &IID_IXMLHttpRequest>::s_dispatchinfo = 
{
    NULL, &IID_IXMLHttpRequest, &LIBID_MSXML, ORD_MSXML
};


STDMETHODIMP
XMLHTTPWrapper::open(BSTR szVerb, BSTR bstrUrl, VARIANT varAsync,
                     VARIANT bstrUser, VARIANT bstrPassword)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->open(szVerb, bstrUrl, varAsync,
                                bstrUser, bstrPassword);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


STDMETHODIMP
XMLHTTPWrapper::abort()
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->abort();
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


STDMETHODIMP
XMLHTTPWrapper::setRequestHeader(BSTR header, BSTR bstrValue)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->setRequestHeader(header, bstrValue);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


STDMETHODIMP
XMLHTTPWrapper::getResponseHeader(BSTR header, BSTR * pbstrValue)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->getResponseHeader(header, pbstrValue);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


STDMETHODIMP
XMLHTTPWrapper::getAllResponseHeaders(BSTR * pbstrValue)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->getAllResponseHeaders(pbstrValue);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


STDMETHODIMP
XMLHTTPWrapper::send(VARIANT varBody)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->send(varBody, FALSE);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


STDMETHODIMP
XMLHTTPWrapper::get_responseXML(IDispatch ** ppBody)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->get_responseXML(ppBody);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


STDMETHODIMP
XMLHTTPWrapper::get_responseBody(VARIANT * pvarVal)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->get_responseBody(pvarVal);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


STDMETHODIMP
XMLHTTPWrapper::get_responseText(BSTR * pbstrVal){
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->get_responseText(pbstrVal);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


STDMETHODIMP
XMLHTTPWrapper::get_responseStream(VARIANT * pvarVal){
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->get_responseStream(pvarVal);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


STDMETHODIMP
XMLHTTPWrapper::get_status(long * plStatus)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->get_status(plStatus);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


STDMETHODIMP
XMLHTTPWrapper::get_statusText(BSTR * pbstrVal)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->get_statusText(pbstrVal);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


STDMETHODIMP
XMLHTTPWrapper::get_readyState(long * plState)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->get_readyState(plState);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


STDMETHODIMP
XMLHTTPWrapper::put_onreadystatechange(IDispatch * pReadyStateSink)
{
    STACK_ENTRY_WRAPPED;
    HRESULT hr;

    TRY 
    {
        hr = getWrapped()->put_onreadystatechange(pReadyStateSink);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

    return hr;
}


//STDAPI
LRESULT
WndProcMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
   case WM_HTTP_INVOKE:
        if (lParam)
        {
            CXMLHTTP *  pXmlHttp = (CXMLHTTP *)lParam;
            DISPPARAMS  dp;
            dp.rgvarg = NULL;
            dp.cArgs = 0;
            dp.rgdispidNamedArgs = NULL;
            dp.cNamedArgs = 0;

            IDispatch* pDisp = pXmlHttp->m_pReadyStateSink;
            if (pDisp)
            {
                pDisp->Invoke(
                            0, // Special Trident trick
                            IID_NULL,
                            LOCALE_SYSTEM_DEFAULT,
                            DISPATCH_METHOD,
                            &dp,
                            NULL, NULL, NULL);
            }
            pXmlHttp->Release();
        }
        break;

   default:
       return DefWindowProc(hwnd, msg, wParam, lParam);
   }

   return 0;
}

extern HINSTANCE g_hInstance;
const WCHAR * c_wszXMLHTTPMsgClass = L"XMLHTTPMsgClass";

void 
XMLHTTPShutdown()
{
    // UnRegister a window class
    UnregisterClass(c_wszXMLHTTPMsgClass, g_hInstance);
}


/*
 -  CXMLHTTP::CXMLHTTP
 -
 *  Purpose:
 *      Constructor
 *
 */

DEFINE_CLASS_MEMBERS(CXMLHTTP, _T("CXMLHTTP"), CSafeControl);

CXMLHTTP::CXMLHTTP()
{
    ::IncrementComponents();

    Initialize();

    m_szUserAgent = NULL;
    
    m_hWininet = LoadLibraryA("wininet");
    if (m_hWininet != NULL)
    {
        m_pfnInternetSetStatusCallback = (InternetSetStatusCallbackFunc *) GetProcAddress(m_hWininet, "InternetSetStatusCallbackA");
        if (!m_pfnInternetSetStatusCallback)
        {
            m_pfnInternetSetStatusCallback = (InternetSetStatusCallbackFunc *) GetProcAddress(m_hWininet, "InternetSetStatusCallback");
        }
    }
    m_pMutex = ApartmentMutex::newApartmentMutex();
    m_pMutex->Release(); // starts with refcount 1

    m_hWnd = NULL;
}


/*
 -  CXMLHTTP::~CXMLHTTP
 -
 *  Purpose:
 *      Destructor
 *
 */

CXMLHTTP::~CXMLHTTP()
{
    Reset(true);

    if (m_hWnd)
    {
        // cleanup the window.
        DestroyWindow(m_hWnd);
        m_hWnd = NULL;
    }

    ::DecrementComponents();
}

ULONG CXMLHTTP::_release()
{
    super::_addRef(); 

    ULONG r = super::_release();
    if (r == 1)
    {
        Reset(true); 
    }
    r = super::_release();
    return r;
}

/*
 -  CXMLHTTP::Initialize
 -
 *  Purpose:
 *      Zero all data members
 *
 */

void
CXMLHTTP::Initialize()
{
    m_eState = CREATED;
    m_dwStatus = 0;
    
    m_pContext = NULL;

    m_hInet = NULL;
    m_hSession = NULL;
    m_hHTTP = NULL;

    m_pReadyStateSink = NULL;

    m_bstrTargetUrl = NULL;

    m_szRequestBuffer = NULL;
    m_cbRequestBody = 0;

    m_szResponseBuffer = NULL;
    m_cbResponseBuffer = 0;
    m_cbResponseBody = 0;

    m_fUseridAndPassword = FALSE;
    m_fXMLResponse = FALSE;
    m_fChunkedRequest = FALSE;
    m_fContentDetermined = FALSE;
    m_fXDomainUrlChecked = FALSE;
    
    m_fAsync = TRUE;

    m_pXMLDoc = NULL;
    m_pXMLParser = NULL;
}


/*
 -  CXMLHTTP::ReleaseResources
 -
 *  Purpose:
 *      Release all handles, events, and buffers
 *
 */

void
CXMLHTTP::ReleaseResources()
{
    //
    // Release internet handles
    //
    
    if (m_pContext)
    {
        // Closing the HTTP handle will cause a callback, so regardless of our
        // state we are guaranteed that the callback will free the context
        // memory.

        m_pContext->Lock();

        if (m_pContext->hHTTP)
        {
            m_pContext->pXmlHttp = NULL;

            // Look for an 'idle' state
            if (m_eState == OPENED || m_eState == RESPONSE)
            {
                HINTERNET hHTTP = m_pContext->hHTTP;
                m_pContext->hHTTP = NULL;
                m_pContext->UnLock();
                InternetCloseHandle(hHTTP);
            }
            else
            {
                m_pContext->UnLock();
            }
        }
        else
        {
            m_pContext->UnLock();
            delete m_pContext;
            m_pContext = null;
            ::DecrementComponents();
        }
    }

    if (m_hSession)
    {
        HINTERNET temp = m_hSession;
        m_hSession = NULL;
        InternetCloseHandle(temp);
    }

    if (m_hInet)
    {
        HINTERNET temp = m_hInet;
        m_hInet = NULL;
        InternetCloseHandle(temp);
    }

    //
    // Release buffers
    //
    
    if(m_szRequestBuffer)
    {
        delete [] m_szRequestBuffer;
        m_szRequestBuffer = NULL;
    }
    
    if(m_szResponseBuffer)
    {
        delete [] m_szResponseBuffer;
        m_szResponseBuffer = NULL;
    }
    
    if(m_bstrTargetUrl)
    {
        SysFreeString(m_bstrTargetUrl);
        m_bstrTargetUrl = NULL;
    }

    // Release XML references
    SafeRelease(m_pXMLDoc);
    SafeRelease(m_pXMLParser);

    // Release ReadyState sink reference
    SafeRelease(m_pReadyStateSink);
}


/*
 -  CXMLHTTP::Reset
 -
 *  Purpose:
 *      Release all resources and initialize data members
 *
 */

void
CXMLHTTP::Reset(bool cleanupall)
{
    ReleaseResources();
    Initialize();

    if (cleanupall)
    {
        if (m_hWininet)
        {
            FreeLibrary(m_hWininet);
            m_hWininet = NULL;
        }

        if(m_szUserAgent)
        {
            delete [] m_szUserAgent;
            m_szUserAgent = null;
        }

        m_pMutex = null;

    }
}


HRESULT
CXMLHTTP::QueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr = S_OK;

    TRY 
    {
        if (iid == IID_IUnknown)
        {
            assign(ppv, (GenericBase *)this);
        }
        else if (iid == IID_IObjectWithSite)
        {
            *ppv = (IObjectWithSite *)new IObjectWithSiteWrapper(this,m_pMutex);
        }
        else if (iid == IID_IObjectSafety)
        {
            *ppv = (IObjectSafety *)new IObjectSafetyWrapper(this, m_pMutex);
        }
        else if (iid == IID_IDispatch || iid == IID_IXMLHttpRequest)
        {
            *ppv = (CXMLHTTP *)this;
            ((LPUNKNOWN)*ppv)->AddRef();
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


/*
 -  CXMLHTTP::SetState
 -
 *  Purpose:
 *      Set the object state and fire state transitions if necessary
 *
 *  Parameters:
 *      eState      IN      New state
 *
 *  Errors:
 *      
 */

void
CXMLHTTP::SetState(State eState)
{
    m_eState = eState;

    // Signal the state change event 
    if (m_pReadyStateSink != NULL && m_hWnd)
    {
        this->AddRef(); // keep the sink alive until we invoke it !!
        // Shawn - I was able to prove using a stress test 
        // (http://xmlweb/msxml/stress/xmlhttp.asp hitting the F5 key while it's
        // running) that a SendMessage() can cause a deadlock here if the
        // UI thread has already entered wait on the m_pContext->Lock() because
        // if it has it cannot possibly process the SendMessage.  So this must
        // be a post message.
        PostMessage(m_hWnd, WM_HTTP_INVOKE, 0, (LPARAM)this);
    }
}


/*
 -  CXMLHTTP::GetContentType
 -
 *  Purpose:
 *      Set the XML content type flag
 *
 *  Parameters:
 *      None
 *
 *  Errors:
 *      E_OUTOFMEMORY
 */

HRESULT
CXMLHTTP::GetContentType()
{
    HRESULT     hr = S_OK;
    DWORD       dwLastError = 0;
    BOOL        fRetCode = FALSE;
    char *      szContentType = NULL;
    DWORD       dwContentLength = 0;
    DWORD       cb = 0;

    if (m_fContentDetermined)
        goto Cleanup;
		
    m_fXMLResponse = FALSE;

RetryQuery:
    fRetCode = HttpQueryInfoA(
                    m_hHTTP,
                    HTTP_QUERY_CONTENT_TYPE,
                    szContentType,
                    &cb,
                    0);

    // Check for ERROR_INSUFFICIENT_BUFFER - reallocate the buffer and retry
    if (!fRetCode)
    {
        dwLastError = GetLastError();
        
        switch(dwLastError)
        {
        case ERROR_INSUFFICIENT_BUFFER:
        {
            szContentType = new_ne char[cb];

            if (!szContentType)
                goto ErrorOutOfMemory;

            goto RetryQuery;
        }

        case ERROR_HTTP_HEADER_NOT_FOUND:
        default:
            // Fall out with szContentType = NULL
            break;
        }
    }

    if (szContentType)
    {
        String * pstrContentTypeValue = String::newString(szContentType);
        String * pstrTextXML = String::newString(TYPE_TEXTXML);
        String * pstrAppXML = String::newString(TYPE_APPXML);

        if (pstrContentTypeValue)
        {
            pstrContentTypeValue = pstrContentTypeValue->toLowerCase();

            if (pstrContentTypeValue->startsWith(pstrTextXML) ||
                pstrContentTypeValue->startsWith(pstrAppXML))
                m_fXMLResponse = TRUE;
        }
    }

    // If we can determine the content length then pre-allocate the
    // response buffer

    cb = sizeof(dwContentLength);
    
    fRetCode = HttpQueryInfoA(
                    m_hHTTP,
                    HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                    &dwContentLength,
                    &cb,
                    0);

    // Ignore any failures since the buffer will be allocated as needed
    // in ReadResponse().
    if (fRetCode && dwContentLength)
    {
        m_szResponseBuffer = new_ne char[dwContentLength];

        if (m_szResponseBuffer)
        {
            m_cbResponseBuffer = dwContentLength;
        }
    }
    
    m_fContentDetermined = TRUE;
    
Cleanup:
    if(szContentType)
        delete [] szContentType;
        
    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

Error:
    goto Cleanup;
}


/*
 -  CXMLHTTP::ReadResponse
 -
 *  Purpose:
 *      Read the response bits
 *
 *  Parameters:
 *      None
 *
 *  Errors:
 *      E_FAIL
 *      E_OUTOFMEMORY
 */

HRESULT
CXMLHTTP::ReadResponse()
{
    HRESULT     hr = S_OK;
    BOOL        fRetCode = TRUE;
    ULONG       cbNewSize;
    char *      szNewBuf;

    // Determine the content type and length exactly once
    if (!m_fContentDetermined)
    {
        hr = GetContentType();

        if (FAILED(hr))
            goto Error;
    }

    // Read data until there is no more - we need to buffer the data
    // as well as Push it into the XML object (if response is XML)

    while (fRetCode)
    {
    	m_cbAvail = 0;
        fRetCode = InternetQueryDataAvailable(m_hHTTP, &m_cbAvail, 0, 0);

        if (!fRetCode)
        {
            if (m_fAsync && GetLastError() == ERROR_IO_PENDING)
                goto Cleanup;
            else
                goto ErrorFail;
        }

        // Check for buffer overflow - dynamically resize if neccessary
        if (m_cbResponseBody + m_cbAvail > m_cbResponseBuffer)
        {
            cbNewSize = m_cbResponseBody + m_cbAvail;
            szNewBuf = new_ne char[cbNewSize];

            if (!szNewBuf)
                goto ErrorOutOfMemory;

            ::memcpy(szNewBuf, m_szResponseBuffer, m_cbResponseBody);

            m_cbResponseBuffer = cbNewSize;

            delete[] m_szResponseBuffer;

            m_szResponseBuffer = szNewBuf;
        }

    	m_cbRead = 0;
        fRetCode = InternetReadFile(
                    m_hHTTP,
                    &m_szResponseBuffer[m_cbResponseBody],
                    m_cbAvail,
                    &m_cbRead);

        if (!fRetCode)
        {
            if (m_fAsync && GetLastError() == ERROR_IO_PENDING)
                goto Cleanup;
            else
                goto ErrorFail;
        }
        
        // Push data into XMLDOM
        if (m_fXMLResponse && m_pXMLParser)
        {
            hr = m_pXMLParser->PushData(
#ifdef UNIX
                                (const unsigned char*)&m_szResponseBuffer[m_cbResponseBody],
#else
                                &m_szResponseBuffer[m_cbResponseBody],
#endif
                                m_cbRead,
                                m_cbRead == 0 ? TRUE : FALSE);

            if (FAILED(hr))
                goto Error;

            m_pXMLParser->Run(-1);
        }
        
        // No more data
        if (m_cbRead == 0)
            break;

        m_cbResponseBody += m_cbRead;
    }

    SetState(CXMLHTTP::RESPONSE);

Cleanup:
    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorFail:
    hr = E_FAIL;
    goto Cleanup;

Error:
    goto Cleanup;
}


/*
 -  CXMLHTTP::SetTargetUrl
 -
 *  Purpose:
 *      Set a new target URL
 *
 *  Parameters:
 *      szUrl   IN      New target URL
 *
 *  Errors:
 *      E_OUTOFMEMORY
 */

HRESULT
CXMLHTTP::SetTargetUrl(char * szUrl)
{
    Assert(szUrl);
    
    if (szUrl)
    {
        if (m_bstrTargetUrl)
        {
            SysFreeString(m_bstrTargetUrl);
            m_bstrTargetUrl = NULL;
        }

        if (FAILED(AsciiToBSTR(&m_bstrTargetUrl, szUrl)))
            return E_OUTOFMEMORY;
    }

    return S_OK;
}


/*
 -  InternetStatusCallBack
 -
 *  Purpose:
 *      Handle all WININET status
 *
 *  Parameters:
 *      hInternet           IN  Internet handle
 *      dwContext           IN  Context
 *      dwInternetStatus    IN  Status
 *      lpvStatusInfo       IN  Async info
 *      dwStatusInfoLen     IN  Async info size
 *
 *  Errors:
 *      None
 */

void InternetStatusCallBack(
    HINTERNET       hInternet,
    DWORD           dwContext,
    DWORD           dwInternetStatus,
    LPVOID          lpvStatusInfo,
    DWORD           dwStatusInfoLen)
{
    HRESULT hrCoInit = CoInitialize(NULL);
	#ifdef WIN64 
		DebugBreak(); // we need to implement this before we can use it!
	#endif;

    // Can't use STACK_ENTRY since this is a void function
    //STACK_ENTRY;
    EnsureTls       _EnsureTls;
    HttpContext *   pContext = (HttpContext *)dwContext;
    CXMLHTTP *      pXmlHttp;

    Assert(pContext);

    // Lock down the context structure for the duration of the
    // callback so we don't get the CXMLHTTP object pulled out
    // from beneath us.
    pContext->Lock();

    pXmlHttp = pContext->pXmlHttp;

    if (!_EnsureTls.getTlsData())
        goto Error;

    //
    // When the CXMLHTTP object is reset, the context's object pointer will
    // be set to NULL and the handle will be closed.
    // Two cases:
    //   1) Aborting an in-progress operation - a pending status callback will
    //      block on the Lock() while ReleaseResources() sets pXmlHttp to NULL.
    //      When the callback resumes it sees that the object is being reset and
    //      aborts the operation by closing the handle.
    //   2) Normal cleanup (no pending operation) - pXmlHttp and hHTTP will be
    //      NULL and dwInternetStatus will be INTERNET_STATUS_HANDLE_CLOSING
    //
    
    if (pXmlHttp == NULL)
    {
        if (pContext->hHTTP)
        {
            // Case 1 ONLY: aborting an in-progress operation
            HINTERNET hHTTP = pContext->hHTTP;
            pContext->hHTTP = NULL;
            InternetCloseHandle(hHTTP);
        }
        if (dwInternetStatus == INTERNET_STATUS_HANDLE_CLOSING)
        {
            // Case 1 and 2: Cleanup the context
            delete pContext;
            pContext = NULL;
            ::DecrementComponents();
        }
        goto Cleanup;
    }
    
    switch (dwInternetStatus)
    {
    case INTERNET_STATUS_RESPONSE_RECEIVED:
        // Successfully received a response from the server.
        if (pXmlHttp->m_eState == CXMLHTTP::SENT)
            pXmlHttp->SetState(CXMLHTTP::RECEIVING);
        break;
    
    case INTERNET_STATUS_REQUEST_COMPLETE:
    {
        // Async operation complete
        LPINTERNET_ASYNC_RESULT pAsyncResult;
        pAsyncResult = (LPINTERNET_ASYNC_RESULT)lpvStatusInfo;

        // We use 3 operations that will cause a REQUEST_COMPLETE:
        //            Function           | dwResult |  dwError  | dwError
        //                               |          | (SUCCESS) | (FAIL)
        //  -----------------------------+----------+-----------+---------
        //  InternetQueryDataAvailable() |   BOOL   |  # bytes  |   N/A
        //  InternetReadFile()           |   BOOL   |    N/A    |   N/A
        //  HttpSendRequest()            |   BOOL   |    N/A    |   N/A
        //

        if (pXmlHttp->m_eState == CXMLHTTP::SENDING)
            pXmlHttp->SetState(CXMLHTTP::SENT);

        if (pAsyncResult->dwResult == TRUE)
        {
            if (FAILED(pXmlHttp->ReadResponse()))
                goto Error;
        }
        else
        {
            // Operation failed
            goto Error;
        }
        break;
    }
    
    case INTERNET_STATUS_REDIRECT:
        // An HTTP request is about to automatically redirect the request
        if (SUCCEEDED(pXmlHttp->SetTargetUrl((char *)lpvStatusInfo)))
        {
            pXmlHttp->m_fXDomainUrlChecked = FALSE;
        }
        break;
    }

Cleanup:
    if (pContext)
    {
        pContext->UnLock();
    }

    if (SUCCEEDED(hrCoInit))
        CoUninitialize();
    return;

Error:
    if (pXmlHttp)
    {
        // Set the 'status code' so that users will see a non-HTTP code
        // indicating an asynchronous operation failed.
        pXmlHttp->m_dwStatus = 900;
        pXmlHttp->SetState(CXMLHTTP::RESPONSE);

        // Abort the XMLDOMDoc due to the error
        if (pXmlHttp->m_pXMLDoc)
        {
            pXmlHttp->m_pXMLDoc->abort();
        }
    }
    goto Cleanup;
}


//	gc_mpchhbHalfByteToChar - map a half-byte (low nibble) value to the
//	correspoding ASCII-encoded wide char.  Used to convert a single byte
//	into a hex string representation.

const CHAR gc_mpchhbHalfByteToChar[] =
{
    '0', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', 'A', 'B',
    'C', 'D', 'E', 'F',
};


/*
 -	testUriEscapeChar
 -
 *	Purpose:
 *		test if the given character should be escaped
 *
 *	Parameters:
 *		char			IN	character to test
 *
 *  Returns:
 *      true            if the character should be escaped
 *
 *	Errors:
 *		E_OUTOFMEMORY
 */
bool
testUriEscapeChar(char ch)
{
    return ((((ch >= 0x01) && (ch <= 0x20)) /* First non-printable range */ ||
             ((ch >= 0x80) && (ch <= 0xEF))  /* UTF8 sequence bytes */ ||
             (ch == '%') ||
             (ch == '?') ||
             (ch == '+') ||
             (ch == '&') ||
             (ch == '#')) &&
            !(ch == '\n' || ch == '\r'));
}


/*
 -	UriEscape
 -
 *	Purpose:
 *		Escape a URL
 *
 *	Parameters:
 *		pszSrc			IN	Non-escaped string
 *		ppszDest		OUT	Escaped string
 *		fEscapePercent	IN	Escape % (defaulted for paths)
 *
 *	Errors:
 *		E_OUTOFMEMORY
 */

HRESULT
UriEscape(char * pszSrc, char ** ppszDest, BOOL fEscapePercent = FALSE)
{
	HRESULT	hr = S_OK;
    BYTE	ch;
	char *	pszDest;
	char *	pszTmp;
	UINT	cbSrc;
	UINT	cbDst;
	UINT	ibDst;
	UINT	ibSrc;

	// Set cbSrc to account for the string length of the url
    cbSrc = strlen (pszSrc);

    // how many characters will we need in the target buffer
    cbDst = 0;
    pszTmp = pszSrc;
    while (0 != (ch = *pszTmp++))
    {
        cbDst += testUriEscapeChar(ch) ? 3 : 1;
    }

	// Allocate enough space for the expanded url
    pszDest = new_ne char [cbDst+1];
    if (!pszDest)
    	goto ErrorOutOfMemory;
	
	for (ibSrc = 0, ibDst = 0; ibSrc < cbSrc; ibSrc++)
    {
    	ch = pszSrc[ibSrc];

        // Escape characters that are in the non-printable range
        // but ignore CR and LF.
		//
		// The inclusive ranges escaped are...
		//
		// 0x01 - 0x20		/* First non-printable range */
		// 0x80 - 0xBF		/* Trailing bytes of UTF8 sequence */
		// 0xC0 - 0xDF		/* Leading byte of UTF8 two byte sequence */
		// 0xE0 - 0xEF		/* Leading byte of UTF8 three byte sequence */

        if ((fEscapePercent || (ch != '%')) &&
            testUriEscapeChar(ch))
        {
            //  Insert the escape character
            pszDest[ibDst + 0] = '%';

            //  Convert the low then the high character to hex
            BYTE bDigit = static_cast<BYTE>(ch % 16);
            pszDest[ibDst + 2] = gc_mpchhbHalfByteToChar[bDigit];
            bDigit = static_cast<BYTE>((ch/16) % 16);
            pszDest[ibDst + 1] = gc_mpchhbHalfByteToChar[bDigit];

			//	Adjust for the two extra characters for this sequence
            ibDst += 3;
        }
        else
		{
            pszDest[ibDst] = ch;
			ibDst += 1;
		}
    }

    pszDest[ibDst] = 0;

Cleanup:
	*ppszDest = pszDest;
	return hr;

ErrorOutOfMemory:
	hr = E_OUTOFMEMORY;
	goto Error;

Error:
	goto Cleanup;
}

/*
 -  CXMLHTTP::CrackUrl
 -
 *  Purpose:
 *      Crack a URL and return components
 *
 *	Parameters:
 *      bstrUrl         IN      Target URL
 *      isScheme        OUT     Scheme type
 *      pszHostName     OUT     Host name
 *      ipPort          OUT     Host port
 *      pszPath         OUT     Url path
 *
 *  Errors:
 *      E_FAIL
 *      E_OUTOFMEMORY
 */

HRESULT
CXMLHTTP::CrackUrl(
    BSTR                bstrUrl,
    INTERNET_SCHEME&    isScheme,
    char **             pszHostName,
    INTERNET_PORT&      ipPort,
    char **             pszPath)
{
    HRESULT             hr = S_OK;
    BOOL                fRetCode = FALSE;
    URL_COMPONENTS      urlComp;
    DWORD               dwPathLen;
    WCHAR               wchTemp;
    char *              pszEscapedPath = NULL;
    char *              pszExtraInfo = NULL;
    WCHAR *             pszUrl = NULL;

    // Initialize components
    *pszHostName = NULL;
    *pszPath = NULL;
	isScheme = INTERNET_SCHEME_UNKNOWN;
	ipPort = 0;

    ZeroMemory(&urlComp, sizeof(URL_COMPONENTS));

    urlComp.dwStructSize = sizeof(URL_COMPONENTS);

    urlComp.dwSchemeLength = 1;
    urlComp.dwHostNameLength = 1;
    urlComp.dwUrlPathLength = 1;
    urlComp.dwExtraInfoLength = 1;

    // Crack the URL into scheme, server, path, and extra info
    int l = lstrlen(bstrUrl);
    pszUrl = new_ne WCHAR[l + 1];
    if (!pszUrl)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    ::memcpy(pszUrl, bstrUrl, l * sizeof(WCHAR));
    pszUrl[l] = 0;
    fRetCode = InternetCrackUrl(pszUrl, 0, 0, &urlComp);

    if(!fRetCode)
        goto ErrorFail;

    isScheme = urlComp.nScheme;
    ipPort = urlComp.nPort;

    if (urlComp.lpszHostName)
    {
        wchTemp = urlComp.lpszHostName[urlComp.dwHostNameLength];
        urlComp.lpszHostName[urlComp.dwHostNameLength] = 0;

        hr = BSTRToAscii(pszHostName, urlComp.lpszHostName);

        if (FAILED(hr))
            goto Error;

        urlComp.lpszHostName[urlComp.dwHostNameLength] = wchTemp;
    }
	
    if (urlComp.lpszUrlPath && urlComp.dwUrlPathLength > 0)
    {
        wchTemp = urlComp.lpszUrlPath[urlComp.dwUrlPathLength];
        urlComp.lpszUrlPath[urlComp.dwUrlPathLength] = 0;

        hr = BSTRToUTF8(pszPath, &dwPathLen, urlComp.lpszUrlPath);
	
        if (FAILED(hr))
            goto Error;

        urlComp.lpszUrlPath[urlComp.dwUrlPathLength] = wchTemp;

        UriEscape(*pszPath, &pszEscapedPath);

        delete [] (*pszPath);
        *pszPath = pszEscapedPath;

        if (urlComp.lpszExtraInfo)
        {
            hr = BSTRToAscii(&pszExtraInfo, urlComp.lpszExtraInfo);

            if (FAILED(hr))
                goto Error;

            int     cbPath = lstrlenA(*pszPath) + lstrlenA(pszExtraInfo) + 1;
            char *  szPath = new_ne char [cbPath];

            if (!szPath)
                goto ErrorOutOfMemory;

            lstrcpyA(szPath, *pszPath);
            lstrcatA(szPath, pszExtraInfo);

            delete [] *pszPath;
            *pszPath = szPath;
        }
    }
	
Cleanup:
    delete [] pszUrl;
	if (pszExtraInfo)
		delete [] pszExtraInfo;
    return hr;

ErrorFail:
    hr = E_FAIL;
    goto Cleanup;

ErrorOutOfMemory:
	hr = E_OUTOFMEMORY;
	goto Error;

Error:
    if (*pszHostName)
    {
        delete [] *pszHostName;
        *pszHostName = NULL;
    }
    if (*pszPath)
    {
        delete [] *pszPath;
        *pszPath = NULL;
    }
	isScheme = INTERNET_SCHEME_UNKNOWN;
	ipPort = 0;
    goto Cleanup;
}


/*
 -  CXMLHTTP::AccessAllowed
 -
 *  Purpose:
 *      Compare the base URL and target URL for a cross-domain violation
 *
 *  Parameters:
 *      None
 *
 *  Errors:
 *      E_FAIL
 */

HRESULT
CXMLHTTP::AccessAllowed()
{
    HRESULT         hr = S_OK;

    if (m_fXDomainUrlChecked)
        goto Cleanup;

    // Obtain the base secure URL
    if (_pSecureBaseURL == NULL)
    {
        getSecureBaseURL();
    }

    if (_pSecureBaseURL != NULL)
    {
        BSTR bstrBaseUrl = (*_pSecureBaseURL).getBSTR();

        if (!bstrBaseUrl)
            goto ErrorOutOfMemory;

        hr = UrlOpenAllowed(m_bstrTargetUrl, bstrBaseUrl, FALSE);

        SysFreeString(bstrBaseUrl);

        if (FAILED(hr))
            goto Error;

        m_fXDomainUrlChecked = TRUE;
    }

Cleanup:
    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

Error:
    goto Cleanup;
}


/*
 -  CXMLHTTP::Open
 -
 *  Purpose:
 *      Open a logical HTTP connection
 *
 *  Parameters:
 *      szVerb      IN      HTTP method (GET, PUT, ...)
 *      bstrUrl         IN      Target URL
 *      fSync           IN      Asynchronous flag
 *      bstrUserid      IN      Requesting userid (optional)
 *      bstrPassword    IN      Requesting userid's password (optional)
 *
 *  Errors:
 *      E_FAIL
 *      E_INVALIDARG
 *      E_OUTOFMEMORY
 *      E_ACCESSDENIED
 *      Errors from InternetOpenA and InternetCrackUrlA and InternetConnectA
 *          and HttpOpenRequestA
 */

STDMETHODIMP
CXMLHTTP::open(
    BSTR            bstrVerb,
    BSTR            bstrUrl,
    VARIANT         varAsync,
    VARIANT         varUserid,
    VARIANT         varPassword)
{
    HRESULT         hr = S_OK;
    DWORD           dwLastError = 0;
    BSTR            bstrUserid = NULL;
    BSTR            bstrPassword = NULL;
    char *          szTargetUrl = NULL;
    char *          szVerb = NULL;
    char *          szUserid = NULL;
    char *          szPassword = NULL;
    char *          szHostName = NULL;
    char *          szUrlPath = NULL;
    DWORD           dwHttpOpenFlags = INTERNET_FLAG_KEEP_CONNECTION;
    BOOL            fDisable = TRUE;
    INTERNET_SCHEME isScheme;
    INTERNET_PORT   ipPort;
    INTERNET_STATUS_CALLBACK pfn = NULL;
    URL             url;

    // Check for reinitialization
    if (m_eState != CREATED)
    {
        Reset();
    }

    // Create a new XML document
    hr = CreateXMLDocument();

    if (FAILED(hr))
        goto Error;

    // Validate parameters (userid and password can be empty or null)
    if (!bstrVerb || !bstrUrl || !lstrlenW(bstrVerb) ||
            !lstrlenW(bstrUrl))
        goto ErrorInvalidArg;

    if (GetBaseURL() && GetBaseURL()->length())
    {
        RATCHAR atchars = GetBaseURL()->toCharArrayZ();
        if (SUCCEEDED(url.set(bstrUrl,atchars->getData())) && 
            url.getResolved())
        {
            bstrUrl = url.getResolved();
        }
    }

    // Convert VARIANT's to BSTR's (may result in NULL BSTR's)
    bstrUserid = GetBSTRFromVariant(varUserid);
    bstrPassword = GetBSTRFromVariant(varPassword);

    // Set a flag if both the userid and password have been set. If both are
    // present we will not try to recover from authentication during send().
    if (bstrUserid && bstrPassword && 
        lstrlenW(bstrUserid) > 0 && lstrlenW(bstrPassword) > 0)
    {
        m_fUseridAndPassword = TRUE;
    }

    // Set the target URL and check for cross-domain access
    m_bstrTargetUrl = SysAllocString(bstrUrl);

    if (!m_bstrTargetUrl)
        goto ErrorOutOfMemory;
        
    hr = AccessAllowed();

    if (FAILED(hr))
        goto Error;
        
    // Interpret the asynchronous flag from the user
    m_fAsync = GetBoolFromVariant(varAsync, TRUE);

    // Get the User Agent from URLMON
    if (!m_szUserAgent)
    {
        DWORD cbUserAgent = USER_AGENT_BUFSIZE;

RetryUserAgent:
        m_szUserAgent = new_ne char[cbUserAgent];

        if (!m_szUserAgent)
            goto ErrorOutOfMemory;
            
        ZeroMemory(m_szUserAgent, cbUserAgent);
        
        hr = ObtainUserAgentString(0, m_szUserAgent, &cbUserAgent);

        if (FAILED(hr))
        {
            if (hr == E_OUTOFMEMORY && cbUserAgent)
            {
                delete [] m_szUserAgent;
                goto RetryUserAgent;
            }
            else
                goto Error;
        }
    }
    
    // Initialize wininet
    m_hInet = InternetOpenA(
                m_szUserAgent,
                INTERNET_OPEN_TYPE_PRECONFIG,
                NULL, NULL,
                m_fAsync ? INTERNET_FLAG_ASYNC : 0);

    if (!m_hInet)
        goto ErrorFail;

    // Create the callback context referring to this instance
    m_pContext = new_ne HttpContext(this);

    if (!m_pContext)
        goto ErrorOutOfMemory;

    // Keep a component count on the callback context to avoid a delayed
    // asynchronous callback
    ::IncrementComponents();

    // Set the status callback
    if (m_pfnInternetSetStatusCallback)
    {
        pfn = (*m_pfnInternetSetStatusCallback)(m_hInet, (INTERNET_STATUS_CALLBACK)InternetStatusCallBack);
        if (pfn == INTERNET_INVALID_STATUS_CALLBACK)
            goto ErrorFail;
    }
    else
    {
        goto ErrorFail;
    }

    // Get ASCII versions of the verb, target URL, userid, and password
    hr = BSTRToAscii(&szVerb, bstrVerb);

    if (FAILED(hr))
        goto Error;
		
    Assert(szVerb);
	
    hr = BSTRToAscii(&szUserid, bstrUserid);

    if (FAILED(hr))
        goto Error;

    hr = BSTRToAscii(&szPassword, bstrPassword);

    if (FAILED(hr))
        goto Error;

    // Break the URL into the required components
    // Note: the port will be returned as either the specified port or
    // the default port for the protocol.

    hr = CrackUrl(bstrUrl, isScheme, &szHostName, ipPort, &szUrlPath);

    if (FAILED(hr))
        goto Error;

    // Check for non-http schemes
    if (isScheme != INTERNET_SCHEME_HTTP && isScheme != INTERNET_SCHEME_HTTPS)
        goto ErrorInvalidArg;

    // Disable NTLM pre-authorization to eliminate excessive roundtrips
    InternetSetOption(
        m_hInet,
        INTERNET_OPTION_DISABLE_NTLM_PREAUTH,
        &fDisable, 
        sizeof(BOOL));
    
    m_hSession = InternetConnectA(
                    m_hInet,
                    szHostName,
                    ipPort,
                    NULL,
                    NULL,
                    INTERNET_SERVICE_HTTP,
                    0,
                    NULL);

    if(!m_hSession)
        goto ErrorFail;

    if (isScheme == INTERNET_SCHEME_HTTPS)
    {
        dwHttpOpenFlags |= INTERNET_FLAG_SECURE;
    }
    
    m_hHTTP = HttpOpenRequestA(
                m_hSession,
                szVerb,
                szUrlPath,
                NULL, // "HTTP/1.1", (according to richards).
                NULL,
                NULL,
                dwHttpOpenFlags,
                (DWORD_PTR)m_pContext);

    if(!m_hHTTP)
        goto ErrorFail;

    m_pContext->hHTTP = m_hHTTP;

    // Set Username and Password.
    InternetSetOptionA(
        m_hHTTP, 
        INTERNET_OPTION_USERNAME, 
        szUserid, 
        szUserid ? sizeof(szUserid) : 0);

    InternetSetOptionA(
        m_hHTTP, 
        INTERNET_OPTION_PASSWORD, 
        szPassword,
        szPassword ? sizeof(szPassword) : 0);

    SetState(OPENED);

Cleanup:
    if (bstrUserid)
        SysFreeString(bstrUserid);
    if (bstrPassword)
        SysFreeString(bstrPassword);
    if (szTargetUrl)
        delete [] szTargetUrl;
    if (szVerb)
        delete [] szVerb;
    if (szUserid)
        delete [] szUserid;
    if (szPassword)
        delete [] szPassword;
    if (szHostName)
        delete [] szHostName;
    if (szUrlPath)
        delete [] szUrlPath;

	// Clear the URL to ensure it gets cleaned up (an exception from
	// SetErrorInfo() will prevent the destructor for url being called).
	url.clear();
	
    SetErrorInfo(hr, dwLastError);
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorFail:
    hr = E_FAIL;
    dwLastError = GetLastError();
    goto Cleanup;

Error:
    Reset();
    dwLastError = hr;
    goto Cleanup;
}


/*
 -  CXMLHTTP::abort
 -
 *  Purpose:
 *      Abort the current send (asynchrononous ONLY)
 *
 *  Parameters:
 *      None
 *
 *  Errors:
 *      E_FAIL
 */

STDMETHODIMP
CXMLHTTP::abort()
{
    if (!m_fAsync || m_eState == CREATED)
        goto Cleanup;

    // Abort the XML document (if we have one)
    if (m_pXMLDoc)
    {
        m_pXMLDoc->abort();
    }

    // Reset the object
    Reset();

Cleanup:
    return S_OK;
}


/*
 -  CXMLHTTP::GetWindow
 -
 *  Purpose:
 *      Obtain a window handle from our container
 *
 *  Parameters:
 *      hwnd    OUT     Window handle
 *
 *  Errors:
 *      E_FAIL
 */

HRESULT
CXMLHTTP::GetWindow(HWND& hwnd)
{
    HRESULT             hr = S_OK;
    IOleClientSite *    pClientSite = NULL;
    IOleContainer *     pContainer = NULL;
    IOleWindow *        pOleWindow = NULL;
    IServiceProvider *  pSP = NULL;

    hwnd = NULL;

    if (!_pSite)
        goto Cleanup;

    if (SUCCEEDED(_pSite->QueryInterface(IID_IOleClientSite, 
                                         (void**)&pClientSite)))
    {
        // Get the Trident window handle
        hr = pClientSite->GetContainer(&pContainer);
    
        if (FAILED(hr))
            goto Error;

        hr = pContainer->QueryInterface(IID_IOleWindow, (void **)&pOleWindow);

        if (FAILED(hr))
            goto Error;
    }
    else
    {
        // Get the top-level browser window handle
        hr = _pSite->QueryInterface(IID_IServiceProvider, (void **)&pSP);

        if (FAILED(hr))
            goto Error;

        hr = pSP->QueryService(
                    SID_STopLevelBrowser,
                    IID_IOleWindow,
                    (void **)&pOleWindow);

        if (FAILED(hr))
            goto Error;
    }

    if (pOleWindow)
    {
        hr = pOleWindow->GetWindow(&hwnd);

        if (FAILED(hr))
            goto Error;
    }
    
    // If we failed to get a handle from IE then try Win32
    if (!hwnd)
    {
        hwnd = GetActiveWindow();
    }
    
Cleanup:
    SafeRelease(pClientSite);
    SafeRelease(pContainer);
    SafeRelease(pOleWindow);    
    SafeRelease(pSP);   
    return hr;

Error:
    goto Cleanup;
}


/*
 -  CXMLHTTP::Authenticate
 -
 *  Purpose:
 *      Check for a 401 or 407 and redrive a send if necessary
 *
 *  Parameters:
 *      None
 *
 *  Errors:
 *
 */

BOOL
CXMLHTTP::Authenticate()
{
    DWORD       dwError;
    HWND        hwnd = NULL;
    BOOL        fResend = FALSE;

    // If the userid and password were set, do not let InternetErrorDlg
    // handle any authentication errors.
    if (!m_fUseridAndPassword)
    {
        // Get a window handle for InternetErrorDlg
        GetWindow(hwnd);

        if (hwnd)
        {
            dwError = InternetErrorDlg(
                        hwnd,
                        m_hHTTP,
                        ERROR_SUCCESS, 
                        FLAGS_ERROR_UI_FILTER_FOR_ERRORS | 
                        FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS |
                        FLAGS_ERROR_UI_FLAGS_GENERATE_DATA,
                        NULL);

            // Ignore any errors - status code will reflect
            // authentication errors
            if (dwError == ERROR_INTERNET_FORCE_RETRY)
            {
                fResend = TRUE;
            }
        }
    }
    
    return fResend;
}

   
/*
 -  CXMLHTTP::SetRequiredRequestHeaders
 -
 *  Purpose:
 *      Set implicit request headers
 *
 *  Parameters:
 *      None
 *
 *  Errors:
 *      E_FAIL
 *      E_UNEXPECTED
 *      E_OUTOFMEMORY
 *      Errors from HttpAddRequestHeaders and HttpSendRequest
 */

HRESULT
CXMLHTTP::SetRequiredRequestHeaders()
{
    HRESULT     hr = S_OK;
    char        rgchTemp[15];

    // Add a Referer header if we know the base URL
    if (_pSecureBaseURL != NULL)
    {
        hr = SetRequestHeader(
                HTTP_HEADER_REFERRER,
                AsciiText(_pSecureBaseURL),
                TRUE);

        if (FAILED(hr))
            goto Error;
    }
    
    // Add a Content-Length header for non-chunked bodies
    if (!m_fChunkedRequest)
    {
        hr = SetRequestHeader(
                HTTP_HEADER_CONTENTLEN,
                _ltoa(m_cbRequestBody, rgchTemp, 10),
                TRUE);

        if (FAILED(hr))
            goto Error;
    }

Cleanup:
    return hr;

Error:
    goto Cleanup;
}


/*
 -  CXMLHTTP::send
 -
 *  Purpose:
 *      Send the HTTP request
 *
 *  Parameters:
 *      varBody     IN      Request body
 *      fResend     IN      Resend indication
 *
 *  Errors:
 *      E_FAIL
 *      E_UNEXPECTED
 *      E_OUTOFMEMORY
 *      Errors from HttpAddRequestHeaders and HttpSendRequest
 */

STDMETHODIMP
CXMLHTTP::send(VARIANT varBody, BOOL fResend)
{
    HRESULT     hr = S_OK;
    DWORD       dwLastError = 0;
    BOOL        fRetCode = FALSE;

    if (!fResend)
    {
        // Validate state
        if (m_eState != OPENED)
            goto ErrorUnexpected;

        // Get the request body
        hr = SetRequestBody(varBody);

        if (FAILED(hr))
            goto Error;

        hr = SetRequiredRequestHeaders();

        if (FAILED(hr))
            goto Error;
    }
    
    SetState(SENDING);
    
RetrySend:
    // Send the HTTP request
    fRetCode = HttpSendRequest(
                    m_hHTTP,
                    NULL, 0,            // No header info here
                    m_szRequestBuffer,
                    m_cbRequestBody);

    if (!fRetCode)
    {
        if (m_fAsync && GetLastError() == ERROR_IO_PENDING)
            goto Cleanup;
        else
            goto ErrorFail;
    }

    // We get to this point for synchronous requests OR asynchronous
    // requests that do not pend (ie cached data)
    if (Authenticate())
        goto RetrySend;     

    // Determine the content length and type
    hr = GetContentType();

    if (FAILED(hr))
        goto Error;

    // Read the response data
    hr = ReadResponse();

    if (FAILED(hr))
        goto Error;

Cleanup:
    SetErrorInfo(hr, dwLastError);
    return hr;

ErrorUnexpected:
    hr = E_UNEXPECTED;
    goto Error;

ErrorFail:
    hr = E_FAIL;
    dwLastError = GetLastError();
    goto Cleanup;

Error:
    dwLastError = hr;
    goto Cleanup;
}


/*
 -  ReadFromStream
 -
 *  Purpose:
 *      Extract the contents of a stream into a buffer.
 *
 *  Parameters:
 *      ppBuf   IN/OUT      Buffer
 *      pStm    IN          Stream
 *
 *  Errors:
 *      E_INVALIDARG
 *      E_OUTOFMEMORY
 */

HRESULT
ReadFromStream(char ** ppData, ULONG * pcbData, IStream * pStm)
{
    HRESULT     hr = S_OK;
    char *      pBuffer = NULL;	    // Buffer
    ULONG       cbBuffer = 0;       // Bytes in buffer
    ULONG       cbData = 0;         // Bytes of data in buffer
    ULONG       cbRead = 0;         // Bytes read from stream
    ULONG       cbNewSize = 0;
    char *      pNewBuf = NULL;

    if (!ppData || !pStm)
        goto ErrorInvalidArg;

    *ppData = NULL;
    *pcbData = 0;
    
    while (TRUE)
    {
        if (cbData + 512 > cbBuffer)
        {
            cbNewSize = (cbData ? cbData*2 : 4096);
            pNewBuf = new_ne char[cbNewSize+1];

            if (!pNewBuf)
                goto ErrorOutOfMemory;

			if (cbData)
                ::memcpy(pNewBuf, pBuffer, cbData);

            cbBuffer = cbNewSize;
            delete[] pBuffer;
            pBuffer = pNewBuf;
            pBuffer[cbData] = 0;
        }

        hr = pStm->Read(
                    &pBuffer[cbData],
                    cbBuffer - cbData,
                    &cbRead);

	    if (FAILED(hr))
            goto Error;
            
        cbData += cbRead;
        pBuffer[cbData] = 0;

        // No more data
        if (cbRead == 0)
            break;
    }

    *ppData = pBuffer;
    *pcbData = cbData;

Cleanup:
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

Error:
    if (pBuffer)
        delete [] pBuffer;
    goto Cleanup;
}


/*
 -  CXMLHTTP::LoadViaPersistStream
 -
 *  Purpose:
 *      Set the request body - given an XML DOM document
 *
 *  Parameters:
 *      pDispatch       IN      IDispatch for an XML DOM Document
 *
 *  Errors:
 *      E_INVALIDARG
 *      E_OUTOFMEMORY
 */

HRESULT
CXMLHTTP::LoadViaPersistStream(IDispatch * pDispatch)
{
    HRESULT hr = SaveDocument(pDispatch, (BYTE**)&m_szRequestBuffer, &m_cbRequestBody);
    if (FAILED(hr))
        goto Error;

    hr = SetRequestHeader(HTTP_HEADER_CONTENTTYPE, TYPE_TEXTXML, TRUE);

    if (FAILED(hr))
        goto Error;

Cleanup:
    return hr;

Error:
    goto Cleanup;

}



/*
 -  CXMLHTTP::SetRequestBody
 -
 *  Purpose:
 *      Set the request body
 *
 *  Parameters:
 *      varBody     IN      Request body
 *
 *  Errors:
 *      E_FAIL
 *      E_OUTOFMEMORY
 *      E_UNEXPECTED
 */

HRESULT
CXMLHTTP::SetRequestBody(VARIANT varBody)
{
    HRESULT             hr = S_OK;
    VARIANT             varTemp;
    BSTR                bstrBody = NULL;
    SAFEARRAY *         psa = NULL;
    VARIANT *           pvarBody = NULL;
    IStream *           pStm = NULL;
    
    VariantInit(&varTemp);

    // Validate state
    if (m_eState != OPENED)
        goto ErrorUnexpected;

    // Free a previously set body and its response
    if(m_szRequestBuffer)
    {
        delete [] m_szRequestBuffer;
        m_szRequestBuffer = NULL;
        m_cbRequestBody = 0;

        if (m_szResponseBuffer)
        {
            delete [] m_szResponseBuffer;
            m_szResponseBuffer = NULL;
            m_cbResponseBuffer = 0;
            m_cbResponseBody = 0;
        }
    }

    if (V_ISBYREF(&varBody))
    {
        pvarBody = varBody.pvarVal;
    }
    else
    {
        pvarBody = &varBody;
    }

    // Check for an empty body
    if (V_VT(pvarBody) == VT_EMPTY || V_VT(pvarBody) == VT_NULL ||
            V_VT(pvarBody) == VT_ERROR)
        goto Cleanup;

    //
    // Extract the body: BSTR, IDispatch, array of UI1, or IStream
    //

    // We need to explicitly look for the byte array since it will be converted
    // to a BSTR by VariantChangeType.
    if (V_ISARRAY(pvarBody) && (V_VT(pvarBody) & VT_UI1))
    {
        BYTE *  pb = NULL;
        long    lUBound = 0;
        long    lLBound = 0;

        psa = V_ARRAY(pvarBody);

        // We only handle 1 dimensional arrays
        if (SafeArrayGetDim(psa) != 1)
            goto ErrorFail;

        // Get access to the blob
        hr = SafeArrayAccessData(psa, (void **)&pb);

        if (FAILED(hr))
            goto Error;

        // Compute the data size from the upper and lower array bounds
        hr = SafeArrayGetLBound(psa, 1, &lLBound);

        if (FAILED(hr))
            goto Error;

        hr = SafeArrayGetUBound(psa, 1, &lUBound);

        if (FAILED(hr))
            goto Error;

        m_cbRequestBody = lUBound - lLBound + 1;

        if (m_cbRequestBody > 0)
        {
            // Copy the data into the request buffer
            m_szRequestBuffer = new_ne char [m_cbRequestBody];

            if (!m_szRequestBuffer)
            {
                m_cbRequestBody = 0;
                goto ErrorOutOfMemory;
            }
            
            ::memcpy(m_szRequestBuffer, pb, m_cbRequestBody);
        }
        
        SafeArrayUnaccessData(psa);
        psa = NULL;
    }
    else
    {
        // Try a BSTR
        bstrBody = GetBSTRFromVariant(*pvarBody);

        if (bstrBody)
        {
            hr = BSTRToUTF8(&m_szRequestBuffer, &m_cbRequestBody, bstrBody);

            if (FAILED(hr))
                goto Error;
        }
        else
        {
            // Try an XML document
            hr = VariantChangeType(
                    &varTemp,
                    pvarBody,
                    VARIANT_NOVALUEPROP,
                    VT_DISPATCH);

            if (SUCCEEDED(hr))
            {
                hr = LoadViaPersistStream(V_DISPATCH(&varTemp));

                if (FAILED(hr))
                    goto Error;
            }
            else
            {
                // Try a Stream
                if (pvarBody->punkVal)
                {
                    hr = pvarBody->punkVal->QueryInterface(
                                                IID_IStream,
                                                (void **)&pStm);

                    if (FAILED(hr))
                        goto Error;

                    hr = ReadFromStream(
                            &m_szRequestBuffer,
                            &m_cbRequestBody,
                            pStm);

                    if (FAILED(hr))
                        goto Error;
                }
            }           
        }
    }
    
Cleanup:
    VariantClear(&varTemp);
    if (bstrBody)
        SysFreeString(bstrBody);
    if (psa)
        SafeArrayUnaccessData(psa);
    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorUnexpected:
    hr = E_UNEXPECTED;
    goto Error;

ErrorFail:
    hr = E_FAIL;
    goto Error;

Error:
    goto Cleanup;
}


/*
 -  CXMLHTTP::SetRequestHeader
 -
 *  Purpose:
 *      Set a request header (internal)
 *
 *  Parameters:
 *      szHeader    IN      HTTP request header
 *      szValue     IN      Header value
 *
 *  Errors:
 *      E_FAIL
 *      E_OUTOFMEMORY
 *      Errors from HttpAddRequestHeadersA
 */

HRESULT
CXMLHTTP::SetRequestHeader(char * szHeader, char * szValue, BOOL fReplace/*= FALSE*/)
{
    HRESULT     hr = S_OK;
    BOOL        fRetCode = FALSE;
    const char* pszHeaderSeparator = ": ";
    const char* pszHeaderEndRecord = "\r\n";
    int         cchHeaderValue;
    char *      szHeaderValue = NULL;
    DWORD       dwModifiers = HTTP_ADDREQ_FLAG_ADD;

    Assert(szHeader);
    Assert(szValue);

    cchHeaderValue = strlen(szHeader) + strlen(pszHeaderSeparator) +
                        strlen(szValue) + strlen(pszHeaderEndRecord) + 1;

    szHeaderValue = new_ne char[cchHeaderValue];

    if (!szHeaderValue)
        goto ErrorOutOfMemory;

    lstrcpyA(szHeaderValue, szHeader);
    lstrcatA(szHeaderValue, pszHeaderSeparator);
    lstrcatA(szHeaderValue, szValue);
    lstrcatA(szHeaderValue, pszHeaderEndRecord);
    
    // For blank header values, erase the header by setting the
    // REPLACE flag.
    if (fReplace || strlen(szValue) == 0)
    {
        dwModifiers |= HTTP_ADDREQ_FLAG_REPLACE;
    }
    
    fRetCode = HttpAddRequestHeadersA(
                    m_hHTTP,
                    szHeaderValue,
                    -1L,
                    dwModifiers);

    if (!fRetCode)
        goto ErrorFail;

Cleanup:
    if (szHeaderValue)
        delete [] szHeaderValue;
    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorFail:
    hr = E_FAIL;
    goto Cleanup;

Error:
    goto Cleanup;
}


/*
 -  CXMLHTTP::setRequestHeader
 -
 *  Purpose:
 *      Set a request header
 *
 *  Parameters:
 *      bstrHeader      IN      HTTP request header
 *      bstrValue       IN      Header value
 *
 *  Errors:
 *      E_FAIL
 *      E_INVALIDARG
 *      E_UNEXPECTED
 */

STDMETHODIMP
CXMLHTTP::setRequestHeader(BSTR bstrHeader, BSTR bstrValue)
{
    HRESULT     hr = S_OK;
    DWORD       dwLastError = 0;
    char *      szHeader = NULL;
    char *      szValue = NULL;
    char        chNullValue = 0;

    // Validate header parameter (null or zero-length value is allowed)
    if (!bstrHeader || !lstrlenW(bstrHeader))
        goto ErrorInvalidArg;

    // Validate state
    if (m_eState != OPENED)
        goto ErrorUnexpected;

    // Convert the header and value to ANSI
    hr = BSTRToAscii(&szHeader, bstrHeader);

    if (FAILED(hr))
        goto Error;

    hr = BSTRToAscii(&szValue, bstrValue);

    if (FAILED(hr))
        goto Error;

    hr = SetRequestHeader(szHeader, szValue ? szValue : &chNullValue);

    if (FAILED(hr))
        goto Error;

    if (!lstrcmpiA(szHeader, "transfer-encoding") &&
        !lstrcmpiA(szValue, "chunked"))
    {
        m_fChunkedRequest = TRUE;
    }
    
Cleanup:
    if (szHeader)
        delete [] szHeader;
    if (szValue)
        delete [] szValue;
    SetErrorInfo(hr, dwLastError);
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorUnexpected:
    hr = E_UNEXPECTED;
    goto Error;

Error:
    dwLastError = hr;
    goto Cleanup;
}


/*
 -  CXMLHTTP::getResponseHeader
 -
 *  Purpose:
 *      Get a response header
 *
 *  Parameters:
 *      bstrHeader      IN      HTTP request header
 *      pbstrValue      OUT     Header value
 *
 *  Errors:
 *      E_FAIL
 *      E_INVALIDARG
 *      E_OUTOFMEMORY
 *      E_UNEXPECTED
 *      Errors from HttpQueryInfoA
 */

STDMETHODIMP
CXMLHTTP::getResponseHeader(BSTR bstrHeader, BSTR * pbstrValue)
{
    HRESULT     hr = S_OK;
    DWORD       dwLastError = 0;
    char *      szHeader = NULL;
    DWORD       cb;
    BOOL        fRetCode;

    // Validate parameters
    if (!bstrHeader || !pbstrValue || !lstrlenW(bstrHeader))
        goto ErrorInvalidArg;

    // Validate state
    if (m_eState < SENT)
        goto ErrorUnexpected;

    // Convert the header to ANSI
    hr = BSTRToAscii(&szHeader, bstrHeader);

    if (FAILED(hr))
        goto Error;

    Assert(szHeader);

    cb = strlen(szHeader) + 1; 

RetryQuery:
    // Determine length of requested header
    fRetCode = HttpQueryInfoA(
                    m_hHTTP,
                    HTTP_QUERY_CUSTOM,
                    szHeader,
                    &cb,
                    0);

    // Check for ERROR_INSUFFICIENT_BUFFER - reallocate the buffer and retry
    if (!fRetCode)
    {
        switch(GetLastError())
        {
        case ERROR_INSUFFICIENT_BUFFER:
        {
            char * szTemp = szHeader;

            szHeader = new_ne char[cb];

            if (!szHeader)
                goto ErrorOutOfMemory;

            lstrcpyA(szHeader, szTemp);
            delete [] szTemp;
            goto RetryQuery;
        }

        case ERROR_HTTP_HEADER_NOT_FOUND:
        {
            // Give back a blank string
            szHeader[0] = 0;
            break;
        }
        
        default:
            goto ErrorFail;
        }
    }

    hr = AsciiToBSTR(pbstrValue, szHeader);

    if (FAILED(hr))
        goto Error;
        
Cleanup:
    if (szHeader)
        delete [] szHeader;
    SetErrorInfo(hr, dwLastError);
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorUnexpected:
    hr = E_UNEXPECTED;
    goto Error;

ErrorFail:
    hr = E_FAIL;
    dwLastError = GetLastError();
    goto Cleanup;

Error:
    dwLastError = hr;
    goto Cleanup;
}


/*
 -  CXMLHTTP::getAllResponseHeaders
 -
 *  Purpose:
 *      Return the response headers
 *
 *  Parameters:
 *      pbstrHeaders    IN/OUT      CRLF delimited headers
 *
 *  Errors:
 *      E_FAIL
 *      E_INVALIDARG
 *      E_OUTOFMEMORY
 *      E_UNEXPECTED
 */

STDMETHODIMP
CXMLHTTP::getAllResponseHeaders(BSTR * pbstrHeaders)
{
    HRESULT     hr = S_OK;
    BOOL        fRetCode;
    DWORD       dwLastError = 0;
    char *      szAllHeaders = NULL;
    char *      szFirstHeader = NULL;
    DWORD       cb = 0;

    // Validate parameter
    if (!pbstrHeaders)
        goto ErrorInvalidArg;

    // Validate state
    if (m_eState < SENT)
        goto ErrorUnexpected;

    *pbstrHeaders = NULL;

RetryQuery:
    // Determine the length of all headers and then get all the headers
    fRetCode = HttpQueryInfoA(
                    m_hHTTP,
                    HTTP_QUERY_RAW_HEADERS_CRLF,
                    szAllHeaders,
                    &cb,
                    0);

    if (!fRetCode)
    {
        // Allocate a buffer large enough to hold all headers
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            szAllHeaders = new_ne char[cb];

            if (!szAllHeaders)
                goto ErrorOutOfMemory;

            goto RetryQuery;
        }
        else
        {
            goto ErrorFail;
        }
    }

    // Bypass status line - jump past first CRLF (0x13, 0x10)
    szFirstHeader = szAllHeaders;

    while (*szFirstHeader != 10 && *szFirstHeader != 0)
    {
        szFirstHeader++;
    }

    if (*szFirstHeader == 10)
    {
        hr = AsciiToBSTR(pbstrHeaders, szFirstHeader + 1);

        if (FAILED(hr))
            goto Error;
    }
    
Cleanup:
    if (szAllHeaders)
        delete [] szAllHeaders;

    SetErrorInfo(hr, dwLastError);
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorUnexpected:
    hr = E_UNEXPECTED;
    goto Error;

ErrorFail:
    hr = E_FAIL;
    dwLastError = GetLastError();
    goto Cleanup;

Error:
    if (pbstrHeaders)
    {
        SysFreeString(*pbstrHeaders);
        *pbstrHeaders = NULL;
    }
    dwLastError = hr;
    goto Cleanup;
}


/*
 -  CXMLHTTP::get_status
 -
 *  Purpose:
 *      Get the request status code
 *
 *  Parameters:
 *      plStatus        OUT     HTTP request status code
 *
 *  Errors:
 *      E_FAIL
 *      E_INVALIDARG
 *      E_UNEXPECTED
 */

STDMETHODIMP
CXMLHTTP::get_status(long * plStatus)
{
    HRESULT hr = S_OK;
    DWORD   dwLastError = 0;
    DWORD   dwStatusCode;
    DWORD   cb = sizeof(DWORD);
    BOOL    fRetCode;

    // Validate parameter
    if (!plStatus)
        goto ErrorInvalidArg;

    // Validate state
    if (m_eState < SENT)
        goto ErrorUnexpected;

    if (m_dwStatus)
    {
        *plStatus = m_dwStatus;
        goto Cleanup;
    }
    
    fRetCode = HttpQueryInfoA(
                    m_hHTTP,
                    HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                    &dwStatusCode,
                    &cb,
                    0);

    if(!fRetCode)
        goto ErrorFail;

    *plStatus = dwStatusCode;

Cleanup:
    SetErrorInfo(hr, dwLastError);
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorUnexpected:
    hr = E_UNEXPECTED;
    goto Error;

ErrorFail:
    hr = E_FAIL;
    dwLastError = GetLastError();
    goto Cleanup;

Error:
    dwLastError = hr;
    goto Cleanup;
}


/*
 -  CXMLHTTP::get_statusText
 -
 *  Purpose:
 *      Get the request status text
 *
 *  Parameters:
 *      pbstrStatus     OUT     HTTP request status text
 *
 *  Errors:
 *      E_FAIL
 *      E_INVALIDARG
 *      E_UNEXPECTED
 */

STDMETHODIMP
CXMLHTTP::get_statusText(BSTR * pbstrStatus)
{
    HRESULT     hr = S_OK;
    DWORD       dwLastError = 0;
    char *      szStatusText = NULL;
    DWORD       cb = 0;
    BOOL        fRetCode;

    // Validate parameter
    if (!pbstrStatus)
        goto ErrorInvalidArg;

    // Validate state
    if (m_eState < SENT)
        goto ErrorUnexpected;

RetryQuery:
    fRetCode = HttpQueryInfoA(
                    m_hHTTP,
                    HTTP_QUERY_STATUS_TEXT,
                    szStatusText,
                    &cb,
                    0);

    if (!fRetCode)
    {
        // Check for ERROR_INSUFFICIENT_BUFFER error
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            // Reallocate the status text buffer
            if (szStatusText)
                delete szStatusText;

            szStatusText = new_ne char[cb];

            if (!szStatusText)
                goto ErrorOutOfMemory;

            goto RetryQuery;
        }
        else
        {
            goto ErrorFail;
        }
    }

    // Convert the status text to a BSTR
    hr = AsciiToBSTR(pbstrStatus, szStatusText);

    if (FAILED(hr))
        goto Error;

Cleanup:
    if (szStatusText)
        delete [] szStatusText;
    SetErrorInfo(hr, dwLastError);
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorUnexpected:
    hr = E_UNEXPECTED;
    goto Error;

ErrorFail:
    hr = E_FAIL;
    dwLastError = GetLastError();
    goto Cleanup;

Error:
    dwLastError = hr;
    goto Cleanup;
}


/*
 -  CXMLHTTP::get_readyState
 -
 *  Purpose:
 *      Get the ready state for the object
 *
 *  Parameters:
 *      plState     OUT     Ready state
 *
 *  Errors:
 *      E_INVALIDARG
 */

STDMETHODIMP
CXMLHTTP::get_readyState(long * plState)
{
    HRESULT     hr = S_OK;
    DWORD       dwLastError = 0;
    BOOL        fRetCode = FALSE;
    DWORD       dwStatusCode = 0;
    DWORD       cb = sizeof(dwStatusCode);

    if (!plState)
        goto ErrorInvalidArg;

    switch (m_eState)
    {
    case OPENED:
    case SENDING:
        *plState = 1;//READYSTATE_LOADING
        break;

    case SENT:
    {
        *plState = 2;//READYSTATE_LOADED

        // Asynchronous authentication
        fRetCode = HttpQueryInfoA(
                        m_hHTTP,
                        HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                        &dwStatusCode,
                        &cb,
                        0);

        if (fRetCode && (dwStatusCode == HTTP_STATUS_DENIED ||
                dwStatusCode == HTTP_STATUS_PROXY_AUTH_REQ))
        {
            VARIANT varEmpty;

            VariantInit(&varEmpty);

            if (Authenticate())
            {
                hr = send(varEmpty, TRUE);

                if (FAILED(hr))
                    goto Error;
            }
        }

        break;
    }
    
    case RECEIVING:
        *plState = 3;//READYSTATE_INTERACTIVE
        break;
    
    case RESPONSE:
        *plState = 4;//READYSTATE_COMPLETE
        break;
        
    case CREATED:
    default:
        *plState = 0;//READYSTATE_UNINITIALIZED
    }

Cleanup:
    SetErrorInfo(hr, dwLastError);
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

Error:
    dwLastError = hr;
    goto Cleanup;
}

   
STDMETHODIMP
CXMLHTTP::put_onreadystatechange(IDispatch * pReadyStateSink)
{
    if (m_pReadyStateSink)
        m_pReadyStateSink->Release();

    m_pReadyStateSink = pReadyStateSink;
    if (m_pReadyStateSink)
        m_pReadyStateSink->AddRef();

    if (! m_hWnd)
    {
        // Lazy construct the hidden window for invoking this event sink
        // on the right thread.
        WNDCLASS wc;
        memset(&wc, 0, sizeof(wc));

        // Register a window class, if necessary
        if (!GetClassInfo(g_hInstance, c_wszXMLHTTPMsgClass, &wc))
        {
            wc.lpfnWndProc = WndProcMain;
            wc.hInstance = g_hInstance;
            wc.hIcon = NULL;
            wc.lpszClassName = c_wszXMLHTTPMsgClass;
            wc.hbrBackground = NULL;

            // Register the class - if this fails we are fubar
            RegisterClass(&wc);
        }

        m_hWnd = CreateWindow(
                    c_wszXMLHTTPMsgClass,
                    NULL, 0, 0, 0, 0, 0, NULL, NULL,
                    g_hInstance, NULL);
    }

    return S_OK;
}


/*
 -  CXMLHTTP::CreateXMLDocument
 -
 *  Purpose:
 *      Create an XML DOM object
 *
 *  Parameters:
 *      None
 *
 *  Notes:
 *      This should only be called from the main thread since
 *      accessing _pSite is not thread safe.
 *
 *  Errors:
 *      
 */

HRESULT
CXMLHTTP::CreateXMLDocument()
{
    HRESULT             hr = S_OK;
    IObjectWithSite *   pObjectWithSite = NULL;
    IObjectSafety *     pSafety = NULL;

    // Clean up an existing doc
    if (m_pXMLDoc)
    {
        SafeRelease(m_pXMLDoc);
        SafeRelease(m_pXMLParser);
        m_pXMLDoc = NULL;
        m_pXMLParser = NULL;
    }

    hr = CreateDOMDocument(IID_IXMLDOMDocument, (void **)(&m_pXMLDoc));

    if (FAILED(hr))
        goto Error;

    // Set the synchronous flag
    hr = m_pXMLDoc->put_async(VARIANT_FALSE);

    if (FAILED(hr))
        goto Error;

    // Set the validate flag
    hr = m_pXMLDoc->put_validateOnParse(VARIANT_FALSE);

    if (FAILED(hr))
        goto Error;

    // Set the resolveExternals flag
    hr = m_pXMLDoc->put_resolveExternals(VARIANT_FALSE);

    if (FAILED(hr))
        goto Error;

    // Setup security !!
    // First we need to tell the document about our site
    hr = m_pXMLDoc->QueryInterface(IID_IObjectWithSite, (LPVOID *)&pObjectWithSite);

    Assert(SUCCEEDED(hr) && "Failed to get IObjectWithSite");
    
    if (FAILED(hr))
        goto Error;

    hr = pObjectWithSite->SetSite((IUnknown *)_pSite); // CSafeControl member.

    if (FAILED(hr))
        goto Error;

    // Now we tell it what our safety options are from CSafeControl member.
    hr = m_pXMLDoc->QueryInterface(IID_IObjectSafety, (LPVOID *)&pSafety);

    Assert(SUCCEEDED(hr) && "Failed to get IObjectSafety");

    if (FAILED(hr))
        goto Error;

    hr = pSafety->SetInterfaceSafetyOptions(IID_IUnknown, _dwSafetyOptions, _dwSafetyOptions); 

    if (FAILED(hr))
        goto Error;

    hr = m_pXMLDoc->QueryInterface(
                        IID_IXMLParser, 
                        (void **)&m_pXMLParser);

    if (FAILED(hr))
        goto Error;

Cleanup:
    SafeRelease(pObjectWithSite);
    SafeRelease(pSafety);
    return hr;

Error:
    SafeRelease(m_pXMLDoc);
    goto Cleanup;
}


/*
 -  CXMLHTTP::get_responseXML
 -
 *  Purpose:
 *      Get the response body as an XML DOM, if the response is in XML
 *
 *  Parameters:
 *      ppBody  OUT     XML DOM
 *
 *  Errors:
 *      E_INVALIDARG
 *      E_UNEXPECTED
 *      E_PENDING
 */

STDMETHODIMP
CXMLHTTP::get_responseXML(IDispatch ** ppBody)
{
    HRESULT             hr = S_OK;
    DWORD               dwLastError = 0;

    // Validate parameter
    if (!ppBody)
        goto ErrorInvalidArg;

    // Validate state
    if (m_eState != RESPONSE)
    {
        // Look for a premature call in asynchronous mode
        if (m_fAsync)
            goto ErrorPending;
        goto ErrorUnexpected;
    }

    // Last-minute (redirect) access check - reset the object if
    // access is not allowed
    hr = AccessAllowed();

    if (FAILED(hr))
    {
        if (hr == E_ACCESSDENIED)
        {
            Reset();
        }
        goto Error;
    }

    if (m_pXMLDoc)
    {
        // Get the IDispatch for the XML DOM to return to the caller
        hr = m_pXMLDoc->QueryInterface(
                            IID_IDispatch, 
                            (void **)ppBody);

        if (FAILED(hr))
            goto Error;
    }
    else
    {
        hr = E_FAIL;
        *ppBody = NULL;
    }

Cleanup:
    SetErrorInfo(hr, dwLastError);
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorUnexpected:
    hr = E_UNEXPECTED;
    goto Error;

ErrorPending:
    hr = E_PENDING;
    goto Error;

Error:
    dwLastError = hr;
    goto Cleanup;
}


/*
 -  CXMLHTTP::get_responseBody
 -
 *  Purpose:
 *      Retrieve the response body as a SAFEARRAY of UI1
 *
 *  Parameters:
 *      pvarBody    OUT     Response blob
 *
 *  Errors:
 *      E_INVALIDARG
 *      E_UNEXPECTED
 *      E_PENDING
 */

STDMETHODIMP
CXMLHTTP::get_responseBody(VARIANT * pvarBody)
{
    HRESULT hr = S_OK;
    DWORD   dwLastError = 0;

    // Validate parameter
    if (!pvarBody)
        goto ErrorInvalidArg;

    // Validate state
    if (m_eState != RESPONSE)
    {
        // Look for a premature call in asynchronous mode
        if (m_fAsync)
            goto ErrorPending;
        goto ErrorUnexpected;
    }

    VariantInit(pvarBody);

    // Last-minute (redirect) access check - reset the object if
    // access is not allowed
    hr = AccessAllowed();

    if (FAILED(hr))
    {
        if (hr == E_ACCESSDENIED)
        {
            Reset();
        }
        goto Error;
    }
    
    if (m_szResponseBuffer)
    {
        hr = CreateVector(
                pvarBody, 
                (const BYTE*)m_szResponseBuffer,
                m_cbResponseBody);

        if (FAILED(hr))
            goto Error;
    }
    else
    {
        V_VT(pvarBody) = VT_EMPTY;
    }

Cleanup:
    SetErrorInfo(hr, dwLastError);
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorUnexpected:
    hr = E_UNEXPECTED;
    goto Error;

ErrorPending:
    hr = E_PENDING;
    goto Error;

Error:
    if (pvarBody)
        VariantClear(pvarBody);
    dwLastError = hr;
    goto Cleanup;
}


/*
 -  CXMLHTTP::CreateStreamOnResponse
 -
 *  Purpose:
 *      Create a Stream containing the Response data
 *
 *  Parameters:
 *      ppStm   IN/OUT      Stream
 *
 *  Errors:
 *      E_INVALIDARG
 *      E_OUTOFMEMORY
 */

HRESULT
CXMLHTTP::CreateStreamOnResponse(IStream ** ppStm)
{
    HRESULT         hr = S_OK;
    ULONG           cbWritten;
    LARGE_INTEGER   liReset = { 0 };

    if (!ppStm)
        goto ErrorInvalidArg;

    *ppStm = NULL;

	hr = CreateStreamOnHGlobal(NULL, TRUE, ppStm);

    if (FAILED(hr))
        goto ErrorOutOfMemory;

    hr = (*ppStm)->Write(m_szResponseBuffer, m_cbResponseBuffer, &cbWritten);

    if (FAILED(hr))
        goto Error;

    hr = (*ppStm)->Seek(liReset, STREAM_SEEK_SET, NULL);

    if (FAILED(hr))
        goto Error;

Cleanup:
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

Error:
    if (ppStm)
        SafeRelease(*ppStm);
    goto Cleanup;
}


/*
 -  CXMLHTTP::get_responseText
 -
 *  Purpose:
 *      Retrieve the XML response body as a BSTR
 *
 *  Parameters:
 *      pbstrBody   OUT     XML response as a BSTR
 *
 *  Errors:
 *      E_INVALIDARG
 *      E_OUTOFMEMORY
 *      E_UNEXPECTED
 *      E_PENDING
 */

STDMETHODIMP
CXMLHTTP::get_responseText(BSTR * pbstrBody)
{
    HRESULT         hr = S_OK;
    DWORD           dwLastError = 0;
    IStream *       pEncStream = NULL;
    IStream *       pStm = NULL;
    WCHAR *         wszBuffer = NULL;
    ULONG           cbRead;

    // Validate parameter
    if (!pbstrBody)
        goto ErrorInvalidArg;

    // Validate state
    if (m_eState != RESPONSE)
    {
        // Look for a premature call in asynchronous mode
        if (m_fAsync)
            goto ErrorPending;
        goto ErrorUnexpected;
    }

    // Last-minute (redirect) access check - reset the object if
    // access is not allowed
    hr = AccessAllowed();

    if (FAILED(hr))
    {
        if (hr == E_ACCESSDENIED)
        {
            Reset();
        }
        goto Error;
    }

    hr = CreateStreamOnResponse(&pStm);
    
    if (FAILED(hr))
        goto Error;

    // Use the EncodingStream XML object to convert the text to Unicode
    if (m_cbResponseBody > 0)
    {
        pEncStream = EncodingStream::newEncodingStream(pStm);

        if (!pEncStream)
            goto ErrorOutOfMemory;

        hr = ReadFromStream((char **)&wszBuffer, &cbRead, pEncStream);

        if (FAILED(hr))
            goto Error;

        wszBuffer[cbRead/sizeof(WCHAR)] = 0;

        *pbstrBody = SysAllocString(wszBuffer);
    }
    else
    {
        *pbstrBody = SysAllocString(L"");
    }

    if (NULL == *pbstrBody)
        hr = E_OUTOFMEMORY;

Cleanup:
    if (wszBuffer)
        delete [] wszBuffer;
    SafeRelease(pEncStream);
    SafeRelease(pStm);
    SetErrorInfo(hr, dwLastError);
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorUnexpected:
    hr = E_UNEXPECTED;
    goto Error;

ErrorPending:
    hr = E_PENDING;
    goto Error;

Error:
    dwLastError = hr;
    goto Cleanup;
}


/*
 -  CXMLHTTP::get_responseStream
 -
 *  Purpose:
 *      Retrieve the XML response body as a IStream
 *
 *  Parameters:
 *      pvarBody    OUT     XML response as an IStream | EMPTY
 *
 *  Errors:
 *      E_INVALIDARG
 *      E_UNEXPECTED
 *      E_PENDING
 */

STDMETHODIMP
CXMLHTTP::get_responseStream(VARIANT * pvarBody)
{
    HRESULT     hr = S_OK;
    DWORD       dwLastError = 0;
    IStream *   pStm = NULL;

    // Validate parameter
    if (!pvarBody)
        goto ErrorInvalidArg;

    // Validate state
    if (m_eState != RESPONSE)
    {
        // Look for a premature call in asynchronous mode
        if (m_fAsync)
            goto ErrorPending;
        goto ErrorUnexpected;
    }

    VariantInit(pvarBody);

    // Last-minute (redirect) access check - reset the object if
    // access is not allowed
    hr = AccessAllowed();

    if (FAILED(hr))
    {
        if (hr == E_ACCESSDENIED)
        {
            Reset();
        }
        goto Error;
    }

    hr = CreateStreamOnResponse(&pStm);
    
    if (FAILED(hr))
        goto Error;

    V_VT(pvarBody) = VT_STREAM;
    pvarBody->punkVal = pStm;

Cleanup:
    SetErrorInfo(hr, dwLastError);
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

ErrorUnexpected:
    hr = E_UNEXPECTED;
    goto Error;

ErrorPending:
    hr = E_PENDING;
    goto Error;

Error:
    if (pvarBody)
        VariantClear(pvarBody);
    dwLastError = hr;
    goto Cleanup;
}

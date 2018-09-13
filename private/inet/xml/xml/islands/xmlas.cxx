/*++

Copyright (c) 1998 - 1999 Microsoft Corporation. All rights reserved.
Module:
    xmlas.cxx

Abstract:
    Implements CXMLScriptEngine, the ActiveScripting host for the XML parser

Author:
    Simon Bernstein (simonb)

History:
    02-26-1998 Created (simonb)
    
--*/


/*
CLSID for the engine:

{989D1DC0-B162-11d1-B6EC-D27DDCF9A923}

DEFINE_GUID(CLSID_XMLScriptEngine, 
0x989d1dc0, 0xb162, 0x11d1, 0xb6, 0xec, 0xd2, 0x7d, 0xdc, 0xf9, 0xa9, 0x23);

*/

#include "core.hxx"
#pragma hdrstop
#include "xmldocument.h"
#include "engine.h"
#include "xmlas.hxx"
#include "xml/tokenizer/net/url.hxx"
#include "xml/tokenizer/parser/xmlparser.hxx"

// Useful function
inline ULONG ReleaseInterface(LPVOID *ppVoid)
{
    ULONG uRet = 0;

    if (NULL != *ppVoid)
    {
        uRet = ((IUnknown *)(*ppVoid))->Release();
        *ppVoid = NULL;
    }

    return uRet;
}

extern WCHAR * StringDup(const WCHAR * s);

// Class factory helper

LPUNKNOWN CXMLScriptEngineConstruct()
{
    HRESULT hr = S_OK;

    CXMLScriptEngine *pEng = new_ne CXMLScriptEngine(&hr);

    if (FAILED(hr))
    {
        delete pEng;
        pEng = NULL;
    }

    return (LPUNKNOWN) (IActiveScript *) pEng;
}

/////////////////////////////////////////////////////////////////////
// CXMLScriptEngine Implementation

CXMLScriptEngine::CXMLScriptEngine(HRESULT *phr)
{
    m_pwszSrcAttrib = NULL;
    m_cRef = 1;
    m_pScriptSite = NULL;
    m_pDocument = NULL;
    m_ssenumState = SCRIPTSTATE_UNINITIALIZED;
    m_fInitNewCalled = false;
    m_dwCurrentSafety = 0;

    // 
    Assert (NULL != phr);

    if (phr)
    {
        *phr = S_OK;
    }
} // CXMLScriptEngine::CXMLScriptEngine


CXMLScriptEngine::~CXMLScriptEngine()
{
    // Destructor
    Assert(0 == m_cRef);

    Assert(SCRIPTSTATE_CLOSED == m_ssenumState);

    delete m_pwszSrcAttrib;

    ReleaseInterface((LPVOID *)&m_pDocument);
    ReleaseInterface((LPVOID *)&m_pScriptSite);
}  // CXMLScriptEngine::~CXMLScriptEngine


IHTMLElement*
CXMLScriptEngine::GetMyScriptElement()
{
    HRESULT hr;
    IHTMLElement *pElem = NULL;
    IDispatch *pDisp = NULL;
    IHTMLElementCollection *pElemCollection = NULL;
    VARIANT varIndex, varDummy;
    long iNumItems, iCurrentElem;

    BSTR bstrAttribName = SysAllocString(ATTRIBUTENAME);

    // Make sure the allocation succeeded
    if (NULL == bstrAttribName)
    {
        Assert(FALSE && "Couldn't allocate BSTR");
        return NULL;
    }

    hr = m_pDocument->get_scripts(&pElemCollection);

    Assert(SUCCEEDED(hr) && NULL != pElemCollection);

    if (FAILED(hr) || NULL == pElemCollection)
        return NULL;

    if (SUCCEEDED(hr = pElemCollection->get_length(&iNumItems)))
    {
        // Start at 0 every time, in case a block of script has been added programmatically
        bool fElemFound = false;
        bool fElementHasCorrectLanguage = false;

        varIndex.vt = VT_I4;
        varIndex.lVal = 0;

        VariantInit(&varDummy);

        while (varIndex.lVal < iNumItems && SUCCEEDED(hr) && !fElemFound)
        {
            // Sometimes we get back S_OK, but pDisp is NULL
            if (SUCCEEDED(hr = pElemCollection->item(varIndex, varDummy, &pDisp)) && NULL != pDisp)
            {
                if (SUCCEEDED(pDisp->QueryInterface(IID_IHTMLElement, (LPVOID *) &pElem)))
                {
                    BSTR bstrLanguage = NULL; 

                    pElem->get_language(&bstrLanguage);

                    if (NULL != bstrLanguage)
                    {
                        fElementHasCorrectLanguage = (0 == StrCmpIW(bstrLanguage, SCRIPTLANGUAGE));
                    }
                    else // They may have specified "type=text/xml"
                    {
                        // Go and get the type of the element
                        IHTMLScriptElement *pScriptElem;
                        hr = pElem->QueryInterface(IID_IHTMLScriptElement, (LPVOID *)&pScriptElem);

                        Assert(SUCCEEDED(hr));
                        if (NULL != pScriptElem)
                        {
                            pScriptElem->get_type(&bstrLanguage);
                            fElementHasCorrectLanguage = ((NULL != bstrLanguage) && (0 == StrCmpIW(bstrLanguage, TYPESCRIPTLANGUAGE)));
                            pScriptElem->Release();
                        }
                    }

                    // Is it the right language ? 
                    if (fElementHasCorrectLanguage)
                    {
                        // Does it have our expando property ?
                        VARIANT varValue;

                        if (SUCCEEDED(hr = pElem->getAttribute(bstrAttribName, TRUE, &varValue)))
                        {
                            fElemFound = (VT_NULL == V_VT(&varValue));
                            VariantClear(&varValue);
                        }

                    }

                    SysFreeString(bstrLanguage);
                }
                ReleaseInterface((LPVOID *)&pDisp);

                if (!fElemFound)
                {
                    (varIndex.lVal)++;
                    ReleaseInterface((LPVOID *) &pElem);
                    fElementHasCorrectLanguage = false;
                }
            }
        }
    }

cleanup:
    ReleaseInterface((LPVOID *)&pElemCollection);
    SysFreeString(bstrAttribName);

#ifdef _DEBUG
    if (NULL == pElem)
        TRACE(TEXT("CXMLScriptEngine::GetMyScriptElement: Couldn't find correct SCRIPT tag\n"));
#endif
    
    return pElem;
}

#ifndef GETMEMBER_ABSOLUTE
#define GETMEMBER_ABSOLUTE 4
#endif

extern String* _getBaseURLFromSite(IUnknown* pSite);

HRESULT
CXMLScriptEngine::GetSrcAttrib(IHTMLElement *pElem)
{
    HRESULT hr;
    IHTMLScriptElement *pSE;
    BSTR bstrSrcAttrib  = NULL;
    
    if (SUCCEEDED(hr = pElem->QueryInterface(IID_IHTMLScriptElement, (LPVOID *)&pSE)))
    {
        hr = pSE->get_src(&bstrSrcAttrib);
        pSE->Release();

        // Do we even have a SRC attribute
        if (SUCCEEDED(hr))
        {
            if (NULL != bstrSrcAttrib)
            {
                // In IE4 GETMEMBER_ABSOLUTE is ignored, use the _getBaseURLFromSite function
                // which correctly finds the BASE tag now, even under IE4 so that we
                // can resolve the SRC attribute correctly.
                TRY
                {
                    String* pBaseURL = _getBaseURLFromSite(m_pDocument);
                    if (pBaseURL)
                    {
                        URL url;
                        hr = url.set(bstrSrcAttrib, pBaseURL->getWCHARPtr());
                        if (SUCCEEDED(hr))
                        {
                            m_pwszSrcAttrib = StringDup(url.getResolved());
                            if (m_pwszSrcAttrib == NULL)
                                hr = E_OUTOFMEMORY;
                        }
                    }
                    else
                    {
                        // If we can't get the base URL then we can't
                        // resolve the SRC attribute.  This should never happen
                        // because the HTMLDocuemnt::get_url should always
                        // return something.
                        hr = E_FAIL;
                    }
                }
                CATCH
                {
                }
                ENDTRY

            }
            else
            {
                // Everything worked, there just isn't a SRC attribute
                hr = S_FALSE;
            }
            SysFreeString(bstrSrcAttrib);
        }
        else
        {
            Assert (FALSE && "Can't get SRC attribute from script tag");
        }

    }
    else
    {
        Assert(FALSE && "Can't get IHTMLScriptElement, can't get SRC");
    }

    return hr;

}


HRESULT 
CXMLScriptEngine::RaiseScriptError(
    ULONG   iLineNumber,
    LONG    iCharNumber,
    LPCWSTR pcwszDescription)
{
    HRESULT hr;

    CActiveScriptError *pASE = new_ne CActiveScriptError(iLineNumber, iCharNumber, pcwszDescription);
    
    if (NULL != pASE)
    {
        Assert(m_pScriptSite && "NULL Script site!");
        hr = m_pScriptSite->OnScriptError((IActiveScriptError *)pASE);
        ((IActiveScriptError *)pASE)->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


extern HRESULT UrlOpenAllowed(LPCWSTR pwszUrl, LPCWSTR pwszBaseUrl, BOOL fDTD);

BOOL 
CXMLScriptEngine::IsSecure(
    IHTMLElement *pElem)
{
    BOOL fRet = FALSE;
    BSTR bstrDocUrl = NULL;
    HRESULT hr;

    if (NULL == pElem)
        return TRUE;

    hr = GetSrcAttrib(pElem); // still get srcattr even when 0 == m_dwCurrentSafety

    Assert (SUCCEEDED(hr) && "Error getting SRC attribute - security check will fail");

    fRet = SUCCEEDED(hr); // S_FALSE is success, and is a valid response from GetSrcAttrib

    if (NULL == m_pwszSrcAttrib || 0 == m_dwCurrentSafety)
    {
        return fRet;
    }

    if (SUCCEEDED(hr = m_pDocument->get_URL(&bstrDocUrl)))
    {
        hr = UrlOpenAllowed(m_pwszSrcAttrib, bstrDocUrl, FALSE);
        fRet = SUCCEEDED(hr);
    }
    SysFreeString(bstrDocUrl);

    return fRet;

} // CXMLScriptEngine::IsSecure


HRESULT 
CXMLScriptEngine::AttachToXMLParser(
    IXMLDOMDocument **ppDoc)
{
    HRESULT hr;
    IObjectWithSite *pSite = NULL;
    IObjectSafety *pSafety = NULL;

    Assert(NULL != ppDoc);
    *ppDoc = NULL;

    // Try to create an instance of the class
    hr = CreateDOMDocument(IID_IXMLDOMDocument, (LPVOID *)ppDoc);

    Assert(SUCCEEDED(hr) && "CXMLScriptEngine::AttachToXMLParser: Couldn't create parser");

    if (FAILED(hr))
        return hr;

    // First we need to tell the document about our site
    hr = (*ppDoc)->QueryInterface(IID_IObjectWithSite, (LPVOID *)&pSite);

    Assert(SUCCEEDED(hr) && "Failed to get IObjectWithSite");
    
    if (FAILED(hr))
        goto error_exit;

    hr = pSite->SetSite((IUnknown *)m_pScriptSite);

    // Now we tell it what our safety options are
    hr = (*ppDoc)->QueryInterface(IID_IObjectSafety, (LPVOID *)&pSafety);

    Assert(SUCCEEDED(hr) && "Failed to get IObjectSafety");

    if (FAILED(hr))
        goto error_exit;

    hr = pSafety->SetInterfaceSafetyOptions(IID_IUnknown, m_dwCurrentSafety, m_dwCurrentSafety);

    Assert (SUCCEEDED(hr) && "Unable to set safety options");

error_exit:
    if (FAILED(hr))
        ReleaseInterface((LPVOID *)ppDoc);
    ReleaseInterface((LPVOID*)&pSite);
    ReleaseInterface((LPVOID*)&pSafety);
    
    return hr;    
}  // CXMLScriptEngine::AttachToXMLParser


// IUnknown implementation

HRESULT
CXMLScriptEngine::QueryInterface(
    REFIID riid,
    void  **ppvObject)
{
    HRESULT hr = S_OK;

    if (ppvObject)
        *ppvObject = NULL;
    else
        return E_POINTER;


    if (InlineIsEqualGUID(riid, IID_IUnknown))
    {
        // All this casting is just to make the compiler happy
        IUnknown *pThis = (IUnknown *)(IActiveScript *)this;
        *ppvObject = (LPVOID *)pThis;
    }
    else if (InlineIsEqualGUID(riid, IID_IActiveScript))
    {
        IActiveScript *pThis = this;
        *ppvObject = (LPVOID *)pThis;
    }
    else if (InlineIsEqualGUID(riid, IID_IActiveScriptParse))
    {
        IActiveScriptParse *pThis = this;
        *ppvObject = (LPVOID *)pThis;
    }
    else if (InlineIsEqualGUID(riid, IID_IObjectSafety))
    {
        IObjectSafety *pThis = this;
        *ppvObject = (LPVOID *)pThis;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    if (NULL != *ppvObject)
        ((IUnknown *) *ppvObject)->AddRef();

    return hr;
}  // CXMLScriptEngine::QueryInterface


ULONG 
CXMLScriptEngine::AddRef()
{
    InterlockedIncrement((LPLONG)&m_cRef);
    return m_cRef;
} // CXMLScriptEngine::AddRef


ULONG 
CXMLScriptEngine::Release()
{
    Assert (m_cRef > 0);

    if (0 == InterlockedDecrement((LPLONG)&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
} // CXMLScriptEngine::Release

// IObjectSafety members

HRESULT 
CXMLScriptEngine::GetInterfaceSafetyOptions( 
    REFIID riid,
    DWORD *pdwSupportedOptions,
    DWORD *pdwEnabledOptions)
{
    IUnknown *pUnk = NULL;
    *pdwSupportedOptions = 0;
    *pdwEnabledOptions = 0;

    // Check that we support the interface
    HRESULT hr = QueryInterface(riid, (LPVOID *) &pUnk);

    if (SUCCEEDED(hr))
    {
        // Let go of the object
        pUnk->Release(); 
        // We support both options for all interfaces we support
        *pdwSupportedOptions = *pdwEnabledOptions = g_dwInterfaceSecurityOptions;

        hr = S_OK;
    }

    return hr;
}


HRESULT 
CXMLScriptEngine::SetInterfaceSafetyOptions( 
    REFIID riid,
    DWORD dwOptionSetMask,
    DWORD dwEnabledOptions)
{
    IUnknown *pUnk = NULL;

    // Check that we support the interface
    HRESULT hr = QueryInterface(riid, (LPVOID *) &pUnk);

    if (SUCCEEDED(hr))
    {
        // Let go of the object
        pUnk->Release(); 

        // Since we support all options, we just return S_OK, assuming we support 
        // the interface 

        // Do we support the bits we are being asked to set ?
        if (!(dwOptionSetMask & ~g_dwInterfaceSecurityOptions)) 
        {
            // All the flags we are being asked to set are supported, so 
            // now make sure we aren't turning off something we do support

            // Ignore any bits we support which the mask isn't interested in
            dwEnabledOptions &= g_dwInterfaceSecurityOptions;  

            if ((dwEnabledOptions & dwOptionSetMask) == dwOptionSetMask)
            {
                hr = S_OK;
                m_dwCurrentSafety = dwEnabledOptions;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else // dwOptionSetMask & ~dwSupportedBits
        {
            // We are being asked to set bits we don't support
            hr = E_FAIL;
        }
    }
    return hr;
}

// IActiveScript members

HRESULT 
CXMLScriptEngine::SetScriptSite( 
    IActiveScriptSite *pass)
{
    // Make sure we don't already have a script site - this might 
    // not be necessary
    Assert(NULL == m_pScriptSite && "non-NULL script site");
    Assert(NULL == m_pDocument && "non-NULL Document");

    HRESULT hr = E_FAIL;

    if (NULL != pass)
    {

        Assert (SCRIPTSTATE_UNINITIALIZED == m_ssenumState);

        pass->AddRef();
        m_pScriptSite = pass;

        if (m_fInitNewCalled)
        {
            m_ssenumState = SCRIPTSTATE_INITIALIZED;
            m_fInitNewCalled = false;
        }
    }

    return S_OK;
} // CXMLScriptEngine::SetScriptSite


HRESULT 
CXMLScriptEngine::GetScriptSite( 
     REFIID riid,
     void **ppvObject)
{
    if (NULL != ppvObject)
    {
        *ppvObject = NULL;

        if (NULL != m_pScriptSite)
            return m_pScriptSite->QueryInterface(riid, ppvObject);
        else
            return S_FALSE; // Indicates we don't have a site set yet
        
    }

    return E_POINTER;

}  // CXMLScriptEngine::GetScriptSite


HRESULT 
CXMLScriptEngine::SetScriptState( 
    SCRIPTSTATE ss)
{
    m_ssenumState = ss;
    return S_OK;
}  // CXMLScriptEngine::SetScriptState


HRESULT 
CXMLScriptEngine::GetScriptState( 
    SCRIPTSTATE *pssState)
{
    HRESULT hr = E_POINTER;

    if (NULL != pssState)
    {
        *pssState = m_ssenumState;
        hr = S_OK;
    }

    return hr;
}  // CXMLScriptEngine::GetScriptState


HRESULT 
CXMLScriptEngine::Close()
{
    m_ssenumState = SCRIPTSTATE_CLOSED;
    return S_OK;
}  // CXMLScriptEngine::Close



HRESULT 
CXMLScriptEngine::AddNamedItem( 
    LPCOLESTR pstrName,
    DWORD dwFlags)
{
#ifdef _DEBUG
    WCHAR rgwch[200];
    wsprintfW(rgwch, L"CXMLScriptEngine::AddNamedItem: %s (0x%lx)\n", pstrName, dwFlags);
    TRACEW(rgwch);
#endif

    HRESULT hr = S_OK;

    IUnknown *pUnkItem = NULL;
    IHTMLWindow2 *pWindow = NULL;
    IHTMLDocument2* pDocument = NULL;

    Assert(NULL != m_pScriptSite);
    
    // Go from the script site to the window
    if (SUCCEEDED(hr = m_pScriptSite->GetItemInfo(pstrName, SCRIPTINFO_IUNKNOWN, &pUnkItem, NULL)))
    {
        hr = pUnkItem->QueryInterface(IID_IHTMLWindow2, (LPVOID *)&pWindow);

        if (FAILED(hr))
            goto error_exit;

        // From the window to the document
        hr = pWindow->get_document(&pDocument);
        if (SUCCEEDED(hr))
        {
            ReleaseInterface((LPVOID *)&m_pDocument);
            m_pDocument = pDocument;
        }
    }

error_exit:

    // Cleanup
    ReleaseInterface((LPVOID *)&pUnkItem);
    ReleaseInterface((LPVOID *)&pWindow);
    // pDocument has been transferred to m_pDocument.
    
    return hr;
}  // CXMLScriptEngine::AddNamedItem



HRESULT 
CXMLScriptEngine::AddTypeLib( 
    REFGUID rguidTypeLib,
    DWORD dwMajor,
    DWORD dwMinor,
    DWORD dwFlags)
{
    Assert(FALSE);
    return E_NOTIMPL;
}  // CXMLScriptEngine::AddTypeLib



HRESULT 
CXMLScriptEngine::GetScriptDispatch( 
    LPCOLESTR pstrItemName,
    IDispatch **ppdisp)
{
    if (NULL != ppdisp)
    {
        *ppdisp = NULL;
        return S_FALSE;
    }
    else
    {
        return E_POINTER;
    }
}  // CXMLScriptEngine::GetScriptDispatch



HRESULT 
CXMLScriptEngine::GetCurrentScriptThreadID( 
    SCRIPTTHREADID *pstidThread)
{
    Assert(FALSE);
    return E_NOTIMPL;
}  // CXMLScriptEngine::GetCurrentScriptThreadID



HRESULT 
CXMLScriptEngine::GetScriptThreadID( 
    DWORD dwWin32ThreadId,
    SCRIPTTHREADID *pstidThread)
{
    Assert(FALSE);
    return E_NOTIMPL;
}  // CXMLScriptEngine::GetScriptThreadID



HRESULT 
CXMLScriptEngine::GetScriptThreadState( 
    SCRIPTTHREADID stidThread,
    SCRIPTTHREADSTATE *pstsState)
{
    Assert(FALSE);
    return E_NOTIMPL;
}  // CXMLScriptEngine::GetScriptThreadState


HRESULT 
CXMLScriptEngine::InterruptScriptThread( 
    SCRIPTTHREADID stidThread,
    const EXCEPINFO *pexcepinfo,
    DWORD dwFlags)
{
    Assert(FALSE);
    return E_NOTIMPL;
}  // CXMLScriptEngine::InterruptScriptThread


HRESULT 
CXMLScriptEngine::Clone( 
    IActiveScript **ppscript)
{
    Assert(FALSE);
    return E_NOTIMPL;
} // CXMLScriptEngine::Clone


// IActiveScriptParse implementation

HRESULT 
CXMLScriptEngine::InitNew()
{
#ifdef _DEBUG
    TRACE(TEXT("CXMLScriptEngine::InitNew\n"));
#endif
    // Change the state if a script site has been set
    m_fInitNewCalled = true;

    return S_OK;
}  // CXMLScriptEngine::InitNew


HRESULT 
CXMLScriptEngine::AddScriptlet( 
    LPCOLESTR pstrDefaultName,
    LPCOLESTR pstrCode,
    LPCOLESTR pstrItemName,
    LPCOLESTR pstrSubItemName,
    LPCOLESTR pstrEventName,
    LPCOLESTR pstrDelimiter,
    DWORD dwSourceContextCookie,
    ULONG ulStartingLineNumber,
    DWORD dwFlags,
    BSTR *pbstrName,
    EXCEPINFO *pexcepinfo)
{
/*
    Probably have a default language issue, like so:

    <SCRIPT language="xml" id="foo">
        <blah/>
    </SCRIPT>

    <INPUT type=button onclick="doit()">

    Whooops !  Because the XML script tag comes first, Trident now assumes that
    any script where the language is not specified is XML.  Definitely not what 
    was intended !

*/
    HRESULT hr;

    String *pstr = Resources::FormatMessage(XMLISLANDS_NOSCRIPTLETS, NULL);
    ATCHAR *patchErr = pstr->toCharArrayZ();
    
    patchErr->AddRef();
    hr = RaiseScriptError(ulStartingLineNumber, 0, patchErr->getData());
    patchErr->Release();

    return hr;

}  // CXMLScriptEngine::AddScriptlet


HRESULT 
CXMLScriptEngine::ParseScriptText( 
    LPCOLESTR pstrCode,
    LPCOLESTR pstrItemName,
    IUnknown *punkContext,
    LPCOLESTR pstrDelimiter,
    DWORD dwSourceContextCookie,
    ULONG ulStartingLineNumber,
    DWORD dwFlags,
    VARIANT *pvarResult,
    EXCEPINFO *pexcepinfo)
{
    STACK_ENTRY;

    HRESULT hr = E_FAIL;
    IHTMLElement *pElem = NULL;
    IXMLDOMDocument *pXMLDoc = NULL;

    // We should never be asked to parse if we aren't in a started state
    Assert (SCRIPTSTATE_STARTED == m_ssenumState || SCRIPTSTATE_CONNECTED == m_ssenumState);

    // We can't handle the island reload case when state = SCRIPTSTATE_CONNECTED because
    // it is out of sequence and so we can't find the corresponding HTML element any more.
    if (SCRIPTSTATE_STARTED == m_ssenumState)
    {
        // Find the element to set the expando property on
        if (pElem = GetMyScriptElement())
        {
            VARIANT varDoc;
            Assert (NULL != pElem && "Unable to find script element");

            //
            // Create an document
            // 
            hr = AttachToXMLParser(&pXMLDoc);

#ifdef RENTAL_MODEL
            Model model(_EnsureTls.getTlsData(), Rental);
#endif

            Assert(SUCCEEDED(hr) && "Unable to create XML Document");

            // attach the doc in the element's XMLDocument expando property
            // (even if failure - we'll attach NULL)
            varDoc.vt = VT_DISPATCH;
            varDoc.pdispVal = SUCCEEDED(hr) ? pXMLDoc : NULL;

            // Use a new variable so we don't lose the HRESULT in the case where AttachToXMLParser fails
            HRESULT hr2 = SetExpandoProperty(pElem, String::newString("XMLDocument"), &varDoc);
            
            if (FAILED(hr) || NULL == pXMLDoc)
            {
                goto cleanup;
            }
            
            // As ugly as this seems, it generates less code than duplicating the call 
            // to SetExpandoProperty above
            
            if (FAILED(hr2))
            {
                hr = hr2;
                goto cleanup;
            }

            
            if (IsSecure(pElem))
            {
                long iReadyState = 0;
                Document *pDoc;

                if (SUCCEEDED(hr = pXMLDoc->QueryInterface(IID_Document, (LPVOID *)&pDoc)))
                {
                    Assert (SUCCEEDED(hr) && "Unable to get Document");
                
                    TRY
                    { 
                        String * pString = String::newString(pstrCode);
                        pString = pString->trim(); 
                        if (pString->length() > 0 )  // ignore empty data islands.
                        {
                            // Pass m_pwszSrcAttrib as currentURL to the parser,
                            // so that the parser can use it to resolve relative DTD/Schema/entity paths
                            // see bug 63092 for details
                            IXMLParser * pXMLParser = NULL;
                            hr = pDoc->QueryInterface(IID_IXMLParser, (void **)&pXMLParser);
                            if (SUCCEEDED(hr))
                            {
                                XMLParser * pParser = NULL;
                                // Get a private parser interface
                                hr = pXMLParser->QueryInterface(IID_Parser, (void**)&pParser);
                                pXMLParser->Release();

                                if (SUCCEEDED(hr))
                                {
                                    VARIANT_BOOL result;
                                    pParser->SetCurrentURL(m_pwszSrcAttrib);
                                    pParser->Release();
                                    // Have to call the wrapper instead of the document directly
                                    // because it's the wrapper that handles dom locks
                                    pXMLDoc->loadXML((WCHAR*)pstrCode, &result);
                                }
                            }
                        }
                    }
                    CATCH
                    {
                    }
                    ENDTRY

                    // At this point, ParseScriptText can be considered to have succeeded
                    // Any parse errors will need to be checked through the XML object model
                    hr = S_OK;
                }
            } // Tag is secure
            else // Tag is not secure
            {
                String *strErrorMessage = Resources::FormatSystemMessage(E_ACCESSDENIED);
                ATCHAR *patchErr = strErrorMessage->toCharArrayZ();

                patchErr->AddRef();
                hr = RaiseScriptError(ulStartingLineNumber, 0, patchErr->getData());
                patchErr->Release();

            }

        } // Found the SCRIPT tag to attach to 
    }
cleanup:
    ReleaseInterface((LPVOID *)&pXMLDoc);
    ReleaseInterface((LPVOID *)&pElem);

    return hr;

}  // CXMLScriptEngine::ParseScriptText


///////////////////////////////////////////////////////////////////////////////////////////////////

CActiveScriptError::CActiveScriptError(
    ULONG iLine,
    LONG iCharNumber,
    LPCWSTR pcwszDescription) : 
        m_iLineNumber(iLine), 
        m_iCharNumber(iCharNumber)
{
    m_pwszDescription = StringDup(pcwszDescription);
    Assert(m_pwszDescription && "Memory allocation failed.  Now what ?");
} // CActiveScriptError::CActiveScriptError

CActiveScriptError::~CActiveScriptError()
{
    delete m_pwszDescription;
} // CActiveScriptError::~CActiveScriptError

////////////////////////////////////////////////
// IActiveScriptError members
////////////////////////////////////////////////

HRESULT 
CActiveScriptError::GetExceptionInfo( 
    EXCEPINFO *pexcepinfo)
{
    HRESULT hr;

    if (pexcepinfo)
    {
        ZeroMemory(pexcepinfo, sizeof(EXCEPINFO));
        pexcepinfo->bstrDescription = SysAllocString(m_pwszDescription);
        hr = S_OK;
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;

} // CActiveScriptError::GetExceptionInfo

HRESULT 
CActiveScriptError::GetSourcePosition( 
    DWORD *pdwSourceContext,
    ULONG *pulLineNumber,
    LONG *plCharacterPosition)
{
    Assert(pdwSourceContext && "Bad parameter passed in");
    Assert(pulLineNumber && "Bad parameter passed in");
    Assert(plCharacterPosition && "Bad parameter passed in");

    if (NULL == pdwSourceContext) 
        return E_POINTER;

    if (NULL == pulLineNumber) 
        return E_POINTER;

    if (NULL == plCharacterPosition) 
        return E_POINTER;

    *pdwSourceContext = 0;
    *pulLineNumber = m_iLineNumber;
    *plCharacterPosition = m_iCharNumber;

    return S_OK;

} // CActiveScriptError::GetSourcePosition

HRESULT 
CActiveScriptError::GetSourceLineText( 
    BSTR *pbstrSourceLine)
{
    Assert (FALSE && "Not implemented");
    return E_NOTIMPL;
} // CActiveScriptError::GetSourceLineText

// end of file: xmlas.cxx

